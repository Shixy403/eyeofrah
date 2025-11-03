//---------------------------------------------------------------------------------------
// file:	CombatSystem.cpp
// author:	Primary: Leonard (60%), Secondary: Jason (15%), Lebon (10%), Aryan (15%)
// email:	leonardjunyi.l@digipen.edu, l.jason@digipen.edu, h.xinyulebon@digipen.edu
//          aryan.b@digipen.edu
//
// brief:	This file contains definitions for the Combat System APIs. Combat during
//          Combat Stage is processed and handled here.
//
// Copyright � 2025 DigiPen, All rights reserved.
//---------------------------------------------------------------------------------------
#include "pch.hpp"
#include "CombatSystem.hpp"
#include "Team.hpp"
#include "Unit.hpp"
#include "Item.hpp"
#include "Utils.hpp"
#include "document.h"
#include "filereadstream.h"
#include "RandomMap.hpp"
#include "PositionManager.hpp"
#include "Animation.hpp"
#include "TraitManager.hpp"
#include "Traits.hpp"
#include <AEEngine.h>
#include <cassert>
#include <iostream>

namespace CombatSystem {
    // declaring everything inside as static to prevent external access + internal linkage (file scope only)
    namespace {
        // Copies of teams from Combat Prep. Only Copies of teams are fighting
        Team playerBattleTeam;
        Team enemyBattleTeam;

        std::array<AEVec2, MAX_UNITS> playerPositions;
        std::array<AEVec2, MAX_UNITS> enemyPositions;

        int currentRound = 0;
        
#pragma region ANIM_VAR
        // Animation state variables
        enum class AnimationState : u8 {
            IDLE,
            MOVING_TO_ATTACK,
            ATTACKING,
            RETURNING,
            COMPLETED
        };
        
        AnimationState currentAnimState = AnimationState::IDLE;
        float animationTimer = 0.0f;
        static float animationTimeout = 0.0f;

        bool skipAnimations = false;

        // Animation timing constants
        const float MOVE_TO_ATTACK_DURATION = 0.03f;
        const float ATTACK_DURATION = 0.02f;
        const float RETURN_DURATION = 0.02f;
        const float MAX_ANIMATION_TIME = 3.0f; // Maximum time an animation can take before force completion
        float currentAnimationTime = 0.0f;     // Track how long the current animation has been running

        float animationSpeedMultiplier = 10.0f;
        
        // Store original positions for returning
        AEVec2 playerOriginalPos;
        AEVec2 enemyOriginalPos;
        
        // Store current active units for animation
        Unit* playerAnimUnit = nullptr;
        Unit* enemyAnimUnit = nullptr;
#pragma endregion
    
#pragma region ANIM_FUNCS
        // Update unit animation based on current state
        bool UpdateCombatAnimation(float dt) {
            if (currentAnimState == AnimationState::IDLE || 
                currentAnimState == AnimationState::COMPLETED || 
                !playerAnimUnit || !enemyAnimUnit) {
                return false;
            }
            
            animationTimer += dt * animationSpeedMultiplier;
            float progress = 0.0f;
            
            switch (currentAnimState) {
                case AnimationState::MOVING_TO_ATTACK: {
                    // Move towards each other
                    progress = animationTimer / MOVE_TO_ATTACK_DURATION;
                    if (progress >= 1.0f) {
                        progress = 1.0f;
                        animationTimer = 0.0f;
                        currentAnimState = AnimationState::ATTACKING;
                    }
                    
                    // Calculate middle point (20% of the distance)
                    AEVec2 midPointPlayer = playerOriginalPos;
                    AEVec2 midPointEnemy = enemyOriginalPos;
                    float distanceX = enemyOriginalPos.x - playerOriginalPos.x;
                    
                    midPointPlayer.x += distanceX * 0.15f;
                    midPointEnemy.x -= distanceX * 0.15f;
                    
                    // Apply eased movement
                    float easedProgress = EaseOutCubic(progress);
                    AEVec2 playerNewPos = {};
                    AEVec2Lerp(&playerNewPos, &playerOriginalPos, &midPointPlayer, easedProgress);
                    AEVec2 enemyNewPos = {};
                    AEVec2Lerp(&enemyNewPos, &enemyOriginalPos, &midPointEnemy, easedProgress);
                    
                    playerAnimUnit->SetPosition(playerNewPos.x, playerNewPos.y);
                    enemyAnimUnit->SetPosition(enemyNewPos.x, enemyNewPos.y);
                    break;
                }
                
                case AnimationState::ATTACKING: {
                    // Brief pause at attack position
                    progress = animationTimer / ATTACK_DURATION;
                    if (progress >= 1.0f) {
                        progress = 1.0f;
                        animationTimer = 0.0f;
                        currentAnimState = AnimationState::RETURNING;
                    }
                    // No movement during attack phase, just a pause
                    break;
                }
                
                case AnimationState::RETURNING: {
                    // Return to original positions
                    progress = animationTimer / RETURN_DURATION;
                    if (progress >= 1.0f) {
                        progress = 1.0f;
                        animationTimer = 0.0f;
                        currentAnimState = AnimationState::COMPLETED;
                        
                        // Ensure units are exactly at their original positions
                        playerAnimUnit->SetPosition(playerOriginalPos.x, playerOriginalPos.y);
                        enemyAnimUnit->SetPosition(enemyOriginalPos.x, enemyOriginalPos.y);
                        return true;
                    }
                    
                    // Get current positions (which are mid-attack positions)
                    AEVec2 currentPlayerPos = playerAnimUnit->GetTransform().position;
                    AEVec2 currentEnemyPos = enemyAnimUnit->GetTransform().position;
                    
                    // Apply eased movement for returning
                    float easedProgress = EaseOutCubic(progress);
                    AEVec2 playerNewPos = {};
                    AEVec2Lerp(&playerNewPos, &currentPlayerPos, &playerOriginalPos, easedProgress);
                    AEVec2 enemyNewPos = {};
                    AEVec2Lerp(&enemyNewPos, &currentEnemyPos, &enemyOriginalPos, easedProgress);
                    
                    playerAnimUnit->SetPosition(playerNewPos.x, playerNewPos.y);
                    enemyAnimUnit->SetPosition(enemyNewPos.x, enemyNewPos.y);
                    break;
                }
                
                default:
                    break;
            }
            
            return false;
        }
        
