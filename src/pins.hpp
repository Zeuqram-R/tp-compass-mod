#pragma once

#include <cstddef>
#include <cstdint>

namespace compass {

// A marker the player placed. Coordinates only make sense inside the stage they were taken in.
struct Pin {
    char stage[8];
    float x, y, z;
    // Where to point from other zones (world map). For pins inside dungeons/interiors this is
    // the entrance of that place.
    uint8_t anchorValid;
    uint8_t pad[3];
    float anchorX, anchorZ;
};

constexpr size_t kMaxPins = 64;

struct PinStore {
    uint32_t version = 2;
    uint32_t count = 0;
    Pin pins[kMaxPins] = {};
};

extern PinStore g_pins;

// Registers save observers so pins follow the active save slot.
void initPins();

// Results used by the UI to show feedback.
enum class PinResult { Ok, Removed, NoSave, NoPlayer, Full, Nothing };

PinResult addPinAtPlayer();
// Removes the closest pin within kPinToggleRadius of the player, otherwise adds one.
PinResult togglePinAtPlayer();
constexpr float kPinToggleRadius = 300.0f;
PinResult clearPinsInCurrentStage();
PinResult clearAllPins();

// Fills in world-map anchors that couldn't be computed when the pins were placed.
void refreshPinAnchors();

// Number of pins in the stage the player is in right now.
size_t pinsInCurrentStage();

}  // namespace compass
