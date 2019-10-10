#include <stdio.h>
#include "fb_lib.h"
#include "screens.h"
#include "main.h"
#include "configuration.h"
#include "pinsetup.h"

struct FbDevState framebuf_state = {
        .state = FBSTATE_INITIALIZE,
        .n_measurements = DEFAULT_N_MEASUREMENTS,
};

int main() {
    struct FbDev framebuf_device;

    if(!fb_init("/dev/fb0", &framebuf_device)) {
        perror("Could not access framebuffer, exit");
        /* We don't need to process any errors here, as the framebuffer is crucial */
        return 1;
    } else {
        /* Frame buffer is opened */
        if(!init_GPIOs()) {
            perror("Could not open GPIOs, exit");
        }

        framebuf_state.state = FBSTATE_IDLE;

        /* Show home screen until external triggers occur (i.e. start button pressed) */
        //draw_screen_home(&framebuf_device);

        fb_close(&framebuf_device);
    }

    return 0;
}
