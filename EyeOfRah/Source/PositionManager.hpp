/******************************************************************************/
/*!
\file           PositionManager.hpp
\project        Eye of Rah
\author(s)      [Leonard], 100% - position storage system, position application, memory management, team integration 

\brief          PositionManager provides centralized storage and retrieval of unit positions.
                Key functionality includes:
                - Maintaining separate arrays (playerPositions, enemyPositions) to store Transform data
                - Position access via GetPlayerPosition() and GetEnemyPosition() functions
                - Position updates through SetPlayerPosition() and SetEnemyPosition() with bound checks
                - Team-wide operations with SavePlayerPositions() and ApplyPlayerPositions()
                - Memory management with Initialize() and FreePositions()
                
                Note: This system was developed alongside Team and Unit implementations and contains
                some parallel functionality to Team.cpp's position management. It serves as the
                centralized position authority across game states.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#pragma once

#include "Core.hpp"
#include "Team.hpp"

namespace PositionManager {
    // Initialize the position manager
    void Initialize();
    
    // Position accessor functions
    Transform GetPlayerPosition(int index);
    Transform GetEnemyPosition(int index);
    
    // Position setter functions
    void SetPlayerPosition(int index, const Transform& transform);
    void SetEnemyPosition(int index, const Transform& transform);
    
    // Save all positions from a team
    void SavePlayerPositions(const EOREntities::Team& team);
    void SaveEnemyPositions(const EOREntities::Team& team);
    
    // Apply saved positions to a team
    void ApplyPlayerPositions(EOREntities::Team& team);
    void ApplyEnemyPositions(EOREntities::Team& team);
    
    // Get the total number of stored positions
    int GetPlayerPositionCount();
    int GetEnemyPositionCount();

    void FreePositions();
}