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

        fb_draw_text(&framebuf_device, "hello world 123456789 !#@", 0, 0, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        fb_close(&framebuf_device);
    }

    return 0;
}
