/********************************** (C) COPYRIGHT *******************************
 * File Name          : UART.c
 * Description        : Eight-channel UART DMA bridge for USB CDC ACM.
 *******************************************************************************/
#include "UART.h"
#include "ch32v30x_usbhs_device.h"
#include <string.h>

typedef struct
{
    USART_TypeDef *uart;
    DMA_Channel_TypeDef *tx_dma;
    DMA_Channel_TypeDef *rx_dma;
    uint8_t line_coding[7];
    volatile uint16_t rx_last_count;
    volatile uint8_t usb_in_busy;
    volatile uint8_t tx_busy;
    volatile uint8_t tx_read;
    volatile uint8_t tx_write;
    volatile uint8_t tx_count;
    uint16_t tx_length[UART_USB_OUT_QUEUE_DEPTH];
} UART_BRIDGE_CTL;

static UART_BRIDGE_CTL UartBridge[UART_PORT_COUNT];
static __attribute__((aligned(4))) uint8_t UartRxBuffer[UART_PORT_COUNT][UART_RX_DMA_SIZE];
static __attribute__((aligned(4))) uint8_t UartTxBuffer[UART_PORT_COUNT][UART_USB_OUT_QUEUE_DEPTH][UART_USB_PACKET_SIZE];

static USART_TypeDef *const UartInstance[UART_PORT_COUNT] =
{
    USART1, USART2, USART3, UART4, UART5, UART6, UART7, UART8
};

static DMA_Channel_TypeDef *const UartTxDma[UART_PORT_COUNT] =
{
    DMA1_Channel4, DMA1_Channel7, DMA1_Channel2, DMA2_Channel5,
    DMA2_Channel4, DMA2_Channel6, DMA2_Channel8, DMA2_Channel10
};

static DMA_Channel_TypeDef *const UartRxDma[UART_PORT_COUNT] =
{
    DMA1_Channel5, DMA1_Channel6, DMA1_Channel3, DMA2_Channel3,
    DMA2_Channel2, DMA2_Channel7, DMA2_Channel9, DMA2_Channel11
};

static void UART_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC |
                           RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);

    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOB, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_12 | GPIO_Pin_2 | GPIO_Pin_4;
    GPIO_Init(GPIOC, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOE, &gpio);

    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_3;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOB, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_3 | GPIO_Pin_5;
    GPIO_Init(GPIOC, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOD, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOE, &gpio);

    GPIO_PinRemapConfig(GPIO_FullRemap_USART6, ENABLE);
}

static void UART_Configure(uint8_t port)
{
    USART_InitTypeDef init = {0};
    UART_BRIDGE_CTL *ctl = &UartBridge[port];
    uint32_t baudrate;

    baudrate = (uint32_t)ctl->line_coding[0] |
               ((uint32_t)ctl->line_coding[1] << 8) |
               ((uint32_t)ctl->line_coding[2] << 16) |
               ((uint32_t)ctl->line_coding[3] << 24);
    if(baudrate == 0)
    {
        baudrate = UART_DEFAULT_BAUDRATE;
    }

    USART_Cmd(ctl->uart, DISABLE);
    init.USART_BaudRate = baudrate;
    init.USART_WordLength = USART_WordLength_8b;
    if(ctl->line_coding[5] == 1)
    {
        init.USART_Parity = USART_Parity_Odd;
        init.USART_WordLength = USART_WordLength_9b;
    }
    else if(ctl->line_coding[5] == 2)
    {
        init.USART_Parity = USART_Parity_Even;
        init.USART_WordLength = USART_WordLength_9b;
    }
    else
    {
        init.USART_Parity = USART_Parity_No;
    }

    if(ctl->line_coding[4] == 1)
    {
        init.USART_StopBits = USART_StopBits_1_5;
    }
    else if(ctl->line_coding[4] == 2)
    {
        init.USART_StopBits = USART_StopBits_2;
    }
    else
    {
        init.USART_StopBits = USART_StopBits_1;
    }
    init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(ctl->uart, &init);
    USART_Cmd(ctl->uart, ENABLE);
    USART_DMACmd(ctl->uart, USART_DMAReq_Rx, ENABLE);
}

