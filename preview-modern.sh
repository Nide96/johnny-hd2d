#!/bin/sh
# Reproducible art preview; injected calendar means no progression is saved.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$project_dir/johnny-hd2d" --modern --seed 1 --clock 2026-10-18T12:00 "$@"
