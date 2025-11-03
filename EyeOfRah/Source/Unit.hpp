#pragma once
//---------------------------------------------------------------------------------------
// file:	Unit.hpp
// author:	Primary: Jason (80%), Secondary: Leonard (10%), Lebon (10%)
// email:	l.jason@digipen.edu, leonardjunyi.l@digipen.edu, h.xinyulebon@digipen.edu
//
// brief:	This file contains declarations for the Unit Object. Also declares enums
//			and structs associated with Units.
//
// Copyright � 2025 DigiPen, All rights reserved.
//---------------------------------------------------------------------------------------
#include "Core.hpp"
#include "Graphics.hpp"
#include <array>
#include <string>
#include "EntityBase.hpp"
#include "Charm.hpp"
#include "Traits.hpp"
#include <vector>

// Forward declare UnitTooltip to prevent circular dependency
namespace EORWidgets
{
	class UnitTooltip;
}

namespace EOREntities
{

	// Unit type enums
	enum UnitType
	{
		NO_UNITS = 0,
		MEDJED,
		ANHUR,
		BENNU,
		RA,
		KHONSU,
		BASTET,
		ISIS,
		ANUBIS,
		APEP,
		OSIRIS,
		THOTH,
		WADJET,
		NEKHBET,
		MUMMY,
		UNIT_COUNT
	};

	// Unit data structure
	struct UnitData
	{
		std::string name;
		std::string description;
		int health = 0;
		int attack = 0;
		int tier = 0;
		int skillCast = -1; // -1 means can cast unlimited, 0 means cannot cast, >1 number times can cast skill.
		Graphics::SPRITE sprite = Graphics::NO_SPRITE;
		UnitType type = NO_UNITS;
		const char *jsonKey = "";
	};

	// Helper functions for unit data
	std::string enumToString(UnitType type);
	int stringToEnum(std::string unitName);

	// Forward Declaration for Team struct to prevent circular dependency between Unit and Team.
	struct Team;
	// Forward Declaration for Item class and enum to prevent circular dependency between Unit and Item.
	enum class ItemID;
	class Item;

	class Unit : public EntityObject
	{
	private:
		/* Private Members */
		Team *attachedTeam;	// The team this unit is attached to. Default constructor sets this unit to teamless, essentially a nullptr.
		Charm equippedCharm; // Reference to base item equipped by this unit. Should always point to a base item in the database if theres smth equipped since items don't change.
		std::vector<Trait> traits; // Add Traits Collection
		std::array<Transform, 3> childTransforms; // Offset transforms from the parent transform for UI elements tied to the unit.
		int hitPoints; // Hitpoints of the unit.
		int attackPoints; // Attack Power of the unit.
		int baseHitPoints; // Base Hitpoints, has no effect in battle. This is meant to remember the original hitpoints outside of battle.
		int baseAttackPoints; // Base Attack Power, has no effect in battle. This is meant to remember the original Attack Power outside of battle.
		short hitPointGrowth[2]; // Each element represents how much Hitpoints is gained upon gaining a level. Element to use corresponds to unit level.
		short attackPointGrowth[2]; // Each element represents how much Attack Power is gained upon gaining a level. Element to use corresponds to unit level.
		int xp; // "XP" is essentially how many duplicates have been applied onto this unit. 3 duplicates are required to level up once.
		bool hasAttacked; // Battle only member variable, has no effect outside battle. Once it has attacked, is set to true and then back to false upon round start.
		int skillCast = -1;
		int castCount = 0;

		/* Private Methods */
		// Internal function that calculates total damage of the unit after all effects.\return a (int) representing total damage of the unit.
		int GetDamage() const;
		// Internal function that updates ChildTransforms. Should be called each time the parent transform is updated.
		void UpdateChildTransforms();
		// Method that is called when mouse cursor hovers over this unit.
		void OnHoverEnter();
		// Method that is called when mouse cursor leaves this unit.
		void OnHoverExit(bool);

		/* Friends */
		friend class EORWidgets::UnitTooltip; // Allows the Unit Tooltip class to access Unit private members.

	public:
		/* Public Members */
		int teamSlot; // The index in the team for which this unit is located in. -1 if the Unit is not in a team.

		/* Constructors */
		// Default Constructor
		Unit();
		// Parameterized Constructor. You should be using this almost everytime.
		Unit(std::string _name, int _hitPoints, int _attackPoints, int _tier, int _cost, int _skillCast, const short _hitPointGrowth[2], const short _attackPointGrowth[2], Transform _transform, const std::vector<Trait>& initialTraits = {});		// Copy Constructor and Assignment. Use this when copying unit from bench to battle team.
		Unit(const Unit &unit);
		Unit &operator=(const Unit &rhs);
		// Move Constructor and Assignment. Use this when moving the unit from one team to another or from index to another index. Mainly for std::swap.
		Unit(Unit &&other) noexcept;
		Unit &operator=(Unit &&other) noexcept;
		// Destructor
		~Unit();

