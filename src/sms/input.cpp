#include "input.h"

#include <algorithm>

// Helper
static inline bool key(char c) {
    return M5Cardputer.Keyboard.isKeyPressed(c);
}

void cardputer_input_init() {
    // nothing for now
}

void cardputer_read_input() {
    int smsButtons = 0; // -> input.pad[0]
    int smsSystem  = 0; // -> input.system

    M5Cardputer.update();
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    // ---------- Raccourcis ----------
    // Volume +
    if (key('=') || (status.fn && key(';'))) {
        int v = M5Cardputer.Speaker.getVolume();
        M5Cardputer.Speaker.setVolume(std::min(v + 5, 200));
        input.pad[0] = 0; input.system = 0;
        return;
    }
    // Volume -
    if (key('-') || (status.fn && key('.'))) {
        int v = M5Cardputer.Speaker.getVolume();
        M5Cardputer.Speaker.setVolume(std::max(v - 5, 0));
        input.pad[0] = 0; input.system = 0;
        return;
    }
    // Bright +
    if (key(']') || (status.fn && key('/'))) {
        int b = M5Cardputer.Display.getBrightness();
        M5Cardputer.Display.setBrightness(std::min(b + 10, 255));
        input.pad[0] = 0; input.system = 0;
        return;
    }
    // Bright -
    if (key('[') || (status.fn && key(','))) {
        int b = M5Cardputer.Display.getBrightness();
        M5Cardputer.Display.setBrightness(std::max(b - 10, 0));
        input.pad[0] = 0; input.system = 0;
        return;
    }

    if (M5Cardputer.Keyboard.isChange() && key('\\')) {
        fullscreen = !fullscreen;
        scanline   = fullscreen;   // full is scanlined by default

        input.pad[0] = 0;
        input.system = 0;
        return;
    }

    // ---------- Mapping  ----------
    if (key('a') || key(',')) smsButtons |= INPUT_LEFT;
    if (key('d') || key('/')) smsButtons |= INPUT_RIGHT;
    if (key('e') || key(';')) smsButtons |= INPUT_UP;
    if (key('s') || key('.')) smsButtons |= INPUT_DOWN;
    if (key('j') || key('l')) smsButtons |= INPUT_BUTTON1;
    if (key('k'))             smsButtons |= INPUT_BUTTON2;
    if (key('p')) smsSystem |= INPUT_PAUSE;
    if (key('1')) smsSystem |= INPUT_START;
    if (key('r')) smsSystem |= INPUT_SOFT_RESET;
    if (key('R')) smsSystem |= INPUT_HARD_RESET;

    input.pad[0]  = smsButtons;
    input.system  = smsSystem;
}
