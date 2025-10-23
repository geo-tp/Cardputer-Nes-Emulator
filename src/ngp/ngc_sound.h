#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void ngc_sound_init(void);
void ngc_sound_shutdown(void);
void ngc_sound_frame(void);
void ngc_sound_set_volume(uint8_t vol);

#ifdef __cplusplus
}
#endif
