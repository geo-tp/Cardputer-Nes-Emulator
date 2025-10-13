#pragma once
#include <stdint.h>

extern bool fullscreen;
extern bool scanline;

#ifdef __cplusplus
extern "C" {
  #include "sms/smsplus/shared.h"
  #include "sms/smsplus/vdp.h"
}
#endif

void sms_palette_init_fixed();
void video_compute_scaler_full();
void video_compute_scaler_square();
void sms_display_write_frame();
void sms_display_clear();
