/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_trans.h
 * Description        : Fade-through-black transitions for the demo content.
 *
 * WS2812 latches its last value, so a transition has to re-send the canvas at
 * every step; the fade is therefore done with the global brightness and
 * app_show(), which also keeps the gamma table and the current limiter in the
 * loop.  No extra framebuffer is needed.
 *
 * Usage around a content change:
 *     trans_fade_out(WSP_TRANS_MS);
 *     draw_the_new_frame();
 *     trans_fade_in(WSP_TRANS_MS);
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_TRANS_H
#define __WSP_TRANS_H

#include "ch32x035.h"
#include "wsp_config.h"

#if WSP_TRANSITION_ENABLE

/* ramp the current canvas down to black (remembers the starting brightness) */
void trans_fade_out(uint16_t ms);

/* ramp back up to the remembered brightness */
void trans_fade_in(uint16_t ms);

/* 1 while a ramp is in progress; useful to block buttons during the fade */
uint8_t trans_active(void);

/* number of ramps executed so far, for debugging over WCH-Link */
extern volatile uint32_t g_trans_runs;

#else

#define trans_fade_out(ms)      ((void)(ms))
#define trans_fade_in(ms)       ((void)(ms))
#define trans_active()          ((uint8_t)0)

#endif

#endif /* __WSP_TRANS_H */
