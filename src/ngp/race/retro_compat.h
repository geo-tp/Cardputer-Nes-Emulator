#pragma once

// --- Standard ---
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// --- INLINE  ---
#ifndef INLINE
#define INLINE inline
#endif

// --------- EMUINFO ----------
typedef struct {
  int  machine;  // NGP ou NGPC
  int  romSize;
  char RomFileName[1]; // unused
} EMUINFO;

// --- BOOL ---
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// --- Activity flag ---
extern int m_bIsActive;

extern EMUINFO m_emuInfo;

// Types machine
#ifndef NGP
#define NGP  0
#endif
#ifndef NGPC
#define NGPC 1
#endif

// --------- Screen for compat ----------
struct ngp_screen;
extern struct ngp_screen *screen;

extern char retro_save_directory[3]; // unused
extern int tipo_consola;

// --------- dbg_print stub ----------
static INLINE void dbg_print(const char* s) { (void)s; }

// --------- Shim libretro VFS----------
typedef FILE RFILE;

#define RETRO_VFS_FILE_ACCESS_READ        1
#define RETRO_VFS_FILE_ACCESS_WRITE       2
#define RETRO_VFS_FILE_ACCESS_UPDATE      3
#define RETRO_VFS_FILE_ACCESS_HINT_NONE   0

static INLINE RFILE* filestream_open(const char* path, int access, int hint) {
  (void)hint;
  const char* mode = (access == RETRO_VFS_FILE_ACCESS_WRITE)  ? "wb" :
                     (access == RETRO_VFS_FILE_ACCESS_UPDATE) ? "rb+" : "rb";
  return fopen(path, mode);
}
static INLINE int64_t filestream_read(RFILE* f, void* buf, size_t bytes) {
  if (!f) return -1;
  size_t n = fread(buf, 1, bytes, f);
  return (int64_t)n;
}
static INLINE int64_t filestream_write(RFILE* f, const void* buf, size_t bytes) {
  if (!f) return -1;
  size_t n = fwrite(buf, 1, bytes, f);
  return (int64_t)n;
}
static INLINE void filestream_close(RFILE* f) {
  if (f) fclose(f);
}

#ifdef RETRO_COMPAT_IMPLEMENTATION
EMUINFO m_emuInfo = { .machine = NGPC, .romSize = 0, .RomFileName = {0} };
struct ngp_screen *screen = NULL;
#endif

