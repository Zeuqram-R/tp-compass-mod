#pragma once

#include <mods/api.h>

namespace compass {

// Pin hotkey: hold the modifier and press the button (defaults: Z + D-pad left).
// While the combo is held, the button is hidden from the game and from Dusklight's
// action bindings (so it doesn't call Midna or open anything).
ModResult installInput();

}  // namespace compass
