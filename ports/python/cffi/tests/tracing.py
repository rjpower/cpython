"""Every cffi type that holds references is traced: objects reachable only
through cdata, fields, iterators and out-of-line FFI objects survive
collections."""

import gc
import importlib.util
import os
import tempfile

import _cffi_backend as backend
import cffi

INT = backend.new_primitive_type("int")
INT_P = backend.new_pointer_type(INT)


def churn():
    # Allocate types that reuse the memory of anything freed by mistake.
    for n in range(20):
        junk = [backend.new_array_type(INT_P, 1000 + 50 * n + i)
                for i in range(50)]
        gc.collect()
    del junk


# Each ctype below is held only by the object that uses it: the unique type
# cache holds weak references.
held = {
    "cdata": backend.cast(backend.new_pointer_type(
        backend.new_array_type(INT_P, 3)), 0),
    "owning": backend.newp(backend.new_array_type(INT_P, 4), [1, 2, 3, 4]),
    "handle": backend.newp_handle(backend.new_pointer_type(
        backend.new_void_type()), ["payload"]),
    "frombuf": backend.from_buffer(backend.new_array_type(
        backend.new_pointer_type(backend.new_primitive_type("char")), None),
        bytearray(b"abc")),
    "iterator": iter(backend.newp(backend.new_array_type(INT_P, 2), [5, 6])),
}
point = backend.new_struct_type("struct point")
backend.complete_struct_or_union(point, [
    ("x", INT, -1), ("y", backend.new_array_type(INT_P, 2), -1)])
held["field"] = backend.typeoffsetof(point, "y")[0]
held["struct_ptr"] = backend.newp(backend.new_pointer_type(point),
                                  [1, [2, 3]])
del point

# Out-of-line ABI mode: the FFI object keeps the strings it was built from.
builder = cffi.FFI()
builder.cdef("size_t strlen(const char *); struct pair { int a, b; };")
builder.set_source("_cffi_smoke_abi", None)
with tempfile.TemporaryDirectory() as tmp:
    path = os.path.join(tmp, "_cffi_smoke_abi.py")
    builder.emit_python_code(path)
    spec = importlib.util.spec_from_file_location("_cffi_smoke_abi", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
abi = module.ffi
del module, spec, builder

churn()
assert repr(held["cdata"]) == "<cdata 'int(*)[3]' NULL>", held["cdata"]
assert list(held["owning"]) == [1, 2, 3, 4]
assert backend.typeof(held["owning"]).cname == "int[4]"
assert backend.from_handle(held["handle"]) == ["payload"]
assert backend.typeof(held["frombuf"]).cname == "char[]"
assert list(held["iterator"]) == [5, 6]
assert held["field"].cname == "int[2]"
assert list(held["struct_ptr"].y) == [2, 3]
libc = abi.dlopen(None)
assert libc.strlen(b"traced") == 6
assert abi.sizeof("struct pair") == 8
assert abi.typeof("struct pair *").cname == "struct pair *"
print("ok")
