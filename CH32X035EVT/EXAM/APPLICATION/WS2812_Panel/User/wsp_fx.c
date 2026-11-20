/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_fx.c
 * Description        : Effect library for the segment engine.
 *
 * Every effect paints a full segment (foreground and background), so the
 * blending is done by seg_put() and segments can be stacked: a later segment
 * with SEG_BLEND_ADD works as an overlay (see seg_scene_plane()).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "wsp_fx.h"

/* 64 entry sine table, 128 + 127*sin(2*pi*i/64) */
static const uint8_t fx_sin8[64] = {
    128, 140, 152, 165, 176, 188, 198, 208,
    218, 226, 234, 240, 245, 250, 253, 254,
    255, 254, 253, 250, 245, 240, 234, 226,
    218, 208, 198, 188, 176, 165, 152, 140,
    128, 115, 103,  90,  79,  67,  57,  47,
     37,  29,  21,  15,  10,   5,   2,   1,
      0,   1,   2,   5,  10,  15,  21,  29,
     37,  47,  57,  67,  79,  90, 103, 115,
};

/* flame palette: 8 steps from black to white-yellow */
static const uint16_t fx_fire_pal[8] = {
    0x0000, 0x2000, 0x4000, 0x6100, 0x8300, 0xA500, 0xE680, 0xFFE0
};

/* heat field, only used by FX_FIRE (one fire segment at a time) */
static uint8_t fx_heat[CANVAS_PIXELS];

/*---------------------------------------------------------------------------
 * colours
 *---------------------------------------------------------------------------*/
uint16_t fx_color565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((uint16_t)(r & 0xF8) << 8) |
                      ((uint16_t)(g & 0xFC) << 3) |
                      (b >> 3));
}

uint16_t fx_scale565(uint16_t c, uint8_t level)
{
    uint16_t r = (uint16_t)(((c >> 11) & 0x1F) * level >> 8);
    uint16_t g = (uint16_t)(((c >> 5) & 0x3F) * level >> 8);
    uint16_t b = (uint16_t)((c & 0x1F) * level >> 8);

    return (uint16_t)((r << 11) | (g << 5) | b);
}

uint16_t fx_mix565(uint16_t a, uint16_t b, uint8_t t)
{
    int16_t ar = (int16_t)((a >> 11) & 0x1F);
    int16_t ag = (int16_t)((a >> 5) & 0x3F);
    int16_t ab = (int16_t)(a & 0x1F);
    int16_t br = (int16_t)((b >> 11) & 0x1F);
    int16_t bg = (int16_t)((b >> 5) & 0x3F);
    int16_t bb = (int16_t)(b & 0x1F);
    int16_t r = (int16_t)(ar + (((br - ar) * t) >> 8));
    int16_t g = (int16_t)(ag + (((bg - ag) * t) >> 8));
    int16_t bl = (int16_t)(ab + (((bb - ab) * t) >> 8));

    return (uint16_t)(((uint16_t)r << 11) | ((uint16_t)g << 5) | (uint16_t)bl);
}

uint16_t fx_hsv565(uint8_t h, uint8_t s, uint8_t v)
{
    uint8_t region, remainder, p, q, t;
    uint8_t r, g, b;

    if (s == 0) {
        return fx_color565(v, v, v);
    }

    region = (uint8_t)(h / 43);
    remainder = (uint8_t)((h - region * 43u) * 6u);
    p = (uint8_t)(((uint16_t)v * (255u - s)) >> 8);
    q = (uint8_t)(((uint16_t)v * (255u - (((uint16_t)s * remainder) >> 8))) >> 8);
    t = (uint8_t)(((uint16_t)v * (255u - (((uint16_t)s * (255u - remainder)) >> 8))) >> 8);

    switch (region) {
    case 0:
        r = v; g = t; b = p;
        break;
    case 1:
        r = q; g = v; b = p;
        break;
    case 2:
        r = p; g = v; b = t;
        break;
    case 3:
        r = p; g = q; b = v;
        break;
    case 4:
        r = t; g = p; b = v;
        break;
    default:
        r = v; g = p; b = q;
        break;
    }

    return fx_color565(r, g, b);
}

/*---------------------------------------------------------------------------
 * helpers
 *---------------------------------------------------------------------------*/
