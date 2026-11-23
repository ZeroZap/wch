#!/usr/bin/env python3
"""Build (and optionally upload) the N1 NOR cartridge image.

The cartridge image is written at NOR 0x010000 and has this shape:

    0x0000  catalog sector (4 KB): header + up to 32 entries, 0xFF padded
    0x1000  game 0 .bin (4 KB aligned)
    ...     game 1 ...

Catalog entry offsets are absolute NOR addresses, so the launcher can read a
game directly.  The file itself is uploaded with n1ctl / this tool:

    python tools/n1pack.py                         # build build/cart.bin
    python tools/n1pack.py --upload --port COM62   # build + upload
    python tools/n1ctl.py --port COM62 --cart-upload build/cart.bin

Upload protocol (User/n1_usb.h):
    0x41 CART_BEGIN  <u32 total> <u32 crc32>
    0x42 CART_DATA   up to 56 bytes
    0x43 CART_END
"""

import argparse
import json
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.dirname(HERE)
IAP_TOOLS = os.path.normpath(os.path.join(PROJ, "..", "IAP_USB_SIGNED", "tools"))
sys.path.insert(0, IAP_TOOLS)

CART_BASE_OFFSET = 0x00010000
CART_CAT_SIZE = 0x00001000
CART_MAGIC = 0x4E314341
CART_VERSION = 1
CART_MAX_GAMES = 32
CART_ENTRY_SIZE = 52

CMD_CART_INFO = 0x40
CMD_CART_BEGIN = 0x41
CMD_CART_DATA = 0x42
CMD_CART_END = 0x43

THEMES = {
    "black": 0,
    "white": 1,
    "red": 2,
    "green": 3,
    "blue": 4,
    "yellow": 5,
    "cyan": 6,
    "magenta": 7,
    "orange": 8,
    "gray": 9,
    "dgreen": 10,
    "dblue": 11,
    "dred": 12,
    "purple": 13,
    "teal": 14,
    "pink": 15,
}


def icon_bytes(rows):
    if len(rows) != 10:
        raise ValueError("icon must have 10 rows")
    out = bytearray()
    for row in rows:
        if len(row) != 16:
            raise ValueError("icon rows must be 16 columns")
        lo = 0
        hi = 0
        for col, ch in enumerate(row):
            if ch not in ".#":
                raise ValueError("icon characters must be '.' or '#'")
            if ch == "#":
                if col < 8:
                    lo |= 1 << col
                else:
                    hi |= 1 << (col - 8)
        out += bytes((lo, hi))
    return bytes(out)


