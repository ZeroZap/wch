#!/usr/bin/env python3
"""Flash a packed image into IAP_USB_SIGNED over the USB CDC port.

The device must be in the bootloader (flag state UPGRADE, no valid image, or
the boot window still open).  The tool speaks the frame protocol from
shared/img_hdr.h:

    A5 5A | CMD | LEN_L LEN_H | PAYLOAD | CRC8      (CRC8 poly 0x07)
    replies use CMD | 0x80

Examples
    python tools/iap_flash.py --port COM7 --info
    python tools/iap_flash.py --port COM7 --image build/app.img
    python tools/iap_flash.py --port COM7 --image build/app.img --erase --no-jump
    python tools/iap_flash.py --port COM7 --read 0xF000:128
"""

import argparse
import os
import struct
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import iap_pack  # noqa: E402

SYNC0 = 0xA5
SYNC1 = 0x5A
ACK = 0x80

CMD_INFO = 0x01
CMD_BEGIN = 0x10
CMD_DATA = 0x11
CMD_END = 0x12
CMD_ABORT = 0x13
CMD_JUMP = 0x14
CMD_ERASE = 0x15
CMD_READ = 0x16

# application side command: write the UPGRADE flag and reset into the bootloader
APP_CMD_UPGRADE = 0x20

STATUS = {
    0x00: "OK",
    0x01: "CRC-32 mismatch",
    0x02: "SHA-256 mismatch",
    0x03: "HMAC mismatch (wrong key?)",
    0x04: "bad length or state",
    0x05: "flash program/erase failed",
    0x06: "no valid app",
    0x07: "busy",
}

STATE = {
    0: "NONE (no app)",
    1: "VALID",
    2: "UPGRADE requested",
    3: "INVALID",
}

MAX_PAYLOAD = 56


# ---------------------------------------------------------------------------
# framing
# ---------------------------------------------------------------------------
def crc8(data, seed=0):
    crc = seed
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x07) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


def build_frame(cmd, payload=b""):
    if len(payload) > 0xFFFF:
        raise ValueError("payload too long")
    body = bytes([cmd, len(payload) & 0xFF, (len(payload) >> 8) & 0xFF]) + payload
    return bytes([SYNC0, SYNC1]) + body + bytes([crc8(body)])


class Link:
    def __init__(self, port, baud=115200, quiet=False):
        try:
            import serial
        except ImportError:
            raise SystemExit("pyserial is required: pip install pyserial")
        self.ser = serial.Serial(port, baud, timeout=0.05, write_timeout=2)
        self.ser.reset_input_buffer()
        self.quiet = quiet

    def close(self):
        self.ser.close()

    def send(self, cmd, payload=b""):
        self.ser.write(build_frame(cmd, payload))
        self.ser.flush()

    def read_reply(self, timeout=2.0):
        """Wait for one complete reply frame, return (cmd, payload)."""
        deadline = time.monotonic() + timeout
        state = 0
        cmd = 0
        length = 0
        data = bytearray()
        crc = 0

        while time.monotonic() < deadline:
            raw = self.ser.read(1)
            if not raw:
                continue
            b = raw[0]

            if state == 0:
                state = 1 if b == SYNC0 else 0
            elif state == 1:
                state = 2 if b == SYNC1 else 0
            elif state == 2:
                cmd = b
                crc = crc8([b])
                state = 3
            elif state == 3:
                length = b
                crc = crc8([b], crc)
                state = 4
            elif state == 4:
                length |= b << 8
                crc = crc8([b], crc)
                data = bytearray()
                state = 5 if length else 6
            elif state == 5:
                data.append(b)
                crc = crc8([b], crc)
                if len(data) >= length:
                    state = 6
            else:
                if b == crc:
                    return cmd, bytes(data)
                state = 0

        raise TimeoutError("no reply within %.1fs" % timeout)

    def request(self, cmd, payload=b"", timeout=2.0):
        self.send(cmd, payload)
        rcmd, rdata = self.read_reply(timeout)
        if rcmd != (cmd | ACK):
            raise RuntimeError(
                "unexpected reply 0x%02x for command 0x%02x" % (rcmd, cmd)
            )
        return rdata

    def status(self, cmd, payload=b"", timeout=2.0):
        rdata = self.request(cmd, payload, timeout)
        st = rdata[0] if rdata else 0xFF
        return st


# ---------------------------------------------------------------------------
# commands
# ---------------------------------------------------------------------------
def do_info(link):
    data = link.request(CMD_INFO)
    if len(data) < 28:
        print("short INFO reply (%d bytes)" % len(data))
        return 1
    ver, max_payload, state = data[0], data[1], data[2]
    boot_major = data[3]
    product_id = struct.unpack("<I", data[4:8])[0]
    boot_minor = data[8]
    usb_pid = (data[10] << 8) | data[11]
    load, slot, hdr_off, flag_off = struct.unpack("<IIII", data[12:28])
    print("boot protocol : v%d, %d byte payloads" % (ver, max_payload))
    print("boot version  : %d.%d" % (boot_major, boot_minor))
    print("product id    : 0x%04x" % product_id)
    print("usb pid       : 0x%04x" % usb_pid)
    print("image state   : %s" % STATE.get(state, "0x%02x" % state))
    print("app load addr : 0x%08x" % load)
    print("slot size     : %d bytes" % slot)
    print("header page   : 0x%04x" % hdr_off)
    print("flag page     : 0x%04x" % flag_off)
    return 0


