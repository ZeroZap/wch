/********************************** (C) COPYRIGHT *******************************
 * File Name          : audio.h
 * Description        : Passive buzzer on PA3 / TIM2_CH4 with a small note queue.
 *
 * The same pin also drives the IR LED through a second MOSFET (38 kHz is
 * outside the buzzer's pass band, 1-4 kHz is outside the IR receiver's band),
 * so no extra pin is spent on the IR blaster.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __AUDIO_H
#define __AUDIO_H

#include <stdint.h>

typedef enum {
    SFX_MOVE = 0,
    SFX_SELECT,
    SFX_BACK,
    SFX_POINT,
    SFX_HIT,
    SFX_OVER,
    SFX_WIN,
    SFX_FLAP,
    SFX_SHOOT,
    SFX_BUMP,
    SFX_COUNT
} sfx_t;

void    audio_init(void);
void    audio_set_enabled(uint8_t on);
uint8_t audio_enabled(void);

/* Queue one note; freq 0 inserts a rest.  frames are 60 Hz frames. */
void audio_note(uint16_t freq, uint8_t frames);
void audio_sfx(sfx_t id);

/* Advance the queue, call once per frame. */
void audio_update(void);

#endif /* __AUDIO_H */
