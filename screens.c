//
// Created by pstadler on 05.10.19.
//
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "fb_lib.h"
#include "main.h"

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
        usleep(1000 / 60);
    }
}
