/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_config.h
 * Description        : Compile-time configuration of the WS2812 driver and the
 *                      console geometry.
 *
 * This header is deliberately named game_config.h because the copied
 * WS2812_Panel_IAP driver expects wsp_config.h -> game_config.h.  Only the
 * panel / driver knobs live here; menu timing and storage offsets are in
 * console_config.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __GAME_CONFIG_H
#define __GAME_CONFIG_H

#include "board_config.h"

#if !N1_BOARD_HAS_DEBUG_UART
/* no bonded-out UART TX on TSSOP20: compile the log calls away */
#define printf(...) ((void)0)
#endif

/*---------------------------------------------------------------------------
 * Panel geometry
 *
 * The default WS2812_Panel_IAP board is a 16x16 serpentine panel.  Keep the
 * same geometry so the games run on the existing hardware without rewiring.
 * 16x32 works too (fewer fps, more RAM); 32x32 needs a smaller scratch
 * framebuffer and a stronger 5 V supply.
 *---------------------------------------------------------------------------*/
#ifndef PANEL_W
#define PANEL_W                 16
#endif
#ifndef PANEL_H
#define PANEL_H                 16
#endif

#define PANEL_SERPENTINE        1       /* 1 = row 1 is wired right -> left */
#define PANEL_START_RIGHT       0
#define PANEL_FLIP_X            0
#define PANEL_FLIP_Y            0

#define PANEL_PIXELS            (PANEL_W * PANEL_H)

#if (PANEL_PIXELS > 1024)
#error "PIOC 1-wire mode supports at most 1024 RGB pixels"
#endif

#define CANVAS_PIXELS           PANEL_PIXELS

/*---------------------------------------------------------------------------
 * WS2812 driver (ws2812.c)
 *---------------------------------------------------------------------------*/
#define WS2812_ORDER_RGB        0
#define WS2812_ORDER_GRB        1
#define WS2812_ORDER_BRG        2
#define WS2812_ORDER_BGR        3
#define WS2812_ORDER_RBG        4
#define WS2812_ORDER_GBR        5

#ifndef WS2812_COLOR_ORDER
#define WS2812_COLOR_ORDER      WS2812_ORDER_GRB
#endif

/* Games mix colours additively, so keep the gamma table out of the way.
 * Brightness is handled once in ws2812_set_brightness(). */
#define WS2812_GAMMA            0

#ifndef WS2812_BRIGHTNESS
#define WS2812_BRIGHTNESS       64      /* 25% of full scale */
#endif

#define WS2812_CYC_MIN          48
#define WS2812_CYC_MAX          72

#ifndef WS2812_WIRE_CYCLES
#define WS2812_WIRE_CYCLES      56
#endif

/* 16x16 all-white at brightness 64 is still several amps, so cap the frame
 * at 500 mA: a plain USB-C port and the 470 uF bulk capacitor stay happy. */
#ifndef WSP_CURRENT_LIMIT_MA
#define WSP_CURRENT_LIMIT_MA    500u
#endif

#define WSP_DITHER              1

/* ws2812.c wants a pixel count for its transmit buffer */
#define STRIP_LEDS              PANEL_PIXELS

#endif /* __GAME_CONFIG_H */
