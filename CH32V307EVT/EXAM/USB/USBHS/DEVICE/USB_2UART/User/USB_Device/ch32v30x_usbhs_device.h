/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v30x_usbhs_device.h
 * Description        : USBHS device interface for eight CDC ACM ports.
 *******************************************************************************/
#ifndef __CH32V30X_USBHS_DEVICE_H__
#define __CH32V30X_USBHS_DEVICE_H__

#include "debug.h"
#include "ch32v30x_usb.h"
#include "usb_desc.h"
#include "UART.h"

#define pUSBHS_SetupReqPak            ((PUSB_SETUP_REQ)USBHS_EP0_Buf)
#define DEF_UEP_IN                    0x80
#define DEF_UEP_OUT                   0x00
#define DEF_UEP_NUM                   16
#define DEF_UEP0                      0

#define USBHSD_UEP_CFG_BASE           0x40023410
#define USBHSD_UEP_RXDMA_BASE         0x40023420
#define USBHSD_UEP_TXDMA_BASE         0x4002345C
#define USBHSD_UEP_MAXLEN_BASE        0x4002349C
#define USBHSD_UEP_TXLEN_BASE         0x400234DC
#define USBHSD_UEP_TXCTL_BASE         0x400234DE
#define USBHSD_UEP_TX_EN(n)           ((uint32_t)1 << (n))
#define USBHSD_UEP_RX_EN(n)           ((uint32_t)1 << ((n) + 16))
#define USBHSD_UEP_RXDMA(n)           (*((volatile uint32_t *)(USBHSD_UEP_RXDMA_BASE + ((n) - 1) * 4)))
#define USBHSD_UEP_TXDMA(n)           (*((volatile uint32_t *)(USBHSD_UEP_TXDMA_BASE + ((n) - 1) * 4)))
#define USBHSD_UEP_MAXLEN(n)          (*((volatile uint16_t *)(USBHSD_UEP_MAXLEN_BASE + ((n) - 1) * 4)))
#define USBHSD_UEP_TLEN(n)            (*((volatile uint16_t *)(USBHSD_UEP_TXLEN_BASE + ((n) - 1) * 4)))
#define USBHSD_UEP_TXCTRL(n)          (*((volatile uint8_t *)(USBHSD_UEP_TXCTL_BASE + ((n) - 1) * 4)))
#define USBHSD_UEP_RXCTRL(n)          (*((volatile uint8_t *)(USBHSD_UEP_TXCTL_BASE + ((n) - 1) * 4 + 1)))

#define USBHS_SPEED_TYPE_MASK         0x03
#define USBHS_SPEED_FULL              0x00
#define USBHS_SPEED_HIGH              0x01

extern volatile uint8_t USBHS_DevEnumStatus;
extern volatile uint16_t USBHS_DevMaxPackLen;
extern __attribute__((aligned(4))) uint8_t USBHS_EP0_Buf[];

void USBHS_RCC_Init(void);
void USBHS_Device_Init(FunctionalState sta);
uint8_t USBHS_PortDataUp(uint8_t port, uint8_t *buffer, uint16_t len);
void USBHS_PortOutSetAck(uint8_t port, FunctionalState ack);

#endif
