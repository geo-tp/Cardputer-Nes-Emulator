#include <string.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include "esp_vfs.h"
#include "vfs_xip.h"

extern const uint8_t* _get_rom_ptr(void);
extern size_t         _get_rom_size(void);

typedef struct {
    const uint8_t* base;
    size_t size;
    size_t pos;
    int used;
} xip_file_t;

#define XIP_MAX_OPEN 4
static xip_file_t s_files[XIP_MAX_OPEN];
static const char* MOUNT_PT = "/xip";
static bool s_registered = false;

static int path_is_rom(const char* rel)
{
    if (!rel) return 0;
    // The VFS receives a relative path to the mount point, often "/Tetris.nes"
    const char* p = rel;
    if (*p == '/') p++;
    // refuse empty or directory
    if (*p == '\0') return 0;
    if (strchr(p, '/')) return 0;
    return 1;  // accepts any filename for now
}

static int xip_open(void* ctx, const char * path, int flags, int mode)
{
    (void)ctx; (void)flags; (void)mode;
    if (!path_is_rom(path)) { errno = ENOENT; return -1; }

    const uint8_t* p = _get_rom_ptr();
    size_t sz = _get_rom_size();
    if (!p || sz == 0) { errno = ENOENT; return -1; }

    for (int i=0;i<XIP_MAX_OPEN;i++){
        if (!s_files[i].used){
            s_files[i].used = 1;
            s_files[i].base = p;
            s_files[i].size = sz;
            s_files[i].pos  = 0;
            return i;
        }
    }
    errno = EMFILE;
    return -1;
}

static ssize_t xip_read(void* ctx, int fd, void * dst, size_t size)
{
    (void)ctx;
    if (fd < 0 || fd >= XIP_MAX_OPEN || !s_files[fd].used) { errno = EBADF; return -1; }
    xip_file_t* f = &s_files[fd];
    size_t avail = (f->pos < f->size) ? (f->size - f->pos) : 0;
    if (size > avail) size = avail;
    if (size == 0) return 0;
    memcpy(dst, f->base + f->pos, size);
    f->pos += size;
    return (ssize_t)size;
}

static off_t xip_lseek(void* ctx, int fd, off_t offset, int whence)
{
    (void)ctx;
    if (fd < 0 || fd >= XIP_MAX_OPEN || !s_files[fd].used) { errno = EBADF; return -1; }
    xip_file_t* f = &s_files[fd];
    size_t newpos = f->pos;
    if (whence == SEEK_SET) newpos = (size_t)offset;
    else if (whence == SEEK_CUR) newpos += (size_t)offset;
    else if (whence == SEEK_END) newpos = f->size + (size_t)offset;
    else { errno = EINVAL; return -1; }
    if (newpos > f->size) { errno = EINVAL; return -1; }
    f->pos = newpos;
    return (off_t)f->pos;
}

static int xip_close(void* ctx, int fd)
{
    (void)ctx;
    if (fd < 0 || fd >= XIP_MAX_OPEN || !s_files[fd].used) { errno = EBADF; return -1; }
    memset(&s_files[fd], 0, sizeof(s_files[fd]));
    return 0;
}

static ssize_t xip_write(void* ctx, int fd, const void * data, size_t size)
{ (void)ctx; (void)fd; (void)data; (void)size; errno = EROFS; return -1; }

static int xip_fstat(void* ctx, int fd, struct stat *st)
{
    (void)ctx;
    if (fd < 0 || fd >= XIP_MAX_OPEN || !s_files[fd].used) { errno = EBADF; return -1; }
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFREG | 0444;
    st->st_size = (off_t)s_files[fd].size;
    return 0;
}

static int xip_stat(void* ctx, const char *path, struct stat *st)
{
    (void)ctx;
    if (!path_is_rom(path)) { errno = ENOENT; return -1; }
    const uint8_t* p = _get_rom_ptr();
    size_t sz = _get_rom_size();
    if (!p || sz == 0) { errno = ENOENT; return -1; }
    memset(st, 0, sizeof(*st));
    st->st_mode = S_IFREG | 0444;
    st->st_size = (off_t)sz;
    return 0;
}

void vfs_xip_register(void)
{
    if (s_registered) return;
    esp_vfs_t vfs = {
        .flags   = ESP_VFS_FLAG_CONTEXT_PTR,
        .write_p = xip_write,
        .open_p  = xip_open,
        .fstat_p = xip_fstat,
        .close_p = xip_close,
        .read_p  = xip_read,
        .lseek_p = xip_lseek,
        .stat_p  = xip_stat,
    };
    esp_vfs_register(MOUNT_PT, &vfs, NULL);
    s_registered = true;
}

void vfs_xip_unregister(void)
{
    if (!s_registered) return;
    esp_vfs_unregister(MOUNT_PT);
    memset(s_files, 0, sizeof(s_files));
    s_registered = false;
}
