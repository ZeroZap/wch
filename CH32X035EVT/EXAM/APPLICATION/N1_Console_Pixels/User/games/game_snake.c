/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_snake.c
 * Description        : 16x16 snake.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "panel_gfx.h"
#include "audio.h"
#include "nor_store.h"

#define SNAKE_MAX       64u

enum { DIR_UP = 0, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

static uint8_t  body[SNAKE_MAX];     /* packed (y << 4) | x */
static uint8_t  length;
static uint8_t  dir;
static uint8_t  next_dir;
static uint8_t  food;
static uint16_t score;
static uint8_t  move_timer;
static uint8_t  move_period;
static uint8_t  over;
static uint8_t  over_timer;
static uint8_t  blink;

static uint8_t pack_xy(uint8_t x, uint8_t y) { return (uint8_t)((y << 4) | x); }
static uint8_t px(uint8_t p) { return (uint8_t)(p & 0x0Fu); }
static uint8_t py(uint8_t p) { return (uint8_t)(p >> 4); }

static void new_food(void)
{
    uint8_t tries;
    uint8_t i;

    for (tries = 0; tries < 200u; tries++) {
        uint8_t p = (uint8_t)(rand() & 0xFF);

        for (i = 0; i < length; i++) {
            if (body[i] == p) {
                break;
            }
        }
        if (i == length) {
            food = p;
            return;
        }
    }

    /* board is almost full: take the first free cell */
    for (i = 0; i < 255u; i++) {
        uint8_t j;
        for (j = 0; j < length; j++) {
            if (body[j] == i) {
                break;
            }
        }
        if (j == length) {
            food = i;
            return;
        }
    }
}

static void snake_init(void)
{
    length     = 3u;
    body[0]    = pack_xy(8, 8);
    body[1]    = pack_xy(7, 8);
    body[2]    = pack_xy(6, 8);
    dir        = DIR_RIGHT;
    next_dir   = dir;
    score      = 0u;
    move_timer = 0u;
    move_period = 10u;
    over       = 0u;
    over_timer = 0u;
    blink      = 0u;

    new_food();
}

static void game_over(void)
{
    over       = 1u;
    over_timer = 150u;
    audio_sfx(SFX_OVER);
    store_set_hi(GAME_ID_SNAKE, score);
}

static void snake_move(void)
{
    uint8_t head = body[0];
    int     hx = px(head);
    int     hy = py(head);
    uint8_t np;
    uint8_t i;

    dir = next_dir;

    switch (dir) {
    case DIR_UP:    hy--; break;
    case DIR_DOWN:  hy++; break;
    case DIR_LEFT:  hx--; break;
    default:        hx++; break;
    }

    if (hx < 0 || hy < 0 || hx >= PANEL_W || hy >= PANEL_H) {
        game_over();
        return;
    }

    np = pack_xy((uint8_t)hx, (uint8_t)hy);

    for (i = 0; i < length; i++) {
        if (body[i] == np) {
            game_over();
            return;
        }
    }

    if (np == food) {
        if (length < SNAKE_MAX) {
            memmove(body + 1, body, length);
            length++;
        } else {
            memmove(body + 1, body, length - 1u);
        }
        body[0] = np;
        score  += 10u;
        if (move_period > 4u) {
            move_period--;
        }
        audio_sfx(SFX_POINT);
        new_food();
    } else {
        memmove(body + 1, body, length - 1u);
        body[0] = np;
    }
}

static uint8_t snake_tick(const input_state_t *in)
{
    uint8_t i;

    if (over) {
        if (in->pressed & BTN_A) {
            snake_init();
            return 0u;
        }
        if (over_timer) {
            over_timer--;
        }

        panel_clear();
        game_draw_over(score, 0u);
        return (over_timer == 0u) ? 1u : 0u;
    }

    if ((in->pressed & BTN_UP) && dir != DIR_DOWN) {
        next_dir = DIR_UP;
    } else if ((in->pressed & BTN_DOWN) && dir != DIR_UP) {
        next_dir = DIR_DOWN;
    } else if ((in->pressed & BTN_LEFT) && dir != DIR_RIGHT) {
        next_dir = DIR_LEFT;
    } else if ((in->pressed & BTN_RIGHT) && dir != DIR_LEFT) {
        next_dir = DIR_RIGHT;
    }

    if (++move_timer >= move_period) {
        move_timer = 0u;
        snake_move();
        if (over) {
            panel_clear();
            game_draw_over(score, 0u);
            return 0u;
        }
    }

    panel_clear();

    for (i = 0; i < length; i++) {
        uint8_t c = (i == 0u) ? P_WHITE : (i < 3u ? P_GREEN : P_DGREEN);
        panel_pixel(px(body[i]), py(body[i]), c);
    }

    blink++;
    panel_pixel(px(food), py(food), ((blink >> 3) & 1u) ? P_RED : P_DRED);

    return 0u;
}

static void snake_icon(void)
{
    /* zig-zag snake, rows 2..8 */
    panel_hline(2, 8, 5, P_DGREEN);
    panel_pixel(2, 7, P_DGREEN);
    panel_vline(6, 4, 5, P_GREEN);
    panel_hline(6, 4, 5, P_GREEN);
    panel_vline(10, 2, 3, P_GREEN);
    panel_pixel(11, 2, P_WHITE);
    panel_pixel(12, 7, P_RED);
}

static uint16_t snake_score(void)
{
    return score;
}

const game_desc_t game_snake = {
    GAME_ID_SNAKE, "SNK", "snake", P_GREEN,
    snake_init, snake_tick, snake_icon, snake_score
};
