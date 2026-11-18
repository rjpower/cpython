#!/bin/bash
# Reserve CPU 15 and its SMT sibling CPU 7 for timing (approved by the user
# on 2026-11-18). `on` restricts system.slice, user.slice and init.scope to
# the other fourteen CPUs; `off` clears the restriction; both are runtime
# properties, so a reboot also clears them. Only bench.slice (bench-run.sh)
# may use CPUs 7 and 15 while the reservation holds.
set -eu
UNITS=(system.slice user.slice init.scope)
case ${1:-status} in
    on) cpus=0-6,8-14 ;;
    off) cpus= ;;
    status)
        for u in "${UNITS[@]}"; do
            printf '%-14s %s\n' "$u" "$(cat /sys/fs/cgroup/$u/cpuset.cpus.effective)"
        done
        exit 0 ;;
    *) echo "usage: $0 on|off|status" >&2; exit 2 ;;
esac
for u in "${UNITS[@]}"; do
    sudo -n systemctl set-property --runtime "$u" "AllowedCPUs=$cpus"
done
"$0" status
