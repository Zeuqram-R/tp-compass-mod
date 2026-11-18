#include "location.hpp"

#include "game_state.hpp"

#include <cstring>

namespace compass {
namespace {

struct LocationEntry {
    const char* stage;
    int room;  // -1 = any room of the stage
    const char* name;
};

constexpr LocationEntry kLocations[] = {
#include "location_names.inc"
};

}  // namespace

const char* currentLocationName() {
    if (g_state.stage[0] == '\0') {
        return nullptr;
    }
    const char* fallback = nullptr;
    for (const LocationEntry& e : kLocations) {
        if (std::strcmp(e.stage, g_state.stage) != 0) {
            continue;
        }
        if (e.room == g_state.room) {
            return e.name;
        }
        if (fallback == nullptr) {
            fallback = e.name;
        }
    }
    return fallback;
}

}  // namespace compass
