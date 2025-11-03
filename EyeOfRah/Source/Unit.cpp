//---------------------------------------------------------------------------------------
// file:	Unit.cpp
// author:	Primary: Jason (80%), Secondary: Leonard (10%), Lebon (10%)
// email:	l.jason@digipen.edu, leonardjunyi.l@digipen.edu, h.xinyulebon@digipen.edu
//
// brief:	This file contains definitions for all things Unit related. The Unit Object
//			is defined here along with other helper functions for manipulating the
//			Unit Database.
//
// Copyright � 2025 DigiPen, All rights reserved.
//---------------------------------------------------------------------------------------
#include "pch.hpp"
#include "Unit.hpp"
#include "PresetWidgets.hpp"
#include "Item.hpp"
#include "document.h"
#include "filereadstream.h"
#include "GameStateManager.hpp"
#include "EntityBase.hpp"
#include "TraitManager.hpp"
#include "CombatSystem.hpp"
#include "Animation.hpp"

// Private Constants for use in Units only
namespace
{
	using namespace LegacyProcessing;
	LG_Color pSlotColorA = {80.f, 80.f, 80.f, 40.f};
	LG_Color pSlotColorB = {120.f, 120.f, 120.f, 40.f};
}

namespace EOREntities
{
	using namespace LegacyProcessing;
	std::vector<Unit> unitDatabase;

#pragma region UNIT CLASS
#pragma region CONSTRUCTORS
	// Default Constructor
	Unit::Unit() : EntityObject(EntityType::UNIT), hitPoints(0), attackPoints(0), xp(1), baseHitPoints(0), baseAttackPoints(0), equippedCharm()
	{
		for (int i = 0; i < 2; ++i)
		{
			hitPointGrowth[i] = 0;
			attackPointGrowth[i] = 0;
		}
		UpdateChildTransforms();
		attachedTeam = nullptr;
		teamSlot = -1;
		hasAttacked = false;
		hoverLerpCounter = 0.f;
	}

	// Parameterized Constructor
	Unit::Unit(std::string _name, int _hitPoints, int _attackPoints, int _tier, int _cost, int _skillCast, const short _hitPointGrowth[2], const short _attackPointGrowth[2], Transform _transform, const std::vector<Trait> &initialTraits)
			: EntityObject(EntityType::UNIT, _transform), hitPoints(_hitPoints), attackPoints(_attackPoints), xp(1), baseHitPoints(_hitPoints), baseAttackPoints(_attackPoints), equippedCharm()
	{
		for (int i = 0; i < 2; ++i)
		{
			hitPointGrowth[i] = _hitPointGrowth[i];
			attackPointGrowth[i] = _attackPointGrowth[i];
		}

		// Parent Members
		name = _name;
		tier = _tier;
		cost = _cost;
		skillCast = _skillCast;
		UpdateChildTransforms();
		attachedTeam = nullptr;
		teamSlot = -1;
		hasAttacked = isValid = showPlaceableSlot = isInHover = isBeingDragged = false;
		hoverLerpCounter = 0.f;
		for (const Trait &trait : initialTraits)
		{
			AddTrait(trait);
		}
	}

	// Copy Constructor
	Unit::Unit(const Unit &unit)
			: EntityObject(unit), teamSlot(unit.teamSlot), hitPoints(unit.hitPoints), attackPoints(unit.attackPoints),
				baseHitPoints(unit.baseHitPoints), baseAttackPoints(unit.baseAttackPoints), xp(unit.xp), equippedCharm(unit.equippedCharm)
	{
		for (int i = 0; i < 2; ++i)
		{
			hitPointGrowth[i] = unit.hitPointGrowth[i];
			attackPointGrowth[i] = unit.attackPointGrowth[i];
		}
		attachedTeam = unit.attachedTeam;
		UpdateChildTransforms();
		hasAttacked = unit.hasAttacked;
		isValid = unit.isValid;
		showBox = unit.showBox;
		skillCast = unit.skillCast;
		traits = unit.traits;
		// Non Copyable Members
		isInHover = isBeingDragged = showPlaceableSlot = false;
		hoverLerpCounter = 0.f;
	}

