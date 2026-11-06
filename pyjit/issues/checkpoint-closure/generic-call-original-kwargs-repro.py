import ctypes

call = ctypes.pythonapi.PyObject_Call
call.argtypes = (ctypes.py_object, ctypes.py_object, ctypes.py_object)
call.restype = ctypes.py_object

events = []
class Descriptor:
    def __get__(self, obj, owner):
        events.append('descriptor')
        def body(*args, **kwargs):
            events.append('body')
            return None
        return body
class Callable:
    __call__ = Descriptor()
try:
    call(Callable(), (), {1: 2})
except Exception as exc:
    print(type(exc).__name__, str(exc))
print(events)
