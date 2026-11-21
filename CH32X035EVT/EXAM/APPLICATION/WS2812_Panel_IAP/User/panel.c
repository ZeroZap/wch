/********************************** (C) COPYRIGHT *******************************
 * File Name          : panel.c
 * Description        : Serpentine RGB panel framebuffer.
 *
 * Physical wiring
 * ---------------
 * One continuous strip of PANEL_W * PANEL_H LEDs folded into rows.  The first
 * LED of the strip is the left (or right, PANEL_START_RIGHT) end of row 0.
 * Every odd row is wired in the opposite direction:
 *
 *      row 0 :  0   1   2  ... 15
 *      row 1 : 31  30  29  ... 16
 *      row 2 : 32  33  34  ... 47
 *
 * panel_map[] translates a position in the strip (0 .. PANEL_PIXELS-1) into
 * an index of the logical framebuffer (y * PANEL_W + x), so drawing code can
 * always work in normal screen coordinates.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "panel.h"
#include "ws2812.h"

#if WSP_OSD_ENABLE
#include "wsp_osd.h"
#endif

uint8_t panel_fb[CANVAS_PIXELS * 3];

/* strip position -> logical framebuffer index */
static uint16_t panel_map[PANEL_PIXELS];

/*********************************************************************
 * @fn      panel_init
 *
 * @brief   Build the strip -> framebuffer mapping table.
 *
 * @return  none
 */
void panel_init(void)
{
    uint16_t x, y, col, lx, ly;

    for (y = 0; y < PANEL_H; y++) {
        for (col = 0; col < PANEL_W; col++) {
            x = col;
            if (PANEL_SERPENTINE && (y & 1u)) {
                x = (uint16_t)(PANEL_W - 1u - x);
            }
            if (PANEL_START_RIGHT) {
                x = (uint16_t)(PANEL_W - 1u - x);
            }

            lx = x;
            ly = y;
            if (PANEL_FLIP_X) {
                lx = (uint16_t)(PANEL_W - 1u - lx);
            }
            if (PANEL_FLIP_Y) {
                ly = (uint16_t)(PANEL_H - 1u - ly);
            }

            panel_map[y * PANEL_W + col] = (uint16_t)(ly * PANEL_W + lx);
        }
    }

    panel_clear();
}

/*********************************************************************
 * @fn      panel_index
 *
 * @brief   Logical (x, y) -> strip position.
 *
 * @param   x - column, 0 = left.
 *          y - row, 0 = first strip row.
 *
 * @return  strip position, 0xFFFF if out of range.
 */
uint16_t panel_index(uint16_t x, uint16_t y)
{
    uint16_t p;

    if ((x >= PANEL_W) || (y >= PANEL_H)) {
        return 0xFFFF;
    }

    for (p = 0; p < PANEL_PIXELS; p++) {
        if (panel_map[p] == (uint16_t)(y * PANEL_W + x)) {
            return p;
        }
    }

    return 0xFFFF;
}

/*********************************************************************
 * @fn      panel_phys_to_logical
 *
 * @brief   Strip position -> logical framebuffer index.
 *
 * @param   phys - position in the strip, 0 .. PANEL_PIXELS-1.
 *
 * @return  logical index, 0xFFFF if out of range.
 */
uint16_t panel_phys_to_logical(uint16_t phys)
{
    if (phys >= PANEL_PIXELS) {
        return 0xFFFF;
    }

    return panel_map[phys];
}

/*********************************************************************
 * @fn      panel_clear
 *
 * @brief   Clear the framebuffer (does not refresh the strip).
 *
 * @return  none
 */
void panel_clear(void)
{
    uint16_t i;

    for (i = 0; i < sizeof(panel_fb); i++) {
        panel_fb[i] = 0;
    }
}

/*********************************************************************
 * @fn      panel_set_pixel
 *
 * @brief   Write one pixel into the framebuffer.
 *
 * @param   x - column.
 *          y - row.
 *          r - red.
 *          g - green.
 *          b - blue.
 *
 * @return  none
 */
void panel_set_pixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t *p;

    if ((x >= PANEL_W) || (y >= PANEL_H)) {
        return;
    }

    p = &panel_fb[(uint32_t)(y * PANEL_W + x) * 3u];
    p[0] = r;
    p[1] = g;
    p[2] = b;
}

/*********************************************************************
 * @fn      panel_set_pixel565
 *
 * @brief   Write one pixel given as RGB565.
 *
 * @param   x        - column.
 *          y        - row.
 *          color565 - 5-6-5 colour.
 *
 * @return  none
 */
void panel_set_pixel565(uint16_t x, uint16_t y, uint16_t color565)
{
    uint8_t r = (uint8_t)((color565 >> 11) & 0x1F);
    uint8_t g = (uint8_t)((color565 >> 5) & 0x3F);
    uint8_t b = (uint8_t)(color565 & 0x1F);

    r = (uint8_t)((r << 3) | (r >> 2));
    g = (uint8_t)((g << 2) | (g >> 4));
    b = (uint8_t)((b << 3) | (b >> 2));

    panel_set_pixel(x, y, r, g, b);
}

/*********************************************************************
 * @fn      panel_show
 *
 * @brief   Push the framebuffer to the strip in physical order.
 *
 * @return  none
 */
void panel_show(void)
{
    uint16_t        p;
    uint32_t        sum = 0;
    const uint8_t  *dt = ws2812_duty_table();
    const uint8_t  *c;

#if WSP_OSD_ENABLE
    /* stamp the badge into the canvas first, so it is included in both the
     * current estimate and the frame we send */
    osd_draw();
    osd_tick();
#endif

    /* pass 1: how much current would this frame draw? */
    for (p = 0; p < PANEL_PIXELS; p++) {
        c = &panel_fb[(uint32_t)panel_map[p] * 3u];
        sum += dt[c[0]] + dt[c[1]] + dt[c[2]];
    }
    ws2812_limit_frame(sum);

    /* pass 2: send it */
    ws2812_begin();
    for (p = 0; p < PANEL_PIXELS; p++) {
        c = &panel_fb[(uint32_t)panel_map[p] * 3u];
        ws2812_write(c[0], c[1], c[2]);
    }
    ws2812_end();
}
