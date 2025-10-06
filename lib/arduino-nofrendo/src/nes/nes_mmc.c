/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** nes_mmc.c
**
** NES Memory Management Controller (mapper) emulation
** $Id: nes_mmc.c,v 1.2 2001/04/27 14:37:11 neil Exp $
*/

#include <string.h>

#include "../noftypes.h"
#include "../cpu/nes6502.h"
#include "nes_mmc.h"
#include "nes_ppu.h"
#include "../libsnss/libsnss.h"
#include "../log.h"
#include "mmclist.h"
#include "nes_rom.h"

#define MMC_8KROM (mmc.cart->rom_banks * 2)
#define MMC_16KROM (mmc.cart->rom_banks)
#define MMC_32KROM (mmc.cart->rom_banks / 2)
#define MMC_8KVROM (mmc.cart->vrom_banks)
#define MMC_4KVROM (mmc.cart->vrom_banks * 2)
#define MMC_2KVROM (mmc.cart->vrom_banks * 4)
#define MMC_1KVROM (mmc.cart->vrom_banks * 8)

#define MMC_LAST8KROM (MMC_8KROM - 1)
#define MMC_LAST16KROM (MMC_16KROM - 1)
#define MMC_LAST32KROM (MMC_32KROM - 1)
#define MMC_LAST8KVROM (MMC_8KVROM - 1)
#define MMC_LAST4KVROM (MMC_4KVROM - 1)
#define MMC_LAST2KVROM (MMC_2KVROM - 1)
#define MMC_LAST1KVROM (MMC_1KVROM - 1)

#include "nes_rom.h"

/* PRG 16 KiB */
static uint8 prg_win16_a[0x4000]; /* 0x4000 */
static uint8 prg_win16_b[0x4000]; /* 0x4000 */
static int   prg16_bank_a = -1;
static int   prg16_bank_b = -1;

/* CHR 8 KiB window for PPU ($0000-$1FFF) */
static uint8 chr_win8k[0x2000];  /* 0x2000 */
static int   chr8_bank = -1;

/* PRG 8 KiB windows — one buffer per 8K slot */
static uint8 prg_win8_8000[0x2000];
static uint8 prg_win8_A000[0x2000];
static uint8 prg_win8_C000[0x2000];
static uint8 prg_win8_E000[0x2000];
static int   prg8_bank_8000 = -1;
static int   prg8_bank_A000 = -1;
static int   prg8_bank_C000 = -1;
static int   prg8_bank_E000 = -1;

/* CHR 4 KiB windows for PPU ($0000-$0FFF and $1000-$1FFF) */
static uint8 chr_win4k_a[0x1000];
static uint8 chr_win4k_b[0x1000];
static int   chr4_bank_a = -1;
static int   chr4_bank_b = -1;

static mmc_t mmc;

rominfo_t *mmc_getinfo(void)
{
   return mmc.cart;
}

void mmc_setcontext(mmc_t *src_mmc)
{
   ASSERT(src_mmc);

   mmc = *src_mmc;
}

void mmc_getcontext(mmc_t *dest_mmc)
{
   *dest_mmc = mmc;
}

