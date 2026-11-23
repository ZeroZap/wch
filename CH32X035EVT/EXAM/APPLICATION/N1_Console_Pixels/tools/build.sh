#!/bin/sh
# Build N1_Console, the 5-in-1 WS2812 handheld for the IAP_USB_SIGNED bootloader.
#
#   sh tools/build.sh                  # LQFP48 app at 0x2800 + img + factory.bin
#   sh tools/build.sh --tssop20        # CH32X033 TSSOP20 pinmap (WS2812 on PC18)
#   sh tools/build.sh --lqfp48         # explicit default
#   sh tools/build.sh -DPANEL_W=16 -DPANEL_H=32
#   sh tools/build.sh --no-pack        # just .elf/.bin
#   sh tools/build.sh --standalone     # linked at 0x0 for pure debugger use
#
# Outputs (build/):
#   n1_console.elf/.bin/.map  application, linked at 0x2800
#   n1_console.img            signed image for the USB updater
#   factory.bin               boot + app + header/flag, for the debugger
#   n1_console_standalone.*   linked at 0x0, no bootloader involved
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
BOARD=LQFP48
EXTRA=""
for arg in "$@"; do
    case "$arg" in
    --standalone) STANDALONE=1 ;;
    --no-pack)    PACK=0 ;;
    --tssop20)    BOARD=TSSOP20; EXTRA="$EXTRA -DWSP_BOARD=WSP_BOARD_TSSOP20" ;;
    --lqfp48)     BOARD=LQFP48;  EXTRA="$EXTRA -DWSP_BOARD=WSP_BOARD_LQFP48" ;;
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
echo "board     : $BOARD"

ARCH="-march=rv32imacxw -mabi=ilp32"
CFLAGS="$ARCH -msmall-data-limit=8 -msave-restore -Os -fmessage-length=0 \
-fsigned-char -ffunction-sections -fdata-sections -fno-common \
-Wunused -Wuninitialized -g -std=gnu99 $EXTRA"
INCS="-I$PROJ/User -I$PROJ/User/games -I$IAP/shared -I$SRC/Core -I$SRC/Debug -I$SRC/Peripheral/inc"

rm -rf "$OUT/obj"
mkdir -p "$OUT/obj"

for f in "$PROJ"/User/*.c "$SRC"/Debug/*.c "$SRC"/Core/*.c "$SRC"/Peripheral/src/*.c; do
    riscv-none-embed-gcc $CFLAGS $INCS -c "$f" -o "$OUT/obj/$(basename "$f" .c).o"
done
riscv-none-embed-gcc $CFLAGS $INCS -c "$SRC/Startup/startup_ch32x035.S" \
    -o "$OUT/obj/startup_ch32x035.o"

link_one() {
    NAME=$1
    LD=$2
    riscv-none-embed-gcc $ARCH -msmall-data-limit=8 -msave-restore -Os \
        -T "$LD" -nostartfiles -Xlinker --gc-sections \
        -Wl,-Map,"$OUT/$NAME.map" --specs=nano.specs --specs=nosys.specs \
        "$OUT/obj"/*.o -o "$OUT/$NAME.elf"
    riscv-none-embed-objcopy -O ihex   "$OUT/$NAME.elf" "$OUT/$NAME.hex"
    riscv-none-embed-objcopy -O binary "$OUT/$NAME.elf" "$OUT/$NAME.bin"
    riscv-none-embed-size "$OUT/$NAME.elf"
}

if [ "$STANDALONE" = "1" ]; then
    sed 's/ORIGIN = 0x00002800, LENGTH = 20K/ORIGIN = 0x00000000, LENGTH = 62K/' \
        "$PROJ/Ld/Link.ld" > "$OUT/Link_standalone.ld"
    link_one n1_console_standalone "$OUT/Link_standalone.ld"
    echo
    echo "standalone build: flash build/n1_console_standalone.elf at 0x0 (no bootloader)"
    exit 0
fi

link_one n1_console "$PROJ/Ld/Link.ld"

if [ "$PACK" = "0" ]; then
    exit 0
fi

echo
echo "== packing launcher =="
echo "board: $BOARD"
PY=${PYTHON:-python}

"$PY" "$IAP/tools/iap_pack.py" "$OUT/n1_console.bin" -o "$OUT/n1_console.img"
if [ -f "$IAP/build/boot/boot.bin" ]; then
    "$PY" "$IAP/tools/mkfactory.py" --boot "$IAP/build/boot/boot.bin" \
        --app "$OUT/n1_console.img" -o "$OUT/factory.bin"
    echo
    echo "flash with the debugger : sh $IAP/tools/flash.sh factory $OUT/factory.bin"
else
    echo
    echo "note: $IAP/build/boot/boot.bin not found, skipped factory.bin"
    echo "      build the bootloader first: (cd $IAP && sh tools/build.sh boot)"
fi

echo
echo "build cartridge games : sh tools/build_games.sh"
echo "make + upload cart    : python tools/n1pack.py --upload --port COMx"
echo "update launcher       : python $IAP/tools/iap_flash.py --port COMx --image $OUT/n1_console.img"
