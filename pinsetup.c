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

    /* OUT MODE_ADC */
    gpioSetMode(GPIO_EXT_MODE_ADC_OUT, PI_OUTPUT);
    gpioSetPullUpDown(GPIO_EXT_MODE_ADC_OUT, PI_PUD_UP);

    /* OUT MODE_DIGITAL */
    gpioSetMode(GPIO_EXT_MODE_DIGITAL_OUT, PI_OUTPUT);
    gpioSetPullUpDown(GPIO_EXT_MODE_DIGITAL_OUT, PI_PUD_UP);

    /* Rotary encoder A, IN digital */
    gpioSetMode(GPIO_INT_ROT_A, PI_INPUT);
    gpioSetPullUpDown(GPIO_INT_ROT_A, PI_PUD_UP);

    /* Rotary encoder B, IN digital */
    gpioSetMode(GPIO_INT_ROT_B, PI_INPUT);
    gpioSetPullUpDown(GPIO_INT_ROT_B, PI_PUD_UP);

    /* Rotary encoder switch, IN digital */
    gpioSetMode(GPIO_INT_ROT_SW, PI_INPUT);
    gpioSetISRFunc(GPIO_INT_ROT_SW, RISING_EDGE, -1, _rot_switch_isr);

    return 1;
}

void
_rot_switch_isr(int gpio, int level, uint32_t tick) {
    /* Interrupt service routine ISR for rotary encoder switch */
    if(gpioRead(GPIO_INT_ROT_SW)) {
        menu_switch_pressed();
    }
}
