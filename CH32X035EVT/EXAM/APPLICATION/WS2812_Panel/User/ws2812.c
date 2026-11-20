/********************************** (C) COPYRIGHT *******************************
 * File Name          : ws2812.c
 * Description        : WS2812 driver using the PIOC 1-wire engine.
 *
 * Implementation notes
 * --------------------
 * The waveform is not bit-banged by the CPU.  Following
 * EXAM/PIOC/1_Wire, a small vendor supplied program (PIOC_1W_CODE in
 * RGB1W.c) is loaded into the 4KB PIOC code RAM at 0x20004000 and the PIOC
 * "long data" mode encodes the whole frame from PIOC SRAM in hardware:
 *
 *   - bit cycle 56 PIOC clocks @48MHz -> 1.167us  (WS2812 target 1.25us)
 *   - MSB first, bytes are shifted in buffer order (so we store G,R,B)
 *   - after the last bit the engine drives the line low for 286us, which
 *     is the WS2812 latch/reset pulse
 *
 * Limits / costs
 * --------------
 *   - one transfer carries at most 3072 bytes -> 1024 RGB pixels
 *     (16x16 = 768 bytes, 32x32 = 3072 bytes)
 *   - the PIOC code RAM occupies the upper 4KB of the 20KB SRAM, therefore
 *     the linker script of this project must set RAM = 16K (see Ld/Link.ld)
 *   - PC18/PC19 are shared with the SDI debug pins; using them disables
 *     SDI.  The default is PC7 via GPIO_Remap_PIOC, which leaves the debug
 *     interface alone.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>
#include "debug.h"
#include "ws2812.h"
#include "wsp_gamma.h"
#include "RGB1W.h"

#if (WS2812_PIOC_PIN == WS2812_PIOC_PC19)
#define WS_PIOC_MOD             1
#else
#define WS_PIOC_MOD             0
#endif

#define WS_DATA_MAX             ((uint16_t)(CANVAS_PIXELS * 3))

/* byte 0..2 of the frame as the strip wants to see them: 0 = R, 1 = G, 2 = B */
static const uint8_t ws_ord_tab[6][3] = {
    {0, 1, 2},      /* RGB */
    {1, 0, 2},      /* GRB */
    {2, 0, 1},      /* BRG */
    {2, 1, 0},      /* BGR */
    {0, 2, 1},      /* RBG */
    {1, 2, 0},      /* GBR */
};

static uint8_t  ws_buf[WS_DATA_MAX];
static uint16_t ws_len;
static uint8_t  ws_bright = WS2812_BRIGHTNESS;
static uint8_t  ws_err;

/* canvas value -> PWM duty, rebuilt whenever the brightness changes so the
 * per-pixel path is a plain table lookup (+ gamma) instead of two multiplies */
static uint8_t  duty_tab[256];

/* frame wide dimming chosen by the current limiter, 255 = no scaling */
static uint8_t  ws_scale = 255;
static uint16_t ws_limit_ma = WSP_CURRENT_LIMIT_MA;
static uint8_t  ws_was_limited;

#if WSP_DITHER
static uint8_t  dither_phase;
#endif

/* runtime copy of WS2812_COLOR_ORDER, changeable from the host with the
 * SET_ORDER command so a different strip does not need a rebuild */
static uint8_t  ws_order = WS2812_COLOR_ORDER;

/*********************************************************************
 * @fn      ws_pioc_init
 *
 * @brief   Load the 1-wire program into PIOC and release the output pin.
 *          Same sequence as RGB1W_Init() in EXAM/PIOC/1_Wire, but the pin
 *          is selected by WS2812_PIOC_PIN.
 *
 * @return  none
 */
static void ws_pioc_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_IO2W, ENABLE);

#if (WS2812_PIOC_PIN == WS2812_PIOC_PC7)
    /* PC7 via the PIOC alternate function, SDI debug stays available */
    GPIO_PinRemapConfig(GPIO_Remap_PIOC, ENABLE);
    GPIO_SetBits(GPIOC, GPIO_Pin_7);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
