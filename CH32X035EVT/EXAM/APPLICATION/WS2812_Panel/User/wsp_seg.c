/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_seg.c
 * Description        : WS2812FX style segment engine adapted to a 2D panel.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "wsp_seg.h"
#include "wsp_fx.h"
#include "wsp_app.h"
#include "panel.h"

static wsp_seg_t segments[WSP_SEG_MAX];

/*********************************************************************
 * @fn      seg_init
 *
 * @brief   Reset the whole segment table.
 *
 * @return  none
 */
void seg_init(void)
{
    uint8_t i;

    for (i = 0; i < WSP_SEG_MAX; i++) {
        wsp_seg_t *s = &segments[i];

        s->kind = SEG_RECT;
        s->enabled = 0;
        s->mode = FX_STATIC;
        s->speed = 128;
        s->intensity = 128;
        s->brightness = 255;
        s->reverse = 0;
        s->blend = SEG_BLEND_REPLACE;
        s->x = 0;
        s->y = 0;
        s->w = PANEL_W;
        s->h = PANEL_H;
        s->start = 0;
        s->len = PANEL_PIXELS;
        s->colors[0] = 0xFFFF;
        s->colors[1] = 0x0000;
        s->colors[2] = 0x0000;
        s->seed = 0x1234567u + (uint32_t)i * 0x9E3779B9u;
        s->t0 = 0;
    }
}

void seg_clear(void)
{
    uint8_t i;

    for (i = 0; i < WSP_SEG_MAX; i++) {
        segments[i].enabled = 0;
    }
}

uint8_t seg_count(void)
{
    return WSP_SEG_MAX;
}

wsp_seg_t *seg(uint8_t index)
{
    if (index >= WSP_SEG_MAX) {
        return 0;
    }

    return &segments[index];
}

wsp_seg_t *seg_config_rect(uint8_t index, uint16_t x, uint16_t y,
                           uint16_t w, uint16_t h, uint8_t mode)
{
    wsp_seg_t *s = seg(index);

    if (s == 0) {
        return 0;
    }

    s->kind = SEG_RECT;
    s->x = x;
    s->y = y;
    s->w = (w == 0) ? 1 : w;
    s->h = (h == 0) ? 1 : h;
    s->mode = mode;
    s->enabled = 1;
    s->t0 = 0;

    return s;
}

wsp_seg_t *seg_config_strip(uint8_t index, uint16_t start, uint16_t len,
                            uint8_t mode)
{
    wsp_seg_t *s = seg(index);

    if (s == 0) {
        return 0;
    }

    s->kind = SEG_STRIP;
    s->start = start;
    s->len = (len == 0) ? 1 : len;
    s->mode = mode;
    s->enabled = 1;
    s->t0 = 0;

    return s;
}

void seg_enable(uint8_t index, uint8_t on)
{
    if (index < WSP_SEG_MAX) {
        segments[index].enabled = on ? 1 : 0;
    }
}

void seg_colors(uint8_t index, uint16_t c0, uint16_t c1, uint16_t c2)
{
    if (index < WSP_SEG_MAX) {
        segments[index].colors[0] = c0;
        segments[index].colors[1] = c1;
        segments[index].colors[2] = c2;
    }
}

void seg_reset_time(uint32_t now_ms)
{
    uint8_t i;

    for (i = 0; i < WSP_SEG_MAX; i++) {
        segments[i].t0 = now_ms;
    }
}

/*********************************************************************
 * @fn      seg_cells
 *
 * @brief   Number of cells of a segment.
 *
 * @return  cell count.
 */
uint16_t seg_cells(const wsp_seg_t *s)
{
    if (s->kind == SEG_STRIP) {
        return s->len;
    }

    return (uint16_t)(s->w * s->h);
}

/*********************************************************************
 * @fn      seg_cell_xy
 *
 * @brief   Cell -> local (x, y) inside a rect segment.
 *
 * @return  none
 */
void seg_cell_xy(const wsp_seg_t *s, uint16_t cell, uint16_t *lx, uint16_t *ly)
{
    if (s->w == 0) {
        *lx = 0;
        *ly = 0;
        return;
    }

    *lx = (uint16_t)(cell % s->w);
    *ly = (uint16_t)(cell / s->w);
}

/*********************************************************************
 * @fn      seg_blend
 *
 * @brief   Write one RGB888 pixel with brightness and blending.
 *
 * @return  none
 */
