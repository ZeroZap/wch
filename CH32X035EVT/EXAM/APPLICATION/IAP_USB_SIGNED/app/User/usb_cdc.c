/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_cdc.c
 * Description        : Minimal USBFS CDC device layer for the application.
 *
 * Copied from EXAM/APPLICATION/WS2812_Panel (that one is verified on hardware).
 * The UART bridge is removed: OUT packets go to usb_cdc_on_packet(), IN packets
 * come from usb_cdc_write().  Endpoint 0 handling is kept close to the vendor
 * code because that is what enumeration depends on.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>
#include "debug.h"
#include "usb_cdc.h"
#include "usb_desc.h"

#ifndef __PACKED
#define __PACKED        __attribute__((packed))
#endif

#define USBFS_UEP_EN(n)         ((uint8_t)(0x01u << (n)))

/* AFIO->CTLR bits, taken from the vendor's ch32x035_usbfs_device.h */
#define USB_IOEN                0x00000080u
#define USB_PHY_V33             0x00000040u
#define UDP_PUE_MASK            0x0000000Cu
#define UDP_PUE_10K             0x00000008u
#define UDP_PUE_1K5             0x0000000Cu
#define UDM_PUE_MASK            0x00000003u

/*---------------------------------------------------------------------------
 * state shared with the interrupt handler
 *---------------------------------------------------------------------------*/
const uint8_t *pUSBFS_Descr;

volatile uint8_t  USBFS_SetupReqCode;
volatile uint8_t  USBFS_SetupReqType;
volatile uint16_t USBFS_SetupReqValue;
volatile uint16_t USBFS_SetupReqIndex;
volatile uint16_t USBFS_SetupReqLen;

volatile uint8_t  USBFS_DevConfig;
volatile uint8_t  USBFS_DevAddr;
volatile uint8_t  USBFS_DevSleepStatus;
volatile uint8_t  USBFS_DevEnumStatus;

__attribute__((aligned(4))) static uint8_t ep0_buf[DEF_UEP0_SIZE];
__attribute__((aligned(4))) static uint8_t ep1_buf[DEF_UEP1_SIZE];
__attribute__((aligned(4))) static uint8_t ep2_buf[DEF_UEP2_SIZE];   /* OUT */
__attribute__((aligned(4))) static uint8_t ep3_buf[DEF_UEP3_SIZE];   /* IN  */

static volatile uint8_t ep3_busy;

/* CDC line coding, only stored so the host's requests are accepted */
static uint8_t line_coding[8] = { 0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00 };

void USBFS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*---------------------------------------------------------------------------
 * init
 *---------------------------------------------------------------------------*/
static void usb_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_16;              /* D- */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_17;              /* D+ */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static void usb_endp_init(void)
{
    USBFSD->UEP4_1_MOD = USBFS_UEP1_TX_EN;
    USBFSD->UEP2_3_MOD = USBFS_UEP2_RX_EN | USBFS_UEP3_TX_EN;

    USBFSD->UEP0_DMA = (uint32_t)ep0_buf;
    USBFSD->UEP1_DMA = (uint32_t)ep1_buf;
    USBFSD->UEP2_DMA = (uint32_t)ep2_buf;
    USBFSD->UEP3_DMA = (uint32_t)ep3_buf;

    USBFSD->UEP0_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_T_RES_NAK;
    USBFSD->UEP2_CTRL_H = USBFS_UEP_R_RES_ACK;

    USBFSD->UEP1_TX_LEN = 0;
    USBFSD->UEP3_TX_LEN = 0;

    USBFSD->UEP1_CTRL_H = USBFS_UEP_T_RES_NAK;
    USBFSD->UEP3_CTRL_H = USBFS_UEP_T_RES_NAK;

    ep3_busy = 0;
}

