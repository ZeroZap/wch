#!/bin/sh
# Host side test suite for IAP_USB_SIGNED -- no hardware required.
#
#   sh tests/run.sh
#
# 1. crypto primitives (SHA-256, HMAC, AES-CTR) against published vectors
# 2. the bootloader image state machine against a simulated flash array
# 3. tools/iap_pack.py self check (AES vectors + header constants vs img_hdr.h)
# 4. byte for byte cross check: the C installer and the Python packer must
#    produce exactly the same image, plain and encrypted
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
cd "$HERE"

CC=${CC:-gcc}
PY=${PYTHON:-python}
OUT=build
INC="-I../shared -I../boot/User -I."

mkdir -p "$OUT"
rm -f "$OUT"/*.img "$OUT"/*.bin

echo "== crypto vectors =="
$CC -std=gnu99 -Wall -Wextra -O1 $INC test_crypto.c \
    ../boot/User/sha256.c ../boot/User/aes.c -o "$OUT/test_crypto.exe"
"./$OUT/test_crypto.exe"

echo
echo "== image state machine (simulated flash) =="
$CC -std=gnu99 -Wall -Wextra -O1 $INC -Istub test_install.c \
    ../boot/User/img_install.c ../boot/User/img_key.c \
    ../boot/User/sha256.c ../boot/User/aes.c stub/img_flash_host.c \
    -o "$OUT/test_install.exe"
"./$OUT/test_install.exe" "$OUT"

IV=000102030405060708090a0b0c0d0e0f

echo
echo "== host packer self check =="
$PY ../tools/iap_pack.py --selfcheck

echo
echo "== C installer vs python packer =="
$PY ../tools/iap_pack.py "$OUT/vec.bin" -o "$OUT/vec_pack.img" --iv $IV
cmp "$OUT/vec_expected.img" "$OUT/vec_pack.img"
echo "  PASS  plain image is byte identical"

$PY ../tools/iap_pack.py "$OUT/vec.bin" -o "$OUT/vec_pack_enc.img" --encrypt --iv $IV
cmp "$OUT/vec_expected_enc.img" "$OUT/vec_pack_enc.img"
echo "  PASS  encrypted image is byte identical"

echo
echo "ALL HOST TESTS PASSED"