        // Start animation sequence for two units
        void StartAttackAnimation(Unit& playerUnit, Unit& enemyUnit) {
            playerAnimUnit = &playerUnit;
            enemyAnimUnit = &enemyUnit;
            playerOriginalPos = playerUnit.GetTransform().position;
            enemyOriginalPos = enemyUnit.GetTransform().position;
            
            currentAnimState = AnimationState::MOVING_TO_ATTACK;
            animationTimer = 0.0f;
            animationTimeout = 0.0f;
        }
        
        bool IsAnimationCompleted() {
            return currentAnimState == AnimationState::COMPLETED;
        }
    } // End anonymous namespace
#pragma endregion

#pragma region ANIM_CONTROL
    void SetAnimationSpeed(float speedMultiplier) {
        // No negative or zero speed
        if (speedMultiplier <= 0.0f) {
            speedMultiplier = 10.0f;
        }
        
        // Update the internal multiplier
        animationSpeedMultiplier = speedMultiplier;
        printf("Combat animation speed set to %.1fx\n", speedMultiplier);
    }

    void SetSkipAnimations(bool skip) {
        skipAnimations = skip;
    }

    int GetCurrentAnimationState() {
        return static_cast<int>(currentAnimState);
    }
#pragma endregion

#pragma region TEAM_MANAGEMENT
    void SetPlayerTeam(const Team& team) {
        // Make a deep copy of the team to ensure all data is preserved
        ClearTeam(playerBattleTeam);
        playerBattleTeam = team;
        SortTeam(playerBattleTeam);

        for (int i = 0; i < MAX_UNITS; ++i) {
            playerBattleTeam.units[i].SetShowBox(false);
        }
        
        // Update positions in PositionManager instead of globals
        PositionManager::SavePlayerPositions(playerBattleTeam);
        
        // Log that team was updated
        printf("CombatSystem: Player team updated with %d units\n", team.currentTeamSize);
    }

    void SetEnemyTeam(const Team& team) {
        // Make a deep copy of the team
        ClearTeam(enemyBattleTeam);
        enemyBattleTeam = team;
        SortTeam(enemyBattleTeam);

        for (int i = 0; i < MAX_UNITS; ++i) {
            enemyBattleTeam.units[i].SetShowBox(false);
        }
        
        // Update positions in PositionManager instead of globals
        PositionManager::SaveEnemyPositions(enemyBattleTeam);
        
        printf("CombatSystem: Enemy team updated with %d units\n", team.currentTeamSize);
    }
    
    Team& GetPlayerTeam() {
        return playerBattleTeam;
    }
    
