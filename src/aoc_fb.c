#include "aoc/aoc.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define FBIOGET_VSCREENINFO 0x4600
#define FBIOGET_FSCREENINFO 0x4602

struct fb_bitfield_local {
    uint32_t offset;
    uint32_t length;
    uint32_t msb_right;
};

struct fb_var_screeninfo_local {
    uint32_t xres;
    uint32_t yres;
    uint32_t xres_virtual;
    uint32_t yres_virtual;
    uint32_t xoffset;
    uint32_t yoffset;
    uint32_t bits_per_pixel;
    uint32_t grayscale;
    struct fb_bitfield_local red;
    struct fb_bitfield_local green;
    struct fb_bitfield_local blue;
    struct fb_bitfield_local transp;
    uint32_t nonstd;
    uint32_t activate;
    uint32_t height;
    uint32_t width;
    uint32_t accel_flags;
    uint32_t pixclock;
    uint32_t left_margin;
    uint32_t right_margin;
    uint32_t upper_margin;
    uint32_t lower_margin;
    uint32_t hsync_len;
    uint32_t vsync_len;
    uint32_t sync;
    uint32_t vmode;
    uint32_t rotate;
    uint32_t reserved[5];
};

struct fb_fix_screeninfo_local {
    char id[16];
    unsigned long smem_start;
    uint32_t smem_len;
    uint32_t type;
    uint32_t type_aux;
    uint32_t visual;
    uint16_t xpanstep;
    uint16_t ypanstep;
    uint16_t ywrapstep;
    uint32_t line_length;
    unsigned long mmio_start;
    uint32_t mmio_len;
    uint32_t accel;
    uint16_t capabilities;
    uint16_t reserved[2];
};

static unsigned long choose_map_len(
    const struct fb_fix_screeninfo_local *fix,
    const struct fb_var_screeninfo_local *var
) {
    unsigned long len = fix->smem_len;
    unsigned long derived = 0;

    if (fix->line_length != 0 && var->yres_virtual != 0) {
        derived = (unsigned long)fix->line_length * (unsigned long)var->yres_virtual;
    }
    if (len == 0 || len > (64UL * 1024UL * 1024UL)) {
        len = derived;
    }
    if (len > (64UL * 1024UL * 1024UL)) {
        len = 64UL * 1024UL * 1024UL;
    }
    return len;
}

static unsigned long framebuffer_page_len(
    const struct fb_fix_screeninfo_local *fix,
    const struct fb_var_screeninfo_local *var
) {
    unsigned int height = var->yres_virtual != 0 ? var->yres_virtual : var->yres;

    return (unsigned long)fix->line_length * (unsigned long)height;
}

static unsigned int parse_uint_env(const char *name, unsigned int fallback) {
    const char *value = getenv(name);
    unsigned long parsed;
    char *end = 0;

    if (value == 0 || value[0] == '\0') {
        return fallback;
    }
    parsed = strtoul(value, &end, 10);
    if (end == value || parsed > 65535UL) {
        return fallback;
    }
    return (unsigned int)parsed;
}

unsigned int aoc_fb_page_count(const struct aoc_fb *fb) {
    if (fb == 0 || fb->page_len == 0) {
        return 0;
    }
    return (unsigned int)(fb->map_len / fb->page_len);
}

unsigned int aoc_fb_configured_present_pages(const struct aoc_fb *fb) {
    if (fb == 0) {
        return 0;
    }
    return fb->present_pages;
}

static void configure_present_pages(struct aoc_fb *fb) {
    const char *pages_env = getenv("AOC_FB_PAGES");
    unsigned int page_count = aoc_fb_page_count(fb);
    unsigned int present_pages = 1U;

    if (page_count == 0) {
        fb->present_pages = 0;
        fb->full_refresh_every = 0;
        return;
    }
    if (pages_env != 0 && strcmp(pages_env, "all") == 0) {
        present_pages = page_count;
    } else if (pages_env != 0 && pages_env[0] != '\0') {
        present_pages = parse_uint_env("AOC_FB_PAGES", present_pages);
    }
    if (present_pages == 0) {
        present_pages = 1U;
    }
    if (present_pages > page_count) {
        present_pages = page_count;
    }
    fb->present_pages = present_pages;
    fb->full_refresh_every = parse_uint_env("AOC_FB_FULL_REFRESH_EVERY", 0);
}

