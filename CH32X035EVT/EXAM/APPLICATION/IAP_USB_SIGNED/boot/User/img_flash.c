/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_flash.c
 * Description        : Flash helpers for the bootloader.
 *
 * The CH32X035 flash has two erase units (datasheet table 3-11):
 *
 *   256 B page   erase 2.5/3.0 ms, program 1.5/2.0 ms   (typ/max)
 *   1 KB sector  erase 2.7/3.3 ms
 *
 * Erasing one sector is about as fast as erasing one page but consumes a
 * quarter of the endurance budget (300K cycles at 25 C), so the installer
 * clears the app slot with sector erases and only uses page programs while
 * the image streams in.  Every erase and program is read back before it is
 * trusted: a flash that refuses to program is a hard error, not a silent one.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "img_flash.h"
#include "ch32x035.h"

void img_flash_read(uint32_t addr, void *dst, uint32_t len)
{
    memcpy(dst, (const void *)addr, len);
}

void img_flash_unlock(void)
{
    FLASH_Unlock();         /* 1 KB sector erase */
    FLASH_Unlock_Fast();    /* 256 B page erase / program */
}

void img_flash_lock(void)
{
    FLASH_Lock_Fast();
    FLASH_Lock();
}

static int erased_ok(uint32_t addr, uint32_t len)
{
    uint32_t i;
    const volatile uint8_t *p = (const volatile uint8_t *)addr;

    for (i = 0; i < len; i++) {
        if (p[i] != 0xFFu) {
            return 0;
        }
    }

    return 1;
}

int img_flash_erase_page(uint32_t addr)
{
    if (addr & (IMG_PAGE_SIZE - 1u)) {
        return -1;
    }

    FLASH_ErasePage_Fast(addr);

    return erased_ok(addr, IMG_PAGE_SIZE) ? 0 : -2;
}

int img_flash_erase_sector(uint32_t addr)
{
    if (addr & (IMG_SECTOR_SIZE - 1u)) {
        return -1;
    }

    if (FLASH_ErasePage(addr) != FLASH_COMPLETE) {
        return -3;
    }

    return erased_ok(addr, IMG_SECTOR_SIZE) ? 0 : -2;
}

int img_flash_write_page(uint32_t addr, const uint8_t *data)
{
    uint32_t i, word;

    if (addr & (IMG_PAGE_SIZE - 1u)) {
        return -1;
    }

    FLASH_BufReset();
    for (i = 0; i < IMG_PAGE_SIZE; i += 4u) {
        word = (uint32_t)data[i] |
               ((uint32_t)data[i + 1u] << 8) |
               ((uint32_t)data[i + 2u] << 16) |
               ((uint32_t)data[i + 3u] << 24);
        FLASH_BufLoad(addr + i, word);
    }
    FLASH_ProgramPage_Fast(addr);

    for (i = 0; i < IMG_PAGE_SIZE; i++) {
        if (*(const volatile uint8_t *)(addr + i) != data[i]) {
            return -2;
        }
    }

    return 0;
}

int img_flash_erase_slot(uint32_t len)
{
    uint32_t addr;
    uint32_t end;
    int      rc;

    if (len == 0u) {
        return 0;
    }
    if (len > IMG_SLOT_SIZE) {
        return -1;
    }

    end = IMG_SLOT_BASE + ((len + IMG_SECTOR_SIZE - 1u) & ~(IMG_SECTOR_SIZE - 1u));

    for (addr = IMG_SLOT_BASE; addr < end; addr += IMG_SECTOR_SIZE) {
        rc = img_flash_erase_sector(addr);
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

int img_flash_erase_image(uint32_t len)
{
    int rc;

    rc = img_flash_erase_slot(len);
    if (rc != 0) {
        return rc;
    }

    rc = img_flash_erase_page(IMG_HDR_BASE);
    if (rc != 0) {
        return rc;
    }

    return img_flash_erase_page(IMG_FLAG_BASE);
}
