#include "worldmap.hpp"

#include "game_state.hpp"

#include <mods/svc/log.hpp>

#include "JSystem/JKernel/JKRAramArchive.h"
#include "d/d_com_inf_game.h"
#include "d/d_lib.h"
#include "d/d_map_path_dmap.h"
#include "d/d_menu_fmap.h"
#include "d/d_menu_map_common.h"

#include <cstring>
#include <memory>

namespace compass {
namespace {

std::unique_ptr<uint8_t[]> g_field;
std::unique_ptr<uint8_t[]> g_portal;
bool g_tried = false;

std::unique_ptr<uint8_t[]> readArc(JKRAramArchive* arc, const char* path) {
    const u32 size = dLib_getExpandSizeFromAramArchive(arc, path);
    if (size == 0) {
        return nullptr;
    }
    std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[size + 32]);
    if (!buf || arc->readResource(buf.get(), size, path) == 0) {
        return nullptr;
    }
    return buf;
}

bool load() {
    if (g_field) {
        return true;
    }
    if (g_tried) {
        return false;
    }
    JKRAramArchive* arc = dComIfGp_getFieldMapArchive2();
    if (arc == nullptr) {
        return false;  // not mounted yet; try again later
    }
    g_tried = true;
    g_field = readArc(arc, "dat/field.dat");
    g_portal = readArc(arc, "dat/portal.dat");
    mods::log::info("world map data: field={} portal={}", g_field != nullptr, g_portal != nullptr);
    return g_field != nullptr;
}

const dMenu_Fmap_field_data_c* fieldHeader() {
    return reinterpret_cast<const dMenu_Fmap_field_data_c*>(g_field.get());
}

const dMenuMapCommon_c::Stage_c* stages() {
    const auto* h = fieldHeader();
    return reinterpret_cast<const dMenuMapCommon_c::Stage_c*>(g_field.get() + static_cast<u32>(h->mStageDataOffset));
}

const dMenu_Fmap_field_region_data_c* regions() {
    const auto* h = fieldHeader();
    return reinterpret_cast<const dMenu_Fmap_field_region_data_c*>(g_field.get() +
                                                                    static_cast<u32>(h->mRegionDataOffset));
}

bool regionOrigin(u8 regionNo, float& ox, float& oz) {
    const auto* r = regions();
    for (int i = 0; i < r->mCount; ++i) {
        if (r->mData[i].mTextureReadNum == regionNo) {
            ox = r->mData[i].mOriginX;
            oz = r->mData[i].mOriginZ;
            return true;
        }
    }
    return false;
}

const dMenuMapCommon_c::Stage_c::data* findStage(const char* stage, int room) {
    const auto* s = stages();
    const dMenuMapCommon_c::Stage_c::data* first = nullptr;
    for (int i = 0; i < s->mCount; ++i) {
        const auto& d = s->mData[i];
        if (std::strncmp(d.mName, stage, 8) != 0) {
            continue;
        }
        if (room >= 0 && d.mRoomNo == room) {
            return &d;
        }
        if (first == nullptr) {
            first = &d;
        }
    }
    return first;
}

}  // namespace

bool worldmapReady() {
    return load();
}

bool isFieldStage(const char* stage) {
    return load() && stage != nullptr && stage[0] != '\0' && findStage(stage, -1) != nullptr;
}

bool stageWorldOffset(const char* stage, int room, float& ox, float& oz) {
    if (!load()) {
        return false;
    }
    const auto* d = findStage(stage, room);
    if (d == nullptr) {
        return false;
    }
    float rx = 0.0f, rz = 0.0f;
    if (!regionOrigin(d->mRegionNo, rx, rz)) {
        return false;
    }
    ox = rx + static_cast<float>(d->mOffsetX);
    oz = rz + static_cast<float>(d->mOffsetZ);
    return true;
}

bool playerWorldPos(float& x, float& z) {
    if (!g_state.hasPlayer || !isFieldStage(g_state.stage)) {
        return false;
    }
    float ox, oz;
    if (!stageWorldOffset(g_state.stage, dComIfGp_roomControl_getStayNo(), ox, oz)) {
        return false;
    }
    const Vec p = dMapInfo_n::getMapPlayerPos();
    x = p.x + ox;
    z = p.z + oz;
    return true;
}

size_t portalCount() {
    if (!load() || !g_portal) {
        return 0;
    }
    return reinterpret_cast<const dMenu_Fmap_portal_data_c*>(g_portal.get())->mCount;
}

bool portalAt(size_t i, PortalInfo& out) {
    if (i >= portalCount()) {
        return false;
    }
    const auto& d = reinterpret_cast<const dMenu_Fmap_portal_data_c*>(g_portal.get())->mData[i];
    std::memset(out.stage, 0, sizeof(out.stage));
    std::memcpy(out.stage, d.mStageName, 8);
    const Vec pos = d.mPosition;
    out.x = pos.x;
    out.z = pos.z;
    out.unlocked = dComIfGs_isStageSwitch(d.mStageNo, d.mSwitchNo) != FALSE;
    return true;
}

}  // namespace compass
