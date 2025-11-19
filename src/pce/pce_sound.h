#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool pce_sound_init(int sample_rate);
void pce_sound_set_paused(bool paused);
void pce_sound_deinit(void);

#ifdef __cplusplus
}
#endif
