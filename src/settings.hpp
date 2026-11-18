#pragma once

#include <mods/svc/config.h>

#include <cstdint>

namespace compass {

// Values read from the ConfigService once per frame. Everything the player can tweak lives here,
// including calibration knobs so we can fix orientation issues without rebuilding the IPA.
struct Settings {
    bool showCompass = true;
    bool showPins = true;
    bool showDebug = false;
    int64_t opacity = 85;       // percent
    int64_t width = 360;        // HUD units (the game's 2D space is ~608 wide)
    int64_t posY = 18;          // distance from the top of the safe area
    int64_t arcDegrees = 180;   // total field of view shown on the bar
    int64_t northOffset = 2;    // index into {0, 90, 180, 270} degrees
    bool invertDirection = false;
    // Pin hotkey: hold `pinModifier` and press `pinButton` (indices into the option tables).
    bool pinHotkey = true;
    int64_t pinModifier = 1;    // 0 none, 1 Z, 2 R, 3 L
    int64_t pinButton = 0;      // 0 left, 1 right, 2 down, 3 up
    // Chests
    bool showChests = true;
    bool chestsNeedCompass = false;
    // Location name
    bool showLocation = true;
    int64_t locationX = 16;     // from the left of the safe area
    int64_t locationY = 12;     // text baseline, from the bottom of the safe area
};

struct SettingsHandles {
    ConfigVarHandle showCompass = 0;
    ConfigVarHandle showPins = 0;
    ConfigVarHandle showDebug = 0;
    ConfigVarHandle opacity = 0;
    ConfigVarHandle width = 0;
    ConfigVarHandle posY = 0;
    ConfigVarHandle arcDegrees = 0;
    ConfigVarHandle northOffset = 0;
    ConfigVarHandle invertDirection = 0;
    ConfigVarHandle pinHotkey = 0;
    ConfigVarHandle pinModifier = 0;
    ConfigVarHandle pinButton = 0;
    ConfigVarHandle showChests = 0;
    ConfigVarHandle chestsNeedCompass = 0;
    ConfigVarHandle showLocation = 0;
    ConfigVarHandle locationX = 0;
    ConfigVarHandle locationY = 0;
};

extern Settings g_settings;
extern SettingsHandles g_settingHandles;

ModResult registerSettings();
void refreshSettings();

// North offset in degrees from the selected option index.
float northOffsetDegrees();

// GameCube pad bits for the pin hotkey options (0 = no modifier).
uint32_t pinModifierMask();
uint32_t pinButtonMask();

}  // namespace compass
