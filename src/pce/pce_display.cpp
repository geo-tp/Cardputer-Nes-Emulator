#include "pce_display.h"
#include <Arduino.h>
#include <M5Cardputer.h>
#include "esp_heap_caps.h"

extern "C" {
  #include "pce-go/pce.h"
  #include "pce-go/psg.h"
  void* PalettePCE(int brightness);
}

typedef struct {
  const uint8_t  *index_fb_base;
  const uint16_t *palette;
  int pitch;
  int width;
  int height;
} PceFrameMsg;

// Globals
static QueueHandle_t  s_frameQ   = nullptr;
static TaskHandle_t   s_task     = nullptr;
static uint16_t      *s_lineBuf  = nullptr;   // ligne RGB565
static int            s_lineCap  = 0;

static uint8_t*  pce_index_fb_base = nullptr;
static uint8_t*  pce_index_fb      = nullptr; // start visible (offset 16)
static uint16_t *pce_palette       = nullptr;
static int       pce_fb_width  = 0;           // visible
static int       pce_fb_height = 0;           // visible
static int       pce_fb_pitch  = 0;

bool pceFullScreen = true;
int  pceZoomLevel = 110;

static int  s_lastZoomLevel   = -1;
static bool s_lastFullScreen  = false;
static int  s_lastSrcW        = 0;
static int  s_lastSrcH        = 0;
static int  s_lastLcdW        = 0;
static int  s_lastLcdH        = 0;

static uint8_t* s_bottomCache        = nullptr;
static int      s_bottomCacheLines   = 0;
static int      s_bottomCacheW       = 0;
static int      s_bottomCacheStartY  = 0;

struct PceDisplayTransform {
  int   dstW;
  int   dstH;
  int   xOffset;
  int   yOffset;
  float invScaleX;
  float invScaleY;
  float srcCX;
  float srcCY;
  float dstCX;
  float dstCY;
};

static PceDisplayTransform s_transform;

static void pce_display_transform(int srcW, int srcH)
{
  int lcdW = M5Cardputer.Display.width();
  int lcdH = M5Cardputer.Display.height();

  if (lcdW <= 0 || lcdH <= 0 || srcW <= 0 || srcH <= 0) {
    return;
  }

  bool full = pceFullScreen;
  int  zoom = (pceZoomLevel > 0) ? pceZoomLevel : 100;

  // nothing changed ?
  if (zoom          == s_lastZoomLevel  &&
      full          == s_lastFullScreen &&
      srcW          == s_lastSrcW       &&
      srcH          == s_lastSrcH       &&
      lcdW          == s_lastLcdW       &&
      lcdH          == s_lastLcdH) {
    return;
  }

  if (!pceFullScreen) M5Cardputer.Display.fillScreen(TFT_BLACK);    

  // memorize current state
  s_lastZoomLevel  = zoom;
  s_lastFullScreen = full;
  s_lastSrcW       = srcW;
  s_lastSrcH       = srcH;
  s_lastLcdW       = lcdW;
  s_lastLcdH       = lcdH;

  float zoomFactor = zoom / 100.0f;
  if (zoomFactor <= 0.0f) zoomFactor = 1.0f;

  int   dstW, dstH;
  int   xOffset, yOffset;
  float baseInvScaleX, baseInvScaleY;

  if (!full) {
    // Mode original
    float scaleX = (float)lcdW / (float)srcW;
    float scaleY = (float)lcdH / (float)srcH;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;
    if (scale <= 0.0f) scale = 1.0f;

    dstW = (int)(srcW * scale);
    dstH = (int)(srcH * scale);
    if (dstW < 1) dstW = 1;
    if (dstH < 1) dstH = 1;

    xOffset = (lcdW - dstW) / 2;
    yOffset = (lcdH - dstH) / 2;
    if (xOffset < 0) xOffset = 0;
    if (yOffset < 0) yOffset = 0;

    baseInvScaleX = (float)srcW / (float)dstW;
    baseInvScaleY = (float)srcH / (float)dstH;
  } else {
    // Mode Fullscreen
    dstW = lcdW;
    dstH = lcdH;

    xOffset = 0;
    yOffset = 0;

    baseInvScaleX = (float)srcW / (float)dstW;
    baseInvScaleY = (float)srcH / (float)dstH;
  }

  // Apply zoom
  float invScaleX = baseInvScaleX / zoomFactor;
  float invScaleY = baseInvScaleY / zoomFactor;

  s_transform.dstW      = dstW;
  s_transform.dstH      = dstH;
  s_transform.xOffset   = xOffset;
  s_transform.yOffset   = yOffset;
  s_transform.invScaleX = invScaleX;
  s_transform.invScaleY = invScaleY;

  s_transform.srcCX = (float)srcW * 0.5f;
  s_transform.srcCY = (float)srcH * 0.5f;
  s_transform.dstCX = (float)(dstW - 1) * 0.5f;
  s_transform.dstCY = (float)(dstH - 1) * 0.5f;
}


