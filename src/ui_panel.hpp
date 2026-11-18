#pragma once

#include <mods/api.h>

#include "pins.hpp"

namespace compass {

// Registers the panel shown for this mod inside Dusklight's Mods window.
ModResult registerUiPanel();

// Shows a short toast for the result of the pin hotkey.
void notifyHotkeyPinResult(PinResult r);

}  // namespace compass