void usb_cdc_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBFS, ENABLE);

    usb_gpio_init();

    /* tell the PHY whether VBUS is 5V or 3.3V, same as the vendor examples */
    if (PWR_VDD_SupplyVoltage() == PWR_VDD_5V) {
        AFIO->CTLR = (AFIO->CTLR & ~(UDP_PUE_MASK | UDM_PUE_MASK | USB_PHY_V33))
                     | UDP_PUE_10K | USB_IOEN;
    } else {
        AFIO->CTLR = (AFIO->CTLR & ~(UDP_PUE_MASK | UDM_PUE_MASK))
                     | USB_PHY_V33 | UDP_PUE_1K5 | USB_IOEN;
    }

    USBFSD->BASE_CTRL = 0x00;
    usb_endp_init();
    USBFSD->DEV_ADDR = 0x00;
    USBFSD->BASE_CTRL = USBFS_UC_DEV_PU_EN | USBFS_UC_INT_BUSY | USBFS_UC_DMA_EN;
    USBFSD->INT_FG = 0xFF;
    USBFSD->UDEV_CTRL = USBFS_UD_PD_DIS | USBFS_UD_PORT_EN;
    USBFSD->INT_EN = USBFS_UIE_SUSPEND | USBFS_UIE_BUS_RST | USBFS_UIE_TRANSFER;

    NVIC_EnableIRQ(USBFS_IRQn);
}

uint8_t usb_cdc_ready(void)
{
    return USBFS_DevEnumStatus;
}

/*---------------------------------------------------------------------------
 * IN endpoint
 *---------------------------------------------------------------------------*/
uint8_t usb_cdc_write(const uint8_t *buf, uint16_t len)
{
    if ((USBFS_DevEnumStatus == 0) || ep3_busy || (len == 0) ||
        (len > DEF_UEP3_SIZE)) {
        return 1;
    }

    memcpy(ep3_buf, buf, len);
    USBFSD->UEP3_TX_LEN = len;
    USBFSD->UEP3_CTRL_H = (USBFSD->UEP3_CTRL_H & ~USBFS_UEP_T_RES_MASK)
                          | USBFS_UEP_T_RES_ACK;
    ep3_busy = 1;

    return 0;
}

void usb_cdc_log(const char *s)
{
    char     line[64];
    uint16_t n = 0;

    while (*s && (n < (sizeof(line) - 3u))) {
        line[n++] = *s++;
    }
    line[n++] = '\r';
    line[n++] = '\n';

    (void)usb_cdc_write((const uint8_t *)line, n);
}

/*---------------------------------------------------------------------------
 * interrupt
 *---------------------------------------------------------------------------*/
