#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/timers.h>

#include <esp_heap_caps.h>

/* Nofrendo / OSD headers */
#include <noftypes.h>
#include <event.h>
#include <gui.h>
#include <log.h>
#include <nes/nes.h>
#include <nes/nes_pal.h>
#include <nes/nesinput.h>
#include <nofconfig.h>
#include <stdbool.h>
#include <osd.h>

#define HW_AUDIO_SAMPLERATE 22050
#define NTSC

TimerHandle_t nes_timer;
static uint8_t *fb = NULL;       // framebuffer 8bpp
static bitmap_t *myBitmap = NULL;

#define FB_WIDTH    NES_SCREEN_WIDTH     // 256
#define FB_HEIGHT   256                  // surallocate
#define VISIBLE_H   NES_SCREEN_HEIGHT    // 240 visible

/* memory allocation */
extern void *mem_alloc(int size, bool prefer_fast_memory) {
    (void)prefer_fast_memory;
    return malloc((size_t)size);
}

/* sound */
extern int osd_init_sound();
extern void osd_stopsound();
extern void do_audio_frame();
extern void osd_getsoundinfo(sndinfo_t *info);

/* display */
extern void display_init();
extern void display_write_frame(const uint8_t *data[]);
extern void display_clear();

// This runs on core 0
QueueHandle_t vidQueue;
static void displayTask(void *arg) {
    bitmap_t *bmp = NULL;
    nofrendo_log_printf("OSD: displayTask started\n");
    while (1) {
        if (xQueueReceive(vidQueue, &bmp, portMAX_DELAY) == pdTRUE) {
            if (!bmp || !bmp->line[0]) { nofrendo_log_printf("OSD: bad bmp\n"); continue; }
            display_write_frame((const uint8_t **)bmp->line);
            static uint32_t frames=0;
            if ((++frames & 0x3F) == 0) nofrendo_log_printf("OSD: frames=%lu\n", (unsigned long)frames);
        }
    }
}

/* initialise video */
static int init(int width, int height) { return 0; }

static void shutdown(void) {}

/* set a video mode */
static int set_mode(int width, int height) { return 0; }

/* copy nes palette over to hardware */
uint16 myPalette[256];
static void set_palette(rgb_t *pal) {
    uint16 c;

    int i;

    for (i = 0; i < 256; i++) {
        c = (pal[i].b >> 3) + ((pal[i].g >> 2) << 5) + ((pal[i].r >> 3) << 11);
        myPalette[i] = (c >> 8) | ((c & 0xff) << 8);
        // myPalette[i] = c;
        // nofrendo_log_printf("myPallete[%d]: %d\n", i, c);
    }
}

/* clear all frames to a particular color */
static void clear(uint8 color) {
    display_clear();
}

static bitmap_t *lock_write(void) {
    if (!fb) {
        const size_t fb_size = (size_t)FB_WIDTH * FB_HEIGHT; // 61 440
        nofrendo_log_printf("OSD: alloc fb %u bytes (%dx%d)\n",
                            (unsigned)fb_size, FB_WIDTH, FB_HEIGHT);

        // DRAM only
        fb = (uint8_t*) malloc(fb_size);
        if (!fb) { nofrendo_log_printf("OSD: alloc fb FAILED\n"); return NULL; }
        memset(fb, 0, fb_size);

        // Create a bitmap that uses this framebuffer
        myBitmap = bmp_createhw((uint8 *)fb, FB_WIDTH, FB_HEIGHT, FB_WIDTH);
        if (!myBitmap) { nofrendo_log_printf("OSD: bmp_createhw FAILED\n"); return NULL; }

        nofrendo_log_printf("OSD: lock_write -> bmp %p\n", (void*)myBitmap);
    }
    return myBitmap; // reuse
}


static void free_write(int num_dirties, rect_t *dirty_rects) {
    // bmp_destroy(&myBitmap);
    // free(fb); fb = NULL;
}

