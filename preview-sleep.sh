#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$project_dir/johnny-hd2d" --modern --scene 51 --story-day 6 \
    --skip-steps 150 --seed 4242 --clock 1993-06-01T17:00 "$@"
