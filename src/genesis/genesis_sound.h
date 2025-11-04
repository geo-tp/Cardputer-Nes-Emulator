#pragma once
#include <stddef.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Constantes audio
static constexpr int  AUDIO_SR      = 53267;   // Hz
static constexpr bool AUDIO_STEREO  = false;
static constexpr int  AUDIO_CHUNK   = 888;     // 53.2kHz / 60fps
static constexpr int  AUDIO_POOL    = 3; 
static constexpr int  AUDIO_Q_DEPTH = 8;

typedef void (*GenesisAudioSink)(int16_t* pcm, size_t n_samples, int sample_rate);

struct AudioMsg {
  int16_t* buf;
  size_t   n;    // samples
};

typedef struct {
  uint32_t mclk;   // horloge master
  uint8_t  port;   // 0,2: addr ; 1,3: data
  uint8_t  value;
} YmWrite;

#ifdef __cplusplus
extern "C" {
#endif

// Buffers PSG
extern int16_t* gwenesis_sn76489_buffer;
extern int      sn76489_index;
extern int      sn76489_clock;

// Buffers YM2612
extern int16_t* gwenesis_ym2612_buffer;
extern int      ym2612_index;
extern int      ym2612_clock;
extern volatile int g_ym_target_clock;

extern uint8_t genesis_audio_volume;

// API audio
void genesis_alloc_audio_buffers(void);
void genesis_sound_init();
void genesis_sound_submit_frame(void);

// API YM
void genesis_sound_ym_init(void);
void genesis_sound_ym_start(void);
void genesis_sound_ym_stop(void);
void genesis_sound_ym_set_target_clock(int target);
int  genesis_sound_ym_get_target_clock(void);

#ifdef __cplusplus
} // extern "C"
#endif
