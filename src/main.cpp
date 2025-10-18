#include <M5Cardputer.h>
#include <select_rom.h>
#include "cardputer/CardputerView.h"
#include "cardputer/CardputerInput.h"
#include "sd/SdService.h"
#include "vfs/vfs_xip.h"
#include "vfs/rom_flash_io.h"
#include "vfs/rom_xip.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nes/run_nes.h"
#include "sms/run_sms.h"
#include "last_game.h"

void setup() {
  auto cfg = M5.config();
  cfg.output_power = true;
  M5Cardputer.begin(cfg);

  CardputerInput input;
  SdService sd;
  CardputerView display;
  display.initialize();

  // SD
  while (!sd.begin()) {
    display.topBar("SD CARD FOR ROMS", false, false);
    display.subMessage("No SD card found", 1000);
    display.subMessage("Insert SD card", 0);
  }

  std::string romPath;
  auto romFolder = getRomFolderFromNvs(display, input, sd);
  romFolder = romFolder.empty() ? "/" : romFolder;

  if (isQuittingGame()) {
    romPath = getRomPath(sd, display, input, romFolder, true);
  } else {
      // Welcome
    display.welcome();
    input.waitPress();

    // Try to get last game from NVS or select a new one
    romPath = getLastGameFromNvs(display, input, sd);
    if (romPath.empty()) {
      auto skipWelcome = romFolder == "/" ? false : true;
      romPath = getRomPath(sd, display, input, romFolder, skipWelcome);
    } else {
      romPath = "/sd" + romPath; // ensure sd prefix
    }
  }

  display.topBar("COPYING TO FLASH", false, false);
  display.subMessage("Loading...", 0);
  
  // Find the rom partition
  const esp_partition_t* romPart = findRomPartition("spiffs");
  if (!romPart) {
    while (1) {
      display.topBar("ERROR", false, false);
      display.subMessage("No ROM partition", 0);
      delay(1500);
    }
  }

  // Copy the ROM file to the partition
  size_t romSize = 0;
  if (!copyFileToPartition(romPath.c_str(), romPart, &romSize)) {
    while (1) {
      display.topBar("ERROR", false, false);
      display.subMessage("SD to Flash failed", 0);
      delay(1500);
    }
  }

  // Map the ROM partition in XIP
  if (xip_map_rom_partition("spiffs", romSize) != 0) {
    while (1) {
      display.topBar("ERROR", false, false);
      display.subMessage("Map ROM failed", 0);
      delay(1500);
    }
  }

  // Register the XIP VFS
  vfs_xip_register();

  // Show keymapping
  display.topBar("- + SOUND [ ] BRIGHT", false, false);
  display.showKeymapping();

  // Wait for key press or show tips
  uint32_t lastUpdate = millis();
  int state = 0;
  for (;;) {
    char key = input.readChar();
    if (key != KEY_NONE) break;

    // Show tips
    uint32_t now = millis();
    if (now - lastUpdate >= 2000) {
      lastUpdate = now;
      switch (state) {
        case 0: display.topBar("PRESS ANY KEY TO START", false, false); break;
        case 1: display.topBar("KEY \\ SCREEN MODE",       false, false); break;
        case 2: display.topBar("G0 1SEC TO QUIT GAME",     false, false); break;
        case 3: display.topBar("FN + ARROWS FOR ZOOM",     false, false); break;
        case 4: display.topBar("- + SOUND [ ] BRIGHT",     false, false); break;
      }
      state = (state + 1) % 5;
    }
    delay(1);
  }

  // Check the extension to choose the emulator
  auto ext = getRomType(romPath);

  // Save last game to nvs
  if (ext != ROM_TYPE_UNKNOWN) {
      saveLastGameToNvs(romPath);
  }

  // Run the emulator
  if (ext == ROM_TYPE_NES) {
      // --- NES ---
      auto pos = romPath.find_last_of("/\\");
      std::string romName = (pos == std::string::npos) ? romPath : romPath.substr(pos + 1);
      static char romArg[256];
      std::snprintf(romArg, sizeof(romArg), "/xip/%s", romName.c_str());
      run_nes(romArg);
  }
  else if (ext == ROM_TYPE_GAMEGEAR || ext == ROM_TYPE_SMS) {
      // --- Master System / Game Gear ---
      bool isGG = (ext == ROM_TYPE_GAMEGEAR);
      run_sms(_get_rom_ptr(), _get_rom_size(), isGG);
  }
  else {
      display.topBar("ERROR", false, false);
      display.subMessage("Unsupported ROM type", 0);
      while (1) delay(1000);
  }
}

void loop() {
  /* run_emulator is blocking */
}
