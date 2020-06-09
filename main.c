#include <stdio.h>
#include "fb_lib.h"
#include "screens.h"
#include "main.h"
#include "configuration.h"
#include "pinsetup.h"
#include <time.h>
#include "communication.h"
#include <unistd.h>
#include <pthread.h>
#include "menu.h"

struct FbDevState framebuf_state = {
        .state = FBSTATE_INITIALIZE,
		.homesw_mode = 0,
        .n_measurements = DEFAULT_N_MEASUREMENTS,
        .mode = FBMODE_HOME,
		.colorm = FBCOLOR_B2W,
        .isCalibrated = false,
};
int uart0_filestream = -1;

bool mainIsRunning = true;
bool usbDriveInserted = false;
bool usbDriveCopied = false;
struct FbDev framebuf_device;

int
main() {
//    struct FbDev framebuf_device;

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

        /* Read monitor name */
        if(com_get_display_name(framebuf_state.displayName, EDID_MAX_DISPLAY_NAME)) {
            printf("Display name successfully received: %s\n", framebuf_state.displayName);
        }

        pthread_t thread_menu;
        pthread_create(&thread_menu, NULL, menu_poll, NULL);

        pthread_t thread_usb;
        pthread_create(&thread_usb, NULL, usbdrive_poll, NULL);

        while(mainIsRunning) {
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

        pthread_join(thread_menu, NULL);
        pthread_join(thread_usb, NULL);

        close(uart0_filestream);
        fb_close(&framebuf_device);
    }

    return 0;
}
