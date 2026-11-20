/********************************** (C) COPYRIGHT *******************************
 * File Name          : norflash.c
 * Description        : Minimal W25Qxx driver on SPI1.
 *
 * The WS2812 data pin is PC7 (PIOC 1-wire engine), so SPI1 is free for
 * the flash and both peripherals can be used at the same time.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>
#include "norflash.h"

#define NOR_CS_PORT             GPIOA
#define NOR_CS_PIN              GPIO_Pin_2

#define NOR_CMD_WRITE_ENABLE    0x06
#define NOR_CMD_READ_STATUS     0x05
#define NOR_CMD_READ_DATA       0x03
#define NOR_CMD_PAGE_PROGRAM    0x02
#define NOR_CMD_SECTOR_ERASE    0x20
#define NOR_CMD_JEDEC_ID        0x9F

#define NOR_SECTOR_SIZE         4096u
#define NOR_PAGE_SIZE           256u

/* sector erase on a GD25Q40 takes at most 300ms, so a healthy flash always
 * clears the busy bit long before this counter runs out */
#define NOR_BUSY_TRIES          2000000u

static uint32_t nor_id;
static uint32_t nor_bytes;

/*********************************************************************
 * @fn      nor_xfer
 *
 * @brief   Full duplex transfer, CS must already be low.
 *
 * @return  none
 */
static void nor_xfer(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++) {
        uint8_t out = tx ? tx[i] : 0xFFu;

        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {
        }
        SPI_I2S_SendData(SPI1, out);
        while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
        }
        out = (uint8_t)SPI_I2S_ReceiveData(SPI1);
        if (rx) {
            rx[i] = out;
        }
    }
}

static void nor_cs_low(void)
{
    GPIO_ResetBits(NOR_CS_PORT, NOR_CS_PIN);
}

static void nor_cs_high(void)
{
    GPIO_SetBits(NOR_CS_PORT, NOR_CS_PIN);
}

static uint8_t nor_read_sr(void)
{
    uint8_t cmd = NOR_CMD_READ_STATUS;
    uint8_t sr = 0;

    nor_cs_low();
    nor_xfer(&cmd, 0, 1);
    nor_xfer(0, &sr, 1);
    nor_cs_high();

    return sr;
}

static uint8_t nor_wait_busy(void)
{
    uint32_t tries = NOR_BUSY_TRIES;

    while (tries-- != 0) {
        if ((nor_read_sr() & 0x01u) == 0) {
            return 1;
        }
    }

    return 0;           /* stays busy: power, wiring or WP problem */
}

static void nor_write_enable(void)
{
    uint8_t cmd = NOR_CMD_WRITE_ENABLE;

    nor_cs_low();
    nor_xfer(&cmd, 0, 1);
    nor_cs_high();
}

/*********************************************************************
 * @fn      nor_spi_cfg
 *
 * @brief   (Re)configure SPI1 as master mode 3 with the given prescaler.
 *          Called twice at boot: fast first, slow retry if the JEDEC ID
 *          came back as all-0 or all-1 (flying leads / long wires).
 *
 * @return  none
 */
static void nor_spi_cfg(uint16_t prescaler)
{
    SPI_InitTypeDef SPI_InitStructure = {0};

    SPI_Cmd(SPI1, DISABLE);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = prescaler;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
}

static uint32_t nor_read_jedec(void)
{
    uint8_t cmd = NOR_CMD_JEDEC_ID;
    uint8_t id[3] = {0};

    nor_cs_low();
    nor_xfer(&cmd, 0, 1);
    nor_xfer(0, id, 3);
    nor_cs_high();

    return ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];
}

/*********************************************************************
 * @fn      nor_init
 *
 * @brief   Configure SPI1 and probe the flash.
 *
 * @return  1 = flash present and supported, 0 = not found.
 */
uint8_t nor_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = NOR_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(NOR_CS_PORT, &GPIO_InitStructure);
    nor_cs_high();

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;   /* SCK  */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;   /* MISO */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;   /* MOSI */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    nor_spi_cfg(WSP_NOR_SPI_PRESCALER);
    nor_id = nor_read_jedec();

    if ((nor_id == 0x000000u) || (nor_id == 0xFFFFFFu)) {
        /* nothing answered at full speed, retry at 1.5MHz before giving up */
        nor_spi_cfg(SPI_BaudRatePrescaler_32);
        nor_id = nor_read_jedec();
    }

    if ((nor_id == 0x000000u) || (nor_id == 0xFFFFFFu)) {
        nor_bytes = 0;
        return 0;
    }

    /* JEDEC capacity byte: 2^n bytes.
     * 0x10 = 64KB, 0x13 = 512KB (GD25Q40 / W25Q40), 0x14 = 1MB,
     * 0x15 = 2MB, 0x16 = 4MB, 0x17 = 8MB, 0x18 = 16MB, 0x19 = 32MB */
    {
        uint8_t cap = (uint8_t)(nor_id & 0xFFu);

        if ((cap >= 16u) && (cap <= 25u)) {
            nor_bytes = 1uL << cap;
        } else {
            nor_bytes = 0;
        }
    }

    return (nor_bytes != 0) ? 1 : 0;
}

uint32_t nor_jedec_id(void)
{
    return nor_id;
}

