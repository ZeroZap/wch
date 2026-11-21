#!/bin/sh
# Build WS2812_Panel_IAP, the panel application for the IAP_USB_SIGNED bootloader.
#
#   sh tools/build.sh                  # app at 0x2800, then pack panel.img + factory.bin
#   sh tools/build.sh -DPANEL_W=16 -DPANEL_H=32
#   sh tools/build.sh --no-pack        # just the .elf/.bin
#   sh tools/build.sh --standalone     # linked at 0x0 for pure debugger use
#
# Outputs (build/):
#   panel.elf/.bin/.map        the application, linked at 0x2800
#   panel.img                  signed image for the USB updater
#   factory.bin                boot + app + header/flag, for the debugger
#   panel_standalone.elf/.bin  linked at 0x0, no bootloader involved
#
# Toolchain lookup, first hit wins:
#   1. $RISCV_GCC_BIN      e.g. /d/MounRiver/.../RISC-V Embedded GCC/bin
#   2. riscv-none-embed-gcc on $PATH
#   3. the MounRiver Studio 3 / 2 install locations below
set -e

PROJ=$(cd "$(dirname "$0")/.." && pwd)
SRC="$PROJ/../../SRC"
IAP="$PROJ/../IAP_USB_SIGNED"
OUT="$PROJ/build"

STANDALONE=0
PACK=1
EXTRA=""
for arg in "$@"; do
    case "$arg" in
    --standalone) STANDALONE=1 ;;
    --no-pack)    PACK=0 ;;
    *)            EXTRA="$EXTRA $arg" ;;
    esac
done

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
echo "toolchain : $TC"

ARCH="-march=rv32imacxw -mabi=ilp32"
CFLAGS="$ARCH -msmall-data-limit=8 -msave-restore -Os -fmessage-length=0 \
-fsigned-char -ffunction-sections -fdata-sections -fno-common \
-Wunused -Wuninitialized -g -std=gnu99 $EXTRA"
INCS="-I$PROJ/User -I$IAP/shared -I$SRC/Core -I$SRC/Debug -I$SRC/Peripheral/inc"

rm -rf "$OUT"
mkdir -p "$OUT"

for f in "$PROJ"/User/*.c "$SRC"/Debug/*.c "$SRC"/Core/*.c "$SRC"/Peripheral/src/*.c; do
    riscv-none-embed-gcc $CFLAGS $INCS -c "$f" -o "$OUT/$(basename "$f" .c).o"
done
riscv-none-embed-gcc $CFLAGS $INCS -c "$SRC/Startup/startup_ch32x035.S" \
    -o "$OUT/startup_ch32x035.o"

link_one() {
    NAME=$1
    LD=$2
    riscv-none-embed-gcc $ARCH -msmall-data-limit=8 -msave-restore -Os \
        -T "$LD" -nostartfiles -Xlinker --gc-sections \
        -Wl,-Map,"$OUT/$NAME.map" --specs=nano.specs --specs=nosys.specs \
        "$OUT"/*.o -o "$OUT/$NAME.elf"
    riscv-none-embed-objcopy -O ihex   "$OUT/$NAME.elf" "$OUT/$NAME.hex"
    riscv-none-embed-objcopy -O binary "$OUT/$NAME.elf" "$OUT/$NAME.bin"
    riscv-none-embed-size "$OUT/$NAME.elf"
}

if [ "$STANDALONE" = "1" ]; then
    # same sources, linked at 0x0 so the debugger can run it without the boot
    sed 's/ORIGIN = 0x00002800, LENGTH = 50K/ORIGIN = 0x00000000, LENGTH = 62K/' \
        "$PROJ/Ld/Link.ld" > "$OUT/Link_standalone.ld"
    link_one panel_standalone "$OUT/Link_standalone.ld"
    echo
    echo "standalone build: flash build/panel_standalone.elf at 0x0 (no bootloader)"
    exit 0
fi

link_one panel "$PROJ/Ld/Link.ld"

if [ "$PACK" = "0" ]; then
    exit 0
fi

echo
echo "== packing =="
PY=${PYTHON:-python}

"$PY" "$IAP/tools/iap_pack.py" "$OUT/panel.bin" -o "$OUT/panel.img"
if [ -f "$IAP/build/boot/boot.bin" ]; then
    "$PY" "$IAP/tools/mkfactory.py" --boot "$IAP/build/boot/boot.bin" \
        --app "$OUT/panel.img" -o "$OUT/factory.bin"
    echo
    echo "flash with the debugger : sh tools/flash.sh factory $OUT/factory.bin"
else
    echo
    echo "note: $IAP/build/boot/boot.bin not found, skipped factory.bin"
    echo "      build the bootloader first: (cd $IAP && sh tools/build.sh boot)"
fi
echo "update over USB         : python $IAP/tools/iap_flash.py --port COMx --image $OUT/panel.img"
