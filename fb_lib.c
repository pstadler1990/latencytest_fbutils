//
// Created by pstadler on 02.10.19.
//
#include "fb_lib.h"
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

// https://gist.github.com/FredEckert/3425429
// https://www.kernel.org/doc/Documentation/fb/api.txt
// https://docs.huihoo.com/doxygen/linux/kernel/3.7/include_2uapi_2linux_2fb_8h_source.html

__always_inline static uint32_t _get_memory_location(const struct FbDev* fb_device, uint32_t ox, uint32_t oy);
static void _put_pixel(const struct FbDev* fb_device, uint32_t x, uint32_t y, uint32_t color);

int8_t
fb_init(const char* fb_dev_id, struct FbDev* fb_device) {
    /* Initializes access to the framebuffer device and maps the memory */
    if(!fb_device) {
        return -1;
    }

    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;

    /* Open frame buffer device */
    if(!(fb_device->fb_fd = open(fb_dev_id, O_RDWR))) {
        return -1;
    }

    if((ioctl(fb_device->fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1)
        || (ioctl(fb_device->fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1)) {
        return -1;
    }

    printf("Display name: %s\nLine length: %d\nDispay width: %d\nDisplay height: %d\nColor depth: %d\n", finfo.id, finfo.line_length, vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    /* Copy important screen information to the frame buffer device */
    fb_device->w = vinfo.xres;
    fb_device->h = vinfo.yres;
    fb_device->bpp = vinfo.bits_per_pixel;
    fb_device->linelen = finfo.line_length;
    fb_device->memlen = finfo.smem_len;

    /* Map memory */
    fb_device->fbuf = (char *)mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_device->fb_fd, 0);
    if(!fb_device->fbuf) {
        return -1;
    }

    memset(fb_device->fbuf, 0, finfo.smem_len);

    return 1;
}

void
fb_close(struct FbDev* fb_device) {
    /* Release frame buffer resources */
    munmap(fb_device->fbuf, fb_device->memlen);
    close(fb_device->fb_fd);
}

/* Convenient helper functions for drawing */
void
fb_draw_line(const struct FbDev* fb_device, int32_t xfrom, int32_t yfrom, int32_t xto, int32_t yto, uint32_t color) {
    /* Draws a line based on the Bresenham algorithm from xfrom, yfrom to xto, yto with the specified color (see "Color palette for this application")
       Implementation details: https://de.wikipedia.org/wiki/Bresenham-Algorithmus */
    int32_t dx = abs(xto - xfrom);
    int32_t sx = xfrom < xto ? 1 : -1;
    int32_t dy = -abs(yto - yfrom);
    int32_t sy = yfrom < yto ? 1 : -1;
    int32_t err = dx + dy;
    int32_t err_t;

    while(1) {
        _put_pixel(fb_device, (uint32_t) xfrom, (uint32_t) yfrom, color);
        if(xfrom == xto && yfrom == yto) {
            break;
        }
        err_t = 2 * err;
        if (err_t > dy) {
            err += dy;
            xfrom += sx;
        }
        if(err_t < dx) {
            err += dx;
            yfrom += sy;
        }
    }
}

void
fb_draw_rect(const struct FbDev* fb_device, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t flags) {
    /* Draws a rectangle at x,y with width and height w,h in the specified color.
       You can pass optional flags - otherwise choose DRAW_CENTER_NONE - to center the drawing.
       If you pass any flag or both of DRAW_CENTER_VERTICAL or DRAW_CENTER_HORIZONTAL, x and / or y coordinate is discarded */
    if(flags & DRAW_CENTER_HORIZONTAL) {
        x = (fb_device->w / 2) - (w / 2);
    }
    if(flags & DRAW_CENTER_VERTICAL) {
        y = (fb_device->h / 2) - (h / 2);
    }

    for(uint32_t _x = x; _x < (x + w); _x++) {
        for(uint32_t _y = y; _y < (y + h); _y++) {
            _put_pixel(fb_device, _x, _y, color);
        }
    }
}

void
_put_pixel(const struct FbDev* fb_device, uint32_t x, uint32_t y, uint32_t color) {
    /* Puts a pixel of given RGBa color (find some pre-defined colors in Color palette)
       at display screen coordinates x, y */
    if(x < fb_device->w && y < fb_device->h) {
        uint32_t mloc = _get_memory_location(fb_device, x, y);
        *((uint32_t*) (fb_device->fbuf + mloc)) = color;
    }
}

uint32_t
_get_memory_location(const struct FbDev *fb_device, uint32_t ox, uint32_t oy) {
    return ox * fb_device->bpp / 8 + oy * fb_device->linelen;
}
