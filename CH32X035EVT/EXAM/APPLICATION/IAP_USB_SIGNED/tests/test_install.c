/********************************** (C) COPYRIGHT *******************************
 * File Name          : test_install.c
 * Description        : Host test for the bootloader image state machine.
 *
 * Links the real boot/User/img_install.c against a simulated flash array
 * (tests/stub/img_flash_host.c) and exercises:
 *
 *   1. plain image: BEGIN / DATA (odd chunk sizes) / END     -> valid
 *   2. encrypted image                                       -> valid
 *   3. one corrupted payload byte                            -> verify fails
 *   4. wrong HMAC                                            -> END rejected,
 *                                                              nothing bootable
 *   5. truncated stream                                      -> END rejected
 *   6. the flash content matches the packed .img format byte for byte
 *
 * Everything is deterministic so tools/iap_pack.py can reproduce the same file
 * from vec.bin and the shell script can diff the two.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used by
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "img_hdr.h"
#include "img_flash.h"
#include "img_install.h"
#include "img_key.h"
#include "sha256.h"
#include "aes.h"

#define PAYLOAD_LEN     1000u

/* provided by tests/stub/img_flash_host.c */
void host_flash_reset(void);
void host_flash_corrupt(uint32_t addr);

static int g_fail;

static void check(const char *name, int cond, const char *detail)
{
    if (cond) {
        printf("  PASS  %s\n", name);
    } else {
        g_fail++;
        printf("  FAIL  %s%s%s\n", name, detail ? " " : "", detail ? detail : "");
    }
}

