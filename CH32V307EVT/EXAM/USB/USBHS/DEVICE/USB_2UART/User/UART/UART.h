/********************************** (C) COPYRIGHT *******************************
 * File Name          : UART.h
 * Description        : Eight-channel UART/USB CDC bridge interface.
 *******************************************************************************/
#ifndef __UART_H__
#define __UART_H__

#include "debug.h"

#define UART_PORT_COUNT              8
#define UART_DEFAULT_BAUDRATE        115200
#define UART_RX_DMA_SIZE             2048
#define UART_USB_PACKET_SIZE         512
#define UART_USB_OUT_QUEUE_DEPTH     2

void UART_Bridge_Init(void);
void UART_Bridge_Task(void);
void UART_Bridge_Reset(void);
void UART_Bridge_SetLineCoding(uint8_t port, const uint8_t *coding);
uint8_t *UART_Bridge_GetLineCoding(uint8_t port);
uint8_t *UART_Bridge_GetUsbOutBuffer(uint8_t port);
uint8_t UART_Bridge_UsbOutComplete(uint8_t port, uint16_t len);
void UART_Bridge_UsbInComplete(uint8_t port);

#endif
