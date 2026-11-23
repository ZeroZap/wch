/********************************** (C) COPYRIGHT *******************************
 * File Name          : nor_store.c
 * Description        : See nor_store.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "nor_store.h"
#include "board_config.h"
#include "debug.h"

#define W25Q_CMD_WRITE_ENABLE   0x06u
#define W25Q_CMD_READ_STATUS    0x05u
#define W25Q_CMD_READ_DATA      0x03u
#define W25Q_CMD_PAGE_PROGRAM   0x02u
#define W25Q_CMD_SECTOR_ERASE   0x20u
#define W25Q_CMD_JEDEC_ID       0x9Fu

#define W25Q_PAGE_SIZE          256u

#define CS_LOW()                GPIO_ResetBits(N1_NOR_PORT, N1_NOR_CS_PIN)
#define CS_HIGH()               GPIO_SetBits(N1_NOR_PORT, N1_NOR_CS_PIN)

static settings_t settings;
static uint8_t    present;
static uint32_t   jedec;

/*---------------------------------------------------------------------------
 * low level SPI
 *---------------------------------------------------------------------------*/
static void spi_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    SPI_InitTypeDef  spi = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    CS_HIGH();

    gpio.GPIO_Pin   = N1_NOR_CS_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(N1_NOR_PORT, &gpio);

    gpio.GPIO_Pin  = N1_NOR_SCK_PIN | N1_NOR_MOSI_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(N1_NOR_PORT, &gpio);

    gpio.GPIO_Pin  = N1_NOR_MISO_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(N1_NOR_PORT, &gpio);

    spi.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode              = SPI_Mode_Master;
    spi.SPI_DataSize          = SPI_DataSize_8b;
    spi.SPI_CPOL              = SPI_CPOL_Low;
    spi.SPI_CPHA              = SPI_CPHA_1Edge;
    spi.SPI_NSS               = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;   /* 48 / 4 = 12 MHz */
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}

static uint8_t spi_rw(uint8_t out)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {
    }
    SPI_I2S_SendData(SPI1, out);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
    }

    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

static void wait_ready(void)
{
    CS_LOW();
    spi_rw(W25Q_CMD_READ_STATUS);
    while (spi_rw(0xFFu) & 0x01u) {
    }
    CS_HIGH();
}

static void write_enable(void)
{
    CS_LOW();
    spi_rw(W25Q_CMD_WRITE_ENABLE);
    CS_HIGH();
}

static void nor_read(uint32_t addr, void *buf, uint32_t len)
{
    uint8_t *p = (uint8_t *)buf;

    CS_LOW();
    spi_rw(W25Q_CMD_READ_DATA);
    spi_rw((uint8_t)(addr >> 16));
    spi_rw((uint8_t)(addr >> 8));
    spi_rw((uint8_t)addr);
    while (len--) {
        *p++ = spi_rw(0xFFu);
    }
    CS_HIGH();
}

static void nor_erase_sector(uint32_t addr)
{
    write_enable();

    CS_LOW();
    spi_rw(W25Q_CMD_SECTOR_ERASE);
    spi_rw((uint8_t)(addr >> 16));
    spi_rw((uint8_t)(addr >> 8));
    spi_rw((uint8_t)addr);
    CS_HIGH();

    wait_ready();
}

static void nor_program_page(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    write_enable();

    CS_LOW();
    spi_rw(W25Q_CMD_PAGE_PROGRAM);
    spi_rw((uint8_t)(addr >> 16));
    spi_rw((uint8_t)(addr >> 8));
    spi_rw((uint8_t)addr);
    for (i = 0; i < len; i++) {
        spi_rw(buf[i]);
    }
    CS_HIGH();

    wait_ready();
}

static void nor_write(uint32_t addr, const void *buf, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)buf;

    nor_erase_sector(addr);

    while (len) {
        uint32_t page_left = W25Q_PAGE_SIZE - (addr % W25Q_PAGE_SIZE);
        uint16_t chunk = (uint16_t)((len < page_left) ? len : page_left);

        nor_program_page(addr, p, chunk);
        addr += chunk;
        p    += chunk;
        len  -= chunk;
    }
}

static uint32_t nor_jedec(void)
{
    uint32_t id;

    CS_LOW();
    spi_rw(W25Q_CMD_JEDEC_ID);
    id  = (uint32_t)spi_rw(0xFFu) << 16;
    id |= (uint32_t)spi_rw(0xFFu) << 8;
    id |= (uint32_t)spi_rw(0xFFu);
    CS_HIGH();

    return id;
}

