extern "C" {
#include <nes/nes.h>
}

#include <M5Unified.h>
#include "display/CardputerView.h"
#include "input/CardputerInput.h"


int16_t frame_scaling;
static int16_t frame_x, frame_y, frame_x_offset, frame_y_offset, frame_width, frame_height, frame_line_pixels;
extern uint16_t myPalette[];
extern bool fullscreenMode;
extern int nesZoomPercent;

extern void display_begin() {}

extern "C" {

void display_init() {
    frame_scaling = 0;

    frame_x = 0;
    frame_y = 0;
    frame_x_offset = 0;
    frame_y_offset = 90;
    frame_width = M5.Lcd.width();
    frame_height = M5.Lcd.height();
    frame_line_pixels = frame_width;
}

void display_write_frame_square(const uint8_t *data[]) {
    M5.Lcd.startWrite();
    M5.Lcd.setWindow(frame_x, frame_y, frame_width - 1, frame_height - 1);

    const int srcW = NES_SCREEN_WIDTH;
    const int srcH = NES_SCREEN_HEIGHT;

    int outH = frame_height;
    int desiredW = 220; 

    int outW = desiredW;
    if (outW > frame_width) outW = frame_width;

    const int x_start = (frame_width - outW) / 2;
    const int x_end   = x_start + outW;          // exclusive
    const int virtW   = (srcH * 4) / 3;
    const int virtOff = (virtW - srcW) / 2;

    static uint16_t lineBuf[NES_SCREEN_WIDTH];

    for (int32_t y = 0; y < frame_height; ++y) {
        // Map Y screen -> Y NES 
        int srcY = (y * srcH) / outH;
        if (srcY < 0) srcY = 0; else if (srcY >= srcH) srcY = srcH - 1;

        // Black bars on the left
        for (int32_t x = 0; x < x_start; ++x) {
            lineBuf[x] = TFT_BLACK;
        }

        // Active area
        for (int32_t x = x_start; x < x_end; ++x) {
            // virtX dans 0..virtW-1, proportional to x in x_start..x_end-1
            int virtX = ((x - x_start) * virtW) / outW;
            int srcX  = virtX - virtOff; // center
            lineBuf[x] = (unsigned)srcX < (unsigned)srcW
                ? myPalette[data[srcY][srcX]]
                : TFT_BLACK; // safety
        }

        // Black bars on the right
        for (int32_t x = x_end; x < frame_width; ++x) {
            lineBuf[x] = TFT_BLACK;
        }

        // Push a complete line
        M5.Lcd.pushPixels(lineBuf, frame_width);
    }

    M5.Lcd.endWrite();
}

static inline int clampi(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }

void display_write_frame_zoom(const uint8_t *data[]) {
    static uint16_t lineBuf[NES_SCREEN_WIDTH];

    M5.Lcd.startWrite();
    M5.Lcd.setWindow(frame_x, frame_y, frame_width - 1, frame_height - 1);

    const int srcW = NES_SCREEN_WIDTH;   // 256
    const int srcH = NES_SCREEN_HEIGHT;  // 240

    // Center
    uint32_t zp = (nesZoomPercent < 100) ? 100u : (uint32_t)nesZoomPercent;
    int roiW = (int)((uint32_t)srcW * 100u / zp);
    int roiH = (int)((uint32_t)srcH * 100u / zp);
    roiW = clampi(roiW, 16, srcW);
    roiH = clampi(roiH, 16, srcH);

    const int roiX0 = (srcW - roiW) / 2;
    const int roiY0 = (srcH - roiH) / 2;

    for (int32_t y = 0; y < frame_height; y++) {
        int srcY = roiY0 + (int)((int32_t)y * roiH / frame_height);
        srcY = clampi(srcY, 0, srcH - 1);

        for (int32_t x = 4; x < frame_width - 1; x++) {
            int srcX = roiX0 + (int)((int32_t)(x - 4) * roiW / (frame_width - 5));
            srcX = clampi(srcX, 0, srcW - 1);
            lineBuf[x] = myPalette[data[srcY][srcX]];
        }

        M5.Lcd.pushPixels(lineBuf, frame_width);
    }

    M5.Lcd.endWrite();
}

void display_write_frame(const uint8_t *data[]) {
    // this might not be the best spot for this
    if (nes_pausestate()) {
        CardputerInput input;
        CardputerView display;

        display.initialize();
        display.pauseScreen();

        input.waitPress();
        delay(100); // this is so it can't do a pause-unpause stutter
        nes_togglepause();
    }
    
    if (!fullscreenMode) {
        display_write_frame_square(data);
        return;
    }

    if (nesZoomPercent != 100) {
        display_write_frame_zoom(data);
        return;
    }

    static uint16_t lineBuf[NES_SCREEN_WIDTH];

    M5.Lcd.startWrite();
    M5.Lcd.setWindow(frame_x, frame_y, frame_width - 1, frame_height - 1);

    for (int32_t y = 0; y < frame_height; y++) {
        int srcY = (y * NES_SCREEN_HEIGHT) / frame_height; 

        // hack to avoid frame bleeding
        lineBuf[0] = BLACK;
        lineBuf[1] = BLACK;
        lineBuf[2] = BLACK;
        lineBuf[3] = BLACK;

        for (int32_t x = 4; x < frame_width - 1; x++) {
            int srcX = (x * NES_SCREEN_WIDTH) / frame_width - 4; // hack to avoid frame bleeding
            lineBuf[x] = myPalette[data[srcY][srcX]];
        }
        M5.Lcd.pushPixels(lineBuf, frame_width);
        lineBuf[frame_width - 2] = BLACK; // same
        lineBuf[frame_width - 1] = BLACK;
    }
    M5.Lcd.endWrite();
}

void display_clear() { 
    M5.Lcd.fillScreen(TFT_BLACK); 
}


}
