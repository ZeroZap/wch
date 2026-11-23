/********************************** (C) COPYRIGHT *******************************
 * File Name          : cart.c
 * Description        : See cart.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "cart.h"
#include "nor_store.h"
#include "game_flash.h"
#include "panel_gfx.h"
#include "board_config.h"
#include "ch32x035.h"
#include "ch32x035_usb.h"
#include "debug.h"

#if N1_GAME_OFFSET != 0x00007800u
#error "cart.c SW_Handler hardcodes 0x7800, update it with n1_layout.h"
#endif

static cart_hdr_t   g_hdr;
static cart_entry_t g_entries[CART_MAX_GAMES];
static uint8_t      g_valid;
static uint8_t      g_count;

/* upload state */
static uint8_t  up_active;
static uint32_t up_total;
static uint32_t up_expected;
static uint32_t up_crc;
static uint32_t up_received;
static uint32_t up_written;
static uint8_t  up_page[256];
static uint16_t up_fill;

static uint32_t crc32_update(uint32_t crc, const uint8_t *p, uint32_t n)
{
    uint32_t i;
    uint8_t  k;

    for (i = 0; i < n; i++) {
        crc ^= p[i];
        for (k = 0; k < 8u; k++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }

    return crc;
}

/*---------------------------------------------------------------------------
 * catalog
 *---------------------------------------------------------------------------*/
uint8_t cart_scan(void)
{
    uint8_t i;

    g_valid = 0u;
    g_count = 0u;

    if (!store_present()) {
        return 0u;
    }

    nor_raw_read(CART_BASE_OFFSET, &g_hdr, sizeof(g_hdr));

    if (g_hdr.magic != CART_MAGIC || g_hdr.version != CART_VERSION) {
        return 0u;
    }
    if (g_hdr.count == 0u || g_hdr.count > CART_MAX_GAMES) {
        return 0u;
    }
    if (g_hdr.total_size < CART_CAT_SIZE ||
        g_hdr.total_size > (store_size() - CART_BASE_OFFSET)) {
        return 0u;
    }

    nor_raw_read(CART_BASE_OFFSET + sizeof(cart_hdr_t),
                 g_entries, (uint32_t)g_hdr.count * sizeof(cart_entry_t));

    for (i = 0; i < g_hdr.count; i++) {
        cart_entry_t *e = &g_entries[i];

        e->name[11] = '\0';
        e->code[3]  = '\0';

        if (e->size == 0u || e->size > N1_GAME_SIZE) {
            return 0u;
        }
        if (e->offset < CART_DATA_OFFSET) {
            return 0u;
        }
        if ((e->offset + e->size) > (CART_BASE_OFFSET + g_hdr.total_size)) {
            return 0u;
        }
    }

    g_count = (uint8_t)g_hdr.count;
    g_valid = 1u;

    return 1u;
}

uint8_t cart_valid(void)
{
    return g_valid;
}

uint8_t cart_count(void)
{
    return g_count;
}

const cart_entry_t *cart_entry(uint8_t index)
{
    return (index < g_count) ? &g_entries[index] : 0;
}

uint32_t cart_total_size(void)
{
    return g_valid ? g_hdr.total_size : 0u;
}

uint32_t cart_crc32(void)
{
    return g_valid ? g_hdr.crc32 : 0u;
}

void cart_draw_icon(const cart_entry_t *e)
{
    uint8_t row;

    if (!e) {
        return;
    }

    for (row = 0; row < 10u; row++) {
        uint16_t bits = (uint16_t)e->icon[row * 2u] |
                        ((uint16_t)e->icon[row * 2u + 1u] << 8);
        uint8_t col;

        for (col = 0; col < 16u; col++) {
            if (bits & (uint16_t)(1u << col)) {
                panel_pixel(col, row, P_WHITE);
            }
        }
    }
}

/*---------------------------------------------------------------------------
 * game load + jump
 *---------------------------------------------------------------------------*/
uint8_t cart_load(uint8_t index)
{
    const cart_entry_t *e = cart_entry(index);
    uint8_t  page[256];
    uint32_t off;
    uint32_t crc;
    uint32_t end;

    if (!g_valid || !e) {
        return CART_ERR_NOCART;
    }
    if (e->size == 0u || e->size > N1_GAME_SIZE) {
        return CART_ERR_PARAM;
    }

    /* erase only the sectors the new game needs; launcher stays untouched */
    end = N1_GAME_BASE + ((e->size + 1023u) & ~1023u);
    for (off = N1_GAME_BASE; off < end; off += 1024u) {
        if (game_flash_erase_sector(off) != 0) {
            return CART_ERR_FLASH;
        }
    }

    crc = 0xFFFFFFFFu;

    for (off = 0u; off < e->size; off += 256u) {
        uint32_t n = e->size - off;

        if (n > 256u) {
            n = 256u;
        }

        nor_raw_read(e->offset + off, page, n);
        crc = crc32_update(crc, page, n);

        if (n < 256u) {
            memset(page + n, 0xFF, 256u - n);
        }

        if (game_flash_write_page(N1_GAME_BASE + off, page) != 0) {
            return CART_ERR_FLASH;
        }
    }

    crc ^= 0xFFFFFFFFu;
    if (crc != e->crc32) {
        return CART_ERR_CRC;
    }

    /* read the slot back, so a marginal flash program cannot run */
    crc = 0xFFFFFFFFu;
    for (off = 0u; off < e->size; off += 256u) {
        uint32_t n = e->size - off;

        if (n > 256u) {
            n = 256u;
        }

        game_flash_read(N1_GAME_BASE + off, page, n);
        crc = crc32_update(crc, page, n);
    }

    crc ^= 0xFFFFFFFFu;

    return (crc == e->crc32) ? CART_OK : CART_ERR_CRC;
}

/* The launcher's SW interrupt handler.  Jumping from a normal function leaves
 * the core in a state where _start's "configure pipelining" write traps, so the
 * hand-over is issued from inside the software interrupt, exactly like the boot. */
__attribute__((interrupt("WCH-Interrupt-fast"), used, externally_visible))
void SW_Handler(void)
{
    __asm volatile("li a6, 0x7800");
    __asm volatile("jr a6");

    while (1) {
    }
}

static void usb_deinit(void)
{
    NVIC_DisableIRQ(USBFS_IRQn);
    NVIC_ClearPendingIRQ(USBFS_IRQn);

    USBFSD->BASE_CTRL = 0x00;
    USBFSD->UDEV_CTRL = 0x00;
    USBFSD->INT_EN    = 0x00;
    USBFSD->INT_FG    = 0xFF;

    AFIO->CTLR &= ~0x00000080u;
}

void cart_jump(void)
{
    __disable_irq();

    usb_deinit();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_DeInit(GPIOA);
    GPIO_DeInit(GPIOB);
    GPIO_DeInit(GPIOC);
    GPIO_AFIODeInit();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, DISABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBFS, DISABLE);

    Delay_Ms(20);

    NVIC_EnableIRQ(Software_IRQn);
    NVIC_SetPendingIRQ(Software_IRQn);
    __enable_irq();

    while (1) {
    }
}

