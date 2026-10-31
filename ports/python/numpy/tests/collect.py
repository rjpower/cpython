"""Collect often while NumPy works: casts between many dtypes, sorts,
reductions, StringDType, and casts to the object dtype whose new objects
only an iterator's buffer holds.  A missing traverse or a
dealloc that reads a freed referent shows up here as a crash or a wrong
result."""

import gc
import warnings

import numpy as np

warnings.simplefilter("ignore", np.exceptions.ComplexWarning)
gc.set_threshold(1 << 16)
types = [np.bool_, np.int8, np.uint16, np.int32, np.int64, np.float16,
         np.float32, np.float64, np.complex64, np.complex128, np.longdouble,
         object, "U5", "S3", "M8[s]", "m8[ms]", np.dtypes.StringDType()]
for _ in range(3):
    for t in types:
        a = np.arange(50).astype(t)
        for u in types:
            try:
                a.copy().astype(u)
            except (TypeError, ValueError, RuntimeError, OverflowError):
                pass
        np.sort(a)
        np.argsort(a)
    s = np.zeros(4, dtype=[("x", "f8"), ("y", "i4", (2,)), ("n", "U3")])
    s["n"] = ["a", "bb", "ccc", "d"]
    np.strings.lower(np.array(["AB"], dtype=np.dtypes.StringDType()))
    gc.collect()

gc.set_threshold(1 << 14)
a = np.arange(10**6, 10**6 + 200000)
for _ in range(3):
    # The ufunc fills a new object array that only the C stack holds.
    assert sum(np.add(a, 1, dtype=object).tolist()) == int(a.sum()) + len(a)
total = 0
for x in np.nditer(a, flags=["buffered", "refs_ok"], op_dtypes=["O"],
                   casting="unsafe", buffersize=8192):
    total += x[()]
assert total == int(a.sum())
