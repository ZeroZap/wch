/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_runtime.c
 * Description        : See game_runtime.h.
 *
 * N1_Console_Pixels build: the game image is linked at N1_GAME_OFFSET and owns
 * the whole machine after the launcher jumps.  Everything is drawn on the
 * 16x16 WS2812 panel; there is no OLED in this product.  The game runs at
 * 60 Hz and resets back into the launcher when B (or UP+DOWN) is held.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <stdlib.h>

#include "game_runtime.h"
#include "console_config.h"
#include "panel_gfx.h"
#include "audio.h"
#include "input.h"
#include "nor_store.h"
#include "ws2812.h"
#include "wsp_tick.h"
#include "debug.h"

static input_state_t g_in;
static uint16_t      g_exit_hold;

void game_runtime_run(const game_desc_t *g)
{
    uint32_t next;

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

    g->init();
    audio_sfx(SFX_SELECT);

    g_exit_hold = 0u;
    next = tick_ms();

    for (;;) {
        uint32_t now;
        uint8_t  exit_held;

        input_update(&g_in);
        audio_update();

        exit_held = 0u;
        if (g_in.held & BTN_B) {
            exit_held = 1u;
        } else if ((g_in.held & (BTN_UP | BTN_DOWN)) == (BTN_UP | BTN_DOWN)) {
            exit_held = 1u;
        }

        if (exit_held) {
            if (g_exit_hold < 0xFFFFu) {
                g_exit_hold++;
            }
        } else {
            g_exit_hold = 0u;
        }

        if (g_exit_hold >= CONSOLE_EXIT_HOLD_FRAMES) {
            NVIC_SystemReset();
        }

        if (g->tick(&g_in)) {
            NVIC_SystemReset();
        }

        panel_flush();

        now = tick_ms();
        if ((uint32_t)(now - next) < CONSOLE_FRAME_MS) {
            while ((uint32_t)(tick_ms() - next) < CONSOLE_FRAME_MS) {
            }
            next += CONSOLE_FRAME_MS;
        } else if ((uint32_t)(now - next) > (4u * CONSOLE_FRAME_MS)) {
            next = now;
        } else {
            next += CONSOLE_FRAME_MS;
        }
    }
}
