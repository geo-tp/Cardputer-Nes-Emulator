#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
  #include "sms/smsplus/shared.h"
  #include "sms/smsplus/vdp.h"
}
#endif

void run_sms(const uint8_t* romPtr, size_t romLen, bool isGG);
