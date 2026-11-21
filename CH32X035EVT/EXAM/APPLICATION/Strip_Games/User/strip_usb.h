/********************************** (C) COPYRIGHT *******************************
 * File Name          : strip_usb.h
 * Description        : Host protocol for Strip_Games over USB CDC.
 *
 * Same framing as IAP_USB_SIGNED (A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8,
 * replies use CMD | 0x80), so iap_flash.py and wsp_cdc.py style tools work.
 *
 *   0x01 INFO        -> ver, leds, fps, game, brightness, rsv[3]
 *   0x02 PING        echoes the payload back
 *   0x10 SET_GAME    1 byte 0..2 (energy duel / bomb defuse / 1D runner)
 *   0x11 SET_BRIGHT  1 byte 0..255 (driver brightness, not the 25% game scale)
 *   0x20 UPGRADE     ACK, then write the flag page and reset into the bootloader
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __STRIP_USB_H
#define __STRIP_USB_H

#include <stdint.h>

#define STRIP_USB_CMD_INFO          0x01
#define STRIP_USB_CMD_PING          0x02
#define STRIP_USB_CMD_SET_GAME      0x10
#define STRIP_USB_CMD_SET_BRIGHT    0x11
#define STRIP_USB_CMD_UPGRADE       0x20

#define STRIP_USB_MAX_PAYLOAD       56u

void    strip_usb_init(void);

/* execute one pending command, returns 1 when it did something */
uint8_t strip_usb_service(void);

/* provided by main.c */
uint8_t     game_mode(void);
const char *game_name(uint8_t mode);
void        game_select(uint8_t mode);

#endif /* __STRIP_USB_H */
