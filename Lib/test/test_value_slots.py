"""Value slots in PyTypeObject (V2a-0).

Covers pyjit/design/proposals/value-api-v2a0.md, sections 4.2-4.4 (the
tp_value field, the pairing invariant and inheritance in PyType_Ready) and
section 4.8 (the test types).
"""

import unittest

from test import support
from test.support import import_helper, script_helper

_testinternalcapi = import_helper.import_module('_testinternalcapi')
_testcapi = import_helper.import_module('_testcapi')

value_slot = _testinternalcapi.value_slot
object_slot = _testinternalcapi.object_slot
value_slot_names = _testinternalcapi.value_slot_names
VInt = _testinternalcapi.VInt
VIntSubNothing = _testinternalcapi.VIntSubNothing
VIntSubRepr = _testinternalcapi.VIntSubRepr
VIntSubAdd = _testinternalcapi.VIntSubAdd
VIntSpec = _testinternalcapi.VIntSpec
NotImpl = _testinternalcapi.NotImpl
VSeq = _testinternalcapi.VSeq
call_value_slot = _testinternalcapi.call_value_slot

VINT_FIELDS = ('nb_add', 'nb_multiply', 'richcompare', 'hash', 'repr',
               'nb_negative', 'nb_power', 'nb_bool')


def slots(t):
    return {name: value_slot(t, name) for name in value_slot_names()}


def vint_slots(t):
    return {name: value_slot(t, name) for name in VINT_FIELDS}


class ValueSlotTableTests(unittest.TestCase):
    def test_names(self):
        names = value_slot_names()
        self.assertEqual(len(names), 53)
        self.assertEqual(len(set(names)), 53)
        self.assertEqual(names[:16], (
            'nb_add', 'nb_subtract', 'nb_multiply', 'nb_true_divide',
            'nb_floor_divide', 'nb_remainder', 'richcompare', 'hash',
            'nb_bool', 'nb_index', 'getattro', 'subscript', 'ass_subscript',
            'contains', 'iternext', 'call'))

    def test_unknown_name(self):
        with self.assertRaises(ValueError):
            value_slot(int, 'nb_nothing')
        with self.assertRaises(ValueError):
            object_slot(int, 'nb_nothing')


class VIntTests(unittest.TestCase):
    def test_own_pairs(self):
        functions = _testinternalcapi.value_functions()
        self.assertEqual(value_slot(VInt, 'nb_add'), functions['VInt.add'])
        self.assertEqual(value_slot(VInt, 'nb_multiply'),
                         functions['VInt.multiply'])
        self.assertEqual(value_slot(VInt, 'richcompare'),
                         functions['VInt.richcompare'])
        self.assertEqual(value_slot(VInt, 'hash'), functions['VInt.hash'])
        self.assertEqual(value_slot(VInt, 'repr'), functions['VInt.repr'])
        self.assertEqual(value_slot(VInt, 'nb_negative'),
                         functions['VInt.negative'])
        self.assertEqual(value_slot(VInt, 'nb_power'), functions['VInt.power'])
        self.assertEqual(value_slot(VInt, 'nb_bool'), functions['VInt.bool'])
        self.assertEqual(value_slot(VSeq, 'sq_concat'),
                         functions['VSeq.concat'])
        self.assertEqual(object_slot(VInt, 'nb_add'),
                         functions['VInt.add.object'])

    def test_static_subtypes(self):
        expected = vint_slots(VInt)
        self.assertEqual(vint_slots(VIntSubNothing), expected)

        got = vint_slots(VIntSubRepr)
        self.assertEqual(got.pop('repr'), 0)
        expected_no_repr = dict(expected)
        del expected_no_repr['repr']
        self.assertEqual(got, expected_no_repr)

        got = vint_slots(VIntSubAdd)
        self.assertEqual(got['nb_add'], 0)
        self.assertEqual(got['nb_multiply'], expected['nb_multiply'])

    def test_static_subtype_behaviour(self):
        self.assertEqual(repr(VIntSubRepr(2)), 'VIntSubRepr(2)')
        self.assertEqual(repr(VInt(2)), 'VInt(2)')
        self.assertEqual((VInt(2) + VInt(3)).value, 5)
        self.assertEqual((VInt(2) * VInt(3)).value, 6)
        self.assertEqual((VIntSubAdd(1) + VIntSubAdd(2)).value, 1003)
        self.assertEqual((VIntSubNothing(2) + 3).value, 5)

    def test_python_subclass_inherits(self):
        class Sub(VInt):
            pass

        self.assertEqual(vint_slots(Sub), vint_slots(VInt))
        self.assertEqual(object_slot(Sub, 'nb_add'),
                         object_slot(VInt, 'nb_add'))

    def test_spec_type(self):
        functions = _testinternalcapi.value_functions()
        self.assertEqual(value_slot(VIntSpec, 'nb_add'),
                         functions['VIntSpec.add'])
        self.assertNotEqual(object_slot(VIntSpec, 'nb_add'),
                            object_slot(VInt, 'nb_add'))
        self.assertEqual(value_slot(VIntSpec, 'nb_multiply'),
                         value_slot(VInt, 'nb_multiply'))
        self.assertEqual((VIntSpec(1) + VIntSpec(2)).value, 4)


