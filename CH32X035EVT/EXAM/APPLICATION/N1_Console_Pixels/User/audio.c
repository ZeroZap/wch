/********************************** (C) COPYRIGHT *******************************
 * File Name          : audio.c
 * Description        : See audio.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "audio.h"
#include "board_config.h"
#include "debug.h"

#define SFX_QUEUE_LEN   10

typedef struct {
    uint16_t freq;
    uint8_t  frames;
} note_t;

static note_t  queue[SFX_QUEUE_LEN];
static uint8_t head;
static uint8_t tail;
static uint8_t timer;
static uint8_t enabled = 1u;

static void buzzer_set_freq(uint16_t freq)
{
    if (freq == 0u) {
        TIM_SetCompare4(TIM2, 0u);
        return;
    }

    if (freq < 100u) {
        freq = 100u;
    } else if (freq > 8000u) {
        freq = 8000u;
    }

    {
        uint16_t arr = (uint16_t)((1000000u / freq) - 1u);
        TIM_SetAutoreload(TIM2, arr);
        TIM_SetCompare4(TIM2, (uint16_t)(arr / 2u));
    }
}

void audio_init(void)
{
    GPIO_InitTypeDef        gpio = {0};
    TIM_OCInitTypeDef       oc = {0};
    TIM_TimeBaseInitTypeDef tb = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    gpio.GPIO_Pin   = N1_BUZZER_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(N1_BUZZER_PORT, &gpio);

    /* 48 MHz / 48 = 1 MHz count clock */
    tb.TIM_Period        = 1000u - 1u;
    tb.TIM_Prescaler     = 48u - 1u;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tb);

    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 0u;
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC4Init(TIM2, &oc);

    TIM_CtrlPWMOutputs(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    head = tail = timer = 0u;
    buzzer_set_freq(0u);
}

void audio_set_enabled(uint8_t on)
{
    enabled = on ? 1u : 0u;

    if (!enabled) {
        head = tail = timer = 0u;
        buzzer_set_freq(0u);
    }
}

uint8_t audio_enabled(void)
{
    return enabled;
}

void audio_note(uint16_t freq, uint8_t frames)
{
    uint8_t next = (uint8_t)((tail + 1u) % SFX_QUEUE_LEN);

    if (!enabled || frames == 0u) {
        return;
    }
    if (next == head) {
        return;             /* queue full: drop the newest note */
    }

    queue[tail].freq   = freq;
    queue[tail].frames = frames;
    tail = next;
}

void audio_sfx(sfx_t id)
{
    if (!enabled) {
        return;
    }

    switch (id) {
    case SFX_MOVE:
        audio_note(1200u, 2u);
        break;
    case SFX_SELECT:
        audio_note(880u, 3u);
        audio_note(1320u, 4u);
        break;
    case SFX_BACK:
        audio_note(700u, 3u);
        audio_note(460u, 4u);
        break;
    case SFX_POINT:
        audio_note(1568u, 2u);
        audio_note(2093u, 3u);
        break;
    case SFX_HIT:
        audio_note(180u, 3u);
        audio_note(120u, 5u);
        break;
    case SFX_OVER:
        audio_note(440u, 5u);
        audio_note(330u, 5u);
        audio_note(220u, 9u);
        break;
    case SFX_WIN:
        audio_note(523u, 4u);
        audio_note(659u, 4u);
        audio_note(784u, 4u);
        audio_note(1046u, 10u);
        break;
    case SFX_FLAP:
        audio_note(620u, 2u);
        audio_note(880u, 2u);
        break;
    case SFX_SHOOT:
        audio_note(1400u, 2u);
        break;
    case SFX_BUMP:
        audio_note(260u, 3u);
        break;
    default:
        break;
    }
}

void audio_update(void)
{
    if (timer > 0u) {
        timer--;
        if (timer == 0u) {
            buzzer_set_freq(0u);
        }
        return;
    }

    if (head != tail) {
        note_t n = queue[head];
        head = (uint8_t)((head + 1u) % SFX_QUEUE_LEN);
        buzzer_set_freq(n.freq);
        timer = n.frames;
    }
}
