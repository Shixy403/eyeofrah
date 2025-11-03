#pragma once
/******************************************************************************/
/*!
\file           CombatSystem.hpp
\project        Eye of Rah
\author(s)      Primary: Leonard (60%), Secondary: Jason (15%), Lebon (10%), Aryan (15%)
\email:	        leonardjunyi.l@digipen.edu, l.jason@digipen.edu, h.xinyulebon@digipen.edu, aryan.b@digipen.edu

\brief          CombatSystem handles the turn-based combat between player and enemy teams.
                This system manages the core battle mechanics including:
                - Processing attacks with ProcessCombatEffects() which calculates playerDamage and enemyDamage
                - Managing animations through UpdateCombatAnimation() and AnimationState enum (IDLE, ATTACKING, etc.)
                - Handling unit deaths via ProcessUnitDeath() and RepositionTeamUnits() functions
                - Integrating with TraitManager via ApplyRecurringTraitEffects() and CastSkill functions
                - Determining outcomes in HandleCombatEnd() which sets playerWon and result text variables
                
                The system uses copies for simultaneous deaths between teams and ensures original team is not modified.
                The system was designed to be a simple helper for combat, but went out of hand and became a full-fledged combat system.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#include "Team.hpp"
#include "Unit.hpp"
#include "Graphics.hpp"
#include <random>


namespace CombatSystem {
    // ======== Team Management ========
    void SetPlayerTeam(const EOREntities::Team& team);
    void SetEnemyTeam(const EOREntities::Team& team);
    EOREntities::Team& GetPlayerTeam();
    EOREntities::Team& GetEnemyTeam();
    
    // Combat-related structures
    struct DamageResult {
        int playerDamageDealt = 0;
        int enemyDamageDealt = 0;
        int playerNewHP = 0;
        int enemyNewHP = 0;
        bool playerWillDie = false;
        bool enemyWillDie = false;
    };
    
    // Combat flow functions
    void CastSkillBeforeCombat(Team& playerTeam, Team& enemyTeam);
    void CastSkillDuringCombat(Unit& playerTeam, Unit& enemyTeam, int& playerAttackPoint, int& enemyAttackPoint);
    bool StartCombat();
    void HandleCombatEnd(bool& playerWon, const char*& resultText, const char*& subText);

    void InitializeCombatSystem();
                    
    // Death processing functions
    void ProcessUnitDeath(Team& team, int& index, float frontPos, float backPos);   // Done via Team.cpp
    void RepositionTeamUnits(Team& team, float frontPos, float backPos);

    // Animation and speed control
    void SetAnimationSpeed(float speedMultiplier);
    void SetSkipAnimations(bool skip);
    int GetCurrentAnimationState();

    void RenderTraitUI();
}