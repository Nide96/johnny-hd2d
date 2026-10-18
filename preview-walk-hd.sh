#!/bin/sh
# Historical launcher name; Johnny now always uses original R8 sprites.
# Isolated art preview: original walk tables/ticks, no story progression saved.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$project_dir/johnny-hd2d" --walk-demo --seed 1 --clock 2026-10-18T12:00 "$@"
