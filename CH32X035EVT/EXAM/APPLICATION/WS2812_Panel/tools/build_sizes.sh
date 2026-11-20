#!/bin/sh
# Build every supported panel geometry and print flash/RAM usage side by side.
#
#   sh tools/build_sizes.sh
#
# The last line is the default configuration, so build/ ends up holding the
# normal 16x16 firmware (or whatever PANEL_W/PANEL_H are set to in
# wsp_config.h) when the script finishes.

cd "$(dirname "$0")" || exit 1

run() {
    label="$1"
    shift
    printf '%-9s ' "$label"
    if sh build_cli.sh "$@" > /tmp/wsp_build.log 2>&1; then
        # riscv-none-embed-size prints: text data bss dec hex filename
        size_line=$(grep -E '^ *[0-9]+[[:space:]]+[0-9]+[[:space:]]+[0-9]+' /tmp/wsp_build.log | tail -1)
        set -- $size_line
        text=$1
        data=$2
        bss=$3
        flash=$((text + data))
        ram=$((data + bss + 2048))          # include the 2K stack
        printf 'flash=%5d B (%2d%%)  ram=%5d B (%2d%%)  free: flash %d KB / ram %d KB\n' \
            "$flash" "$((flash * 100 / 63488))" \
            "$ram" "$((ram * 100 / 16384))" \
            "$(((63488 - flash) / 1024))" "$(((16384 - ram) / 1024))"
    else
        printf 'BUILD FAILED\n'
        grep -E "error|Error" /tmp/wsp_build.log | head -3
    fi
}

run "8x8"     -DPANEL_W=8  -DPANEL_H=8
run "8x16"    -DPANEL_W=8  -DPANEL_H=16
run "8x32"    -DPANEL_W=8  -DPANEL_H=32
run "16x16"   -DPANEL_W=16 -DPANEL_H=16
run "16x32"   -DPANEL_W=16 -DPANEL_H=32
run "32x16"   -DPANEL_W=32 -DPANEL_H=16
run "32x32!"  -DPANEL_W=32 -DPANEL_H=32      # expected to fail: >512 px
run "default"
