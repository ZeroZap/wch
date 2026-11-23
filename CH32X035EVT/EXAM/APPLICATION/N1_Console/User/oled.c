/********************************** (C) COPYRIGHT *******************************
 * File Name          : oled.c
 * Description        : See oled.h.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>

#include "oled.h"
#include "board_config.h"
#include "debug.h"

#if N1_BOARD_HAS_OLED

/* 44 glyphs: 0-9, A-Z, space, '-', '.', ':', '/', '?', '>', '!'.
 * One byte per column, bit 0 = top row. */
static const uint8_t font5x7[][5] = {
    { 0x3E, 0x51, 0x49, 0x45, 0x3E }, /* 0 */
    { 0x00, 0x42, 0x7F, 0x40, 0x00 }, /* 1 */
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, /* 2 */
    { 0x21, 0x41, 0x45, 0x4B, 0x31 }, /* 3 */
    { 0x18, 0x14, 0x12, 0x7F, 0x10 }, /* 4 */
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, /* 5 */
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, /* 6 */
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, /* 7 */
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, /* 8 */
    { 0x06, 0x49, 0x49, 0x29, 0x1E }, /* 9 */
    { 0x7E, 0x11, 0x11, 0x11, 0x7E }, /* A */
    { 0x7F, 0x49, 0x49, 0x49, 0x36 }, /* B */
    { 0x3E, 0x41, 0x41, 0x41, 0x22 }, /* C */
    { 0x7F, 0x41, 0x41, 0x22, 0x1C }, /* D */
    { 0x7F, 0x49, 0x49, 0x49, 0x41 }, /* E */
    { 0x7F, 0x09, 0x09, 0x09, 0x01 }, /* F */
    { 0x3E, 0x41, 0x49, 0x49, 0x7A }, /* G */
    { 0x7F, 0x08, 0x08, 0x08, 0x7F }, /* H */
    { 0x00, 0x41, 0x7F, 0x41, 0x00 }, /* I */
    { 0x20, 0x40, 0x41, 0x3F, 0x01 }, /* J */
    { 0x7F, 0x08, 0x14, 0x22, 0x41 }, /* K */
    { 0x7F, 0x40, 0x40, 0x40, 0x40 }, /* L */
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, /* M */
    { 0x7F, 0x04, 0x08, 0x10, 0x7F }, /* N */
    { 0x3E, 0x41, 0x41, 0x41, 0x3E }, /* O */
    { 0x7F, 0x09, 0x09, 0x09, 0x06 }, /* P */
    { 0x3E, 0x41, 0x51, 0x21, 0x5E }, /* Q */
    { 0x7F, 0x09, 0x19, 0x29, 0x46 }, /* R */
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, /* S */
    { 0x01, 0x01, 0x7F, 0x01, 0x01 }, /* T */
    { 0x3F, 0x40, 0x40, 0x40, 0x3F }, /* U */
    { 0x1F, 0x20, 0x40, 0x20, 0x1F }, /* V */
    { 0x3F, 0x40, 0x38, 0x40, 0x3F }, /* W */
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, /* X */
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, /* Y */
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, /* Z */
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, /* space */
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, /* - */
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, /* . */
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, /* : */
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, /* / */
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, /* ? */
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, /* > */
    { 0x00, 0x00, 0x5F, 0x00, 0x00 }  /* ! */
};

static uint8_t fb[8][128];
static uint8_t present;
static uint8_t g_addr = 0x78u;   /* 8-bit write address, 0x78 = 0x3C<<1 */

/*---------------------------------------------------------------------------
 * software I2C
 *---------------------------------------------------------------------------*/
static void scl_high(void) { GPIO_SetBits(N1_OLED_PORT, N1_OLED_SCL_PIN); }
static void scl_low(void)  { GPIO_ResetBits(N1_OLED_PORT, N1_OLED_SCL_PIN); }

/* The CH32X035 GPIO library has no open-drain mode.  Drive SDA push-pull for
 * data bits (the SSD1306 never drives it then) and release it high to an
 * internal pull-up for the ACK bit, so the display can pull it low.  The
 * internal pull-up also covers modules with weak / missing onboard resistors. */
static void sda_output(void)
{
    GPIO_InitTypeDef g = {0};

    g.GPIO_Pin   = N1_OLED_SDA_PIN;
    g.GPIO_Mode  = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(N1_OLED_PORT, &g);
}

static void sda_input(void)
{
    GPIO_InitTypeDef g = {0};

    g.GPIO_Pin  = N1_OLED_SDA_PIN;
    g.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(N1_OLED_PORT, &g);
}

