#include "poi.hpp"

#include "entrances.hpp"
#include "game_state.hpp"
#include "settings.hpp"
#include "worldmap.hpp"

#include "d/d_com_inf_game.h"
#include "d/d_save.h"

#include <cmath>
#include <cstring>

namespace compass {
namespace {

struct StagePoi {
    const char* stage;
    MarkerKind kind;
};

// Overworld places shown from their spot on the world map.
constexpr StagePoi kStagePois[] = {
    {"F_SP103", MarkerKind::Village},  // Ordon Village
    {"F_SP109", MarkerKind::Village},  // Kakariko Village
    {"F_SP128", MarkerKind::Village},  // Hidden Village
    {"F_SP113", MarkerKind::ZoraDomain},  // Zora's Domain
    {"F_SP116", MarkerKind::TownCenter},  // Castle Town
};

constexpr float kNearEntranceDistance = 6000.0f;

bool sameStage(const char* a, const char* b) {
    return std::strncmp(a, b, 8) == 0;
}

void push(Marker* out, size_t max, size_t& n, MarkerKind kind, float wx, float wz) {
    float bearing, distance;
    if (n < max && worldBearing(wx, wz, bearing, distance)) {
        out[n++] = Marker{kind, bearing, distance, 0.0f};
    }
}

// ---- Story objective ----

struct Step {
    u16 doneFlag;            // dSv_event_flag_c label; 0 = final step
    const char* dungeon;     // stage whose learned entrance is the precise target (may be null)
    const char* fieldStage;  // overworld stage to point at otherwise
    const char* name;
};

const Step kSteps[] = {
    {dSv_event_flag_c::M_022, "D_MN05", "F_SP108", "Forest Temple"},
    {dSv_event_flag_c::M_031, "D_MN04", "F_SP110", "Goron Mines"},
    {dSv_event_flag_c::M_070, nullptr, "F_SP115", "Lanayru Province"},
    {dSv_event_flag_c::M_037, nullptr, "F_SP111", "Kakariko Graveyard"},
    {dSv_event_flag_c::M_045, "D_MN01", "F_SP115", "Lakebed Temple"},
    {dSv_event_flag_c::F_0264, nullptr, "F_SP117", "Sacred Grove (Master Sword)"},
    {dSv_event_flag_c::F_0265, "D_MN10", "F_SP124", "Arbiter's Grounds"},
    {dSv_event_flag_c::F_0266, "D_MN11", "F_SP114", "Snowpeak Ruins"},
    {dSv_event_flag_c::F_0267, "D_MN06", "F_SP117", "Temple of Time"},
    {dSv_event_flag_c::F_0268, "D_MN07", "F_SP115", "City in the Sky"},
    {dSv_event_flag_c::F_0570, "D_MN08", "F_SP125", "Palace of Twilight"},
    {0, "D_MN09", "F_SP116", "Hyrule Castle"},
};

}  // namespace

bool worldBearing(float wx, float wz, float& bearing, float& distance) {
    float px, pz;
    if (!playerWorldPos(px, pz)) {
        return false;
    }
    const float dx = wx - px, dz = wz - pz;
    distance = std::sqrt(dx * dx + dz * dz);
    if (distance < 1.0f) {
        return false;
    }
    bearing = std::atan2(dx, dz);
    return true;
}

size_t collectPoiMarkers(Marker* out, size_t max) {
    size_t n = 0;
    if (!g_settings.showPois || !isFieldStage(g_state.stage)) {
        return 0;
    }
    for (const StagePoi& p : kStagePois) {
        float ox, oz;
        if (!sameStage(p.stage, g_state.stage) && stageWorldOffset(p.stage, -1, ox, oz)) {
            push(out, max, n, p.kind, ox, oz);
        }
    }
    for (size_t i = 0; i < portalCount(); ++i) {
        PortalInfo info;
        // Only the portals of the map Link is in.
        if (portalAt(i, info) && info.unlocked && sameStage(info.stage, g_state.stage)) {
            push(out, max, n, MarkerKind::Portal, info.x, info.z);
        }
    }
    for (size_t i = 0; i < entranceCount(); ++i) {
        const Entrance& e = entranceAt(i);
        // Learned entrances only show up around them: in the overworld map they're in (or, for
        // ones learned by an older version that didn't record it, when reasonably close).
        if (e.fieldStage[0] != '\0') {
            if (!sameStage(e.fieldStage, g_state.stage)) {
                continue;
            }
        } else {
            float b, d;
            if (!worldBearing(e.x, e.z, b, d) || d > kNearEntranceDistance) {
                continue;
            }
        }
        switch (e.kind) {
        case EntranceKind::Dungeon:
            push(out, max, n, MarkerKind::Dungeon, e.x, e.z);
            break;
        case EntranceKind::Shop:
            push(out, max, n, MarkerKind::Shop, e.x, e.z);
            break;
        case EntranceKind::Castle:
            push(out, max, n, MarkerKind::Castle, e.x, e.z);
            break;
        default:
            break;
        }
    }
    return n;
}

bool currentObjective(Marker& out, const char*& name) {
    if (!g_settings.showObjective || g_settings.randomizerMode || !isFieldStage(g_state.stage)) {
        return false;
    }
    const Step* step = nullptr;
    for (const Step& s : kSteps) {
        if (s.doneFlag == 0 || !dComIfGs_isEventBit(s.doneFlag)) {
            step = &s;
            break;
        }
    }
    if (step == nullptr) {
        return false;
    }
    name = step->name;
    float wx, wz;
    const Entrance* e = step->dungeon ? findEntrance(step->dungeon, -1) : nullptr;
    if (e != nullptr) {
        wx = e->x, wz = e->z;
    } else if (!stageWorldOffset(step->fieldStage, -1, wx, wz)) {
        return false;
    }
    float bearing, distance;
    if (!worldBearing(wx, wz, bearing, distance)) {
        out = Marker{MarkerKind::Objective, 0.0f, 0.0f, 0.0f};
        return false;
    }
    out = Marker{MarkerKind::Objective, bearing, distance, 0.0f};
    return true;
}

}  // namespace compass
