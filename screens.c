//
// Created by pstadler on 05.10.19.
//
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <pigpio.h>
#include <stdbool.h>
#include "fb_lib.h"
#include "main.h"
#include <unistd.h>
#include "configuration.h"

extern struct FbDevState framebuf_state;

void
draw_screen_home(struct FbDev* fb_device) {
    uint32_t w = fb_device->w;
    uint32_t h = fb_device->h;

    /* Bouncing rect */
    uint8_t bb_color_index = 0;
    const uint32_t bb_colors[5] = {
            COLOR_GREEN,
            COLOR_MAGENTA,
            COLOR_YELLOW,
            COLOR_RED,
            COLOR_BLUE
    };
    uint32_t yy = h / 2;
    uint32_t xx = w / 2;
    int32_t xs = 5;
    int32_t ys = 5;

    char display_info_str[50];
    char bpp_info_str[20];

    sprintf(display_info_str, "Screen Resolution: %dx%d", w, h);
    sprintf(bpp_info_str, "Color depth: %d", fb_device->bpp);

    /* Drawing idle / welcome screen
       This screen can only be exit on external triggers
       - START button fires next state: FBSTATE_TRIGGERED
       - Rotary encoder (navigation) changes settings */
    while(framebuf_state.state == FBSTATE_IDLE) {
        /* Bouncing rect */
        fb_draw_rect(fb_device, xx, yy, 100, 100, bb_colors[bb_color_index], DRAW_CENTER_NONE);

        fb_draw_line(fb_device, 0, 0, w, h, COLOR_WHITE);
        fb_draw_line(fb_device, w, 0, 0, h, COLOR_WHITE);
        fb_draw_rect(fb_device, 0, 0, w / 2, h / 2, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        fb_draw_rect(fb_device, 0, 0, 30, 30, COLOR_RED, DRAW_CENTER_NONE);
        fb_draw_rect(fb_device, w - 30, 0, 30, 30, COLOR_GREEN, DRAW_CENTER_NONE);
        fb_draw_rect(fb_device, 0, h - 30, 30, 30, COLOR_BLUE, DRAW_CENTER_NONE);
        fb_draw_rect(fb_device, w - 30, h - 30, 30, 30, COLOR_YELLOW, DRAW_CENTER_NONE);

        /* Texts */
        fb_draw_text(fb_device, display_info_str, 0, (h / 4) - 80, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        fb_draw_text(fb_device, bpp_info_str, 0, ((h / 4) - 60), COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        if(framebuf_state.state == FBSTATE_IDLE) {
            fb_draw_text(fb_device, "- Waiting for device. Place device on display -", 0, (h / 4) - 20, COLOR_RED, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        } else if(framebuf_state.state == FBSTATE_READY_FOR_MEASUREMENTS) {
            fb_draw_text(fb_device, "- Ready! Press START to begin measurements -", 0, (h / 4) - 20, COLOR_BLUE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        }

        /* Bouncing rect animation */
        yy += ys;
        xx += xs;
        if((xx + 100 == w) || (xx == 0)) {
            xs = -xs;
            bb_color_index += 1;
        }
        if((yy + 100 == h) || (yy == 0)) {
            ys = -ys;
            bb_color_index += 1;
        }
        if(bb_color_index > 4) {
            bb_color_index = 0;
        }

        /* Update buffers */
        fb_update(fb_device);
        usleep(10000 / 60);
    }
}

void
draw_screen_test(struct FbDev* fb_device) {

    uint32_t w = fb_device->w;
    uint32_t h = fb_device->h;

    printf("Test mode\n");

    /* Test screen procedure */
    bool m_is_processing = true;
    uint32_t m_done = 0;
    uint32_t m_timeout = 1000; // TODO: define in configuration!

    gpioWrite(GPIO_EXT_MODE_DIGITAL_OUT, 0);

    sleep(1);

    gpioWrite(GPIO_EXT_MODE_DIGITAL_OUT, 1);
    while(!gpioRead(GPIO_EXT_TRIGGER_IN)) {
        printf("..\n");
        if(--m_timeout == 0) {
            printf("Failed to set digital mode, exit\n");
            break;
        }
    }
    printf("...done\n");

    while(m_done < DEFAULT_N_MEASUREMENTS && m_is_processing) {

        // 1. Show black screen
        fb_clear_screen(fb_device);
        m_timeout = 1000;

        // 2. Send TRIGGER to STM8
        gpioWrite(GPIO_EXT_TRIGGER_IN, 1);

        // (2a) STM8 starts time measurement
        // (2b) STM8 waits for Interrupt

        // 3. Show white screen
        fb_draw_filled_screen(fb_device, COLOR_WHITE);

        // 4. Wait until MEAS_COMPLETE pin is set by STM8
        while(!gpioRead(GPIO_EXT_TRIGGER_IN)) {
            if(--m_timeout == 0) {
                // TODO: Failed test, stop
                m_is_processing = false;
                break;
            }
        }
    }

}
