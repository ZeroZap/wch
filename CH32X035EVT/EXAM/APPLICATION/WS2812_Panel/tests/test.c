/* Host side test for the portable modules of the WS2812_Panel project.
 *
 * It compiles panel.c / gfx.c / img_player.c with a stub ch32x035.h and
 * verifies:
 *   - the serpentine mapping table
 *   - WSP1 decoding (RLE565 / RAW565 / RGB332)
 *   - scaling of a 4x4 picture to the 16x16 panel
 *   - drawing primitives and clipping
 *   - error handling of broken / truncated files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "panel.h"
#include "gfx.h"
#include "wsp_img.h"
#include "img_demo.h"
#include "wsp_app.h"
#include "wsp_seg.h"
#include "wsp_fx.h"

/* --- stubs for the hardware modules ------------------------------------ */
static unsigned ws_pixels_sent;

void ws2812_init(void) {}
void ws2812_set_brightness(uint8_t level) { (void)level; }
void ws2812_begin(void) { ws_pixels_sent = 0; }
void ws2812_write(uint8_t r, uint8_t g, uint8_t b)
{
    (void)r; (void)g; (void)b;
    ws_pixels_sent++;
}
void ws2812_end(void) {}
void ws2812_show(const uint8_t *rgb, uint16_t n) { (void)rgb; ws_pixels_sent = n; }

/* panel.c now runs the current limiter and the badge; give them no-ops so the
 * portable modules can be tested on the host */
const uint8_t *ws2812_duty_table(void)
{
    static uint8_t identity[256];
    static uint8_t init_done;
    unsigned i;

    if (!init_done) {
        for (i = 0; i < 256; i++) {
            identity[i] = (uint8_t)i;
        }
        init_done = 1;
    }
    return identity;
}
void ws2812_limit_frame(uint32_t duty_sum) { (void)duty_sum; }
uint8_t ws2812_limited(void) { return 0; }

void osd_draw(void) {}
void osd_tick(void) {}
void osd_init(void) {}
void osd_set(uint8_t on) { (void)on; }
uint8_t osd_on(void) { return 0; }
uint16_t osd_fps(void) { return 0; }

void Delay_Ms(uint32_t n) { (void)n; }
void Delay_Us(uint32_t n) { (void)n; }

/* --- helpers ---------------------------------------------------------- */
static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);         \
            failures++;                                                    \
        }                                                                  \
    } while (0)

