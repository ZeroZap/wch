#!/usr/bin/env python3
"""Generate the .wsp fixtures used by the host side decoder test.

The files are written next to this script's build directory
(tests/build/fixtures) so the test binary can open them by plain name.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.join(HERE, "..", "tools")
OUT = os.path.join(HERE, "build", "fixtures")

sys.path.insert(0, os.path.abspath(TOOLS))

import wsp_pack as W  # noqa: E402


def write(name, data):
    path = os.path.join(OUT, name)
    with open(path, "wb") as f:
        f.write(data)
    print("%s: %d bytes" % (path, len(data)))


def main():
    os.makedirs(OUT, exist_ok=True)

    # 1) demo still picture, 16x16, RLE
    write("still.wsp", W.build_wsp([W.demo_still(16, 16)], 16, 16, W.FMT_RLE565, 0))

    # 2) same picture, raw 565, must decode identically
    write("still_raw.wsp", W.build_wsp([W.demo_still(16, 16)], 16, 16, W.FMT_RAW565, 0))

    # 3) same picture, RGB332 (lossy)
    write("still_332.wsp", W.build_wsp([W.demo_still(16, 16)], 16, 16, W.FMT_RAW332, 0))

    # 4) animation
    write("anim.wsp", W.build_wsp(W.demo_anim(16, 16, 8), 16, 16, W.FMT_RLE565, 60))

    # 5) 4x4 picture with 16 distinct colours, for the scaling check
    colours = [
        W.rgb_to_565(255, 0, 0),
        W.rgb_to_565(0, 255, 0),
        W.rgb_to_565(0, 0, 255),
        W.rgb_to_565(255, 255, 0),
        W.rgb_to_565(255, 255, 255),
        W.rgb_to_565(0, 0, 0),
        W.rgb_to_565(0, 255, 255),
        W.rgb_to_565(255, 0, 255),
        W.rgb_to_565(128, 128, 128),
        W.rgb_to_565(64, 64, 64),
        W.rgb_to_565(200, 100, 50),
        W.rgb_to_565(10, 200, 30),
        W.rgb_to_565(1, 2, 3),
        W.rgb_to_565(250, 250, 250),
        W.rgb_to_565(30, 60, 90),
        W.rgb_to_565(120, 0, 240),
    ]
    write("small.wsp", W.build_wsp([colours], 4, 4, W.FMT_RAW565, 0))

    # 6) broken files, must be rejected or fail cleanly
    bad = bytearray(W.build_wsp([W.demo_still(16, 16)], 16, 16, W.FMT_RLE565, 0))
    write("bad_magic.wsp", b"XXXX" + bytes(bad[4:]))
    write("truncated.wsp", bytes(bad[: len(bad) // 2]))

    return 0


if __name__ == "__main__":
    sys.exit(main())
