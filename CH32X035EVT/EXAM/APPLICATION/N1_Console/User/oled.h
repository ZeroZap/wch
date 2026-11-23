/********************************** (C) COPYRIGHT *******************************
 * File Name          : oled.h
 * Description        : SSD1306 128x64 OLED on a software I2C bus.
 *
 * Pins (LQFP48 only): PA10 = SCL, PA11 = SDA, 4.7k pull-ups to 3.3V.
 * Software I2C is used so a missing display cannot hang the hardware I2C
 * peripheral; every transaction returns NACK and the console just continues
 * with the WS2812 panel.  On TSSOP20 this whole module compiles to no-ops.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

uint8_t oled_init(void);        /* 1 = display ACKed and initialised */
uint8_t oled_present(void);

void oled_clear(void);
void oled_power(uint8_t on);
void oled_set_brightness(uint8_t level);   /* 0..255 -> SSD1306 contrast */
void oled_pixel(uint8_t x, uint8_t y, uint8_t on);
void oled_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void oled_text(uint8_t x, uint8_t page, const char *s);
void oled_text_center(uint8_t page, const char *s);
void oled_num3(uint8_t x, uint8_t page, uint16_t value);

void oled_refresh(void);        /* all 8 pages, ~23 ms at 400 kHz */
void oled_refresh_page(uint8_t page);

/* Probe the bus for the display.  Fills out[] with the 7-bit addresses that
 * ACK and returns the count (0 = nothing on the bus). */
uint8_t oled_scan(uint8_t *out, uint8_t max);

#endif /* __OLED_H */