static void fx_fill(wsp_seg_t *s, uint16_t color565)
{
    uint16_t cells = seg_cells(s);
    uint16_t i;

    for (i = 0; i < cells; i++) {
        seg_put(s, i, color565);
    }
}

/* position that ping-pongs between 0 and cells-1 */
static uint16_t fx_pingpong(uint32_t step, uint16_t cells)
{
    uint32_t span;

    if (cells < 2) {
        return 0;
    }

    span = (uint32_t)cells * 2u - 2u;
    step %= span;

    return (uint16_t)((step < cells) ? step : (span - step));
}

/*---------------------------------------------------------------------------
 * effects
 *---------------------------------------------------------------------------*/
static void fx_static(wsp_seg_t *s)
{
    fx_fill(s, s->colors[0]);
}

static void fx_blink(wsp_seg_t *s, uint32_t now)
{
    uint32_t on = ((now - s->t0) / seg_period_ms(s)) & 1u;

    fx_fill(s, on ? s->colors[0] : s->colors[1]);
}

static void fx_breath(wsp_seg_t *s, uint32_t now)
{
    fx_fill(s, fx_scale565(s->colors[0], seg_pulse8(s, now)));
}

static void fx_rainbow(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t step = seg_step(s, now);
    uint16_t lx, ly;
    uint16_t i;
    uint8_t  hue;

    for (i = 0; i < cells; i++) {
        if (s->kind == SEG_RECT) {
            seg_cell_xy(s, i, &lx, &ly);
            hue = (uint8_t)(lx * 8u + ly * 4u + step * 4u);
        } else {
            hue = (uint8_t)((uint32_t)i * 256u / cells + step * 4u);
        }
        seg_put(s, i, fx_hsv565(hue, 255, 255));
    }
}

static void fx_wipe(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t period = seg_period_ms(s);
    uint32_t phase = (now - s->t0) % period;
    uint16_t head = (uint16_t)((phase * cells) / period);
    uint16_t pos;

    if (head >= cells) {
        head = (uint16_t)(cells - 1u);
    }

    for (pos = 0; pos < cells; pos++) {
        seg_put(s, seg_cell_at(s, pos), (pos <= head) ? s->colors[0] : s->colors[1]);
    }
}

static void fx_scanner(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t tail = (uint16_t)(1u + (s->intensity >> 4));
    uint16_t head = fx_pingpong(seg_step(s, now), cells);
    uint16_t pos;
    uint16_t d;
    uint8_t  lvl;
    uint16_t color;

    for (pos = 0; pos < cells; pos++) {
        d = (head > pos) ? (uint16_t)(head - pos) : (uint16_t)(pos - head);
        if (d < tail) {
            lvl = (uint8_t)(255u - ((uint32_t)d * 255u) / tail);
            color = fx_scale565(s->colors[0], lvl);
        } else {
            color = s->colors[1];
        }
        seg_put(s, seg_cell_at(s, pos), color);
    }
}

static void fx_theater_chase(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t step = seg_step(s, now);
    uint16_t pos;

    for (pos = 0; pos < cells; pos++) {
        uint8_t on = (((pos + step) % 3u) == 0) ? 1 : 0;
        seg_put(s, seg_cell_at(s, pos), on ? s->colors[0] : s->colors[1]);
    }
}

static void fx_running_dot(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t head;
    uint16_t pos;

    if (cells == 0) {
        return;
    }

    head = (uint16_t)(seg_step(s, now) % cells);

    for (pos = 0; pos < cells; pos++) {
        seg_put(s, seg_cell_at(s, pos), (pos == head) ? s->colors[0] : s->colors[1]);
    }
}

