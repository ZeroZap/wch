/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_invaders.c
 * Description        : 16x16 space invaders.
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

#define ENEMY_ROWS      3
#define ENEMY_COLS      5
#define ENEMY_COUNT     (ENEMY_ROWS * ENEMY_COLS)
#define PLAYER_W        3
#define PLAYER_Y        14
#define START_LIVES     3u

static uint16_t alive;
static int      form_x;
static int      form_y;
static int      form_dir;
static uint8_t  move_timer;
static uint8_t  move_period;
static uint8_t  fire_timer;
static int      player_x;
static uint8_t  lives;
static uint16_t score;
static uint8_t  over;
static uint8_t  win;
static uint8_t  over_timer;
static uint8_t  invuln;
static uint8_t  blink;

static uint8_t  pb_active;
static float    pb_x;
static float    pb_y;
static uint8_t  eb_active;
static float    eb_x;
static float    eb_y;

static void enemy_xy(uint8_t idx, int *x, int *y)
{
    uint8_t row = (uint8_t)(idx / ENEMY_COLS);
    uint8_t col = (uint8_t)(idx % ENEMY_COLS);

    *x = 1 + (int)col * 3 + form_x;
    *y = 1 + (int)row * 3 + form_y;
}

static void invaders_init(void)
{
    alive       = 0x7FFFu;      /* 15 enemies */
    form_x      = 0;
    form_y      = 0;
    form_dir    = 1;
    move_timer  = 0u;
    move_period = 18u;
    fire_timer  = 80u;
    player_x    = 6;
    lives       = START_LIVES;
    score       = 0u;
    over        = 0u;
    win         = 0u;
    over_timer  = 0u;
    invuln      = 0u;
    blink       = 0u;
    pb_active   = 0u;
    eb_active   = 0u;
}

static void finish(uint8_t won)
{
    over       = 1u;
    win        = won;
    over_timer = 150u;
    audio_sfx(won ? SFX_WIN : SFX_OVER);
    store_set_hi(GAME_ID_INVADERS, score);
}

static void hit_player(void)
{
    if (invuln) {
        return;
    }

    audio_sfx(SFX_HIT);
    invuln = 70u;

    if (lives) {
        lives--;
    }
    if (lives == 0u) {
        finish(0u);
    }
}

static void move_formation(void)
{
    int min_x = 99;
    int max_x = -99;
    uint8_t i;

    form_x += form_dir;

    for (i = 0; i < ENEMY_COUNT; i++) {
        if (alive & (uint16_t)(1u << i)) {
            int x;
            int y;
            enemy_xy(i, &x, &y);
            if (x < min_x) min_x = x;
            if (x + 1 > max_x) max_x = x + 1;
        }
    }

    if (min_x < 0 || max_x >= PANEL_W) {
        form_x -= form_dir;
        form_dir = -form_dir;
        form_y++;

        for (i = 0; i < ENEMY_COUNT; i++) {
            if (alive & (uint16_t)(1u << i)) {
                int x;
                int y;
                enemy_xy(i, &x, &y);
                if (y + 1 >= PLAYER_Y) {
                    finish(0u);
                    return;
                }
            }
        }
    }
}

static void enemy_fire(void)
{
    uint8_t start = (uint8_t)(rand() % ENEMY_COUNT);
    uint8_t n;

    for (n = 0; n < ENEMY_COUNT; n++) {
        uint8_t idx = (uint8_t)((start + n) % ENEMY_COUNT);

        if (alive & (uint16_t)(1u << idx)) {
            int x;
            int y;
            enemy_xy(idx, &x, &y);
            eb_active = 1u;
            eb_x = (float)x;
            eb_y = (float)(y + 2);
            return;
        }
    }
}

