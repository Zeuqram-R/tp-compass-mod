#pragma once

#include "chests.hpp"

#include <cstddef>

namespace compass {

constexpr size_t kMaxCollectibles = 64;

// Scans the actors that are currently loaded for collectibles still to be picked up:
// golden bugs, Poes, freestanding Pieces of Heart and Howling Stones whose song is still
// unlearned. Collected ones don't spawn, so they never show up. Call once per frame.
void updateCollectibles();

// Markers for what updateCollectibles() found, relative to Link's current position.
size_t collectCollectibleMarkers(Marker* out, size_t max);

}  // namespace compass
