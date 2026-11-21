/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_hdr.h
 * Description        : On-flash image format shared by boot, app and host tools.
 *
 * This header is the single source of truth for
 *   - the flash layout (boot / app slot / header page / flag page)
 *   - the signed image header written at IMG_HDR_OFFSET
 *   - the flag page that tells the bootloader what to do
 *
 * Both the bootloader and the host tool (tools/iap_pack.py) must agree on it
 * byte for byte.  The layout is little endian throughout.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __IMG_HDR_H
#define __IMG_HDR_H

#include <stdint.h>

/*---------------------------------------------------------------------------
 * flash layout
 *
 *   0x00000000 .. 0x000027FF  bootloader            10 KB
 *   0x00002800 .. 0x0000EFFF  app slot              50 KB
 *   0x0000F000 .. 0x0000F0FF  image header page     256 B
 *   0x0000F100 .. 0x0000F1FF  flag page             256 B
 *   0x0000F200 .. 0x0000F7FF  user data / spare     1.5 KB
 *
 * The bootloader is capped at 10 KB: one image per product with its key and
 * version compiled in (see boot/User/boot_info.h and boot/User/img_key.c), so
 * a product never needs the key to be provisioned at run time.  Everything
 * below 0xF000 that the boot does not use belongs to the application.
 *
 * The offsets are flash-relative.  Code runs from the 0x00000000 alias, while
 * the flash controller wants the 0x08000000 alias, so both are provided.
 *---------------------------------------------------------------------------*/
#define IMG_FLASH_ALIAS         0x08000000u

#define IMG_BOOT_OFFSET         0x00000000u
#define IMG_BOOT_SIZE           0x00002800u     /* 10 KB, enforced by boot/Ld/Link.ld */
#define IMG_SLOT_OFFSET         0x00002800u
#define IMG_SLOT_SIZE           0x0000C800u     /* 50 KB, 200 sectors of 1 KB */
#define IMG_HDR_OFFSET          0x0000F000u
#define IMG_FLAG_OFFSET         0x0000F100u
#define IMG_USER_OFFSET         0x0000F200u     /* spare, not used by the boot */
#define IMG_USER_SIZE           0x00000600u     /* 1.5 KB */
#define IMG_FLASH_END           0x0000F800u     /* 62 KB, every CH32X033/035 package */

#define IMG_SLOT_BASE           (IMG_FLASH_ALIAS + IMG_SLOT_OFFSET)
#define IMG_SLOT_END            (IMG_SLOT_BASE + IMG_SLOT_SIZE)
#define IMG_HDR_BASE            (IMG_FLASH_ALIAS + IMG_HDR_OFFSET)
#define IMG_FLAG_BASE           (IMG_FLASH_ALIAS + IMG_FLAG_OFFSET)

/* Execution address of the app: must match app/Ld/Link.ld ORIGIN. */
#define IMG_APP_EXEC_ADDR       IMG_SLOT_OFFSET

/* Flash granularity.
 *
 * The CH32X035 flash has two erase units (datasheet table 3-11/3-12):
 *   page   256 B   erase 2.5 ms typ / 3.0 ms max, program 1.5 ms typ / 2.0 ms max
 *   sector   1 KB  erase 2.7 ms typ / 3.3 ms max
 * Endurance 300K cycles at 25 C (100K at 70 C), retention 20 / 10 years.
 *
 * Erasing one sector instead of four pages costs about the same time but uses
 * a quarter of the endurance budget, so the installer erases the app slot in
 * 1 KB sectors and only programs 256 B pages afterwards.
 */
#define IMG_PAGE_SIZE           256u
#define IMG_SECTOR_SIZE         1024u

/*---------------------------------------------------------------------------
 * image header, 128 bytes, written to IMG_HDR_BASE
 *
 * The first IMG_HMAC_COVER bytes are known before the payload is streamed, so
 * the HMAC can be computed over (hdr[0..IMG_HMAC_COVER) || payload) in a single
 * pass while the image is being received.
 *
 *   HMAC   = HMAC-SHA256(hmac_key, hdr[0..IMG_HMAC_COVER) || plaintext payload)
 *   sha256 = SHA-256(plaintext payload)
 *
 * Both cover the *plaintext*: the app executes in place out of the slot, so
 * encryption is a transport/at-rest-in-the-file transform only and the bootloader
 * always finds plaintext on flash.  That also lets it re-verify the installed
 * image on every reset without keeping the ciphertext around.
 *
 * img_crc32 and sha256 are derived values: the bootloader recomputes them, they
 * are not part of the authenticated prefix, and they exist so a host can sanity
 * check a flash dump.
 *---------------------------------------------------------------------------*/
