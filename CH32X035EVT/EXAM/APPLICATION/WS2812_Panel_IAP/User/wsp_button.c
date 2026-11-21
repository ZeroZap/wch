/********************************** (C) Copyright *******************************
 * File Name          : wsp_button.c
 * Description        : Debounced user button (PA0, active low).
 *
 * Events
 *   short press : released before WSP_BTN_LONG_MS
 *   long press  : reported as soon as WSP_BTN_LONG_MS is reached, the
 *                 following release does not generate a short press
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
static uint8_t  long_sent;
static uint8_t  ev_short;
static uint8_t  ev_long;

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
    long_sent = 0;
    ev_short = 0;
    ev_long = 0;
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
            long_sent = 0;
        } else if (!long_sent) {
            ev_short = 1;
        }
    }

    if ((stable == 0) && !long_sent && ((now - press_ms) >= WSP_BTN_LONG_MS)) {
        long_sent = 1;
        ev_long = 1;
    }
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

#else   /* WSP_BTN_ENABLE == 0 */

void button_init(void) {}
void button_poll(void) {}
uint8_t button_short(void) { return 0; }
uint8_t button_long(void) { return 0; }

#endif
