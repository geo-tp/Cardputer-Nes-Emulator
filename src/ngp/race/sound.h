/*---------------------------------------------------------------------------
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version. See also the license.txt file for
 *	additional informations.
 *---------------------------------------------------------------------------
 */

/* sound.h: interface for the sound class. */

#ifndef AFX_SOUND_H
#define AFX_SOUND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* General sound system functions */
void soundStep(int cycles);

#define NUM_CHANNELS 32

/* Neogeo pocket sound functions */
void ngpSoundStart(void);
void ngpSoundExecute(void);
void ngpSoundOff(void);
void ngpSoundInterrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* !defined(AFX_SOUND_H) */
