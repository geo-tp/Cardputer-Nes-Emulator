#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "esp_heap_caps.h"
#include "genesis/run_genesis.h"
#include "genesis_sound.h"
#include "genesis_display.h"
#include <Arduino.h>
#include <M5Cardputer.h>

static uint32_t frame_count = 0;
static uint64_t last_fps_log_time = 0;
static volatile int g_target_fps = 60;
static bool s_draw_toggle = false;  // for skipping frames
static volatile bool s_skipZ80Next = false; // skip Z80 on next frame if true

extern "C" {
  #include "genesis/gwenesis/vdp/gwenesis_vdp.h"
  #include "genesis/gwenesis/cpus/M68K/m68k.h"
  #include "genesis/gwenesis/sound/z80inst.h"
  #include "genesis/gwenesis/bus/gwenesis_bus.h"

  extern unsigned char  *M68K_RAM;
  extern uint8_t  *ZRAM;
  extern uint8_t  *VRAM;
  extern uint8_t  gwenesis_vdp_regs[];
  extern unsigned short gwenesis_vdp_status;
  extern int hint_pending;

  int scan_line = 0;
  extern unsigned int screen_width;
  extern unsigned int screen_height;

  // API Gwenesis
  void           load_cartridge(unsigned char *buffer, size_t size);
  void           power_on(void);
  void           reset_emulation(void);

#ifndef GENESIS_NO_SOUND
  void           gwenesis_SN76489_run(int target);
  void           gwenesis_SN76489_Init(int PSGClockValue, int SamplingRate, int freq_divisor);
  void           gwenesis_vdp_allocate_buffers();
#endif
}

/* Allocate memory if pointer is null, abort on failure */
static inline void ensure_alloc(void** p, size_t bytes, const char* name) {
  if (!*p) {
    *p = heap_caps_calloc(1, bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!*p) { printf("[FATAL] alloc %s (%u bytes) failed\n", name, (unsigned)bytes); abort(); }
  }
}

/* Sleep until specified time in microseconds, for frame pacing */
static inline void sleep_until_us(uint64_t t_us) {
  for (;;) {
    int64_t now = (int64_t)esp_timer_get_time();
    int64_t remain = (int64_t)t_us - now;
    if (remain <= 0) break;
    if (remain > 2000) {
      // big sleep en ms
      vTaskDelay(pdMS_TO_TICKS((remain - 1000) / 1000));
    } else {
      // small sleep en us
      ets_delay_us((uint32_t)remain);
      break;
    }
  }
}

/* Allocate core buffers: VRAM, M68K RAM, Z80 RAM, VDP buffers */
static void genesis_alloc_core_buffers(void) {
  ensure_alloc((void**)&VRAM, VRAM_MAX_SIZE, "VRAM");          // 64 KiB
  ensure_alloc((void**)&M68K_RAM, MAX_RAM_SIZE, "M68K RAM");   // 64K main RAM
  ensure_alloc((void**)&ZRAM, MAX_Z80_RAM_SIZE, "Z80 RAM");    // 8K Z80 RAM
  gwenesis_vdp_allocate_buffers();
}

