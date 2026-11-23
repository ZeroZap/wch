/********************************** (C) COPYRIGHT *******************************
 * File Name          : iap_proto.c
 * Description        : See iap_proto.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "iap_proto.h"
#include "img_install.h"
#include "img_flash.h"
#include "boot_info.h"
#include "usb_cdc.h"
#include "ch32x035.h"
#include "debug.h"

#define SYNC0                   0xA5u
#define SYNC1                   0x5Au

#define ST_SYNC0                0u
#define ST_SYNC1                1u
#define ST_CMD                  2u
#define ST_LEN_L                3u
#define ST_LEN_H                4u
#define ST_PAYLOAD              5u
#define ST_CRC                  6u

#define REPLY_FRAME_MAX         64u
#define REPLY_RETRY_MS          100u

/* parser state, written from the interrupt */
static volatile uint8_t  p_state;
static volatile uint8_t  p_cmd;
static volatile uint16_t p_len;
static volatile uint16_t p_idx;
static volatile uint8_t  p_crc;

/* rx_buf is written by the USB interrupt only; cmd_buf is the stable copy the
 * main loop works on, so a frame that starts arriving mid-command cannot
 * corrupt the payload that is being written to flash */
static uint8_t           rx_buf[IAP_MAX_PAYLOAD];
static uint8_t           cmd_buf[IAP_MAX_PAYLOAD];

static volatile uint8_t  cmd_ready;
static volatile uint8_t  cmd_code;
static volatile uint16_t cmd_len;

static volatile uint8_t  jump_req;

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

/* usb_cdc_write() is single buffered, so retry for a little while instead of
 * silently dropping a reply the host is waiting for. */
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
    uint8_t  frame[REPLY_FRAME_MAX];
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

