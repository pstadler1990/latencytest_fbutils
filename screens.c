//
// Created by pstadler on 05.10.19.
//

void
draw_screen_home(void) {
    uint32_t w = framebuf_device.w;
    uint32_t h = framebuf_device.h;

    char display_info_str[50];
    sprintf(display_info_str, "Screen Resolution: %dx%d", framebuf_device.w, framebuf_device.h);
    char bpp_info_str[20];
    sprintf(bpp_info_str, "Color depth: %d", framebuf_device.bpp);

    /* Drawing idle / welcome screen */
    fb_draw_line(&framebuf_device, 0, 0, w, h, COLOR_WHITE);
    fb_draw_line(&framebuf_device, w, 0, 0, h, COLOR_WHITE);
    fb_draw_rect(&framebuf_device, 0, 0, w / 2, h / 2, COLOR_WHITE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

    fb_draw_rect(&framebuf_device, 0, 0, 30, 30, COLOR_RED, DRAW_CENTER_NONE);
    fb_draw_rect(&framebuf_device, w - 30, 0, 30, 30, COLOR_GREEN, DRAW_CENTER_NONE);
    fb_draw_rect(&framebuf_device, 0, h - 30, 30, 30, COLOR_BLUE, DRAW_CENTER_NONE);
    fb_draw_rect(&framebuf_device, w - 30, h - 30, 30, 30, COLOR_YELLOW, DRAW_CENTER_NONE);

    fb_draw_text(&framebuf_device, display_info_str, 0, 0, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
    fb_draw_text(&framebuf_device, bpp_info_str, 0, 20, COLOR_BLACK, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);
    fb_draw_text(&framebuf_device, "- Press START to begin measurements -", 0, 90, COLOR_BLUE, DRAW_CENTER_HORIZONTAL | DRAW_CENTER_VERTICAL);

}