/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Description        : N1_Console_Pixels launcher: WS2812 cartridge menu + loader.
 *
 * This is the pure-WS2812B product.  The 16x16 serpentine panel is the only
 * display: the menu icon, scroll thumb and all game pixels are drawn there.
 * There is no OLED.  Everything else (signed boot, NOR cartridge, USB CDC) is
 * shared with the N1_Console OLED build.
 *
 * Boot chain:
 *   IAP_USB_SIGNED boot (0x0000, 10 KB) verifies the signed launcher and jumps
 *   to 0x2800.  The launcher reads the cartridge catalog from NOR, copies the
 *   selected game into the internal game slot at 0x7800, verifies CRC32 and
 *   jumps.  Games run without USB and return with NVIC_SystemReset().
 *
 * Controls:
 *   menu       UP/DOWN select, A play, B sound on/off
 *   no cart    A rescan, B sound on/off
 *   menu       hold UP+DOWN 2 s to enter the USB bootloader
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <stdlib.h>

#include "debug.h"
#include "console_config.h"
#include "board_config.h"
#include "n1_layout.h"
#include "input.h"
#include "panel_gfx.h"
#include "audio.h"
#include "nor_store.h"
#include "cart.h"
#include "n1_usb.h"
#include "wsp_tick.h"
#include "ws2812.h"
#include "app_iap.h"
#include "game.h"
#include "console.h"

#define MENU_VISIBLE    5u

static input_state_t g_in;
static uint8_t       g_menu_sel;
static uint8_t       g_menu_top;
static uint8_t       g_ui;
static uint16_t      g_iap_hold;
static uint8_t       g_blink;
static uint8_t       g_error;
static uint32_t      g_frame_next;

/*---------------------------------------------------------------------------
 * launcher API
 *---------------------------------------------------------------------------*/
uint8_t console_ui_state(void)
{
    return g_ui;
}

uint8_t console_current_game(void)
{
    return g_menu_sel;
}

uint8_t console_game_count(void)
{
    return cart_count();
}

void console_cart_updated(void)
{
    if (cart_count() == 0u) {
        g_menu_sel = 0u;
    } else if (g_menu_sel >= cart_count()) {
        g_menu_sel = 0u;
    }

    if (g_menu_sel < (uint8_t)MENU_VISIBLE) {
        g_menu_top = 0u;
    } else {
        g_menu_top = (uint8_t)(g_menu_sel - (MENU_VISIBLE - 1u));
    }

    g_error = 0u;
}

uint8_t console_load_game(uint8_t index)
{
    const cart_entry_t *e = cart_entry(index);
    uint8_t rc;

    if (!e) {
        g_error = CART_ERR_NOCART;
        return g_error;
    }

    g_menu_sel = index;
    g_error    = 0u;
    store_set_last_game(index);

    panel_clear();
    panel_text3x5_center(1, "LOAD", P_CYAN);
    panel_text3x5_center(7, e->code, P_WHITE);
    panel_text3x5_center(13, "....", P_GRAY);
    panel_flush();

    rc = cart_load(index);

    if (rc != CART_OK) {
        g_error = rc;

        panel_clear();
        panel_text3x5_center(2, "LOAD", P_RED);
        panel_text3x5_center(8, "ERR", P_RED);
        panel_flush();
    }

    return rc;
}

void console_jump_game(void)
{
    cart_jump();
}

void console_return_menu(void)
{
    g_ui = CONSOLE_UI_MENU;
}

void console_set_brightness(uint8_t level)
{
    store_set_brightness(level);
    panel_set_brightness(store_brightness());
    audio_sfx(SFX_MOVE);
}

void console_set_sound(uint8_t on)
{
    store_set_sound(on);
    audio_set_enabled(on);

    if (on) {
        audio_sfx(SFX_SELECT);
    }
}

void console_fill_info(uint8_t out[11])
{
    out[0] = CONSOLE_PROTO_VER;
    out[1] = (uint8_t)PANEL_W;
    out[2] = (uint8_t)PANEL_H;
    out[3] = cart_count();
    out[4] = g_menu_sel;
    out[5] = g_ui;
    out[6] = panel_brightness();
    out[7] = audio_enabled();
    out[8] = store_present();
    out[9] = cart_valid();
    out[10] = 0u;               /* no OLED in this product */
}

/* Present for the shared USB layer; the Pixels product has no OLED. */
uint8_t console_oled_reinit(void)
{
    return 0u;
}

void console_fill_scores(uint8_t *out)
{
    uint8_t i;

    for (i = 0; i < CONSOLE_MAX_GAMES; i++) {
        uint16_t s = store_hi(i);
        out[i * 2u]      = (uint8_t)(s & 0xFFu);
        out[i * 2u + 1u] = (uint8_t)(s >> 8);
    }
}

void console_reset_scores(void)
{
    store_reset_hi();
}

/*---------------------------------------------------------------------------
 * panel menu
 *---------------------------------------------------------------------------*/
static uint8_t key_repeat(uint8_t mask, uint8_t idx)
{
    uint8_t held;

    if (g_in.pressed & mask) {
        return 1u;
    }

    if (g_in.held & mask) {
        held = g_in.held_frames[idx];
        if (held >= CONSOLE_REPEAT_DELAY &&
            ((held - CONSOLE_REPEAT_DELAY) % CONSOLE_REPEAT_RATE) == 0u) {
            return 1u;
        }
    }

    return 0u;
}

