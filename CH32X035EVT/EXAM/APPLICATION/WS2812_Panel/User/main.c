/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2026/11/20
 * Description        : WS2812 strip / panel demo with segments and effects.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 * Two display modes, switched with a long press on the user button:
 *
 *   STRIP mode : one linear LED strip (STRIP_LEDS), short press cycles the
 *                WS2812FX style effects.
 *   PANEL mode : serpentine LED panel, short press cycles scenes:
 *                still image -> scaled image -> animation -> NOR animation
 *                -> point -> line -> plane (segment composites)
 *
 * Hardware
 *   WS2812 data : PC7 (PIOC IO0 via GPIO_Remap_PIOC, SDI debug unaffected)
 *   User button : PA0, active low, internal pull-up
 *   NOR flash   : SCK PA5, MISO PA6, MOSI PA7, CS PA2 (optional, W25Qxx/GD25Qxx)
 *   Debug UART  : PB10 (USART1 TX), 115200 8N1
 *
 * Images are packed with tools/wsp_pack.py, see README.md.
 */

#include "debug.h"
#include "wsp_config.h"
#include "panel.h"
#include "ws2812.h"
#include "gfx.h"
#include "wsp_app.h"
#include "wsp_tick.h"
#include "wsp_seg.h"
#include "wsp_fx.h"
#include "wsp_button.h"
#include "wsp_img.h"
#include "img_demo.h"

#if WSP_NOR_ENABLE
#include "norflash.h"
#endif

#include "wsp_cfg.h"

#if WSP_USB_ENABLE
#include "wsp_usb.h"
#endif

#if WSP_OSD_ENABLE
#include "wsp_osd.h"
#endif

#define ARRAY_SIZE(a)           (sizeof(a) / sizeof((a)[0]))

#define DEMO_HOLD_MS            2500

static const char *color_order_name(uint8_t order)
{
    static const char *const names[] = { "RGB", "GRB", "BRG", "BGR", "RBG", "GBR" };

    return (order < ARRAY_SIZE(names)) ? names[order] : "?";
}

#if WSP_USB_ENABLE
/*********************************************************************
 * @fn      usb_yield
 *
 * @brief   Service the USB protocol once and report whether the host is
 *          currently streaming.  Every local animation loop calls this and
 *          stands still while it returns 1, so USB frames win over the
 *          built-in demo content and the two never draw on top of each other.
 *
 * @return  1 when the host owns the panel.
 */
static uint8_t usb_yield(void)
{
    (void)wsp_usb_service();
    return wsp_usb_active();
}
#else
#define usb_yield()             0
#endif

/*---------------------------------------------------------------------------
 * STRIP mode
 *---------------------------------------------------------------------------*/
static const uint8_t strip_effects[] = {
    FX_RAINBOW, FX_SCANNER, FX_THEATER_CHASE, FX_BREATH,
    FX_FIRE, FX_PLASMA, FX_SPARKLE, FX_GRADIENT,
    FX_WIPE, FX_RUNNING_DOT, FX_BLINK, FX_STATIC,
    FX_WAVE, FX_CHASER, FX_BOUNCE, FX_METEOR,
};

static void strip_apply(uint8_t index)
{
    uint8_t     mode = strip_effects[index % ARRAY_SIZE(strip_effects)];
    uint8_t     hue = (uint8_t)(index * 29u);
    wsp_seg_t  *s;

    seg_clear();
    app_clear();

    s = seg_config_strip(0, 0, STRIP_LEDS, mode);
    s->speed = 128;
    s->intensity = 128;
    s->brightness = 255;
    s->blend = SEG_BLEND_REPLACE;

    seg_colors(0, fx_hsv565(hue, 255, 255),
                  fx_color565(0, 0, 0),
                  fx_hsv565((uint8_t)(hue + 128u), 255, 255));

    seg_reset_time(tick_ms());

    printf("STRIP effect %u: %s\r\n", (unsigned)index, fx_name(mode));
}

static void enter_strip(void)
{
    app_set_mode(WSP_MODE_STRIP);
    printf("mode: STRIP, %u LEDs\r\n", (unsigned)STRIP_LEDS);
}

/*---------------------------------------------------------------------------
 * PANEL mode
 *---------------------------------------------------------------------------*/
enum {
    P_STILL = 0,
    P_SMALL,
    P_ANIM,
#if WSP_NOR_ENABLE
    P_NOR,
#endif
    P_POINT,
    P_LINE,
    P_PLANE,
};

static const uint8_t panel_sequence[] = {
    P_STILL, P_SMALL, P_ANIM,
#if WSP_NOR_ENABLE
    P_NOR,
#endif
    P_POINT, P_LINE, P_PLANE,
};

