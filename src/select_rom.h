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

enum RomType {
    ROM_TYPE_UNKNOWN = 0,
    ROM_TYPE_NES,
    ROM_TYPE_SMS,
    ROM_TYPE_GAMEGEAR
};

static inline bool hasRomExt(const std::string& path) {
    if (path.size() < 3) return false;

    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) return false;

    std::string ext = path.substr(dotPos + 1);

    for (auto &ch : ext)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

    return (ext == "nes" || ext == "gg" || ext == "sms");
}


RomType getRomType(const std::string& path) {
    if (path.empty()) return ROM_TYPE_UNKNOWN;

    // find last dot
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || dotPos + 1 >= path.size())
        return ROM_TYPE_UNKNOWN;

    // Extract the extension
    std::string ext = path.substr(dotPos + 1);

    // Convert to lowercase
    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (ext == "nes") return ROM_TYPE_NES;
    if (ext == "sms") return ROM_TYPE_SMS;
    if (ext == "gg")  return ROM_TYPE_GAMEGEAR;

    return ROM_TYPE_UNKNOWN;
}

static inline std::string getRomPath(SdService& sdService, CardputerView& display, CardputerInput& input, const std::string& initialFolder = "/", bool skipWelcome = false) {
    VerticalSelector verticalSelector(display, input);

    display.initialize();
    if (!skipWelcome) {
        display.topBar("LOAD ROM CARTRIDGE", false, false);
        display.subMessage(".nes .gg .sms files", 500);
    }
    if (!sdService.begin()) {
        display.subMessage("SD card not found", 2000);
        return "";
    }
    input.flushInput(1000); // avoid double tap, flush any previous input

    std::string currentPath = initialFolder.empty() ? "/" : initialFolder;
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
            if (!hasRomExt(nextPath)) {
                display.subMessage(".sms .gg .nes required", 2000);
                continue; // non rom file
            }
            return "/sd" + nextPath; // file selected
        }
    }

    return "";
}