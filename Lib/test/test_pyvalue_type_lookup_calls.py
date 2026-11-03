import contextlib
import io
import sys
import unittest


class TypeLookupCallTests(unittest.TestCase):
    def test_strict_special_method_calls_and_descriptor_binding(self):
        class ScalarMethods:
            __str__ = 1000
            __repr__ = 2**200
            __call__ = 0
            __len__ = 0.0
            __getitem__ = 1000
            def __init__(self, value):
                pass
        for operation in (str, repr, lambda obj: obj(), len,
                          lambda obj: obj[0]):
            with self.subTest(operation=operation), self.assertRaises(TypeError):
                operation(ScalarMethods(None))
        class BadInit:
            __init__ = 1000
        with self.assertRaisesRegex(TypeError, 'not callable'):
            BadInit()

        for scalar in (0, 1000, 2**200, 0.0):
            with self.subTest(scalar=type(scalar).__name__):
                with self.assertRaises(TypeError):
                    scalar()

        events = []
        sentinel = object()
        class Callable:
            def __call__(self, *args, **kwargs):
                events.append(('call', args, kwargs))
                return sentinel
        class Descriptor:
            def __get__(self, receiver, owner):
                events.append(('get', receiver, owner))
                return Callable()
        class Receiver:
            __call__ = Descriptor()
        receiver = Receiver()
        self.assertIs(receiver(3, key=4), sentinel)
        self.assertEqual([event[0] for event in events], ['get', 'call'])
        self.assertIs(events[0][1], receiver)
        self.assertIs(events[0][2], Receiver)
        self.assertEqual(events[1][1:], ((3,), {'key': 4}))
        with self.assertRaisesRegex(TypeError, 'descriptor'):
            list.append(None, 1)
        self.assertEqual(list.append([], 1), None)
        bound = [].append
        self.assertIsNone(bound(2))

    def test_maybe_arithmetic_and_none_policies(self):
        calls = []
        class Left:
            def __add__(self, other):
                calls.append(('add', other))
                return NotImplemented
        class Right:
            def __radd__(self, other):
                calls.append(('radd', other))
                return result
        result = object()
        left, right = Left(), Right()
        self.assertIs(left + right, result)
        self.assertEqual(calls, [('add', right), ('radd', left)])

        class PowLeft:
            def __pow__(self, other, modulo=None):
                calls.append(('pow', other, modulo))
                return NotImplemented
        class PowRight:
            def __rpow__(self, other, modulo=None):
                calls.append(('rpow', other, modulo))
                return result
        pow_left, pow_right = PowLeft(), PowRight()
        self.assertIs(pow(pow_left, pow_right), result)
        self.assertEqual(calls[-2:], [('pow', pow_right, None), ('rpow', pow_left, None)])

        marker = LookupError('lookup marker')
        class BrokenAdd:
            @property
            def __add__(self):
                raise marker
        with self.assertRaises(LookupError) as caught:
            BrokenAdd() + 1
        self.assertIs(caught.exception, marker)

        class NoneMethods:
            __iter__ = None
            __bool__ = None
            __hash__ = None
            __contains__ = None
        value = NoneMethods()
        with self.assertRaises(TypeError):
            iter(value)
        with self.assertRaises(TypeError):
            bool(value)
        with self.assertRaises(TypeError):
            hash(value)
        with self.assertRaises(TypeError):
            1 in value

        class MissingIter:
            pass
        with self.assertRaises(TypeError):
            iter(MissingIter())

    def test_presence_fallback_and_lookup_errors(self):
        class PresentScalarGetitem:
            __getitem__ = 7
        iterator = iter(PresentScalarGetitem())
        with self.assertRaisesRegex(TypeError, "'int' object is not callable"):
            next(iterator)

        class MissingGetitem:
            pass
        with self.assertRaisesRegex(TypeError, 'not iterable'):
            iter(MissingGetitem())

        class AttributeMissingGetitem:
            @property
            def __getitem__(self):
                raise AttributeError('descriptor says absent')
        with self.assertRaisesRegex(AttributeError, 'descriptor says absent'):
            next(iter(AttributeMissingGetitem()))

        marker = LookupError('getitem marker')
        class BrokenGetitem:
            @property
            def __getitem__(self):
                raise marker
        with self.assertRaises(LookupError) as caught:
            next(iter(BrokenGetitem()))
        self.assertIs(caught.exception, marker)

        class NoneGetitem:
            __getitem__ = None
        with self.assertRaisesRegex(TypeError, "'NoneType' object is not callable"):
            next(iter(NoneGetitem()))

    def test_finalizer_unraisable_reporting_and_restoration(self):
        previous_hook = sys.unraisablehook
        records = []
        sys.unraisablehook = records.append
        try:
            class BrokenFinalizer:
                __del__ = 1000
            doomed = BrokenFinalizer()
            del doomed
            self.assertEqual(len(records), 1)
            self.assertIsInstance(records[0].exc_value, TypeError)
            self.assertRegex(str(records[0].exc_value), 'not callable')

            calls = []
            class GoodFinalizer:
                def __del__(self):
                    calls.append('called')
            obj = GoodFinalizer()
            del obj
            self.assertEqual(calls, ['called'])

            hook_calls = []
            def broken_hook(unraisable):
                hook_calls.append(unraisable.exc_value)
                raise RuntimeError('hook marker')
            sys.unraisablehook = broken_hook
            with contextlib.redirect_stderr(io.StringIO()):
                another = BrokenFinalizer()
                del another
            self.assertEqual(len(hook_calls), 1)
            self.assertIsInstance(hook_calls[0], TypeError)
        finally:
            sys.unraisablehook = previous_hook

    def test_presence_fallback_via_slot_tp_iter(self):
        class IterSlotBase:
            iter_reads = 0
            @property
            def __iter__(self):
                type(self).iter_reads += 1
                raise AttributeError('selected iter slot is maybe-missing')

        class ScalarGetitem(IterSlotBase):
            __getitem__ = 7
        iterator = iter(ScalarGetitem())
        with self.assertRaisesRegex(TypeError, "'int' object is not callable"):
            next(iterator)

        class MissingGetitem(IterSlotBase):
            pass
        with self.assertRaisesRegex(TypeError, 'not iterable'):
            iter(MissingGetitem())

        class AttributeGetitem(IterSlotBase):
            reads = 0
            @property
            def __getitem__(self):
                type(self).reads += 1
                raise AttributeError('getitem maybe-missing')
        with self.assertRaisesRegex(TypeError, 'not iterable'):
            iter(AttributeGetitem())
        self.assertEqual(AttributeGetitem.reads, 1)

        class NoneGetitem(IterSlotBase):
            __getitem__ = None
        iterator = iter(NoneGetitem())
        with self.assertRaisesRegex(TypeError, "'NoneType' object is not callable"):
            next(iterator)

    def test_candidate_presence_lookup_error_stops_fallback(self):
        class IterSlotBase:
            @property
            def __iter__(self):
                raise AttributeError('selected iter slot is maybe-missing')
        marker = LookupError('selected getitem lookup error')
        class BrokenGetitem(IterSlotBase):
            reads = 0
            @property
            def __getitem__(self):
                type(self).reads += 1
                raise marker
        with self.assertRaises(LookupError) as caught:
            iter(BrokenGetitem())
        self.assertIs(caught.exception, marker)
        self.assertEqual(BrokenGetitem.reads, 1)

    def test_candidate_finalizer_descriptor_lookup_error_reported(self):
        previous_hook = sys.unraisablehook
        records = []
        marker = LookupError('finalizer descriptor lookup marker')
        lookups = []
        class BrokenDescriptor:
            def __get__(self, receiver, owner):
                lookups.append((receiver, owner))
                raise marker
        class Host:
            __del__ = BrokenDescriptor()
        try:
            sys.unraisablehook = records.append
            instance = Host()
            del instance
            self.assertEqual(len(lookups), 1)
            self.assertEqual(len(records), 1)
            self.assertIs(records[0].exc_value, marker)
        finally:
            sys.unraisablehook = previous_hook

    def test_receiver_mutation_root_drop_and_result_lifetime(self):
        events = []
        result = object()
        roots = {}
        class Callable:
            def __call__(self):
                events.append(('call', type(receiver)))
                return result
            def __del__(self):
                events.append(('callable-finalized',))
        class Descriptor:
            def __get__(self, obj, owner):
                events.append(('get', owner))
                roots.clear()
                obj.__class__ = NewReceiver
                return Callable()
        class OldReceiver:
            __call__ = Descriptor()
        class NewReceiver:
            pass
        receiver = OldReceiver()
        roots['receiver'] = receiver
        value = receiver()
        self.assertIs(value, result)
        self.assertIs(type(receiver), NewReceiver)
        self.assertIs(events[0][1], OldReceiver)
        self.assertEqual([event[0] for event in events],
                         ['get', 'call', 'callable-finalized'])


if __name__ == '__main__':
    unittest.main()
