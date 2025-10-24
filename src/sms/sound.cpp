#include "sound.h"
#include <M5Cardputer.h>
#include <algorithm>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t gAudioTask = nullptr;
static volatile bool gRunAudio = true;

static constexpr int kSampleRate = 22050;   // Hz
static constexpr int kChannel    = 0;
static constexpr int   kChunk      = 128; 

static int16_t s_buf[2][512];
static uint8_t s_flip = 0;

static inline int clamp16(int x){ return x < -32768 ? -32768 : (x > 32767 ? 32767 : x); }
static void mix_lr_to(int16_t* dst, const int16_t* L, const int16_t* R, int n){
  for (int i=0;i<n;i++) dst[i] = (int16_t)clamp16(((int)L[i] + (int)R[i]) >> 1);
}

void sms_audio_init(){
  auto cfg = M5Cardputer.Speaker.config();
  cfg.sample_rate       = kSampleRate;
  cfg.stereo            = false;
  cfg.dma_buf_len       = 512;
  cfg.dma_buf_count     = 8;
  cfg.task_priority     = 4;
  cfg.task_pinned_core  = 1;
  M5Cardputer.Speaker.config(cfg);

  if (!M5Cardputer.Speaker.isRunning())
    M5Cardputer.Speaker.begin();

  M5Cardputer.Speaker.setVolume(80);
  M5Cardputer.Speaker.stop(kChannel);
}

void sms_audio_stop(){
  M5Cardputer.Speaker.stop(kChannel);
}

IRAM_ATTR void sms_audio_frame(){
  if (!snd.buffer[0] || !snd.buffer[1] || snd.bufsize <= 0) return;

  int cap = (int)(sizeof(s_buf[0]) / sizeof(int16_t));
  int n = std::min((int)snd.bufsize, cap);

  size_t st = M5Cardputer.Speaker.isPlaying(kChannel);

  if (st == 0) {
    int16_t* out0 = s_buf[s_flip];
    mix_lr_to(out0, snd.buffer[0], snd.buffer[1], n);
    (void)M5Cardputer.Speaker.playRaw(out0, (size_t)n, (uint32_t)kSampleRate, false, 1, kChannel, false);

    s_flip ^= 1;
    int16_t* out1 = s_buf[s_flip];
    mix_lr_to(out1, snd.buffer[0], snd.buffer[1], n);
    (void)M5Cardputer.Speaker.playRaw(out1, (size_t)n, (uint32_t)kSampleRate, false, 1, kChannel, false);

    s_flip ^= 1;
    return;
  }

  if (st == 1) {
    int16_t* out = s_buf[s_flip];
    mix_lr_to(out, snd.buffer[0], snd.buffer[1], n);
    (void)M5Cardputer.Speaker.playRaw(out, (size_t)n, (uint32_t)kSampleRate, false, 1, kChannel, false);
    s_flip ^= 1;
  }
  // st == 2 => deux buffers déjà en file, on laisse tourner
}

static void audio_task(void*){
  const TickType_t period =
      std::max<TickType_t>(1, pdMS_TO_TICKS((kChunk * 1000) / kSampleRate)); // ~11-12 ms
  TickType_t last = xTaskGetTickCount();

  for(;;){
    if (!gRunAudio) break;
    sms_audio_frame();
    vTaskDelayUntil(&last, period); // synchro sur la durée du chunk
  }
  vTaskDelete(NULL);
}

void sms_audio_start_task(){
  gRunAudio = true;
  if (!gAudioTask){
    xTaskCreatePinnedToCore(audio_task, "audio", 4096, nullptr, 6, &gAudioTask, 1);
  }
}

void sms_audio_stop_task(){
  gRunAudio = false;
  // on ne delete pas explicitement ici; la task s’auto-delete à la sortie
  gAudioTask = nullptr;
}
