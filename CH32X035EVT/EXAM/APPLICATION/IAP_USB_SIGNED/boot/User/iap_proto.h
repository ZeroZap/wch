/********************************** (C) COPYRIGHT *******************************
 * File Name          : iap_proto.h
 * Description        : USB command protocol of the bootloader.
 *
 * Frame format, identical in both directions (and identical to the WS2812_Panel
 * protocol, so tools/wsp_cdc.py style code can be reused):
 *
 *   A5 5A | CMD | LEN_L LEN_H | PAYLOAD... | CRC8
 *
 *   CRC8 = poly 0x07, init 0x00, over CMD..PAYLOAD (sync bytes excluded).
 *   Replies carry CMD | 0x80 and fit in one 64 byte USB packet.
 *
 * The byte level parser runs in the USB interrupt and only accumulates into a
 * buffer; every command that touches flash is executed from the main loop via
 * iap_proto_service(), so a page erase never stalls the endpoint for long.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __IAP_PROTO_H
#define __IAP_PROTO_H

#include <stdint.h>
#include "img_hdr.h"

void    iap_proto_init(void);

/* Execute one pending command.  Returns 1 when a command was handled. */
uint8_t iap_proto_service(void);

/* 1 once the host asked to leave the bootloader (JUMP was accepted). */
uint8_t iap_proto_jump_requested(void);

#endif /* __IAP_PROTO_H */
