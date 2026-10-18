#!/bin/sh
# Historical launcher name; Johnny now always uses original R8 sprites.
# Original fishing scripts; fixed clock prevents writing the real-clock INI.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
scene=${1:-20}
if [ "$#" -gt 0 ]; then shift; fi
exec "$project_dir/johnny-hd2d" --modern --johnny-hd --scene "$scene" \
    --seed 4242 --clock 1993-06-01T10:00 "$@"
