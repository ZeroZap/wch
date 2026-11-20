/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_usb.c
 * Description        : USB frame/command protocol, see wsp_usb.h.
 *
 * The parser runs in the USB interrupt (one 64-byte OUT packet at a time) and
 * only accumulates into payload_buf[]; all drawing and flash access happen in
 * wsp_usb_service() from the main loop, so neither PIOC transfers nor a NOR
 * erase can stall the USB endpoint for long.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>
#include "usb_cdc.h"
#include "wsp_usb.h"
#include "panel.h"
#include "ws2812.h"
#include "wsp_tick.h"
#include "norflash.h"
#include "wsp_cfg.h"

#if WSP_OSD_ENABLE
#include "wsp_osd.h"
#endif

#define SYNC0                   0xA5u
#define SYNC1                   0x5Au

#define ST_SYNC0                0u
#define ST_SYNC1                1u
#define ST_CMD                  2u
#define ST_LEN_L                3u
#define ST_LEN_H                4u
#define ST_PAYLOAD              5u
#define ST_CRC                  6u

#define USB_ACTIVE_MS           500u

volatile wsp_usb_stat_t g_usb_stat;

/* parser state, written from the interrupt */
static volatile uint8_t  p_state;
static volatile uint8_t  p_cmd;
static volatile uint16_t p_len;
static volatile uint16_t p_idx;
static volatile uint8_t  p_crc;

static uint8_t payload_buf[WSP_USB_MAX_PAYLOAD];

static volatile uint8_t  cmd_ready;
static volatile uint8_t  cmd_code;
static volatile uint16_t cmd_len;

static volatile uint32_t last_frame_ms;
static volatile uint8_t  last_cmd_ok;

/*---------------------------------------------------------------------------
 * helpers
 *---------------------------------------------------------------------------*/
static uint8_t crc8_update(uint8_t crc, uint8_t byte)
{
    uint8_t i;

    crc ^= byte;
    for (i = 0; i < 8u; i++) {
        crc = (crc & 0x80u) ? (uint8_t)((crc << 1) ^ 0x07u) : (uint8_t)(crc << 1);
    }

    return crc;
}

static void reply(uint8_t code, const uint8_t *data, uint16_t len)
{
    uint8_t  frame[64];
    uint16_t n = 0;
    uint16_t i;
    uint8_t  crc = 0;

    if ((uint16_t)(6u + len) > sizeof(frame)) {
        return;
    }

    frame[n++] = SYNC0;
    frame[n++] = SYNC1;
    frame[n++] = (uint8_t)(code | WSP_USB_ACK);
    frame[n++] = (uint8_t)(len & 0xFFu);
    frame[n++] = (uint8_t)(len >> 8);

    crc = crc8_update(crc, frame[2]);
    crc = crc8_update(crc, frame[3]);
    crc = crc8_update(crc, frame[4]);

    for (i = 0; i < len; i++) {
        frame[n++] = data[i];
        crc = crc8_update(crc, data[i]);
    }

    frame[n++] = crc;
    (void)usb_cdc_write(frame, n);
}

static void reply_ok(uint8_t code)
{
    reply(code, 0, 0);
}

/*---------------------------------------------------------------------------
 * interrupt side: byte level parser
 *---------------------------------------------------------------------------*/
void wsp_usb_on_packet(const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    g_usb_stat.packets++;

    for (i = 0; i < len; i++) {
        uint8_t b = buf[i];

        switch (p_state) {
        case ST_SYNC0:
            if (b == SYNC0) {
                p_state = ST_SYNC1;
            }
            break;

        case ST_SYNC1:
            p_state = (b == SYNC1) ? ST_CMD : ST_SYNC0;
            break;

        case ST_CMD:
            p_cmd   = b;
            p_crc   = crc8_update(0, b);
            p_state = ST_LEN_L;
            break;

        case ST_LEN_L:
            p_len   = b;
            p_crc   = crc8_update(p_crc, b);
            p_state = ST_LEN_H;
            break;

        case ST_LEN_H:
            p_len  |= (uint16_t)((uint16_t)b << 8);
            p_crc   = crc8_update(p_crc, b);
            p_idx   = 0;
            if (p_len > WSP_USB_MAX_PAYLOAD) {
                /* impossible length: resynchronise, the next A5 5A restarts */
                g_usb_stat.len_errors++;
                p_state = ST_SYNC0;
            } else {
                p_state = (p_len == 0) ? ST_CRC : ST_PAYLOAD;
            }
            break;

        case ST_PAYLOAD:
            payload_buf[p_idx++] = b;
            p_crc = crc8_update(p_crc, b);
            if (p_idx >= p_len) {
                p_state = ST_CRC;
            }
            break;

        case ST_CRC:
            if (b == p_crc) {
                if (cmd_ready == 0) {
                    cmd_code  = p_cmd;
                    cmd_len   = p_len;
                    cmd_ready = 1;
                }
            } else {
                g_usb_stat.crc_errors++;
            }
            p_state = ST_SYNC0;
            break;

        default:
            p_state = ST_SYNC0;
            break;
        }
    }
}

