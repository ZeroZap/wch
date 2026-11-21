/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_config.h
 * Description        : Strip_Games build configuration.
 *
 * This header provides the handful of macros that the shared WS2812 driver
 * (ws2812.c, copied from WS2812_Panel_IAP) expects, plus the game parameters.
 * Geometry and colour order are overridable from the build system.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __GAME_CONFIG_H
#define __GAME_CONFIG_H

#include "board_config.h"

#if !WSP_DEBUG_UART
/* no bonded-out UART TX on TSSOP20: compile the log calls away */
#define printf(...) ((void)0)
#endif

/*---------------------------------------------------------------------------
 * Strip geometry
 *   One continuous WS2812B strip, 1D.  48 LEDs is the reference build; the
 *   PIOC engine handles up to 1024 pixels, the game logic is written for any
 *   count but looks best between 24 and 120.
 *---------------------------------------------------------------------------*/
#ifndef GAME_LEDS
#define GAME_LEDS               48
#endif

/* Frame rate of the game loop (TIM3 tick is 1 ms, the loop accumulates). */
#ifndef GAME_FPS
#define GAME_FPS                60
#endif
#define GAME_FRAME_MS           (1000u / GAME_FPS)

/* The driver allocates its transmit buffer from this. */
#define CANVAS_PIXELS           GAME_LEDS

/*---------------------------------------------------------------------------
 * WS2812 driver configuration (ws2812.c reads these)
 *---------------------------------------------------------------------------*/
#define WS2812_ORDER_RGB        0
#define WS2812_ORDER_GRB        1
#define WS2812_ORDER_BRG        2
#define WS2812_ORDER_BGR        3
#define WS2812_ORDER_RBG        4
#define WS2812_ORDER_GBR        5

/* PIOC output pin comes from board_config.h (PC7 on LQFP48, PC18 on TSSOP20) */
#ifndef WS2812_PIOC_PIN
#define WS2812_PIOC_PIN         WSP_WS2812_PIOC_PIN
#endif
#ifndef WS2812_BRIGHTNESS
#define WS2812_BRIGHTNESS       255     /* games do their own 25% scaling */
#endif

/* 0 = linear duty table.  The games blend additively, so a gamma curve would
 * distort the mix; the panel build keeps gamma on, this one does not. */
#define WS2812_GAMMA            0

#define WS2812_COLOR_ORDER      WS2812_ORDER_GRB

#define WS2812_CYC_MIN          48
#define WS2812_CYC_MAX          72

/* Keep the strip under ~350 mA so a plain USB port can feed it. */
#define WSP_CURRENT_LIMIT_MA    350

/* Temporal dithering on the low bits; free in RAM, hides 8 bit banding. */
#define WSP_DITHER              1

/*---------------------------------------------------------------------------
 * Game tuning
 *---------------------------------------------------------------------------*/

/* Global render scale used by Add_Pixel(); 0.25 keeps peak current low and
 * is what the reference design uses. */
#ifndef GAME_BRIGHTNESS
#define GAME_BRIGHTNESS         0.25f
#endif

/* Both buttons held this long switches to the next game.  5 s is far above
 * the 0.5 s charge threshold, so a normal duel exchange (hold, release and
 * fire) does not reach it.  Override with -DGAME_SWITCH_MS=8000 if needed. */
#ifndef GAME_SWITCH_MS
#define GAME_SWITCH_MS          5000u
#endif
#define GAME_SWITCH_FRAMES      ((uint16_t)((GAME_SWITCH_MS + GAME_FRAME_MS - 1u) / GAME_FRAME_MS))

/* Long press threshold for the charged shot in game 1 (0.5 s). */
#define GAME_CHARGE_FRAMES      (GAME_FPS / 2)

#endif /* __GAME_CONFIG_H */
