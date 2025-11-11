// ngc_scheduler.cpp
#include "ngc_scheduler.h"
#include "ngc_sound.h"
#include "ngc_input.h"
#include "ngc_display.h"
#include <M5Cardputer.h>
#include <Arduino.h>

// Periodic task intervals
static const TickType_t kInputPeriodTicks = pdMS_TO_TICKS(1);
static const TickType_t kAudioPeriodTicks = pdMS_TO_TICKS(8);

static TaskHandle_t s_taskInput  = nullptr;
static TaskHandle_t s_taskAudio  = nullptr;
static TaskHandle_t s_taskVideo  = nullptr;
static volatile bool s_running   = false;

extern volatile unsigned g_frame_ready;

#ifndef NGC_INPUT_CORE
#define NGC_INPUT_CORE  0
#endif
#ifndef NGC_AUDIO_CORE
#define NGC_AUDIO_CORE  0
#endif
#ifndef NGC_VIDEO_CORE
#define NGC_VIDEO_CORE  0
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

static void taskVideo(void*){
  while (s_running) {
    if (g_frame_ready) {
      g_frame_ready = 0;
      graphics_paint(1);
    } else {
      vTaskDelay(1);
    }
  }
}

extern "C" void ngc_scheduler_start(void)
{
  if (s_running) return;
  s_running = true;

  //   Handled in main loop now
  //   xTaskCreatePinnedToCore(taskInput, "ngp_input", 4096, nullptr, 5, &s_taskInput, NGC_INPUT_CORE);
  xTaskCreatePinnedToCore(taskAudio, "ngp_audio", 2048, nullptr, 6, &s_taskAudio, NGC_AUDIO_CORE);
  xTaskCreatePinnedToCore(taskVideo, "ngp_video", 2048, nullptr, 6, &s_taskVideo, NGC_VIDEO_CORE);
}

extern "C" void ngc_scheduler_stop(void)
{
  if (!s_running) return;
  s_running = false;
}
