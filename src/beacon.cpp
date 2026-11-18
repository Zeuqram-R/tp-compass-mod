#include "beacon.hpp"

#include "game_state.hpp"
#include "pins.hpp"
#include "settings.hpp"

#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "JSystem/J3DGraphBase/J3DPacket.h"
#include "JSystem/J3DGraphBase/J3DSys.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_camera_mng.h"
#include <dolphin/gx.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace compass {
namespace {

constexpr float kBeamHeight = 1800.0f;

// A vertical quad that always faces the camera, brighter at the base and fading out upwards.
void beam(const lookat_class& la, float x, float y, float z, float halfWidth, u8 r, u8 g, u8 b, u8 alpha) {
    float dx = la.eye.x - x;
    float dz = la.eye.z - z;
    const float len = std::sqrt(dx * dx + dz * dz);
    if (len < 1.0f) {
        return;
    }
    // Perpendicular to the camera direction, on the ground plane.
    const float rx = dz / len * halfWidth;
    const float rz = -dx / len * halfWidth;
    const float top = y + kBeamHeight;

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(x - rx, y, z - rz);
    GXColor4u8(r, g, b, alpha);
    GXPosition3f32(x + rx, y, z + rz);
    GXColor4u8(r, g, b, alpha);
    GXPosition3f32(x + rx, top, z + rz);
    GXColor4u8(r, g, b, 0);
    GXPosition3f32(x - rx, top, z - rz);
    GXColor4u8(r, g, b, 0);
    GXEnd();
}

class BeaconPacket : public J3DPacket {
public:
    void draw() override;
};

BeaconPacket g_packet;

void BeaconPacket::draw() {
    camera_process_class* cam = dComIfGp_getCamera(0);
    if (cam == nullptr) {
        return;
    }
    const lookat_class& la = cam->view.lookat;

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    // Additive glow, hidden behind walls, without writing depth.
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
    GXSetZMode(GX_ENABLE, GX_LEQUAL, GX_DISABLE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
    GXSetCullMode(GX_CULL_NONE);
    GXLoadPosMtxImm(j3dSys.getViewMtx(), GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);

    const float near2 = kPinToggleRadius * kPinToggleRadius;
    for (uint32_t i = 0; i < g_pins.count; ++i) {
        const Pin& p = g_pins.pins[i];
        if (std::strncmp(p.stage, g_state.stage, sizeof(p.stage)) != 0) {
            continue;
        }
        const float dx = p.x - g_state.x, dy = p.y - g_state.y, dz = p.z - g_state.z;
        const bool inRange = dx * dx + dy * dy + dz * dz <= near2;
        // Gold when Link is close enough for the hotkey to remove it, white otherwise.
        const u8 r = 255, g = inRange ? 205 : 255, b = inRange ? 90 : 255;
        beam(la, p.x, p.y, p.z, 26.0f, r, g, b, inRange ? 110 : 70);  // soft glow
        beam(la, p.x, p.y, p.z, 7.0f, r, g, b, inRange ? 230 : 170);  // bright core
    }

    GXSetCullMode(GX_CULL_BACK);
    GXSetZMode(GX_ENABLE, GX_LEQUAL, GX_ENABLE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
}

}  // namespace

void queuePinBeacons() {
    if (!g_settings.showPins || !g_settings.pinBeacons || !g_state.hasPlayer || g_pins.count == 0) {
        return;
    }
    J3DDrawBuffer* xlu = dComIfGd_getXluListBG();
    if (xlu != nullptr) {
        xlu->entryImm(&g_packet, 0);
    }
}

}  // namespace compass
