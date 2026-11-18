#include "input.hpp"

#include "game_state.hpp"
#include "pins.hpp"
#include "settings.hpp"
#include "ui_panel.hpp"

#include <mods/svc/hook.hpp>
#include <mods/svc/log.hpp>

#include "m_Do/m_Do_controller_pad.h"
#include <dolphin/pad.h>

DEFINE_HOOK(&mDoCPd_c::read, PadRead);
// Dusklight's custom bindings (e.g. "Call Midna" on the D-pad) read the native controller
// directly, so they need to be silenced separately from the GameCube pad bits.
DEFINE_HOOK_SYMBOL("dusk::getActionBindTrig", bool(int, u32), ActionBindTrig);
DEFINE_HOOK_SYMBOL("dusk::getActionBindHold", bool(int, u32), ActionBindHold);

namespace compass {
namespace {

bool g_suppressBinds = false;

bool inGameplay() {
    return g_state.hasPlayer && !g_state.eventRunning && g_state.windowStatus == 0;
}

void onPadReadPost(ModContext*, void*, void*, void*) {
    g_suppressBinds = false;
    if (!g_settings.pinHotkey || !inGameplay()) {
        return;
    }
    const uint32_t mod = pinModifierMask();
    const uint32_t btn = pinButtonMask();
    interface_of_controller_pad& pad = mDoCPd_c::getCpadInfo(PAD_1);
    const uint32_t hold = pad.mButtonFlags;
    const uint32_t trig = pad.mPressedButtonFlags;
    if (mod != 0 && (hold & mod) == 0) {
        return;
    }
    if (trig & btn) {
        notifyHotkeyPinResult(togglePinAtPlayer());
    }
    if ((hold | trig) & btn) {
        // Keep the game from also reacting to the button while the combo is in use.
        pad.mButtonFlags &= ~btn;
        pad.mPressedButtonFlags &= ~btn;
        g_suppressBinds = true;
    }
}

void onActionBindPost(ModContext*, void*, void* retval, void*) {
    if (g_suppressBinds && retval != nullptr) {
        *static_cast<bool*>(retval) = false;
    }
}

}  // namespace

ModResult installInput() {
    ModResult r = mods::hook::add_post<PadRead>(onPadReadPost);
    if (r != MOD_OK) {
        mods::log::error("failed to hook mDoCPd_c::read ({})", static_cast<int>(r));
        return r;
    }
    // Not fatal: without these the hotkey still works, Dusklight bindings just aren't muted.
    if (mods::hook::add_post<ActionBindTrig>(onActionBindPost) != MOD_OK ||
        mods::hook::add_post<ActionBindHold>(onActionBindPost) != MOD_OK)
    {
        mods::log::warn("could not hook Dusklight action bindings; the hotkey may also trigger them");
    }
    return MOD_OK;
}

}  // namespace compass
