/******************************************************************************/
/*!
\file           PositionManager.cpp
\project        Eye of Rah
\author(s)      [Leonard], 100% - position management implementation, memory handling

\brief          Implementation of Position Management API for storing and retrieving unit positions.
                Provides central access to unit positioning data across game states including:
                - Memory management with Initialize() and FreePositions()
                - Position storage with GetPlayerPosition() and GetEnemyPosition()
                - Team Operations through SavePlayerPositions() and ApplyPlayerPositions()

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#include "pch.hpp"
#include "PositionManager.hpp"
#include "Team.hpp"
#include <cassert>


namespace PositionManager {
    namespace {
        // Internal storage for positions
        Transform* playerPositions = nullptr;
        Transform* enemyPositions = nullptr;
        int playerPositionCount = 0;
        int enemyPositionCount = 0;
    }
    
    void Initialize() {
        // Clean up any previous allocation
        FreePositions();
        
        // Allocate based on MAX_UNITS
        playerPositions = new Transform[EOREntities::MAX_UNITS];
        enemyPositions = new Transform[EOREntities::MAX_UNITS];
        
        // Initialize with default values
        for (int i = 0; i < EOREntities::MAX_UNITS; i++) {
            playerPositions[i] = Transform();
            enemyPositions[i] = Transform();
        }
        
        playerPositionCount = 0;
        enemyPositionCount = 0;
    }
    
    Transform GetPlayerPosition(int index) {
        if (index >= 0 && index < EOREntities::MAX_UNITS && index < playerPositionCount) {
            return playerPositions[index];
        }
        return Transform(); // Default transform if out of bounds
    }
    
    Transform GetEnemyPosition(int index) {
        if (index >= 0 && index < EOREntities::MAX_UNITS && index < enemyPositionCount) {
            return enemyPositions[index];
        }
        return Transform(); // Default transform if out of bounds
    }
    
    void SetPlayerPosition(int index, const Transform& transform) {
        // Auto-initialize if needed
        if (playerPositions == nullptr) {
            Initialize();
        }
        
        if (index >= 0 && index < EOREntities::MAX_UNITS) {
            playerPositions[index] = transform;
            
            // Update position count if needed
            if (index >= playerPositionCount) {
                playerPositionCount = index + 1;
            }
            
        }
    }
    
    void SetEnemyPosition(int index, const Transform& transform) {
            // Auto-initialize if needed
        if (enemyPositions == nullptr) {
            Initialize();
        }

        if (index >= 0 && index < EOREntities::MAX_UNITS) {
            enemyPositions[index] = transform;
            
            // Update position count if needed
            if (index >= enemyPositionCount) {
                enemyPositionCount = index + 1;
            }
            
        }
    }
    
    void SavePlayerPositions(const EOREntities::Team& team) {
        playerPositionCount = team.currentTeamSize;
        
        for (int i = 0; i < team.currentTeamSize && i < EOREntities::MAX_UNITS; i++) {
            if (team.units[i].isValid) {
                SetPlayerPosition(i, team.units[i].GetTransform());
            }
        }
        
        printf("Saved %d player positions\n", playerPositionCount);
    }
    
    void SaveEnemyPositions(const EOREntities::Team& team) {
        enemyPositionCount = team.currentTeamSize;
        
        for (int i = 0; i < team.currentTeamSize && i < EOREntities::MAX_UNITS; i++) {
            if (team.units[i].isValid) {
                SetEnemyPosition(i, team.units[i].GetTransform());
            }
        }
        
        printf("Saved %d enemy positions\n", enemyPositionCount);
    }
    
    void ApplyPlayerPositions(EOREntities::Team& team) {
        for (int i = 0; i < team.currentTeamSize && i < playerPositionCount; i++) {
            if (team.units[i].isValid) {
                team.units[i].SetTransform(playerPositions[i]);
            }
        }
    }
    
    void ApplyEnemyPositions(EOREntities::Team& team) {
        for (int i = 0; i < team.currentTeamSize && i < enemyPositionCount; i++) {
            if (team.units[i].isValid) {
                team.units[i].SetTransform(enemyPositions[i]);
            }
        }
    }
    
    int GetPlayerPositionCount() {
        return playerPositionCount;
    }
    
    int GetEnemyPositionCount() {
        return enemyPositionCount;
    }

    void FreePositions() {
        if (playerPositions) {
            delete[] playerPositions;
        }

        if (enemyPositions) {
            delete[] enemyPositions;
        }
    }
}