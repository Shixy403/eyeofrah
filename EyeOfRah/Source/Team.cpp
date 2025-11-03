//---------------------------------------------------------
// file:	Team.cpp
// author:	Primary: Leonard (60%), Secondary: Jason (30%), Aryan (10%)
// email:	leonardjunyi.l@digipen.edu, l.jason@digipen.edu, 
//          aryan.b@digipen.edu
//
// brief:	
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#include "pch.hpp"
#include "Team.hpp"
#include "document.h"
#include "filereadstream.h"
#include "CombatSystem.hpp"
#include "TraitManager.hpp"
#include "RandomMap.hpp"
#include "prettywriter.h"
#include "filewritestream.h"

namespace EOREntities
{
    void CopyTeam(Team &source, Team &destination)
    {
        destination = source;
    }

    std::vector<EOREntities::Unit> ReadTeam(int round)
    {
        std::vector<EOREntities::Unit> readTeam{};
        if (round < 0)
            return readTeam;
        FILE *fp;
        std::string sRound = std::to_string(round);
        //std::cout << sRound << std::endl;
        if (fopen_s(&fp, "Assets/teams.json", "rb") == 0)
        {
            char *buffer = new char[65536];
            rapidjson::FileReadStream is(fp, buffer, sizeof(buffer));
            rapidjson::Document document;
            document.ParseStream(is);
            fclose(fp);
            if (!document.HasMember(sRound.c_str()))
                return readTeam;
            rapidjson::Value &roundTeams = document[sRound.c_str()].GetArray();
            assert(roundTeams.IsArray());

            // select random team out of possible teams
            std::random_device rd;
            std::default_random_engine generator(rd());
            std::uniform_int_distribution<int> distribution(0, static_cast<int>(roundTeams.Size() - 1));
            int index = distribution(generator);
            const rapidjson::Value& selectedteam = roundTeams[index].GetArray();
            assert(selectedteam.IsArray());
            for (rapidjson::SizeType i = 0; i < selectedteam.Size(); ++i)
            {
                readTeam.push_back(unitDatabase[EOREntities::stringToEnum(selectedteam[i].GetString())]);
            }

            // add player team to array
            if (PlayerManager::GetInstance()->GetPlayerRef().GetTeam().currentTeamSize > 0 &&
                round > 1 && round < MapGraph::GetMapHeight() &&
                fopen_s(&fp, "Assets/teams.json", "wb") == 0) {
                rapidjson::Value playerTeam(rapidjson::kArrayType);
                for (const EOREntities::Unit& u : PlayerManager::GetInstance()->GetPlayerRef().GetTeam().units) {
                    if (!u.isValid) continue;
                    // in-place Value parameter
                    playerTeam.PushBack(rapidjson::Value(u.name.c_str(), document.GetAllocator()).Move(), // copy string
                                        document.GetAllocator());
                }

                rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
                roundTeams.PushBack(playerTeam, allocator); // allocator is needed for potential realloc()
                rapidjson::FileWriteStream os(fp, buffer, sizeof(buffer));
                rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(os);
                document.Accept(writer);
                fclose(fp);
            }
            delete[] buffer;
        }
        return readTeam;
    }

    // Fix: Changed from const reference to match the declaration in Team.hpp
    void AddToTeam(Unit unit, Team &team, int index)
    {
        try
        {
            if (index < 0 || index >= MAX_UNITS)
            {
                throw std::out_of_range("Index out of bounds for team");
            }

            if (team.units[index].isValid)
            {
                std::cout << "Slot is already occupied!" << std::endl;
                return;
            }

            // No need to create a copy since it's already passed by value
            // Set important team properties
            unit.teamSlot = index;
            unit.isValid = true;
            unit.SetTeam(&team); // Use the proper setter method instead of direct assignment

            // Place the unit at the specified position
            team.units[index] = unit;

            // Update team size if needed
            if (index >= team.currentTeamSize)
            {
                team.currentTeamSize = (short)index + 1;
            }
            EOREntities::TraitManager::GetInstance().UpdateTraits(PlayerManager::GetInstance()->GetPlayerRef().GetTeam());


            printf("Added unit %s to team at position %d (team size: %d)\n",
                   unit.name.c_str(), index, team.currentTeamSize);
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "Caught exception: " << e.what() << std::endl;
        }
    }

    void ReplaceUnitInTeam(Unit unit, Team &team, int index)
    {
        try
        {
            if (index < 0 || index >= MAX_UNITS)
            {
                throw std::out_of_range("Index out of bounds for team");
            }

            // Assign team slot and reference
            unit.teamSlot = index;
            unit.SetTeam(&team);

            // Preserve the old unit's position
            AEVec2 replacePosition = {team.units[index].GetTransform().position.x, team.units[index].GetTransform().position.y};

            // Place the copied unit in the specified slot
            team.units[index] = unit;
            team.units[index].SetPosition(replacePosition.x, replacePosition.y); // Restore Position
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "Caught exception: " << e.what() << std::endl;
        }
    }

