"""Hold an instance of each NumPy type that keeps references, so that the
tracing verifier of ports check sees them alive at a collection: arrays of
each kind of dtype, views, iterators, scalars and the flags object."""

import numpy as np
from numpy._core._multiarray_umath import _array_converter

a = np.arange(12.0).reshape(3, 4)
held = [
    np.array(["ab", "cde"], dtype="U5"),
    np.zeros(3, dtype=[("x", "f8"), ("n", "U3"), ("s", "i4", (2,))]),
    a[1:], a.T, a[[0, 2]],
    np.array(["x"], dtype=np.dtypes.StringDType()),
    np.array(["x", None], dtype=np.dtypes.StringDType(na_object=None)),
    np.dtype("f8", metadata={"unit": "m"}), np.dtype(("i4", (2, 3))),
    np.dtype(">f8"), np.datetime64("2020-01-01", "s"),
    np.arange("2020-01-01", "2020-01-04", dtype="M8[D]"),
    np.array([object(), [1]], dtype=object),
    np.array([(object(), [2])], dtype=[("o", "O"), ("s", "O", (1,))]),
    np.nditer([a, None]),
    np.nditer(np.arange(4), flags=["buffered"], op_dtypes=["f8"],
              casting="same_kind"),
    a.flat, np.broadcast(np.arange(3), np.ones((2, 1))), a.flags,
    np.void(b"ab"), np.zeros(1, dtype=[("a", "i4"), ("b", "O")])[0],
    np.rec.array([(1, 2.0)], dtype=[("a", "i4"), ("b", "f8")]),
    _array_converter(a, 1.0), np.ma.masked_array([1, 2], mask=[0, 1]),
    np.frombuffer(bytearray(16)), memoryview(np.arange(4)),
    np.busdaycalendar(holidays=["2020-01-01"]),
]
# A buffered cast to object fills the buffer with new ints that only the
# buffer holds.
cast = np.nditer(np.arange(10**6, 10**6 + 10), flags=["buffered", "refs_ok"],
                 op_dtypes=["O"], casting="unsafe")
held.append(next(cast))
held.append(cast)
assert all(item is not None for item in held)
