//
// Created by pstadler on 02.10.19.
//
#ifndef FB_FB_LIB_H
#define FB_FB_LIB_H

#include <stdint.h>

/* Color palette for this application */
#define COLOR_WHITE     ((uint32_t)0x00FFFFFF)
#define COLOR_BLACK     ((uint32_t)0x00000000)
#define COLOR_RED       ((uint32_t)0x00FF0000)
#define COLOR_GREEN     ((uint32_t)0x0000FF00)
#define COLOR_BLUE      ((uint32_t)0x000000FF)
#define COLOR_YELLOW    ((uint32_t)0x00FFFF00)
#define COLOR_MAGENTA   ((uint32_t)0x00FF00FF)

/* Flags for drawing helpers */
#define DRAW_CENTER_VERTICAL    ((uint32_t)0x1)
#define DRAW_CENTER_HORIZONTAL  ((uint32_t)0x2)
#define DRAW_CENTER_NONE        ((uint32_t)0x0)

/* Font padding */
#define FONT_PADDING    ((uint32_t)0)   /* in pixels */

#define CHAR_NONE   ((uint32_t)59)      /* Use empty char (offset 59) generated in the font file as none char */

/* FbDev holds all required (essential) information
   for further screen drawing operations */
struct FbDev {
    int fb_fd;          /* File descriptor */
    uint32_t w;         /* Visible screen width */
    uint32_t h;         /* Visible screen height */
    uint32_t bpp;       /* Bits per pixel (color depth) */
    uint32_t linelen;   /* Pre-calculated horizontal length */
    uint32_t memlen;    /* Pre-calculated memory size */
    char* fbuf;         /* Mapped memory (front buffer) */
    char* bbuf;         /* Mapped memory (back buffer) */
};

int8_t fb_init(const char* fb_dev_id, struct FbDev* fb_device);
void fb_close(struct FbDev* fb_device);
void fb_clear_screen(struct FbDev* fb_device);
void fb_update(struct FbDev* fb_device);
void fb_draw_line(const struct FbDev* fb_device, int32_t xfrom, int32_t yfrom, int32_t xto, int32_t yto, uint32_t color);
void fb_draw_rect(const struct FbDev* fb_device, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint32_t flags);
void fb_draw_text(const struct FbDev* fb_device, const char* text, uint32_t x, uint32_t y, uint32_t color, uint32_t flags);
#endif //FB_FB_LIB_H
