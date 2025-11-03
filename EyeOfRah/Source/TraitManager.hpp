/******************************************************************************/
/*!
\file           TraitManager.hpp
\project        Eye of Rah
\author(s)      Leonard, 100% - trait system design, effect handlers implementation, JSON parsing

\brief          TraitManager is a singleton class that manages the trait system and 
                applies bonuses based on the units of the same trait category.
                Unit traits are defined in Traits.hpp and are used to determine the bonuses.

                - Load trait configs from traits.json using LoadTraitConfigs()
                - Tracking active traits with CountTeamTraits() and CalculateTraitLevels()
                - Applying effects through effectHandlers and ApplyEffect() function
                - Providing trait information to console with GetTraitDescription() and UI with GetActiveTraits()
                
                Traits.hpp - Defines Trait enum, BonusEffectType enum for resuable trait effect, Struct TraitBonusEffect struct for effect data
                traits.json - Contains structured array of trait definitions, including thresholds and effects
                TraitManager.hpp - Singleton class, loads and parses configs tracks which traits are active, calculates levels, applies effects
                The system integrates with CombatSystem.cpp, ShopStage.cpp which calls UpdateTraits() and 
                ApplyTraitBonuses() during team change phases and combat initialization respectively.

                
All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#pragma once

#include "Unit.hpp"
#include "Team.hpp"
#include "Traits.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>


namespace EOREntities {

// Represents data of single trait bonus effect
struct TraitBonusEffect {
    BonusEffectType effectType = BonusEffectType::NO_EFFECT; // Undefined effect type
    float value = 0.0f;                     // Default value of 0
    bool applyToTraitOnly = false;          // Default to applying to all units
    UnitType unitTypeToSummon = WADJET;     // Only used for SUMMON_ON_DEATH
};

// Add configuration for a trait effect
struct TraitBonusConfig {
    std::vector<int> thresholds; // Units needed for each level
    std::vector<std::vector<TraitBonusEffect>> effects; // Effects by level
    
    void AddEffect(int level, TraitBonusEffect effect) {
        if (level > 0 && level <= static_cast<int>(effects.size())) {
            effects[level - 1].push_back(effect);
        }
    }
};

// Data structure to store trait counts and levels for player team
struct TeamTraitData {
    std::unordered_map<Trait, int> counts;
    std::unordered_map<Trait, int> levels;
    
    void Clear() {
        counts.clear();
        levels.clear();
    }
};

// TraitManager class - Singleton class that manages unit trait synergies
// It tracks which traits are active based on team composition and applies their effects
class TraitManager {
private:
    // Private constructor for singleton
    TraitManager();
    ~TraitManager() = default;
    
    // Prevent copying
    TraitManager(const TraitManager&) = delete;
    TraitManager& operator=(const TraitManager&) = delete;
    
    // Static instance
    static TraitManager* instance;
    
    // Effect handlers for applying effects to units
    using EffectHandler = std::function<void(Unit&, const TraitBonusEffect&)>;
    std::unordered_map<BonusEffectType, EffectHandler> effectHandlers;

    // Team trait data - only for player team
    TeamTraitData playerTraitData;
    
    // Configuration for all trait bonuses
    std::unordered_map<Trait, TraitBonusConfig> traitConfigs;
    
    // Initialize handlers for different effect types
    void InitEffectHandlers();
    
    // Helper methods for trait configuration
    BonusEffectType StringToEffectType(const char* effectStr);
    UnitType StringToUnitType(const char* unitTypeStr);

    // Load configs (called automatically in constructor)
    void LoadDefaultTraitConfigs(); // Fallback on hardcode configs
    bool LoadTraitConfigs();        // Loads from JSON
    
    // Helper methods for trait processing
    template<typename EffectFunction>
    void ForEachTraitEffect(Team& team, EffectFunction&& effectFunc);
    
    void CountTeamTraits(const Team& team, std::unordered_map<Trait, int>& traitCounts);
    void CalculateTraitLevels(const std::unordered_map<Trait, int>& counts, 
                             std::unordered_map<Trait, int>& levels);
    
    // Apply a specific effect to a team
    void ApplyEffect(const TraitBonusEffect& effect, Team& team, Trait trait);
    
public:
    // Singleton access method
    static TraitManager& GetInstance();
    static void DestroyInstance();
    
    // Update trait counts and active levels - (Player Team Only)
    void UpdateTraits(const Team& playerTeam);
    
    // Apply trait bonuses to team
    void ApplyTraitBonuses(Team& playerTeam);  // Apply all effects
    void ApplyInitialTraitBonuses(Team& playerTeam);  // Non-recurring effects
    void ApplyRecurringTraitEffects(Team& playerTeam);  // Recurring effects like healing
    void ApplyPostCombatEffects(Team& playerTeam);
    bool IsRecurringEffect(BonusEffectType effectType) const;
    
    
    // Team death handling though Unit.cpp OnDeath() method
    void SpawnUnit(Unit& dyingUnit);

    // Get information about traits
    Trait StringToTrait(const char* traitStr);
    int GetTraitCount(Trait trait) const;
    int GetTraitLevel(Trait trait) const;
    std::string GetTraitDescription(Trait trait) const;
    
    // Get active trait info for UI
    std::vector<std::pair<Trait, int>> GetActiveTraits() const;
};

// Reads active traits from the player's team composition
// Looks up configurations in traitConfigs to determine what should happen
// Filters effects based on the provided lambda function
// Applies the effects to appropriate units
template<typename EffectFunction>
void TraitManager::ForEachTraitEffect(Team& team, EffectFunction&& effectFunc) {
    const auto& traitLevels = playerTraitData.levels;
    
    for (const auto& pair : traitLevels) {
        const Trait& currentTrait = pair.first;
        const int level = pair.second;
        if (traitConfigs.find(currentTrait) == traitConfigs.end()) 
            continue;
        
        for (int i = 0; i < level && i < static_cast<int>(traitConfigs[currentTrait].effects.size()); ++i) {
            const auto& effectsForLevel = traitConfigs[currentTrait].effects[i];
            for (const auto& effect : effectsForLevel) {
                if (effectFunc(effect, currentTrait))
                    ApplyEffect(effect, team, currentTrait);
            }
        }
    }
}

} // namespace EOREntities