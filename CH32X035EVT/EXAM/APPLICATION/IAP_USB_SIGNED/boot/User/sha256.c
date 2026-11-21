/********************************** (C) COPYRIGHT *******************************
 * File Name          : sha256.c
 * Description        : SHA-256 and HMAC-SHA256, see sha256.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>
#include "sha256.h"

static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

#define ROTR(x, n)      (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_block(sha256_ctx *ctx, const uint8_t *p)
{
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2;
    uint32_t i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)p[i * 4] << 24) | ((uint32_t)p[i * 4 + 1] << 16) |
               ((uint32_t)p[i * 4 + 2] << 8) | (uint32_t)p[i * 4 + 3];
    }
    for (i = 16; i < 64; i++) {
        uint32_t s0 = ROTR(w[i - 15], 7) ^ ROTR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ROTR(w[i - 2], 17) ^ ROTR(w[i - 2], 19) ^ (w[i - 2] >> 10);

        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = ctx->h[0]; b = ctx->h[1]; c = ctx->h[2]; d = ctx->h[3];
    e = ctx->h[4]; f = ctx->h[5]; g = ctx->h[6]; h = ctx->h[7];

    for (i = 0; i < 64; i++) {
        uint32_t S1 = ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t S0 = ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);

        t1 = h + S1 + ch + K[i] + w[i];
        t2 = S0 + maj;

        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->h[0] += a; ctx->h[1] += b; ctx->h[2] += c; ctx->h[3] += d;
    ctx->h[4] += e; ctx->h[5] += f; ctx->h[6] += g; ctx->h[7] += h;
}

void sha256_init(sha256_ctx *ctx)
{
    ctx->h[0] = 0x6a09e667u; ctx->h[1] = 0xbb67ae85u;
    ctx->h[2] = 0x3c6ef372u; ctx->h[3] = 0xa54ff53au;
    ctx->h[4] = 0x510e527fu; ctx->h[5] = 0x9b05688cu;
    ctx->h[6] = 0x1f83d9abu; ctx->h[7] = 0x5be0cd19u;
    ctx->total = 0;
    ctx->fill = 0;
}

void sha256_update(sha256_ctx *ctx, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;

    ctx->total += len;

    if (ctx->fill != 0) {
        size_t need = 64u - ctx->fill;

        if (len < need) {
            memcpy(&ctx->buf[ctx->fill], p, len);
            ctx->fill += (uint32_t)len;
            return;
        }
        memcpy(&ctx->buf[ctx->fill], p, need);
        sha256_block(ctx, ctx->buf);
        p += need;
        len -= need;
        ctx->fill = 0;
    }

    while (len >= 64u) {
        sha256_block(ctx, p);
        p += 64;
        len -= 64u;
    }

    if (len != 0) {
        memcpy(ctx->buf, p, len);
        ctx->fill = (uint32_t)len;
    }
}

void sha256_final(sha256_ctx *ctx, uint8_t out[SHA256_LEN])
{
    uint64_t bits = ctx->total * 8u;
    uint8_t  pad[72];
    size_t   padlen;
    uint32_t i;

    padlen = (ctx->fill < 56u) ? (56u - ctx->fill) : (120u - ctx->fill);
    memset(pad, 0, sizeof(pad));
    pad[0] = 0x80u;

    sha256_update(ctx, pad, padlen);

    for (i = 0; i < 8; i++) {
        pad[i] = (uint8_t)(bits >> (56 - i * 8));
    }
    sha256_update(ctx, pad, 8);

    for (i = 0; i < 8; i++) {
        out[i * 4 + 0] = (uint8_t)(ctx->h[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(ctx->h[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(ctx->h[i] >> 8);
        out[i * 4 + 3] = (uint8_t)(ctx->h[i]);
    }
}

void sha256(const void *data, size_t len, uint8_t out[SHA256_LEN])
{
    sha256_ctx ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out);
}

void hmac_sha256_init(hmac_sha256_ctx *ctx, const uint8_t *key, size_t key_len)
{
    uint8_t pad[64];
    uint8_t tk[SHA256_LEN];
    size_t  i;

    if (key_len > 64u) {
        sha256(key, key_len, tk);
        key = tk;
        key_len = SHA256_LEN;
    }

    /* inner: H((key ^ 0x36) || data) */
    memset(pad, 0x36, sizeof(pad));
    for (i = 0; i < key_len; i++) {
        pad[i] ^= key[i];
    }
    sha256_init(&ctx->inner);
    sha256_update(&ctx->inner, pad, sizeof(pad));

    /* remember the outer key pad */
    memset(ctx->okey, 0x5c, sizeof(ctx->okey));
    for (i = 0; i < key_len; i++) {
        ctx->okey[i] ^= key[i];
    }

    /* wipe the temporaries that hold key material */
    memset(pad, 0, sizeof(pad));
    memset(tk, 0, sizeof(tk));
}

void hmac_sha256_update(hmac_sha256_ctx *ctx, const void *data, size_t len)
{
    sha256_update(&ctx->inner, data, len);
}

void hmac_sha256_final(hmac_sha256_ctx *ctx, uint8_t out[SHA256_LEN])
{
    uint8_t inner[SHA256_LEN];
    sha256_ctx outer;

    sha256_final(&ctx->inner, inner);

    sha256_init(&outer);
    sha256_update(&outer, ctx->okey, sizeof(ctx->okey));
    sha256_update(&outer, inner, sizeof(inner));
    sha256_final(&outer, out);

    memset(inner, 0, sizeof(inner));
    memset(ctx->okey, 0, sizeof(ctx->okey));
}
