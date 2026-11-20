/********************************** (C) COPYRIGHT *******************************
 * File Name          : wsp_img.h
 * Description        : "WSP1" image / animation container and player.
 *
 * File layout (little endian, see tools/wsp_pack.py)
 * -------------------------------------------------
 *   offset 0   : wsp_hdr_t (16 bytes)
 *   offset 16  : uint32_t frame_offset[frames]   (absolute, from file start)
 *   ...        : frame payloads
 *
 * A frame is stored as
 *   WSP_FMT_RLE565 : stream of (count, colour) pairs
 *                    count  : 1..255 pixels, uint8_t
 *                    colour : RGB565, uint16_t
 *                    A run never crosses a frame boundary; the decoder
 *                    stops after width*height pixels.
 *   WSP_FMT_RAW565 : width * height uint16_t colours
 *   WSP_FMT_RAW332 : width * height RGB332 bytes
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __WSP_IMG_H
#define __WSP_IMG_H

#include "ch32x035.h"
#include "wsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WSP_MAGIC               0x31505357UL    /* "WSP1" */

#define WSP_FMT_RLE565          0
#define WSP_FMT_RAW565          1
#define WSP_FMT_RAW332          2

#define WSP_HDR_SIZE            16u

typedef struct
{
    uint32_t magic;         /* WSP_MAGIC */
    uint8_t  version;
    uint8_t  format;
    uint16_t width;
    uint16_t height;
    uint16_t frames;
    uint16_t frame_ms;
    uint16_t flags;
} wsp_hdr_t;

/* Data source: internal flash (memory mapped) or external SPI NOR */
typedef struct
{
    uint32_t (*read)(void *ctx, uint32_t off, void *buf, uint32_t len);
    void    *ctx;
    uint32_t size;
} img_src_t;

typedef struct
{
    img_src_t src;
    wsp_hdr_t hdr;
    uint16_t  cur;
} img_t;

/* return codes */
#define IMG_OK                  0
#define IMG_ERR_ARG            -1
#define IMG_ERR_MAGIC          -2
#define IMG_ERR_FORMAT         -3
#define IMG_ERR_TOO_BIG        -4
#define IMG_ERR_SRC            -5

int  img_open(img_t *img, const img_src_t *src);
int  img_show(img_t *img, uint16_t frame);

/* decode and display the animation, loops = 0 means "forever" */
void img_play(img_t *img, uint16_t loops);

/* memory mapped source helper (internal flash / const arrays) */
void img_src_mem(img_src_t *src, const void *base, uint32_t size);
uint32_t img_src_mem_read(void *ctx, uint32_t off, void *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __WSP_IMG_H */