static void sda_high(void) { sda_output(); GPIO_SetBits(N1_OLED_PORT, N1_OLED_SDA_PIN); }
static void sda_low(void)  { sda_output(); GPIO_ResetBits(N1_OLED_PORT, N1_OLED_SDA_PIN); }
static void sda_release(void) { sda_input(); }

static uint8_t sda_read(void)
{
    return GPIO_ReadInputDataBit(N1_OLED_PORT, N1_OLED_SDA_PIN) ? 1u : 0u;
}

static void i2c_delay(void)
{
    Delay_Us(1);
}

static void i2c_start(void)
{
    sda_high();
    scl_high();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
    i2c_delay();
}

static void i2c_stop(void)
{
    sda_low();
    i2c_delay();
    scl_high();
    i2c_delay();
    sda_high();
    i2c_delay();
}

/* Free a bus left half-driven by a previous reset / hot-plug: release SDA, clock
 * SCL nine times so a stuck slave can finish its byte, then emit a STOP. */
static void i2c_bus_recover(void)
{
    uint8_t i;

    sda_release();
    for (i = 0; i < 9u; i++) {
        scl_low();
        i2c_delay();
        scl_high();
        i2c_delay();
    }
    sda_low();
    i2c_delay();
    scl_high();
    i2c_delay();
    sda_high();
    i2c_delay();
}

/* returns 0 when the slave acknowledged */
static uint8_t i2c_write(uint8_t b)
{
    uint8_t i;
    uint8_t ack;

    for (i = 0; i < 8u; i++) {
        if (b & 0x80u) {
            sda_high();
        } else {
            sda_low();
        }
        i2c_delay();
        scl_high();
        i2c_delay();
        scl_low();
        b <<= 1;
    }

    sda_release();
    i2c_delay();
    scl_high();
    i2c_delay();
    ack = sda_read();           /* 0 = ACK */
    scl_low();
    i2c_delay();
    sda_high();                 /* re-arm the output driver */

    return ack;
}

/* returns 0 when the device at addr8 acknowledges */
static uint8_t i2c_probe(uint8_t addr8)
{
    uint8_t ack;

    i2c_start();
    ack = i2c_write(addr8);
    i2c_stop();

    return ack;
}

static uint8_t oled_cmd(uint8_t c)
{
    uint8_t rc;

    i2c_start();
    rc  = i2c_write(g_addr);    /* SSD1306 write address */
    rc |= i2c_write(0x00u);     /* command stream */
    rc |= i2c_write(c);
    i2c_stop();

    return rc;
}

static void oled_data(const uint8_t *p, uint16_t len)
{
    uint16_t i;

    i2c_start();
    if (i2c_write(g_addr) || i2c_write(0x40u)) {
        i2c_stop();
        return;
    }

    for (i = 0; i < len; i++) {
        if (i2c_write(p[i])) {
            break;
        }
    }
    i2c_stop();
}

/*---------------------------------------------------------------------------
 * text
 *---------------------------------------------------------------------------*/
static uint8_t glyph_index(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'Z') return (uint8_t)(10 + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (uint8_t)(10 + (c - 'a'));

    switch (c) {
    case '-': return 37u;
    case '.': return 38u;
    case ':': return 39u;
    case '/': return 40u;
    case '?': return 41u;
    case '>': return 42u;
    case '!': return 43u;
    default:  return 36u;       /* space */
    }
}

void oled_clear(void)
{
    memset(fb, 0, sizeof(fb));
}

void oled_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= 128u || y >= 64u) {
        return;
    }

    if (on) {
        fb[y >> 3][x] |= (uint8_t)(1u << (y & 7u));
    } else {
        fb[y >> 3][x] &= (uint8_t)~(1u << (y & 7u));
    }
}

void oled_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            oled_pixel((uint8_t)(x + i), (uint8_t)(y + j), 1u);
        }
    }
}

void oled_power(uint8_t on)
{
    if (!present) {
        return;
    }
    (void)oled_cmd(on ? 0xAFu : 0xAEu);
}

void oled_set_brightness(uint8_t level)
{
    /* Map the shared 0..255 brightness onto a usable contrast window so that
     * --bright 0 is still readable and 255 is maximum. */
    uint8_t c = (uint8_t)(128u + (uint16_t)level / 2u);

    if (!present) {
        return;
    }
    (void)oled_cmd(0x81u);
    (void)oled_cmd(c);
}

void oled_text(uint8_t x, uint8_t page, const char *s)
{
    if (!present || page > 7u) {
        return;
    }

    while (*s && (uint16_t)x + 5u < 128u) {
        const uint8_t *g = font5x7[glyph_index(*s++)];
        uint8_t col;

        for (col = 0; col < 5u; col++) {
            fb[page][x + col] = g[col];
        }
        fb[page][x + 5u] = 0u;
        x = (uint8_t)(x + 6u);
    }
}