static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects) {
    static uint32_t blits=0;
    if ((++blits & 0x3F) == 0) nofrendo_log_printf("OSD: blit %lu\n", (unsigned long)blits);
    if (xQueueSend(vidQueue, &bmp, 0) != pdPASS) {
        bitmap_t *tmp; xQueueReceive(vidQueue, &tmp, 0);
        (void)xQueueSend(vidQueue, &bmp, 0);
    }
    do_audio_frame();
}

viddriver_t sdlDriver = {
    "Simple DirectMedia Layer", /* name */
    init,                       /* init */
    shutdown,                   /* shutdown */
    set_mode,                   /* set_mode */
    set_palette,                /* set_palette */
    clear,                      /* clear */
    lock_write,                 /* lock_write */
    free_write,                 /* free_write */
    custom_blit,                /* custom_blit */
    false                       /* invalidate flag */
};

void osd_getvideoinfo(vidinfo_t *info) {
    info->default_width = NES_SCREEN_WIDTH;
    info->default_height = NES_SCREEN_HEIGHT;
    info->driver = &sdlDriver;
}

/* input */
extern void controller_init();
extern uint32_t controller_read_input();

static void osd_initinput() { controller_init(); }

static void osd_freeinput(void) {}

void osd_getinput(void) {
    const int ev[32] = {
        event_joypad1_up,
        event_joypad1_down,
        event_joypad1_left,
        event_joypad1_right,
        event_joypad1_select,
        event_joypad1_start,
        event_joypad1_a,
        event_joypad1_b,
        event_state_save,
        event_state_load,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    };
    static int oldb = 0xffff;
    uint32_t b = controller_read_input();
    uint32_t chg = b ^ oldb;
    int x;
    oldb = b;
    event_t evh;
    // nofrendo_log_printf("Input: %x\n", b);
    for (x = 0; x < 16; x++) {
        if (chg & 1) {
            evh = event_get(ev[x]);
            if (evh) evh((b & 1) ? INP_STATE_BREAK : INP_STATE_MAKE);
        }
        chg >>= 1;
        b >>= 1;
    }
}

void osd_getmouse(int *x, int *y, int *button) {}

/* init / shutdown */
static int logprint(const char *string) { return printf("%s", string); }

int osd_init() {
    nofrendo_log_chain_logfunc(logprint);

    if (osd_init_sound()) return -1;

    display_init();
    vidQueue = xQueueCreate(2, sizeof(bitmap_t *));
    xTaskCreatePinnedToCore(&displayTask, "displayTask", 4096, NULL, 3, NULL, 0);
    osd_initinput();
    return 0;
}

void osd_shutdown() {
    osd_stopsound();
    osd_freeinput();
}

char configfilename[] = "na";
int osd_main(int argc, char *argv[]) {
    config.filename = configfilename;

    return main_loop(argv[0], system_autodetect);
}

// Seemingly, this will be called only once. Should call func with a freq of frequency,
int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize) {
    nofrendo_log_printf("Timer install, configTICK_RATE_HZ=%d, freq=%d\n", configTICK_RATE_HZ, frequency);
    nes_timer =
        xTimerCreate("nes", configTICK_RATE_HZ / frequency, pdTRUE, NULL, (TimerCallbackFunction_t)func);
    xTimerStart(nes_timer, 0);
    return 0;
}

/* filename manipulation */
void osd_fullname(char *fullname, const char *shortname) { strncpy(fullname, shortname, PATH_MAX); }

/* This gives filenames for storage of saves */
char *osd_newextension(char *string, char *ext) {
    // assume extensions is 3 characters
    size_t l = strlen(string);
    string[l - 3] = ext[1];
    string[l - 2] = ext[2];
    string[l - 1] = ext[3];

    return string;
}

/* This gives filenames for storage of PCX snapshots */
int osd_makesnapname(char *filename, int len) { return -1; }

/* Wrap all bitmap creation from nofrendo */
bitmap_t *__wrap_bmp_create(int width, int height, int pitch)
{
    if (!myBitmap) lock_write();
    return myBitmap;
}

/* Wrap all bitmap destruction from nofrendo */
void __wrap_bmp_destroy(bitmap_t **bmp)
{
    (void)bmp; // no-op
}