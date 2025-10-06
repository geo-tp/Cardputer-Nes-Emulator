
#include <M5Cardputer.h>
#include <load_rom.h>
#include "display/CardputerView.h"
#include "input/CardputerInput.h"
#include "sd/SdService.h"

extern "C" int nofrendo_main(int argc, char *argv[]);

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
  
  // Get ROM path
  auto romPath = getRomPath(sd, display, input);

  // Infos
  display.topBar("PRESS ANY KEY TO START", false, false);
  display.showKeymapping();
  input.waitPress();

  // Launch NES
  static char romArg[256];
  strncpy(romArg, romPath.c_str(), sizeof(romArg)-1);
  romArg[sizeof(romArg)-1] = 0;
  char* argv[1] = { romArg };
  nofrendo_main(1, argv);
}

void loop() {
  /* nofrendo_main is blocking */
}