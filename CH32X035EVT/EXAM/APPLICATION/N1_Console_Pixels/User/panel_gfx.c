/********************************** (C) COPYRIGHT *******************************
 * File Name          : panel_gfx.c
 * Description        : See panel_gfx.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "panel_gfx.h"
#include "ws2812.h"

typedef struct {
    uint8_t r, g, b;
} rgb_t;

static const rgb_t palette[P_COLOR_COUNT] = {
    {   0,   0,   0 },   /* P_BLACK   */
    { 255, 255, 255 },   /* P_WHITE   */
    { 255,   0,   0 },   /* P_RED     */
    {   0, 255,   0 },   /* P_GREEN   */
    {   0,   0, 255 },   /* P_BLUE    */
    { 255, 255,   0 },   /* P_YELLOW  */
    {   0, 255, 255 },   /* P_CYAN    */
    { 255,   0, 255 },   /* P_MAGENTA */
    { 255,  96,   0 },   /* P_ORANGE  */
    {  96,  96,  96 },   /* P_GRAY    */
    {   0,  96,   0 },   /* P_DGREEN  */
    {   0,   0,  96 },   /* P_DBLUE   */
    {  96,   0,   0 },   /* P_DRED    */
    { 128,   0, 255 },   /* P_PURPLE  */
    {   0, 128, 128 },   /* P_TEAL    */
    { 255,  96, 160 }    /* P_PINK    */
};

/* 3x5 glyphs, one byte per row, bits 2..0 are left..right.
 * Index 0..9 = '0'..'9', 10..35 = 'A'..'Z', then space, '-', '.', ':', '!', '?'. */
static const uint8_t font3x5[][5] = {
    { 0x07, 0x05, 0x05, 0x05, 0x07 }, /* 0 */
    { 0x02, 0x06, 0x02, 0x02, 0x07 }, /* 1 */
    { 0x07, 0x01, 0x07, 0x04, 0x07 }, /* 2 */
    { 0x07, 0x01, 0x07, 0x01, 0x07 }, /* 3 */
    { 0x05, 0x05, 0x07, 0x01, 0x01 }, /* 4 */
    { 0x07, 0x04, 0x07, 0x01, 0x07 }, /* 5 */
    { 0x07, 0x04, 0x07, 0x05, 0x07 }, /* 6 */
    { 0x07, 0x01, 0x01, 0x02, 0x02 }, /* 7 */
    { 0x07, 0x05, 0x07, 0x05, 0x07 }, /* 8 */
    { 0x07, 0x05, 0x07, 0x01, 0x07 }, /* 9 */
    { 0x02, 0x05, 0x07, 0x05, 0x05 }, /* A */
    { 0x06, 0x05, 0x06, 0x05, 0x06 }, /* B */
    { 0x03, 0x04, 0x04, 0x04, 0x03 }, /* C */
    { 0x06, 0x05, 0x05, 0x05, 0x06 }, /* D */
    { 0x07, 0x04, 0x06, 0x04, 0x07 }, /* E */
    { 0x07, 0x04, 0x06, 0x04, 0x04 }, /* F */
    { 0x03, 0x04, 0x05, 0x05, 0x03 }, /* G */
    { 0x05, 0x05, 0x07, 0x05, 0x05 }, /* H */
    { 0x07, 0x02, 0x02, 0x02, 0x07 }, /* I */
    { 0x01, 0x01, 0x01, 0x05, 0x02 }, /* J */
    { 0x05, 0x05, 0x06, 0x05, 0x05 }, /* K */
    { 0x04, 0x04, 0x04, 0x04, 0x07 }, /* L */
    { 0x05, 0x07, 0x07, 0x05, 0x05 }, /* M */
    { 0x05, 0x07, 0x07, 0x07, 0x05 }, /* N */
    { 0x02, 0x05, 0x05, 0x05, 0x02 }, /* O */
    { 0x06, 0x05, 0x06, 0x04, 0x04 }, /* P */
    { 0x02, 0x05, 0x05, 0x06, 0x03 }, /* Q */
    { 0x06, 0x05, 0x06, 0x05, 0x05 }, /* R */
    { 0x03, 0x04, 0x02, 0x01, 0x06 }, /* S */
    { 0x07, 0x02, 0x02, 0x02, 0x02 }, /* T */
    { 0x05, 0x05, 0x05, 0x05, 0x07 }, /* U */
    { 0x05, 0x05, 0x05, 0x05, 0x02 }, /* V */
    { 0x05, 0x05, 0x07, 0x07, 0x05 }, /* W */
    { 0x05, 0x05, 0x02, 0x05, 0x05 }, /* X */
    { 0x05, 0x05, 0x02, 0x02, 0x02 }, /* Y */
    { 0x07, 0x01, 0x02, 0x04, 0x07 }, /* Z */
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, /* space */
    { 0x00, 0x00, 0x07, 0x00, 0x00 }, /* - */
    { 0x00, 0x00, 0x00, 0x00, 0x02 }, /* . */
    { 0x00, 0x02, 0x00, 0x02, 0x00 }, /* : */
    { 0x02, 0x02, 0x02, 0x00, 0x02 }, /* ! */
    { 0x06, 0x01, 0x02, 0x00, 0x02 }  /* ? */
};

