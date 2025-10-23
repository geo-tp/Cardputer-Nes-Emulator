// ngc_scheduler.cpp
#include "ngc_scheduler.h"
#include "ngc_sound.h"
#include "ngc_input.h"
#include <M5Cardputer.h>
#include <Arduino.h>

// Periodic task intervals
static const TickType_t kInputPeriodTicks = pdMS_TO_TICKS(1);
static const TickType_t kAudioPeriodTicks = pdMS_TO_TICKS(8);

static TaskHandle_t s_taskInput  = nullptr;
static TaskHandle_t s_taskAudio  = nullptr;
static volatile bool s_running   = false;

#ifndef NGC_INPUT_CORE
#define NGC_INPUT_CORE  0
#endif
#ifndef NGC_AUDIO_CORE
#define NGC_AUDIO_CORE  0
#endif

static void taskInput(void* arg)
{
  (void)arg;
  TickType_t last = xTaskGetTickCount();
  while (s_running) {
    M5.update();
    ngc_input_poll();
    taskYIELD();
  }
  vTaskDelete(nullptr);
}


static void taskAudio(void* arg)
{
  (void)arg;
  TickType_t last = xTaskGetTickCount();
  while (s_running) {
    ngc_sound_frame();
    vTaskDelayUntil(&last, kAudioPeriodTicks);
  }
  vTaskDelete(nullptr);
}

extern "C" void ngc_scheduler_start(void)
{
  if (s_running) return;
  s_running = true;

  //   Handled in main loop now
  //   xTaskCreatePinnedToCore(taskInput, "ngp_input", 4096, nullptr, 5, &s_taskInput, NGC_INPUT_CORE);
  xTaskCreatePinnedToCore(taskAudio, "ngp_audio", 4096, nullptr, 3, &s_taskAudio, NGC_AUDIO_CORE);
}

extern "C" void ngc_scheduler_stop(void)
{
  if (!s_running) return;
  s_running = false;
}
