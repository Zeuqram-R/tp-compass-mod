#include "chests.hpp"

#include "game_state.hpp"
#include "settings.hpp"

#include "d/d_com_inf_game.h"
#include "d/d_map_path_dmap.h"
#include "d/d_stage.h"
#include "d/d_tresure.h"

#include <cmath>

namespace compass {
namespace {

constexpr float kS16ToRad = 3.14159265358979f / 32768.0f;

// Type groups the game's dungeon map uses for treasure boxes (see renderingDmap_c::isDrawIconSingle2).
constexpr unsigned char kChestGroups[] = {0, 2};

}  // namespace

size_t collectChestMarkers(Marker* out, size_t max) {
    if (!g_settings.showChests || !g_state.hasPlayer) {
        return 0;
    }
    if (g_settings.chestsNeedCompass && !dMapInfo_n::chkGetCompass()) {
        return 0;
    }

    // Treasure positions are stored in "map space": each room is rotated/offset by its
    // FileList2 entry. Put Link in that same space, then rotate the result back to world space.
    const Vec player = dMapInfo_n::getMapPlayerPos();
    float roomRot = 0.0f;
    if (dStage_FileList2_dt_c* fl = dStage_roomControl_c::getFileList2(dComIfGp_roomControl_getStayNo())) {
        roomRot = static_cast<float>(static_cast<s16>(fl->field_0x1c)) * kS16ToRad;
    }

    size_t n = 0;
    for (unsigned char group : kChestGroups) {
        const int count = dTres_c::getTypeGroupNumber(group);
        dTres_c::typeGroupData_c* data = dTres_c::getFirstData(group);
        for (int i = 0; i < count && data != nullptr && n < max; ++i, data = dTres_c::getNextData(data)) {
            const u8 no = data->getNo();
            if (no == 0xFF || dComIfGs_isTbox(no)) {
                continue;  // not a real chest, or already opened
            }
            const Vec pos = *data->getPos();
            const float dx = pos.x - player.x;
            const float dz = pos.z - player.z;
            const float dist = std::sqrt(dx * dx + dz * dz);
            if (dist < 1.0f) {
                continue;
            }
            out[n++] = Marker{MarkerKind::Chest, std::atan2(dx, dz) - roomRot, dist, pos.y - player.y};
        }
    }
    return n;
}

}  // namespace compass
