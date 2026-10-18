#!/bin/sh
# Original palm and leaf for the complete coconut gag, modern background.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$project_dir/johnny-hd2d" --modern --palm-original --scene 35 \
    --seed 1 --clock 2026-10-18T12:00 --skip-steps 125 "$@"
