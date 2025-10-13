#include <M5Cardputer.h>
#include <select_rom.h>
#include "display/CardputerView.h"
#include "input/CardputerInput.h"
#include "sd/SdService.h"
#include "vfs/vfs_xip.h"
#include "vfs/rom_flash_io.h"
#include "vfs/rom_xip.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nes/run_nes.h"
#include "sms/run_sms.h"

void setup() {
  auto cfg = M5.config();
  cfg.output_power = true;
  M5Cardputer.begin(cfg);

  CardputerInput input;
  SdService sd;
  CardputerView display;
  display.initialize();

  // Welcome
  display.welcome();
  input.waitPress();

  // SD
  if (!sd.begin()) {
    while (1) {
      display.topBar("SD CARD ROM", false, false);
      display.subMessage("No SD card found", 0);
      delay(1000);
    }
  }

  // Get the ROM path from SD
  auto romPath = getRomPath(sd, display, input);

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

  // Check the extension to choose the emulator
  auto ext = getRomType(romPath);

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