static void fx_fire(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t i;

    (void)now;

    if (cells == 0 || cells > CANVAS_PIXELS) {
        return;
    }

    /* cool down */
    for (i = 0; i < cells; i++) {
        fx_heat[i] = (fx_heat[i] > 24u) ? (uint8_t)(fx_heat[i] - 24u) : 0;
    }

    if (s->kind == SEG_RECT) {
        uint16_t x, y;

        /* heat floats upwards: row y takes energy from row y+1 */
        for (y = 0; y + 1u < s->h; y++) {
            for (x = 0; x < s->w; x++) {
                uint16_t idx = (uint16_t)(y * s->w + x);
                fx_heat[idx] = (uint8_t)(((uint16_t)fx_heat[idx] * 3u + fx_heat[idx + s->w]) >> 2);
            }
        }

        /* sparks at the bottom row */
        for (x = 0; x < s->w; x++) {
            if ((seg_rand(s) & 0xFFu) < s->intensity) {
                fx_heat[(uint16_t)((s->h - 1u) * s->w + x)] = 255;
            }
        }
    } else {
        /* linear strip: heat drifts towards index 0 */
        for (i = cells - 1u; i > 0; i--) {
            fx_heat[i] = (uint8_t)(((uint16_t)fx_heat[i] * 3u + fx_heat[i - 1u]) >> 2);
        }
        if ((seg_rand(s) & 0xFFu) < s->intensity) {
            fx_heat[cells - 1u] = 255;
        }
    }

    for (i = 0; i < cells; i++) {
        seg_put(s, i, fx_fire_pal[fx_heat[i] >> 5]);
    }
}

static void fx_plasma(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t step = seg_step(s, now);
    uint16_t i, lx, ly;
    uint8_t  a, b, hue;

    for (i = 0; i < cells; i++) {
        if (s->kind == SEG_RECT) {
            seg_cell_xy(s, i, &lx, &ly);
        } else {
            lx = i;
            ly = 0;
        }

        a = fx_sin8[(uint8_t)(lx * 8u + step * 6u) & 0x3Fu];
        b = fx_sin8[(uint8_t)(ly * 10u + step * 4u) & 0x3Fu];
        hue = (uint8_t)(a + b);

        seg_put(s, i, fx_hsv565(hue, 220, 255));
    }
}

static void fx_sparkle(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t i;
    uint8_t  threshold = (uint8_t)(4u + (s->intensity >> 3));

    (void)now;

    for (i = 0; i < cells; i++) {
        uint8_t hit = ((seg_rand(s) & 0xFFu) < threshold) ? 1 : 0;
        seg_put(s, i, hit ? s->colors[0] : s->colors[1]);
    }
}

static void fx_gradient(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t step = seg_step(s, now);
    uint16_t pos;
    uint16_t t;

    if (cells == 0) {
        return;
    }

    for (pos = 0; pos < cells; pos++) {
        t = (uint16_t)(((uint32_t)pos * 255u) / cells + step * 8u);
        seg_put(s, seg_cell_at(s, pos),
                fx_mix565(s->colors[0], s->colors[1], (uint8_t)t));
    }
}

/*---------------------------------------------------------------------------
 * 12..15: added later, all stateless (position derived from step time)
 *---------------------------------------------------------------------------*/

/*********************************************************************
 * @fn      fx_wave
 *
 * @brief   One brightness wave travelling along the segment.
 *
 * @return  none
 */
static void fx_wave(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t step = seg_step(s, now);
    uint16_t i;
    uint8_t  phase;
    uint8_t  level;

    if (cells == 0) {
        return;
    }

    for (i = 0; i < cells; i++) {
        phase = (uint8_t)(((uint32_t)i * 192u) / cells + (uint32_t)step * 6u);
        level = fx_sin8[(phase >> 2) & 0x3Fu];
        seg_put(s, i, fx_scale565(s->colors[0], level));
    }
}

/*********************************************************************
 * @fn      fx_chaser
 *
 * @brief   Three evenly spaced heads chasing in the same direction.
 *
 * @return  none
 */
static void fx_chaser(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t step = seg_step(s, now);
    uint16_t tail = (uint16_t)(1u + (s->intensity >> 5));
    uint16_t i, g, head, d;
    uint8_t  lit;

    if (cells < 3u) {
        return;
    }

    for (i = 0; i < cells; i++) {
        lit = 0;
        for (g = 0; g < 3u; g++) {
            head = (uint16_t)(((uint32_t)step * 2u +
                               ((uint32_t)g * cells) / 3u) % cells);
            d = (head >= i) ? (uint16_t)(head - i)
                            : (uint16_t)(cells - i + head);
            if (d < tail) {
                lit = 1;
            }
        }
        seg_put(s, i, lit ? s->colors[0] : s->colors[1]);
    }
}

