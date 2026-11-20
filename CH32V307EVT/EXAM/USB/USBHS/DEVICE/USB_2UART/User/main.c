/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Description        : USBHS to eight UART bridge.
 *******************************************************************************/
#include "UART.h"
#include "ch32v30x_usbhs_device.h"

#if (SDI_PRINT == SDI_PR_OPEN)
#define LOG_INIT()       SDI_Printf_Enable()
#define LOG_PRINTF(...)  printf(__VA_ARGS__)
#else
#define LOG_INIT()       ((void)0)
#define LOG_PRINTF(...)  ((void)0)
#endif

int main(void)
{
    uint8_t last_enum_status = 0;

    SystemCoreClockUpdate();
    Delay_Init();
    LOG_INIT();

    LOG_PRINTF("\r\nUSB_2UART start, SystemClk:%lu\r\n",
               (unsigned long)SystemCoreClock);

    UART_Bridge_Init();
    LOG_PRINTF("UART bridge ready: 8 ports, default 115200 8N1\r\n");

    USBHS_RCC_Init();
    USBHS_Device_Init(ENABLE);
    LOG_PRINTF("USBHS device enabled, waiting for host\r\n");

    while(1)
    {
        UART_Bridge_Task();

        if(USBHS_DevEnumStatus != last_enum_status)
        {
            last_enum_status = USBHS_DevEnumStatus;
            if(last_enum_status)
            {
                LOG_PRINTF("USB configured: %s, packet size %u\r\n",
                           USBHS_DevMaxPackLen == DEF_USBD_HS_PACK_SIZE ? "High-Speed" : "Full-Speed",
                           (unsigned int)USBHS_DevMaxPackLen);
            }
            else
            {
                LOG_PRINTF("USB disconnected or reset\r\n");
            }
        }
    }
}
