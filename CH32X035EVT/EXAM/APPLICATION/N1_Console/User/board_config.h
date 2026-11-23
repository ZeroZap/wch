/********************************** (C) COPYRIGHT *******************************
 * File Name          : board_config.h
 * Description        : Board / package pinmap for the N1 handheld console.
 *
 * Two packages share one die, so the firmware is selected at compile time:
 *
 *   -DWSP_BOARD=WSP_BOARD_LQFP48    full board (CH32X035C-R0-1V2, default)
 *   -DWSP_BOARD=WSP_BOARD_TSSOP20   compact board (CH32X033F8P6, 20 pins)
 *
 * Full board (LQFP48) pin allocation
 * ---------------------------------------------------------------------------
 *   pin  1..7, 22..24, 38..39, 44..46  spare / expansion
 *   7     PA21     RESET (10k pull-up, 100nF, reset button)
 *   10    PA0      BTN UP, also the IAP "stay in boot" button
 *   11    PA1      BTN DOWN
 *   12    PA2      NOR CS (SPI1, 10k pull-up)
 *   13    PA3      BUZZER + IR TX (TIM2_CH4)
 *   14    PA4      BTN LEFT
 *   15    PA5      NOR SCK
 *   16    PA6      NOR MISO
 *   17    PA7      NOR MOSI
 *   19    PC7      WS2812 DIN (PIOC IO0 via GPIO_Remap_PIOC, SDI kept)
 *   20    PB0      BTN B
 *   21    PB1      IR RX (optional receiver module)
 *   30    PB10     DEBUG TX (USART1, 115200)
 *   31    PB11     DEBUG RX
 *   32    PC16     USB D-
 *   33    PC17     USB D+
 *   34    PC18     SWDIO / SDI
 *   35    PB12     CARD DETECT (cartridge socket switch to GND)
 *   37    PC19     SWCLK / DCK
 *   40    PA8      BTN RIGHT
 *   41    PA9      BTN A
 *   42    PA10     OLED SCL (optional SSD1306)
 *   43    PA11     OLED SDA (optional)
 *   47    GND
 *   48    VDD
 *
 * Battery voltage uses the internal VREFINT channel, so it costs no pin.  The
 * LQFP48 package does not bond out PC0..PC5, and every bonded ADC pin (PA0..
 * PA7, PB0, PB1) is already used by buttons / SPI / the buzzer.
 *
 * Compact board (TSSOP20) keeps WS2812 + cartridge + USB + 6 buttons +
 * buzzer.  PC7/PB10/PA8/PA9/PA10/PA11 are not bonded out, so the OLED, IR
 * receiver and debug UART disappear; WS2812 moves to PC18, which costs SDI.
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
#define WS2812_PIOC_PC7         1       /* PIOC IO0 via GPIO_Remap_PIOC */
#define WS2812_PIOC_PC18        2       /* PIOC IO0, needs SWJ disabled   */
#define WS2812_PIOC_PC19        3       /* PIOC IO1, needs SWJ disabled   */

/*---------------------------------------------------------------------------
 * Common: SPI1 cartridge and USB are wired the same on both packages
 *---------------------------------------------------------------------------*/
#define N1_NOR_PORT             GPIOA
#define N1_NOR_CS_PIN           GPIO_Pin_2   /* PA2 */
#define N1_NOR_SCK_PIN          GPIO_Pin_5   /* PA5 */
#define N1_NOR_MISO_PIN         GPIO_Pin_6   /* PA6 */
#define N1_NOR_MOSI_PIN         GPIO_Pin_7   /* PA7 */

#define N1_USB_DM_PORT          GPIOC
#define N1_USB_DM_PIN           GPIO_Pin_16  /* PC16 */
#define N1_USB_DP_PORT          GPIOC
#define N1_USB_DP_PIN           GPIO_Pin_17  /* PC17 */

