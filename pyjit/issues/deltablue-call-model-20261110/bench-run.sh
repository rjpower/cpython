#!/bin/bash
# Run a measurement command in bench.slice, the only cgroup allowed on CPUs
# 7 and 15 while reserve-cpus.sh holds the reservation. The command is pinned
# to CPU 7; measure.py pins each timed child to CPU 15.
exec sudo -n systemd-run --quiet --scope --slice=bench.slice -p AllowedCPUs=7,15 \
    --uid="$(id -u)" --gid="$(id -g)" --setenv=HOME="$HOME" --setenv=PATH="$PATH" \
    --setenv=UV_CACHE_DIR="${UV_CACHE_DIR:-$HOME/.cache/uv}" \
    --working-directory="$PWD" -- taskset -c 7 "$@"
