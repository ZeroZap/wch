/********************************** (C) COPYRIGHT *******************************
 * File Name          : boot_ctrl.c
 * Description        : Handover from the bootloader to the application.
 *
 * The app is linked at IMG_APP_EXEC_ADDR and its entry point is _start, which
 * re-initialises gp, sp, bss, mtvec and the clock tree.
 *
 * The hand-over goes through the software interrupt exactly like the vendor
 * IAP example: calling the entry point as a normal function leaves the core in
 * a state where the app's own "configure pipelining" write (csrw 0xbc0) traps
 * as an illegal instruction, so the jump is issued from inside the software
 * interrupt handler instead.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "boot_ctrl.h"
#include "boot_info.h"
#include "img_hdr.h"
#include "usb_cdc.h"
#include "ch32x035.h"
#include "debug.h"

#define USB_IOEN    0x00000080u

/* the assembly below needs a literal address */
#if IMG_APP_EXEC_ADDR != 0x00002800
#error "SW_Handler hardcodes the app entry address, update it together with img_hdr.h"
#endif

void boot_usb_deinit(void)
{
    __disable_irq();

    NVIC_DisableIRQ(USBFS_IRQn);
    NVIC_ClearPendingIRQ(USBFS_IRQn);

    USBFSD->BASE_CTRL = 0x00;
    USBFSD->UDEV_CTRL = 0x00;
    USBFSD->INT_EN    = 0x00;
    USBFSD->INT_FG    = 0xFF;

    AFIO->CTLR &= ~USB_IOEN;
}

uint8_t boot_button_held(void)
{
#if BOOT_BUTTON_STAY
    GPIO_InitTypeDef g = {0};
    uint32_t i;
    uint32_t low = 0u;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    g.GPIO_Pin  = BOOT_BUTTON_PIN;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(BOOT_BUTTON_PORT, &g);

    Delay_Ms(10);                       /* let the pull-up settle */

    if (GPIO_ReadInputDataBit(BOOT_BUTTON_PORT, BOOT_BUTTON_PIN) != 0) {
        return 0u;                      /* not pressed: the common case */
    }

    /* confirm, so a glitch on a floating pin does not trap the bootloader */
    for (i = 0u; i < 10u; i++) {
        if (GPIO_ReadInputDataBit(BOOT_BUTTON_PORT, BOOT_BUTTON_PIN) == 0) {
            low++;
        }
        Delay_Ms(5);
    }

    return (low >= 8u) ? 1u : 0u;
#else
    return 0u;
#endif
}

/* The vector table is written in startup_ch32x035.S, so LTO must not
 * internalize this symbol: without externally_visible the SW interrupt would
 * fall back to the weak "j ." stub and the jump to the app would hang.
 * The attributes have to sit on the definition, a prototype is not enough. */
__attribute__((interrupt("WCH-Interrupt-fast"), used, externally_visible))
void SW_Handler(void)
{
    __asm volatile("li a6, 0x2800");
    __asm volatile("jr a6");

    while (1) {
    }
}

void boot_jump_to_app(void)
{
    boot_usb_deinit();

    /* release every pin we touched; the app configures what it needs.  The
     * GPIO/AFIO clocks have to be on while the registers are reset, so they are
     * only switched off at the end. */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_DeInit(GPIOA);
    GPIO_DeInit(GPIOB);
    GPIO_DeInit(GPIOC);
    GPIO_AFIODeInit();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, DISABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBFS, DISABLE);

    Delay_Ms(20);

    /* boot_usb_deinit() masked interrupts; the software interrupt needs them */
    NVIC_EnableIRQ(Software_IRQn);
    NVIC_SetPendingIRQ(Software_IRQn);
    __enable_irq();

    while (1) {
    }
}