/*---------------------------------------------------------------------------
 * main loop side
 *---------------------------------------------------------------------------*/
static void draw_565(const uint8_t *src, uint16_t pixels)
{
    uint16_t i;

    for (i = 0; i < pixels; i++) {
        uint16_t c = (uint16_t)(((uint16_t)src[0] << 8) | src[1]);

        panel_fb[(uint32_t)i * 3u + 0u] = (uint8_t)((c >> 11) & 0x1Fu) << 3;
        panel_fb[(uint32_t)i * 3u + 1u] = (uint8_t)((c >> 5) & 0x3Fu) << 2;
        panel_fb[(uint32_t)i * 3u + 2u] = (uint8_t)(c & 0x1Fu) << 3;
        src += 2;
    }
}

static uint8_t handle_frame_565(void)
{
    if (cmd_len != (uint16_t)(PANEL_W * PANEL_H * 2)) {
        g_usb_stat.len_errors++;
        return 0;
    }

    draw_565(payload_buf, (uint16_t)(PANEL_W * PANEL_H));
    panel_show();
    g_usb_stat.frames++;
    last_frame_ms = tick_ms();

    return 1;
}

static uint8_t handle_frame_888(void)
{
    if (cmd_len != (uint16_t)(PANEL_W * PANEL_H * 3)) {
        g_usb_stat.len_errors++;
        return 0;
    }

    memcpy(panel_fb, payload_buf, (uint32_t)PANEL_W * PANEL_H * 3u);
    panel_show();
    g_usb_stat.frames++;
    last_frame_ms = tick_ms();

    return 1;
}

static uint8_t handle_rect_565(void)
{
    uint16_t x, y, w, h, n, i;

    if (cmd_len < 4) {
        g_usb_stat.len_errors++;
        return 0;
    }

    x = payload_buf[0];
    y = payload_buf[1];
    w = payload_buf[2];
    h = payload_buf[3];
    n = (uint16_t)(w * h);

    if ((cmd_len != (uint16_t)(4u + n * 2u)) || (x >= PANEL_W) || (y >= PANEL_H) ||
        ((uint16_t)(x + w) > PANEL_W) || ((uint16_t)(y + h) > PANEL_H)) {
        g_usb_stat.len_errors++;
        return 0;
    }

    for (i = 0; i < n; i++) {
        uint16_t c = (uint16_t)(((uint16_t)payload_buf[4u + i * 2u] << 8) |
                                payload_buf[5u + i * 2u]);
        uint16_t px = (uint16_t)(x + (i % w));
        uint16_t py = (uint16_t)(y + (i / w));
        panel_set_pixel565(px, py, c);
    }

    panel_show();
    g_usb_stat.frames++;
    last_frame_ms = tick_ms();

    return 1;
}

static void handle_info(void)
{
    uint8_t  info[9];
    uint32_t size = nor_size();

    info[0] = 2;                            /* protocol version          */
    info[1] = PANEL_W;
    info[2] = PANEL_H;
    info[3] = ws2812_order();
    info[4] = ws2812_cycles();
    info[5] = (uint8_t)(size);
    info[6] = (uint8_t)(size >> 8);
    info[7] = (uint8_t)(size >> 16);
    info[8] = (uint8_t)(size >> 24);

    reply(WSP_USB_CMD_INFO, info, sizeof(info));
}

static void handle_nor(void)
{
    uint32_t addr;

    addr = (uint32_t)payload_buf[0]
         | ((uint32_t)payload_buf[1] << 8)
         | ((uint32_t)payload_buf[2] << 16)
         | ((uint32_t)payload_buf[3] << 24);

    switch (cmd_code) {
    case WSP_USB_CMD_NOR_ERASE:
        nor_erase_sector(addr);
        reply_ok(cmd_code);
        break;

    case WSP_USB_CMD_NOR_WRITE:
    {
        uint32_t n = (uint32_t)(cmd_len - 4u);

        if (n > 256u) {
            n = 256u;               /* one page per command */
        }
        nor_write(addr, &payload_buf[4], n);
        g_usb_stat.nor_writes++;
        reply_ok(cmd_code);
        break;
    }

    case WSP_USB_CMD_NOR_READ:
    {
        uint8_t  data[40];
        uint16_t n = (uint16_t)(payload_buf[4] | ((uint16_t)payload_buf[5] << 8));

        if (n > sizeof(data)) {
            n = sizeof(data);
        }
        nor_read(addr, data, n);
        reply(cmd_code, data, n);
        g_usb_stat.nor_reads++;
        break;
    }

    default:
        break;
    }
}

