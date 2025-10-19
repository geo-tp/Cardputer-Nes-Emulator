#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void sms_save_init(const char* romName, uint8_t* sramPtr, size_t sramLen);
void sms_save_load(void);
void sms_save_tick(void);
void sms_save_force_flush(void);

#ifdef __cplusplus
}
#endif
