//
// Created by pstadler on 06.10.19.
//

#include <stdio.h>
#include "pinsetup.h"
#include "pigpio.h"
#include "configuration.h"
#include <unistd.h>

static void _trigger_in_isr(int gpio, int level, uint32_t tick);


int8_t
init_GPIOs(void) {
    /* */
    if (gpioInitialise() < 0) {
        return -1;
    }

    /* Configure pins for exchange with external uC (STM8) */
    gpioSetMode(GPIO_EXT_TRIGGER_IN, PI_INPUT);
    gpioSetISRFunc(GPIO_EXT_TRIGGER_IN, FALLING_EDGE, -1, _trigger_in_isr);

    /* OUT trigger */
    gpioSetMode(GPIO_EXT_TRIGGER_OUT, PI_OUTPUT);
    gpioSetPullUpDown(GPIO_EXT_TRIGGER_OUT, PI_PUD_OFF);
    gpioWrite(GPIO_EXT_TRIGGER_OUT, 1);
    usleep(100000);
    gpioWrite(GPIO_EXT_TRIGGER_OUT, 0);

    /* OUT MODE_ADC */
    gpioSetMode(GPIO_EXT_MODE_ADC_OUT, PI_OUTPUT);
    gpioSetPullUpDown(GPIO_EXT_MODE_ADC_OUT, PI_PUD_UP);

    /* OUT MODE_DIGITAL */
    gpioSetMode(GPIO_EXT_MODE_DIGITAL_OUT, PI_OUTPUT);
    gpioSetPullUpDown(GPIO_EXT_MODE_DIGITAL_OUT, PI_PUD_UP);

    // TODO: Add USART for intercommunication with external uC (for measurement values for calibration)
    return 1;
}

void
_trigger_in_isr(int gpio, int level, uint32_t tick) {
    /* Interrupt service routine ISR for trigger IN */
    printf("Interrupt occured on GPIO %d, level: %d, tick: %d\n", gpio, level, tick);
}
