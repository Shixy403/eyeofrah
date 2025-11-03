/******************************************************************************/
/*!
\file           CombatStage.hpp
\project        Eye of Rah
\author(s)      Primary: Leonard (40%), Secondary: Jason (40%), Aryan (30%)
				combat system integration, animation handling, game state management, UI implementation
				trait system integration, victory/defeat logic, battle rendering
                UI elements, audio integration, input handling

\brief          Main combat gameplay involving:
                - Turn-based combat
                - Toggle between pause and double-speed with ActivateDoubleSpeed()
                - Trait system which involves bonuses based on unit collection of the same trait category
                - Results are displayed with rewards/penalties based on outcome
                - Victory transitions to world map, game over if health is too low

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#pragma once
namespace CombatStage {
	void Load();

	void Initialize();

	void Update();

	void Draw();

	void Free();

	void Unload();
}