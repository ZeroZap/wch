#!/usr/bin/env python3
"""Host tool for the WS2812_Panel USB CDC interface.

Packet format (see User/wsp_usb.h):

    A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8

CRC8 is poly 0x07 / init 0x00 over CMD..PAYLOAD.  Replies use CMD | 0x80 and
are at most 64 bytes, i.e. one USB packet.  Display frames are not acknowledged
unless they are rejected (CMD | 0xC0).

Examples
    python wsp_cdc.py --port COM5 info
    python wsp_cdc.py --port COM5 bars
    python wsp_cdc.py --port COM5 solid 255 0 0
    python wsp_cdc.py --port COM5 bright 64
    python wsp_cdc.py --port COM5 image logo.png            # needs pillow
    python wsp_cdc.py --port COM5 anim anim.gif --fps 20    # needs pillow
    python wsp_cdc.py --port COM5 nor-upload anim.wsp
    python wsp_cdc.py --port COM5 nor-read 0 16
"""

import argparse
import struct
import sys
import time

try:
    import serial
except ImportError:  # pragma: no cover
    sys.exit("需要 pyserial: pip install pyserial")

SYNC = b"\xa5\x5a"

CMD_SET_BRIGHT = 0x01
CMD_FRAME_565 = 0x02
CMD_FRAME_888 = 0x03
CMD_RECT_565 = 0x04
CMD_NOR_ERASE = 0x05
CMD_NOR_WRITE = 0x06
CMD_NOR_READ = 0x07
CMD_INFO = 0x0A
CMD_SET_ORDER = 0x0B
CMD_CFG_GET = 0x0C
CMD_CFG_SET = 0x0D
CMD_SET_CYC = 0x0E
CMD_OSD = 0x0F

ORDER_NAMES = ["RGB", "GRB", "BRG", "BGR", "RBG", "GBR"]
MODE_NAMES = ["panel", "strip"]

# settings record stored in the NOR: magic(4) cyc(1) ver(1) order(1) bright(1)
# mode(1) crc(1) pad(6).  The panel recomputes magic/crc, the host only patches
# the value fields.
CFG_FMT = "<IBBBBBB"

ACK = 0x80
NAK = 0x40


# --------------------------------------------------------------------------
def crc8(data):
    crc = 0
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


def packet(cmd, payload=b""):
    body = bytes([cmd]) + struct.pack("<H", len(payload)) + payload
    return SYNC + body + bytes([crc8(body)])


def open_port(name, timeout=1.0):
    ser = serial.Serial(name, 115200, timeout=timeout, write_timeout=5)
    ser.reset_input_buffer()
    return ser


def read_reply(ser, want_cmd, timeout=1.0):
    """Return the payload of the reply to want_cmd, or None."""
    deadline = time.time() + timeout
    buf = bytearray()
    while time.time() < deadline:
        chunk = ser.read(64)
        if not chunk:
            continue
        buf += chunk
        while True:
            start = buf.find(SYNC)
            if start < 0:
                buf.clear()
                break
            del buf[:start]
            if len(buf) < 5:
                break
            cmd = buf[2]
            length = buf[3] | (buf[4] << 8)
            if len(buf) < 6 + length:
                break
            body = bytes(buf[2 : 5 + length])
            crc = buf[5 + length]
            del buf[: 6 + length]
            if crc8(body) != crc:
                continue
            base = cmd & ~(ACK | NAK)
            if base == want_cmd:
                if cmd & NAK:
                    return False
                return body[3:]
    return None


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def frame_565_bytes(pixels):
    return b"".join(struct.pack(">H", p) for p in pixels)


# --------------------------------------------------------------------------
def get_info(ser):
    ser.write(packet(CMD_INFO))
    data = read_reply(ser, CMD_INFO)
    if not data or len(data) < 9:
        return None
    return {
        "ver": data[0],
        "w": data[1],
        "h": data[2],
        "order": data[3],
        "cyc": data[4],
        "nor": struct.unpack("<I", data[5:9])[0],
    }


def order_name(value):
    return ORDER_NAMES[value] if 0 <= value < len(ORDER_NAMES) else str(value)


