#!/bin/sh
# Host side unit tests for the portable modules (panel / gfx / img_player /
# wsp_app / wsp_seg / wsp_fx).  No hardware and no RISC-V toolchain needed.
#
#   sh tests/run.sh                       # all supported geometries
#   sh tests/run.sh 16x16                 # one geometry
#
# Requires a host gcc on PATH and python (for the .wsp fixtures).
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
PROJ=$(cd "$HERE/.." && pwd)
LIB="$PROJ/User"
OUT="$HERE/build"
SRCS="$LIB/panel.c $LIB/gfx.c $LIB/img_player.c $LIB/img_demo.c \
$LIB/wsp_app.c $LIB/wsp_seg.c $LIB/wsp_fx.c"

CC=${CC:-gcc}

mkdir -p "$OUT/fixtures"
(cd "$HERE" && python gen_fixtures.py > /dev/null)

run_geom() {
    w=${1%x*}
    h=${1#*x}
    printf '%-7s ' "$1"
    # WSP_OSD_ENABLE=0 keeps wsp_osd.c out, the test stubs it instead
    $CC -std=c99 -Wall -Wextra -O1 -DWSP_OSD_ENABLE=0 \
        -DPANEL_W="$w" -DPANEL_H="$h" \
        -I "$HERE/stub" -I "$LIB" \
        "$HERE/test.c" $SRCS -o "$OUT/test_$1" 2> "$OUT/cc_$1.log"
    if [ $? -ne 0 ]; then
        printf 'BUILD FAILED\n'
        head -5 "$OUT/cc_$1.log"
        return 1
    fi
    (cd "$OUT/fixtures" && "../test_$1" | tail -1)
}

FAILED=0
if [ $# -gt 0 ]; then
    GEOMS="$*"
else
    GEOMS="8x8 8x16 8x32 16x16 16x32 32x16"
fi

for g in $GEOMS; do
    run_geom "$g" || FAILED=1
done

exit $FAILED
