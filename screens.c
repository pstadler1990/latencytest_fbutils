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
#include "configuration.h"
#include <memory.h>
#include <errno.h>
#include <stdlib.h>
#include "communication.h"

extern struct FbDevState framebuf_state;

void
draw_screen_home(struct FbDev* fb_device) {
    uint32_t w = fb_device->w;
    uint32_t h = fb_device->h;

    struct timespec refresh_time_start;
    struct timespec refresh_time_end;

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

        clock_gettime(CLOCK_MONOTONIC_RAW, &refresh_time_start);
        __time_t delta_us = (refresh_time_end.tv_sec - refresh_time_start.tv_sec) * 1000000 + (refresh_time_end.tv_nsec - refresh_time_start.tv_nsec) / 1000;

        if(delta_us > (1000000 / 60)) {
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
            fb_draw_text(fb_device, display_info_str, 0, (h / 4) - 80, COLOR_BLACK,
                         DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            fb_draw_text(fb_device, bpp_info_str, 0, ((h / 4) - 60), COLOR_BLACK,
                         DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

            if (framebuf_state.state == FBSTATE_IDLE) {
                fb_draw_text(fb_device, "- Waiting for device. Place device on display -", 0, (h / 4) - 20, COLOR_RED,
                             DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            } else if (framebuf_state.state == FBSTATE_READY_FOR_MEASUREMENTS) {
                fb_draw_text(fb_device, "- Ready! Press START to begin measurements -", 0, (h / 4) - 20, COLOR_BLUE,
                             DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            }

            /* Bouncing rect animation */
            yy += ys;
            xx += xs;
            if ((xx + 100 == w) || (xx == 0)) {
                xs = -xs;
                bb_color_index += 1;
            }
            if ((yy + 100 == h) || (yy == 0)) {
                ys = -ys;
                bb_color_index += 1;
            }
            if (bb_color_index > 4) {
                bb_color_index = 0;
            }

            /* Update buffers */
            fb_update(fb_device);
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &refresh_time_end);
    }
}

void
draw_screen_test(struct FbDev* fb_device) {

#define BUF_SIZE ((uint32_t)100)

    uint32_t w = fb_device->w;
    uint32_t h = fb_device->h;
    char receiveBuf[BUF_SIZE];

    /* Test screen procedure */
    uint32_t m_done = 0;
    uint32_t m_failed = 0;  /* Failed measurements */
    bool m_failed_test = false;

    /* Wait for trigger reception from STM8 to synchronize the measurements */
    if(!uart_send_command(CTRL_CMD_TEST_MODE, false)) {
        printf("Failed to set test mode, exit!\n");
        return;
    }

    /* STM8 is in test mode */

    while(m_done < DEFAULT_N_MEASUREMENTS) {
	printf("Measurement %d\n", m_done);
        /* Show black screen */
        fb_clear_screen(fb_device);
        fb_update(fb_device);
        sleep(1);

        fb_draw_filled_screen(fb_device, COLOR_WHITE);
	//fb_update(fb_device);

        /*  Send TRIGGER to STM8 */
        gpioWrite(GPIO_EXT_TRIGGER_OUT, 0);
        usleep(100);
        gpioWrite(GPIO_EXT_TRIGGER_OUT, 1);
        clock_t time_start = clock();

	//usleep(100);
        fb_update(fb_device);

        /* Wait until MEAS_COMPLETE pin is set by STM8 */
        if(!uart_receive_response(7, "MEAS OK", false)) {
            if(++m_failed >= MAX_FAILED_MEASUREMENTS) {
                m_failed_test = true;
                break;
            }
        }
        m_done++;
    }

    /* Completed measurements */
    fb_clear_screen(fb_device);
    fb_update(fb_device);

    printf("**** MEASURE DONE ****\n");

    memset(receiveBuf, 0, sizeof(uint8_t) * (DEFAULT_N_MEASUREMENTS + 1));

    /* Get measurements from slave device */
    if(!uart_send_command(CTRL_CMD_GET_MEASURES, false)) {
        printf("Failed to set measurement command, exit!\n");
        return;
    }
    printf("*** RECEIVING ***\n");

    struct Measurement measurements[DEFAULT_N_MEASUREMENTS];

    bool is_receiving = true;
    int receiveStatus = -1;
    uint32_t receivePtr = 0;
    bool has_package = false;
    uint32_t measurementTmpBuf[3];
    uint32_t m_index = 0;
    uint32_t lastReceivedTimeout = MEASUREMENT_TIMEOUT;

    while(is_receiving) {

        if((receivePtr + 1 > DEFAULT_N_MEASUREMENTS) || --lastReceivedTimeout == 0) {
            is_receiving = false;
        }

        receiveStatus = uart_receive(receiveBuf, BUF_SIZE);

        if(receiveStatus != -1) {

            lastReceivedTimeout = MEASUREMENT_TIMEOUT;

            if(strncmp("{", receiveBuf, 1) == 0) {
                /* Begin of new measurement package */
                has_package = true;
            } else if(strncmp("}", receiveBuf, 1) == 0) {
                /* End of measurement package */
                receivePtr++;
                has_package = false;
            } else {
                /* Should be measurement data */
                if(has_package && m_index < 3) {
                    uint32_t num = (uint32_t) strtoll(receiveBuf, NULL, 10);
                    measurementTmpBuf[m_index] = num;
                    if(m_index + 1 < 3) {
                        m_index++;
                    } else {
                        /* Received three values */
                        measurements[receivePtr].tTrigger = measurementTmpBuf[0];
                        measurements[receivePtr].tBlack = measurementTmpBuf[1];
                        measurements[receivePtr].tWhite = measurementTmpBuf[2];
                        m_index = 0;
		            }
                }
            }
        }
    };
    printf("received measurement data\n");

    for(uint32_t i = 0; i < DEFAULT_N_MEASUREMENTS; i++) {
	    //printf("Receive (%d) -> %d %d %d\n", i, measurements[i].tTrigger, measurements[i].tBlack, measurements[i].tWhite);
	    uint32_t tGesamt = measurements[i].tWhite - measurements[i].tTrigger;
	    uint32_t tUmschalt = measurements[i].tWhite - measurements[i].tBlack;
        printf("[%d] -> t_gesamt: %d ms \t t_umschalt: %d ms\n", i, tGesamt, tUmschalt);
    }
    return;

    while(framebuf_state.state == FBSTATE_IDLE) {

        if(!m_failed_test) {
            fb_draw_rect(fb_device, 0, 0, w / 2, h / 2, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);

            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_GREEN, DRAW_CENTER_HORIZONTAL);
            char m_complete_info[100];
            sprintf(m_complete_info, "Measurement valid (%d/%d failed)", m_failed, DEFAULT_N_MEASUREMENTS);

            fb_draw_text(fb_device, m_complete_info, 0, 40, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            /* Show results from UART */
            for(uint32_t i=0; i < DEFAULT_N_MEASUREMENTS + 1; i++) {
                char str_data[50];
                sprintf(str_data, "(%d): %d", i, receiveBuf[i]);

                fb_draw_text(fb_device, str_data, 0, 60 + (i * 20), COLOR_BLACK, DRAW_CENTER_HORIZONTAL);
            }

        } else {
            fb_draw_rect(fb_device, 0, 0, w / 2, 200, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "- Press START to retry -", 0, 145, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_RED, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "Measurement invalid - too many failures", 0, 40, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);

            // TODO: Show text "Press START to retry measurements"
        }

        /* Update buffers */
        fb_update(fb_device);
        usleep(10000 / 60);
    }
}

void
draw_screen_calib_bw_digits(struct FbDev* fb_device) {
    /* Black and white digits calibration screen */
    uint32_t w = fb_device->w;
    uint32_t h = fb_device->h;

    if(!uart_send_command(CTRL_CMD_CALIB_MODE, true)) {
        /* Failed to set STM8 in calibration mode, exit */
        return;
    }

    bool m_failed_test = false;

    /* STM8 is in calibration mode */
    while(1) {
        /* Show black screen */
        fb_clear_screen(fb_device);
        fb_update(fb_device);
        sleep(1);

        if(!uart_send_command(CTRL_CMD_CALIB_BLACK, false)) {
            m_failed_test = true;
            break;
        }

        /* Wait for device's response */
        if(!uart_receive_response(8, "BLACK OK", false)) {
            m_failed_test = true;
            break;
        }

        /* Show white screen */
        fb_draw_filled_screen(fb_device, COLOR_WHITE);
        fb_update(fb_device);
        sleep(1);

        if(!uart_send_command(CTRL_CMD_CALIB_WHITE, true)) {
            m_failed_test = true;
            break;
        }
        if(!uart_receive_response(8, "CALIB OK", false)) {
            m_failed_test = true;
        }
        break;
    }

    /* Show calibration test status on screen */
    fb_clear_screen(fb_device);
    fb_update(fb_device);

    while(framebuf_state.state == FBSTATE_IDLE) {
        if(!m_failed_test) {
            fb_draw_rect(fb_device, 0, 0, w / 2, 200, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_GREEN, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "Calibration successful!", 0, 40, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "The device has been successfully calibrated!", 0, 140, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

        } else {
            fb_draw_rect(fb_device, 0, 0, w / 2, 200, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "- Press START to retry -", 0, 145, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_RED, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "Calibration failed!", 0, 40, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
        }

        fb_update(fb_device);
        usleep(10000 / 60);
    }

}

void
draw_screen_alternating(struct FbDev* fb_device) {
    /* Black and white alternating screen without timeouts */
    while(1) {
        /* Show black screen */
        fb_clear_screen(fb_device);
        fb_update(fb_device);
        sleep(2);

        /* Show white screen */
        fb_draw_filled_screen(fb_device, COLOR_WHITE);
        fb_update(fb_device);
        sleep(2);
    }
}
