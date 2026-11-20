/********************************** (C) COPYRIGHT *******************************
 * File Name          : ws2812.h
 * Description        : WS2812 (800kHz single wire) driver on the PIOC engine.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WS2812_H
#define __WS2812_H

#include "ch32x035.h"
#include "wsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void ws2812_init(void);

/* Global dimming, 0..255 */
void ws2812_set_brightness(uint8_t level);

/* Byte order sent to the strip.  Starts at WS2812_COLOR_ORDER from
 * wsp_config.h; the SET_ORDER USB command can change it at runtime (RAM only,
 * a power cycle restores the compile time value). */
uint8_t ws2812_set_order(uint8_t order);     /* 0 = accepted, 1 = bad value */
uint8_t ws2812_order(void);

/* 1-wire bit rate: number of PIOC clocks per WS2812 bit.  56 -> 1.167us at
 * 48MHz (vendor default), 60 -> 1.25us (WS2812B nominal).  Clamped to
 * WS2812_CYC_MIN..WS2812_CYC_MAX; 0 = query only. */
uint8_t ws2812_set_cycles(uint8_t cycles);   /* returns the value in use */
uint8_t ws2812_cycles(void);

/*---------------------------------------------------------------------------
 * Current limiter
 *   ws2812_duty_table()  : 256-entry table, canvas value -> PWM duty
 *                          (brightness and gamma already applied)
 *   ws2812_limit_frame() : given the duty sum of a frame, pick a scale so the
 *                          frame stays inside WSP_CURRENT_LIMIT_MA
 *   ws2812_set_limit_ma(): 0 disables the limiter
 *---------------------------------------------------------------------------*/
const uint8_t *ws2812_duty_table(void);
void     ws2812_limit_frame(uint32_t duty_sum);
void     ws2812_set_limit_ma(uint16_t ma);
uint16_t ws2812_limit_ma(void);
/* 1 when the last frame had to be dimmed, 0 otherwise */
uint8_t  ws2812_limited(void);

/* Stream pixels into the strip.  Each ws2812_write() call appends one
 * RGB triplet (logical RGB order, the driver takes care of the strip order) to the
 * frame buffer, ws2812_end() hands the whole frame to PIOC and waits for
 * the 286us latch pulse.
 * A frame is limited to 1024 pixels by the PIOC 1-wire engine. */
void ws2812_begin(void);
void ws2812_write(uint8_t r, uint8_t g, uint8_t b);
void ws2812_end(void);

/* 0 = last PIOC transfer succeeded, otherwise RGB1W_ERR_* */
uint8_t ws2812_last_error(void);

/* Convenience helper: send n RGB triplets from a byte buffer. */
void ws2812_show(const uint8_t *rgb, uint16_t n);

#ifdef __cplusplus
}
#endif

#endif /* __WS2812_H */