#define IMG_MAGIC               0x474D4957u     /* "WIMG" */
#define IMG_HDR_VER             1u
#define IMG_HDR_SIZE            128u
#define IMG_HMAC_COVER          0x20u

#define IMG_FLAG_ENCRYPTED      0x0001u
#define IMG_FLAG_KNOWN_MASK     0x0001u

typedef struct {
    uint32_t magic;         /* 0x00 */
    uint16_t hdr_ver;       /* 0x04 */
    uint16_t flags;         /* 0x06 */
    uint32_t load_addr;     /* 0x08, execution address, must be IMG_APP_EXEC_ADDR */
    uint32_t img_len;       /* 0x0C, plaintext length in bytes */
    uint8_t  iv[16];        /* 0x10, AES-CTR initial counter block */
    /* ---- end of the HMAC covered prefix (0x20) ---- */
    uint32_t img_crc32;     /* 0x20, CRC-32 of the plaintext payload */
    uint8_t  sha256[32];    /* 0x24, SHA-256 of the plaintext payload */
    uint8_t  hmac[32];      /* 0x44, see above */
    uint8_t  reserved[28];  /* 0x64, zero filled, must stay zero */
} __attribute__((packed)) img_hdr_t;

/*---------------------------------------------------------------------------
 * flag page, written to IMG_FLAG_BASE
 *---------------------------------------------------------------------------*/
#define IMG_FLAG_MAGIC          0x47414C46u     /* "FLAG" */

#define IMG_STATE_NONE          0u              /* no app, stay in bootloader */
#define IMG_STATE_VALID         1u              /* verified app, safe to jump  */
#define IMG_STATE_UPGRADE       2u              /* app asked to be replaced    */
#define IMG_STATE_INVALID       3u              /* image failed verification   */

typedef struct {
    uint32_t magic;         /* 0x00 */
    uint32_t state;         /* 0x04, IMG_STATE_* */
    uint32_t img_len;       /* 0x08, copy of the header img_len */
    uint32_t hdr_crc32;     /* 0x0C, CRC-32 of hdr[0..IMG_HMAC_COVER) */
    uint8_t  reserved[16];  /* 0x10 */
} __attribute__((packed)) img_flag_t;

/*---------------------------------------------------------------------------
 * helpers usable on the target and in host tests
 *---------------------------------------------------------------------------*/
static inline uint32_t img_get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void img_put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

/*---------------------------------------------------------------------------
 * boot -> host protocol command codes (frame format matches wsp_usb.c:
 *   A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8 )
 *---------------------------------------------------------------------------*/
#define IAP_PROTO_VER           2u      /* boot INFO layout version            */
#define IAP_CMD_INFO            0x01    /* -> ver, max payload, state, product */
#define IAP_CMD_BEGIN           0x10    /* <- img_len u32, flags u16, load u32, iv[16] */
#define IAP_CMD_DATA            0x11    /* <- up to IAP_MAX_PAYLOAD bytes       */
#define IAP_CMD_END             0x12    /* <- hmac[32] -> status byte           */
#define IAP_CMD_ABORT           0x13    /* abort the running transfer           */
#define IAP_CMD_JUMP            0x14    /* leave the bootloader                 */
#define IAP_CMD_ERASE           0x15    /* erase app slot + header + flag       */
#define IAP_CMD_READ            0x16    /* <- addr u32, len u16 -> len bytes    */

#define IAP_ACK                 0x80

/* status bytes returned by END (also usable as a generic reply payload) */
#define IAP_ST_OK               0x00
#define IAP_ST_CRC              0x01    /* CRC-32 mismatch  */
#define IAP_ST_SHA              0x02    /* SHA-256 mismatch */
#define IAP_ST_MAC              0x03    /* HMAC mismatch    */
#define IAP_ST_LEN              0x04    /* bad length / state */
#define IAP_ST_FLASH            0x05    /* program or erase failed */
#define IAP_ST_NOAPP            0x06    /* nothing valid to jump to */
#define IAP_ST_BUSY             0x07

/* Largest payload that still fits a 64 byte USB packet: 64 - 6 bytes framing */
#define IAP_MAX_PAYLOAD         56u

#endif /* __IMG_HDR_H */
