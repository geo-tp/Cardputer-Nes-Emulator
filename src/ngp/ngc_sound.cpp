#include "ngc_sound.h"

#include <Arduino.h>
#include <M5Cardputer.h>

extern "C" {
  #include "ngp/race/types.h"     // _u16
  void sound_init(int SampleRate);
  void sound_update(_u16* chip_buffer, int length_bytes); // PSG
  void dac_update  (_u16* dac_buffer,  int length_bytes); // DAC
}

// -------- Settings --------
static constexpr int kSampleRate = 22050;  // Hz
static constexpr int kFps        = 60;     // NGP 60 Hz
static constexpr int kChunk      = (kSampleRate + kFps/2) / kFps; // 368 smp/frame
static constexpr int kChannel    = 0;

// -------- Buffers --------
static int16_t  s_buf[2][kChunk]; // double buffer mono
static uint8_t  s_flip = 0;
static uint16_t s_psg[kChunk];
static uint16_t s_dac[kChunk];

// -------- Helpers --------
static inline int16_t clamp16(int32_t v) {
  if (v >  32767) return  32767;
  if (v < -32768) return -32768;
  return (int16_t)v;
}

static inline void mix_build_block(int16_t* dst /*kChunk*/) {
  const int lenB = kChunk * (int)sizeof(uint16_t);
  sound_update(s_psg, lenB);
  dac_update  (s_dac, lenB);

  for (int i = 0; i < kChunk; ++i) {
    int32_t a = (int16_t)s_psg[i];
    int32_t b = (int16_t)s_dac[i];
    dst[i] = clamp16(a + b); // mono mix
  }
}

static inline void queue_block(const int16_t* pcm /*kChunk*/) {
  (void)M5Cardputer.Speaker.playRaw(
    pcm, (size_t)kChunk, (uint32_t)kSampleRate,
    false /*mono*/, 1 /*repeat*/,
    kChannel, false /*no cut current*/
  );
}

// -------- API --------
extern "C" {

void ngc_sound_init(void) {
  sound_init(kSampleRate);

  if (!M5Cardputer.Speaker.isRunning()) {
    auto cfg = M5Cardputer.Speaker.config();
    cfg.sample_rate       = kSampleRate;
    cfg.stereo            = false; // mono out
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

void ngc_sound_set_volume(uint8_t vol) {
  M5Cardputer.Speaker.setVolume(vol);
}

void ngc_sound_shutdown(void) {
  M5Cardputer.Speaker.stop(kChannel);
}

void ngc_sound_frame(void) {
  size_t queued = M5Cardputer.Speaker.isPlaying(kChannel);

  if (queued == 0) {
    // Prime with 2 blocks
    for (int i = 0; i < 2; ++i) {
      mix_build_block(s_buf[s_flip]);
      queue_block(s_buf[s_flip]);
      s_flip ^= 1;
    }
  } else if (queued == 1) {
    // Keep at 2 blocks depth
    mix_build_block(s_buf[s_flip]);
    queue_block(s_buf[s_flip]);
    s_flip ^= 1;
  } else {
    // queued == 2 → nothing, we’re buffered
  }
}

} // extern "C"
