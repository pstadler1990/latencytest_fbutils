//
// Created by pstadler on 06.11.19.
//
#include <stdio.h>
#include <stdint.h>
#include <pigpio.h>
#include <printf.h>
#include "menu.h"
#include "configuration.h"
#include "main.h"
#include "screens.h"
#include <unistd.h>

extern struct FbDevState framebuf_state;
extern bool mainIsRunning;
extern bool usbDriveInserted;

static int last_state_a = 0;
static unsigned int last_poll_time = ROT_DEBOUNCE_TIME;

static void usb_copy_files(void);

void
menu_switch_pressed(void) {
    /* */
    framebuf_state.mode = FBMODE_HOME;
    framebuf_state.state = FBSTATE_IDLE;
}

void
calib_switch_pressed(void) {
    /* */
    framebuf_state.mode = FBMODE_CALIB;
    framebuf_state.state = FBSTATE_INITIALIZE;
}

void
start_switch_pressed(void) {
    /* */
    if(!framebuf_state.isCalibrated) {
        return;
    }
    framebuf_state.mode = FBMODE_TEST;
    framebuf_state.state = FBSTATE_INITIALIZE;
}

void*
menu_poll(void* vargp) {
    /* Threaded function to be polled */
    while(mainIsRunning) {

        if(last_poll_time > 0) {
            last_poll_time--;
        }

        int a_state = gpioRead(GPIO_INT_ROT_A);
        int b_state = gpioRead(GPIO_INT_ROT_B);

        if(last_poll_time == 0 && a_state != last_state_a) {

            if(a_state != b_state) {
                menu_rot_changed(ROTSTATE_COUNTERCLOCKWISE);
            } else {
                menu_rot_changed(ROTSTATE_CLOCKWISE);
            }

            last_state_a = a_state;
            last_poll_time = ROT_DEBOUNCE_TIME;
        }
    }
    pthread_exit(NULL);
}

void*
usbdrive_poll(void* vargp) {
    /* Check whether usb drive is inserted and therefor mounted every 5 seconds */
    while(mainIsRunning) {

        if(framebuf_state.mode == FBMODE_HOME) {

            FILE *f_mount = popen("mount /dev/sda1 /media/usb/", "r");
            pclose(f_mount);

            FILE *f = popen("mount | grep /dev/sda1", "r");
            if (NULL != f) {

                if (EOF != fgetc(f)) {
                    puts("/dev/sda1 is mounted");

                    /* As soon as the drive is inserted and mounted, copy
                      all result files from the result dir onto the drive
                      (in home screen only!) */
                    usbDriveInserted = true;

                    usb_copy_files();

                    sleep(2);   // Needed for better UI visibility

                    /* umount drive again */
                    FILE *f_umount = popen("umount /media/usb/", "r");
                    pclose(f_umount);

                    usbDriveInserted = false;
                }
                pclose(f);
            }

            sleep(5);
        }
    }
    pthread_exit(NULL);
}

static void
usb_copy_files(void) {
    /* Copies all .csv files from the results dir onto the mounted usb drive */
    char dir[100];
    sprintf(dir, "cp -r %s/*.csv /media/usb/", RESULT_OUTPUT_DIR);
    FILE *f_mount = popen(dir, "r");
    pclose(f_mount);
}