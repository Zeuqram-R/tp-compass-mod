#include "settings.hpp"

#include <mods/svc/log.hpp>

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
}

float northOffsetDegrees() {
    static constexpr float kOffsets[] = {0.0f, 90.0f, 180.0f, 270.0f};
    int64_t i = g_settings.northOffset;
    if (i < 0 || i > 3) {
        i = 0;
    }
    return kOffsets[i];
}

}  // namespace compass
