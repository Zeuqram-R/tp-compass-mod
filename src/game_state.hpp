#pragma once

#include <cstdint>

namespace compass {

// A per-frame snapshot of everything the compass needs from the game.
struct GameState {
    bool hasPlayer = false;
    char stage[9] = {};       // e.g. "F_SP102"
    int room = -1;
    float x = 0.0f, y = 0.0f, z = 0.0f;  // Link's position
    bool hasCamera = false;
    float cameraYaw = 0.0f;   // radians, direction the camera is looking (0 = +Z)
    bool eventRunning = false;  // cutscene / dialogue
    int windowStatus = 0;     // pause menus etc. (0 = none)
    const char* hiddenReason = "";
};

extern GameState g_state;

// Reads the current game state. Safe to call when no game is loaded.
void updateGameState();

// True when the compass should be drawn this frame (fills g_state.hiddenReason otherwise).
bool shouldDrawCompass();

}  // namespace compass