#define CHECK_EQ(a, b)                                                     \
    do {                                                                   \
        long _a = (long)(a), _b = (long)(b);                                \
        if (_a != _b) {                                                    \
            printf("FAIL %s:%d  %s=%ld expected %ld\n", __FILE__, __LINE__, \
                   #a, _a, _b);                                           \
            failures++;                                                    \
        }                                                                  \
    } while (0)

static uint8_t *load(const char *name, uint32_t *size)
{
    FILE *f = fopen(name, "rb");
    long len;
    uint8_t *buf;

    if (!f) {
        printf("cannot open %s\n", name);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc((size_t)len);
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
        printf("short read %s\n", name);
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (uint32_t)len;
    return buf;
}

static const uint8_t *pixel(uint16_t x, uint16_t y)
{
    return &panel_fb[(uint32_t)(y * PANEL_W + x) * 3u];
}

/* RGB565 -> expanded RGB888, same as panel_set_pixel565() */
static void expect565(uint16_t c, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t r5 = (uint8_t)((c >> 11) & 0x1F);
    uint8_t g6 = (uint8_t)((c >> 5) & 0x3F);
    uint8_t b5 = (uint8_t)(c & 0x1F);

    *r = (uint8_t)((r5 << 3) | (r5 >> 2));
    *g = (uint8_t)((g6 << 2) | (g6 >> 4));
    *b = (uint8_t)((b5 << 3) | (b5 >> 2));
}

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/* --- tests ------------------------------------------------------------ */
static void test_mapping(void)
{
    const uint16_t W = PANEL_W;

    /* row 0 : left -> right */
    CHECK_EQ(panel_index(0, 0), 0);
    CHECK_EQ(panel_index(1, 0), 1);
    CHECK_EQ(panel_index(W - 1, 0), W - 1);

    /* row 1 : right -> left, strip position W is the right end */
    CHECK_EQ(panel_index(W - 1, 1), W);
    CHECK_EQ(panel_index(W - 2, 1), W + 1);
    CHECK_EQ(panel_index(0, 1), 2 * W - 1);

    /* row 2 : left -> right again */
    CHECK_EQ(panel_index(0, 2), 2 * W);
    CHECK_EQ(panel_index(W - 1, 2), 3 * W - 1);

    /* last row (odd) ends at the left edge */
    CHECK_EQ(panel_index(0, PANEL_H - 1), PANEL_PIXELS - 1);

    /* out of range */
    CHECK_EQ(panel_index(PANEL_W, 0), 0xFFFF);
    CHECK_EQ(panel_index(0, PANEL_H), 0xFFFF);
}

static void test_draw(void)
{
    uint16_t c = rgb565(255, 0, 0);
    uint8_t r, g, b;

    gfx_clear();
    gfx_pixel565(3, 4, c);
    expect565(c, &r, &g, &b);
    CHECK_EQ(pixel(3, 4)[0], r);
    CHECK_EQ(pixel(3, 4)[1], g);
    CHECK_EQ(pixel(3, 4)[2], b);
    CHECK_EQ(pixel(4, 4)[0], 0);

    /* clipping: must not write outside the panel */
    gfx_clear();
    gfx_hline((uint16_t)(PANEL_W - 2), (uint16_t)(PANEL_H - 1), 8, c);
    CHECK_EQ(pixel((uint16_t)(PANEL_W - 1), (uint16_t)(PANEL_H - 1))[0], r);
    gfx_clear();
    gfx_pixel565(200, 200, c);
    CHECK_EQ(panel_fb[0], 0);

    /* line + rect sanity */
    gfx_clear();
    gfx_line(0, 0, (uint16_t)(PANEL_W - 1), (uint16_t)(PANEL_H - 1), c);
    CHECK(pixel(0, 0)[0] != 0);
    CHECK(pixel((uint16_t)(PANEL_W - 1), (uint16_t)(PANEL_H - 1))[0] != 0);
    CHECK(pixel((uint16_t)(PANEL_W / 2), (uint16_t)(PANEL_H / 2))[0] != 0);

    gfx_clear();
    gfx_fill_rect(2, 2, 4, 4, c);
    CHECK(pixel(2, 2)[0] != 0);
    CHECK(pixel(5, 5)[0] != 0);
    CHECK(pixel(6, 6)[0] == 0);
}

static void test_image(const char *file, int spot_x, int spot_y, uint16_t spot)
{
    uint32_t size = 0;
    uint8_t *buf = load(file, &size);
    img_src_t src;
    img_t img;
    uint8_t r, g, b;

    if (!buf) {
        failures++;
        return;
    }

    img_src_mem(&src, buf, size);
    CHECK_EQ(img_open(&img, &src), IMG_OK);
    CHECK_EQ(img_show(&img, 0), IMG_OK);

    expect565(spot, &r, &g, &b);
    /* the fixtures are 16x16: only probe pixels when the panel is 1:1 with
     * the source, otherwise img_show() above is the smoke test */
    if ((PANEL_W == img.hdr.width) && (PANEL_H == img.hdr.height)) {
        CHECK_EQ(pixel(spot_x, spot_y)[0], r);
        CHECK_EQ(pixel(spot_x, spot_y)[1], g);
        CHECK_EQ(pixel(spot_x, spot_y)[2], b);
    }

    free(buf);
}

static void test_anim(void)
{
    uint32_t size = 0;
    uint8_t *buf = load("anim.wsp", &size);
    img_src_t src;
    img_t img;

    if (!buf) {
        failures++;
        return;
    }

    img_src_mem(&src, buf, size);
    CHECK_EQ(img_open(&img, &src), IMG_OK);
    CHECK_EQ(img.hdr.frames, 8);
    CHECK_EQ(img.hdr.frame_ms, 60);

    /* every frame must decode; the bright column moves by 2 px */
    CHECK_EQ(img_show(&img, 0), IMG_OK);
    CHECK(pixel(0, 0)[0] > 240 && pixel(0, 0)[1] > 240 && pixel(0, 0)[2] > 240);
    CHECK_EQ(img_show(&img, 3), IMG_OK);

    free(buf);
}

static void test_scale(void)
{
    /* 4x4 -> 16x16 : nearest neighbour, 4x4 blocks */
    uint32_t size = 0;
    uint8_t *buf = load("small.wsp", &size);
    img_src_t src;
    img_t img;
    uint8_t r, g, b;
    uint16_t colours[16];
    int i, j;

    if (!buf) {
        failures++;
        return;
    }

    colours[0]  = rgb565(255, 0, 0);
    colours[1]  = rgb565(0, 255, 0);
    colours[2]  = rgb565(0, 0, 255);
    colours[3]  = rgb565(255, 255, 0);
    colours[4]  = rgb565(255, 255, 255);
    colours[5]  = rgb565(0, 0, 0);
    colours[6]  = rgb565(0, 255, 255);
    colours[7]  = rgb565(255, 0, 255);
    colours[8]  = rgb565(128, 128, 128);
    colours[9]  = rgb565(64, 64, 64);
    colours[10] = rgb565(200, 100, 50);
    colours[11] = rgb565(10, 200, 30);
    colours[12] = rgb565(1, 2, 3);
    colours[13] = rgb565(250, 250, 250);
    colours[14] = rgb565(30, 60, 90);
    colours[15] = rgb565(120, 0, 240);

    img_src_mem(&src, buf, size);
    CHECK_EQ(img_open(&img, &src), IMG_OK);
    CHECK_EQ(img_show(&img, 0), IMG_OK);

    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            /* centre of the scaled block, works for any panel size */
            int bw = PANEL_W / 4;
            int bh = PANEL_H / 4;
            int px = i * bw + bw / 2;
            int py = j * bh + bh / 2;

            expect565(colours[j * 4 + i], &r, &g, &b);
            CHECK_EQ(pixel((uint16_t)px, (uint16_t)py)[0], r);
            CHECK_EQ(pixel((uint16_t)px, (uint16_t)py)[1], g);
            CHECK_EQ(pixel((uint16_t)px, (uint16_t)py)[2], b);
        }
    }

    free(buf);
}

