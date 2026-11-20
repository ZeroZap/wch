#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""wsp_pack.py - pack images / GIFs into the "WSP1" container used by
CH32X035EVT/EXAM/APPLICATION/WS2812_Panel.

File format (little endian)
    wsp_hdr_t (16 bytes)
        uint32 magic      'W''S''P''1'  -> 0x31505357
        uint8  version    1
        uint8  format     0 = RLE565, 1 = RAW565, 2 = RAW332
        uint16 width
        uint16 height
        uint16 frames
        uint16 frame_ms
        uint16 flags      reserved, 0
    uint32 frame_offset[frames]   absolute offsets from the start of the file
    frame payloads

Frame payload
    RLE565 : repeated (uint8 count, uint16 colour) pairs, count in 1..255
    RAW565 : width*height uint16
    RAW332 : width*height uint8

Examples
    # 16x16 panel, RGB565 + RLE, one PNG
    python wsp_pack.py --input logo.png --size 16x16 --format rle565 \
        --output logo.wsp

    # animated GIF -> animation with 80ms per frame, plus a C array
    python wsp_pack.py --input anim.gif --size 16x16 --frame-ms 80 \
        --output anim.wsp --c-array anim

    # print the header of an existing file
    python wsp_pack.py --info logo.wsp

    # regenerate User/img_demo.c (no Pillow required)
    python wsp_pack.py --demo ../User/img_demo.c