    Team& GetEnemyTeam() {
        return enemyBattleTeam;
    }
#pragma endregion

#pragma region DEATH HANDLING
    // Reposition team units after death
    void RepositionTeamUnits(Team& team, float frontPos, float backPos) {
        if (team.currentTeamSize <= 0) {
            return; // No units to reposition
        }
        
        // Position the first unit (frontmost)
        if (team.currentTeamSize >= 1 && team.units[0].isValid) {
            team.units[0].SetPosition(frontPos, 0.0f);
            
            // Update position in PositionManager
            if (&team == &playerBattleTeam) {
                PositionManager::SetPlayerPosition(0, team.units[0].GetTransform());
            } else {
                PositionManager::SetEnemyPosition(0, team.units[0].GetTransform());
            }
        }
        
        // Position the second unit if it exists
        if (team.currentTeamSize >= 2 && team.units[1].isValid) {
            team.units[1].SetPosition(backPos, 0.0f);
            
            // Update position in PositionManager
            if (&team == &playerBattleTeam) {
                PositionManager::SetPlayerPosition(1, team.units[1].GetTransform());
            } else {
                PositionManager::SetEnemyPosition(1, team.units[1].GetTransform());
            }
        }
        
        // Position any additional units if they exist
        for (int i = 2; i < team.currentTeamSize; i++) {
            if (team.units[i].isValid) {
                // Calculate position for additional units (spaced evenly behind second unit)
                float xPos = backPos - (100.0f * (i - 1));
                team.units[i].SetPosition(xPos, 0.0f);
                
                // Update position in PositionManager
                if (&team == &playerBattleTeam) {
                    PositionManager::SetPlayerPosition(i, team.units[i].GetTransform());
                } else {
                    PositionManager::SetEnemyPosition(i, team.units[i].GetTransform());
                }
            }
        }
        
        printf("Team repositioned: front unit at x=%f, second unit at x=%f\n", frontPos, backPos);
    }
    
    // Process death for a single unit - now using Team's HandleUnitDeath
    void ProcessUnitDeath(Team& team, int& index, float frontPos, float backPos) {
        // Leverage the Team functionality to handle unit death
        EOREntities::HandleUnitDeath(team, index, frontPos, backPos);
    }
#pragma endregion

#pragma region COMBAT LOGIC
    static void TriggerOnHitCombatEffects() {
        Unit* charmOwner = nullptr;
        int targetSlot = 0;

        // Items
        for (int i = 0; i < MAX_UNITS; ++i) {
            charmOwner = &playerBattleTeam.units[i];
            targetSlot = 0;

            if (!charmOwner->GetEquippedItem().charmItem) continue;

            switch (charmOwner->GetEquippedItem().charmItem->GetItemID()) {
            case ItemID::ID_COBRA:
                charmOwner->GetEquippedItem().charmItem->TriggerItem(*charmOwner);
                continue;
            default:
                continue;
            }
        }

        for (int i = 0; i < MAX_UNITS; ++i) {
            charmOwner = &enemyBattleTeam.units[i];
            targetSlot = 0;

            if (!charmOwner->GetEquippedItem().charmItem) continue;

            switch (charmOwner->GetEquippedItem().charmItem->GetItemID()) {
            case ItemID::ID_COBRA:
                charmOwner->GetEquippedItem().charmItem->TriggerItem(*charmOwner);
                continue;
            default:
                continue;
            }
        }
    }

