/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_flash_host.c
 * Description        : Simulated CH32X035 flash for the host tests.
 *
 * The real img_flash.c talks to the FLASH controller; this one keeps a 32 KB
 * array and maps IMG_FLASH_ALIAS (0x08000000) onto its start, so the bootloader
 * code under test runs unmodified.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "img_flash.h"

#define HOST_FLASH_SIZE     0x10000u

static uint8_t g_flash[HOST_FLASH_SIZE];

/* the real controller ignores erase/program while locked, so does this one:
 * that catches a missing unlock in code that the stub would otherwise accept */
static int g_locked = 1;

uint8_t *host_flash_ptr(void)
{
    return g_flash;
}

void host_flash_reset(void)
{
    memset(g_flash, 0xFF, sizeof(g_flash));
    g_locked = 1;
}

void host_flash_corrupt(uint32_t addr)
{
    g_flash[addr - IMG_FLASH_ALIAS] ^= 0xFFu;
}

void img_flash_read(uint32_t addr, void *dst, uint32_t len)
{
    memcpy(dst, g_flash + (addr - IMG_FLASH_ALIAS), len);
}

void img_flash_unlock(void)
{
    g_locked = 0;
}

void img_flash_lock(void)
{
    g_locked = 1;
}

int img_flash_erase_page(uint32_t addr)
{
    uint32_t off = addr - IMG_FLASH_ALIAS;

    if (g_locked) {
        return -1;
    }
    if (addr & (IMG_PAGE_SIZE - 1u)) {
        return -1;
    }
    if ((off + IMG_PAGE_SIZE) > sizeof(g_flash)) {
        return -1;
    }

    memset(g_flash + off, 0xFF, IMG_PAGE_SIZE);

    return 0;
}

int img_flash_erase_sector(uint32_t addr)
{
    uint32_t off = addr - IMG_FLASH_ALIAS;

    if (g_locked) {
        return -1;
    }
    if (addr & (IMG_SECTOR_SIZE - 1u)) {
        return -1;
    }
    if ((off + IMG_SECTOR_SIZE) > sizeof(g_flash)) {
        return -1;
    }

    memset(g_flash + off, 0xFF, IMG_SECTOR_SIZE);

    return 0;
}

int img_flash_write_page(uint32_t addr, const uint8_t *data)
{
    uint32_t off = addr - IMG_FLASH_ALIAS;

    if (g_locked) {
        return -1;
    }
    if (addr & (IMG_PAGE_SIZE - 1u)) {
        return -1;
    }
    if ((off + IMG_PAGE_SIZE) > sizeof(g_flash)) {
        return -1;
    }

    /* flash can only clear bits: a page that was not erased will not match */
    {
        uint32_t i;
        for (i = 0; i < IMG_PAGE_SIZE; i++) {
            g_flash[off + i] &= data[i];
        }
    }

    return 0;
}

int img_flash_erase_slot(uint32_t len)
{
    uint32_t addr;
    uint32_t end;

    if (len == 0u) {
        return 0;
    }
    if (len > IMG_SLOT_SIZE) {
        return -1;
    }

    end = IMG_SLOT_BASE + ((len + IMG_SECTOR_SIZE - 1u) & ~(IMG_SECTOR_SIZE - 1u));

    for (addr = IMG_SLOT_BASE; addr < end; addr += IMG_SECTOR_SIZE) {
        if (img_flash_erase_sector(addr) != 0) {
            return -1;
        }
    }

    return 0;
}

int img_flash_erase_image(uint32_t len)
{
    if (img_flash_erase_slot(len) != 0) {
        return -1;
    }
    if (img_flash_erase_page(IMG_HDR_BASE) != 0) {
        return -1;
    }

    return img_flash_erase_page(IMG_FLAG_BASE);
}
