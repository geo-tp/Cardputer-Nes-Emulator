// src/nes/nes_save.c
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef PATH_MAX
#define PATH_MAX 261
#endif

extern void nofrendo_log_printf(const char *fmt, ...);
extern char *osd_newextension(char *string, char *ext);

/* ---------- Autosave SRAM state ---------- */
static uint8_t   *g_sram_ptr = NULL;
static size_t     g_sram_len = 0;
static uint32_t   g_crc_last = 0;
static TickType_t g_next_check = 0;
static TickType_t g_dirty_deadline = 0;
static TickType_t g_next_allowed_write = 0;

static char       g_rompath_for_saves[PATH_MAX] = {0};

#define SRAM_CHECK_PERIOD_MS   250   /* Check period */
#define SRAM_DEBOUNCE_MS       2000  /* Wait after last change */
#define SRAM_MIN_WRITE_GAP_MS  5000  /* Min time between 2 writes */

/* CRC32 standard */
static uint32_t crc32_update(uint32_t crc, const uint8_t *buf, size_t len)
{
    static uint32_t* table = NULL;
    static int inited = 0;

    if (!inited) {
        if (!table) {
            table = (uint32_t*)malloc(256 * sizeof(uint32_t));
            if (!table) abort();
        }
        for (uint32_t i = 0; i < 256; i++){
            uint32_t c = i;
            for (int k = 0; k < 8; k++)
                c = (c & 1U) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        inited = 1;
    }

    crc ^= 0xFFFFFFFFU;
    for (size_t i = 0; i < len; i++)
        crc = table[(crc ^ buf[i]) & 0xFFU] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFU;
}

static void ensure_saves_dir(void)
{
    mkdir("/sd/nes_saves", 0777); // ignore errors
}

static void make_save_path(char *dst, size_t dstlen)
{
    char tmp[PATH_MAX];
    ensure_saves_dir();

    /* source */
    const char *src = g_rompath_for_saves[0] ? g_rompath_for_saves : "/xip/rom.nes";
    strncpy(tmp, src, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0';

    /* redirect to /sd/nes_saves/ */
    osd_newextension(tmp, (char*)".sav");
    strncpy(dst, tmp, dstlen-1);
    dst[dstlen-1] = '\0';
}

static void sram_autosave_flush(void)
{
    if (!g_sram_ptr || g_sram_len == 0) return;
    char path[PATH_MAX];
    make_save_path(path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) {
        nofrendo_log_printf("SRAM autosave: fopen fail (%s)\n", path);
        return;
    }
    size_t w = fwrite(g_sram_ptr, 1, g_sram_len, f);
    fclose(f);
    nofrendo_log_printf("SRAM autosave: wrote %u/%u bytes -> %s\n",
                        (unsigned)w, (unsigned)g_sram_len, path);
}

/* called by nes_rom.c after alloc/load SRAM, and before free (with ptr=NULL) */
void osd_set_sram_ptr(uint8_t *ptr, size_t len)
{
    g_sram_ptr = ptr;
    g_sram_len = len;
    g_crc_last = (ptr && len) ? crc32_update(0, ptr, len) : 0;
    g_next_check = 0;
    g_dirty_deadline = 0;
}

/* called by nes_osd.c (osd_main) to remember argv[0] for naming the save */
void osd_set_rompath_for_saves(const char *p)
{
    if (!p) p = "/xip/rom.nes";
    strncpy(g_rompath_for_saves, p, sizeof(g_rompath_for_saves)-1);
    g_rompath_for_saves[sizeof(g_rompath_for_saves)-1] = '\0';
}

/* called by custom_blit to trigger autosave */
void sram_autosave_tick(void)
{
    if (!g_sram_ptr || g_sram_len == 0) return;

    TickType_t now = xTaskGetTickCount();

    /* check périodique */
    if (now >= g_next_check) {
        g_next_check = now + pdMS_TO_TICKS(SRAM_CHECK_PERIOD_MS);
        uint32_t crc = crc32_update(0, g_sram_ptr, g_sram_len);
        if (crc != g_crc_last) {
            g_crc_last = crc;
            g_dirty_deadline = now + pdMS_TO_TICKS(SRAM_DEBOUNCE_MS);
        }
    }

    /* flush if stable since debounce, and gap respected */
    if (g_dirty_deadline && now >= g_dirty_deadline) {
        if (now >= g_next_allowed_write) {
            sram_autosave_flush();
            g_next_allowed_write = now + pdMS_TO_TICKS(SRAM_MIN_WRITE_GAP_MS);
        }
        g_dirty_deadline = 0;
    }
}

void sram_autosave_force_flush(void)
{
    sram_autosave_flush();
}

static int path_is_xip_mount(const char *p) { 
    return p && strncmp(p, "/xip/", 5) == 0; 
}

void osd_fullname(char *fullname, const char *shortname)
{
    strncpy(fullname, shortname, PATH_MAX);
    fullname[PATH_MAX-1] = '\0';
}

/* Redirect XIP saves to SD card */
static int path_starts_with_xip(const char *p)
{
    return (p && strncmp(p, "xip:", 4) == 0);
}

/* Construct SD save path from XIP */
static void build_sd_save_path_from_xip(char *dst, size_t dstlen, const char *xip_path, const char *ext4)
{
    const char *base = strrchr(xip_path, '/');
    base = base ? base + 1 : xip_path;  /* ex: "rom.nes" */
    char name[128];
    strncpy(name, base, sizeof(name)-1);
    name[sizeof(name)-1] = '\0';

    size_t nl = strlen(name);
    if (nl >= 4 && ext4 && strlen(ext4) == 4) { /* ".sav"/".sta" */
        name[nl-3] = ext4[1];
        name[nl-2] = ext4[2];
        name[nl-1] = ext4[3];
    }
    snprintf(dst, dstlen, "/sd/nes_saves/%s", name);
    dst[dstlen-1] = '\0';
}

/* This gives filenames for storage of saves */
char *osd_newextension(char *string, char *ext)
{
    if (path_is_xip_mount(string)) {
        static char buf[PATH_MAX];
        ensure_saves_dir();
        build_sd_save_path_from_xip(buf, sizeof(buf), string, ext);
        strncpy(string, buf, PATH_MAX - 1);
        string[PATH_MAX - 1] = '\0';
        return string;
    }

    size_t l = strlen(string);
    if (l >= 3) {
        string[l - 3] = ext[1];
        string[l - 2] = ext[2];
        string[l - 1] = ext[3];
    }
    return string;
}

/* This gives filenames for storage of PCX snapshots */
int osd_makesnapname(char *filename, int len)
{
    (void)filename; (void)len;
    return -1; /* not supported */
}