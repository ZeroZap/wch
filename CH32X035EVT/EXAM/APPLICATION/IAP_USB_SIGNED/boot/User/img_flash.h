/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_flash.h
 * Description        : Thin wrapper around the CH32X035 fast page program API.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __IMG_FLASH_H
#define __IMG_FLASH_H

#include <stdint.h>
#include "img_hdr.h"

/* Program and erase are only allowed between these two calls. */
void img_flash_unlock(void);
void img_flash_lock(void);

/* All flash reads go through this so the same code can run against a
 * simulated flash array in the host tests. */
void img_flash_read(uint32_t addr, void *dst, uint32_t len);

/* addr must be 256 / 1024 byte aligned.  Returns 0 on success, negative on
 * error: -1 bad alignment, -2 erase or write did not verify, -3 controller
 * reported a failure. */
int  img_flash_erase_page(uint32_t addr);        /* 256 B */
int  img_flash_erase_sector(uint32_t addr);      /* 1 KB  */
int  img_flash_write_page(uint32_t addr, const uint8_t *data);

/* Erase `len` bytes of the app slot (rounded up to whole sectors) plus the
 * header and flag pages.  Used for a full install and for IAP_CMD_ERASE. */
int  img_flash_erase_image(uint32_t len);

/* Convenience: erase whole sectors covering [IMG_SLOT_BASE, IMG_SLOT_BASE+len) */
int  img_flash_erase_slot(uint32_t len);

#endif /* __IMG_FLASH_H */