/* VROM bankswitching */
void mmc_bankvrom(int size, uint32 address, int bank)
{
   if (0 == mmc.cart->vrom_banks)
      return;

   if (mmc.cart->streaming) {
      /* --- Buffer composite --- */
      static uint8 chr_comp8k[0x2000];
      static int   chr_initialized = 0;

      /* Taille totale*/
      const size_t chr_total_bytes = (size_t)mmc.cart->vrom_banks * 0x2000;

      /* safe reading */
      #define READ_CHR_SAFE(src_off, dst, len)                                         \
         do {                                                                          \
            if ((src_off) + (len) > chr_total_bytes) {                                 \
               /* hors bornes  */                                                      \
               memset((dst), 0x00, (len));                                             \
               nofrendo_log_printf("WARN CHR OOB req off=0x%zx len=0x%zx tot=0x%zx\n", \
                                    (size_t)(src_off), (size_t)(len), chr_total_bytes);\
            } else if (!rom_read_chr_at(mmc.cart, (src_off), (dst), (len))) {          \
               memset((dst), 0x00, (len));                                             \
               nofrendo_log_printf("WARN CHR read fail off=0x%zx len=0x%zx\n",         \
                                    (size_t)(src_off), (size_t)(len));                 \
            }                                                                          \
         } while (0)

      /* Init  buffer composite */
      if (!chr_initialized) {
         /* charge banque 0 full */
         READ_CHR_SAFE(0, chr_comp8k, 0x2000);
         chr_initialized = 1;
         ppu_setpage(8, 0, chr_comp8k);
      }

      switch (size)
      {
      case 8: { /* CHR 8K ($0000-$1FFF) */
         int eff = (bank == MMC_LASTBANK) ? MMC_LAST8KVROM : (bank % MMC_8KVROM);
         size_t src_off = (size_t)eff << 13; /* * 0x2000 */
         READ_CHR_SAFE(src_off, chr_comp8k, 0x2000);
         ppu_setpage(8, 0, chr_comp8k);

         nofrendo_log_printf(
         "[CHR8 ] map @%04X req=%d eff=%d src=0x%06lX\n",
         (unsigned)address, bank, eff, (unsigned long)src_off
         );
         return;
      }

      case 4: { /* CHR 4K ($0000 ou $1000) */
         /* slot A = $0000..$0FFF, slot B = $1000..$1FFF */

         if (bank == MMC_LASTBANK)
               bank = MMC_LAST4KVROM;
         int eff = bank % MMC_4KVROM;

         uint8 *dst;
         int   *cur_bank;
         int slot = (address >= 0x1000) ? 1 : 0;
         if (slot) { dst = chr_win4k_b; cur_bank = &chr4_bank_b; }
         else      { dst = chr_win4k_a; cur_bank = &chr4_bank_a; }

         if (*cur_bank != eff) {
               size_t off = (size_t)eff << 12; /* eff * 0x1000 */
               if (!rom_read_chr_at(mmc.cart, off, dst, 0x1000)) {
                  memset(dst, 0x00, 0x1000);
                  nofrendo_log_printf("WARN mmc_bankvrom(4): read CHR4K bank %d failed -> 0x00\n", eff);
               }
               *cur_bank = eff;
         }

         ppu_setpage(4, address >> 10, dst - (int)address);
         break;
      }

      case 2: { /* CHR 2K (MMC3) */
         int eff = (bank == MMC_LASTBANK) ? MMC_LAST2KVROM : (bank % MMC_2KVROM);
         size_t dst_off = (size_t)(address & 0x1FFF);
         dst_off &= ~0x7FF;                    /* align 2K (0x800) */
         if (dst_off > 0x1800) dst_off = 0x1800;
         size_t src_off = (size_t)eff << 11;   /* * 0x800 */
         READ_CHR_SAFE(src_off, chr_comp8k + dst_off, 0x800);
         ppu_setpage(8, 0, chr_comp8k);

         nofrendo_log_printf(
         "[CHR2 ] map @%04X req=%d eff=%d dst=0x%04X src=0x%06lX\n",
         (unsigned)address, bank, eff, (unsigned)dst_off, (unsigned long)src_off
         );
         return;
      }

      case 1: { /* CHR 1K (MMC3) */
         int eff = (bank == MMC_LASTBANK) ? MMC_LAST1KVROM : (bank % MMC_1KVROM);
         size_t dst_off = (size_t)(address & 0x1FFF);
         dst_off &= ~0x3FF;                    /* align 1K (0x400) */
         if (dst_off > 0x1C00) dst_off = 0x1C00;
         size_t src_off = (size_t)eff << 10;   /* * 0x400 */
         READ_CHR_SAFE(src_off, chr_comp8k + dst_off, 0x400);
         ppu_setpage(8, 0, chr_comp8k);

         nofrendo_log_printf(
         "[CHR1 ] map @%04X req=%d eff=%d dst=0x%04X src=0x%06lX\n",
         (unsigned)address, bank, eff, (unsigned)dst_off, (unsigned long)src_off
         );
         return;
      }

      default:
         nofrendo_log_printf("invalid VROM bank size %d (stream)\n", size);
         return;
      }
   }
}

