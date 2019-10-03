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

    return 1;
}

void
fb_close(struct FbDev* fb_device) {
    /* Release frame buffer resources */
    munmap(fb_device->fbuf, fb_device->memlen);
    close(fb_device->fb_fd);
}

void
fb_put_pixel(const struct FbDev* fb_device, uint32_t x, uint32_t y /*, color */) {
    uint32_t mloc = _get_memory_location(fb_device, x, y);
    memset(fb_device->fbuf + mloc, 0xFFFFFFFF, sizeof(uint32_t));
    //*((uint32_t*) (fb_device->fbuf + mloc)) = 0xFFFFFFFF;
}

uint32_t
_get_memory_location(const struct FbDev *fb_device, uint32_t ox, uint32_t oy) {
    // https://www.i-programmer.info/programming/cc/12839-applying-c-framebuffer-graphics.html
    return ox * fb_device->bpp / 8 + oy * fb_device->linelen;
}
