#!/bin/sh
# Build the five NOR cartridge game images for N1_Console.
#
#   sh tools/build_games.sh              # LQFP48 pinmap
#   sh tools/build_games.sh --tssop20    # TSSOP20 game images
#   sh tools/build_games.sh --no-pack    # accepted for symmetry (nothing to pack)
#
# Outputs:
#   build/games/snake.bin / .elf / .map
#   build/games/breakout.bin ...
#
# Each game is linked at 0x6800 with a 32 KB cap (Ld/Game.ld).  The launcher
# copies one of these files from NOR into the internal game slot and jumps.
set -e

PROJ=$(cd "$(dirname "$0")/.." && pwd)
SRC="$PROJ/../../SRC"
IAP="$PROJ/../IAP_USB_SIGNED"
OUT="$PROJ/build/games"
OBJ="$OUT/obj"

STANDALONE=0
BOARD=LQFP48
EXTRA=""
for arg in "$@"; do
    case "$arg" in
    --tssop20) BOARD=TSSOP20; EXTRA="$EXTRA -DWSP_BOARD=WSP_BOARD_TSSOP20" ;;
    --lqfp48)  BOARD=LQFP48;  EXTRA="$EXTRA -DWSP_BOARD=WSP_BOARD_LQFP48" ;;
    --standalone) STANDALONE=1 ;;
    --no-pack) ;;
    *)         EXTRA="$EXTRA $arg" ;;
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

rm -rf "$OUT"
mkdir -p "$OBJ/common"

if [ "$STANDALONE" = "1" ]; then
    sed 's/ORIGIN = 0x00007800, LENGTH = 30K/ORIGIN = 0x00000000, LENGTH = 62K/' \
        "$PROJ/Ld/Game.ld" > "$OUT/Game_standalone.ld"
    GAME_LD="$OUT/Game_standalone.ld"
    SUFFIX=_standalone
else
    GAME_LD="$PROJ/Ld/Game.ld"
    SUFFIX=
fi

COMMON="panel_gfx input audio nor_store oled game_runtime game_util ws2812 RGB1W wsp_gamma wsp_tick system_ch32x035 ch32x035_it"
for n in $COMMON; do
    riscv-none-embed-gcc $CFLAGS $INCS -c "$PROJ/User/$n.c" -o "$OBJ/common/$n.o"
done
for f in "$SRC"/Debug/*.c "$SRC"/Core/*.c "$SRC"/Peripheral/src/*.c; do
    riscv-none-embed-gcc $CFLAGS $INCS -c "$f" -o "$OBJ/common/$(basename "$f" .c).o"
done
riscv-none-embed-gcc $CFLAGS $INCS -c "$SRC/Startup/startup_ch32x035.S" \
    -o "$OBJ/common/startup_ch32x035.o"

build_game() {
    NAME=$1
    riscv-none-embed-gcc $CFLAGS $INCS -c "$PROJ/User/games/game_$NAME.c" -o "$OBJ/game_$NAME.o"
    riscv-none-embed-gcc $CFLAGS $INCS -c "$PROJ/User/games/main_$NAME.c" -o "$OBJ/main_$NAME.o"
    riscv-none-embed-gcc $ARCH -msmall-data-limit=8 -msave-restore -Os \
        -T "$GAME_LD" -nostartfiles -Xlinker --gc-sections \
        -Wl,-Map,"$OUT/$NAME$SUFFIX.map" --specs=nano.specs --specs=nosys.specs \
        "$OBJ/common"/*.o "$OBJ/game_$NAME.o" "$OBJ/main_$NAME.o" \
        -o "$OUT/$NAME$SUFFIX.elf"
    riscv-none-embed-objcopy -O binary "$OUT/$NAME$SUFFIX.elf" "$OUT/$NAME$SUFFIX.bin"
    riscv-none-embed-size "$OUT/$NAME$SUFFIX.elf"
    SZ=$(wc -c < "$OUT/$NAME$SUFFIX.bin")
    if [ "$SZ" -gt 30720 ]; then
        echo "$NAME$SUFFIX.bin is $SZ bytes, over the 30 KB cartridge limit" >&2
        exit 1
    fi
}

for g in snake breakout invaders flappy simon; do
    build_game "$g"
done

echo
echo "game images in $OUT"
if [ "$STANDALONE" = "1" ]; then
    echo "standalone: flash build/games/<name>_standalone.elf at 0x0"
    echo "            this overwrites boot; restore factory.bin when done"
else
    echo "build a cartridge : python tools/n1pack.py"
fi
