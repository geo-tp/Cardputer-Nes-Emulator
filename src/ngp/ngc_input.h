#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ngc_input_init(void);
uint32_t ngc_input_poll(void);

#ifdef __cplusplus
}
#endif
