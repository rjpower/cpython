"""Exercise the object-result tp_new ABI independently of the value wrapper."""

import ctypes


get_slot = ctypes.pythonapi.PyType_GetSlot
get_slot.argtypes = (ctypes.py_object, ctypes.c_int)
get_slot.restype = ctypes.c_void_p
newfunc = ctypes.PYFUNCTYPE(ctypes.py_object, ctypes.py_object,
                            ctypes.py_object, ctypes.c_void_p)


def call_new(cls, arg):
    # Py_tp_new is slot 65; the last argument is a NULL keyword dictionary.
    return newfunc(get_slot(cls, 65))(cls, (arg,), None)


assert call_new(int, 17) == 17
assert call_new(float, 1.5) == 1.5
assert call_new(int, 2**80) == 2**80


class IntSubclass(int):
    pass


class OtherResult:
    def __new__(cls, arg):
        return arg

    def __init__(self, arg):
        raise AssertionError("tp_new must not initialize")


assert type(call_new(IntSubclass, 17)) is IntSubclass
assert call_new(OtherResult, 17) == 17
print("Object-result tp_new entries passed")
