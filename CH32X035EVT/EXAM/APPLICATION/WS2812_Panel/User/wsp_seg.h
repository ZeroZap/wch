/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_seg.h
 * Description        : WS2812FX style segment engine adapted to a 2D panel.
 *
 * A segment is either
 *   SEG_RECT  : a rectangle on the panel (点 / 线 / 面), clipped to the panel
 *   SEG_STRIP : a contiguous range of the physical strip. In PANEL mode the
 *               cells follow the serpentine wiring, in STRIP mode they are
 *               the strip positions 1:1.
 *
 * Every segment owns an effect mode, three colours, speed, intensity,
 * brightness and a blend mode, and is rendered into the shared canvas by
 * seg_service().
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_SEG_H
#define __WSP_SEG_H

#include "ch32x035.h"
#include "wsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SEG_RECT                0
#define SEG_STRIP               1

#define SEG_BLEND_REPLACE       0
#define SEG_BLEND_ADD           1
#define SEG_BLEND_MAX           2

typedef struct wsp_seg
{
    uint8_t  kind;          /* SEG_RECT / SEG_STRIP */
    uint8_t  enabled;
    uint8_t  mode;          /* FX_* from wsp_fx.h */
    uint8_t  speed;         /* 0 = slow, 255 = fast */
    uint8_t  intensity;     /* effect specific, 0..255 */
    uint8_t  brightness;    /* 0..255 */
    uint8_t  reverse;
    uint8_t  blend;         /* SEG_BLEND_* */

    uint16_t x, y, w, h;    /* SEG_RECT */
    uint16_t start, len;    /* SEG_STRIP */

    uint16_t colors[3];     /* RGB565 */
    uint32_t seed;          /* pseudo random state */
    uint32_t t0;            /* time origin, ms */
} wsp_seg_t;

void      seg_init(void);
void      seg_clear(void);
uint8_t   seg_count(void);
wsp_seg_t *seg(uint8_t index);

wsp_seg_t *seg_config_rect(uint8_t index, uint16_t x, uint16_t y,
                           uint16_t w, uint16_t h, uint8_t mode);
wsp_seg_t *seg_config_strip(uint8_t index, uint16_t start, uint16_t len,
                            uint8_t mode);
void      seg_enable(uint8_t index, uint8_t on);
void      seg_colors(uint8_t index, uint16_t c0, uint16_t c1, uint16_t c2);
void      seg_reset_time(uint32_t now_ms);

/* ---- used by the effect library ---------------------------------------- */
uint16_t  seg_cells(const wsp_seg_t *s);
uint16_t  seg_cell_at(const wsp_seg_t *s, uint16_t pos);   /* honours reverse */
void      seg_cell_xy(const wsp_seg_t *s, uint16_t cell, uint16_t *lx, uint16_t *ly);
void      seg_put(const wsp_seg_t *s, uint16_t cell, uint16_t color565);

uint16_t  seg_period_ms(const wsp_seg_t *s);               /* 60..2000 ms */
uint16_t  seg_step(const wsp_seg_t *s, uint32_t now_ms);
uint8_t   seg_pulse8(const wsp_seg_t *s, uint32_t now_ms); /* triangle 0..255 */
uint32_t  seg_rand(wsp_seg_t *s);

/* ---- rendering --------------------------------------------------------- */
void      seg_render(uint32_t now_ms);
void      seg_service(uint32_t now_ms);     /* render + push to the LEDs */

/* ---- ready made scenes (PANEL mode) ------------------------------------ */
void      seg_scene_point(void);
void      seg_scene_line(void);
void      seg_scene_plane(void);

#ifdef __cplusplus
}
#endif

#endif /* __WSP_SEG_H */
