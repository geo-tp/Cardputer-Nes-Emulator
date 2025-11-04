/*---------------------------------------------------------------------------
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version. See also the license.txt file for
 *	additional informations.
 *---------------------------------------------------------------------------
 */

/* graphics.h: interface for the graphics class.
 *
 */

#ifndef AFX_GRAPHICS_H
#define AFX_GRAPHICS_H

/* actual NGPC */
#define NGPC_SIZEX 160
#define NGPC_SIZEY 152

/* render screen 260x152 is good for NGPC */
#define SIZEX	260
#define SIZEY	152

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char *sprite_table;
extern unsigned char *pattern_table;
extern unsigned short*patterns;
extern unsigned short *tile_table_front;
extern unsigned short *tile_table_back;
extern unsigned short *palette_table;
extern unsigned char *bw_palette_table;
extern unsigned char *sprite_palette_numbers;

extern unsigned char *scanlineY;
/* frame 0/1 priority registers */
extern unsigned char *frame0Pri;
extern unsigned char *frame1Pri;
/* windowing registers */
extern unsigned char *wndTopLeftX;
extern unsigned char *wndTopLeftY;
extern unsigned char *wndSizeX;
extern unsigned char *wndSizeY;
/* scrolling registers */
extern unsigned char *scrollSpriteX;
extern unsigned char *scrollSpriteY;
extern unsigned char *scrollFrontX;
extern unsigned char *scrollFrontY;
extern unsigned char *scrollBackX;
extern unsigned char *scrollBackY;
/* background color selection register and table */
extern unsigned char *bgSelect;
extern unsigned short *bgTable;
extern unsigned char *oowSelect;
extern unsigned short *oowTable;
/* machine constants */
extern unsigned char *color_switch;

BOOL graphics_init(void);
void graphics_paint(unsigned char render);
void graphicsSetDarkFilterLevel(unsigned filterLevel);
/* new renderer (NeoGeo Pocket (Color)) */
void myGraphicsBlitLine(unsigned char render);

/*
 * adventure vision stuff
 */

extern uint16_t* totalpalette;
extern uint16_t palettes[16*4+16*4+16*4];
#define NGPC_TO_SDL16(col) totalpalette[col & 0x0FFF]

#define setColPaletteEntry(addr, data) palettes[(addr)] = NGPC_TO_SDL16(data)
#define setBWPaletteEntry(addr, data) palettes[(addr)] = NGPC_TO_SDL16(data)

extern unsigned char *scanlineY;

struct ngp_screen
{
   int w, h;                 
   void *pixels;
};

#ifdef __cplusplus
}
#endif

#endif /* !defined(AFX_GRAPHICS_H) */
