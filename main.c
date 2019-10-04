#include <stdio.h>
#include "fb_lib.h"

int main() {

    struct FbDev framebuf_device;

    if(!fb_init("/dev/fb0", &framebuf_device)) {
        perror("Could not access framebuffer, exit");
        return 1;
    } else {
        /* Frame buffer is opened */
        uint32_t w = framebuf_device.w;
        uint32_t h = framebuf_device.h;

        fb_draw_line(&framebuf_device, 0, 0, w, h, COLOR_WHITE);
        fb_draw_line(&framebuf_device, w, 0, 0, h, COLOR_WHITE);

        fb_draw_rect(&framebuf_device, 0, 0, w / 2, h / 2, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        char display_info_str[50];
        sprintf(display_info_str, "Screen Resolution: %dx%d", framebuf_device.w, framebuf_device.h);
	    char bpp_info_str[20];
	    sprintf(bpp_info_str, "Color depth: %d", framebuf_device.bpp);

        fb_draw_text(&framebuf_device, display_info_str, 0, 0, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        fb_draw_text(&framebuf_device, bpp_info_str, 0, 20, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
	    fb_draw_text(&framebuf_device, "2019 University of Regensburg", 0, 60, COLOR_BLUE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        fb_close(&framebuf_device);
    }

    return 0;
}
