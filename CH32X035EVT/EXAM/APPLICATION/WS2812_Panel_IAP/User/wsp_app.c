/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_app.c
 * Description        : Display mode (STRIP / PANEL) and canvas access.
 *
 * Both modes share panel_fb[]:
 *   PANEL : index is the logical pixel, panel_show() walks the serpentine map.
 *   STRIP : index is the physical position, ws2812_show() sends it 1:1.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>
#include "wsp_app.h"
#include "panel.h"
#include "ws2812.h"

#if WSP_OSD_ENABLE
#include "wsp_osd.h"
#endif

static uint8_t app_display_mode = WSP_DEFAULT_MODE;

void app_set_mode(uint8_t mode)
{
    app_display_mode = (mode == WSP_MODE_STRIP) ? WSP_MODE_STRIP : WSP_MODE_PANEL;
    app_clear();
}

uint8_t app_mode(void)
{
    return app_display_mode;
}

uint16_t app_pixels(void)
{
    return (app_display_mode == WSP_MODE_STRIP) ? STRIP_LEDS : PANEL_PIXELS;
}

void app_clear(void)
{
    memset(panel_fb, 0, (uint32_t)app_pixels() * 3u);
}

void app_put(uint16_t idx, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t *p;

    if (idx >= app_pixels()) {
        return;
    }

    p = &panel_fb[(uint32_t)idx * 3u];
    p[0] = r;
    p[1] = g;
    p[2] = b;
}

uint8_t *app_fb(void)
{
    return panel_fb;
}

void app_show(void)
{
    if (app_display_mode == WSP_MODE_STRIP) {
#if WSP_OSD_ENABLE
        /* panel_show() stamps the badge itself; the strip path bypasses it,
         * so do it here to keep the badge visible in both modes */
        osd_draw();
        osd_tick();
#endif
        ws2812_show(panel_fb, STRIP_LEDS);
    } else {
        panel_show();
    }
}
