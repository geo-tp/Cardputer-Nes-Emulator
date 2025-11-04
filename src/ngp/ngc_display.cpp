// ngc_display.cpp

#include "ngc_display.h"
#include <M5Cardputer.h>
#include <string.h>
#include <atomic>

// Framebuffer NGPC 160x152
int ngpZoomPercent = 100;
bool ngpFullscreen =  true;
volatile unsigned g_frame_ready = 0;
volatile unsigned g_frame_counter = 0;
bool s_lastScreenMode = ngpFullscreen;

// LUT state
static bool s_lut_ready = false;

// Entrelacement odd/even
static bool s_interlace_parity = false;

static uint16_t* s_linebuf_panel = nullptr;
static uint16_t* s_lut_x_full    = nullptr;
static uint16_t* s_lut_y_full    = nullptr;
static uint16_t* s_lut_x_4x3     = nullptr;
static uint16_t* s_fb            = nullptr;
unsigned short *drawBuffer = s_fb; 

extern "C" void ngc_display_init(void)
{
  const size_t fbBytes = (size_t)NGPC_W * (size_t)NGPC_H * sizeof(uint16_t);

  // --- Framebuffer ---
  if (!s_fb) {
    s_fb = (uint16_t*)heap_caps_malloc(fbBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_fb) s_fb = (uint16_t*)malloc(fbBytes);
    if (s_fb) memset(s_fb, 0, fbBytes);
    drawBuffer = s_fb;
  }

  // --- Buffers dynamiques ---
  if (!s_linebuf_panel)
    s_linebuf_panel = (uint16_t*)heap_caps_malloc(240 * sizeof(uint16_t),
                                                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  if (!s_lut_x_full)
    s_lut_x_full = (uint16_t*)heap_caps_malloc(240 * sizeof(uint16_t),
                                               MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  if (!s_lut_y_full)
    s_lut_y_full = (uint16_t*)heap_caps_malloc(135 * sizeof(uint16_t),
                                               MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  if (!s_lut_x_4x3)
    s_lut_x_4x3 = (uint16_t*)heap_caps_malloc(180 * sizeof(uint16_t),
                                              MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  M5.Display.setSwapBytes(true);
  M5.Display.fillScreen(TFT_BLACK);
}

extern "C" void ngc_display_set_parity(bool odd) {
  s_interlace_parity = odd;
}

static void build_scale_luts()
{
  const int panelW = M5.Display.width();   // 240
  const int panelH = M5.Display.height();  // 135
  const int srcW   = NGPC_W;               // 160
  const int srcH   = NGPC_H;               // 152

  // Fullscreen
  for (int x = 0; x < panelW; ++x) {
    uint32_t sx = ((uint32_t)x * (uint32_t)srcW) / (uint32_t)panelW;
    s_lut_x_full[x] = (uint16_t)((sx < (uint32_t)srcW ? sx : (uint32_t)(srcW-1)) << 1);
  }
  for (int y = 0; y < panelH; ++y) {
    uint32_t sy = ((uint32_t)y * (uint32_t)srcH) / (uint32_t)panelH;
    s_lut_y_full[y] = (uint16_t)((sy < (uint32_t)srcH ? sy : (uint32_t)(srcH-1)));
  }

  // 4:3 aspect ratio
  const int outH   = panelH;              // 135
  const int outW   = (outH * 4) / 3;      // 180
  const int virtW  = (srcH * 4) / 3;      // 202
  const int virtOff= (virtW - srcW) / 2;  // 21

  for (int x = 0; x < outW; ++x) {
    int virtX = ((int32_t)x * virtW) / outW;
    int srcX  = virtX - virtOff;
    s_lut_x_4x3[x] = (unsigned)srcX < (unsigned)srcW ? (uint16_t)srcX : 0;
  }

  s_lut_ready = true;
}

static inline IRAM_ATTR void paint_fullscreen_stretch()
{
  const int panelW = 240;
  const int panelH = 135;
  const int srcW   = NGPC_W;

  const int parity = (int)s_interlace_parity;

  M5.Display.startWrite();

  for (int y = parity; y < panelH; y += 2) {
    const uint16_t  srcY    = s_lut_y_full[y];                         // 0..(srcH-1)
    const uint16_t* srcLine = drawBuffer + (size_t)srcY * srcW;        // source
    const uint8_t*  base    = (const uint8_t*)srcLine;                 // base 8-bit
    const uint16_t* lutx    = s_lut_x_full;                            // offsets
    uint16_t*       dst     = s_linebuf_panel;

    // Unrolled loop for speed
    int x = 0;
    for (; x <= panelW - 8; x += 8) {
      dst[x+0] = *(const uint16_t*)(base + lutx[x+0]);
      dst[x+1] = *(const uint16_t*)(base + lutx[x+1]);
      dst[x+2] = *(const uint16_t*)(base + lutx[x+2]);
      dst[x+3] = *(const uint16_t*)(base + lutx[x+3]);
      dst[x+4] = *(const uint16_t*)(base + lutx[x+4]);
      dst[x+5] = *(const uint16_t*)(base + lutx[x+5]);
      dst[x+6] = *(const uint16_t*)(base + lutx[x+6]);
      dst[x+7] = *(const uint16_t*)(base + lutx[x+7]);
    }
    for (; x < panelW; ++x) {
      dst[x] = *(const uint16_t*)(base + lutx[x]);
    }

    M5.Display.setAddrWindow(0, y, panelW, 1);
    M5.Display.writePixels(dst, panelW, /*swapBytesAlready=*/true);
  }

  M5.Display.endWrite();
}

static inline IRAM_ATTR void paint_fullheight_4x3()
{
  const int panelW = 240;
  const int panelH = 135;
  const int srcW   = NGPC_W;   // 160
  const int srcH   = NGPC_H;   // 152

  const int outW    = 160;
  const int x_start = (panelW - outW) / 2; // 40
  const uint32_t stepY  = ((uint32_t)srcH << 16) / (uint32_t)panelH;
  const int      parity = (int)s_interlace_parity;
  uint32_t       accY   = (uint32_t)parity * stepY;
  const uint32_t stepY2 = stepY << 1; // we skip one line

  M5.Display.startWrite();

  for (int y = parity; y < panelH; y += 2) {
    const int srcY = (int)(accY >> 16);
    accY += stepY2;

    const uint16_t* __restrict srcLine = drawBuffer + (size_t)srcY * srcW;

    M5.Display.setAddrWindow(x_start, y, outW, 1);
    M5.Display.writePixels(srcLine, outW, /*swapBytesAlready=*/true);
  }

  M5.Display.endWrite();
}

extern "C" IRAM_ATTR void graphics_paint(unsigned char render)
{
  if (!render || !drawBuffer) return;
  if (!s_lut_ready) build_scale_luts();
  
  if (s_lastScreenMode && !ngpFullscreen) {
    M5.Display.fillScreen(TFT_BLACK);
  }

  s_lastScreenMode = ngpFullscreen;
  
  if (ngpFullscreen) {
    paint_fullscreen_stretch();
  } else {
    paint_fullheight_4x3();
  }

  // alternate
  s_interlace_parity = !s_interlace_parity;
}
