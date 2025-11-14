#include "ws_save.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
// Core WS 
extern int RAMSize;
extern int RAMBanks;
extern unsigned char** RAMMap;
extern int CartKind;
}

#ifndef CK_EEP
#define CK_EEP 0x01
#endif

#define WS_SAVE_DIR "/sd/ws_saves"
#define CHECK_MS    500
#define GAP_MS      5000

static uint8_t*     g_sram        = nullptr;   // pointer vers RAMMap
static size_t       g_sram_len    = 0;         // = RAMSize
static char*        g_save_path   = nullptr;
static uint32_t     g_crc_last    = 0;
static TickType_t   g_next_check  = 0;
static TickType_t   g_next_allow  = 0;
static TaskHandle_t g_task        = nullptr;
static volatile bool g_flag_flush = false;
static volatile bool g_flag_check = false;

// ====================== CRC32 ======================
static uint32_t* s_crc32_tbl = NULL;  // 256 * 4
static int       s_crc32_ok  = 0;    

static void crc32_init_once(void) {
  if (s_crc32_ok != 0) return; 
  uint32_t* T = (uint32_t*)malloc(256 * sizeof(uint32_t));
  if (!T) { s_crc32_ok = -1; return; }

  for (uint32_t i = 0; i < 256; ++i) {
    uint32_t c = i;
    for (int k = 0; k < 8; ++k)
      c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
    T[i] = c;
  }
  s_crc32_tbl = T;
  s_crc32_ok  = 1;
}

static uint32_t crc32_update(uint32_t crc, const uint8_t* b, size_t n) {
  if (s_crc32_ok == 0) crc32_init_once();

  crc ^= 0xFFFFFFFFu;

  if (s_crc32_ok > 0 && s_crc32_tbl) {
    const uint32_t* T = s_crc32_tbl;
    for (size_t i = 0; i < n; ++i)
      crc = T[(crc ^ b[i]) & 0xFFu] ^ (crc >> 8);
  } else {
    // Fallback bitwise (si malloc a échoué)
    for (size_t i = 0; i < n; ++i) {
      uint32_t c = crc ^ b[i];
      for (int k = 0; k < 8; ++k)
        c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
      crc = c ^ (crc >> 8);
    }
  }

  return crc ^ 0xFFFFFFFFu;
}

// ====================== FS utils ======================
static void ensure_dir(){ mkdir(WS_SAVE_DIR, 0777); }

static const char* basename_ptr(const char* p){
  if (!p) return nullptr;
  const char* s = strrchr(p, '/'); if (s) return s+1;
  s = strrchr(p, '\\'); if (s) return s+1;
  return p;
}

static void sanitize_filename(char* s){
  size_t w=0;
  for(size_t i=0; s[i] && w<64; ++i){
    char c=s[i];
    bool ok=(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-';
    s[w++]= ok?c:'_';
  }
  s[w]='\0';
}

static void make_save_path(const char* romPathOrName){
  ensure_dir();

  const char* base = basename_ptr(romPathOrName);
  char name[160] = {0};

  if (base && *base){
    strncpy(name, base, sizeof(name)-1);
    size_t L = strlen(name);
    if (L >= 4) { name[L-3]='s'; name[L-2]='a'; name[L-1]='v'; }
    else strncat(name, ".sav", sizeof(name)-strlen(name)-1);
  } else {
    strcpy(name, "ws_autosave.sav");
  }
  sanitize_filename(name);

  int n = snprintf(g_save_path, PATH_MAX, WS_SAVE_DIR "/%s", name);
  if (n < 0 || (size_t)n >= PATH_MAX) {
    g_save_path[PATH_MAX-1] = '\0';
  }
}

static bool path_parent_ready(const char* fullpath){
  if (!fullpath) return false;
  struct stat st;
  if (stat("/sd", &st)!=0) return false;
  if (stat(WS_SAVE_DIR, &st)!=0){
    if (mkdir(WS_SAVE_DIR, 0777)!=0) return false;
  }
  return true;
}

// 0 sauvegarder, 1 trivial
static int sram_is_trivial(const uint8_t* p, size_t n){
  if (!p || n==0) return 1;
  size_t i=0; for(;i<n;i++) if(p[i]!=0xFF) break; if(i==n) return 1;
  for(i=0;i<n;i++) if(p[i]!=0x00) break; if(i==n) return 1;
  return 0;
}

// ====================== I/O ======================
static void flush_now(){
  if (!g_sram || !g_sram_len) return;
  if (!path_parent_ready(g_save_path)) { printf("[WS][SAVE] skip flush (storage not ready)\n"); return; }
  if (sram_is_trivial(g_sram, g_sram_len)) { printf("[WS][SAVE] trivial SRAM/EEP, skip write\n"); return; }

  char tmp[PATH_MAX]; snprintf(tmp, sizeof(tmp), "%s.tmp", g_save_path); tmp[sizeof(tmp)-1]='\0';

  FILE* f = fopen(tmp, "wb");
  if (!f){ printf("[WS][SAVE] fopen tmp fail %s\n", tmp); return; }
  setvbuf(f, NULL, _IONBF, 0);

  size_t w = fwrite(g_sram, 1, g_sram_len, f);
  fflush(f); fsync(fileno(f)); fclose(f);
  if (w != g_sram_len){ printf("[WS][SAVE] short write %u/%u -> %s\n",(unsigned)w,(unsigned)g_sram_len,tmp); return; }

  unlink(g_save_path);
  if (rename(tmp, g_save_path)!=0){ printf("[WS][SAVE] rename failed %s -> %s\n", tmp, g_save_path); return; }
  printf("[WS][SAVE] wrote %u bytes -> %s\n", (unsigned)w, g_save_path);
}

static void SaveTask(void*){
  for(;;){
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(CHECK_MS));
    bool do_check = g_flag_check; g_flag_check=false;
    bool do_flush = g_flag_flush; g_flag_flush=false;
    TickType_t now = xTaskGetTickCount();

    if (do_check && g_sram && g_sram_len){
      uint32_t crc = crc32_update(0, g_sram, g_sram_len); // CRC zone utile
      if (crc != g_crc_last && now >= g_next_allow){
        g_crc_last = crc;
        if (!sram_is_trivial(g_sram, g_sram_len)) do_flush = true;
        else printf("[WS][SAVE] trivial after change, skip\n");
      }
    }
    if (do_flush && now >= g_next_allow){
      flush_now();
      g_next_allow = xTaskGetTickCount() + pdMS_TO_TICKS(GAP_MS);
    }
  }
}

