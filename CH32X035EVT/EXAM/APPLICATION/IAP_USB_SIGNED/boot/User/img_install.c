/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_install.c
 * Description        : See img_install.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <stddef.h>
#include <string.h>

#include "img_install.h"
#include "img_flash.h"
#include "img_key.h"
#include "sha256.h"
#include "aes.h"

/* The HMAC prefix is everything up to (but excluding) img_crc32. */
#define MAC_PREFIX_LEN      ((uint32_t)offsetof(img_hdr_t, img_crc32))

typedef struct {
    img_hdr_t      hdr;
    sha256_ctx     sha;
    hmac_sha256_ctx mac;
    uint8_t        aes_rk[176];
    uint32_t       crc;
    uint32_t       received;
    uint32_t       page_addr;
    uint16_t       page_fill;
    uint8_t        page_buf[IMG_PAGE_SIZE];
    uint8_t        write_flash;
    uint8_t        encrypted;
    uint8_t        active;
} img_ctx_t;

static img_ctx_t g_img;

/* staging buffer for reading pages back out of flash */
static uint8_t g_rd[IMG_PAGE_SIZE];

/* decrypted page while a transfer is encrypted */
static uint8_t g_pt[IMG_PAGE_SIZE];

/* page as it is written to flash (plaintext + 0xFF padding) */
static uint8_t g_pg[IMG_PAGE_SIZE];

/*---------------------------------------------------------------------------
 * CRC-32 (IEEE 802.3, reflected, poly 0xEDB88320) -- same as zlib.crc32()
 *---------------------------------------------------------------------------*/
