/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v30x_usbhs_device.c
 * Description        : USBHS composite device with eight CDC ACM data ports.
 *******************************************************************************/
#include "ch32v30x_usbhs_device.h"
#include <string.h>


static const uint8_t *SetupDescriptor;
static volatile uint8_t SetupRequest;
static volatile uint8_t SetupType;
static volatile uint16_t SetupValue;
static volatile uint16_t SetupIndex;
static volatile uint16_t SetupLength;
static volatile uint8_t DeviceAddress;
static volatile uint8_t DeviceConfiguration;
static volatile uint8_t PendingLinePort = 0xFF;
static volatile uint8_t DeviceSpeed;

volatile uint8_t USBHS_DevEnumStatus;
volatile uint16_t USBHS_DevMaxPackLen = DEF_USBD_HS_PACK_SIZE;
__attribute__((aligned(4))) uint8_t USBHS_EP0_Buf[DEF_USBD_UEP0_SIZE];
static volatile uint8_t EndpointBusy[DEF_UEP_NUM];

void USBHS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

static uint8_t ControlInterfaceToPort(uint16_t interface)
{
    if(interface < 16 && (interface & 1) == 0)
    {
        return (uint8_t)(interface >> 1);
    }
    return 0xFF;
}

void USBHS_RCC_Init(void)
{
    RCC_USBCLK48MConfig(RCC_USBCLK48MCLKSource_USBPHY);
    RCC_USBHSPLLCLKConfig(RCC_HSBHSPLLCLKSource_HSE);
    RCC_USBHSConfig(RCC_USBPLL_Div2);
    RCC_USBHSPLLCKREFCLKConfig(RCC_USBHSPLLCKREFCLK_4M);
    RCC_USBHSPHYPLLALIVEcmd(ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBHS, ENABLE);
}

static void USBHS_Device_Endp_Init(void)
{
    uint8_t endpoint;

    USBHSD->ENDP_CONFIG = 0x01FE01FE;
    USBHSD->ENDP_TYPE = 0;
    USBHSD->BUF_MODE = 0;
    USBHSD->UEP0_MAX_LEN = DEF_USBD_UEP0_SIZE;
    USBHSD->UEP0_DMA = (uint32_t)USBHS_EP0_Buf;
    USBHSD->UEP0_TX_LEN = 0;
    USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_RES_NAK;
    USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_RES_ACK;

    for(endpoint = 1; endpoint <= UART_PORT_COUNT; endpoint++)
    {
        USBHSD_UEP_MAXLEN(endpoint) = DEF_USBD_HS_PACK_SIZE;
        USBHSD_UEP_RXDMA(endpoint) = (uint32_t)UART_Bridge_GetUsbOutBuffer(endpoint - 1);
        USBHSD_UEP_TXDMA(endpoint) = 0;
        USBHSD_UEP_TLEN(endpoint) = 0;
        USBHSD_UEP_TXCTRL(endpoint) = USBHS_UEP_T_RES_NAK;
        USBHSD_UEP_RXCTRL(endpoint) = USBHS_UEP_R_RES_ACK;
        EndpointBusy[endpoint] = 0;
    }
}

void USBHS_Device_Init(FunctionalState sta)
{
    if(sta == ENABLE)
    {
        USBHSD->CONTROL = USBHS_UC_CLR_ALL | USBHS_UC_RESET_SIE;
        Delay_Us(10);
        USBHSD->CONTROL &= ~USBHS_UC_RESET_SIE;
        USBHSD->HOST_CTRL = USBHS_UH_PHY_SUSPENDM;
        USBHSD->CONTROL = USBHS_UC_DMA_EN | USBHS_UC_INT_BUSY | USBHS_UC_SPEED_HIGH;
        USBHSD->INT_EN = USBHS_UIE_SETUP_ACT | USBHS_UIE_TRANSFER |
                         USBHS_UIE_DETECT | USBHS_UIE_SUSPEND;
        USBHS_Device_Endp_Init();
        USBHSD->CONTROL |= USBHS_UC_DEV_PU_EN;
        NVIC_EnableIRQ(USBHS_IRQn);
    }
    else
    {
        USBHSD->CONTROL = USBHS_UC_CLR_ALL | USBHS_UC_RESET_SIE;
        Delay_Us(10);
        USBHSD->CONTROL = 0;
        NVIC_DisableIRQ(USBHS_IRQn);
    }
}

