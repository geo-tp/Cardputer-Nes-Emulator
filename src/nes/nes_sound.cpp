#include <M5Cardputer.h>

extern "C" {
  #include <osd.h>
  #include <stdbool.h>
}

// Param audio 
static constexpr int kSampleRate = 22050; // Hz
static constexpr int kNesHz      = 60;    // 60 FPS
static constexpr int kChunk      = (kSampleRate + kNesHz/2) / kNesHz; // 368 samples per frame

// Canal audio
static constexpr int kChannel    = 0;

// Callback Nofrendo
static void (*s_audio_cb)(void *buffer, int length) = nullptr;

// Double buffer mono (Nofrendo to HP)
static int16_t* s_buf[2] = { nullptr, nullptr };
static uint8_t s_flip = 0;

extern "C" {

int osd_init_sound() {
  s_buf[0] = (int16_t*) heap_caps_calloc(kChunk, sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  s_buf[1] = (int16_t*) heap_caps_calloc(kChunk, sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  auto cfg = M5Cardputer.Speaker.config();
  cfg.sample_rate    = kSampleRate;
  cfg.stereo         = false;   // mono
  cfg.dma_buf_len    = 512;     // buffers DMA
  cfg.dma_buf_count  = 8;
  cfg.task_priority  = 4;       // high priority
  cfg.task_pinned_core = 1;     // pin to core 1 (avoid conflict with other tasks)
  M5Cardputer.Speaker.config(cfg);

  if (!M5Cardputer.Speaker.isRunning()) {
    M5Cardputer.Speaker.begin();
  }
  M5Cardputer.Speaker.setVolume(80);// 0..255

  M5Cardputer.Speaker.stop(kChannel);
  return 0;
}

void osd_stopsound() {
  s_audio_cb = nullptr;
  M5Cardputer.Speaker.stop(kChannel);
}

void osd_setsound(void (*playfunc)(void *buffer, int length)) {
  s_audio_cb = playfunc;
}

void osd_getsoundinfo(sndinfo_t *info) {
  info->sample_rate = kSampleRate;
  info->bps         = 16;
}

void do_audio_frame() {
  if (!s_audio_cb) return;

  size_t st = M5Cardputer.Speaker.isPlaying(kChannel);

  // if nothing is playing, we queue 2 chunks to avoid gaps
  if (st == 0) {
    for (int i = 0; i < 2; ++i) {
      s_audio_cb((void*)s_buf[s_flip], kChunk);
      (void)M5Cardputer.Speaker.playRaw(
        (const int16_t*)s_buf[s_flip], (size_t)kChunk,
        (uint32_t)kSampleRate, false /*mono*/,
        1 /*repeat*/, kChannel, false /*stop_current_sound*/
      );
      s_flip ^= 1;
    }
    return;
  }

  // if we have space, we add exactly 1 chunk
  if (st == 1) {
    s_audio_cb((void*)s_buf[s_flip], kChunk);
    (void)M5Cardputer.Speaker.playRaw(
      (const int16_t*)s_buf[s_flip], (size_t)kChunk,
      (uint32_t)kSampleRate, false /*mono*/,
      1 /*repeat*/, kChannel, false /*no cut*/
    );
    s_flip ^= 1;
  }

  // st == 2, we have 2 chunks playing, nothing to do
}

} // extern "C"
