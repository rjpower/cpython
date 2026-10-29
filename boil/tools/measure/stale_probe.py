#!/usr/bin/env python3
"""The stale-root probe (note 11 section 8.1, 10a-2.md section 4).

Usage: python3 boil/tools/measure/stale_probe.py PYTHON [RUNS]

Runs each case RUNS times (default 100) in one child of the interpreter
under test, each inside a sorted(key=...) callback so that list.sort's
opaque C frame lies between two eval-loop frames, and prints how many runs
freed the dead object at the first collection.  A precise collector frees
it every time.  The child asserts the parsed verifier mode against the
requested environment before testing roots.
"""

import os
import subprocess
import sys

CHILD = r'''
import contextvars, gc, sys, weakref, _testinternalcapi
expected_mode = int(sys.argv[2]), int(sys.argv[3])
assert _testinternalcapi.gc_verify_mode() == expected_mode, (
    _testinternalcapi.gc_verify_mode(), expected_mode)
runs = int(sys.argv[1])
class A:
    def f(self):
        return 1
class B(A):
    def f(self):
        return super().f()
refs = []
def dead_super():
    b = B(); b.self = b; b.f()
    ref = weakref.ref(b); del b
    gc.collect()
    return ref() is None
def dead_local():
    a = A(); a.self = a
    ref = weakref.ref(a); del a
    gc.collect()
    return ref() is None
def dead_item():
    items = [A()]
    ref = weakref.ref(items[0]); del items
    gc.collect()
    return ref() is None
class Callable:
    def __call__(self, x):
        refs.append(weakref.ref(x))
def dead_tp_call():
    Callable()(A())
    gc.collect()
    return refs.pop()() is None
def store(x):
    refs.append(weakref.ref(x))
def dead_context_run():
    contextvars.copy_context().run(store, A())
    gc.collect()
    return refs.pop()() is None
def receiver():
    while True:
        x = yield
        refs.append(weakref.ref(x))
        del x
def dead_generator_send():
    g = receiver(); next(g)
    g.send(A())
    gc.collect()
    return refs.pop()() is None
for func in (dead_super, dead_local, dead_item, dead_tp_call,
             dead_context_run, dead_generator_send):
    freed = []
    sorted(range(runs), key=lambda i: freed.append(func()) or i)
    print(f"{func.__name__} {freed.count(True)}/{runs}")
'''


def main(argv):
    if len(argv) < 2:
        print(__doc__, file=sys.stderr)
        return 2
    runs = argv[2] if len(argv) > 2 else "100"
    name, _, extra = os.environ.get("PYTHON_GCVERIFY", "").partition(":")
    mode = {"": 0, "retain": 1, "kill": 2}[name]
    extras = sum({"pymem": 1, "structs": 2}[part]
                 for part in set(extra.split(",")) if part)
    proc = subprocess.run([argv[1], "-c", CHILD, runs, str(mode),
                           str(extras)], text=True,
                          capture_output=True)
    sys.stdout.write(proc.stdout)
    sys.stderr.write(proc.stderr)
    if proc.returncode:
        return proc.returncode
    return 0 if all(line.split()[1] == f"{runs}/{runs}"
                    for line in proc.stdout.splitlines()) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