/* ROM bankswitching */
void mmc_bankrom(int size, uint32 address, int bank)
{
    nes6502_context mmc_cpu;
    nes6502_getcontext(&mmc_cpu);

    static int   prg16_bank_a = -1;
    static int   prg16_bank_b = -1;

    static uint8 prg_win8[0x2000];
    static int   prg8_bank = -1;

    if (mmc.cart && mmc.cart->streaming)
    {
        const int have16 = (mmc.cart->rom_banks > 0);
        const int have32 = (mmc.cart->rom_banks >= 2);

        switch (size)
        {
        case 16:
        {
            if (!have16) {
                memset(prg_win16_a, 0xFF, sizeof(prg_win16_a));
                int page = address >> NES6502_BANKSHIFT;
                mmc_cpu.mem_page[page + 0] = prg_win16_a + 0x0000;
                mmc_cpu.mem_page[page + 1] = prg_win16_a + 0x1000;
                mmc_cpu.mem_page[page + 2] = prg_win16_a + 0x2000;
                mmc_cpu.mem_page[page + 3] = prg_win16_a + 0x3000;
                nofrendo_log_printf("mmc_bankrom(16): no PRG available, mapped 0xFF stub\n");
                break;
            }

            if (bank == MMC_LASTBANK) bank = MMC_LAST16KROM;

            int page = address >> NES6502_BANKSHIFT;
            uint8 *dst;
            int   *cur_bank;

            if (address >= 0xC000) { dst = prg_win16_b; cur_bank = &prg16_bank_b; }
            else                   { dst = prg_win16_a; cur_bank = &prg16_bank_a; }

            int eff = bank % MMC_16KROM;
            if (*cur_bank != eff) {
                if (!rom_read_prg_bank(mmc.cart, eff, dst)) {
                    memset(dst, 0xFF, 0x4000);
                    nofrendo_log_printf("WARN mmc_bankrom(16): read PRG bank %d failed -> 0xFF\n", eff);
                }
                *cur_bank = eff;
            }

            mmc_cpu.mem_page[page + 0] = dst + 0x0000;
            mmc_cpu.mem_page[page + 1] = dst + 0x1000;
            mmc_cpu.mem_page[page + 2] = dst + 0x2000;
            mmc_cpu.mem_page[page + 3] = dst + 0x3000;
            break;
        }

        case 32:
        {
            if (!have32) {
                if (have16) {
                    if (prg16_bank_a != 0) {
                        if (!rom_read_prg_bank(mmc.cart, 0, prg_win16_a))
                            memset(prg_win16_a, 0xFF, sizeof(prg_win16_a));
                        prg16_bank_a = 0;
                    }
                    memcpy(prg_win16_b, prg_win16_a, 0x4000);
                    prg16_bank_b = 1;

                    mmc_cpu.mem_page[8]  = prg_win16_a + 0x0000;
                    mmc_cpu.mem_page[9]  = prg_win16_a + 0x1000;
                    mmc_cpu.mem_page[10] = prg_win16_a + 0x2000;
                    mmc_cpu.mem_page[11] = prg_win16_a + 0x3000;

                    mmc_cpu.mem_page[12] = prg_win16_b + 0x0000;
                    mmc_cpu.mem_page[13] = prg_win16_b + 0x1000;
                    mmc_cpu.mem_page[14] = prg_win16_b + 0x2000;
                    mmc_cpu.mem_page[15] = prg_win16_b + 0x3000;
                    nofrendo_log_printf("mmc_bankrom(32): fallback single 16K\n");
                    break;
                }

                memset(prg_win16_a, 0xFF, sizeof(prg_win16_a));
                memset(prg_win16_b, 0xFF, sizeof(prg_win16_b));
                mmc_cpu.mem_page[8]  = prg_win16_a + 0x0000;
                mmc_cpu.mem_page[9]  = prg_win16_a + 0x1000;
                mmc_cpu.mem_page[10] = prg_win16_a + 0x2000;
                mmc_cpu.mem_page[11] = prg_win16_a + 0x3000;
                mmc_cpu.mem_page[12] = prg_win16_b + 0x0000;
                mmc_cpu.mem_page[13] = prg_win16_b + 0x1000;
                mmc_cpu.mem_page[14] = prg_win16_b + 0x2000;
                mmc_cpu.mem_page[15] = prg_win16_b + 0x3000;
                nofrendo_log_printf("mmc_bankrom(32): no PRG available, mapped 0xFF stub\n");
                break;
            }

            if (bank == MMC_LASTBANK) bank = MMC_LAST32KROM;
            int base_bank16 = (bank % MMC_32KROM) * 2;

            if (prg16_bank_a != base_bank16) {
                if (!rom_read_prg_bank(mmc.cart, base_bank16, prg_win16_a)) {
                    memset(prg_win16_a, 0xFF, sizeof(prg_win16_a));
                    nofrendo_log_printf("WARN mmc_bankrom(32): read PRG bank %d failed -> 0xFF\n", base_bank16);
                }
                prg16_bank_a = base_bank16;
            }
            if (prg16_bank_b != base_bank16 + 1) {
                if (!rom_read_prg_bank(mmc.cart, base_bank16 + 1, prg_win16_b)) {
                    memset(prg_win16_b, 0xFF, sizeof(prg_win16_b));
                    nofrendo_log_printf("WARN mmc_bankrom(32): read PRG bank %d failed -> 0xFF\n", base_bank16 + 1);
                }
                prg16_bank_b = base_bank16 + 1;
            }

            mmc_cpu.mem_page[8]  = prg_win16_a + 0x0000;
            mmc_cpu.mem_page[9]  = prg_win16_a + 0x1000;
            mmc_cpu.mem_page[10] = prg_win16_a + 0x2000;
            mmc_cpu.mem_page[11] = prg_win16_a + 0x3000;
            mmc_cpu.mem_page[12] = prg_win16_b + 0x0000;
            mmc_cpu.mem_page[13] = prg_win16_b + 0x1000;
            mmc_cpu.mem_page[14] = prg_win16_b + 0x2000;
            mmc_cpu.mem_page[15] = prg_win16_b + 0x3000;
            break;
        }

         case 8:
         {
            if (bank == MMC_LASTBANK) bank = MMC_LAST8KROM;
            if (MMC_8KROM <= 0) {
               static uint8 stub8[0x2000];
               int page = (int)(address >> NES6502_BANKSHIFT);
               memset(stub8, 0xFF, sizeof(stub8));
               mmc_cpu.mem_page[page+0] = stub8 + 0x0000;
               mmc_cpu.mem_page[page+1] = stub8 + 0x1000;
               break;
            }

            int eff = bank % MMC_8KROM;
            uint8 *buf; int *cur;

            if      (address >= 0xE000) { buf = prg_win8_E000; cur = &prg8_bank_E000; }
            else if (address >= 0xC000) { buf = prg_win8_C000; cur = &prg8_bank_C000; }
            else if (address >= 0xA000) { buf = prg_win8_A000; cur = &prg8_bank_A000; }
            else                        { buf = prg_win8_8000; cur = &prg8_bank_8000; }

            if (*cur != eff) {
               if (!rom_read_prg8k(mmc.cart, eff, buf)) {
                     memset(buf, 0xFF, 0x2000);
                     nofrendo_log_printf("WARN PRG8 read bank %d fail -> 0xFF\n", eff);
               }
               *cur = eff;
            }

            int page = (int)(address >> NES6502_BANKSHIFT);
            mmc_cpu.mem_page[page+0] = buf + 0x0000;
            mmc_cpu.mem_page[page+1] = buf + 0x1000;

            nofrendo_log_printf(
            "[PRG8] map @%04X slot=%c req=%d eff=%d page=%d\n",
            (unsigned)address,
            (address >= 0xE000) ? 'E' : (address >= 0xC000) ? 'C' : (address >= 0xA000) ? 'A' : '8',
            bank,
            eff,
            (int)(address >> NES6502_BANKSHIFT)
            );
            break;
         }

        default:
            nofrendo_log_printf("invalid ROM bank size %d\n", size);
            break;
        }

        nes6502_setcontext(&mmc_cpu);
        return;
    }
}

