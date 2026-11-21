/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_fx.h
 * Description        : Effect library for the segment engine (WS2812FX style).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_FX_H
#define __WSP_FX_H

#include "ch32x035.h"
#include "wsp_seg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FX_STATIC               0
#define FX_BLINK                1
#define FX_BREATH               2
#define FX_RAINBOW              3
#define FX_WIPE                 4
#define FX_SCANNER              5       /* comet with fading tail   */
#define FX_THEATER_CHASE        6
#define FX_RUNNING_DOT          7
#define FX_FIRE                 8
#define FX_PLASMA               9
#define FX_SPARKLE              10
#define FX_GRADIENT             11
#define FX_WAVE                 12      /* travelling brightness wave */
#define FX_CHASER               13      /* 3 evenly spaced comets    */
#define FX_BOUNCE               14      /* ping-pong dot with glow   */
#define FX_METEOR               15      /* falling heads, hue cycling */
#define FX_MODE_COUNT           16

/* colour helpers */
uint16_t fx_color565(uint8_t r, uint8_t g, uint8_t b);
uint16_t fx_hsv565(uint8_t h, uint8_t s, uint8_t v);
uint16_t fx_scale565(uint16_t c, uint8_t level);
uint16_t fx_mix565(uint16_t a, uint16_t b, uint8_t t);

/* render one segment at time now_ms */
void     fx_render(wsp_seg_t *s, uint32_t now_ms);

/* human readable effect name */
const char *fx_name(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif /* __WSP_FX_H */
