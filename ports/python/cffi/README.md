# cffi

`fs-encoding.patch` replaces `Py_FileSystemDefaultEncoding`, which CPython 3.16 removed, with `PyConfig_Get("filesystem_encoding")` in `ffi.dlopen()`.