uint8_t wsp_usb_service(void)
{
    uint8_t  code;
    uint16_t len;
    uint8_t  shown = 0;

    if (cmd_ready == 0) {
        return 0;
    }

    code = cmd_code;
    len  = cmd_len;
    cmd_ready = 0;
    g_usb_stat.last_cmd = code;

    switch (code) {
    case WSP_USB_CMD_SET_BRIGHT:
        if (len >= 1) {
            ws2812_set_brightness(payload_buf[0]);
        }
        reply_ok(code);
        break;

    case WSP_USB_CMD_FRAME_565:
        shown = handle_frame_565();
        if (shown == 0) {
            reply_ok(code | 0x40);          /* rejected, see g_usb_stat */
        }
        break;

    case WSP_USB_CMD_FRAME_888:
        shown = handle_frame_888();
        if (shown == 0) {
            reply_ok(code | 0x40);
        }
        break;

    case WSP_USB_CMD_RECT_565:
        shown = handle_rect_565();
        if (shown == 0) {
            reply_ok(code | 0x40);
        }
        break;

    case WSP_USB_CMD_NOR_ERASE:
    case WSP_USB_CMD_NOR_WRITE:
    case WSP_USB_CMD_NOR_READ:
        if (len >= 4) {
            handle_nor();
        } else {
            g_usb_stat.len_errors++;
        }
        break;

    case WSP_USB_CMD_INFO:
        handle_info();
        break;

    case WSP_USB_CMD_OSD:
    {
#if WSP_OSD_ENABLE
        uint8_t state;

        if (len >= 1) {
            osd_set(payload_buf[0]);
        }
        state = osd_on();
        reply(code, &state, 1);
        panel_show();
        last_frame_ms = tick_ms();
        shown = 1;
#else
        reply_ok(code | 0x40);
#endif
        break;
    }

    case WSP_USB_CMD_SET_ORDER:
        if ((len >= 1) && (ws2812_set_order(payload_buf[0]) == 0)) {
            reply_ok(code);
            /* repaint so the change is visible immediately */
            panel_show();
            last_frame_ms = tick_ms();
            shown = 1;
        } else {
            g_usb_stat.len_errors++;
            reply_ok(code | 0x40);
        }
        break;

    case WSP_USB_CMD_SET_CYC:
        if (len >= 1) {
            uint8_t used = ws2812_set_cycles(payload_buf[0]);

            reply(code, &used, 1);
            panel_show();
            last_frame_ms = tick_ms();
            shown = 1;
        } else {
            g_usb_stat.len_errors++;
            reply_ok(code | 0x40);
        }
        break;

    case WSP_USB_CMD_CFG_GET:
    {
        uint8_t rec[sizeof(wsp_cfg_t)];

        memcpy(rec, (const void *)wsp_cfg(), sizeof(rec));
        reply(code, rec, (uint16_t)sizeof(rec));
        break;
    }

    case WSP_USB_CMD_CFG_SET:
    {
        wsp_cfg_t c;

        if (len < sizeof(c)) {
            g_usb_stat.len_errors++;
            reply_ok(code | 0x40);
            break;
        }

        memcpy(&c, payload_buf, sizeof(c));

        /* ranges are owned by the device, the host only proposes values */
        if ((c.wire_cyc < WS2812_CYC_MIN) || (c.wire_cyc > WS2812_CYC_MAX)) {
            c.wire_cyc = ws2812_cycles();
        }
        if (c.order > WS2812_ORDER_GBR) {
            c.order = ws2812_order();
        }
        if (c.mode > WSP_MODE_STRIP) {
            c.mode = WSP_MODE_PANEL;
        }

        if (wsp_cfg_save(&c)) {
            wsp_cfg_apply();
            reply_ok(code);
            panel_show();
            last_frame_ms = tick_ms();
            shown = 1;
        } else {
            reply_ok(code | 0x40);
        }
        break;
    }

    default:
        g_usb_stat.unknown_cmds++;
        reply_ok(code | 0x40);
        break;
    }

    last_cmd_ok = shown;
    return shown;
}

uint8_t wsp_usb_active(void)
{
    return (uint8_t)((tick_ms() - last_frame_ms) < USB_ACTIVE_MS);
}

void wsp_usb_init(void)
{
    p_state   = ST_SYNC0;
    cmd_ready = 0;
    last_frame_ms = tick_ms() - USB_ACTIVE_MS - 1u;

    usb_cdc_init();
    usb_cdc_log("# WS2812 Panel CDC ready");
}
