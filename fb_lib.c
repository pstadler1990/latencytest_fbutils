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


// https://gist.github.com/FredEckert/3425429
// https://www.kernel.org/doc/Documentation/fb/api.txt
// https://docs.huihoo.com/doxygen/linux/kernel/3.7/include_2uapi_2linux_2fb_8h_source.html

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

    printf("%s %d\n", finfo.id, finfo.line_length);

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