/*********************************************************************
 * @fn      fx_bounce
 *
 * @brief   One dot ping-ponging across the segment with a symmetric glow.
 *
 * @return  none
 */
static void fx_bounce(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t head = fx_pingpong((uint32_t)seg_step(s, now) * 3u, cells);
    uint16_t glow = (uint16_t)(2u + (s->intensity >> 3));
    uint16_t i, d;
    uint8_t  level;

    for (i = 0; i < cells; i++) {
        d = (head >= i) ? (uint16_t)(head - i) : (uint16_t)(i - head);
        if (d <= glow) {
            level = (uint8_t)(255u - ((uint32_t)d * 220u) / (glow + 1u));
            seg_put(s, i, fx_scale565(s->colors[0], level));
        } else {
            seg_put(s, i, s->colors[1]);
        }
    }
}

/*********************************************************************
 * @fn      fx_meteor
 *
 * @brief   Two heads falling in one direction, tail decaying, hue drifts.
 *
 * @return  none
 */
static void fx_meteor(wsp_seg_t *s, uint32_t now)
{
    uint16_t cells = seg_cells(s);
    uint16_t step = seg_step(s, now);
    uint16_t tail = (uint16_t)(2u + (s->intensity >> 4));
    uint16_t span = (uint16_t)(cells + tail);
    uint16_t i, g, head, d;
    uint8_t  level;
    uint16_t head_color;

    if ((cells == 0) || (span == 0)) {
        return;
    }

    head_color = fx_hsv565((uint8_t)(step * 8u), 255, 255);

    for (i = 0; i < cells; i++) {
        level = 0;
        for (g = 0; g < 2u; g++) {
            head = (uint16_t)(((uint32_t)step * 3u +
                               ((uint32_t)g * span) / 2u) % span);

            if (head >= i) {
                d = (uint16_t)(head - i);
                if (d <= tail) {
                    uint8_t l = (uint8_t)(200u - ((uint32_t)d * 180u) / (tail + 1u));

                    if (l > level) {
                        level = l;
                    }
                }
            }
        }

        if (level != 0) {
            seg_put(s, i, fx_scale565(head_color, level));
        } else {
            seg_put(s, i, s->colors[1]);
        }
    }
}

/*---------------------------------------------------------------------------
 * dispatch
 *---------------------------------------------------------------------------*/
void fx_render(wsp_seg_t *s, uint32_t now_ms)
{
    switch (s->mode) {
    case FX_BLINK:
        fx_blink(s, now_ms);
        break;
    case FX_BREATH:
        fx_breath(s, now_ms);
        break;
    case FX_RAINBOW:
        fx_rainbow(s, now_ms);
        break;
    case FX_WIPE:
        fx_wipe(s, now_ms);
        break;
    case FX_SCANNER:
        fx_scanner(s, now_ms);
        break;
    case FX_THEATER_CHASE:
        fx_theater_chase(s, now_ms);
        break;
    case FX_RUNNING_DOT:
        fx_running_dot(s, now_ms);
        break;
    case FX_FIRE:
        fx_fire(s, now_ms);
        break;
    case FX_PLASMA:
        fx_plasma(s, now_ms);
        break;
    case FX_SPARKLE:
        fx_sparkle(s, now_ms);
        break;
    case FX_GRADIENT:
        fx_gradient(s, now_ms);
        break;
    case FX_WAVE:
        fx_wave(s, now_ms);
        break;
    case FX_CHASER:
        fx_chaser(s, now_ms);
        break;
    case FX_BOUNCE:
        fx_bounce(s, now_ms);
        break;
    case FX_METEOR:
        fx_meteor(s, now_ms);
        break;
    case FX_STATIC:
    default:
        fx_static(s);
        break;
    }
}

const char *fx_name(uint8_t mode)
{
    static const char *names[FX_MODE_COUNT] = {
        "static", "blink", "breath", "rainbow", "wipe", "scanner",
        "theater", "dot", "fire", "plasma", "sparkle", "gradient",
        "wave", "chaser", "bounce", "meteor"
    };

    if (mode >= FX_MODE_COUNT) {
        return "?";
    }

    return names[mode];
}
