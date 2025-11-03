/******************************************************************************/
/*!
\file           CombatStage.cpp
\project        Eye of Rah
\author(s)      Primary: Leonard (40%), Secondary: Jason (40%), Aryan (30%)
				combat system integration, animation handling, game state management, UI implementation
				trait system integration, victory/defeat logic, battle rendering
                UI elements, audio integration, input handling

\brief          CombatStage implements the turn-based combat system where:
                - Combat unfolds with CombatSystem::StartCombat() executing each turn
                - UI controls allow pausing (ActionButtonEvent()) and speed adjustment (ActivateDoubleSpeed())
                - Active traits are updated and displayed on Top Left Screen
                - Combat animations to visualize unit actions
                - Victory rewards players with gold (doubled with certain traits)
                - Defeat applies health penalties based on surviving enemy units
                - Post combat, transitions to WorldMap or GameOver based on player health

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/

#include <iostream>
#include <fstream>
#include "pch.hpp"
#include "CombatStage.hpp"
#include "GameStateManager.hpp"
#include "Team.hpp"
#include "CombatSystem.hpp"
#include "TraitManager.hpp"
#include "document.h"
#include "filereadstream.h"
#include "Unit.hpp"
#include "Widgets.hpp"
#include "PresetWidgets.hpp"
#include "RandomMap.hpp"
#include "Animation.hpp"

namespace CombatStage {
	// ---------------------------------------------------------------------------
	// Variables
	// ---------------------------------------------------------------------------
	f32 TimeElapsed = 0.0f;
	// Remove the local definitions of Transform variables
	Transform battleRow;

	LGCY_CP::LG_Color sandColor;

	// ---------------------------------------------------------------------------
	// System
	// ---------------------------------------------------------------------------
	EORWidgets::Button actionBtn, autoBtn, fastForwardBtn;
	static EORWidgets::TextBox* victoryTextBox = nullptr;
	static EORWidgets::TextBox* subTextBox = nullptr;
	static EORWidgets::TextBox* traitTitleBox = nullptr;
	static std::vector<EORWidgets::TextBox*> traitTextBoxes;
	static float traitUIRefreshTimer = 0.0f;
	//EORWidgets::GridLayout gridLayout1(3, 5, 0.f, 0.f, { 20.f, 20.f }, { 10.f, 10.f }, Graphics::NO_SPRITE, 0.f, 500.f, 400.f);
	bool isAutoEnabled = false, isPaused = false;
	float doubleSpeed = 1.0f; // For fast forward functionality
	// ---------------------------------------------------------------------------
	// Battle
	// ---------------------------------------------------------------------------
	using namespace EOREntities;
	bool combatEnded = false;
	bool showResultText = false;
	bool playerWon = false;
	float resultTextTimer = 0.0f;
	const float RESULT_DISPLAY_TIME = 3.0f;
	const char* resultText = "";
	const char* subText = "";
	float textX, textY;
	Transform combatBg{};
	// ---------------------------------------------------------------------------
	// Input
	// ---------------------------------------------------------------------------

	// Add these variables at the top with other globals
	static float combatTimer = 0.0f;
	// Player and preset enemy teams will be read from 
	// playerStats.json to determine team size.


	// ---------------------------------------------------------------------------
	// Functions
	// ---------------------------------------------------------------------------
	// BATTLE
#pragma region INPUT STUFF
	static void HandleInputs() {

		for (Unit& unit : CombatSystem::GetPlayerTeam().units) {
			if (!unit.isValid) continue;
			unit.OnHover();
		}

		if (AEInputCheckTriggered(AEVK_LBUTTON)) {
			std::cout << "Left click" << std::endl;
			actionBtn.OnClick();
			autoBtn.OnClick();
			fastForwardBtn.OnClick();
		}
	}
	static void ActivateDoubleSpeed() {
		// Toggle between normal and double speed
		if (doubleSpeed == 1.0f) {
			doubleSpeed = 2.0f;
			fastForwardBtn.SetFontColor(LGCY_CP::LG_Color::yellow());
			// Also speed up animations
			// CombatSystem::SetAnimationSpeed(20.0f);
			// CombatSystem::SetSkipAnimations(false);
		}
		else {
			doubleSpeed = 1.0f;
			fastForwardBtn.SetFontColor(LGCY_CP::LG_Color::white());
			// Reset animation speed
			// CombatSystem::SetAnimationSpeed(10.0f);
			// CombatSystem::SetSkipAnimations(false);
		}
		printf("Combat speed set to %.1fx\n", doubleSpeed);
	}
#pragma endregion

	// WORLD
#pragma region WORLD STUFF
	// Initializes and prepares transformation matrix for the zones
	static void InitWorldTransforms() {
		battleRow.position = { 0.f };
		battleRow.rotation = 0;
		battleRow.scale = { 1600.f, 100.f };
		sandColor = LGCY_CP::LG_Color(194, 178, 128, 255);
	}

	// Draws/Renders the zones
	static void RenderWorld() {
		DrawQuad(sandColor.a, sandColor.g, sandColor.b, sandColor.a, battleRow);
	}
#pragma endregion

	// User Interface
#pragma region UI STUFF
	static void ActionButtonEvent() {
		isPaused = !isPaused;
		if (!isPaused) {
			actionBtn.SetContentImage(Graphics::COMBAT_PAUSEBUTTON);
		}
		else {
			if (isAutoEnabled) {
				actionBtn.SetContentImage(Graphics::COMBAT_PLAYBUTTON);
			}
			else {
				actionBtn.SetContentImage(Graphics::COMBAT_NEXTBUTTON);
			}
		}
	}

	static void ToggleAutoMode() {
		isAutoEnabled = !isAutoEnabled;

		if (isAutoEnabled) {
			autoBtn.SetFontColor(LGCY_CP::LG_Color::yellow());
		}
		else {
			autoBtn.SetFontColor(LGCY_CP::LG_Color::white());
		}
	}

	static void InitUI() {
		using namespace Graphics;
		using namespace LegacyProcessing;

		int windowWidth = AEGfxGetWindowWidth();
		int windowHeight = AEGfxGetWindowHeight();
		float minGap = windowWidth * 0.05f;
		float worldPositionX = ScreenToWorldPosition(static_cast<s32>(windowWidth * 0.4f), static_cast<s32>(windowHeight * 0.1f)).x - 55.f;
		float worldPositionY = ScreenToWorldPosition(static_cast<s32>(windowWidth * 0.4f), static_cast<s32>(windowHeight * 0.1f)).y;

		AEVec2 buttonSize{100.f, 100.f};
		actionBtn = EORWidgets::Button(worldPositionX, worldPositionY, BUTTON_BG, 0.f, buttonSize.x, buttonSize.y);
		actionBtn.SetContentImage(COMBAT_PAUSEBUTTON, { 0.f, 0.f }, 0.f, buttonSize);
		actionBtn.SetClickMethod(ActionButtonEvent);
		worldPositionX += (actionBtn.GetScale().x * 0.5f) + minGap + 100.f;
		buttonSize = { 200.f, 100.f };
		autoBtn = EORWidgets::Button(worldPositionX, worldPositionY, BUTTON_BGVARIANT, 0.f, buttonSize.x, buttonSize.y);
		autoBtn.SetContentImage(NO_SPRITE, { 0.f, 0.f }, 0.f, buttonSize);
		autoBtn.SetFontColor(LG_Color::white());
		autoBtn.SetText("AUTO");
		autoBtn.SetClickMethod(ToggleAutoMode);
		worldPositionX += (autoBtn.GetScale().x * 0.5f) + minGap + 50.f;
		buttonSize = { 100.f, 100.f };
		fastForwardBtn = EORWidgets::Button(worldPositionX, worldPositionY, BUTTON_BG, 0.f, buttonSize.x, buttonSize.y);
		fastForwardBtn.SetContentImage(COMBAT_DOUBLESPEEDBUTTON, { 0.f, 0.f }, 0.f, buttonSize);
		fastForwardBtn.SetClickMethod(ActivateDoubleSpeed);

		InputManager::GetInstance()->EnableDrag(false);

		PlayerManager::GetInstance()->GetPlayerRef().GetHUD().InitHUD();
	}

	static void UpdateTraitBoxes() {
		// Clear previous trait boxes
		for (auto box : traitTextBoxes) {
			delete box;
		}
		traitTextBoxes.clear();
		
		// Get active traits
		auto activeTraits = EOREntities::TraitManager::GetInstance().GetActiveTraits();
		
		// Create new text boxes
		float yOffset = 50.0f;
		
		for (const auto& pair : activeTraits) {
			const EOREntities::Trait trait = pair.first;
			const int level = pair.second;
			
			// Position below title
			AEVec2 position = ScreenToWorldPosition(static_cast<s32>(200), 
												   static_cast<s32>(100 + yOffset));
			
			EORWidgets::TextBox* box = new EORWidgets::TextBox(position.x, position.y,
															 Graphics::BUTTON_BG, 0.0f, 180.0f, 35.0f);
			box->SetPadding({10.0f, 5.0f});
			box->SetColors(LGCY_CP::LG_Color(30, 30, 30, 180), LGCY_CP::LG_Color(20, 20, 20, 180));
			box->SetFontColor(LGCY_CP::LG_Color::white());
			box->SetFontSize(14.0f);
			
			// Get trait name and level
			std::string traitName = EOREntities::TraitToString(trait);
			box->SetText(traitName + " (Lv." + std::to_string(level) + ")");
			
			traitTextBoxes.push_back(box);
			yOffset += 40.0f;
		}
	}

	static void InitTraitUI() {
		// Create title box for traits
		AEVec2 position = ScreenToWorldPosition(static_cast<s32>(200), 100);
		
		traitTitleBox = new EORWidgets::TextBox(position.x, position.y, 
											   Graphics::BUTTON_BGVARIANT, 0.0f, 180.0f, 40.0f);
		traitTitleBox->SetPadding({10.0f, 5.0f});
		traitTitleBox->SetColors(LGCY_CP::LG_Color(50, 50, 50, 200), LGCY_CP::LG_Color(30, 30, 30, 220));
		traitTitleBox->SetFontColor(LGCY_CP::LG_Color::yellow());
		traitTitleBox->SetFontSize(18.0f);
		traitTitleBox->SetText("ACTIVE TRAITS");
		
		// Initial update of trait boxes
		UpdateTraitBoxes();
	}

	static void RenderTraitUI() {
		if (traitTitleBox) {
			traitTitleBox->DrawTextbox();
		}
		
		for (auto box : traitTextBoxes) {
			box->DrawTextbox();
		}
	}

	static void FreeUI() {
		actionBtn = EORWidgets::Button();
		autoBtn = EORWidgets::Button();
		fastForwardBtn = EORWidgets::Button();
	}
#pragma endregion

	// RENDERING
#pragma region RENDERING
	static void RenderEntities() {
		for (const std::pair<Transform, Graphics::SPRITE>& sp : animationCopies) {
			DrawSprite(sp.first, sp.second);
		}
		
		Team& playerTeam = CombatSystem::GetPlayerTeam();
		Team& enemyTeam = CombatSystem::GetEnemyTeam();

		// Draw player team units
		for (int i = 0; i < MAX_UNITS; ++i) {
			if (playerTeam.units[i].isValid) {
				playerTeam.units[i].DrawUnit();
			}
		}

		// Draw enemy team units
		for (int i = 0; i < MAX_UNITS; ++i) {
			if (enemyTeam.units[i].isValid) {
				enemyTeam.units[i].DrawUnit(true);
			}
		}
	}

	static void RenderUI() {
		actionBtn.DrawButton();
		//autoBtn.DrawButton();
		fastForwardBtn.DrawButton();
		
		if (showResultText) {
			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE); // Change to texture mode
			AEGfxSetBlendMode(AE_GFX_BM_BLEND);
			AEGfxSetTransparency(1.0f);
			AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
			victoryTextBox->DrawTextbox();
			subTextBox->DrawTextbox();
			// int windowWidth = AEGfxGetWindowWidth();
			// int windowHeight = AEGfxGetWindowHeight();
			// Fonts_DrawText(resultText, windowWidth/2.0f, windowHeight/2.0f - 25, 32, defaultFont);
			// Fonts_DrawText(subText, windowWidth/2.0f, windowHeight/2.0f + 25, 24, defaultFont);
		}
		
		PlayerManager::GetInstance()->GetPlayerRef().GetHUD().RenderHUD();
	}
#pragma endregion

#pragma region GAME STATE FUNCTIONS
	void Load()
	{
		//some code here
		std::cout << "Combat Stage: Load" << std::endl;
		//std::ifstream fs("Counter.txt");
		//fs >> Counter;
		//fs.close();
		Fonts_Initialize();
		Graphics_Initialize();
		LoadUnitSprites();
		LoadItemSprites();
		LoadSprites({ Graphics::COMBAT_PLAYBUTTON, Graphics::COMBAT_NEXTBUTTON, Graphics::COMBAT_PAUSEBUTTON, Graphics::COMBAT_DOUBLESPEEDBUTTON, Graphics::NIGHT_BACKGROUND ,  Graphics::DAY_BACKGROUND });
		AudioManager& am = AudioManager::GetInstance();
		am.Load("CombatLevel");
		am.PlayBGMMusic("bgm");
	}

	void Initialize()
	{
		std::cout << "Combat Stage: Initialize" << std::endl;
		// Whatever u want to setup when level starts
		TimeElapsed = 0.0f;
		// Level 1 INIT
	
		InitWorldTransforms();
		CombatSystem::InitializeCombatSystem();
		InitUI();
		InitTraitUI();
		std::cout << "Updating traits in Level1..." << std::endl;
		EOREntities::TraitManager::GetInstance().UpdateTraits(PlayerManager::GetInstance()->GetPlayerRef().GetTeam());
		InputManager::GetInstance()->RegisterHandler(HandleInputs);

		auto activeTraits = EOREntities::TraitManager::GetInstance().GetActiveTraits();
		std::cout << "Active traits for player: " << activeTraits.size() << std::endl;
		
		victoryTextBox = new EORWidgets::TextBox(0.0f, 0.0f, Graphics::NO_SPRITE, 0.0f, 500.0f, 100.0f);
		victoryTextBox->SetPadding({13.0f, 5.0f});
		victoryTextBox->SetColors(LGCY_CP::LG_Color::clear(), LGCY_CP::LG_Color::clear());
		victoryTextBox->SetFontColor(LGCY_CP::LG_Color::white());
		victoryTextBox->SetFontSize(32.0f);
		victoryTextBox->isVisible = false;
		
		subTextBox = new EORWidgets::TextBox(0.0f, 0.0f, Graphics::NO_SPRITE, 0.0f, 500.0f, 100.0f);
		subTextBox->SetPadding({13.0f, 5.0f});
		subTextBox->SetColors(LGCY_CP::LG_Color::clear(), LGCY_CP::LG_Color::clear());
		subTextBox->SetFontColor(LGCY_CP::LG_Color::white());
		subTextBox->SetFontSize(24.0f);
		subTextBox->isVisible = false;
		f32 windowWidth = static_cast<f32>(AEGfxGetWindowWidth());
		f32 windowHeight = static_cast<f32>(AEGfxGetWindowHeight());
		combatBg.position = ScreenToWorldPosition(static_cast<s32>(windowWidth * 0.5f), static_cast<s32>(windowHeight * 0.5f));
		combatBg.rotation = 0.f;
		combatBg.scale = { windowWidth, windowHeight };
	}

	void Update()
	{
		f32 dt = static_cast<f32>(AEFrameRateControllerGetFrameTime());
		// INPUTS
		System_Update();
		InputManager::GetInstance()->HandleInputs();
		PlayerManager::GetInstance()->GetPlayerRef().GetHUD().HUD_Update();
		traitUIRefreshTimer += dt;
		if (traitUIRefreshTimer >= 1.0f) {  // Refresh every second
			UpdateTraitBoxes();
			traitUIRefreshTimer = 0.0f;
		}

		// Handle inputs for combat system
		if (!isPaused) {
			// Apply time scale for normal and double speed
			TimeElapsed += dt * doubleSpeed;
			for (size_t i = 0; i < animators.size(); ++i)
			{
				animators[i].Animate();
				if (!animators[i].IsAnimationActive()) {
					animators.erase(animators.begin() + i);
					animationCopies.erase(animationCopies.begin() + i);
				}
			}
		}
		
		// LOGIC
		if (!combatEnded) {
			if (TimeElapsed > 0.5f) {
				TimeElapsed = 0.0f;
				// Execute a single combat turn and get the result
				combatEnded = CombatSystem::StartCombat();
			}
#pragma region WIN_LOSE CONDITION
//============Check win / lose condition after combat ends (bool) StartCombat=================
			if (combatEnded) {
				// Use CombatSystem's HandleCombatEnd instead of local logic
				CombatSystem::HandleCombatEnd(playerWon, resultText, subText);
				TraitManager::GetInstance().ApplyPostCombatEffects(PlayerManager::GetInstance()->GetPlayerRef().GetTeam());
											
				// Position in center of screen
				int windowWidth = AEGfxGetWindowWidth();
				int windowHeight = AEGfxGetWindowHeight();
				AEVec2 screenCenter = {windowWidth/2.0f, windowHeight/2.0f};
				AEVec2 worldCenter = ScreenToWorldPosition(
					static_cast<s32>(screenCenter.x), 
					static_cast<s32>(screenCenter.y)
				);

				// Use the result and sub text from CombatSystem
				victoryTextBox->SetText(resultText);
				subTextBox->SetText(subText);
				victoryTextBox->SetPosition(worldCenter.x, worldCenter.y - 90.0f);
				subTextBox->SetPosition(worldCenter.x, worldCenter.y + 120.0f);
				
				// Make visible
				victoryTextBox->isVisible = true;
				subTextBox->isVisible = true;
				showResultText = true;
				
				if (playerWon) {
					int goldReward = 13;
					if (TraitManager::GetInstance().GetTraitLevel(Trait::LESSER) >= 2) {
						goldReward *= 2;
					}
					PlayerManager::GetInstance()->GetPlayerRef().AddGold(goldReward);
					std::cout << "Victory! +" << goldReward << " gold awarded" << std::endl;
				} else {
					// Player lost - health penalty
					int healthToDeduct = 0;
					for (int i = 0; i < MAX_UNITS; ++i) {
						if (CombatSystem::GetEnemyTeam().units[i].isValid) healthToDeduct += 10;
					}

					PlayerManager::GetInstance()->GetPlayerRef().DeductHealth(healthToDeduct);
					PlayerManager::GetInstance()->GetPlayerRef().AddGold(5);
					std::cout << "Defeat! -" << healthToDeduct << " health penalty" << std::endl;
				}
			}
		}
		// Handle result display and transition
		else if (showResultText) {
			// Update result text timer
			resultTextTimer += dt;
			
			// Transition after display time
			if (resultTextTimer >= RESULT_DISPLAY_TIME) {
				showResultText = false;
				
				// Check if player health is too low for game over condition
				if (PlayerManager::GetInstance()->GetPlayerRef().GetHealth() <= 0 || MapGraph::GetInstance().current_floor >= MapGraph::GetInstance().GetMapHeight()) {
					// Game over - transition to game over screen
					GameStateManager::GetInstance().SetNextGameState(GS_GAMEOVER);
				} else { // Continue game - transition to world map
					GameStateManager::GetInstance().SetNextGameState(GS_WORLDMAP);
				}
			}
		}
	}

	void Draw()
	{
		// RENDER
		if (GameStateManager::GetInstance().dayCycle == DayCycle::NIGHT) {
			DrawSprite(combatBg, Graphics::NIGHT_BACKGROUND);
		}
		else {
			DrawSprite(combatBg, Graphics::DAY_BACKGROUND);
		}
		
		AEGfxSetBackgroundColor(0.4f, 0.4f, 0.4f);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);

		// Set the the color to multiply to white, so that the sprite can
		// display the full range of colors (default is black).
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
		// Set blend mode to AE_GFX_BM_BLEND, which will allow transparency.
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(1.0f);

		RenderWorld();
		//RenderSprites();
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		RenderEntities();
		RenderUI();
		RenderTraitUI();
	}

	void Free()
	{
		std::cout << "Combat Stage: Free" << std::endl;

		// Unregister input handler
		InputManager::GetInstance()->UnregisterHandler();
		
		// Reset combat state variables
		combatEnded = false;
		showResultText = false;
		resultTextTimer = 0.0f;
		TimeElapsed = 0.0f;
		isPaused = false;
		isAutoEnabled = false;
		doubleSpeed = 1.0f;
		
		// Clean up TextBox objects created with new
		if (victoryTextBox) {
			delete victoryTextBox;
			victoryTextBox = nullptr;
		}
		
		if (subTextBox) {
			delete subTextBox;
			subTextBox = nullptr;
		}

		if (traitTitleBox) {
			delete traitTitleBox;
			traitTitleBox = nullptr;
		}

		for (auto box : traitTextBoxes) {
			delete box;
		}
		traitTextBoxes.clear();
		animators.clear();
		animationCopies.clear();
	}

	void Unload()
	{
		std::cout << "Combat Stage: Unload" << std::endl;
		// Frees the meshes
		Graphics_Unload();
		FreeUI();
		InputManager::GetInstance()->Clean();

		AEGfxDestroyFont(defaultFont.font);

		AudioManager::GetInstance().Unload();
	}
#pragma endregion

}