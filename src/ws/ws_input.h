#pragma once
#include <stdint.h>

// Bit layout core WS
enum {
  WS_Y1     = 1 << 0,
  WS_Y2     = 1 << 1,
  WS_Y3     = 1 << 2,
  WS_Y4     = 1 << 3,
  WS_X1     = 1 << 4,
  WS_X2     = 1 << 5,
  WS_X3     = 1 << 6,
  WS_X4     = 1 << 7,
  WS_OPTION = 1 << 8,
  WS_START  = 1 << 9,
  WS_A      = 1 << 10,
  WS_B      = 1 << 11,
};

#ifdef __cplusplus
extern "C" {
#endif
int ws_input_poll(int mode);
#ifdef __cplusplus
}
#endif
