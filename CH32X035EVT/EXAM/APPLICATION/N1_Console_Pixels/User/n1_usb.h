/********************************** (C) COPYRIGHT *******************************
 * File Name          : n1_usb.h
 * Description        : USB CDC command protocol for the N1 console.
 *
 * Framing is identical to IAP_USB_SIGNED and WS2812_Panel_IAP:
 *   A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8
 * replies use CMD | 0x80.
 *
 *   0x01 INFO        -> ver, w, h, count, current, state, bright, sound, nor, 0
 *   0x02 PING        echoes the payload
 *   0x10 SELECT_GAME 1 byte, 0..count-1 starts a game, 0xFF returns to the menu
 *   0x11 SET_BRIGHT  1 byte, 0..255
 *   0x12 SET_SOUND   1 byte, 0/1
 *   0x20 UPGRADE     ACK, then write the flag page and reset into the bootloader
 *   0x21 MENU        leave the running game and return to the launcher
 *   0x30 SCORES      -> count x uint16 little endian
 *   0x31 RESET_SCORES
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __N1_USB_H
#define __N1_USB_H

#include <stdint.h>
#include "console.h"

#define N1_USB_CMD_INFO         0x01u
#define N1_USB_CMD_PING         0x02u
#define N1_USB_CMD_SELECT_GAME  0x10u
#define N1_USB_CMD_SET_BRIGHT   0x11u
#define N1_USB_CMD_SET_SOUND    0x12u
#define N1_USB_CMD_UPGRADE      0x20u
#define N1_USB_CMD_MENU         0x21u
#define N1_USB_CMD_SCORES       0x30u
#define N1_USB_CMD_RESET_SCORES 0x31u

/* NOR cartridge management */
#define N1_USB_CMD_CART_INFO    0x40u
#define N1_USB_CMD_CART_BEGIN   0x41u
#define N1_USB_CMD_CART_DATA    0x42u
#define N1_USB_CMD_CART_END     0x43u
#define N1_USB_CMD_CART_ABORT   0x44u
#define N1_USB_CMD_CART_ERASE   0x45u
#define N1_USB_CMD_CART_LIST    0x46u
#define N1_USB_CMD_OLED_SCAN    0x47u
#define N1_USB_CMD_OLED_INIT    0x48u

#define N1_USB_MAX_PAYLOAD      56u

void    n1_usb_init(void);

/* Execute one pending command, returns 1 when it did something. */
uint8_t n1_usb_service(void);

#endif /* __N1_USB_H */
