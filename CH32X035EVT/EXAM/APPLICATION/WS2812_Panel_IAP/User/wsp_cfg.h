/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_cfg.h
 * Description        : Persistent settings block stored in the external NOR.
 *
 * A 16-byte record lives in the second-to-last 4KB sector of the NOR flash, so
 * the 1-wire bit rate, colour order, brightness and the default display mode
 * survive a power cycle without rebuilding the firmware.
 *
 *   offset 0  magic   "WCFG"                 uint32 LE
 *   offset 4  wire_cyc PIOC clocks per bit   (WS2812_CYC_MIN..MAX)
 *   offset 5  version 2
 *   offset 6  order   WS2812_ORDER_*
 *   offset 7  bright  0..255
 *   offset 8  mode    WSP_MODE_*
 *   offset 9  limit   電流上限，单位 100mA，0 = 不限
 *   offset 10 crc     crc8(poly 0x07) over bytes 0..9
 *   offset 11 pad[5]
 *
 * Any of magic / version / crc / range being wrong falls back to the compile
 * time defaults from wsp_config.h, so a blank or unreadable flash never bricks
 * the panel -- it simply behaves like a stock build.
 *
 * The last 4KB sector is reserved by nor_selftest(), hence "second to last".
 * Keep image data below (nor_size() - 8192).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_CFG_H
#define __WSP_CFG_H

#include "ch32x035.h"
#include "wsp_config.h"

#define WSP_CFG_MAGIC           0x47464357u     /* "WCFG" little endian */
#define WSP_CFG_VERSION         2

typedef struct {
    uint32_t magic;
    uint8_t  wire_cyc;      /* PIOC clocks per WS2812 bit      */
    uint8_t  version;
    uint8_t  order;         /* WS2812_ORDER_*                  */
    uint8_t  brightness;    /* 0..255                          */
    uint8_t  mode;          /* WSP_MODE_*                      */
    uint8_t  limit100;      /* current limit in 100mA, 0 = off */
    uint8_t  crc;           /* crc8 over bytes 0..9            */
    uint8_t  pad[5];
} wsp_cfg_t;                /* 16 bytes */

/* fill a record with the compile time defaults */
void             wsp_cfg_defaults(wsp_cfg_t *cfg);

/* read the record from NOR, or install the defaults when it is unusable */
void             wsp_cfg_load(void);

/* push order + brightness + bit rate into the driver; call after ws2812_init() */
void             wsp_cfg_apply(void);

const wsp_cfg_t *wsp_cfg(void);

/* erase + program + verify; 1 = stored, 0 = no flash / mismatch */
uint8_t          wsp_cfg_save(const wsp_cfg_t *cfg);

/* NOR address of the record, 0 when there is no usable flash */
uint32_t         wsp_cfg_addr(void);

#endif /* __WSP_CFG_H */
