/********************************** (C) COPYRIGHT *******************************
 * File Name          : console_config.h
 * Description        : Application level configuration of the N1 console.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __CONSOLE_CONFIG_H
#define __CONSOLE_CONFIG_H

#include "game_config.h"

/* software version reported over USB */
#define CONSOLE_PROTO_VER       1u

/* 60 fps target.  A 16x16 frame takes ~7.2 ms to shift out, so there is room
 * for the game logic plus one USB command per frame. */
#define CONSOLE_FPS             60u
#define CONSOLE_FRAME_MS        (1000u / CONSOLE_FPS)

/* menu */
#define CONSOLE_REPEAT_DELAY    26u     /* frames before a held key repeats  */
#define CONSOLE_REPEAT_RATE     7u      /* frames between repeats             */
#define CONSOLE_IAP_HOLD_FRAMES 120u    /* UP+DOWN held 2 s -> bootloader     */
#define CONSOLE_EXIT_HOLD_FRAMES 45u    /* B held 0.75 s inside a game        */

/* external NOR layout (W25Q32 / 4 MB) */
#define STORE_SECTOR_SIZE       4096u
#define STORE_SLOT_A            0x000000u
#define STORE_SLOT_B            0x001000u
#define STORE_ASSET_BASE        0x010000u   /* future game packs / levels */
#define STORE_HI_COUNT          8u

#define CONSOLE_MAX_GAMES       8u

/* battery sense: 1:1 divider, 3.3 V reference, 12-bit ADC */
#define BAT_ADC_FULL_MV         3300u
#define BAT_DIVIDER_NUM         2u

#endif /* __CONSOLE_CONFIG_H */
