# cffi

`fs-encoding.patch` replaces `Py_FileSystemDefaultEncoding`, which CPython 3.16 removed, with `PyConfig_Get("filesystem_encoding")` in `ffi.dlopen()`.

`pyvalue-backend.patch` ports `_cffi_backend` to the PyValue C API: its METH_O functions and `tp_iternext` take and return `PyValue`, it reads tuple, list and slice items as values instead of through `PyObject **` item arrays, and it uses `Py_TYPE()` instead of `ob_type`.

`pyvalue-generated.patch` makes the C that cffi generates for users (`ffi.set_source()` and the deprecated `ffi.verify()`) define its METH_O wrappers with `PyValue`, and stops it from defining `Py_LIMITED_API`, which this CPython does not have.

`tracing-collector.patch` gives every cffi type that holds references a `tp_traverse` (cdata, fields, iterators, global variables and the FFI object's realized types and source strings), allocates owning cdata from the object heap instead of `malloc()`, releases a buffer-backed cdata's buffer in `tp_finalize`, and drops an assertion on `Py_REFCNT()`.
