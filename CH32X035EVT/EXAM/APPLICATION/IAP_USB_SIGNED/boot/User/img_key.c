/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_key.c
 * Description        : Image authentication / encryption keys.
 *
 * Default: the built-in development keys below.  Everything is overridable at
 * build time, see img_key.h:
 *
 *   sh tools/build.sh boot -DIMG_HMAC_KEY='"0123456789abcdef0123456789abcdef"' \
 *                          -DIMG_AES_KEY='"0123456789abcdef"'
 *
 *   sh tools/build.sh boot -DIMG_KEY_FILE='"../../keys/acme.c"'
 *
 * The IMG_KEY_FILE path is resolved like any quoted #include, i.e. relative to
 * this file first, so a relative path is relative to boot/User/.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "img_key.h"

#ifdef IMG_KEY_FILE

/* The included file must define exactly these two arrays. */
#include IMG_KEY_FILE

#else

/* DEVELOPMENT KEYS -- anyone can read them out of the bootloader.  Replace
 * them for production (`python tools/gen_key.py --write`). */
#ifndef IMG_HMAC_KEY
#define IMG_HMAC_KEY    "WCH-IAP-DEV-HMAC-KEY-REPLACE-ME!"
#endif

#ifndef IMG_AES_KEY
#define IMG_AES_KEY     "WCH-IAP-AES-DEV!"
#endif

/* A string literal of exactly N characters initialises an N byte array and
 * drops the terminating NUL; these two typedefs turn a wrong length into a
 * build error instead of a silent truncation. */
typedef char img_hmac_key_len_check[
    (sizeof(IMG_HMAC_KEY) == (IMG_HMAC_KEY_LEN + 1u)) ? 1 : -1];
typedef char img_aes_key_len_check[
    (sizeof(IMG_AES_KEY) == (IMG_AES_KEY_LEN + 1u)) ? 1 : -1];

const uint8_t img_hmac_key[IMG_HMAC_KEY_LEN] = IMG_HMAC_KEY;
const uint8_t img_aes_key[IMG_AES_KEY_LEN] = IMG_AES_KEY;

#endif /* IMG_KEY_FILE */
