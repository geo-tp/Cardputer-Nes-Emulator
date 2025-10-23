#include <Arduino.h> 
#include <string.h>
#include "run_ngp.h"
#include "../race/race-memory.h"
#include "../race/types.h"
#include "../race/tlcs900h.h"
#include "../race/input.h"
#include "../race/flash.h"
#include <M5Cardputer.h>
#include "ngc_sound.h"
#include "ngc_input.h"
#include "ngc_display.h"
#include "ngc_scheduler.h"

#define NGP_LANG_EN 1
#define NGP_LANG    NGP_LANG_EN  // 0 = JP, 1 = EN
char retro_save_directory[2048] = "/sd/ngp_saves";
int tipo_consola = 0;

extern "C" {

int m_bIsActive = 1;
int gfx_hacks = 0;

extern int finscan;
extern "C" unsigned int gen_regsPC;
#define tlcsPc gen_regsPC
extern EMUINFO m_emuInfo;
extern int tipo_consola;
__attribute__((weak)) void tlcs_execute(int cycles) { (void)cycles; }
__attribute__((weak)) void tlcs_reset(void) {}

unsigned char *rasterY = 0;
unsigned char  *frame0Pri = 0;
unsigned char  *frame1Pri = 0;
unsigned char  *color_switch = 0;
unsigned char *scanlineY = 0;
unsigned char *scrollSpriteX = 0;
unsigned char *scrollSpriteY = 0;
unsigned char *sprite_palette_numbers = 0;
unsigned char *sprite_table = 0;
unsigned short *patterns = 0;
unsigned char *oowSelect = 0;
unsigned short *oowTable = 0;
unsigned char *wndTopLeftY = 0;
unsigned char *wndSizeY = 0;
unsigned char *wndSizeX = 0;
unsigned char *bgSelect = 0;
unsigned char *bw_palette_table = 0;
unsigned short *palette_table = 0;
unsigned short *bgTable = 0;
unsigned char *wndTopLeftX = 0;
unsigned char *scrollFrontY = 0;
unsigned char *scrollFrontX = 0;
unsigned short *tile_table_front = 0;
unsigned char *scrollBackY = 0;
unsigned short *tile_table_back = 0;
unsigned char *scrollBackX = 0;
unsigned char  *pattern_table = NULL;

void ngpSoundOff(void);
void ngpSoundExecute(void) __attribute__((weak));
void ngpSoundExecute(void) {}
void audio_dac_init(void);

}

static void set_defaults_after_boot(void)
{
  // Couleur/mono selon ROM
  tlcsMemWriteB(0x00006F91, tlcsMemReadB(0x00200023));
  if (tipo_consola == 1) tlcsMemWriteB(0x00006F91, 0x00); // forcer mono

  // Langue ROM
  tlcsMemWriteB(0x00006F87, (NGP_LANG == NGP_LANG_EN) ? 0x01 : 0x00);

  // IRQ video
  tlcsMemWriteB(0x00004000, tlcsMemReadB(0x00004000) | 0xC0);

  // Bits Power
  tlcsMemWriteB(0x00006F84, 0x40);
  tlcsMemWriteB(0x00006F85, 0x00);
  tlcsMemWriteB(0x00006F86, 0x00);
}

