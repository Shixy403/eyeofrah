/******************************************************************************/
/*!
\file           CombatPreparation.cpp
\project        Eye of Rah
\author(s)      [Leonard], 75% - unit drag and drop system, team management, animation for possible unit swap using green DrawQuad to pulse
                [Jason], 10% - minor changes to properly link to Combat System
                [Aryan], 5% - Reading enemy team from file and shuffling displayed enemy team

\brief          CombatPreparation handles the pre-combat setup phase where players can:
                - Rearrange units via drag and drop using IsMouseOverEntity() and HandleUnitSwap()
                - Reset positions with ResetUnitPositions() using originalPlayerPositions[]
                - Review enemy formations before entering combat
                - Transition to the combat stage through GoNext() which transfers teams to CombatSystem

All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/

#include "pch.hpp"
#include "CombatPreparation.hpp"
#include "GameStateManager.hpp"
#include "Team.hpp"
#include "Unit.hpp"
#include "Widgets.hpp"
#include "PresetWidgets.hpp"
#include "CombatSystem.hpp"
#include "Legacy.hpp"
#include "RandomMap.hpp"
#include "PositionManager.hpp"
#include "TraitManager.hpp"
#include <algorithm>
#include <AEEngine.h>
#include "Utils.hpp"
#include "Item.hpp"

namespace CombatPreparation
{

    using namespace EOREntities;

#pragma region CONSTANTS
    // ======== Constants ========
    const float RETURN_ANIMATION_DURATION = 0.3f; // Animation duration in seconds
    const float DRAG_START_DELAY = 0.1f;          // Delay before drag starts (prevents accidental drags)
    const float PULSE_ANIMATION_SPEED = 5.0f;     // Speed of pulse animation for swap highlight
#pragma endregion

#pragma region VARIABLES
    // ======== State Variables ========
    // Teams
    EOREntities::Team playerTeam;
    EOREntities::Team enemyTeam;
    EOREntities::Team displayedEnemyTeam;
    bool isRearrangePhase = true;

    // Drag & Drop System
    float dragTimer = 0.0f;
    EOREntities::EntityObject *draggedEntity = nullptr;
    int draggedUnitIndex = -1;
    AEVec2 dragOffset = {0.0f, 0.0f};
    Transform originalTransform;

    // Storage for original positions
    Transform originalPlayerPositions[EOREntities::MAX_UNITS];
    // Initial unit data
    EOREntities::Team initialPlayerTeam;
    bool initialTeamStored = false;

    // Animation system
    bool isAnimatingReturn = false;
    float returnAnimationTime = 0.0f;
    float pulseAnimationTime = 0.0f;
    int potentialSwapIndex = -1;

    // UI Elements
    EORWidgets::Button resetPositionsBtn;
    EORWidgets::Button playLevel1Btn;
    LGCY_CP::LG_Color battleAreaColor;
    AEVec2 worldPosition;

    // Battle Areas
    Transform battleArea;
    Transform playerArea;
    Transform enemyArea;
#pragma endregion

#pragma region UTIL FUNCTIONS
    // ======== Utility Functions ========

    // Keep entity within player area bounds
    void ConstrainEntityPosition(Transform &transform)
    {
        float halfWidth = transform.scale.x * 0.5f;
        float halfHeight = transform.scale.y * 0.5f;

        float leftBound = playerArea.position.x - playerArea.scale.x * 0.5f + halfWidth;
        float rightBound = playerArea.position.x + playerArea.scale.x * 0.5f - halfWidth;
        float topBound = playerArea.position.y + playerArea.scale.y * 0.5f - halfHeight;
        float bottomBound = playerArea.position.y - playerArea.scale.y * 0.5f + halfHeight;

#undef max
#undef min
        transform.position.x = std::max(leftBound, std::min(rightBound, transform.position.x));
        transform.position.y = std::max(bottomBound, std::min(topBound, transform.position.y));
    }

    // Convert screen coordinates to world coordinates
    AEVec2 ConvertScreenToWorld(s32 screenX, s32 screenY)
    {
        AEVec2 worldPos = {};
        f32 camX, camY;
        AEGfxGetCamPosition(&camX, &camY);
        s32 winWidth = AEGfxGetWindowWidth();
        s32 winHeight = AEGfxGetWindowHeight();

        // Convert to normalized device coordinates, then to world coordinates
        worldPos.x = ((f32)screenX / winWidth * 2.0f - 1.0f) * (winWidth / 2.0f) + camX;
        worldPos.y = (1.0f - (f32)screenY / winHeight * 2.0f) * (winHeight / 2.0f) + camY;

        return worldPos;
    }

    // Check if mouse is over an entity
    bool IsMouseOverEntity(const EOREntities::EntityObject &entity)
    {
        s32 localMouseX, localMouseY;
        AEInputGetCursorPosition(&localMouseX, &localMouseY);
        AEVec2 worldPos = ConvertScreenToWorld(localMouseX, localMouseY);
        const Transform &transform = entity.GetTransform();

        // Add a small margin to make selection easier (10% of width/height)
        float halfWidth = transform.scale.x * 0.55f;
        float halfHeight = transform.scale.y * 0.55f;

        return (worldPos.x >= transform.position.x - halfWidth &&
                worldPos.x <= transform.position.x + halfWidth &&
                worldPos.y >= transform.position.y - halfHeight &&
                worldPos.y <= transform.position.y + halfHeight);
    }

    // Check if two entities overlap (for swap detection)
    bool DoEntitiesOverlap(const EOREntities::EntityObject &entity1, const EOREntities::EntityObject &entity2)
    {
        const Transform &t1 = entity1.GetTransform();
        const Transform &t2 = entity2.GetTransform();

        float dx = std::abs(t1.position.x - t2.position.x);
        float dy = std::abs(t1.position.y - t2.position.y);

        float halfWidth1 = t1.scale.x * 0.5f;
        float halfHeight1 = t1.scale.y * 0.5f;
        float halfWidth2 = t2.scale.x * 0.5f;
        float halfHeight2 = t2.scale.y * 0.5f;

        // 70% overlap needed for swap
        float overlapThreshold = 0.7f;
        return (dx < (halfWidth1 + halfWidth2) * overlapThreshold &&
                dy < (halfHeight1 + halfHeight2) * overlapThreshold);
    }
#pragma endregion

#pragma region TEAM MANAGEMENT
    // Reset unit positions to original arrangement
    void ResetUnitPositions()
    {
        if (!isRearrangePhase || !initialTeamStored)
            return;

        // Restore the complete team state from our initial copy
        for (int i = 0; i < initialPlayerTeam.currentTeamSize; i++)
        {
            if (initialPlayerTeam.units[i].isValid)
            {
                // Copy unit data (stats, name, etc.)
                playerTeam.units[i] = initialPlayerTeam.units[i];

                // Set position explicitly
                playerTeam.units[i].SetPosition(
                    originalPlayerPositions[i].position.x,
                    originalPlayerPositions[i].position.y);
            }
        }

        // Reset team size to initial state
        SetTeamSize(playerTeam, initialPlayerTeam.currentTeamSize);

        PositionManager::SavePlayerPositions(playerTeam);
        CombatSystem::SetPlayerTeam(playerTeam); // Restoring the complete unit data, not just positions
        // Any stats that may have changed during swapping need to be reset to initial values
        printf("Units reset to original positions and stats\n");
    }

    // Handle unit position swapping
    void HandleUnitSwap(int index1, int index2)
    {
        if (index1 == index2)
            return;

        // Use the existing MoveUnit function to handle the swap
        EOREntities::MoveUnit(playerTeam, index1, index2);

        // Update positions in other systems (these are specific to CombatPreparation)
        PositionManager::SavePlayerPositions(playerTeam);
        CombatSystem::SetPlayerTeam(playerTeam);
        EOREntities::TraitManager::GetInstance().UpdateTraits(PlayerManager::GetInstance()->GetPlayerRef().GetTeam());

        // Make sure all units have their showBox enabled
        for (int i = 0; i < playerTeam.currentTeamSize; i++)
        {
            playerTeam.units[i].SetShowBox(true);
        }

        printf("Swapped units with all child elements at positions %d and %d\n", index1, index2);
    }

    void LoadPlayerTeamFromManager()
    {
        try
        {
            // Check if PlayerManager exists
            if (!PlayerManager::Exists())
            {
                std::cout << "PlayerManager not initialized yet, creating default team" << std::endl;
                throw std::runtime_error("PlayerManager not available");
            }

            // Get PlayerManager instance
            PlayerManager *playerManager = PlayerManager::GetInstance();
            if (!playerManager)
            {
                throw std::runtime_error("Failed to get PlayerManager instance");
            }

            // Get reference to player and team
            Player &player = playerManager->GetPlayerRef();
            Team &playerManagerTeam = player.GetTeam();

            // Clear current team to avoid duplicates
            EOREntities::ClearTeam(playerTeam);

            // Debug info
            std::cout << "Player team has " << playerManagerTeam.currentTeamSize << " units before copying" << std::endl;

            // Copy team from player manager to combat preparation system
            if (playerManagerTeam.currentTeamSize > 0)
            {
                EOREntities::CopyTeam(playerManagerTeam, playerTeam);
                std::cout << "Copied " << playerTeam.currentTeamSize << " units to combat team" << std::endl;
            }

            // Position units for combat
            for (int i = 0; i < playerTeam.currentTeamSize; ++i)
            {
                if (playerTeam.units[i].isValid)
                {
                    // Position based on index (frontline to backline)
                    float xPos = -100.f - (150.f * i);
                    playerTeam.units[i].SetPosition(xPos, 0.f);
                    std::cout << "Positioned unit " << i << " at (" << xPos << ", 0.0)" << std::endl;
                }
            }
            for (int i = 0; i < playerTeam.currentTeamSize; i++)
            {
                if (playerTeam.units[i].isValid)
                {
                    playerTeam.units[i].SetShowBox(true);
                }
            }
            // Save positions to Position Manager
            PositionManager::SavePlayerPositions(playerTeam);

            std::cout << "Successfully loaded player team with " << playerTeam.currentTeamSize << " units" << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error loading player team: " << e.what() << std::endl;

            // Create emergency fallback team with one unit
            std::cout << "Creating emergency fallback team with 1 unit" << std::endl;
            EOREntities::ClearTeam(playerTeam);
            Unit emergencyUnit = unitDatabase[RA];
            emergencyUnit.isValid = true;
            AddToTeam(emergencyUnit, playerTeam, 0);

            // Position the emergency unit
            playerTeam.units[0].SetPosition(-100.f, 0.f);
            PositionManager::SavePlayerPositions(playerTeam);
        }
    }

    void LoadRandomEnemyTeam(Transform enemyPositions[])
    {
        // Clear existing enemy team
        EOREntities::ClearTeam(enemyTeam);

        // Get enemy team for current floor from MapGraph
        std::vector<EOREntities::Unit> readTeam = EOREntities::ReadTeam(MapGraph::GetInstance().current_floor);

        // Fallback if no team is available for current floor
        if (readTeam.empty())
        {
            std::cout << "No enemy team data found for floor " << MapGraph::GetInstance().current_floor
                      << ", using default enemies" << std::endl;
            readTeam = {
                EOREntities::unitDatabase[EOREntities::RA],
                EOREntities::unitDatabase[EOREntities::MEDJED],
                EOREntities::unitDatabase[EOREntities::MEDJED]};
        }

        // Add units to enemy team
        for (int i = 0; i < readTeam.size() && i < EOREntities::MAX_UNITS; ++i)
        {
            // Make a copy of the unit and set its position
            EOREntities::Unit enemyUnit = readTeam[i];
            enemyUnit.SetPosition(enemyPositions[i].position.x, enemyPositions[i].position.y);

            // Add to team using Team functionality
            EOREntities::AddToTeam(enemyUnit, enemyTeam, i);

            std::cout << "Enemy Unit " << i << ": " << enemyUnit.name << ", "
                      << enemyUnit.GetHitPoints() << " HP, "
                      << enemyUnit.GetAttackPoints() << " ATK" << std::endl;
        }

        // Update CombatSystem with the new enemy team
        CombatSystem::SetEnemyTeam(enemyTeam);
        size_t i = 0;
        for (const EOREntities::Unit &u : enemyTeam.units)
        {
            bool found = false;
            for (const EOREntities::Unit& uu : displayedEnemyTeam.units)
            {
                if (uu.name == u.name)
                    found = true;
            }
            if (!found)
            {
                EOREntities::Unit tmp = u;
                AddToTeam(tmp, displayedEnemyTeam, (int)i);
                ++i;
            }
        }
        shuffle(enemyPositions, displayedEnemyTeam.currentTeamSize);
        for (size_t i_ = 0; i_ < displayedEnemyTeam.currentTeamSize; ++i_) {
            displayedEnemyTeam.units[i_].SetPosition(enemyPositions[i_].position.x, enemyPositions[i_].position.y);
        }
        std::cout << "Loaded enemy team with " << enemyTeam.currentTeamSize << " units for combat" << std::endl;
    }

    // Setup the preparation stage with player and enemy teams
    void PrepareArrangementStage()
    {
        isRearrangePhase = true;

        // Define positions for teams
        // Transform playerPositions[] = {
        //     {{100.f, 0.f}, 0.f, {100.f, 100.f}},
        //     {{275.f, 0.f}, 0.f, {100.f, 100.f}},
        //     {{400.f, 0.f}, 0.f, {100.f, 100.f}},
        //     {{525.f, 0.f}, 0.f, {100.f, 100.f}},
        //     {{650.f, 0.f}, 0.f, {100.f, 100.f}},
        //     {{775.f, 0.f}, 0.f, {100.f, 100.f}}
        // };

        //==================Define enemy positions only========================
#pragma region ENEMY POSITIONS
        Transform enemyPositions[MAX_UNITS] = {
            {{100.f, 0.f}, 0.f, {100.f, 100.f}},
            {{275.f, 0.f}, 0.f, {100.f, 100.f}},
            {{450.f, 0.f}, 0.f, {100.f, 100.f}},
            {{625.f, 0.f}, 0.f, {100.f, 100.f}},
            {{800.f, 0.f}, 0.f, {100.f, 100.f}}};
#pragma endregion
        //==================Define positions using xPos var=====================
        // Load player team from PlayerManager and define positions
        LoadPlayerTeamFromManager();

        // Load a random enemy team
        LoadRandomEnemyTeam(enemyPositions);

        // Save original positions and team state
        for (int i = 0; i < EOREntities::MAX_UNITS && i < playerTeam.currentTeamSize; i++)
        {
            originalPlayerPositions[i] = playerTeam.units[i].GetTransform();
        }

        // Store the complete initial team (make a deep copy)
        initialPlayerTeam = playerTeam;
        initialTeamStored = true;
    }

    static void GoNext()
    {
        ClearTeam(displayedEnemyTeam);
        // Night time enemy HP and Attack Increase by 2. 
        if (GameStateManager::GetInstance().dayCycle == DayCycle::NIGHT) {
            for (Unit& enemy : enemyTeam.units) {
                enemy.SetHitPoints(enemy.GetHitPoints() + 2);
                enemy.SetAttackPoints(enemy.GetAttackPoints() + 2);
            }
        }
        CombatSystem::SetPlayerTeam(playerTeam);
        CombatSystem::SetEnemyTeam(enemyTeam);
        GameStateManager::GetInstance().SetNextGameState(GS_COMBATSTAGE);
    }
#pragma endregion // End of Team Management Functions

#pragma region UI MANAGEMENT
    // Initialize UI elements
    void InitializeUI()
    {
        using namespace Graphics;
        using namespace LegacyProcessing;

        int windowWidth = AEGfxGetWindowWidth();
        int windowHeight = AEGfxGetWindowHeight();

        worldPosition = ScreenToWorldPosition(static_cast<s32>(windowWidth), static_cast<s32>(windowHeight));

        resetPositionsBtn = EORWidgets::Button(-worldPosition.x * 0.5f, worldPosition.y * 0.8f, BUTTON_BG, 0.f, 200.f, 100.f);
        resetPositionsBtn.SetFontColor(LG_Color::white());
        resetPositionsBtn.SetText("RESET");
        resetPositionsBtn.SetClickMethod(ResetUnitPositions);

        playLevel1Btn = EORWidgets::Button(worldPosition.x * 0.8f, worldPosition.y * 0.8f, COMBAT_PLAYBUTTON, 0.f, 200.f, 100.f);
        playLevel1Btn.SetFontColor(LG_Color::white());
        playLevel1Btn.SetClickMethod(GoNext);

        battleArea = {{0.f, 0.f}, 0.f, {1600.f, 400.f}};
        playerArea = {{-400.f, 0.f}, 0.f, {800.f, 300.f}};
        enemyArea = {{400.f, 0.f}, 0.f, {800.f, 300.f}};
        battleAreaColor = LGCY_CP::LG_Color(194.0f, 178.0f, 128.0f, 100.0f);
        // Make sure all units have their showBox enabled
        for (int i = 0; i < playerTeam.currentTeamSize; i++)
        {
            playerTeam.units[i].SetShowBox(true);
        }
        PlayerManager::GetInstance()->GetPlayerRef().GetHUD().InitHUD();
    }
#pragma endregion

#pragma region INPUTS
    // Process all user inputs (to be updated with input manager)
    void HandleInputs()
    {
        // Check for shortcuts and button clicks
        if (AEInputCheckTriggered(AEVK_ESCAPE))
        {
            PositionManager::SavePlayerPositions(playerTeam);
            CombatSystem::SetPlayerTeam(playerTeam);
            CombatSystem::SetEnemyTeam(enemyTeam);
            GameStateManager::GetInstance().SetNextGameState(GS_COMBATSTAGE);
            return;
        }

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            /*Check MouseX and MouseY as there is conflict before OnClick internal position checking.*/
            s32 LocalX, LocalY;
            AEInputGetCursorPosition(&LocalX, &LocalY);
            AEVec2 mouseWorldPos = ConvertScreenToWorld(LocalX, LocalY);
            // Check if mouse is over each button before triggering click
            if (IsPointInRect(mouseWorldPos, resetPositionsBtn.GetPosition(), resetPositionsBtn.GetScale()))
            {
                ResetUnitPositions();
            }

            if (IsPointInRect(mouseWorldPos, playLevel1Btn.GetPosition(), playLevel1Btn.GetScale()))
            {
                GoNext();
            }
        }

        potentialSwapIndex = -1;

        // Handle return animation
        if (isAnimatingReturn)
        {
            returnAnimationTime += (f32)AEFrameRateControllerGetFrameTime();
            float t = std::min(returnAnimationTime / RETURN_ANIMATION_DURATION, 1.0f);

            if (t >= 1.0f)
            {
                isAnimatingReturn = false;
                returnAnimationTime = 0.0f;

                if (draggedEntity)
                {
                    const_cast<Transform &>(draggedEntity->GetTransform()) = originalTransform;
                    draggedEntity = nullptr;
                }
            }
            else if (draggedEntity)
            {
                Transform &currentTransform = const_cast<Transform &>(draggedEntity->GetTransform());
                float easeOut = EaseOutCubic(t);

                // Position interpolation
                currentTransform.position.x = currentTransform.position.x +
                                              (originalTransform.position.x - currentTransform.position.x) * easeOut;
                currentTransform.position.y = currentTransform.position.y +
                                              (originalTransform.position.y - currentTransform.position.y) * easeOut;
            }

            // return; // Skip other input handling during animation
        }

        // Handle hover and drag detection
        if (draggedEntity == nullptr)
        {
            for (int i = 0; i < playerTeam.currentTeamSize; i++)
            {
                if (IsMouseOverEntity(playerTeam.units[i]))
                {
                    playerTeam.units[i].OnHover();
                }
            }
        }
        else
        {
            // Check for potential swap targets
            for (int i = 0; i < playerTeam.currentTeamSize; i++)
            {
                if (i != draggedUnitIndex && DoEntitiesOverlap(*draggedEntity, playerTeam.units[i]))
                {
                    potentialSwapIndex = i;
                    break;
                }
            }
        }

        // Handle drag start and movement
        if (AEInputCheckCurr(AEVK_LBUTTON))
        {
            if (draggedEntity == nullptr && !isAnimatingReturn)
            {
                dragTimer += (f32)AEFrameRateControllerGetFrameTime();
                if (dragTimer > DRAG_START_DELAY)
                {
                    for (int i = 0; i < playerTeam.currentTeamSize; i++)
                    {
                        if (IsMouseOverEntity(playerTeam.units[i]))
                        {
                            // Store original transform
                            originalTransform = playerTeam.units[i].GetTransform();
                            draggedEntity = &playerTeam.units[i];
                            draggedUnitIndex = i;

                            playerTeam.units[i].OnDragBegin();

                            // Calculate offset between mouse position and entity center
                            s32 localX, localY;
                            AEInputGetCursorPosition(&localX, &localY);
                            AEVec2 mouseWorldPos = ConvertScreenToWorld(localX, localY);

                            // Store offset from mouse to object center for smooth dragging
                            dragOffset.x = originalTransform.position.x - mouseWorldPos.x;
                            dragOffset.y = originalTransform.position.y - mouseWorldPos.y;

                            dragTimer = 0.f;
                            break;
                        }
                    }
                }
            }
            else
            {
                // Move dragged entity
                s32 LocalmouseX, LocalmouseY;
                AEInputGetCursorPosition(&LocalmouseX, &LocalmouseY);
                AEVec2 mouseWorldPos = ConvertScreenToWorld(LocalmouseX, LocalmouseY);

                if (!draggedEntity)
                    return;
                Transform &transform = const_cast<Transform &>(draggedEntity->GetTransform());
                transform.position.x = mouseWorldPos.x + dragOffset.x;
                transform.position.y = mouseWorldPos.y + dragOffset.y;

                ConstrainEntityPosition(transform);
            }
        }

        // Handle drag end/drop
        if (AEInputCheckReleased(AEVK_LBUTTON))
        {
            if (draggedEntity != nullptr)
            {
                static_cast<Unit *>(draggedEntity)->OnDragEnd();

                if (potentialSwapIndex != -1)
                {
                    HandleUnitSwap(draggedUnitIndex, potentialSwapIndex);
                    draggedEntity = nullptr;
                    draggedUnitIndex = -1;
                }
                else
                {
                    isAnimatingReturn = true;
                    returnAnimationTime = 0.0f;
                }
            }
            dragTimer = 0.f;
        }
    }
