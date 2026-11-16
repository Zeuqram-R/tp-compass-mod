#include "game_state.hpp"

#include "settings.hpp"

#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_camera_mng.h"

#include <cmath>
#include <cstring>

namespace compass {

GameState g_state;

void updateGameState() {
    GameState s;

    fopAc_ac_c* player = dComIfGp_getPlayer(0);
    if (player != nullptr) {
        s.hasPlayer = true;
        s.x = player->current.pos.x;
        s.y = player->current.pos.y;
        s.z = player->current.pos.z;
        s.room = fopAcM_GetRoomNo(player);
    }

    const char* stage = dComIfGp_getStartStageName();
    if (stage != nullptr) {
        std::strncpy(s.stage, stage, sizeof(s.stage) - 1);
    }

    camera_process_class* cam = dComIfGp_getCamera(0);
    if (cam != nullptr) {
        const lookat_class& la = cam->view.lookat;
        const float dx = la.center.x - la.eye.x;
        const float dz = la.center.z - la.eye.z;
        if (dx * dx + dz * dz > 0.0001f) {
            s.hasCamera = true;
            s.cameraYaw = std::atan2(dx, dz);
        }
    }

    s.eventRunning = dComIfGp_event_runCheck() != FALSE;
    s.windowStatus = dMeter2Info_getWindowStatus();

    g_state = s;
}

bool shouldDrawCompass() {
    if (!g_settings.showCompass) {
        g_state.hiddenReason = "desactivado en opciones";
        return false;
    }
    if (!g_state.hasPlayer) {
        g_state.hiddenReason = "sin jugador";
        return false;
    }
    if (!g_state.hasCamera) {
        g_state.hiddenReason = "sin cámara";
        return false;
    }
    if (g_state.eventRunning) {
        g_state.hiddenReason = "cinemática / diálogo";
        return false;
    }
    if (g_state.windowStatus != 0) {
        g_state.hiddenReason = "menú abierto";
        return false;
    }
    g_state.hiddenReason = "";
    return true;
}

}  // namespace compass
