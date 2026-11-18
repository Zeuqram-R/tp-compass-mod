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
    case PinResult::Removed:
        toast("TP Compass", "Marca quitada.");
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

}  // namespace

void notifyHotkeyPinResult(PinResult r) {
    reportPinResult(r, "Marca puesta.");
}

namespace {

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

ModResult addDropdown(UiElementHandle pane, const char* label, ConfigVarHandle var,
    const char* const* options, uint32_t count)
{
    UiControlDesc c = UI_CONTROL_DESC_INIT;
    c.kind = UI_CONTROL_DROPDOWN;
    c.label = label;
    c.binding = UI_BINDING_CONFIG_VAR;
    c.config_var = var;
    c.options = options;
    c.option_count = count;
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
    if ((r = addToggle(panel, "Pilar de luz sobre las marcas", h.pinBeacons)) != MOD_OK) return r;
    if ((r = addToggle(panel, "Atajo de control para marcas", h.pinHotkey)) != MOD_OK) return r;
    {
        static const char* const kMods[] = {"Ninguno", "Z", "R", "L"};
        static const char* const kButtons[] = {"Cruceta izquierda", "Cruceta derecha", "Cruceta abajo",
            "Cruceta arriba"};
        if ((r = addDropdown(panel, "Botón que mantenés", h.pinModifier, kMods, 4)) != MOD_OK) return r;
        if ((r = addDropdown(panel, "Botón que presionás", h.pinButton, kButtons, 4)) != MOD_OK) return r;
    }
    svc_ui->pane_add_text(mod_ctx, panel,
        "Lejos de una marca: pone una nueva. Cerca de una (su pilar se pone dorado): la quita. "
        "Los nombres son los del "
        "control de GameCube (en tu control pueden llamarse distinto).",
        nullptr);

    svc_ui->pane_add_section(mod_ctx, panel, "Cofres");
    if ((r = addToggle(panel, "Mostrar cofres sin abrir", h.showChests)) != MOD_OK) return r;
    if ((r = addToggle(panel, "Solo si tenés la brújula del dungeon", h.chestsNeedCompass)) != MOD_OK) return r;

    svc_ui->pane_add_section(mod_ctx, panel, "Nombre del lugar");
    if ((r = addToggle(panel, "Mostrar nombre del lugar", h.showLocation)) != MOD_OK) return r;
    {
        static const char* const kAnchors[] = {"Abajo a la izquierda", "Arriba a la izquierda"};
        if ((r = addDropdown(panel, "Posición", h.locationAnchor, kAnchors, 2)) != MOD_OK) return r;
    }
    if ((r = addNumber(panel, "Distancia desde la izquierda", h.locationX, 0, 600, 4, "")) != MOD_OK) return r;
    if ((r = addNumber(panel, "Distancia desde el borde", h.locationY, 0, 440, 2, "")) != MOD_OK) return r;

    svc_ui->pane_add_section(mod_ctx, panel, "Calibración");
    {
        static const char* const kNorth[] = {"0°", "90°", "180°", "270°"};
        if ((r = addDropdown(panel, "Dirección del norte", h.northOffset, kNorth, 4)) != MOD_OK) return r;
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
