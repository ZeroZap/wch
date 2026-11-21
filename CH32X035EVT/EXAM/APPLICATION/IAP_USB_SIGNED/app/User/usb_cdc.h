/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_cdc.h
 * Description        : Minimal USBFS CDC (virtual COM port) device layer.
 *
 * Copied from EXAM/APPLICATION/WS2812_Panel (same device layer, verified on
 * hardware) and reduced to the hooks the bootloader needs.
 *
 * Data plane
 *   host -> device : EP2 OUT, 64 bytes per packet.  Every packet is handed to
 *                    usb_cdc_on_packet() straight from the interrupt.
 *   device -> host : EP3 IN, 64 bytes per transfer, single buffered.
 *
 * usb_cdc_on_packet() must be provided by the application (iap_proto.c).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __USB_CDC_H
#define __USB_CDC_H

#include "ch32x035.h"
#include "ch32x035_usb.h"

/* enable the port, called once after the clocks are up */
void     usb_cdc_init(void);

/* 1 once the host has sent SET_CONFIGURATION */
uint8_t  usb_cdc_ready(void);

/* queue one packet on the IN endpoint; 0 = accepted, 1 = busy/rejected */
uint8_t  usb_cdc_write(const uint8_t *buf, uint16_t len);

/* send a '\r\n' terminated text line (logs); silently dropped when busy */
void     usb_cdc_log(const char *s);

/* provided by the application: one 64-byte OUT packet, called in interrupt */
void     usb_cdc_on_packet(const uint8_t *buf, uint16_t len);

#endif /* __USB_CDC_H */
