#pragma once

#include <string>
#include <algorithm>
#include <Preferences.h>
#include "sd/SdService.h"
#include "cardputer/CardputerView.h"
#include "cardputer/CardputerInput.h"
#include "selector/ConfirmationSelector.h"

// Helper: basename from path
static inline std::string _basename(const std::string& path) {
    if (path.empty()) return "";
    size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

static inline std::string getLastGameFromNvs(
    CardputerView& display,
    CardputerInput& input,
    SdService& sdService
) {
    Preferences prefs;
    prefs.begin("cardputer_emu", true);
    String lastGame = prefs.getString("last_game", "");
    prefs.end();

    // No last game saved
    if (lastGame.isEmpty()) {
        return "";
    }

    std::string path = lastGame.c_str();

    // Verify if the file still exists
    if (!sdService.isFile(path)) {
        return "";
    }

    // User confirmation
    ConfirmationSelector confirm(display, input);
    bool confirmed = confirm.select("Resume last game?", _basename(path));

    return confirmed ? path : "";
}

static inline void saveLastGameToNvs(const std::string& filePath) {
    if (filePath.empty()) return;

    Preferences prefs;
    prefs.begin("cardputer_emu", false);

    std::string cleanPath = filePath;
    if (cleanPath.rfind("/sd/", 0) == 0)
        cleanPath = cleanPath.substr(3); // retire /sd

    prefs.putString("last_game", cleanPath.c_str());
    prefs.end();
}

static inline std::string getRomFolderFromNvs(
    CardputerView& display,
    CardputerInput& input,
    SdService& sdService
) {
    Preferences prefs;
    prefs.begin("cardputer_emu", true);
    String lastGame = prefs.getString("last_game", "");
    prefs.end();

    if (lastGame.isEmpty()) {
        return "";
    }

    std::string path = lastGame.c_str();

    // Get the folder
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return "";
    }

    return path.substr(0, slash);
}


static inline bool isQuittingGame() {
  // We use deep sleep as a hack to avoid memory management issues when releasing emu cores
  return (esp_reset_reason() == ESP_RST_DEEPSLEEP) &&
         (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER);
}