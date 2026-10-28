#!/bin/bash
# Type-check the extension modules that this machine does not build, so
# that a change to a slot typedef or to their code is compiled somewhere
# (boil/PROCESS.md section 3, gate 3 for rungs that edit Modules/ or the
# slot typedefs).  Each module is compiled with gcc -fsyntax-only against
# headers borrowed from elsewhere on the machine; nothing is linked.
#
# usage: boil/tools/check_unbuilt.sh [--src TREE] [BUILDDIR]
#   TREE      the source tree to check (default: this script's tree)
#   BUILDDIR  a configured build directory whose pyconfig.h is used
#             (default: TREE/build/debug)
# Exit status 1 if any module reports an error or a warning.
#
# The headers and their pinned locations; override with the variables.
#   MPDEC_INCLUDE   libmpdec's mpdecimal.h.  CPython 3.13 still bundles
#                   libmpdec: any 3.13 source tree's
#                   Modules/_decimal/libmpdec (or an installed mpdecimal
#                   2.5).  Compiled with CONFIG_64 and ASM, as configure
#                   does on x86-64, and once more with EXTRA_FUNCTIONALITY.
#   OPENSSL_INCLUDE OpenSSL 3 headers.  node-gyp's copy
#                   (`npx node-gyp install` fills ~/.cache/node-gyp), with
#                   its linux-x86_64 opensslconf.h directory as OPENSSL_ARCH.
#   SQLITE_INCLUDE  sqlite3.h: the libsqlite3-sys crate's bundled copy
#                   (`cargo fetch` of any crate depending on rusqlite).
#   BZ2_INCLUDE     bzlib.h: the libbz2-rs-sys crate's include directory.
#   ZSTD_INCLUDE    zstd.h, zdict.h and zstd_errors.h: the zstd-sys crate
#                   (1.5.7), its root and zstd/lib.
#   uuid            a stub uuid/uuid.h written to a temporary directory
#                   (uuid_t, uuid_generate_time, uuid_generate_time_safe),
#                   compiled with HAVE_UUID_UUID_H and
#                   HAVE_UUID_GENERATE_TIME_SAFE.
# Not checked, for want of headers: _tkinter (Tcl/Tk), _dbm and _gdbm
# (ndbm.h, gdbm.h) and readline.
set -u
SRC=$(cd "$(dirname "$0")/../.." && pwd)
if [ "${1:-}" = --src ]; then SRC=$(cd "$2" && pwd); shift 2; fi
BUILD=$(cd "${1:-$SRC/build/debug}" && pwd)
[ -f "$BUILD/pyconfig.h" ] || { echo "no pyconfig.h in $BUILD" >&2; exit 2; }
REG=$HOME/.cargo/registry/src/index.crates.io-1949cf8c6b5b557f
MPDEC_INCLUDE=${MPDEC_INCLUDE:-/home/power/code/shellsim/.worktrees/ports-imaging-native/target/imaging-native/cpython/Python-3.13.7/Modules/_decimal/libmpdec}
OPENSSL_INCLUDE=${OPENSSL_INCLUDE:-$HOME/.cache/node-gyp/24.19.0/include/node}
OPENSSL_ARCH=${OPENSSL_ARCH:-$OPENSSL_INCLUDE/openssl/archs/linux-x86_64/asm/include}
SQLITE_INCLUDE=${SQLITE_INCLUDE:-$REG/libsqlite3-sys-0.30.1/sqlite3}
BZ2_INCLUDE=${BZ2_INCLUDE:-$REG/libbz2-rs-sys-0.2.5/include}
ZSTD_INCLUDE=${ZSTD_INCLUDE:-$REG/zstd-sys-2.1.1+zstd.1.5.7}
stub=$(mktemp -d)
trap 'rm -rf "$stub"' EXIT
mkdir -p "$stub/uuid"
cat > "$stub/uuid/uuid.h" <<'H'
typedef unsigned char uuid_t[16];
void uuid_generate_time(uuid_t out);
int uuid_generate_time_safe(uuid_t out);
H
status=0
check() {  # file, then extra flags
    local f=$1; shift
    local out rc
    out=$(cd "$BUILD" && gcc -fsyntax-only -std=c11 -Wall \
          -I"$SRC/Include/internal" -I"$BUILD" -I"$SRC/Include" \
          -DPy_BUILD_CORE_MODULE "$@" "$SRC/$f" 2>&1)
    rc=$?
    # A failed compile fails the check whatever it printed; a clean one
    # still fails on any warning.
    if [ $rc -ne 0 ] || printf '%s\n' "$out" | grep -qE "error|warning:"; then
        echo "FAIL $f $* (gcc exit $rc)"; printf '%s\n' "$out"; status=1
    else
        echo "ok   $f$(printf " %s" "$@" | grep -o -- " -DEXTRA_FUNCTIONALITY")"
    fi
}
for d in "$MPDEC_INCLUDE" "$OPENSSL_INCLUDE" "$OPENSSL_ARCH" \
         "$SQLITE_INCLUDE" "$BZ2_INCLUDE" "$ZSTD_INCLUDE"; do
    [ -d "$d" ] || { echo "missing headers: $d" >&2; exit 2; }
done
check Modules/_decimal/_decimal.c -DCONFIG_64 -DASM -I"$MPDEC_INCLUDE"
check Modules/_decimal/_decimal.c -DCONFIG_64 -DASM -DEXTRA_FUNCTIONALITY \
    -I"$MPDEC_INCLUDE"
check Modules/_ssl.c -I"$OPENSSL_INCLUDE" -I"$OPENSSL_ARCH"
check Modules/_hashopenssl.c -I"$OPENSSL_INCLUDE" -I"$OPENSSL_ARCH"
for f in "$SRC"/Modules/_sqlite/*.c; do
    check "${f#$SRC/}" -I"$SQLITE_INCLUDE"
done
check Modules/_bz2module.c -I"$BZ2_INCLUDE"
for f in "$SRC"/Modules/_zstd/*.c; do
    check "${f#$SRC/}" -I"$ZSTD_INCLUDE" -I"$ZSTD_INCLUDE/zstd/lib"
done
check Modules/_uuidmodule.c -I"$stub" -DHAVE_UUID_UUID_H \
    -DHAVE_UUID_GENERATE_TIME_SAFE
echo "not checked: _tkinter, _dbm, _gdbm, readline (no headers)"
exit $status
