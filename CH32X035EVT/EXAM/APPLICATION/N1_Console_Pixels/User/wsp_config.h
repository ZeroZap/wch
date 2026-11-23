/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_config.h
 * Description        : Shim for the copied WS2812 driver, which includes
 *                      "wsp_config.h".  All driver knobs live in game_config.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_CONFIG_H
#define __WSP_CONFIG_H

#include "game_config.h"

/* The copied WS2812 driver selects its PIOC output pin from WS2812_PIOC_PIN.
 * Derive it from the board map: PC7 on LQFP48 (SWD/SDI stays usable),
 * PC18 on TSSOP20 (SWJ disabled).  Without this the driver falls through to
 * the PC18 branch, which disables SDI and drives the wrong pin. */
#ifndef WS2812_PIOC_PIN
#define WS2812_PIOC_PIN         WSP_WS2812_PIOC_PIN
#endif

#endif /* __WSP_CONFIG_H */
