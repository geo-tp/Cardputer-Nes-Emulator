// ngc_display.h
#pragma once
#include <stdint.h>

#define NGPC_W 160
#define NGPC_H 152

#ifdef __cplusplus
extern "C" {
#endif

// Framebuffer utilisé par graphics.cpp (RGB565 160x152)
extern unsigned short *drawBuffer;

// Initialisation de la couche display (à appeler une fois au boot)
void ngc_display_init(void);

// Appelée par graphics.cpp en VBlank pour pousser l’image vers l’écran.
// Version entrelacée : n’écrit qu’une ligne sur deux par frame.
void graphics_paint(unsigned char render);

#ifdef __cplusplus
}
#endif
