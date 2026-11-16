#include "pins.hpp"

#include "game_state.hpp"

#include <mods/svc/log.hpp>
#include <mods/svc/save.h>

#include <cstring>

namespace compass {

PinStore g_pins;

namespace {

constexpr const char* kBlobName = "pins_v1";

bool g_slotActive = false;

bool sameStage(const Pin& p, const char* stage) {
    return std::strncmp(p.stage, stage, sizeof(p.stage)) == 0;
}

void loadFromSlot() {
    PinStore loaded;
    size_t size = sizeof(loaded);
    if (svc_save->get_blob(mod_ctx, kBlobName, &loaded, &size) == MOD_OK && size == sizeof(loaded) &&
        loaded.version == 1 && loaded.count <= kMaxPins)
    {
        g_pins = loaded;
    } else {
        g_pins = PinStore{};
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
    mods::log::info("pin added in {} at ({:.0f}, {:.0f}, {:.0f})", g_state.stage, p.x, p.y, p.z);
    return store();
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
