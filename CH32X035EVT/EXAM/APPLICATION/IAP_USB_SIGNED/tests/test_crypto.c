/* Host side known-answer tests for the bootloader crypto.
 *
 *   gcc -std=c99 -Wall -Wextra -O1 -I ../boot/User test_crypto.c \
 *       ../boot/User/sha256.c ../boot/User/aes.c -o test_crypto && ./test_crypto
 *
 * Vectors: FIPS 180-4 (SHA-256), RFC 4231 (HMAC-SHA256),
 *          NIST SP 800-38A F.5.1 (AES-128-CTR).
 */
#include <stdio.h>
#include <string.h>
#include "sha256.h"
#include "aes.h"

static int failures;

static void hex(const uint8_t *p, size_t n, char *out)
{
    static const char *d = "0123456789abcdef";
    size_t i;

    for (i = 0; i < n; i++) {
        out[i * 2] = d[p[i] >> 4];
        out[i * 2 + 1] = d[p[i] & 0x0F];
    }
    out[n * 2] = '\0';
}

static void check_hex(const char *what, const uint8_t *got, const char *want, size_t n)
{
    char h[2 * 64 + 1];

    hex(got, n, h);
    if (strcmp(h, want) != 0) {
        printf("FAIL %-28s got %s\n%*s want %s\n", what, h, 30, "", want);
        failures++;
    } else {
        printf("ok   %s\n", what);
    }
}

static void test_sha256(void)
{
    uint8_t out[32];

    sha256("abc", 3, out);
    check_hex("sha256(abc)", out,
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", 32);

    sha256("", 0, out);
    check_hex("sha256(\"\")", out,
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", 32);

    {
        /* 448 bit message from FIPS 180-4 */
        const char *m = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        sha256(m, strlen(m), out);
        check_hex("sha256(abcdbcde...)", out,
                  "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", 32);
    }

    {
        /* streaming in 1 byte chunks must give the same digest */
        sha256_ctx ctx;
        const char *m = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        size_t i;

        sha256_init(&ctx);
        for (i = 0; i < strlen(m); i++) {
            sha256_update(&ctx, &m[i], 1);
        }
        sha256_final(&ctx, out);
        check_hex("sha256(streamed)", out,
                  "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", 32);
    }
}

static void test_hmac(void)
{
    uint8_t out[32];
    uint8_t key[20];
    const char *m = "Hi There";

    memset(key, 0x0b, sizeof(key));

    {
        hmac_sha256_ctx ctx;

        hmac_sha256_init(&ctx, key, sizeof(key));
        hmac_sha256_update(&ctx, m, strlen(m));
        hmac_sha256_final(&ctx, out);
    }
    check_hex("hmac rfc4231 #1", out,
              "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7", 32);

    {
        hmac_sha256_ctx ctx;
        const char *m2 = "what do ya want for nothing?";
        const char *k2 = "Jefe";

        hmac_sha256_init(&ctx, (const uint8_t *)k2, strlen(k2));
        hmac_sha256_update(&ctx, m2, strlen(m2));
        hmac_sha256_final(&ctx, out);
    }
    check_hex("hmac rfc4231 #2", out,
              "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843", 32);

    {
        /* 131 byte key must be hashed first (rfc4231 #6, key = 131 * 0xaa) */
        hmac_sha256_ctx ctx;
        uint8_t k[131];
        const char *m3 = "Test Using Larger Than Block-Size Key - Hash Key First";

        memset(k, 0xaa, sizeof(k));
        hmac_sha256_init(&ctx, k, sizeof(k));
        hmac_sha256_update(&ctx, m3, strlen(m3));
        hmac_sha256_final(&ctx, out);
    }
    check_hex("hmac rfc4231 #6", out,
              "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54", 32);
}

static void test_aes_ctr(void)
{
    /* NIST SP 800-38A F.5.1: AES-128-CTR, 4 blocks */
    static const uint8_t key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    static const uint8_t iv[16] = {
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };
    static const uint8_t pt[64] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
        0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
        0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
        0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
    };
    uint8_t buf[64];
    uint8_t rk[176];

    aes128_init(key, rk);
    memcpy(buf, pt, sizeof(buf));
    aes128_ctr_xor(rk, iv, 0, buf, sizeof(buf));
    check_hex("aes128-ctr 4 blocks", buf,
              "874d6191b620e3261bef6864990db6ce"
              "9806f66b7970fdff8617187bb9fffdff"
              "5ae4df3edbd5d35e5b4f09020db03eab"
              "1e031dda2fbe03d1792170a0f3009cee", 64);

    /* decrypting again with the same counter must restore the plaintext */
    aes128_ctr_xor(rk, iv, 0, buf, sizeof(buf));
    if (memcmp(buf, pt, sizeof(buf)) != 0) {
        printf("FAIL aes128-ctr round trip\n");
        failures++;
    } else {
        printf("ok   aes128-ctr round trip\n");
    }

    /* 256 byte pages are programmed separately: block offsets must line up */
    {
        uint8_t  stream[512];
        uint8_t  whole[512];
        uint8_t  page0[256];
        uint8_t  page1[256];
        uint32_t i;

        for (i = 0; i < sizeof(stream); i++) {
            stream[i] = (uint8_t)(i * 7u + 3u);
        }

        /* one shot */
        memcpy(whole, stream, sizeof(stream));
        aes128_ctr_xor(rk, iv, 0, whole, sizeof(whole));

        /* page by page */
        memcpy(page0, stream, 256);
        memcpy(page1, stream + 256, 256);
        aes128_ctr_xor(rk, iv, 0, page0, 256);
        aes128_ctr_xor(rk, iv, 256, page1, 256);

        if ((memcmp(page0, whole, 256) != 0) ||
            (memcmp(page1, whole + 256, 256) != 0)) {
            printf("FAIL aes128-ctr page offset\n");
            failures++;
        } else {
            printf("ok   aes128-ctr page offset\n");
        }
    }
}

int main(void)
{
    test_sha256();
    test_hmac();
    test_aes_ctr();

    if (failures == 0) {
        printf("ALL CRYPTO TESTS PASSED\n");
        return 0;
    }

    printf("%d FAILURE(S)\n", failures);

    return 1;
}
