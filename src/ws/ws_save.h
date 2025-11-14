#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ws_save_init(const char* romPathOrName);
void ws_save_load(void);
void ws_save_tick(void);
void ws_save_request_flush(void);
void ws_save_force_flush(void);

#ifdef __cplusplus
}
#endif
