"""Pickle and state ownership controls for universal values."""
import unittest
import weakref


class PyValueTypePickleReaders(unittest.TestCase):
    def test_default_state(self):
        class C:
            pass
        value = C()
        value.zero = 0
        value.large = 1 << 40
        value.floating = 0.0
        result = object.__reduce_ex__(value, 4)
        self.assertEqual(result[0].__name__, '__newobj__')
        self.assertEqual(result[1], (C,))
        self.assertIs(result[2], value.__dict__)
        self.assertEqual(object.__getstate__(value), value.__dict__)


    def test_ex_pair_lifetime(self):
        events = []
        external = []
        marker = object()
        class Pair(tuple):
            def __del__(self):
                events.append('outer-close')
                external.clear()
        class C:
            def __getnewargs_ex__(self):
                events.append('ex')
                args = (0, 1 << 40, 0.0, marker)
                kwargs = {'token': marker}
                external.extend((args, kwargs))
                return Pair((args, kwargs))
            def __getnewargs__(self):
                raise AssertionError('fallback must not run')
            def __getstate__(self):
                events.append('state')
                return None
        result = object.__reduce_ex__(C(), 4)
        self.assertEqual(events, ['ex', 'outer-close', 'state'])
        self.assertEqual(external, [])
        self.assertEqual(result[0].__name__, '__newobj_ex__')
        self.assertEqual(result[1][1][:3], (0, 1 << 40, 0.0))
        self.assertIs(result[1][1][3], marker)
        self.assertIs(result[1][2]['token'], marker)


    def test_fallback_tuple_copy(self):
        events = []
        marker = object()
        class Args(tuple):
            def __del__(self):
                events.append('args-close')
        class C:
            def __getnewargs__(self):
                events.append('args')
                return Args((0, 1 << 40, 0.0, marker))
            def __getstate__(self):
                events.append('state')
                return None
        result = object.__reduce_ex__(C(), 4)
        self.assertEqual(events, ['args', 'args-close', 'state'])
        self.assertEqual(result[0].__name__, '__newobj__')
        self.assertEqual(result[1][1:4], (0, 1 << 40, 0.0))
        self.assertIs(result[1][4], marker)


    def test_invalid_shapes(self):
        for output, kind, message in [
            (0, TypeError, "__getnewargs_ex__ should return a tuple, not 'int'"),
            (((),), ValueError, '__getnewargs_ex__ should return a tuple of length 2, not 1'),
            ((0, {}), TypeError, "first item of the tuple returned by __getnewargs_ex__ must be a tuple, not 'int'"),
            (((), 0.0), TypeError, "second item of the tuple returned by __getnewargs_ex__ must be a dict, not 'float'"),
        ]:
            class C:
                def __getnewargs_ex__(self):
                    return output
            with self.assertRaises(kind) as caught:
                object.__reduce_ex__(C(), 4)
            self.assertEqual(str(caught.exception), message)
        class C:
            def __getnewargs__(self):
                return 0
        with self.assertRaises(TypeError) as caught:
            object.__reduce_ex__(C(), 4)
        self.assertEqual(str(caught.exception), "__getnewargs__ should return a tuple, not 'int'")
        class D:
            __slots__ = ()
            __slotnames__ = [0]
        with self.assertRaises(TypeError) as caught:
            object.__getstate__(D())
        self.assertEqual(str(caught.exception), "attribute name must be string, not 'int'")


    def test_lookup_and_slot_primary(self):
        primary = ValueError('exact-primary')
        events = []
        class Hook:
            def __get__(self, instance, owner):
                events.append('descriptor')
                raise primary
        class C:
            __getnewargs_ex__ = Hook()
            def __getnewargs__(self):
                events.append('fallback')
                return ()
        with self.assertRaises(ValueError) as caught:
            object.__reduce_ex__(C(), 4)
        self.assertIs(caught.exception, primary)
        self.assertEqual(events, ['descriptor'])
        names = ['first']
        class D:
            __slots__ = ()
            __slotnames__ = names
            def __getattribute__(self, name):
                if name == 'first':
                    events.append('attribute')
                    names.append('extra')
                    raise primary
                return object.__getattribute__(self, name)
        with self.assertRaises(ValueError) as caught:
            object.__getstate__(D())
        self.assertIs(caught.exception, primary)
        self.assertEqual(events, ['descriptor', 'attribute'])


    def test_slot_replacement_and_missing(self):
        events = []
        names = ['first', 'later', 'absent']
        class C:
            __slots__ = ()
            __slotnames__ = names
            def __getattribute__(self, name):
                if name == 'first':
                    events.append(name)
                    names[1] = 'replacement'
                    return 0
                if name == 'replacement':
                    events.append(name)
                    return 1 << 40
                if name == 'absent':
                    events.append(name)
                    raise AttributeError(name)
                return object.__getattribute__(self, name)
        self.assertEqual(object.__getstate__(C()),
                          (None, {'first': 0, 'replacement': 1 << 40}))
        self.assertEqual(events, ['first', 'replacement', 'absent'])


    def test_slot_size_error_and_owned_name(self):
        names = ['first', 'later']
        events = []
        class C:
            __slots__ = ()
            __slotnames__ = names
            def __getattribute__(self, name):
                if name == 'first':
                    events.append(name)
                    names.pop()
                    return 0
                return object.__getattribute__(self, name)
        with self.assertRaises(RuntimeError) as caught:
            object.__getstate__(C())
        self.assertEqual(str(caught.exception), '__slotnames__ changed size during iteration')
        self.assertEqual(events, ['first'])
        class Name(str):
            def __hash__(self):
                events.append('hash')
                owned_names[0] = 'replacement'
                return str.__hash__(self)
        selected = Name('chosen')
        owned_names = [selected]
        selected_ref = weakref.ref(selected)
        del selected
        class D:
            __slots__ = ()
            __slotnames__ = owned_names
            def __getattribute__(self, name):
                if name == 'chosen':
                    events.append('chosen')
                    return 0.0
                return object.__getattribute__(self, name)
        result = object.__getstate__(D())
        self.assertEqual(events, ['first', 'chosen', 'hash'])
        self.assertIs(next(iter(result[1])), selected_ref())
        self.assertEqual(result[1], {'chosen': 0.0})


    def test_current_class_selection(self):
        events = []
        class After:
            def __getstate__(self):
                events.append('after-state')
                return None
        class Before:
            def __getnewargs__(self):
                events.append('before-args')
                self.__class__ = After
                return (0,)
        result = object.__reduce_ex__(Before(), 4)
        self.assertEqual(events, ['before-args', 'after-state'])
        self.assertEqual(result[1], (After, 0))



if __name__ == "__main__":
    unittest.main()
