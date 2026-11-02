import unittest

try:
    import _testinternalcapi as _capi
except ImportError:
    _capi = None


@unittest.skipIf(_capi is None, "requires the restored interpreter fixture")
class FlatFormatTests(unittest.TestCase):
    def test_views_survive_callback_and_optional_default(self):
        class Text(str):
            pass

        calls = []

        def callback():
            calls.append("callback")
            return None

        default_source = [2 ** 100, Text("leased text")]
        default_args = tuple(default_source)
        default_source.clear()
        self.assertEqual(_capi.flat_format_views((default_args, callback)),
                         (2 ** 100, "leased text", -7))

        external = [2 ** 100, Text("leased text")]

        class Index:
            def __init__(self, references):
                self.references = references

            def __index__(self):
                self.references.clear()
                calls.append("index")
                return 19

        index = Index(external)
        parse_args = (external[0], external[1], index)
        del external
        self.assertEqual(_capi.flat_format_views((parse_args, callback)),
                         (2 ** 100, "leased text", 19))
        self.assertEqual(index.references, [])
        self.assertEqual(calls, ["callback", "index", "callback"])

        class ConverterError(Exception):
            pass

        converter_error = ConverterError("index failure")

        class RaisingIndex:
            def __index__(self):
                raise converter_error

        calls_before = calls.copy()
        with self.assertRaises(ConverterError) as caught:
            _capi.flat_format_views(((2 ** 100, Text("x"), RaisingIndex()), callback))
        self.assertIs(caught.exception, converter_error)
        self.assertEqual(calls, calls_before)

        class CallbackError(Exception):
            pass

        callback_error = CallbackError("callback failure")
        callback_calls = []

        def failing_callback():
            callback_calls.append(None)
            raise callback_error

        with self.assertRaises(CallbackError) as caught:
            _capi.flat_format_views(((2 ** 100, Text("x")), failing_callback))
        self.assertIs(caught.exception, callback_error)
        self.assertEqual(len(callback_calls), 1)

        with self.assertRaises(ValueError):
            _capi.flat_format_views(((2 ** 100, "a\0b"), callback))
        self.assertEqual(calls, ["callback", "index", "callback"])
        with self.assertRaises(TypeError):
            _capi.flat_format_views(((2 ** 100, Text("x"), 1, 2), callback))
        self.assertEqual(calls, ["callback", "index", "callback"])

    def test_unsupported_formats_are_rejected_before_varargs(self):
        errors = _capi.flat_format_rejects_unsupported()
        self.assertEqual(len(errors), 8)
        self.assertTrue(all("unsupported scoped argument format" in e
                            for e in errors))

    def test_typed_nullable(self):
        calls = []
        callback = lambda: calls.append(None)
        large = 2 ** 100

        class IntSubclass(int):
            pass

        self.assertEqual(
            _capi.flat_format_typed_nullable(((large, None), callback)),
            (large, None))
        self.assertEqual(
            _capi.flat_format_typed_nullable(((IntSubclass(8), "text"), callback)),
            (8, "text"))
        calls_before_bool = len(calls)
        self.assertEqual(
            _capi.flat_format_typed_nullable(((True, "bool text"), callback)),
            (True, "bool text"))
        self.assertEqual(len(calls), calls_before_bool + 1)
        with self.assertRaises(ValueError):
            _capi.flat_format_typed_nullable(((True, "a\0b"), callback))
        self.assertEqual(len(calls), 3)
        with self.assertRaises(TypeError):
            _capi.flat_format_typed_nullable(((1.0, "x"), callback))
        self.assertEqual(len(calls), 3)
        with self.assertRaises(TypeError):
            _capi.flat_format_typed_nullable(((1, "x", "extra"), callback))
        self.assertEqual(len(calls), 3)


if __name__ == "__main__":
    unittest.main()
