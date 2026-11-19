#include "entrances.hpp"

#include "game_state.hpp"
#include "location.hpp"
#include "worldmap.hpp"

#include <mods/svc/log.hpp>
#include <mods/svc/save.h>

#include "d/d_com_inf_game.h"

#include <cstring>

namespace compass {
namespace {

constexpr const char* kBlobName = "entrances_v2";
constexpr const char* kOldBlobName = "entrances_v1";

struct EntranceV1 {
    char stage[8];
    int8_t room;
    EntranceKind kind;
    uint8_t pad[2];
    float x, z;
};
struct StoreV1 {
    uint32_t version;
    uint32_t count;
    EntranceV1 items[kMaxEntrances];
};

struct Store {
    uint32_t version = 2;
    uint32_t count = 0;
    Entrance items[kMaxEntrances] = {};
};

Store g_store;
char g_prevStage[9] = {};
bool g_prevWasField = false;
bool g_pending = false;  // stage changed from overworld, waiting for Link to appear
float g_lastX = 0.0f, g_lastZ = 0.0f;
bool g_lastValid = false;
char g_lastFieldStage[8] = {};

bool contains(const char* s, const char* needle) {
    return s != nullptr && std::strstr(s, needle) != nullptr;
}

EntranceKind classify(const char* stage) {
    if (std::strncmp(stage, "D_MN09", 6) == 0) {
        return EntranceKind::Castle;  // Hyrule Castle
    }
    if (stage[0] == 'D' && stage[1] == '_') {
        return EntranceKind::Dungeon;
    }
    const char* name = currentLocationName();
    if (contains(name, "Sundries") || contains(name, "Bombs") || contains(name, "Mart") ||
        contains(name, "Shop") || contains(name, "Coro"))
    {
        return EntranceKind::Shop;
    }
    return EntranceKind::Other;
}

void save() {
    svc_save->set_blob(mod_ctx, kBlobName, &g_store, sizeof(g_store));
}

void load() {
    Store s;
    size_t size = sizeof(s);
    if (svc_save->get_blob(mod_ctx, kBlobName, &s, &size) == MOD_OK && size == sizeof(s) && s.version == 2 &&
        s.count <= kMaxEntrances)
    {
        g_store = s;
        return;
    }
    g_store = Store{};
    static StoreV1 old;
    size = sizeof(old);
    if (svc_save->get_blob(mod_ctx, kOldBlobName, &old, &size) == MOD_OK && size == sizeof(old) &&
        old.count <= kMaxEntrances)
    {
        for (uint32_t i = 0; i < old.count; ++i) {
            std::memcpy(&g_store.items[i], &old.items[i], sizeof(EntranceV1));  // fieldStage stays empty
        }
        g_store.count = old.count;
    }
}

void onNewSave(ModContext*, uint32_t, void*) {
    g_store = Store{};
}

void onSaveLoaded(ModContext*, uint32_t, void*) {
    load();
}

void record(const char* stage, int room, float x, float z) {
    const bool dungeon = stage[0] == 'D' && stage[1] == '_';
    const int8_t key = dungeon ? -1 : static_cast<int8_t>(room);
    for (uint32_t i = 0; i < g_store.count; ++i) {
        Entrance& e = g_store.items[i];
        if (std::strncmp(e.stage, stage, 8) == 0 && e.room == key) {
            return;  // first visit wins: that's the main entrance
        }
    }
    if (g_store.count >= kMaxEntrances) {
        return;
    }
    Entrance& e = g_store.items[g_store.count++];
    std::memset(&e, 0, sizeof(e));
    std::strncpy(e.stage, stage, 8);
    e.room = key;
    e.kind = classify(stage);
    e.x = x;
    e.z = z;
    std::memcpy(e.fieldStage, g_lastFieldStage, sizeof(e.fieldStage));
    mods::log::info("learned entrance of {} room {} (kind {})", stage, room, static_cast<int>(e.kind));
    save();
}

}  // namespace

void initEntrances() {
    svc_save->observe_saves(mod_ctx, onNewSave, onSaveLoaded, nullptr, nullptr, nullptr);
    size_t size = 0;
    if (svc_save->get_blob(mod_ctx, kBlobName, nullptr, &size) == MOD_OK) {
        load();
    }
}

void updateEntrances() {
    const bool changed = std::strncmp(g_prevStage, g_state.stage, 8) != 0;
    if (changed) {
        if (g_prevWasField && g_lastValid && !isFieldStage(g_state.stage) && g_state.stage[0] != '\0') {
            g_pending = true;
        }
        std::strncpy(g_prevStage, g_state.stage, 8);
    }
    if (g_state.stage[0] == '\0') {
        return;
    }
    const bool field = isFieldStage(g_state.stage);
    if (field) {
        float x, z;
        if (playerWorldPos(x, z)) {
            g_lastX = x;
            g_lastZ = z;
            g_lastValid = true;
            std::strncpy(g_lastFieldStage, g_state.stage, sizeof(g_lastFieldStage));
        }
        g_pending = false;
    } else if (g_pending && g_state.hasPlayer) {
        // Room is known once Link exists in the new stage.
        record(g_state.stage, dComIfGp_roomControl_getStayNo(), g_lastX, g_lastZ);
        g_pending = false;
    }
    g_prevWasField = field;
}

size_t entranceCount() {
    return g_store.count;
}

const Entrance& entranceAt(size_t i) {
    return g_store.items[i];
}

const Entrance* findEntrance(const char* stage, int room) {
    const Entrance* any = nullptr;
    for (uint32_t i = 0; i < g_store.count; ++i) {
        const Entrance& e = g_store.items[i];
        if (std::strncmp(e.stage, stage, 8) != 0) {
            continue;
        }
        if (e.room == -1 || e.room == room) {
            return &e;
        }
        if (any == nullptr) {
            any = &e;
        }
    }
    return any;
}

bool lastFieldWorldPos(float& x, float& z) {
    if (!g_lastValid) {
        return false;
    }
    x = g_lastX;
    z = g_lastZ;
    return true;
}

}  // namespace compass