int aoc_fb_open(struct aoc_fb *fb, const char *device_path) {
    struct fb_fix_screeninfo_local fix;
    struct fb_var_screeninfo_local var;
    const char *path = device_path != 0 ? device_path : AOC_FB_DEFAULT_DEVICE;

    if (fb == 0) {
        return -1;
    }

    memset(fb, 0, sizeof(*fb));
    fb->fd = -1;
    fb->map = MAP_FAILED;

    fb->fd = open(path, O_RDWR);
    if (fb->fd < 0) {
        aoc_log_errno("aoc_fb: open failed");
        return -2;
    }
    aoc_log_line("aoc_fb: open ok");

    memset(&fix, 0, sizeof(fix));
    if (aoc_raw_ioctl(fb->fd, FBIOGET_FSCREENINFO, &fix) != 0) {
        aoc_log_errno("aoc_fb: FBIOGET_FSCREENINFO failed");
        aoc_fb_close(fb);
        return -3;
    }

    memset(&var, 0, sizeof(var));
    if (aoc_raw_ioctl(fb->fd, FBIOGET_VSCREENINFO, &var) != 0) {
        aoc_log_errno("aoc_fb: FBIOGET_VSCREENINFO failed");
        aoc_fb_close(fb);
        return -4;
    }

    fb->xres = var.xres;
    fb->yres = var.yres;
    fb->xres_virtual = var.xres_virtual;
    fb->yres_virtual = var.yres_virtual;
    fb->bits_per_pixel = var.bits_per_pixel;
    fb->line_length = fix.line_length;

    aoc_log_dec("aoc_fb: xres=", (unsigned long)fb->xres);
    aoc_log_dec("aoc_fb: yres=", (unsigned long)fb->yres);
    aoc_log_dec("aoc_fb: bpp=", (unsigned long)fb->bits_per_pixel);
    aoc_log_dec("aoc_fb: line_length=", (unsigned long)fb->line_length);

    if (fb->bits_per_pixel != 32 || fb->xres == 0 || fb->yres == 0 ||
        fb->line_length < (fb->xres * 4U)) {
        aoc_log_line("aoc_fb: unsupported framebuffer layout");
        aoc_fb_close(fb);
        return -5;
    }

    fb->map_len = choose_map_len(&fix, &var);
    fb->page_len = framebuffer_page_len(&fix, &var);
    aoc_log_dec("aoc_fb: map_len=", fb->map_len);
    aoc_log_dec("aoc_fb: page_len=", fb->page_len);
    if (fb->page_len == 0 || fb->map_len < fb->page_len) {
        aoc_log_line("aoc_fb: invalid map length");
        aoc_fb_close(fb);
        return -6;
    }

    fb->map = mmap(0, fb->map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
    if (fb->map == MAP_FAILED) {
        aoc_log_errno("aoc_fb: mmap failed");
        aoc_fb_close(fb);
        return -7;
    }

    configure_present_pages(fb);
    aoc_log_dec("aoc_fb: page_count=", (unsigned long)aoc_fb_page_count(fb));
    aoc_log_dec("aoc_fb: present_pages=", (unsigned long)fb->present_pages);
    aoc_log_line("aoc_fb: mmap ok");
    return 0;
}

void aoc_fb_close(struct aoc_fb *fb) {
    if (fb == 0) {
        return;
    }
    if (fb->map != MAP_FAILED && fb->map != 0 && fb->map_len != 0) {
        munmap(fb->map, fb->map_len);
    }
    if (fb->fd >= 0) {
        close(fb->fd);
    }
    fb->fd = -1;
    fb->map = MAP_FAILED;
    fb->map_len = 0;
    fb->page_len = 0;
}

static void paint_scaled_page(
    struct aoc_fb *fb,
    unsigned long page_offset,
    const uint32_t *src,
    unsigned int src_width,
    unsigned int src_height
) {
    unsigned int y;

    for (y = 0; y < fb->yres; ++y) {
        unsigned int x;
        unsigned int src_y = (y * src_height) / fb->yres;
        volatile uint32_t *row = (volatile uint32_t *)((char *)fb->map + page_offset + ((unsigned long)y * fb->line_length));
        const uint32_t *src_row = src + ((unsigned long)src_y * src_width);

        for (x = 0; x < fb->xres; ++x) {
            unsigned int src_x = (x * src_width) / fb->xres;
            row[x] = src_row[src_x] | 0xff000000U;
        }
    }
}

void aoc_fb_present_xrgb8888_scaled(
    struct aoc_fb *fb,
    const uint32_t *src,
    unsigned int src_width,
    unsigned int src_height
) {
    unsigned int page;
    unsigned int page_count;
    unsigned int pages;
    unsigned long sync_len;

    if (fb == 0 || src == 0 || fb->map == MAP_FAILED || fb->page_len == 0 ||
        src_width == 0 || src_height == 0) {
        return;
    }

    ++fb->frame_count;
    page_count = aoc_fb_page_count(fb);
    pages = fb->present_pages;
    if (fb->full_refresh_every != 0 &&
        (fb->frame_count % fb->full_refresh_every) == 0) {
        pages = page_count;
    }
    if (pages == 0 || pages > page_count) {
        pages = page_count;
    }

    for (page = 0; page < pages; ++page) {
        paint_scaled_page(fb, (unsigned long)page * fb->page_len, src, src_width, src_height);
    }

    sync_len = (unsigned long)pages * fb->page_len;
    if (sync_len > fb->map_len) {
        sync_len = fb->map_len;
    }
    if (sync_len != 0) {
        msync(fb->map, sync_len, MS_ASYNC);
    }
}