class IntSubtypeTests(unittest.TestCase):
    def test_no_value_slots_yet(self):
        for t in (int, _testcapi.IntSubNothing, _testcapi.IntSubRepr,
                  _testcapi.IntSubAdd):
            with self.subTest(t=t):
                self.assertEqual(value_slot(t, 'nb_add'), 0)
                self.assertEqual(value_slot(t, 'repr'), 0)


class PairTableTests(unittest.TestCase):
    def test_hash(self):
        functions = _testinternalcapi.value_functions()
        generic = functions['_PyValue_GenericHash']
        self.assertEqual(value_slot(object, 'hash'), generic)

        class Plain:
            pass

        self.assertEqual(value_slot(Plain, 'hash'), generic)
        self.assertEqual(value_slot(list, 'hash'),
                         functions['_PyValue_HashNotImplemented'])
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            hash([])
        hash(object())


class NotImplementedSideTests(unittest.TestCase):
    """The _Py_VALUE_NOTIMPL_* value sides (script class N) of NotImpl, whose
    object slots return NotImplemented unless every operand is a NotImpl."""

    def setUp(self):
        self.log = []
        _testinternalcapi.set_value_slots_log(self.log)
        self.addCleanup(_testinternalcapi.set_value_slots_log, None)

    def test_pairs(self):
        functions = _testinternalcapi.value_functions()
        for name, function in (('nb_add', 'NotImpl.add'),
                               ('nb_power', 'NotImpl.power'),
                               ('richcompare', 'NotImpl.richcompare')):
            with self.subTest(slot=name):
                self.assertEqual(value_slot(NotImpl, name),
                                 functions[function])

    def test_inline_operand(self):
        # An inline operand gives NotImplemented without a call.
        self.assertIs(call_value_slot(NotImpl, 'nb_add', NotImpl(1), 5),
                      NotImplemented)
        self.assertIs(call_value_slot(NotImpl, 'nb_add', 2.5, NotImpl(1)),
                      NotImplemented)
        self.assertIs(call_value_slot(NotImpl, 'richcompare',
                                      NotImpl(1), 5, 2), NotImplemented)
        self.assertIs(call_value_slot(NotImpl, 'nb_power',
                                      5, NotImpl(1), None), NotImplemented)
        self.assertEqual(self.log, [])

    def test_object_operands(self):
        self.assertEqual(call_value_slot(NotImpl, 'nb_add',
                                         NotImpl(1), NotImpl(2)).value, 3)
        # An int too large to be inline is an object: the object slot
        # decides.
        self.assertIs(call_value_slot(NotImpl, 'nb_add', NotImpl(1), 2**60),
                      NotImplemented)
        self.assertIs(call_value_slot(NotImpl, 'richcompare',
                                      NotImpl(1), NotImpl(2), 0), True)
        # An inline modulus is boxed for the object slot.
        self.assertEqual(call_value_slot(NotImpl, 'nb_power',
                                         NotImpl(3), NotImpl(2), 5).value, 4)
        self.assertEqual(self.log, ['NotImpl.add', 'NotImpl.add',
                                    'NotImpl.richcompare', 'NotImpl.power'])

    def test_operators(self):
        self.assertEqual((NotImpl(1) + NotImpl(2)).value, 3)
        self.assertEqual(pow(NotImpl(3), NotImpl(2), 5).value, 4)
        self.assertIs(NotImpl(1) < NotImpl(2), True)
        with self.assertRaises(TypeError):
            NotImpl(1) + 1
        # ternary_op also calls the modulus's slot, which must return
        # NotImplemented for operands of other types.
        with self.assertRaises(TypeError):
            pow(2, 3, NotImpl(5))


