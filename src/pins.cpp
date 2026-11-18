#include "pins.hpp"

#include "entrances.hpp"
#include "game_state.hpp"
#include "worldmap.hpp"

#include <mods/svc/log.hpp>
#include <mods/svc/save.h>

#include <cstring>

namespace compass {

PinStore g_pins;

namespace {

constexpr const char* kBlobName = "pins_v2";
constexpr const char* kOldBlobName = "pins_v1";

// Layout of the first version (no world-map anchor).
struct PinV1 {
    char stage[8];
    float x, y, z;
};
struct PinStoreV1 {
    uint32_t version;
    uint32_t count;
    PinV1 pins[kMaxPins];
};

bool g_slotActive = false;

bool sameStage(const Pin& p, const char* stage) {
    return std::strncmp(p.stage, stage, sizeof(p.stage)) == 0;
}

void loadFromSlot() {
    PinStore loaded;
    size_t size = sizeof(loaded);
    if (svc_save->get_blob(mod_ctx, kBlobName, &loaded, &size) == MOD_OK && size == sizeof(loaded) &&
        loaded.version == 2 && loaded.count <= kMaxPins)
    {
        g_pins = loaded;
    } else {
        g_pins = PinStore{};
        // Migrate pins saved by older versions of the mod.
        static PinStoreV1 old;
        size = sizeof(old);
        if (svc_save->get_blob(mod_ctx, kOldBlobName, &old, &size) == MOD_OK && size == sizeof(old) &&
            old.count <= kMaxPins)
        {
            for (uint32_t i = 0; i < old.count; ++i) {
                Pin& p = g_pins.pins[i];
                std::memset(&p, 0, sizeof(p));
                std::memcpy(p.stage, old.pins[i].stage, sizeof(p.stage));
                p.x = old.pins[i].x;
                p.y = old.pins[i].y;
                p.z = old.pins[i].z;
            }
            g_pins.count = old.count;
        }
    }
    mods::log::info("pins loaded: {}", g_pins.count);
}

PinResult store() {
    // Written to the save file together with the next in-game save.
    if (svc_save->set_blob(mod_ctx, kBlobName, &g_pins, sizeof(g_pins)) != MOD_OK) {
        mods::log::warn("could not store pins (no active save slot?)");
        return PinResult::NoSave;
    }
    return PinResult::Ok;
}

void onNewSave(ModContext*, uint32_t, void*) {
    g_slotActive = true;
    g_pins = PinStore{};
}

void onSaveLoaded(ModContext*, uint32_t, void*) {
    g_slotActive = true;
    loadFromSlot();
}

}  // namespace

void initPins() {
    if (svc_save->observe_saves(mod_ctx, onNewSave, onSaveLoaded, nullptr, nullptr, nullptr) != MOD_OK) {
        mods::log::warn("observe_saves failed; pins will not persist");
    }
    // If the mod is (re)loaded mid-game, a slot may already be active.
    size_t size = 0;
    if (svc_save->get_blob(mod_ctx, kBlobName, nullptr, &size) == MOD_OK) {
        g_slotActive = true;
        loadFromSlot();
    }
}

PinResult addPinAtPlayer() {
    if (!g_state.hasPlayer || g_state.stage[0] == '\0') {
        return PinResult::NoPlayer;
    }
    if (g_pins.count >= kMaxPins) {
        return PinResult::Full;
    }
    Pin& p = g_pins.pins[g_pins.count++];
    std::memset(&p, 0, sizeof(p));
    std::strncpy(p.stage, g_state.stage, sizeof(p.stage));
    p.x = g_state.x;
    p.y = g_state.y;
    p.z = g_state.z;
    float ax, az;
    if (playerWorldPos(ax, az)) {
        p.anchorValid = 1;  // overworld: the pin itself
    } else if (const Entrance* e = findEntrance(g_state.stage, g_state.room)) {
        ax = e->x, az = e->z, p.anchorValid = 1;  // inside: its entrance
    } else if (lastFieldWorldPos(ax, az)) {
        p.anchorValid = 1;  // inside, entrance unknown: where Link last was outside
    }
    if (p.anchorValid) {
        p.anchorX = ax;
        p.anchorZ = az;
    }
    mods::log::info("pin added in {} at ({:.0f}, {:.0f}, {:.0f})", g_state.stage, p.x, p.y, p.z);
    return store();
}

PinResult togglePinAtPlayer() {
    if (!g_state.hasPlayer || g_state.stage[0] == '\0') {
        return PinResult::NoPlayer;
    }
    int best = -1;
    float bestDist2 = kPinToggleRadius * kPinToggleRadius;
    for (uint32_t i = 0; i < g_pins.count; ++i) {
        const Pin& p = g_pins.pins[i];
        if (!sameStage(p, g_state.stage)) {
            continue;
        }
        const float dx = p.x - g_state.x, dy = p.y - g_state.y, dz = p.z - g_state.z;
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 <= bestDist2) {
            bestDist2 = d2;
            best = static_cast<int>(i);
        }
    }
    if (best < 0) {
        return addPinAtPlayer();
    }
    for (uint32_t i = static_cast<uint32_t>(best); i + 1 < g_pins.count; ++i) {
        g_pins.pins[i] = g_pins.pins[i + 1];
    }
    --g_pins.count;
    mods::log::info("pin removed in {}", g_state.stage);
    PinResult r = store();
    return r == PinResult::Ok ? PinResult::Removed : r;
}

PinResult clearPinsInCurrentStage() {
    uint32_t kept = 0;
    for (uint32_t i = 0; i < g_pins.count; ++i) {
        if (!sameStage(g_pins.pins[i], g_state.stage)) {
            g_pins.pins[kept++] = g_pins.pins[i];
        }
    }
    if (kept == g_pins.count) {
        return PinResult::Nothing;
    }
    g_pins.count = kept;
    return store();
}

PinResult clearAllPins() {
    if (g_pins.count == 0) {
        return PinResult::Nothing;
    }
    g_pins.count = 0;
    return store();
}

void refreshPinAnchors() {
    if (!worldmapReady()) {
        return;
    }
    bool changed = false;
    for (uint32_t i = 0; i < g_pins.count; ++i) {
        Pin& p = g_pins.pins[i];
        if (p.anchorValid) {
            continue;
        }
        char stage[9] = {};
        std::memcpy(stage, p.stage, 8);
        float ox, oz;
        if (isFieldStage(stage) && stageWorldOffset(stage, -1, ox, oz)) {
            // Overworld pin from an older version: its map position is local + stage offset.
            p.anchorX = p.x + ox;
            p.anchorZ = p.z + oz;
            p.anchorValid = 1;
            changed = true;
        } else if (const Entrance* e = findEntrance(stage, -1)) {
            p.anchorX = e->x;
            p.anchorZ = e->z;
            p.anchorValid = 1;
            changed = true;
        }
    }
    if (changed) {
        store();
    }
}

size_t pinsInCurrentStage() {
    size_t n = 0;
    for (uint32_t i = 0; i < g_pins.count; ++i) {
        if (sameStage(g_pins.pins[i], g_state.stage)) {
            ++n;
        }
    }
    return n;
}

}  // namespace compass