static void update_bullets(void)
{
    uint8_t i;

    if (pb_active) {
        pb_y -= 1.6f;
        if (pb_y < -1.0f) {
            pb_active = 0u;
        } else {
            for (i = 0; i < ENEMY_COUNT; i++) {
                if (alive & (uint16_t)(1u << i)) {
                    int x;
                    int y;
                    enemy_xy(i, &x, &y);
                    if ((int)(pb_x + 0.5f) >= x && (int)(pb_x + 0.5f) <= x + 1 &&
                        (int)(pb_y + 0.5f) >= y && (int)(pb_y + 0.5f) <= y + 1) {
                        alive &= (uint16_t)~(1u << i);
                        pb_active = 0u;
                        score += 20u;
                        audio_sfx(SFX_POINT);

                        if (alive == 0u) {
                            finish(1u);
                        }
                        break;
                    }
                }
            }
        }
    }

    if (eb_active) {
        eb_y += 0.7f;

        if (eb_y > (float)PANEL_H) {
            eb_active = 0u;
        } else if ((int)(eb_x + 0.5f) >= player_x &&
                   (int)(eb_x + 0.5f) < player_x + PLAYER_W &&
                   (int)(eb_y + 0.5f) >= PLAYER_Y &&
                   (int)(eb_y + 0.5f) < PLAYER_Y + 2) {
            eb_active = 0u;
            hit_player();
        }
    }
}

static void draw(void)
{
    uint8_t i;

    panel_clear();

    for (i = 0; i < ENEMY_COUNT; i++) {
        if (alive & (uint16_t)(1u << i)) {
            int x;
            int y;
            uint8_t c;

            enemy_xy(i, &x, &y);
            c = (i < ENEMY_COLS) ? P_RED :
                (i < ENEMY_COLS * 2) ? P_ORANGE : P_MAGENTA;

            panel_rect(x, y, 2, 2, c);
        }
    }

    if (!invuln || ((blink >> 2) & 1u)) {
        panel_rect(player_x, PLAYER_Y, PLAYER_W, 2, P_CYAN);
        panel_pixel(player_x + 1, PLAYER_Y - 1, P_WHITE);
    }

    if (pb_active) {
        panel_pixel((int)(pb_x + 0.5f), (int)(pb_y + 0.5f), P_YELLOW);
    }
    if (eb_active) {
        panel_pixel((int)(eb_x + 0.5f), (int)(eb_y + 0.5f), P_RED);
    }

    if (lives) {
        panel_hline(PANEL_W - (int)lives, 15, (int)lives, P_CYAN);
    }
}

static uint8_t invaders_tick(const input_state_t *in)
{
    blink++;

    if (over) {
        if (in->pressed & BTN_A) {
            invaders_init();
            return 0u;
        }
        if (over_timer) {
            over_timer--;
        }

        panel_clear();
        game_draw_over(score, win);
        return (over_timer == 0u) ? 1u : 0u;
    }

    if (invuln) {
        invuln--;
    }

    if (in->held & BTN_LEFT) {
        if (player_x > 0) player_x--;
    }
    if (in->held & BTN_RIGHT) {
        if (player_x < PANEL_W - PLAYER_W) player_x++;
    }

    if ((in->pressed & BTN_A) && !pb_active) {
        pb_active = 1u;
        pb_x = (float)player_x + 1.0f;
        pb_y = (float)(PLAYER_Y - 1);
        audio_sfx(SFX_SHOOT);
    }

    if (++move_timer >= move_period) {
        move_timer = 0u;
        move_formation();
        if (over) {
            panel_clear();
            game_draw_over(score, win);
            return 0u;
        }
    }

    if (fire_timer) {
        fire_timer--;
    } else if (!eb_active) {
        enemy_fire();
        fire_timer = (uint8_t)(40u + (rand() % 60));
    }

    update_bullets();
    draw();

    return 0u;
}

static void invaders_icon(void)
{
    panel_rect(2, 1, 2, 2, P_RED);
    panel_rect(7, 1, 2, 2, P_ORANGE);
    panel_rect(12, 1, 2, 2, P_MAGENTA);
    panel_rect(6, 8, 3, 2, P_CYAN);
    panel_pixel(7, 6, P_YELLOW);
}

static uint16_t invaders_score(void)
{
    return score;
}

const game_desc_t game_invaders = {
    GAME_ID_INVADERS, "INV", "invaders", P_RED,
    invaders_init, invaders_tick, invaders_icon, invaders_score
};
