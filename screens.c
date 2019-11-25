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
#include "calculations.h"
#include "menu.h"

extern struct FbDevState framebuf_state;

static uint32_t testNumber = 0;

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
    char display_name_str[EDID_MAX_DISPLAY_NAME + 20];

    sprintf(display_info_str, "Screen Resolution: %dx%d", w, h);
    sprintf(bpp_info_str, "Color depth: %d", fb_device->bpp);
    sprintf(display_name_str, "Display name: %s", framebuf_state.displayName);

    /* Drawing idle / welcome screen
       This screen can only be exit on external triggers
       - START button fires next state: FBSTATE_TRIGGERED
       - Rotary encoder (navigation) changes settings */
    while(framebuf_state.mode == FBMODE_HOME) {

        fb_draw_rect(fb_device, xx, yy, 100, 100, bb_colors[bb_color_index], DRAW_CENTER_NONE);

        fb_draw_line(fb_device, 0, 0, w, h, COLOR_WHITE);
        fb_draw_line(fb_device, w, 0, 0, h, COLOR_WHITE);
        fb_draw_rect(fb_device, 0, 0, w / 2, h / 2, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        fb_draw_rect(fb_device, 0, 0, 30, 30, COLOR_RED, DRAW_CENTER_NONE);
        fb_draw_rect(fb_device, w - 30, 0, 30, 30, COLOR_GREEN, DRAW_CENTER_NONE);
        fb_draw_rect(fb_device, 0, h - 30, 30, 30, COLOR_BLUE, DRAW_CENTER_NONE);
        fb_draw_rect(fb_device, w - 30, h - 30, 30, 30, COLOR_YELLOW, DRAW_CENTER_NONE);

        /* Texts */
        fb_draw_text(fb_device, display_name_str, 0, 0, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        fb_draw_text(fb_device, display_info_str, 0, 20, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        fb_draw_text(fb_device, bpp_info_str, 0, 40, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        if (!framebuf_state.isCalibrated) {
	    fb_draw_rect(fb_device, 0, 140, w / 2, 60, COLOR_RED, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            fb_draw_text(fb_device, "* Device is not calibrated yet - please calibrate first *", 0, 140, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        } else {
	    fb_draw_rect(fb_device, 0, 140, w / 2, 60, COLOR_BLUE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            fb_draw_text(fb_device, "- Ready! Press START to begin measurements -", 0, 140, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        }

        char test_number_str[50];
        sprintf(test_number_str, "Test number: %d", testNumber);
        fb_draw_rect(fb_device, 0, 100, w / 2, 20, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        fb_draw_text(fb_device, test_number_str, 0, 100, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

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
}

void
draw_screen_test(struct FbDev* fb_device) {

#define BUF_SIZE ((uint32_t)DEFAULT_N_MEASUREMENTS * 2)

    uint32_t w = fb_device->w;
    uint32_t h = fb_device->h;
    char receiveBuf[BUF_SIZE];

    double execTimes[BUF_SIZE];

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
    /* Turn on start switch LED */
    gpioWrite(GPIO_EXT_START_LED, 1);
    gpioWrite(GPIO_EXT_CALIB_LED, 0);

    while(m_done < DEFAULT_N_MEASUREMENTS) {
        /* Show black screen */
        fb_clear_screen(fb_device);
        fb_update(fb_device);

        gpioWrite(GPIO_EXT_TRIGGER_OUT, 0);

        srand(time(0));
        usleep(((rand() % 500) + 1000) * 1000);

        fb_draw_filled_screen(fb_device, COLOR_WHITE);
        //fb_draw_rect(fb_device, 0, 0, 150, 150, COLOR_WHITE, DRAW_CENTER_NONE);
        //fb_draw_rect(fb_device, w - 150, h - 150, 150, 150, COLOR_WHITE, DRAW_CENTER_NONE);

        /*  Send TRIGGER to STM8 */
        gpioWrite(GPIO_EXT_TRIGGER_OUT, 1);

        fb_update(fb_device);
        clock_t time_start = clock();

        /* Wait until MEAS_COMPLETE pin is set by STM8 */
        if(!uart_receive_response(7, "MEAS OK", false)) {
            if(++m_failed >= MAX_FAILED_MEASUREMENTS) {
                m_failed_test = true;
                break;
            }
        }
        clock_t time_end = clock();
        execTimes[m_done] = ((double)(time_end - time_start) / CLOCKS_PER_SEC);
        m_done++;
    }

    /* Completed measurements */
    fb_clear_screen(fb_device);
    fb_update(fb_device);

    memset(receiveBuf, 0, sizeof(uint8_t) * (DEFAULT_N_MEASUREMENTS + 1));

    /* Get measurements from slave device */
    if(!uart_send_command(CTRL_CMD_GET_MEASURES, false)) {
        printf("Failed to set measurement command, exit!\n");
        return;
    }

    struct Measurement measurements[N_CHANNELS][DEFAULT_N_MEASUREMENTS];

    bool is_receiving = true;
    int receiveStatus = -1;
    uint32_t receivePtr = 0;
    uint32_t channelPtr = 0;
    bool has_package = false;
    uint32_t measurementTmpBuf[3];
    uint32_t m_index = 0;
    uint32_t p_index = 0;
    uint32_t lastReceivedTimeout = MEASUREMENT_TIMEOUT;

    while(is_receiving) {
        if((receivePtr + 1 > DEFAULT_N_MEASUREMENTS) || --lastReceivedTimeout == 0) {
            is_receiving = false;
        }

        receiveStatus = uart_receive(receiveBuf, BUF_SIZE);

        if(receiveStatus != -1) {

            lastReceivedTimeout = MEASUREMENT_TIMEOUT;

            // { t1, b1, w1, t2, b2, w2, t3, b3, w3}, ... { ... } for N = all measurements
            if(strncmp("{", receiveBuf, 1) == 0) {
                /* Begin of new measurement package */
                has_package = true;
            } else if(strncmp("}", receiveBuf, 1) == 0) {
                /* End of measurement package */
                receivePtr++;
                has_package = false;
            } else {
                /* Should be measurement data */
                if(has_package && m_index < (3 * N_CHANNELS)) {
                    uint32_t num = (uint32_t) strtoll(receiveBuf, NULL, 10);

                    if(p_index == 0) {
                        measurements[channelPtr][receivePtr].tTrigger = num;
                        p_index = 1;
                    } else if(p_index == 1) {
                        measurements[channelPtr][receivePtr].tBlack = num;
                        p_index = 2;
                    } else {
                        measurements[channelPtr][receivePtr].tWhite = num;
                        p_index = 0;

                        if(channelPtr + 1 < N_CHANNELS) {
                            channelPtr++;
                        } else {
                            channelPtr = 0;
                        }
                    }

                    if(m_index + 1 < (3 * N_CHANNELS)) {
                        m_index++;
                    } else {
                        m_index = 0;
                    }
                }
            }
        }
    };

    /* Write to file */
    char fileName[128];
    sprintf(fileName, "%s/%s_%d.txt", RESULT_OUTPUT_DIR, framebuf_state.displayName, testNumber);
    FILE* file = fopen(fileName, "w");
    if(file) {
        // [ META DATA ]
        // # Test: <testNumber>
        // # Monitor: <framebuf_state.displayName>
        // [ MEASUREMENT DATA ]
        // Timestamp, Test#, Device name, tGesamt, tUmschalt
        // ..., ...
        // ...

        /* Write data labels */
        fprintf(file, "Timestamp, Test#, Device name, tGesamt, tUmschalt, ");

        for(uint32_t c = 0; c < N_CHANNELS; c++) {
            fprintf(file, "Sensor %d, ", c);
        }

    } else {
        printf("Could not open file %s, exit\n", fileName);
        return;
    }

    double tAvg, tAvgUmschalt;
    double tSum = 0;
    double tSumUmschalt = 0;
    uint32_t n = 0;
    uint32_t mBufGesamt[N_CHANNELS][DEFAULT_N_MEASUREMENTS - 4];
    uint32_t mBufUmschalt[N_CHANNELS][DEFAULT_N_MEASUREMENTS - 4];

    for(uint32_t i = 0; i < DEFAULT_N_MEASUREMENTS; i++) {
        /* Calculate average etc. */
        double tGesamt[N_CHANNELS];
        uint32_t tGesamtDigit[N_CHANNELS];
        uint32_t tUmschalt[N_CHANNELS];

        /* Write meta headers and actual data */
        fprintf(file, "%d, %d, %s, ", (int)time(NULL), testNumber, framebuf_state.displayName);

        for(uint32_t c = 0; c < N_CHANNELS; c++) {
            tGesamt[c] = (measurements[c][i].tWhite - measurements[c][i].tTrigger) - execTimes[i];
            tGesamtDigit[c] = measurements[c][i].tWhite - measurements[c][i].tTrigger;
            tUmschalt[c] = measurements[c][i].tWhite - measurements[c][i].tBlack;

            if(i > 1 && i < DEFAULT_N_MEASUREMENTS - 2) {
                mBufGesamt[c][i] = tGesamtDigit[c];
                mBufUmschalt[c][i] = tUmschalt[c];

                tSum += tGesamt[c];
                tSumUmschalt += tUmschalt[c];

                fprintf(file, "%f, %d, ", tGesamt[c], tUmschalt[c]);

                n++;
            }
        }
    }

    /* Finished writing to data */
    fclose(file);

    tAvg = tSum / n;
    tAvgUmschalt = tSumUmschalt / n;

    /* Calculate median */
    uint32_t medianGesamt[N_CHANNELS];
    uint32_t medianUmschalt[N_CHANNELS];

    uint32_t medianGesamtAvg, medianGesamtSum = 0;
    uint32_t medianUmschaltAvg, medianUmschaltSum = 0;

    for(uint32_t c = 0; c < N_CHANNELS; c++) {
        medianGesamt[c] = median_u32(mBufGesamt[c], DEFAULT_N_MEASUREMENTS - 4);
        medianUmschalt[c] = median_u32(mBufUmschalt[c], DEFAULT_N_MEASUREMENTS - 4);

        medianGesamtSum += medianGesamt[c];
        medianUmschaltSum += medianUmschalt[c];
    }
    medianGesamtAvg = medianGesamtSum / N_CHANNELS;
    medianUmschaltAvg = medianUmschaltSum / N_CHANNELS;

    /* Read min and max */
    int minGesamt[N_CHANNELS];
    int maxGesamt[N_CHANNELS];
    int minUmschalt[N_CHANNELS];
    int maxUmschalt[N_CHANNELS];

    for(uint32_t c = 0; c < N_CHANNELS; c++) {
        minGesamt[c] = mBufGesamt[c][0];
        maxGesamt[c] = mBufGesamt[c][n];
        minUmschalt[c] = mBufUmschalt[c][0];
        maxUmschalt[c] = mBufUmschalt[c][n];
    }

    /* Turn off start switch LED */
    gpioWrite(GPIO_EXT_START_LED, 0);

    while(framebuf_state.mode == FBMODE_TEST) {

        if(!m_failed_test) {
            fb_draw_rect(fb_device, 0, 0, w / 2, 300, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);

            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_GREEN, DRAW_CENTER_HORIZONTAL);
            char m_complete_info[100];
            sprintf(m_complete_info, "Measurement valid (%d/%d failed)", m_failed, DEFAULT_N_MEASUREMENTS);
            fb_draw_text(fb_device, m_complete_info, 0, 40, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            /* Results */
            char str_dataAvg[50];
            sprintf(str_dataAvg, "Average display lag: %f ms", tAvg);
            fb_draw_text(fb_device, str_dataAvg, 0, 120, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            char str_dataAvgUmschalt[50];
            sprintf(str_dataAvgUmschalt, "Average switching times: %f ms", tAvgUmschalt);
            fb_draw_text(fb_device, str_dataAvgUmschalt, 0, 150, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            // Channels
            for(uint32_t c = 0; c < N_CHANNELS; c++) {
                char str_dataMedianGesamt[100];
                sprintf(str_dataMedianGesamt, "Median total lag: %u ms (sensor %d)", medianGesamt[c], c);
                fb_draw_text(fb_device, str_dataMedianGesamt, 0, 180 + (c * 20), COLOR_BLACK, DRAW_CENTER_HORIZONTAL);
            }

            for(uint32_t c = 0; c < N_CHANNELS; c++) {
                char str_dataMedianUmschalt[100];
                sprintf(str_dataMedianUmschalt, "Median switching times: %u ms (sensor %d)", medianUmschalt[c], c);
                fb_draw_text(fb_device, str_dataMedianUmschalt, 0, 210 + (c * 20), COLOR_BLACK, DRAW_CENTER_HORIZONTAL);
            }

            for(uint32_t c = 0; c < N_CHANNELS; c++) {
                char str_dataMinMaxGesamt[100];
                sprintf(str_dataMinMaxGesamt, "Min total lag: %u ms | Max total lag: %u ms (sensor %d)", minGesamt[c], maxGesamt[c], c);
                fb_draw_text(fb_device, str_dataMinMaxGesamt, 0, 240 + (c* 20), COLOR_BLACK, DRAW_CENTER_HORIZONTAL);
            }

//            char str_dataMinMaxUmschalt[50];
//            sprintf(str_dataMinMaxUmschalt, "Min switching: %u ms | Max switching: %u ms", minUmschalt, maxUmschalt);
//            fb_draw_text(fb_device, str_dataMinMaxUmschalt, 0, 270, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);
        } else {
            fb_draw_rect(fb_device, 0, 0, w / 2, 200, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "- Press START to retry -", 0, 145, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_RED, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "Measurement invalid - too many failures", 0, 40, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
        }

        /* Update buffers */
        fb_update(fb_device);
    }
}

void
draw_screen_calib_bw_digits(struct FbDev* fb_device) {
    /* Black and white digits calibration screen */
    uint32_t w = fb_device->w;

    framebuf_state.isCalibrated = false;

    if(!uart_send_command(CTRL_CMD_CALIB_MODE, true)) {
        /* Failed to set STM8 in calibration mode, exit */
        return;
    }

    /* Turn on calib switch LED */
    gpioWrite(GPIO_EXT_CALIB_LED, 1);
    gpioWrite(GPIO_EXT_START_LED, 0);

    bool m_failed_test = false;
    uint32_t lowerThreshold[N_CHANNELS];
    uint32_t upperThreshold[N_CHANNELS];

    framebuf_state.state = FBSTATE_READY;

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

        bool is_receiving = true;
        bool has_package = false;
        int m_index = 0;    // 0 < m_index < 3
        int p_index = 0;    // 0 < p_index < 2
        char receiveBuf[100];

        while(is_receiving) {

            int receiveStatus = uart_receive(receiveBuf, BUF_SIZE);
            if(receiveStatus != -1) {

                // { b1, w1, b2, w2, b3, w3}
                if(strncmp("{", receiveBuf, 1) == 0) {
                    /* Begin of new calibration value package */
                    has_package = true;
                } else if(strncmp("}", receiveBuf, 1) == 0) {
                    /* End of measurement package */
                    is_receiving = false;
                } else {
                    /* Should be measurement data */
                    if(has_package) {
                        uint32_t num = (uint32_t) strtoll(receiveBuf, NULL, 10);

                        if(p_index == 0) {
                            upperThreshold[m_index] = num;
                            p_index = 1;
                        } else {
                            lowerThreshold[m_index] = num;
                            if(m_index + 1 < N_CHANNELS) {
                                m_index++;
                            }
                            p_index = 0;
                        }
                    }
                }
            }
        }

        break;
    }

    /* Show calibration test status on screen */
    fb_clear_screen(fb_device);
    fb_update(fb_device);

    framebuf_state.state = FBSTATE_IDLE;

    /* Turn off calib switch LED */
    gpioWrite(GPIO_EXT_CALIB_LED, 0);

    framebuf_state.isCalibrated = true;

    while(framebuf_state.mode == FBMODE_CALIB) {
        if(!m_failed_test) {
            fb_draw_rect(fb_device, 0, 0, w / 2, 200, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_GREEN, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "Calibration successful!", 0, 40, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            for(uint32_t c = 0; c < N_CHANNELS; c++) {
                char str_dataLowerUpperThreshold[50];
                sprintf(str_dataLowerUpperThreshold, "Black: %d, White: %d", upperThreshold[c], lowerThreshold[c]);
                fb_draw_text(fb_device, str_dataLowerUpperThreshold, 0, 140 + (c * 20), COLOR_BLACK, DRAW_CENTER_HORIZONTAL);
            }

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

void
menu_rot_changed(ROT_STATE state) {
    /* */
    if(state == ROTSTATE_CLOCKWISE) {
        /* clockwise */
        switch(framebuf_state.mode) {
            case FBMODE_HOME:
                /* Increase test number */
                testNumber++;
                break;
            default:
                break;
        }
    } else {
        /* counter clockwise */
        switch(framebuf_state.mode) {
            case FBMODE_HOME:
                /* Decrease test number */
                if(testNumber == 0) {
                    testNumber = 0;
                } else {
                    testNumber--;
                }
                break;
            default:
                break;
        }

    }
}
