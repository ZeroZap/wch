/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_usb.h
 * Description        : USB frame/command protocol for WS2812_Panel (stage 5/6).
 *
 * Packet format (both directions):
 *
 *   A5 5A | CMD | LEN_L LEN_H | PAYLOAD... | CRC8
 *
 *   CRC8 = poly 0x07, init 0x00, over CMD..PAYLOAD (sync bytes excluded)
 *   Replies use CMD | 0x80 and are at most 64 bytes, i.e. one USB packet.
 *
 * Commands (host -> panel)
 *   0x01 SET_BRIGHT  1 byte, 0..255
 *   0x02 FRAME_565   W*H*2 bytes, RGB565, big endian per pixel
 *   0x03 FRAME_888   W*H*3 bytes, RGB
 *   0x04 RECT_565    x,y,w,h (4 bytes) + w*h*2
 *   0x05 NOR_ERASE   4 byte address, erases that 4KB sector
 *   0x06 NOR_WRITE   4 byte address + <=256 bytes, host keeps it page aligned
 *   0x07 NOR_READ    4 byte address + 2 byte length (<=40), reply carries data
 *   0x0A INFO        reply: ver, w, h, colour order, nor size (4 byte LE)
 *   0x0B SET_ORDER   1 byte, WS2812_ORDER_* (0..5), RAM only
 *   0x0C CFG_GET     reply: the 16 byte NOR settings record
 *   0x0D CFG_SET     16 byte record, validated and stored in NOR
 *   0x0E SET_CYC     1 byte, PIOC clocks per bit (48..72), RAM only
 *   0x0F OSD         no payload = query, 1 byte = on/off, reply is the state
 *   0x20 UPGRADE     no payload, replies then writes the UPGRADE flag and
 *                    resets into the bootloader (same frame format, so
 *                    tools/iap_flash.py --reboot-to-boot works)
 *
 * A frame that fails the CRC or has a bad length is dropped silently, so the
 * panel simply keeps showing the previous image.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_USB_H
#define __WSP_USB_H

#include "ch32x035.h"
#include "wsp_config.h"

#define WSP_USB_CMD_SET_BRIGHT      0x01
#define WSP_USB_CMD_FRAME_565       0x02
#define WSP_USB_CMD_FRAME_888       0x03
#define WSP_USB_CMD_RECT_565        0x04
#define WSP_USB_CMD_NOR_ERASE       0x05
#define WSP_USB_CMD_NOR_WRITE       0x06
#define WSP_USB_CMD_NOR_READ        0x07
#define WSP_USB_CMD_INFO            0x0A
#define WSP_USB_CMD_SET_ORDER       0x0B
#define WSP_USB_CMD_CFG_GET         0x0C
#define WSP_USB_CMD_CFG_SET         0x0D
#define WSP_USB_CMD_SET_CYC         0x0E
#define WSP_USB_CMD_OSD             0x0F
#define WSP_USB_CMD_UPGRADE         0x20

#define WSP_USB_ACK                 0x80

#if (PANEL_W * PANEL_H * 3) > (PANEL_W * PANEL_H * 2)
#define WSP_USB_MAX_PAYLOAD         (PANEL_W * PANEL_H * 3)
#else
#define WSP_USB_MAX_PAYLOAD         (PANEL_W * PANEL_H * 2)
#endif

/* counters, readable over WCH-Link for debugging */
typedef struct {
    uint32_t packets;       /* OUT packets received            */
    uint32_t frames;        /* accepted display frames         */
    uint32_t crc_errors;
    uint32_t len_errors;
    uint32_t unknown_cmds;
    uint32_t last_cmd;
    uint32_t nor_writes;
    uint32_t nor_reads;
} wsp_usb_stat_t;

extern volatile wsp_usb_stat_t g_usb_stat;

void     wsp_usb_init(void);

/* handle one pending request; returns 1 when it did something visible */
uint8_t  wsp_usb_service(void);

/* 1 while the host is streaming (a frame arrived within the last 500ms) */
uint8_t  wsp_usb_active(void);

#endif /* __WSP_USB_H */
