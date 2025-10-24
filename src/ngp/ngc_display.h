// ngc_display.h
#pragma once
#include <stdint.h>

#define NGPC_W 160
#define NGPC_H 152

#ifdef __cplusplus
extern "C" {
#endif

// Framebuffer
extern unsigned short *drawBuffer;
extern volatile unsigned g_frame_ready;
extern volatile unsigned g_frame_counter;

// Init
void ngc_display_init(void);
void ngc_display_set_parity(bool odd);

// Draw frame
void graphics_paint(unsigned char render);

#ifdef __cplusplus
}
#endif