	// Copy Assignment
	Unit &Unit::operator=(const Unit &rhs)
	{
		if (this != &rhs)
		{
			EntityObject::operator=(rhs);

			for (int i = 0; i < 2; ++i)
			{
				hitPointGrowth[i] = rhs.hitPointGrowth[i];
				attackPointGrowth[i] = rhs.attackPointGrowth[i];
			}

			name = rhs.name;
			description = rhs.description;
			teamSlot = rhs.teamSlot;
			tier = rhs.tier;
			cost = rhs.cost;
			hitPoints = rhs.hitPoints;
			attackPoints = rhs.attackPoints;
			baseHitPoints = rhs.baseHitPoints;
			baseAttackPoints = rhs.baseAttackPoints;
			skillCast = rhs.skillCast;
			xp = rhs.xp;
			attachedTeam = rhs.attachedTeam;
			equippedCharm = rhs.equippedCharm;
			traits = rhs.traits;
			UpdateChildTransforms();
			hasAttacked = rhs.hasAttacked;
			isValid = rhs.isValid;
			showBox = rhs.showBox;

			// Non Copyable Members
			isInHover = isBeingDragged = showPlaceableSlot = false;
			hoverLerpCounter = 0.f;
		}
		return *this;
	}

	// Move Constructor
	Unit::Unit(Unit &&unit) noexcept
			: EntityObject(unit), teamSlot(unit.teamSlot), hitPoints(unit.hitPoints), attackPoints(unit.attackPoints),
				baseHitPoints(unit.baseHitPoints), baseAttackPoints(unit.baseAttackPoints), xp(unit.xp), equippedCharm(unit.equippedCharm)
	{
		skillCast = unit.skillCast;
		for (int i = 0; i < 2; ++i)
		{
			hitPointGrowth[i] = unit.hitPointGrowth[i];
			attackPointGrowth[i] = unit.attackPointGrowth[i];
		}

		attachedTeam = unit.attachedTeam;
		UpdateChildTransforms();
		hasAttacked = unit.hasAttacked;
		showBox = unit.showBox;
		traits = std::move(unit.traits);

		// Non Transferable Members
		isInHover = isBeingDragged = showPlaceableSlot = false;
		hoverLerpCounter = 0.f;

		// Reset the moved-from object
		unit.attachedTeam = nullptr;
		unit.teamSlot = -1;
		unit.isValid = false;
	}

	// Move Assignment
	Unit &Unit::operator=(Unit &&rhs) noexcept
	{
		if (this != &rhs)
		{
			// Move Base Class first
			EntityObject::operator=(rhs);

			// Transfer all member values
			for (int i = 0; i < 2; ++i)
			{
				hitPointGrowth[i] = rhs.hitPointGrowth[i];
				attackPointGrowth[i] = rhs.attackPointGrowth[i];
			}

			name = rhs.name;
			description = rhs.description;
			teamSlot = rhs.teamSlot;
			tier = rhs.tier;
			cost = rhs.cost;
			hitPoints = rhs.hitPoints;
			attackPoints = rhs.attackPoints;
			baseHitPoints = rhs.baseHitPoints;
			baseAttackPoints = rhs.baseAttackPoints;
			xp = rhs.xp;
			attachedTeam = rhs.attachedTeam;
			equippedCharm = rhs.equippedCharm;
			traits = std::move(rhs.traits);
			UpdateChildTransforms();
			hasAttacked = rhs.hasAttacked;
			isValid = rhs.isValid;
			showBox = rhs.showBox;
			skillCast = rhs.skillCast;

			// Non Transferable Members
			isInHover = isBeingDragged = showPlaceableSlot = false;
			hoverLerpCounter = 0.f;

			// Reset the moved-from object by calling its default constructor
			rhs.attachedTeam = nullptr;
			rhs.teamSlot = -1;
			rhs.isValid = false;
		}
		return *this;
	}

	Unit::~Unit()
	{
		if (!PlayerManager::Exists())
			return;
		if (PlayerManager::GetInstance()->GetPlayerRef().GetHUD().GetCurrentHoveredEntity() == this)
		{
			PlayerManager::GetInstance()->GetPlayerRef().GetHUD().HideEntityTooltip();
		}
	}
#pragma endregion

#pragma region SETTERS & GETTERS
	// SETTERS AND GETTERS METHODS
	void Unit::SetTeam(Team *team) { attachedTeam = team; }

	Team *Unit::GetTeam() { return attachedTeam; }

	void Unit::EquipItem(ItemID item_id)
	{
		if ((item_id >= ItemID::ITEM_COUNT) || (item_id <= ItemID::ID_NONE))
			return;

		equippedCharm.charmItem = &GetItemData()[static_cast<size_t>(item_id)];
		equippedCharm.charges = equippedCharm.charmItem->GetInitialCharges();
	}

