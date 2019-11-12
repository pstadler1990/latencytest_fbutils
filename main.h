//
// Created by pstadler on 06.10.19.
//

#ifndef FB_MAIN_H
#define FB_MAIN_H

struct Measurement {
    uint32_t tTrigger;
    uint32_t tBlack;
    uint32_t tWhite;
};

typedef enum {
    FBSTATE_ERROR_MEASURE = -2,             /* Error whilst measurement */
    FBSTATE_ERROR = -1,                     /* General, unknown error */
    FBSTATE_INITIALIZE = 0,                 /* Pre-idle state */
    FBSTATE_IDLE,                           /* Waiting, idle */
    FBSTATE_READY_FOR_MEASUREMENTS,         /* Conditions clear for (next) measurement series */
    FBSTATE_TRIGGERED,                      /* External trigger to start measurements (series) occurred */
    FBSTATE_READY_FOR_MEASURE,              /* Conditions clear for (next) measure (single measure) */
    FBSTATE_FINISHED_MEASURE,               /* Finished single measurement */
    FB_STATE_FINISHED_ALL_MEASUREMENTS,     /* Finished all measurements (end of series) */
} FB_STATE;

struct FbDevState {
    FB_STATE state;                         /* Internal state machine */
    /* TODO: configuration here */
    uint32_t n_measurements;                /* Number of measurements per series, default = 100 */
};

#endif //FB_MAIN_H