def cmd_info(ser):
    info = get_info(ser)
    if not info:
        print("no reply")
        return
    print("protocol  : %u" % info["ver"])
    print("panel     : %ux%u (%u px)" % (info["w"], info["h"], info["w"] * info["h"]))
    print("byte order: %s" % order_name(info["order"]))
    print("1-wire    : %u cyc/bit (~%u ns)" % (info["cyc"], info["cyc"] * 1000 // 48))
    print("nor       : %u bytes (%u KB)" % (info["nor"], info["nor"] // 1024))


def cmd_order(ser, value=None):
    """Query or change the strip byte order (RAM only, resets on power cycle)."""
    if value is None:
        info = get_info(ser)
        if not info:
            print("no reply")
            return
        print("byte order: %s (%u)" % (order_name(info["order"]), info["order"]))
        print(
            "options   : "
            + "  ".join("%u=%s" % (i, n) for i, n in enumerate(ORDER_NAMES))
        )
        return

    ser.write(packet(CMD_SET_ORDER, bytes([value & 0xFF])))
    reply = read_reply(ser, CMD_SET_ORDER, 0.5)
    if reply is None or reply is False:
        sys.exit("set order failed (valid range 0..5)")
    info = get_info(ser)
    if info:
        print("byte order -> %s (%u)" % (order_name(info["order"]), info["order"]))


def cmd_solid(ser, r, g, b, times=10):
    w, h = panel_size()
    px = [rgb565(r, g, b)] * (w * h)
    data = packet(CMD_FRAME_565, frame_565_bytes(px))
    for _ in range(times):
        ser.write(data)
        time.sleep(0.05)


def cmd_bars(ser, hold=2.0):
    """8 vertical colour bars, the same pattern the firmware self-test uses."""
    w, h = panel_size()
    bars = [
        (255, 255, 255),
        (255, 255, 0),
        (0, 255, 255),
        (0, 255, 0),
        (255, 0, 255),
        (255, 0, 0),
        (0, 0, 255),
        (0, 0, 0),
    ]
    px = []
    for _y in range(h):
        for x in range(w):
            r, g, b = bars[x * 8 // w]
            px.append(rgb565(r, g, b))
    ser.write(packet(CMD_FRAME_565, frame_565_bytes(px)))
    print("bars sent, holding %.1fs" % hold)
    time.sleep(hold)


def panel_size():
    return args.size


def send_frame_565(ser, px):
    ser.write(packet(CMD_FRAME_565, frame_565_bytes(px)))


def send_frame_888(ser, data):
    ser.write(packet(CMD_FRAME_888, data))


def load_frames(path, size, resample):
    """Return a list of RGB888 byte strings, one per frame."""
    from PIL import Image, ImageSequence

    w, h = size
    out = []
    with Image.open(path) as im:
        for frame in ImageSequence.Iterator(im):
            rgb = frame.convert("RGB").resize((w, h), resample)
            out.append(rgb.tobytes())
    return out


def cmd_image(ser, path, fps, loops):
    from PIL import Image

    frames = load_frames(path, args.size, Image.NEAREST)
    if not frames:
        sys.exit("no frames decoded")
    delay = 1.0 / fps
    print(
        "%u frame(s), %dx%d, %.1f fps" % (len(frames), args.size[0], args.size[1], fps)
    )
    for _ in range(loops):
        for data in frames:
            t0 = time.time()
            send_frame_888(ser, data)
            rest = delay - (time.time() - t0)
            if rest > 0:
                time.sleep(rest)


def cmd_bright(ser, value):
    ser.write(packet(CMD_SET_BRIGHT, bytes([value & 0xFF])))
    read_reply(ser, CMD_SET_BRIGHT, 0.3)
    print("brightness -> %u" % (value & 0xFF))


def cmd_nor_upload(ser, path, base):
    data = open(path, "rb").read()
    if base % 4096:
        sys.exit("base address must be 4KB aligned")
    print("uploading %u bytes to NOR 0x%X" % (len(data), base))

    # erase every sector we are going to touch; an empty ACK means success,
    # None means the panel did not answer at all
    addr = base
    while addr < base + len(data):
        ser.write(packet(CMD_NOR_ERASE, struct.pack("<I", addr)))
        if read_reply(ser, CMD_NOR_ERASE, 2.0) is None:
            print("  erase 0x%X: no ack" % addr)
        addr += 4096

    off = 0
    while off < len(data):
        chunk = data[off : off + 256]
        ser.write(packet(CMD_NOR_WRITE, struct.pack("<I", base + off) + chunk))
        reply = read_reply(ser, CMD_NOR_WRITE, 1.0)
        if reply is None or reply is False:
            sys.exit("write failed at 0x%X" % (base + off))
        off += len(chunk)
        if off % 4096 == 0:
            print("  %u / %u" % (off, len(data)))
    print("done")


def cmd_nor_read(ser, addr, length):
    got = bytearray()
    while len(got) < length:
        n = min(40, length - len(got))
        ser.write(packet(CMD_NOR_READ, struct.pack("<IH", addr + len(got), n)))
        data = read_reply(ser, CMD_NOR_READ, 1.0)
        if not data:
            sys.exit("no reply at 0x%X" % (addr + len(got)))
        got += data
    for i in range(0, len(got), 16):
        row = got[i : i + 16]
        print(
            "%08X  %s  |%s|"
            % (
                addr + i,
                " ".join("%02X" % c for c in row),
                "".join(chr(c) if 32 <= c < 127 else "." for c in row),
            )
        )


def cmd_nor_dump(ser, addr, length, path):
    got = bytearray()
    while len(got) < length:
        n = min(40, length - len(got))
        ser.write(packet(CMD_NOR_READ, struct.pack("<IH", addr + len(got), n)))
        data = read_reply(ser, CMD_NOR_READ, 1.0)
        if not data:
            sys.exit("no reply at 0x%X" % (addr + len(got)))
        got += data
        if len(got) % 512 < 40:
            print("  %u / %u" % (len(got), length))
    with open(path, "wb") as f:
        f.write(got)
    print("wrote %u bytes to %s" % (len(got), path))


def cmd_rate(ser, cycles=None):
    """Query or temporarily change the 1-wire bit rate (RAM only)."""
    if cycles is None:
        info = get_info(ser)
        if info:
            print(
                "1-wire: %u cycles/bit (~%u ns)"
                % (info["cyc"], info["cyc"] * 1000 // 48)
            )
        return

    ser.write(packet(CMD_SET_CYC, bytes([cycles & 0xFF])))
    reply = read_reply(ser, CMD_SET_CYC, 0.5)
    if reply is None or reply is False:
        sys.exit("set cycles failed")
    used = reply[0] if reply else cycles
    print(
        "1-wire -> %u cycles/bit (~%u ns)  [RAM only, use 'config --rate' to persist]"
        % (used, used * 1000 // 48)
    )


def read_cfg(ser):
    ser.write(packet(CMD_CFG_GET))
    data = read_reply(ser, CMD_CFG_GET)
    if not data or len(data) < 16:
        return None
    magic, cyc, ver, order, bright, mode, limit100 = struct.unpack(CFG_FMT, data[:10])
    return {
        "magic": magic,
        "cyc": cyc,
        "ver": ver,
        "order": order,
        "bright": bright,
        "mode": mode,
        "limit100": limit100,
        "raw": bytearray(data[:16]),
    }


def cmd_config(ser, rate=None, order=None, bright=None, mode=None, limit=None):
    cfg = read_cfg(ser)
    if cfg is None:
        sys.exit("no reply to CFG_GET")

    if (
        rate is None
        and order is None
        and bright is None
        and mode is None
        and limit is None
    ):
        print('magic   : 0x%08X  (0x47464357 = "WCFG")' % cfg["magic"])
        print("version : %u" % cfg["ver"])
        print("1-wire  : %u cyc/bit (~%u ns)" % (cfg["cyc"], cfg["cyc"] * 1000 // 48))
        print("order   : %s" % order_name(cfg["order"]))
        print("bright  : %u" % cfg["bright"])
        print(
            "mode    : %s"
            % (MODE_NAMES[cfg["mode"]] if cfg["mode"] < 2 else cfg["mode"])
        )
        print(
            "limit   : %s"
            % (("%u mA" % (cfg["limit100"] * 100)) if cfg["limit100"] else "unlimited")
        )
        return

    if rate is not None:
        cfg["cyc"] = rate
    if order is not None:
        cfg["order"] = order
    if bright is not None:
        cfg["bright"] = bright
    if mode is not None:
        cfg["mode"] = 1 if mode.lower().startswith("s") else 0
    if limit is not None:
        cfg["limit100"] = max(0, min(255, limit // 100))

    rec = bytearray(cfg["raw"])
    struct.pack_into(
        CFG_FMT,
        rec,
        0,
        cfg["magic"],
        cfg["cyc"],
        cfg["ver"],
        cfg["order"],
        cfg["bright"],
        cfg["mode"],
        cfg["limit100"],
    )

    ser.write(packet(CMD_CFG_SET, bytes(rec)))
    if read_reply(ser, CMD_CFG_SET, 3.0) is None:
        sys.exit("config write failed")

    cfg = read_cfg(ser)
    print(
        "stored  : 1-wire %u cyc, order %s, bright %u, mode %s, limit %s"
        % (
            cfg["cyc"],
            order_name(cfg["order"]),
            cfg["bright"],
            MODE_NAMES[cfg["mode"]] if cfg["mode"] < 2 else cfg["mode"],
            ("%u mA" % (cfg["limit100"] * 100)) if cfg["limit100"] else "unlimited",
        )
    )


def cmd_osd(ser, state=None):
    """Query or toggle the frame rate / current limit badge."""
    payload = b"" if state is None else bytes([1 if state else 0])
    ser.write(packet(CMD_OSD, payload))
    reply = read_reply(ser, CMD_OSD, 1.0)
    if reply is None or reply is False:
        sys.exit("osd command failed (built with WSP_OSD_ENABLE=0?)")
    print("osd: %s" % ("on" if reply[0] else "off"))


# --------------------------------------------------------------------------
def main():
    global args

    ap = argparse.ArgumentParser(description="WS2812_Panel USB CDC tool")
    ap.add_argument("--port", default=None, help="serial port, e.g. COM5")
    ap.add_argument("--size", default="16x16", help="panel size, default 16x16")
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("info")
    p = sub.add_parser("order")
    p.add_argument(
        "value",
        type=int,
        nargs="?",
        help="0=RGB 1=GRB 2=BRG 3=BGR 4=RBG 5=GBR; omit to query",
    )
    sub.add_parser("bars")
    p = sub.add_parser("osd")
    p.add_argument("state", nargs="?", help="on | off; omit to query")
    p = sub.add_parser("rate")
    p.add_argument(
        "cycles",
        type=int,
        nargs="?",
        help="PIOC clocks per bit, 48..72; omit to query (RAM only)",
    )
    p = sub.add_parser("config")
    p.add_argument("--rate", type=int, help="persist the 1-wire bit rate (48..72)")
    p.add_argument("--order", type=int, help="persist the byte order (0..5)")
    p.add_argument("--bright", type=int, help="persist the brightness (0..255)")
    p.add_argument("--mode", help="panel | strip")
    p.add_argument("--limit", type=int, help="frame current limit in mA, 0 = unlimited")
    p = sub.add_parser("solid")
    p.add_argument("r", type=int)
    p.add_argument("g", type=int)
    p.add_argument("b", type=int)
    p = sub.add_parser("bright")
    p.add_argument("value", type=int)
    p = sub.add_parser("image")
    p.add_argument("path")
    p.add_argument("--fps", type=float, default=10.0)
    p.add_argument("--loops", type=int, default=1)
    p = sub.add_parser("anim")
    p.add_argument("path")
    p.add_argument("--fps", type=float, default=10.0)
    p.add_argument("--loops", type=int, default=1000)
    p = sub.add_parser("nor-upload")
    p.add_argument("path")
    p.add_argument("--base", default="0")
    p = sub.add_parser("nor-read")
    p.add_argument("addr")
    p.add_argument("length", type=int)
    p = sub.add_parser("nor-dump")
    p.add_argument("path")
    p.add_argument("--base", default="0")
    p.add_argument("--length", type=int, default=4096)

    args = ap.parse_args()
    w, h = args.size.lower().split("x")
    args.size = (int(w), int(h))

    if not args.port:
        sys.exit("需要 --port COMx")

    ser = open_port(args.port)
    try:
        if args.cmd == "info":
            cmd_info(ser)
        elif args.cmd == "order":
            cmd_order(ser, args.value)
        elif args.cmd == "rate":
            cmd_rate(ser, args.cycles)
        elif args.cmd == "osd":
            if args.state is None:
                cmd_osd(ser)
            else:
                cmd_osd(ser, args.state.lower().startswith("o"))
        elif args.cmd == "config":
            cmd_config(ser, args.rate, args.order, args.bright, args.mode, args.limit)
        elif args.cmd == "bars":
            cmd_bars(ser)
        elif args.cmd == "solid":
            cmd_solid(ser, args.r, args.g, args.b)
        elif args.cmd == "bright":
            cmd_bright(ser, args.value)
        elif args.cmd == "image":
            cmd_image(ser, args.path, args.fps, args.loops)
        elif args.cmd == "anim":
            cmd_image(ser, args.path, args.fps, args.loops)
        elif args.cmd == "nor-upload":
            cmd_nor_upload(ser, args.path, int(args.base, 0))
        elif args.cmd == "nor-read":
            cmd_nor_read(ser, int(args.addr, 0), args.length)
        elif args.cmd == "nor-dump":
            cmd_nor_dump(ser, int(args.base, 0), args.length, args.path)
    finally:
        ser.close()


if __name__ == "__main__":
    main()
