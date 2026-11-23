#!/usr/bin/env python3
"""Talk to the N1 console over USB CDC (PID 1A86:FE0F).

Same framing as the bootloader (see User/n1_usb.h):

    A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8      (CRC8 poly 0x07, init 0)
    replies use CMD | 0x80

Examples
    python tools/n1ctl.py --port COM62 --info
    python tools/n1ctl.py --port COM62 --games
    python tools/n1ctl.py --port COM62 --ping 00112233
    python tools/n1ctl.py --port COM62 --game 2
    python tools/n1ctl.py --port COM62 --bright 64
    python tools/n1ctl.py --port COM62 --sound 0
    python tools/n1ctl.py --port COM62 --scores
    python tools/n1ctl.py --port COM62 --menu
    python tools/n1ctl.py --port COM62 --upgrade

    python tools/n1ctl.py --port COM62 --cart-info
    python tools/n1ctl.py --port COM62 --cart-upload build/cart.bin
    python tools/n1ctl.py --port COM62 --cart-erase

`--upgrade` writes the UPGRADE flag and resets; the board then re-enumerates as
the bootloader (1A86:FE0D) and iap_flash.py can push a new launcher image.
"""

import argparse
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
IAP_TOOLS = os.path.normpath(os.path.join(HERE, "..", "..", "IAP_USB_SIGNED", "tools"))
sys.path.insert(0, IAP_TOOLS)

from iap_flash import Link  # noqa: E402
from n1pack import upload_cart  # noqa: E402

CMD_INFO = 0x01
CMD_PING = 0x02
CMD_SELECT_GAME = 0x10
CMD_SET_BRIGHT = 0x11
CMD_SET_SOUND = 0x12
CMD_UPGRADE = 0x20
CMD_MENU = 0x21
CMD_SCORES = 0x30
CMD_RESET_SCORES = 0x31

CMD_CART_INFO = 0x40
CMD_CART_ERASE = 0x45
CMD_CART_LIST = 0x46
CMD_OLED_SCAN = 0x47
CMD_OLED_INIT = 0x48

MAX_PAYLOAD = 56


def u32(b, off):
    return b[off] | (b[off + 1] << 8) | (b[off + 2] << 16) | (b[off + 3] << 24)


def do_info(link):
    data = link.request(CMD_INFO)
    if len(data) < 10:
        print("short INFO reply (%d bytes)" % len(data))
        return 1

    ver, w, h, count, cur, state, bright, sound, nor, cart_valid = data[:10]
    oled = data[10] if len(data) >= 11 else None
    print("protocol   : v%d" % ver)
    print("panel      : %dx%d" % (w, h))
    print("cartridge  : %s (%d games)" % ("valid" if cart_valid else "invalid", count))
    print("current    : %d" % cur)
    print("state      : %s" % ("game" if state else "menu"))
    print("brightness : %d/255" % bright)
    print("sound      : %s" % ("on" if sound else "off"))
    print("NOR        : %s" % ("present" if nor else "not found"))
    if oled is not None:
        print(
            "OLED       : %s"
            % (
                "present"
                if oled
                else "not found (check SCL=PA10/SDA=PA11, addr 0x3C/0x3D)"
            )
        )
    return 0


def do_ping(link, hexstr):
    payload = bytes.fromhex(hexstr or "a55a")
    if len(payload) > MAX_PAYLOAD:
        print("ping payload is limited to %d bytes" % MAX_PAYLOAD)
        return 1
    echo = link.request(CMD_PING, payload)
    if echo == payload:
        print("ping ok, echoed %d bytes" % len(payload))
        return 0
    print("ping MISMATCH: sent %s, got %s" % (payload.hex(), echo.hex()))
    return 1


def cart_info(link):
    data = link.request(CMD_CART_INFO)
    if len(data) < 12:
        return None
    return {
        "valid": data[0],
        "count": data[1],
        "total": u32(data, 2),
        "jedec": u32(data, 6),
        "max_game": data[10] | (data[11] << 8),
    }