	void Unit::UnequipItem() { equippedCharm = Charm(); }

	Charm &Unit::GetEquippedItem() { return equippedCharm; };

	void Unit::SetHasAttacked(bool attacked) { hasAttacked = attacked; }

	void Unit::SetPosition(float x, float y)
	{
		SetMainPosition(x, y);
		UpdateChildTransforms();
	}

	void Unit::SetRotation(float _rotation) { transform.rotation = _rotation; }

	void Unit::SetScale(float x, float y)
	{
		transform.scale.x = x;
		transform.scale.y = y;
		UpdatePSlotScale();
	}

	void Unit::SetTransform(Transform _transform)
	{
		transform = _transform;
		UpdateChildTransforms();
	}

	void Unit::SetHitPoints(int _hitPoints)
	{
		LG_Clamp(_hitPoints, 0, 99);
		hitPoints = _hitPoints;
	}
	int Unit::GetHitPoints() const { return hitPoints; }

	void Unit::SetAttackPoints(int _attackPoints)
	{
		LG_Clamp(_attackPoints, 0, 99);
		attackPoints = _attackPoints;
	}

	int Unit::GetAttackPoints() const
	{
		int damageBoost = 0;
		// inheritance will be better but got alot of pet type.
		if (name == "ra")
		{
			if (GameStateManager::GetInstance().dayCycle == DayCycle::DAY)
			{
				damageBoost += 4;
			}
		}
		else if (name == "isis")
		{
			if (GameStateManager::GetInstance().dayCycle == DayCycle::NIGHT)
			{
				damageBoost += 4;
			}
		}

		return attackPoints + damageBoost;
	}

	void Unit::SetBaseHitPoints(int _hitPoints)
	{
		LG_Clamp(_hitPoints, 0, 99);
		baseHitPoints = _hitPoints;
	}

	int Unit::GetBaseHitPoints() const { return baseHitPoints; }

	void Unit::SetBaseAttackPoints(int _attackPoints)
	{
		LG_Clamp(_attackPoints, 0, 99);
		baseAttackPoints = _attackPoints;
	}

	int Unit::GetBaseAttackPoints() const { return baseAttackPoints; }

	void Unit::SetXP(int _xp)
	{
		LG_Clamp(_xp, 0, 99);
		xp = _xp;
	}
	int Unit::GetXP() const
	{
		return xp;
	}

	int Unit::GetLevel() const
	{
		int level = (xp / 3) + 1;
		return level > 3 ? 3 : level;
	}

	void Unit::CastSkill(Team &ownTeam)
	{
		if (skillCast == -1 || castCount < skillCast)
		{

			if (name == "osiris")
			{
				for (Unit &unit : ownTeam.units)
				{
					// check unit is not myself, increase their max hp
					if (&unit != this)
					{

						unit.SetHitPoints(unit.GetHitPoints() + 3);
					}
				}
			}
			else if (name == "khonsu")
			{
				SetHitPoints(GetHitPoints() + 1); // lifesteal
			}

			if (skillCast > 0)
			{
				++castCount;
			}
		}
	}

#pragma endregion

#pragma region PRIVATE METHODS
	// Private Methods
	void Unit::UpdateChildTransforms()
	{
		for (Transform &childTransform : childTransforms)
		{
			childTransform.position.y = transform.position.y - (transform.scale.y * 0.5f);
			childTransform.rotation = 0;
			childTransform.scale = {48.f, 48.f};
		}
		childTransforms[0].position.x = transform.position.x - (transform.scale.x * 0.5f); // Attack Icon
		childTransforms[1].position.x = transform.position.x + (transform.scale.x * 0.5f); // Health Icon

		childTransforms[2].scale = {48.f, 48.f};
		childTransforms[2].position.y = transform.position.y + (transform.scale.y * 0.5f);
		childTransforms[2].position.x = transform.position.x + (transform.scale.x * 0.5f);

		UpdatePSlotScale();
	}

	int Unit::GetDamage() const
	{
		// Special Effects go here in the future
		return attackPoints;
	}