"""

import argparse
import os
import struct
import sys

MAGIC = 0x31505357  # 'W''S''P''1'

FMT_RLE565 = 0
FMT_RAW565 = 1
FMT_RAW332 = 2

HDR = struct.Struct("<IBBHHHHH")  # 16 bytes
assert HDR.size == 16

FMT_NAMES = {
    "rle565": FMT_RLE565,
    "raw565": FMT_RAW565,
    "raw332": FMT_RAW332,
}


# --------------------------------------------------------------------------
# colour helpers
# --------------------------------------------------------------------------
def rgb_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rgb_to_332(r, g, b):
    return (r & 0xE0) | ((g & 0xE0) >> 3) | (b >> 6)


# --------------------------------------------------------------------------
# frame encoders (pixels = list of 565 ints, row major)
# --------------------------------------------------------------------------
def encode_raw565(pixels):
    return b"".join(struct.pack("<H", p) for p in pixels)


def encode_rle565(pixels):
    out = bytearray()
    i = 0
    n = len(pixels)
    while i < n:
        colour = pixels[i]
        run = 1
        while (i + run < n) and (pixels[i + run] == colour) and (run < 255):
            run += 1
        out += struct.pack("<BH", run, colour)
        i += run
    return bytes(out)


def encode_raw332(pixels):
    out = bytearray()
    for p in pixels:
        r = (p >> 11) & 0x1F
        g = (p >> 5) & 0x3F
        b = p & 0x1F
        # expand to 8 bit, then keep the 3/3/2 MSBs
        r8 = (r << 3) | (r >> 2)
        g8 = (g << 2) | (g >> 4)
        b8 = (b << 3) | (b >> 2)
        out.append(rgb_to_332(r8, g8, b8))
    return bytes(out)


ENCODERS = {
    FMT_RLE565: encode_rle565,
    FMT_RAW565: encode_raw565,
    FMT_RAW332: encode_raw332,
}


# --------------------------------------------------------------------------
# container
# --------------------------------------------------------------------------
def build_wsp(frames, width, height, fmt, frame_ms):
    """frames: list of pixel lists (already resized to width*height)."""
    payloads = [ENCODERS[fmt](f) for f in frames]
    offsets = []
    off = HDR.size + 4 * len(payloads)
    for p in payloads:
        offsets.append(off)
        off += len(p)

    out = bytearray()
    out += HDR.pack(MAGIC, 1, fmt, width, height, len(payloads), frame_ms, 0)
    for o in offsets:
        out += struct.pack("<I", o)
    for p in payloads:
        out += p
    return bytes(out)


def read_wsp(path):
    with open(path, "rb") as f:
        data = f.read()
    magic, ver, fmt, w, h, frames, ms, flags = HDR.unpack_from(data, 0)
    if magic != MAGIC:
        raise ValueError("not a WSP1 file")
    offsets = list(struct.unpack_from("<%dI" % frames, data, HDR.size))
    info = {
        "version": ver,
        "format": fmt,
        "width": w,
        "height": h,
        "frames": frames,
        "frame_ms": ms,
        "flags": flags,
    }
    return data, info, offsets


# --------------------------------------------------------------------------
# Pillow based loading
# --------------------------------------------------------------------------
def load_frames(path, size):
    from PIL import Image, ImageSequence  # imported lazily

    im = Image.open(path)
    frames = []
    durations = []

    for frame in ImageSequence.Iterator(im):
        rgb = frame.convert("RGB")
        if size is not None and rgb.size != size:
            rgb = rgb.resize(size, Image.NEAREST)
        px = list(rgb.getdata())
        frames.append([rgb_to_565(r, g, b) for (r, g, b) in px])
        durations.append(frame.info.get("duration", 0))

    return frames, im.size, durations


# --------------------------------------------------------------------------
# demo assets (pure python, no Pillow)
# --------------------------------------------------------------------------
def demo_still(w=16, h=16):
    """Yellow smiley on a dark blue background (RLE friendly)."""
    bg = rgb_to_565(10, 20, 60)
    face = rgb_to_565(255, 200, 0)
    ink = rgb_to_565(0, 0, 0)
    px = [bg] * (w * h)

    cx, cy, r = 7.5, 7.5, 6.0
    for y in range(h):
        for x in range(w):
            dx = x - cx
            dy = y - cy
            if dx * dx + dy * dy <= r * r:
                px[y * w + x] = face

    # eyes
    for ex, ey in ((5, 5), (10, 5)):
        px[ey * w + ex] = ink
        px[ey * w + ex + 1] = ink
        px[(ey + 1) * w + ex] = ink
        px[(ey + 1) * w + ex + 1] = ink

    # smile
    for x, y in (
        (4, 9),
        (5, 10),
        (6, 11),
        (7, 11),
        (8, 11),
        (9, 11),
        (10, 10),
        (11, 9),
    ):
        px[y * w + x] = ink

    return px


def demo_anim(w=16, h=16, frames=8):
    """A bright column sweeping over a colour that changes per frame.

    Flat colours keep the RLE stream small, which matters because this
    animation is compiled into the 62K internal flash.
    """
    colours = [
        rgb_to_565(40, 0, 0),
        rgb_to_565(40, 20, 0),
        rgb_to_565(40, 40, 0),
        rgb_to_565(0, 40, 0),
        rgb_to_565(0, 40, 40),
        rgb_to_565(0, 0, 40),
        rgb_to_565(20, 0, 40),
        rgb_to_565(40, 0, 20),
    ]
    out = []
    for f in range(frames):
        bg = colours[f % len(colours)]
        col = (f * 2) % w
        px = []
        for y in range(h):
            for x in range(w):
                if x == col:
                    px.append(rgb_to_565(255, 255, 255))
                elif x == col - 1 or x == col + 1:
                    px.append(rgb_to_565(120, 120, 120))
                else:
                    px.append(bg)
        out.append(px)
    return out


def demo_small(w=8, h=8):
    """8x8 checker board, used to verify scaling to the panel."""
    a = rgb_to_565(255, 0, 0)
    b = rgb_to_565(0, 0, 255)
    return [a if ((x + y) & 1) else b for y in range(h) for x in range(w)]


def emit_c_array(name, data, comment):
    lines = []
    lines.append("/* %s */" % comment)
    lines.append("const uint8_t %s[] = {" % name)
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append("    " + ", ".join("0x%02X" % b for b in chunk) + ",")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def write_demo(path, panel_w=16, panel_h=16):
    still = build_wsp([demo_still(panel_w, panel_h)], panel_w, panel_h, FMT_RLE565, 0)
    anim = build_wsp(demo_anim(panel_w, panel_h), panel_w, panel_h, FMT_RLE565, 60)
    small = build_wsp([demo_small(8, 8)], 8, 8, FMT_RAW565, 0)

    body = []
    body.append(
        "/********************************** (C) COPYRIGHT *******************************"
    )
    body.append(" * File Name          : img_demo.c")
    body.append(" * Description        : Built-in WSP1 demo assets.")
    body.append(" *")
    body.append(" * GENERATED BY tools/wsp_pack.py --demo, DO NOT EDIT BY HAND.")
    body.append(
        " *********************************************************************************"
    )
    body.append(" * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.")
    body.append(" * Attention: This software (modified or not) and binary are used for")
    body.append(" * microcontroller manufactured by Nanjing Qinheng Microelectronics.")
    body.append(
        " *******************************************************************************/"
    )
    body.append('#include "img_demo.h"')
    body.append("")
    body.append(
        emit_c_array(
            "img_demo_still_data",
            still,
            "%dx%d static picture (RLE565)" % (panel_w, panel_h),
        )
    )
    body.append(
        emit_c_array(
            "img_demo_anim_data",
            anim,
            "%dx%d animation, 8 frames @60ms (RLE565)" % (panel_w, panel_h),
        )
    )
    body.append(
        emit_c_array(
            "img_demo_small_data", small, "8x8 picture (RAW565), scaled up to the panel"
        )
    )
    body.append("const uint32_t img_demo_still_size = sizeof(img_demo_still_data);")
    body.append("const uint32_t img_demo_anim_size = sizeof(img_demo_anim_data);")
    body.append("const uint32_t img_demo_small_size = sizeof(img_demo_small_data);")
    body.append("")

    with open(path, "w", newline="\n") as f:
        f.write("\n".join(body))

    print("wrote %s" % path)


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------
def parse_size(text):
    parts = text.lower().split("x")
    if len(parts) != 2:
        raise argparse.ArgumentTypeError("expected WxH, e.g. 16x16")
    return int(parts[0]), int(parts[1])


def main(argv=None):
    ap = argparse.ArgumentParser(description="WSP1 image packer")
    ap.add_argument("--input", help="PNG / BMP / GIF / JPEG input file")
    ap.add_argument("--output", help="output .wsp file")
    ap.add_argument(
        "--size",
        type=parse_size,
        default=None,
        help="resize frames, e.g. 16x16 (nearest neighbour)",
    )
    ap.add_argument("--format", choices=sorted(FMT_NAMES), default="rle565")
    ap.add_argument(
        "--frame-ms",
        type=int,
        default=0,
        help="frame duration; 0 = use the GIF duration",
    )
    ap.add_argument(
        "--c-array",
        metavar="NAME",
        default=None,
        help="also write NAME.c / NAME.h with the file as array",
    )
    ap.add_argument(
        "--info",
        metavar="WSP",
        default=None,
        help="print the header of an existing file",
    )
    ap.add_argument(
        "--demo", metavar="PATH", default=None, help="generate User/img_demo.c"
    )
    args = ap.parse_args(argv)

    if args.demo:
        write_demo(args.demo)
        return 0

    if args.info:
        data, info, offsets = read_wsp(args.info)
        print("file      : %s (%d bytes)" % (args.info, len(data)))
        print("version   : %d" % info["version"])
        print("format    : %s" % {v: k for k, v in FMT_NAMES.items()}[info["format"]])
        print("size      : %dx%d" % (info["width"], info["height"]))
        print("frames    : %d" % info["frames"])
        print("frame_ms  : %d" % info["frame_ms"])
        for i, off in enumerate(offsets):
            end = offsets[i + 1] if i + 1 < len(offsets) else len(data)
            print("  frame %2d: offset 0x%06X, %d bytes" % (i, off, end - off))
        return 0

    if not args.input or not args.output:
        ap.error("--input and --output are required (or use --info / --demo)")

    frames, src_size, durations = load_frames(args.input, args.size)

    frame_ms = args.frame_ms
    if frame_ms == 0:
        if len(frames) > 1:
            valid = [d for d in durations if d > 0]
            frame_ms = int(sum(valid) / len(valid)) if valid else 100
        else:
            frame_ms = 0

    width, height = args.size if args.size else src_size
    data = build_wsp(frames, width, height, FMT_NAMES[args.format], frame_ms)

    with open(args.output, "wb") as f:
        f.write(data)

    print(
        "%s -> %s : %dx%d, %d frame(s), %d byte(s), %s"
        % (args.input, args.output, width, height, len(frames), len(data), args.format)
    )

    if args.c_array:
        base = os.path.splitext(args.output)[0]
        c_path = base + ".c"
        h_path = base + ".h"
        guard = (
            os.path.basename(base).upper().replace("-", "_").replace(".", "_") + "_H"
        )
        with open(c_path, "w", newline="\n") as f:
            f.write('#include "%s"\n\n' % os.path.basename(h_path))
            f.write(emit_c_array(args.c_array, data, args.output))
        with open(h_path, "w", newline="\n") as f:
            f.write(
                "#ifndef %s\n#define %s\n\n#include <stdint.h>\n\n" % (guard, guard)
            )
            f.write("extern const uint8_t %s[];\n" % args.c_array)
            f.write("extern const uint32_t %s_size;\n\n#endif\n" % args.c_array)
        print("wrote %s and %s" % (c_path, h_path))

    return 0


if __name__ == "__main__":
    sys.exit(main())
