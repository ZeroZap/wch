/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_flash.c
 * Description        : See game_flash.h.  Mirrors the bootloader's flash
 *                      helpers but targets N1_GAME_BASE (0x08006800).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "game_flash.h"
#include "n1_layout.h"
#include "ch32x035.h"

#define PAGE_SIZE       256u
#define SECTOR_SIZE     1024u

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

int game_flash_erase_sector(uint32_t addr)
{
    if (addr & (SECTOR_SIZE - 1u)) {
        return -1;
    }

    FLASH_Unlock();
    if (FLASH_ErasePage(addr) != FLASH_COMPLETE) {
        FLASH_Lock();
        return -2;
    }
    FLASH_Lock();

    return erased_ok(addr, SECTOR_SIZE) ? 0 : -3;
}

int game_flash_write_page(uint32_t addr, const uint8_t *data)
{
    uint32_t i;
    uint32_t word;

    if (addr & (PAGE_SIZE - 1u)) {
        return -1;
    }

    FLASH_Unlock();
    FLASH_Unlock_Fast();

    FLASH_BufReset();
    for (i = 0; i < PAGE_SIZE; i += 4u) {
        word = (uint32_t)data[i] |
               ((uint32_t)data[i + 1u] << 8) |
               ((uint32_t)data[i + 2u] << 16) |
               ((uint32_t)data[i + 3u] << 24);
        FLASH_BufLoad(addr + i, word);
    }
    FLASH_ProgramPage_Fast(addr);

    FLASH_Lock_Fast();
    FLASH_Lock();

    for (i = 0; i < PAGE_SIZE; i++) {
        if (*(const volatile uint8_t *)(addr + i) != data[i]) {
            return -2;
        }
    }

    return 0;
}

void game_flash_read(uint32_t addr, void *dst, uint32_t len)
{
    memcpy(dst, (const void *)addr, len);
}
