#!/usr/bin/env python3
"""Talk to Strip_Games over USB CDC (PID 1A86:FE0E).

Same framing as the bootloader (see User/strip_usb.h):

    A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8      (CRC8 poly 0x07, init 0)
    replies use CMD | 0x80

Examples
    python tools/stripctl.py --port COM62 --info
    python tools/stripctl.py --port COM62 --games
    python tools/stripctl.py --port COM62 --ping 00112233
    python tools/stripctl.py --port COM62 --game 1
    python tools/stripctl.py --port COM62 --bright 64
    python tools/stripctl.py --port COM62 --upgrade

`--upgrade` writes the UPGRADE flag and resets; the board then re-enumerates as
the bootloader (1A86:FE0D) and iap_flash.py can push a new image:

    python ../IAP_USB_SIGNED/tools/iap_flash.py --port COM61 --image build/strip_games.img
"""

import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
IAP_TOOLS = os.path.normpath(os.path.join(HERE, "..", "..", "IAP_USB_SIGNED", "tools"))
sys.path.insert(0, IAP_TOOLS)

from iap_flash import Link  # noqa: E402  (also pulls in iap_pack)

CMD_INFO = 0x01
CMD_PING = 0x02
CMD_SET_GAME = 0x10
CMD_SET_BRIGHT = 0x11
CMD_UPGRADE = 0x20

GAMES = ["energy duel", "bomb defuse", "1D runner"]

MAX_PAYLOAD = 56


def do_info(link):
    data = link.request(CMD_INFO)
    if len(data) < 5:
        print("short INFO reply (%d bytes)" % len(data))
        return 1

    ver, leds, fps, game, bright = data[0], data[1], data[2], data[3], data[4]
    name = GAMES[game] if game < len(GAMES) else "?"
    print("protocol   : v%d" % ver)
    print("leds       : %d" % leds)
    print("frame rate : %d fps" % fps)
    print("game       : %d (%s)" % (game, name))
    print("brightness : %d/255" % bright)
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


def do_games(link):
    data = link.request(CMD_INFO)
    current = data[3] if len(data) >= 4 else 0xFF
    for i, name in enumerate(GAMES):
        print("%s %d  %s" % ("*" if i == current else " ", i, name))
    print("(* = running now; switch with --game N)")
    return 0


def do_game(link, index):
    link.request(CMD_SET_GAME, bytes([index & 0xFF]))
    print("game set to %d" % index)
    return 0


def do_bright(link, level):
    link.request(CMD_SET_BRIGHT, bytes([level & 0xFF]))
    print("brightness set to %d" % level)
    return 0


def do_upgrade(link):
    link.request(CMD_UPGRADE)
    print("UPGRADE accepted")
    print("the board resets into the bootloader (1A86:FE0D); then run:")
    print(
        "  python ../IAP_USB_SIGNED/tools/iap_flash.py --port COMx --image build/strip_games.img"
    )
    return 0


def main():
    ap = argparse.ArgumentParser(description="Strip_Games USB CDC control tool")
    ap.add_argument("--port", required=True, help="COM7 / /dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument(
        "--info", action="store_true", help="read protocol/leds/fps/game/brightness"
    )
    ap.add_argument(
        "--ping",
        nargs="?",
        const="a55a",
        metavar="HEX",
        help="echo test, optional hex payload (default a55a)",
    )
    ap.add_argument(
        "--game",
        type=int,
        metavar="N",
        help="switch game: 0 energy duel, 1 bomb defuse, 2 1D runner",
    )
    ap.add_argument(
        "--bright",
        type=int,
        metavar="0..255",
        help="driver brightness (not the 25%% game scale)",
    )
    ap.add_argument("--upgrade", action="store_true", help="reset into the bootloader")
    ap.add_argument(
        "--games",
        action="store_true",
        help="list the three games and mark the running one",
    )
    args = ap.parse_args()

    if not (
        args.info
        or args.games
        or args.ping is not None
        or args.game is not None
        or args.bright is not None
        or args.upgrade
    ):
        ap.error(
            "nothing to do, pick one of --info/--games/--ping/--game/--bright/--upgrade"
        )

    if args.game is not None and not (0 <= args.game <= 2):
        ap.error("--game must be 0..2")
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
        if args.upgrade:
            rc |= do_upgrade(link)
        return rc
    except TimeoutError as exc:
        print("no reply: %s" % exc)
        print("check the port: the app answers as 1A86:FE0E, the boot as 1A86:FE0D")
        return 1
    finally:
        link.close()


if __name__ == "__main__":
    sys.exit(main())