static void menu_draw_panel(void)
{
    panel_clear();

    if (!cart_valid() || cart_count() == 0u) {
        panel_text3x5_center(0, "NO", P_RED);
        panel_text3x5_center(6, "CART", P_WHITE);
        panel_text3x5_center(11, "USB", P_CYAN);
    } else {
        const cart_entry_t *e = cart_entry(g_menu_sel);

        if (e) {
            cart_draw_icon(e);
            panel_text3x5_center(10, e->code, e->theme);

            /* 3 px scroll thumb, works for any cartridge size */
            if (cart_count() > 1u) {
                int x = ((int)g_menu_sel * (PANEL_W - 3)) / ((int)cart_count() - 1);
                panel_hline(0, 15, PANEL_W, P_GRAY);
                panel_hline(x, 15, 3, e->theme);
            } else {
                panel_hline(0, 15, PANEL_W, e->theme);
            }
        }
    }

    panel_pixel(PANEL_W - 1, PANEL_H - 2, audio_enabled() ? P_GREEN : P_DRED);

    if (g_error) {
        panel_frame(0, 0, PANEL_W, PANEL_H, P_RED);
    }

    if ((g_blink >> 4) & 1u) {
        uint8_t c = (cart_valid() && cart_count()) ? cart_entry(g_menu_sel)->theme : P_CYAN;
        panel_pixel(0, 4, c);
        panel_pixel(PANEL_W - 1, 4, c);
    }
}

static void menu_tick(void)
{
    uint8_t count = cart_count();
    uint8_t old = g_menu_sel;
    uint8_t both = (uint8_t)((g_in.held & (BTN_UP | BTN_DOWN)) == (BTN_UP | BTN_DOWN));

    g_blink++;

    if (count == 0u) {
        if (g_in.pressed & BTN_A) {
            (void)cart_scan();
            console_cart_updated();
            audio_sfx(SFX_SELECT);
        }
        if (g_in.pressed & BTN_B) {
            console_set_sound(audio_enabled() ? 0u : 1u);
        }
    } else {
        if (!both) {
            if (key_repeat(BTN_DOWN, BTN_DOWN_IDX)) {
                g_menu_sel = (uint8_t)((g_menu_sel + 1u) % count);
            } else if (key_repeat(BTN_UP, BTN_UP_IDX)) {
                g_menu_sel = (uint8_t)((g_menu_sel + count - 1u) % count);
            }
        }

        if (g_menu_sel != old) {
            g_error = 0u;
            audio_sfx(SFX_MOVE);
        }

        if (g_menu_sel < g_menu_top) {
            g_menu_top = g_menu_sel;
        } else if (g_menu_sel >= (uint8_t)(g_menu_top + MENU_VISIBLE)) {
            g_menu_top = (uint8_t)(g_menu_sel - (MENU_VISIBLE - 1u));
        }

        if (g_in.pressed & BTN_A) {
            if (console_load_game(g_menu_sel) == CART_OK) {
                console_jump_game();
            }
        }

        if (g_in.pressed & BTN_B) {
            console_set_sound(audio_enabled() ? 0u : 1u);
        }
    }

    /* UP + DOWN held: bring the IAP bootloader up, even when USB is dead */
    if (both) {
        if (g_iap_hold < CONSOLE_IAP_HOLD_FRAMES) {
            g_iap_hold++;
        }
    } else {
        g_iap_hold = 0u;
    }

    menu_draw_panel();

    if (g_iap_hold > 8u) {
        uint8_t w = (uint8_t)(((uint32_t)g_iap_hold * PANEL_W) /
                              CONSOLE_IAP_HOLD_FRAMES);
        panel_hline(0, PANEL_H - 1, w, P_CYAN);

        if ((g_iap_hold >> 3) & 1u) {
            panel_text3x5_center(3, "USB", P_WHITE);
        }
    }

    if (g_iap_hold >= CONSOLE_IAP_HOLD_FRAMES) {
        panel_clear();
        panel_text3x5_center(5, "USB", P_CYAN);
        panel_flush();

        Delay_Ms(120);
        app_iap_request();              /* does not return */
    }
}

/*---------------------------------------------------------------------------
 * main
 *---------------------------------------------------------------------------*/
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    tick_init();
    input_init();
    audio_init();
    ws2812_init();
    panel_init();

    store_init();
    panel_set_brightness(store_brightness());
    audio_set_enabled(store_sound());

    srand(tick_ms());

    n1_usb_init();

    (void)cart_scan();

    g_menu_sel = store_last_game();
    if (g_menu_sel >= cart_count()) {
        g_menu_sel = 0u;
    }
    g_menu_top = 0u;
    g_ui       = CONSOLE_UI_MENU;
    g_iap_hold = 0u;
    g_blink    = 0u;
    g_error    = 0u;

    audio_sfx(SFX_SELECT);

    g_frame_next = tick_ms();

    while (1) {
        uint32_t now;

        input_update(&g_in);
        n1_usb_service();
        audio_update();

        menu_tick();
        panel_flush();

        now = tick_ms();

        if ((uint32_t)(now - g_frame_next) < CONSOLE_FRAME_MS) {
            while ((uint32_t)(tick_ms() - g_frame_next) < CONSOLE_FRAME_MS) {
            }
            g_frame_next += CONSOLE_FRAME_MS;
        } else if ((uint32_t)(now - g_frame_next) > (4u * CONSOLE_FRAME_MS)) {
            g_frame_next = now;
        } else {
            g_frame_next += CONSOLE_FRAME_MS;
        }
    }
}
