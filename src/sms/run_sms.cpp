#include "run_sms.h"

#include <M5Cardputer.h>
#include <stdio.h>

#include "display/CardputerView.h"
#include "input/CardputerInput.h"

#include "sms/display.h"
#include "sms/sound.h"
#include "sms/input.h"

void run_sms(const uint8_t* romPtr, size_t romLen, bool isGG)
{
  CardputerView display;
  display.initialize();
  display.topBar("PRESS ANY KEY TO START", false, false);
  display.showKeymapping();

  CardputerInput input;
  input.waitPress();

  // Buffers video & SRAM
  uint8_t* videoBuf = (uint8_t*)heap_caps_aligned_alloc(
      32, 256 * 240, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
  uint8_t* sram = (uint8_t*)heap_caps_aligned_alloc(
      32, 0x8000, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  // Mapping structures core
  sms.dummy = videoBuf; 
  sms.sram  = sram;

  bitmap.width  = 256;
  bitmap.height = 192;
  bitmap.pitch  = 256;
  bitmap.depth  = 8;
  bitmap.data   = videoBuf + 24 * 256;

  cart.rom   = (uint8_t*)romPtr;
  cart.pages = (romLen + 0x3FFF) / 0x4000;
  cart.type  = isGG ? TYPE_GG : TYPE_SMS;

  emu_system_init(22050);
  system_reset();

  sms_palette_init_fixed();          // 1x
  video_compute_scaler_full();       // fullscreen

  // Audio
  sms_audio_init();
  sms_audio_start_task(); 

  // Main loop
  bool lastToggle = fullscreen;
  uint32_t frameCount = 0;
  uint32_t lastFpsTime = millis();
  float accFrameUs = 0.f;

  for (;;) {
    uint32_t t0 = micros();

    sms_frame(0); // emule 1 frame

    // Toggle fullscreen
    if (lastToggle != fullscreen) {
      lastToggle = fullscreen;
      if (fullscreen) video_compute_scaler_full();
      else            video_compute_scaler_square();
    }

    cardputer_read_input();
    sms_display_write_frame();
  }
}
