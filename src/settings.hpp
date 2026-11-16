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
};

extern Settings g_settings;
extern SettingsHandles g_settingHandles;

ModResult registerSettings();
void refreshSettings();

// North offset in degrees from the selected option index.
float northOffsetDegrees();

}  // namespace compass
