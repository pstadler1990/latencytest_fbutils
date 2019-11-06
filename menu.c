//
// Created by pstadler on 06.11.19.
//
#include <stdio.h>
#include <stdint.h>
#include <pigpio.h>
#include <printf.h>
#include "menu.h"
#include "configuration.h"


static int last_state_a = 0;
static unsigned int last_poll_time = 0;

static void menu_rot_changed(ROT_STATE state);
static void menu_poll(void);

void
menu_switch_pressed(void) {
    /* */
    printf("** Switch interrupt **\n");
}

void
menu_draw(struct FbDev* fb_device) {
    (void) fb_device;

    menu_poll();
}

static void
menu_rot_changed(ROT_STATE state) {
    /* */
    if(state == ROTSTATE_CLOCKWISE) {
        printf("clockwise\n");
    } else {
        printf("counter clockwise\n");
    }
}

static void
menu_poll(void) {
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
