/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_trans.c
 * Description        : Fade-through-black transitions, see wsp_trans.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "wsp_trans.h"
#include "wsp_config.h"
#include "wsp_app.h"
#include "wsp_cfg.h"
#include "ws2812.h"
#include "debug.h"
#include "wsp_tick.h"

#if WSP_TRANSITION_ENABLE

#define TRANS_STEPS         8u

static uint8_t trans_saved;
static uint8_t trans_have_saved;
static uint8_t trans_running;

volatile uint32_t g_trans_runs;

static void ramp(uint8_t from, uint8_t to, uint16_t ms)
{
    uint8_t  i;
    uint16_t delay = (uint16_t)(ms / TRANS_STEPS);

    trans_running = 1;
    g_trans_runs++;

    if (delay == 0) {
        delay = 1;
    }

    for (i = 1; i <= TRANS_STEPS; i++) {
        int v = (int)from + (((int)to - (int)from) * (int)i) / (int)TRANS_STEPS;

        if (v < 0) {
            v = 0;
        } else if (v > 255) {
            v = 255;
        }

        ws2812_set_brightness((uint8_t)v);
        app_show();                 /* re-send the canvas at the new level */
        Delay_Ms(delay);
    }

    trans_running = 0;
}

void trans_fade_out(uint16_t ms)
{
    trans_saved = ws2812_brightness();
    trans_have_saved = 1;
    ramp(trans_saved, 0, ms);
}

void trans_fade_in(uint16_t ms)
{
    if (trans_have_saved == 0) {
        /* no fade_out first: fall back to the configured brightness */
        trans_saved = wsp_cfg()->brightness;
    }

    ramp(0, trans_saved, ms);
    trans_have_saved = 0;
}

uint8_t trans_active(void)
{
    return trans_running;
}

#endif /* WSP_TRANSITION_ENABLE */
