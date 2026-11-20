/********************************** (C) COPYRIGHT *******************************
 * File Name          : norflash.h
 * Description        : Minimal W25Qxx driver on SPI1 (SCK PA5, MISO PA6,
 *                      MOSI PA7, CS PA2).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __NORFLASH_H
#define __NORFLASH_H

#include "ch32x035.h"
#include "wsp_config.h"
#include "wsp_img.h"

#ifdef __cplusplus
extern "C" {
#endif

/* returns 1 if a supported NOR flash answered */
uint8_t  nor_init(void);

uint32_t nor_jedec_id(void);
uint32_t nor_size(void);

void nor_read(uint32_t addr, void *buf, uint32_t len);
void nor_erase_sector(uint32_t addr);

/* Erase + program. "addr" must be 4K aligned and "buf" is written as is
 * (no read-modify-write). */
void nor_write(uint32_t addr, const void *buf, uint32_t len);

/* img_src_t that reads a WSP1 file stored at "base" inside the NOR flash */
int nor_img_src(img_src_t *src, uint32_t base);

/* manufacturer name decoded from the JEDEC ID, e.g. "GigaDevice" */
const char *nor_vendor(void);

/* erase + write + read back a pattern in the last 4KB sector.
 * Destroys that sector, intended for wiring bring-up only. */
uint8_t nor_selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* __NORFLASH_H */
