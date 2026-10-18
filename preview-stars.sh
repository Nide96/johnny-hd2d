#!/bin/sh
# Fixed calendar, live visual clock: stars/clouds/water animate normally.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$project_dir/johnny-hd2d" --modern --palm-original \
    --seed 4242 --clock 2026-10-18T00:00 "$@"