static void map_vdp_tables_full()
{
  sprite_table           = (unsigned char*)  get_address(0x00008800);
  pattern_table          = (unsigned char*)  get_address(0x0000A000);
  patterns               = (unsigned short*) pattern_table;
  tile_table_front       = (unsigned short*) get_address(0x00009000);
  tile_table_back        = (unsigned short*) get_address(0x00009800);
  palette_table          = (unsigned short*) get_address(0x00008200);
  bw_palette_table       = (unsigned char*)  get_address(0x00008100);
  sprite_palette_numbers = (unsigned char*)  get_address(0x00008C00);

  scanlineY              = (unsigned char*)  get_address(0x00008009);
  frame0Pri              = (unsigned char*)  get_address(0x00008000);
  frame1Pri              = (unsigned char*)  get_address(0x00008030);

  wndTopLeftX            = (unsigned char*)  get_address(0x00008002);
  wndTopLeftY            = (unsigned char*)  get_address(0x00008003);
  wndSizeX               = (unsigned char*)  get_address(0x00008004);
  wndSizeY               = (unsigned char*)  get_address(0x00008005);

  scrollSpriteX          = (unsigned char*)  get_address(0x00008020);
  scrollSpriteY          = (unsigned char*)  get_address(0x00008021);
  scrollFrontX           = (unsigned char*)  get_address(0x00008032);
  scrollFrontY           = (unsigned char*)  get_address(0x00008033);
  scrollBackX            = (unsigned char*)  get_address(0x00008034);
  scrollBackY            = (unsigned char*)  get_address(0x00008035);

  bgSelect               = (unsigned char*)  get_address(0x00008118);
  bgTable                = (unsigned short*) get_address(0x000083E0);
  oowSelect              = (unsigned char*)  get_address(0x00008012);
  oowTable               = (unsigned short*) get_address(0x000083F0);

  color_switch           = (unsigned char*)  get_address(0x00006F91);

  static unsigned char s_dummy_scan = 0;
  if (!scanlineY) scanlineY = &s_dummy_scan;
  rasterY = scanlineY;
}

void run_ngp(const uint8_t* rom_base, size_t rom_size, int machine)
{
  // Load ROM
  ngp_mem_set_rom(rom_base, rom_size);

  // Sys info
  m_emuInfo.machine = machine;
  m_emuInfo.romSize = (int)rom_size;
  tipo_consola      = 0;      // 0 = NGPC, 1 = NGP (mono)

  // Core init
  ngp_mem_init();

  // Map VRAM/regs
  map_vdp_tables_full();

  // default BG palette + enable
  if (bgTable)   bgTable[0] = 0xFFFF;
  if (bgSelect) *bgSelect |= 0x80;  // enable bgTable[index]

  // CPU / Z80 / son
  tlcs_init();
  tlcs_reset();
  Z80_Init(); 
  Z80_Reset();

  // Flags post boot
  set_defaults_after_boot();

  // Init video
  ngc_display_init();
  graphics_init();

  // Fullscreen si vide
  if (wndTopLeftX && wndTopLeftY && wndSizeX && wndSizeY) {
    if (*wndSizeX == 0 || *wndSizeY == 0) {
      *wndTopLeftX = 0; *wndTopLeftY = 0;
      *wndSizeX    = 160; *wndSizeY  = 152;
    }
  }
  if (bgSelect) *bgSelect |= 0x80;

  // LCD ON + priorité frame0
  if (frame0Pri) *frame0Pri |= 0x80 | 0x40;

  // Master IE (VBlank/HBlank)
  tlcsMemWriteB(0x00004000, tlcsMemReadB(0x00004000) | 0xC0);

  // Fin ecran selon rom
  finscan = 198;
  if (mainrom[0x000020] == 0x65 || mainrom[0x000020] == 0x93) finscan = 199;

  // Kludges ROM
  switch (tlcsMemReadW(0x00200020)) {
    case 0x0059:   // Sonic
    case 0x0061:   // Metal Slug 2nd
        tlcsMemWriteB(0x0020001F, 0xFF);
      break;
  }

  audio_dac_init();
  ngc_sound_init();
  ngc_sound_frame();
  ngc_sound_frame();
  ngc_input_init();
  ngc_scheduler_start();

  // Go
  printf("[NGPC_RUN] entering ngpc_run() ... m_bIsActive=%d\n", m_bIsActive);
  printf("[FORCE] Enabling LCD + VBLANK IRQ @ 0x4000 = 0xC0\n");
  tlcsMemWriteB(0x00004000, 0xC0);   // bit 7 = LCD ON, bit 6 = VBlank IRQ enable
  m_bIsActive = 1;

  printf("[NGPC_RUN] starting core loop\n");

  unsigned long status_last = millis();
  unsigned long frames = 0;

  while (m_bIsActive)
  {
      // Execute one frame (1/60s)
      tlcs_execute((6 * 1024 * 1024) / 60);
      ngc_input_poll();
      taskYIELD();
      
      // // Log framerate
      // frames++;
      // if (millis() - status_last >= 2000)
      // {
      //     printf("[NGPC_RUN] %lu frames / 2s (~%lu FPS)\n",
      //             frames, frames / 2);

      //     frames = 0;
      //     status_last = millis();
      // }
  }
}
