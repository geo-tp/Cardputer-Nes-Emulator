#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ws_display_init(void);
void ws_display_start(void);
void ws_display_stop(void);

/* Oswan Core hook */
void ws_graphics_paint(void);

extern bool ws_fullscreen;
extern int  ws_zoomPercent;

#ifdef __cplusplus
}
#endif
