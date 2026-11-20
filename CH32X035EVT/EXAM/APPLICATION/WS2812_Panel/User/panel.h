/********************************** (C) COPYRIGHT *******************************
 * File Name          : panel.h
 * Description        : Serpentine RGB panel framebuffer.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __PANEL_H
#define __PANEL_H

#include "ch32x035.h"
#include "wsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* logical framebuffer, CANVAS_PIXELS * 3 bytes, RGB888.
 * PANEL mode: index = y * PANEL_W + x
 * STRIP mode: index = position along the strip */
extern uint8_t panel_fb[CANVAS_PIXELS * 3];

void panel_init(void);
void panel_clear(void);

/*********************************************************************
 * x = 0 is the left column, y = 0 is the first row of the strip.
 * Out of range coordinates are ignored.
 *********************************************************************/
void panel_set_pixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
void panel_set_pixel565(uint16_t x, uint16_t y, uint16_t color565);

/* logical pixel -> position inside the strip */
uint16_t panel_index(uint16_t x, uint16_t y);

/* position inside the strip -> logical pixel index (0xFFFF if out of range) */
uint16_t panel_phys_to_logical(uint16_t phys);

/* push the whole framebuffer to the strip */
void panel_show(void);

#ifdef __cplusplus
}
#endif

#endif /* __PANEL_H */