		/* Setters and Getters Methods */
		// Sets reference to the team this unit is attached to. Do not call this explicitly, this function should be called by the team instead when adding or removing units.\param (Team*) team - reference pointer to the team this unit belongs to.
		void SetTeam(Team *team);
		// Gets the team this unit is attached to.\return a (Team*) pointer to the team this unit is attached to.
		Team *GetTeam();
		// Sets the item equipped onto the unit. Only the id should be used.
		void EquipItem(ItemID item_id);
		// Sets the item equipped by the unit to be empty.
		void UnequipItem();
		// Gets the item currently equipped by the unit.
		Charm& GetEquippedItem();
		void SetHasAttacked(bool attacked);
		// Sets a new position of the Unit Object.\param (float) x - World Space X Coordinates.\param (float) y - World Space Y Coordinates.
		void SetPosition(float x, float y);
		// Sets a new counter-clockwise rotation of the Unit Object.\param (float) rotation - Rotation in Degrees. 0 being East.
		void SetRotation(float _rotation);
		// Sets the scale of the Unit Object.\param (float) x - Vertical Scaling.\param (float) y - Horizontal Scaling.
		void SetScale(float x, float y);
		// Sets the transform of the Unit Object with a given transform.\param (Transform) _transform - Copy of a transform to be set.
		void SetTransform(Transform _transform);
		// Sets the hitpoints of the Unit Object.\param (int) _hitpoints.
		void SetHitPoints(int _hitPoints);
		// Gets the current hitpoints of the Unit Object.\return a (int).
		int GetHitPoints() const;
		// Sets the Attack Power of the Unit Object.\param (int) _attackPoints.
		void SetAttackPoints(int _attackPoints);
		// Gets the current Attack power of the Unit Object.\return a (int).
		int GetAttackPoints() const;
		// Sets the base hitpoints of the Unit Object.\param (int) _hitpoints.
		void SetBaseHitPoints(int _hitPoints);
		// Gets the current base hitpoints of the Unit Object.\return a (int).
		int GetBaseHitPoints() const;
		// Sets the Base Attack Power of the Unit Object.\param (int) _attackPoints.
		void SetBaseAttackPoints(int _attackPoints);
		// Gets the current Base Attack power of the Unit Object.\return a (int).
		int GetBaseAttackPoints() const;
		// Sets the current XP accumulated by the Unit Object. Every 3 XP equals +1 level.\param (int) _xp.
		void SetXP(int _xp);
		// Gets the current XP accumulated by the Unit Object. Every 3 XP equals +1 level.\return a (int).
		int GetXP() const;
		// Gets the current level of the Unit Object. Every 3 XP equals +1 level.\return a (int).
		int GetLevel() const;

		// // Effect methods
		void CastSkill(Team &playerTeam);

		/* Public Methods */
		// Restores the Unit's state back to preparation phase. Might replace this with destructor calls post combat in the future instead.
		void RestoreState();
		// Sets Unit Object as "NULL". In a team, it changes that slot this unit is at into a "empty" slot.
		void Reset();
		// Method that is called when the Unit Object gets damaged.
		void Hurt(int damage, bool checkDeath = true);
		// Method used to check if they are valid to die. If valid, they die.
		void CheckDeath();
		// Method that is called when the Unit Object dies.\param (int) damage - damage taken by the unit.
		void OnDeath();
		// Method that is called upon being fed a duplicate.
		void LevelUnit(Unit &unit);
		// Method that is called to draw the Unit.
		void DrawUnit(bool flipped = false);
		// Method that is called to when unit is starting to be dragged. Might move this to a interface class in the future instead.
		void OnDragBegin();
		// Method that is called to when unit is being dragged. Might move this to a interface class in the future instead.
		void OnDrag();
		// Method that is called to when unit is ending drag. Might move this to a interface class in the future instead.
		void OnDragEnd();

		// Trait methods
		const std::vector<Trait>& GetTraits() const { return traits; }
		void AddTrait(Trait trait);
		bool HasTrait(Trait trait) const;
	};

	extern std::vector<Unit> unitDatabase;

	// Unit Database
	void LoadUnitDataIntoPool(bool forceReload = false);
	inline const std::vector<Unit> &GetUnitData() { return unitDatabase; };
}