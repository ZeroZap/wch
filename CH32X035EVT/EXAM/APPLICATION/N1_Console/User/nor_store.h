/********************************** (C) COPYRIGHT *******************************
 * File Name          : nor_store.h
 * Description        : SPI NOR cartridge driver + persistent settings / scores.
 *
 * The cartridge is the same W25Qxx socket used by WS2812_Panel_IAP:
 *   SCK PA5, MISO PA6, MOSI PA7, CS PA2.
 *
 * Layout (default 4 MB W25Q32):
 *   0x000000   settings slot A (4 KB)
 *   0x001000   settings slot B (4 KB)
 *   0x010000   reserved for game packs / level data
 *   rest       free
 *
 * Two slots with a sequence number are used so a power loss during a save
 * leaves the previous settings intact.  Without a cartridge the console still
 * runs; settings simply live in RAM.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __NOR_STORE_H
#define __NOR_STORE_H

#include <stdint.h>
#include "console_config.h"

#define STORE_MAGIC     0x4E315354u     /* "N1ST" */
#define STORE_VERSION   1u

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t seq;
    uint8_t  bright;
    uint8_t  sound;
    uint8_t  last_game;
    uint8_t  reserved;
    uint16_t hi[STORE_HI_COUNT];
    uint16_t crc;
} __attribute__((packed)) settings_t;

/* Returns 1 when a cartridge answered.  Always leaves the settings loaded. */
uint8_t store_init(void);
uint8_t store_present(void);

/* Flush the RAM copy to the next slot.  No-op without a cartridge. */
void store_save(void);

uint8_t  store_brightness(void);
void     store_set_brightness(uint8_t level);
uint8_t  store_sound(void);
void     store_set_sound(uint8_t on);
uint8_t  store_last_game(void);
void     store_set_last_game(uint8_t index);

uint16_t store_hi(uint8_t game);
void     store_set_hi(uint8_t game, uint16_t score);
void     store_reset_hi(void);

uint32_t store_jedec_id(void);
uint32_t store_size(void);

/* Raw NOR access for the cartridge uploader.  Addresses are absolute NOR
 * addresses; erase is 4 KB aligned, write_page needs a 256 B aligned address
 * and at most 256 bytes. */
void nor_raw_read(uint32_t addr, void *buf, uint32_t len);
void nor_raw_erase_sector(uint32_t addr);
void nor_raw_write_page(uint32_t addr, const uint8_t *buf, uint16_t len);

#endif /* __NOR_STORE_H */
