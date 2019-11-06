//
// Created by pstadler on 06.10.19.
//

#ifndef FB_CONFIGURATION_H
#define FB_CONFIGURATION_H

#define DEFAULT_N_MEASUREMENTS      ((uint32_t)10)         /* Number of single measurements per series */
#define MAX_FAILED_MEASUREMENTS     ((uint32_t)10)         /* Maximum number of failed measurements within a single test */
#define MEASUREMENT_TIMEOUT         ((uint32_t)1000000)    /* Maximum timeout */
#define ROT_DEBOUNCE_TIME           ((uint32_t)100)

/* GPIOs */
#define GPIO_EXT_TRIGGER_IN         ((unsigned int)5)       /* Physical pin 29, BCM pin 5 */
#define GPIO_EXT_TRIGGER_OUT        ((unsigned int)17)      /* Physical pin 11, BCM pin 17 */
#define GPIO_EXT_MODE_ADC_OUT       ((unsigned int)27)      /* Physical pin 13, BCM pin 27*/
#define GPIO_EXT_MODE_DIGITAL_OUT   ((unsigned int)22)      /* Physical pin 15, BCM pin 22*/

#define GPIO_INT_ROT_A              ((unsigned int)26)      /* Physical pin 37, BCM pin 26*/
#define GPIO_INT_ROT_B              ((unsigned int)19)      /* Physical pin 35, BCM pin 19*/
#define GPIO_INT_ROT_SW             ((unsigned int)13)      /* Physical pin 33, BCM pin 13*/

#endif //FB_CONFIGURATION_H
