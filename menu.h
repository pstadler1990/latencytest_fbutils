//
// Created by pstadler on 06.11.19.
//

#ifndef FB_MENU_H
#define FB_MENU_H

#include "fb_lib.h"

typedef enum {
    MENUSTATE_INITIALIZE = 0,
} MENU_STATE;

typedef enum {
    ROTSTATE_CLOCKWISE,
    ROTSTATE_COUNTERCLOCKWISE
} ROT_STATE;

void menu_switch_pressed(void);
void calib_switch_pressed(void);
void start_switch_pressed(void);
void* menu_poll(void* vargp);
void* usbdrive_poll(void* vargp);

#endif //FB_MENU_H
