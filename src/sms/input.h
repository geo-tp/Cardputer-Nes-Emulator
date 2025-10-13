#pragma once

#ifndef INPUT_UP
#define INPUT_UP       0x01
#endif
#ifndef INPUT_DOWN
#define INPUT_DOWN     0x02
#endif
#ifndef INPUT_LEFT
#define INPUT_LEFT     0x04
#endif
#ifndef INPUT_RIGHT
#define INPUT_RIGHT    0x08
#endif
#ifndef INPUT_BUTTON1
#define INPUT_BUTTON1  0x10
#endif
#ifndef INPUT_BUTTON2
#define INPUT_BUTTON2  0x20
#endif
#ifndef INPUT_START
#define INPUT_START    0x02
#endif
#ifndef INPUT_PAUSE
#define INPUT_PAUSE    0x01
#endif
#ifndef INPUT_SOFT_RESET
#define INPUT_SOFT_RESET 0x04
#endif
#ifndef INPUT_HARD_RESET
#define INPUT_HARD_RESET 0x08
#endif

extern "C" {
    #include "sms/smsplus/shared.h"
}

#include <M5Cardputer.h>
#include "display.h"  // fullscreen/scanline bool

extern bool fullscreen;
extern bool scanline;

void cardputer_input_init();
void cardputer_read_input();
