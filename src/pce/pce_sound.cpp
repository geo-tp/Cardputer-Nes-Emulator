#include "pce_sound.h"
#include <M5Cardputer.h>
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
  #include "pce-go/psg.h"
}

static constexpr int kChannel = 0;
static constexpr size_t kNumFrames = 62;
static constexpr size_t kNumSamples = kNumFrames;
static int16_t* s_buf[2] = { nullptr, nullptr };
static uint8_t  s_flip   = 0;
static TaskHandle_t s_audioTaskHandle = nullptr;
static volatile bool s_paused  = false;
static volatile bool s_running = false;
static int s_sampleRate        = 22050;

// ---------- Task audio ----------

static void pce_audio_task(void* arg)
{
  printf("[PCE][AUDIO] task started. frames=%d, samples=%d, rate=%d\n",
         (int)kNumFrames, (int)kNumSamples, s_sampleRate);

  while (s_running) {
    // Pause
    while (s_paused && s_running) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (!s_running) break;

    // too much sound queued
    while (s_running && M5Cardputer.Speaker.isPlaying(kChannel) >= 2) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (!s_running) break;

    int16_t* out = s_buf[s_flip];
    psg_update(out, (int)kNumFrames, 0xFF);

    (void)M5Cardputer.Speaker.playRaw(
      (const int16_t*)out,
      kNumSamples,
      (uint32_t)s_sampleRate,
      false,                    // mono=false
      1,                        // repeat
      kChannel,
      false                     // do not cut what is already playing
    );

    s_flip ^= 1;
  }

  printf("[PCE][AUDIO] task exit\n");
  vTaskDelete(nullptr);
}

// ---------- Public API ----------

extern "C" bool pce_sound_init(int sample_rate)
{
  if (s_audioTaskHandle) {
    return true;
  }

  s_sampleRate = sample_rate > 0 ? sample_rate : s_sampleRate;

  // Buffers audio
  for (int i = 0; i < 2; ++i) {
    s_buf[i] = (int16_t*)heap_caps_calloc(
      kNumSamples, sizeof(int16_t),
      MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
    );
    if (!s_buf[i]) {
      printf("[PCE][AUDIO] buffer alloc failed (%d)\n", i);
      return false;
    }
  }

  auto cfg = M5Cardputer.Speaker.config();
  cfg.sample_rate       = s_sampleRate;
  cfg.stereo            = false;
  cfg.dma_buf_len       = 512;
  cfg.dma_buf_count     = 8;
  cfg.task_priority     = 6;
  cfg.task_pinned_core  = 0;
  M5Cardputer.Speaker.config(cfg);

  if (!M5Cardputer.Speaker.isRunning()) {
    M5Cardputer.Speaker.begin();
  }
  M5Cardputer.Speaker.setVolume(80);

  M5Cardputer.Speaker.stop(kChannel);

  s_flip    = 0;
  s_paused  = false;
  s_running = true;

  BaseType_t ok = xTaskCreatePinnedToCore(
    pce_audio_task,
    "pce_sound",
    3192,
    nullptr,
    6,              // prio
    &s_audioTaskHandle,
    0               // core 0
  );

  if (ok != pdPASS) {
    printf("[PCE][AUDIO] xTaskCreate failed\n");
    s_audioTaskHandle = nullptr;
    s_running = false;
    return false;
  }

  return true;
}

extern "C" void pce_sound_set_paused(bool paused)
{
  s_paused = paused;
  if (paused) {
    M5Cardputer.Speaker.stop(kChannel);
  }
}

extern "C" void pce_sound_deinit(void)
{
  if (!s_audioTaskHandle) return;

  s_running = false;
  s_audioTaskHandle = nullptr;

  M5Cardputer.Speaker.stop(kChannel);

  for (int i = 0; i < 2; ++i) {
    if (s_buf[i]) {
      heap_caps_free(s_buf[i]);
      s_buf[i] = nullptr;
    }
  }
}