static void enter_panel(void)
{
    app_set_mode(WSP_MODE_PANEL);
    printf("mode: PANEL, %ux%u\r\n", (unsigned)PANEL_W, (unsigned)PANEL_H);
}

/*********************************************************************
 * @fn      panel_wait
 *
 * @brief   Wait while polling the button.
 *
 * @param   ms - time to wait.
 *
 * @return  1 when the user asked for the next item (or switched mode).
 */
static uint8_t panel_wait(uint32_t ms)
{
    uint32_t start = tick_ms();

    while ((tick_ms() - start) < ms) {
        if (usb_yield()) {
            Delay_Ms(2);
            continue;
        }

        button_poll();

        if (button_long()) {
            enter_strip();
            return 1;
        }

        if (button_short()) {
            return 1;
        }

        Delay_Ms(5);
    }

    return 0;
}

static void panel_show_mem(const uint8_t *data, uint32_t size, uint32_t hold)
{
    img_src_t src;
    img_t     img;

    img_src_mem(&src, data, size);
    if (img_open(&img, &src) != IMG_OK) {
        return;
    }

    img_show(&img, 0);
    panel_wait(hold);
}

static void panel_play_mem(const uint8_t *data, uint32_t size, uint16_t loops)
{
    img_src_t src;
    img_t     img;
    uint16_t  f;
    uint16_t  l;

    img_src_mem(&src, data, size);
    if (img_open(&img, &src) != IMG_OK) {
        return;
    }

    for (l = 0; l < loops; l++) {
        for (f = 0; f < img.hdr.frames; f++) {
            if (img_show(&img, f) != IMG_OK) {
                return;
            }
            if (panel_wait(img.hdr.frame_ms ? img.hdr.frame_ms : 60)) {
                return;
            }
        }
    }
}

#if WSP_NOR_ENABLE
static void panel_play_nor(void)
{
    img_src_t src;
    img_t     img;
    uint16_t  f;
    int       rc;

    nor_img_src(&src, 0);
    rc = img_open(&img, &src);
    if (rc != IMG_OK) {
        /* nothing programmed yet: say so instead of silently skipping */
        printf("NOR item: no image at 0 (err %d), flash %u bytes\r\n",
               rc, (unsigned)nor_size());
        return;
    }

    for (f = 0; f < img.hdr.frames; f++) {
        if (img_show(&img, f) != IMG_OK) {
            return;
        }
        if (panel_wait(img.hdr.frame_ms ? img.hdr.frame_ms : 60)) {
            return;
        }
    }
}
#endif

/*********************************************************************
 * @fn      panel_run_segments
 *
 * @brief   Animate the configured segment scene.
 *
 * @param   duration - milliseconds.
 *
 * @return  none
 */
static void panel_run_segments(uint32_t duration)
{
    uint32_t start = tick_ms();

    seg_reset_time(start);

    while ((tick_ms() - start) < duration) {
        if (usb_yield()) {
            Delay_Ms(2);
            continue;
        }

        button_poll();

        if (button_long()) {
            enter_strip();
            return;
        }

        if (button_short()) {
            return;
        }

        seg_service(tick_ms());
        Delay_Ms(10);
    }
}

/*********************************************************************
 * @fn      startup_check
 *
 * @brief   Colour bars and origin check, only useful in PANEL mode.
 *
 * @return  none
 */
static void startup_check(void)
{
    uint16_t bright;

    static const uint16_t bars[8] = {
        RGB565(255, 255, 255), RGB565(255, 255, 0), RGB565(0, 255, 255),
        RGB565(0, 255, 0),     RGB565(255, 0, 255), RGB565(255, 0, 0),
        RGB565(0, 0, 255),     RGB565(0, 0, 0)
    };
    uint16_t x;

    /* top-left dim red dot: verifies origin and serpentine direction */
    gfx_clear();
    panel_set_pixel(0, 0, 32, 0, 0);
    panel_show();

    if (ws2812_last_error() != 0) {
        printf("PIOC 1-wire error: 0x%02X (check the WS2812 data pin)\r\n",
               ws2812_last_error());
    }

    panel_wait(500);
    if (app_mode() != WSP_MODE_PANEL) {
        return;
    }

    for (x = 0; x < PANEL_W; x++) {
        gfx_vline(x, 0, PANEL_H, bars[(uint32_t)x * 8u / PANEL_W]);
    }

    for (bright = 16; bright <= 256; bright += 32) {
        ws2812_set_brightness((uint8_t)((bright > 255) ? 255 : bright));
        panel_show();
        if (panel_wait(30)) {
            break;
        }
    }
    ws2812_set_brightness(WS2812_BRIGHTNESS);
    if (app_mode() != WSP_MODE_PANEL) {
        return;
    }
    panel_wait(700);
}

