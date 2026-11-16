#include "compass_draw.hpp"

#include "game_state.hpp"
#include "pins.hpp"
#include "settings.hpp"

#include <mods/svc/hook.hpp>
#include <mods/svc/log.hpp>

#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/JUtility/TColor.h"
#include "d/d_com_inf_game.h"
#include "d/d_drawlist.h"
#include "d/d_meter2.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_graphic.h"
#include <dolphin/gx.h>

#include <algorithm>
#include <cmath>
#include <cstring>

DEFINE_HOOK(&dMeter2_c::_draw, Meter2Draw);

namespace compass {
namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kDegToRad = kPi / 180.0f;
// Fraction of each half of the bar used for the soft fade at the ends.
constexpr float kEdgeFade = 0.35f;

float wrapAngle(float a) {
    while (a > kPi) a -= 2.0f * kPi;
    while (a < -kPi) a += 2.0f * kPi;
    return a;
}

u8 scaleAlpha(float base, float factor) {
    float a = base * factor * (static_cast<float>(g_settings.opacity) / 100.0f);
    return static_cast<u8>(std::clamp(a, 0.0f, 255.0f));
}

// ---- compass layout ----

struct Layout {
    float cx;
    float left;
    float right;
    float top;
    float bottom;
    float halfWidth;
    float halfArc;  // radians
};

Layout computeLayout() {
    Layout l;
    const float safeMin = mDoGph_gInf_c::getSafeMinXF();
    const float safeMax = mDoGph_gInf_c::getSafeMaxXF();
    const float safeTop = mDoGph_gInf_c::getSafeMinYF();
    const float width = std::clamp(static_cast<float>(g_settings.width), 120.0f, safeMax - safeMin);
    l.cx = (safeMin + safeMax) * 0.5f;
    l.halfWidth = width * 0.5f;
    l.left = l.cx - l.halfWidth;
    l.right = l.cx + l.halfWidth;
    l.top = safeTop + static_cast<float>(g_settings.posY);
    l.bottom = l.top + 16.0f;
    l.halfArc = std::clamp(static_cast<float>(g_settings.arcDegrees), 30.0f, 360.0f) * 0.5f * kDegToRad;
    return l;
}

// ---- tiny immediate-mode helpers on top of GX (same setup the game uses for dDlst_2DQuad_c) ----

void beginFlat(const GXColor& color) {
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
        GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, color);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXLoadPosMtxImm(cMtx_getIdentity(), GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

void rect(float x0, float y0, float x1, float y1, const GXColor& color) {
    beginFlat(color);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(x0, y0, 0.0f);
    GXPosition3f32(x1, y0, 0.0f);
    GXPosition3f32(x1, y1, 0.0f);
    GXPosition3f32(x0, y1, 0.0f);
    GXEnd();
}

// Quad whose left and right edges have different colors (used to fade the bar's ends).
void hGradientRect(float x0, float y0, float x1, float y1, const GXColor& left, const GXColor& right) {
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE,
        GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXLoadPosMtxImm(cMtx_getIdentity(), GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(x0, y0, 0.0f);
    GXColor4u8(left.r, left.g, left.b, left.a);
    GXPosition3f32(x1, y0, 0.0f);
    GXColor4u8(right.r, right.g, right.b, right.a);
    GXPosition3f32(x1, y1, 0.0f);
    GXColor4u8(right.r, right.g, right.b, right.a);
    GXPosition3f32(x0, y1, 0.0f);
    GXColor4u8(left.r, left.g, left.b, left.a);
    GXEnd();
}

// A horizontal band that is solid in the middle and fades to transparent at both ends.
// The fade is split into steps with an ease-in curve so it reads as soft, not linear.
void fadedBand(const Layout& l, float y0, float y1, u8 r, u8 g, u8 b, float baseAlpha) {
    const float fadeW = l.halfWidth * kEdgeFade;
    constexpr int kSteps = 6;
    auto alphaAt = [&](float f) {  // f: 0 at the outer edge, 1 where the band is solid
        return scaleAlpha(baseAlpha, f * f * (3.0f - 2.0f * f));  // smoothstep
    };
    for (int i = 0; i < kSteps; ++i) {
        const float f0 = static_cast<float>(i) / kSteps;
        const float f1 = static_cast<float>(i + 1) / kSteps;
        const GXColor c0{r, g, b, alphaAt(f0)};
        const GXColor c1{r, g, b, alphaAt(f1)};
        // Left end: transparent -> solid.
        hGradientRect(l.left + fadeW * f0, y0, l.left + fadeW * f1, y1, c0, c1);
        // Right end: solid -> transparent.
        hGradientRect(l.right - fadeW * f1, y0, l.right - fadeW * f0, y1, c1, c0);
    }
    rect(l.left + fadeW, y0, l.right - fadeW, y1, GXColor{r, g, b, scaleAlpha(baseAlpha, 1.0f)});
}

void diamond(float cx, float cy, float r, const GXColor& color) {
    beginFlat(color);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(cx, cy - r, 0.0f);
    GXPosition3f32(cx + r, cy, 0.0f);
    GXPosition3f32(cx, cy + r, 0.0f);
    GXPosition3f32(cx - r, cy, 0.0f);
    GXEnd();
}

void triangleDown(float cx, float top, float halfW, float h, const GXColor& color) {
    beginFlat(color);
    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    GXPosition3f32(cx - halfW, top, 0.0f);
    GXPosition3f32(cx + halfW, top, 0.0f);
    GXPosition3f32(cx, top + h, 0.0f);
    GXEnd();
}

// Maps a world bearing (atan2(dx, dz)) to an x position on the bar.
// Returns false when the bearing is outside the visible arc. `edgeFade` is 1 in the
// middle and falls to 0 at the ends of the bar.
bool projectBearing(const Layout& l, float bearing, float& outX, float& edgeFade) {
    float rel = wrapAngle(bearing - g_state.cameraYaw);
    if (std::fabs(rel) > l.halfArc) {
        return false;
    }
    // Verified in game: the camera yaw grows when turning right, so a larger relative
    // angle sits further right on the bar.
    float t = rel / l.halfArc;
    if (g_settings.invertDirection) {
        t = -t;
    }
    outX = l.cx + t * l.halfWidth;
    float edge = 1.0f - std::fabs(t);
    edgeFade = std::clamp(edge / kEdgeFade, 0.0f, 1.0f);
    edgeFade = edgeFade * edgeFade * (3.0f - 2.0f * edgeFade);
    return true;
}

class CompassDlst : public dDlst_base_c {
public:
    void draw() override;
};

CompassDlst g_dlst;

void CompassDlst::draw() {
    J2DGrafContext* port = dComIfGp_getCurrentGrafPort();
    if (port == nullptr) {
        return;
    }
    port->setup2D();

    const Layout l = computeLayout();
    const float midY = (l.top + l.bottom) * 0.5f;

    // Background strip and a thin baseline, both fading out at the ends so the bar
    // reads as continuing past what is visible.
    fadedBand(l, l.top, l.bottom, 0, 0, 0, 110.0f);
    fadedBand(l, l.bottom - 1.0f, l.bottom, 255, 255, 255, 90.0f);

    // Ticks every 15 degrees, measured from north.
    const float north = northOffsetDegrees() * kDegToRad;
    for (int i = 0; i < 24; ++i) {
        if (i % 3 == 0) {
            continue;  // labels go there
        }
        float bearing = north + static_cast<float>(i) * 15.0f * kDegToRad;
        float x, fade;
        if (!projectBearing(l, bearing, x, fade)) {
            continue;
        }
        rect(x - 0.5f, l.bottom - 5.0f, x + 0.5f, l.bottom - 1.0f,
            GXColor{255, 255, 255, scaleAlpha(140.0f, fade)});
    }

    // Player pins (white diamonds), only for the stage we're in.
    if (g_settings.showPins) {
        for (uint32_t i = 0; i < g_pins.count; ++i) {
            const Pin& p = g_pins.pins[i];
            if (std::strncmp(p.stage, g_state.stage, sizeof(p.stage)) != 0) {
                continue;
            }
            const float dx = p.x - g_state.x;
            const float dz = p.z - g_state.z;
            if (dx * dx + dz * dz < 1.0f) {
                continue;  // standing on it
            }
            float x, fade;
            if (!projectBearing(l, std::atan2(dx, dz), x, fade)) {
                continue;
            }
            diamond(x, midY, 5.0f, GXColor{20, 20, 20, scaleAlpha(200.0f, fade)});
            diamond(x, midY, 4.0f, GXColor{255, 255, 255, scaleAlpha(255.0f, fade)});
        }
    }

    // Center indicator.
    triangleDown(l.cx, l.bottom + 1.0f, 4.0f, 5.0f, GXColor{255, 255, 255, scaleAlpha(230.0f, 1.0f)});

    // Cardinal labels. Spanish: Oeste = O.
    static const char* const kLabels[8] = {"N", "NE", "E", "SE", "S", "SO", "O", "NO"};
    JUTFont* font = mDoExt_getMesgFont();
    if (font != nullptr) {
        font->setGX();
        for (int i = 0; i < 8; ++i) {
            float bearing = north + static_cast<float>(i) * 45.0f * kDegToRad;
            float x, fade;
            if (!projectBearing(l, bearing, x, fade)) {
                continue;
            }
            const bool major = (i % 2) == 0;
            const float size = major ? 14.0f : 10.0f;
            const char* label = kLabels[i];
            const float approxWidth = static_cast<float>(std::strlen(label)) * size * 0.6f;
            u8 alpha = scaleAlpha(major ? 255.0f : 190.0f, fade);
            JUtility::TColor color = (i == 0) ? JUtility::TColor(255, 120, 90, alpha)  // north stands out
                                              : JUtility::TColor(255, 255, 255, alpha);
            font->setCharColor(color);
            font->drawString_scale(x - approxWidth * 0.5f, l.bottom - 3.0f, size, size, label, true);
        }
    }

    // Restore the standard 2D state for whatever draws next.
    port->setup2D();
}

void onMeterDrawPost(ModContext*, void*, void*, void*) {
    if (shouldDrawCompass()) {
        dComIfGd_set2DOpa(&g_dlst);
    }
}

}  // namespace

ModResult installCompassDraw() {
    ModResult r = mods::hook::add_post<Meter2Draw>(onMeterDrawPost);
    if (r != MOD_OK) {
        mods::log::error("failed to hook dMeter2_c::_draw ({})", static_cast<int>(r));
    }
    return r;
}

}  // namespace compass
