/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_breakout.c
 * Description        : 16x16 brick breaker.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <stdlib.h>

#include "game.h"
#include "panel_gfx.h"
#include "audio.h"
#include "nor_store.h"

#define BRICK_ROWS      4
#define BRICK_COLS      8
#define PADDLE_W        4
#define PADDLE_Y        15
#define START_LIVES     3u

static uint8_t  bricks[BRICK_ROWS];
static uint8_t  paddle;
static float    ball_x;
static float    ball_y;
static float    ball_vx;
static float    ball_vy;
static uint8_t  lives;
static uint16_t score;
static uint8_t  rest;
static uint8_t  over;
static uint8_t  win;
static uint8_t  over_timer;
static uint8_t  blink;

static void reset_ball(void)
{
    ball_x  = 8.0f;
    ball_y  = 12.0f;
    ball_vx = (rand() & 1) ? 0.55f : -0.55f;
    ball_vy = -0.65f;
    rest    = 30u;
}

static void breakout_init(void)
{
    uint8_t r;

    for (r = 0; r < BRICK_ROWS; r++) {
        bricks[r] = 0xFFu;
    }

    paddle     = 6u;
    lives      = START_LIVES;
    score      = 0u;
    over       = 0u;
    win        = 0u;
    over_timer = 0u;
    blink      = 0u;

    reset_ball();
}

static void finish(uint8_t won)
{
    over       = 1u;
    win        = won;
    over_timer = 150u;
    audio_sfx(won ? SFX_WIN : SFX_OVER);
    store_set_hi(GAME_ID_BREAKOUT, score);
}

static void lose_life(void)
{
    if (lives) {
        lives--;
    }

    audio_sfx(SFX_HIT);

    if (lives == 0u) {
        finish(0u);
    } else {
        reset_ball();
    }
}

static void move_ball(void)
{
    int row;
    int col;
    uint8_t r;

    ball_x += ball_vx;
    ball_y += ball_vy;

    if (ball_x < 0.0f) {
        ball_x = 0.0f;
        ball_vx = -ball_vx;
        audio_sfx(SFX_BUMP);
    } else if (ball_x > (float)(PANEL_W - 1)) {
        ball_x = (float)(PANEL_W - 1);
        ball_vx = -ball_vx;
        audio_sfx(SFX_BUMP);
    }

    if (ball_y < 0.0f) {
        ball_y = 0.0f;
        ball_vy = -ball_vy;
        audio_sfx(SFX_BUMP);
    }

    if (ball_y > (float)PANEL_H) {
        lose_life();
        return;
    }

    /* paddle */
    if (ball_vy > 0.0f && ball_y >= (float)(PADDLE_Y - 1)) {
        if (ball_x >= (float)paddle && ball_x <= (float)(paddle + PADDLE_W - 1)) {
            ball_y = (float)(PADDLE_Y - 1);
            ball_vy = -ball_vy;
            ball_vx += (ball_x - ((float)paddle + 1.5f)) * 0.15f;
            if (ball_vx > 0.9f) ball_vx = 0.9f;
            if (ball_vx < -0.9f) ball_vx = -0.9f;
            audio_sfx(SFX_BUMP);
        }
    }

    /* bricks */
    if (ball_vy < 0.0f && ball_y < (float)BRICK_ROWS) {
        row = (int)ball_y;
        col = (int)(ball_x / 2.0f);
        if (col < 0) col = 0;
        if (col >= BRICK_COLS) col = BRICK_COLS - 1;

        if (bricks[row] & (uint8_t)(1u << col)) {
            bricks[row] &= (uint8_t)~(1u << col);
            score += 10u;
            ball_vy = -ball_vy;
            audio_sfx(SFX_POINT);

            for (r = 0; r < BRICK_ROWS; r++) {
                if (bricks[r]) {
                    break;
                }
            }
            if (r == BRICK_ROWS) {
                finish(1u);
            }
        }
    }
}

static void draw(void)
{
    uint8_t r;
    uint8_t color;

    panel_clear();

    for (r = 0; r < BRICK_ROWS; r++) {
        uint8_t col;

        switch (r) {
        case 0:  color = P_RED;    break;
        case 1:  color = P_ORANGE; break;
        case 2:  color = P_YELLOW; break;
        default: color = P_GREEN;  break;
        }

        for (col = 0; col < BRICK_COLS; col++) {
            if (bricks[r] & (uint8_t)(1u << col)) {
                panel_rect(col * 2, r, 2, 1, color);
            }
        }
    }

    panel_hline(paddle, PADDLE_Y, PADDLE_W, P_WHITE);
    panel_pixel((int)(ball_x + 0.5f), (int)(ball_y + 0.5f),
                ((blink >> 2) & 1u) ? P_WHITE : P_YELLOW);

    /* lives as a short bar in the top-right corner */
    if (lives) {
        panel_hline(PANEL_W - (int)lives, 0, (int)lives, P_CYAN);
    }
}

static uint8_t breakout_tick(const input_state_t *in)
{
    blink++;

    if (over) {
        if (in->pressed & BTN_A) {
            breakout_init();
            return 0u;
        }
        if (over_timer) {
            over_timer--;
        }

        panel_clear();
        game_draw_over(score, win);
        return (over_timer == 0u) ? 1u : 0u;
    }

    if (rest) {
        rest--;
        draw();
        return 0u;
    }

    if (in->held & BTN_LEFT) {
        if (paddle > 0u) {
            paddle--;
        }
    }
    if (in->held & BTN_RIGHT) {
        if (paddle < (PANEL_W - PADDLE_W)) {
            paddle++;
        }
    }

    move_ball();
    draw();

    return 0u;
}

static void breakout_icon(void)
{
    panel_rect(0, 1, 16, 3, P_RED);
    panel_hline(0, 2, 16, P_BLACK);
    panel_hline(0, 3, 16, P_BLACK);
    panel_hline(6, 8, 4, P_WHITE);
    panel_pixel(8, 6, P_YELLOW);
}

static uint16_t breakout_score(void)
{
    return score;
}

const game_desc_t game_breakout = {
    GAME_ID_BREAKOUT, "BRK", "breakout", P_ORANGE,
    breakout_init, breakout_tick, breakout_icon, breakout_score
};
