//
// Created by pstadler on 06.10.19.
//

#ifndef FB_CONFIGURATION_H
#define FB_CONFIGURATION_H

#define DEFAULT_N_MEASUREMENTS      ((uint32_t)20)        /* Number of single measurements per series */
#define MAX_FAILED_MEASUREMENTS     ((uint32_t)10)         /* Maximum number of failed measurements within a single test */
#define MEASUREMENT_TIMEOUT         ((uint32_t)1000000)    /* Maximum timeout */
#define ROT_DEBOUNCE_TIME           ((uint32_t)100)

#define EDID_MAX_DISPLAY_NAME       ((uint32_t)64)

#define RESULT_OUTPUT_DIR           "results"

/* GPIOs */
#define GPIO_EXT_TRIGGER_IN         ((unsigned int)5)       /* Physical pin 29, BCM pin 5 */
#define GPIO_EXT_TRIGGER_OUT        ((unsigned int)17)      /* Physical pin 11, BCM pin 17 */
#define GPIO_EXT_MODE_ADC_OUT       ((unsigned int)27)      /* Physical pin 13, BCM pin 27*/
#define GPIO_EXT_MODE_DIGITAL_OUT   ((unsigned int)22)      /* Physical pin 15, BCM pin 22*/

#define GPIO_INT_ROT_A              ((unsigned int)26)      /* Physical pin 37, BCM pin 26*/
#define GPIO_INT_ROT_B              ((unsigned int)19)      /* Physical pin 35, BCM pin 19*/
#define GPIO_INT_ROT_SW             ((unsigned int)13)      /* Physical pin 33, BCM pin 13*/

#define GPIO_INT_CALIB_SW           ((unsigned int)21)      /* Physical pin 40, BCM pin 21*/
#define GPIO_INT_START_SW           ((unsigned int)20)      /* Physical pin 38, BCM pin 20*/
#define GPIO_EXT_CALIB_LED          ((unsigned int)16)      /* Physical pin 36, BCM pin 16*/
#define GPIO_EXT_START_LED          ((unsigned int)12)      /* Physical pin 32, BCM pin 12*/

#endif //FB_CONFIGURATION_H
