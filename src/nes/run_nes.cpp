#include "run_nes.h"

#include <M5Cardputer.h>
#include <stdio.h>
#include <string>

#include "display/CardputerView.h"
#include "input/CardputerInput.h"

void run_nes(const char* xipPath)
{
  CardputerView display;
  display.initialize();

  display.topBar("- + SOUND [ ] BRIGHT", false, false);
  display.showKeymapping();

  // Wait for key press or show tips
  CardputerInput input;
  uint32_t lastUpdate = millis();
  int state = 0;
  for (;;) {
    char key = input.readChar();
    if (key != KEY_NONE) break;

    uint32_t now = millis();
    if (now - lastUpdate >= 2000) {
      lastUpdate = now;
      switch (state) {
        case 0: display.topBar("PRESS ANY KEY TO START", false, false); break;
        case 1: display.topBar("KEY \\ SCREEN ZOOM",       false, false); break;
        case 2: display.topBar("- + SOUND [ ] BRIGHT",     false, false); break;
      }
      state = (state + 1) % 3;
    }
    delay(1);
  }

  // Call the NES core
  // Note: the core expects argv[0] to be the ROM path
  static char romArg[256];
  std::snprintf(romArg, sizeof(romArg), "%s", xipPath);
  char* argv_[1] = { romArg };

  nofrendo_main(1, argv_);
}
