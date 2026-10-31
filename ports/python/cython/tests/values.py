"""Cython-generated C works against the PyValue C API: cyvalue.pyx is
translated, compiled with this interpreter's headers and imported, and its
METH_O, METH_FASTCALL, vectorcall and tp_iternext functions are called with
inline ints and floats."""

import asyncio
import functools
import importlib.util
import os
import pickle
import subprocess
import sysconfig
import tempfile
import weakref

from Cython.Compiler import Main

PYX = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cyvalue.pyx")


def build(tmp):
    result = Main.compile(PYX, output_file=os.path.join(tmp, "cyvalue.c"))
    assert result.num_errors == 0, result.num_errors
    if sysconfig.is_python_build():
        srcdir = sysconfig.get_config_var("srcdir")
        includes = [os.path.join(srcdir, "Include"),
                    os.path.dirname(sysconfig.get_config_h_filename())]
    else:
        includes = [sysconfig.get_path("include")]
    so = os.path.join(tmp, "cyvalue" + sysconfig.get_config_var("EXT_SUFFIX"))
    # An unconverted signature is a pointer type mismatch: make it an error.
    subprocess.run(
        [sysconfig.get_config_var("CC"), "-shared", "-fPIC", "-O1",
         "-Werror=incompatible-pointer-types",
         *(f"-I{d}" for d in includes), result.c_file, "-o", so],
        check=True)
    spec = importlib.util.spec_from_file_location("cyvalue", so)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


with tempfile.TemporaryDirectory() as tmp:
    m = build(tmp)

# def functions: METH_NOARGS, METH_O and METH_FASTCALL with keywords.
assert m.nullary() == "nullary"
assert m.unary(5) == 5 and m.unary(1.5) == 1.5 and m.unary(a=2**40) == 2**40
assert m.unary_o(3) == (3, "int") and m.unary_o(2.5) == (2.5, "float")
assert m.nary(1, 2, 3) == 6 and m.nary(0.5, 1, c=2) == 3.5
assert m.kw(1) == (1, 2, 3, [])
assert m.kw(1, 5, c=6, z=7, y=1.5) == (1, 5, 6, [("y", 1.5), ("z", 7)])
assert m.star(1, 2.5, k=3) == ((1, 2.5), [("k", 3)])
assert m.typed(3, 5.0, 2**40) == (6, 2.5, 2**40 + 1)
assert m.typed(i=3, d=1, big=-1) == (6, 0.5, 0)
assert m.fused_add(1, 2) == 3 and m.fused_add(1.5, 2.0) == 3.5
assert m.module_cpdef(3) == 3 and m.module_cpdef(x=1.5) == 1.5
assert functools.partial(m.nary, 1, 2)(3) == 6
assert weakref.ref(m.nary)() is m.nary

# A cdef class: tp_iternext, __getitem__, __richcmp__, __class_getitem__.
counter = m.Counter(5)
assert list(counter) == [0, 0.5, 2, 1.5, 4]
assert m.Counter(2).__next__() == 0
assert counter[1] == ("item", 1) and counter[2.5] == ("item", 2.5)
assert m.Counter(3) == m.Counter(3) and m.Counter(3) < m.Counter(4)
assert m.Counter[int] == ("class_getitem", "Counter", int)
assert counter.twice(4) == 13 and m.Sub(1).twice(4) == -4
assert counter.method(2, b=3) == 6 and m.Counter.method(counter, 2) == 3.0
assert counter.method_o(9.25) == 9.25 and m.Counter.make(7).n == 7
assert m.Counter.static(5, 2) == 3
assert pickle.loads(pickle.dumps(m.Counter(9))).n == 9
special = m.Special(1.5)
assert special.missing == ("getattr", "missing") and special + 2 == ("add", 2)
assert special(1, 2.5, k=3) == ((1, 2.5), [("k", 3)])
assert m.Special.one(4.5) == 4.5 and special.prop == 1.5
assert list(m.PyIter(2)) == [1.5, 0.5]

# Generators, coroutines, closures, lambdas, exceptions, comprehensions.
generator = m.gen(3)
assert next(generator) == 0 and generator.send(None) == 0.5
try:
    generator.throw(KeyError("x"))
except KeyError:
    pass
assert list(m.gen(2)) == [0, 0.5, 1, 1.5] == list(m.delegate(2))
assert asyncio.run(m.awaiter(1.5)) == 4.0
assert asyncio.run(m.consume(3)) == [0, 0.5, 1.0]
assert m.closure(3)(0.5) == 1.5 and m.use_lambda([1, 3, 2.5]) == [3, 2.5, 1]
assert m.catch(lambda a, b: a / b, 1, 0) == "zde"
assert m.catch(lambda d: d[1], {}) == "lookup"
ints, floats, d, s, total = m.comps(5)
assert ints == [0, 1, 4, 9, 16] and floats == [0.0, 0.25, 0.5, 0.75, 1.0]
assert d == dict(zip(ints, floats)) and s == set(floats) and total == 30
assert m.iterate(x for x in [1.5, 2]) == [1.5, 2]
assert m.call(max, 1, 4.5, 2) == 4.5 and m.call(m.unary_o, 3) == (3, "int")
assert m.call(m.Counter.method, counter, 2, b=0.5) == 1.0
assert m.builtins_use([3, 1.5, 2]) == (3, 3, 1.5, [2, 1.5, 3], (3, 1.5, 2), 3)
assert m.unpack([1, 2.5, 3, 4]) == (1, 2.5, [3, 4], 1, 2.5)
assert m.pops([1, 2.5, 3, 4.5, 5], 1) == (2.5, 5, [1, 3, 4.5], [3, 4.5], (3, 4.5))
assert m.match_seq((1, 2.5, 3)) == (1, [2.5, 3])

# Hold an instance of each type for the tracing verifier.
held = [counter, special, generator, m.coro(1), m.closure(2)]
held[3].close()
print("ok")
