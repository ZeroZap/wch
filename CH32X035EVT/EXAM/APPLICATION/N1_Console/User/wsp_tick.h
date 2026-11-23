/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_tick.h
 * Description        : 1ms system tick on TIM3 (used by effects and the button).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_TICK_H
#define __WSP_TICK_H

#include "ch32x035.h"

#ifdef __cplusplus
extern "C" {
#endif

void     tick_init(void);
uint32_t tick_ms(void);

/* busy wait, keeps the tick running (Delay_Ms uses SysTick, this one is TIM3) */
void     tick_delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __WSP_TICK_H */
