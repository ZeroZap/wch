/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Description        : Demo application for the signed-image bootloader.
 *
 * This is a template, not the product: it enumerates as a CDC port, answers the
 * protocol in app_usb.c, blinks PB0 and can hop back into the bootloader on
 * request.  Replace the body of the main loop with the real application.
 *
 * The image is linked at IMG_APP_EXEC_ADDR (0x2800); the bootloader verifies
 * the SHA-256/HMAC stored next to it before every hand-over.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "ch32x035.h"
#include "debug.h"

#include "usb_cdc.h"
#include "app_usb.h"

#define LED_GPIO        GPIOB
#define LED_PIN         GPIO_Pin_0
#define LED_HALF_MS     500u

/*********************************************************************
 * @fn      led_init
 *
 * @brief   Heartbeat pin, proves the bootloader jumped into the app.
 *
 * @return  none
 */
static void led_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Pin   = LED_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO, &gpio);
    GPIO_WriteBit(LED_GPIO, LED_PIN, Bit_RESET);
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    uint32_t tick = 0u;
    uint8_t  led  = 0u;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    led_init();

    usb_cdc_init();
    app_usb_init();

    while (1) {
        (void)app_usb_service();

        Delay_Ms(1);

        if (++tick >= LED_HALF_MS) {
            tick = 0u;
            led = (uint8_t)!led;
            GPIO_WriteBit(LED_GPIO, LED_PIN, led ? Bit_SET : Bit_RESET);
        }
    }
}