static void test_identical_encodings(void)
{
    /* RLE565 and RAW565 must produce the same framebuffer */
    static uint8_t fb_rle[PANEL_PIXELS * 3];
    uint32_t s1 = 0, s2 = 0;
    uint8_t *a = load("still.wsp", &s1);
    uint8_t *b = load("still_raw.wsp", &s2);
    img_src_t src;
    img_t img;

    if (!a || !b) {
        failures++;
        free(a);
        free(b);
        return;
    }

    img_src_mem(&src, a, s1);
    CHECK_EQ(img_open(&img, &src), IMG_OK);
    CHECK_EQ(img_show(&img, 0), IMG_OK);
    memcpy(fb_rle, panel_fb, sizeof(fb_rle));

    img_src_mem(&src, b, s2);
    CHECK_EQ(img_open(&img, &src), IMG_OK);
    CHECK_EQ(img_show(&img, 0), IMG_OK);
    CHECK_EQ(memcmp(fb_rle, panel_fb, sizeof(fb_rle)), 0);

    free(a);
    free(b);
}

static void test_errors(void)
{
    uint32_t size = 0;
    uint8_t *buf;
    img_src_t src;
    img_t img;

    buf = load("bad_magic.wsp", &size);
    if (buf) {
        img_src_mem(&src, buf, size);
        CHECK_EQ(img_open(&img, &src), IMG_ERR_MAGIC);
        free(buf);
    }

    buf = load("truncated.wsp", &size);
    if (buf) {
        img_src_mem(&src, buf, size);
        CHECK_EQ(img_open(&img, &src), IMG_OK);
        CHECK(img_show(&img, 0) != IMG_OK);
        free(buf);
    }
}