/* Check to see if this mapper is supported */
bool mmc_peek(int map_num)
{
   mapintf_t **map_ptr = mappers;

   while (NULL != *map_ptr)
   {
      if ((*map_ptr)->number == map_num)
         return true;
      map_ptr++;
   }

   return false;
}

static void mmc_setpages(void)
{
   nofrendo_log_printf("setting up mapper %d\n", mmc.intf->number);

   /* Switch ROM into CPU space, set VROM/VRAM (done for ALL ROMs) */
   mmc_bankrom(16, 0x8000, 0);
   mmc_bankrom(16, 0xC000, MMC_LASTBANK);
   mmc_bankvrom(8, 0x0000, 0);

   if (mmc.cart->flags & ROM_FLAG_FOURSCREEN)
   {
      ppu_mirror(0, 1, 2, 3);
   }
   else
   {
      if (MIRROR_VERT == mmc.cart->mirror)
         ppu_mirror(0, 1, 0, 1);
      else
         ppu_mirror(0, 0, 1, 1);
   }

   /* if we have no VROM, switch in VRAM */
   /* TODO: fix this hack implementation */
   if (0 == mmc.cart->vrom_banks)
   {
      ASSERT(mmc.cart->vram);

      ppu_setpage(8, 0, mmc.cart->vram);
      ppu_mirrorhipages();
   }
}

