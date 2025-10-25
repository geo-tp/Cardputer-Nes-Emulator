#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
void run_ngp(const uint8_t* rom_base, size_t rom_size, int machine);
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern unsigned short* drawBuffer;
#ifdef __cplusplus
}
#endif
