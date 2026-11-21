/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_player.c
 * Description        : Decode "WSP1" frames and push them to the panel.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include <string.h>
#include "debug.h"
#include "wsp_img.h"
#include "gfx.h"

#define WSP_READ_CHUNK          64

/* decoded frame, RGB565 */
static uint16_t img_scratch[IMG_SCRATCH_PIXELS];

typedef struct
{
    img_t   *img;
    uint32_t pos;
    uint16_t len;
    uint16_t idx;
    uint8_t  eof;
    uint8_t  buf[WSP_READ_CHUNK];
} img_rd_t;

/*********************************************************************
 * @fn      img_src_mem_read
 *
 * @brief   Read callback for memory mapped sources (internal flash).
 *
 * @return  number of bytes read.
 */
uint32_t img_src_mem_read(void *ctx, uint32_t off, void *buf, uint32_t len)
{
    const uint8_t *base = (const uint8_t *)ctx;

    memcpy(buf, base + off, len);
    return len;
}

/*********************************************************************
 * @fn      img_src_mem
 *
 * @brief   Prepare an img_src_t that points at a memory mapped blob.
 *
 * @return  none
 */
void img_src_mem(img_src_t *src, const void *base, uint32_t size)
{
    src->read = img_src_mem_read;
    src->ctx  = (void *)base;
    src->size = size;
}

static int rd_fill(img_rd_t *rd)
{
    uint32_t got;

    if ((rd->img->src.size != 0) && (rd->pos >= rd->img->src.size)) {
        rd->eof = 1;
        return 0;
    }

    got = rd->img->src.read(rd->img->src.ctx, rd->pos, rd->buf, WSP_READ_CHUNK);
    if (got == 0) {
        rd->eof = 1;
        return 0;
    }

    rd->pos += got;
    rd->len = (uint16_t)got;
    rd->idx = 0;

    return 1;
}

static uint8_t rd_u8(img_rd_t *rd)
{
    if (rd->idx >= rd->len) {
        if (!rd_fill(rd)) {
            return 0;
        }
    }

    return rd->buf[rd->idx++];
}

static uint16_t rd_u16(img_rd_t *rd)
{
    uint16_t lo = rd_u8(rd);
    uint16_t hi = rd_u8(rd);

    return (uint16_t)(lo | (hi << 8));
}

/*********************************************************************
 * @fn      img_open
 *
 * @brief   Read and validate the WSP1 header.
 *
 * @param   img - player handle.
 *          src - data source.
 *
 * @return  IMG_OK or a negative error code.
 */
int img_open(img_t *img, const img_src_t *src)
{
    uint32_t total;

    if ((img == 0) || (src == 0) || (src->read == 0)) {
        return IMG_ERR_ARG;
    }

    img->src = *src;
    img->cur = 0;

    if (src->read(src->ctx, 0, &img->hdr, sizeof(wsp_hdr_t)) != sizeof(wsp_hdr_t)) {
        return IMG_ERR_SRC;
    }

    if (img->hdr.magic != WSP_MAGIC) {
        return IMG_ERR_MAGIC;
    }

    if ((img->hdr.format > WSP_FMT_RAW332) || (img->hdr.width == 0) ||
        (img->hdr.height == 0) || (img->hdr.frames == 0)) {
        return IMG_ERR_FORMAT;
    }

    total = (uint32_t)img->hdr.width * img->hdr.height;
    if (total > IMG_SCRATCH_PIXELS) {
        return IMG_ERR_TOO_BIG;
    }

    if ((src->size != 0) &&
        ((WSP_HDR_SIZE + 4u * img->hdr.frames) > src->size)) {
        return IMG_ERR_FORMAT;
    }

    return IMG_OK;
}

/*********************************************************************
 * @fn      img_decode
 *
 * @brief   Decode one frame into img_scratch.
 *
 * @return  IMG_OK or a negative error code.
 */
static int img_decode(img_t *img, uint16_t frame)
{
    img_rd_t rd = {0};
    uint32_t off = 0;
    uint32_t total;
    uint32_t n = 0;
    uint16_t color;
    uint8_t  count, v, r, g, b;

    if (frame >= img->hdr.frames) {
        return IMG_ERR_ARG;
    }

    if (img->src.read(img->src.ctx, WSP_HDR_SIZE + 4u * frame, &off, 4) != 4) {
        return IMG_ERR_SRC;
    }

    rd.img = img;
    rd.pos = off;

    total = (uint32_t)img->hdr.width * img->hdr.height;

    switch (img->hdr.format) {
    case WSP_FMT_RAW565:
        while (n < total) {
            img_scratch[n++] = rd_u16(&rd);
            if (rd.eof) {
                return IMG_ERR_SRC;
            }
        }
        break;

    case WSP_FMT_RAW332:
        while (n < total) {
            v = rd_u8(&rd);
            if (rd.eof) {
                return IMG_ERR_SRC;
            }
            r = (uint8_t)((v >> 5) & 0x07);
            g = (uint8_t)((v >> 2) & 0x07);
            b = (uint8_t)(v & 0x03);
            r = (uint8_t)((r << 5) | (r << 2) | (r >> 1));
            g = (uint8_t)((g << 5) | (g << 2) | (g >> 1));
            b = (uint8_t)((b << 6) | (b << 4) | (b << 2) | b);
            img_scratch[n++] = RGB565(r, g, b);
        }
        break;

    case WSP_FMT_RLE565:
    default:
        while (n < total) {
            count = rd_u8(&rd);
            color = rd_u16(&rd);
            if (rd.eof) {
                return IMG_ERR_SRC;
            }
            while ((count-- != 0) && (n < total)) {
                img_scratch[n++] = color;
            }
        }
        break;
    }

    return IMG_OK;
}

/*********************************************************************
 * @fn      img_show
 *
 * @brief   Decode one frame, scale it to the panel and refresh the strip.
 *
 * @return  IMG_OK or a negative error code.
 */
int img_show(img_t *img, uint16_t frame)
{
    int ret;

    ret = img_decode(img, frame);
    if (ret != IMG_OK) {
        return ret;
    }

    gfx_blit565_fit(img_scratch, img->hdr.width, img->hdr.height);
    panel_show();

    img->cur = frame;

    return IMG_OK;
}

/*********************************************************************
 * @fn      img_play
 *
 * @brief   Play the animation.
 *
 * @param   img   - player handle.
 *          loops - number of loops, 0 = endless.
 *
 * @return  none
 */
void img_play(img_t *img, uint16_t loops)
{
    uint16_t f;
    uint16_t done = 0;

    for (;;) {
        for (f = 0; f < img->hdr.frames; f++) {
            if (img_show(img, f) != IMG_OK) {
                return;
            }
            Delay_Ms(img->hdr.frame_ms);
        }

        if (loops != 0) {
            done++;
            if (done >= loops) {
                break;
            }
        }
    }
}
