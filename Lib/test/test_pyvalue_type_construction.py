import gc
import types
import unittest
import weakref


def make_cell(value):
    def read():
        return value
    return read.__closure__[0]


class ConstructionReaderReferences(unittest.TestCase):
    def test_slots_validation_mangling_and_secondary_bases(self):
        cls = type('Selected', (), {'__slots__': ('__secret', 'visible')})
        value = cls()
        value._Selected__secret = 123
        value.visible = 4.5
        self.assertEqual((value._Selected__secret, value.visible), (123, 4.5))
        with self.assertRaisesRegex(TypeError, "__slots__ items must be strings, not 'int'"):
            type('BadScalar', (), {'__slots__': (123,)})
        with self.assertRaisesRegex(TypeError, '__slots__ must be identifiers'):
            type('BadName', (), {'__slots__': ('not valid',)})
        with self.assertRaisesRegex(ValueError, 'conflicts with class variable'):
            type('Conflict', (), {'__slots__': ('entry',), 'entry': 1})
        class Empty:
            __slots__ = ()
        class HasStorage:
            pass
        combined = type('Combined', (Empty, HasStorage), {'__slots__': ()})
        value = combined()
        value.extra = 123
        self.assertEqual(value.extra, 123)
        self.assertIs(weakref.ref(value)(), value)

    def test_slot_string_subclass_callback_order(self):
        events = []
        class Slot(str):
            def __hash__(self):
                events.append(('hash', str(self)))
                return str.__hash__(self)
            def __lt__(self, other):
                events.append(('lt', str(self), str(other)))
                return str.__lt__(self, other)
        cls = type('SortedSlots', (), {'__slots__': (Slot('b'), Slot('a'))})
        self.assertEqual(events, [('hash', 'b'), ('hash', 'a'), ('lt', 'a', 'b')])
        value = cls()
        value.a = 1
        value.b = 2
        self.assertEqual((value.a, value.b), (1, 2))

    def test_selected_metaclass_survives_base_metaclass_change(self):
        class OldMeta(type):
            pass
        class NewMeta(type):
            pass
        class Base(metaclass=OldMeta):
            pass
        events = []
        class Slot(str):
            def __hash__(self):
                events.append('hash')
                Base.__class__ = NewMeta
                gc.collect()
                return str.__hash__(self)
        cls = type('SelectedMeta', (Base,), {'__slots__': (Slot('entry'),)})
        self.assertIs(type(cls), OldMeta)
        self.assertIs(type(Base), NewMeta)
        self.assertEqual(events, ['hash'])
        # This host control deliberately keeps OldMeta alive. It demonstrates
        # selection order; it does not claim the candidate's new owner exists.

    def test_dictionary_selection_wrappers_and_scalar_ignores(self):
        events = []
        def new(cls):
            events.append('new')
            return object.__new__(cls)
        def initialize(cls, **kwargs):
            events.append(('init_subclass', cls.__name__))
        def getitem(cls, key):
            return cls, key
        cls = type('Wrapped', (), {'__new__': new,
                                  '__init_subclass__': initialize,
                                  '__class_getitem__': getitem,
                                  '__doc__': 'selected\0doc'})
        self.assertIsInstance(cls.__dict__['__new__'], staticmethod)
        self.assertIsInstance(cls.__dict__['__init_subclass__'], classmethod)
        self.assertIsInstance(cls.__dict__['__class_getitem__'], classmethod)
        self.assertIsInstance(cls(), cls)
        child = type('Child', (cls,), {})
        self.assertEqual(events, ['new', ('init_subclass', child.__name__)])
        self.assertEqual(cls[123], (cls, 123))
        self.assertEqual(cls.__doc__, 'selected\0doc')
        scalar = type('ScalarValues', (), {'__new__': 123,
                                           '__class_getitem__': 4.5,
                                           '__doc__': 7})
        self.assertEqual(scalar.__dict__['__new__'], 123)
        self.assertEqual(scalar.__dict__['__class_getitem__'], 4.5)
        self.assertEqual(scalar.__doc__, 7)

    def test_cells_and_invalid_selected_type_repr(self):
        class_cell = make_cell(None)
        dict_cell = make_cell(None)
        token = object()
        cls = type('Cells', (), {'__classcell__': class_cell,
                                 '__classdictcell__': dict_cell,
                                 'token': token})
        self.assertIs(class_cell.cell_contents, cls)
        self.assertIs(dict_cell.cell_contents['token'], token)
        self.assertNotIn('__classcell__', cls.__dict__)
        self.assertNotIn('__classdictcell__', dict_cell.cell_contents)
        for key in ['__classcell__', '__classdictcell__']:
            with self.assertRaisesRegex(TypeError, key + ' must be a nonlocal cell'):
                type('BadCell', (), {key: 123})
        events = []
        class Meta(type):
            def __repr__(cls):
                events.append('repr')
                return 'SelectedCellType'
        class Invalid(metaclass=Meta):
            pass
        with self.assertRaisesRegex(TypeError, 'SelectedCellType'):
            type('BadSelectedCell', (), {'__classcell__': Invalid()})
        self.assertEqual(events, ['repr'])

    def test_slots_iterator_and_bases_resolution_once(self):
        events = []
        sentinel = RuntimeError('selected slots sentinel')
        class Slots:
            def __iter__(self):
                events.append('iter')
                yield 'selected'
                raise sentinel
        with self.assertRaises(RuntimeError) as caught:
            type('BrokenSlots', (), {'__slots__': Slots()})
        self.assertIs(caught.exception, sentinel)
        self.assertEqual(events, ['iter'])
        events.clear()
        class Proxy:
            def __getattribute__(self, name):
                if name == '__mro_entries__':
                    events.append('lookup')
                    return lambda bases: events.append('resolve')
                return object.__getattribute__(self, name)
        with self.assertRaisesRegex(TypeError, "doesn't support MRO entry resolution"):
            type('NoResolution', (Proxy(),), {})
        self.assertEqual(events, ['lookup'])
        with self.assertRaises(TypeError):
            type('ScalarBase', (123,), {})
        self.assertEqual(type('EmptyBases', (), {}).__bases__, (object,))
        events.clear()
        result = object()
        class Meta(type):
            def __new__(meta, name, bases, namespace, **kwargs):
                if name == 'Delegated':
                    events.append('delegate')
                    return result
                return super().__new__(meta, name, bases, namespace, **kwargs)
        class Base(metaclass=Meta):
            pass
        self.assertIs(type('Delegated', (Base,), {}), result)
        self.assertEqual(events, ['delegate'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
