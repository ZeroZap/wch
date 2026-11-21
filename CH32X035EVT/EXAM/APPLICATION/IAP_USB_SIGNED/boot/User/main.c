/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Description        : Signed-image bootloader for CH32X035 (USB CDC only).
 *
 * Boot flow
 *   1. read the flag page
 *   2. if the app requested an upgrade, or no valid image is installed, stay
 *      in the bootloader and serve the IAP protocol on USB CDC
 *   3. otherwise open a short window so the host can still abort the jump,
 *      then verify the image (SHA-256 + HMAC) and hand over to the app
 *
 * There is no UART and no "jump pin": the app is expected to be reachable over
 * USB and to ask for an upgrade itself (see app/User/app_iap.c).
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "ch32x035.h"
#include "debug.h"

#include "usb_cdc.h"
#include "iap_proto.h"
#include "img_install.h"
#include "img_hdr.h"
#include "boot_ctrl.h"

/* How long the bootloader waits after the host has enumerated before it hands
 * over.  Set to 0 for the fastest possible boot, at the price of having to
 * force an upgrade from the application. */
#ifndef BOOT_STAY_WINDOW_MS
#define BOOT_STAY_WINDOW_MS     1200u
#endif

/* Hard limit for the window, so a host that never enumerates cannot keep the
 * device in the bootloader forever. */
#define BOOT_WINDOW_MAX_MS      3000u

/*********************************************************************
 * @fn      boot_wait_window
 *
 * @brief   Keep the USB port open for a moment; return when the host asked to
 *          jump or when the window expired.
 *
 * @return  none
 */
static void boot_wait_window(void)
{
#if BOOT_STAY_WINDOW_MS > 0
    uint32_t elapsed = 0u;
    uint32_t ready_ms = 0u;

    while (elapsed < BOOT_WINDOW_MAX_MS) {
        Delay_Ms(10);
        elapsed += 10u;

        (void)iap_proto_service();

        if (iap_proto_jump_requested()) {
            return;
        }
        if (usb_cdc_ready()) {
            ready_ms += 10u;
            if (ready_ms >= BOOT_STAY_WINDOW_MS) {
                return;
            }
        }
    }
#else
    (void)iap_proto_service();
#endif
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
    img_flag_t flag;
    uint8_t    have_flag;
    uint8_t    upgrade;
    uint8_t    usb_up = 0u;
    uint8_t    st;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    have_flag = img_load_flag(&flag);
    upgrade   = (uint8_t)(have_flag && (flag.state == IMG_STATE_UPGRADE));

    st = img_verify_stored();

    if ((st == IAP_ST_OK) && !upgrade) {
        /* a good image is installed: give the host a chance to interfere, then
         * get out of the way */
        usb_cdc_init();
        usb_up = 1u;
        iap_proto_init();
        boot_wait_window();

        if (!iap_proto_jump_requested() && !img_install_active()) {
            boot_jump_to_app();
        }
    }

    /* stay in the bootloader; do not re-init USB if a transfer already started
     * during the window above */
    if (!usb_up) {
        usb_cdc_init();
    }
    iap_proto_init();

    while (1) {
        (void)iap_proto_service();

        if (iap_proto_jump_requested()) {
            boot_jump_to_app();
        }
    }
}
