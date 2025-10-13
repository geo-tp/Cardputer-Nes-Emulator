#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
  #include "sms/smsplus/shared.h" // t_snd snd
}
#endif

void sms_audio_init();
void sms_audio_stop();
void sms_audio_frame();
void sms_audio_start_task();   // task FreeRTOS
void sms_audio_stop_task();
