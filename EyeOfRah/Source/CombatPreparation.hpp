#pragma once
/******************************************************************************/
/*!
\file           CombatPreparation.hpp
\project        Eye of Rah
\author(s)      [Leonard], 80% - unit drag and drop system, team management, animation for possible unit swap using green DrawQuad to pulse
                [Jason], 10% - minor changes to properly link to Combat System

\brief          CombatPreparation handles the pre-combat setup phase where players can:
                - Rearrange units via drag and drop using IsMouseOverEntity() and HandleUnitSwap()
                - Reset positions with ResetUnitPositions() using originalPlayerPositions[]
                - Review enemy formations before entering combat
                - Transition to the combat stage through GoNext() which transfers teams to CombatSystem

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#include "pch.hpp"
#include "GameStateManager.hpp"
#include "Team.hpp"
#include "Unit.hpp"
#include "Widgets.hpp"
#include "PresetWidgets.hpp"
#include "CombatSystem.hpp"
#include "PlayerManager.hpp"
#include <algorithm>


namespace CombatPreparation {
    // ======== Constants ========
    extern const float DRAG_SCALE_MULTIPLIER;
    extern const float RETURN_ANIMATION_DURATION;
    extern const float DRAG_START_DELAY;
    extern const float PULSE_ANIMATION_SPEED;
    
    // ======== Variables ========
    extern EOREntities::Team playerTeam;
    extern EOREntities::Team enemyTeam;
    extern EOREntities::Team initialPlayerTeam;
    extern Transform originalPlayerPositions[EOREntities::MAX_UNITS];
    extern bool initialTeamStored;
    extern bool isRearrangePhase;
    extern Transform playerArea;
    
    // ======== Utility Functions ========
    // Coordinate conversion and detection
    AEVec2 ConvertScreenToWorld(s32 screenX, s32 screenY);
    bool IsMouseOverEntity(const ::EntityObject& entity);
    bool DoEntitiesOverlap(const ::EntityObject& entity1, const ::EntityObject& entity2);
    
    // ======== Team Management Functions ========
    void ResetUnitPositions();
    void HandleUnitSwap(int index1, int index2);
    void LoadPlayerTeamFromManager();
    void LoadRandomEnemyTeam(Transform enemyPositions[]);
    
    // ======== Constraint Functions ========
    void ConstrainEntityPosition(Transform& transform);
    
    // ======== Core System Functions ========
    void PrepareArrangementStage();
    void InitializeUI();
    void HandleInputs();
    void RenderStage();
    
    // Game state management functions
    void Load();
    void Initialize();
    void Update();
    void Draw();
    void Free();
    void Unload();
    
    // External systems can use these to modify state
    void SetPlayerTeam(const ::Team& team);
    void SetEnemyTeam(const ::Team& team);
    void Reset();
    bool IsInRearrangePhase();
}