static uint32_t crc32_update(uint32_t crc, const uint8_t *p, uint32_t n)
{
    uint32_t i, k;

    for (i = 0; i < n; i++) {
        crc ^= p[i];
        for (k = 0; k < 8u; k++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }

    return crc;
}

/*---------------------------------------------------------------------------
 * header helpers
 *---------------------------------------------------------------------------*/
static uint8_t hdr_fields_ok(const img_hdr_t *h)
{
    if (h->magic != IMG_MAGIC || h->hdr_ver != IMG_HDR_VER) {
        return 0;
    }
    if (h->load_addr != IMG_APP_EXEC_ADDR) {
        return 0;
    }
    if ((h->flags & (uint16_t)~IMG_FLAG_KNOWN_MASK) != 0u) {
        return 0;
    }
    if (h->img_len == 0u || h->img_len > IMG_SLOT_SIZE) {
        return 0;
    }

    return 1;
}

uint8_t img_load_hdr(img_hdr_t *out)
{
    img_flash_read(IMG_HDR_BASE, out, sizeof(*out));

    return hdr_fields_ok(out);
}

uint8_t img_load_flag(img_flag_t *out)
{
    img_flash_read(IMG_FLAG_BASE, out, sizeof(*out));

    if (out->magic != IMG_FLAG_MAGIC) {
        return 0;
    }
    if (out->state > IMG_STATE_INVALID) {
        return 0;
    }

    return 1;
}

uint8_t img_write_flag(uint32_t state, const img_hdr_t *hdr)
{
    uint8_t page[IMG_PAGE_SIZE];
    img_flag_t *f = (img_flag_t *)page;
    uint8_t rc;

    memset(page, 0xFF, sizeof(page));
    memset(f, 0, sizeof(*f));
    f->magic   = IMG_FLAG_MAGIC;
    f->state   = state;
    f->img_len = hdr ? hdr->img_len : 0u;
    f->hdr_crc32 = hdr ? crc32_update(0xFFFFFFFFu, (const uint8_t *)hdr,
                                      MAC_PREFIX_LEN) ^ 0xFFFFFFFFu : 0u;

    img_flash_unlock();
    rc = (uint8_t)img_flash_erase_page(IMG_FLAG_BASE);
    if (rc == 0u) {
        rc = (uint8_t)img_flash_write_page(IMG_FLAG_BASE, page);
    }
    img_flash_lock();

    return (rc == 0u) ? IAP_ST_OK : IAP_ST_FLASH;
}

/*---------------------------------------------------------------------------
 * streaming digest
 *---------------------------------------------------------------------------*/
static void digest_start(img_ctx_t *c)
{
    sha256_init(&c->sha);
    c->crc = 0xFFFFFFFFu;

    hmac_sha256_init(&c->mac, img_hmac_key, sizeof(img_hmac_key));
    hmac_sha256_update(&c->mac, (const uint8_t *)&c->hdr, MAC_PREFIX_LEN);

    c->received  = 0u;
    c->page_addr = IMG_SLOT_BASE;
    c->page_fill = 0u;
    c->encrypted = (uint8_t)((c->hdr.flags & IMG_FLAG_ENCRYPTED) ? 1u : 0u);

    if (c->encrypted) {
        aes128_init(img_aes_key, c->aes_rk);
    }
}

/* Hash and, when write_flash is set, store one page.  n is the number of real
 * bytes in page_buf; the remainder of the 256 byte page is padded with 0xFF. */
static uint8_t flush_page(img_ctx_t *c, uint32_t n)
{
    uint32_t       page_index = (c->page_addr - IMG_SLOT_BASE) / IMG_PAGE_SIZE;
    const uint8_t *plain = c->page_buf;

    if (c->encrypted) {
        /* The app executes in place from the slot, so the ciphertext is only
         * a transport format: decrypt, then store the plaintext.  page_index *
         * 256 is always a multiple of 16; the CTR helper wants whole blocks,
         * so round up and drop the extra keystream bytes below. */
        uint32_t dlen = (n + 15u) & ~15u;

        memcpy(g_pt, c->page_buf, dlen);
        aes128_ctr_xor(c->aes_rk, c->hdr.iv, page_index * IMG_PAGE_SIZE,
                       g_pt, dlen);
        plain = g_pt;
    }

    /* the MAC and the digest both cover the plaintext, so a stored image can be
     * re-verified on every reset without keeping the cipher around */
    hmac_sha256_update(&c->mac, plain, n);
    sha256_update(&c->sha, plain, n);
    c->crc = crc32_update(c->crc, plain, n);

    if (c->write_flash) {
        uint8_t st;

        memcpy(g_pg, plain, n);
        if (n < IMG_PAGE_SIZE) {
            memset(g_pg + n, 0xFF, IMG_PAGE_SIZE - n);
        }

        /* the slot was sector erased in img_install_begin(), so a page
         * program is enough.  Keep the flash locked between operations. */
        img_flash_unlock();
        st = (img_flash_write_page(c->page_addr, g_pg) != 0)
             ? IAP_ST_FLASH : IAP_ST_OK;
        img_flash_lock();

        if (st != IAP_ST_OK) {
            return st;
        }
    }

    c->page_addr += IMG_PAGE_SIZE;
    c->page_fill  = 0u;

    return IAP_ST_OK;
}

static uint8_t digest_update(img_ctx_t *c, const uint8_t *data, uint32_t len)
{
    uint32_t off = 0u;

    if ((c->received + len) > c->hdr.img_len) {
        return IAP_ST_LEN;
    }

    c->received += len;

    while (off < len) {
        uint32_t space = IMG_PAGE_SIZE - c->page_fill;
        uint32_t take  = len - off;

        if (take > space) {
            take = space;
        }

        memcpy(c->page_buf + c->page_fill, data + off, take);
        c->page_fill = (uint16_t)(c->page_fill + take);
        off += take;

        if (c->page_fill == IMG_PAGE_SIZE) {
            uint8_t st = flush_page(c, IMG_PAGE_SIZE);
            if (st != IAP_ST_OK) {
                return st;
            }
        }
    }

    return IAP_ST_OK;
}

static uint8_t digest_final(img_ctx_t *c, uint8_t hmac_out[32], uint8_t sha_out[32])
{
    uint8_t st;

    if (c->page_fill != 0u) {
        st = flush_page(c, c->page_fill);
        if (st != IAP_ST_OK) {
            return st;
        }
    }

    sha256_final(&c->sha, sha_out);
    hmac_sha256_final(&c->mac, hmac_out);

    return IAP_ST_OK;
}

/*---------------------------------------------------------------------------
 * public API
 *---------------------------------------------------------------------------*/
uint8_t img_install_begin(uint32_t img_len, uint16_t flags, uint32_t load_addr,
                          const uint8_t iv[16])
{
    memset(&g_img, 0, sizeof(g_img));

    g_img.hdr.magic     = IMG_MAGIC;
    g_img.hdr.hdr_ver   = IMG_HDR_VER;
    g_img.hdr.flags     = flags;
    g_img.hdr.load_addr = load_addr;
    g_img.hdr.img_len   = img_len;
    memcpy(g_img.hdr.iv, iv, 16);

    if (!hdr_fields_ok(&g_img.hdr)) {
        return IAP_ST_LEN;
    }

    /* The slot is cleared up front, in 1 KB sectors: about the same time as
     * erasing four pages but a quarter of the endurance cost, and it lets the
     * streaming path do nothing but page programs.  A reset in the middle of
     * the update therefore cannot jump into a half written app. */
    img_flash_unlock();
    if (img_flash_erase_image(img_len) != 0) {
        img_flash_lock();
        return IAP_ST_FLASH;
    }
    img_flash_lock();

    digest_start(&g_img);
    g_img.write_flash = 1u;
    g_img.active      = 1u;

    return IAP_ST_OK;
}

uint8_t img_install_data(const uint8_t *data, uint32_t len)
{
    if (!g_img.active) {
        return IAP_ST_LEN;
    }

    return digest_update(&g_img, data, len);
}

uint8_t img_install_finish(const uint8_t hmac_in[32])
{
    uint8_t hmac_calc[32];
    uint8_t sha_calc[32];
    uint8_t page[IMG_PAGE_SIZE];
    uint8_t st;

    if (!g_img.active) {
        return IAP_ST_LEN;
    }
    g_img.active = 0u;

    if (g_img.received != g_img.hdr.img_len) {
        return IAP_ST_LEN;
    }

    st = digest_final(&g_img, hmac_calc, sha_calc);
    if (st != IAP_ST_OK) {
        return st;
    }

    if (memcmp(hmac_calc, hmac_in, sizeof(hmac_calc)) != 0) {
        /* the flag/header pages are already erased, stay in the bootloader */
        return IAP_ST_MAC;
    }

    g_img.hdr.img_crc32 = g_img.crc ^ 0xFFFFFFFFu;
    memcpy(g_img.hdr.sha256, sha_calc, sizeof(sha_calc));
    memcpy(g_img.hdr.hmac, hmac_calc, sizeof(hmac_calc));

    memset(page, 0xFF, sizeof(page));
    memcpy(page, &g_img.hdr, sizeof(g_img.hdr));

    img_flash_unlock();
    if (img_flash_erase_page(IMG_HDR_BASE) != 0) {
        img_flash_lock();
        return IAP_ST_FLASH;
    }
    if (img_flash_write_page(IMG_HDR_BASE, page) != 0) {
        img_flash_lock();
        return IAP_ST_FLASH;
    }
    img_flash_lock();

    return img_write_flag(IMG_STATE_VALID, &g_img.hdr);
}

uint8_t img_install_active(void)
{
    return g_img.active;
}

void img_install_abort(void)
{
    g_img.active = 0u;

    (void)img_write_flag(IMG_STATE_NONE, NULL);
}

uint8_t img_verify_stored(void)
{
    img_hdr_t   hdr;
    uint8_t     hmac_calc[32];
    uint8_t     sha_calc[32];
    uint32_t    off;
    uint8_t     st;

    if (!img_load_hdr(&hdr)) {
        return IAP_ST_NOAPP;
    }
    /* the flag state does not matter here: JUMP is also the rescue path when
     * the app asked for an upgrade and the host then changed its mind */

    memset(&g_img, 0, sizeof(g_img));
    g_img.hdr = hdr;
    digest_start(&g_img);
    g_img.write_flash = 0u;
    /* the slot always holds plaintext (the app runs in place), so the stored
     * image is hashed as it is and never decrypted again */
    g_img.encrypted = 0u;

    for (off = 0u; off < hdr.img_len; off += IMG_PAGE_SIZE) {
        uint32_t n = hdr.img_len - off;

        if (n > IMG_PAGE_SIZE) {
            n = IMG_PAGE_SIZE;
        }

        img_flash_read(IMG_SLOT_BASE + off, g_rd, n);
        st = digest_update(&g_img, g_rd, n);
        if (st != IAP_ST_OK) {
            return st;
        }
    }

    st = digest_final(&g_img, hmac_calc, sha_calc);
    if (st != IAP_ST_OK) {
        return st;
    }

    if (memcmp(sha_calc, hdr.sha256, sizeof(sha_calc)) != 0) {
        return IAP_ST_SHA;
    }
    if ((g_img.crc ^ 0xFFFFFFFFu) != hdr.img_crc32) {
        return IAP_ST_CRC;
    }
    if (memcmp(hmac_calc, hdr.hmac, sizeof(hmac_calc)) != 0) {
        return IAP_ST_MAC;
    }

    return IAP_ST_OK;
}