#elif (WS2812_PIOC_PIN == WS2812_PIOC_PC19)
    /* PC19 = PIOC IO1, shares the SDI pins */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
    GPIO_SetBits(GPIOC, GPIO_Pin_19);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_19;
#else
    /* PC18 = PIOC IO0, shares the SDI pins */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
    GPIO_SetBits(GPIOC, GPIO_Pin_18);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_18;
#endif

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* reset/halt the PIOC so the code RAM can be loaded */
    PIOC->D8_SYS_CFG = RB_MST_RESET | RB_MST_IO_EN0 | RB_MST_IO_EN1;
    memcpy((uint8_t *)(uintptr_t)PIOC_SRAM_BASE, PIOC_1W_CODE, PIOC_1W_CODE_SIZE);

#if defined SYSCLK_FREQ_24MHz_HSI
    *(uint32_t *)(uintptr_t)(PIOC_SRAM_BASE + PIOC_FREQ_CFG) = 0x00300030;
#endif
}

/*********************************************************************
 * @fn      ws2812_init
 *
 * @brief   Configure the PIOC WS2812 engine.
 *
 * @return  none
 */
void ws2812_init(void)
{
    ws_pioc_init();

    ws_len = 0;
    ws_err = RGB1W_ERR_OK;
    ws_scale = 255;
    ws2812_set_brightness(ws_bright);     /* build the duty table */
}

/*********************************************************************
 * @fn      ws2812_set_brightness
 *
 * @brief   Global dimming applied to every pixel.
 *
 * @param   level - 0 (off) .. 255 (full).
 *
 * @return  none
 */
void ws2812_set_brightness(uint8_t level)
{
    uint16_t i;

    ws_bright = level;

    for (i = 0; i < 256u; i++) {
        /* exact /255 (not >>8, which would make level 255 land on 254) */
        uint16_t v = (uint16_t)(((uint32_t)i * level + 127u) / 255u);

#if WS2812_GAMMA
        /* brightness first, gamma last: the user control stays perceptually
         * linear while the PWM duty follows the eye's response */
        duty_tab[i] = ws_gamma_table[v];
#else
        duty_tab[i] = (uint8_t)v;
#endif
    }
}

/*********************************************************************
 * @fn      ws2812_set_order
 *
 * @brief   Change the byte order sent to the strip at runtime.
 *
 * @param   order - WS2812_ORDER_* (0..5), out of range is ignored.
 *
 * @return  0 = accepted, 1 = rejected.
 */
uint8_t ws2812_set_order(uint8_t order)
{
    if (order > WS2812_ORDER_GBR) {
        return 1;
    }

    ws_order = order;

    return 0;
}

uint8_t ws2812_brightness(void)
{
    return ws_bright;
}

uint8_t ws2812_order(void)
{
    return ws_order;
}

/*********************************************************************
 * @fn      ws2812_set_cycles
 *
 * @brief   Change the 1-wire bit rate (PIOC clocks per bit).
 *
 * @param   cycles - WS2812_CYC_MIN..WS2812_CYC_MAX, other values clamp.
 *
 * @return  the value now in use.
 */
uint8_t ws2812_set_cycles(uint8_t cycles)
{
    if (cycles != 0) {
        if (cycles < WS2812_CYC_MIN) {
            cycles = WS2812_CYC_MIN;
        } else if (cycles > WS2812_CYC_MAX) {
            cycles = WS2812_CYC_MAX;
        }
        RGB1W_SetCyc(cycles);
    }

    return RGB1W_Cyc;
}

uint8_t ws2812_cycles(void)
{
    return RGB1W_Cyc;
}

/*********************************************************************
 * @fn      ws2812_limit_frame
 *
 * @brief   Pick the frame wide dimming factor from the duty sum of the frame
 *          that is about to be sent.
 *
 * @param   duty_sum - sum of every PWM duty value in the frame.
 *
 * @return  none
 */
