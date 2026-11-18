#include "beacon.hpp"

#include "game_state.hpp"
#include "pins.hpp"
#include "settings.hpp"

#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "JSystem/J3DGraphBase/J3DPacket.h"
#include "JSystem/J3DGraphBase/J3DSys.h"
#include "SSystem/SComponent/c_math.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_camera_mng.h"
#include <dolphin/gx.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace compass {
namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kBeamHeight = 1800.0f;
constexpr float kParticleRange = 4000.0f;  // only pins this close get sparkles

struct Rgb {
    u8 r, g, b;
};

// Far away: cool white-blue, like the Master Sword's glow.
constexpr Rgb kFarCore{255, 255, 255};
constexpr Rgb kFarGlow{170, 215, 255};
// Close enough to remove: warm gold, like the Majora's Mask aura.
constexpr Rgb kNearCore{255, 244, 200};
constexpr Rgb kNearGlow{255, 176, 60};

bool inRemoveRange(const Pin& p) {
    const float dx = p.x - g_state.x, dy = p.y - g_state.y, dz = p.z - g_state.z;
    return dx * dx + dy * dy + dz * dz <= kPinToggleRadius * kPinToggleRadius;
}

bool pinInStage(const Pin& p) {
    return std::strncmp(p.stage, g_state.stage, sizeof(p.stage)) == 0;
}

void vtx(float x, float y, float z, const Rgb& c, float alpha) {
    GXPosition3f32(x, y, z);
    GXColor4u8(c.r, c.g, c.b, static_cast<u8>(std::clamp(alpha, 0.0f, 255.0f)));
}

// A camera-facing vertical band. The alpha falls off smoothly from the centre line to both
// edges (several steps of a smoothstep curve) and from the base to the top, so layers blend
// into each other without a visible edge.
void softBeam(float rx, float rz, float x, float y, float z, float halfWidth, const Rgb& c, float alpha) {
    constexpr int kSteps = 4;
    const float top = y + kBeamHeight;
    auto falloff = [](float t) {  // t: 0 centre .. 1 edge
        const float u = 1.0f - t;
        return u * u * (3.0f - 2.0f * u);
    };
    GXBegin(GX_QUADS, GX_VTXFMT0, kSteps * 2 * 4);
    for (int side = -1; side <= 1; side += 2) {
        for (int i = 0; i < kSteps; ++i) {
            const float t0 = static_cast<float>(i) / kSteps;
            const float t1 = static_cast<float>(i + 1) / kSteps;
            const float a0 = alpha * falloff(t0), a1 = alpha * falloff(t1);
            const float o0 = side * t0 * halfWidth, o1 = side * t1 * halfWidth;
            vtx(x + rx * o0, y, z + rz * o0, c, a0);
            vtx(x + rx * o1, y, z + rz * o1, c, a1);
            vtx(x + rx * o1, top, z + rz * o1, c, 0.0f);
            vtx(x + rx * o0, top, z + rz * o0, c, 0.0f);
        }
    }
    GXEnd();
}

// A round, camera-facing glow (bright centre fading to nothing), used as the aura at the base.
void glowDisc(MtxP view, float x, float y, float z, float radius, const Rgb& c, float alpha) {
    // Camera right/up vectors are the first two rows of the view matrix.
    const float rx = view[0][0], ry = view[0][1], rz = view[0][2];
    const float ux = view[1][0], uy = view[1][1], uz = view[1][2];
    constexpr int kSeg = 20;
    GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, kSeg + 2);
    vtx(x, y, z, c, alpha);
    for (int i = 0; i <= kSeg; ++i) {
        const float a = static_cast<float>(i) / kSeg * 2.0f * kPi;
        const float cs = std::cos(a) * radius, sn = std::sin(a) * radius;
        vtx(x + rx * cs + ux * sn, y + ry * cs + uy * sn, z + rz * cs + uz * sn, c, 0.0f);
    }
    GXEnd();
}

class BeaconPacket : public J3DPacket {
public:
    void draw() override;
};

BeaconPacket g_packet;
u32 g_frame = 0;

void BeaconPacket::draw() {
    camera_process_class* cam = dComIfGp_getCamera(0);
    if (cam == nullptr) {
        return;
    }
    const lookat_class& la = cam->view.lookat;
    MtxP view = j3dSys.getViewMtx();

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
    // Ignore the scene fog (wolf senses use a very short one that hid the pillar from afar).
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, GXColor{0, 0, 0, 0});
    GXLoadPosMtxImm(view, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);

    // Slow breathing so the aura feels alive.
    const float pulse = 0.85f + 0.15f * std::sin(static_cast<float>(g_frame) * 0.08f);

    for (uint32_t i = 0; i < g_pins.count; ++i) {
        const Pin& p = g_pins.pins[i];
        if (!pinInStage(p)) {
            continue;
        }
        float dx = la.eye.x - p.x;
        float dz = la.eye.z - p.z;
        const float len = std::sqrt(dx * dx + dz * dz);
        if (len < 1.0f) {
            continue;
        }
        // Unit vector perpendicular to the camera direction, on the ground plane.
        const float rx = dz / len, rz = -dx / len;

        const bool near = inRemoveRange(p);
        const Rgb& core = near ? kNearCore : kFarCore;
        const Rgb& glow = near ? kNearGlow : kFarGlow;

        softBeam(rx, rz, p.x, p.y, p.z, 80.0f, glow, 55.0f * pulse);   // wide haze
        softBeam(rx, rz, p.x, p.y, p.z, 30.0f, glow, 120.0f * pulse);  // body
        softBeam(rx, rz, p.x, p.y, p.z, 10.0f, core, 230.0f);          // bright core
        glowDisc(view, p.x, p.y + 60.0f, p.z, near ? 150.0f : 120.0f, glow, (near ? 150.0f : 110.0f) * pulse);
        glowDisc(view, p.x, p.y + 40.0f, p.z, 55.0f, core, 120.0f * pulse);
    }

    GXSetCullMode(GX_CULL_BACK);
    GXSetZMode(GX_ENABLE, GX_LEQUAL, GX_ENABLE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
}

