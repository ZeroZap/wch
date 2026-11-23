/********************************** (C) COPYRIGHT *******************************
 * File Name          : n1_layout.h
 * Description        : Internal flash partition for the N1 console.
 *
 * The IAP bootloader keeps its 50 KB app-slot limit, but a normal update only
 * erases/programs the launcher length from the signed header.  That lets the
 * launcher be much smaller and leaves the rest of the slot as a game slot:
 *
 *   0x0000..0x27FF  IAP_USB_SIGNED boot        10 KB
 *   0x2800..0x77FF  N1 launcher                20 KB   signed image
 *   0x7800..0xEFFF  game slot                  30 KB   loaded from NOR
 *   0xF000..0xF0FF  image header page
 *   0xF100..0xF1FF  flag page
 *   0xF200..0xF7FF  spare
 *
 * A game .bin is linked at N1_GAME_OFFSET and capped at N1_GAME_SIZE.
 * The launcher copies it from NOR, verifies CRC32, then jumps via SW_Handler.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __N1_LAYOUT_H
#define __N1_LAYOUT_H

/* flash-relative offsets (code runs from the 0x00000000 alias) */
#define N1_BOOT_OFFSET          0x00000000u
#define N1_BOOT_SIZE            0x00002800u
#define N1_LAUNCHER_OFFSET      0x00002800u
#define N1_LAUNCHER_SIZE        0x00005000u   /* 20 KB */
#define N1_GAME_OFFSET          0x00007800u
#define N1_GAME_SIZE            0x00007800u   /* 30 KB */
#define N1_HDR_OFFSET           0x0000F000u
#define N1_FLAG_OFFSET          0x0000F100u
#define N1_FLASH_END            0x0000F800u

/* flash controller alias used for erase/program */
#define N1_FLASH_ALIAS          0x08000000u
#define N1_LAUNCHER_BASE        (N1_FLASH_ALIAS + N1_LAUNCHER_OFFSET)
#define N1_GAME_BASE            (N1_FLASH_ALIAS + N1_GAME_OFFSET)

#if (N1_LAUNCHER_OFFSET + N1_LAUNCHER_SIZE) != N1_GAME_OFFSET
#error "launcher partition must end exactly at N1_GAME_OFFSET"
#endif

#if (N1_GAME_OFFSET + N1_GAME_SIZE) != N1_HDR_OFFSET
#error "game slot must end exactly at the image header page"
#endif

/*---------------------------------------------------------------------------
 * NOR cartridge layout (W25Q32 / 4 MB)
 *
 * The host tool writes one contiguous image at CART_BASE_OFFSET:
 *
 *   +0x0000  catalog sector (4 KB, header + entries, 0xFF padded)
 *   +0x1000  game 0 (4 KB aligned)
 *   +...     game 1 ...
 *
 * Catalog entry offsets are absolute NOR addresses, so the launcher can read
 * a game without caring how the file was uploaded.
 *---------------------------------------------------------------------------*/
#define CART_BASE_OFFSET        0x00010000u
#define CART_CAT_SIZE           0x00001000u
#define CART_DATA_OFFSET        0x00011000u
#define CART_MAGIC              0x4E314341u     /* "N1CA" */
#define CART_VERSION            1u
#define CART_MAX_GAMES          32u

#endif /* __N1_LAYOUT_H */