def do_cart_info(link):
    info = cart_info(link)
    if not info:
        print("short CART_INFO reply")
        return 1
    print("cart valid : %s" % ("yes" if info["valid"] else "no"))
    print("games      : %d" % info["count"])
    print("image size : %d bytes" % info["total"])
    print("JEDEC id   : 0x%06X" % info["jedec"])
    print("game limit : %d bytes" % info["max_game"])
    return 0


def do_games(link):
    info = cart_info(link)
    if not info:
        print("short CART_INFO reply")
        return 1
    if not info["valid"] or info["count"] == 0:
        print("no valid cartridge")
        return 0

    scores = link.request(CMD_SCORES)
    for i in range(info["count"]):
        data = link.request(CMD_CART_LIST, bytes([i]))
        if len(data) < 26:
            continue
        name = data[0:12].split(b"\0", 1)[0].decode("ascii", "replace")
        code = data[12:16].split(b"\0", 1)[0].decode("ascii", "replace")
        size = u32(data, 18)
        crc = u32(data, 22)
        hi = 0
        if (i * 2 + 1) < len(scores):
            hi = scores[i * 2] | (scores[i * 2 + 1] << 8)
        print(
            "%2d  %-11s %-3s  size=%-5d hi=%-3d crc=0x%08X"
            % (i, name, code, size, hi, crc)
        )
    return 0


