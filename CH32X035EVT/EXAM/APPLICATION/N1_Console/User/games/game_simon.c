/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_simon.c
 * Description        : 16x16 colour / direction memory game.
 *
 *   UP = top-left, RIGHT = top-right, LEFT = bottom-left, DOWN = bottom-right.
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

#define SEQ_MAX         32u

enum { SIMON_SHOW = 0, SIMON_INPUT, SIMON_OVER };

static const uint8_t quad_color[4] = { P_RED, P_GREEN, P_BLUE, P_YELLOW };
static const uint8_t quad_dim[4]   = { P_DRED, P_DGREEN, P_DBLUE, P_GRAY };
static const uint16_t quad_tone[4] = { 440u, 554u, 659u, 880u };
static const int quad_cx[4] = { 3, 12, 3, 12 };
static const int quad_cy[4] = { 3, 3, 12, 12 };

static uint8_t  seq[SEQ_MAX];
static uint8_t  seq_len;
static uint8_t  state;
static uint8_t  show_idx;
static uint8_t  input_idx;
static uint8_t  timer;
static uint8_t  active;
static uint8_t  flash;
static uint8_t  over_timer;
static uint16_t score;

static void next_sequence(void)
{
    if (seq_len < SEQ_MAX) {
        seq[seq_len] = (uint8_t)(rand() & 3);
        seq_len++;
    }
}

static void simon_init(void)
{
    seq_len    = 0u;
    state      = SIMON_SHOW;
    show_idx   = 0u;
    input_idx  = 0u;
    timer      = 20u;
    active     = 0xFFu;
    flash      = 0u;
    over_timer = 0u;
    score      = 0u;

    seq[0] = (uint8_t)(rand() & 3);
    seq_len = 1u;
}

static void simon_fail(void)
{
    state      = SIMON_OVER;
    over_timer = 150u;
    active     = 0xFFu;
    audio_sfx(SFX_OVER);
    store_set_hi(GAME_ID_SIMON, score);
}

static void arrow(int cx, int cy, uint8_t dir, uint8_t color)
{
    switch (dir) {
    case 0:     /* up */
        panel_vline(cx, cy - 2, 5, color);
        panel_pixel(cx - 1, cy - 1, color);
        panel_pixel(cx + 1, cy - 1, color);
        break;
    case 1:     /* right */
        panel_hline(cx - 2, cy, 5, color);
        panel_pixel(cx + 1, cy - 1, color);
        panel_pixel(cx + 1, cy + 1, color);
        break;
    case 2:     /* left */
        panel_hline(cx - 2, cy, 5, color);
        panel_pixel(cx - 1, cy - 1, color);
        panel_pixel(cx - 1, cy + 1, color);
        break;
    default:    /* down */
        panel_vline(cx, cy - 2, 5, color);
        panel_pixel(cx - 1, cy + 1, color);
        panel_pixel(cx + 1, cy + 1, color);
        break;
    }
}

static void draw(void)
{
    uint8_t q;

    panel_clear();

    for (q = 0; q < 4u; q++) {
        int x = quad_cx[q] - 3;
        int y = quad_cy[q] - 3;
        uint8_t fill = (active == q) ? quad_color[q] : quad_dim[q];
        uint8_t ink  = (active == q) ? P_WHITE : P_GRAY;

        panel_rect(x, y, 7, 7, fill);
        arrow(quad_cx[q], quad_cy[q], q, ink);
    }
}

static uint8_t key_to_quad(uint8_t pressed)
{
    if (pressed & BTN_UP)    return 0u;
    if (pressed & BTN_RIGHT) return 1u;
    if (pressed & BTN_LEFT)  return 2u;
    if (pressed & BTN_DOWN)  return 3u;
    return 0xFFu;
}

static uint8_t simon_tick(const input_state_t *in)
{
    if (state == SIMON_OVER) {
        if (in->pressed & BTN_A) {
            simon_init();
            return 0u;
        }
        if (over_timer) {
            over_timer--;
        }

        panel_clear();
        game_draw_over(score, 0u);
        return (over_timer == 0u) ? 1u : 0u;
    }

    if (flash) {
        flash--;
        if (flash == 0u) {
            active = 0xFFu;
        }
    }

    if (state == SIMON_SHOW) {
        if (timer) {
            timer--;
        } else if (active == 0xFFu) {
            active = seq[show_idx];
            timer  = 22u;
            audio_note(quad_tone[active], 8u);
        } else {
            active = 0xFFu;
            show_idx++;
            timer = 8u;

            if (show_idx >= seq_len) {
                state     = SIMON_INPUT;
                input_idx = 0u;
            }
        }
    } else {
        uint8_t q = key_to_quad(in->pressed);

        if (q != 0xFFu) {
            if (q == seq[input_idx]) {
                active = q;
                flash  = 8u;
                audio_note(quad_tone[q], 6u);
                input_idx++;

                if (input_idx >= seq_len) {
                    score = seq_len;
                    next_sequence();
                    state     = SIMON_SHOW;
                    show_idx  = 0u;
                    timer     = 18u;
                    active    = 0xFFu;
                    audio_note(1046u, 4u);
                    audio_note(1318u, 4u);
                }
            } else {
                simon_fail();
                draw();
                return 0u;
            }
        }
    }

    draw();
    return 0u;
}

static void simon_icon(void)
{
    /* 16x10 compact icon: four 7x4 quadrants with a 3x3 direction mark */
    panel_rect(0, 0, 7, 4, P_DRED);
    panel_rect(9, 0, 7, 4, P_DGREEN);
    panel_rect(0, 6, 7, 4, P_DBLUE);
    panel_rect(9, 6, 7, 4, P_GRAY);

    panel_pixel(3, 1, P_WHITE);
    panel_pixel(2, 2, P_WHITE);
    panel_pixel(4, 2, P_WHITE);
    panel_pixel(3, 3, P_WHITE);

    panel_pixel(13, 2, P_WHITE);
    panel_pixel(12, 1, P_WHITE);
    panel_pixel(12, 3, P_WHITE);
    panel_pixel(11, 2, P_WHITE);

    panel_pixel(1, 8, P_WHITE);
    panel_pixel(2, 7, P_WHITE);
    panel_pixel(2, 9, P_WHITE);
    panel_pixel(3, 8, P_WHITE);

    panel_pixel(12, 9, P_WHITE);
    panel_pixel(11, 8, P_WHITE);
    panel_pixel(13, 8, P_WHITE);
    panel_pixel(12, 7, P_WHITE);
}

static uint16_t simon_score(void)
{
    return score;
}

const game_desc_t game_simon = {
    GAME_ID_SIMON, "SIM", "simon", P_BLUE,
    simon_init, simon_tick, simon_icon, simon_score
};
