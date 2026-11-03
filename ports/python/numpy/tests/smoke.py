"""Smoke test for NumPy: core arrays, lapack_lite, fft, random and the
buffer and callback paths that cross the C API."""

import importlib.metadata

import numpy as np

assert np.__version__ == "2.5.4", np.__version__
assert importlib.metadata.version("numpy") == "2.5.4"

a = np.arange(12, dtype=np.float64).reshape(3, 4)
b = a.T @ a
assert b.shape == (4, 4) and b[0, 0] == 0 + 16 + 64
assert np.allclose(np.matmul(a, np.ones(4)), a.sum(axis=1))

# No BLAS on the host: solve runs on the bundled lapack_lite.
m, rhs = np.array([[2.0, 1.0], [1.0, 3.0]]), np.array([3.0, 5.0])
assert np.allclose(m @ np.linalg.solve(m, rhs), rhs)

signal = np.sin(np.linspace(0, 8 * np.pi, 64))
assert np.allclose(np.fft.ifft(np.fft.fft(signal)).real, signal)

draws = np.random.default_rng(1234).normal(size=10_000)
assert abs(draws.mean()) < 0.05 and abs(draws.std() - 1) < 0.05

assert np.dtype(np.intp).itemsize == 8
assert np.dtype(np.longdouble).itemsize == 16  # x86-64 extended precision

try:
    np.arange(10).reshape(3, 4)
except ValueError:
    pass
else:
    raise AssertionError("an invalid reshape was accepted")

point = np.dtype([("x", "<i4"), ("name", "S3")])
records = np.array([(1, b"abc"), (2, b"de")], dtype=point)
assert records["x"].tolist() == [1, 2] and records["name"][1] == b"de"
assert records.dtype.itemsize == 7

# frompyfunc calls back into Python for every element.
square_plus_one = np.frompyfunc(lambda x: x * x + 1, 1, 1)
assert square_plus_one(np.arange(4)).tolist() == [1, 2, 5, 10]

# The array interface exposes the buffer that memoryview shares.
data = np.zeros(4, dtype=np.int32)
view = memoryview(data)
interface = data.__array_interface__
assert interface["typestr"] == "<i4" and interface["shape"] == (4,)
view[2] = 7
assert data[2] == 7 and interface["data"][0] == data.ctypes.data
