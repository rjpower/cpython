#!/bin/sh
# The C stack of a process that aborts, without gdb (note 11 section 8.3).
#
# usage: boil/tools/cstack.sh BUILDDIR/python ARGS...
#   Runs the interpreter with ARGS with boil/tools/abortbt.c preloaded, which prints
#   backtrace() on SIGABRT, and then resolves each frame of the interpreter
#   with addr2line (function and line, inlined frames included).  The
#   command's own output is in $CSTACK_LOG (default: a temporary file).
set -u
py=$1
shift
here=$(cd "$(dirname "$0")" && pwd)
so=${TMPDIR:-/tmp}/boil-abortbt.so
if [ ! -f "$so" ] || [ "$here/abortbt.c" -nt "$so" ]; then
    gcc -shared -fPIC -O0 -o "$so" "$here/abortbt.c" || exit 2
fi
log=${CSTACK_LOG:-$(mktemp)}
LD_PRELOAD=$so "$py" "$@" > "$log" 2>&1
rc=$?
echo "rc $rc, output in $log"
sed -n '/=== C backtrace (abortbt) ===/,$p' "$log" | while read -r line; do
    case $line in
    *"$py("*)
        off=$(printf '%s\n' "$line" | sed -n 's/.*(\(+0x[0-9a-f]*\)).*/\1/p')
        if [ -n "$off" ]; then
            addr2line -f -i -C -e "$py" "${off#+}" | paste - - | tr '\n' ' '
            echo
        else
            printf '%s\n' "$line"
        fi
        ;;
    *) printf '%s\n' "$line" ;;
    esac
done
