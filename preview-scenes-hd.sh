#!/bin/sh
# Historical launcher name; Johnny now always uses original R8 sprites.
# Actual story scripts: 51..53 rest/sleep; no isolated walk loop or saved state.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
scene=${1:-51}
if [ "$#" -gt 0 ]; then shift; fi
exec "$project_dir/johnny-hd2d" --modern --johnny-hd --scene "$scene" \
    --seed 1 --clock 2026-10-18T12:00 "$@"
