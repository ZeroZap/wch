#!/usr/bin/env python3
"""Build a factory image (boot + app + metadata) for IAP_USB_SIGNED.

The WCH OpenOCD flash driver mass-erases before programming, so "flash only the
app" over the debugger is not possible: every debugger write has to carry the
bootloader too.  This tool produces that single file, plus the two pieces that
other programmers may want separately.

Layout produced (dense, 0x0000..0xF200, gaps filled with 0xFF):

    0x0000  bootloader            (boot.bin, padded to 10 KB)
    0x2800  app payload           (plaintext; the app executes in place)
    0xF000  image header page     (128 B header + 0xFF padding)
    0xF100  flag page             (state = VALID, so the boot jumps)

Outputs next to -o:
    <stem>.bin              the dense factory image
    <stem>_payload.bin      just the plaintext app, for `program ... 0x2800`
    <stem>_meta.bin         512 B header+flag page, for `program ... 0xF000`
    <stem>.hex              only with --hex (sparse, absolute addresses)

Examples
    python tools/mkfactory.py --boot build/boot/boot.bin --app build/app.img -o build/factory.bin
    python tools/mkfactory.py --app build/app.img -o build/app_only.bin
    python tools/mkfactory.py --app build/app.img -o build/factory.bin --hex
"""

import argparse
import os
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import iap_pack  # noqa: E402

PAGE = 256
BOOT_END = iap_pack.IMG_APP_EXEC_ADDR  # 0x2800
HDR_OFF = iap_pack.IMG_HDR_OFFSET  # 0xF000
FLAG_OFF = iap_pack.IMG_FLAG_OFFSET  # 0xF100
FACTORY_END = FLAG_OFF + PAGE  # 0xF200


def _flag_page(img_len, hdr_crc32):
    page = bytearray(b"\xff" * PAGE)
    struct.pack_into(
        "<IIII",
        page,
        0,
        iap_pack.IMG_FLAG_MAGIC,
        iap_pack.IMG_STATE_VALID,
        img_len,
        hdr_crc32,
    )
    return bytes(page)


def _header_page(header):
    page = bytearray(b"\xff" * PAGE)
    page[0 : len(header)] = header
    return bytes(page)


def _ihex_record(rtype, addr, data):
    n = len(data)
    total = n + ((addr >> 8) & 0xFF) + (addr & 0xFF) + rtype + sum(data)
    head = ":%02X%04X%02X" % (n, addr & 0xFFFF, rtype)
    body = "".join("%02X" % b for b in data)
    return "%s%s%02X\n" % (head, body, (-total) & 0xFF)


def write_ihex(path, base, data):
    """Minimal Intel HEX writer, 16 bytes per record."""
    with open(path, "w", encoding="ascii", newline="\n") as fh:
        upper = None
        for off in range(0, len(data), 16):
            addr = base + off
            hi = addr >> 16
            if hi != upper:
                upper = hi
                fh.write(_ihex_record(0x04, 0, struct.pack(">H", hi & 0xFFFF)))
            fh.write(_ihex_record(0x00, addr & 0xFFFF, data[off : off + 16]))
        fh.write(":00000001FF\n")


def main(argv):
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--boot", help="bootloader .bin (optional)")
    ap.add_argument("--app", required=True, help="packed image from iap_pack.py")
    ap.add_argument("-o", "--output", required=True, help="factory .bin to write")
    ap.add_argument("--hex", action="store_true", help="also write an Intel HEX")
    ap.add_argument("--key-file", help="C key file (default: boot/User/img_key.c)")
    ap.add_argument("--hmac-key", help="32 byte HMAC key, hex or ASCII")
    ap.add_argument("--aes-key", help="16 byte AES key, hex or ASCII")
    args = ap.parse_args(argv)

    hmac_key, aes_key = iap_pack.load_keys(args.key_file, args.hmac_key, args.aes_key)

    blob = open(args.app, "rb").read()
    ok, msg = iap_pack.verify_image(blob, hmac_key, aes_key)
    if not ok:
        print("refusing to build a factory image: " + msg, file=sys.stderr)
        return 1

    info = iap_pack.parse_image(blob)
    header = blob[0 : iap_pack.IMG_HDR_SIZE]

    # flash always holds plaintext: the app runs in place
    if info["encrypted"]:
        payload = iap_pack.aes128_ctr_xor(aes_key, info["iv"], info["payload"])
    else:
        payload = info["payload"]

    if len(payload) > iap_pack.IMG_SLOT_SIZE:
        print(
            "error: payload %d B > slot %d B" % (len(payload), iap_pack.IMG_SLOT_SIZE),
            file=sys.stderr,
        )
        return 1

    stem = os.path.splitext(args.output)[0]
    os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)

    # pieces for programmers that take several regions
    with open(stem + "_payload.bin", "wb") as fh:
        fh.write(payload)

    hdr_crc32 = zlib.crc32(header[0 : iap_pack.IMG_HMAC_COVER]) & 0xFFFFFFFF
    meta = _header_page(header) + _flag_page(info["img_len"], hdr_crc32)
    with open(stem + "_meta.bin", "wb") as fh:
        fh.write(meta)

    if not args.boot:
        print(
            "wrote %s_payload.bin (%d B) and %s_meta.bin (%d B)"
            % (stem, len(payload), stem, len(meta))
        )
        return 0

    boot = open(args.boot, "rb").read()
    if len(boot) > BOOT_END:
        print(
            "error: boot is %d B, the region is %d B" % (len(boot), BOOT_END),
            file=sys.stderr,
        )
        return 1

    image = bytearray(b"\xff" * FACTORY_END)
    image[0 : len(boot)] = boot
    image[BOOT_END : BOOT_END + len(payload)] = payload
    image[HDR_OFF : HDR_OFF + PAGE] = meta[0:PAGE]
    image[FLAG_OFF : FLAG_OFF + PAGE] = meta[PAGE : 2 * PAGE]

    with open(args.output, "wb") as fh:
        fh.write(image)
    if args.hex:
        write_ihex(stem + ".hex", 0, image)

    print("boot     : %d B" % len(boot))
    print(
        "app      : %d B%s"
        % (len(payload), " (decrypted)" if info["encrypted"] else "")
    )
    print(
        "wrote %s: %d B (0x0000..0x%04X)" % (args.output, len(image), FACTORY_END - 1)
    )
    print("wrote %s_payload.bin and %s_meta.bin" % (stem, stem))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
