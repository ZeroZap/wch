/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_tick.c
 * Description        : 1ms system tick on TIM3.
 *
 * TIM3 runs at 1MHz with ARR = 999 and raises an update interrupt every
 * millisecond.  The SDK's Delay_Ms()/Delay_Us() use SysTick, so both time
 * bases can be used at the same time.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "wsp_tick.h"

static volatile uint32_t tick_counter_ms;

void TIM3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      TIM3_IRQHandler
 *
 * @brief   1ms tick.
 *
 * @return  none
 */
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET) {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        tick_counter_ms++;
    }
}

/*********************************************************************
 * @fn      tick_init
 *
 * @brief   Start the 1ms tick.
 *
 * @return  none
 */
void tick_init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};
    NVIC_InitTypeDef        NVIC_InitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = (uint16_t)(SystemCoreClock / 1000000u - 1u);
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);

    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM3, ENABLE);
}

/*********************************************************************
 * @fn      tick_ms
 *
 * @brief   Milliseconds since tick_init() (wraps after ~49 days).
 *
 * @return  none
 */
uint32_t tick_ms(void)
{
    return tick_counter_ms;
}

/*********************************************************************
 * @fn      tick_delay
 *
 * @brief   Wait while the tick keeps running.
 *
 * @param   ms - delay in milliseconds.
 *
 * @return  none
 */
void tick_delay(uint32_t ms)
{
    uint32_t start = tick_counter_ms;

    while ((tick_counter_ms - start) < ms) {
    }
}
