/********************************** (C) COPYRIGHT *******************************
 * File Name          : boot_ctrl.h
 * Description        : Everything the bootloader does around the image itself:
 *                      tearing down USB before the handover and jumping.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __BOOT_CTRL_H
#define __BOOT_CTRL_H

#include <stdint.h>

/* Release every peripheral and jump to the application entry point.
 * Does not return. */
void boot_jump_to_app(void) __attribute__((noreturn));

/* Stop USB activity and release PC16/PC17 so the app can re-init the PHY.
 * Also called at the start of boot_jump_to_app(). */
void boot_usb_deinit(void);

/* 1 when BOOT_BUTTON_PIN is held low at reset, i.e. the user wants to stay in
 * the bootloader.  Costs ~10 ms on a normal boot, ~60 ms when the pin is low.
 * Always 0 when BOOT_BUTTON_STAY is 0. */
uint8_t boot_button_held(void);

#endif /* __BOOT_CTRL_H */