/*---------------------------------------------------------------------------
 * settings
 *---------------------------------------------------------------------------*/
static uint16_t crc16(const uint8_t *p, uint32_t len)
{
    uint16_t crc = 0xFFFFu;
    uint32_t i;
    uint8_t  b;

    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)p[i] << 8;
        for (b = 0; b < 8u; b++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }

    return crc;
}

static void settings_defaults(void)
{
    memset(&settings, 0, sizeof(settings));
    settings.magic     = STORE_MAGIC;
    settings.version   = STORE_VERSION;
    settings.seq       = 0u;
    settings.bright    = WS2812_BRIGHTNESS;
    settings.sound     = 1u;
    settings.last_game = 0u;
    settings.crc       = crc16((const uint8_t *)&settings,
                               sizeof(settings) - sizeof(settings.crc));
}

static uint8_t settings_valid(const settings_t *s)
{
    if (s->magic != STORE_MAGIC || s->version != STORE_VERSION) {
        return 0u;
    }
    if (s->bright == 0u) {
        return 0u;
    }

    return (crc16((const uint8_t *)s, sizeof(*s) - sizeof(s->crc)) == s->crc) ? 1u : 0u;
}

static uint8_t settings_load_slot(uint32_t addr, settings_t *out)
{
    nor_read(addr, out, sizeof(*out));

    return settings_valid(out);
}

static void settings_load(void)
{
    settings_t a;
    settings_t b;
    uint8_t    va = settings_load_slot(STORE_SLOT_A, &a);
    uint8_t    vb = settings_load_slot(STORE_SLOT_B, &b);

    if (va && vb) {
        settings = ((int16_t)(a.seq - b.seq) >= 0) ? a : b;
    } else if (va) {
        settings = a;
    } else if (vb) {
        settings = b;
    } else {
        settings_defaults();
    }
}

uint8_t store_init(void)
{
    spi_init();
    jedec = nor_jedec();

    present = (jedec != 0u && jedec != 0xFFFFFFu) ? 1u : 0u;

    if (present) {
        settings_load();
    } else {
        settings_defaults();
    }

    return present;
}

uint8_t store_present(void)
{
    return present;
}

uint32_t store_jedec_id(void)
{
    return jedec;
}

uint32_t store_size(void)
{
    uint8_t cap = (uint8_t)(jedec & 0xFFu);

    if (!present || cap < 0x10u || cap > 0x1Fu) {
        return 4u * 1024u * 1024u;      /* default W25Q32 */
    }

    return 1u << cap;                   /* 0x16 -> 4 MB */
}

void nor_raw_read(uint32_t addr, void *buf, uint32_t len)
{
    nor_read(addr, buf, len);
}

void nor_raw_erase_sector(uint32_t addr)
{
    nor_erase_sector(addr);
}

void nor_raw_write_page(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    nor_program_page(addr, buf, len);
}

void store_save(void)
{
    settings_t copy = settings;
    uint32_t   slot;

    settings.seq++;
    copy.seq = settings.seq;
    copy.crc = crc16((const uint8_t *)&copy, sizeof(copy) - sizeof(copy.crc));

    if (!present) {
        settings = copy;
        return;
    }

    /* alternate slots, so the previous good copy survives a power loss */
    slot = (settings.seq & 1u) ? STORE_SLOT_B : STORE_SLOT_A;
    nor_write(slot, &copy, sizeof(copy));

    settings = copy;
}

uint8_t store_brightness(void)
{
    return settings.bright;
}

void store_set_brightness(uint8_t level)
{
    if (level < 8u) {
        level = 8u;
    }
    if (settings.bright != level) {
        settings.bright = level;
        store_save();
    }
}

uint8_t store_sound(void)
{
    return settings.sound;
}

void store_set_sound(uint8_t on)
{
    uint8_t v = on ? 1u : 0u;

    if (settings.sound != v) {
        settings.sound = v;
        store_save();
    }
}

uint8_t store_last_game(void)
{
    return settings.last_game;
}

void store_set_last_game(uint8_t index)
{
    if (settings.last_game != index) {
        settings.last_game = index;
        store_save();
    }
}

uint16_t store_hi(uint8_t game)
{
    return (game < STORE_HI_COUNT) ? settings.hi[game] : 0u;
}

void store_set_hi(uint8_t game, uint16_t score)
{
    if (game >= STORE_HI_COUNT) {
        return;
    }
    if (score > settings.hi[game]) {
        settings.hi[game] = score;
        store_save();
    }
}

void store_reset_hi(void)
{
    memset(settings.hi, 0, sizeof(settings.hi));
    store_save();
}
