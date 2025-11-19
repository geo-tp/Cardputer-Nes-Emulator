// pce_display.h
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool pceFullScreen;
extern int  pceZoomLevel;

void pce_display_init(void);
void pce_display_start(void);
void pce_display_stop(void);

void pce_display_submit_frame(const uint8_t *index_fb_base,
                              int pitch,
                              int width,
                              int height,
                              const uint16_t *palette);

#ifdef __cplusplus
}
#endif