static void UART_DMA_Init(uint8_t port)
{
    DMA_InitTypeDef dma = {0};
    UART_BRIDGE_CTL *ctl = &UartBridge[port];

    DMA_DeInit(ctl->tx_dma);
    dma.DMA_PeripheralBaseAddr = (uint32_t)&ctl->uart->DATAR;
    dma.DMA_MemoryBaseAddr = (uint32_t)UartTxBuffer[port][0];
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = 0;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_VeryHigh;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(ctl->tx_dma, &dma);

    DMA_DeInit(ctl->rx_dma);
    dma.DMA_MemoryBaseAddr = (uint32_t)UartRxBuffer[port];
    dma.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma.DMA_BufferSize = UART_RX_DMA_SIZE;
    dma.DMA_Mode = DMA_Mode_Circular;
    DMA_Init(ctl->rx_dma, &dma);
    DMA_Cmd(ctl->rx_dma, ENABLE);
}

void UART_Bridge_Init(void)
{
    uint8_t port;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB1Periph_USART3 |
                           RCC_APB1Periph_UART4 | RCC_APB1Periph_UART5 |
                           RCC_APB1Periph_UART6 | RCC_APB1Periph_UART7 |
                           RCC_APB1Periph_UART8, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 | RCC_AHBPeriph_DMA2, ENABLE);
    UART_GPIO_Init();

    for(port = 0; port < UART_PORT_COUNT; port++)
    {
        UartBridge[port].uart = UartInstance[port];
        UartBridge[port].tx_dma = UartTxDma[port];
        UartBridge[port].rx_dma = UartRxDma[port];
        UartBridge[port].line_coding[0] = (uint8_t)UART_DEFAULT_BAUDRATE;
        UartBridge[port].line_coding[1] = (uint8_t)(UART_DEFAULT_BAUDRATE >> 8);
        UartBridge[port].line_coding[2] = (uint8_t)(UART_DEFAULT_BAUDRATE >> 16);
        UartBridge[port].line_coding[3] = (uint8_t)(UART_DEFAULT_BAUDRATE >> 24);
        UartBridge[port].line_coding[4] = 0;
        UartBridge[port].line_coding[5] = 0;
        UartBridge[port].line_coding[6] = 8;
        UART_DMA_Init(port);
        UART_Configure(port);
    }
    UART_Bridge_Reset();
}

void UART_Bridge_Reset(void)
{
    uint8_t port;
    for(port = 0; port < UART_PORT_COUNT; port++)
    {
        UartBridge[port].rx_last_count = UartBridge[port].rx_dma->CNTR;
        UartBridge[port].usb_in_busy = 0;
        UartBridge[port].tx_busy = 0;
        UartBridge[port].tx_read = 0;
        UartBridge[port].tx_write = 0;
        UartBridge[port].tx_count = 0;
        DMA_Cmd(UartBridge[port].tx_dma, DISABLE);
        USART_DMACmd(UartBridge[port].uart, USART_DMAReq_Tx, DISABLE);
    }
}

void UART_Bridge_SetLineCoding(uint8_t port, const uint8_t *coding)
{
    if(port >= UART_PORT_COUNT || coding[6] != 8 || coding[5] > 2 || coding[4] > 2)
    {
        return;
    }
    memcpy(UartBridge[port].line_coding, coding, 7);
    UART_Configure(port);
}

uint8_t *UART_Bridge_GetLineCoding(uint8_t port)
{
    return (port < UART_PORT_COUNT) ? UartBridge[port].line_coding : 0;
}

uint8_t *UART_Bridge_GetUsbOutBuffer(uint8_t port)
{
    if(port >= UART_PORT_COUNT)
    {
        return 0;
    }
    return UartTxBuffer[port][UartBridge[port].tx_write];
}

