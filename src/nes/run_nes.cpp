#include "run_nes.h"

#include <M5Cardputer.h>
#include <stdio.h>
#include <string>

#include "cardputer/CardputerView.h"
#include "cardputer/CardputerInput.h"

void run_nes(const char* xipPath)
{
  CardputerView display;
  display.initialize();

  // Call the NES core
  // Note: the core expects argv to be the ROM path
  char* argv_[1] = { const_cast<char*>(xipPath) };
  nofrendo_main(1, argv_);
}