static void test_builtin_assets(void)
{
    /* exactly the blobs that are compiled into the firmware */
    img_src_t src;
    img_t img;
    uint8_t r, g, b;

    img_src_mem(&src, img_demo_still_data, img_demo_still_size);
    CHECK_EQ(img_open(&img, &src), IMG_OK);
    CHECK_EQ(img.hdr.width, 16);
    CHECK_EQ(img.hdr.height, 16);
    CHECK_EQ(img.hdr.frames, 1);
    CHECK_EQ(img_show(&img, 0), IMG_OK);
    expect565(rgb565(255, 200, 0), &r, &g, &b);
    if ((PANEL_W == img.hdr.width) && (PANEL_H == img.hdr.height)) {
        CHECK_EQ(pixel(7, 7)[0], r);
        CHECK_EQ(pixel(7, 7)[1], g);
        CHECK_EQ(pixel(7, 7)[2], b);
    }

    img_src_mem(&src, img_demo_anim_data, img_demo_anim_size);
    CHECK_EQ(img_open(&img, &src), IMG_OK);
    CHECK_EQ(img.hdr.frames, 8);
    CHECK_EQ(img.hdr.frame_ms, 60);
    CHECK_EQ(img_show(&img, 7), IMG_OK);

    img_src_mem(&src, img_demo_small_data, img_demo_small_size);
    CHECK_EQ(img_open(&img, &src), IMG_OK);
    CHECK_EQ(img.hdr.width, 8);
    CHECK_EQ(img_show(&img, 0), IMG_OK);
}

static void test_segments(void)
{
    wsp_seg_t *s;
    int i;

    app_set_mode(WSP_MODE_PANEL);
    app_clear();
    seg_init();

    s = seg_config_rect(0, 4, 4, 8, 8, FX_STATIC);
    seg_colors(0, rgb565(255, 0, 0), 0, 0);
    seg_reset_time(0);
    seg_render(0);

    CHECK(pixel(4, 4)[0] > 200);
    {
        /* far corner of the 8x8 rect, clamped to the panel */
        uint16_t cx = (uint16_t)((11 < PANEL_W) ? 11 : (PANEL_W - 1));
        uint16_t cy = (uint16_t)((11 < PANEL_H) ? 11 : (PANEL_H - 1));

        CHECK(pixel(cx, cy)[0] > 200);
    }
    CHECK(pixel(3, 4)[0] == 0);
    if ((PANEL_W > 12) && (PANEL_H > 12)) {
        CHECK(pixel(12, 12)[0] == 0);
    }

    /* brightness 0 hides the segment */
    s->brightness = 0;
    app_clear();
    seg_render(0);
    CHECK(pixel(4, 4)[0] == 0);

    /* clipping: rect that sticks out of the panel must not corrupt memory */
    seg_init();
    app_clear();
    s = seg_config_rect(0, (uint16_t)(PANEL_W - 3), (uint16_t)(PANEL_H - 3), 8, 8,
                        FX_STATIC);
    seg_colors(0, rgb565(0, 255, 0), 0, 0);
    seg_reset_time(0);
    seg_render(0);
    CHECK(pixel((uint16_t)(PANEL_W - 1), (uint16_t)(PANEL_H - 1))[1] > 200);
    for (i = 0; i < PANEL_PIXELS; i++) {
        const uint8_t *p = &panel_fb[i * 3];
        CHECK((p[1] > 200) || ((p[0] + p[1] + p[2]) == 0));
    }
}

static void test_strip_segment_order(void)
{
    wsp_seg_t *s;
    uint16_t logical;

    app_set_mode(WSP_MODE_PANEL);
    seg_init();
    app_clear();

    s = seg_config_strip(0, 0, PANEL_PIXELS, FX_RUNNING_DOT);
    seg_colors(0, rgb565(255, 255, 255), 0, 0);
    s->speed = 255;                 /* period 60ms */
    seg_reset_time(0);
    seg_render(0);

    logical = panel_phys_to_logical(0);
    CHECK(pixel(logical % PANEL_W, logical / PANEL_W)[0] > 200);

    /* one period later the dot is on strip position 1 */
    app_clear();
    seg_render(70);
    logical = panel_phys_to_logical(1);
    CHECK(pixel(logical % PANEL_W, logical / PANEL_W)[0] > 200);

    /* reverse must mirror the segment: the dot on cell 0 now lands on the
     * last physical LED (this used to be cancelled out by a second reverse
     * inside the effects) */
    seg_init();
    app_clear();
    s = seg_config_strip(0, 0, PANEL_PIXELS, FX_RUNNING_DOT);
    seg_colors(0, rgb565(255, 255, 255), 0, 0);
    s->speed = 255;
    s->reverse = 1;
    seg_reset_time(0);
    seg_render(0);

    logical = panel_phys_to_logical((uint16_t)(PANEL_PIXELS - 1));
    CHECK(pixel(logical % PANEL_W, logical / PANEL_W)[0] > 200);

    logical = panel_phys_to_logical(0);
    CHECK(pixel(logical % PANEL_W, logical / PANEL_W)[0] == 0);
}

