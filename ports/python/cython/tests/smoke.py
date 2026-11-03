"""Cython translates a module, and the generated C roots the code cache."""

import os
import tempfile

import Cython
from Cython.Compiler import Main

assert Cython.__version__ == "3.3.0", Cython.__version__

SOURCE = """
def fail(n):
    if n:
        raise ValueError(n)
    return sum(i * i for i in range(10))
"""

with tempfile.TemporaryDirectory() as tmp:
    pyx = os.path.join(tmp, "mod.pyx")
    with open(pyx, "w") as f:
        f.write(SOURCE)
    result = Main.compile(pyx, language_level=3)
    assert result.num_errors == 0, result.num_errors
    with open(os.path.join(tmp, "mod.c")) as f:
        c = f.read()

assert "__pyx_insert_code_object" in c
# The code object cache's dict of code objects is a collector root.
assert "PyGC_AddRoot(&code_cache->roots)" in c
print("ok")
