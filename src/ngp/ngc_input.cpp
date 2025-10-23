#include "ngc_input.h"
#include <M5Cardputer.h>
#include <Arduino.h>
#include "race/input.h"

#ifndef NGP_INPUT_ACTIVE_LOW
#define NGP_INPUT_ACTIVE_LOW 0
#endif

#define NGP_BTN_UP       (1u << 0)
#define NGP_BTN_DOWN     (1u << 1)
#define NGP_BTN_LEFT     (1u << 2)
#define NGP_BTN_RIGHT    (1u << 3)
#define NGP_BTN_A        (1u << 4)
#define NGP_BTN_B        (1u << 5)
#define NGP_BTN_OPTION   (1u << 6)

extern bool ngpFullscreen;
extern int  ngpZoomPercent;
static bool prevBs = false;
static uint32_t lastToggle = 0;

extern "C" void ngc_input_init(void) {
  ngpInputState = 0;
}

extern "C" uint32_t ngc_input_poll(void) {
  uint32_t dummy_ret = 0xFFFFFFFF;

  M5Cardputer.update();
  Keyboard_Class::KeysState ks = M5Cardputer.Keyboard.keysState();

  if (M5Cardputer.BtnA.pressedFor(1000)) {
    esp_sleep_enable_timer_wakeup(1000);
    esp_deep_sleep_start();
  }

  // --- state de base ---
#if NGP_INPUT_ACTIVE_LOW
  ngpInputState = 0xFF;
#else
  ngpInputState = 0x00;
#endif

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isKeyPressed('\\')) {
    ngpFullscreen = !ngpFullscreen;
    M5Cardputer.Display.fillScreen(TFT_BLACK);
  }

  // -------- DIRECTIONS --------
  if (M5Cardputer.Keyboard.isKeyPressed('a') || M5Cardputer.Keyboard.isKeyPressed(',')) {
#if NGP_INPUT_ACTIVE_LOW
    ngpInputState &= ~NGP_BTN_LEFT;
#else
    ngpInputState |=  NGP_BTN_LEFT;
#endif
  }
  if (M5Cardputer.Keyboard.isKeyPressed('d') || M5Cardputer.Keyboard.isKeyPressed('/')) {
#if NGP_INPUT_ACTIVE_LOW
    ngpInputState &= ~NGP_BTN_RIGHT;
#else
    ngpInputState |=  NGP_BTN_RIGHT;
#endif
  }
  if (M5Cardputer.Keyboard.isKeyPressed('e') || M5Cardputer.Keyboard.isKeyPressed(';')) {
#if NGP_INPUT_ACTIVE_LOW
    ngpInputState &= ~NGP_BTN_UP;
#else
    ngpInputState |=  NGP_BTN_UP;
#endif
  }
  if (M5Cardputer.Keyboard.isKeyPressed('s') ||
      M5Cardputer.Keyboard.isKeyPressed('.') ||
      M5Cardputer.Keyboard.isKeyPressed('z')) {
#if NGP_INPUT_ACTIVE_LOW
    ngpInputState &= ~NGP_BTN_DOWN;
#else
    ngpInputState |=  NGP_BTN_DOWN;
#endif
  }

  // -------- BOUTONS --------
  if (M5Cardputer.Keyboard.isKeyPressed('l') || M5Cardputer.Keyboard.isKeyPressed('j')) {
#if NGP_INPUT_ACTIVE_LOW
    ngpInputState &= ~NGP_BTN_A;
#else
    ngpInputState |=  NGP_BTN_A;
#endif
  }
  if (M5Cardputer.Keyboard.isKeyPressed('k')) {
#if NGP_INPUT_ACTIVE_LOW
    ngpInputState &= ~NGP_BTN_B;
#else
    ngpInputState |=  NGP_BTN_B;
#endif
  }
  if (M5Cardputer.Keyboard.isKeyPressed('1') || (ks.fn && M5Cardputer.Keyboard.isKeyPressed(' '))) {
#if NGP_INPUT_ACTIVE_LOW
    ngpInputState &= ~NGP_BTN_OPTION;
#else
    ngpInputState |=  NGP_BTN_OPTION;
#endif
  }

  // Zoom
  if (ks.fn && M5Cardputer.Keyboard.isKeyPressed('/')) {
    if (!ngpFullscreen) ngpFullscreen = true;
    ngpZoomPercent = (ngpZoomPercent < 150) ? (ngpZoomPercent + 1) : 150;
    return dummy_ret;
  }
  if (ks.fn && M5Cardputer.Keyboard.isKeyPressed(',')) {
    if (!ngpFullscreen) ngpFullscreen = true;
    ngpZoomPercent = (ngpZoomPercent > 100) ? (ngpZoomPercent - 1) : 100;
    return dummy_ret;
  }

  // Volume
  if (M5Cardputer.Keyboard.isKeyPressed('=') || (ks.fn && M5Cardputer.Keyboard.isKeyPressed(';'))) {
    M5Cardputer.Speaker.setVolume(min(M5Cardputer.Speaker.getVolume() + 3, 255));
    return dummy_ret;
  }
  if (M5Cardputer.Keyboard.isKeyPressed('-') || (ks.fn && M5Cardputer.Keyboard.isKeyPressed('.'))) {
    M5Cardputer.Speaker.setVolume(max(M5Cardputer.Speaker.getVolume() - 3, 0));
    return dummy_ret;
  }

  // Luminosity
  if (M5Cardputer.Keyboard.isKeyPressed(']')) {
    M5Cardputer.Display.setBrightness(min(M5Cardputer.Display.getBrightness() + 2, 255));
    return dummy_ret;
  }
  if (M5Cardputer.Keyboard.isKeyPressed('[') || (ks.fn && M5Cardputer.Keyboard.isKeyPressed(','))) {
    M5Cardputer.Display.setBrightness(max(M5Cardputer.Display.getBrightness() - 2, 0));
    return dummy_ret;
  }

  return ngpInputState;
}