    // Helper function to process combat effects and damage
    static void ProcessCombatEffects(Unit& playerUnit, Unit& enemyUnit) {
        // Calculate damage
        int playerDamage = playerUnit.GetAttackPoints();
        int enemyDamage = enemyUnit.GetAttackPoints();

        // For Items later
        int storedPHitPoints = playerUnit.GetBaseHitPoints();
        int storedEHitPoints = enemyUnit.GetBaseHitPoints();

        // require in order for some skills to affect opponent damage
        CastSkillDuringCombat(playerUnit, enemyUnit, playerDamage, enemyDamage);
        
        // Apply damage
        playerUnit.Hurt(enemyDamage);
        enemyUnit.Hurt(playerDamage);
        // Apply lifesteal after damage is dealt

        if (playerUnit.HasTrait(Trait::WARRIOR) && TraitManager::GetInstance().GetTraitLevel(Trait::WARRIOR) >= 3) {
        int healAmount = playerDamage / 2;
        playerUnit.SetHitPoints(playerUnit.GetHitPoints() + healAmount);
        printf("Warrior %s heals for %d from lifesteal!\n", playerUnit.name.c_str(), healAmount);
        }

        // Trigger On Hit events
        TriggerOnHitCombatEffects();

        // Means Enemy Died, trigger on kill events
        if (!enemyUnit.isValid) {
            if (playerUnit.GetEquippedItem().charmItem) {
                if (playerUnit.GetEquippedItem().charmItem->GetItemID() == ItemID::ID_SEKHMETCREST) {
                    std::cout << "Triggering Sekhmet Crest!" << std::endl;
                    playerUnit.SetHitPoints(playerUnit.GetHitPoints() + storedEHitPoints);
                }
            }
        }
        else if (!playerUnit.isValid) {
            if (enemyUnit.GetEquippedItem().charmItem) {
                if (enemyUnit.GetEquippedItem().charmItem->GetItemID() == ItemID::ID_SEKHMETCREST)
                    enemyUnit.SetHitPoints(enemyUnit.GetHitPoints() + storedPHitPoints);
            }
        }
        
        printf("Player unit deals %d damage, Enemy unit deals %d damage\n", playerDamage, enemyDamage);
        ++currentRound;
    }

    // Helper Function for item effects
    static void TriggerItemCombatEffect(Unit& charmOwner, Team& alliedTeam, Team& opposingTeam) {
        UNREFERENCED_PARAMETER(alliedTeam);
        int targetIndex = 0;

        switch (charmOwner.GetEquippedItem().charmItem->GetItemID()) {
        case ItemID::ID_RAEYE:
            targetIndex = charmOwner.teamSlot;
            while (!opposingTeam.units[targetIndex].isValid) --targetIndex;

            if (targetIndex > 0 && targetIndex < MAX_UNITS) {
                opposingTeam.units[targetIndex].Hurt(2, false);
                charmOwner.UnequipItem();
            }
            return;
        default:
            return;
        }
    }

    // Helper function to process combat effects before the first round
    static void ProcessPreCombatEffects() {

        // Process Items that trigger at the start of combat
        Unit* itemOwner = nullptr;

        // By right supposed to do a priority system to determine which items should trigger first. But for now items are triggered based
        // first to last units, with player units triggering them first.
        for (int i = 0; i < MAX_UNITS; ++i) {
            itemOwner = &playerBattleTeam.units[i];
            if (!itemOwner->GetEquippedItem().charmItem) continue;

            TriggerItemCombatEffect(*itemOwner, playerBattleTeam, enemyBattleTeam);
        }

        for (int i = 0; i < MAX_UNITS; ++i) {
            itemOwner = &enemyBattleTeam.units[i];
            if (!itemOwner->GetEquippedItem().charmItem) continue;

            TriggerItemCombatEffect(*itemOwner, enemyBattleTeam, playerBattleTeam);
        }

        for (int i = 0; i < MAX_UNITS; ++i) {
            if (playerBattleTeam.units[i].isValid) {
                playerBattleTeam.units[i].CheckDeath();
            }

            if (enemyBattleTeam.units[i].isValid) {
                enemyBattleTeam.units[i].CheckDeath();
            }
        }

        currentRound = 1;
    }

    void CastSkillDuringCombat(Unit& playerUnit, Unit& enemyUnit, int& playerAttackPoint, int& enemyAttackPoint) {
    
        // to control which unit can cast skill.
        if (playerUnit.name == "apep") {
            if (rand() % 100 + 1 > 80) {
                enemyAttackPoint = 0;
                printf("Enemy is confused by Apep!\n");
            }
        }
        else if (playerUnit.name == "khonsu") {
            playerUnit.CastSkill(playerBattleTeam);
        }

        // to control which unit can cast skill.
        if (enemyUnit.name == "apep") {
            if (rand() % 100 + 1 > 80) {
                playerAttackPoint = 0;
                printf("Player is confused by Apep!\n");
            }
        }
        else if (enemyUnit.name == "khonsu") {
            enemyUnit.CastSkill(enemyBattleTeam);
        }

        
    }

    void CastSkillBeforeCombat(Team& playerTeam, Team& enemyTeam) {
        for (Unit& unit : playerTeam.units) {
            // to control which unit can cast skill.
            if (unit.name == "osiris") {
                unit.CastSkill(playerTeam);
            }
        }

        for (Unit& unit : enemyTeam.units) {
            // to control which unit can cast skill.
            if (unit.name == "osiris") {
                unit.CastSkill(enemyTeam);
            }
        }
    }

