/********************************** (C) COPYRIGHT *******************************
 * File Name          : panel_gfx.h
 * Description        : 16x16 logical framebuffer and drawing primitives for the
 *                      WS2812 panel.
 *
 * Games draw palette indices into one byte per pixel, panel_flush() maps the
 * logical (x, y) grid onto the physical serpentine strip and hands RGB to
 * ws2812.c.  A 3x5 font is included for menu labels and game-over screens.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __PANEL_GFX_H
#define __PANEL_GFX_H

#include <stdint.h>
#include "game_config.h"

/* 16-entry palette.  Index 0 must stay black: games clear the screen with it. */
enum {
    P_BLACK = 0,
    P_WHITE,
    P_RED,
    P_GREEN,
    P_BLUE,
    P_YELLOW,
    P_CYAN,
    P_MAGENTA,
    P_ORANGE,
    P_GRAY,
    P_DGREEN,
    P_DBLUE,
    P_DRED,
    P_PURPLE,
    P_TEAL,
    P_PINK,
    P_COLOR_COUNT
};

void panel_init(void);
void panel_clear(void);

void panel_pixel(int x, int y, uint8_t color);

/* Read back a logical pixel's palette index (0 = P_BLACK). */
uint8_t panel_get(int x, int y);
void panel_hline(int x, int y, int w, uint8_t color);
void panel_vline(int x, int y, int h, uint8_t color);
void panel_rect(int x, int y, int w, int h, uint8_t color);
void panel_frame(int x, int y, int w, int h, uint8_t color);

/* 3x5 font: 3 pixels wide, 5 tall, one blank column between characters. */
void    panel_text3x5(int x, int y, const char *s, uint8_t color);
void    panel_text3x5_center(int y, const char *s, uint8_t color);
uint8_t panel_text3x5_width(const char *s);

void    panel_flush(void);
void    panel_set_brightness(uint8_t level);
uint8_t panel_brightness(void);

#endif /* __PANEL_GFX_H */
