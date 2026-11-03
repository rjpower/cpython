#!/bin/bash
# Preserve regrtest's interpreter flags before entering the fixed launcher.
set -eu
tools_dir=$(cd "$(dirname "$0")" && pwd)
: "${GATE_VERIFY_PLAN:?}" "${GATE_VERIFY_PY:?}"
flags=()
while [[ $# -gt 0 && $1 != -m ]]; do
    case "$1" in
        -W|-X|--check-hash-based-pycs)
            [[ $# -ge 2 ]] || exit 2
            flags+=("$1" "$2"); shift 2 ;;
        -W?*|-X?*|--check-hash-based-pycs=*)
            flags+=("$1"); shift ;;
        -E|-I|-B|-P|-s|-S|-u)
            flags+=("$1"); shift ;;
        *) if [[ $1 =~ ^-(b+|d+|q+|v+|O+)$ ]]; then
               flags+=("$1"); shift; continue
           fi
           exec "$GATE_VERIFY_PY" "${flags[@]}" "$tools_dir/verify_launch.py" \
             --plan "$GATE_VERIFY_PLAN" --role worker -- --rejected-interpreter-argument "$1" ;;
    esac
done
exec "$GATE_VERIFY_PY" "${flags[@]}" "$tools_dir/verify_launch.py" \
    --plan "$GATE_VERIFY_PLAN" --role worker -- "$@"
