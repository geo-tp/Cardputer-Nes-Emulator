#include "genesis_input.h"

#include <M5Cardputer.h>
#include <Arduino.h>

extern "C" {
  // Gwenesis APIs 
  void gwenesis_io_pad_press_button(int pad, int idx);
  void gwenesis_io_pad_release_button(int pad, int idx);
  void gwenesis_io_get_buttons(void);
}

// Shared state
extern bool fullscreenMode;
extern uint8_t genesis_audio_volume;
extern int  genesisZoomPercent;

// Buttons mapping
enum {
  BTN_UP = 0,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_C,
  BTN_B,
  BTN_A,
  BTN_START,
};

/* Button state management */
static inline void set_button(int idx, bool pressed) {
  if (pressed) gwenesis_io_pad_press_button(0, idx);
  else         gwenesis_io_pad_release_button(0, idx);
}

/* Polling cardputer keyboard */
extern "C" void genesis_controller_poll() {
  M5Cardputer.update();
  Keyboard_Class::KeysState ks = M5Cardputer.Keyboard.keysState();

  // Quit
  if (M5Cardputer.BtnA.pressedFor(1000)) {
    esp_sleep_enable_timer_wakeup(1000); // 1 ms
    esp_deep_sleep_start();
  }

  // Screen mode toggle with '\'
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isKeyPressed('\\')) {
    if (!fullscreenMode) {
      fullscreenMode = true;
      genesisZoomPercent = 100;
    } else {
      genesisZoomPercent += 10;
      if (genesisZoomPercent > 150) {
        genesisZoomPercent = 100;
        fullscreenMode = false;
      }
    }
    return;
  }

  // Zoom +/−
  if (ks.fn && M5Cardputer.Keyboard.isKeyPressed('/')) {
    if (!fullscreenMode) fullscreenMode = true;
    genesisZoomPercent += 1;
    if (genesisZoomPercent > 150) genesisZoomPercent = 150;
    return;
  }
  if (ks.fn && M5Cardputer.Keyboard.isKeyPressed(',')) {
    if (!fullscreenMode) fullscreenMode = true;
    genesisZoomPercent -= 1;
    if (genesisZoomPercent < 100) genesisZoomPercent = 100;
    return;
  }

  // Volume +/−
  if (M5Cardputer.Keyboard.isKeyPressed('=') || (ks.fn && M5Cardputer.Keyboard.isKeyPressed(';'))) {
    genesis_audio_volume = min(genesis_audio_volume + 1, 255);
    M5Cardputer.Speaker.setVolume(genesis_audio_volume);
    return;
  }
  if (M5Cardputer.Keyboard.isKeyPressed('-') || (ks.fn && M5Cardputer.Keyboard.isKeyPressed('.'))) {
    genesis_audio_volume = max(genesis_audio_volume - 1, 0);
    M5Cardputer.Speaker.setVolume(genesis_audio_volume);
    return;
  }

  // Luminosity +/−
  if (M5Cardputer.Keyboard.isKeyPressed(']')) {
    M5Cardputer.Display.setBrightness(min(M5Cardputer.Display.getBrightness() + 1, 255));
    return;
  }
  if (M5Cardputer.Keyboard.isKeyPressed('[') || (ks.fn && M5Cardputer.Keyboard.isKeyPressed(','))) {
    M5Cardputer.Display.setBrightness(max(M5Cardputer.Display.getBrightness() - 1, 0));
    return;
  }

  // Arrow keys: ZQSD / ,./
  const bool left  = M5Cardputer.Keyboard.isKeyPressed('a') || M5Cardputer.Keyboard.isKeyPressed(',');
  const bool right = M5Cardputer.Keyboard.isKeyPressed('d') || M5Cardputer.Keyboard.isKeyPressed('/');
  const bool up    = M5Cardputer.Keyboard.isKeyPressed('e') || M5Cardputer.Keyboard.isKeyPressed(';');
  const bool down  = M5Cardputer.Keyboard.isKeyPressed('s') || M5Cardputer.Keyboard.isKeyPressed('.')  || M5Cardputer.Keyboard.isKeyPressed('z');

  // Buttons A/B/C to j/k/l
  const bool btnA     = M5Cardputer.Keyboard.isKeyPressed('j'); // A
  const bool btnB     = M5Cardputer.Keyboard.isKeyPressed('k'); // B
  const bool btnC     = M5Cardputer.Keyboard.isKeyPressed('l'); // C
  const bool btnStart = M5Cardputer.Keyboard.isKeyPressed('1'); // Start

  // Apply state to the 8 buttons
  set_button(BTN_UP,    up);
  set_button(BTN_DOWN,  down);
  set_button(BTN_LEFT,  left);
  set_button(BTN_RIGHT, right);
  set_button(BTN_A,     btnA);
  set_button(BTN_B,     btnB);
  set_button(BTN_C,     btnC);
  set_button(BTN_START, btnStart);
}

/* Called by Gwenesis to poll button states */
extern "C" void gwenesis_io_get_buttons(void) {
  genesis_controller_poll();
}
