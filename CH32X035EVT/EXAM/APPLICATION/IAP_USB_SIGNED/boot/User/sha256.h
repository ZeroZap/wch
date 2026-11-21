/********************************** (C) COPYRIGHT *******************************
 * File Name          : sha256.h
 * Description        : SHA-256 and HMAC-SHA256 (FIPS 180-4 / RFC 2104).
 *
 * Small streaming implementation: the bootloader has to hash the whole app
 * region out of flash in 256-byte chunks, so update() can be called repeatedly.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __SHA256_H
#define __SHA256_H

#include <stdint.h>
#include <stddef.h>

#define SHA256_LEN          32

typedef struct {
    uint32_t h[8];
    uint64_t total;         /* bytes fed so far                  */
    uint8_t  buf[64];
    uint32_t fill;          /* bytes currently in buf            */
} sha256_ctx;

typedef struct {
    sha256_ctx inner;
    uint8_t    okey[64];    /* key ^ 0x5c, ready for the outer hash */
} hmac_sha256_ctx;

void sha256_init(sha256_ctx *ctx);
void sha256_update(sha256_ctx *ctx, const void *data, size_t len);
void sha256_final(sha256_ctx *ctx, uint8_t out[SHA256_LEN]);
void sha256(const void *data, size_t len, uint8_t out[SHA256_LEN]);

void hmac_sha256_init(hmac_sha256_ctx *ctx, const uint8_t *key, size_t key_len);
void hmac_sha256_update(hmac_sha256_ctx *ctx, const void *data, size_t len);
void hmac_sha256_final(hmac_sha256_ctx *ctx, uint8_t out[SHA256_LEN]);

#endif /* __SHA256_H */
