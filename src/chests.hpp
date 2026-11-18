#pragma once

#include <cstddef>

namespace compass {

enum class MarkerKind { Chest, GoldenBug, Poe, HeartPiece, HowlingStone };

// A marker to draw on the compass, already converted to world space.
struct Marker {
    MarkerKind kind;
    float bearing;   // radians, world space (atan2(dx, dz))
    float distance;  // horizontal distance from Link
    float dy;        // height difference (marker - Link)
};

constexpr size_t kMaxChestMarkers = 96;

// Fills `out` with the unopened chests of the current stage. Returns how many were written.
size_t collectChestMarkers(Marker* out, size_t max);

}  // namespace compass