    static void SetPositions() {
        for (int i = 0; i < MAX_UNITS; ++i) {
            playerBattleTeam.units[i].SetPosition(playerPositions[i].x, playerPositions[i].y);
            enemyBattleTeam.units[i].SetPosition(enemyPositions[i].x, enemyPositions[i].y);
        }
    }

    void InitializeCombatSystem() {
        currentRound = 0;

        for (int i = 0; i < MAX_UNITS; ++i)
        {
            playerPositions[i] = { -100.f - (150.f * i), 0.f };
            enemyPositions[i] = { 100.f + (150.f * i), 0.f };
        }

        SortTeam(playerBattleTeam);
        SortTeam(enemyBattleTeam);
        SetPositions();
        EOREntities::TraitManager::GetInstance().UpdateTraits(playerBattleTeam);
        // Apply one-time trait bonuses at the start of combat
        EOREntities::TraitManager::GetInstance().ApplyInitialTraitBonuses(playerBattleTeam);
        CastSkillBeforeCombat(playerBattleTeam, enemyBattleTeam);
    }
    
    // Starts the Combat Round
    bool StartCombat() {
        // Expicitly intiailize animation state to reset after 1 round of combat
        if (currentAnimState == AnimationState::COMPLETED) {
            currentAnimState = AnimationState::IDLE;
            animationTimer = 0.0f;
            playerAnimUnit = nullptr;
            enemyAnimUnit = nullptr;
            // Give the system a moment to finalize position changes
            printf("Animation reset and ready for next combat round\n");
        }

        // --- PART 1: EARLY EXIT CHECKS ---
        
        // Check player & enemy team units' health and validity
        bool allPlayerUnitsDefeated = true;
        bool allEnemyUnitsDefeated = true;

        Unit* playerUnit = nullptr;
        Unit* enemyUnit = nullptr;

        for (int i = 0; i < EOREntities::MAX_UNITS; ++i) {
            playerUnit = &playerBattleTeam.units[i];
            enemyUnit = &enemyBattleTeam.units[i];

            if (playerUnit->isValid && playerUnit->GetHitPoints() > 0) {
                allPlayerUnitsDefeated = false;
            }

            if (enemyUnit->isValid && enemyUnit->GetHitPoints() > 0) {
                allEnemyUnitsDefeated = false;
            }
        }

        // If any team has all units at 0 HP, end combat
        if (allPlayerUnitsDefeated || allEnemyUnitsDefeated) {
            
            printf("Combat has ended based on health check! Winner: %s team\n", 
                !allPlayerUnitsDefeated ? "Player" : "Enemy");
            return true; // Combat has ended
        }

        if (currentRound == 0) {
            ProcessPreCombatEffects();
            return false;
        }
        
        // --- PART 2: ANIMATION HANDLING ---
        
        // Handle animation skipping
        if (skipAnimations) {
            currentAnimState = AnimationState::COMPLETED;
        }
        
        // Handle animation progress without timeout
        if (currentAnimState != AnimationState::IDLE && currentAnimState != AnimationState::COMPLETED) {
            // Update animation
            bool animationComplete = UpdateCombatAnimation(static_cast<float>(AEFrameRateControllerGetFrameTime()));
            if (!animationComplete) {
                return false; // Still animating, combat continues
            }
        }
        
        // --- PART 3: SORT UNITS WITH EMPTY SLOTS TO THE BACK ---
        
        SortTeam(playerBattleTeam);
        SortTeam(enemyBattleTeam);
        SetPositions();
        playerUnit = &playerBattleTeam.units[0];
        enemyUnit = &enemyBattleTeam.units[0];
        
        // --- PART 4: COMBAT STATE MACHINE ---
        
        switch (currentAnimState) {
            case AnimationState::IDLE:
                // Start new combat round
                printf("\n---- COMBAT ROUND START ----\n");
                printf("Player Unit: %s (HP: %d, ATK: %d) vs Enemy Unit: %s (HP: %d, ATK: %d)\n",
                       playerUnit->name.c_str(), playerUnit->GetHitPoints(), playerUnit->GetAttackPoints(),
                       enemyUnit->name.c_str(), enemyUnit->GetHitPoints(), enemyUnit->GetAttackPoints());
                
                // Start animation
                StartAttackAnimation(*playerUnit, *enemyUnit);
                return false; // Combat continues
                
            case AnimationState::COMPLETED:
                // Apply combat effects
                ProcessCombatEffects(*playerUnit, *enemyUnit);
                if (playerUnit->GetHitPoints() <= 0 || enemyUnit->GetHitPoints() <= 0) 
                    EOREntities::TraitManager::GetInstance().ApplyRecurringTraitEffects(playerBattleTeam);

                // Check player & enemy team units' health and validity
                allPlayerUnitsDefeated = true;
                allEnemyUnitsDefeated = true;

                playerUnit = nullptr;
                enemyUnit = nullptr;

            #pragma region CHECK_DEFEATED
                for (int i = 0; i < EOREntities::MAX_UNITS; ++i) {
                    playerUnit = &playerBattleTeam.units[i];
                    enemyUnit = &enemyBattleTeam.units[i];

                    if (playerUnit->isValid && playerUnit->GetHitPoints() > 0) {
                        allPlayerUnitsDefeated = false;
                    }

                    if (enemyUnit->isValid && enemyUnit->GetHitPoints() > 0) {
                        allEnemyUnitsDefeated = false;
                    }
                }

                // If any team has all units at 0 HP, end combat
                if (allPlayerUnitsDefeated || allEnemyUnitsDefeated) {
                    printf("Combat has ended based on health check! Winner: %s team\n",
                        !allPlayerUnitsDefeated ? "Player" : "Enemy");
                    return true; // Combat has ended
                }
            #pragma endregion
                
                // Reset for next round
                currentAnimState = AnimationState::IDLE;
                playerAnimUnit = nullptr;
                enemyAnimUnit = nullptr;
                return false; // Combat continues
              
            default:
                // Shouldn't be an occurrence
                printf("Warning: Unexpected animation state %d\n", static_cast<int>(currentAnimState));
                return false; // Combat continues
        }
    }

