/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_usb.h
 * Description        : Host protocol of the demo application.
 *
 * Same framing as the bootloader (shared/img_hdr.h):
 *
 *   A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8
 *
 * Commands
 *   0x01 INFO      no payload -> ver u8, state u8, rsv u16, slot_size u32,
 *                                img_len u32
 *   0x02 PING      echoes the payload back
 *   0x20 UPGRADE   reply, then write the UPGRADE flag and reset into the
 *                  bootloader
 *
 * Replace app_usb.c with the real application protocol; the file exists to
 * prove the app side of the framework and to give the host tool something to
 * talk to.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __APP_USB_H
#define __APP_USB_H

#include <stdint.h>

#define APP_CMD_INFO        0x01
#define APP_CMD_PING        0x02
#define APP_CMD_UPGRADE     0x20

#define APP_MAX_PAYLOAD     56u

void    app_usb_init(void);

/* Execute one pending command.  Returns 1 when a command was handled. */
uint8_t app_usb_service(void);

#endif /* __APP_USB_H */
