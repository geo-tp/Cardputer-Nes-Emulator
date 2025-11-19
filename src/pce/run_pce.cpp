extern "C" {
  #include "pce-go/pce.h"
  #include "pce-go/psg.h"
  void* PalettePCE(int brightness);
}

#include <M5Cardputer.h>
#include "esp_timer.h"
#include "pce_display.h"
#include "pce_sound.h"
#include "pce_input.h"

// ================== SAVE STATE ==================

extern "C" bool pce_save_state_to_file(const char *path)
{
  // todo
  return SaveState(path) == 0;
}

extern "C" bool pce_load_state_from_file(const char *path)
{
  if (LoadState(path) != 0) {
    ResetPCE(false);
    return false;
  }
  return true;
}

// ================== RUN PCE ==================

void run_pce(const uint8_t* rom, size_t len, const char* rom_name)
{
  printf("[PCE] ===== PC Engine Start =====\n");
  printf("[PCE] ROM size: %u bytes (%s)\n", (unsigned)len, rom_name);
  const int sampleRate = 22050;

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setSwapBytes(true);
  M5Cardputer.Display.fillScreen(TFT_BLACK);

  pce_display_init();
  pce_display_start();
  pce_sound_init(sampleRate);

  InitPCE(sampleRate, false);
  printf("[PCE] Core initialized\n");

  if (LoadCard((uint8_t*)rom, len) != 0) {
    printf("[PCE][ERR] ROM loading failed (len=%u)\n", (unsigned)len);
    for (;;) delay(1000);
  }
  printf("[PCE] ROM loaded successfully\n");

  printf("[PCE] Entering RunPCE() loop\n");
  RunPCE();
}
