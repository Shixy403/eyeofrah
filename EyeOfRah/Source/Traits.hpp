#pragma once

namespace EOREntities {
    enum class Trait {
        // Divine Categories
        DIVINE = 0,     // Major gods (Ra, Osiris, Isis)
        LESSER,     // Minor deities
        
        // Elemental Alignments
        SOLAR,      // Sun-aligned (Ra, Bennu) 
        LUNAR,      // Moon-aligned (Khonsu, Isis)
        WATER,      // Nile-aligned (Wadjet)
        
        // Specialties
        WARRIOR,    // Combat specialists (Anhur)
        SLAYER,     // Guardian deities (Bastet, Anubis)
        CHAOS,      // Destructive entities (Apep)
        WISDOM,     // Knowledge deities (Thoth)
        DEATH,      // Underworld beings (Osiris, Anubis)
        
        TRAIT_COUNT
    };

    // Bonus effect types for traits
    enum class BonusEffectType {
        INCREASE_ATTACK,
        INCREASE_HEALTH,
        HEALTH_REGEN,
        LIFE_STEAL,         // Defined in Unit::Hurt() instead of EffectHandler
        GOLD_BONUS,         // Directly modifies Combat Rewards in level1.cpp 
        RESURRECT_CHANCE,
        ATTACK_MULTIPLIER,
        CUMULATIVE_ATTACK,
        SUMMON_ON_DEATH,    // Defined in Unit::OnDeath() instead of EffectHandler
        NO_EFFECT      // Used for Water trait dodge chance, defined in Unit::Hurt() for now
    };

    // Convert effect type to string
    inline const char* EffectTypeToString(BonusEffectType effectType) {
        switch (effectType) {
            case BonusEffectType::INCREASE_ATTACK: return "INCREASE_ATTACK";
            case BonusEffectType::INCREASE_HEALTH: return "INCREASE_HEALTH";
            case BonusEffectType::HEALTH_REGEN: return "HEALTH_REGEN";
            case BonusEffectType::LIFE_STEAL: return "LIFE_STEAL";
            case BonusEffectType::GOLD_BONUS: return "GOLD_BONUS";
            case BonusEffectType::RESURRECT_CHANCE: return "RESURRECT_CHANCE";
            case BonusEffectType::ATTACK_MULTIPLIER: return "ATTACK_MULTIPLIER";
            case BonusEffectType::CUMULATIVE_ATTACK: return "CUMULATIVE_ATTACK";
            case BonusEffectType::SUMMON_ON_DEATH: return "SUMMON_ON_DEATH";
            default: return "UNKNOWN";
        }
    }

    inline const char* TraitToString(Trait trait) {
        switch (trait) {
            case Trait::DIVINE: return "DIVINE";
            case Trait::LESSER: return "LESSER";
            case Trait::SOLAR: return "SOLAR";
            case Trait::LUNAR: return "LUNAR";
            case Trait::WATER: return "WATER";
            case Trait::SLAYER: return "SLAYER";
            case Trait::WARRIOR: return "WARRIOR";
            case Trait::DEATH: return "DEATH";
            case Trait::WISDOM: return "WISDOM";
            case Trait::CHAOS: return "CHAOS";
            default: return "UNKNOWN";
        }
    }

    inline const char* GetTraitDescription(Trait trait) {
        switch (trait) {
        case Trait::DIVINE:
            return "Major deities: (2) +2 attack and health, (3) resurrect once";
        case Trait::LESSER:
            return "Minor deities: (3) +3 max health, (5) double gold from combat";
        case Trait::SOLAR:
            return "Sun aligned: (2) +2 damage at day, (4) health regeneration";
        case Trait::LUNAR:
            return "Moon aligned: (2) +4 health at night, (4) +2 attack";
        case Trait::WATER:
            return "Water deities: (1) 30% dodge chance in day";
        case Trait::WARRIOR:
            return "Combat specialists: (3) 50% lifesteal";
        case Trait::SLAYER:
            return "Death knights: (3) double damage";
        case Trait::CHAOS:
            return "Destructive entities: (1) gain +1 attack on any unit death in combat";
        case Trait::WISDOM:
            return "Knowledge deities: (1) +3 gold every turn";
        case Trait::DEATH:
            return "Death themed: (2) summon MedJed on death";
        default:
            return "Unknown trait";
        }
        // Enemy team has traits like Water (30% dodge chance), Warrior (50% lifesteal), Death (summon medjed on death)
        // But player team has full trait list
    }
    
}   // namespace EOREntities