uint8_t USBHS_PortDataUp(uint8_t port, uint8_t *buffer, uint16_t len)
{
    uint8_t endpoint = port + 1;

    if(port >= UART_PORT_COUNT || EndpointBusy[endpoint] || len > USBHS_DevMaxPackLen)
    {
        return 1;
    }
    USBHSD_UEP_TXDMA(endpoint) = (uint32_t)buffer;
    USBHSD_UEP_TLEN(endpoint) = len;
    USBHSD_UEP_TXCTRL(endpoint) =
        (USBHSD_UEP_TXCTRL(endpoint) & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_ACK;
    EndpointBusy[endpoint] = 1;
    return 0;
}

void USBHS_PortOutSetAck(uint8_t port, FunctionalState ack)
{
    uint8_t endpoint = port + 1;
    if(port >= UART_PORT_COUNT)
    {
        return;
    }
    USBHSD_UEP_RXCTRL(endpoint) =
        (USBHSD_UEP_RXCTRL(endpoint) & ~USBHS_UEP_R_RES_MASK) |
        ((ack == ENABLE) ? USBHS_UEP_R_RES_ACK : USBHS_UEP_R_RES_NAK);
}

static void EndpointInComplete(uint8_t endpoint)
{
    USBHSD_UEP_TXCTRL(endpoint) =
        (USBHSD_UEP_TXCTRL(endpoint) & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_NAK;
    USBHSD_UEP_TXCTRL(endpoint) ^= USBHS_UEP_T_TOG_DATA1;
    EndpointBusy[endpoint] = 0;
    UART_Bridge_UsbInComplete(endpoint - 1);
}

static void EndpointOutComplete(uint8_t endpoint)
{
    uint8_t port = endpoint - 1;
    uint8_t can_accept_more;

    USBHSD_UEP_RXCTRL(endpoint) ^= USBHS_UEP_R_TOG_DATA1;
    can_accept_more = UART_Bridge_UsbOutComplete(port, USBHSD->RX_LEN);
    USBHSD_UEP_RXDMA(endpoint) = (uint32_t)UART_Bridge_GetUsbOutBuffer(port);
    USBHS_PortOutSetAck(port, can_accept_more ? ENABLE : DISABLE);
}

static uint8_t PrepareDescriptor(uint16_t *length)
{
    uint8_t type = (uint8_t)(SetupValue >> 8);
    uint8_t index = (uint8_t)SetupValue;

    switch(type)
    {
        case USB_DESCR_TYP_DEVICE:
            SetupDescriptor = MyDevDescr;
            *length = DEF_USBD_DEVICE_DESC_LEN;
            break;
        case USB_DESCR_TYP_CONFIG:
            if((USBHSD->SPEED_TYPE & USBHS_SPEED_TYPE_MASK) == USBHS_SPEED_HIGH)
            {
                DeviceSpeed = USBHS_SPEED_HIGH;
                USBHS_DevMaxPackLen = DEF_USBD_HS_PACK_SIZE;
                for(uint8_t endpoint = 1; endpoint <= UART_PORT_COUNT; endpoint++)
                {
                    USBHSD_UEP_MAXLEN(endpoint) = USBHS_DevMaxPackLen;
                }
                SetupDescriptor = MyCfgDescr_HS;
                *length = DEF_USBD_CONFIG_HS_DESC_LEN;
            }
            else
            {
                DeviceSpeed = USBHS_SPEED_FULL;
                USBHS_DevMaxPackLen = DEF_USBD_FS_PACK_SIZE;
                for(uint8_t endpoint = 1; endpoint <= UART_PORT_COUNT; endpoint++)
                {
                    USBHSD_UEP_MAXLEN(endpoint) = USBHS_DevMaxPackLen;
                }
                SetupDescriptor = MyCfgDescr_FS;
                *length = DEF_USBD_CONFIG_FS_DESC_LEN;
            }
            break;
        case USB_DESCR_TYP_STRING:
            if(index == 0)
            {
                SetupDescriptor = MyLangDescr;
                *length = DEF_USBD_LANG_DESC_LEN;
            }
            else if(index == 1)
            {
                SetupDescriptor = MyManuInfo;
                *length = DEF_USBD_MANU_DESC_LEN;
            }
            else if(index == 2)
            {
                SetupDescriptor = MyProdInfo;
                *length = DEF_USBD_PROD_DESC_LEN;
            }
            else if(index == 3)
            {
                SetupDescriptor = MySerNumInfo;
                *length = DEF_USBD_SN_DESC_LEN;
            }
            else if(index >= 4 && index <= 11)
            {
                SetupDescriptor = MyPortInfo[index - 4];
                *length = MyPortInfo[index - 4][0];
            }
            else
            {
                return 0;
            }
            break;
        case USB_DESCR_TYP_QUALIF:
            SetupDescriptor = MyQuaDesc;
            *length = DEF_USBD_QUALFY_DESC_LEN;
            break;
        case USB_DESCR_TYP_SPEED:
            if(DeviceSpeed == USBHS_SPEED_HIGH)
            {
                memcpy(&TAB_USB_HS_OSC_DESC[2], &MyCfgDescr_FS[2], DEF_USBD_CONFIG_FS_DESC_LEN - 2);
                SetupDescriptor = TAB_USB_HS_OSC_DESC;
                *length = DEF_USBD_CONFIG_FS_DESC_LEN;
            }
            else
            {
                memcpy(&TAB_USB_FS_OSC_DESC[2], &MyCfgDescr_HS[2], DEF_USBD_CONFIG_HS_DESC_LEN - 2);
                SetupDescriptor = TAB_USB_FS_OSC_DESC;
                *length = DEF_USBD_CONFIG_HS_DESC_LEN;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

static void HandleSetup(void)
{
    uint16_t length = 0;
    uint8_t error = 0;
    uint8_t port;

    USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_NAK;
    USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_NAK;
    SetupType = pUSBHS_SetupReqPak->bRequestType;
    SetupRequest = pUSBHS_SetupReqPak->bRequest;
    SetupValue = pUSBHS_SetupReqPak->wValue;
    SetupIndex = pUSBHS_SetupReqPak->wIndex;
    SetupLength = pUSBHS_SetupReqPak->wLength;
    PendingLinePort = 0xFF;
    SetupDescriptor = USBHS_EP0_Buf;

    if((SetupType & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD)
    {
        switch(SetupRequest)
        {
            case USB_GET_DESCRIPTOR:
                if(!PrepareDescriptor(&length)) error = 1;
                break;
            case USB_SET_ADDRESS:
                DeviceAddress = (uint8_t)SetupValue;
                break;
            case USB_GET_CONFIGURATION:
                USBHS_EP0_Buf[0] = DeviceConfiguration;
                length = 1;
                break;
            case USB_SET_CONFIGURATION:
                DeviceConfiguration = (uint8_t)SetupValue;
                USBHS_DevEnumStatus = DeviceConfiguration ? 1 : 0;
                break;
            case USB_GET_INTERFACE:
                USBHS_EP0_Buf[0] = 0;
                length = 1;
                break;
            case USB_SET_INTERFACE:
                break;
            case USB_GET_STATUS:
                USBHS_EP0_Buf[0] = 0;
                USBHS_EP0_Buf[1] = 0;
                length = 2;
                break;
            case USB_CLEAR_FEATURE:
                if((SetupType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP &&
                   SetupValue == USB_REQ_FEAT_ENDP_HALT)
                {
                    uint8_t endpoint = (uint8_t)SetupIndex & 0x0F;
                    if(endpoint == 0 || endpoint > UART_PORT_COUNT)
                    {
                        error = 1;
                    }
                    else if(SetupIndex & DEF_UEP_IN)
                    {
                        USBHSD_UEP_TXCTRL(endpoint) = USBHS_UEP_T_RES_NAK;
                    }
                    else
                    {
                        USBHSD_UEP_RXCTRL(endpoint) = USBHS_UEP_R_RES_ACK;
                    }
                }
                else
                {
                    error = 1;
                }
                break;
            case USB_SET_FEATURE:
                if((SetupType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP &&
                   SetupValue == USB_REQ_FEAT_ENDP_HALT)
                {
                    uint8_t endpoint = (uint8_t)SetupIndex & 0x0F;
                    if(endpoint == 0 || endpoint > UART_PORT_COUNT)
                    {
                        error = 1;
                    }
                    else if(SetupIndex & DEF_UEP_IN)
                    {
                        USBHSD_UEP_TXCTRL(endpoint) = USBHS_UEP_T_RES_STALL;
                    }
                    else
                    {
                        USBHSD_UEP_RXCTRL(endpoint) = USBHS_UEP_R_RES_STALL;
                    }
                }
                else
                {
                    error = 1;
                }
                break;
            default:
                error = 1;
                break;
        }
    }
    else if((SetupType & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS)
    {
        port = ControlInterfaceToPort(SetupIndex);
        if(port >= UART_PORT_COUNT)
        {
            error = 1;
        }
        else
        {
            switch(SetupRequest)
            {
                case CDC_GET_LINE_CODING:
                    SetupDescriptor = UART_Bridge_GetLineCoding(port);
                    length = 7;
                    break;
                case CDC_SET_LINE_CODING:
                    PendingLinePort = port;
                    break;
                case CDC_SET_LINE_CTLSTE:
                case CDC_SEND_BREAK:
                    break;
                default:
                    error = 1;
                    break;
            }
        }
    }
    else
    {
        error = 1;
    }

    if(error)
    {
        USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_STALL;
        USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_STALL;
    }
    else if(SetupType & DEF_UEP_IN)
    {
        if(SetupLength > length) SetupLength = length;
        length = SetupLength > DEF_USBD_UEP0_SIZE ? DEF_USBD_UEP0_SIZE : SetupLength;
        if(SetupDescriptor != 0 && length != 0)
        {
            memcpy(USBHS_EP0_Buf, SetupDescriptor, length);
            SetupDescriptor += length;
        }
        SetupLength -= length;
        USBHSD->UEP0_TX_LEN = length;
        USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
    }
    else if(SetupLength == 0)
    {
        USBHSD->UEP0_TX_LEN = 0;
        USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
    }
    else
    {
        USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_ACK;
    }
}

void USBHS_IRQHandler(void)
{
    uint8_t flags = USBHSD->INT_FG;
    uint8_t status = USBHSD->INT_ST;
    uint8_t endpoint = status & USBHS_UIS_ENDP_MASK;
    uint16_t length;

    if(flags & USBHS_UIF_TRANSFER)
    {
        if((status & USBHS_UIS_TOKEN_MASK) == USBHS_UIS_TOKEN_IN)
        {
            if(endpoint == 0)
            {
                if(SetupLength == 0)
                {
                    USBHSD->UEP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_ACK;
                }
                if(SetupRequest == USB_GET_DESCRIPTOR && SetupLength)
                {
                    length = SetupLength > DEF_USBD_UEP0_SIZE ? DEF_USBD_UEP0_SIZE : SetupLength;
                    memcpy(USBHS_EP0_Buf, SetupDescriptor, length);
                    SetupDescriptor += length;
                    SetupLength -= length;
                    USBHSD->UEP0_TX_LEN = length;
                    USBHSD->UEP0_TX_CTRL ^= USBHS_UEP_T_TOG_DATA1;
                }
                else if(SetupRequest == USB_SET_ADDRESS)
                {
                    USBHSD->DEV_AD = DeviceAddress;
                }
                else
                {
                    USBHSD->UEP0_TX_LEN = 0;
                }
            }
            else if(endpoint <= UART_PORT_COUNT)
            {
                EndpointInComplete(endpoint);
            }
        }
        else if((status & USBHS_UIS_TOKEN_MASK) == USBHS_UIS_TOKEN_OUT &&
                (status & USBHS_UIS_TOG_OK))
        {
            if(endpoint == 0)
            {
                if(PendingLinePort < UART_PORT_COUNT && SetupRequest == CDC_SET_LINE_CODING &&
                   USBHSD->RX_LEN == 7)
                {
                    UART_Bridge_SetLineCoding(PendingLinePort, USBHS_EP0_Buf);
                }
                PendingLinePort = 0xFF;
                SetupLength = 0;
                USBHSD->UEP0_TX_LEN = 0;
                USBHSD->UEP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
            }
            else if(endpoint <= UART_PORT_COUNT)
            {
                EndpointOutComplete(endpoint);
            }
        }
        USBHSD->INT_FG = USBHS_UIF_TRANSFER;
    }
    else if(flags & USBHS_UIF_SETUP_ACT)
    {
        HandleSetup();
        USBHSD->INT_FG = USBHS_UIF_SETUP_ACT;
    }
    else if(flags & USBHS_UIF_BUS_RST)
    {
        DeviceAddress = 0;
        DeviceConfiguration = 0;
        USBHS_DevEnumStatus = 0;
        USBHSD->DEV_AD = 0;
        UART_Bridge_Reset();
        USBHS_Device_Endp_Init();
        USBHSD->INT_FG = USBHS_UIF_BUS_RST;
    }
    else if(flags & USBHS_UIF_SUSPEND)
    {
        USBHSD->INT_FG = USBHS_UIF_SUSPEND;
    }
    else
    {
        USBHSD->INT_FG = flags;
    }
}
