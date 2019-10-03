#include <stdio.h>
#include "fb_lib.h"

int main() {

    struct FbDev framebuf_device;

    if(!fb_init("/dev/fb0", &framebuf_device)) {
        perror("Could not access framebuffer, exit");
        return 1;
    } else {
        /* Frame buffer is opened */
        for(uint32_t x = 0; x < 320; x++) {
            for(uint32_t y = 0; y < 240; y++) {
                fb_put_pixel(&framebuf_device, x, y);
            }
        }
        //fb_close(&framebuf_device);
    }

    return 0;
}