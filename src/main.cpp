#include <M5Cardputer.h>
#include <select_rom.h>
#include "display/CardputerView.h"
#include "input/CardputerInput.h"
#include "sd/SdService.h"
#include "vfs/vfs_xip.h"
#include "vfs/rom_flash_io.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

extern "C" int nofrendo_main(int argc, char *argv[]);
extern "C" int osd_xip_map_rom_partition(const char *part_name, size_t rom_size_effective);

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
      display.topBar("SD CARD .nes ROM", false, false);
      display.subMessage("No SD card found", 0);
      delay(1000);
    }
  }

  // Get the .nes ROM path from SD
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
  if (osd_xip_map_rom_partition("spiffs", romSize) != 0) {
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
  
  // Wait for key with animated hints
  uint32_t lastUpdate = millis();
  int state = 0;
  while (true) {
    char key = input.readChar();
    if (key != KEY_NONE) break;

    uint32_t now = millis();
    if (now - lastUpdate >= 2000) {
        lastUpdate = now;

        switch (state) {
            case 0:
                display.topBar("PRESS ANY KEY TO START", false, false);
                break;
            case 1:
                display.topBar("KEY \\ SCREEN ZOOM", false, false);
                break;
            case 2:
                display.topBar("- + SOUND [ ] BRIGHT", false, false);
                break;
        }
        state = (state + 1) % 3;  // cycle
    }
    delay(1);
  }

  // Launch nofrendo with the XIP path and the rom name
  auto pos = romPath.find_last_of("/\\");
  std::string romName = (pos == std::string::npos) ? romPath : romPath.substr(pos + 1);
  static char romArg[256];
  std::snprintf(romArg, sizeof(romArg), "/xip/%s", romName.c_str());
  char* argv[1] = { romArg };
  nofrendo_main(1, argv);
}

void loop() {
  /* nofrendo_main is blocking */
}
