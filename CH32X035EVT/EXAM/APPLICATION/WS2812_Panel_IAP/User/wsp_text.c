/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_text.c
 * Description        : 5x7 text drawing, see wsp_text.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "wsp_text.h"
#include "wsp_font.h"
#include "panel.h"

uint16_t text_char(uint16_t x, uint16_t y, char c, uint16_t color565)
{
    const uint8_t *glyph;
    uint8_t        idx;
    uint8_t        col, row;

    if ((c < 32) || (c > 127)) {
        return 0;
    }

    idx = wsp_font_index[(uint8_t)c - 32u];
    if (idx == WSP_FONT_MISSING) {
        return 0;
    }

    glyph = wsp_font_data[idx];

    for (col = 0; col < WSP_FONT_W; col++) {
        uint8_t bits = glyph[col];

        for (row = 0; row < WSP_FONT_H; row++) {
            if (bits & (uint8_t)(1u << row)) {
                panel_set_pixel565((uint16_t)(x + col), (uint16_t)(y + row),
                                   color565);
            }
        }
    }

    return TEXT_ADVANCE;
}

uint16_t text_draw(uint16_t x, uint16_t y, const char *s, uint16_t color565)
{
    while (*s) {
        x = (uint16_t)(x + text_char(x, y, *s, color565));
        s++;
    }

    return x;
}

uint16_t text_width(const char *s)
{
    uint16_t n = 0;

    while (*s) {
        n++;
        s++;
    }

    return (n == 0) ? 0 : (uint16_t)(n * TEXT_ADVANCE - 1u);
}