	void Unit::OnDeath()
	{
		if (hitPoints > 0)
		{
			return;
		}
		if (HasTrait(Trait::DEATH))
		{
			TraitManager::GetInstance().SpawnUnit(*this);
			return;
		}
		if (HasTrait(Trait::SOLAR))
		{
			// *this.Unit =
			// return;
		}
		animationCopies.push_back({transform, sprite});
		animators.push_back(&animationCopies.back().first);
		AudioManager::GetInstance().PlaySFX("singleUnitDeath");
		AEVec2 targetPosition{ transform.position.x, transform.position.y };
		targetPosition.y = transform.position.y;
		targetPosition.x = attachedTeam == &CombatSystem::GetPlayerTeam() ? transform.position.x - 1000.f : transform.position.x + 1000.f;
		animators.back().SetAnimation(targetPosition, 0.5f, 100.f, AnimationController::Animation::ARC_OUT);
		Reset();
	}

	void Unit::LevelUnit(Unit &unit)
	{
		// Check if unit is not valid
		if (!unit.isValid)
			return;

		// Check if it is a duplicate unit
		if (unit.name != name)
		{
			return;
		}

		int prevLevel = GetLevel();
		SetXP(GetXP() + unit.GetLevel());

		// If previous level is lower than current level, means Unit levelled up.
		if (prevLevel < GetLevel())
		{
			int growthIndex = LGCY_CP::LG_Clamp(GetLevel() - 2, 0, 1);
			baseHitPoints += hitPointGrowth[growthIndex];
			baseAttackPoints += attackPointGrowth[growthIndex];
			hitPoints += hitPointGrowth[growthIndex];
			attackPoints += attackPointGrowth[growthIndex];
			cost = tier * GetLevel();
		}
	}

	void Unit::OnHoverEnter()
	{
		if (hoverLerpCounter < 1.f)
		{
			hoverLerpCounter += static_cast<float>(AEFrameRateControllerGetFrameTime());
			hoverLerpCounter = LGCY_CP::LG_Clamp(hoverLerpCounter, 0.f, 0.3f);
		}
		Transform hoverTransform = transform;
		hoverTransform.scale.x = LGCY_CP::LG_Lerp(transform.scale.x, transform.scale.x + 50.f, hoverLerpCounter);
		hoverTransform.scale.y = LGCY_CP::LG_Lerp(transform.scale.y, transform.scale.y + 50.f, hoverLerpCounter);
		DrawSprite(hoverTransform, sprite);

		if (PlayerManager::GetInstance()->GetPlayerRef().GetHUD().GetCurrentHoveredEntity() != this && isValid)
		{
			PlayerManager::GetInstance()->GetPlayerRef().GetHUD().ShowEntityTooltip(*this);
		}
	}

	void Unit::OnHoverExit(bool flipped = false)
	{
		if (hoverLerpCounter > 0.f)
		{
			hoverLerpCounter -= static_cast<float>(AEFrameRateControllerGetFrameTime());
			hoverLerpCounter = LGCY_CP::LG_Clamp(hoverLerpCounter, 0.f, 0.3f);
			Transform hoverTransform = transform;
			hoverTransform.scale.x = LGCY_CP::LG_Lerp(transform.scale.x, transform.scale.x + 50.f, hoverLerpCounter);
			hoverTransform.scale.y = LGCY_CP::LG_Lerp(transform.scale.y, transform.scale.y + 50.f, hoverLerpCounter);
			DrawSprite(hoverTransform, sprite);
		}
		else
		{
			DrawSprite(transform, sprite, flipped);
		}

		if (PlayerManager::GetInstance()->GetPlayerRef().GetHUD().GetCurrentHoveredEntity() == this)
		{
			PlayerManager::GetInstance()->GetPlayerRef().GetHUD().HideEntityTooltip();
		}
	}

#pragma endregion

#pragma region PUBLIC METHODS
	// Public Methods
	void Unit::RestoreState()
	{
		hitPoints = baseHitPoints;
		attackPoints = baseAttackPoints;
		hasAttacked = false;
	}

	void Unit::Reset()
	{
		PlayerManager::GetInstance()->GetPlayerRef().GetHUD().HideEntityTooltip();
		// Preserve data that do not need to be resetted
		Team *localAttachedTeam = attachedTeam;
		Charm localCharm = equippedCharm;
		int localTeamSlot = teamSlot;
		Transform localTransform = transform;

		*this = Unit();

		// Pops preserved data
		transform = localTransform;
		teamSlot = localTeamSlot;
		equippedCharm = localCharm;
		attachedTeam = localAttachedTeam;
		UpdateChildTransforms();
	}