static void seg_blend(const wsp_seg_t *s, uint16_t idx, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t *p;

    if (s->brightness != 255) {
        r = (uint8_t)(((uint16_t)r * s->brightness + 128) >> 8);
        g = (uint8_t)(((uint16_t)g * s->brightness + 128) >> 8);
        b = (uint8_t)(((uint16_t)b * s->brightness + 128) >> 8);
    }

    p = &app_fb()[(uint32_t)idx * 3u];

    switch (s->blend) {
    case SEG_BLEND_ADD:
        p[0] = (uint8_t)((p[0] + r > 255) ? 255 : (p[0] + r));
        p[1] = (uint8_t)((p[1] + g > 255) ? 255 : (p[1] + g));
        p[2] = (uint8_t)((p[2] + b > 255) ? 255 : (p[2] + b));
        break;

    case SEG_BLEND_MAX:
        p[0] = (r > p[0]) ? r : p[0];
        p[1] = (g > p[1]) ? g : p[1];
        p[2] = (b > p[2]) ? b : p[2];
        break;

    case SEG_BLEND_REPLACE:
    default:
        p[0] = r;
        p[1] = g;
        p[2] = b;
        break;
    }
}

/*********************************************************************
 * @fn      seg_put
 *
 * @brief   Write one RGB565 pixel of a segment to the canvas.
 *
 * @return  none
 */
void seg_put(const wsp_seg_t *s, uint16_t cell, uint16_t color565)
{
    uint16_t idx;
    uint16_t phys;
    uint16_t lx, ly;
    uint8_t  r, g, b;

    if (!s->enabled || (cell >= seg_cells(s))) {
        return;
    }

    if (s->kind == SEG_STRIP) {
        phys = s->reverse ? (uint16_t)(s->start + s->len - 1u - cell)
                          : (uint16_t)(s->start + cell);

        if (app_mode() == WSP_MODE_STRIP) {
            idx = phys;
        } else {
            idx = panel_phys_to_logical(phys);
            if (idx == 0xFFFF) {
                return;
            }
        }
    } else {
        if (app_mode() != WSP_MODE_PANEL) {
            return;
        }

        seg_cell_xy(s, cell, &lx, &ly);
        if (((uint16_t)(s->x + lx) >= PANEL_W) || ((uint16_t)(s->y + ly) >= PANEL_H)) {
            return;
        }
        idx = (uint16_t)((s->y + ly) * PANEL_W + (s->x + lx));
    }

    if (idx >= app_pixels()) {
        return;
    }

    r = (uint8_t)((color565 >> 11) & 0x1F);
    g = (uint8_t)((color565 >> 5) & 0x3F);
    b = (uint8_t)(color565 & 0x1F);
    r = (uint8_t)((r << 3) | (r >> 2));
    g = (uint8_t)((g << 2) | (g >> 4));
    b = (uint8_t)((b << 3) | (b >> 2));

    seg_blend(s, idx, r, g, b);
}

/*********************************************************************
 * @fn      seg_period_ms
 *
 * @brief   Effect period derived from the speed field (60..2000 ms).
 *
 * @return  period in milliseconds.
 */
uint16_t seg_period_ms(const wsp_seg_t *s)
{
    return (uint16_t)(2000u - ((uint32_t)s->speed * 1940u) / 255u);
}

/*********************************************************************
 * @fn      seg_step
 *
 * @brief   Number of whole periods elapsed since t0.
 *
 * @return  step counter.
 */
uint16_t seg_step(const wsp_seg_t *s, uint32_t now_ms)
{
    uint16_t period = seg_period_ms(s);

    if (period == 0) {
        return 0;
    }

    return (uint16_t)((now_ms - s->t0) / period);
}

/*********************************************************************
 * @fn      seg_pulse8
 *
 * @brief   Triangle wave 0..255 over one period.
 *
 * @return  level.
 */
uint8_t seg_pulse8(const wsp_seg_t *s, uint32_t now_ms)
{
    uint16_t period = seg_period_ms(s);
    uint32_t phase;
    uint8_t  v;

    if (period == 0) {
        return 0;
    }

    phase = (now_ms - s->t0) % period;
    v = (uint8_t)((phase * 255u) / period);

    return (v < 128) ? (uint8_t)(v << 1) : (uint8_t)((255u - v) << 1);
}

