/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_iap.c
 * Description        : See app_iap.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "app_iap.h"
#include "img_hdr.h"
#include "ch32x035.h"
#include "debug.h"

uint8_t app_iap_get_flag(img_flag_t *out)
{
    memcpy(out, (const void *)IMG_FLAG_BASE, sizeof(*out));

    if (out->magic != IMG_FLAG_MAGIC || out->state > IMG_STATE_INVALID) {
        return 0u;
    }

    return 1u;
}

uint8_t app_iap_get_hdr(img_hdr_t *out)
{
    memcpy(out, (const void *)IMG_HDR_BASE, sizeof(*out));

    if (out->magic != IMG_MAGIC || out->hdr_ver != IMG_HDR_VER) {
        return 0u;
    }
    if (out->load_addr != IMG_APP_EXEC_ADDR || out->img_len > IMG_SLOT_SIZE) {
        return 0u;
    }

    return 1u;
}

void app_iap_request(void)
{
    uint8_t     page[IMG_PAGE_SIZE];
    img_flag_t *f = (img_flag_t *)page;

    memset(page, 0xFF, sizeof(page));
    memset(f, 0, sizeof(*f));
    f->magic   = IMG_FLAG_MAGIC;
    f->state   = IMG_STATE_UPGRADE;
    f->img_len = 0u;
    /* hdr_crc32 is only informational here; the bootloader re-verifies the
     * image header from the app slot before jumping. */
    f->hdr_crc32 = 0u;

    FLASH_Unlock();
    FLASH_Unlock_Fast();

    FLASH_ErasePage_Fast(IMG_FLAG_BASE);
    FLASH_BufReset();
    {
        uint32_t i;
        for (i = 0; i < IMG_PAGE_SIZE; i += 4u) {
            uint32_t w = (uint32_t)page[i] |
                         ((uint32_t)page[i + 1u] << 8) |
                         ((uint32_t)page[i + 2u] << 16) |
                         ((uint32_t)page[i + 3u] << 24);
            FLASH_BufLoad(IMG_FLAG_BASE + i, w);
        }
    }
    FLASH_ProgramPage_Fast(IMG_FLAG_BASE);

    FLASH_Lock_Fast();
    FLASH_Lock();

    Delay_Ms(10);
    NVIC_SystemReset();

    while (1) {
    }
}
