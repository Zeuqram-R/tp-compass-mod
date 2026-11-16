#include "ui_panel.hpp"

#include "game_state.hpp"
#include "pins.hpp"
#include "settings.hpp"

#include <mods/svc/log.hpp>
#include <mods/svc/ui.h>

#include <fmt/format.h>

#include <string>

namespace compass {
namespace {

UiElementHandle g_pinCountText = 0;
UiElementHandle g_debugText = 0;

void toast(const char* title, const char* body) {
    UiToastDesc desc = UI_TOAST_DESC_INIT;
    desc.title_rml = title;
    desc.body_rml = body;
    desc.duration_ms = 2500;
    svc_ui->push_toast(mod_ctx, &desc);
}

void reportPinResult(PinResult r, const char* okMessage) {
    switch (r) {
    case PinResult::Ok:
        toast("TP Compass", okMessage);
        break;
    case PinResult::NoSave:
        toast("TP Compass", "Cargá una partida primero.");
        break;
    case PinResult::NoPlayer:
        toast("TP Compass", "Entrá al juego para marcar tu posición.");
        break;
    case PinResult::Full:
        toast("TP Compass", "Llegaste al máximo de marcas (64). Borrá algunas.");
        break;
    case PinResult::Nothing:
        toast("TP Compass", "No había marcas que borrar.");
        break;
    }
}

void onAddPin(ModContext*, void*) {
    reportPinResult(addPinAtPlayer(), "Marca guardada. Se guarda en tu partida al salvar el juego.");
}

void onClearStage(ModContext*, void*) {
    reportPinResult(clearPinsInCurrentStage(), "Marcas de esta zona borradas.");
}

void onClearAll(ModContext*, void*) {
    reportPinResult(clearAllPins(), "Todas las marcas borradas.");
}

ModResult addToggle(UiElementHandle pane, const char* label, ConfigVarHandle var) {
    UiControlDesc c = UI_CONTROL_DESC_INIT;
    c.kind = UI_CONTROL_TOGGLE;
    c.label = label;
    c.binding = UI_BINDING_CONFIG_VAR;
    c.config_var = var;
    return svc_ui->pane_add_control(mod_ctx, pane, &c, nullptr);
}

ModResult addNumber(UiElementHandle pane, const char* label, ConfigVarHandle var, int64_t min,
    int64_t max, int64_t step, const char* suffix)
{
    UiControlDesc c = UI_CONTROL_DESC_INIT;
    c.kind = UI_CONTROL_NUMBER;
    c.label = label;
    c.binding = UI_BINDING_CONFIG_VAR;
    c.config_var = var;
    c.min = min;
    c.max = max;
    c.step = step;
    c.suffix = suffix;
    return svc_ui->pane_add_control(mod_ctx, pane, &c, nullptr);
}

ModResult addButton(UiElementHandle pane, const char* label, UiPressedFn fn) {
    UiControlDesc c = UI_CONTROL_DESC_INIT;
    c.kind = UI_CONTROL_BUTTON;
    c.label = label;
    c.on_pressed = fn;
    return svc_ui->pane_add_control(mod_ctx, pane, &c, nullptr);
}

ModResult build(ModContext*, UiElementHandle panel, void*, ModError*) {
    const auto& h = g_settingHandles;
    ModResult r = MOD_OK;

    svc_ui->pane_add_section(mod_ctx, panel, "Brújula");
    if ((r = addToggle(panel, "Mostrar brújula", h.showCompass)) != MOD_OK) return r;
    if ((r = addNumber(panel, "Opacidad", h.opacity, 10, 100, 5, "%")) != MOD_OK) return r;
    if ((r = addNumber(panel, "Ancho", h.width, 160, 600, 20, "")) != MOD_OK) return r;
    if ((r = addNumber(panel, "Distancia desde arriba", h.posY, 0, 200, 2, "")) != MOD_OK) return r;
    if ((r = addNumber(panel, "Ángulo visible", h.arcDegrees, 60, 360, 15, "°")) != MOD_OK) return r;

    svc_ui->pane_add_section(mod_ctx, panel, "Marcas");
    if ((r = addToggle(panel, "Mostrar marcas", h.showPins)) != MOD_OK) return r;
    if ((r = addButton(panel, "Marcar mi posición", onAddPin)) != MOD_OK) return r;
    if ((r = addButton(panel, "Borrar marcas de esta zona", onClearStage)) != MOD_OK) return r;
    if ((r = addButton(panel, "Borrar todas las marcas", onClearAll)) != MOD_OK) return r;
    svc_ui->pane_add_text(mod_ctx, panel, "", &g_pinCountText);

    svc_ui->pane_add_section(mod_ctx, panel, "Calibración");
    {
        static const char* const kNorth[] = {"0°", "90°", "180°", "270°"};
        UiControlDesc c = UI_CONTROL_DESC_INIT;
        c.kind = UI_CONTROL_DROPDOWN;
        c.label = "Dirección del norte";
        c.binding = UI_BINDING_CONFIG_VAR;
        c.config_var = h.northOffset;
        c.options = kNorth;
        c.option_count = 4;
        if ((r = svc_ui->pane_add_control(mod_ctx, panel, &c, nullptr)) != MOD_OK) return r;
    }
    if ((r = addToggle(panel, "Invertir izquierda/derecha", h.invertDirection)) != MOD_OK) return r;

    svc_ui->pane_add_section(mod_ctx, panel, "Debug");
    if ((r = addToggle(panel, "Mostrar info de debug", h.showDebug)) != MOD_OK) return r;
    svc_ui->pane_add_text(mod_ctx, panel, "", &g_debugText);
    return MOD_OK;
}

ModResult update(ModContext*, void*, ModError*) {
    if (g_pinCountText != 0) {
        std::string s = fmt::format("Marcas: {} en esta zona, {} en total", pinsInCurrentStage(), g_pins.count);
        svc_ui->elem_set_text(mod_ctx, g_pinCountText, s.c_str());
    }
    if (g_debugText != 0) {
        std::string s;
        if (g_settings.showDebug) {
            const float yawDeg = g_state.cameraYaw * 57.2957795f;
            s = fmt::format(
                "Stage: {} | Cuarto: {}\n"
                "Posición: ({:.0f}, {:.0f}, {:.0f})\n"
                "Cámara (yaw): {:.1f}°\n"
                "Evento: {} | Menú: {}\n"
                "Oculta por: {}",
                g_state.stage[0] ? g_state.stage : "-", g_state.room, g_state.x, g_state.y, g_state.z,
                yawDeg, g_state.eventRunning ? "sí" : "no", g_state.windowStatus,
                g_state.hiddenReason[0] ? g_state.hiddenReason : "visible");
        }
        svc_ui->elem_set_text(mod_ctx, g_debugText, s.c_str());
    }
    return MOD_OK;
}

}  // namespace

ModResult registerUiPanel() {
    UiModsPanelDesc desc = UI_MODS_PANEL_DESC_INIT;
    desc.build = build;
    desc.update = update;
    return svc_ui->register_mods_panel(mod_ctx, &desc);
}

}  // namespace compass