def do_scores(link):
    info = link.request(CMD_INFO)
    count = info[3] if len(info) >= 4 else 0
    if count > 8:
        count = 8
    data = link.request(CMD_SCORES)
    for i in range(min(count, len(data) // 2)):
        hi = data[i * 2] | (data[i * 2 + 1] << 8)
        print("%d  hi=%d" % (i, hi))
    return 0


def do_game(link, index):
    if index < 0 or index > 0xFF:
        print("--game must be 0..255 (0xFF returns to the menu)")
        return 1
    reply = link.request(CMD_SELECT_GAME, bytes([index & 0xFF]))
    if reply and reply[0] != 0:
        print("load failed, status %d" % reply[0])
        return 1
    print("game %d started" % index)
    return 0


def do_bright(link, level):
    link.request(CMD_SET_BRIGHT, bytes([level & 0xFF]))
    print("brightness set to %d" % level)
    return 0


def do_sound(link, on):
    link.request(CMD_SET_SOUND, bytes([1 if on else 0]))
    print("sound %s" % ("on" if on else "off"))
    return 0


def do_menu(link):
    link.request(CMD_MENU)
    print("returned to the menu")
    return 0


def do_reset_scores(link):
    link.request(CMD_RESET_SCORES)
    print("high scores cleared")
    return 0


def do_cart_erase(link):
    reply = link.request(CMD_CART_ERASE)
    if reply and reply[0] != 0:
        print("cartridge erase failed, status %d" % reply[0])
        return 1
    print("cartridge erased")
    return 0


def do_upgrade(link):
    link.request(CMD_UPGRADE)
    print("UPGRADE accepted")
    print("the board resets into the bootloader (1A86:FE0D); then run:")
    print(
        "  python ../IAP_USB_SIGNED/tools/iap_flash.py --port COMx --image build/n1_console.img"
    )
    return 0


def do_oled_scan(link):
    reply = link.request(CMD_OLED_SCAN)
    if not reply:
        print("empty OLED_SCAN reply")
        return 1
    n = reply[0]
    addrs = list(reply[1 : 1 + n])
    if not addrs:
        print("I2C bus    : no device answered (check SCL=PA10, SDA=PA11,")
        print("             pull-ups, 3.3V power and that the module is I2C)")
        return 0
    print("I2C bus    : %d device(s)" % len(addrs))
    for a in addrs:
        tag = "  <- SSD1306" if a in (0x3C, 0x3D) else ""
        print("  0x%02X (%d)%s" % (a, a, tag))
    return 0


def do_oled_init(link):
    reply = link.request(CMD_OLED_INIT)
    if not reply:
        print("empty OLED_INIT reply")
        return 1
    if reply[0]:
        print("OLED       : init OK, display should be on")
        return 0
    print("OLED       : init failed (no ACK at 0x3C/0x3D)")
    return 0


def main():
    ap = argparse.ArgumentParser(description="N1 console USB CDC control tool")
    ap.add_argument("--port", required=True, help="COM7 / /dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument(
        "--info", action="store_true", help="protocol / panel / cartridge state"
    )
    ap.add_argument(
        "--games", action="store_true", help="list cartridge games and high scores"
    )
    ap.add_argument(
        "--ping",
        nargs="?",
        const="a55a",
        metavar="HEX",
        help="echo test, optional hex payload",
    )
    ap.add_argument(
        "--game", type=int, metavar="N", help="start cartridge game N (255 = menu)"
    )
    ap.add_argument("--bright", type=int, metavar="0..255", help="global brightness")
    ap.add_argument("--sound", type=int, choices=(0, 1), help="buzzer on/off")
    ap.add_argument("--scores", action="store_true", help="read high scores")
    ap.add_argument("--reset-scores", action="store_true", help="clear high scores")
    ap.add_argument(
        "--menu", action="store_true", help="leave the running game / redraw menu"
    )
    ap.add_argument(
        "--cart-info", action="store_true", help="cartridge catalog summary"
    )
    ap.add_argument("--cart-upload", metavar="FILE", help="upload a cart.bin image")
    ap.add_argument(
        "--cart-erase", action="store_true", help="erase the cartridge area"
    )
    ap.add_argument("--upgrade", action="store_true", help="reset into the bootloader")
    ap.add_argument("--oled-scan", action="store_true", help="scan the OLED I2C bus")
    ap.add_argument(
        "--oled-init", action="store_true", help="re-run OLED init (hot-plug)"
    )
    args = ap.parse_args()

    if not any(
        [
            args.info,
            args.games,
            args.ping is not None,
            args.game is not None,
            args.bright is not None,
            args.sound is not None,
            args.scores,
            args.reset_scores,
            args.menu,
            args.cart_info,
            args.cart_upload,
            args.cart_erase,
            args.upgrade,
            args.oled_scan,
            args.oled_init,
        ]
    ):
        ap.error("nothing to do")

    if args.bright is not None and not (0 <= args.bright <= 255):
        ap.error("--bright must be 0..255")

    link = Link(args.port, args.baud)
    try:
        rc = 0
        if args.info:
            rc |= do_info(link)
        if args.games:
            rc |= do_games(link)
        if args.ping is not None:
            rc |= do_ping(link, args.ping)
        if args.game is not None:
            rc |= do_game(link, args.game)
        if args.bright is not None:
            rc |= do_bright(link, args.bright)
        if args.sound is not None:
            rc |= do_sound(link, args.sound)
        if args.scores:
            rc |= do_scores(link)
        if args.reset_scores:
            rc |= do_reset_scores(link)
        if args.menu:
            rc |= do_menu(link)
        if args.cart_info:
            rc |= do_cart_info(link)
        if args.cart_erase:
            rc |= do_cart_erase(link)
        if args.cart_upload:
            with open(args.cart_upload, "rb") as fh:
                cart = fh.read()
            upload_cart(args.port, args.baud, cart, link=link)
        if args.upgrade:
            rc |= do_upgrade(link)
        if args.oled_scan:
            rc |= do_oled_scan(link)
        if args.oled_init:
            rc |= do_oled_init(link)
        return rc
    except TimeoutError as exc:
        print("no reply: %s" % exc)
        print("check the port: the app answers as 1A86:FE0F, the boot as 1A86:FE0D")
        return 1
    finally:
        link.close()


if __name__ == "__main__":
    sys.exit(main())
