/********************************** (C) COPYRIGHT *******************************
 * File Name          : input.h
 * Description        : Six-button input with debounce and per-frame edges.
 *
 * The buttons are wired to GND and use the internal pull-up, so a pressed
 * button reads 0.  input_update() is called once per 60 Hz frame.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

#define BTN_UP      0x01u
#define BTN_DOWN    0x02u
#define BTN_LEFT    0x04u
#define BTN_RIGHT   0x08u
#define BTN_A       0x10u
#define BTN_B       0x20u

#define BTN_UP_IDX      0
#define BTN_DOWN_IDX    1
#define BTN_LEFT_IDX    2
#define BTN_RIGHT_IDX   3
#define BTN_A_IDX       4
#define BTN_B_IDX       5
#define BTN_COUNT       6

typedef struct {
    uint8_t held;           /* 1 while the button is down              */
    uint8_t pressed;        /* 1 only on the frame it goes down        */
    uint8_t released;       /* 1 only on the frame it comes back up    */
    uint8_t held_frames[BTN_COUNT];
} input_state_t;

void input_init(void);

/* Sample the pins, debounce, and update the edge/held fields. */
void input_update(input_state_t *st);

/* Raw active-high mask, useful for the menu's "hold both" gesture. */
uint8_t input_raw(void);

#endif /* __INPUT_H */
