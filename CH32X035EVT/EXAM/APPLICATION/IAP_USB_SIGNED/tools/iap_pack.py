#!/usr/bin/env python3
"""Pack, optionally encrypt and sign an application image for IAP_USB_SIGNED.

The output file is exactly what the bootloader reconstructs on flash:

    +0x00  128 byte header  (shared/img_hdr.h, img_hdr_t)
    +0x80  img_len bytes payload (ciphertext when --encrypt is used)

Signing is HMAC-SHA256 over header[0..0x20) || stored payload; the AES key and
the HMAC key are read from boot/User/img_key.c so hero and host cannot drift
apart.

Examples
    python tools/iap_pack.py build/app/app.bin -o build/app.img
    python tools/iap_pack.py build/app/app.bin -o build/app.img --encrypt

    # check the tool against the C implementation
    python tools/iap_pack.py --selfcheck
    # inspect / verify an existing image
    python tools/iap_pack.py --verify build/app.img
"""

import argparse
import hashlib
import hmac as hmac_mod
import os
import re
import struct
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.dirname(HERE)

# ---------------------------------------------------------------------------
# constants, mirrored from shared/img_hdr.h (the self check parses the header)
# ---------------------------------------------------------------------------
IMG_MAGIC = 0x474D4957
IMG_HDR_VER = 1
IMG_HDR_SIZE = 128
IMG_HMAC_COVER = 0x20

IMG_FLAG_ENCRYPTED = 0x0001
IMG_FLAG_KNOWN_MASK = 0x0001

IMG_APP_EXEC_ADDR = 0x00002800
IMG_SLOT_SIZE = 0x0000C800

IMG_HDR_OFFSET = 0x0000F000
IMG_FLAG_OFFSET = 0x0000F100

IMG_FLAG_MAGIC = 0x47414C46
IMG_STATE_VALID = 1

DEFAULT_HMAC_KEY = b"WCH-IAP-DEV-HMAC-KEY-REPLACE-ME!"
DEFAULT_AES_KEY = b"WCH-IAP-AES-DEV!"

HEADER_FMT = "<IHHII16s"  # magic, ver, flags, load, len, iv
assert struct.calcsize(HEADER_FMT) == IMG_HMAC_COVER


# ---------------------------------------------------------------------------
# AES-128 (encryption only, CTR mode)
# ---------------------------------------------------------------------------
SBOX = bytes.fromhex(
    "637c777bf26b6fc53001672bfed7ab76ca82c97dfa5947f0add4a2af9ca472c0"
    "b7fd9326363ff7cc34a5e5f171d8311504c723c31896059a071280e2eb27b275"
    "09832c1a1b6e5aa0523bd6b329e32f8453d100ed20fcb15b6acbbe394a4c58cf"
    "d0efaafb434d338545f9027f503c9fa851a3408f929d38f5bcb6da2110fff3d2"
    "cd0c13ec5f974417c4a77e3d645d197360814fdc222a908846eeb814de5e0bdb"
    "e0323a0a4906245cc2d3ac629195e479e7c8376d8dd54ea96c56f4ea657aae08"
    "ba78252e1ca6b4c6e8dd741f4bbd8b8a703eb5664803f60e613557b986c11d9e"
    "e1f8981169d98e949b1e87e9ce5528df8ca1890dbfe6426841992d0fb054bb16"
)

RCON = [0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36]


def _xtime(x):
    x <<= 1
    if x & 0x100:
        x = (x ^ 0x1B) & 0xFF
    return x