def do_erase(link):
    print("erasing app slot ...")
    st = link.status(CMD_ERASE, timeout=30.0)
    print("erase: %s" % STATUS.get(st, "0x%02x" % st))
    return 0 if st == 0 else 1


def do_read(link, spec):
    if ":" in spec:
        addr_s, len_s = spec.split(":", 1)
    else:
        addr_s, len_s = spec, "64"
    addr = int(addr_s, 0)
    length = int(len_s, 0)

    out = bytearray()
    while len(out) < length:
        n = min(MAX_PAYLOAD, length - len(out))
        data = link.request(CMD_READ, struct.pack("<IH", addr + len(out), n))
        if len(data) != n:
            print("read failed at 0x%08x" % (addr + len(out)))
            return 1
        out += data

    for off in range(0, len(out), 16):
        chunk = out[off : off + 16]
        print(
            "%08x  %s  %s"
            % (
                addr + off,
                " ".join("%02x" % b for b in chunk),
                "".join(chr(b) if 32 <= b < 127 else "." for b in chunk),
            )
        )
    return 0


def do_upgrade(link, image_path, jump=True, key_args=None):
    blob = open(image_path, "rb").read()
    hmac_key, aes_key = iap_pack.load_keys(*(key_args or (None, None, None)))

    ok, msg = iap_pack.verify_image(blob, hmac_key, aes_key)
    if not ok:
        print("refusing to flash: " + msg)
        return 1

    info = iap_pack.parse_image(blob)
    payload = info["payload"]
    print("image    : %s" % image_path)
    print(
        "payload  : %d bytes%s"
        % (len(payload), ", encrypted" if info["encrypted"] else "")
    )
    print("load addr: 0x%08x" % info["load_addr"])

    begin = (
        struct.pack("<IHI", info["img_len"], info["flags"], info["load_addr"])
        + info["iv"]
    )
    t0 = time.monotonic()
    st = link.status(CMD_BEGIN, begin, timeout=10.0)
    print(
        "BEGIN    : %s (%.0f ms, slot erased in 1 KB sectors)"
        % (STATUS.get(st, "0x%02x" % st), (time.monotonic() - t0) * 1000.0)
    )
    if st != 0:
        return 1

    total = len(payload)
    sent = 0
    t0 = time.monotonic()
    try:
        while sent < total:
            chunk = payload[sent : sent + MAX_PAYLOAD]
            st = link.status(CMD_DATA, chunk, timeout=5.0)
            if st != 0:
                print("\nDATA failed at %d: %s" % (sent, STATUS.get(st, "0x%02x" % st)))
                link.status(CMD_ABORT, timeout=5.0)
                return 1
            sent += len(chunk)
            if not link.quiet:
                pct = sent * 100 // total
                print("\r  %5.1f%%  %d/%d" % (pct, sent, total), end="", flush=True)
    finally:
        if not link.quiet:
            print()

    dt = time.monotonic() - t0
    if dt > 0:
        print("  %.0f bytes/s" % (total / dt))

    t0 = time.monotonic()
    st = link.status(CMD_END, info["hmac"], timeout=15.0)
    print(
        "END      : %s (%.0f ms, header+flag write)"
        % (STATUS.get(st, "0x%02x" % st), (time.monotonic() - t0) * 1000.0)
    )
    if st != 0:
        return 1

    if jump:
        try:
            st = link.status(CMD_JUMP, timeout=5.0)
            print("JUMP: %s" % STATUS.get(st, "0x%02x" % st))
            if st != 0:
                return 1
        except Exception:
            # the device tears USB down as it jumps, so the port can disappear
            # before the reply reaches us: that is the success path
            print("JUMP: device left the bootloader")

    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main(argv):
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--port", help="CDC port, e.g. COM7 or /dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200, help="ignored by USB CDC")
    ap.add_argument("--image", help="image produced by iap_pack.py")
    ap.add_argument("--erase", action="store_true", help="erase the app slot first")
    ap.add_argument(
        "--no-jump", action="store_true", help="stay in the bootloader after flashing"
    )
    ap.add_argument("--info", action="store_true", help="print bootloader info")
    ap.add_argument("--read", metavar="ADDR:LEN", help="dump flash, e.g. 0xF000:128")
    ap.add_argument(
        "--reboot-to-boot",
        action="store_true",
        help="talk to a running app: ask it to re-enter the bootloader",
    )
    ap.add_argument("--key-file", help="C file with the image keys (default: boot/User/img_key.c)")
    ap.add_argument("--hmac-key", help="32 byte HMAC key: 64 hex chars or 32 ASCII chars")
    ap.add_argument("--aes-key", help="16 byte AES key: 32 hex chars or 16 ASCII chars")
    ap.add_argument("--quiet", action="store_true", help="no progress output")
    args = ap.parse_args(argv)

    if not args.port:
        ap.error("--port is required")
    if not (args.info or args.erase or args.image or args.read or args.reboot_to_boot):
        ap.error("nothing to do: use --info, --erase, --image or --read")

    link = Link(args.port, args.baud, args.quiet)
    try:
        rc = 0
        if args.reboot_to_boot:
            link.status(APP_CMD_UPGRADE, timeout=3.0)
            print("app asked to enter the bootloader, wait for the boot port")
        if args.erase:
            rc = do_erase(link)
        if args.image and rc == 0:
            rc = do_upgrade(link, args.image, jump=not args.no_jump,
                            key_args=(args.key_file, args.hmac_key, args.aes_key))
        if args.read and rc == 0:
            rc = do_read(link, args.read)
        if args.info and rc == 0:
            rc = do_info(link)
        return rc
    finally:
        link.close()


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
