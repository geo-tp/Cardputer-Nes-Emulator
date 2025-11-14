#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void run_ws(const uint8_t* rom, size_t len, const char* rom_name, bool is_color);
#ifdef __cplusplus
}
#endif