class ThunkTests(unittest.TestCase):
    """VInt's object slots are thunks of its value slots."""

    def test_value_slots_direct(self):
        self.assertEqual(call_value_slot(VInt, 'nb_add', VInt(2), 3).value, 5)
        self.assertEqual(call_value_slot(VInt, 'nb_add', 3, VInt(2)).value, 5)
        self.assertIs(call_value_slot(VInt, 'nb_add', VInt(2), 'x'),
                      NotImplemented)
        self.assertIs(call_value_slot(VInt, 'richcompare', VInt(2), 3, 0),
                      True)
        self.assertEqual(call_value_slot(VInt, 'hash', VInt(-1)), -2)
        self.assertEqual(call_value_slot(VInt, 'repr', VInt(7)), 'VInt(7)')

    def test_thunks(self):
        self.assertEqual((VInt(2) + 3).value, 5)
        self.assertEqual((3 + VInt(2)).value, 5)
        self.assertEqual((VInt(2) * VInt(4)).value, 8)
        self.assertIs(VInt(2) < 3, True)
        self.assertEqual(hash(VInt(-1)), -2)
        self.assertEqual(repr(VInt(7)), 'VInt(7)')
        with self.assertRaises(TypeError):
            VInt(2) + 'x'

    def test_no_value_slot(self):
        with self.assertRaises(LookupError):
            call_value_slot(int, 'nb_add', 1, 2)

    def test_unary_ternary_inquiry_concat(self):
        log = []
        _testinternalcapi.set_value_slots_log(log)
        self.addCleanup(_testinternalcapi.set_value_slots_log, None)

        self.assertEqual((-VInt(3)).value, -3)
        self.assertEqual(call_value_slot(VInt, 'nb_negative', VInt(3)).value,
                         -3)
        self.assertEqual((VInt(2) ** 3).value, 8)
        self.assertEqual(pow(VInt(2), 5, 7).value, 4)
        self.assertEqual(pow(2, VInt(3)).value, 8)
        self.assertEqual(call_value_slot(VInt, 'nb_power',
                                         VInt(2), 5, None).value, 32)
        self.assertEqual(call_value_slot(VInt, 'nb_power',
                                         VInt(2), 5, 7).value, 4)
        self.assertIs(bool(VInt(0)), False)
        self.assertIs(bool(VInt(2)), True)
        self.assertIs(call_value_slot(VInt, 'nb_bool', VInt(0)), False)
        self.assertEqual((VSeq(1) + 2).value, 1002)
        self.assertEqual(call_value_slot(VSeq, 'sq_concat', VSeq(1), 2).value,
                         1002)
        self.assertEqual(log, ['VInt.negative'] * 2 + ['VInt.power'] * 5
                         + ['VInt.bool'] * 3 + ['VSeq.concat'] * 2)
        with self.assertRaises(TypeError):
            VSeq(1) + 'x'


class InvariantTests(unittest.TestCase):
    def test_value_slot_has_object_slot(self):
        types = (int, float, str, list, dict, tuple, object, type, VInt,
                 VIntSubNothing, VIntSubRepr, VIntSubAdd, VIntSpec, NotImpl,
                 VSeq)
        for t in types:
            for name in value_slot_names():
                with self.subTest(type=t, slot=name):
                    if value_slot(t, name):
                        self.assertNotEqual(object_slot(t, name), 0)