def build_cart(manifest_path, output_path):
    with open(manifest_path, "r", encoding="utf-8") as fh:
        games = json.load(fh)

    if not games:
        raise ValueError("manifest is empty")
    if len(games) > CART_MAX_GAMES:
        raise ValueError("at most %d games" % CART_MAX_GAMES)

    catalog = bytearray(b"\xff" * CART_CAT_SIZE)
    data = bytearray()
    entries = []

    for idx, game in enumerate(games):
        path = game["file"]
        if not os.path.isabs(path):
            path = os.path.join(PROJ, path)

        with open(path, "rb") as fh:
            payload = fh.read()

        if not payload:
            raise ValueError("%s is empty" % path)
        if len(payload) > 30 * 1024:
            raise ValueError(
                "%s is %d bytes, over the 30 KB game slot" % (path, len(payload))
            )

        # 4 KB align every game inside the file
        if len(data) % CART_CAT_SIZE:
            data += b"\xff" * (CART_CAT_SIZE - (len(data) % CART_CAT_SIZE))

        file_offset = CART_CAT_SIZE + len(data)
        nor_offset = CART_BASE_OFFSET + file_offset
        crc = zlib.crc32(payload) & 0xFFFFFFFF

        name = game["name"].upper().encode("ascii", "replace")[:11].ljust(12, b"\0")
        code = game["code"].upper().encode("ascii", "replace")[:3].ljust(4, b"\0")
        theme = THEMES[game.get("theme", "white").lower()]
        icon = icon_bytes(game["icon"])

        entries.append(
            {
                "index": idx,
                "name": game["name"].upper()[:11],
                "code": game["code"].upper()[:3],
                "offset": nor_offset,
                "size": len(payload),
                "crc": crc,
                "payload": payload,
            }
        )

        data += payload

    if len(data) % CART_CAT_SIZE:
        data += b"\xff" * (CART_CAT_SIZE - (len(data) % CART_CAT_SIZE))

    total_size = CART_CAT_SIZE + len(data)
    data_crc = zlib.crc32(bytes(data)) & 0xFFFFFFFF

    struct.pack_into(
        "<IHHII16s",
        catalog,
        0,
        CART_MAGIC,
        CART_VERSION,
        len(entries),
        total_size,
        data_crc,
        b"\0" * 16,
    )

    # Rebuild the data area with the final alignment so the catalog offsets
    # match the bytes that will actually be uploaded.
    data = bytearray()
    for idx, e in enumerate(entries):
        if len(data) % CART_CAT_SIZE:
            data += b"\xff" * (CART_CAT_SIZE - (len(data) % CART_CAT_SIZE))
        e["offset"] = CART_BASE_OFFSET + CART_CAT_SIZE + len(data)
        data += e["payload"]
    if len(data) % CART_CAT_SIZE:
        data += b"\xff" * (CART_CAT_SIZE - (len(data) % CART_CAT_SIZE))

    total_size = CART_CAT_SIZE + len(data)
    data_crc = zlib.crc32(bytes(data)) & 0xFFFFFFFF
    struct.pack_into(
        "<IHHII16s",
        catalog,
        0,
        CART_MAGIC,
        CART_VERSION,
        len(entries),
        total_size,
        data_crc,
        b"\0" * 16,
    )

    for idx, e in enumerate(entries):
        off = 32 + idx * CART_ENTRY_SIZE
        name = e["name"].encode("ascii", "replace")[:11].ljust(12, b"\0")
        code = e["code"].encode("ascii", "replace")[:3].ljust(4, b"\0")
        game = games[idx]
        theme = THEMES[game.get("theme", "white").lower()]
        icon = icon_bytes(game["icon"])

        struct.pack_into(
            "<12s4sBBHIII20s",
            catalog,
            off,
            name,
            code,
            theme,
            0,
            0,
            e["offset"],
            e["size"],
            e["crc"],
            icon,
        )

    cart = bytes(catalog) + bytes(data)

    out_dir = os.path.dirname(os.path.abspath(output_path))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(output_path, "wb") as fh:
        fh.write(cart)

    print("cartridge : %s" % output_path)
    print("size      : %d bytes (%.1f KB)" % (len(cart), len(cart) / 1024.0))
    print("crc32     : 0x%08X" % (zlib.crc32(cart) & 0xFFFFFFFF))
    print("games     : %d" % len(entries))
    print()
    print("  #  name        code  offset    size   crc32")
    for e in entries:
        print(
            "  %d  %-10s  %-3s  0x%06X  %5d  0x%08X"
            % (e["index"], e["name"], e["code"], e["offset"], e["size"], e["crc"])
        )

    return cart, entries


def upload_cart(port, baud, cart, link=None, progress=True):
    from iap_flash import Link

    own_link = link is None
    if own_link:
        link = Link(port, baud)

    crc = zlib.crc32(cart) & 0xFFFFFFFF
    try:
        reply = link.request(CMD_CART_BEGIN, struct.pack("<II", len(cart), crc))
        if not reply or reply[0] != 0:
            raise RuntimeError("CART_BEGIN failed: %s" % reply.hex())

        sent = 0
        while sent < len(cart):
            chunk = cart[sent : sent + 56]
            reply = link.request(CMD_CART_DATA, chunk)
            if not reply or reply[0] != 0:
                raise RuntimeError("CART_DATA failed at %d: %s" % (sent, reply.hex()))
            sent += len(chunk)
            if progress and (sent % (56 * 32) == 0 or sent == len(cart)):
                print(
                    "  upload %5.1f%% (%d/%d)"
                    % (100.0 * sent / len(cart), sent, len(cart))
                )

        reply = link.request(CMD_CART_END)
        if not reply or reply[0] != 0:
            raise RuntimeError("CART_END failed: %s" % reply.hex())

        info = link.request(CMD_CART_INFO)
        if len(info) >= 12:
            print(
                "upload ok: cart_valid=%d games=%d total=%d"
                % (
                    info[0],
                    info[1],
                    info[2] | (info[3] << 8) | (info[4] << 16) | (info[5] << 24),
                )
            )
    finally:
        if own_link:
            link.close()


def main(argv):
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--manifest",
        default=os.path.join(HERE, "games.json"),
        help="game metadata JSON (default: tools/games.json)",
    )
    ap.add_argument(
        "-o",
        "--output",
        default=os.path.join(PROJ, "build", "cart.bin"),
        help="cartridge image to write",
    )
    ap.add_argument("--upload", action="store_true", help="upload after building")
    ap.add_argument("--port", help="COM7 / /dev/ttyACM0 for --upload")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args(argv)

    if args.upload and not args.port:
        ap.error("--upload requires --port")

    cart, _entries = build_cart(args.manifest, args.output)

    if args.upload:
        print()
        print("uploading to %s ..." % args.port)
        upload_cart(args.port, args.baud, cart)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
