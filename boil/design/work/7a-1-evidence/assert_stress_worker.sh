#!/bin/bash
set -eu
artifact_dir=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$artifact_dir/../../../.." && pwd)
python_bin="$repo/build/debug/python"
interpreter_flags=()
while [[ $# -gt 0 && $1 != -m ]]; do
    case "$1" in
        -W|-X|--check-hash-based-pycs)
            [[ $# -ge 2 ]] || exit 2
            interpreter_flags+=("$1" "$2")
            shift 2 ;;
        -*) interpreter_flags+=("$1"); shift ;;
        *) printf 'unexpected worker argument: %s\n' "$1" >&2; exit 2 ;;
    esac
done
[[ $# -ge 2 && $1 == -m ]] || exit 2
exec "$python_bin" "${interpreter_flags[@]}" "$artifact_dir/assert_stress_worker.py" "$@"
