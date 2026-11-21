/********************************** (C) COPYRIGHT *******************************
 * File Name          : gfx.h
 * Description        : Small drawing helpers on top of the panel framebuffer.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __GFX_H
#define __GFX_H

#include "ch32x035.h"
#include "panel.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RGB565(r, g, b)     ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
#define RGB332(r, g, b)     ((uint8_t)(((r) & 0xE0) | (((g) & 0xE0) >> 3) | ((b) >> 6)))

void gfx_clear(void);
void gfx_fill(uint8_t r, uint8_t g, uint8_t b);
void gfx_fill565(uint16_t color);
void gfx_pixel565(uint16_t x, uint16_t y, uint16_t color);
void gfx_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
void gfx_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
void gfx_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void gfx_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void gfx_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/* copy a w*h RGB565 / RGB332 bitmap at (x, y), clipped to the panel */
void gfx_bitmap565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void gfx_bitmap332(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *data);

/* nearest neighbour scale a w*h RGB565 bitmap to the whole panel */
void gfx_blit565_fit(const uint16_t *data, uint16_t w, uint16_t h);

#ifdef __cplusplus
}
#endif

#endif /* __GFX_H */