// ---- Particles: the same game effects the Majora's Mask mod uses for its aura ----
// 0x109 = small glowing specks, 0x108 = soft smoke. Each emitter is re-triggered with its own
// random timer, which keeps it alive and moves it.

struct ParticleFx {
    u16 id;
    float scale;
    u8 alpha;
    int minTicks, randTicks;
    float height;  // max height of the random spawn point above the pin
};

constexpr ParticleFx kSparkle{0x109, 0.6f, 120, 4, 6, 260.0f};
constexpr ParticleFx kSmoke{0x108, 0.5f, 40, 6, 8, 80.0f};

struct PinEmitters {
    u32 sparkle = 0, smoke = 0;
    int sparkleTimer = 0, smokeTimer = 0;
};

PinEmitters g_emitters[kMaxPins];
char g_emitterStage[8] = {};
uint32_t g_emitterPinCount = 0;

void stopEmitter(u32& handle) {
    if (handle != 0) {
        dComIfGp_particle_setStopContinue(handle);
        handle = 0;
    }
}

void stopAll() {
    for (auto& e : g_emitters) {
        stopEmitter(e.sparkle);
        stopEmitter(e.smoke);
        e.sparkleTimer = e.smokeTimer = 0;
    }
}

int nextTicks(const ParticleFx& fx) {
    return std::max(1, fx.minTicks + static_cast<int>(cM_rndF(static_cast<float>(fx.randTicks))));
}

void emit(u32& handle, int& timer, const ParticleFx& fx, const Pin& p, const dKy_tevstr_c* tev,
    const GXColor* tint)
{
    if (--timer > 0) {
        return;
    }
    timer = nextTicks(fx);
    const float ang = cM_rndF(2.0f * kPi);
    const float rad = cM_rndF(45.0f);
    cXyz pos(p.x + std::cos(ang) * rad, p.y + cM_rndF(fx.height), p.z + std::sin(ang) * rad);
    cXyz scale(fx.scale, fx.scale, fx.scale);
    csXyz rot(0, 0, 0);
    handle = dComIfGp_particle_set(handle, fx.id, &pos, tev, &rot, &scale, fx.alpha, nullptr, -1, tint,
        tint, nullptr);
}

}  // namespace

void queuePinBeacons() {
    ++g_frame;
    if (!g_settings.showPins || !g_settings.pinBeacons || !g_state.hasPlayer || g_pins.count == 0) {
        return;
    }
    J3DDrawBuffer* xlu = dComIfGd_getXluListBG();
    if (xlu != nullptr) {
        xlu->entryImm(&g_packet, 0);
    }
}

void updateBeaconParticles() {
    // Pins were added/removed or the stage changed: slots no longer match their emitters.
    if (g_emitterPinCount != g_pins.count || std::strncmp(g_emitterStage, g_state.stage, sizeof(g_emitterStage)) != 0) {
        stopAll();
        g_emitterPinCount = g_pins.count;
        std::strncpy(g_emitterStage, g_state.stage, sizeof(g_emitterStage));
    }
    fopAc_ac_c* player = dComIfGp_getPlayer(0);
    if (player == nullptr || !g_settings.showPins || !g_settings.pinBeacons || g_state.eventRunning) {
        stopAll();
        return;
    }
    static const GXColor kWhite{235, 245, 255, 255};
    for (uint32_t i = 0; i < g_pins.count && i < kMaxPins; ++i) {
        const Pin& p = g_pins.pins[i];
        PinEmitters& e = g_emitters[i];
        const float dx = p.x - g_state.x, dz = p.z - g_state.z;
        if (!pinInStage(p) || dx * dx + dz * dz > kParticleRange * kParticleRange) {
            stopEmitter(e.sparkle);
            stopEmitter(e.smoke);
            continue;
        }
        if (inRemoveRange(p)) {
            // Native colours of the mask's aura: warm orange specks and smoke.
            emit(e.sparkle, e.sparkleTimer, kSparkle, p, &player->tevStr, nullptr);
            emit(e.smoke, e.smokeTimer, kSmoke, p, &player->tevStr, nullptr);
        } else {
            emit(e.sparkle, e.sparkleTimer, kSparkle, p, &player->tevStr, &kWhite);
            stopEmitter(e.smoke);
        }
    }
}

}  // namespace compass