/*********************************************************************
 * @fn      nor_vendor
 *
 * @brief   Manufacturer name from the JEDEC ID (for the boot log).
 *
 * @return  short vendor string.
 */
const char *nor_vendor(void)
{
    switch ((nor_id >> 16) & 0xFFu) {
    case 0xEF:
        return "Winbond";
    case 0xC8:
        return "GigaDevice";
    case 0x1C:
        return "EON";
    case 0x9D:
        return "ISSI";
    case 0x5E:
        return "Zbit";
    case 0x68:
        return "Boya";
    case 0x20:
        return "Micron";
    case 0xBF:
        return "SST";
    default:
        return "unknown";
    }
}

uint32_t nor_size(void)
{
    return nor_bytes;
}

/*********************************************************************
 * @fn      nor_read
 *
 * @brief   Read len bytes at addr.
 *
 * @return  none
 */
void nor_read(uint32_t addr, void *buf, uint32_t len)
{
    uint8_t cmd[4];

    cmd[0] = NOR_CMD_READ_DATA;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)addr;

    nor_cs_low();
    nor_xfer(cmd, 0, 4);
    nor_xfer(0, (uint8_t *)buf, len);
    nor_cs_high();
}

/*********************************************************************
 * @fn      nor_erase_sector
 *
 * @brief   Erase the 4K sector that contains addr.
 *
 * @return  none
 */
void nor_erase_sector(uint32_t addr)
{
    uint8_t cmd[4];

    nor_write_enable();

    cmd[0] = NOR_CMD_SECTOR_ERASE;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)addr;

    nor_cs_low();
    nor_xfer(cmd, 0, 4);
    nor_cs_high();

    (void)nor_wait_busy();
}

/*********************************************************************
 * @fn      nor_write
 *
 * @brief   Erase + page program a buffer.  Slow (sector erase per 4K),
 *          only used when uploading images.
 *
 * @return  none
 */
void nor_write(uint32_t addr, const void *buf, uint32_t len)
{
    const uint8_t *src = (const uint8_t *)buf;
    uint32_t chunk;
    uint32_t off = 0;

    /* addr must be 4K aligned: every sector is erased once, then the pages
       are programmed in ascending order */
    while (off < len) {
        if ((addr % NOR_SECTOR_SIZE) == 0) {
            nor_erase_sector(addr);
        }

        chunk = NOR_PAGE_SIZE - (addr % NOR_PAGE_SIZE);
        if (chunk > (len - off)) {
            chunk = len - off;
        }

        if (chunk != 0) {
            uint8_t cmd[4];

            nor_write_enable();
            cmd[0] = NOR_CMD_PAGE_PROGRAM;
            cmd[1] = (uint8_t)(addr >> 16);
            cmd[2] = (uint8_t)(addr >> 8);
            cmd[3] = (uint8_t)addr;

            nor_cs_low();
            nor_xfer(cmd, 0, 4);
            nor_xfer(src + off, 0, chunk);
            nor_cs_high();

            if (!nor_wait_busy()) {
                return;     /* flash stuck busy: abort the upload */
            }

            off += chunk;
            addr += chunk;
        }
    }
}

/*********************************************************************
 * @fn      nor_src_read
 *
 * @brief   img_src_t read callback for the NOR flash.
 *
 * @return  number of bytes read.
 */
static uint32_t nor_src_read(void *ctx, uint32_t off, void *buf, uint32_t len)
{
    uintptr_t base = (uintptr_t)ctx;

    if ((nor_bytes != 0) && ((base + off + len) > nor_bytes)) {
        if ((base + off) >= nor_bytes) {
            return 0;
        }
        len = nor_bytes - (base + off);
    }

    nor_read(base + off, buf, len);

    return len;
}

/*********************************************************************
 * @fn      nor_img_src
 *
 * @brief   Prepare an img_src_t for a WSP1 file stored at "base".
 *
 * @return  IMG_OK or a negative error code.
 */
int nor_img_src(img_src_t *src, uint32_t base)
{
    if (src == 0) {
        return IMG_ERR_ARG;
    }

    src->read = nor_src_read;
    src->ctx  = (void *)(uintptr_t)base;
    src->size = (nor_bytes > base) ? (nor_bytes - base) : 0;

    return IMG_OK;
}

/*********************************************************************
 * @fn      nor_selftest
 *
 * @brief   Erase / program / read back a pattern in the last 4KB sector.
 *          Use it once after wiring the flash, it destroys that sector.
 *
 * @return  1 = flash works, 0 = failure (or no flash).
 */
uint8_t nor_selftest(void)
{
    uint8_t  buf[16];
    uint8_t  tmp[16];
    uint32_t addr;
    uint8_t  i;

    if (nor_bytes < NOR_SECTOR_SIZE) {
        return 0;
    }

    addr = nor_bytes - NOR_SECTOR_SIZE;      /* last 4K sector */

    for (i = 0; i < sizeof(buf); i++) {
        buf[i] = (uint8_t)(0xA5u ^ (uint8_t)(i * 17u));
    }

    nor_write(addr, buf, sizeof(buf));       /* erases the sector, then programs */
    nor_read(addr, tmp, sizeof(tmp));

    for (i = 0; i < sizeof(tmp); i++) {
        if (tmp[i] != buf[i]) {
            return 0;
        }
    }

    nor_erase_sector(addr);                  /* leave the sector clean */

    return 1;
}
