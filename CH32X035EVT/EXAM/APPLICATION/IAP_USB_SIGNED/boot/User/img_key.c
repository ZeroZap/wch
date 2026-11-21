/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_key.c
 * Description        : Development keys.  DO NOT SHIP THESE.
 *
 * Replace with the output of `python tools/gen_key.py --write` (or provide your
 * own file and build with -DIMG_KEY_FILE=path/to/img_key_private.c).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "img_key.h"

const uint8_t img_hmac_key[32] = {
    'W', 'C', 'H', '-', 'I', 'A', 'P', '-',
    'D', 'E', 'V', '-', 'H', 'M', 'A', 'C',
    '-', 'K', 'E', 'Y', '-', 'R', 'E', 'P',
    'L', 'A', 'C', 'E', '-', 'M', 'E', '!'
};

const uint8_t img_aes_key[16] = {
    'W', 'C', 'H', '-', 'I', 'A', 'P', '-',
    'A', 'E', 'S', '-', 'D', 'E', 'V', '!'
};
