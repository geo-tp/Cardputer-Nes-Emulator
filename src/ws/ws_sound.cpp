#include "ws_sound.h"
extern "C" {
  #include "oswan/WSApu.h" 
}
#include <Arduino.h>
#include <M5Cardputer.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static int           g_sample_rate = 48000;
static constexpr int kFps          = 75;
static int           g_chunk       = 0; 
static constexpr int kChannel      = 0;
static constexpr int kMaxChunk = 400;
static int16_t* s_buf0 = NULL;
static int16_t* s_buf1 = NULL;
static int16_t* s_buf[2] = { NULL, NULL };
static uint8_t  s_flip   = 0;

// Task
static TaskHandle_t s_taskAudio  = nullptr;
static volatile bool s_runAudio  = false;
static TickType_t    s_periodTicks = 0;

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------
static inline int16_t clamp16(int32_t v) {
  if (v >  32767) return  32767;
  if (v < -32768) return -32768;
  return (int16_t)v;
}

static bool buffers_ok() {
  return g_chunk <= kMaxChunk;
}

static inline void build_block_from_apu(int16_t* dst) {
  int need = g_chunk;
  int have = apuBufLen();

  // Si pas assez
  int to_read = (have >= need) ? need : have;

  // Consommation 
  for (int i = 0; i < to_read; ++i) {
    int16_t L = sndbuffer[0][rBuf];
    int16_t R = sndbuffer[1][rBuf];
    rBuf = (rBuf + 1);
    if (rBuf >= SND_RNGSIZE) rBuf = 0;

    int32_t m = ((int32_t)L + (int32_t)R) / 2;
    dst[i] = clamp16(m);
  }

  // Pad silence
  for (int i = to_read; i < need; ++i) {
    dst[i] = 0;
  }
}

static inline void queue_block(const int16_t* pcm) {
  (void)M5Cardputer.Speaker.playRaw(
    pcm,
    (size_t)g_chunk,
    (uint32_t)g_sample_rate,
    false,     // mono
    1,         // play once
    kChannel,
    false // dont stop current sound
  );
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------
extern "C" void ws_sound_init(int sample_rate_hz) {
  g_sample_rate = sample_rate_hz > 0 ? sample_rate_hz : 48000;
  g_chunk = (g_sample_rate + kFps/2) / kFps;

  if (!buffers_ok()) {
    g_chunk = kMaxChunk;
  }

  if (s_buf0 || s_buf1) {
          free(s_buf0);
          free(s_buf1);
          s_buf0 = s_buf1 = NULL;
          s_buf[0] = s_buf[1] = NULL;
      }

  s_buf0 = (int16_t*)malloc(kMaxChunk * sizeof(int16_t));
  s_buf1 = (int16_t*)malloc(kMaxChunk * sizeof(int16_t));
  memset(s_buf0, 0, kMaxChunk * sizeof(int16_t));
  memset(s_buf1, 0, kMaxChunk * sizeof(int16_t));
  s_buf[0] = s_buf0;
  s_buf[1] = s_buf1;


  if (!M5Cardputer.Speaker.isRunning()) {
    auto cfg = M5Cardputer.Speaker.config();
    cfg.sample_rate       = g_sample_rate;
    cfg.stereo            = false;  // sortie mono
    cfg.dma_buf_len       = 512;
    cfg.dma_buf_count     = 8;
    cfg.task_priority     = 4;
    cfg.task_pinned_core  = 1;
    M5Cardputer.Speaker.config(cfg);
    M5Cardputer.Speaker.begin();
  }

  M5Cardputer.Speaker.setVolume(80);
  s_flip = 0;
}

extern "C" void ws_sound_set_volume(uint8_t vol) {
  M5Cardputer.Speaker.setVolume(vol);
}

extern "C" void ws_sound_shutdown(void) {
  ws_sound_stop_task();
  M5Cardputer.Speaker.stop(kChannel);
}

extern "C" void ws_sound_frame(void) {
  size_t queued = M5Cardputer.Speaker.isPlaying(kChannel);

  if (queued == 0) {
    // Amorcer 2 blocs
    for (int i = 0; i < 2; ++i) {
      build_block_from_apu(s_buf[s_flip]);
      queue_block(s_buf[s_flip]);
      s_flip ^= 1;
    }
  } else if (queued == 1) {
    // Maintenir 2 blocs
    build_block_from_apu(s_buf[s_flip]);
    queue_block(s_buf[s_flip]);
    s_flip ^= 1;
  } else {
    // queued >= 2
  }
}

// -----------------------------------------------------------------------------
// Task
// -----------------------------------------------------------------------------
static void ws_audio_task(void* arg) {
  (void)arg;
  TickType_t last = xTaskGetTickCount();

  while (s_runAudio) {
    ws_sound_frame();
    vTaskDelayUntil(&last, s_periodTicks);
  }
  vTaskDelete(nullptr);
}

extern "C" void ws_sound_start_task(uint32_t period_ms, int core) {
  if (s_taskAudio) return;

  if (period_ms == 0) period_ms = 8; 
  s_periodTicks = pdMS_TO_TICKS(period_ms);

  s_runAudio = true;
  xTaskCreatePinnedToCore(ws_audio_task, "ws_audio", 2048, nullptr, 5, &s_taskAudio, core < 0 ? 0 : core);
}

extern "C" void ws_sound_stop_task(void) {
  if (!s_taskAudio) return;
  s_runAudio = false;
  s_taskAudio = nullptr;
}
