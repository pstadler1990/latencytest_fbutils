//
// Created by pstadler on 06.10.19.
//

#include <stdio.h>
#include "pinsetup.h"
#include "pigpio.h"
#include "configuration.h"
#include <unistd.h>
#include <stdlib.h>
#include "menu.h"

static void _rot_switch_isr(int gpio, int level, uint32_t tick);
static void _calib_switch_isr(int gpio, int level, uint32_t tick);
static void _start_switch_isr(int gpio, int level, uint32_t tick);


int8_t
init_GPIOs(void) {
    /* */
    if (gpioInitialise() < 0) {
        return -1;
    }

    /* Configure pins for exchange with external uC (STM8) */
    gpioSetMode(GPIO_EXT_TRIGGER_IN, PI_INPUT);

    /* OUT trigger */
    gpioSetMode(GPIO_EXT_TRIGGER_OUT, PI_OUTPUT);
    gpioSetPullUpDown(GPIO_EXT_TRIGGER_OUT, PI_PUD_OFF);

    /* Rotary encoder A, IN digital */
    gpioSetMode(GPIO_INT_ROT_A, PI_INPUT);
    gpioSetPullUpDown(GPIO_INT_ROT_A, PI_PUD_UP);

    /* Rotary encoder B, IN digital */
    gpioSetMode(GPIO_INT_ROT_B, PI_INPUT);
    gpioSetPullUpDown(GPIO_INT_ROT_B, PI_PUD_UP);

    /* Rotary encoder switch, IN digital */
    gpioSetMode(GPIO_INT_ROT_SW, PI_INPUT);
    gpioSetISRFunc(GPIO_INT_ROT_SW, RISING_EDGE, -1, _rot_switch_isr);

    /* Calibration switch, IN digital */
    gpioSetMode(GPIO_INT_CALIB_SW, PI_INPUT);
    gpioSetISRFunc(GPIO_INT_CALIB_SW, RISING_EDGE, -1, _calib_switch_isr);

    /* Start switch, IN digital */
    gpioSetMode(GPIO_INT_START_SW, PI_INPUT);
    gpioSetISRFunc(GPIO_INT_START_SW, RISING_EDGE, -1, _start_switch_isr);

    /* Calib switch LED MODE_DIGITAL */
    gpioSetMode(GPIO_EXT_CALIB_LED, PI_OUTPUT);
    gpioSetPullUpDown(GPIO_EXT_CALIB_LED, PI_PUD_UP);
    gpioWrite(GPIO_EXT_CALIB_LED, 0);

    /* Start switch LED MODE_DIGITAL */
    gpioSetMode(GPIO_EXT_START_LED, PI_OUTPUT);
    gpioSetPullUpDown(GPIO_EXT_START_LED, PI_PUD_UP);
    gpioWrite(GPIO_EXT_START_LED, 0);

    return 1;
}

void
_rot_switch_isr(int gpio, int level, uint32_t tick) {
    /* Interrupt service routine ISR for rotary encoder switch */
    if(gpioRead(GPIO_INT_ROT_SW)) {
        menu_switch_pressed();
    }
}

void
_calib_switch_isr(int gpio, int level, uint32_t tick) {
    /* Interrupt service routine ISR for calibration switch */
    if(gpioRead(GPIO_INT_CALIB_SW)) {
        calib_switch_pressed();
    }
}

void
_start_switch_isr(int gpio, int level, uint32_t tick) {
    /* Interrupt service routine ISR for start switch */
    if(gpioRead(GPIO_INT_START_SW)) {
        start_switch_pressed();
    }
}
