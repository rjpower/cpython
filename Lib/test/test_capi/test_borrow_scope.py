"""Tests for the borrow stack behind the borrowed results of the PyArg_*
parsers (Include/internal/pycore_borrow.h)."""

import codecs
import pickle
import sys
import unittest
from test import support
from test.support import import_helper
from test.support.script_helper import assert_python_failure, assert_python_ok

_testinternalcapi = import_helper.import_module('_testinternalcapi')

borrow_depth = _testinternalcapi.borrow_depth
parse_then_churn = _testinternalcapi.parse_then_churn

# Exact ints and floats that tuples, lists and dicts store inline, so that
# parsing them with "O" allocates a box that only the borrow stack owns.
INLINE = (1, 2.5, 2**40, -7)


def tick():
    """Pass an instruction boundary in a new activation's entry, so the
    caller's pending release runs."""


class BorrowScopeTests(unittest.TestCase):

    def test_tuple_items_survive_churn(self):
        for args in [(1, 2.5), (2**40, "x"), (-7, 2**40)]:
            with self.subTest(args=args):
                delta, values = parse_then_churn("OO", args)
                self.assertEqual(delta, 2)
                self.assertEqual(values, args)

    def test_keywords_survive_churn(self):
        for fast in (False, True):
            with self.subTest(fast=fast):
                delta, values = _testinternalcapi.parse_kw_then_churn(
                    fast, (), {"x": 7, "y": 1.5})
                self.assertEqual(values, (7, 1.5))
                delta, values = _testinternalcapi.parse_kw_then_churn(
                    fast, (2**40,), {"y": -2.5})
                self.assertEqual(values, (2**40, -2.5))

    def test_release_at_call_boundaries(self):
        # call_loop() calls from C, where no eval-loop instruction boundary
        # runs, so only the call boundaries' scopes release: the PyCFunction
        # trampoline, type_call() around a Clinic tp_new that parses its
        # tuple, and _PyObject_VectorcallTstate() around a vectorcall
        # function that has no trampoline of its own.
        cases = [
            (_testinternalcapi.varargs_OO, (1, 2.5)),
            (F, (2.5,)),
            (_testinternalcapi.ParseOnAdd((2**40,)), ()),
        ]
        for callable, args in cases:
            with self.subTest(callable=callable):
                self.assertEqual(
                    _testinternalcapi.call_loop(callable, args, 1000), 0)

    def test_loop_in_c(self):
        n = 1000
        pairs = [(i, i + 0.5) for i in range(n)]
        before = borrow_depth()
        self.assertEqual(_testinternalcapi.parse_pairs(pairs, False), 2 * n)
        self.assertEqual(borrow_depth(), before)
        self.assertEqual(_testinternalcapi.parse_pairs(pairs, True), 0)
        self.assertEqual(borrow_depth(), before)

    def check_slot_release(self, callback):
        t = _testinternalcapi.ParseOnAdd((2**40,), callback)
        before = borrow_depth()
        for _ in range(10**5):
            self.assertEqual(t + 1, 2**40)
        tick()
        self.assertEqual(borrow_depth(), before)

    def test_slot_entries_released(self):
        # nb_add parses without a native call boundary; the eval loop
        # releases the entry at the next instruction boundary.
        self.check_slot_release(None)

    def test_slot_entries_released_after_nested_activation(self):
        # The callback's activation must re-arm the release that its own
        # instruction boundaries consumed.
        self.check_slot_release(tick)

    def test_nested_sequence(self):
        for arg in ((1, 2.5), (2**40, -0.5)):
            with self.subTest(arg=arg):
                delta, values = parse_then_churn("(OO)", (arg,))
                self.assertEqual(values, arg)
        # A list is copied to a temporary tuple; the values outlive it.
        for arg in ([1, 2.5], [2**40, -0.5]):
            with self.subTest(arg=arg):
                with self.assertWarns(DeprecationWarning):
                    delta, values = parse_then_churn("(OO)", (arg,))
                self.assertEqual(values, tuple(arg))

    def test_reentrant_converter(self):
        inner = []

        def callback():
            before = borrow_depth()
            inner.append(parse_then_churn("OO", (3, 4.5)))
            inner.append(borrow_depth() - before)
            return "done"

        a, b, depth_before, depth_after, result = (
            _testinternalcapi.parse_reentrant(2**40, callback, 0.75))
        self.assertEqual((a, b), (2**40, 0.75))
        self.assertEqual(result, "done")
        self.assertEqual(depth_after, depth_before)
        self.assertEqual(inner, [(2, (3, 4.5)), 0])

    def test_generator_resumed_from_c(self):
        def gen():
            for i in range(100):
                parse_then_churn("OO", (i, i + 0.5))
                yield borrow_depth()

        depths = list(gen())
        self.assertEqual(len(set(depths)), 1, depths)

    def test_error_after_boxing(self):
        before = borrow_depth()
        with self.assertRaisesRegex(
                TypeError,
                "^'str' object cannot be interpreted as an integer$"):
            _testinternalcapi.parse_Oi(1, "x")
        self.assertEqual(borrow_depth(), before)

    def test_wrong_arity_pushes_nothing(self):
        delta, exc = parse_then_churn("OO", tuple(range(10**6)))
        self.assertIsInstance(exc, TypeError)
        self.assertEqual(delta, 0)
        delta, exc = parse_then_churn("OO", (1,))
        self.assertIsInstance(exc, TypeError)
        self.assertEqual(delta, 0)

    def test_chunk_growth_out_of_memory(self):
        # A run longer than a chunk needs a new chunk; when that fails the
        # push raises MemoryError and leaves nothing on the stack.
        delta, exc = _testinternalcapi.borrow_grow_oom(("x",) * 5000)
        self.assertIsInstance(exc, MemoryError)
        self.assertEqual(delta, 0)
        self.assertEqual(parse_then_churn("OO", (2**40, 0.5)),
                         (2, (2**40, 0.5)))

    def test_eval_frame_hook(self):
        def loop():
            total = 0
            for i in range(10**4):
                total += i
            return total

        record = []
        _testinternalcapi.set_eval_frame_parse((2**40,), record)
        try:
            loop()
        finally:
            _testinternalcapi.set_eval_frame_parse()
        self.assertGreaterEqual(len(record), 1)
        self.assertEqual(set(record), {2**40})

    @support.requires_refcounting
    def test_native_call_arguments_borrowed(self):
        # Native calls from bytecode pass the objects of the call window
        # borrowed, as upstream passes its stack, and box only inline values:
        # a temporary argument has one reference.  The loop covers the cold
        # call path and the specialized METH_O and METH_FASTCALL paths.
        refcounts = _testinternalcapi.argument_refcounts

        def call():
            return sys.getrefcount(object()), refcounts(object(), [], 2.5)

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD + 3):
            self.assertEqual(call(), (1, [1, 1, 1]))

    def test_codec_error_handler_scope(self):
        # A codec calls the error handler once per error and parses each
        # result; the entries of one call must not stay for the next.
        depths = []

        def handler(exc):
            depths.append(borrow_depth())
            return ('?', exc.end)

        codecs.register_error('test_borrow_scope.depth', handler)
        # Codecs pass a run of bad characters to one handler call, so the
        # bad characters are kept apart.
        for data, op in [(b'\xffa' * 50, 'decode'), ('\xe9a' * 50, 'encode')]:
            with self.subTest(op=op):
                depths.clear()
                getattr(data, op)('ascii', 'test_borrow_scope.depth')
                self.assertEqual(len(depths), 50)
                self.assertEqual(len(set(depths)), 1, depths)

    def test_pickle_newobj_scope(self):
        # load_newobj calls tp_new directly; float's tp_new parses its
        # argument tuple, so each NEWOBJ pushes one entry unless load_newobj
        # gives the call its own scope.
        n = 10**5
        payload = [Probe()] + [F(i + 0.5) for i in range(n)] + [Probe()]
        for proto in range(2, pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                data = pickle.dumps(payload, proto)
                del probe_depths[:]
                loaded = pickle.loads(data)
                self.assertEqual(len(loaded), n + 2)
                self.assertEqual(loaded[1], 0.5)
                self.assertEqual(len(probe_depths), 2)
                self.assertEqual(probe_depths[1] - probe_depths[0], 0)


class F(float):
    pass


class FinalizationTests(unittest.TestCase):

    def test_late_finalizer_parses(self):
        # interpreter_clear() clears every thread state, then runs the
        # finalizers of objects kept alive until its last collection.
        assert_python_ok('-c', '''if 1:
            from test import support
            import _testinternalcapi

            class Late:
                def __del__(self, parse=_testinternalcapi.varargs_OO):
                    parse(2**40, 0.5)

            support.late_deletion(Late())
        ''')

    def test_daemon_thread_keeps_its_entries(self):
        # A daemon thread still inside a call when the main thread
        # finalizes keeps the boxes it parsed, as it keeps its frames.
        rc, out, err = assert_python_ok('-c', '''if 1:
            import threading, time
            import _testinternalcapi

            ready = threading.Event()
            def run():
                ready.set()
                _testinternalcapi.borrow_hold_and_wait((2**40,), 2.0)
            threading.Thread(target=run, daemon=True).start()
            ready.wait()
            time.sleep(0.2)
        ''')
        self.assertNotIn(b'freed under its thread', err)


probe_depths = []


def record_depth():
    probe_depths.append(borrow_depth())
    return Probe()


class Probe:
    def __reduce__(self):
        return (record_depth, ())


@unittest.skipUnless(support.Py_DEBUG, 'requires a debug build')
class DebugChecksTests(unittest.TestCase):

    def test_bad_exit_is_fatal(self):
        rc, out, err = assert_python_failure('-c', '''if 1:
            import _testinternalcapi
            _testinternalcapi.borrow_bad_exit()
        ''')
        self.assertIn(b'Fatal Python error: _PyBorrowScope_Release: '
                      b'borrow scope mark', err)
        self.assertIn(b'is not on the borrow stack', err)

    def test_scope_size_warning(self):
        rc, out, err = assert_python_ok('-c', '''if 1:
            import _testinternalcapi
            pairs = [(1, 2.5)] * 6000
            for _ in range(2):
                assert _testinternalcapi.parse_pairs(pairs, False) == 12000
            assert _testinternalcapi.parse_pairs(pairs, True) == 0
        ''')
        self.assertEqual(err.count(b'Warning: a borrow scope holds'), 1, err)
        self.assertIn(b'is in an unbounded loop without its own scope', err)


if __name__ == "__main__":
    unittest.main()
