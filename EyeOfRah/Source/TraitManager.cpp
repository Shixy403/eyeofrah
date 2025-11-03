/******************************************************************************/
/*!
\file           TraitManager.cpp
\project        Eye of Rah
\author(s)      [Leonard], 100% - trait system design, effect handlers implementation, JSON parsing

\brief          TraitManager implementation handles all trait-related functionality:
                - Loading from JSON with LoadTraitConfigs(), fallback using LoadDefaultTraitConfigs() with hardcoded values
                - Effect handler functions using lambda expressions for each effect type
                - Team trait counting and level calculation (CountTeamTraits, CalculateTraitLevels)
                - Applying effects through ForEachTraitEffect which calls ApplyEffect() to specific or all units

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#include "pch.hpp"
#include "TraitManager.hpp"
#include "Unit.hpp" 
#include "Team.hpp"
#include "CombatSystem.hpp"
#include "document.h"
#include "filereadstream.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace EOREntities {

//==============================================================================
// Singleton Setup and Initialization
//==============================================================================

// Initialize static instance pointer
TraitManager* TraitManager::instance = nullptr;

// Gets the singleton instance, creating it if it doesn't exist
// Then, return Reference to the TraitManager singleton
TraitManager& TraitManager::GetInstance() {
    if (!instance) {
        instance = new TraitManager();
    }
    return *instance;
}

// Destroys the singleton instance and frees memory
void TraitManager::DestroyInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}
// Constructor loads configs and initializes effect handlers

TraitManager::TraitManager() {
    // Try to load from JSON first, fall back to hardcoded configs if that fails
    if (!LoadTraitConfigs()) {
        LoadDefaultTraitConfigs();
    }
    InitEffectHandlers();
}

//==============================================================================
// Effect Handler Implementation
//==============================================================================
// This section defines how each trait effect type is processed and applied to units based on effect type

void TraitManager::InitEffectHandlers() {
    // Initialize effect handlers for different bonus types
    effectHandlers[BonusEffectType::INCREASE_ATTACK] = [](Unit& unit, const TraitBonusEffect& effect) {
        // Apply attack increase
        unit.SetAttackPoints(unit.GetAttackPoints() + static_cast<int>(effect.value));
    };
    
    effectHandlers[BonusEffectType::INCREASE_HEALTH] = [](Unit& unit, const TraitBonusEffect& effect) {
        // Apply health increase
        unit.SetHitPoints(unit.GetHitPoints() + static_cast<int>(effect.value));
    };
    
    effectHandlers[BonusEffectType::HEALTH_REGEN] = [](Unit& unit, const TraitBonusEffect& effect) {
        // Apply health regeneration
        unit.SetHitPoints(unit.GetHitPoints() + static_cast<int>(effect.value));
    };
    
    // effectHandlers[BonusEffectType::LIFE_STEAL] = [](Unit& unit, const TraitBonusEffect& effect) {
    //     // Warrior trait, applied in Unit::Hurt() method
    // };
    
    // effectHandlers[BonusEffectType::GOLD_BONUS] = [](Unit&, const TraitBonusEffect&) {
    //     // (Wisdom) Handled in WoldMap.cpp whenever player clicks onto new node (bonus 3 gold)
    //     // (Lesser) level1.cpp on win to double gold reward
    // };
    
    effectHandlers[BonusEffectType::RESURRECT_CHANCE] = [](Unit& unit, const TraitBonusEffect& effect) {
        (void)effect;
        unit.SetHitPoints(unit.GetBaseHitPoints());
        unit.SetShowBox(true);
    };
    
    effectHandlers[BonusEffectType::ATTACK_MULTIPLIER] = [](Unit& unit, const TraitBonusEffect& effect) {
        unit.SetAttackPoints(unit.GetAttackPoints() * static_cast<int>(effect.value));
    };

    effectHandlers[BonusEffectType::CUMULATIVE_ATTACK] = [](Unit& unit, const TraitBonusEffect& effect) {
        int newAttack = unit.GetAttackPoints() + static_cast<int>(effect.value);
        unit.SetAttackPoints(newAttack);
        unit.SetBaseAttackPoints(newAttack);
        std::cout << unit.name << "gained +1 permanent attack from chaos trait" << std::endl;
    };
    
}

// Called when a unit with DEATH trait dies to create a replacement unit
void TraitManager::SpawnUnit(Unit& dyingUnit) {
    dyingUnit = GetUnitData()[UnitType::MEDJED];
    dyingUnit.isValid = true;  // Explicit calls
    dyingUnit.SetShowBox(true);
    std::cout << "SpawnUnit called for " << dyingUnit.name << std::endl;
}

#pragma region CONVERSION HELPERS
//==============================================================================
// String Conversion Utilities
//==============================================================================

Trait TraitManager::StringToTrait(const char* traitStr) {
    static const std::unordered_map<std::string, Trait> traitMap = {
        {"DIVINE", Trait::DIVINE},
        {"SOLAR", Trait::SOLAR},
        {"LUNAR", Trait::LUNAR},
        {"WATER", Trait::WATER},
        {"SLAYER", Trait::SLAYER},
        {"WARRIOR", Trait::WARRIOR},
        {"DEATH", Trait::DEATH},
        {"WISDOM", Trait::WISDOM},
        {"CHAOS", Trait::CHAOS},
        {"LESSER", Trait::LESSER}
    };
    
    auto it = traitMap.find(traitStr);
    if (it != traitMap.end()) {
        return it->second;
    }
    
    printf("Warning: Unknown trait string: %s\n", traitStr);
    return Trait::DIVINE; // Default value
}

BonusEffectType TraitManager::StringToEffectType(const char* effectStr) {
    static const std::unordered_map<std::string, BonusEffectType> effectMap = {
        {"INCREASE_ATTACK", BonusEffectType::INCREASE_ATTACK},
        {"INCREASE_HEALTH", BonusEffectType::INCREASE_HEALTH},
        {"HEALTH_REGEN", BonusEffectType::HEALTH_REGEN},
        {"LIFE_STEAL", BonusEffectType::LIFE_STEAL},
        {"GOLD_BONUS", BonusEffectType::GOLD_BONUS},
        {"RESURRECT_CHANCE", BonusEffectType::RESURRECT_CHANCE},
        {"ATTACK_MULTIPLIER", BonusEffectType::ATTACK_MULTIPLIER},
        {"CUMULATIVE_ATTACK", BonusEffectType::CUMULATIVE_ATTACK},
        {"SUMMON_ON_DEATH", BonusEffectType::SUMMON_ON_DEATH}
    };
    
    auto it = effectMap.find(effectStr);
    if (it != effectMap.end()) {
        return it->second;
    }
    
    printf("Warning: Unknown effect type string: %s\n", effectStr);
    return BonusEffectType::INCREASE_ATTACK; // Default value
}

UnitType TraitManager::StringToUnitType(const char* unitTypeStr) {
    static const std::unordered_map<std::string, UnitType> unitMap = {
        {"WADJET", WADJET},
        {"MEDJED", MEDJED},
        {"APEP", APEP},
        {"ANHUR", ANHUR},
        {"ANUBIS", ANUBIS},
        {"BASTET", BASTET},
        {"BENNU", BENNU},
        {"ISIS", ISIS},
        {"KHONSU", KHONSU},
        {"NEKHBET", NEKHBET},
        {"OSIRIS", OSIRIS},
        {"RA", RA},
        {"THOTH", THOTH}
    };
    
    auto it = unitMap.find(unitTypeStr);
    if (it != unitMap.end()) {
        return it->second;
    }
    
    printf("Warning: Unknown unit type string: %s\n", unitTypeStr);
    return WADJET; // Default value
}

#pragma endregion

//==============================================================================
// Configuration Loading
//==============================================================================

bool TraitManager::LoadTraitConfigs() {
    const char* traitsPath = "Assets/traits.json";
    
    // Open the JSON file
    FILE* fp = nullptr;
    fopen_s(&fp, traitsPath, "rb");
    if (!fp) {
        printf("Error: Cannot open traits configuration file: %s\n", traitsPath);
        return false;
    }
    
    // Read the file into a buffer
    char readBuffer[8192] = "";
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    // Parse the JSON document
    rapidjson::Document document;
    document.ParseStream(is);
    fclose(fp);
    
    // Check for parsing errors
    if (document.HasParseError()) {
        printf("Error parsing traits configuration: error code %d (offset %zu)\n", 
               document.GetParseError(),
               document.GetErrorOffset());
        return false;
    }
    
    // Check if the document has the expected structure
    if (!document.IsObject() || !document.HasMember("traits") || !document["traits"].IsArray()) {
        printf("Error: Invalid traits configuration format\n");
        return false;
    }
    
    // Clear existing trait configs
    traitConfigs.clear();
    
    // Process each trait
    const rapidjson::Value& traits = document["traits"];
    for (rapidjson::SizeType i = 0; i < traits.Size(); i++) {
        const rapidjson::Value& traitData = traits[i];
        
        // Validate trait data
        if (!traitData.IsObject() || 
            !traitData.HasMember("id") || !traitData["id"].IsString() ||
            !traitData.HasMember("thresholds") || !traitData["thresholds"].IsArray() ||
            !traitData.HasMember("effects") || !traitData["effects"].IsArray()) {
            printf("Warning: Skipping invalid trait at index %d\n", i);
            continue;
        }
        
        // Get trait ID
        Trait traitId = StringToTrait(traitData["id"].GetString());
        
        // Get thresholds
        auto& config = traitConfigs[traitId];
        const rapidjson::Value& thresholds = traitData["thresholds"];
        for (rapidjson::SizeType j = 0; j < thresholds.Size(); j++) {
            if (thresholds[j].IsInt()) {
                config.thresholds.push_back(thresholds[j].GetInt());
            }
        }
        
        // Process effects for each level
        const rapidjson::Value& effects = traitData["effects"];
        config.effects.resize(effects.Size());
        
        for (rapidjson::SizeType j = 0; j < effects.Size(); j++) {
            const rapidjson::Value& levelData = effects[j];
            
            if (!levelData.IsObject() || !levelData.HasMember("level") || 
                !levelData["level"].IsInt() || !levelData.HasMember("bonuses") || 
                !levelData["bonuses"].IsArray()) {
                printf("Warning: Skipping invalid effect data for trait %s at level %d\n", 
                       traitData["id"].GetString(), j + 1);
                continue;
            }
            
            int level = levelData["level"].GetInt();
            const rapidjson::Value& bonuses = levelData["bonuses"];
            
            for (rapidjson::SizeType k = 0; k < bonuses.Size(); k++) {
                const rapidjson::Value& bonusData = bonuses[k];
                
                if (!bonusData.IsObject() || !bonusData.HasMember("type") || 
                    !bonusData["type"].IsString() || !bonusData.HasMember("value") || 
                    !bonusData.HasMember("traitOnly") || !bonusData["traitOnly"].IsBool()) {
                    printf("Warning: Skipping invalid bonus data for trait %s at level %d, bonus %d\n", 
                           traitData["id"].GetString(), level, k);
                    continue;
                }
                
                // Create effect
                TraitBonusEffect effect;
                effect.effectType = StringToEffectType(bonusData["type"].GetString());
                effect.value = bonusData["value"].IsFloat() ? 
                               bonusData["value"].GetFloat() : 
                               static_cast<float>(bonusData["value"].GetInt());
                effect.applyToTraitOnly = bonusData["traitOnly"].GetBool();
                
                // Handle optional unit type for summon effects
                if (effect.effectType == BonusEffectType::SUMMON_ON_DEATH && 
                    bonusData.HasMember("unitType") && bonusData["unitType"].IsString()) {
                    effect.unitTypeToSummon = StringToUnitType(bonusData["unitType"].GetString());
                }
                
                // Add effect to configuration
                config.AddEffect(level, effect);
            }
        }
    }
    
    printf("Successfully loaded trait configurations from %s\n", traitsPath);
    return true;
}


//==============================================================================
// Trait Processing Implementation
//==============================================================================

void TraitManager::CountTeamTraits(const Team& team, std::unordered_map<Trait, int>& traitCounts) {
    // Clear existing counts
    traitCounts.clear();
    
    // Count traits for each valid unit in the team
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit& unit = team.units[i];
        if (!unit.isValid) continue;
        
        // Get traits from the unit and increment counts
        for (const auto& trait : unit.GetTraits()) {
            traitCounts[trait]++;
        }
    }
}

void TraitManager::CalculateTraitLevels(const std::unordered_map<Trait, int>& counts, 
                                       std::unordered_map<Trait, int>& levels) {
    // Clear existing levels
    levels.clear();
    
    // Calculate level for each trait based on count and thresholds
    for (const auto& pair : counts) {
        const Trait& trait = pair.first;
        const int count = pair.second;

        // Skip if no configuration exists for this trait
        if (traitConfigs.find(trait) == traitConfigs.end()) continue;
        
        const auto& thresholds = traitConfigs[trait].thresholds;
        int level = 0;
        
        // Find highest threshold met (e.g., having 5 DIVINE units gives level 2 if thresholds are [2,4])
        for (size_t i = 0; i < thresholds.size(); i++) {
            if (count >= thresholds[i]) {
                level = static_cast<int>(i + 1);
            } else {
                break;
            }
        }
        
        // Store non-zero level values
        if (level > 0) {
            levels[trait] = level;
        }
    }
}

#pragma region CORE USAGE FUNCTIONS
//==============================================================================
// Core Public API Functions
//==============================================================================

// This is called whenever the team composition changes
void TraitManager::UpdateTraits(const Team& playerTeam) {    
    // Calculate new trait counts and levels
    CountTeamTraits(playerTeam, playerTraitData.counts);
    CalculateTraitLevels(playerTraitData.counts, playerTraitData.levels);
}

// Called by ForEachTraitEffect, applies a specific effect to all eligible units in a team or entire team based on effect.applyToTraitOnly
void TraitManager::ApplyEffect(const TraitBonusEffect& effect, Team& team, Trait trait) {
    // Apply effects to all valid units in the team
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit& unit = team.units[i];
        if (!unit.isValid) continue;
        
        // If effect should only apply to units with the trait, check for it
        if (effect.applyToTraitOnly) {
            bool hasMatchingTrait = false;
            // Check if unit has the trait this effect is associated with
            for (const auto& unitTrait : unit.GetTraits()) {
                if (unitTrait == trait) {
                    hasMatchingTrait = true;
                    break;
                }
            }
            
            if (!hasMatchingTrait) continue;
        }
        
        // Use the appropriate effect handler if one exists
        auto handlerIt = effectHandlers.find(effect.effectType);
        if (handlerIt != effectHandlers.end()) {
            handlerIt->second(unit, effect);
        }
    }
}

// Applies all trait effects to a team
void TraitManager::ApplyTraitBonuses(Team& playerTeam) {
    // Apply all effects using ForEachTraitEffect
    ForEachTraitEffect(playerTeam, [](const TraitBonusEffect&, Trait) {
        return true; // Apply all effects
    });
}

// Applies only non-recurring trait effects, used when initializing the combat system
void TraitManager::ApplyInitialTraitBonuses(Team& playerTeam) {
    // Only apply non-recurring effects
    ForEachTraitEffect(playerTeam, [this](const TraitBonusEffect& effect, Trait) {
        return !IsRecurringEffect(effect.effectType);
    });
}

// Currently only applies to effect Health Regen to handle effects that trigger repeatedly
void TraitManager::ApplyRecurringTraitEffects(Team& playerTeam) {
    ForEachTraitEffect(playerTeam, [this](const TraitBonusEffect& effect, Trait) {
        return IsRecurringEffect(effect.effectType);
    });
}

// Chaos Trait Attack increase. Called after combat ends to apply permanent effects
void TraitManager::ApplyPostCombatEffects(Team& team) {
    ForEachTraitEffect(team, [](const TraitBonusEffect& effect, Trait) {
        return effect.effectType == BonusEffectType::CUMULATIVE_ATTACK;
    });
}

bool TraitManager::IsRecurringEffect(BonusEffectType effectType) const {
    // Effects that are applied every turn
    return effectType == BonusEffectType::HEALTH_REGEN ||
           effectType == BonusEffectType::CUMULATIVE_ATTACK;
}
#pragma endregion
#pragma region INFORMATION RETRIEVAL METHODS
//==============================================================================
// Information Retrieval Methods for UI and Console
//==============================================================================

int TraitManager::GetTraitCount(Trait trait) const {
    auto it = playerTraitData.counts.find(trait);
    return it != playerTraitData.counts.end() ? it->second : 0;
}

// Gets the active level of a specific trait
int TraitManager::GetTraitLevel(Trait trait) const {
    auto it = playerTraitData.levels.find(trait);
    return it != playerTraitData.levels.end() ? it->second : 0;
}

std::string TraitManager::GetTraitDescription(Trait trait) const {
    int level = GetTraitLevel(trait);
    
    std::string desc = ::EOREntities::GetTraitDescription(trait);
    
    if (level > 0) {
        return desc + " (Active: Level " + std::to_string(level) + ")";
    }
    return desc + " (Inactive)";
}

std::vector<std::pair<Trait, int>> TraitManager::GetActiveTraits() const {
    std::vector<std::pair<Trait, int>> result;
    
    for (const auto& pair : playerTraitData.levels) {
        if (pair.second > 0) {
            result.push_back(pair);
        }
    }
    
    return result;
}
#pragma endregion

//==============================================================================
// Default Configuration Setup
//==============================================================================
// Fallback configuration if JSON loading fails

void TraitManager::LoadDefaultTraitConfigs() {
    printf("Using default trait configurations\n");
    traitConfigs.clear();

    traitConfigs[Trait::DIVINE].thresholds = {2, 3};
    traitConfigs[Trait::DIVINE].effects.resize(2);
    traitConfigs[Trait::DIVINE].AddEffect(1, {BonusEffectType::INCREASE_ATTACK, 2.0f, true});
    traitConfigs[Trait::DIVINE].AddEffect(1, {BonusEffectType::INCREASE_HEALTH, 2.0f, true});
    traitConfigs[Trait::DIVINE].AddEffect(2, {BonusEffectType::RESURRECT_CHANCE, 100.0f, true});

    traitConfigs[Trait::LESSER].thresholds = {3, 5};
    traitConfigs[Trait::LESSER].effects.resize(2);
    traitConfigs[Trait::LESSER].AddEffect(1, {BonusEffectType::INCREASE_HEALTH, 3.0f, true});
    // (Lesser) level1.cpp on win to double gold reward

    traitConfigs[Trait::SOLAR].thresholds = {2, 4};
    traitConfigs[Trait::SOLAR].effects.resize(2);
    traitConfigs[Trait::SOLAR].AddEffect(1, {BonusEffectType::INCREASE_ATTACK, 2.0f, true});
    traitConfigs[Trait::SOLAR].AddEffect(2, {BonusEffectType::HEALTH_REGEN, 1.0f, false});

    traitConfigs[Trait::LUNAR].thresholds = {2, 4};
    traitConfigs[Trait::LUNAR].effects.resize(2);
    traitConfigs[Trait::LUNAR].AddEffect(1, {BonusEffectType::INCREASE_HEALTH, 4.0f, true});
    traitConfigs[Trait::LUNAR].AddEffect(2, {BonusEffectType::INCREASE_ATTACK, 2.0f, true});

    traitConfigs[Trait::WATER].thresholds = {1};
    traitConfigs[Trait::WATER].effects.resize(1);
    // No effects added - dodge is handled in Unit::Hurt()

    traitConfigs[Trait::SLAYER].thresholds = {3};
    traitConfigs[Trait::SLAYER].effects.resize(1);
    traitConfigs[Trait::SLAYER].AddEffect(1, {BonusEffectType::ATTACK_MULTIPLIER, 2.0f, false});

    traitConfigs[Trait::WARRIOR].thresholds = {3};
    traitConfigs[Trait::WARRIOR].effects.resize(1);
    // Warrior trait, applied in Unit::Hurt() method

    traitConfigs[Trait::DEATH].thresholds = {2};
    traitConfigs[Trait::DEATH].effects.resize(1);
    // traitConfigs[Trait::DEATH].AddEffect(1, {BonusEffectType::SUMMON_ON_DEATH, 100.0f, true, MEDJED});

    traitConfigs[Trait::WISDOM].thresholds = {1};
    traitConfigs[Trait::WISDOM].effects.resize(1);
    // (Wisdom) Handled in WoldMap.cpp whenever player clicks onto new node (bonus 3 gold)

    traitConfigs[Trait::CHAOS].thresholds = {1};
    traitConfigs[Trait::CHAOS].effects.resize(1);
    traitConfigs[Trait::CHAOS].AddEffect(1, {BonusEffectType::CUMULATIVE_ATTACK, 1.0f, true});
}

} // namespace EOREntities