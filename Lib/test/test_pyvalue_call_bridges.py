"""Object C call APIs over canonical tuple and dictionary storage."""

import gc
import unittest
import weakref

import _testcapi


def record_call(*args, **kwargs):
    return args, kwargs


class StorageTuple(tuple):
    def __iter__(self):
        raise AssertionError("C argument views must use tuple storage")

    def __getitem__(self, index):
        raise AssertionError("C argument views must use tuple storage")


class CallBridgeReferenceTests(unittest.TestCase):
    """Controls using existing C API fixtures, also runnable on the host."""

    def test_mixed_tuple_views(self):
        marker = object()
        expected = (2147483647, 3.25, marker, 1 << 100, True, None)
        for arguments in (expected, StorageTuple(expected)):
            for invoke, keywords in ((_testcapi.pyobject_fastcalldict, None),
                                     (_testcapi.pyobject_vectorcall, None),
                                     (_testcapi.pyvectorcall_call, {})):
                with self.subTest(invoke=invoke, tuple_type=type(arguments)):
                    args, kwargs = invoke(record_call, arguments, keywords)
                    self.assertEqual(args, expected)
                    self.assertIs(args[2], marker)
                    self.assertEqual(kwargs, {})

    def test_empty_and_none_views(self):
        for arguments in (None, ()):
            for invoke in (_testcapi.pyobject_fastcalldict,
                           _testcapi.pyobject_vectorcall):
                self.assertEqual(invoke(record_call, arguments, None), ((), {}))
        self.assertEqual(_testcapi.pyvectorcall_call(record_call, (), {}),
                         ((), {}))

    def test_keyword_windows(self):
        marker = object()
        kwargs = {"integer": 2147483647, "float": 3.25, "owner": marker}
        expected = ((True,), kwargs)
        self.assertEqual(_testcapi.pyobject_fastcalldict(
            record_call, (True,), kwargs), expected)
        self.assertEqual(_testcapi.pyvectorcall_call(
            record_call, (True,), kwargs), expected)
        self.assertEqual(_testcapi.pyobject_vectorcall(
            record_call, (True, 2147483647, 3.25, marker),
            ("integer", "float", "owner")), expected)

    def test_invalid_keyword_keys(self):
        for key in (2147483647, 3.25, None, object()):
            for invoke in (_testcapi.pyobject_fastcalldict,
                           _testcapi.pyvectorcall_call):
                with self.subTest(key=key, invoke=invoke):
                    with self.assertRaisesRegex(TypeError,
                                                "keywords must be strings"):
                        invoke(record_call, (), {key: 1})

    def test_existing_fixture_validation(self):
        for invoke in (_testcapi.pyobject_fastcalldict,
                       _testcapi.pyobject_vectorcall):
            with self.assertRaisesRegex(TypeError, "args must be None or a tuple"):
                invoke(record_call, [], None)
        with self.assertRaisesRegex(TypeError, "None or a dict"):
            _testcapi.pyobject_fastcalldict(record_call, (2147483647,), [])
        with self.assertRaisesRegex(TypeError, "None or a tuple"):
            _testcapi.pyobject_vectorcall(record_call, (3.25,), [])
        with self.assertRaisesRegex(ValueError, "kwnames longer than args"):
            _testcapi.pyobject_vectorcall(record_call, (), ("value",))

    def test_public_vectorcall_ignores_have_flag(self):
        cls = _testcapi.make_vectorcall_class()
        instance = cls()
        instance.set_vectorcall(cls)
        self.assertEqual(instance(), "vectorcall")
        cls.__call__ = lambda self, *args, **kwargs: "ordinary"
        self.assertFalse(_testcapi.has_vectorcall_flag(cls))
        for args, kwargs in (((), {}), ((2147483647,), {"value": 3.25})):
            self.assertEqual(_testcapi.pyvectorcall_call(instance, args, kwargs),
                             "vectorcall")
            self.assertEqual(_testcapi.pyobject_fastcalldict(instance, args, kwargs),
                             "ordinary")
        self.assertEqual(_testcapi.pyobject_vectorcall(instance, (), None),
                         "ordinary")

    def test_ordinary_tp_call_fallback(self):
        class Callable:
            def __call__(self, *args, **kwargs):
                return args, kwargs

        instance = Callable()
        self.assertEqual(_testcapi.pyobject_fastcalldict(
            instance, (2147483647,), {"value": 3.25}),
            ((2147483647,), {"value": 3.25}))
        self.assertEqual(_testcapi.pyobject_vectorcall(
            instance, (2147483647, 3.25), ("value",)),
            ((2147483647,), {"value": 3.25}))

    def test_keyword_owner_survives_source_clear_and_result(self):
        class Value:
            pass

        for invoke in (_testcapi.pyobject_fastcalldict,
                       _testcapi.pyvectorcall_call):
            kwargs = {"value": Value()}
            reference = weakref.ref(kwargs["value"])
            calls = []

            def consume(*, value):
                calls.append("called")
                kwargs.clear()
                gc.collect()
                self.assertIs(reference(), value)
                return value

            result = invoke(consume, (), kwargs)
            self.assertIs(result, reference())
            self.assertEqual(calls, ["called"])
            self.assertEqual(kwargs, {})
            del result
            gc.collect()
            self.assertIsNone(reference())

    def test_call_error_identity_without_replay(self):
        for invoke in (_testcapi.pyobject_fastcalldict,
                       _testcapi.pyvectorcall_call):
            for error in (MemoryError("call memory"), RuntimeError("call primary")):
                calls = []

                def fail(*args, **kwargs):
                    calls.append((args, kwargs))
                    raise error

                with self.assertRaises(type(error)) as caught:
                    invoke(fail, (2147483647,), {"value": 3.25})
                self.assertIs(caught.exception, error)
                self.assertEqual(calls, [((2147483647,), {"value": 3.25})])


