/********************************** (C) COPYRIGHT *******************************
 * File Name          : console.h
 * Description        : Launcher API shared by main.c and the USB command layer.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __CONSOLE_H
#define __CONSOLE_H

#include <stdint.h>

#define CONSOLE_UI_MENU     0u
#define CONSOLE_UI_GAME     1u

uint8_t console_ui_state(void);
uint8_t console_current_game(void);
uint8_t console_game_count(void);

/* Copy a cartridge game into the internal game slot.  0 = OK. */
uint8_t console_load_game(uint8_t index);

/* Execute the loaded game slot.  Does not return. */
void console_jump_game(void) __attribute__((noreturn));

void console_return_menu(void);
void console_cart_updated(void);

void console_set_brightness(uint8_t level);
void console_set_sound(uint8_t on);

void console_fill_info(uint8_t out[11]);

/* Re-run SSD1306 init on demand (hot-plug).  Returns 1 when the display ACKs. */
uint8_t console_oled_reinit(void);
void console_fill_scores(uint8_t *out);
void console_reset_scores(void);

#endif /* __CONSOLE_H */