void USBFS_IRQHandler(void)
{
    uint8_t  intflag, intst, errflag;
    uint16_t len;

    intflag = USBFSD->INT_FG;
    intst   = USBFSD->INT_ST;

    if (intflag & USBFS_UIF_TRANSFER) {
        switch (intst & USBFS_UIS_TOKEN_MASK) {
        case USBFS_UIS_TOKEN_IN:
            switch (intst & (USBFS_UIS_TOKEN_MASK | USBFS_UIS_ENDP_MASK)) {
            case USBFS_UIS_TOKEN_IN | 0:            /* EP0 */
                if (USBFS_SetupReqLen == 0) {
                    USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_R_RES_MASK)
                                          | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_ACK;
                }
                if ((USBFS_SetupReqType & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD) {
                    switch (USBFS_SetupReqCode) {
                    case USB_GET_DESCRIPTOR:
                        len = (USBFS_SetupReqLen >= DEF_UEP0_SIZE)
                              ? DEF_UEP0_SIZE : USBFS_SetupReqLen;
                        memcpy(ep0_buf, pUSBFS_Descr, len);
                        USBFS_SetupReqLen -= len;
                        pUSBFS_Descr += len;
                        USBFSD->UEP0_TX_LEN = len;
                        USBFSD->UEP0_CTRL_H ^= USBFS_UEP_T_TOG;
                        break;

                    case USB_SET_ADDRESS:
                        USBFSD->DEV_ADDR = (USBFSD->DEV_ADDR & USBFS_UDA_GP_BIT)
                                           | USBFS_DevAddr;
                        break;

                    default:
                        break;
                    }
                }
                break;

            case USBFS_UIS_TOKEN_IN | 3:            /* EP3 bulk IN done */
                USBFSD->UEP3_CTRL_H ^= USBFS_UEP_T_TOG;
                USBFSD->UEP3_CTRL_H = (USBFSD->UEP3_CTRL_H & ~USBFS_UEP_T_RES_MASK)
                                      | USBFS_UEP_T_RES_NAK;
                ep3_busy = 0;
                break;

            case USBFS_UIS_TOKEN_IN | 1:            /* EP1 notification done */
                USBFSD->UEP1_CTRL_H ^= USBFS_UEP_T_TOG;
                USBFSD->UEP1_CTRL_H = (USBFSD->UEP1_CTRL_H & ~USBFS_UEP_T_RES_MASK)
                                      | USBFS_UEP_T_RES_NAK;
                break;

            default:
                break;
            }
            break;

        case USBFS_UIS_TOKEN_OUT:
            switch (intst & (USBFS_UIS_TOKEN_MASK | USBFS_UIS_ENDP_MASK)) {
            case USBFS_UIS_TOKEN_OUT | 0:           /* EP0 */
                len = USBFSD->RX_LEN;
                if (intst & USBFS_UIS_TOG_OK) {
                    if ((USBFS_SetupReqType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {
                        USBFS_SetupReqLen = 0;
                        if (USBFS_SetupReqCode == CDC_SET_LINE_CODING) {
                            uint16_t i;
                            for (i = 0; (i < len) && (i < 7u); i++) {
                                line_coding[i] = ep0_buf[i];
                            }
                        }
                    } else {
                        /* no standard OUT data expected */
                    }
                    if (USBFS_SetupReqLen == 0) {
                        USBFSD->UEP0_TX_LEN  = 0;
                        USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_T_RES_MASK)
                                              | USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK;
                    }
                }
                break;

            case USBFS_UIS_TOKEN_OUT | 2:           /* EP2 bulk OUT: panel data */
                USBFSD->UEP2_CTRL_H ^= USBFS_UEP_R_TOG;
                len = USBFSD->RX_LEN;
                if (len > DEF_UEP2_SIZE) {
                    len = DEF_UEP2_SIZE;
                }
                usb_cdc_on_packet(ep2_buf, len);
                break;

            default:
                break;
            }
            break;

        case USBFS_UIS_TOKEN_SETUP:
            USBFSD->UEP0_CTRL_H = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_NAK
                                  | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_NAK;

            USBFS_SetupReqType  = ((PUSB_SETUP_REQ)ep0_buf)->bRequestType;
            USBFS_SetupReqCode  = ((PUSB_SETUP_REQ)ep0_buf)->bRequest;
            USBFS_SetupReqLen   = ((PUSB_SETUP_REQ)ep0_buf)->wLength;
            USBFS_SetupReqValue = ((PUSB_SETUP_REQ)ep0_buf)->wValue;
            USBFS_SetupReqIndex = ((PUSB_SETUP_REQ)ep0_buf)->wIndex;
            len = 0;
            errflag = 0;

            if ((USBFS_SetupReqType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {
                if (USBFS_SetupReqType & USB_REQ_TYP_CLASS) {
                    switch (USBFS_SetupReqCode) {
                    case CDC_GET_LINE_CODING:
                        pUSBFS_Descr = line_coding;
                        len = 7;
                        break;

                    case CDC_SET_LINE_CODING:
                    case CDC_SET_LINE_CTLSTE:
                    case CDC_SEND_BREAK:
                        break;

                    default:
                        errflag = 0xFF;
                        break;
                    }
                } else {
                    errflag = 0xFF;
                }

                if (errflag != 0xFF) {
                    len = (USBFS_SetupReqLen >= DEF_UEP0_SIZE)
                          ? DEF_UEP0_SIZE : USBFS_SetupReqLen;
                    memcpy(ep0_buf, pUSBFS_Descr, len);
                    pUSBFS_Descr += len;
                }
            } else {
                switch (USBFS_SetupReqCode) {
                case USB_GET_DESCRIPTOR:
                    switch ((uint8_t)(USBFS_SetupReqValue >> 8)) {
                    case USB_DESCR_TYP_DEVICE:
                        pUSBFS_Descr = MyDevDescr;
                        len = DEF_DEV_DESC_LEN;
                        break;

                    case USB_DESCR_TYP_CONFIG:
                        pUSBFS_Descr = MyCfgDescr;
                        len = DEF_CFG_DESC_LEN;
                        break;

                    case USB_DESCR_TYP_STRING:
                        switch ((uint8_t)(USBFS_SetupReqValue & 0xFF)) {
                        case 0:
                            pUSBFS_Descr = MyLangDescr;
                            len = DEF_LANG_DESC_LEN;
                            break;
                        case 1:
                            pUSBFS_Descr = MyManuInfo;
                            len = DEF_MANU_DESC_LEN;
                            break;
                        case 2:
                            pUSBFS_Descr = MyProdInfo;
                            len = DEF_PROD_DESC_LEN;
                            break;
                        case 3:
                            pUSBFS_Descr = MySerNumInfo;
                            len = DEF_SN_DESC_LEN;
                            break;
                        default:
                            errflag = 0xFF;
                            break;
                        }
                        break;

                    default:
                        errflag = 0xFF;
                        break;
                    }

                    if (errflag != 0xFF) {
                        if (USBFS_SetupReqLen > len) {
                            USBFS_SetupReqLen = len;
                        }
                        len = (USBFS_SetupReqLen >= DEF_UEP0_SIZE)
                              ? DEF_UEP0_SIZE : USBFS_SetupReqLen;
                        memcpy(ep0_buf, pUSBFS_Descr, len);
                        pUSBFS_Descr += len;
                    }
                    break;

                case USB_SET_ADDRESS:
                    USBFS_DevAddr = (uint8_t)(USBFS_SetupReqValue & 0xFF);
                    break;

                case USB_GET_CONFIGURATION:
                    ep0_buf[0] = USBFS_DevConfig;
                    if (USBFS_SetupReqLen > 1) {
                        USBFS_SetupReqLen = 1;
                    }
                    break;

                case USB_SET_CONFIGURATION:
                    USBFS_DevConfig = (uint8_t)(USBFS_SetupReqValue & 0xFF);
                    USBFS_DevEnumStatus = 0x01;
                    break;

                case USB_CLEAR_FEATURE:
                case USB_SET_FEATURE:
                    /* only ENDPOINT_HALT is meaningful here; just recover */
                    if ((USBFS_SetupReqType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
                        switch ((uint8_t)(USBFS_SetupReqIndex & 0xFF)) {
                        case 0x81:
                            USBFSD->UEP1_CTRL_H = USBFS_UEP_T_RES_NAK;
                            break;
                        case 0x02:
                            USBFSD->UEP2_CTRL_H = USBFS_UEP_R_RES_ACK;
                            break;
                        case 0x83:
                            USBFSD->UEP3_CTRL_H = USBFS_UEP_T_RES_NAK;
                            ep3_busy = 0;
                            break;
                        default:
                            break;
                        }
                    }
                    break;

                case USB_GET_INTERFACE:
                    ep0_buf[0] = 0x00;
                    if (USBFS_SetupReqLen > 1) {
                        USBFS_SetupReqLen = 1;
                    }
                    break;

                case USB_SET_INTERFACE:
                    break;

                case USB_GET_STATUS:
                    ep0_buf[0] = 0x00;
                    ep0_buf[1] = 0x00;
                    if (USBFS_SetupReqLen > 2) {
                        USBFS_SetupReqLen = 2;
                    }
                    break;

                default:
                    errflag = 0xFF;
                    break;
                }
            }

            if (errflag == 0xFF) {
                USBFSD->UEP0_CTRL_H = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_STALL
                                      | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_STALL;
            } else if (USBFS_SetupReqType & 0x80) {
                len = (USBFS_SetupReqLen > DEF_UEP0_SIZE)
                      ? DEF_UEP0_SIZE : USBFS_SetupReqLen;
                USBFS_SetupReqLen -= len;
                USBFSD->UEP0_TX_LEN  = len;
                USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_T_RES_MASK)
                                      | USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK;
            } else if (USBFS_SetupReqLen == 0) {
                USBFSD->UEP0_TX_LEN  = 0;
                USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_T_RES_MASK)
                                      | USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK;
            } else {
                USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_R_RES_MASK)
                                      | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_ACK;
            }
            break;

        case USBFS_UIS_TOKEN_SOF:
        default:
            break;
        }

        USBFSD->INT_FG = USBFS_UIF_TRANSFER;
    } else if (intflag & USBFS_UIF_BUS_RST) {
        USBFS_DevConfig = 0;
        USBFS_DevAddr = 0;
        USBFS_DevSleepStatus = 0;
        USBFS_DevEnumStatus = 0;

        USBFSD->DEV_ADDR = 0;
        usb_endp_init();
        USBFSD->INT_FG = USBFS_UIF_BUS_RST;
    } else if (intflag & USBFS_UIF_SUSPEND) {
        USBFSD->INT_FG = USBFS_UIF_SUSPEND;
        Delay_Us(10);
        if (USBFSD->MIS_ST & USBFS_UMS_SUSPEND) {
            USBFS_DevSleepStatus |= 0x02;
        } else {
            USBFS_DevSleepStatus &= ~0x02;
        }
    } else {
        USBFSD->INT_FG = intflag;
    }
}
