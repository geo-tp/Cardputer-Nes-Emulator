#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "sd/SdService.h"
#include "display/CardputerView.h"
#include "selector/VerticalSelector.h" 
#include "input/CardputerInput.h"

// Returns the absolute path of a .nes selected file
// - Navigates folders with verticalSelector
// - When a selected item is a nes file, returns its path

static inline bool hasNesExt(const std::string& path) {
    if (path.size() < 4) return false;
    char a = (char)std::tolower((unsigned char)path[path.size()-4]);
    char b = (char)std::tolower((unsigned char)path[path.size()-3]);
    char c = (char)std::tolower((unsigned char)path[path.size()-2]);
    char d = (char)std::tolower((unsigned char)path[path.size()-1]);
    return (a=='.' && b=='n' && c=='e' && d=='s');
}

static inline std::string getRomPath(SdService& sdService, CardputerView& display, CardputerInput& input) {
    VerticalSelector verticalSelector(display, input);

    display.initialize();
    display.topBar("LOAD .nes ROM", false, false);
    display.subMessage("Loading...", 0);
    if (!sdService.begin()) {
        display.subMessage("SD card not found", 2000);
        return "";
    }
    input.flushInput(1000); // avoid double tap, flush any previous input

    std::string currentPath = "/";
    std::vector<std::string> elementNames;

    while (true) {
        // List elements
        display.subMessage("Loading...", 0);
        elementNames = sdService.getCachedDirectoryElements(currentPath);
        if (elementNames.empty()) {
            display.subMessage("No elements found", 2000);
            auto parent = sdService.getParentDirectory(currentPath);
            if (parent.empty() || parent == currentPath) {
                sdService.close();
                return "";
            }
            currentPath = parent;
            continue;
        }

        // Select element
        uint16_t selectedIndex = verticalSelector.select(
            currentPath,
            elementNames,
            true,   // back item support
            true,   // UI extra
            {},
            {},
            false,
            false
        );

        // Retour
        if (selectedIndex >= elementNames.size()) {
            currentPath = sdService.getParentDirectory(currentPath);
            if (currentPath.empty()) currentPath = "/";
            continue;
        }

        // Construct next path
        std::string nextPath = currentPath;
        if (!nextPath.empty() && nextPath.back() != '/') nextPath += "/";
        nextPath += elementNames[selectedIndex];

        // folder
        if (sdService.isDirectory(nextPath)) {
            currentPath = nextPath;
            continue;
        // file
        } else {
            if (!hasNesExt(nextPath)) {
                display.subMessage("file .nes required", 2000);
                continue; // non .nes
            }
            return "/sd" + nextPath; // file selected
        }
    }

    sdService.close();
    return "";
}