    void MoveUnit(Team &team, int fromIndex, int toIndex)
    {
        try
        {
            if (fromIndex < 0 || fromIndex >= team.units.size() || toIndex < 0 || toIndex >= team.units.size())
            {
                throw std::out_of_range("Index out of bounds");
            }
            AEVec2 pos1 = team.units[fromIndex].GetTransform().position;
            AEVec2 pos2 = team.units[toIndex].GetTransform().position;
            std::swap(team.units[fromIndex], team.units[toIndex]);
            EOREntities::TraitManager::GetInstance().UpdateTraits(PlayerManager::GetInstance()->GetPlayerRef().GetTeam());

            team.units[fromIndex].teamSlot = fromIndex;
            team.units[toIndex].teamSlot = toIndex;

            std::cout << "Unit that was at Index: " << fromIndex << " is now at Index: " << team.units[toIndex].teamSlot << std::endl;
            std::cout << "Unit that was at Index: " << toIndex << " is now at Index: " << team.units[fromIndex].teamSlot << std::endl;
            team.units[toIndex].SetPosition(pos2.x, pos2.y);
            team.units[fromIndex].SetPosition(pos1.x, pos1.y);
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "Caught an out_of_range exception: " << e.what() << std::endl;
        }
    }

    /**
     * Removes a unit from the team at the specified index.
     * IMPORTANT: After removal, the index will automatically point to what was the next unit,
     * since all subsequent units shift down one position.
     * In combat contexts, DO NOT advance the index further after calling this function.
     */

    void RemoveFromTeam(Team &team, int index)
    {
        try
        {
            if (index < 0 || index >= team.units.size())
            {
                throw std::out_of_range("Index out of bounds for removing unit");
            }

            std::string removedUnitName = team.units[index].name;

            // Simply mark the unit as invalid
            team.units[index].Reset();
            team.currentTeamSize--;

            // Sort to move all valid units to the front
            SortTeam(team, false);

            std::cout << "Removed unit '" << removedUnitName << "' from team at index " << index
                      << ". New team size: " << team.currentTeamSize << std::endl;
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "Caught exception: " << e.what() << std::endl;
        }
    }

    void SortTeam(Team &team, bool emptyFirst)
    {
        std::vector<Unit> occupiedSlots;
        std::vector<Unit> emptySlots;

        // Separate valid and invalid units while keeping their order
        for (int i = 0; i < MAX_UNITS; ++i)
        {
            if (team.units[i].isValid)
            {
                occupiedSlots.push_back(team.units[i]);
            }
            else
            {
                emptySlots.push_back(team.units[i]);
            }
        }

        size_t unitStartingIndex = emptyFirst ? emptySlots.size() : 0;     // If sorting by empty slots first, unit should start being added after the index of the last added emptyslot. Otherwise start at index 0.
        size_t emptyStartingIndex = emptyFirst ? 0 : occupiedSlots.size(); // If sorting by empty slots first, start at index 0. Otherwise, begin after all units have been added.

        // Assign valid units first
        for (size_t i = 0; i < occupiedSlots.size(); ++i)
        {
            ReplaceUnitInTeam(occupiedSlots[i], team, static_cast<int>(unitStartingIndex + i));
        }

        // Assign invalid units to their respective slots
        for (size_t i = 0; i < emptySlots.size(); ++i)
        {
            ReplaceUnitInTeam(emptySlots[i], team, static_cast<int>(emptyStartingIndex + i));
        }
        std::cout << (emptyFirst ? "Team sorted with empty slots at the front!" : "Team sorted with empty slots at the back!") << std::endl;
    }

    void ClearTeam(Team &team)
    {
        team.currentTeamSize = 0;
        for (Unit& unit : team.units)
        {
            unit.Reset();
        }
    }

    void SetTeamSize(Team &team, short newSize)
    {
        if (newSize >= 0 && newSize <= MAX_UNITS)
        {
            team.currentTeamSize = newSize;
        }
    }

    // New position management functions (PositionManager.cpp implemented before this existed)
    void SaveTeamPositions(const Team &team, Transform *globalPositions)
    {
        // Save positions to provided array
        for (int i = 0; i < team.currentTeamSize && i < MAX_UNITS; i++)
        {
            if (team.units[i].isValid)
            {
                globalPositions[i] = team.units[i].GetTransform();
                printf("Saved unit %d position: x=%f, y=%f\n", i,
                       team.units[i].GetTransform().position.x,
                       team.units[i].GetTransform().position.y);
            }
        }
    }

    void ApplyTeamPositions(Team &team, const Transform *globalPositions)
    {
        // Apply positions if the team has units
        for (int i = 0; i < team.currentTeamSize && i < MAX_UNITS; i++)
        {
            if (team.units[i].isValid)
            {
                team.units[i].SetPosition(globalPositions[i].position.x, globalPositions[i].position.y);
                printf("Applied position to unit %d: x=%f, y=%f\n", i,
                       globalPositions[i].position.x, globalPositions[i].position.y);
            }
        }
    }

