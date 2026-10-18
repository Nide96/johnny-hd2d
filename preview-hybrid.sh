#!/bin/sh
# Unified R8 fishing preview, modern lighting and adaptive shadows.
# Start before sound ID 4 at step 82; skipping to 100 leaves mostly bass ID 6.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$project_dir/johnny-hd2d" --modern --scene 20 --skip-steps 65 \
    --seed 4242 --clock 1993-06-01T17:00 "$@"
