#pragma once

#include <cstddef>
#include <cstdint>

namespace compass {

// Entrances the mod has learned: when Link walks from an overworld stage into a dungeon or an
// interior, the world-map spot he left from is remembered (per save) as that place's entrance.
enum class EntranceKind : uint8_t { Other = 0, Dungeon = 1, Shop = 2, Castle = 3 };

struct Entrance {
    char stage[8];
    int8_t room;  // -1 = any room (dungeons)
    EntranceKind kind;
    uint8_t pad[2];
    float x, z;   // world map position
    char fieldStage[8];  // overworld stage the entrance is in (empty if learned by an old version)
};

constexpr size_t kMaxEntrances = 96;

// Call every game frame (after updateGameState).
void updateEntrances();
void initEntrances();

size_t entranceCount();
const Entrance& entranceAt(size_t i);
// Entrance of a stage (+room for interiors that share a stage).
const Entrance* findEntrance(const char* stage, int room);

// Last world-map position Link had while in an overworld stage (for anchoring things placed
// inside places whose entrance isn't known yet).
bool lastFieldWorldPos(float& x, float& z);

}  // namespace compass