    void HandleCombatEnd(bool& playerWon, const char*& resultText, const char*& subText) {
        
        int playerValidUnits = 0;
        int enemyValidUnits = 0;
        
        Unit* playerUnit = nullptr;
        Unit* enemyUnit = nullptr;

        for (int i = 0; i < EOREntities::MAX_UNITS; ++i) {
            playerUnit = &playerBattleTeam.units[i];
            enemyUnit = &enemyBattleTeam.units[i];

            if (playerUnit->isValid) {
                ++playerValidUnits;
            }

            if (enemyUnit->isValid) {
                ++enemyValidUnits;
            }
        }
        
        // Player wins if they have more valid units
        playerWon = (playerValidUnits > enemyValidUnits);
        
        if (playerWon) {
            resultText = "VICTORY";
            if (playerValidUnits > enemyValidUnits) {
                subText = "Your units have triumphed!";
                AudioManager::GetInstance().PlaySFX("victory");
            }
        } else {
            resultText = "DEFEAT";
            if (playerValidUnits == 0) {
                subText = "Enemy forces overwhelmed you!";
                AudioManager::GetInstance().PlaySFX("defeated");
            }
        }
        
        printf("Combat ended: Player units: %d, Enemy units: %d, Player Won: %s\n",
               playerValidUnits, enemyValidUnits, playerWon ? "Yes" : "No");
    }
#pragma endregion

    void RenderTraitUI() {
        using namespace EOREntities;
        TraitManager& traitManager = TraitManager::GetInstance();
        std::vector<std::pair<Trait, int>> activeTraits = traitManager.GetActiveTraits();  // true for player team

        if (activeTraits.empty()) return;

        float x = -550.0f;  // Left side of screen 
        float y = 300.0f;   // Top area

        // Draw title
        Fonts_DrawText("ACTIVE TRAITS", x, y, 24, defaultFont);
        y -= 40.0f;

        // Draw each active trait - avoid structured bindings for C++14 compatibility
        for (size_t i = 0; i < activeTraits.size(); ++i) {
            Trait trait = activeTraits[i].first;
            int level = activeTraits[i].second;

            // Fix string concatenation with const char*
            std::string traitText = std::string(TraitToString(trait)) + " (" + std::to_string(level) + ")";
            Fonts_DrawText(traitText.c_str(), x, y, 20.f, defaultFont);

            std::string description = traitManager.GetTraitDescription(trait);
            Fonts_DrawText(description.c_str(), x + 20.0f, y - 25.0f, 16.f, defaultFont);

            y -= 60.0f;
        }
        for (int i = 0; i < MAX_UNITS; ++i) {
            playerBattleTeam.units[i].SetShowBox(true);
            enemyBattleTeam.units[i].SetShowBox(true);
        }
    }

} // End CombatSystem namespace