@unittest.skipUnless(support.Py_DEBUG, 'the check is in debug builds only')
class InvariantFailureTests(unittest.TestCase):
    """The debug check aborts on a value field that breaks the invariant."""

    def check_abort(self, kind, message):
        code = f"""if 1:
            import _testinternalcapi
            from test.support import SuppressCrashReport
            with SuppressCrashReport():
                _testinternalcapi.make_invalid_value_type({kind!r})
            """
        rc, out, err = script_helper.assert_python_failure('-c', code)
        self.assertIn(message.encode(), err)

    def test_value_beside_null_object_slot(self):
        self.check_abort('null', 'value slot nb_negative is set beside a '
                         'NULL object slot')

    def test_value_beside_inherited_object_slot(self):
        self.check_abort('inherited', 'value slot nb_add is set beside an '
                         'inherited object slot')


class UpdateSlotTests(unittest.TestCase):
    """update_one_slot() writes a new object function with the pair table's
    value side or 0 (D6). Until 0c writes the slotdef pairs, a slot wrapper
    that brings back VInt's object function brings back value side 0."""

    def pair(self, t):
        return object_slot(t, 'nb_add'), value_slot(t, 'nb_add')

    def test_class_body(self):
        class S(VInt):
            def __add__(self, other):
                return 'S'

        self.assertEqual(value_slot(S, 'nb_add'), 0)
        self.assertNotEqual(object_slot(S, 'nb_add'),
                            object_slot(VInt, 'nb_add'))
        self.assertEqual(S(1) + 1, 'S')

    def test_class_body_same_wrapper(self):
        class S(VInt):
            __add__ = VInt.__add__

        self.assertEqual(self.pair(S), self.pair(VInt))
        del S.__add__
        self.assertEqual(self.pair(S), self.pair(VInt))

    def test_assign_and_delete(self):
        class S(VInt):
            pass

        S.__add__ = lambda self, other: 'S'
        self.assertEqual(value_slot(S, 'nb_add'), 0)
        self.assertEqual(S(1) + 1, 'S')
        del S.__add__
        self.assertEqual(self.pair(S), (object_slot(VInt, 'nb_add'), 0))
        self.assertEqual((S(1) + 2).value, 3)

    def test_assign_on_base(self):
        class B(VInt):
            pass

        class C(B):
            pass

        self.assertEqual(self.pair(C), self.pair(VInt))
        B.__add__ = lambda self, other: 'B'
        self.assertEqual(value_slot(B, 'nb_add'), 0)
        self.assertEqual(value_slot(C, 'nb_add'), 0)
        self.assertEqual(C(1) + 1, 'B')
        del B.__add__
        self.assertEqual(self.pair(C), (object_slot(VInt, 'nb_add'), 0))

    def test_hash_none(self):
        functions = _testinternalcapi.value_functions()
        not_implemented = (object_slot(list, 'hash'),
                           functions['_PyValue_HashNotImplemented'])

        class N(VInt):
            __hash__ = None

        class E(VInt):
            def __eq__(self, other):
                return True

        for t in (N, E):
            with self.subTest(type=t):
                self.assertEqual((object_slot(t, 'hash'),
                                  value_slot(t, 'hash')), not_implemented)
                with self.assertRaisesRegex(TypeError, 'unhashable'):
                    hash(t(1))

    def test_class_assignment(self):
        class A(VInt):
            pass

        class B(VInt):
            def __add__(self, other):
                return 'B'

        a = A(5)
        a.__class__ = B
        self.assertEqual(a + 1, 'B')
        a.__class__ = A
        self.assertEqual((a + 1).value, 6)


# Later rows of the V2a-0 plan go here:
# - Dispatch order with _testcapi.ObjAdd (object slots only) and
#   set_value_slots_log() on both modules: which side runs first, which
#   calls are logged.


if __name__ == '__main__':
    unittest.main()