/* Mapper initialization routine */
void mmc_reset(void)
{
   mmc_setpages();

   ppu_setlatchfunc(NULL);
   ppu_setvromswitch(NULL);

   if (mmc.intf->init)
      mmc.intf->init();

   nofrendo_log_printf("reset memory mapper\n");
}

void mmc_destroy(mmc_t **nes_mmc)
{
   if (*nes_mmc)
      NOFRENDO_FREE(*nes_mmc);
}

mmc_t *mmc_create(rominfo_t *rominfo)
{
   mmc_t *temp;
   mapintf_t **map_ptr;

   for (map_ptr = mappers; (*map_ptr)->number != rominfo->mapper_number; map_ptr++)
   {
      if (NULL == *map_ptr)
         return NULL; /* Should *never* happen */
   }

   temp = NOFRENDO_MALLOC(sizeof(mmc_t));
   if (NULL == temp)
      return NULL;

   memset(temp, 0, sizeof(mmc_t));

   temp->intf = *map_ptr;
   temp->cart = rominfo;

   mmc_setcontext(temp);

   nofrendo_log_printf("created memory mapper: %s\n", (*map_ptr)->name);

   return temp;
}

/*
** $Log: nes_mmc.c,v $
** Revision 1.2  2001/04/27 14:37:11  neil
** wheeee
**
** Revision 1.1.1.1  2001/04/27 07:03:54  neil
** initial
**
** Revision 1.4  2000/11/21 13:28:40  matt
** take care to zero allocated mem
**
** Revision 1.3  2000/10/27 12:55:58  matt
** nes6502 now uses 4kB banks across the boards
**
** Revision 1.2  2000/10/25 00:23:16  matt
** makefiles updated for new directory structure
**
** Revision 1.1  2000/10/24 12:20:28  matt
** changed directory structure
**
** Revision 1.28  2000/10/22 19:17:24  matt
** mapper cleanups galore
**
** Revision 1.27  2000/10/22 15:02:32  matt
** simplified mirroring
**
** Revision 1.26  2000/10/21 19:38:56  matt
** that two year old crap code *was* flushed
**
** Revision 1.25  2000/10/21 19:26:59  matt
** many more cleanups
**
** Revision 1.24  2000/10/17 03:22:57  matt
** cleaning up rom module
**
** Revision 1.23  2000/10/10 13:58:15  matt
** stroustrup squeezing his way in the door
**
** Revision 1.22  2000/08/16 02:51:55  matt
** random cleanups
**
** Revision 1.21  2000/07/31 04:27:59  matt
** one million cleanups
**
** Revision 1.20  2000/07/25 02:25:53  matt
** safer xxx_destroy calls
**
** Revision 1.19  2000/07/23 15:11:45  matt
** removed unused variables
**
** Revision 1.18  2000/07/15 23:50:03  matt
** migrated state get/set from nes_mmc.c to state.c
**
** Revision 1.17  2000/07/11 03:15:09  melanson
** Added support for mappers 16, 34, and 231
**
** Revision 1.16  2000/07/10 05:27:41  matt
** cleaned up mapper-specific callbacks
**
** Revision 1.15  2000/07/10 03:02:49  matt
** minor change on loading state
**
** Revision 1.14  2000/07/06 17:38:49  matt
** replaced missing string.h include
**
** Revision 1.13  2000/07/06 02:47:11  matt
** mapper addition madness
**
** Revision 1.12  2000/07/05 05:04:15  matt
** added more mappers
**
** Revision 1.11  2000/07/04 23:12:58  matt
** brand spankin' new mapper interface implemented
**
** Revision 1.10  2000/07/04 04:56:36  matt
** modifications for new SNSS
**
** Revision 1.9  2000/06/29 14:17:18  matt
** uses snsslib now
**
** Revision 1.8  2000/06/29 03:09:24  matt
** modified to support new snss code
**
** Revision 1.7  2000/06/26 04:57:54  matt
** bugfix - irqs/mmcstate not cleared on reset
**
** Revision 1.6  2000/06/23 11:01:10  matt
** updated for new external sound interface
**
** Revision 1.5  2000/06/20 04:04:57  matt
** hacked to use new external soundchip struct
**
** Revision 1.4  2000/06/09 15:12:26  matt
** initial revision
**
*/
