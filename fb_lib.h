//
// Created by pstadler on 02.10.19.
//
#ifndef FB_FB_LIB_H
#define FB_FB_LIB_H

#include <stdint.h>

/* FbDev holds all required (essential) information
   for further screen drawing operations */
struct FbDev {
    int fb_fd;          /* File descriptor */
    uint32_t w;         /* Visible screen width */
    uint32_t h;         /* Visible screen height */
    uint32_t bpp;       /* Bits per pixel (color depth) */
    uint32_t linelen;   /* Pre-calculated horizontal length */
    uint32_t memlen;    /* Pre-calculated memory size */
    char* fbuf;         /* Holds mapped memory */
};

int8_t fb_init(const char* fb_dev_id, struct FbDev* fb_device);
void fb_close(struct FbDev* fb_device);

#endif //FB_FB_LIB_H
