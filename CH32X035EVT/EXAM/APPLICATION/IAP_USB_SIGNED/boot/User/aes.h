/********************************** (C) COPYRIGHT *******************************
 * File Name          : aes.h
 * Description        : AES-128 CTR mode (FIPS 197 / SP 800-38A), encryption
 *                      only -- CTR uses the same operation for both directions.
 *
 * The bootloader decrypts the image while programming it: the host sends
 * ciphertext, the boot XORs it with the keystream and writes plaintext to
 * flash.  Only 16-byte aligned blocks are handled, which is all the image
 * format needs (pages are 256 bytes).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __AES_H
#define __AES_H

#include <stdint.h>

#define AES128_KEY_LEN      16
#define AES128_BLOCK_LEN    16

/* Expand a 128-bit key into the 11 round keys. */
void aes128_init(const uint8_t key[AES128_KEY_LEN], uint8_t rk[176]);

/* Encrypt one block with pre-expanded keys. */
void aes128_encrypt(const uint8_t rk[176], const uint8_t in[AES128_BLOCK_LEN],
                    uint8_t out[AES128_BLOCK_LEN]);

/*
 * CTR keystream XOR.
 *   iv      : 16 byte initial counter block
 *   byte_off: byte offset inside the stream (must be a multiple of 16)
 *   buf     : data to encrypt/decrypt in place, len must be a multiple of 16
 */
void aes128_ctr_xor(const uint8_t rk[176], const uint8_t iv[AES128_BLOCK_LEN],
                    uint32_t byte_off, uint8_t *buf, uint32_t len);

#endif /* __AES_H */
