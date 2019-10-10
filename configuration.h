//
// Created by pstadler on 06.10.19.
//

#ifndef FB_CONFIGURATION_H
#define FB_CONFIGURATION_H

#define DEFAULT_N_MEASUREMENTS      ((uint32_t)100)         /* Number of single measurements per series */

/* GPIOs */
#define GPIO_EXT_TRIGGER_IN         ((unsigned int)5)       /* Physical pin 29, BCM pin 5 */
#define GPIO_EXT_TRIGGER_OUT        ((unsigned int)17)      /* Physical pin 11, BCM pin 17 */
#define GPIO_EXT_MODE_ADC_OUT       ((unsigned int)27)      /* Physical pin 13, BCM pin 27*/
#define GPIO_EXT_MODE_DIGITAL_OUT   ((unsigned int)22)      /* Physical pin 15, BCM pin 22*/

#endif //FB_CONFIGURATION_H
