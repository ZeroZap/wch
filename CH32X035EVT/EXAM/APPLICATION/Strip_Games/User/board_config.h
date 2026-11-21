/********************************** (C) COPYRIGHT *******************************
 * File Name          : board_config.h
 * Description        : Package / board pinmap for Strip_Games.
 *
 *   -DWSP_BOARD=WSP_BOARD_LQFP48    CH32X035 LQFP48 dev board (default)
 *   -DWSP_BOARD=WSP_BOARD_TSSOP20   CH32X033F8P6 / TSSOP20
 *
 * Pin numbers from PinMux/CH32X035_pins.csv.  Both packages share the die, so
 * only the bonded-out pins differ; PC7 and PA8..PA10 are LQFP-only.
 *
 * Game wiring (same on both packages except the WS2812 output):
 *
 *   WS2812 DIN  LQFP48: PC7 (PIOC IO0 via GPIO_Remap_PIOC)
 *               TSSOP20: PC18 (PIOC IO0, SDI debug pins are lost)
 *   P1 button   PA0   pull-up, press to GND
 *   P2 button   PA1   pull-up, press to GND
 *   Buzzer      PA2   TIM2_CH3 (passive buzzer, 50% square wave)
 *   USB         PC16 (D-), PC17 (D+)
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include "ch32x035.h"

#define WSP_BOARD_LQFP48        1
#define WSP_BOARD_TSSOP20       2

#ifndef WSP_BOARD
#define WSP_BOARD               WSP_BOARD_LQFP48
#endif

/* PIOC 1-wire output options */
#define WS2812_PIOC_PC7         1       /* PIOC IO0 via GPIO_Remap_PIOC   */
#define WS2812_PIOC_PC18        2       /* PIOC IO0, needs SWJ disabled   */
#define WS2812_PIOC_PC19        3       /* PIOC IO1, needs SWJ disabled   */

#if (WSP_BOARD == WSP_BOARD_TSSOP20)

#define WSP_BOARD_NAME          "CH32X033 TSSOP20"

/* PC7 is not bonded: the PIOC output moves to PC18 (DIO), which disables the
 * SDI debug pins once the app runs.  Update over USB or with a factory image. */
#define WSP_WS2812_PIOC_PIN     WS2812_PIOC_PC18

/* PB10 is not bonded and every other UART TX is taken, so no log output. */
#define WSP_DEBUG_UART          0

#else /* WSP_BOARD_LQFP48 */

#define WSP_BOARD_NAME          "CH32X035 LQFP48"

/* PC7 via GPIO_Remap_PIOC: the SDI pins stay free, debugging keeps working */
#define WSP_WS2812_PIOC_PIN     WS2812_PIOC_PC7

#define WSP_DEBUG_UART          1           /* USART1 TX on PB10, pin 30 */

#endif

/* Identical on both packages */
#define WSP_P1_PORT             GPIOA
#define WSP_P1_PIN              GPIO_Pin_0   /* LQFP48 pin 10 / TSSOP20 pin 6 */
#define WSP_P2_PORT             GPIOA
#define WSP_P2_PIN              GPIO_Pin_1   /* LQFP48 pin 11 / TSSOP20 pin 7 */
#define WSP_BUZZER_PORT         GPIOA
#define WSP_BUZZER_PIN          GPIO_Pin_2   /* LQFP48 pin 12 / TSSOP20 pin 8 */

#endif /* __BOARD_CONFIG_H */
