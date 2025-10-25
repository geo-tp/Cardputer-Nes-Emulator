extern "C" {
#include <nes/nes.h>
}

#include <M5Cardputer.h>

extern bool fullscreenMode;
extern int nesZoomPercent;

extern "C" {

void controller_init() {
  // nothing to do
}

uint32_t controller_read_input() {
    uint32_t value = 0xFFFFFFFF;

    if (nes_pausestate()) return value; // block input while paused

    M5Cardputer.update();
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    if (M5Cardputer.BtnA.pressedFor(1000)) {
        // use as a hack to quit the game
        esp_sleep_enable_timer_wakeup(1000); // 1ms
        esp_deep_sleep_start();    
    }

    // Zoom control and screen mode toggle
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isKeyPressed('\\')) {
        if (!fullscreenMode) {
            fullscreenMode = true;
            nesZoomPercent = 100;
        } else {
            nesZoomPercent += 10;
            if (nesZoomPercent > 150) {
                nesZoomPercent = 100;
                fullscreenMode = false;
            }
        }
        return value;
    }

    // Zoom in / out
    if (status.fn && M5Cardputer.Keyboard.isKeyPressed('/')) {
        if (!fullscreenMode) fullscreenMode = true;
        nesZoomPercent+= 1;
        if (nesZoomPercent > 150) nesZoomPercent = 150;
        return value;

    }

    if (status.fn && M5Cardputer.Keyboard.isKeyPressed(',')) {
        if (!fullscreenMode) fullscreenMode = true;
        nesZoomPercent-= 1;
        if (nesZoomPercent < 100) nesZoomPercent = 100;
        return value;
    }

    // Volume up
    if (M5Cardputer.Keyboard.isKeyPressed('=') || (status.fn && M5Cardputer.Keyboard.isKeyPressed(';'))) {
        M5Cardputer.Speaker.setVolume(min(M5Cardputer.Speaker.getVolume() + 3, 255)); // volume up
        return value;
    }

    // Volume down
    if (M5Cardputer.Keyboard.isKeyPressed('-') || (status.fn && M5Cardputer.Keyboard.isKeyPressed('.'))) {
        M5Cardputer.Speaker.setVolume(max(M5Cardputer.Speaker.getVolume() - 3, 0)); // volume down
        return value;
    }

    // Brightness control
    if (M5Cardputer.Keyboard.isKeyPressed(']')) {
        M5Cardputer.Display.setBrightness(min(M5Cardputer.Display.getBrightness() + 2, 255)); // brightness up
        return value;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('[') || (status.fn && M5Cardputer.Keyboard.isKeyPressed(','))) {
        M5Cardputer.Display.setBrightness(max(M5Cardputer.Display.getBrightness() - 2, 0)); // brightness down
        return value;
    }
 
    // Arrows and buttons
    if (M5Cardputer.Keyboard.isKeyPressed('a') || M5Cardputer.Keyboard.isKeyPressed(',')) {
        value ^= (1 << 2); // left
    }
    if (M5Cardputer.Keyboard.isKeyPressed('d') || M5Cardputer.Keyboard.isKeyPressed('/')) {
        value ^= (1 << 3); // right
    }

    if (M5Cardputer.Keyboard.isKeyPressed('e') || M5Cardputer.Keyboard.isKeyPressed(';')) {
        value ^= (1 << 0); // up
    }
    if (M5Cardputer.Keyboard.isKeyPressed('s') || M5Cardputer.Keyboard.isKeyPressed('.') || M5Cardputer.Keyboard.isKeyPressed('z')) {
        value ^= (1 << 1); // down
    }

    if (M5Cardputer.Keyboard.isKeyPressed('2')) {
        value ^= (1 << 4); // select
    }

    if (M5Cardputer.Keyboard.isKeyPressed('1')) {
        value ^= (1 << 5); // start
    }

    if (M5Cardputer.Keyboard.isKeyPressed('l') || M5Cardputer.Keyboard.isKeyPressed('j')) {
        value ^= (1 << 6); // A
    }
  
    if (M5Cardputer.Keyboard.isKeyPressed('k')) {
        value ^= (1 << 7); // B
    }
  
    if (M5Cardputer.Keyboard.isKeyPressed(' ')) {
        value ^= (1 << 10); // pause
    }

    return value;
}

} // extern "C"
