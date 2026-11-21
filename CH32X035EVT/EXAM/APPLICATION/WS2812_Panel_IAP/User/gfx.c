/********************************** (C) COPYRIGHT *******************************
 * File Name          : gfx.c
 * Description        : Small drawing helpers on top of the panel framebuffer.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "gfx.h"

/*********************************************************************
 * @fn      gfx_clear
 *
 * @brief   Clear the whole framebuffer to black.
 *
 * @return  none
 */
void gfx_clear(void)
{
    panel_clear();
}

/*********************************************************************
 * @fn      gfx_fill
 *
 * @brief   Fill the panel with one RGB888 colour.
 *
 * @return  none
 */
void gfx_fill(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t x, y;

    for (y = 0; y < PANEL_H; y++) {
        for (x = 0; x < PANEL_W; x++) {
            panel_set_pixel(x, y, r, g, b);
        }
    }
}

/*********************************************************************
 * @fn      gfx_fill565
 *
 * @brief   Fill the panel with one RGB565 colour.
 *
 * @return  none
 */
void gfx_fill565(uint16_t color)
{
    uint16_t x, y;

    for (y = 0; y < PANEL_H; y++) {
        for (x = 0; x < PANEL_W; x++) {
            panel_set_pixel565(x, y, color);
        }
    }
}

/*********************************************************************
 * @fn      gfx_pixel565
 *
 * @brief   Draw one pixel.
 *
 * @return  none
 */
void gfx_pixel565(uint16_t x, uint16_t y, uint16_t color)
{
    panel_set_pixel565(x, y, color);
}

/*********************************************************************
 * @fn      gfx_hline
 *
 * @brief   Draw a horizontal line.
 *
 * @return  none
 */
void gfx_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{
    uint16_t i;

    for (i = 0; i < w; i++) {
        panel_set_pixel565((uint16_t)(x + i), y, color);
    }
}

/*********************************************************************
 * @fn      gfx_vline
 *
 * @brief   Draw a vertical line.
 *
 * @return  none
 */
void gfx_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
    uint16_t i;

    for (i = 0; i < h; i++) {
        panel_set_pixel565(x, (uint16_t)(y + i), color);
    }
}

/*********************************************************************
 * @fn      gfx_rect
 *
 * @brief   Draw a rectangle outline.
 *
 * @return  none
 */
void gfx_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if ((w == 0) || (h == 0)) {
        return;
    }

    gfx_hline(x, y, w, color);
    gfx_hline(x, (uint16_t)(y + h - 1u), w, color);
    gfx_vline(x, y, h, color);
    gfx_vline((uint16_t)(x + w - 1u), y, h, color);
}

/*********************************************************************
 * @fn      gfx_fill_rect
 *
 * @brief   Draw a filled rectangle.
 *
 * @return  none
 */
void gfx_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t i;

    for (i = 0; i < h; i++) {
        gfx_hline(x, (uint16_t)(y + i), w, color);
    }
}

/*********************************************************************
 * @fn      gfx_line
 *
 * @brief   Bresenham line.
 *
 * @return  none
 */
void gfx_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    int16_t dx = (int16_t)((x1 > x0) ? (x1 - x0) : (x0 - x1));
    int16_t dy = (int16_t)((y1 > y0) ? (y1 - y0) : (y0 - y1));
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = (int16_t)(dx - dy);
    int16_t e2;
    int16_t x = (int16_t)x0;
    int16_t y = (int16_t)y0;

    while (1) {
        if ((x >= 0) && (y >= 0)) {
            panel_set_pixel565((uint16_t)x, (uint16_t)y, color);
        }
        if ((x == (int16_t)x1) && (y == (int16_t)y1)) {
            break;
        }
        e2 = (int16_t)(err << 1);
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

/*********************************************************************
 * @fn      gfx_bitmap565
 *
 * @brief   Blit a w*h RGB565 bitmap at (x, y).
 *
 * @return  none
 */
void gfx_bitmap565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    uint16_t i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            panel_set_pixel565((uint16_t)(x + i), (uint16_t)(y + j), data[(uint32_t)j * w + i]);
        }
    }
}

/*********************************************************************
 * @fn      gfx_bitmap332
 *
 * @brief   Blit a w*h RGB332 bitmap at (x, y).
 *
 * @return  none
 */
void gfx_bitmap332(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data)
{
    uint16_t i, j;
    uint8_t v;
    uint16_t r, g, b;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            v = data[(uint32_t)j * w + i];
            r = (uint16_t)((v >> 5) & 0x07);
            g = (uint16_t)((v >> 2) & 0x07);
            b = (uint16_t)(v & 0x03);
            r = (uint16_t)((r << 5) | (r << 2) | (r >> 1));
            g = (uint16_t)((g << 5) | (g << 2) | (g >> 1));
            b = (uint16_t)((b << 6) | (b << 4) | (b << 2) | b);
            panel_set_pixel565((uint16_t)(x + i), (uint16_t)(y + j), RGB565(r, g, b));
        }
    }
}

/*********************************************************************
 * @fn      gfx_blit565_fit
 *
 * @brief   Scale a w*h RGB565 bitmap to the panel (nearest neighbour).
 *
 * @return  none
 */
void gfx_blit565_fit(const uint16_t *data, uint16_t w, uint16_t h)
{
    uint16_t x, y, sx, sy;

    if ((w == 0) || (h == 0)) {
        return;
    }

    for (y = 0; y < PANEL_H; y++) {
        sy = (uint16_t)(((uint32_t)y * h) / PANEL_H);
        for (x = 0; x < PANEL_W; x++) {
            sx = (uint16_t)(((uint32_t)x * w) / PANEL_W);
            panel_set_pixel565(x, y, data[(uint32_t)sy * w + sx]);
        }
    }
}
