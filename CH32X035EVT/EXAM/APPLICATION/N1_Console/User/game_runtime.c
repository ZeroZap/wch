/********************************** (C) COPYRIGHT *******************************
 * File Name          : game_runtime.c
 * Description        : See game_runtime.h.
 *
 * A game image is linked at N1_GAME_OFFSET and owns the whole machine after
 * the launcher jumps.  It initialises the shared drivers, runs the game at
 * 60 Hz and resets back into the launcher (boot -> menu) when the player holds
 * B (or UP+DOWN) or the game asks to exit.
 *
 * N1_Console is the OLED product: the 16x16 logical game buffer is upscaled
 * 4x into the left 64x64 half of the SSD1306, the right half carries the HUD.
 * The WS2812 panel output has been removed from this build.
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
#include "oled.h"
#include "wsp_tick.h"
#include "debug.h"

static input_state_t g_in;
static uint16_t      g_exit_hold;

/* Draw the game on the OLED: 16x16 -> 64x64 (4x) on the left, HUD on the right. */
static void oled_game_draw(const game_desc_t *g)
{
    uint8_t x, y;

    if (!oled_present()) {
        return;
    }

    oled_clear();

    for (y = 0; y < PANEL_H; y++) {
        for (x = 0; x < PANEL_W; x++) {
            if (panel_get(x, y) != P_BLACK) {
                oled_block((uint8_t)(x * 4), (uint8_t)(y * 4), 4u, 4u);
            }
        }
    }

    oled_text(66, 0, g->name);
    oled_text(66, 2, "HI");
    oled_num3(84, 2, store_hi(g->id));
    oled_text(66, 4, "SCORE");
    oled_num3(66, 5, g->score ? g->score() : 0u);
    oled_text(66, 7, "HOLD B/UD");

    oled_refresh();
}

void game_runtime_run(const game_desc_t *g)
{
    uint32_t next;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    tick_init();
    input_init();
    audio_init();
    panel_init();

    store_init();
    oled_init();
    oled_set_brightness(store_brightness());
    audio_set_enabled(store_sound());

    srand(tick_ms());

    g->init();
    oled_game_draw(g);
    audio_sfx(SFX_SELECT);

    g_exit_hold = 0u;
    next = tick_ms();

    for (;;) {
        uint32_t now;
        uint8_t  exit_held;

        input_update(&g_in);
        audio_update();

        /* Exit with B, or UP+DOWN when B is not wired on the bench. */
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
            if (oled_present()) {
                oled_clear();
                oled_text_center(3, "BYE");
                oled_refresh();
            }
            NVIC_SystemReset();
        }

        if (g->tick(&g_in)) {
            NVIC_SystemReset();
        }

        if (oled_present()) {
            oled_game_draw(g);
        }

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
