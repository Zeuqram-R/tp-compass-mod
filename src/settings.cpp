#include "settings.hpp"

#include <mods/svc/log.hpp>

#include <dolphin/pad.h>

namespace compass {

Settings g_settings;
SettingsHandles g_settingHandles;

namespace {

ModResult regBool(const char* name, bool def, ConfigVarHandle* out) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = name;
    desc.type = CONFIG_VAR_BOOL;
    desc.default_bool = def;
    return svc_config->register_var(mod_ctx, &desc, out);
}

ModResult regInt(const char* name, int64_t def, ConfigVarHandle* out) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = name;
    desc.type = CONFIG_VAR_INT;
    desc.default_int = def;
    return svc_config->register_var(mod_ctx, &desc, out);
}

void readBool(ConfigVarHandle h, bool& out) {
    bool v = out;
    if (h != 0 && svc_config->get_bool(mod_ctx, h, &v) == MOD_OK) {
        out = v;
    }
}

void readInt(ConfigVarHandle h, int64_t& out) {
    int64_t v = out;
    if (h != 0 && svc_config->get_int(mod_ctx, h, &v) == MOD_OK) {
        out = v;
    }
}

}  // namespace

ModResult registerSettings() {
    const Settings d;  // defaults
    auto& h = g_settingHandles;
    ModResult r = MOD_OK;
    if ((r = regBool("showCompass", d.showCompass, &h.showCompass)) != MOD_OK) return r;
    if ((r = regBool("showPins", d.showPins, &h.showPins)) != MOD_OK) return r;
    if ((r = regBool("showDebug", d.showDebug, &h.showDebug)) != MOD_OK) return r;
    if ((r = regInt("opacity", d.opacity, &h.opacity)) != MOD_OK) return r;
    if ((r = regInt("width", d.width, &h.width)) != MOD_OK) return r;
    if ((r = regInt("posY", d.posY, &h.posY)) != MOD_OK) return r;
    if ((r = regInt("arcDegrees", d.arcDegrees, &h.arcDegrees)) != MOD_OK) return r;
    if ((r = regInt("northOffset", d.northOffset, &h.northOffset)) != MOD_OK) return r;
    if ((r = regBool("invertDirection", d.invertDirection, &h.invertDirection)) != MOD_OK) return r;
    if ((r = regBool("pinHotkey", d.pinHotkey, &h.pinHotkey)) != MOD_OK) return r;
    if ((r = regInt("pinModifier", d.pinModifier, &h.pinModifier)) != MOD_OK) return r;
    if ((r = regInt("pinButton", d.pinButton, &h.pinButton)) != MOD_OK) return r;
    if ((r = regBool("pinBeacons", d.pinBeacons, &h.pinBeacons)) != MOD_OK) return r;
    if ((r = regInt("locationAnchor", d.locationAnchor, &h.locationAnchor)) != MOD_OK) return r;
    if ((r = regBool("showChests", d.showChests, &h.showChests)) != MOD_OK) return r;
    if ((r = regBool("showPois", d.showPois, &h.showPois)) != MOD_OK) return r;
    if ((r = regBool("showObjective", d.showObjective, &h.showObjective)) != MOD_OK) return r;
    if ((r = regBool("showObjectiveText", d.showObjectiveText, &h.showObjectiveText)) != MOD_OK) return r;
    if ((r = regInt("objectiveX", d.objectiveX, &h.objectiveX)) != MOD_OK) return r;
    if ((r = regInt("objectiveY", d.objectiveY, &h.objectiveY)) != MOD_OK) return r;
    if ((r = regBool("showRemotePins", d.showRemotePins, &h.showRemotePins)) != MOD_OK) return r;
    if ((r = regBool("randomizerMode", d.randomizerMode, &h.randomizerMode)) != MOD_OK) return r;
    if ((r = regBool("showBugs", d.showBugs, &h.showBugs)) != MOD_OK) return r;
    if ((r = regBool("showPoes", d.showPoes, &h.showPoes)) != MOD_OK) return r;
    if ((r = regBool("showHeartPieces", d.showHeartPieces, &h.showHeartPieces)) != MOD_OK) return r;
    if ((r = regBool("showHowlingStones", d.showHowlingStones, &h.showHowlingStones)) != MOD_OK) return r;
    if ((r = regBool("showHeightArrows", d.showHeightArrows, &h.showHeightArrows)) != MOD_OK) return r;
    if ((r = regBool("scaleByDistance", d.scaleByDistance, &h.scaleByDistance)) != MOD_OK) return r;
    if ((r = regBool("chestsNeedCompass", d.chestsNeedCompass, &h.chestsNeedCompass)) != MOD_OK) return r;
    if ((r = regBool("showLocation", d.showLocation, &h.showLocation)) != MOD_OK) return r;
    if ((r = regInt("locationX", d.locationX, &h.locationX)) != MOD_OK) return r;
    if ((r = regInt("locationY", d.locationY, &h.locationY)) != MOD_OK) return r;
    refreshSettings();
    return MOD_OK;
}

