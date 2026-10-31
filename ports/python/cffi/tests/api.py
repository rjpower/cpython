"""cffi's out-of-line API mode: set_source() generates a C extension, which
is compiled and imported.  Its one-argument functions are METH_O, which take
and return PyValue."""

import importlib.util
import os
import subprocess
import sysconfig
import tempfile

import cffi

builder = cffi.FFI()
builder.cdef("""
    struct point { int x, y; };
    int add(int, int);
    int negate(int);
    double half(double);
    int answer(void);
    int manhattan(struct point *);
    void set_errno_to(int);
    extern int counter;
    extern "Python" int twice(int);
    int call_twice(int);
    int call_twice_in_thread(int);
""")
builder.set_source("_cffi_smoke_api", """
    #include <errno.h>
    #include <pthread.h>
    struct point { int x, y; };
    static int add(int a, int b) { return a + b; }
    static int negate(int a) { return -a; }
    static double half(double d) { return d / 2; }
    static int answer(void) { return 42; }
    static int manhattan(struct point *p) { return p->x + p->y; }
    static void set_errno_to(int e) { errno = e; }
    static int counter = 3;
    static int twice(int);
    static int call_twice(int x) { return twice(x) + 1; }
    static void *thread_main(void *arg) {
        *(int *)arg = twice(*(int *)arg);
        return NULL;
    }
    /* A thread that Python did not start calls into Python. */
    static int call_twice_in_thread(int x) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, thread_main, &x) != 0) return -1;
        pthread_join(thread, NULL);
        return x;
    }
""")

with tempfile.TemporaryDirectory() as tmp:
    source = os.path.join(tmp, "_cffi_smoke_api.c")
    builder.emit_c_code(source)
    # ffi.compile() needs setuptools, which the check venv does not have.
    target = os.path.join(
        tmp, "_cffi_smoke_api" + sysconfig.get_config_var("EXT_SUFFIX"))
    if sysconfig.is_python_build():  # the uninstalled interpreter
        include = [os.path.join(sysconfig.get_config_var("srcdir"), "Include"),
                   sysconfig.get_config_var("abs_builddir")]
    else:
        include = [sysconfig.get_paths()["include"]]
    subprocess.run([*sysconfig.get_config_var("LDSHARED").split(),
                    *sysconfig.get_config_var("CCSHARED").split(),
                    "-Wall", "-Werror",
                    *("-I" + path for path in include), source, "-o", target],
                   check=True)
    spec = importlib.util.spec_from_file_location("_cffi_smoke_api", target)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
ffi, lib = module.ffi, module.lib

# METH_VARARGS, METH_O with an inline int and float, METH_NOARGS.
assert lib.add(2, 3) == 5 and lib.add(2**30, 2**30 - 1) == 2**31 - 1
assert lib.negate(7) == -7 and lib.negate(-(2**31) + 1) == 2**31 - 1
assert lib.half(3.0) == 1.5 and lib.half(5) == 2.5
assert lib.answer() == 42
point = ffi.new("struct point *", [3, 4])  # a struct from a list of ints
assert lib.manhattan(point) == 7
try:
    lib.negate("x")
except TypeError:
    pass
else:
    raise AssertionError("negate('x') did not raise")
lib.set_errno_to(5)
assert ffi.errno == 5
ffi.errno = 0
assert lib.counter == 3  # a global variable
lib.counter += 1
assert lib.counter == 4


@ffi.def_extern()  # the decorator is a METH_O function
def twice(x):
    return 2 * x


assert lib.call_twice(20) == 41
assert lib.call_twice_in_thread(21) == 42

# The FFI object's METH_O methods.
assert ffi.sizeof("struct point") == 8 and ffi.sizeof(point[0]) == 8
assert ffi.alignof("int") == 4
assert ffi.typeof(point) is ffi.typeof("struct point *")
handle = ffi.new_handle(builder)
assert ffi.from_handle(handle) is builder
array = ffi.new("double[3]", (0.5, 1, 2.5))  # inline floats and an int
assert list(array) == [0.5, 1.0, 2.5]
array[0:2] = iter([7, 8.5])  # slice assignment from an iterator
assert list(array) == [7.0, 8.5, 2.5]
with ffi.new("int[2]", [1, 2]) as pair:
    assert sum(pair) == 3
ffi.release(array)
callback = ffi.callback("int(int)")(lambda x: x + 1)  # the decorator form
assert callback(1) == 2
print("ok")
