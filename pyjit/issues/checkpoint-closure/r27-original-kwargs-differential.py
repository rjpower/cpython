"""Compare original-kwargs call order across upstream and candidate builds."""
import ctypes
import json
import sys

c_call = ctypes.pythonapi.PyObject_Call
c_call.argtypes = (ctypes.py_object, ctypes.py_object, ctypes.py_object)
c_call.restype = ctypes.py_object
results = []
events = []


def record(name, operation):
    events.clear()
    try:
        value = operation()
        result = {"result": value}
    except Exception as exc:
        result = {"error": type(exc).__name__, "message": str(exc)}
    results.append({"case": name, "events": events.copy(), **result})


class CallDescriptor:
    def __get__(self, obj, owner):
        events.append("call-get")
        def call(*args, **kwargs):
            events.append("call-body")
            return "called"
        return call


class RaisingDescriptor:
    def __get__(self, obj, owner):
        events.append("raising-get")
        raise ValueError("descriptor failure")


class Callable:
    __call__ = CallDescriptor()


class RaisingCallable:
    __call__ = RaisingDescriptor()


class NewDescriptor:
    def __get__(self, obj, owner):
        events.append("new-get")
        def make(cls, *args, **kwargs):
            events.append("new-body")
            return object.__new__(cls)
        return make


class InitDescriptor:
    def __get__(self, obj, owner):
        events.append("init-get")
        def init(*args, **kwargs):
            events.append("init-body")
        return init


class NewClass:
    __new__ = NewDescriptor()


class InitClass:
    __init__ = InitDescriptor()


bad = {None: 1}
for label, args in (("zero", ()), ("one", (1,)), ("three", ("X", (), {}))):
    record("type-c-" + label, lambda a=args: c_call(type, a, bad))
record("callable-python-bad", lambda: Callable()(**bad))
record("callable-c-bad", lambda: c_call(Callable(), (), bad))
record("raising-python-bad", lambda: RaisingCallable()(**bad))
record("raising-c-bad", lambda: c_call(RaisingCallable(), (), bad))
inline_int, inline_float = 17, 1.5
record("inline-int-bad", lambda: inline_int(**bad))
record("inline-float-bad", lambda: inline_float(**bad))
record("new-descriptor-c-bad", lambda: c_call(NewClass, (), bad))
record("init-descriptor-c-bad", lambda: c_call(InitClass, (), bad))
record("callable-python-good", lambda: Callable()(answer=42))
record("callable-c-good", lambda: c_call(Callable(), (), {"answer": 42}))


class Recursive:
    def __call__(self, *args, **kwargs):
        return self(*args, **kwargs)


def recurse():
    old_limit = sys.getrecursionlimit()
    try:
        sys.setrecursionlimit(80)
        return c_call(Recursive(), (), {"x": 1})
    finally:
        sys.setrecursionlimit(old_limit)


record("recursion", recurse)
print(json.dumps(results, ensure_ascii=True, separators=(",", ":")))