class CallBridgeCandidateTests(unittest.TestCase):
    """New direct API witnesses; unavailable host methods are not skipped."""

    def test_objargs_method_dispatch(self):
        class Receiver:
            def method(self, value):
                return self, value

            @classmethod
            def classmethod(cls, value):
                return cls, value

            @staticmethod
            def staticmethod(value):
                return value

            @property
            def property(self):
                return lambda value: ("property", value)

        receiver = Receiver()
        marker = object()
        receiver.instance = lambda value: ("instance", value)
        receiver.bound = receiver.method
        for name, expected in (("method", (receiver, marker)),
                               ("classmethod", (Receiver, marker)),
                               ("staticmethod", marker),
                               ("property", ("property", marker)),
                               ("instance", ("instance", marker)),
                               ("bound", (receiver, marker))):
            with self.subTest(name=name):
                self.assertEqual(_testcapi.callmethod_objargs(
                    receiver, name, marker), expected)
        values = []
        self.assertIsNone(_testcapi.callmethod_objargs(values, "append", marker))
        self.assertEqual(values, [marker])

    def test_noargs_method_dispatch(self):
        class Receiver:
            def method(self):
                return self

            @classmethod
            def classmethod(cls):
                return cls

            @staticmethod
            def staticmethod():
                return "static"

        receiver = Receiver()
        for name, expected in (("method", receiver), ("classmethod", Receiver),
                               ("staticmethod", "static")):
            self.assertEqual(_testcapi.method_noargs(receiver, name), expected)
        self.assertEqual(_testcapi.method_noargs(2147483647, "bit_length"), 31)
        self.assertIs(_testcapi.method_noargs(3.25, "is_integer"), False)

    def test_formatted_single_tuple_expansion(self):
        class Receiver:
            def method(self, *args):
                return self, args

            @classmethod
            def classmethod(cls, *args):
                return cls, args

            @staticmethod
            def staticmethod(*args):
                return args

        receiver = Receiver()
        marker = object()
        for expected in ((), (2147483647, 3.25, marker, True, None)):
            for args in (expected, StorageTuple(expected)):
                self.assertEqual(_testcapi.callfunction_format(record_call, args),
                                 (expected, {}))
                self.assertEqual(_testcapi.callmethod_format(
                    receiver, "method", args), (receiver, expected))
                self.assertEqual(_testcapi.callmethod_format(
                    receiver, "classmethod", args), (Receiver, expected))
                self.assertEqual(_testcapi.callmethod_format(
                    receiver, "staticmethod", args), expected)

    def test_inline_noncallable_attributes(self):
        class Receiver:
            pass

        receiver = Receiver()
        for value in (2147483647, 3.25, 1 << 100, True, None):
            for target in (Receiver, receiver):
                target.run = value
                for invoke in (lambda: _testcapi.callmethod_objargs(
                                   target, "run", 1),
                               lambda: _testcapi.method_noargs(target, "run"),
                               lambda: _testcapi.callmethod_format(
                                   target, "run", ())):
                    with self.subTest(value=value, target=target):
                        with self.assertRaises(TypeError):
                            invoke()

    def test_descriptor_error_identity_without_replay(self):
        for error in (MemoryError("lookup memory"), RuntimeError("lookup primary")):
            calls = []

            class Descriptor:
                def __get__(self, instance, owner):
                    calls.append("get")
                    raise error

            class Receiver:
                run = Descriptor()

            receiver = Receiver()
            for invoke in (lambda: _testcapi.callmethod_objargs(receiver, "run", 1),
                           lambda: _testcapi.method_noargs(receiver, "run"),
                           lambda: _testcapi.callmethod_format(receiver, "run", ())):
                with self.assertRaises(type(error)) as caught:
                    invoke()
                self.assertIs(caught.exception, error)
            self.assertEqual(calls, ["get"] * 3)

    def test_selected_method_and_result_survive_source_clear(self):
        class Receiver:
            pass

        for route in ("objargs", "noargs", "format"):
            receiver = Receiver()
            calls = []

            class Callable:
                def __call__(self, *args):
                    calls.append(args)
                    del receiver.run
                    gc.collect()
                    self_reference = reference()
                    if self_reference is not self:
                        raise AssertionError("selected method owner was lost")
                    return self

            receiver.run = Callable()
            reference = weakref.ref(receiver.run)
            if route == "objargs":
                result = _testcapi.callmethod_objargs(receiver, "run", 2147483647)
                expected = (2147483647,)
            elif route == "noargs":
                result = _testcapi.method_noargs(receiver, "run")
                expected = ()
            else:
                result = _testcapi.callmethod_format(receiver, "run", (3.25,))
                expected = (3.25,)
            self.assertIs(result, reference())
            self.assertEqual(calls, [expected])
            del result
            gc.collect()
            self.assertIsNone(reference())

    def test_formatted_call_error_identity(self):
        calls = []
        error = RuntimeError("formatted primary")

        def fail(*args):
            calls.append(args)
            raise error

        class Receiver:
            run = staticmethod(fail)

        for invoke in (lambda: _testcapi.callfunction_format(fail, (2147483647, 3.25)),
                       lambda: _testcapi.callmethod_format(
                           Receiver(), "run", (2147483647, 3.25))):
            with self.assertRaises(RuntimeError) as caught:
                invoke()
            self.assertIs(caught.exception, error)
        self.assertEqual(calls, [(2147483647, 3.25)] * 2)

    def test_new_fixture_validation(self):
        for invoke in (_testcapi.callmethod_objargs, _testcapi.method_noargs,
                       _testcapi.callmethod_format, _testcapi.callfunction_format):
            with self.assertRaises(TypeError):
                invoke()
        for invoke in (lambda: _testcapi.callmethod_objargs(object(), 1, None),
                       lambda: _testcapi.method_noargs(object(), 1),
                       lambda: _testcapi.callmethod_format(object(), 1, ())):
            with self.assertRaisesRegex(TypeError, "name must be str"):
                invoke()
        for invoke in (lambda: _testcapi.callmethod_format(object(), "run", []),
                       lambda: _testcapi.callfunction_format(record_call, [])):
            with self.assertRaisesRegex(TypeError, "args_tuple must be a tuple"):
                invoke()
        with self.assertRaises(UnicodeEncodeError):
            _testcapi.callmethod_format(object(), "\ud800", ())


if __name__ == "__main__":
    unittest.main()