// ====================== API ======================
void ws_save_init(const char* romPathOrName){
  bool is_eep  = (CartKind & CK_EEP) != 0;
  bool ok_size = is_eep ? (RAMSize == 0x80 || RAMSize == 0x400 || RAMSize == 0x800)
                        : (RAMSize > 0 && RAMSize <= 0x8000); // max 32KB SRAM

  if (!ok_size || RAMBanks < 1 || !RAMMap || !RAMMap[0]){
    printf("[WS][SAVE] ignored (size=%d, banks=%d, kind=%s)\n",
           RAMSize, RAMBanks, is_eep ? "EEP" : "SRAM");
    g_sram=nullptr; g_sram_len=0; return;
  }

  g_save_path = (char*)malloc(PATH_MAX);
  if (!g_save_path){
    printf("[WS][SAVE] ignored (OOM on path alloc)\n");
    g_sram=nullptr; g_sram_len=0; return;
  }
  g_save_path[0]='\0';
  g_sram     = RAMMap[0];
  g_sram_len = (size_t)RAMSize;

  make_save_path(romPathOrName);
  g_crc_last = crc32_update(0, g_sram, g_sram_len);
  g_next_check = g_next_allow = 0;

  if (!g_task){
    xTaskCreatePinnedToCore(SaveTask, "WS_SaveTask", 3072, nullptr, 6, &g_task, 0);
  }
  printf("[WS][SAVE] path=%s len=%u (%s)\n",
         g_save_path, (unsigned)g_sram_len, is_eep ? "EEP" : "SRAM");
}

void ws_save_load(void){
  if (!g_sram || !g_sram_len) return;
  if (!path_parent_ready(g_save_path)) {
    printf("[WS][SAVE] skip load (storage not ready)\n");
    return;
  }

  memset(g_sram, 0xFF, g_sram_len);

  FILE* f = fopen(g_save_path, "rb");
  if (!f){
    // .sav manquant
    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", g_save_path);

    // Tenter le rename
    if (rename(tmp_path, g_save_path) == 0) {
      printf("[WS][SAVE] promoted temp -> sav: %s\n", g_save_path);
      f = fopen(g_save_path, "rb"); // rouvre le .sav
    } else {
      // lire directement le .tmp
      f = fopen(tmp_path, "rb");
      if (f) {
        printf("[WS][SAVE] loading from temp (rename failed)\n");
      } else {
        printf("[WS][SAVE] no save and no temp: %s\n", g_save_path);
        return;
      }
    }
  }

  const size_t CHUNK = 512;
  size_t remaining = g_sram_len;
  uint8_t* dst = g_sram;

  while (remaining > 0) {
    size_t want = remaining < CHUNK ? remaining : CHUNK;
    size_t got  = fread(dst, 1, want, f);
    if (got == 0) break;
    dst       += got;
    remaining -= got;
    taskYIELD();
  }
  fclose(f);

  if (remaining) memset(dst, 0xFF, remaining);

  g_crc_last = crc32_update(0, g_sram, g_sram_len);
  printf("[WS][SAVE] loaded %u/%u from %s\n",
         (unsigned)(g_sram_len - remaining), (unsigned)g_sram_len, g_save_path);
}

void ws_save_tick(void){
  if (!g_sram || !g_sram_len) return;
  TickType_t now = xTaskGetTickCount();
  if (now < g_next_check) return;
  g_next_check = now + pdMS_TO_TICKS(CHECK_MS);
  g_flag_check = true;
  if (g_task) xTaskNotifyGive(g_task);
}

void ws_save_request_flush(void){
  g_flag_flush = true;
  if (g_task) xTaskNotifyGive(g_task);
}

void ws_save_force_flush(void){
  flush_now();
}