def _key_expand(key):
    assert len(key) == 16
    rk = bytearray(key)
    for i in range(16, 176, 4):
        t = list(rk[i - 4 : i])
        if i % 16 == 0:
            t = t[1:] + t[:1]
            t = [SBOX[b] for b in t]
            t[0] ^= RCON[i // 16]
        for j in range(4):
            rk.append(rk[i - 16 + j] ^ t[j])
    return bytes(rk)


def _encrypt_block(rk, block):
    st = list(block)
    st = [a ^ b for a, b in zip(st, rk[0:16])]

    for rnd in range(1, 10):
        st = [SBOX[b] for b in st]
        st = [st[(r + 4 * ((c + r) % 4))] for c in range(4) for r in range(4)]
        mixed = []
        for c in range(4):
            a0, a1, a2, a3 = st[4 * c : 4 * c + 4]
            allb = a0 ^ a1 ^ a2 ^ a3
            mixed += [
                a0 ^ allb ^ _xtime(a0 ^ a1),
                a1 ^ allb ^ _xtime(a1 ^ a2),
                a2 ^ allb ^ _xtime(a2 ^ a3),
                a3 ^ allb ^ _xtime(a3 ^ a0),
            ]
        st = [a ^ b for a, b in zip(mixed, rk[rnd * 16 : rnd * 16 + 16])]

    st = [SBOX[b] for b in st]
    st = [st[(r + 4 * ((c + r) % 4))] for c in range(4) for r in range(4)]
    st = [a ^ b for a, b in zip(st, rk[160:176])]
    return bytes(st)


def aes128_ctr_xor(key, iv, data, byte_off=0):
    """CTR keystream XOR, byte_off must be a multiple of 16.

    Matches aes128_ctr_xor() in boot/User/aes.c: a 32 bit big-endian counter in
    bytes 12..15 that is added to (iv + byte_off/16) mod 2**32.
    """
    if byte_off % 16:
        raise ValueError("byte_off must be a multiple of 16")
    rk = _key_expand(key)
    base = int.from_bytes(iv[12:16], "big") + byte_off // 16
    out = bytearray(len(data))
    for i in range(0, len(data), 16):
        ctr = base + i // 16
        block = bytearray(iv)
        block[12:16] = (ctr & 0xFFFFFFFF).to_bytes(4, "big")
        ks = _encrypt_block(rk, bytes(block))
        chunk = data[i : i + 16]
        out[i : i + len(chunk)] = bytes(a ^ b for a, b in zip(chunk, ks))
    return bytes(out)


# ---------------------------------------------------------------------------
# keys
# ---------------------------------------------------------------------------
_ARRAY_RE = re.compile(r"\b(\w+)\s*\[[^\]]*\]\s*=\s*\{(.*?)\}", re.S)
_TOKEN_RE = re.compile(r"0[xX][0-9a-fA-F]{1,2}|'(?:\\.|[^'\\])'|\b\d{1,3}\b")


def _token_value(tok):
    if tok[:2] in ("0x", "0X"):
        return int(tok, 16)
    if tok[0] == "'":
        body = tok[1:-1]
        escapes = {"\\0": 0, "\\n": 10, "\\r": 13, "\\t": 9, "\\\\": 92, "\\'": 39}
        if body in escapes:
            return escapes[body]
        return ord(body)
    return int(tok)


def load_keys_from_c(path):
    src = open(path, "r", encoding="utf-8", errors="replace").read()
    arrays = {}
    for name, body in _ARRAY_RE.findall(src):
        arrays[name] = bytes(_token_value(t) for t in _TOKEN_RE.findall(body))

    hmac_key = arrays.get("img_hmac_key")
    aes_key = arrays.get("img_aes_key")
    if hmac_key is None or len(hmac_key) != 32:
        raise SystemExit("error: img_hmac_key[32] not found in %s" % path)
    if aes_key is None or len(aes_key) != 16:
        raise SystemExit("error: img_aes_key[16] not found in %s" % path)
    return hmac_key, aes_key


def load_keys(path=None):
    path = path or os.path.join(PROJ, "boot", "User", "img_key.c")
    if os.path.exists(path):
        return load_keys_from_c(path)
    return DEFAULT_HMAC_KEY, DEFAULT_AES_KEY


# ---------------------------------------------------------------------------
# image
# ---------------------------------------------------------------------------
def crc32(data):
    return zlib.crc32(data) & 0xFFFFFFFF


def build_image(
    payload, hmac_key, aes_key, encrypt=False, iv=None, load_addr=IMG_APP_EXEC_ADDR
):
    if not payload:
        raise ValueError("empty payload")
    if len(payload) > IMG_SLOT_SIZE:
        raise ValueError(
            "payload is %d bytes, slot is %d" % (len(payload), IMG_SLOT_SIZE)
        )

    if iv is None:
        iv = os.urandom(16)
    if len(iv) != 16:
        raise ValueError("iv must be 16 bytes")

    flags = IMG_FLAG_ENCRYPTED if encrypt else 0

    if encrypt:
        padded = payload + b"\x00" * ((16 - len(payload) % 16) % 16)
        ct = aes128_ctr_xor(aes_key, iv, padded)
        stored = ct[: len(payload)]
    else:
        stored = payload

    prefix = struct.pack(
        HEADER_FMT, IMG_MAGIC, IMG_HDR_VER, flags, load_addr, len(payload), iv
    )
    assert len(prefix) == IMG_HMAC_COVER

    mac = hmac_mod.new(hmac_key, prefix + payload, hashlib.sha256).digest()
    digest = hashlib.sha256(payload).digest()

    hdr = bytearray(IMG_HDR_SIZE)
    hdr[0:IMG_HMAC_COVER] = prefix
    hdr[0x20:0x24] = struct.pack("<I", crc32(payload))
    hdr[0x24:0x44] = digest
    hdr[0x44:0x64] = mac

    return bytes(hdr) + stored


def parse_image(blob):
    if len(blob) < IMG_HDR_SIZE:
        raise ValueError("file shorter than the header")

    magic, ver, flags, load_addr, img_len, iv = struct.unpack(
        HEADER_FMT, blob[0:IMG_HMAC_COVER]
    )

    info = {
        "magic": magic,
        "hdr_ver": ver,
        "flags": flags,
        "encrypted": bool(flags & IMG_FLAG_ENCRYPTED),
        "load_addr": load_addr,
        "img_len": img_len,
        "iv": iv,
        "crc32": struct.unpack("<I", blob[0x20:0x24])[0],
        "sha256": blob[0x24:0x44],
        "hmac": blob[0x44:0x64],
    }
    info["payload"] = blob[IMG_HDR_SIZE : IMG_HDR_SIZE + img_len]
    if len(info["payload"]) != img_len:
        raise ValueError("file is truncated: header says %d payload bytes" % img_len)
    return info


def verify_image(blob, hmac_key, aes_key):
    """Return (ok, message).  Checks structure, HMAC/SHA and decryptability."""
    try:
        info = parse_image(blob)
    except ValueError as exc:
        return False, str(exc)

    if info["magic"] != IMG_MAGIC:
        return False, "bad magic 0x%08x" % info["magic"]
    if info["hdr_ver"] != IMG_HDR_VER:
        return False, "unsupported header version %d" % info["hdr_ver"]
    if info["flags"] & ~IMG_FLAG_KNOWN_MASK:
        return False, "unknown flags 0x%04x" % info["flags"]
    if info["load_addr"] != IMG_APP_EXEC_ADDR:
        return False, "load address 0x%08x != 0x%08x" % (
            info["load_addr"],
            IMG_APP_EXEC_ADDR,
        )
    if info["img_len"] == 0 or info["img_len"] > IMG_SLOT_SIZE:
        return False, "img_len %d out of range" % info["img_len"]

    if info["encrypted"]:
        pt = aes128_ctr_xor(aes_key, info["iv"], info["payload"])
    else:
        pt = info["payload"]

    # the MAC, the digest and the CRC all cover the plaintext
    prefix = blob[0:IMG_HMAC_COVER]
    mac = hmac_mod.new(hmac_key, prefix + pt, hashlib.sha256).digest()
    if mac != info["hmac"]:
        return False, "HMAC mismatch (wrong key or corrupted payload)"

    if hashlib.sha256(pt).digest() != info["sha256"]:
        return False, "SHA-256 mismatch"
    if crc32(pt) != info["crc32"]:
        return False, "CRC-32 mismatch"

    return True, "ok, %d payload bytes%s" % (
        info["img_len"],
        ", encrypted" if info["encrypted"] else "",
    )


# ---------------------------------------------------------------------------
# self check
# ---------------------------------------------------------------------------
def _load_c_defines(src):
    out = {}
    for m in re.finditer(r"^#define\s+(\w+)\s+\(?\s*([0-9A-Za-z_xX]+)", src, re.M):
        out[m.group(1)] = m.group(2)
    return out


def _resolve_c_define(defs, name, depth=0):
    """Follow one level of aliasing, e.g. IMG_APP_EXEC_ADDR = IMG_SLOT_OFFSET."""
    if name not in defs:
        raise SystemExit("error: %s not found in img_hdr.h" % name)
    value = defs[name]
    literal = re.fullmatch(r"(0[xX][0-9a-fA-F]+|\d+)[uUlL]*", value)
    if literal:
        return int(re.sub(r"[uUlL]+$", "", literal.group(1)), 0)
    if depth > 4:
        raise SystemExit("error: cannot resolve %s (cycle?)" % name)
    return _resolve_c_define(defs, value, depth + 1)


def selfcheck():
    failures = []

    def check(name, cond, detail=""):
        if cond:
            print("  PASS  %s" % name)
        else:
            failures.append(name)
            print("  FAIL  %s %s" % (name, detail))

    print("AES-128 known answer (FIPS-197 C.1):")
    key = bytes.fromhex("000102030405060708090a0b0c0d0e0f")
    pt = bytes.fromhex("00112233445566778899aabbccddeeff")
    ct = _encrypt_block(_key_expand(key), pt)
    check("aes encrypt", ct.hex() == "69c4e0d86a7b0430d8cdb78070b4c55a", ct.hex())

    print("AES-128-CTR (NIST SP 800-38A F.5.1, 4 blocks, offset 0):")
    key = bytes.fromhex("2b7e151628aed2a6abf7158809cf4f3c")
    iv = bytes.fromhex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff")
    pt = bytes.fromhex(
        "6bc1bee22e409f96e93d7e117393172a"
        "ae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52ef"
        "f69f2445df4f9b17ad2b417be66c3710"
    )
    expected = bytes.fromhex(
        "874d6191b620e3261bef6864990db6ce"
        "9806f66b7970fdff8617187bb9fffdff"
        "5ae4df3edbd5d35e5b4f09020db03eab"
        "1e031dda2fbe03d1792170a0f3009cee"
    )
    check("ctr block 0..3", aes128_ctr_xor(key, iv, pt) == expected)

    # splitting the stream must produce the same keystream
    joined = (
        aes128_ctr_xor(key, iv, pt[0:16], 0)
        + aes128_ctr_xor(key, iv, pt[16:48], 16)
        + aes128_ctr_xor(key, iv, pt[48:64], 48)
    )
    check("ctr resumable at 16 byte offsets", joined == expected)

    print("header layout vs shared/img_hdr.h:")
    hdr_path = os.path.join(PROJ, "shared", "img_hdr.h")
    src = open(hdr_path, "r", encoding="utf-8").read()
    defs = _load_c_defines(src)
    for name, value in (
        ("IMG_MAGIC", IMG_MAGIC),
        ("IMG_HDR_VER", IMG_HDR_VER),
        ("IMG_HDR_SIZE", IMG_HDR_SIZE),
        ("IMG_HMAC_COVER", IMG_HMAC_COVER),
        ("IMG_APP_EXEC_ADDR", IMG_APP_EXEC_ADDR),
        ("IMG_SLOT_SIZE", IMG_SLOT_SIZE),
        ("IMG_BOOT_SIZE", 0x2800),
        ("IMG_HDR_OFFSET", IMG_HDR_OFFSET),
        ("IMG_FLAG_OFFSET", IMG_FLAG_OFFSET),
        ("IMG_FLAG_MAGIC", IMG_FLAG_MAGIC),
    ):
        got = _resolve_c_define(defs, name)
        check(name, got == value, "C=%s py=%s" % (got, value))

    # round trip
    payload = bytes(((i * 31 + 7) ^ (i >> 3)) & 0xFF for i in range(777))
    hmac_key, aes_key = load_keys()
    iv = bytes(range(16))
    for enc in (False, True):
        blob = build_image(payload, hmac_key, aes_key, encrypt=enc, iv=iv)
        ok, msg = verify_image(blob, hmac_key, aes_key)
        check("roundtrip encrypt=%d" % enc, ok, msg)
        check(
            "roundtrip size encrypt=%d" % enc, len(blob) == IMG_HDR_SIZE + len(payload)
        )

    if failures:
        print("\nSELF CHECK FAILED: %s" % ", ".join(failures))
        return 1

    print("\nPACK SELFCHECK PASSED")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main(argv):
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("input", nargs="?", help="raw application binary (.bin)")
    ap.add_argument("-o", "--output", help="output image (.img)")
    ap.add_argument(
        "--encrypt", action="store_true", help="AES-128-CTR encrypt the payload"
    )
    ap.add_argument("--iv", help="16 byte IV as hex (default: random)")
    ap.add_argument("--load", default=None, help="load address, default 0x2800")
    ap.add_argument("--key-file", help="C file with img_hmac_key / img_aes_key")
    ap.add_argument("--verify", metavar="IMG", help="verify an existing image file")
    ap.add_argument("--selfcheck", action="store_true", help="run built-in tests")
    args = ap.parse_args(argv)

    if args.selfcheck:
        return selfcheck()

    hmac_key, aes_key = load_keys(args.key_file)

    if args.verify:
        blob = open(args.verify, "rb").read()
        ok, msg = verify_image(blob, hmac_key, aes_key)
        print(("OK   " if ok else "FAIL ") + args.verify + ": " + msg)
        return 0 if ok else 1

    if not args.input or not args.output:
        ap.error("input and -o/--output are required (or use --verify/--selfcheck)")

    payload = open(args.input, "rb").read()
    iv = bytes.fromhex(args.iv) if args.iv else None
    if iv is not None and len(iv) != 16:
        ap.error("--iv must be 32 hex characters")

    load = int(args.load, 0) if args.load else IMG_APP_EXEC_ADDR

    blob = build_image(
        payload, hmac_key, aes_key, encrypt=args.encrypt, iv=iv, load_addr=load
    )

    ok, msg = verify_image(blob, hmac_key, aes_key)
    if not ok:
        print("error: generated image does not verify: " + msg, file=sys.stderr)
        return 1

    with open(args.output, "wb") as fh:
        fh.write(blob)

    print(
        "wrote %s: %d payload bytes, %d bytes total, %s"
        % (
            args.output,
            len(payload),
            len(blob),
            "encrypted" if args.encrypt else "plain",
        )
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
