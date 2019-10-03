#include <stdio.h>
#include "fb_lib.h"

int main() {

    struct FbDev framebuf_device;

    if(!fb_init("/dev/fb0", &framebuf_device)) {
        perror("Could not access framebuffer, exit");
        return 1;
    } else {
        /* Frame buffer is opened */


        fb_close(&framebuf_device);
    }

    return 0;
}