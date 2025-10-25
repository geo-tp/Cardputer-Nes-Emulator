/* Flavor modified sound.c and sound.h from NEOPOP
 *  which was originally based on sn76496.c from MAME
 *  some ideas also taken from NeoPop-SDL code
 */

/*---------------------------------------------------------------------------
 * Originally from
 * NEOPOP : Emulator as in Dreamland
 *
 * Copyright (c) 2001-2002 by neopop_uk
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version. See also the license.txt file for
 *	additional informations.
 *---------------------------------------------------------------------------
 */

#ifndef __NEOPOPSOUND__
#define __NEOPOPSOUND__

#include "types.h"

typedef struct
{
	int LastRegister;	/* last register written */
	int Register[8];	/* registers */
	int Volume[4];
	int Period[4];
	int Count[4];
	int Output[4];

	unsigned int RNG;	/* noise generator      */
	int NoiseFB;		/* noise feedback mask */

} SoundChip;

extern SoundChip toneChip;
extern SoundChip noiseChip;

#ifdef __cplusplus
extern "C" {
#endif

void WriteSoundChip(SoundChip* chip, _u8 data);

int sound_system_init(void);
BOOL system_sound_init(void);
void system_sound_chipreset(void);

void system_VBL(void);

#define Write_SoundChipTone(VALUE)		(WriteSoundChip(&toneChip, VALUE))
#define Write_SoundChipNoise(VALUE)		(WriteSoundChip(&noiseChip, VALUE))

void sound_init(int SampleRate);

void dac_update(_u16* dac_buffer, int length_bytes);
void sound_update(_u16* chip_buffer, int length_bytes);

void dac_writeL(unsigned char a);
void dac_write(unsigned char a);

#ifdef __cplusplus
}
#endif

#endif
