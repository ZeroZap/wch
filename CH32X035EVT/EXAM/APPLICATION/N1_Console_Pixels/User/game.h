/********************************** (C) COPYRIGHT *******************************
 * File Name          : game.h
 * Description        : Game module interface for a NOR cartridge game image.
 *
 * Each game is built as a standalone binary linked at N1_GAME_OFFSET.  The
 * launcher copies it from NOR, verifies CRC32 and jumps; game_runtime.c then
 * initialises the shared drivers and calls init/tick.  A game never links the
 * launcher's USB or menu code, so game images stay small.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __GAME_H
#define __GAME_H

#include <stdint.h>
#include "input.h"

#define GAME_ID_SNAKE       0u
#define GAME_ID_BREAKOUT    1u
#define GAME_ID_INVADERS    2u
#define GAME_ID_FLAPPY      3u
#define GAME_ID_SIMON       4u
#define GAME_ID_MAX         8u

typedef struct {
    uint8_t     id;             /* high-score slot, matches the cart index */
    const char  code[4];        /* three characters, shown on the panel   */
    const char *name;           /* long name for OLED / USB               */
    uint8_t     theme;          /* palette colour for the menu            */
    void      (*init)(void);
    uint8_t   (*tick)(const input_state_t *in);   /* 1 = leave the game */
    void      (*icon)(void);    /* 16x10 menu icon (only used if linked)  */
    uint16_t  (*score)(void);   /* current score for the OLED HUD         */
} game_desc_t;

/* Shared helpers for the game-over / win screens (game_util.c). */
void game_draw_3digits(uint16_t value, int y, uint8_t color);
void game_draw_over(uint16_t score, uint8_t win);

#endif /* __GAME_H */
