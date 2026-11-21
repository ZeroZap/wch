#!/bin/sh
# Put an image on a CH32X035 board.
#
#   sh tools/flash.sh factory build/factory.bin      # debugger: boot + app + meta
#   sh tools/flash.sh boot    build/boot/boot.bin    # debugger: bootloader only
#   sh tools/flash.sh app     COM7 build/app.img     # USB CDC, no debugger needed
#
# Why the debugger does not flash "just the app": the WCH OpenOCD flash driver
# mass-erases before programming, so an app-only write would wipe the bootloader
# and the image header/flag pages.  Always use the factory image there; use the
# USB path for field updates.
#
# Environment:
#   OPENOCD_BIN   path to openocd.exe        (default: MounRiver Studio 3)
#   OPENOCD_ROOT  its install root           (default: dirname of OPENOCD_BIN)
#   OPENOCD_CFG   board config               (default: bin/wch-riscv.cfg)
#   PYTHON        python interpreter         (default: python)
set -e

ACTION=$1
shift || true

if [ -z "$ACTION" ]; then
    echo "usage: sh tools/flash.sh factory|boot <file> | app <port> <image>" >&2
    exit 1
fi

PROJ=$(cd "$(dirname "$0")/.." && pwd)
PY=${PYTHON:-python}

find_openocd() {
    if [ -n "$OPENOCD_BIN" ] && [ -x "$OPENOCD_BIN" ]; then
        printf '%s\n' "$OPENOCD_BIN"
        return 0
    fi
    for d in \
        /d/MounRiver/MounRiver_Studio3/resources/app/resources/win32/components/WCH/OpenOCD/OpenOCD \
        /c/MounRiver/MounRiver_Studio3/resources/app/resources/win32/components/WCH/OpenOCD/OpenOCD \
        /d/MounRiver/MounRiver_Studio2/resources/app/resources/win32/components/WCH/OpenOCD/OpenOCD
    do
        if [ -x "$d/bin/openocd.exe" ]; then
            printf '%s\n' "$d/bin/openocd.exe"
            return 0
        fi
    done
    return 1
}

program_at_zero() {
    FILE=$1
    [ -f "$FILE" ] || { echo "no such file: $FILE" >&2; exit 1; }

    OCD=$(find_openocd) || {
        echo "openocd not found; set OPENOCD_BIN to .../OpenOCD/OpenOCD/bin/openocd.exe" >&2
        exit 1
    }
    ROOT=${OPENOCD_ROOT:-$(cd "$(dirname "$OCD")/.." && pwd)}
    CFG=${OPENOCD_CFG:-bin/wch-riscv.cfg}

    echo "openocd : $OCD"
    echo "program : $FILE @ 0x00000000"
    "$OCD" -s "$ROOT/scripts" -f "$ROOT/$CFG" \
        -c "program \"$FILE\" 0x00000000 verify reset exit"
}

case "$ACTION" in
factory|boot)
    program_at_zero "$1"
    ;;

app)
    PORT=$1
    IMAGE=$2
    [ -n "$PORT" ] && [ -n "$IMAGE" ] || {
        echo "usage: sh tools/flash.sh app <port> <image>" >&2
        exit 1
    }
    echo "usb     : $PORT"
    echo "image   : $IMAGE"
    echo "(use 'sh tools/flash.sh factory ...' with the debugger instead)"
    "$PY" "$PROJ/tools/iap_flash.py" --port "$PORT" --image "$IMAGE"
    ;;

*)
    echo "unknown action '$ACTION' (expected factory, boot or app)" >&2
    exit 1
    ;;
esac
