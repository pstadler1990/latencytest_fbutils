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
#include "bcm_host.h"

extern struct FbDev framebuf_device;
extern struct FbDevState framebuf_state;
extern bool usbDriveInserted;
extern bool usbDriveCopied;

static uint32_t testNumber = 0;
static void vsync_func(DISPMANX_UPDATE_HANDLE_T u, void* arg);

bool vsync_flag = 0;
bool enable_cb = 0;
long int time_start;
long int time_end;
struct timespec gettime_now;

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
    char timing_str[30];

    sprintf(display_info_str, "Screen Resolution: %dx%d", w, h);
    sprintf(bpp_info_str, "Color depth: %d", fb_device->bpp);
    sprintf(display_name_str, "Display name: %s", framebuf_state.displayName);
    sprintf(timing_str, "Left margin: %d", fb_device->timing);

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
        fb_draw_text(fb_device, timing_str, 0, 60, COLOR_RED, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        if (!framebuf_state.isCalibrated) {
	    fb_draw_rect(fb_device, 0, 160, w / 2, 60, COLOR_RED, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            fb_draw_text(fb_device, "* Device is not calibrated yet - please calibrate first *", 0, 160, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        } else {
	    fb_draw_rect(fb_device, 0, 160, w / 2, 60, COLOR_BLUE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            fb_draw_text(fb_device, "- Ready! Press START to begin measurements -", 0, 160, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        }

        char test_number_str[50];
        sprintf(test_number_str, "Test number: %d", testNumber);
        fb_draw_rect(fb_device, 0, 100, w / 2, 20, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
        fb_draw_text(fb_device, test_number_str, 0, 100, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

	/* Color mode selection */
	char test_colorm_str[50];
	char color_mode_str[20];
	switch(framebuf_state.colorm) {
	    default:
	    case FBCOLOR_B2W:
		sprintf(color_mode_str, "Black to white");
		break;
	    case FBCOLOR_B2R:
		sprintf(color_mode_str, "Black to red");
		break;
	    case FBCOLOR_R2G:
		sprintf(color_mode_str, "Blue to green");
		break;
	    case FBCOLOR_SID:
		sprintf(color_mode_str, "SID mode");
		break;
	}
	sprintf(test_colorm_str, "Color mode: %s", color_mode_str);
	fb_draw_rect(fb_device, 0, 140, w / 2, 20, COLOR_YELLOW, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
	fb_draw_text(fb_device, test_colorm_str, 0, 140, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

        /* USB drive copy dialog */
        if(usbDriveInserted) {
            fb_draw_rect(fb_device, 0, 0, (w / 2) + 20, 120, COLOR_MAGENTA, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
	    if(!usbDriveCopied) {
		fb_draw_text(fb_device, "USB drive found.. Copying files..", 0, 0, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
            } else {
		fb_draw_text(fb_device, "Copied files to USB drive!", 0, 0, COLOR_BLUE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
	    }
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
}

void
draw_screen_test(struct FbDev* fb_device) {

#define BUF_SIZE ((uint32_t)DEFAULT_N_MEASUREMENTS * 2)
    uint32_t w = fb_device->w;
    uint32_t h = fb_device->h;

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T* display;
    display = vc_dispmanx_display_open(0/*DISPMANX_ID_HDMI0*/);
    vc_dispmanx_vsync_callback(display, NULL, NULL);
    vc_dispmanx_vsync_callback(display, vsync_func, fb_device);

    char receiveBuf[BUF_SIZE];

    double execTimes[BUF_SIZE];

    /* Test screen procedure */
    uint32_t m_done = 0;
    uint32_t m_failed = 0;  /* Failed measurements */
    bool m_failed_test = false;
    uint32_t single_m = 0;

    /* Wait for trigger reception from STM8 to synchronize the measurements */
    if(!uart_send_command(CTRL_CMD_TEST_MODE, false)) {
        printf("Failed to set test mode, exit!\n");
        return;
    }

    /* STM8 is in test mode */
    /* Turn on start switch LED */
    gpioWrite(GPIO_EXT_START_LED, 1);
    gpioWrite(GPIO_EXT_CALIB_LED, 0);

    /* Get measurements from slave device */
    struct Measurement measurements[DEFAULT_SINGLE_MEASURES][DEFAULT_N_MEASUREMENTS];
    uint32_t timestamp[DEFAULT_SINGLE_MEASURES];

    while(m_done < DEFAULT_SINGLE_MEASURES/*DEFAULT_N_MEASUREMENTS*/) {
        /* Show black screen */
	if(framebuf_state.colorm != FBCOLOR_R2G) {
	        fb_clear_screen(fb_device);
	} else {
		fb_draw_filled_screen(fb_device, COLOR_BLUE);
	}

        fb_update(fb_device);

        gpioWrite(GPIO_EXT_TRIGGER_OUT, 0);

       	switch(framebuf_state.colorm) {
		default:
		case FBCOLOR_B2W:
	 	    fb_draw_filled_screen(fb_device, COLOR_WHITE);
		    break;
		case FBCOLOR_B2R:
		    fb_draw_filled_screen(fb_device, COLOR_RED);
		    break;
		case FBCOLOR_R2G:
		    fb_draw_filled_screen(fb_device, COLOR_GREEN);
		    break;
	}

	usleep(200000);
	enable_cb = 1;
	/* Wait for VSYNC */
	while(!vsync_flag);

	printf("VSYNC received, white screen\n");

   	fb_update(fb_device);
	clock_gettime(CLOCK_REALTIME, &gettime_now);

	memset(receiveBuf, 0, sizeof(uint8_t) * (DEFAULT_N_MEASUREMENTS + 1));

	bool is_receiving = true;
	int receiveStatus = -1;
	uint32_t receivePtr = 0;
	bool has_package = false;
	uint32_t measurementTmpBuf[3];
	uint32_t m_index = 0;
	uint32_t lastReceivedTimeout = MEASUREMENT_TIMEOUT;

	while(is_receiving) {
		if((receivePtr >= (DEFAULT_N_MEASUREMENTS-2)) || --lastReceivedTimeout == 0) {
			is_receiving = false;
		}

		receiveStatus = uart_receive(receiveBuf, BUF_SIZE);

		if(receiveStatus != -1) {

			lastReceivedTimeout = MEASUREMENT_TIMEOUT;

			if(strncmp("{", receiveBuf, 1) == 0) {
				/* Begin of new measurement package */
				has_package = true;
				printf("New measurement data packet (%d)\n", receivePtr);
			} else if(strncmp("}", receiveBuf, 1) == 0) {
				/* End of measurement package */
				receivePtr++;
				has_package = false;
			} else {
				/* Should be measurement data */
				if(has_package && m_index < 2) {
					uint32_t num = (uint32_t) strtoll(receiveBuf, NULL, 10);
					measurementTmpBuf[m_index] = num;
					if(m_index + 1 < 2) {
						m_index++;
					} else {
						/* Received three values */
						measurements[single_m][receivePtr].tTrigger = measurementTmpBuf[0];
						measurements[single_m][receivePtr].tBlack = measurementTmpBuf[1];
						//measurements[receivePtr].tWhite = measurementTmpBuf[2];
						m_index = 0;
						printf("Measurement data -> time: %d, digit: %d\n", measurementTmpBuf[0], measurementTmpBuf[1]);
					}
				}
			}
		}
	};
	/* Receive final 50% reached timestamp */
	if(uart_receive_response(1, "T", false)) {
		printf("50 timestamp received..\n");	
	}
	bool isReceiving = true;
	char timestampBuf[10];
//	uint32_t timestamp = 0;
	while(isReceiving) {
		receiveStatus = uart_receive(timestampBuf, 10);
		if(receiveStatus != -1) {
			uint32_t num = (uint32_t) strtoll(timestampBuf, NULL, 10);
			if(num > 0) {
				isReceiving = false;
				timestamp[single_m] = num;
				printf("Timestamp received: %d\n", timestamp[single_m]);
			}
		}
	}
	execTimes[m_done] = (double)(gettime_now.tv_nsec - time_start) / 1.0e6;/*((double)(time_end - time_start) / (CLOCKS_PER_SEC / 1000));*/
	m_done++;
	vsync_flag = 0;
	enable_cb = 0;
	single_m++;
	printf("**** COMPLETED MEASUREMENT %d of %d\n", m_done, DEFAULT_SINGLE_MEASURES);
    }

    /* Completed measurements */
    fb_clear_screen(fb_device);
    fb_update(fb_device);

    /* Write to file */
    char fileName[128];
    sprintf(fileName, "%s/sid_%s_%d.csv", RESULT_OUTPUT_DIR, framebuf_state.displayName, testNumber);
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
        //fprintf(file, "Timestamp, Test#, Device name, tGesamt, tGesamtRaw, tUmschalt, tUmschaltRaw, tWhite, tBlack, execTimes\n");
		fprintf(file, "Timestamp, Test#, Device name, Single#, t, digit, reached 50 t\n");
    } else {
        printf("Could not open file %s, exit\n", fileName);
        return;
    }

    double tAvg, tAvgUmschalt;
    double tSum = 0;
    double tSumUmschalt = 0;
    uint32_t n = 0;
//    uint32_t mBufGesamt[DEFAULT_N_MEASUREMENTS - 4];
//    uint32_t mBufUmschalt[DEFAULT_N_MEASUREMENTS - 4];

    for(uint32_t i = 1; i < DEFAULT_SINGLE_MEASURES/*DEFAULT_N_MEASUREMENTS*/; i++) {
        /* Calculate average etc. */
//	if(execTimes[i] < 0 ||execTimes[i] > (measurements[i].tWhite - measurements[i].tTrigger)) {
//		execTimes[i] = 0;
//	}
//	execTimes[i] /= 2;

#ifdef REMOVE_ME
        double tGesamtRaw = measurements[i].tWhite - measurements[i].tTrigger;
        double tGesamt = (measurements[i].tWhite - measurements[i].tTrigger) - execTimes[i];
        uint32_t tGesamtDigit = measurements[i].tWhite - measurements[i].tTrigger;
        uint32_t tUmschaltRaw = measurements[i].tWhite - measurements[i].tBlack;
        double tUmschalt = (measurements[i].tWhite - measurements[i].tBlack) - execTimes[i];

	if(tGesamtRaw < 0) tGesamtRaw = 0;
	if(tGesamt < 0) tGesamt = 0;
	if(tGesamtDigit < 0) tGesamtDigit = 0;
	if(tUmschaltRaw <0) tUmschaltRaw = 0;
	if(tUmschalt < 0) tUmschalt = 0;

        if(i > 1 && i < DEFAULT_N_MEASUREMENTS - 2) {
            mBufGesamt[i] = tGesamtDigit;
            mBufUmschalt[i] = tUmschaltRaw;

            tSum += tGesamt;
            tSumUmschalt += tUmschalt;

            /* Write meta headers and actual data */
            fprintf(file, "%d, %d, %s, %f, %f, %f, %d, %d, %d, %f\n", (int)time(NULL), testNumber, framebuf_state.displayName, tGesamt, tGesamtRaw, tUmschalt, tUmschaltRaw, measurements[i].tWhite, measurements[i].tBlack, execTimes[i]);

            n++;
        }
#endif
	for(uint32_t j = 0; j < DEFAULT_N_MEASUREMENTS; j++) {
		fprintf(file, "%d, %d, %s, %d, %d, %d, %d\n", (int)time(NULL), testNumber, 
				framebuf_state.displayName,
				i,
				measurements[i][j].tTrigger,
				measurements[i][j].tBlack,
				timestamp[i]);
	}
    }

    /* Finished writing to data */
    fclose(file);

#ifdef REMOVE_ME
    tAvg = tSum / n;
    tAvgUmschalt = tSumUmschalt / n;

    /* Calculate median */
    uint32_t medianGesamt = median_u32(mBufGesamt, DEFAULT_N_MEASUREMENTS - 4);
    uint32_t medianUmschalt = median_u32(mBufUmschalt, DEFAULT_N_MEASUREMENTS - 4);

    /* Read min and max */
    int minGesamt = mBufGesamt[0];
    int maxGesamt = mBufGesamt[n];
    int minUmschalt = mBufUmschalt[0];
    int maxUmschalt = mBufUmschalt[n];

    if(minGesamt < 0) minGesamt = 0;
    if(maxGesamt < 0) maxGesamt = 0;
    if(minUmschalt < 0) minUmschalt = 0;
    if(maxUmschalt < 0) maxUmschalt = 0;
#endif
    /* Turn off start switch LED */
    gpioWrite(GPIO_EXT_START_LED, 0);

    while(framebuf_state.mode == FBMODE_TEST) {

        if(!m_failed_test) {
            //fb_draw_rect(fb_device, 0, 0, w / 2, 300, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);

            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_GREEN, DRAW_CENTER_HORIZONTAL);
            char m_complete_info[100];
            sprintf(m_complete_info, "Measurement valid (%d/%d failed)", m_failed, DEFAULT_SINGLE_MEASURES);
            fb_draw_text(fb_device, m_complete_info, 0, 40, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            /* Results */
            //char str_dataAvg[50];
            //sprintf(str_dataAvg, "Average display lag: %f ms", tAvg);
            //fb_draw_text(fb_device, str_dataAvg, 0, 120, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            //char str_dataAvgUmschalt[50];
            //sprintf(str_dataAvgUmschalt, "Average switching times: %f ms", tAvgUmschalt);
            //fb_draw_text(fb_device, str_dataAvgUmschalt, 0, 150, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            //char str_dataMedianGesamt[50];
            //sprintf(str_dataMedianGesamt, "Median total lag: %u ms", medianGesamt);
            //fb_draw_text(fb_device, str_dataMedianGesamt, 0, 180, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            //char str_dataMedianUmschalt[50];
            //sprintf(str_dataMedianUmschalt, "Median switching times: %u ms", medianUmschalt);
            //fb_draw_text(fb_device, str_dataMedianUmschalt, 0, 210, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

           // char str_dataMinMaxGesamt[50];
            //sprintf(str_dataMinMaxGesamt, "Min total lag: %u ms | Max total lag: %u ms", minGesamt, maxGesamt);
            //fb_draw_text(fb_device, str_dataMinMaxGesamt, 0, 240, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

//            char str_dataMinMaxUmschalt[50];
  //          sprintf(str_dataMinMaxUmschalt, "Min switching: %u ms | Max switching: %u ms", minUmschalt, maxUmschalt);
    //        fb_draw_text(fb_device, str_dataMinMaxUmschalt, 0, 270, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);
        } else {
            fb_draw_rect(fb_device, 0, 0, w / 2, 200, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "- Press START to retry -", 0, 145, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

            fb_draw_rect(fb_device, 0, 0, w / 2, 100, COLOR_RED, DRAW_CENTER_HORIZONTAL);
            fb_draw_text(fb_device, "Measurement invalid - too many failures", 0, 40, COLOR_WHITE, DRAW_CENTER_HORIZONTAL);
        }

        /* Update buffers */
        fb_update(fb_device);
    }
//#endif

}

void
draw_screen_calib_bw_digits(struct FbDev* fb_device) {
    /* Black and white digits calibration screen */
    uint32_t w = fb_device->w;
    uint32_t h = fb_device->h;

    framebuf_state.isCalibrated = false;

    if(!uart_send_command(CTRL_CMD_CALIB_MODE, false)) {
        /* Failed to set STM8 in calibration mode, exit */
	printf("FAILED TO ENABLE CALIB MODE\n");
        return;
    }

    printf("Calib mode on\n");

    /* Turn on calib switch LED */
    gpioWrite(GPIO_EXT_CALIB_LED, 1);
    gpioWrite(GPIO_EXT_START_LED, 0);

    bool m_failed_test = false;
    uint32_t lowerThreshold = 0, upperThreshold = 0;

    framebuf_state.state = FBSTATE_READY;

    /* STM8 is in calibration mode */
    while(1) {
	if(framebuf_state.colorm == FBCOLOR_SID) {
	    printf("SID Mode calibration\n");
 	    fb_draw_filled_screen(fb_device, COLOR_50GRAY);
	    fb_update(fb_device);
	    sleep(1);
	    if(!uart_send_command(CTRL_CMD_CALIB_BLACK, false)) {
		m_failed_test = true;
		break;
	    }
	}
       	else {

	        /* Show black screen */
	        fb_clear_screen(fb_device);
		if(framebuf_state.colorm != FBCOLOR_R2G) {
		    fb_clear_screen(fb_device);
		} else {
		    fb_draw_filled_screen(fb_device, COLOR_BLUE);
		}

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
	        switch(framebuf_state.colorm) {
		   default:
		   case FBCOLOR_B2W:
			fb_draw_filled_screen(fb_device, COLOR_WHITE);
			break;
		   case FBCOLOR_B2R:
			fb_draw_filled_screen(fb_device, COLOR_RED);
			break;
		   case FBCOLOR_R2G:
	       		fb_draw_filled_screen(fb_device, COLOR_GREEN);
			break;
		}
		//fb_draw_rect(fb_device, w/3, 0, w/3, h, COLOR_RED, DRAW_CENTER_NONE);
		//fb_draw_rect(fb_device, (w/3)*2, 0, w/3, h, COLOR_GREEN, DRAW_CENTER_NONE);
		fb_update(fb_device);
	        sleep(1);

	        if(!uart_send_command(CTRL_CMD_CALIB_WHITE, true)) {
	            m_failed_test = true;
	            break;
	        }
	}
	if(!uart_receive_response(8, "BLACK OK", false)) {
	    m_failed_test = true;
	    printf("Received no black ok\n");
	}

        if(!uart_receive_response(8, "CALIB OK", false)) {
            m_failed_test = true;
        }
	printf("Received CALIB OK\n");

        bool is_receiving = true;
        bool has_package = false;
        uint32_t lastReceivedTimeout = MEASUREMENT_TIMEOUT;
        int m_index = 0;
        char receiveBuf[100];

        while(is_receiving) {

            if(--lastReceivedTimeout == 0) {
                is_receiving = false;
            }

            int receiveStatus = uart_receive(receiveBuf, BUF_SIZE);

            if(receiveStatus != -1) {

                lastReceivedTimeout = MEASUREMENT_TIMEOUT;

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
                        if(m_index == 0) {
                            lowerThreshold = num;
                            m_index = 1;
                        } else {
                            upperThreshold = num;
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

            char str_dataLowerUpperThreshold[50];
            sprintf(str_dataLowerUpperThreshold, "Black: %d, White: %d", upperThreshold, lowerThreshold);
            fb_draw_text(fb_device, str_dataLowerUpperThreshold, 0, 140, COLOR_BLACK, DRAW_CENTER_HORIZONTAL);

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
		if(!framebuf_state.homesw_mode) {
                    /* Increase test number */
                    testNumber++;
		} else {
		   if(framebuf_state.colorm == 3) {
			framebuf_state.colorm = 0;
		   } else {
			   framebuf_state.colorm +=1;
		   }
		}
                break;
            default:
                break;
        }
    } else {
        /* counter clockwise */
        switch(framebuf_state.mode) {
            case FBMODE_HOME:
		if(!framebuf_state.homesw_mode) {
	                /* Decrease test number */
        	        if(testNumber == 0) {
                	    testNumber = 0;
               		} else {
                    	   testNumber--;
                	}
		} else {
	    		if(framebuf_state.colorm == 0) {
			    framebuf_state.colorm = 3;
			} else {
			    framebuf_state.colorm--;
			}
		}
                break;
            default:
                break;
        }

    }
}

void vsync_func(DISPMANX_UPDATE_HANDLE_T u, void* arg) {
	if(!enable_cb) return;
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	time_start = gettime_now.tv_nsec;
	gpioWrite(GPIO_EXT_TRIGGER_OUT, 1);
	vsync_flag = 1;
}
