/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_util.c
 * Description        : Shared game-over rendering used by every game image.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "game.h"
#include "panel_gfx.h"

void game_draw_3digits(uint16_t value, int y, uint8_t color)
{
    char s[4];

    if (value > 999u) {
        value = 999u;
    }

    s[0] = (char)('0' + (value / 100u));
    s[1] = (char)('0' + ((value / 10u) % 10u));
    s[2] = (char)('0' + (value % 10u));
    s[3] = '\0';

    panel_text3x5_center(y, s, color);
}

void game_draw_over(uint16_t score, uint8_t win)
{
    if (win) {
        panel_text3x5_center(0, "YOU", P_GREEN);
        panel_text3x5_center(6, "WIN!", P_WHITE);
    } else {
        panel_text3x5_center(0, "GAME", P_RED);
        panel_text3x5_center(6, "OVER", P_ORANGE);
    }

    game_draw_3digits(score, 11, P_YELLOW);
}
