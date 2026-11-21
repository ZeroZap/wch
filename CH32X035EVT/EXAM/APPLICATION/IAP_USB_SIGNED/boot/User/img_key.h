/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_key.h
 * Description        : Keys used to authenticate (and optionally decrypt) images.
 *
 * Three ways to set them, in increasing order of convenience for products:
 *
 *   1. edit img_key.c           (default; tools/gen_key.py --write does it)
 *   2. -DIMG_KEY_FILE=...       include a generated key file at build time
 *   3. -DIMG_HMAC_KEY=...       inline 32 character ASCII key (and IMG_AES_KEY,
 *                               16 characters)
 *
 * Whatever you use, the host packer must be told the same keys:
 *   python tools/iap_pack.py --key-file keys/acme.c ...
 *   python tools/iap_pack.py --hmac-key '...32 chars...' --aes-key '...16 chars...'
 * (without them it reads boot/User/img_key.c, which is only correct for mode 1)
 *
 * SECURITY NOTE
 *   These are symmetric keys compiled into the bootloader, so anyone who can
 *   read the bootloader flash can also forge images.  They protect against
 *   corrupted downloads, accidental cross-flashing and casual tampering, not
 *   against a determined attacker with physical access.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __IMG_KEY_H
#define __IMG_KEY_H

#include <stdint.h>

#define IMG_HMAC_KEY_LEN    32u
#define IMG_AES_KEY_LEN     16u

extern const uint8_t img_hmac_key[IMG_HMAC_KEY_LEN];    /* HMAC-SHA256 */
extern const uint8_t img_aes_key[IMG_AES_KEY_LEN];      /* AES-128-CTR */

#endif /* __IMG_KEY_H */