static uint32_t crc32_calc(const uint8_t *p, uint32_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i, k;

    for (i = 0; i < n; i++) {
        crc ^= p[i];
        for (k = 0; k < 8u; k++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }

    return crc ^ 0xFFFFFFFFu;
}

/* Build the reference header the way the bootloader does after a successful
 * transfer, and the reference stored payload (ciphertext when encrypted). */
static void build_reference(const uint8_t *pt, uint32_t len, uint8_t encrypt,
                            const uint8_t iv[16], img_hdr_t *h, uint8_t *stored)
{
    uint8_t rk[176];

    memcpy(stored, pt, len);

    if (encrypt) {
        uint32_t padded = (len + 15u) & ~15u;
        if (padded > len) {
            memset(stored + len, 0, padded - len);
        }
        aes128_init(img_aes_key, rk);
        aes128_ctr_xor(rk, iv, 0u, stored, padded);
    }

    memset(h, 0, sizeof(*h));
    h->magic     = IMG_MAGIC;
    h->hdr_ver   = IMG_HDR_VER;
    h->flags     = encrypt ? IMG_FLAG_ENCRYPTED : 0u;
    h->load_addr = IMG_APP_EXEC_ADDR;
    h->img_len   = len;
    memcpy(h->iv, iv, 16);

    h->img_crc32 = crc32_calc(pt, len);
    sha256(pt, len, h->sha256);

    {
        hmac_sha256_ctx m;
        hmac_sha256_init(&m, img_hmac_key, sizeof(img_hmac_key));
        hmac_sha256_update(&m, (const uint8_t *)h, IMG_HMAC_COVER);
        hmac_sha256_update(&m, pt, len);
        hmac_sha256_final(&m, h->hmac);
    }
}

static uint8_t transfer(const uint8_t *stored, uint32_t len, const uint8_t *iv,
                        uint32_t flags, const uint8_t *hmac,
                        uint32_t truncate_by)
{
    static const uint16_t steps[] = { 1, 7, 56, 3, 41, 16, 2, 56, 5, 32 };
    uint32_t off = 0u;
    uint32_t step = 0u;
    uint32_t limit = len - truncate_by;
    uint8_t  st;

    st = img_install_begin(len, (uint16_t)flags, IMG_APP_EXEC_ADDR, iv);
    if (st != IAP_ST_OK) {
        return st;
    }

    while (off < limit) {
        uint32_t n = steps[step % (sizeof(steps) / sizeof(steps[0]))];
        if (n > (limit - off)) {
            n = limit - off;
        }
        st = img_install_data(stored + off, n);
        if (st != IAP_ST_OK) {
            return st;
        }
        off += n;
        step++;
    }

    return img_install_finish(hmac);
}

static void compare_flash(const img_hdr_t *h, const uint8_t *stored, uint32_t len,
                          const char *tag)
{
    uint8_t  buf[IMG_PAGE_SIZE];
    uint32_t off;

    img_flash_read(IMG_HDR_BASE, buf, sizeof(buf));
    check("header page on flash", memcmp(buf, h, sizeof(*h)) == 0, tag);

    for (off = 0u; off < len; off += IMG_PAGE_SIZE) {
        uint32_t n = len - off;
        if (n > IMG_PAGE_SIZE) {
            n = IMG_PAGE_SIZE;
        }
        img_flash_read(IMG_SLOT_BASE + off, buf, n);
        if (memcmp(buf, stored + off, n) != 0) {
            check("payload on flash", 0, tag);
            return;
        }
    }
    check("payload on flash", 1, tag);
}

int main(int argc, char **argv)
{
    static uint8_t payload[PAYLOAD_LEN];
    static uint8_t stored[PAYLOAD_LEN + 16];
    static uint8_t image[IMG_HDR_SIZE + PAYLOAD_LEN];
    static uint8_t image_enc[IMG_HDR_SIZE + PAYLOAD_LEN];
    img_hdr_t hdr;
    uint8_t   iv[16];
    uint32_t  i;
    uint8_t   st;
    const char *outdir = (argc > 1) ? argv[1] : ".";

    for (i = 0u; i < PAYLOAD_LEN; i++) {
        payload[i] = (uint8_t)((i * 31u + 7u) ^ (i >> 3));
    }
    for (i = 0u; i < 16u; i++) {
        iv[i] = (uint8_t)i;
    }

    /* ---- 1. plain image ------------------------------------------------ */
    printf("plain image:\n");
    host_flash_reset();
    build_reference(payload, PAYLOAD_LEN, 0u, iv, &hdr, stored);
    st = transfer(stored, PAYLOAD_LEN, iv, 0u, hdr.hmac, 0u);
    check("install accepted", st == IAP_ST_OK, NULL);
    check("verify_stored", img_verify_stored() == IAP_ST_OK, NULL);
    compare_flash(&hdr, stored, PAYLOAD_LEN, NULL);

    /* keep the plain reference for the Python cross check */
    memcpy(image, &hdr, sizeof(hdr));
    memcpy(image + IMG_HDR_SIZE, stored, PAYLOAD_LEN);

    /* ---- 2. encrypted image -------------------------------------------- */
    printf("encrypted image:\n");
    host_flash_reset();
    build_reference(payload, PAYLOAD_LEN, 1u, iv, &hdr, stored);
    st = transfer(stored, PAYLOAD_LEN, iv, IMG_FLAG_ENCRYPTED, hdr.hmac, 0u);
    check("install accepted", st == IAP_ST_OK, NULL);
    check("verify_stored", img_verify_stored() == IAP_ST_OK, NULL);
    /* the slot always holds plaintext, the app executes in place */
    compare_flash(&hdr, payload, PAYLOAD_LEN, NULL);

    memcpy(image_enc, &hdr, sizeof(hdr));
    memcpy(image_enc + IMG_HDR_SIZE, stored, PAYLOAD_LEN);

    {
        uint8_t rd[PAYLOAD_LEN];
        img_flash_read(IMG_SLOT_BASE, rd, PAYLOAD_LEN);
        check("payload stored as plaintext (XIP)",
              memcmp(rd, payload, PAYLOAD_LEN) == 0, NULL);
    }

    /* ---- 3. corrupted payload ------------------------------------------ */
    printf("corrupted payload:\n");
    host_flash_corrupt(IMG_SLOT_BASE + 100u);
    st = img_verify_stored();
    check("verify rejects the image", st != IAP_ST_OK, NULL);
    check("fails on SHA-256 or HMAC", st == IAP_ST_SHA || st == IAP_ST_MAC || st == IAP_ST_CRC, NULL);

    /* ---- 4. wrong HMAC -------------------------------------------------- */
    printf("wrong hmac:\n");
    host_flash_reset();
    build_reference(payload, PAYLOAD_LEN, 0u, iv, &hdr, stored);
    hdr.hmac[0] ^= 0x01u;
    st = transfer(stored, PAYLOAD_LEN, iv, 0u, hdr.hmac, 0u);
    check("END rejected", st == IAP_ST_MAC, NULL);
    check("nothing bootable", img_verify_stored() != IAP_ST_OK, NULL);

    /* ---- 5. truncated stream -------------------------------------------- */
    printf("truncated stream:\n");
    host_flash_reset();
    build_reference(payload, PAYLOAD_LEN, 0u, iv, &hdr, stored);
    st = transfer(stored, PAYLOAD_LEN, iv, 0u, hdr.hmac, 1u);
    check("END rejected", st == IAP_ST_LEN, NULL);
    check("nothing bootable", img_verify_stored() != IAP_ST_OK, NULL);

    /* ---- 6. abort -------------------------------------------------------- */
    printf("abort:\n");
    host_flash_reset();
    build_reference(payload, PAYLOAD_LEN, 0u, iv, &hdr, stored);
    st = img_install_begin(PAYLOAD_LEN, 0u, IMG_APP_EXEC_ADDR, iv);
    check("begin", st == IAP_ST_OK, NULL);
    (void)img_install_data(stored, 300u);
    img_install_abort();
    check("nothing bootable", img_verify_stored() != IAP_ST_OK, NULL);

    /* ---- 7. re-install over the leftovers of the aborted one ------------- */
    printf("re-install after abort:\n");
    build_reference(payload, PAYLOAD_LEN, 0u, iv, &hdr, stored);
    st = transfer(stored, PAYLOAD_LEN, iv, 0u, hdr.hmac, 0u);
    check("install accepted", st == IAP_ST_OK, NULL);
    check("verify_stored", img_verify_stored() == IAP_ST_OK, NULL);
    compare_flash(&hdr, stored, PAYLOAD_LEN, NULL);

    /* ---- write the cross check files ------------------------------------ */
    {
        char path[512];
        FILE *fh;

        snprintf(path, sizeof(path), "%s/vec.bin", outdir);
        fh = fopen(path, "wb");
        if (!fh) { perror(path); return 1; }
        fwrite(payload, 1, PAYLOAD_LEN, fh);
        fclose(fh);

        snprintf(path, sizeof(path), "%s/vec_expected.img", outdir);
        fh = fopen(path, "wb");
        if (!fh) { perror(path); return 1; }
        fwrite(image, 1, sizeof(image), fh);
        fclose(fh);

        snprintf(path, sizeof(path), "%s/vec_expected_enc.img", outdir);
        fh = fopen(path, "wb");
        if (!fh) { perror(path); return 1; }
        fwrite(image_enc, 1, sizeof(image_enc), fh);
        fclose(fh);
    }

    printf("\n%s\n", g_fail ? "INSTALL TESTS FAILED" : "ALL INSTALL TESTS PASSED");
    return g_fail ? 1 : 0;
}
