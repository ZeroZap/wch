/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_desc.h
 * Description        : USB CDC (virtual COM port) descriptors for the bootloader.
 *
 * Copied from EXAM/APPLICATION/WS2812_Panel, with a distinct PID so the host
 * tool can tell the bootloader port from the application port.
 *
 *   EP1 IN  (0x81)  interrupt, 8 bytes   - CDC notifications only
 *   EP2 OUT (0x02)  bulk,     64 bytes   - host -> bootloader (commands)
 *   EP3 IN  (0x83)  bulk,     64 bytes   - bootloader -> host (replies/logs)
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __USB_DESC_H
#define __USB_DESC_H

#include "ch32x035.h"
#include "ch32x035_usb.h"
#include "boot_info.h"

/* WCH VID with our own PIDs; Windows binds the inbox usbser.sys driver because
 * bDeviceClass is 0x02.  The bootloader PID comes from boot_info.h so each
 * product can be told apart, the app uses 0xFE0E. */
#define DEF_USB_VID                 0x1A86
#define DEF_USB_PID                 BOOT_USB_PID
#define DEF_IC_PRG_VER              0x01

#define DEF_UEP0_SIZE               64
#define DEF_UEP1_SIZE               8       /* CDC notification, interrupt IN */
#define DEF_UEP2_SIZE               64      /* bulk OUT                      */
#define DEF_UEP3_SIZE               64      /* bulk IN                       */

#define DEF_DEV_DESC_LEN            ((uint16_t)MyDevDescr[0])
#define DEF_CFG_DESC_LEN            ((uint16_t)MyCfgDescr[2] + ((uint16_t)MyCfgDescr[3] << 8))
#define DEF_LANG_DESC_LEN           ((uint16_t)MyLangDescr[0])
#define DEF_MANU_DESC_LEN           ((uint16_t)MyManuInfo[0])
#define DEF_PROD_DESC_LEN           ((uint16_t)MyProdInfo[0])
#define DEF_SN_DESC_LEN             ((uint16_t)MySerNumInfo[0])

extern const uint8_t MyDevDescr[];
extern const uint8_t MyCfgDescr[];
extern const uint8_t MyLangDescr[];
extern const uint8_t MyManuInfo[];
extern const uint8_t MyProdInfo[];
extern const uint8_t MySerNumInfo[];

#endif /* __USB_DESC_H */