static void reply_status(uint8_t code, uint8_t status)
{
    reply(code, &status, 1u);
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
            if (p_len > IAP_MAX_PAYLOAD) {
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
 * command handlers, main loop side
 *---------------------------------------------------------------------------*/
static void cmd_info(void)
{
    img_flag_t flag;
    uint8_t    state = IMG_STATE_NONE;
    uint8_t    out[28];
    uint16_t   n = 0;

    if (img_load_flag(&flag)) {
        state = (uint8_t)flag.state;
    }

    out[n++] = (uint8_t)IAP_PROTO_VER;
    out[n++] = (uint8_t)IAP_MAX_PAYLOAD;
    out[n++] = state;
    out[n++] = (uint8_t)BOOT_VERSION_MAJOR;
    img_put_u32(&out[n], (uint32_t)BOOT_PRODUCT_ID); n += 4u;
    out[n++] = (uint8_t)(BOOT_VERSION_MINOR & 0xFFu);
    out[n++] = 0u;
    out[n++] = (uint8_t)((BOOT_USB_PID >> 8) & 0xFFu);
    out[n++] = (uint8_t)(BOOT_USB_PID & 0xFFu);
    img_put_u32(&out[n], IMG_APP_EXEC_ADDR); n += 4u;
    img_put_u32(&out[n], IMG_SLOT_SIZE);     n += 4u;
    img_put_u32(&out[n], IMG_HDR_OFFSET);    n += 4u;
    img_put_u32(&out[n], IMG_FLAG_OFFSET);   n += 4u;

    reply(IAP_CMD_INFO, out, n);
}

static void cmd_begin(void)
{
    uint32_t img_len;
    uint16_t flags;
    uint32_t load_addr;
    uint8_t  iv[16];
    uint8_t  st;

    /* img_len u32 | flags u16 | load_addr u32 | iv[16] */
    if (cmd_len != 26u) {
        reply_status(IAP_CMD_BEGIN, IAP_ST_LEN);
        return;
    }

    img_len   = img_get_u32(&cmd_buf[0]);
    flags     = (uint16_t)cmd_buf[4] | (uint16_t)((uint16_t)cmd_buf[5] << 8);
    load_addr = img_get_u32(&cmd_buf[6]);
    memcpy(iv, &cmd_buf[10], 16);

    st = img_install_begin(img_len, flags, load_addr, iv);
    reply_status(IAP_CMD_BEGIN, st);
}

static void cmd_data(void)
{
    uint8_t st;

    if (cmd_len == 0u) {
        reply_status(IAP_CMD_DATA, IAP_ST_LEN);
        return;
    }

    st = img_install_data(cmd_buf, cmd_len);
    reply_status(IAP_CMD_DATA, st);
}

static void cmd_end(void)
{
    uint8_t st;

    if (cmd_len != 32u) {
        reply_status(IAP_CMD_END, IAP_ST_LEN);
        return;
    }

    st = img_install_finish(cmd_buf);
    reply_status(IAP_CMD_END, st);
}

static void cmd_erase(void)
{
    uint8_t   st = IAP_ST_OK;
    img_hdr_t hdr;
    uint32_t  len = IMG_SLOT_SIZE;

    img_install_abort();

    /* Products such as N1_Console put a game slot after the signed launcher.
     * Erase only the installed launcher length when a valid header exists, so
     * ERASE does not wipe the cartridge/game area; a corrupt/absent header
     * falls back to the old full-slot erase for rescue. */
    if (img_load_hdr(&hdr)) {
        len = hdr.img_len;
    }

    img_flash_unlock();
    if (img_flash_erase_image(len) != 0) {
        st = IAP_ST_FLASH;
    }
    img_flash_lock();

    reply_status(IAP_CMD_ERASE, st);
}

static void cmd_read(void)
{
    uint32_t addr;
    uint16_t len;

    if (cmd_len != 6u) {
        reply_status(IAP_CMD_READ, IAP_ST_LEN);
        return;
    }

    addr = img_get_u32(&cmd_buf[0]);
    len  = (uint16_t)cmd_buf[4] | (uint16_t)((uint16_t)cmd_buf[5] << 8);

    if (len > IAP_MAX_PAYLOAD) {
        reply_status(IAP_CMD_READ, IAP_ST_LEN);
        return;
    }
    if (addr < IMG_FLASH_ALIAS || (addr + len) > (IMG_FLASH_ALIAS + IMG_FLASH_END)) {
        reply_status(IAP_CMD_READ, IAP_ST_LEN);
        return;
    }

    reply(IAP_CMD_READ, (const uint8_t *)addr, len);
}

static void cmd_jump(void)
{
    uint8_t st = img_verify_stored();

    if (st != IAP_ST_OK) {
        reply_status(IAP_CMD_JUMP, st);
        return;
    }

    reply_status(IAP_CMD_JUMP, IAP_ST_OK);
    jump_req = 1u;
}

/*---------------------------------------------------------------------------
 * public API
 *---------------------------------------------------------------------------*/
void iap_proto_init(void)
{
    p_state   = ST_SYNC0;
    p_cmd     = 0u;
    p_len     = 0u;
    p_idx     = 0u;
    p_crc     = 0u;
    cmd_ready = 0u;
    jump_req  = 0u;
}

uint8_t iap_proto_service(void)
{
    uint8_t code;

    if (!cmd_ready) {
        return 0u;
    }

    code = cmd_code;

    switch (code) {
    case IAP_CMD_INFO:  cmd_info();  break;
    case IAP_CMD_BEGIN: cmd_begin(); break;
    case IAP_CMD_DATA:  cmd_data();  break;
    case IAP_CMD_END:   cmd_end();   break;
    case IAP_CMD_ABORT:
        img_install_abort();
        reply_status(IAP_CMD_ABORT, IAP_ST_OK);
        break;
    case IAP_CMD_JUMP:  cmd_jump();  break;
    case IAP_CMD_ERASE: cmd_erase(); break;
    case IAP_CMD_READ:  cmd_read();  break;
    default:
        reply_status(code, IAP_ST_LEN);
        break;
    }

    cmd_ready = 0u;

    return 1u;
}

uint8_t iap_proto_jump_requested(void)
{
    return jump_req;
}
