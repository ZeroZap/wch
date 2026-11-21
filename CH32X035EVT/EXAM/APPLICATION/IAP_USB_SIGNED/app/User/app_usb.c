/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_usb.c
 * Description        : See app_usb.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "app_usb.h"
#include "app_iap.h"
#include "usb_cdc.h"
#include "img_hdr.h"
#include "ch32x035.h"
#include "debug.h"

#define SYNC0               0xA5u
#define SYNC1               0x5Au

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

/* rx_buf is written by the USB interrupt only; cmd_buf is the stable copy the
 * main loop works on, so a frame that starts arriving mid-command cannot
 * corrupt the payload */
static uint8_t           rx_buf[APP_MAX_PAYLOAD];
static uint8_t           cmd_buf[APP_MAX_PAYLOAD];

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

static void send_frame(const uint8_t *frame, uint16_t n)
{
    uint16_t tries = 0;

    while (usb_cdc_write(frame, n) != 0) {
        if (++tries > REPLY_RETRY_MS) {
            return;
        }
        Delay_Ms(1);
    }
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
    frame[n++] = (uint8_t)(code | IAP_ACK);
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
    send_frame(frame, n);
}

/*---------------------------------------------------------------------------
 * interrupt side
 *---------------------------------------------------------------------------*/
void usb_cdc_on_packet(const uint8_t *buf, uint16_t len)
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
            p_idx   = 0;
            if (p_len > APP_MAX_PAYLOAD) {
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
            if (b == p_crc && cmd_ready == 0u) {
                uint16_t i;

                for (i = 0u; i < p_len; i++) {
                    cmd_buf[i] = rx_buf[i];
                }
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
    img_hdr_t hdr;
    img_flag_t flag;
    uint8_t  out[12];
    uint16_t n = 0;
    uint8_t  state = IMG_STATE_NONE;
    uint32_t img_len = 0u;

    if (app_iap_get_flag(&flag)) {
        state = (uint8_t)flag.state;
    }
    if (app_iap_get_hdr(&hdr)) {
        img_len = hdr.img_len;
    }

    out[n++] = 1u;                          /* protocol version */
    out[n++] = state;
    out[n++] = 0u;
    out[n++] = 0u;
    img_put_u32(&out[n], IMG_SLOT_SIZE); n += 4u;
    img_put_u32(&out[n], img_len);       n += 4u;

    reply(APP_CMD_INFO, out, n);
}

/*---------------------------------------------------------------------------
 * public API
 *---------------------------------------------------------------------------*/
void app_usb_init(void)
{
    p_state   = ST_SYNC0;
    p_cmd     = 0u;
    p_len     = 0u;
    p_idx     = 0u;
    p_crc     = 0u;
    cmd_ready = 0u;
}

uint8_t app_usb_service(void)
{
    uint8_t code;

    if (!cmd_ready) {
        return 0u;
    }

    code = cmd_code;
    cmd_ready = 0u;

    switch (code) {
    case APP_CMD_INFO:
        cmd_info();
        break;

    case APP_CMD_PING:
        reply(APP_CMD_PING, cmd_buf, cmd_len);
        break;

    case APP_CMD_UPGRADE:
        reply(APP_CMD_UPGRADE, cmd_buf, 0u);
        Delay_Ms(20);
        app_iap_request();              /* does not return */
        break;

    default:
        reply(code, 0u, 0u);
        break;
    }

    return 1u;
}