/*---------------------------------------------------------------------------
 * mode runners
 *---------------------------------------------------------------------------*/
static void run_strip_mode(void)
{
    uint8_t index = 0;

    strip_apply(index);

    for (;;) {
        if (usb_yield()) {
            Delay_Ms(2);
            continue;
        }

        button_poll();

        if (button_long()) {
            enter_panel();
            return;
        }

        if (button_short()) {
            index = (uint8_t)((index + 1u) % ARRAY_SIZE(strip_effects));
            strip_apply(index);
        }

        seg_service(tick_ms());
        Delay_Ms(10);
    }
}

static void run_panel_mode(void)
{
    uint8_t item = 0;

    seg_init();
    seg_clear();
    printf("PANEL items: image / scaled / animation / NOR / point / line / plane\r\n");

    for (;;) {
        if (app_mode() != WSP_MODE_PANEL) {
            return;
        }

        if (usb_yield()) {
            Delay_Ms(2);
            continue;
        }

        switch (panel_sequence[item]) {
        case P_STILL:
            panel_show_mem(img_demo_still_data, img_demo_still_size, DEMO_HOLD_MS);
            break;

        case P_SMALL:
            panel_show_mem(img_demo_small_data, img_demo_small_size, 2000);
            break;

        case P_ANIM:
            panel_play_mem(img_demo_anim_data, img_demo_anim_size, 3);
            break;

#if WSP_NOR_ENABLE
        case P_NOR:
            panel_play_nor();
            panel_wait(300);
            break;
#endif

        case P_POINT:
            seg_clear();
            seg_scene_point();
            panel_run_segments(6000);
            break;

        case P_LINE:
            seg_clear();
            seg_scene_line();
            panel_run_segments(6000);
            break;

        case P_PLANE:
            seg_clear();
            seg_scene_plane();
            panel_run_segments(8000);
            break;

        default:
            break;
        }

        if (app_mode() != WSP_MODE_PANEL) {
            return;
        }

        item = (uint8_t)((item + 1u) % ARRAY_SIZE(panel_sequence));
    }
}

/*---------------------------------------------------------------------------
 * main
 *---------------------------------------------------------------------------*/
int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    tick_init();
    panel_init();
    ws2812_init();
    button_init();
    seg_init();

#if WSP_OSD_ENABLE
    osd_init();
#endif

    printf("\r\nWS2812 demo: panel %dx%d, strip %u LEDs\r\n",
           PANEL_W, PANEL_H, (unsigned)STRIP_LEDS);

#if WSP_NOR_ENABLE
    if (nor_init()) {
        printf("NOR flash: %s JEDEC 0x%06X, %u bytes\r\n",
               nor_vendor(), (unsigned)nor_jedec_id(), (unsigned)nor_size());
    } else {
        printf("NOR flash: not found (JEDEC 0x%06X)\r\n",
               (unsigned)nor_jedec_id());
    }
#if WSP_NOR_SELFTEST
    printf("NOR selftest: %s\r\n", nor_selftest() ? "OK" : "FAIL");
#endif
#endif

#if WSP_USB_ENABLE
    /* settings: 1-wire bit rate, colour order, brightness, default mode */
    wsp_cfg_load();
    wsp_cfg_apply();
    printf("cfg: 1-wire %u cyc (~%u ns/bit), order %s, bright %u, mode %s\r\n",
           (unsigned)wsp_cfg()->wire_cyc,
           (unsigned)((wsp_cfg()->wire_cyc * 1000u) / 48u),
           color_order_name(wsp_cfg()->order),
           (unsigned)wsp_cfg()->brightness,
           (wsp_cfg()->mode == WSP_MODE_STRIP) ? "STRIP" : "PANEL");

    wsp_usb_init();
    printf("USB CDC: waiting for host (PC16/PC17)\r\n");
#else
    wsp_cfg_load();
    wsp_cfg_apply();
    printf("cfg: 1-wire %u cyc, order %s, bright %u\r\n",
           (unsigned)wsp_cfg()->wire_cyc,
           color_order_name(wsp_cfg()->order),
           (unsigned)wsp_cfg()->brightness);
#endif

    /* start black, then enter the default mode */
    app_set_mode(wsp_cfg()->mode);
    ws2812_set_brightness(0);
    app_clear();
    app_show();
    Delay_Ms(20);
    ws2812_set_brightness(wsp_cfg()->brightness);

    if (app_mode() == WSP_MODE_PANEL) {
        printf("button: short = next item, long = switch to STRIP\r\n");
        startup_check();
    } else {
        printf("button: short = next effect, long = switch to PANEL\r\n");
        enter_strip();
    }

    for (;;) {
        if (app_mode() == WSP_MODE_STRIP) {
            run_strip_mode();
        } else {
            run_panel_mode();
        }
    }
}
