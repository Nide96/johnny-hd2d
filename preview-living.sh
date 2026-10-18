#!/bin/sh
# Presentation-only 24h cycle in two minutes. Does not alter VM time or INI.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$project_dir/johnny-hd2d" --modern --palm-original \
    --seed 4242 --clock 2026-10-18T17:00 --time-lapse 120 "$@"
