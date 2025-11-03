/******************************************************************************/
/*!
\file           Settings.cpp
\project        Eye of Rah
\author(s)      [Leonard], 80% - settings functionality, event systems, UI state management
                [Jason], 20% - input handling, UI implementation, audio integration

\brief          Settings implements the game's settings menu:
                - UI controls are created with InitUI() for volume sliders and toggle buttons
                - Audio settings can be adjusted with volume sliders and mute toggle
                - Volume changes are handled with OnVolumeChange() callback and ToggleMuteEvent()
                - Fullscreen/Windowed mode switching
                - Input handling uses a dedicated SettingsLevelInputs() function
                - All UI elements respond to mouse interactions

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#include "pch.hpp"
#include "Settings.hpp"
#include "GameStateManager.hpp"
#include "Widgets.hpp"
#include "Graphics.hpp"
#include "AudioManager.hpp"
#include "InputManager.hpp"
#include "PlayerManager.hpp"
#include "Utils.hpp"

// Forward declare input handler
namespace Settings {
    void SettingsLevelInputs();     // Define my own input handler and uses settings control scheme
}

namespace Settings {
#pragma region VARIABLES
    // Input system reference
    InputManager* inputSystem = nullptr;
    
    // UI Elements
    EORWidgets::Button backBtn, fullscreenBtn, muteBtn;
    EORWidgets::Slider volumeSlider; // Using Slider widget
    EORWidgets::TextBox titleText;
    
    // Settings state
    bool isFullscreen = false;
    bool isMuted = false;
    
    // Background color
    LGCY_CP::LG_Color sandColor = LGCY_CP::LG_Color(194, 178, 128, 255);
#pragma endregion

#pragma region HELPERS
    void RenderWorld() {
        Transform transform{ 1.f, 1.f, 0.f, 0.f, 0.f };
        DrawQuad(sandColor.r, sandColor.g, sandColor.b, sandColor.a, transform);
    }
#pragma endregion

#pragma region EVENTS
    void BackButtonEvent() {
        std::cout << "Back button clicked!" << std::endl;
        GameStateManager& gsm = GameStateManager::GetInstance();
        gsm.SetNextGameState(GS_MENU);
    }
    
    void ToggleFullscreenEvent() {
        std::cout << "Fullscreen toggled!" << std::endl;
        isFullscreen = !isFullscreen;
        fullscreenBtn.SetText(isFullscreen ? "WINDOWED" : "FULLSCREEN");
        AESysSetFullScreen(isFullscreen);
    }
    
    void ToggleMuteEvent() {
        std::cout << "Mute toggled!" << std::endl;
        isMuted = !isMuted;
        muteBtn.SetText(isMuted ? "UNMUTE" : "MUTE");
        
        // Use the AudioManager to mute/unmute
        AudioManager& audioMgr = AudioManager::GetInstance();

        if (isMuted) {
            audioMgr.SetMasterVolume(0.0f);
        } else {
            audioMgr.SetMasterVolume(volumeSlider.GetValue());
        }
    }
    
    // Volume change callback function
    void OnVolumeChange(float value) {
        // Apply volume if not muted
        if (!isMuted) {
            AudioManager::GetInstance().SetMasterVolume(value);
        }
        
        // Update mute state based on volume
        if (value <= 0.01f && !isMuted) {
            isMuted = true;
            muteBtn.SetText("UNMUTE");
        } else if (value > 0.01f && isMuted) {
            isMuted = false;
            muteBtn.SetText("MUTE");
        }
    }
#pragma endregion

#pragma region INITIALIZATION
    //void InitWorldTransforms() {
    //    // Calculate world center
    //    int windowWidth = AEGfxGetWindowWidth();
    //    int windowHeight = AEGfxGetWindowHeight();
    //}
    
    void InitUI() {
        using namespace Graphics;
        using namespace LegacyProcessing;
        
        std::cout << "Level2: Initializing UI" << std::endl;
        
        int windowWidth = AEGfxGetWindowWidth();
        int windowHeight = AEGfxGetWindowHeight();
        
        // Convert screen positions to world positions
        AEVec2 worldCenter = ScreenToWorldPosition(
            static_cast<s32>(windowWidth/2), 
            static_cast<s32>(windowHeight/2)
        );
        
        // Title
        titleText = EORWidgets::TextBox(worldCenter.x, worldCenter.y - 200.0f, Graphics::BUTTON_BGVARIANT, 0.0f, 400.0f, 100.0f);
        titleText.SetColors(LGCY_CP::LG_Color(200, 180, 150, 255), LGCY_CP::LG_Color(100, 90, 70, 200));
        titleText.SetText("SETTINGS");
        
        // Volume Slider
		float currentVolume = AudioManager::GetInstance().GetMasterVolume();
        volumeSlider = EORWidgets::Slider(
            worldCenter.x, worldCenter.y - 50.0f, 
            Graphics::BUTTON_BG, 0.0f, 400.0f, 50.0f, 
            LGCY_CP::LG_Color(140, 140, 150, 255), LGCY_CP::LG_Color(80, 80, 100, 255)
        );
        volumeSlider.SetValue(currentVolume);
        volumeSlider.SetLabelText("VOLUME");
        volumeSlider.SetValueChangeCallback(OnVolumeChange);
        
        // Get current mute state
		isMuted = AudioManager::GetInstance().IsMuted();
        
        // Mute Button
        muteBtn = EORWidgets::Button(worldCenter.x, worldCenter.y + 30.0f, Graphics::BUTTON_BG, 0.0f, 300.0f, 80.0f);
        muteBtn.SetClickMethod(ToggleMuteEvent);
        muteBtn.SetFontColor(LG_Color::white());
        muteBtn.SetText(isMuted ? "UNMUTE" : "MUTE");
        
        // Fullscreen Button
        isFullscreen = AESysIsFullScreen();
        fullscreenBtn = EORWidgets::Button(worldCenter.x, worldCenter.y + 120.0f, Graphics::BUTTON_BG, 0.0f, 300.0f, 80.0f);
        fullscreenBtn.SetClickMethod(ToggleFullscreenEvent);
        fullscreenBtn.SetFontColor(LG_Color::white());
        fullscreenBtn.SetText(isFullscreen ? "WINDOWED" : "FULLSCREEN");
        
        // Back Button
        backBtn = EORWidgets::Button(worldCenter.x, worldCenter.y + 210.0f, Graphics::BUTTON_BGVARIANT, 0.0f, 300.0f, 80.0f);
        backBtn.SetClickMethod(BackButtonEvent);
        backBtn.SetFontColor(LG_Color::white());
        backBtn.SetText("BACK TO MENU");

        backBtn.isEnabled = true;
        fullscreenBtn.isEnabled = true;
        muteBtn.isEnabled = true;
    }
    
    void InitInputs() {
        // Initialize input system
        inputSystem = InputManager::GetInstance();
        
        inputSystem->SetInputMode(InputMode::IMODE_GAMEANDUI); // Avoid HUD issues
		inputSystem->SetInputSchemeToUse(ControlScheme::SETTINGS_SCHEME); 
        inputSystem->EnableDrag(false); // Settings don't need drag
		inputSystem->RegisterHandler(SettingsLevelInputs);
    }
#pragma endregion

#pragma region UI FUNCTIONS
    void FreeUI() {
        titleText = EORWidgets::TextBox();
        backBtn = EORWidgets::Button();
        fullscreenBtn = EORWidgets::Button();
        muteBtn = EORWidgets::Button();
        volumeSlider = EORWidgets::Slider(); // Match the type declaration
    }
    
    void RenderUI() {
        titleText.DrawTextbox();
        volumeSlider.Draw();
        muteBtn.DrawButton();
        fullscreenBtn.DrawButton();
        backBtn.DrawButton();
    }
#pragma endregion

#pragma region INPUT HANDLING
    // Input handler for settings page
    void SettingsLevelInputs() {
        if (!inputSystem) {
            std::cout << "Error: Input system not initialized" << std::endl;
            return;
        }
        
        if (!inputSystem->IsGameInputEnabled()) return;
        
        // Use the Slider's built-in input handling
        volumeSlider.HandleInput();
        
        // Handle button clicks
        if (AEInputCheckTriggered(AEVK_LBUTTON)) {
            // Get mouse position
            s32 localMouseX, localMouseY;
            AEInputGetCursorPosition(&localMouseX, &localMouseY);
            // Convert to world coordinates
            AEVec2 mouseWorldPos = ScreenToWorldPosition(localMouseX, localMouseY);
            // BRUH DONT WORK for these two 
            // backBtn.OnClick();
            // fullscreenBtn.OnClick();
            // muteBtn.OnClick();
            if (IsPointInRect(mouseWorldPos, backBtn.GetPosition(), backBtn.GetScale())) {
                AudioManager::GetInstance().PlaySFX("clickSound1");
                BackButtonEvent();
            }
            else if (IsPointInRect(mouseWorldPos, fullscreenBtn.GetPosition(), fullscreenBtn.GetScale())) {
                AudioManager::GetInstance().PlaySFX("clickSound1");
                ToggleFullscreenEvent();
            }
            else if (IsPointInRect(mouseWorldPos, muteBtn.GetPosition(), muteBtn.GetScale())) {
                muteBtn.OnClick();
            }
        }
        
        // Optional: Add ESC key to return to menu
        if (AEInputCheckTriggered(AEVK_ESCAPE)) {
            BackButtonEvent();
        }
    }
#pragma endregion

#pragma region GAME STATE FUNCTIONS
    void Load() {
        std::cout << "Settings: Load" << std::endl;
        
        // Graphics must be initialized before UI creation
        Graphics_Initialize();
        LoadSprites({
            Graphics::BUTTON_BG, 
            Graphics::BUTTON_BGVARIANT
        });
        
        // Initialize fonts
        Fonts_Initialize();
        
        // Load audio for the settings page
        AudioManager& am = AudioManager::GetInstance();
		am.Load("Settings");
		am.PlayBGMMusic("bgm");
    }
    
    void Initialize() {
        std::cout << "Settings: Initialize" << std::endl;
        
        //InitWorldTransforms();
        InitUI();
        InitInputs();
    }
    
    void Update() {
        // Process inputs through input manager
        InputManager::GetInstance()->HandleInputs();
    }
    
    void Draw() {
        RenderWorld();
        RenderUI();
    }
    
    void Free() {
        std::cout << "Settings: Free" << std::endl;
        
        InputManager::GetInstance()->UnregisterHandler();
        InputManager::GetInstance()->SetInputSchemeToUse(ControlScheme::DEFAULT_SCHEME);
        inputSystem = nullptr;
    }
    
    void Unload() {
        // Frees the meshes
        InputManager::GetInstance()->Clean();
        Graphics_Unload();
        FreeUI();
        AEGfxDestroyFont(defaultFont.font);
        AudioManager::GetInstance().Unload();
    }
#pragma endregion

}