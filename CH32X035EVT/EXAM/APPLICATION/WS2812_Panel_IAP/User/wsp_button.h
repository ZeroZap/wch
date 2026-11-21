/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_button.h
 * Description        : Debounced user button (PA0, active low).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_BUTTON_H
#define __WSP_BUTTON_H

#include "ch32x035.h"
#include "wsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void    button_init(void);
void    button_poll(void);

/* consume one event, returns 1 when it happened */
uint8_t button_short(void);
uint8_t button_long(void);

/* held past WSP_BTN_UPGRADE_MS: ask the bootloader to take over */
uint8_t button_upgrade(void);

#ifdef __cplusplus
}
#endif

#endif /* __WSP_BUTTON_H */
