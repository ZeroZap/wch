/********************************** (C) COPYRIGHT *******************************
 * File Name          : n1_usb.c
 * Description        : See n1_usb.h.
 *
 * The byte parser runs in the USB interrupt and only accumulates into rx_buf;
 * commands are dispatched from the main loop so a slow NOR write or a reset
 * cannot stall the endpoint.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "debug.h"
#include "n1_usb.h"
#include "usb_cdc.h"
#include "app_iap.h"
#include "console_config.h"
#include "cart.h"
#include "nor_store.h"
#include "n1_layout.h"
#include "oled.h"

#define SYNC0               0xA5u
#define SYNC1               0x5Au
#define N1_USB_ACK          0x80u

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

static uint8_t           rx_buf[N1_USB_MAX_PAYLOAD];
static uint8_t           cmd_buf[N1_USB_MAX_PAYLOAD];

static volatile uint8_t  cmd_ready;
static volatile uint8_t  cmd_code;
static volatile uint16_t cmd_len;

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
    frame[n++] = (uint8_t)(code | N1_USB_ACK);
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
            if (p_len > N1_USB_MAX_PAYLOAD) {
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

void n1_usb_init(void)
{
    p_state   = ST_SYNC0;
    p_cmd     = 0u;
    p_len     = 0u;
    p_idx     = 0u;
    p_crc     = 0u;
    cmd_ready = 0u;

    usb_cdc_init();
}

uint8_t n1_usb_service(void)
{
    uint8_t code;

    if (!cmd_ready) {
        return 0u;
    }

    code = cmd_code;
    cmd_ready = 0u;

    switch (code) {
    case N1_USB_CMD_INFO: {
        uint8_t out[11];
        console_fill_info(out);
        reply(code, out, sizeof(out));
        break;
    }

    case N1_USB_CMD_PING:
        reply(code, cmd_buf, cmd_len);
        break;

    case N1_USB_CMD_SELECT_GAME:
        if (cmd_len >= 1u) {
            if (cmd_buf[0] == 0xFFu) {
                console_return_menu();
                reply(code, 0, 0);
            } else {
                uint8_t st = console_load_game(cmd_buf[0]);
                reply(code, &st, 1u);
                if (st == CART_OK) {
                    Delay_Ms(20);
                    console_jump_game();    /* does not return */
                }
            }
        } else {
            reply(code, 0, 0);
        }
        break;

    case N1_USB_CMD_SET_BRIGHT:
        if (cmd_len >= 1u) {
            console_set_brightness(cmd_buf[0]);
        }
        reply(code, 0, 0);
        break;

    case N1_USB_CMD_SET_SOUND:
        if (cmd_len >= 1u) {
            console_set_sound(cmd_buf[0]);
        }
        reply(code, 0, 0);
        break;

    case N1_USB_CMD_UPGRADE:
        reply(code, 0, 0);
        Delay_Ms(20);
        app_iap_request();              /* does not return */
        break;

    case N1_USB_CMD_MENU:
        console_return_menu();
        reply(code, 0, 0);
        break;

    case N1_USB_CMD_SCORES: {
        uint8_t out[CONSOLE_MAX_GAMES * 2u];
        uint8_t n = console_game_count();

        if (n > CONSOLE_MAX_GAMES) {
            n = CONSOLE_MAX_GAMES;
        }
        console_fill_scores(out);
        reply(code, out, (uint16_t)(n * 2u));
        break;
    }

    case N1_USB_CMD_RESET_SCORES:
        console_reset_scores();
        reply(code, 0, 0);
        break;

    case N1_USB_CMD_OLED_SCAN: {
        uint8_t out[16];
        uint8_t n = oled_scan(&out[1], 15u);
        out[0] = n;
        reply(code, out, (uint16_t)(n + 1u));
        break;
    }

    case N1_USB_CMD_OLED_INIT: {
        uint8_t st = console_oled_reinit();
        reply(code, &st, 1u);
        break;
    }

    case N1_USB_CMD_CART_INFO: {
        uint8_t  out[12];
        uint32_t total = cart_total_size();
        uint32_t jedec = store_jedec_id();

        out[0] = cart_valid();
        out[1] = cart_count();
        out[2] = (uint8_t)(total & 0xFFu);
        out[3] = (uint8_t)((total >> 8) & 0xFFu);
        out[4] = (uint8_t)((total >> 16) & 0xFFu);
        out[5] = (uint8_t)((total >> 24) & 0xFFu);
        out[6] = (uint8_t)(jedec & 0xFFu);
        out[7] = (uint8_t)((jedec >> 8) & 0xFFu);
        out[8] = (uint8_t)((jedec >> 16) & 0xFFu);
        out[9] = (uint8_t)((jedec >> 24) & 0xFFu);
        out[10] = (uint8_t)(N1_GAME_SIZE & 0xFFu);
        out[11] = (uint8_t)((N1_GAME_SIZE >> 8) & 0xFFu);
        reply(code, out, sizeof(out));
        break;
    }

    case N1_USB_CMD_CART_BEGIN: {
        uint8_t st;

        if (cmd_len != 8u) {
            st = CART_ERR_PARAM;
        } else {
            uint32_t total = (uint32_t)cmd_buf[0] |
                             ((uint32_t)cmd_buf[1] << 8) |
                             ((uint32_t)cmd_buf[2] << 16) |
                             ((uint32_t)cmd_buf[3] << 24);
            uint32_t crc   = (uint32_t)cmd_buf[4] |
                             ((uint32_t)cmd_buf[5] << 8) |
                             ((uint32_t)cmd_buf[6] << 16) |
                             ((uint32_t)cmd_buf[7] << 24);
            st = cart_upload_begin(total, crc);
        }
        reply(code, &st, 1u);
        break;
    }

    case N1_USB_CMD_CART_DATA: {
        uint8_t st = cart_upload_data(cmd_buf, cmd_len);
        reply(code, &st, 1u);
        break;
    }

    case N1_USB_CMD_CART_END: {
        uint8_t st = cart_upload_end();
        if (st == CART_OK) {
            console_cart_updated();
        }
        reply(code, &st, 1u);
        break;
    }

    case N1_USB_CMD_CART_ABORT:
        cart_upload_abort();
        reply(code, 0, 0);
        break;

    case N1_USB_CMD_CART_ERASE: {
        uint8_t st = cart_erase();
        console_cart_updated();
        reply(code, &st, 1u);
        break;
    }

    case N1_USB_CMD_CART_LIST: {
        const cart_entry_t *e = 0;

        if (cmd_len >= 1u) {
            e = cart_entry(cmd_buf[0]);
        }

        if (!e) {
            reply(code, 0, 0);
        } else {
            uint8_t out[26];

            memcpy(out, e->name, 12);
            memcpy(out + 12, e->code, 4);
            out[16] = e->theme;
            out[17] = e->flags;
            out[18] = (uint8_t)(e->size & 0xFFu);
            out[19] = (uint8_t)((e->size >> 8) & 0xFFu);
            out[20] = (uint8_t)((e->size >> 16) & 0xFFu);
            out[21] = (uint8_t)((e->size >> 24) & 0xFFu);
            out[22] = (uint8_t)(e->crc32 & 0xFFu);
            out[23] = (uint8_t)((e->crc32 >> 8) & 0xFFu);
            out[24] = (uint8_t)((e->crc32 >> 16) & 0xFFu);
            out[25] = (uint8_t)((e->crc32 >> 24) & 0xFFu);
            reply(code, out, sizeof(out));
        }
        break;
    }

    default:
        reply(code, 0, 0);
        break;
    }

    return 1u;
}
