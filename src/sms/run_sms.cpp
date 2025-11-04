#include "run_sms.h"

#include <M5Cardputer.h>
#include <stdio.h>

#include "cardputer/CardputerView.h"
#include "cardputer/CardputerInput.h"

#include "sms/display.h"
#include "sms/sound.h"
#include "sms/input.h"
#include "sms/save.h"

void run_sms(const uint8_t* romPtr, size_t romLen, bool isGG, const char* romName)
{
  CardputerView display;
  CardputerInput input;
  display.initialize();

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
  smsZoomPercent = isGG ? 100 : 110;
  
  z80_allocate_flag_tables();
  sms_init_ram();
  emu_system_init(22050);
  system_reset();

  // Save
  sms_save_init(romName, sram, 0x8000);
  sms_save_load();

  // Display
  sms_display_init(); 
  sms_palette_init_fixed();   
  video_compute_scaler_full();

  // Audio
  sms_audio_init();
  sms_audio_start_task(); 

  // Main loop
  bool lastToggle = fullscreen;
  uint32_t frameCount = 0;
  uint32_t lastFpsTime = millis();
  float avgFrameTime = 0;
  float avgFrameTimeRT = 0;
  float accFrameUs = 0.f;
  const uint32_t TARGET_US = 16667; // 60 Hz

  for (;;) {
    uint32_t t0 = micros();

    sms_save_tick();
    sms_frame(0);
    if (lastToggle != fullscreen) {
      lastToggle = fullscreen;
      if (fullscreen) video_compute_scaler_full();
      else            video_compute_scaler_square();
    }
    cardputer_read_input(isGG);
    sms_display_write_frame();

    // Pacing 60 Hz
    uint32_t emuUs = micros() - t0;
    int32_t remaining = TARGET_US - emuUs;
    if (remaining > 0) {
      delayMicroseconds(remaining);
    }
    
    // // Realtime
    // uint32_t frameUs = micros() - t0;

    // // Stats
    // avgFrameTime += emuUs;      // perf brute
    // avgFrameTimeRT += frameUs;  // FPS effectif
    // frameCount++;

    // if (millis() - lastFpsTime >= 1000) {
    //   float avgRaw = avgFrameTime / frameCount;
    //   float fpsRaw = 1000000.0f / avgRaw;
    //   float speedPct = (fpsRaw / 60.0f) * 100.0f;

    //   float avgRT = avgFrameTimeRT / frameCount;
    //   float fpsRT = 1000000.0f / avgRT;

    //   printf("[Perf] raw=%.1f us (%.1f fps, %.1f%%) | realtime=%.1f us (%.2f fps)\n",
    //         avgRaw, fpsRaw, speedPct, avgRT, fpsRT);

    //   avgFrameTime = 0;
    //   avgFrameTimeRT = 0;
    //   frameCount = 0;
    //   lastFpsTime = millis();
    // }
  }
}