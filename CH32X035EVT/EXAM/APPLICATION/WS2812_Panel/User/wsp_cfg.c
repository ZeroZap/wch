/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_cfg.c
 * Description        : Persistent settings block in the external NOR,
 *                      see wsp_cfg.h for the record layout.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>
#include "wsp_cfg.h"
#include "norflash.h"
#include "ws2812.h"

#define CFG_SECTOR_SIZE         4096u
#define CFG_TRAILER             8192u   /* last 4KB belongs to nor_selftest() */
#define CFG_CRC_LEN             10u     /* magic .. limit100 */

static wsp_cfg_t cfg_ram;

/*---------------------------------------------------------------------------
 * helpers
 *---------------------------------------------------------------------------*/
static uint8_t cfg_crc(const wsp_cfg_t *cfg)
{
    const uint8_t *p = (const uint8_t *)cfg;
    uint8_t        crc = 0;
    uint8_t        i, b;

    for (i = 0; i < CFG_CRC_LEN; i++) {
        crc ^= p[i];
        for (b = 0; b < 8u; b++) {
            crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
        }
    }

    return crc;
}

static void cfg_seal(wsp_cfg_t *cfg)
{
    uint8_t i;

    cfg->magic   = WSP_CFG_MAGIC;
    cfg->version = WSP_CFG_VERSION;
    for (i = 0; i < sizeof(cfg->pad); i++) {
        cfg->pad[i] = 0;
    }
    cfg->crc = cfg_crc(cfg);
}

static uint8_t cfg_valid(const wsp_cfg_t *cfg)
{
    if (cfg->magic != WSP_CFG_MAGIC) {
        return 0;
    }
    if (cfg->version != WSP_CFG_VERSION) {
        return 0;
    }
    if (cfg->crc != cfg_crc(cfg)) {
        return 0;
    }
    if ((cfg->order > WS2812_ORDER_GBR) || (cfg->mode > WSP_MODE_STRIP)) {
        return 0;
    }
    if ((cfg->wire_cyc < WS2812_CYC_MIN) || (cfg->wire_cyc > WS2812_CYC_MAX)) {
        return 0;
    }

    return 1;
}

uint32_t wsp_cfg_addr(void)
{
    uint32_t size = nor_size();

    if (size < (CFG_TRAILER + CFG_SECTOR_SIZE)) {
        return 0;
    }

    return size - CFG_TRAILER;
}

/*---------------------------------------------------------------------------
 * public
 *---------------------------------------------------------------------------*/
void wsp_cfg_defaults(wsp_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->wire_cyc   = WS2812_WIRE_CYCLES;
    cfg->order      = WS2812_COLOR_ORDER;
    cfg->brightness = WS2812_BRIGHTNESS;
    cfg->mode       = WSP_DEFAULT_MODE;
    cfg->limit100   = (uint8_t)(WSP_CURRENT_LIMIT_MA / WSP_LIMIT_UNIT_MA);
    cfg_seal(cfg);
}

void wsp_cfg_load(void)
{
    uint32_t  addr = wsp_cfg_addr();
    wsp_cfg_t tmp;

    wsp_cfg_defaults(&cfg_ram);

    if (addr == 0) {
        return;                 /* no flash: keep the defaults */
    }

    nor_read(addr, &tmp, sizeof(tmp));

    if (cfg_valid(&tmp)) {
        cfg_ram = tmp;
    }
}

const wsp_cfg_t *wsp_cfg(void)
{
    return &cfg_ram;
}

void wsp_cfg_apply(void)
{
    (void)ws2812_set_order(cfg_ram.order);
    (void)ws2812_set_cycles(cfg_ram.wire_cyc);
    ws2812_set_brightness(cfg_ram.brightness);
    ws2812_set_limit_ma((uint16_t)((uint16_t)cfg_ram.limit100 * WSP_LIMIT_UNIT_MA));
}

uint8_t wsp_cfg_save(const wsp_cfg_t *cfg)
{
    wsp_cfg_t tmp = *cfg;
    wsp_cfg_t back;
    uint32_t  addr = wsp_cfg_addr();

    if (addr == 0) {
        return 0;
    }

    cfg_seal(&tmp);
    nor_write(addr, &tmp, sizeof(tmp));

    nor_read(addr, &back, sizeof(back));
    if (memcmp(&back, &tmp, sizeof(tmp)) != 0) {
        return 0;
    }

    cfg_ram = tmp;

    return 1;
}
