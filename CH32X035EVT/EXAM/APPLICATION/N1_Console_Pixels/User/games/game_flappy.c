/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_flappy.c
 * Description        : 16x16 flappy bird.
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

#define BIRD_X          4
#define BIRD_W          2
#define GAP_HEIGHT      5
#define PIPE_W          2

static float    bird_y;
static float    bird_vy;
static float    pipe_x;
static uint8_t  gap_y;
static uint8_t  passed;
static uint16_t score;
static uint8_t  over;
static uint8_t  over_timer;

static void flappy_init(void)
{
    bird_y     = 6.0f;
    bird_vy    = 0.0f;
    pipe_x     = (float)PANEL_W + 2.0f;
    gap_y      = 4u;
    passed     = 0u;
    score      = 0u;
    over       = 0u;
    over_timer = 0u;
}

static void finish(void)
{
    over       = 1u;
    over_timer = 150u;
    audio_sfx(SFX_OVER);
    store_set_hi(GAME_ID_FLAPPY, score);
}

static void flap(void)
{
    bird_vy = -2.0f;
    audio_sfx(SFX_FLAP);
}

static uint8_t bird_hits_pipe(void)
{
    int px = (int)(pipe_x + 0.5f);
    int by = (int)(bird_y + 0.5f);

    if (px > BIRD_X + BIRD_W - 1 || px + PIPE_W - 1 < BIRD_X) {
        return 0u;
    }

    if (by < (int)gap_y) {
        return 1u;
    }
    if (by + BIRD_W - 1 >= (int)gap_y + GAP_HEIGHT) {
        return 1u;
    }

    return 0u;
}

static void draw(void)
{
    int px = (int)(pipe_x + 0.5f);
    int by = (int)(bird_y + 0.5f);
    int i;

    panel_clear();

    for (i = 0; i < PIPE_W; i++) {
        int x = px + i;
        if (x < 0 || x >= PANEL_W) {
            continue;
        }
        panel_vline(x, 0, (int)gap_y, P_GREEN);
        panel_vline(x, (int)gap_y + GAP_HEIGHT,
                    PANEL_H - ((int)gap_y + GAP_HEIGHT), P_GREEN);
    }

    panel_rect(BIRD_X, by, BIRD_W, BIRD_W, P_YELLOW);
    panel_pixel(BIRD_X + BIRD_W - 1, by, P_WHITE);

    if (score) {
        panel_hline(0, 0, 1 + (int)(score % 3u), P_CYAN);
    }
}

static uint8_t flappy_tick(const input_state_t *in)
{
    if (over) {
        if (in->pressed & BTN_A) {
            flappy_init();
            return 0u;
        }
        if (over_timer) {
            over_timer--;
        }

        panel_clear();
        game_draw_over(score, 0u);
        return (over_timer == 0u) ? 1u : 0u;
    }

    if (in->pressed & (BTN_A | BTN_UP)) {
        flap();
    }

    bird_vy += 0.28f;
    if (bird_vy > 2.4f) {
        bird_vy = 2.4f;
    }
    bird_y += bird_vy;

    if (bird_y < 0.0f) {
        bird_y = 0.0f;
        if (bird_vy < 0.0f) {
            bird_vy = 0.0f;
        }
    }

    if (bird_y > (float)(PANEL_H - BIRD_W)) {
        bird_y = (float)(PANEL_H - BIRD_W);
        finish();
        draw();
        return 0u;
    }

    pipe_x -= 0.55f;

    if (!passed && (pipe_x + PIPE_W) < (float)BIRD_X) {
        passed = 1u;
        score++;
        audio_sfx(SFX_POINT);
    }

    if (pipe_x < -(float)PIPE_W - 1.0f) {
        pipe_x   = (float)PANEL_W + 1.0f;
        gap_y    = (uint8_t)(1u + (rand() % (PANEL_H - GAP_HEIGHT - 2)));
        passed   = 0u;
    }

    if (bird_hits_pipe()) {
        finish();
        draw();
        return 0u;
    }

    draw();

    return 0u;
}

static void flappy_icon(void)
{
    panel_vline(10, 0, 3, P_GREEN);
    panel_vline(11, 0, 3, P_GREEN);
    panel_vline(10, 8, 2, P_GREEN);
    panel_vline(11, 8, 2, P_GREEN);
    panel_rect(3, 4, 2, 2, P_YELLOW);
}

static uint16_t flappy_score(void)
{
    return score;
}

const game_desc_t game_flappy = {
    GAME_ID_FLAPPY, "FLY", "flappy", P_YELLOW,
    flappy_init, flappy_tick, flappy_icon, flappy_score
};
