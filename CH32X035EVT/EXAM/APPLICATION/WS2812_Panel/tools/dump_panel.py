#!/usr/bin/env python3
"""Read the panel framebuffer out of a running board and save it as a PNG.

The panel has no serial console half the time, and "look at the LEDs" is not a
unit test -- this reads panel_fb over WCH-Link instead, so a rendering bug can
be looked at on the PC.

    python dump_panel.py                                  # build/WS2812_Panel.elf, 16x16
    python dump_panel.py --size 16x32 --out panel.png
    python dump_panel.py --addr 0x20000944                # skip the ELF lookup
    python dump_panel.py --scale 20

Requires: Pillow, plus the WCH OpenOCD that ships with MounRiver Studio
(or point $WCH_OPENOCD at its install directory).
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

from PIL import Image

MRS_OPENOCD = [
    "D:/MounRiver/MounRiver_Studio3/resources/app/resources/win32/"
    "components/WCH/OpenOCD/OpenOCD",
    "C:/MounRiver/MounRiver_Studio3/resources/app/resources/win32/"
    "components/WCH/OpenOCD/OpenOCD",
    "D:/MounRiver/MounRiver_Studio2/resources/app/resources/win32/"
    "components/WCH/OpenOCD/OpenOCD",
]

MRS_TOOLCHAIN = [
    "D:/MounRiver/MounRiver_Studio3/resources/app/resources/win32/"
    "components/WCH/Toolchain/RISC-V Embedded GCC/bin",
    "C:/MounRiver/MounRiver_Studio3/resources/app/resources/win32/"
    "components/WCH/Toolchain/RISC-V Embedded GCC/bin",
]

SYMBOL = "panel_fb"


def find_openocd():
    env = os.environ.get("WCH_OPENOCD")
    if env and os.path.isfile(os.path.join(env, "bin", "openocd.exe")):
        return env
    for path in MRS_OPENOCD:
        if os.path.isfile(os.path.join(path, "bin", "openocd.exe")):
            return path
    sys.exit("openocd.exe not found; set WCH_OPENOCD to the WCH/OpenOCD dir")


def find_nm():
    for name in ("riscv-none-embed-nm", "riscv-none-embed-nm.exe"):
        if shutil.which(name):
            return shutil.which(name)
    env = os.environ.get("RISCV_GCC_BIN")
    candidates = ([env] if env else []) + MRS_TOOLCHAIN
    for path in candidates:
        for name in ("riscv-none-embed-nm.exe", "riscv-none-embed-nm"):
            full = os.path.join(path, name)
            if os.path.isfile(full):
                return full
    return None


def symbol_addr(elf, symbol):
    nm = find_nm()
    if not nm or not os.path.isfile(elf):
        return None
    out = subprocess.run([nm, "-n", elf], capture_output=True, text=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == symbol:
            return int(parts[0], 16)
    return None


def dump(addr, count):
    ocd = find_openocd()
    res = subprocess.run(
        [
            os.path.join(ocd, "bin", "openocd.exe"),
            "-f",
            os.path.join(ocd, "bin", "wch-riscv.cfg"),
            "-s",
            os.path.join(ocd, "scripts"),
            "-c",
            "init",
            "-c",
            "halt",
            "-c",
            "mdb 0x%X %u" % (addr, count),
            "-c",
            "shutdown",
        ],
        capture_output=True,
        text=True,
    )
    data = bytearray()
    for line in (res.stdout + res.stderr).splitlines():
        m = re.match(r"^0x[0-9a-fA-F]+:\s+(.*)$", line.strip())
        if m:
            data += bytes(int(b, 16) for b in m.group(1).split())
    return data


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    default_elf = os.path.normpath(
        os.path.join(here, "..", "build", "WS2812_Panel.elf")
    )

    ap = argparse.ArgumentParser(description="dump panel_fb to a PNG")
    ap.add_argument("--elf", default=default_elf, help="ELF used to look up panel_fb")
    ap.add_argument(
        "--addr", default=None, help="panel_fb address, skips the ELF lookup"
    )
    ap.add_argument("--size", default="16x16", help="panel geometry, default 16x16")
    ap.add_argument("--out", default="panel_dump.png")
    ap.add_argument("--scale", type=int, default=16)
    args = ap.parse_args()

    w, h = (int(v) for v in args.size.lower().split("x"))

    if args.addr:
        addr = int(args.addr, 0)
    else:
        addr = symbol_addr(args.elf, SYMBOL)
        if addr is None:
            sys.exit("cannot find %s in %s; pass --addr" % (SYMBOL, args.elf))

    data = dump(addr, w * h * 3)
    if len(data) < w * h * 3:
        sys.exit("short read: got %u of %u bytes" % (len(data), w * h * 3))

    im = Image.new("RGB", (w, h))
    px = im.load()
    for i in range(w * h):
        px[i % w, i // w] = (data[i * 3], data[i * 3 + 1], data[i * 3 + 2])

    im.resize((w * args.scale, h * args.scale), Image.NEAREST).save(args.out)
    print("%s @ 0x%X, %ux%u -> %s" % (SYMBOL, addr, w, h, args.out))


if __name__ == "__main__":
    main()
