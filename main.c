#include <stdio.h>
#include "fb_lib.h"
#include "screens.h"
#include "main.h"
#include "configuration.h"
#include "pinsetup.h"
#include <time.h>
#include "communication.h"
#include <unistd.h>
#include "menu.h"

struct FbDevState framebuf_state = {
        .state = FBSTATE_INITIALIZE,
        .n_measurements = DEFAULT_N_MEASUREMENTS,
        .mode = FBMODE_HOME,
};
int uart0_filestream = -1;

int
main() {
    struct FbDev framebuf_device;

    if(!fb_init("/dev/fb0", &framebuf_device)) {
        perror("Could not access framebuffer, exit");
        /* We don't need to process any errors here, as the framebuffer is crucial */
        return 1;
    } else {
        /* Frame buffer is opened */
#ifdef __arm__
        if(!init_GPIOs()) {
            perror("Could not open GPIOs, exit");
        }
#endif
        /* Initialize communication */
        if(!init_uart("/dev/ttyS0")) {
            perror("Could not initialize UART, exit");
        }

        /* Show screens */
        framebuf_state.state = FBSTATE_IDLE;

        while(1) {
            switch(framebuf_state.mode) {
                case FBMODE_HOME:
                default:
                    draw_screen_home(&framebuf_device);
                    break;
                case FBMODE_CALIB:
                    draw_screen_calib_bw_digits(&framebuf_device);
                    break;
                case FBMODE_TEST:
                    draw_screen_test(&framebuf_device);
                    break;
            }
        }

        //draw_screen_alternating(&framebuf_device);

        close(uart0_filestream);
        fb_close(&framebuf_device);
    }

    return 0;
}
