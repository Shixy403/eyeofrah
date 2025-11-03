/******************************************************************************/
/*!
\file           Settings.hpp
\project        Eye of Rah
\author(s)      [Leonard], 80% - settings functionality, event systems, UI state management
                [Jason], 20% - input handling, UI implementation, audio integration

\brief          Settings implements the game's settings menu where players can:
                - Adjust audio settings through ToggleMuteEvent() and OnVolumeChange()
                - Configure display options via ToggleFullscreenEvent() 
                - Go to main menu with BackButtonEvent()
                

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
*/
/******************************************************************************/
#pragma once

namespace Settings {
    void Load();
    void Initialize();
    void Update();
    void Draw();
    void Free();
    void Unload();
    
    // UI Init
    void InitUI();
    //void InitWorldTransforms();
    void InitInputs();
    void FreeUI();
    
    // Rendering functions
    void RenderWorld();
    void RenderUI();
    
    // Input handler
    void SettingsLevelInputs();
    
    // Event handlers
    void BackButtonEvent();
    void ToggleFullscreenEvent();
    void ToggleMuteEvent();
    void OnVolumeChange(float value);
}
