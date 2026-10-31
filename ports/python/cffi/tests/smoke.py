"""Smoke test for cffi: call libc through in-line ABI mode, including a
callback."""

import importlib.metadata

import _cffi_backend
import cffi
import pycparser

assert cffi.__version__ == "2.1.1", cffi.__version__
assert _cffi_backend.__version__ == "2.1.1"
assert importlib.metadata.version("cffi") == "2.1.1"
# cdef() parses declarations with pycparser; it must be the pinned port.
assert pycparser.__version__ == "3.12"

ffi = cffi.FFI()
ffi.cdef("""
    size_t strlen(const char *);
    int snprintf(char *, size_t, const char *, ...);
    void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
    struct pair { int key; double value; };
    double sqrt(double);
""")
libc = ffi.dlopen(None)  # the interpreter's own symbols, which include libc
# Opening by file name takes the code path fs-encoding.patch changes.
libm = ffi.dlopen("libm.so.6")
assert libm.sqrt(2.25) == 1.5

assert libc.strlen(b"ports") == 5
buffer = ffi.new("char[64]")
text = ffi.new("char[]", b"x")  # variadic arguments must be typed cdata
assert libc.snprintf(buffer, 64, b"%d-%s", ffi.cast("int", 42), text) == 4
assert ffi.string(buffer) == b"42-x"


@ffi.callback("int(const void *, const void *)")
def compare(left, right):
    a, b = ffi.cast("int *", left)[0], ffi.cast("int *", right)[0]
    return (a > b) - (a < b)


values = ffi.new("int[]", [5, 3, 9, 1])
libc.qsort(values, len(values), ffi.sizeof("int"), compare)
assert list(values) == [1, 3, 5, 9]

pairs = ffi.new("struct pair[2]", [(1, 0.5), (2, 1.5)])
assert ffi.sizeof("struct pair") == 16 and pairs[1].value == 1.5
assert ffi.unpack(ffi.cast("char *", buffer), 2) == b"42"

# The backend's METH_O functions, and slice assignment from an iterator.
assert ffi.alignof("struct pair") == 8
assert ffi.typeof(pairs) is ffi.typeof("struct pair[2]")
values[1:3] = iter([7, 8])
assert list(values) == [1, 7, 8, 9]
handle = ffi.new_handle(pairs)
assert ffi.from_handle(handle) is pairs
ffi.errno = 7
assert ffi.errno == 7
ffi.release(values)