// ================== DISPLAY TASK ==================

static void pce_display_task(void *arg) {
  for (;;) {
    PceFrameMsg msg;
    if (xQueueReceive(s_frameQ, &msg, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    if (!msg.index_fb_base || !msg.palette || msg.width <= 0 || msg.height <= 0) {
      continue;
    }

    int srcW  = msg.width;
    int srcH  = msg.height;
    int pitch = msg.pitch;

    pce_display_transform(srcW, srcH);

    int   dstW      = s_transform.dstW;
    int   dstH      = s_transform.dstH;
    int   xOffset   = s_transform.xOffset;
    int   yOffset   = s_transform.yOffset;
    float invScaleX = s_transform.invScaleX;
    float invScaleY = s_transform.invScaleY;
    float srcCX     = s_transform.srcCX;
    float srcCY     = s_transform.srcCY;
    float dstCX     = s_transform.dstCX;
    float dstCY     = s_transform.dstCY;

    if (dstW <= 0 || dstH <= 0) {
      continue;
    }

    // Allocate RGB buffer per line
    if (dstW > s_lineCap) {
      if (s_lineBuf) {
        free(s_lineBuf);
        s_lineBuf = nullptr;
      }
      s_lineBuf = (uint16_t*)heap_caps_malloc(dstW * sizeof(uint16_t),
                                              MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
      s_lineCap = dstW;
    }
    if (!s_lineBuf) {
      continue;
    }

    // Prepare cache: 50% bottom of the framebuffer (tearing reduction)
    // this will be overritten by the ppu during rendering, so we need to snapshot it
    int cacheLines = (srcH * 60 + 99) / 100;   // ~55%, arrondi vers le haut
    if (cacheLines < 1) cacheLines = 1;

    // On commence le cache de façon à couvrir les 55% du bas
    int cacheStartY = srcH - cacheLines;
    if (cacheStartY < 0) cacheStartY = 0;

    int neededCacheBytes = cacheLines * srcW;

    if (!s_bottomCache || neededCacheBytes > (s_bottomCacheLines * s_bottomCacheW)) {
      if (s_bottomCache) {
        free(s_bottomCache);
        s_bottomCache = nullptr;
      }
      s_bottomCache = (uint8_t*)heap_caps_malloc(neededCacheBytes, MALLOC_CAP_8BIT);
      if (!s_bottomCache) {
        printf("[PCE] bottomCache alloc failed (%d bytes)\n", neededCacheBytes);
      } else {
        s_bottomCacheLines  = cacheLines;
        s_bottomCacheW      = srcW;
      }
    }

    s_bottomCacheStartY = cacheStartY;

    // Snapshot: copy the bottom 50% into the cache
    if (s_bottomCache) {
      for (int i = 0; i < cacheLines; ++i) {
        int ySrc = cacheStartY + i;
        if (ySrc >= srcH) break;
        const uint8_t* srcLine = msg.index_fb_base + ySrc * pitch + 16; // +16 overscan
        memcpy(s_bottomCache + i * srcW, srcLine, srcW);
      }
    }

    // Render
    M5Cardputer.Display.startWrite();

    for (int y = 0; y < dstH; ++y) {
      float fy    = (float)y - dstCY;
      float srcYf = srcCY + fy * invScaleY;
      int   srcY  = (int)srcYf;
      if (srcY < 0)      srcY = 0;
      if (srcY >= srcH)  srcY = srcH - 1;

      const uint8_t* srcLine = nullptr;

      // if srcY is in the bottom half snapshot, read from the cache
      if (s_bottomCache && srcY >= s_bottomCacheStartY) {
        int idxLine = srcY - s_bottomCacheStartY;
        if (idxLine >= 0 && idxLine < s_bottomCacheLines) {
          srcLine = s_bottomCache + idxLine * srcW;
        }
      }

      // direct read from the framebuffer
      if (!srcLine) {
        srcLine = msg.index_fb_base + srcY * pitch + 16; // +16 overscan
      }

      for (int x = 0; x < dstW; ++x) {
        float fx    = (float)x - dstCX;
        float srcXf = srcCX + fx * invScaleX;
        int   srcX  = (int)srcXf;
        if (srcX < 0)      srcX = 0;
        if (srcX >= srcW)  srcX = srcW - 1;

        uint8_t idx = srcLine[srcX];
        s_lineBuf[x] = msg.palette[idx];
      }

      int dstY = yOffset + y;
      M5Cardputer.Display.setAddrWindow(xOffset, dstY, dstW, 1);
      M5Cardputer.Display.pushPixels(s_lineBuf, dstW);

      if ((y & 31) == 31) vTaskDelay(0);
    }

    M5Cardputer.Display.endWrite();
  }
}

// ================== API DISPLAY ==================

extern "C" void pce_display_init(void) {
  uint16_t* corePal = (uint16_t*)PalettePCE(16);
  if (!corePal) {
    printf("[PCE][ERR] PalettePCE(16) returned NULL\n");
    for(;;) delay(1000);
  }

  if (!pce_palette) {
    pce_palette = (uint16_t*)heap_caps_malloc(
      256 * sizeof(uint16_t),
      MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL
    );
    if (!pce_palette) {
      printf("[PCE][ERR] pce_palette alloc failed\n");
      free(corePal);
      for(;;) delay(1000);
    }
  }

  for (int i = 0; i < 256; i++) {
    uint16_t color = corePal[i];
    pce_palette[i] = color;
  }

  free(corePal);

  if (!s_frameQ) {
    s_frameQ = xQueueCreate(2, sizeof(PceFrameMsg)); // 2 frames max
    if (!s_frameQ) {
      printf("[PCE-DISP] queue create failed\n");
    }
  }
}

extern "C" void pce_display_start(void) {
  if (!s_frameQ) pce_display_init();
  if (!s_task && s_frameQ) {
    BaseType_t ok = xTaskCreatePinnedToCore(
      pce_display_task, "PceDisp",
      3092, nullptr, 6, &s_task,
      0
    );
    if (ok != pdPASS) {
      printf("[PCE-DISP] task create failed\n");
      if (s_task) {
        vTaskDelete(s_task);
      }
      s_task = nullptr;
    }
  }
}

extern "C" void pce_display_stop(void) {
  if (s_task) {
    vTaskDelete(s_task);
    s_task = nullptr;
  }
  if (s_frameQ) {
    vQueueDelete(s_frameQ);
    s_frameQ = nullptr;
  }
  if (s_lineBuf) {
    free(s_lineBuf);
    s_lineBuf = nullptr;
    s_lineCap = 0;
  }
  if (pce_palette) {
    heap_caps_free(pce_palette);
    pce_palette = nullptr;
  }
}

extern "C" void pce_display_submit_frame(const uint8_t *index_fb_base,
                                         int pitch,
                                         int width,
                                         int height,
                                         const uint16_t *palette)
{
  if (!s_frameQ) return;

  PceFrameMsg msg;
  msg.index_fb_base = index_fb_base;
  msg.palette       = palette;
  msg.pitch         = pitch;
  msg.width         = width;
  msg.height        = height;

  // drop if full
  BaseType_t ok = xQueueSend(s_frameQ, &msg, 0);
  if (ok != pdTRUE) {
    // frame drop (silent to avoid spam)
    // printf("[PCE-DISP] frame dropped\n");
  }
}

// ================== HOOKS PCE-GO ==================

extern "C" uint8_t* osd_gfx_framebuffer(int width, int height)
{
  if (width <= 0 || height <= 0) {
    return pce_index_fb;
  }

  pce_fb_width  = width;
  pce_fb_height = height;
  pce_fb_pitch  = XBUF_WIDTH; 

  if (!pce_index_fb_base) {
    size_t indexBytes = (size_t)pce_fb_pitch * (size_t)pce_fb_height;

    pce_index_fb_base = (uint8_t*)heap_caps_malloc(
        indexBytes,
        MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL
    );
    if (!pce_index_fb_base) {
      printf("[PCE] index_fb_base alloc failed\n");
      return nullptr;
    }

    // overscan of 16 pixels
    pce_index_fb = pce_index_fb_base + 16;
  }

  return pce_index_fb;
}

extern "C" void osd_vsync(void)
{
  // send the frame to the display thread (non-blocking)
  pce_display_submit_frame(pce_index_fb_base,
                           pce_fb_pitch,
                           pce_fb_width,
                           pce_fb_height,
                           pce_palette);
}