#pragma endregion

#pragma region RENDERING
    // Draw the battle stage, units, and UI + quad with the specified color and transform
    void RenderStage()
    {
        // Draw backgrounds
        DrawQuad(battleAreaColor.r, battleAreaColor.g, battleAreaColor.b, battleAreaColor.a, battleArea);

        LGCY_CP::LG_Color playerAreaColor(150, 150, 255, 50);
        LGCY_CP::LG_Color enemyAreaColor(255, 150, 150, 50);

        DrawQuad(playerAreaColor.r, playerAreaColor.g, playerAreaColor.b, playerAreaColor.a, playerArea);
        DrawQuad(enemyAreaColor.r, enemyAreaColor.g, enemyAreaColor.b, enemyAreaColor.a, enemyArea);

        // Handle visual feedback for dragging
        if (draggedEntity != nullptr && draggedUnitIndex >= 0)
        {
            // Draw ghost at original position
            LGCY_CP::LG_Color ghostColor(200, 200, 200, 128);
            DrawQuad(ghostColor.r, ghostColor.g, ghostColor.b, ghostColor.a, originalTransform);

            // Highlight swap target
            if (potentialSwapIndex != -1)
            {
                Transform targetTransform = playerTeam.units[potentialSwapIndex].GetTransform();
                float pulseAmount = 0.1f * sinf(pulseAnimationTime * PULSE_ANIMATION_SPEED) + 1.15f;

                LGCY_CP::LG_Color highlightColor(0, 255, 0, 150);
                Transform highlightTransform = targetTransform;
                highlightTransform.scale.x *= pulseAmount;
                highlightTransform.scale.y *= pulseAmount;

                DrawQuad(highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a, highlightTransform);
            }
        }

        // Draw units
        for (int i = 0; i < playerTeam.currentTeamSize; i++)
        {
            playerTeam.units[i].DrawUnit();
        }

        for (int i = 0; i < displayedEnemyTeam.currentTeamSize; i++)
        {
            displayedEnemyTeam.units[i].DrawUnit(true);
        }

        // Draw UI
        resetPositionsBtn.DrawButton();
        playLevel1Btn.DrawButton();
    }
