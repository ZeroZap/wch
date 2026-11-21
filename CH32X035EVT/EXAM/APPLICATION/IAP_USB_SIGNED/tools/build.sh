#!/bin/sh
# Build the bootloader or the demo application with the RISC-V toolchain that
# ships with MounRiver Studio.
#
#   sh tools/build.sh boot
#   sh tools/build.sh app
#   sh tools/build.sh app -DBOOT_STAY_WINDOW_MS=0     # extra flags are passed on
#
# Output: build/<target>/<target>.elf .hex .bin .map
#
# Toolchain lookup, first hit wins:
#   1. $RISCV_GCC_BIN      e.g. /d/MounRiver/.../RISC-V Embedded GCC/bin
#   2. riscv-none-embed-gcc on $PATH
#   3. the MounRiver Studio 3 / 2 install locations below
set -e

TARGET=$1
if [ -z "$TARGET" ]; then
    echo "usage: sh tools/build.sh boot|app [extra cflags]" >&2
    exit 1
fi
shift

case "$TARGET" in
    boot|app) ;;
    *) echo "unknown target '$TARGET' (expected boot or app)" >&2; exit 1 ;;
esac

PROJ=$(cd "$(dirname "$0")/.." && pwd)
SRC="$PROJ/../../SRC"
SRCDIR="$PROJ/$TARGET/User"
OUT="$PROJ/build/$TARGET"
EXTRA="$*"

find_toolchain() {
    if [ -n "$RISCV_GCC_BIN" ] && \
       { [ -x "$RISCV_GCC_BIN/riscv-none-embed-gcc.exe" ] || \
         [ -x "$RISCV_GCC_BIN/riscv-none-embed-gcc" ]; }; then
        printf '%s\n' "$RISCV_GCC_BIN"
        return 0
    fi
    if command -v riscv-none-embed-gcc >/dev/null 2>&1; then
        dirname "$(command -v riscv-none-embed-gcc)"
        return 0
    fi
    for d in \
        /d/MounRiver/MounRiver_Studio3/resources/app/resources/win32/components/WCH/Toolchain/RISC-V\ Embedded\ GCC/bin \
        /c/MounRiver/MounRiver_Studio3/resources/app/resources/win32/components/WCH/Toolchain/RISC-V\ Embedded\ GCC/bin \
        /d/MounRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V\ Embedded\ GCC/bin
    do
        if [ -x "$d/riscv-none-embed-gcc.exe" ]; then
            printf '%s\n' "$d"
            return 0
        fi
    done
    return 1
}

TC=$(find_toolchain) || {
    echo "riscv-none-embed-gcc not found." >&2
    echo "Set RISCV_GCC_BIN to the 'RISC-V Embedded GCC/bin' directory." >&2
    exit 1
}

PATH="$TC:$PATH"
export PATH
echo "toolchain: $TC"
echo "target   : $TARGET"

ARCH="-march=rv32imacxw -mabi=ilp32"
CFLAGS="$ARCH -msmall-data-limit=8 -msave-restore -Os -fmessage-length=0 \
-fsigned-char -ffunction-sections -fdata-sections -fno-common \
-Wunused -Wuninitialized -g -std=gnu99 $EXTRA"
INCS="-I$SRCDIR -I$PROJ/shared -I$SRC/Core -I$SRC/Debug -I$SRC/Peripheral/inc"

rm -rf "$OUT"
mkdir -p "$OUT"

for f in "$SRCDIR"/*.c "$SRC"/Debug/*.c "$SRC"/Core/*.c "$SRC"/Peripheral/src/*.c; do
    riscv-none-embed-gcc $CFLAGS $INCS -c "$f" -o "$OUT/$(basename "$f" .c).o"
done
riscv-none-embed-gcc $CFLAGS $INCS -c "$SRC/Startup/startup_ch32x035.S" \
    -o "$OUT/startup_ch32x035.o"

riscv-none-embed-gcc $ARCH -msmall-data-limit=8 -msave-restore -Os \
    -T "$PROJ/$TARGET/Ld/Link.ld" -nostartfiles -Xlinker --gc-sections \
    -Wl,-Map,"$OUT/$TARGET.map" --specs=nano.specs --specs=nosys.specs \
    "$OUT"/*.o -o "$OUT/$TARGET.elf"

riscv-none-embed-objcopy -O ihex   "$OUT/$TARGET.elf" "$OUT/$TARGET.hex"
riscv-none-embed-objcopy -O binary "$OUT/$TARGET.elf" "$OUT/$TARGET.bin"

riscv-none-embed-size "$OUT/$TARGET.elf"
