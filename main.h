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
    FBSTATE_READY,                          /* Conditions clear for (next) measure (single measure) */
} FB_STATE;

typedef enum {
    FBMODE_HOME,
    FBMODE_TEST,
    FBMODE_CALIB,
} FB_MODE;

struct FbDevState {
    FB_STATE state;                         /* Internal state machine */
    FB_MODE mode;                           /* Internal mode */
    /* TODO: configuration here */
    uint32_t n_measurements;                /* Number of measurements per series, default = 100 */
};

#endif //FB_MAIN_H