	void Unit::Hurt(int damage, bool checkDeath)
	{
		// Pre-Damage Item Effects
		if (equippedCharm.charmItem)
		{
			switch (equippedCharm.charmItem->GetItemID())
			{
			case ItemID::ID_HORUSEYE:
				UnequipItem();
				return;
			default:
				break;
			}
		}

		// Water trait 30% dodge chance
		if (HasTrait(Trait::WATER) && TraitManager::GetInstance().GetTraitLevel(Trait::WATER) > 0 && GameStateManager::GetInstance().dayCycle == DayCycle::DAY)
		{
			if (rand() % 100 < 30)
			{
				return; // Exit without taking damage
			}
		}

		hitPoints -= damage;

		if (checkDeath)
			CheckDeath();
	}

	void Unit::CheckDeath()
	{
		if (hitPoints <= 0)
		{
			if (equippedCharm.charmItem)
			{
				equippedCharm.charmItem->TriggerItem(*this);
			}
			OnDeath();
		}
	}

	bool Unit::HasTrait(Trait trait) const {
		// // Only player team units can have traits
		// if (!attachedTeam || attachedTeam != &CombatSystem::GetPlayerTeam())
		// 	return false;

		// Check if unit has the trait
		return std::find(traits.begin(), traits.end(), trait) != traits.end();
	}
	void Unit::AddTrait(Trait trait)
	{
		if (!HasTrait(trait))
		{
			traits.push_back(trait);
		}
	}

	void Unit::DrawUnit(bool flipped)
	{
		if (showPlaceableSlot)
		{
			Transform slotTransform = GetTransform();
			if (isBeingDragged)
				slotTransform.position = tempPosition;

			// Animation
			f64 dt = AEFrameRateControllerGetFrameTime();
			if (invertAnim)
			{
				pSlotAnimTimer -= static_cast<float>(dt);
			}
			else
			{
				pSlotAnimTimer += static_cast<float>(dt);
			}

			if (pSlotAnimTimer >= 0.75f)
			{
				pSlotAnimTimer = 0.75f;
				invertAnim = true;
			}
			else if (pSlotAnimTimer <= 0.0f)
			{
				pSlotAnimTimer = 0.0f;
				invertAnim = false;
			}

			float lerpFactor = pSlotAnimTimer / 0.75f;

			slotTransform.scale.x = LGCY_CP::LG_Lerp(pSlotInitialScale.x, pSlotInitialScale.x + 10.f, lerpFactor);
			slotTransform.scale.y = LGCY_CP::LG_Lerp(pSlotInitialScale.y, pSlotInitialScale.y + 10.f, lerpFactor);
			DrawSprite(slotTransform, Graphics::SLOT_EMPTY);
		}

		if (!isValid)
			return;
		if (showBox)
		{
			// box is behind everyone.

			if (isInHover)
			{
				DrawSprite(GetTransform(), Graphics::BUTTON_BG, CP::LG_Color{0.f, 127.5f, 0.f, 0.f});
			}
			else
			{
				DrawSprite(GetTransform(), Graphics::BUTTON_BG);
			}
		}
		if (isInHover)
		{
			OnHoverEnter();
		}
		else
		{
			OnHoverExit(flipped);
		}

		DrawSprite(childTransforms[0], Graphics::POWER_ICON);
		DrawSprite(childTransforms[1], Graphics::HEART_ICON);
		if (equippedCharm.charmItem)
			DrawSprite(childTransforms[2], equippedCharm.charmItem->GetSprite());
		Fonts_DrawText(std::to_string(GetAttackPoints()), childTransforms[0].position.x, childTransforms[0].position.y, 32, defaultFont);
		Fonts_DrawText(std::to_string(GetHitPoints()), childTransforms[1].position.x, childTransforms[1].position.y, 32, defaultFont);
	}

	void Unit::OnDragBegin()
	{
		if (!isValid)
			return;

		isBeingDragged = true;
		isInHover = false;
		tempPosition = transform.position;
	}

	void Unit::OnDrag()
	{
		if (!isValid)
			return;

		transform.position = ScreenToWorldPosition(mouseX, mouseY);
		transform.position.y -= (transform.scale.y * 0.4f);
	}

	void Unit::OnDragEnd()
	{
		if (!isValid || !isBeingDragged)
			return;

		transform.position = tempPosition;
		isBeingDragged = false;
	}
#pragma endregion
#pragma endregion

#pragma region DATA RELATED

