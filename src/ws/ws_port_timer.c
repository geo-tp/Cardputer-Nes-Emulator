#include "esp_timer.h"

// Implementation of timer for the oswan core

unsigned long SDL_UXTimerRead(void) {
  // microseconds since boot
  return (unsigned long) esp_timer_get_time();
}