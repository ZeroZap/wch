/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_osd.c
 * Description        : Frame rate / current limit badge, see wsp_osd.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "wsp_osd.h"
#include "wsp_config.h"
#include "panel.h"
#include "gfx.h"
#include "wsp_font.h"
#include "wsp_text.h"
#include "wsp_tick.h"
#include "ws2812.h"

#define OSD_COLOR           RGB565(255, 255, 255)
#define OSD_LIMIT_COLOR     RGB565(255, 0, 0)
#define OSD_WINDOW_MS       500u

static uint8_t  osd_en = WSP_OSD_DEFAULT;
static uint32_t osd_frames;
static uint32_t osd_t0;
static uint16_t osd_rate;

void osd_init(void)
{
    osd_frames = 0;
    osd_t0     = tick_ms();
    osd_rate   = 0;
}

void osd_set(uint8_t on)
{
    osd_en = on ? 1u : 0u;
}

uint8_t osd_on(void)
{
    return osd_en;
}

uint16_t osd_fps(void)
{
    return osd_rate;
}

void osd_tick(void)
{
    uint32_t now = tick_ms();
    uint32_t elapsed = now - osd_t0;

    osd_frames++;

    if (elapsed >= OSD_WINDOW_MS) {
        uint32_t fps = (osd_frames * 1000u) / elapsed;

        osd_rate = (uint16_t)((fps > 999u) ? 999u : fps);
        osd_frames = 0;
        osd_t0 = now;
    }
}

void osd_draw(void)
{
    char     text[3];
    uint16_t value;
    uint16_t x;

    if (osd_en == 0) {
        return;
    }

    /* the badge needs one text row and at least one glyph column */
    if ((PANEL_H < WSP_FONT_H) || (PANEL_W < WSP_FONT_W)) {
        return;
    }

    value = (osd_rate > 99u) ? 99u : osd_rate;

    /* two digits fit from 11 columns up (5 + 1 gap + 5); below that show the
     * units digit only, which still tells you roughly what the frame rate is */
    if (PANEL_W >= (uint16_t)(2u * TEXT_ADVANCE - 1u)) {
        text[0] = (char)('0' + (value / 10u));
        text[1] = (char)('0' + (value % 10u));
        text[2] = '\0';
    } else {
        text[0] = (char)('0' + (value % 10u));
        text[1] = '\0';
    }

    x = (uint16_t)(PANEL_W - text_width(text));
    (void)text_draw(x, 0, text, OSD_COLOR);

    /* bottom-right dot while the current limiter is dimming the frame */
    if ((ws2812_limited() != 0) && (PANEL_W >= 2) && (PANEL_H >= 2)) {
        panel_set_pixel565((uint16_t)(PANEL_W - 2), (uint16_t)(PANEL_H - 1),
                           OSD_LIMIT_COLOR);
        panel_set_pixel565((uint16_t)(PANEL_W - 1), (uint16_t)(PANEL_H - 1),
                           OSD_LIMIT_COLOR);
        panel_set_pixel565((uint16_t)(PANEL_W - 2), (uint16_t)(PANEL_H - 2),
                           OSD_LIMIT_COLOR);
        panel_set_pixel565((uint16_t)(PANEL_W - 1), (uint16_t)(PANEL_H - 2),
                           OSD_LIMIT_COLOR);
    }
}
