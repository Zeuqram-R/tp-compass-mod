// TP Compass — an Elden Ring style compass for Dusklight (Twilight Princess).
//
// Compass bar with cardinal points, player pins (+ hotkey), unopened chests, location name,
// settings panel and debug info.

#include <mods/service.hpp>
#include <mods/svc/config.h>
#include <mods/svc/hook.h>
#include <mods/svc/log.hpp>
#include <mods/svc/save.h>
#include <mods/svc/ui.h>

#include "beacon.hpp"
#include "collectibles.hpp"
#include "compass_draw.hpp"
#include "entrances.hpp"
#include "game_state.hpp"
#include "input.hpp"
#include "pins.hpp"
#include "settings.hpp"
#include "ui_panel.hpp"

DEFINE_MOD();
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(SaveService, svc_save);
IMPORT_SERVICE(UiService, svc_ui);
IMPORT_SERVICE(HookService, svc_hook);

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError*) {
    ModResult r = compass::registerSettings();
    if (r != MOD_OK) {
        mods::log::error("settings registration failed");
        return r;
    }
    compass::initPins();
    compass::initEntrances();
    if ((r = compass::installCompassDraw()) != MOD_OK) {
        return r;
    }
    if ((r = compass::installInput()) != MOD_OK) {
        return r;
    }
    if ((r = compass::registerUiPanel()) != MOD_OK) {
        mods::log::error("UI panel registration failed");
        return r;
    }
    mods::log::info("TP Compass ready");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    compass::refreshSettings();
    compass::updateGameState();
    compass::shouldDrawCompass();  // keeps the debug "hidden by" reason fresh
    compass::updateEntrances();
    compass::refreshPinAnchors();
    compass::updateCollectibles();
    compass::updateBeaconParticles();
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    mods::log::info("TP Compass unloaded");
    return MOD_OK;
}

}
