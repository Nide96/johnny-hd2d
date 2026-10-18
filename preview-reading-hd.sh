#!/bin/sh
# Historical launcher name; Johnny now always uses original R8 sprites.
# Original reading scripts: 34 book, 35 cap/leaf gag; fixed clock, no INI save.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
scene=${1:-34}
if [ "$#" -gt 0 ]; then shift; fi
exec "$project_dir/johnny-hd2d" --modern --johnny-hd --scene "$scene" \
    --seed 1 --clock 2026-10-18T12:00 "$@"