/* cartridge detect is on PB12 in both packages */
#define N1_CARD_DETECT_PORT     GPIOB
#define N1_CARD_DETECT_PIN      GPIO_Pin_12

#if (WSP_BOARD == WSP_BOARD_TSSOP20)

#define N1_BOARD_NAME           "CH32X033 TSSOP20"
#define N1_BOARD_HAS_OLED       0
#define N1_BOARD_HAS_IR_RX      0
#define N1_BOARD_HAS_DEBUG_UART 0
#define N1_BOARD_HAS_BAT_SENSE  0

/* PC7 is not bonded: PIOC IO0 is only on PC18, which is the SDI DIO pin. */
#define WSP_WS2812_PIOC_PIN     WS2812_PIOC_PC18

#define N1_BTN_UP_PORT          GPIOA
#define N1_BTN_UP_PIN           GPIO_Pin_0   /* pin 6  */
#define N1_BTN_DOWN_PORT        GPIOA
#define N1_BTN_DOWN_PIN         GPIO_Pin_1   /* pin 7  */
#define N1_BTN_LEFT_PORT        GPIOC
#define N1_BTN_LEFT_PIN         GPIO_Pin_14  /* pin 2  */
#define N1_BTN_RIGHT_PORT       GPIOC
#define N1_BTN_RIGHT_PIN        GPIO_Pin_15  /* pin 3  */
#define N1_BTN_A_PORT           GPIOC
#define N1_BTN_A_PIN            GPIO_Pin_1   /* pin 5  */
#define N1_BTN_B_PORT           GPIOB
#define N1_BTN_B_PIN            GPIO_Pin_1   /* pin 14 */

#define N1_BUZZER_PORT          GPIOA
#define N1_BUZZER_PIN           GPIO_Pin_3   /* PA3 / TIM2_CH4, pin 9 */

#else /* WSP_BOARD_LQFP48 */

#define N1_BOARD_NAME           "CH32X035 LQFP48"
#define N1_BOARD_HAS_OLED       1
#define N1_BOARD_HAS_IR_RX      1
#define N1_BOARD_HAS_DEBUG_UART 1
#define N1_BOARD_HAS_BAT_SENSE  0   /* VREFINT only, no external pin */

/* PC7 through GPIO_Remap_PIOC: SDI debug stays available */
#define WSP_WS2812_PIOC_PIN     WS2812_PIOC_PC7

#define N1_BTN_UP_PORT          GPIOA
#define N1_BTN_UP_PIN           GPIO_Pin_0   /* pin 10 */
#define N1_BTN_DOWN_PORT        GPIOA
#define N1_BTN_DOWN_PIN         GPIO_Pin_1   /* pin 11 */
#define N1_BTN_LEFT_PORT        GPIOA
#define N1_BTN_LEFT_PIN         GPIO_Pin_4   /* pin 14 */
#define N1_BTN_RIGHT_PORT       GPIOA
#define N1_BTN_RIGHT_PIN        GPIO_Pin_8   /* pin 40 */
#define N1_BTN_A_PORT           GPIOA
#define N1_BTN_A_PIN            GPIO_Pin_9   /* pin 41 */
#define N1_BTN_B_PORT           GPIOB
#define N1_BTN_B_PIN            GPIO_Pin_0   /* pin 20 */

#define N1_BUZZER_PORT          GPIOA
#define N1_BUZZER_PIN           GPIO_Pin_3   /* PA3 / TIM2_CH4, pin 13 */
#define N1_IR_RX_PORT           GPIOB
#define N1_IR_RX_PIN            GPIO_Pin_1   /* pin 21 */

#define N1_OLED_PORT            GPIOA
#define N1_OLED_SCL_PIN         GPIO_Pin_10  /* pin 42 */
#define N1_OLED_SDA_PIN         GPIO_Pin_11  /* pin 43 */

#endif

#endif /* __BOARD_CONFIG_H */
