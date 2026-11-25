"""The stores into a heap slot release the value they displace exactly once.

_STORE_ATTR_SLOT, _STORE_ATTR_INSTANCE_VALUE, _STORE_ATTR_SHAPE,
_STORE_ATTR_SHAPE_ADD, _STORE_ATTR_WITH_HINT and _SetItem.ListIndex move
the stored value into the slot (pycore_object.h's _PyValueSlot_Exchange,
_PyValueSlot_ExchangeFilled and _PyValueSlot_Fill) and close the value that
the slot held.  Each case warms a store until its specialized handler runs,
then stores objects that a weak reference watches.  The script runs with
PYTHON_JIT off and always.
"""

import textwrap
import unittest

from test.support import import_helper
from test.support.script_helper import assert_python_ok

import_helper.import_module("_testinternalcapi")

# Each case asserts that its handler is among the words that get_ir reports
# for the warmed function.
SCRIPT = """
import _testinternalcapi as capi
import gc
import sys
import weakref

WARM = 400


class Value:
    "An object whose release a weak reference sees"
    released = []

    def __init__(self, n):
        self.n = n
        weakref.finalize(self, Value.released.append, n)


class Plain:
    def __init__(self):
        self.x = None


class Slotted:
    __slots__ = ("x",)

    def __init__(self):
        self.x = None


class Empty:
    pass


def handlers(func):
    return {w[0] for w in capi.get_ir(func.__code__, True)}


def check_released(handlers_of, func, targets):
    "func(n) stores a new Value, which only the slot owns, into targets[n % len(targets)]"
    Value.released.clear()
    for n in range(WARM):
        func(n)
    assert handlers_of & handlers(func), (handlers_of, sorted(handlers(func)))
    gc.collect()
    # Every store displaced one value, once; the last store of each target stays
    assert sorted(Value.released) == list(range(WARM - len(targets))), (
        handlers_of, len(Value.released), Value.released[:5])


def check_held_elsewhere(handlers_of, func, targets):
    "A value that a list also holds loses exactly the slot's count when displaced"
    keep = [Value(1000 + n) for n in range(WARM)]
    for n, value in enumerate(keep):
        func(targets[n % len(targets)], value)
    assert handlers_of & handlers(func), (handlers_of, sorted(handlers(func)))
    for n, value in enumerate(keep):
        # the list, the loop's name and the argument, and the slot for the last
        expected = 4 if n >= WARM - len(targets) else 3
        assert sys.getrefcount(value) == expected, (handlers_of, n, sys.getrefcount(value))
    assert not any(n >= 1000 for n in Value.released), Value.released


# The warm stores must reach their handler: the cases name which ones.  A
# store is compiled with its function, so each case defines its own.
def slot_case():
    targets = [Slotted(), Slotted()]
    def f(n):
        targets[n % 2].x = Value(n)
    check_released({"STORE_ATTR_SLOT"}, f, targets)
    def g(target, value):
        target.x = value
    check_held_elsewhere({"STORE_ATTR_SLOT"}, g, targets)


def shape_case():
    targets = [Plain(), Plain()]
    def f(n):
        targets[n % 2].x = Value(n)
    check_released({"STORE_ATTR_SHAPE"}, f, targets)
    def g(target, value):
        target.x = value
    check_held_elsewhere({"STORE_ATTR_SHAPE"}, g, targets)


def instance_value_case():
    # Instances of one class with different shapes: the shape guard cannot
    # hold, so the store goes through the instance's values
    targets = []
    for i in range(8):
        o = Empty()
        if i % 2:
            o.a = 1
        o.x = None
        targets.append(o)
    def f(n):
        targets[n % 8].x = Value(n)
    check_released({"STORE_ATTR_INSTANCE_VALUE"}, f, targets)
    def g(target, value):
        target.x = value
    check_held_elsewhere({"STORE_ATTR_INSTANCE_VALUE"}, g, targets)


def add_case():
    # The slot holds NULL: nothing is displaced, and the added values are
    # released with their object
    Value.released.clear()
    def f(n):
        o = Empty()
        o.x = Value(n)
        o.y = Value(n + 10_000)
    for n in range(WARM):
        f(n)
    gc.collect()
    assert {"STORE_ATTR_SHAPE_ADD"} & handlers(f), sorted(handlers(f))
    assert sorted(Value.released) == [*range(WARM), *range(10_000, 10_000 + WARM)]


def hint_case():
    # An instance whose dict was materialized
    targets = [Plain(), Plain()]
    for o in targets:
        o.__dict__
    def f(n):
        targets[n % 2].x = Value(n)
    check_released({"STORE_ATTR_WITH_HINT"}, f, targets)
    def g(target, value):
        target.x = value
    check_held_elsewhere({"STORE_ATTR_WITH_HINT"}, g, targets)


def list_case():
    targets = [[None, None], [None, None]]
    def f(n):
        targets[n % 2][1] = Value(n)
    check_released({"SetItem.ListIndex"}, f, targets)
    def g(target, value):
        target[1] = value
    check_held_elsewhere({"SetItem.ListIndex"}, g, targets)


for case in (slot_case, shape_case, instance_value_case, add_case, hint_case, list_case):
    case()
print("ok")
"""


class SlotStoreTests(unittest.TestCase):
    def check(self, mode):
        _, out, _ = assert_python_ok("-c", textwrap.dedent(SCRIPT), PYTHON_JIT=mode)
        self.assertEqual(out.strip(), b"ok")

    def test_jit_off(self):
        self.check("off")

    def test_jit_always(self):
        self.check("always")


if __name__ == "__main__":
    unittest.main()
