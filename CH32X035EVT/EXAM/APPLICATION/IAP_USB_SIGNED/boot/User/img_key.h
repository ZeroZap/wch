/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_key.h
 * Description        : Keys used to authenticate (and optionally decrypt) images.
 *
 * SECURITY NOTE
 *   These are symmetric keys compiled into the bootloader, so anyone who can
 *   read the bootloader flash can also forge images.  They protect against
 *   corrupted downloads, accidental cross-flashing and casual tampering, not
 *   against a determined attacker with physical access.  Replace the values in
 *   img_key.c (or point IMG_KEY_FILE at a private file) before shipping.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __IMG_KEY_H
#define __IMG_KEY_H

#include <stdint.h>

extern const uint8_t img_hmac_key[32];      /* HMAC-SHA256 key */
extern const uint8_t img_aes_key[16];       /* AES-128-CTR key */

#endif /* __IMG_KEY_H */