void refreshSettings() {
    auto& h = g_settingHandles;
    auto& s = g_settings;
    readBool(h.showCompass, s.showCompass);
    readBool(h.showPins, s.showPins);
    readBool(h.showDebug, s.showDebug);
    readInt(h.opacity, s.opacity);
    readInt(h.width, s.width);
    readInt(h.posY, s.posY);
    readInt(h.arcDegrees, s.arcDegrees);
    readInt(h.northOffset, s.northOffset);
    readBool(h.invertDirection, s.invertDirection);
    readBool(h.pinHotkey, s.pinHotkey);
    readInt(h.pinModifier, s.pinModifier);
    readInt(h.pinButton, s.pinButton);
    readBool(h.pinBeacons, s.pinBeacons);
    readInt(h.locationAnchor, s.locationAnchor);
    readBool(h.showChests, s.showChests);
    readBool(h.showPois, s.showPois);
    readBool(h.showObjective, s.showObjective);
    readBool(h.showObjectiveText, s.showObjectiveText);
    readInt(h.objectiveX, s.objectiveX);
    readInt(h.objectiveY, s.objectiveY);
    readBool(h.showRemotePins, s.showRemotePins);
    readBool(h.randomizerMode, s.randomizerMode);
    readBool(h.showBugs, s.showBugs);
    readBool(h.showPoes, s.showPoes);
    readBool(h.showHeartPieces, s.showHeartPieces);
    readBool(h.showHowlingStones, s.showHowlingStones);
    readBool(h.showHeightArrows, s.showHeightArrows);
    readBool(h.scaleByDistance, s.scaleByDistance);
    readBool(h.chestsNeedCompass, s.chestsNeedCompass);
    readBool(h.showLocation, s.showLocation);
    readInt(h.locationX, s.locationX);
    readInt(h.locationY, s.locationY);
}

float northOffsetDegrees() {
    static constexpr float kOffsets[] = {0.0f, 90.0f, 180.0f, 270.0f};
    int64_t i = g_settings.northOffset;
    if (i < 0 || i > 3) {
        i = 0;
    }
    return kOffsets[i];
}

uint32_t pinModifierMask() {
    static constexpr uint32_t kMods[] = {0, PAD_TRIGGER_Z, PAD_TRIGGER_R, PAD_TRIGGER_L};
    int64_t i = g_settings.pinModifier;
    return (i >= 0 && i < 4) ? kMods[i] : 0;
}

uint32_t pinButtonMask() {
    static constexpr uint32_t kButtons[] = {PAD_BUTTON_LEFT, PAD_BUTTON_RIGHT, PAD_BUTTON_DOWN, PAD_BUTTON_UP};
    int64_t i = g_settings.pinButton;
    return (i >= 0 && i < 4) ? kButtons[i] : PAD_BUTTON_LEFT;
}

}  // namespace compass
