//
// Created by pstadler on 02.10.19.
//
#include "fb_lib.h"
#include "font_monospaced16px.h"
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
static void _put_char(const struct FbDev* fb_device, char c, uint32_t x, uint32_t y, uint32_t color);

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
       If you pass any flag or both of DRAW_CENTER_VERTICAL or DRAW_CENTER_HORIZONTAL, x and / or y coordinate are added as offsets */
    if(flags & DRAW_CENTER_HORIZONTAL) {
        x = ((fb_device->w / 2) - (w / 2)) + x;
    }
    if(flags & DRAW_CENTER_VERTICAL) {
        y = ((fb_device->h / 2) - (h / 2)) + y;
    }

    for(uint32_t _x = x; _x < (x + w); _x++) {
        for(uint32_t _y = y; _y < (y + h); _y++) {
            _put_pixel(fb_device, _x, _y, color);
        }
    }
}

void
fb_draw_text(const struct FbDev* fb_device, const char* text, uint32_t x, uint32_t y, uint32_t color, uint32_t flags) {
    /* High-level function to render given text at coordinates x,y with the specified color,
       You can pass optional flags - otherwise choose DRAW_CENTER_NONE - to center the drawing.
       If you pass any flag or both of DRAW_CENTER_VERTICAL or DRAW_CENTER_HORIZONTAL, x and / or y coordinate are added as offsets */
    if(flags & DRAW_CENTER_HORIZONTAL) {
        uint32_t w = (uint32_t) strlen(text) * (FONT_WIDTH + 2 * FONT_PADDING);
        x = ((fb_device->w / 2) - (w / 2)) + x;
    }
    if(flags & DRAW_CENTER_VERTICAL) {
        uint32_t h = FONT_HEIGHT;
        y = ((fb_device->h / 2) - (h / 2)) + y;
    }

    for(char c = 0; c < strlen(text); c++) {
        unsigned char c_offset_real = (unsigned char) (text[c] - FONT_CHAR_START);
        if(c_offset_real == 0xFF) {
            /* In case the character is not available in the font file, use defined non-char character */
            c_offset_real = CHAR_NONE;
        }
        uint32_t char_x = x + (c * (FONT_WIDTH + 2 * FONT_PADDING));
        _put_char(fb_device, c_offset_real, char_x, y, color);
    }
}

void
_put_char(const struct FbDev* fb_device, char c, uint32_t x, uint32_t y, uint32_t color) {
    /* Low-level function to plot a character c at coordinates x,y,
       based on the monochrome xbm file format,
       refer to https://en.wikipedia.org/wiki/X_BitMap for further information */
    uint32_t xbit = 0;
    uint32_t xreq = FONT_WIDTH;
    uint32_t xpro = 0;
    unsigned char byte;

    uint32_t cur_char = 0;
    while(cur_char <= 34) {
        byte = chars[c][cur_char];
        while(xreq) {
            /* Put pixel if bit is set */
            if(((byte >> xbit) & 1)) {
                _put_pixel(fb_device, x + xpro, y, color);
                _put_pixel(fb_device, x + xpro - 1, y, color);
                _put_pixel(fb_device, x + xpro + 1, y, color);
            }
            xbit += 1;
            xpro += 1;

            if(xreq > 0) {
                xreq -= 1;
            }

            if(xpro == 8) {
                /* Has processed 8 bits (full byte), proceed with next byte */
                xbit = (FONT_WIDTH - xpro) - 1;
                xpro = 0;
                cur_char += 1;
                byte = chars[c][cur_char];
            }
        }
        /* New line */
        cur_char += 1;
        xreq = FONT_WIDTH;
        xpro = 0;
        xbit = 0;
        y += 1;
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
