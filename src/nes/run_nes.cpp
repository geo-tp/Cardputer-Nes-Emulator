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

  // Call the NES core
  // Note: the core expects argv[0] to be the ROM path
  static char romArg[256];
  std::snprintf(romArg, sizeof(romArg), "%s", xipPath);
  char* argv_[1] = { romArg };

  nofrendo_main(1, argv_);
}
