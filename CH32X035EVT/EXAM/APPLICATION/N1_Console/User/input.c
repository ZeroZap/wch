/********************************** (C) COPYRIGHT *******************************
 * File Name          : input.c
 * Description        : See input.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "input.h"
#include "board_config.h"
#include "debug.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       mask;
} btn_map_t;

static const btn_map_t btn_map[BTN_COUNT] = {
    { N1_BTN_UP_PORT,    N1_BTN_UP_PIN,    BTN_UP    },
    { N1_BTN_DOWN_PORT,  N1_BTN_DOWN_PIN,  BTN_DOWN  },
    { N1_BTN_LEFT_PORT,  N1_BTN_LEFT_PIN,  BTN_LEFT  },
    { N1_BTN_RIGHT_PORT, N1_BTN_RIGHT_PIN, BTN_RIGHT },
    { N1_BTN_A_PORT,     N1_BTN_A_PIN,     BTN_A     },
    { N1_BTN_B_PORT,     N1_BTN_B_PIN,     BTN_B     },
};

static uint8_t stable;
static uint8_t previous;
static uint8_t debounce[BTN_COUNT];

static uint8_t read_mask(void)
{
    uint8_t i;
    uint8_t raw = 0u;

    for (i = 0; i < BTN_COUNT; i++) {
        if (GPIO_ReadInputDataBit(btn_map[i].port, btn_map[i].pin) == Bit_RESET) {
            raw |= btn_map[i].mask;
        }
    }

    return raw;
}

void input_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t i;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC, ENABLE);

    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;

    for (i = 0; i < BTN_COUNT; i++) {
        gpio.GPIO_Pin = btn_map[i].pin;
        GPIO_Init(btn_map[i].port, &gpio);
        debounce[i] = 0u;
    }

    stable   = read_mask();
    previous = stable;
}

void input_update(input_state_t *st)
{
    uint8_t raw = read_mask();
    uint8_t i;

    for (i = 0; i < BTN_COUNT; i++) {
        uint8_t mask = btn_map[i].mask;

        if ((raw & mask) != (stable & mask)) {
            if (++debounce[i] >= 2u) {
                stable ^= mask;
                debounce[i] = 0u;
            }
        } else {
            debounce[i] = 0u;
        }
    }

    st->pressed  = (uint8_t)(stable & (uint8_t)~previous);
    st->released = (uint8_t)(previous & (uint8_t)~stable);
    st->held     = stable;
    previous     = stable;

    for (i = 0; i < BTN_COUNT; i++) {
        if (stable & btn_map[i].mask) {
            if (st->held_frames[i] < 255u) {
                st->held_frames[i]++;
            }
        } else {
            st->held_frames[i] = 0u;
        }
    }
}

uint8_t input_raw(void)
{
    return read_mask();
}