    // Function to handle unit death and reposition team
    void HandleUnitDeath(Team &team, int &combatIndex, float newXPos, float newNextXPos)
    {
        std::string deadUnitName = team.units[combatIndex].name;
        team.units[combatIndex].Reset();
        team.currentTeamSize--;
        SortTeam(team, false);
        // Go back to the first
        if (combatIndex >= team.currentTeamSize)
        {
            combatIndex = 0;
        }

        // Update positions based on the new team arrangement
        for (int i = 0; i < team.currentTeamSize; ++i)
        {
            float xPos = (i == 0) ? newXPos : newNextXPos;
            team.units[i].SetPosition(xPos, team.units[i].GetTransform().position.y);
        }
        //AudioManager::GetInstance().PlaySFX("singleUnitDeath");
        std::cout << "Unit '" << deadUnitName << "' has died and been removed from combat." << std::endl;
    }
    /**
     * IMPORTANT: Both Hurt() and ProcessUnitDeath() are potentially
     * trying to mark the unit as dead, creating a race condition.
     *
     */
    // Function to handle both teams' units' death simultaneously
    void HandleBothTeamUnitsDeath(Team &playerTeam, Team &enemyTeam, int &pIndex, int &eIndex,
                                  float playerNewX1, float playerNewX2, float enemyNewX1, float enemyNewX2)
    {
        // Store copies of units before removal
        Unit playerUnitCopy = playerTeam.units[pIndex];
        Unit enemyUnitCopy = enemyTeam.units[eIndex];

        printf("Both units died simultaneously: %s and %s\n",
               playerUnitCopy.name.c_str(), enemyUnitCopy.name.c_str());

        // Call death effects before removal
        playerTeam.units[pIndex].OnDeath();
        enemyTeam.units[eIndex].OnDeath();
        //AudioManager::GetInstance().PlaySFX("bothUnitDeath");
        // Process player team first to avoid index confusion
        RemoveFromTeam(playerTeam, pIndex);
        RemoveFromTeam(enemyTeam, eIndex);

        // Adjust indices if needed
        if (pIndex >= playerTeam.currentTeamSize)
        {
            pIndex = 0;
        }

        if (eIndex >= enemyTeam.currentTeamSize)
        {
            eIndex = 0;
        }

        // Reposition player team
        for (int i = 0; i < playerTeam.currentTeamSize; ++i)
        {
            float xPos = (i == 0) ? playerNewX1 : playerNewX1 + (playerNewX2 - playerNewX1) * i;
            playerTeam.units[i].SetPosition(xPos, playerTeam.units[i].GetTransform().position.y);
            printf("Repositioned player unit %d to x=%f\n", i, xPos);
        }

        // Reposition enemy team
        for (int i = 0; i < enemyTeam.currentTeamSize; ++i)
        {
            float xPos = (i == 0) ? enemyNewX1 : enemyNewX1 + (enemyNewX2 - enemyNewX1) * i;
            enemyTeam.units[i].SetPosition(xPos, enemyTeam.units[i].GetTransform().position.y);
            printf("Repositioned enemy unit %d to x=%f\n", i, xPos);
        }
    }

    void RandomizeTeam(Team &team, bool shouldRandomEmpty, int fixedEmptySlots, int maxTier)
    {
        LoadUnitDataIntoPool();

        if (GetUnitData().empty())
        {
            printf("Error: petsDatabase is empty. Load data first.\n");
            return;
        }

        std::vector<const Unit *> unitPool;

        // Conditional Filtering (Also excludes NO_UNIT and UNIT_COUNT)
        for (size_t i = 1; i < unitDatabase.size() - 1; ++i)
        {
            if (i == UnitType::MUMMY) break; // Stops filtering upon encountering the first summoned unit
            if (unitDatabase[i].tier <= maxTier)
            {
                unitPool.push_back(&unitDatabase[i]);
            }
        }

        ClearTeam(team);
        int remainingEmptySlots = fixedEmptySlots;
        for (size_t i = 0; i < MAX_UNITS; ++i)
        {
            int randomIndex = rand() % unitPool.size();
            AddToTeam(*unitPool[randomIndex], team, static_cast<int>(i));
            team.units[i].SetSprite(unitPool[randomIndex]->GetSprite());
            if (remainingEmptySlots != 0)
            {
                team.units[i].isValid = false;
                --remainingEmptySlots;
                continue;
            }

            if (shouldRandomEmpty && (fixedEmptySlots == 0))
            {
                // Randomly set a empty unit
                if (AERandFloat() <= 0.3f)
                {
                    team.units[i].isValid = false;
                    std::cout << "Empty Slot Added!" << std::endl;
                }
            }
        }

        std::cout << "Player Team Randomized" << std::endl;
    }
}