void ws2812_limit_frame(uint32_t duty_sum)
{
    uint32_t budget;
    uint8_t  scale = 255;

    ws_was_limited = 0;

    if ((ws_limit_ma != 0) && (duty_sum != 0)) {
        /* a channel at duty 255 draws 20mA, so the budget in duty units is
         * limit_ma * 255 / 20 */
        budget = ((uint32_t)ws_limit_ma * 255u) / 20u;

        if (duty_sum > budget) {
            scale = (uint8_t)((budget * 255u) / duty_sum);
            if (scale == 0) {
                scale = 1;
            }
            ws_was_limited = 1;
        }
    }

    ws_scale = scale;
}

void ws2812_set_limit_ma(uint16_t ma)
{
    ws_limit_ma = ma;
}

uint16_t ws2812_limit_ma(void)
{
    return ws_limit_ma;
}

uint8_t ws2812_limited(void)
{
    return ws_was_limited;
}

const uint8_t *ws2812_duty_table(void)
{
    return duty_tab;
}

/*********************************************************************
 * @fn      ws2812_begin
 *
 * @brief   Start a new frame.
 *
 * @return  none
 */
void ws2812_begin(void)
{
    ws_len = 0;
}

/*********************************************************************
 * @fn      ws2812_write
 *
 * @brief   Append one pixel (logical RGB order) to the frame buffer.
 *
 * @param   r - red.
 *          g - green.
 *          b - blue.
 *
 * @return  none
 */
void ws2812_write(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t         d[3];
    const uint8_t  *ord = ws_ord_tab[ws_order];

    if ((uint16_t)(ws_len + 3u) > WS_DATA_MAX) {
        return;
    }

    d[0] = duty_tab[r];
    d[1] = duty_tab[g];
    d[2] = duty_tab[b];

#if WSP_DITHER
    /* +1 on a rotating subset: breaks up gamma quantisation bands without
     * changing the average level much */
    dither_phase++;
    if ((dither_phase & 3u) == 0) {
        if (d[0] < 255u) d[0]++;
        if (d[1] < 255u) d[1]++;
        if (d[2] < 255u) d[2]++;
    }
#endif

    if (ws_scale != 255) {
        d[0] = (uint8_t)(((uint16_t)d[0] * ws_scale + 128u) >> 8);
        d[1] = (uint8_t)(((uint16_t)d[1] * ws_scale + 128u) >> 8);
        d[2] = (uint8_t)(((uint16_t)d[2] * ws_scale + 128u) >> 8);
    }

    ws_buf[ws_len++] = d[ord[0]];
    ws_buf[ws_len++] = d[ord[1]];
    ws_buf[ws_len++] = d[ord[2]];
}

/*********************************************************************
 * @fn      ws2812_end
 *
 * @brief   Hand the frame to the PIOC engine and wait for the reset pulse.
 *
 * @return  none
 */
void ws2812_end(void)
{
    if (ws_len == 0) {
        return;
    }

    ws_err = RGB1W_SendRAM_Wait(ws_len, ws_buf, WS_PIOC_MOD);
    ws_len = 0;
}

/*********************************************************************
 * @fn      ws2812_last_error
 *
 * @brief   Status of the last PIOC transfer, 0 = success.
 *
 * @return  RGB1W_ERR_* code.
 */
uint8_t ws2812_last_error(void)
{
    return ws_err;
}

/*********************************************************************
 * @fn      ws2812_show
 *
 * @brief   Send n RGB triplets from a linear buffer.
 *
 * @param   rgb - buffer of n*3 bytes, logical RGB order.
 *          n   - number of pixels.
 *
 * @return  none
 */
void ws2812_show(const uint8_t *rgb, uint16_t n)
{
    uint16_t        i;
    uint32_t        sum = 0;
    const uint8_t  *dt = duty_tab;

    for (i = 0; i < (uint16_t)(n * 3u); i++) {
        sum += dt[rgb[i]];
    }
    ws2812_limit_frame(sum);

    ws2812_begin();
    for (i = 0; i < n; i++) {
        ws2812_write(rgb[0], rgb[1], rgb[2]);
        rgb += 3;
    }
    ws2812_end();
}
