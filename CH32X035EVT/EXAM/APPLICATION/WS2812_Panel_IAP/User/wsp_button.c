/********************************** (C) Copyright *******************************
 * File Name          : wsp_button.c
 * Description        : Debounced user button (PA0, active low).
 *
 * Gestures
 *   short press : released before WSP_BTN_LONG_MS        -> next effect / item
 *   long press  : released between WSP_BTN_LONG_MS and
 *                 WSP_BTN_UPGRADE_MS                     -> next item / mode
 *   upgrade     : still held at WSP_BTN_UPGRADE_MS       -> ask the bootloader
 *                 to take over (set to 0 to disable)
 *
 * The long press is reported on release, not when the threshold is crossed, so
 * holding through the upgrade threshold does not fire a mode change first.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "wsp_button.h"
#include "wsp_tick.h"

#if WSP_BTN_ENABLE

static uint8_t  raw_last = 1;       /* 1 = released */
static uint8_t  stable = 1;
static uint32_t raw_change_ms;
static uint32_t press_ms;
static uint8_t  upgrade_sent;
static uint8_t  ev_short;
static uint8_t  ev_long;
static uint8_t  ev_upgrade;

static uint8_t btn_read(void)
{
    return GPIO_ReadInputDataBit(WSP_BTN_PORT, WSP_BTN_PIN) ? 1 : 0;
}

void button_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = WSP_BTN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    raw_last = btn_read();
    stable = raw_last;
    raw_change_ms = tick_ms();
    press_ms = raw_change_ms;
    upgrade_sent = 0;
    ev_short = 0;
    ev_long = 0;
    ev_upgrade = 0;
}

void button_poll(void)
{
    uint32_t now = tick_ms();
    uint8_t  raw = btn_read();

    if (raw != raw_last) {
        raw_last = raw;
        raw_change_ms = now;
        return;
    }

    if ((now - raw_change_ms) < WSP_BTN_DEBOUNCE_MS) {
        return;
    }

    if (raw != stable) {
        stable = raw;
        if (stable == 0) {
            press_ms = now;
            upgrade_sent = 0;
        } else if (!upgrade_sent) {
            /* released before the upgrade threshold: short or long */
            if ((now - press_ms) >= WSP_BTN_LONG_MS) {
                ev_long = 1;
            } else {
                ev_short = 1;
            }
        }
    }

#if (WSP_BTN_UPGRADE_MS > 0)
    if ((stable == 0) && !upgrade_sent &&
        ((now - press_ms) >= WSP_BTN_UPGRADE_MS)) {
        upgrade_sent = 1;
        ev_upgrade = 1;
    }
#endif
}

uint8_t button_short(void)
{
    if (ev_short) {
        ev_short = 0;
        return 1;
    }

    return 0;
}

uint8_t button_long(void)
{
    if (ev_long) {
        ev_long = 0;
        return 1;
    }

    return 0;
}

uint8_t button_upgrade(void)
{
    if (ev_upgrade) {
        ev_upgrade = 0;
        return 1;
    }

    return 0;
}

#else   /* WSP_BTN_ENABLE == 0 */

void button_init(void) {}
void button_poll(void) {}
uint8_t button_short(void) { return 0; }
uint8_t button_long(void) { return 0; }
uint8_t button_upgrade(void) { return 0; }

#endif
