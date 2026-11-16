#pragma once

#include <mods/api.h>

namespace compass {

// Hooks the game's HUD draw so the compass is queued into the 2D draw list every frame.
ModResult installCompassDraw();

}  // namespace compass
