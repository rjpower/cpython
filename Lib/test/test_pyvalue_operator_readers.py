import functools
import gc
import operator
import pickle
import unittest
import weakref


def legacy_call(functor, target):
    return type(functor).__call__(functor, target)


class OperatorLRUReferenceTests(unittest.TestCase):
    def test_itemgetter_generic_scalars_both_dispatches(self):
        for invoke in [lambda f, value: f(value), legacy_call]:
            self.assertEqual(invoke(operator.itemgetter(0), (123, 4.5)), 123)
            self.assertEqual(invoke(operator.itemgetter(-1), (123, 4.5)), 4.5)
            self.assertEqual(invoke(operator.itemgetter(0, 1), (123, 4.5)), (123, 4.5))
            events = []
            sentinel = RuntimeError('selected getitem sentinel')
            class Target:
                def __getitem__(self, key):
                    events.append(key)
                    if key == 2:
                        raise sentinel
                    return key + 0.5
            with self.assertRaises(RuntimeError) as caught:
                invoke(operator.itemgetter(1, 2, 3), Target())
            self.assertIs(caught.exception, sentinel)
            self.assertEqual(events, [1, 2])
        with self.assertRaises(TypeError):
            operator.itemgetter()
        functor = operator.itemgetter(0)
        with self.assertRaises(TypeError):
            type(functor).__call__(functor)

    def test_attrgetter_metadata_constructor_and_intermediate_retirement(self):
        events = []
        class Middle:
            @property
            def value(self):
                events.append('value')
                return 4.5
            def __del__(self):
                events.append('retire')
        class Root:
            @property
            def middle(self):
                events.append('middle')
                return Middle()
        for invoke in [lambda f, value: f(value), legacy_call]:
            events.clear()
            self.assertEqual(invoke(operator.attrgetter('middle.value'), Root()), 4.5)
            self.assertEqual(events, ['middle', 'value', 'retire'])
        class Nested:
            pass
        root = Nested(); root.a = Nested(); root.a.b = Nested(); root.a.b.c = 123
        self.assertEqual(operator.attrgetter('a.b.c')(root), 123)
        self.assertEqual(operator.attrgetter('a.b.c', 'a.b')(root), (123, root.a.b))
        with self.assertRaisesRegex(TypeError, 'attribute name must be a string'):
            operator.attrgetter(123)
        with self.assertRaisesRegex(TypeError, '^attrgetter expected 1 argument, got 0$'):
            operator.attrgetter()

    def test_attrgetter_stops_on_selected_error(self):
        events = []
        sentinel = RuntimeError('selected getattr sentinel')
        class Target:
            def __getattribute__(self, name):
                events.append(name)
                if name == 'first':
                    raise sentinel
                return 123
        for invoke in [lambda f, value: f(value), legacy_call]:
            events.clear()
            with self.assertRaises(RuntimeError) as caught:
                invoke(operator.attrgetter('first', 'never'), Target())
            self.assertIs(caught.exception, sentinel)
            self.assertEqual(events, ['first'])

    def test_methodcaller_vector_and_legacy_small_large(self):
        events = []
        class Target:
            def selected(self, *args, **kw):
                events.append((args, kw))
                return args, kw
        for count in [2, 9]:
            args = tuple(range(count))
            functor = operator.methodcaller('selected', *args, scalar=4.5)
            for invoke in [lambda f, value: f(value), legacy_call]:
                events.clear()
                self.assertEqual(invoke(functor, Target()), (args, {'scalar': 4.5}))
                self.assertEqual(events, [(args, {'scalar': 4.5})])
        with self.assertRaisesRegex(TypeError, '^method name must be a string$'):
            operator.methodcaller(123)
        class MethodName(str):
            pass
        name = MethodName('selected')
        name_ref = weakref.ref(name)
        functor = operator.methodcaller(name)
        del name, functor
        gc.collect()
        self.assertIsNone(name_ref())
        sentinel = RuntimeError('method lookup sentinel')
        class Missing:
            def __getattribute__(self, name):
                raise sentinel
        with self.assertRaises(RuntimeError) as caught:
            operator.methodcaller('selected', 123)(Missing())
        self.assertIs(caught.exception, sentinel)

    def test_repr_reduce_and_pickle_keep_scalar_arguments(self):
        functors = [operator.itemgetter(0, 1),
                    operator.attrgetter('a.b.c', 'other'),
                    operator.methodcaller('selected', 123, scalar=4.5)]
        for functor in functors:
            restored = pickle.loads(pickle.dumps(functor))
            self.assertEqual(repr(restored), repr(functor))
        self.assertEqual(repr(operator.attrgetter('a.b.c')), "operator.attrgetter('a.b.c')")
        self.assertEqual(repr(operator.methodcaller('selected', 123, scalar=4.5)),
                         "operator.methodcaller('selected', 123, scalar=4.5)")

    def test_methodcaller_repr_live_keyword_mutation_order(self):
        events = []
        holder = {}
        class Extra:
            def __repr__(self):
                events.append('extra')
                return 'extra'
        class First:
            def __repr__(self):
                events.append('first')
                kwds = next(value for value in gc.get_referents(holder['functor'])
                            if isinstance(value, dict))
                kwds['extra'] = Extra()
                return 'first'
        holder['functor'] = operator.methodcaller('selected', first=First())
        with self.assertRaisesRegex(RuntimeError, 'keywords dict changed size during iteration'):
            repr(holder['functor'])
        # Existing code formats the overflowing entry before its size guard.
        self.assertEqual(events, ['first', 'extra'])

    def test_lru_key_scalar_types_keyword_order_and_once_hash(self):
        calls = []
        @functools.lru_cache(maxsize=32, typed=True)
        def typed(value):
            calls.append(type(value))
            return type(value)
        for value in [1, 1.0, True, 'selected']:
            self.assertIs(typed(value), type(value))
            self.assertIs(typed(value), type(value))
        self.assertEqual(calls, [int, float, bool, str])
        order = []
        @functools.lru_cache(maxsize=32)
        def ordered(*args, **kw):
            order.append((args, list(kw.items())))
            return len(order)
        self.assertEqual(ordered(123), 1)
        self.assertEqual(ordered(123), 1)
        self.assertEqual(order, [((123,), [])])
        self.assertNotEqual(ordered(x=1, y=2), ordered(y=2, x=1))
        events = []
        class Key:
            def __hash__(self):
                events.append('hash')
                return 1234
        key = Key()
        self.assertEqual(ordered(key), ordered(key))
        self.assertEqual(events, ['hash', 'hash'])
        sentinel = RuntimeError('key hash sentinel')
        class Bad:
            def __hash__(self):
                raise sentinel
        with self.assertRaises(RuntimeError) as caught:
            ordered(Bad())
        self.assertIs(caught.exception, sentinel)


if __name__ == '__main__':
    unittest.main(verbosity=2)
