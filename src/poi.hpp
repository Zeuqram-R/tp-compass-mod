#pragma once

#include "chests.hpp"

#include <cstddef>

namespace compass {

constexpr size_t kMaxPoiMarkers = 96;

// Points of interest on the world map: villages, Castle Town, unlocked Midna portals, and the
// dungeon/shop/castle entrances learned so far. Only while Link is in the overworld.
size_t collectPoiMarkers(Marker* out, size_t max);

// Current story objective (overworld only, never in randomizer mode). Returns false when there
// is nothing to show. `name` receives a short label for the HUD.
bool currentObjective(Marker& out, const char*& name);

// Bearing/distance from Link to a world-map point (fails outside the overworld).
bool worldBearing(float wx, float wz, float& bearing, float& distance);

}  // namespace compass
