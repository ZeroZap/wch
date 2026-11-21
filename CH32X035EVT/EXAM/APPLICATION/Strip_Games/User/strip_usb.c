/********************************** (C) COPYRIGHT *******************************
 * File Name          : strip_usb.c
 * Description        : See strip_usb.h.
 *
 * The byte parser runs in the USB interrupt and only accumulates into rx_buf;
 * commands are dispatched from the main loop so a slow action cannot stall the
 * endpoint, exactly like the bootloader's iap_proto.c.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "debug.h"
#include "strip_usb.h"
#include "game_config.h"
#include "usb_cdc.h"
#include "ws2812.h"
#include "app_iap.h"

#define SYNC0               0xA5u
#define SYNC1               0x5Au
#define STRIP_USB_ACK       0x80u

#define ST_SYNC0            0u
#define ST_SYNC1            1u
#define ST_CMD              2u
#define ST_LEN_L            3u
#define ST_LEN_H            4u
#define ST_PAYLOAD          5u
#define ST_CRC              6u

#define REPLY_RETRY_MS      100u

static volatile uint8_t  p_state;
static volatile uint8_t  p_cmd;
static volatile uint16_t p_len;
static volatile uint16_t p_idx;
static volatile uint8_t  p_crc;

static uint8_t           rx_buf[STRIP_USB_MAX_PAYLOAD];
static uint8_t           cmd_buf[STRIP_USB_MAX_PAYLOAD];

static volatile uint8_t  cmd_ready;
static volatile uint8_t  cmd_code;
static volatile uint16_t cmd_len;

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
    uint16_t tries = 0;

    if ((uint16_t)(6u + len) > sizeof(frame)) {
        return;
    }

    frame[n++] = SYNC0;
    frame[n++] = SYNC1;
    frame[n++] = (uint8_t)(code | STRIP_USB_ACK);
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

    while (usb_cdc_write(frame, n) != 0) {
        if (++tries > REPLY_RETRY_MS) {
            return;
        }
        Delay_Ms(1);
    }
}

/*---------------------------------------------------------------------------
 * interrupt side
 *---------------------------------------------------------------------------*/
void wsp_usb_on_packet(const uint8_t *buf, uint16_t len)
{
    uint16_t i;

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
            p_idx   = 0u;
            if (p_len > STRIP_USB_MAX_PAYLOAD) {
                p_state = ST_SYNC0;
            } else {
                p_state = (p_len == 0u) ? ST_CRC : ST_PAYLOAD;
            }
            break;

        case ST_PAYLOAD:
            rx_buf[p_idx++] = b;
            p_crc = crc8_update(p_crc, b);
            if (p_idx >= p_len) {
                p_state = ST_CRC;
            }
            break;

        case ST_CRC:
            if ((b == p_crc) && (cmd_ready == 0u)) {
                memcpy(cmd_buf, rx_buf, p_len);
                cmd_code  = p_cmd;
                cmd_len   = p_len;
                cmd_ready = 1u;
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
 * commands
 *---------------------------------------------------------------------------*/
static void cmd_info(void)
{
    uint8_t out[8];

    out[0] = 1u;                          /* protocol version */
    out[1] = (uint8_t)GAME_LEDS;
    out[2] = (uint8_t)GAME_FPS;
    out[3] = game_mode();
    out[4] = ws2812_brightness();
    out[5] = 0u;
    out[6] = 0u;
    out[7] = 0u;

    reply(STRIP_USB_CMD_INFO, out, sizeof(out));
}

/*---------------------------------------------------------------------------
 * public API
 *---------------------------------------------------------------------------*/
void strip_usb_init(void)
{
    p_state   = ST_SYNC0;
    p_cmd     = 0u;
    p_len     = 0u;
    p_idx     = 0u;
    p_crc     = 0u;
    cmd_ready = 0u;

    usb_cdc_init();
}

uint8_t strip_usb_service(void)
{
    uint8_t code;

    if (!cmd_ready) {
        return 0u;
    }

    code = cmd_code;
    cmd_ready = 0u;

    switch (code) {
    case STRIP_USB_CMD_INFO:
        cmd_info();
        break;

    case STRIP_USB_CMD_PING:
        reply(code, cmd_buf, cmd_len);
        break;

    case STRIP_USB_CMD_SET_GAME:
        if (cmd_len >= 1u) {
            game_select(cmd_buf[0]);
        }
        reply(code, 0, 0);
        break;

    case STRIP_USB_CMD_SET_BRIGHT:
        if (cmd_len >= 1u) {
            ws2812_set_brightness(cmd_buf[0]);
        }
        reply(code, 0, 0);
        break;

    case STRIP_USB_CMD_UPGRADE:
        reply(code, 0, 0);
        Delay_Ms(20);
        app_iap_request();              /* does not return */
        break;

    default:
        reply(code, 0, 0);
        break;
    }

    return 1u;
}
