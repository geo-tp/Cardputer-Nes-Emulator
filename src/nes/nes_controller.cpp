#include <M5Cardputer.h>

extern "C" {

void controller_init() {
  // nothing to do
}

uint32_t controller_read_input() {
  uint32_t value = 0xFFFFFFFF;
  auto &kb = M5Cardputer.Keyboard;

  M5Cardputer.update();
        
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
    if (M5Cardputer.Keyboard.isKeyPressed('s') || M5Cardputer.Keyboard.isKeyPressed('.')) {
        value ^= (1 << 1); // down
    }

    if (M5Cardputer.Keyboard.isKeyPressed('2')) {
        value ^= (1 << 4); // select
    }

    if (M5Cardputer.Keyboard.isKeyPressed('1')) {
        value ^= (1 << 5); // start
    }

    if (M5Cardputer.Keyboard.isKeyPressed('l')) {
        value ^= (1 << 6); // A
    }
    if (M5Cardputer.Keyboard.isKeyPressed('k')) {
        value ^= (1 << 7); // B
    }

    // Sound and brightness control
    if (M5Cardputer.Keyboard.isKeyPressed('=')) {
        M5Cardputer.Speaker.setVolume(min(M5Cardputer.Speaker.getVolume() + 5, 200)); // volume up
    }
    if (M5Cardputer.Keyboard.isKeyPressed('-')) {
        M5Cardputer.Speaker.setVolume(max(M5Cardputer.Speaker.getVolume() - 5, 0)); // volume down
    }
    if (M5Cardputer.Keyboard.isKeyPressed(']')) {
        M5Cardputer.Display.setBrightness(min(M5Cardputer.Display.getBrightness() + 10, 255)); // brightness up
    }
    if (M5Cardputer.Keyboard.isKeyPressed('[')) {
        M5Cardputer.Display.setBrightness(max(M5Cardputer.Display.getBrightness() - 10, 0)); // brightness down
    }

  return value;
}

} // extern "C"