static void test_blend_overlay(void)
{
    wsp_seg_t *ov;

    app_set_mode(WSP_MODE_PANEL);
    seg_init();
    app_clear();

    seg_config_rect(0, 0, 0, PANEL_W, PANEL_H, FX_STATIC);
    seg_colors(0, rgb565(0, 0, 80), 0, 0);

    ov = seg_config_rect(1, 0, 0, PANEL_W, 1, FX_STATIC);
    seg_colors(1, rgb565(0, 0, 80), 0, 0);
    ov->blend = SEG_BLEND_ADD;

    seg_render(0);
    CHECK(pixel(0, 0)[2] > 120);
    CHECK((pixel(0, 1)[2] > 60) && (pixel(0, 1)[2] < 100));
}

static void test_effects_smoke(void)
{
    uint8_t  mode;
    wsp_seg_t *s;
    int i;

    app_set_mode(WSP_MODE_PANEL);
    seg_init();
    app_clear();

    s = seg_config_rect(0, 0, 0, PANEL_W, PANEL_H, FX_STATIC);
    seg_colors(0, rgb565(255, 255, 255), rgb565(0, 0, 255), 0);
    s->intensity = 255;
    seg_reset_time(0);

    for (mode = 0; mode < FX_MODE_COUNT; mode++) {
        int lit = 0;

        app_clear();
        s->mode = mode;
        seg_render(123);

        for (i = 0; i < PANEL_PIXELS; i++) {
            if (panel_fb[i * 3] || panel_fb[i * 3 + 1] || panel_fb[i * 3 + 2]) {
                lit = 1;
                break;
            }
        }
        if (!lit) {
            printf("FAIL effect %u (%s) produced an empty frame\n",
                   (unsigned)mode, fx_name(mode));
            failures++;
        }
    }
}

static void test_scenes(void)
{
    int i;

    app_set_mode(WSP_MODE_PANEL);

    seg_init();
    app_clear();
    seg_scene_point();
    seg_render(100);
    CHECK(pixel(0, 0)[0] || pixel(0, 0)[1] || pixel(0, 0)[2]);

    seg_clear();
    app_clear();
    seg_scene_line();
    seg_render(100);
    {
        int lit = 0;
        for (i = 0; i < PANEL_PIXELS; i++) {
            if (panel_fb[i * 3] || panel_fb[i * 3 + 1] || panel_fb[i * 3 + 2]) {
                lit++;
            }
        }
        CHECK(lit > 8);
    }

    seg_clear();
    app_clear();
    seg_scene_plane();
    seg_render(100);
    {
        int lit = 0;
        for (i = 0; i < PANEL_PIXELS; i++) {
            if (panel_fb[i * 3] || panel_fb[i * 3 + 1] || panel_fb[i * 3 + 2]) {
                lit++;
            }
        }
        CHECK(lit > PANEL_PIXELS / 4);
    }
}

static void test_show_count(void)
{
    /* panel_show must push exactly one RGB triplet per LED */
    gfx_clear();
    ws_pixels_sent = 0;
    panel_show();
    CHECK_EQ(ws_pixels_sent, PANEL_PIXELS);
}

int main(void)
{
    panel_init();

    test_mapping();
    test_draw();
    test_image("still.wsp", 7, 7, rgb565(255, 200, 0));
    test_image("still_raw.wsp", 7, 7, rgb565(255, 200, 0));
    test_image("still_332.wsp", 0, 0, rgb565(0, 0, 0));
    test_anim();
    test_scale();
    test_identical_encodings();
    test_errors();
    test_builtin_assets();
    test_segments();
    test_strip_segment_order();
    test_blend_overlay();
    test_effects_smoke();
    test_scenes();
    test_show_count();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    printf("%d FAILURE(S)\n", failures);
    return 1;
}
