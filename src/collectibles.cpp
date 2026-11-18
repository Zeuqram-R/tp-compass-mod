#include "collectibles.hpp"

#include "game_state.hpp"
#include "settings.hpp"

#include "d/actor/d_a_obj_item.h"
#include "d/actor/d_a_obj_wind_stone.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "f_op/f_op_actor_iter.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"

#include <cmath>
#include <cstring>

namespace compass {
namespace {

struct Found {
    MarkerKind kind;
    float x, y, z;
};

Found g_found[kMaxCollectibles];
size_t g_foundCount = 0;

bool isGoldenBug(s16 name) {
    // Obj_Kabuto .. Obj_Kat: the 12 golden bug species (male and female share the actor).
    return name >= fpcNm_Obj_Kabuto_e && name <= fpcNm_Obj_Kat_e;
}

bool classify(fopAc_ac_c* actor, MarkerKind& kind) {
    const s16 name = fopAcM_GetName(actor);
    if (isGoldenBug(name)) {
        if (!g_settings.showBugs || std::strcmp(g_state.stage, "R_SP160") == 0) {
            return false;  // Agitha's castle shows the bugs you already gave her
        }
        kind = MarkerKind::GoldenBug;
        return true;
    }
    if (name == fpcNm_E_PO_e || name == fpcNm_E_HP_e) {
        kind = MarkerKind::Poe;
        return g_settings.showPoes;
    }
    if (name == fpcNm_ITEM_e) {
        daItem_c* item = static_cast<daItem_c*>(actor);
        if (g_settings.randomizerMode) {
            // Any one-time pickup (it has a save bit) is a check; its contents are shuffled.
            if (daItem_prm::getItemBitNo(item) == 0xFF) {
                return false;
            }
            kind = MarkerKind::Item;
            return g_settings.showHeartPieces;
        }
        if (daItem_prm::getItemNo(item) != dItemNo_KAKERA_HEART_e) {
            return false;
        }
        kind = MarkerKind::HeartPiece;
        return g_settings.showHeartPieces;
    }
    if (name == fpcNm_Obj_WindStone_e) {
        // Only stones whose song Link hasn't learned yet.
        if (!static_cast<daWindStone_c*>(actor)->chkEveOccur()) {
            return false;
        }
        kind = MarkerKind::HowlingStone;
        return g_settings.showHowlingStones;
    }
    return false;
}

int scanActor(void* p, void*) {
    if (g_foundCount >= kMaxCollectibles) {
        return 0;
    }
    fopAc_ac_c* actor = static_cast<fopAc_ac_c*>(p);
    MarkerKind kind;
    if (actor != nullptr && classify(actor, kind)) {
        g_found[g_foundCount++] = Found{kind, actor->current.pos.x, actor->current.pos.y, actor->current.pos.z};
    }
    return 1;
}

}  // namespace

void updateCollectibles() {
    g_foundCount = 0;
    if (!g_state.hasPlayer) {
        return;
    }
    if (!g_settings.showBugs && !g_settings.showPoes && !g_settings.showHeartPieces &&
        !g_settings.showHowlingStones)
    {
        return;
    }
    fopAcIt_Executor(scanActor, nullptr);
}

size_t collectCollectibleMarkers(Marker* out, size_t max) {
    size_t n = 0;
    for (size_t i = 0; i < g_foundCount && n < max; ++i) {
        const Found& f = g_found[i];
        const float dx = f.x - g_state.x;
        const float dz = f.z - g_state.z;
        const float dist = std::sqrt(dx * dx + dz * dz);
        if (dist < 1.0f) {
            continue;
        }
        out[n++] = Marker{f.kind, std::atan2(dx, dz), dist, f.y - g_state.y};
    }
    return n;
}

}  // namespace compass