static uint8_t fb[PANEL_PIXELS];
static uint8_t rgb[PANEL_PIXELS * 3u];

static uint8_t glyph_index(char c)
{
    if (c >= '0' && c <= '9') {
        return (uint8_t)(c - '0');
    }
    if (c >= 'A' && c <= 'Z') {
        return (uint8_t)(10 + (c - 'A'));
    }
    if (c >= 'a' && c <= 'z') {
        return (uint8_t)(10 + (c - 'a'));
    }

    switch (c) {
    case '-': return 37u;
    case '.': return 38u;
    case ':': return 39u;
    case '!': return 40u;
    case '?': return 41u;
    default:  return 36u;   /* space */
    }
}

static int logical_x(int x, int y)
{
    int lx = x;

#if PANEL_FLIP_X
    lx = (PANEL_W - 1) - lx;
#endif
#if PANEL_SERPENTINE
    if (y & 1) {
        lx = (PANEL_W - 1) - lx;
    }
#endif
#if PANEL_START_RIGHT
    lx = (PANEL_W - 1) - lx;
#endif

    return lx;
}

static int logical_y(int y)
{
#if PANEL_FLIP_Y
    return (PANEL_H - 1) - y;
#else
    (void)y;
    return y;
#endif
}

void panel_init(void)
{
    memset(fb, 0, sizeof(fb));
    memset(rgb, 0, sizeof(rgb));
}

void panel_clear(void)
{
    memset(fb, 0, sizeof(fb));
}

void panel_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || y < 0 || x >= PANEL_W || y >= PANEL_H) {
        return;
    }

    fb[y * PANEL_W + x] = color;
}

uint8_t panel_get(int x, int y)
{
    if (x < 0 || y < 0 || x >= PANEL_W || y >= PANEL_H) {
        return P_BLACK;
    }

    return fb[y * PANEL_W + x];
}

void panel_hline(int x, int y, int w, uint8_t color)
{
    int i;

    for (i = 0; i < w; i++) {
        panel_pixel(x + i, y, color);
    }
}

void panel_vline(int x, int y, int h, uint8_t color)
{
    int i;

    for (i = 0; i < h; i++) {
        panel_pixel(x, y + i, color);
    }
}

void panel_rect(int x, int y, int w, int h, uint8_t color)
{
    int i;

    for (i = 0; i < h; i++) {
        panel_hline(x, y + i, w, color);
    }
}

void panel_frame(int x, int y, int w, int h, uint8_t color)
{
    panel_hline(x, y, w, color);
    panel_hline(x, y + h - 1, w, color);
    panel_vline(x, y, h, color);
    panel_vline(x + w - 1, y, h, color);
}

void panel_text3x5(int x, int y, const char *s, uint8_t color)
{
    while (*s) {
        uint8_t idx = glyph_index(*s);
        uint8_t row;

        for (row = 0; row < 5u; row++) {
            uint8_t bits = font3x5[idx][row];

            if (bits & 0x04u) panel_pixel(x + 0, y + row, color);
            if (bits & 0x02u) panel_pixel(x + 1, y + row, color);
            if (bits & 0x01u) panel_pixel(x + 2, y + row, color);
        }

        x += 4;
        s++;
    }
}

uint8_t panel_text3x5_width(const char *s)
{
    uint8_t n = 0u;

    while (*s++) {
        n++;
    }

    return n ? (uint8_t)(n * 4u - 1u) : 0u;
}

void panel_text3x5_center(int y, const char *s, uint8_t color)
{
    int x = (PANEL_W - (int)panel_text3x5_width(s)) / 2;

    panel_text3x5(x, y, s, color);
}

void panel_flush(void)
{
    uint8_t *p = rgb;
    int y;

    for (y = 0; y < PANEL_H; y++) {
        int i;

        for (i = 0; i < PANEL_W; i++) {
            int lx = logical_x(i, y);
            int ly = logical_y(y);
            uint8_t c = fb[ly * PANEL_W + lx];
            const rgb_t *col = &palette[c % P_COLOR_COUNT];

            *p++ = col->r;
            *p++ = col->g;
            *p++ = col->b;
        }
    }

    ws2812_show(rgb, PANEL_PIXELS);
}

void panel_set_brightness(uint8_t level)
{
    ws2812_set_brightness(level);
}

uint8_t panel_brightness(void)
{
    return ws2812_brightness();
}