/*********************************************************************
 * @fn      seg_rand
 *
 * @brief   xorshift32 random source, deterministic per segment.
 *
 * @return  random value.
 */
uint32_t seg_rand(wsp_seg_t *s)
{
    uint32_t x = s->seed;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s->seed = x;

    return x;
}

/*********************************************************************
 * @fn      seg_render
 *
 * @brief   Render every enabled segment into the canvas.
 *
 * @return  none
 */
void seg_render(uint32_t now_ms)
{
    uint8_t i;

    for (i = 0; i < WSP_SEG_MAX; i++) {
        if (segments[i].enabled) {
            fx_render(&segments[i], now_ms);
        }
    }
}

/*********************************************************************
 * @fn      seg_service
 *
 * @brief   Render and push to the LEDs. Call every ~10..20 ms.
 *
 * @return  none
 */
void seg_service(uint32_t now_ms)
{
    seg_render(now_ms);
    app_show();
}

/*---------------------------------------------------------------------------
 * Scenes: point / line / plane
 *---------------------------------------------------------------------------*/

/*********************************************************************
 * @fn      seg_scene_point
 *
 * @brief   点：一个亮点沿灯带走线（蛇形路径）移动，带拖尾。
 *
 * @return  none
 */
void seg_scene_point(void)
{
    wsp_seg_t *s;

    seg_clear();
    app_clear();

    s = seg_config_strip(0, 0, app_pixels(), FX_SCANNER);
    s->speed = 160;
    s->intensity = 96;
    s->brightness = 255;
    seg_colors(0, fx_color565(255, 255, 255), fx_color565(0, 0, 0), 0);
}

/*********************************************************************
 * @fn      seg_scene_line
 *
 * @brief   线：一条横向 theater-chase 线 + 一条纵向扫描线（叠加混合）。
 *
 * @return  none
 */
void seg_scene_line(void)
{
    wsp_seg_t *h;
    wsp_seg_t *v;

    seg_clear();
    app_clear();

    /* 面：黑色底 */
    seg_config_rect(0, 0, 0, PANEL_W, PANEL_H, FX_STATIC);
    seg_colors(0, fx_color565(0, 0, 0), 0, 0);

    /* 线：横向 */
    h = seg_config_rect(1, 0, (uint16_t)(PANEL_H / 2 - 1), PANEL_W, 2, FX_THEATER_CHASE);
    h->speed = 140;
    h->intensity = 128;
    seg_colors(1, fx_color565(0, 255, 255), fx_color565(0, 8, 24), 0);

    /* 线：纵向扫描，ADD 叠加 */
    v = seg_config_rect(2, 0, 0, 1, PANEL_H, FX_SCANNER);
    v->x = 0;
    v->w = 1;
    v->h = PANEL_H;
    v->speed = 120;
    v->intensity = 48;
    v->blend = SEG_BLEND_ADD;
    seg_colors(2, fx_color565(255, 180, 0), 0, 0);
}

/*********************************************************************
 * @fn      seg_scene_plane
 *
 * @brief   面：四个象限不同效果 + 叠加一个移动的亮点。
 *
 * @return  none
 */
void seg_scene_plane(void)
{
    wsp_seg_t *s;
    uint16_t hw = PANEL_W / 2;
    uint16_t hh = PANEL_H / 2;

    seg_clear();
    app_clear();

    s = seg_config_rect(0, 0, 0, hw, hh, FX_RAINBOW);
    s->speed = 128;

    s = seg_config_rect(1, hw, 0, (uint16_t)(PANEL_W - hw), hh, FX_PLASMA);
    s->speed = 96;

    s = seg_config_rect(2, 0, hh, hw, (uint16_t)(PANEL_H - hh), FX_FIRE);
    s->speed = 160;
    s->intensity = 160;

    s = seg_config_rect(3, hw, hh, (uint16_t)(PANEL_W - hw),
                        (uint16_t)(PANEL_H - hh), FX_BREATH);
    s->speed = 100;
    seg_colors(3, fx_color565(0, 120, 255), 0, 0);

    /* 叠加层：沿蛇形走线跑的亮点 */
    s = seg_config_strip(4, 0, PANEL_PIXELS, FX_RUNNING_DOT);
    s->speed = 90;
    s->blend = SEG_BLEND_ADD;
    seg_colors(4, fx_color565(255, 255, 255), 0, 0);
}
