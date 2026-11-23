/********************************** (C) COPYRIGHT *******************************
 * File Name          : cart.h
 * Description        : NOR cartridge catalog, game loader and USB upload state.
 *
 * The host writes one contiguous image at CART_BASE_OFFSET (0x010000):
 * a 4 KB catalog sector followed by 4 KB aligned game binaries.  The launcher
 * reads the catalog, copies the selected game into the internal game slot and
 * jumps; it never executes from NOR.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __CART_H
#define __CART_H

#include <stdint.h>
#include "n1_layout.h"

#define CART_OK             0u
#define CART_ERR_NOCART     1u
#define CART_ERR_PARAM      2u
#define CART_ERR_NOR        3u
#define CART_ERR_CRC        4u
#define CART_ERR_FLASH      5u

typedef struct {
    uint32_t magic;         /* CART_MAGIC "N1CA"            */
    uint16_t version;       /* CART_VERSION                 */
    uint16_t count;         /* 1..CART_MAX_GAMES            */
    uint32_t total_size;    /* bytes from CART_BASE_OFFSET  */
    uint32_t crc32;         /* CRC32 of the whole image     */
    uint8_t  reserved[16];
} __attribute__((packed)) cart_hdr_t;

typedef struct {
    char     name[12];
    char     code[4];
    uint8_t  theme;         /* panel_gfx palette index      */
    uint8_t  flags;
    uint16_t reserved;
    uint32_t offset;        /* absolute NOR address         */
    uint32_t size;          /* game .bin length             */
    uint32_t crc32;         /* CRC32 of the game .bin       */
    uint8_t  icon[20];      /* 16x10 1-bit, 2 bytes per row */
} __attribute__((packed)) cart_entry_t;

/* Read/validate the catalog at CART_BASE_OFFSET.  1 = usable cart. */
uint8_t cart_scan(void);
uint8_t cart_valid(void);
uint8_t cart_count(void);
const cart_entry_t *cart_entry(uint8_t index);
uint32_t cart_total_size(void);
uint32_t cart_crc32(void);

/* Draw the catalog icon on the WS2812 panel (16x10 pixels). */
void cart_draw_icon(const cart_entry_t *e);

/* Copy a game from NOR to N1_GAME_BASE and verify CRC32.  0 = OK. */
uint8_t cart_load(uint8_t index);

/* Leave the launcher and execute the game slot.  Does not return. */
void cart_jump(void) __attribute__((noreturn));

/*---------------------------------------------------------------------------
 * USB cartridge uploader
 *---------------------------------------------------------------------------*/
uint8_t  cart_upload_begin(uint32_t total_size, uint32_t crc32);
uint8_t  cart_upload_data(const uint8_t *data, uint16_t len);
uint8_t  cart_upload_end(void);
void     cart_upload_abort(void);
uint8_t  cart_upload_active(void);
uint32_t cart_upload_received(void);

/* Erase the catalog + the currently described game area. */
uint8_t cart_erase(void);

#endif /* __CART_H */
