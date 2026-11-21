/********************************** (C) COPYRIGHT *******************************
 * File Name          : board_config.h
 * Description        : Package / board pinmap selection for WS2812_Panel_IAP.
 *
 * One firmware source, two packages.  Select with
 *
 *     -DWSP_BOARD=WSP_BOARD_TSSOP20     (or the numeric 2, see tools/build.sh)
 *
 *   1  WSP_BOARD_LQFP48    CH32X035 LQFP48 dev board (CH32X035C-R0-1V2)
 *   2  WSP_BOARD_TSSOP20   CH32X033F8P6 / CH32X035 TSSOP20 (20 pins)
 *
 * Pin numbers below come from PinMux/CH32X035_pins.csv, column TSSOP20 /
 * LQFP48.  Both packages share the die, so only the bonded-out pins differ.
 *
 * TSSOP20 (CH32X033F8P6) has exactly these GPIOs:
 *     pin  1 PB12     pin  2 PC14     pin  3 PC15     pin  5 PC1
 *     pin  6 PA0      pin  7 PA1      pin  8 PA2      pin  9 PA3
 *     pin 10 PA4      pin 11 PA5      pin 12 PA6      pin 13 PA7
 *     pin 14 PB1      pin 17 PC16     pin 18 PC17
 *     pin 19 PC18     pin 20 PC19
 *
 * PC7 and PB10 are LQFP-only, which drives the two differences below.
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

/* PIOC 1-wire output options, shared by both boards */
#define WS2812_PIOC_PC7         1       /* PIOC IO0 via GPIO_Remap_PIOC   */
#define WS2812_PIOC_PC18        2       /* PIOC IO0, needs SWJ disabled   */
#define WS2812_PIOC_PC19        3       /* PIOC IO1, needs SWJ disabled   */

#if (WSP_BOARD == WSP_BOARD_TSSOP20)

#define WSP_BOARD_NAME          "CH32X033 TSSOP20"

/* PC7 is not bonded, so the PIOC output has to sit on PC18 (DIO).  That pin is
 * part of the SDI debug interface, so ws2812.c calls GPIO_Remap_SWJ_Disable
 * and WCH-Link can no longer attach once the app runs: flash with the
 * factory image and debug on an LQFP48 board (or with --standalone). */
#define WSP_WS2812_PIOC_PIN     WS2812_PIOC_PC18

/* PB10 is not bonded, and every other UART TX pin is taken (PA2 nor-cs,
 * PA5/PA7 SPI, PC16/PC17 USB, PC18 ws2812).  Keep USART1 initialised so the
 * printf() path stays harmless, but there is nothing on the wire. */
#define WSP_DEBUG_UART          0

#define WSP_BTN_PORT            GPIOA
#define WSP_BTN_PIN             GPIO_Pin_0   /* PA0, pin 6  */
#define WSP_NOR_PORT            GPIOA
#define WSP_NOR_CS_PIN          GPIO_Pin_2   /* PA2, pin 8  */
#define WSP_NOR_SCK_PIN         GPIO_Pin_5   /* PA5, pin 11 */
#define WSP_NOR_MISO_PIN        GPIO_Pin_6   /* PA6, pin 12 */
#define WSP_NOR_MOSI_PIN        GPIO_Pin_7   /* PA7, pin 13 */

#else /* WSP_BOARD_LQFP48 */

#define WSP_BOARD_NAME          "CH32X035 LQFP48"

/* PC7 via GPIO_Remap_PIOC: the SDI pins stay free, debugging keeps working */
#define WSP_WS2812_PIOC_PIN     WS2812_PIOC_PC7

#define WSP_DEBUG_UART          1           /* USART1 TX on PB10, pin 30 */

#define WSP_BTN_PORT            GPIOA
#define WSP_BTN_PIN             GPIO_Pin_0   /* PA0,  pin 10 */
#define WSP_NOR_PORT            GPIOA
#define WSP_NOR_CS_PIN          GPIO_Pin_2   /* PA2,  pin 12 */
#define WSP_NOR_SCK_PIN         GPIO_Pin_5   /* PA5,  pin 15 */
#define WSP_NOR_MISO_PIN        GPIO_Pin_6   /* PA6,  pin 16 */
#define WSP_NOR_MOSI_PIN        GPIO_Pin_7   /* PA7,  pin 17 */

#endif

#endif /* __BOARD_CONFIG_H */
