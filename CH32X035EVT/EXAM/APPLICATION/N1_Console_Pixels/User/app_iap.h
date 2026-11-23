/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_iap.h
 * Description        : Ask the bootloader to replace this application.
 *
 * The request is persisted in the flag page (shared/img_hdr.h), because a plain
 * reset would not survive the bootloader's bss clear.  State UPGRADE makes the
 * bootloader stay in its USB update mode instead of jumping back here.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __APP_IAP_H
#define __APP_IAP_H

#include "img_hdr.h"

/* Write the UPGRADE flag and reset.  Does not return. */
void app_iap_request(void) __attribute__((noreturn));

/* Read back the flag / header pages (read-only helpers for the app protocol).
 * Return 1 when the magic and the basic fields look sane. */
uint8_t app_iap_get_flag(img_flag_t *out);
uint8_t app_iap_get_hdr(img_hdr_t *out);

#endif /* __APP_IAP_H */