void oled_text_center(uint8_t page, const char *s)
{
    uint8_t len = (uint8_t)strlen(s);
    uint8_t w = (uint8_t)(len ? (len * 6u - 1u) : 0u);
    uint8_t x = (uint8_t)((128u - w) / 2u);

    oled_text(x, page, s);
}

void oled_num3(uint8_t x, uint8_t page, uint16_t value)
{
    char s[4];

    if (value > 999u) {
        value = 999u;
    }

    s[0] = (char)('0' + (value / 100u));
    s[1] = (char)('0' + ((value / 10u) % 10u));
    s[2] = (char)('0' + (value % 10u));
    s[3] = '\0';

    oled_text(x, page, s);
}

void oled_refresh_page(uint8_t page)
{
    if (!present || page > 7u) {
        return;
    }

    (void)oled_cmd((uint8_t)(0xB0u + page));
    (void)oled_cmd(0x00u);
    (void)oled_cmd(0x10u);
    oled_data(fb[page], 128u);
}

void oled_refresh(void)
{
    uint8_t page;

    if (!present) {
        return;
    }

    for (page = 0; page < 8u; page++) {
        oled_refresh_page(page);
    }
}

uint8_t oled_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,       /* display off */
        0xD5, 0x80, /* clock divide */
        0xA8, 0x3F, /* multiplex 64 */
        0xD3, 0x00, /* display offset */
        0x40,       /* start line 0 */
        0x8D, 0x14, /* charge pump on */
        0x20, 0x10, /* page addressing (matches oled_refresh_page) */
        0xA1,       /* segment remap */
        0xC8,       /* COM scan direction */
        0xDA, 0x12, /* COM pins */
        0x81, 0xCF, /* contrast */
        0xD9, 0xF1, /* precharge */
        0xDB, 0x40, /* VCOMH */
        0xA4,       /* resume from RAM */
        0xA6,       /* normal display */
        0xAF        /* display on */
    };
    GPIO_InitTypeDef gpio = {0};
    uint16_t i;
    uint8_t attempt;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin   = N1_OLED_SCL_PIN | N1_OLED_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(N1_OLED_PORT, &gpio);
    scl_high();
    sda_high();

    present = 0u;

    /* SSD1306 needs its supply to settle before the first command. */
    Delay_Ms(100);
    i2c_bus_recover();

    /* 0x3C is the usual address; some modules strap 0x3D. */
    if (i2c_probe(0x78u) == 0u) {
        g_addr = 0x78u;
    } else if (i2c_probe(0x7Au) == 0u) {
        g_addr = 0x7Au;
    } else {
        return 0u;
    }

    /* Two attempts: a marginal module sometimes drops the first byte. */
    for (attempt = 0; attempt < 2u; attempt++) {
        uint8_t ok = 1u;

        for (i = 0; i < sizeof(init_seq); i++) {
            if (oled_cmd(init_seq[i])) {
                ok = 0u;
                break;
            }
        }

        if (ok) {
            present = 1u;
            oled_clear();
            oled_refresh();
            return 1u;
        }

        Delay_Ms(10);
        i2c_bus_recover();
    }

    return 0u;
}

uint8_t oled_present(void)
{
    return present;
}

uint8_t oled_scan(uint8_t *out, uint8_t max)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t addr;
    uint8_t n = 0u;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    gpio.GPIO_Pin   = N1_OLED_SCL_PIN | N1_OLED_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(N1_OLED_PORT, &gpio);
    scl_high();
    sda_high();
    Delay_Ms(2);
    i2c_bus_recover();

    for (addr = 1u; addr < 128u && n < max; addr++) {
        if (i2c_probe((uint8_t)(addr << 1)) == 0u) {
            out[n++] = addr;
        }
    }

    return n;
}

#else /* !N1_BOARD_HAS_OLED */

uint8_t oled_init(void) { return 0u; }
uint8_t oled_present(void) { return 0u; }
void oled_clear(void) { }
void oled_pixel(uint8_t x, uint8_t y, uint8_t on) { (void)x; (void)y; (void)on; }
void oled_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h) { (void)x; (void)y; (void)w; (void)h; }
void oled_power(uint8_t on) { (void)on; }
void oled_set_brightness(uint8_t level) { (void)level; }
void oled_text(uint8_t x, uint8_t page, const char *s) { (void)x; (void)page; (void)s; }
void oled_text_center(uint8_t page, const char *s) { (void)page; (void)s; }
void oled_num3(uint8_t x, uint8_t page, uint16_t value) { (void)x; (void)page; (void)value; }
void oled_refresh(void) { }
void oled_refresh_page(uint8_t page) { (void)page; }
uint8_t oled_scan(uint8_t *out, uint8_t max) { (void)out; (void)max; return 0u; }

#endif
