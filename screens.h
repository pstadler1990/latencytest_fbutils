//
// Created by pstadler on 05.10.19.
//

#ifndef FB_SCREENS_H
#define FB_SCREENS_H

#include "menu.h"

void draw_screen_home(struct FbDev* fb_device);
void draw_screen_test(struct FbDev* fb_device);
void draw_screen_calib_bw_digits(struct FbDev* fb_device);
void draw_screen_alternating(struct FbDev* fb_device);
void menu_rot_changed(ROT_STATE state);
#endif //FB_SCREENS_H
