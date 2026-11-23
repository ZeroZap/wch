/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_flash.h
 * Description        : Erase/program the internal game slot without touching
 *                      the launcher.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __GAME_FLASH_H
#define __GAME_FLASH_H

#include <stdint.h>

int  game_flash_erase_sector(uint32_t addr);        /* 1 KB, 0 = OK */
int  game_flash_write_page(uint32_t addr, const uint8_t *data); /* 256 B */
void game_flash_read(uint32_t addr, void *dst, uint32_t len);

#endif /* __GAME_FLASH_H */
