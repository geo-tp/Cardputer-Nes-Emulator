#include "ws_input.h"
#include <M5Cardputer.h>
#include <stdint.h>
#include "ws_input.h"

extern bool ws_fullscreen;
extern int  ws_zoomPercent;

extern "C" int ws_input_poll(int mode)
{
  M5Cardputer.update();
  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
  uint16_t state = 0;

  // Quit
  if (M5Cardputer.BtnA.pressedFor(1000)) {
    esp_sleep_enable_timer_wakeup(1000);
    esp_deep_sleep_start();
  }

  // Directional pad
  const bool left  = M5Cardputer.Keyboard.isKeyPressed('e');
  const bool right = M5Cardputer.Keyboard.isKeyPressed('z') || M5Cardputer.Keyboard.isKeyPressed('s');
  const bool up    = M5Cardputer.Keyboard.isKeyPressed('d');
  const bool down  = M5Cardputer.Keyboard.isKeyPressed('a');

  // mode == 0 : horizontal
  if (mode == 0) {
    if (left)  state |= WS_X1;
    if (up)    state |= WS_X2;
    if (right) state |= WS_X3;
    if (down)  state |= WS_X4;
  // mode == 1 : vertical 
  } else {
    if (left)  state |= WS_Y1;
    if (up)    state |= WS_Y2;
    if (right) state |= WS_Y3;
    if (down)  state |= WS_Y4;
  }

  // Secondary directional pad 
  const bool a = M5Cardputer.Keyboard.isKeyPressed(';');
  const bool w = M5Cardputer.Keyboard.isKeyPressed('.');
  const bool d = M5Cardputer.Keyboard.isKeyPressed('/');
  const bool s = M5Cardputer.Keyboard.isKeyPressed(',');

  if (mode == 0) {
    // In horizontal mode, WASD -> Y cross
    if (a) state |= WS_Y1;
    if (w) state |= WS_Y2;
    if (d) state |= WS_Y3;
    if (s) state |= WS_Y4;
  } else {
    // In vertical mode, WASD -> X cross
    if (a) state |= WS_X1;
    if (w || M5Cardputer.Keyboard.isKeyPressed('k')) state |= WS_X2;
    if (d || M5Cardputer.Keyboard.isKeyPressed('l')) state |= WS_X3;
    if (s) state |= WS_X4;
  }

  //  Boutons
  if (M5Cardputer.Keyboard.isKeyPressed('l') && mode == 0) state |= WS_A;       // A
  if (M5Cardputer.Keyboard.isKeyPressed('k') && mode == 0) state |= WS_B;       // B

  if (M5Cardputer.Keyboard.isKeyPressed('1')) state |= WS_START;  // START 
  if (M5Cardputer.Keyboard.isKeyPressed('2'))  state |= WS_OPTION; // OPTION 

  // Volume +/-
  if (M5Cardputer.Keyboard.isKeyPressed('='))
    M5Cardputer.Speaker.setVolume(min(M5Cardputer.Speaker.getVolume() + 3, 255));
  if (M5Cardputer.Keyboard.isKeyPressed('-'))
    M5Cardputer.Speaker.setVolume(max(M5Cardputer.Speaker.getVolume() - 3, 0));

  // Luminosity +/-
  if (M5Cardputer.Keyboard.isKeyPressed(']'))
    M5Cardputer.Display.setBrightness(min(M5Cardputer.Display.getBrightness() + 2, 255));
  if (M5Cardputer.Keyboard.isKeyPressed('['))
    M5Cardputer.Display.setBrightness(max(M5Cardputer.Display.getBrightness() - 2, 0));

  // Zoom / fullscreen toggle
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isKeyPressed('\\')) {
    if (!ws_fullscreen) {
      ws_fullscreen  = true;
      ws_zoomPercent = 100;
    } else {
      ws_zoomPercent += 10;
      if (ws_zoomPercent > 150) {
        ws_zoomPercent = 100;
        ws_fullscreen  = false;
      }
    }
    return (int)state;
  }

  if (status.fn && M5Cardputer.Keyboard.isKeyPressed('/')) {
    ws_zoomPercent = std::min(150, ws_zoomPercent + 1);
    return (int)state;
  }

  if (status.fn && M5Cardputer.Keyboard.isKeyPressed(',')) {
    ws_zoomPercent = std::max(100, ws_zoomPercent - 1);
    return (int)state;
  }

  return (int)state;
}