#pragma once

#include <cstddef>
#include <cstdint>

namespace compass {

// A marker the player placed. Coordinates only make sense inside the stage they were taken in.
struct Pin {
    char stage[8];
    float x, y, z;
};

constexpr size_t kMaxPins = 64;

struct PinStore {
    uint32_t version = 1;
    uint32_t count = 0;
    Pin pins[kMaxPins] = {};
};

extern PinStore g_pins;

// Registers save observers so pins follow the active save slot.
void initPins();

// Results used by the UI to show feedback.
enum class PinResult { Ok, NoSave, NoPlayer, Full, Nothing };

PinResult addPinAtPlayer();
PinResult clearPinsInCurrentStage();
PinResult clearAllPins();

// Number of pins in the stage the player is in right now.
size_t pinsInCurrentStage();

}  // namespace compass