uint8_t UART_Bridge_UsbOutComplete(uint8_t port, uint16_t len)
{
    UART_BRIDGE_CTL *ctl;
    if(port >= UART_PORT_COUNT || len > UART_USB_PACKET_SIZE)
    {
        return 0;
    }
    if(len == 0)
    {
        return 1;
    }
    ctl = &UartBridge[port];
    ctl->tx_length[ctl->tx_write] = len;
    ctl->tx_write = (ctl->tx_write + 1) % UART_USB_OUT_QUEUE_DEPTH;
    ctl->tx_count++;
    return ctl->tx_count < UART_USB_OUT_QUEUE_DEPTH;
}

void UART_Bridge_UsbInComplete(uint8_t port)
{
    if(port < UART_PORT_COUNT)
    {
        UartBridge[port].usb_in_busy = 0;
    }
}

static void UART_StartTx(uint8_t port)
{
    UART_BRIDGE_CTL *ctl = &UartBridge[port];

    if(ctl->tx_busy || ctl->tx_count == 0)
    {
        return;
    }
    DMA_Cmd(ctl->tx_dma, DISABLE);
    ctl->tx_dma->MADDR = (uint32_t)UartTxBuffer[port][ctl->tx_read];
    ctl->tx_dma->CNTR = ctl->tx_length[ctl->tx_read];
    USART_ClearFlag(ctl->uart, USART_FLAG_TC);
    DMA_Cmd(ctl->tx_dma, ENABLE);
    USART_DMACmd(ctl->uart, USART_DMAReq_Tx, ENABLE);
    ctl->tx_busy = 1;
}

static void UART_PollTx(uint8_t port)
{
    UART_BRIDGE_CTL *ctl = &UartBridge[port];

    if(ctl->tx_busy && ctl->tx_dma->CNTR == 0 && USART_GetFlagStatus(ctl->uart, USART_FLAG_TC) != RESET)
    {
        DMA_Cmd(ctl->tx_dma, DISABLE);
        USART_DMACmd(ctl->uart, USART_DMAReq_Tx, DISABLE);
        ctl->tx_busy = 0;
        ctl->tx_read = (ctl->tx_read + 1) % UART_USB_OUT_QUEUE_DEPTH;
        ctl->tx_count--;
        USBHS_PortOutSetAck(port, ENABLE);
    }
    UART_StartTx(port);
}

static void UART_PollRx(uint8_t port)
{
    UART_BRIDGE_CTL *ctl = &UartBridge[port];
    uint16_t current;
    uint16_t offset;
    uint16_t length;

    if(ctl->usb_in_busy || !USBHS_DevEnumStatus)
    {
        return;
    }

    current = ctl->rx_dma->CNTR;
    if(current == ctl->rx_last_count)
    {
        return;
    }

    offset = UART_RX_DMA_SIZE - ctl->rx_last_count;
    if(current < ctl->rx_last_count)
    {
        length = ctl->rx_last_count - current;
    }
    else
    {
        length = ctl->rx_last_count;
    }
    if(length > USBHS_DevMaxPackLen)
    {
        length = USBHS_DevMaxPackLen;
    }
    if(length > UART_RX_DMA_SIZE - offset)
    {
        length = UART_RX_DMA_SIZE - offset;
    }

    if(USBHS_PortDataUp(port, &UartRxBuffer[port][offset], length) == 0)
    {
        ctl->usb_in_busy = 1;
        ctl->rx_last_count -= length;
        if(ctl->rx_last_count == 0)
        {
            ctl->rx_last_count = UART_RX_DMA_SIZE;
        }
    }
}

void UART_Bridge_Task(void)
{
    uint8_t port;
    for(port = 0; port < UART_PORT_COUNT; port++)
    {
        UART_PollTx(port);
        UART_PollRx(port);
    }
}
