extern "C" {
#include <nes/nes.h>
}

#include <M5Unified.h>

int16_t frame_scaling;
static int16_t frame_x, frame_y, frame_x_offset, frame_y_offset, frame_width, frame_height, frame_line_pixels;
extern uint16_t myPalette[];

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

void display_write_frame(const uint8_t *data[]) {
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

        for (int32_t x = 4; x < frame_width; x++) {
            int srcX = (x * NES_SCREEN_WIDTH) / frame_width - 4; // hack to avoid frame bleeding
            lineBuf[x] = myPalette[data[srcY][srcX]];
        }
        M5.Lcd.pushPixels(lineBuf, frame_width);
    }
    M5.Lcd.endWrite();
}

void display_clear() { M5.Lcd.fillScreen(TFT_BLACK); }
}