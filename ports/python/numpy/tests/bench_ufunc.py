"""Many ufunc calls on 10-element arrays: the time is spent crossing the
C API (argument parsing, dispatch, allocation), not in BLAS or SIMD loops."""

import numpy as np

ROUNDS = 350_000  # fixed, so runs on different interpreters do equal work

x = np.arange(10, dtype=np.float64)
y = np.linspace(1.0, 2.0, 10)
out = np.empty(10)

total = 0.0
for _ in range(ROUNDS):
    z = np.add(x, y)
    z = np.multiply(z, y)
    np.subtract(z, x, out=out)
    np.sqrt(out, out=out)
    np.maximum(out, x, out=out)
    z = np.greater(out, y)
    total += out[3] + np.add.reduce(out) + z.sum()
assert total > 0