	std::string enumToString(UnitType type)
	{
		switch (type)
		{
		case MEDJED:
			return "medjed";
		case ANHUR:
			return "anhur";
		case BENNU:
			return "bennu";
		case RA:
			return "ra";
		case KHONSU:
			return "khonsu";
		case BASTET:
			return "bastet";
		case ISIS:
			return "isis";
		case ANUBIS:
			return "anubis";
		case APEP:
			return "apep";
		case OSIRIS:
			return "osiris";
		case THOTH:
			return "thoth";
		case WADJET:
			return "wadjet";
		case NEKHBET:
			return "nekhbet";
		case MUMMY:
			return "mummy";
		default:
			return "";
		}
	}

	int stringToEnum(std::string unitName)
	{
		if (unitName == "medjed")
			return MEDJED;
		else if (unitName == "anhur")
			return ANHUR;
		else if (unitName == "bennu")
			return BENNU;
		else if (unitName == "ra")
			return RA;
		else if (unitName == "khonsu")
			return KHONSU;
		else if (unitName == "bastet")
			return BASTET;
		else if (unitName == "isis")
			return ISIS;
		else if (unitName == "anubis")
			return ANUBIS;
		else if (unitName == "apep")
			return APEP;
		else if (unitName == "osiris")
			return OSIRIS;
		else if (unitName == "thoth")
			return THOTH;
		else if (unitName == "wadjet")
			return WADJET;
		else if (unitName == "nekhbet")
			return NEKHBET;
		else if (unitName == "mummy")
			return MUMMY;
		else
			return NO_UNITS;
	}

	void LoadUnitDataIntoPool(bool forceReload)
	{
		static bool dataLoaded = false;
		if (dataLoaded && !forceReload)
			return;

		unitDatabase.clear();
		FILE *fp;
		char *readBuffer = new char[65536]; // buffer size
		short healthGrowth[2] = {1, 1};
		short attackGrowth[2] = {1, 1};
		if (fopen_s(&fp, "Assets/units.json", "rb") == 0)
		{
			rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
			rapidjson::Document document;
			document.ParseStream(is);
			fclose(fp);

			if (!document.IsObject())
			{
				printf("Invalid JSON format.\n");
				return;
			}

			unitDatabase.resize(static_cast<size_t>(UNIT_COUNT + 1));
			for (auto it = document.MemberBegin(); it != document.MemberEnd(); ++it)
			{
				const std::string unitName = it->name.GetString();
				const rapidjson::Value &unitData = it->value;

				if (!unitData.IsObject())
					continue;

				std::string unitDesc = unitData.HasMember("description") ? unitData["description"].GetString() : "Nameless";
				int health = unitData.HasMember("health") ? unitData["health"].GetInt() : 1;
				int attack = unitData.HasMember("attack") ? unitData["attack"].GetInt() : 1;
				int tier = unitData.HasMember("tier") ? unitData["tier"].GetInt() : 1;
				int cost = unitData.HasMember("cost") ? unitData["cost"].GetInt() : 1;
				int spriteID = unitData.HasMember("spriteID") ? unitData["spriteID"].GetInt() : 1;
				int skillCast = unitData.HasMember("skillCast") ? unitData["skillCast"].GetInt() : -1;
				std::string description = unitData.HasMember("description") ? unitData["description"].GetString() : "Description not loaded";
				Transform spriteTransform = {
						{0.f, 0.f},
						0.f,
						{DEFAULT_UNITSCALE, DEFAULT_UNITSCALE}};
				unitDatabase[stringToEnum(unitName)] = Unit(unitName, health, attack, tier, cost, skillCast, healthGrowth, attackGrowth, spriteTransform);
				unitDatabase[stringToEnum(unitName)].description = unitDesc;
				unitDatabase[stringToEnum(unitName)].SetSprite(static_cast<Graphics::SPRITE>(spriteID));
				unitDatabase[stringToEnum(unitName)].description = description;
				// Add traits if they exist
				if (unitData.HasMember("traits") && unitData["traits"].IsArray())
				{
					const rapidjson::Value &traits = unitData["traits"];
					for (rapidjson::SizeType i = 0; i < traits.Size(); i++)
					{
						if (traits[i].IsString())
						{
							// Convert string to enum and add trait
							Trait trait = TraitManager::GetInstance().StringToTrait(traits[i].GetString());
							unitDatabase[stringToEnum(unitName)].AddTrait(trait);
						}
					}
				}
			}
		}

		delete[] readBuffer;
		dataLoaded = true;
	}

#pragma endregion
} // End of Namespace