/*---------------------------------------------------------------------------
 * USB uploader
 *---------------------------------------------------------------------------*/
void cart_upload_abort(void)
{
    up_active = 0u;
    up_fill   = 0u;
}

uint8_t cart_upload_active(void)
{
    return up_active;
}

uint32_t cart_upload_received(void)
{
    return up_received;
}

static uint8_t upload_flush_page(void)
{
    if (up_fill == 0u) {
        return CART_OK;
    }

    nor_raw_write_page(CART_BASE_OFFSET + up_written, up_page, up_fill);
    up_written += up_fill;
    up_fill = 0u;

    return CART_OK;
}

uint8_t cart_upload_begin(uint32_t total_size, uint32_t crc32)
{
    uint32_t addr;
    uint32_t end;

    cart_upload_abort();

    if (!store_present()) {
        return CART_ERR_NOR;
    }
    if (total_size < CART_CAT_SIZE ||
        total_size > (store_size() - CART_BASE_OFFSET)) {
        return CART_ERR_PARAM;
    }

    end = CART_BASE_OFFSET + ((total_size + 4095u) & ~4095u);
    for (addr = CART_BASE_OFFSET; addr < end; addr += 4096u) {
        nor_raw_erase_sector(addr);
    }

    up_total    = total_size;
    up_expected = crc32;
    up_crc      = 0xFFFFFFFFu;
    up_received = 0u;
    up_written  = 0u;
    up_fill     = 0u;
    up_active   = 1u;

    return CART_OK;
}

uint8_t cart_upload_data(const uint8_t *data, uint16_t len)
{
    if (!up_active || len == 0u) {
        return CART_ERR_PARAM;
    }
    if ((up_received + len) > up_total) {
        cart_upload_abort();
        return CART_ERR_PARAM;
    }

    up_crc = crc32_update(up_crc, data, len);
    up_received += len;

    while (len) {
        uint16_t space = (uint16_t)(256u - up_fill);
        uint16_t take  = (len < space) ? len : space;

        memcpy(up_page + up_fill, data, take);
        up_fill = (uint16_t)(up_fill + take);
        data += take;
        len  = (uint16_t)(len - take);

        if (up_fill == 256u) {
            (void)upload_flush_page();
        }
    }

    return CART_OK;
}

uint8_t cart_upload_end(void)
{
    if (!up_active) {
        return CART_ERR_PARAM;
    }

    (void)upload_flush_page();

    up_active = 0u;

    if ((up_crc ^ 0xFFFFFFFFu) != up_expected) {
        return CART_ERR_CRC;
    }

    return cart_scan() ? CART_OK : CART_ERR_CRC;
}

uint8_t cart_erase(void)
{
    uint32_t end;
    uint32_t addr;

    if (g_valid && g_hdr.total_size >= CART_CAT_SIZE) {
        end = CART_BASE_OFFSET + ((g_hdr.total_size + 4095u) & ~4095u);
    } else {
        end = CART_BASE_OFFSET + CART_CAT_SIZE;
    }

    if (!store_present()) {
        return CART_ERR_NOR;
    }

    for (addr = CART_BASE_OFFSET; addr < end; addr += 4096u) {
        nor_raw_erase_sector(addr);
    }

    g_valid = 0u;
    g_count = 0u;
    memset(&g_hdr, 0, sizeof(g_hdr));

    return CART_OK;
}