/* RUN ONE FRAME with VDP, M68K, Z80, Sound, etc. */
IRAM_ATTR static void run_one_frame() {
  const uint64_t t_start = micros();
  const bool drawFrame = (!s_skipZ80Next) && (s_draw_toggle = !s_draw_toggle); // frame skip logic
  const bool skipZ80 = s_skipZ80Next;   // snapshot
  s_skipZ80Next = false;

  // Reset sound state
  #ifndef GENESIS_NO_SOUND
    ym2612_clock  = 0;
    ym2612_index  = 0;
    sn76489_clock = 0;
    sn76489_index = 0;
    zclk = 0;
  #endif

  // VDP and CPU setup for the frame
  gwenesis_vdp_render_config();
  int cpu_deadline = 0;
  screen_width = REG12_MODE_H40 ? 320 : 256;
  screen_height = REG1_PAL ? 240 : 224;
  const unsigned h = screen_height ? screen_height : 224u;
  const int lines_per_frame = (h >= 240u) ? 313 : 262;
  int hint_counter = gwenesis_vdp_regs[10];
  scan_line = 0;
  const uint8_t field_ofs = g_field_ofs; // render 1/2 lines each frame

  // Notify start of frame to display task
  if (g_scanQ) {
    ScanMsg b = { MSG_BEGIN_FRAME, 0, (uint16_t)FB_W, (uint16_t)h, {0} };
    xQueueSend(g_scanQ, &b, 0);
  }

  // Per line emulation loop
  while (scan_line < lines_per_frame) {
    cpu_deadline += VDP_CYCLES_PER_LINE;

    // Run M68K CPU
    m68k_run(cpu_deadline);
    
    // Run Z80 and update YM2612 clock for sound
    #ifndef GENESIS_NO_SOUND
      if (genesis_audio_volume > 0) {
        if (!skipZ80) {
          z80_run(cpu_deadline);
        }
        genesis_sound_ym_set_target_clock(cpu_deadline);
      }
    #endif
    
    // VDP line rendering, if no frame skip and not the lines to skip
    if (drawFrame && (unsigned)scan_line < h && ((scan_line & 1) == field_ofs)) {
      gwenesis_vdp_render_line(scan_line);
    }

    // On these lines, the line counter interrupt is reloaded
    if (scan_line == 0 || (unsigned)scan_line > h) {
      hint_counter = REG10_LINE_COUNTER;
    }

    // interrupt line counter
    if (--hint_counter < 0) {
      if ((REG0_LINE_INTERRUPT != 0) && (unsigned)scan_line <= h) {
        hint_pending = 1;
        if ((gwenesis_vdp_status & STATUS_VIRQPENDING) == 0)
          m68k_update_irq(4);
      }
      hint_counter = REG10_LINE_COUNTER;
    }

    // vblank begin at the end of last rendered line
    if (scan_line == (int)h) {
      if (REG1_VBLANK_INTERRUPT != 0) {
        gwenesis_vdp_status |= STATUS_VIRQPENDING;
        m68k_set_irq(6);
      }
      #ifndef GENESIS_NO_SOUND
        if (genesis_audio_volume > 0) z80_irq_line(1);
      #endif
    }

    #ifndef GENESIS_NO_SOUND
    if (scan_line == (int)h + 1) {
      if (genesis_audio_volume > 0) z80_irq_line(0);
    }
    #endif

    ++scan_line;
  }

  m68k.cycles -= cpu_deadline; // reset cycle

  // Notify end of frame to display task
  if (g_scanQ) {
    ScanMsg e = { MSG_END_FRAME, 0, (uint16_t)FB_W, (uint16_t)h, {0} };
    xQueueSend(g_scanQ, &e, 0);
  }

  // Run SN76489 and push sound samples for this frame
  #ifndef GENESIS_NO_SOUND
    if (genesis_audio_volume > 0) {
      if (!skipZ80) {
        gwenesis_SN76489_run(cpu_deadline);
      }
      genesis_sound_submit_frame();
    }
  #endif
  
  // FPS logging every 2 seconds
  frame_count++;
  uint64_t now = millis();
  if (now - last_fps_log_time >= 2000) {
    float fps = (frame_count * 1000.0f) / (now - last_fps_log_time);
    printf("[FPS] ~%.1f fps\n", fps);
    last_fps_log_time = now;
    frame_count = 0;
    // Log FPS + heap RAM 
    printf("[FPS] ~%.1f fps | heap: %u\n", fps, heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  }

   // Frame time budget check for the next frame
  const uint32_t kFrameBudgetUs = 1000000u / (g_target_fps - 2); // 58FPS target
  const uint32_t elapsedUs = (uint32_t)(micros() - t_start);
  if (elapsedUs > kFrameBudgetUs) {
    s_skipZ80Next = true;  // we are late, skip Z80 next frame
  }
}

/* Run genesis emulation with XIP mapped rom */
extern "C" void IRAM_ATTR run_genesis(const uint8_t* rom, size_t len) {
  // Allocate buffers
  genesis_alloc_core_buffers();
  #ifndef GENESIS_NO_SOUND
    genesis_alloc_audio_buffers();
  #endif

  // Load the xip ROM into Gwenesis
  load_cartridge((unsigned char*)rom, len);

  // header ASCII "SEGA"0x100
  printf("[ROM] ptr=%p size=%u\n", rom, (unsigned)len);
  if (len >= 0x120) {
    printf("[ROM] hdr[0x100..0x10F]= ");
    for (int i=0; i<16; ++i) printf("%02X ", rom[0x100+i]);
    printf("\n");
  }

  // Init Gwenesis
  power_on();
  reset_emulation();

  // Init display and sound
  genesis_display_init();
  genesis_display_start();
  #ifndef GENESIS_NO_SOUND
    genesis_sound_init();
    genesis_sound_ym_init();
    genesis_sound_ym_start();
  #endif
  
  printf("[GWENESIS] Start\n");

  // Main emulation loop with frame pacing
  uint64_t next_frame_us = esp_timer_get_time();
  for (;;) {
    const int fps = g_target_fps; // snapshot
    const uint32_t frame_us = (fps > 0) ? (1000000u / (uint32_t)fps) : 0u;

    // Emulate one frame
    run_one_frame();

    // Pacing to maintain target FPS
    if (frame_us > 0) {
      next_frame_us += frame_us;

      // If we are late, adjust next_frame_us to now + frame_us
      int64_t now = (int64_t)esp_timer_get_time();
      if (now - (int64_t)next_frame_us > (int64_t)frame_us) {
        next_frame_us = (uint64_t)now + frame_us;
      }

      sleep_until_us(next_frame_us);
    } else {
      taskYIELD();
    }
  }
}
