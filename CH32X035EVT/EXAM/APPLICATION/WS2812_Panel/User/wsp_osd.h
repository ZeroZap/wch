/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_osd.h
 * Description        : Tiny on-screen badge: frame rate + "current limited" dot.
 *
 * panel_show() calls osd_draw() (stamp into the framebuffer) and osd_tick()
 * (fps accounting) before the frame is converted, so the badge also goes
 * through the gamma and the current limiter.
 *
 * On a 16x16 panel the two fps digits take 11 of the 16 columns of row 0, so it
 * is meant for bring-up / USB debugging.  WSP_OSD_ENABLE compiles it in,
 * WSP_OSD_DEFAULT picks the power-on state, and the USB OSD command flips it
 * at runtime.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_OSD_H
#define __WSP_OSD_H

#include "ch32x035.h"

void     osd_init(void);

/* call once per displayed frame, after osd_draw() */
void     osd_tick(void);

/* stamp the badge into panel_fb when it is enabled */
void     osd_draw(void);

void     osd_set(uint8_t on);
uint8_t  osd_on(void);

uint16_t osd_fps(void);

#endif /* __WSP_OSD_H */
