#include "save.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 261
#endif

#define CHECK_MS  500
#define GAP_MS    5000

static uint8_t* g_sram = NULL;
static size_t   g_sram_len = 0;
static char*    g_save_path = nullptr;
static uint32_t g_crc_last = 0;
static TickType_t g_next_check = 0;
static TickType_t g_next_allowed_write = 0;
static TaskHandle_t g_save_task = nullptr;
static uint8_t* g_sram_shadow = nullptr;
static volatile bool g_flush_req = false;     // ask for flush
static volatile bool g_check_req = false;     // ask for check (CRC)

static void ensure_dir(void){ 
  mkdir("/sd/sms_saves", 0777); 
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *buf, size_t len){
  static uint32_t* T = nullptr;
  static int init = 0;

  if (!T) {
    T = (uint32_t*)malloc(256 * sizeof(uint32_t));
    if (!T) abort();
  }

  if(!init){ for(uint32_t i=0;i<256;i++){ uint32_t c=i; for(int k=0;k<8;k++) c=(c&1)?(0xEDB88320^(c>>1)):(c>>1); T[i]=c; } init=1; }
  crc^=0xFFFFFFFFU; for(size_t i=0;i<len;i++) crc=T[(crc^buf[i])&0xFF]^ (crc>>8); return crc^0xFFFFFFFFU;
}

static int sram_is_trivial(const uint8_t* p, size_t n) {
  if (!p || n == 0) return 1;
  // Test 0xFF
  size_t i = 0;
  for (; i < n; ++i) { if (p[i] != 0xFF) break; }
  if (i == n) return 1;

  // Test 0x00
  for (i = 0; i < n; ++i) { if (p[i] != 0x00) break; }
  if (i == n) return 1;

  return 0; // not trivial
}

static void make_save_path_from_name(char* dst, size_t dstlen, const char* romName){
  char name[128];
  strncpy(name, romName ? romName : "rom.sms", sizeof(name)-1);
  name[sizeof(name)-1]='\0';

  size_t L = strlen(name);
  if (L >= 4) { name[L-3]='s'; name[L-2]='a'; name[L-1]='v'; }
  else { strncat(name, ".sav", sizeof(name)-strlen(name)-1); }

  ensure_dir();
  snprintf(dst, dstlen, "/sd/sms_saves/%s", name);
  dst[dstlen-1]='\0';
}

static void flush_now(void){
  if (!g_sram || !g_sram_len) return;

  if (sram_is_trivial(g_sram, g_sram_len)) {
    printf("SMS save: skip trivial SRAM, no write.\n");
    return;
  }

  const uint8_t* src = g_sram;
  if (g_sram_shadow) {
    memcpy(g_sram_shadow, g_sram, g_sram_len);
    src = g_sram_shadow;
  }

  ensure_dir();

  char tmp_path[PATH_MAX];
  snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", g_save_path);
  tmp_path[sizeof(tmp_path)-1] = '\0';

  // Write to temp file
  FILE* f = fopen(tmp_path, "wb");
  if (!f) {
    printf("SMS save: fopen tmp fail %s\n", tmp_path);
    return;
  }
  setvbuf(f, NULL, _IONBF, 0);

  size_t w = fwrite(src, 1, g_sram_len, f);
  fflush(f);
  fsync(fileno(f));
  fclose(f);

  if (w != g_sram_len) {
    printf("SMS save: short write %u/%u to %s\n",
           (unsigned)w, (unsigned)g_sram_len, tmp_path);
    return;
  }

  // Unlink + rename
  unlink(g_save_path);
  if (rename(tmp_path, g_save_path) != 0) {
    printf("SMS save: rename failed %s -> %s\n", tmp_path, g_save_path);
    return;
  }

  printf("SMS save: wrote %u/%u -> %s \n",
         (unsigned)w, (unsigned)g_sram_len, g_save_path);
}

static void SaveTask(void*){
  for(;;){
    // sleep until notified
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(CHECK_MS));

    bool do_check = g_check_req; g_check_req = false;
    bool do_flush = g_flush_req; g_flush_req = false;

    TickType_t now = xTaskGetTickCount();

    if (do_check) {
      uint32_t crc = crc32_update(0, g_sram, g_sram_len);
      if (crc != g_crc_last && now >= g_next_allowed_write){
        g_crc_last = crc;
        if (!sram_is_trivial(g_sram, g_sram_len)) {
          do_flush = true;
        } else {
          printf("SMS save: trivial after change, skip write.\n");
        }
      }
    }

    // flush if requested and allowed
    if (do_flush && now >= g_next_allowed_write) {
      flush_now();
      g_next_allowed_write = xTaskGetTickCount() + pdMS_TO_TICKS(GAP_MS);
    }
  }
}

void sms_save_init(const char* romName, uint8_t* sramPtr, size_t sramLen){
  if (!g_save_path) {
    g_save_path = (char*)malloc(PATH_MAX);
    if (!g_save_path) abort();
  }
  g_sram = sramPtr;
  g_sram_len = sramLen;
  make_save_path_from_name(g_save_path, PATH_MAX, romName);
  g_crc_last = (g_sram && g_sram_len) ? crc32_update(0, g_sram, g_sram_len) : 0;
  g_next_check = g_next_allowed_write = 0;

  // snapshot SRAM
  if (!g_sram_shadow && g_sram_len) {
    g_sram_shadow = (uint8_t*)malloc(g_sram_len);
    if (!g_sram_shadow) {
      printf("SMS save: no shadow buffer, will write live.\n");
    }
  }

  // launch save task
  if (!g_save_task) {
    xTaskCreatePinnedToCore(SaveTask, "SaveTask", 4096, nullptr, 2, &g_save_task, 0);
  }
}

void sms_save_load(void){
  if (!g_sram || !g_sram_len) return;
  memset(g_sram, 0xFF, g_sram_len);

  FILE* f = fopen(g_save_path, "rb");
  if (!f){ printf("SMS load: no save, %s\n", g_save_path); return; }
  size_t n = fread(g_sram, 1, g_sram_len, f);
  fclose(f);
  if (n < g_sram_len) memset(g_sram+n, 0xFF, g_sram_len-n);
  g_crc_last = crc32_update(0, g_sram, g_sram_len);
  printf("SMS load: read %u/%u from %s\n",(unsigned)n,(unsigned)g_sram_len,g_save_path);
}

void sms_save_tick(void){
  if (!g_sram || !g_sram_len) return;
  TickType_t now = xTaskGetTickCount();
  if (now < g_next_check) return;
  g_next_check = now + pdMS_TO_TICKS(CHECK_MS);

  g_check_req = true;
  if (g_save_task) xTaskNotifyGive(g_save_task);
}

void sms_save_force_flush(void){
  flush_now();
}