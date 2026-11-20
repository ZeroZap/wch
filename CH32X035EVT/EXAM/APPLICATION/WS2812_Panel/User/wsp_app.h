/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_app.h
 * Description        : Display mode (STRIP / PANEL) and canvas access.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_APP_H
#define __WSP_APP_H

#include "ch32x035.h"
#include "wsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void     app_set_mode(uint8_t mode);
uint8_t  app_mode(void);

/* number of logical pixels of the current mode */
uint16_t app_pixels(void);

/* canvas helpers, index is y*PANEL_W+x (PANEL) or strip position (STRIP) */
void     app_clear(void);
void     app_put(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
uint8_t *app_fb(void);

/* push the canvas to the LEDs */
void     app_show(void);

#ifdef __cplusplus
}
#endif

#endif /* __WSP_APP_H */
