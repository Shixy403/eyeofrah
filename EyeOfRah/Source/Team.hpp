#pragma once
//---------------------------------------------------------
// file:	Team.hpp
// author:	Primary: Jason, Secondary: Leonard, Aryan
// email:	l.jason@digipen.edu, leonardjunyi.l@digipen.edu, aryan.b@digipen.edu
//
// brief:	
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#include "Unit.hpp"
#include <array>

namespace EOREntities {
	const int MAX_UNITS = 5;
	const int MAX_COMSUMABLES = 2;
	struct Team {
		std::array<Unit, MAX_UNITS> units;
		short currentTeamSize = 0;
	};

	// Copies all units from 1 team over to another other team.
	void CopyTeam(Team& source, Team& destination);
	// Reads a random team from file for loading into enemy team,
	// also writes player team to file
	std::vector<EOREntities::Unit> ReadTeam(int round);
	void AddToTeam(Unit unit, Team& team, int index = 0);
	void ReplaceUnitInTeam(Unit unit, Team& team, int index = MAX_UNITS);
	void MoveUnit(Team& team, int fromIndex, int toIndex);
	// void SwapUnitData(Team& team, int index1, int index2);	// Replaced call in CombatPreparation to use MoveUnit
	void RemoveFromTeam(Team& team, int index);
	void SortTeam(Team& team, bool emptyFirst = false);
	void ClearTeam(Team& team);
	void SetTeamSize(Team& team, short newSize);
	
	// Position management functions
	void SaveTeamPositions(const Team& team, Transform* globalPositions);
	void ApplyTeamPositions(Team& team, const Transform* globalPositions);
	void HandleUnitDeath(Team& team, int& combatIndex, float newXPos, float newNextXPos);
	void HandleBothTeamUnitsDeath(Team& playerTeam, Team& enemyTeam, int& pIndex, int& eIndex,
	                             float playerNewX1, float playerNewX2, float enemyNewX1, float enemyNewX2);
	void RandomizeTeam(Team& team, bool shouldRandomEmpty = true, int fixedEmptySlots = 0, int maxTier = 3);
}