#pragma endregion

#pragma region GAME STATE
    // ======== Game State Management ========

    void Load()
    {
        Fonts_Initialize();
        Graphics_Initialize();
        LoadUnitSprites();
        LoadItemSprites();
        LoadSprites({ Graphics::COMBAT_PLAYBUTTON, Graphics::COMBAT_NEXTBUTTON });
    }

    void Initialize()
    {

        if (!PlayerManager::Exists())
        {
            std::cout << "PlayerManager not initialized, creating instance in CombatPreparation" << std::endl;
            PlayerManager::GetInstance(); // This will create the instance if it doesn't exist
        }
        else
        {
            std::cout << "PlayerManager already exists" << std::endl;
        }

        // Trigger Item specific events
        EOREntities::TriggerPlayerItem(ItemID::ID_PHAROAHIDOL);

        PrepareArrangementStage();
        InitializeUI();

        dragTimer = 0.f;
        draggedEntity = nullptr;
        draggedUnitIndex = -1;
        isAnimatingReturn = false;
        returnAnimationTime = 0.0f;
        potentialSwapIndex = -1;
        pulseAnimationTime = 0.0f;

        AudioManager& am = AudioManager::GetInstance();
        am.Load("Combatprep");
        am.PlayBGMMusic("bgm");
    }

    void Update()
    {
        HandleInputs();
        pulseAnimationTime += static_cast<f32>(AEFrameRateControllerGetFrameTime());
    }

    void Draw()
    {
        AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f);
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        RenderStage();
    }

    void Free()
    {
        // Reset state variables
        draggedEntity = nullptr;
        dragTimer = 0.f;
        draggedUnitIndex = -1;
        isAnimatingReturn = false;
        returnAnimationTime = 0.0f;
        potentialSwapIndex = -1;

        // Reset UI elements
        resetPositionsBtn = EORWidgets::Button();
        playLevel1Btn = EORWidgets::Button();
    }

    void Unload()
    {
        std::cout << "CombatPreparation::Unload" << std::endl;

        // Clean up teams
        ClearTeam(playerTeam);
        ClearTeam(enemyTeam);
        ClearTeam(displayedEnemyTeam);
        ClearTeam(initialPlayerTeam);

        // Reset state variables
        initialTeamStored = false;
        isRearrangePhase = true;

        // Cleanup input system
        InputManager::GetInstance()->Clean();

        // Unload graphics resources
        Graphics_Unload();

        // Clean up font resources
        AEGfxDestroyFont(defaultFont.font);

        AudioManager::GetInstance().Unload();

    }
#pragma endregion

#pragma region Public API
    bool IsInRearrangePhase()
    {
        return isRearrangePhase;
    }

    void SetPlayerTeam(const Team &team)
    {
        playerTeam = team;
        PositionManager::SavePlayerPositions(playerTeam);
    }

    void SetEnemyTeam(const Team &team)
    {
        enemyTeam = team;
    }

    void Reset()
    {
        isRearrangePhase = true;
        ResetUnitPositions();
        draggedEntity = nullptr;
        draggedUnitIndex = -1;
        isAnimatingReturn = false;
    }
#pragma endregion

}