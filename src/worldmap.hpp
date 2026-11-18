#pragma once

#include <cstddef>
#include <cstdint>

namespace compass {

// The game's field map data (dat/field.dat + dat/portal.dat) puts every overworld stage on one
// shared plane ("world map" coordinates). That's what lets the compass point to things in other
// zones. Loaded lazily from the resident field-map archive.
bool worldmapReady();

// True when `stage` is an overworld stage that has a place on the world map.
bool isFieldStage(const char* stage);

// World-map offset of a stage (local map position + offset = world map position).
bool stageWorldOffset(const char* stage, int room, float& ox, float& oz);

// Link's world-map position. Only available in overworld stages.
bool playerWorldPos(float& x, float& z);

struct PortalInfo {
    char stage[9];
    float x, z;     // world map position
    bool unlocked;  // Link has opened this Midna portal
};
size_t portalCount();
bool portalAt(size_t i, PortalInfo& out);

}  // namespace compass
