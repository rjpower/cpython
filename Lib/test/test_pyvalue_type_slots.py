import collections
import unittest


class SlotWrapperReferences(unittest.TestCase):
    def test_object_results_scalar_and_alias(self):
        self.assertEqual(int.__add__(2, 3), 5)
        self.assertEqual(int.__rsub__(3, 10), 7)
        self.assertEqual(int.__pow__(2, 5), 32)
        self.assertEqual(int.__pow__(2, 5, 7), 4)
        self.assertEqual(int.__rpow__(5, 2), 32)
        self.assertEqual(int.__rpow__(5, 2, 7), 4)
        self.assertIs(int.__add__(2, object()), NotImplemented)
        values = [1]
        self.assertIs(list.__iadd__(values, [2]), values)
        self.assertEqual(values, [1, 2])
        self.assertEqual(dict.__getitem__({3: 4.5}, 3), 4.5)
        self.assertTrue(int.__lt__(2, 3))
        self.assertIs(object.__eq__(object(), object()), NotImplemented)

    def test_arity_precedes_index_callback(self):
        events = []
        class Index:
            def __index__(self):
                events.append('index')
                return 2
        with self.assertRaisesRegex(TypeError, r'expected 1 argument, got 2'):
            str.__mul__('x', Index(), 99)
        with self.assertRaisesRegex(TypeError, r'expected 1 or 2 arguments, got 3'):
            int.__pow__(2, 3, 4, Index())
        self.assertEqual(events, [])

    def test_index_conversion_and_negative_sequence_order(self):
        events = []
        class Index:
            def __index__(self):
                events.append('index')
                return -1
        class Values(collections.deque):
            def __len__(self):
                events.append('length')
                return super().__len__()
        values = Values([10, 20])
        self.assertEqual(collections.deque.__getitem__(values, Index()), 20)
        self.assertEqual(events, ['index', 'length'])
        events.clear()
        self.assertIsNone(collections.deque.__delitem__(values, Index()))
        self.assertEqual(events, ['index', 'length'])
        self.assertEqual(list(values), [10])
        class Repeat:
            def __index__(self):
                events.append('repeat')
                return 3
        self.assertEqual(str.__mul__('x', Repeat()), 'xxx')
        self.assertEqual(events[-1], 'repeat')

    def test_index_error_identity_and_no_sequence_effect(self):
        sentinel = RuntimeError('selected index sentinel')
        events = []
        class Index:
            def __index__(self):
                events.append('index')
                raise sentinel
        values = collections.deque([10, 20])
        calls = [lambda: str.__mul__('x', Index()),
                 lambda: collections.deque.__getitem__(values, Index()),
                 lambda: collections.deque.__delitem__(values, Index())]
        for call in calls:
            events.clear()
            with self.assertRaises(RuntimeError) as caught:
                call()
            self.assertIs(caught.exception, sentinel)
            self.assertEqual(events, ['index'])
            self.assertEqual(list(values), [10, 20])

    def test_integer_status_and_predicate_paths(self):
        events = []
        class Item:
            def __eq__(self, other):
                events.append(other)
                return True
        self.assertIs(list.__contains__([Item()], 2.5), True)
        self.assertEqual(events, [2.5])
        values = {2: 'selected'}
        self.assertIsNone(dict.__delitem__(values, 2))
        self.assertEqual(values, {})
        class Value:
            pass
        target = Value()
        target.selected = 1
        self.assertIsNone(object.__delattr__(target, 'selected'))
        with self.assertRaises(AttributeError):
            object.__delattr__(target, 'selected')
        with self.assertRaises(TypeError):
            object.__delattr__(target, 1)
        # The hackcheck failure order remains the original native policy.
        with self.assertRaises(TypeError):
            object.__delattr__(int, 'selected')

    def test_native_callback_error_and_descriptor_delete_once(self):
        sentinel = RuntimeError('native callback sentinel')
        events = []
        class Item:
            def __eq__(self, other):
                events.append(('eq', other))
                raise sentinel
        with self.assertRaises(RuntimeError) as caught:
            list.__contains__([Item()], 123)
        self.assertIs(caught.exception, sentinel)
        self.assertEqual(events, [('eq', 123)])
        events.clear()
        def delete(value):
            events.append(value)
        descriptor = property(fdel=delete)
        self.assertIsNone(property.__delete__(descriptor, 4.5))
        self.assertEqual(events, [4.5])
        events.clear()
        def fail(value):
            events.append(value)
            raise sentinel
        descriptor = property(fdel=fail)
        with self.assertRaises(RuntimeError) as caught:
            property.__delete__(descriptor, 123)
        self.assertIs(caught.exception, sentinel)
        self.assertEqual(events, [123])


if __name__ == '__main__':
    unittest.main(verbosity=2)
