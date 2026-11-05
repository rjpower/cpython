"""Value slots in PyTypeObject (V2a-0).

Covers pyjit/design/proposals/value-api-v2a0.md, sections 4.2-4.4 (the
tp_value field, the pairing invariant and inheritance in PyType_Ready) and
section 4.8 (the test types).
"""

import builtins
import collections
import itertools
import math
import opcode
import operator
import os
import re
import sys
import types
import typing
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
VIntCopyAdd = _testinternalcapi.VIntCopyAdd
NotImpl = _testinternalcapi.NotImpl
VSeq = _testinternalcapi.VSeq
call_value_slot = _testinternalcapi.call_value_slot
Split = _testinternalcapi.Split
SplitSeq = _testinternalcapi.SplitSeq
wrapper_pair = _testinternalcapi.wrapper_pair
value_dispatch = _testinternalcapi.value_dispatch
int_span_path = _testinternalcapi.int_span_path
value_extract = _testinternalcapi.value_extract
object_extract = _testinternalcapi.object_extract
value_construct = _testinternalcapi.value_construct
ObjAdd = _testcapi.ObjAdd
ObjIndex = _testcapi.ObjIndex
ObjNotImpl = _testcapi.ObjNotImpl
value_getattr = _testinternalcapi.value_getattr
value_getoptionalattr = _testinternalcapi.value_getoptionalattr
value_setattr = _testinternalcapi.value_setattr
value_delattr = _testinternalcapi.value_delattr
value_functions = _testinternalcapi.value_functions
getset_direct_get = _testinternalcapi.getset_direct_get
VGetSet = _testinternalcapi.VGetSet
ObjGetAttr = _testcapi.ObjGetAttr

VINT_FIELDS = ('nb_add', 'nb_multiply', 'richcompare', 'hash', 'repr',
               'nb_negative', 'nb_power', 'nb_bool')


def slots(t):
    return {name: value_slot(t, name) for name in value_slot_names()}


INT_ROWS = (
    'nb_add', 'nb_subtract', 'nb_multiply', 'nb_remainder', 'nb_divmod',
    'nb_power', 'nb_negative', 'nb_positive', 'nb_absolute', 'nb_bool',
    'nb_invert', 'nb_lshift', 'nb_rshift', 'nb_and', 'nb_xor', 'nb_or',
    'nb_int', 'nb_float', 'nb_floor_divide', 'nb_true_divide', 'nb_index',
    'richcompare', 'hash', 'repr')
FLOAT_ROWS = (
    'nb_add', 'nb_subtract', 'nb_multiply', 'nb_remainder', 'nb_divmod',
    'nb_power', 'nb_negative', 'nb_positive', 'nb_absolute', 'nb_bool',
    'nb_int', 'nb_float', 'nb_floor_divide', 'nb_true_divide', 'richcompare',
    'hash', 'repr')
BOOL_OWN_ROWS = ('nb_and', 'nb_or', 'nb_xor', 'nb_invert')


def int_slots(t):
    return {name: value_slot(t, name) for name in INT_ROWS}


def set_rows(t):
    """The names of the value slots of t that are set."""
    return {name for name in value_slot_names() if value_slot(t, name)}


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
    def test_static_subtypes(self):
        # V2a-1a: int has value slots. A static extension subtype that
        # overrides nothing inherits all of them; one that overrides an object
        # slot loses that row only.
        expected = int_slots(int)
        self.assertEqual(int_slots(_testcapi.IntSubNothing), expected)

        got = int_slots(_testcapi.IntSubRepr)
        self.assertEqual(got.pop('repr'), 0)
        expected_no_repr = dict(expected)
        del expected_no_repr['repr']
        self.assertEqual(got, expected_no_repr)

        got = int_slots(_testcapi.IntSubAdd)
        self.assertEqual(got.pop('nb_add'), 0)
        expected_no_add = dict(expected)
        del expected_no_add['nb_add']
        self.assertEqual(got, expected_no_add)

    def test_static_subtype_object_slots(self):
        for t in (_testcapi.IntSubNothing, _testcapi.IntSubRepr):
            with self.subTest(t=t):
                self.assertEqual(object_slot(t, 'nb_add'),
                                 object_slot(int, 'nb_add'))
        self.assertNotEqual(object_slot(_testcapi.IntSubAdd, 'nb_add'),
                            object_slot(int, 'nb_add'))


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

    def test_object_hash_of_inline_values(self):
        # object.__hash__ applied to an exact int or float calls the pair's
        # value wrapper, so _PyValue_GenericHash() gets an inline value. It
        # hashes the value's identity without a box, as
        # PyObject_GenericHash() hashes any box of the value.
        generichash = _testcapi.object_generichash
        for x in (1, 1000, -2**50, 2.5, -0.0):
            with self.subTest(x=x):
                self.assertEqual(object.__hash__(x), object.__hash__(x))
                self.assertEqual(object.__hash__(x), generichash(x))
        # A value that holds an object hashes that object.
        for x in (2**60, float('nan'), 'string'):
            with self.subTest(x=x):
                self.assertEqual(object.__hash__(x), generichash(x))
        # The slot wrappers of int and float check their argument's type.
        with self.assertRaises(TypeError):
            float.__hash__(1)
        with self.assertRaises(TypeError):
            int.__hash__(2.5)


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

    def test_int_value_slot(self):
        self.assertEqual(call_value_slot(int, 'nb_add', 1, 2), 3)
        self.assertIs(call_value_slot(int, 'nb_add', 1, 2.5), NotImplemented)

    def test_no_value_slot(self):
        with self.assertRaises(LookupError):
            call_value_slot(int, 'nb_matrix_multiply', 1, 2)

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


class ValueWrapperTests(unittest.TestCase):
    """The slot wrappers of Split and SplitSeq call the value side."""

    def setUp(self):
        self.log = []
        _testinternalcapi.set_value_slots_log(self.log)
        self.addCleanup(_testinternalcapi.set_value_slots_log, None)

    def check(self, op, expected, call, *args):
        """Call through the class and bound; the value side runs each time."""
        name = f'Split.{op}.value'
        for how in ('class', 'bound'):
            with self.subTest(how=how):
                self.log.clear()
                if how == 'class':
                    result = getattr(Split, call)(*args)
                else:
                    result = getattr(args[0], call)(*args[1:])
                if isinstance(result, Split):
                    result = result.value
                self.assertEqual(result, expected)
                self.assertEqual(self.log, [name])

    def test_binary(self):
        self.check('sub', 5, '__sub__', Split(7), Split(2))
        self.check('sub', 5, '__sub__', Split(7), 2)
        self.check('sub', -5, '__rsub__', Split(7), 2)
        self.check('isub', 5, '__isub__', Split(7), 2)

    def test_power(self):
        self.check('pow', 8, '__pow__', Split(2), 3)
        self.check('pow', 8, '__pow__', Split(2), Split(3))
        self.check('pow', 1, '__pow__', Split(3), 4, 5)
        self.check('pow', 9, '__rpow__', Split(2), 3)

    def test_unary(self):
        self.check('neg', -7, '__neg__', Split(7))
        self.check('index', 7, '__index__', Split(7))
        self.check('bool', True, '__bool__', Split(7))
        self.check('bool', False, '__bool__', Split(0))
        self.check('hash', 7, '__hash__', Split(7))
        self.check('repr', 'Split(7)', '__repr__', Split(7))
        self.check('str', 'split 7', '__str__', Split(7))

    def test_richcompare(self):
        for call, expected in (('__lt__', False), ('__le__', True),
                               ('__eq__', True), ('__ne__', False),
                               ('__gt__', False), ('__ge__', True)):
            with self.subTest(call=call):
                self.check('richcompare', expected, call, Split(7), 7)
        self.check('richcompare', True, '__lt__', Split(7), Split(8))

    def test_not_implemented(self):
        self.assertIs(Split.__sub__(Split(7), 'x'), NotImplemented)
        self.assertEqual(self.log, ['Split.sub.value'])

    def test_sequence(self):
        for call, op in (('__add__', 'concat'), ('__iadd__', 'iconcat')):
            with self.subTest(call=call):
                self.log.clear()
                result = getattr(SplitSeq, call)(SplitSeq(1), 2)
                self.assertEqual(result.value, 1002)
                self.assertEqual(self.log, [f'SplitSeq.{op}.value'])
                self.log.clear()
                result = getattr(SplitSeq(1), call)(SplitSeq(3))
                self.assertEqual(result.value, 1003)
                self.assertEqual(self.log, [f'SplitSeq.{op}.value'])
                with self.assertRaises(TypeError):
                    getattr(SplitSeq, call)(SplitSeq(1), 'x')

    def test_argument_errors(self):
        for call, args, int_call, int_args, message in (
            (Split.__sub__, (Split(1),), int.__sub__, (1,),
             "expected 1 argument, got 0"),
            (Split.__pow__, (Split(1), 1, 2, 3), int.__pow__, (1, 1, 2, 3),
             "expected 1 or 2 arguments, got 3"),
            (Split.__neg__, (Split(1), 2), int.__neg__, (1, 2),
             "expected 0 arguments, got 1"),
        ):
            with self.subTest(message=message):
                with self.assertRaises(TypeError) as cm:
                    call(*args)
                self.assertEqual(str(cm.exception), message)
                with self.assertRaises(TypeError) as cm:
                    int_call(*int_args)
                self.assertEqual(str(cm.exception), message)
                self.assertEqual(self.log, [])

    def test_wrong_self(self):
        with self.assertRaises(TypeError):
            Split.__sub__(1, 2)
        self.assertEqual(self.log, [])

    def test_operators_use_value_side(self):
        x = Split(7)

        def inplace_sub():
            y = x
            y -= 2

        operations = (
            ('sub', lambda: x - 2),
            ('isub', inplace_sub),
            ('pow', lambda: pow(x, 2)),
            ('neg', lambda: -x),
            ('index', lambda: operator.index(x)),
            ('bool', lambda: bool(x)),
            ('hash', lambda: hash(x)),
            ('richcompare', lambda: x < Split(9)),
            ('repr', lambda: repr(x)),
            ('str', lambda: str(x)),
        )
        for op, call in operations:
            with self.subTest(op=op):
                self.log.clear()
                call()
                self.assertEqual(self.log, [f'Split.{op}.value'])


class WrapperPairTests(unittest.TestCase):
    def test_split_pairs(self):
        functions = _testinternalcapi.value_functions()
        for descr, t, field in (
            (Split.__sub__, Split, 'nb_subtract'),
            (Split.__rsub__, Split, 'nb_subtract'),
            (Split.__neg__, Split, 'nb_negative'),
            (Split.__lt__, Split, 'richcompare'),
            (Split.__hash__, Split, 'hash'),
            (Split.__repr__, Split, 'repr'),
            (SplitSeq.__add__, SplitSeq, 'sq_concat'),
        ):
            with self.subTest(descr=descr):
                self.assertEqual(wrapper_pair(descr),
                                 (object_slot(t, field), value_slot(t, field)))
        self.assertEqual(wrapper_pair(Split.__sub__)[1],
                         functions['Split.sub.value'])
        self.assertEqual(wrapper_pair(Split.__neg__)[1],
                         functions['Split.neg.value'])

    def test_vint(self):
        self.assertEqual(wrapper_pair(VInt.__add__)[1],
                         value_slot(VInt, 'nb_add'))

    def test_int_pair(self):
        self.assertEqual(wrapper_pair(int.__add__),
                         (object_slot(int, 'nb_add'),
                          value_slot(int, 'nb_add')))
        self.assertNotEqual(wrapper_pair(int.__add__)[1], 0)

    def test_pair_table(self):
        functions = _testinternalcapi.value_functions()
        self.assertEqual(wrapper_pair(object.__hash__),
                         (functions['PyObject_GenericHash'],
                          functions['_PyValue_GenericHash']))

    def test_not_a_wrapper(self):
        with self.assertRaises(TypeError):
            wrapper_pair(1)


def pair(t):
    return object_slot(t, 'nb_subtract'), value_slot(t, 'nb_subtract')


class UpdateSlotTests(unittest.TestCase):
    """Class bodies and attribute assignment rewrite both sides."""

    split_pair = pair(Split)

    def test_inherit(self):
        class S(Split):
            pass

        self.assertEqual(pair(S), self.split_pair)

    def test_class_body(self):
        class S(Split):
            def __sub__(self, o):
                return 'S'

        self.assertNotIn(value_slot(S, 'nb_subtract'),
                         (0, self.split_pair[1]))
        self.assertNotEqual(object_slot(S, 'nb_subtract'), self.split_pair[0])
        self.assertEqual(S(1) - 1, 'S')

    def test_class_body_same_wrapper(self):
        class S(Split):
            __sub__ = Split.__sub__

        self.assertEqual(pair(S), self.split_pair)

    def test_assign_and_delete(self):
        class S(Split):
            pass

        S.__sub__ = lambda self, o: 'S'
        self.assertNotIn(value_slot(S, 'nb_subtract'),
                         (0, self.split_pair[1]))
        self.assertNotEqual(object_slot(S, 'nb_subtract'), self.split_pair[0])
        self.assertEqual(S(1) - 1, 'S')
        del S.__sub__
        self.assertEqual(pair(S), self.split_pair)
        self.assertEqual((S(7) - 2).value, 5)

    def test_assign_on_base(self):
        class B(Split):
            pass

        class C(B):
            pass

        B.__sub__ = lambda self, o: 'B'
        self.assertNotIn(value_slot(C, 'nb_subtract'),
                         (0, self.split_pair[1]))
        self.assertEqual(C(1) - 1, 'B')
        del B.__sub__
        self.assertEqual(pair(C), self.split_pair)

    def test_reflected_conflict(self):
        class S(Split):
            __rsub__ = lambda self, o: 'r'

        self.assertNotIn(value_slot(S, 'nb_subtract'),
                         (0, self.split_pair[1]))
        self.assertEqual(1 - S(1), 'r')
        # The generic slot finds Split's __sub__ by lookup.
        self.assertEqual((S(7) - 2).value, 5)

    def test_hash_none(self):
        functions = _testinternalcapi.value_functions()

        class S(Split):
            __hash__ = None

        class E(Split):
            def __eq__(self, o):
                return True

        for t in (S, E):
            with self.subTest(t=t):
                self.assertEqual(value_slot(t, 'hash'),
                                 functions['_PyValue_HashNotImplemented'])
                self.assertEqual(object_slot(t, 'hash'),
                                 functions['PyObject_HashNotImplemented'])
                with self.assertRaises(TypeError):
                    hash(t(1))

    def test_class_assignment(self):
        class A(Split):
            pass

        class B(Split):
            def __sub__(self, o):
                return 'B'

        a = A(5)
        a.__class__ = B
        self.assertEqual(a - 1, 'B')
        a.__class__ = A
        self.assertEqual((a - 1).value, 4)


def set_logs(test, log):
    """Collect the calls of both test modules in log."""
    for module in (_testinternalcapi, _testcapi):
        module.set_value_slots_log(log)
        test.addCleanup(module.set_value_slots_log, None)


class DispatchOrderTests(unittest.TestCase):
    """Which slots the dispatchers call, and in which order."""

    def setUp(self):
        self.log = []
        set_logs(self, self.log)

    def test_value_slot_before_object_slot(self):
        # VInt has no relation to ObjAdd: VInt's slot comes first, returns
        # NotImplemented, then ObjAdd's runs.
        self.assertEqual(VInt(3) + ObjAdd(),
                         'ObjAdd.add(_testinternalcapi.VInt, _testcapi.ObjAdd)')
        self.assertEqual(self.log, ['VInt.add', 'ObjAdd.add'])

    def test_object_slot_first(self):
        self.assertEqual(ObjAdd() + VInt(3),
                         'ObjAdd.add(_testcapi.ObjAdd, _testinternalcapi.VInt)')
        self.assertEqual(self.log, ['ObjAdd.add'])

    def test_unrelated_extension_types(self):
        # Neither type shares a slot with the other: both are tried.
        self.assertEqual(ObjNotImpl() + ObjAdd(),
                         'ObjAdd.add(_testcapi.ObjNotImpl, _testcapi.ObjAdd)')
        self.assertEqual(self.log, ['ObjNotImpl.add', 'ObjAdd.add'])

    def test_both_not_implemented(self):
        with self.assertRaises(TypeError):
            ObjNotImpl() + ObjNotImpl()
        # Same type: one call.
        self.assertEqual(self.log, ['ObjNotImpl.add'])
        self.log.clear()
        with self.assertRaises(TypeError):
            VInt(1) + ObjNotImpl()
        self.assertEqual(self.log, ['VInt.add', 'ObjNotImpl.add'])

    def test_reflected_compare_python_subclass(self):
        log = self.log

        class Sub(VInt):
            def __gt__(self, other):
                log.append('Sub.__gt__')
                return NotImplemented

        # The subclass's reflected method goes first. It returns
        # NotImplemented, so the left operand's slot runs.
        self.assertIs(VInt(1) < Sub(2), True)
        self.assertEqual(log, ['Sub.__gt__', 'VInt.richcompare'])

        class Answer(VInt):
            def __gt__(self, other):
                log.append('Answer.__gt__')
                return 'answer'

        log.clear()
        self.assertEqual(VInt(1) < Answer(2), 'answer')
        self.assertEqual(log, ['Answer.__gt__'])

    def test_reflected_compare_inherited_slot(self):
        # VIntSubNothing inherits VInt's tp_richcompare: do_richcompare()
        # does not compare the slots, so the reflected call goes first and
        # its result ends the search.
        self.assertIs(VInt(1) < VIntSubNothing(2), True)
        self.assertEqual(self.log, ['VInt.richcompare'])
        self.log.clear()
        # The subclass is on the left: its slot is the only one called.
        self.assertIs(VIntSubNothing(1) < VInt(2), True)
        self.assertEqual(self.log, ['VInt.richcompare'])

    def test_compare_extension_types(self):
        self.assertEqual(
            VInt(3) < ObjAdd(),
            'ObjAdd.richcompare(_testcapi.ObjAdd, '
            '_testinternalcapi.VInt, 4)')
        self.assertEqual(self.log, ['VInt.richcompare', 'ObjAdd.richcompare'])
        # Operands of the same type: do_richcompare() still tries the
        # right operand's slot, reflected, after the left one's.
        self.log.clear()
        with self.assertRaises(TypeError):
            ObjNotImpl() < ObjNotImpl()
        self.assertEqual(self.log, ['ObjNotImpl.richcompare'] * 2)
        self.log.clear()
        self.assertIs(ObjNotImpl() == ObjNotImpl(), False)
        self.assertEqual(self.log, ['ObjNotImpl.richcompare'] * 2)

    def test_pow_order(self):
        # v, then w, then z; a slot that returns NotImplemented passes on.
        with self.assertRaises(TypeError):
            pow(NotImpl(1), VInt(3), ObjNotImpl())
        self.assertEqual(self.log, ['NotImpl.power', 'VInt.power',
                                    'ObjNotImpl.power'])
        self.log.clear()
        with self.assertRaises(TypeError):
            pow(VInt(2), 3, ObjNotImpl())
        # int's own nb_power is not logged.
        self.assertEqual(self.log, ['VInt.power', 'ObjNotImpl.power'])
        self.log.clear()
        self.assertEqual(pow(VInt(2), 3, 5).value, 3)
        self.assertEqual(self.log, ['VInt.power'])

    def test_shared_object_slot_is_called_once(self):
        # D8: VIntCopyAdd has VInt's nb_add function, so binary_op1() drops
        # the right operand's slot as a duplicate.
        with self.assertRaises(TypeError):
            VInt(1) + VIntCopyAdd()
        self.assertEqual(self.log, ['VInt.add'])
        self.log.clear()
        with self.assertRaises(TypeError):
            VIntCopyAdd() + VInt(1)
        self.assertEqual(self.log, ['VInt.add'])


class IndexSlotTests(unittest.TestCase):
    def setUp(self):
        self.log = []
        set_logs(self, self.log)

    def test_object_only_index(self):
        self.assertEqual(value_slot(ObjIndex, 'nb_index'), 0)
        self.assertEqual(operator.index(ObjIndex()), 7)
        self.assertEqual(self.log, ['ObjIndex.index'])
        self.assertEqual(value_dispatch('Index', ObjIndex()), (7, True))
        self.assertEqual(self.log, ['ObjIndex.index'] * 2)
        self.assertEqual([0, 1, 2, 3, 4, 5, 6, 7, 8][ObjIndex()], 7)
        self.assertEqual(self.log, ['ObjIndex.index'] * 3)


class Key:
    """Equal to nothing, hashes to 1, and runs a callback in __eq__."""

    def __init__(self, on_eq=None):
        self.on_eq = on_eq

    def __hash__(self):
        return 1

    def __eq__(self, other):
        if self.on_eq is not None:
            self.on_eq()
        return False


class MutationDuringCompareTests(unittest.TestCase):
    def test_dict_cleared_by_stored_key(self):
        d = {}
        # The first two keys are referenced only by the dict.
        d[Key(d.clear)] = 1
        d[Key(d.clear)] = 2
        missing = object()
        self.assertIs(d.get(Key(), missing), missing)
        self.assertEqual(len(d), 0)

    def test_list_cleared_by_item(self):
        lst = []
        lst.append(Key(lst.clear))
        lst.append(Key(lst.clear))
        self.assertNotIn(Key(), lst)
        self.assertEqual(lst, [])
        lst.append(Key(lst.clear))
        with self.assertRaises(ValueError):
            lst.index(Key())
        lst.append(Key(lst.clear))
        self.assertEqual(lst.count(Key()), 0)


class ConcatErrorTests(unittest.TestCase):
    def test_list(self):
        with self.assertRaisesRegex(
                TypeError, re.escape('can only concatenate list '
                                     '(not "int") to list')):
            [] + 1

    def test_bytes(self):
        with self.assertRaisesRegex(TypeError,
                                    re.escape("can't concat int to bytes")):
            b'' + 1

    def test_bytearray(self):
        x = bytearray()
        with self.assertRaisesRegex(TypeError,
                                    re.escape("can't concat int to bytearray")):
            x += 1


class DispatcherTests(unittest.TestCase):
    """The dispatchers called directly with inline and object operands."""

    def setUp(self):
        self.log = []
        set_logs(self, self.log)

    def check(self, expected, name, *args):
        with self.subTest(name=name, args=args):
            self.assertEqual(value_dispatch(name, *args), expected)

    def test_binary_inline(self):
        for name, args, expected in (
            ('Add', (1, 2), 3),
            ('Subtract', (5, 2), 3),
            ('Multiply', (3, 4), 12),
            ('Remainder', (7, 4), 3),
            ('Lshift', (1, 3), 8),
            ('Rshift', (8, 3), 1),
            ('And', (6, 3), 2),
            ('Xor', (6, 3), 5),
            ('Or', (6, 3), 7),
            ('FloorDivide', (7, 2), 3),
            ('TrueDivide', (1, 2), 0.5),
            ('Add', (1.5, 2), 3.5),
        ):
            self.check((expected, True), name, *args)

    def test_binary_object_result(self):
        self.check((2**61, False), 'Add', 2**60, 2**60)
        self.check(((3, 1), False), 'Divmod', 7, 2)
        self.check(('ab', False), 'Add', 'a', 'b')
        self.check(((1, 2), False), 'Add', (1,), (2,))

    def test_binary_errors(self):
        with self.assertRaisesRegex(TypeError, 'unsupported operand'):
            value_dispatch('MatrixMultiply', 1, 2)
        with self.assertRaisesRegex(TypeError, re.escape(
                'can only concatenate list (not "int") to list')):
            value_dispatch('Add', [], 1)
        with self.assertRaises(ZeroDivisionError):
            value_dispatch('FloorDivide', 1, 0)

    def test_binary_vint(self):
        result, was_inline = value_dispatch('Add', VInt(2), 3)
        self.assertIsInstance(result, VInt)
        self.assertEqual(result.value, 5)
        self.assertFalse(was_inline)
        result, was_inline = value_dispatch('Multiply', 3, VInt(2))
        self.assertEqual(result.value, 6)
        self.assertEqual(self.log, ['VInt.add', 'VInt.multiply'])

    def test_inplace(self):
        for name, args, expected in (
            ('InPlaceAdd', (1, 2), 3),
            ('InPlaceSubtract', (5, 2), 3),
            ('InPlaceMultiply', (3, 4), 12),
            ('InPlaceRemainder', (7, 4), 3),
            ('InPlaceLshift', (1, 3), 8),
            ('InPlaceRshift', (8, 3), 1),
            ('InPlaceAnd', (6, 3), 2),
            ('InPlaceXor', (6, 3), 5),
            ('InPlaceOr', (6, 3), 7),
            ('InPlaceFloorDivide', (7, 2), 3),
            ('InPlaceTrueDivide', (1, 2), 0.5),
        ):
            self.check((expected, True), name, *args)
        with self.assertRaises(TypeError):
            value_dispatch('InPlaceMatrixMultiply', 1, 2)

    def test_inplace_list(self):
        lst = [1]
        result, was_inline = value_dispatch('InPlaceAdd', lst, [2])
        self.assertIs(result, lst)
        self.assertFalse(was_inline)
        self.assertEqual(lst, [1, 2])
        result, was_inline = value_dispatch('InPlaceMultiply', lst, 2)
        self.assertIs(result, lst)
        self.assertFalse(was_inline)
        self.assertEqual(lst, [1, 2, 1, 2])

    def test_inplace_falls_back_to_binary(self):
        result, was_inline = value_dispatch('InPlaceAdd', VInt(2), 3)
        self.assertEqual(result.value, 5)
        self.assertEqual(self.log, ['VInt.add'])

    def test_power(self):
        self.check((1024, True), 'Power', 2, 10, None)
        self.check((3, True), 'Power', 2, 3, 5)
        self.check((0.5, True), 'Power', 2, -1, None)
        self.check((8, True), 'InPlacePower', 2, 3, None)
        result, was_inline = value_dispatch('Power', VInt(2), 5, None)
        self.assertEqual(result.value, 32)
        self.assertFalse(was_inline)
        result, was_inline = value_dispatch('InPlacePower', VInt(2), 5, 7)
        self.assertEqual(result.value, 4)
        with self.assertRaises(TypeError):
            value_dispatch('Power', 'a', 2, None)

    def test_unary(self):
        self.check((-5, True), 'Negative', 5)
        self.check((-5, True), 'Positive', -5)
        self.check((5, True), 'Absolute', -5)
        self.check((2.5, True), 'Absolute', -2.5)
        self.check((-6, True), 'Invert', 5)
        self.check((-2**60, False), 'Negative', 2**60)
        result, was_inline = value_dispatch('Negative', VInt(3))
        self.assertEqual(result.value, -3)
        self.assertFalse(was_inline)
        for name in ('Negative', 'Positive', 'Absolute', 'Invert'):
            with self.subTest(name=name):
                with self.assertRaises(TypeError):
                    value_dispatch(name, 'x')
        with self.assertRaises(TypeError):
            value_dispatch('Invert', 1.5)

    def test_index_long_float(self):
        self.check((5, True), 'Index', 5)
        self.check((2**70, False), 'Index', 2**70)
        self.check((3, True), 'Long', 3)
        self.check((3, True), 'Long', 3.5)
        self.check((-3, True), 'Long', -3.5)
        self.check((3.0, True), 'Float', 3)
        self.check((2.5, True), 'Float', 2.5)
        with self.assertRaisesRegex(TypeError, 'cannot be interpreted'):
            value_dispatch('Index', 1.5)
        with self.assertRaises(OverflowError):
            value_dispatch('Float', 10**400)
        result, was_inline = value_dispatch('Index', Split(4))
        self.assertEqual((result, was_inline), (4, True))

    def test_as_ssize_t(self):
        self.check(5, 'AsSsize_t', 5, OverflowError)
        self.check(-1, 'AsSsize_t', -1, IndexError)
        self.check(5, 'AsSsize_t', 5, None)
        self.check(sys.maxsize, 'AsSsize_t', 2**70, None)
        self.check(-sys.maxsize - 1, 'AsSsize_t', -2**70, None)
        with self.assertRaises(OverflowError):
            value_dispatch('AsSsize_t', 2**70, OverflowError)
        with self.assertRaises(IndexError):
            value_dispatch('AsSsize_t', 2**70, IndexError)
        with self.assertRaises(TypeError):
            value_dispatch('AsSsize_t', 1.5, OverflowError)

    def test_richcompare(self):
        for args, expected in (
            ((1, 1.0, 2), True),
            ((1, 2, 0), True),
            ((2, 1, 0), False),
            ((1, 2, 5), False),
            ((1, 2, 3), True),
            (([1], [1], 2), True),
            (('a', 'b', 0), True),
            ((float('nan'), 1, 2), False),
        ):
            with self.subTest(args=args):
                result, was_inline = value_dispatch('RichCompare', *args)
                self.assertIs(result, expected)
        result, was_inline = value_dispatch('RichCompare', VInt(1), VInt(2), 0)
        self.assertIs(result, True)
        with self.assertRaises(TypeError):
            value_dispatch('RichCompare', 1, 'a', 0)
        with self.assertRaises(ValueError):
            value_dispatch('RichCompare', 1, 2, 6)

    def test_richcompare_object_result(self):
        class Weird:
            def __eq__(self, other):
                return 'weird'

        result, was_inline = value_dispatch('RichCompare', Weird(), 1, 2)
        self.assertEqual(result, 'weird')
        self.assertFalse(was_inline)

    def test_richcompare_bool(self):
        nan = float('nan')
        for args, expected in (
            ((1, 1.0, 2), True),
            ((1, 2, 0), True),
            ((1, 2, 2), False),
            ((nan, nan, 2), True),
            ((nan, float('nan'), 2), False),
            (([nan], [nan], 2), True),
            ((VInt(1), VInt(1), 2), True),
        ):
            with self.subTest(args=args):
                self.assertIs(value_dispatch('RichCompareBool', *args),
                              expected)
        with self.assertRaises(TypeError):
            value_dispatch('RichCompareBool', 1, 'a', 0)

    def test_truth(self):
        for value in (0, 1, 0.0, 2.5, [], [1], None, '', 'a', VInt(0),
                      VInt(5)):
            with self.subTest(value=value):
                expected = bool(value) if not isinstance(value, VInt) \
                    else value.value != 0
                self.assertIs(value_dispatch('IsTrue', value), expected)
                self.assertIs(value_dispatch('Not', value), not expected)

    def test_hash(self):
        self.assertEqual(value_dispatch('Hash', -1), -2)
        self.assertEqual(value_dispatch('Hash', 5), 5)
        self.assertEqual(value_dispatch('Hash', 2.5), hash(2.5))
        self.assertEqual(value_dispatch('Hash', 'abc'), hash('abc'))
        self.assertEqual(value_dispatch('Hash', VInt(-1)), -2)
        self.assertEqual(value_dispatch('Hash', VInt(9)), 9)
        self.assertEqual(self.log, ['VInt.hash'] * 2)
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            value_dispatch('Hash', [])

    def test_repr_str_ascii(self):
        text = ''.join(['ab', 'c'])
        self.assertEqual(value_dispatch('Repr', 5), '5')
        self.assertEqual(value_dispatch('Repr', 2.5), '2.5')
        self.assertEqual(value_dispatch('Repr', text), "'abc'")
        self.assertEqual(value_dispatch('Repr', VInt(7)), 'VInt(7)')
        self.assertEqual(value_dispatch('Str', 5), '5')
        self.assertIs(value_dispatch('Str', text), text)
        self.assertEqual(value_dispatch('Str', VInt(7)), 'VInt(7)')
        self.assertEqual(value_dispatch('Str', Split(3)), 'split 3')
        self.assertEqual(value_dispatch('ASCII', 'é'), "'\\xe9'")
        self.assertEqual(value_dispatch('ASCII', 5), '5')
        self.assertEqual(value_dispatch('ASCII', VInt(7)), 'VInt(7)')

    def test_argument_errors(self):
        for args in ((), (1,), ('Nothing', 1), ('Add',), ('Add', 1),
                     ('Add', 1, 2, 3), ('Negative',), ('Negative', 1, 2),
                     ('Power', 1, 2), ('RichCompare', 1, 2),
                     ('AsSsize_t', 1), ('Hash',)):
            with self.subTest(args=args):
                with self.assertRaises((TypeError, ValueError)):
                    value_dispatch(*args)
        with self.assertRaises(ValueError):
            value_dispatch('Nothing', 1)
        with self.assertRaises(TypeError):
            value_dispatch('Add', 1)


# V2a-1a: value slots of int, float, bool, object and str

EDGE = sorted({0, 1, -1, 2**29, -2**29, 2**30, -2**30, 2**50 - 1, -2**50,
               2**50, -2**50 - 1, 2**51, -2**51, 2**59, -2**59, 2**60,
               -2**60, 2**62, -2**62, 2**63, -2**63, 2**64, -2**64,
               2**200, -2**200})

NB_CODES = {symbol: code
            for code, (name, symbol) in enumerate(opcode._nb_ops)}

SWEEP_OPS = (
    ('+', operator.add), ('-', operator.sub), ('*', operator.mul),
    ('//', operator.floordiv), ('%', operator.mod),
    ('/', operator.truediv), ('<<', operator.lshift),
    ('>>', operator.rshift), ('&', operator.and_), ('|', operator.or_),
    ('^', operator.xor), ('**', operator.pow),
)


class I(int):
    pass


def modular_inverse(a, m):
    """The inverse of a modulo m > 0 by the extended Euclid algorithm."""
    r0, r1 = m, a % m
    t0, t1 = 0, 1
    while r1:
        q = r0 // r1
        r0, r1 = r1, r0 - q * r1
        t0, t1 = t1, t0 - q * t1
    if r0 != 1:
        raise ValueError("base is not invertible for the given modulus")
    return t0 % m


def reference_pow(a, b, c):
    """pow(a, b, c) by square and multiply, with upstream's sign rules."""
    if c == 0:
        raise ValueError("pow() 3rd argument cannot be 0")
    m = abs(c)
    if b < 0:
        a = modular_inverse(a, m)
        b = -b
    a %= m
    result = 1 % m
    while b:
        if b & 1:
            result = result * a % m
        a = a * a % m
        b >>= 1
    if c < 0 and result:
        result += c
    return result


def reference_decimal(n):
    digits = []
    m = abs(n)
    while m:
        m, d = divmod(m, 10)
        digits.append('0123456789'[d])
    return ('-' if n < 0 else '') + (''.join(reversed(digits)) or '0')


def reference_hash(n):
    h = abs(n) % (2**61 - 1)
    if n < 0:
        h = -h
    return -2 if h == -1 else h


def outcome(function, *args):
    try:
        return ('value', function(*args))
    except Exception as exc:
        return ('error', type(exc))


class IntValueRowsTests(unittest.TestCase):
    def test_int_rows(self):
        # str is object's, inherited.
        self.assertEqual(len(INT_ROWS), 24)
        self.assertEqual(set_rows(int), set(INT_ROWS) | {'str'} | {'getattro', 'setattro'})
        for name in INT_ROWS:
            with self.subTest(row=name):
                self.assertNotEqual(object_slot(int, name), 0)

    def test_float_rows(self):
        self.assertEqual(len(FLOAT_ROWS), 17)
        self.assertEqual(set_rows(float), set(FLOAT_ROWS) | {'str'} | {'getattro', 'setattro'})
        for name in FLOAT_ROWS:
            with self.subTest(row=name):
                self.assertNotEqual(object_slot(float, name), 0)

    def test_bool_rows(self):
        # bool has its own four rows and inherits int's others, but not
        # repr: bool_repr has no value side. str is object's.
        self.assertEqual(set_rows(bool), set(INT_ROWS) - {'repr'} | {'str'} | {'getattro', 'setattro'})
        self.assertEqual(value_slot(bool, 'repr'), 0)
        for name in INT_ROWS:
            if name == 'repr':
                continue
            with self.subTest(row=name):
                if name in BOOL_OWN_ROWS:
                    self.assertNotEqual(value_slot(bool, name),
                                        value_slot(int, name))
                else:
                    self.assertEqual(value_slot(bool, name),
                                     value_slot(int, name))

    def test_object_rows(self):
        # hash predates V2a-1a; richcompare and str are its.
        self.assertEqual(set_rows(object), {'hash', 'richcompare', 'str', 'getattro', 'setattro'})

    def test_none_and_sentinel_inherit_richcompare(self):
        types = [type(None)]
        sentinel = getattr(builtins, 'sentinel', None)
        if sentinel is not None:
            types.append(sentinel)
        for t in types:
            with self.subTest(type=t):
                self.assertEqual(value_slot(t, 'richcompare'),
                                 value_slot(object, 'richcompare'))
                self.assertEqual(object_slot(t, 'richcompare'),
                                 object_slot(object, 'richcompare'))

    def test_str_rows(self):
        self.assertEqual(set_rows(str), {
            'richcompare', 'getattro', 'setattro', 'nb_remainder',
            'subscript', 'contains', 'sq_concat'})

    def test_container_rows(self):
        base = {'getattro', 'setattro', 'hash', 'str', 'richcompare'}
        self.assertEqual(set_rows(list) - base, {
            'subscript', 'ass_subscript', 'contains', 'item', 'ass_item',
            'sq_concat', 'sq_inplace_concat'})
        self.assertEqual(set_rows(tuple) - base, {
            'subscript', 'contains', 'item', 'sq_concat'})
        self.assertEqual(set_rows(dict) - base, {
            'subscript', 'ass_subscript', 'contains', 'nb_or',
            'nb_inplace_or'})
        self.assertEqual(set_rows(range) - base, {
            'subscript', 'item', 'contains'})
        set_ops = {'nb_subtract', 'nb_and', 'nb_xor', 'nb_or'}
        self.assertEqual(set_rows(frozenset) - base, {'contains'} | set_ops)
        self.assertEqual(set_rows(set) - base, {
            'contains', 'nb_inplace_subtract', 'nb_inplace_and',
            'nb_inplace_xor', 'nb_inplace_or'} | set_ops)
        for view in (type({}.keys()), type({}.items())):
            with self.subTest(view=view):
                self.assertEqual(set_rows(view) - base, {'contains'})

    def test_bytes_rows(self):
        base = {'getattro', 'setattro', 'hash', 'str', 'richcompare'}
        read = {'item', 'subscript', 'contains', 'nb_remainder', 'sq_concat'}
        self.assertEqual(set_rows(bytes) - base, read)
        self.assertEqual(set_rows(bytearray) - base, read | {
            'ass_item', 'ass_subscript', 'sq_inplace_concat'})
        self.assertEqual(set_rows(memoryview) - base, {
            'item', 'subscript', 'ass_subscript'})

    def test_iterator_rows(self):
        for it in (iter(range(3)), iter(range(2**70, 2**70 + 3)), iter(b''),
                   iter(bytearray()), iter(memoryview(b'')), reversed([]),
                   iter(lambda: 0, 1)):
            with self.subTest(type=type(it)):
                self.assertIn('iternext', set_rows(type(it)))

    def test_rare_richcompare_rows(self):
        # Class N: the slot yields NotImplemented for an inline operand.
        for t in (types.CellType, types.CodeType, slice, types.MethodType,
                  types.GenericAlias, types.UnionType, types.SimpleNamespace,
                  type({}.keys()), type({}.items()), range, list, tuple, set,
                  frozenset, dict, memoryview, complex):
            with self.subTest(type=t):
                self.assertTrue(value_slot(t, 'richcompare'))

    def test_union_nb_or_rows(self):
        for t in (type, types.GenericAlias, typing.TypeAliasType):
            with self.subTest(type=t):
                self.assertTrue(value_slot(t, 'nb_or'))

    def test_complex_rows(self):
        self.assertEqual(set_rows(complex) - {'getattro', 'setattro', 'str'}, {
            'richcompare', 'nb_add', 'nb_subtract', 'nb_multiply',
            'nb_true_divide', 'nb_power', 'nb_negative', 'nb_positive',
            'nb_absolute', 'nb_bool'})

    def test_python_subclass_inherits(self):
        self.assertEqual(int_slots(I), int_slots(int))


class IntParityTests(unittest.TestCase):
    """The operators (the value slots) against the span path."""

    def test_sweep(self):
        for symbol, op in SWEEP_OPS:
            code = NB_CODES[symbol]
            for a, b in itertools.product(EDGE, repeat=2):
                if symbol == '<<' and b > 10_000:
                    continue
                if symbol == '**' and abs(b) > 64 and a not in (0, 1, -1):
                    continue
                with self.subTest(op=symbol, a=a, b=b):
                    got = outcome(op, a, b)
                    expected = outcome(int_span_path, code, a, b)
                    self.assertEqual(got, expected)
                    if got[0] == 'value' and symbol != '/':
                        if isinstance(got[1], int):
                            self.assertIs(type(got[1]), int)

    def test_subclass_operands(self):
        for symbol, op in SWEEP_OPS:
            if symbol in ('<<', '**', '/'):
                continue
            with self.subTest(op=symbol):
                result = op(I(2**40), 3)
                self.assertIs(type(result), int)
                self.assertEqual(result, op(2**40, 3))

    def test_upstream_values(self):
        big = 2**70
        self.assertEqual(divmod(5, big), (0, 5))
        self.assertEqual(-5 % big, big - 5)
        self.assertEqual(pow(5, 1, big), 5)
        r = I(2**40) // 3
        self.assertIs(type(r), int)
        self.assertEqual(r, 366503875925)
        one = big - (big - 1)
        self.assertEqual(one, 1)

    def test_range_edges(self):
        minus_one = -1
        low = -2**50
        self.assertEqual(low // minus_one, 2**50)
        self.assertEqual(-low, 2**50)
        self.assertEqual(abs(low), 2**50)
        self.assertEqual(operator.neg(low), 2**50)
        self.assertEqual(1 << 1000, 2**1000)
        self.assertEqual((1 << 1000).bit_length(), 1001)
        self.assertEqual(minus_one >> 100, -1)
        self.assertEqual(1 >> 100, 0)

    def test_errors(self):
        for x in (0, 1, 2**50, 2**70):
            with self.subTest(x=x):
                with self.assertRaisesRegex(ValueError,
                                            'negative shift count'):
                    x << -1
                with self.assertRaisesRegex(ValueError,
                                            'negative shift count'):
                    x >> -1
                with self.assertRaisesRegex(ZeroDivisionError,
                                            'division by zero'):
                    x // 0
                with self.assertRaisesRegex(ZeroDivisionError,
                                            'division by zero'):
                    x % 0
                with self.assertRaisesRegex(ZeroDivisionError,
                                            'division by zero'):
                    divmod(x, 0)
        with self.assertRaises(ZeroDivisionError):
            pow(0, -1)
        with self.assertRaisesRegex(ValueError,
                                    r'pow\(\) 3rd argument cannot be 0'):
            pow(2, 3, 0)
        with self.assertRaisesRegex(ValueError,
                                    'base is not invertible for the given '
                                    'modulus'):
            pow(2, -1, 4)
        with self.assertRaises(OverflowError):
            5 << 2**70

    def test_modular_pow(self):
        self.assertEqual(pow(3, -1, 7), 5)
        self.assertEqual(pow(3, 5, -7), -2)
        self.assertEqual(pow(-3, 5, 7), 2)
        cases = [
            (3, 5, -7), (-3, 5, 7), (10**30, 10**20, 97),
            (3, 10**5, 2**61 - 1), (3, -1, 2**70),
            (2**70 + 1, -1, 2**65 + 5), (3, 5, 1), (0, 0, 7), (5, 0, 1),
            (2**64, 2**64, 2**59 + 1), (-2**64, 3, -(2**70)),
            (7, 10**18, 2**60 - 93), (7, 1000, 2**120 + 5),
        ]
        for a, b, c in cases:
            with self.subTest(a=a, b=b, c=c):
                self.assertEqual(pow(a, b, c), reference_pow(a, b, c))
        inverse = pow(2**70 + 1, -1, 2**65 + 5)
        self.assertEqual((inverse * (2**70 + 1)) % (2**65 + 5), 1)
        inverse = pow(3, -1, 2**70)
        self.assertEqual((inverse * 3) % 2**70, 1)

    def test_modular_pow_negative_base_wide_modulus(self):
        # A modulus of 31 to 60 bits with |a| as wide as the modulus or wider:
        # the remainder of the division has a's sign.
        self.assertEqual(pow(-2**50, 1, 2**31 + 1), 524288)
        self.assertEqual(pow(-2**31 - 5, 1, 2**31 + 1), 2147483645)
        bases = [1, 2**30 - 1, 2**31 + 5, 2**50, 2**60 - 1, 2**61 + 7,
                 2**90 + 12345]
        moduli = [2**30 + 1, 2**31 + 1, 2**45 + 9, 2**59 + 3, 2**60 - 93]
        for a in bases:
            for c in moduli:
                for sign_a, sign_c in itertools.product((1, -1), repeat=2):
                    for e in (1, 2, 5):
                        with self.subTest(a=sign_a * a, e=e, c=sign_c * c):
                            self.assertEqual(pow(sign_a * a, e, sign_c * c),
                                             reference_pow(sign_a * a, e,
                                                           sign_c * c))

    def test_lshift_huge_count(self):
        # A count that fits int64 but not the allocation fails as upstream.
        for n in (2**60, 2**61 + 5, 2**62):
            with self.subTest(n=n):
                with self.assertRaises(MemoryError):
                    1 << n
        for n in (2**63, 2**64, 2**90):
            with self.subTest(n=n):
                with self.assertRaises(OverflowError):
                    1 << n

    def test_bool(self):
        t = True
        five = 5
        self.assertEqual(t + five, 6)
        self.assertEqual(five & t, 1)
        self.assertIs(type(five & t), int)
        self.assertIs(t & t, True)
        self.assertIs(t | False, True)
        self.assertIs(t ^ t, False)
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(operator.invert(t), -2)
        self.assertTrue(t == 1)
        self.assertEqual(hash(t), 1)
        three = I(3)
        self.assertIs(type(three + 1), int)
        self.assertIs(type(1 + three), int)


class MixedTests(unittest.TestCase):
    def test_int_float(self):
        one = 1
        big = 2**60
        self.assertEqual(one + 2.5, 3.5)
        self.assertEqual(big + 2.5, float(big) + 2.5)
        with self.assertRaises(OverflowError):
            10**400 + 1.0
        two53 = 2**53
        self.assertFalse(two53 + 1 == float(two53))
        self.assertTrue(two53 == float(two53))
        self.assertTrue(two53 + 1 > float(two53))
        self.assertFalse(1000 < float('nan'))
        self.assertFalse(1000 > float('nan'))
        self.assertFalse(1000 == float('nan'))
        self.assertTrue(1000 != float('nan'))

    def test_reflected_compare(self):
        log = []

        class J(int):
            def __lt__(self, other):
                log.append('lt')
                return int.__lt__(self, other)

            def __gt__(self, other):
                log.append('gt')
                return int.__gt__(self, other)

        two53 = 2**53
        value = J(two53 + 1)
        # J.__lt__ runs first and gives NotImplemented; float's reflected
        # comparison then compares int(2.0**53) with the J itself, so J's
        # reflected __lt__ runs again, as upstream.
        self.assertFalse(value < float(two53))
        self.assertEqual(log, ['lt', 'lt'])
        log.clear()
        # float's compare runs first and reaches J's reflected __lt__ the
        # same way.
        self.assertFalse(float(two53) > value)
        self.assertEqual(log, ['lt'])

    def test_reflected_arithmetic(self):
        class F(float):
            def __radd__(self, other):
                return 'radd'

        self.assertEqual(1 + F(2.0), 'radd')
        self.assertEqual(2**60 + F(2.0), 'radd')

    def test_sorting_and_search(self):
        self.assertEqual(sorted([3, 1.5, 2]), [1.5, 2, 3])
        self.assertEqual([1, 2.5].index(2.5), 1)
        self.assertEqual(sorted([2**60, 1.5, -3, 2**50]),
                         [-3, 1.5, 2**50, 2**60])


class HashReprTests(unittest.TestCase):
    def test_int_hash(self):
        for x in EDGE:
            with self.subTest(x=x):
                self.assertEqual(hash(x), reference_hash(x))
                self.assertEqual(hash(I(x)), reference_hash(x))

    def test_hash_identities(self):
        self.assertEqual(hash(3), hash(3.0))
        self.assertEqual(hash(2**60), hash(float(2**60)))
        self.assertEqual(hash(-1), -2)
        self.assertEqual(hash(-2), -2)
        self.assertEqual(hash(2.5), hash(2.5))
        self.assertEqual(hash(-1.0), -2)
        self.assertEqual(hash(float('inf')), sys.hash_info.inf)

    def test_nan_hash_is_per_object(self):
        nans = [float('nan') for _ in range(4)]
        self.assertEqual(hash(nans[0]), hash(nans[0]))
        self.assertEqual(len({hash(n) for n in nans}), len(nans))

    def test_int_repr(self):
        for x in EDGE:
            with self.subTest(x=x):
                text = reference_decimal(x)
                self.assertEqual(repr(x), text)
                self.assertEqual(str(x), text)
                self.assertEqual(format(x), text)
                self.assertEqual('%d' % x, text)
                self.assertEqual(repr(I(x)), text)
        self.assertEqual(repr(True), 'True')

    def test_float_repr(self):
        for x, text in ((1.0, '1.0'), (-0.0, '-0.0'), (1e16, '1e+16'),
                        (float('nan'), 'nan'), (float('inf'), 'inf'),
                        (-float('inf'), '-inf'), (2.5, '2.5'),
                        (0.1, '0.1'), (1e-7, '1e-07'), (123456.0, '123456.0')):
            with self.subTest(x=x):
                self.assertEqual(repr(x), text)
                self.assertEqual(str(x), text)

    def test_container_repr(self):
        self.assertEqual(repr([1000, 2000]), '[1000, 2000]')
        self.assertEqual(repr(EDGE), '[' + ', '.join(
            reference_decimal(x) for x in EDGE) + ']')
        self.assertEqual(
            repr([1.0, -0.0, 1e16, float('nan'), float('inf'), 2.5]),
            '[1.0, -0.0, 1e+16, nan, inf, 2.5]')
        self.assertEqual(repr((1, 2.5, 2**60, None)),
                         '(1, 2.5, 1152921504606846976, None)')
        self.assertEqual(repr({1000: 2.5}), '{1000: 2.5}')


EXTRACT_NAMES = (
    'AsInt', 'AsLong', 'AsLongLong', 'AsLongAndOverflow',
    'AsLongLongAndOverflow', 'AsInt32', 'AsInt64', 'AsUInt32', 'AsUInt64',
    'IntAsUnsignedLong', 'IntAsUnsignedLongLong', 'AsUnsignedLongMask',
    'AsUnsignedLongLongMask', 'IntAsSsize_t', 'IntAsSize_t', 'IntAsDouble',
    'IntAsVoidPtr', 'AsDouble')


class HasIndex:
    def __index__(self):
        return 42


class HasFloat:
    def __float__(self):
        return 2.5


def extract_outcome(function, name, obj):
    try:
        return ('value', function(name, obj))
    except Exception as exc:
        return ('error', type(exc), str(exc))


class ExtractorTests(unittest.TestCase):
    VALUES = (
        [0, 1, -1, 2**31, -2**31 - 1, 2**50 - 1]
        + [2**50, 2**63 - 1, 2**63, 2**64, -2**63, -2**63 - 1, 2**64 - 1,
           2**100]
        + [1.5, HasIndex(), HasFloat(), 'a', True, I(7), None])

    def test_parity(self):
        for name in EXTRACT_NAMES:
            for value in self.VALUES:
                with self.subTest(row=name, value=value):
                    self.assertEqual(
                        extract_outcome(value_extract, name, value),
                        extract_outcome(object_extract, name, value))

    def test_inline_values(self):
        self.assertEqual(value_extract('AsLong', 2**50 - 1), 2**50 - 1)
        self.assertEqual(value_extract('AsLongAndOverflow', 2**63),
                         (-1, 1))
        self.assertEqual(value_extract('AsLongAndOverflow', -2**63 - 1),
                         (-1, -1))
        self.assertEqual(value_extract('AsDouble', 1.5), 1.5)
        self.assertEqual(value_extract('AsDouble', HasIndex()), 42.0)
        self.assertEqual(value_extract('AsDouble', HasFloat()), 2.5)

    def test_unknown_name(self):
        with self.assertRaises(ValueError):
            value_extract('Nothing', 1)
        with self.assertRaises(ValueError):
            object_extract('Nothing', 1)


class ConstructorTests(unittest.TestCase):
    def test_int64(self):
        for x, inline in ((0, True), (-2**50, True), (2**50 - 1, True),
                          (2**50, False), (-2**50 - 1, False),
                          (2**51, False), (-2**51, False),
                          (2**63 - 1, False), (-2**63, False)):
            with self.subTest(x=x):
                result, is_inline = value_construct('Int64', x)
                self.assertEqual(result, x)
                self.assertIs(type(result), int)
                self.assertIs(bool(is_inline), inline)

    def test_uint64(self):
        for x, inline in ((0, True), (5, True), (2**50 - 1, True),
                          (2**50, False), (2**63, False), (2**64 - 1, False)):
            with self.subTest(x=x):
                result, is_inline = value_construct('UInt64', x)
                self.assertEqual(result, x)
                self.assertIs(type(result), int)
                self.assertIs(bool(is_inline), inline)

    def test_double(self):
        result, is_inline = value_construct('Double', float('nan'))
        self.assertTrue(math.isnan(result))
        self.assertFalse(is_inline)
        for x in (1.5, -0.0, float('inf'), 1e300):
            with self.subTest(x=x):
                result, is_inline = value_construct('Double', x)
                self.assertEqual(result, x)
                self.assertEqual(math.copysign(1.0, result),
                                 math.copysign(1.0, x))
                self.assertTrue(is_inline)

    def test_bool(self):
        # The argument is a C long: 1 << 40 must not be cut to an int.
        result, is_inline = value_construct('Bool', 1 << 40)
        self.assertIs(result, True)
        self.assertFalse(is_inline)
        result, is_inline = value_construct('Bool', 0)
        self.assertIs(result, False)
        self.assertFalse(is_inline)

    def test_double_integral(self):
        result, is_inline = value_construct('DoubleIntegral', 3.0)
        self.assertEqual(result, 3)
        self.assertIs(type(result), int)
        self.assertTrue(is_inline)
        result, is_inline = value_construct('DoubleIntegral', 2.0**60)
        self.assertEqual(result, 2**60)
        self.assertFalse(is_inline)
        result, is_inline = value_construct('DoubleIntegral', -2.0**50)
        self.assertEqual(result, -2**50)
        self.assertTrue(is_inline)
        with self.assertRaises(OverflowError):
            value_construct('DoubleIntegral', float('inf'))
        with self.assertRaises(ValueError):
            value_construct('DoubleIntegral', float('nan'))

    def test_unknown_kind(self):
        with self.assertRaises(ValueError):
            value_construct('Nothing', 1)


class ObjectCompareTests(unittest.TestCase):
    def test_object_eq_ne(self):
        a = 1000
        self.assertIs(object.__eq__(a, 1000), True)
        self.assertIs(object.__eq__(a, 'a'), NotImplemented)
        self.assertIs(object.__ne__(a, 'a'), NotImplemented)
        self.assertIs(object.__ne__(a, 1000), False)

    def test_none_compare(self):
        five = 5
        self.assertIs(None == five, False)
        self.assertIs(None != five, True)
        self.assertIs(None == None, True)


# M1b: the iteration contract and the object APIs over value slots
# (pyjit/design/proposals/value-api-m1b-m2.md, sections 1.2 and 1.6). Each
# expectation is upstream CPython's behaviour, which the step preserves.

CIter = _testinternalcapi.CIter
CIterBare = _testinternalcapi.CIterBare
MapOnly = _testinternalcapi.MapOnly
SeqOnly = _testinternalcapi.SeqOnly
VMap = _testinternalcapi.VMap
VItems = _testinternalcapi.VItems
iternext_call = _testinternalcapi.iternext_call
iter_send = _testinternalcapi.iter_send
set_key_error_value = _testinternalcapi.set_key_error_value

PYGEN_RETURN = 0
PYGEN_NEXT = 1


def returns_five():
    return 5
    yield


def returns_none():
    return None
    yield


def yields_then_returns_seven():
    yield
    return 7


def echo():
    x = yield
    while True:
        x = yield x


def primed(gen):
    next(gen)
    return gen


def stop_iteration_of(func, *args):
    """The StopIteration that func(*args) raises."""
    with unittest.TestCase().assertRaises(StopIteration) as cm:
        func(*args)
    return cm.exception


class StopValueTests(unittest.TestCase):
    """R16: a generator's return value reaches next()'s caller."""

    wrappers = {
        'map': lambda it: map(str, it),
        'filter': lambda it: filter(None, it),
        'zip first': lambda it: zip(it),
        'zip first of two': lambda it: zip(it, iter([1])),
        'zip later': lambda it: zip(iter([1]), it),
        'enumerate': enumerate,
    }

    def test_return_value(self):
        for name, wrap in self.wrappers.items():
            with self.subTest(name):
                exc = stop_iteration_of(next, wrap(returns_five()))
                self.assertEqual(exc.value, 5)
                self.assertEqual(exc.args, (5,))

    def test_return_none(self):
        for name, wrap in self.wrappers.items():
            with self.subTest(name):
                exc = stop_iteration_of(next, wrap(returns_none()))
                self.assertIsNone(exc.value)
                self.assertEqual(exc.args, ())

    def test_consumers_end_quietly(self):
        for name, wrap in self.wrappers.items():
            with self.subTest(name):
                self.assertEqual(list(wrap(returns_five())), [])
                self.assertEqual(list(wrap(returns_none())), [])
                self.assertEqual(next(wrap(returns_five()), 'd'), 'd')

    def test_strict(self):
        # An exhausted first argument with an empty second one is equal
        # length: the generator's value is dropped and the end is bare.
        exc = stop_iteration_of(next, zip(returns_five(), [], strict=True))
        self.assertEqual(exc.args, ())
        exc = stop_iteration_of(next, zip([], returns_five(), strict=True))
        self.assertEqual(exc.args, ())
        exc = stop_iteration_of(next, zip(returns_none(), [], strict=True))
        self.assertEqual(exc.args, ())

    def test_strict_lengths(self):
        with self.assertRaisesRegex(
                ValueError, r'zip\(\) argument 2 is shorter than argument 1'):
            next(zip([1], returns_five(), strict=True))
        with self.assertRaisesRegex(
                ValueError, r'zip\(\) argument 2 is longer than argument 1'):
            next(zip(returns_five(), [1], strict=True))
        with self.assertRaisesRegex(
                ValueError, r'map\(\) argument 2 is longer than argument 1'):
            next(map(str, returns_five(), [1], strict=True))
        with self.assertRaisesRegex(
                ValueError, r'map\(\) argument 2 is shorter than argument 1'):
            next(map(str, [1], returns_five(), strict=True))
        exc = stop_iteration_of(
            next, map(str, returns_five(), [], strict=True))
        self.assertEqual(exc.args, ())


class CIteratorTests(unittest.TestCase):
    """Iterators that only have an object tp_iternext."""

    def test_iter(self):
        for args in (('stop', 7), ('null',), ('error',)):
            with self.subTest(args):
                it = CIter(*args)
                self.assertIs(iter(it), it)

    def test_without_tp_iter(self):
        it = CIterBare('stop', 7)
        self.assertEqual(stop_iteration_of(next, it).value, 7)
        self.assertEqual(next(CIterBare('null'), 'd'), 'd')
        with self.assertRaisesRegex(TypeError, 'is not iterable'):
            iter(it)
        with self.assertRaisesRegex(TypeError, 'is not iterable'):
            list(it)

    def test_stop_iteration_value(self):
        exc = stop_iteration_of(next, CIter('stop', 7))
        self.assertEqual(exc.value, 7)
        self.assertEqual(exc.args, (7,))
        exc = stop_iteration_of(next, CIter('stop'))
        self.assertIsNone(exc.value)
        self.assertEqual(exc.args, ())

    def test_null_without_exception(self):
        exc = stop_iteration_of(next, CIter('null'))
        self.assertIsNone(exc.value)
        self.assertEqual(exc.args, ())

    def test_next_default(self):
        sentinel = object()
        self.assertIs(next(CIter('stop', 7), sentinel), sentinel)
        self.assertIs(next(CIter('null'), sentinel), sentinel)
        with self.assertRaises(ValueError):
            next(CIter('error'), sentinel)

    def test_for_and_list(self):
        for args in (('stop', 7), ('null',)):
            with self.subTest(args):
                self.assertEqual(list(CIter(*args)), [])
                self.assertEqual([x for x in CIter(*args)], [])
                items = []
                for x in CIter(*args):
                    items.append(x)
                self.assertEqual(items, [])
        for func in (list, tuple, sum):
            with self.subTest(func):
                with self.assertRaises(ValueError):
                    func(CIter('error'))
        with self.assertRaises(ValueError):
            for x in CIter('error'):
                pass

    def test_yield_from(self):
        def delegate(it):
            result = yield from it
            return result

        for args, expected in ((('stop', 7), 7), (('null',), None),
                               (('stop',), None)):
            with self.subTest(args):
                gen = delegate(CIter(*args))
                exc = stop_iteration_of(next, gen)
                self.assertEqual(exc.value, expected)
        with self.assertRaises(ValueError):
            next(delegate(CIter('error')))

    def test_zip_and_map(self):
        for wrap in (lambda it: zip(it), lambda it: zip([1], it),
                     lambda it: map(str, it), lambda it: filter(None, it),
                     enumerate):
            with self.subTest(wrap):
                self.assertEqual(
                    stop_iteration_of(next, wrap(CIter('stop', 7))).value, 7)
                exc = stop_iteration_of(next, wrap(CIter('null')))
                self.assertEqual(exc.args, ())
                self.assertEqual(list(wrap(CIter('stop', 7))), [])
                with self.assertRaises(ValueError):
                    next(wrap(CIter('error')))
        # strict mode drops the value, as it drops a generator's
        exc = stop_iteration_of(
            next, zip(CIter('stop', 7), [], strict=True))
        self.assertEqual(exc.args, ())
        with self.assertRaises(ValueError):
            next(zip(CIter('error'), [], strict=True))

    def test_iternext_from_c(self):
        item, exc = iternext_call(CIter('stop', 7))
        self.assertIsNone(item)
        self.assertIs(type(exc), StopIteration)
        self.assertEqual(exc.value, 7)
        self.assertEqual(iternext_call(CIter('null')), (None, None))
        item, exc = iternext_call(CIter('error'))
        self.assertIs(type(exc), ValueError)

    def test_send_from_c(self):
        self.assertEqual(iter_send(CIter('stop', 7), None),
                         (PYGEN_RETURN, 7))
        self.assertEqual(iter_send(CIter('null'), None),
                         (PYGEN_RETURN, None))
        self.assertEqual(iter_send(iter([4, 5]), None), (PYGEN_NEXT, 4))
        exhausted = iter([])
        self.assertEqual(iter_send(exhausted, None), (PYGEN_RETURN, None))
        with self.assertRaises(ValueError):
            iter_send(CIter('error'), None)
        with self.assertRaises(AttributeError):
            iter_send(iter([4, 5]), 1000)


class PythonNextTests(unittest.TestCase):
    def test_stop_iteration_reaches_caller(self):
        class Stops:
            def __init__(self, exc):
                self.exc = exc

            def __iter__(self):
                return self

            def __next__(self):
                raise self.exc

        class MyStop(StopIteration):
            pass

        for exc in (StopIteration(7), MyStop(7)):
            with self.subTest(type(exc)):
                with self.assertRaises(StopIteration) as cm:
                    next(Stops(exc))
                self.assertIs(cm.exception, exc)
                self.assertIs(type(cm.exception), type(exc))
                self.assertEqual(cm.exception.value, 7)
                with self.assertRaises(StopIteration) as cm:
                    next(zip(Stops(exc)))
                self.assertIs(cm.exception, exc)
                self.assertEqual(next(Stops(exc), 'd'), 'd')
                self.assertEqual(list(Stops(exc)), [])

    def test_next_is_none(self):
        class X:
            __next__ = None

        class Y:
            def __iter__(self):
                return self
            __next__ = None

        with self.assertRaisesRegex(TypeError,
                                    "'NoneType' object is not callable"):
            next(X())
        with self.assertRaisesRegex(TypeError,
                                    "'X' object is not iterable"):
            iter(X())
        with self.assertRaisesRegex(TypeError,
                                    "'NoneType' object is not callable"):
            list(Y())

    def test_class_without_next_is_not_an_iterator(self):
        # PyIter_Check, reached through iter(): the pair table gives a
        # class with no __next__ the NextNotImplemented value side.
        class Y:
            pass

        class X:
            def __iter__(self):
                return Y()

        with self.assertRaisesRegex(TypeError,
                                    "must return an iterator, not .*Y"):
            iter(X())

    def test_next_added_later(self):
        class Z:
            pass

        z = Z()
        with self.assertRaisesRegex(TypeError,
                                    "'Z' object is not an iterator"):
            next(z)
        Z.__next__ = lambda self: 7
        self.assertEqual(next(z), 7)
        self.assertEqual(next(Z()), 7)

    def test_next_added_to_base(self):
        class Base:
            pass

        class Sub(Base):
            pass

        sub = Sub()
        with self.assertRaisesRegex(TypeError,
                                    "'Sub' object is not an iterator"):
            next(sub)
        Base.__next__ = lambda self: 8
        self.assertEqual(next(sub), 8)
        self.assertEqual(next(Base()), 8)
        self.assertEqual(next(Sub()), 8)


class SendAndGeneratorFromCTests(unittest.TestCase):
    def test_generator_iternext(self):
        self.assertEqual(iternext_call(returns_none()), (None, None))
        item, exc = iternext_call(returns_five())
        self.assertIsNone(item)
        self.assertIs(type(exc), StopIteration)
        self.assertEqual(exc.args, (5,))
        self.assertEqual(iternext_call(primed(echo())), (None, None))

        def one():
            yield 1
        self.assertEqual(iternext_call(one()), (1, None))

    def test_send(self):
        gen = primed(echo())
        self.assertEqual(iter_send(gen, 1000), (PYGEN_NEXT, 1000))
        self.assertEqual(iter_send(gen, None), (PYGEN_NEXT, None))
        self.assertEqual(iter_send(gen, 2.5), (PYGEN_NEXT, 2.5))

    def test_send_just_started(self):
        with self.assertRaisesRegex(
                TypeError, "can't send non-None value to a just-started"):
            iter_send(echo(), 1000)
        self.assertEqual(iter_send(echo(), None), (PYGEN_NEXT, None))

    def test_send_return(self):
        for arg in (1000, None):
            with self.subTest(arg):
                gen = primed(yields_then_returns_seven())
                self.assertEqual(iter_send(gen, arg), (PYGEN_RETURN, 7))
        self.assertEqual(iter_send(returns_five(), None), (PYGEN_RETURN, 5))
        self.assertEqual(iter_send(returns_none(), None),
                         (PYGEN_RETURN, None))

    def test_async_generator_asend(self):
        async def yields():
            yield 1

        async def ends():
            return
            yield

        # The yielded value travels as the awaitable's StopIteration value
        self.assertEqual(list(yields().asend(None)), [])
        with self.assertRaises(StopAsyncIteration):
            list(ends().asend(None))


class ForLoopMonitoringTests(unittest.TestCase):
    """The end of a for loop over a generator, as FOR_ITER reports it in
    this tree (value-api-m1b-m2.md, section 1.2, FOR_ITER row): one RAISE
    event with StopIteration(v) for a return value, none for a None return.
    The base tree already does this; upstream's FOR_ITER_GEN path reports
    STOP_ITERATION at END_FOR instead, which the register IR does not take
    here."""

    def record(self, gen_func):
        monitoring = sys.monitoring
        events = monitoring.events
        tool = next(i for i in range(6) if monitoring.get_tool(i) is None)
        seen = []

        def on_raise(code, offset, exc):
            seen.append(('raise', type(exc), exc.value))

        def on_stop(code, offset, exc):
            seen.append(('stop', type(exc), exc.value))

        def loop():
            for _ in gen_func():
                pass

        loop()
        monitoring.use_tool_id(tool, 'test_value_slots')
        try:
            monitoring.register_callback(tool, events.RAISE, on_raise)
            monitoring.register_callback(tool, events.STOP_ITERATION, on_stop)
            monitoring.set_events(tool, events.RAISE | events.STOP_ITERATION)
            loop()
            monitoring.set_events(tool, 0)
        finally:
            monitoring.set_events(tool, 0)
            monitoring.register_callback(tool, events.RAISE, None)
            monitoring.register_callback(tool, events.STOP_ITERATION, None)
            monitoring.free_tool_id(tool)
        return seen

    def test_return_value(self):
        def gen():
            yield 1
            return 5

        self.assertEqual(self.record(gen), [('raise', StopIteration, 5)])

    def test_return_none(self):
        def gen():
            yield 1
            return None

        self.assertEqual(self.record(gen), [])


class UnsupportedOperandTests(unittest.TestCase):
    def test_object_errors(self):
        five = 5
        one = 1
        number = 2.5
        with self.assertRaisesRegex(TypeError,
                                    "'int' object is not subscriptable"):
            five[0]
        with self.assertRaisesRegex(TypeError,
                                    "object of type 'int' has no len"):
            len(five)
        with self.assertRaisesRegex(TypeError,
                                    "'float' object is not iterable"):
            iter(number)
        with self.assertRaisesRegex(
                TypeError,
                "argument of type 'int' is not a container or iterable"):
            one in five
        with self.assertRaisesRegex(TypeError,
                                    "'int' object is not an iterator"):
            next(five)

    def test_mapping_only(self):
        obj = MapOnly()
        two_pow_70 = 2**70
        # mp_subscript sees the key as given: no index conversion
        self.assertEqual(obj[-1], ('mp', -1))
        self.assertEqual(obj[1000], ('mp', 1000))
        self.assertEqual(obj[two_pow_70], ('mp', two_pow_70))
        self.assertEqual(obj['a'], ('mp', 'a'))
        with self.assertRaisesRegex(TypeError, 'has no len'):
            len(obj)

    def test_sequence_only(self):
        obj = SeqOnly()
        two_pow_70 = 2**70
        one_float = 1.0
        self.assertEqual(len(obj), 10)
        # A negative index is adjusted by sq_length
        self.assertEqual(obj[-1], ('sq', 9))
        self.assertEqual(obj[-10], ('sq', 0))
        self.assertEqual(obj[-11], ('sq', -1))
        self.assertEqual(obj[1], ('sq', 1))
        self.assertEqual(obj[1000], ('sq', 1000))
        with self.assertRaisesRegex(
                IndexError,
                "cannot fit 'int' into an index-sized integer"):
            obj[two_pow_70]
        with self.assertRaisesRegex(
                TypeError, "sequence index must be integer, not 'str'"):
            obj['a']
        with self.assertRaisesRegex(
                TypeError, "sequence index must be integer, not 'float'"):
            obj[one_float]


class ItemValueSlotTests(unittest.TestCase):
    """The value subscript, assignment, item and contains slots.

    VMap's two sides log their own names; VItems's object slots are the
    thunks of its value slots. A key reaches the value slot as a word: an
    int or float inline.
    """

    def setUp(self):
        self.log = []
        set_logs(self, self.log)

    def test_subscript_takes_the_word(self):
        m = VMap()
        big = 2**70
        for key, inline in ((5, True), (2.5, True), (big, False),
                            ('a', False)):
            with self.subTest(key=key):
                self.assertEqual(m[key], ('subscript', key, inline))
                self.assertEqual(operator.getitem(m, key),
                                 ('subscript', key, inline))
        self.assertEqual(self.log, ['VMap.subscript.value'] * 8)

    def test_assignment_deletion_and_contains(self):
        m = VMap()
        m[1] = 'one'
        m[2.5] = 7
        self.assertEqual(m.store, {1: 'one', 2.5: 7})
        self.assertIn(1, m)
        self.assertNotIn(3, m)
        del m[1]
        self.assertEqual(m.store, {2.5: 7})
        with self.assertRaises(KeyError) as cm:
            del m[1]
        self.assertEqual(cm.exception.args, (1,))
        self.assertEqual(self.log,
                         ['VMap.ass_subscript.value'] * 2
                         + ['VMap.contains.value'] * 2
                         + ['VMap.ass_subscript.value'] * 2)

    def test_mapping_wrappers(self):
        # The slot wrappers of these families call the value side.
        m = VMap()
        self.assertEqual(VMap.__getitem__(m, 5), ('subscript', 5, True))
        VMap.__setitem__(m, 5, 'x')
        self.assertTrue(VMap.__contains__(m, 5))
        VMap.__delitem__(m, 5)
        self.assertEqual(m.store, {})
        self.assertEqual(self.log, ['VMap.subscript.value',
                                    'VMap.ass_subscript.value',
                                    'VMap.contains.value',
                                    'VMap.ass_subscript.value'])

    def test_c_api(self):
        m = VMap()
        self.assertEqual(_testcapi.mapping_getoptionalitem(m, 5),
                         ('subscript', 5, True))
        s = VItems()
        s[1] = 'one'
        _testlimitedcapi = import_helper.import_module('_testlimitedcapi')
        self.assertEqual(_testlimitedcapi.sequence_getitem(s, -1),
                         ('item', 9))
        self.assertEqual(self.log, ['VMap.subscript.value', 'VItems.ass_item',
                                    'VItems.item'])

    def test_item_index(self):
        s = VItems()
        # A negative index is adjusted by sq_length before the value slot.
        self.assertEqual(s[-1], ('item', 9))
        self.assertEqual(s[3], ('item', 3))
        with self.assertRaisesRegex(IndexError, 'VItems index out of range'):
            s[10]
        with self.assertRaisesRegex(
                IndexError, "cannot fit 'int' into an index-sized integer"):
            s[2**70]
        with self.assertRaisesRegex(
                TypeError, "sequence index must be integer, not 'str'"):
            s['a']
        self.assertEqual(self.log, ['VItems.item'] * 3)

    def test_item_assignment_and_wrappers(self):
        s = VItems()
        s[-1] = 'last'
        s[0] = 'first'
        del s[0]
        self.assertEqual(s.store, {9: 'last'})
        self.assertIn(9, s)
        self.assertEqual(VItems.__getitem__(s, -2), ('item', 8))
        VItems.__setitem__(s, -2, 'x')
        VItems.__delitem__(s, -1)
        self.assertEqual(s.store, {8: 'x'})
        self.assertEqual(self.log,
                         ['VItems.ass_item'] * 3 + ['VItems.contains',
                          'VItems.item'] + ['VItems.ass_item'] * 2)

    def test_python_subclass(self):
        class Inherit(VMap):
            pass

        class Override(VMap):
            def __getitem__(self, key):
                return ('py', key)

            def __contains__(self, item):
                return item == 'py'

        self.assertEqual(Inherit()[5], ('subscript', 5, True))
        self.assertEqual(Override()[5], ('py', 5))
        self.assertIn('py', Override())
        self.assertNotIn(5, Override())
        self.assertEqual(self.log, ['VMap.subscript.value'])


class ThinFormTests(unittest.TestCase):
    def test_slot_wrappers(self):
        t = (1, 2.5, 3)
        self.assertEqual(tuple.__getitem__(t, -1), 3)
        self.assertEqual(tuple.__getitem__(t, 1), 2.5)
        with self.assertRaises(IndexError):
            tuple.__getitem__(t, 3)
        self.assertIs(tuple.__contains__(t, 2.5), True)
        self.assertIs(tuple.__contains__(t, 2.0), False)
        self.assertIs(tuple.__contains__(t, 3.0), True)
        self.assertEqual(str.__getitem__('abc', 1), 'b')
        with self.assertRaisesRegex(IndexError,
                                    'range object index out of range'):
            range.__getitem__(range(10), 1000)
        self.assertEqual(range.__getitem__(range(10), -1), 9)

    def test_key_error_value(self):
        for key in (1000, (1, 2), 'a', 2.5, None):
            with self.subTest(key):
                with self.assertRaises(KeyError) as cm:
                    set_key_error_value(key)
                self.assertEqual(cm.exception.args, (key,))
                self.assertIs(cm.exception.args[0], key)


def _box_kind_counters(func):
    """The Py_STATS counters of one call func(), by line name."""
    func()
    # The dump directory of this process (Python/pystats.c); it may be shared,
    # and holds markers beside the dumps, so take this process's dump.
    stats_dir = os.environ.get('PYTHON_PYSTATS_DIR') or '/tmp/py_stats'
    before = set(os.listdir(stats_dir)) if os.path.isdir(stats_dir) else set()
    sys._stats_clear()
    sys._stats_on()
    func()
    sys._stats_off()
    sys._stats_dump()
    mine = f'Stats process: pid={os.getpid()} '
    new = []
    for name in set(os.listdir(stats_dir)) - before:
        if not name.endswith('.txt'):
            continue
        with open(os.path.join(stats_dir, name)) as stream:
            if stream.readline().startswith(mine):
                new.append(name)
    assert len(new) == 1, new
    path = os.path.join(stats_dir, new.pop())
    counters = {}
    with open(path) as stream:
        for line in stream:
            name, sep, value = line.strip().rpartition(':')
            if sep and value.strip().isdigit():
                counters[name.strip()] = int(value)
    os.unlink(path)
    return counters


@unittest.skipUnless(hasattr(sys, '_stats_on'), 'needs a Py_STATS build')
class IterationBoxTests(unittest.TestCase):
    KINDS = ('Boxing by kind (slot adapter)', 'Boxing by kind (thunk)')

    def extra_boxes(self, func):
        counters = _box_kind_counters(func)
        base = _box_kind_counters(lambda: None)
        for kind in self.KINDS:
            self.assertIn(kind, counters)
        return {kind: counters[kind] - base[kind] for kind in self.KINDS}

    def test_enumerate_and_zip_of_floats(self):
        floats = [i + 0.5 for i in range(200)]

        def enumerated():
            total = 0
            for i, x in enumerate(floats):
                total += i
            return total

        def zipped():
            total = 0.0
            for a, b in zip(floats, floats):
                total += a
            return total

        for func in (enumerated, zipped):
            with self.subTest(func.__name__):
                self.assertEqual(self.extra_boxes(func),
                                 {kind: 0 for kind in self.KINDS})


# M2: attributes with value receivers (value-api-m1b-m2.md, section 2.8).
# A NULL slot reads as 0 through value_slot() and object_slot().

INLINE_READS = (
    (1, 'real', 1),
    (2.5, 'imag', 0.0),
    (1000, 'numerator', 1000),
    (2**60, 'denominator', 1),
)


class AttributeReadTests(unittest.TestCase):
    def test_python_reads(self):
        self.assertEqual((1).real, 1)
        self.assertEqual((2.5).imag, 0.0)
        self.assertEqual((1000).numerator, 1000)
        self.assertEqual((2**60).denominator, 1)
        self.assertIs((1).__class__, int)
        self.assertIs(type(True.real), int)
        self.assertEqual(True.real, 1)

    def test_float_real_identity(self):
        # A heap float is its own real part; no box and no copy.
        for x in (2.5, float('nan'), 1e300, -0.0):
            self.assertIs(x.real, x)
        y = float('1e300') * 1.0
        self.assertIs(y.real, y)

    def test_explicit_getattribute(self):
        self.assertEqual(object.__getattribute__(1000, 'real'), 1000)
        self.assertIs(type(object.__getattribute__(1000, 'real')), int)
        self.assertIs(type.__getattribute__(int, 'real'), int.real)

    def test_value_getattr(self):
        for obj, name, expected in INLINE_READS:
            with self.subTest(obj=obj, name=name):
                result, was_inline = value_getattr(obj, name)
                self.assertEqual(result, expected)
                self.assertIs(type(result), type(expected))
                self.assertTrue(was_inline)

    def test_value_getattr_class_and_bool(self):
        self.assertEqual(value_getattr(1, '__class__'), (int, False))
        result, _ = value_getattr(True, 'real')
        self.assertEqual(result, 1)
        self.assertIs(type(result), int)
        self.assertIs(value_getattr(int, 'real')[0], int.real)

    def test_value_getattr_method(self):
        method, was_inline = value_getattr(5, 'bit_length')
        self.assertFalse(was_inline)
        self.assertEqual(method(), 3)

    def test_getset_descriptor_type(self):
        self.assertIs(type(int.real), types.GetSetDescriptorType)

    def test_class_attribute_1000(self):
        class C:
            x = 1000
        self.assertEqual(C().x, 1000)
        self.assertEqual(C.x, 1000)
        self.assertEqual(getattr(C(), 'x'), 1000)
        self.assertEqual(getattr(C(), 'y', 7), 7)
        self.assertEqual(value_getattr(C(), 'x'), (1000, True))
        self.assertEqual(value_getattr(C, 'x'), (1000, True))
        self.assertEqual(value_getoptionalattr(C(), 'y'), (0, None))


class AttributeErrorTests(unittest.TestCase):
    def test_missing_attribute(self):
        with self.assertRaises(AttributeError) as cm:
            (1).foo
        self.assertEqual(cm.exception.name, 'foo')
        self.assertEqual(cm.exception.obj, 1)
        self.assertEqual(str(cm.exception),
                         "'int' object has no attribute 'foo'")

    def test_value_getattr_missing(self):
        with self.assertRaises(AttributeError) as cm:
            value_getattr(1, 'foo')
        self.assertEqual(cm.exception.name, 'foo')
        self.assertEqual(cm.exception.obj, 1)

    def test_getoptionalattr(self):
        self.assertEqual(value_getoptionalattr(1, 'foo'), (0, None))
        self.assertEqual(value_getoptionalattr(1, 'real'), (1, 1))
        self.assertEqual(value_getoptionalattr(2.5, 'imag'), (1, 0.0))

    def test_non_str_name(self):
        with self.assertRaises(TypeError):
            value_getattr(1, 5)
        with self.assertRaises(TypeError):
            value_setattr(1, 5, 2)

    def test_set_read_only(self):
        message = "attribute 'real' of 'int' objects is not writable"
        with self.assertRaisesRegex(AttributeError, message):
            setattr(1, 'real', 2)
        with self.assertRaisesRegex(AttributeError, message):
            value_setattr(1, 'real', 2)

    def test_delete_read_only(self):
        message = "attribute 'real' of 'int' objects is not writable"
        with self.assertRaisesRegex(AttributeError, message):
            del (1).real
        with self.assertRaisesRegex(AttributeError, message):
            value_delattr(1, 'real')

    def test_set_new_attribute(self):
        message = ("'int' object has no attribute 'foo' and no __dict__ "
                   "for setting new attributes")
        with self.assertRaisesRegex(AttributeError, message) as cm:
            setattr(1, 'foo', 2)
        self.assertEqual(cm.exception.name, 'foo')
        self.assertEqual(cm.exception.obj, 1)
        with self.assertRaisesRegex(AttributeError, message):
            value_setattr(1, 'foo', 2)

    def test_assign_class(self):
        message = ("__class__ assignment only supported for mutable types "
                   "or ModuleType subclasses")
        with self.assertRaisesRegex(TypeError, message):
            (1).__class__ = bool
        with self.assertRaisesRegex(TypeError, message):
            value_setattr(1, '__class__', bool)


class ObjGetAttrTests(unittest.TestCase):
    def test_python_round_trip(self):
        o = ObjGetAttr()
        setattr(o, 'x', 1000)
        self.assertEqual(o.x, 1000)
        self.assertEqual(getattr(o, 'x'), 1000)

    def test_value_round_trip(self):
        o = ObjGetAttr()
        value_setattr(o, 'x', 1000)
        self.assertEqual(value_getattr(o, 'x')[0], 1000)
        self.assertEqual(o.x, 1000)

    def test_missing_and_delete(self):
        o = ObjGetAttr()
        with self.assertRaises(AttributeError):
            o.x
        self.assertEqual(value_getoptionalattr(o, 'x'), (0, None))
        o.x = 1
        del o.x
        with self.assertRaises(AttributeError):
            o.x
        value_setattr(o, 'x', 2.5)
        value_delattr(o, 'x')
        with self.assertRaises(AttributeError):
            o.x

    def test_getset_through_adapter(self):
        o = ObjGetAttr()
        o.g = 'a'
        self.assertEqual(o.g, 'a')
        value_setattr(o, 'g', 1000)
        self.assertEqual(o.g, 1000)
        self.assertEqual(value_getattr(o, 'g')[0], 1000)
        o.g = 2.5
        self.assertEqual(value_getattr(o, 'g')[0], 2.5)

    def test_getset_error(self):
        o = ObjGetAttr()
        with self.assertRaises(AttributeError):
            o.g
        with self.assertRaises(AttributeError):
            del o.g


class ValueGetSetTests(unittest.TestCase):
    # VGetSet.x is a _Py_VALUE_GETSET; the value starts as the inline 1000
    # and a delete raises AttributeError.

    def read_all(self, o):
        descr = VGetSet.__dict__['x']
        return (o.x, value_getattr(o, 'x')[0], getset_direct_get(descr, o),
                descr.__get__(o))

    def test_descriptor_type(self):
        self.assertIs(type(VGetSet.__dict__['x']), types.GetSetDescriptorType)

    def test_initial_value_each_way(self):
        o = VGetSet()
        self.assertEqual(self.read_all(o), (1000,) * 4)
        self.assertEqual(value_getattr(o, 'x'), (1000, True))

    def test_write_from_python(self):
        for new in (7, 2.5, 2**70, 'text', None):
            with self.subTest(new=new):
                o = VGetSet()
                o.x = new
                self.assertEqual(self.read_all(o), (new,) * 4)

    def test_write_through_value_setattr(self):
        # Inline ints and floats, a boxed int and objects.
        for new in (7, 2.5, 2**70, 'text', None):
            with self.subTest(new=new):
                o = VGetSet()
                value_setattr(o, 'x', new)
                self.assertEqual(self.read_all(o), (new,) * 4)

    def test_read_inline_result(self):
        o = VGetSet()
        value_setattr(o, 'x', 2.5)
        self.assertEqual(value_getattr(o, 'x'), (2.5, True))
        value_setattr(o, 'x', 2**70)
        self.assertEqual(value_getattr(o, 'x'), (2**70, False))

    def test_delete(self):
        o = VGetSet()
        with self.assertRaises(AttributeError):
            del o.x
        with self.assertRaises(AttributeError):
            value_delattr(o, 'x')
        self.assertEqual(o.x, 1000)

    def test_wrong_receiver(self):
        descr = VGetSet.__dict__['x']
        with self.assertRaises(TypeError):
            descr.__get__(1000)


class SuperTests(unittest.TestCase):
    def test_super_int(self):
        self.assertEqual(super(int, 5).__self__, 5)
        self.assertRegex(super(int, 5).__repr__(),
                         r'^<int object at 0x[0-9a-f]+>$')
        self.assertEqual(super(float, 2.5).__sizeof__(), (2.5).__sizeof__())
        self.assertIs(super(int, 5).__thisclass__, int)

    def test_super_unrelated_zero_argument(self):
        class C:
            def f(self):
                return super().__repr__()
        message = (r"super\(type, obj\): obj \(instance of int\) is not an "
                   r"instance or subtype of type \(C\)")
        with self.assertRaisesRegex(TypeError, message):
            C.f(5)

    def test_super_subclass(self):
        class MySuper(super):
            pass
        s = MySuper(int, 5)
        self.assertEqual(s.__self__, 5)
        self.assertIs(s.__thisclass__, int)
        self.assertRegex(s.__repr__(), r'^<int object at 0x[0-9a-f]+>$')

    def test_super_classmethod(self):
        class B:
            @classmethod
            def m(cls):
                return super().__thisclass__, super().__self_class__
        self.assertEqual(B.m(), (B, B))

    def test_self_captured_by_lambda(self):
        class M(int):
            def f(self):
                get_self = lambda: self
                return get_self(), super().__sizeof__()
        m = M(5)
        result, size = M.f(m)
        self.assertIs(result, m)
        self.assertEqual(size, int.__sizeof__(m))
        with self.assertRaises(TypeError):
            M.f(5)

    def test_rebound_class_cell(self):
        class A:
            def f(self):
                return super().__repr__()
        index = A.f.__code__.co_freevars.index('__class__')
        A.f.__closure__[index].cell_contents = object
        # Nothing follows object in the MRO, so the lookup falls back to
        # the attributes of the super object itself.
        self.assertEqual(A.f(5), "<super: <class 'object'>, <int object>>")
        self.assertEqual(A.f(2.5), "<super: <class 'object'>, <float object>>")


def getattro_pair_names(cls):
    names = {address: name for name, address in value_functions().items()}
    return (names.get(object_slot(cls, 'getattro')),
            names.get(value_slot(cls, 'getattro')))


class GetAttroPairTests(unittest.TestCase):
    def test_plain_class(self):
        functions = value_functions()
        class Plain:
            pass
        self.assertEqual(value_slot(Plain, 'getattro'),
                         functions['_PyObject_GenericGetAttrValue'])
        self.assertEqual(value_slot(Plain, 'setattro'),
                         functions['_PyObject_GenericSetAttrValue'])
        self.assertEqual(object_slot(Plain, 'getattro'),
                         functions['PyObject_GenericGetAttr'])
        self.assertEqual(object_slot(Plain, 'setattro'),
                         functions['PyObject_GenericSetAttr'])

    def test_type_and_module(self):
        functions = value_functions()
        self.assertEqual(value_slot(type, 'getattro'),
                         functions['_Py_type_getattro_value'])
        self.assertEqual(object_slot(type, 'getattro'),
                         functions['_Py_type_getattro'])
        self.assertEqual(value_slot(types.ModuleType, 'getattro'),
                         functions['_Py_module_getattro_value'])
        self.assertEqual(object_slot(types.ModuleType, 'getattro'),
                         functions['_Py_module_getattro'])

    def test_getattr_hook_added_and_removed(self):
        hook = ('_Py_slot_tp_getattr_hook', '_Py_slot_tp_getattr_hook_value')
        plain = ('_Py_slot_tp_getattro', '_Py_slot_tp_getattro_value')

        class C:
            def __getattribute__(self, name):
                return object.__getattribute__(self, name)
        c = C()
        c.x = 1000
        # update_one_slot() installs the hook pair; a read that finds no
        # __getattr__ replaces it with the simpler pair.
        self.assertEqual(getattro_pair_names(C), hook)
        self.assertEqual(c.x, 1000)
        self.assertEqual(getattro_pair_names(C), plain)
        self.assertEqual(value_getattr(c, 'x')[0], 1000)

        C.__getattr__ = lambda self, name: name
        self.assertEqual(getattro_pair_names(C), hook)
        self.assertEqual(c.x, 1000)
        self.assertEqual(c.missing, 'missing')
        self.assertEqual(value_getattr(c, 'missing'), ('missing', False))
        self.assertEqual(getattro_pair_names(C), hook)

        del C.__getattr__
        self.assertEqual(getattro_pair_names(C), hook)
        self.assertEqual(c.x, 1000)
        with self.assertRaises(AttributeError):
            c.missing
        self.assertEqual(getattro_pair_names(C), plain)
        self.assertEqual(value_getattr(c, 'x')[0], 1000)

    def test_getattr_hook_inline_attribute(self):
        class C:
            def __getattr__(self, name):
                return 1000
        self.assertEqual(C().anything, 1000)
        self.assertEqual(value_getattr(C(), 'anything'), (1000, True))


class RoundTripIdentityTests(unittest.TestCase):
    """Round-trip identity (value-api.md, section 1.3): Python code compares
    inline values by value identity, through every boundary that a value
    crosses, including the value slot_* functions of Python classes."""

    def test_list_item(self):
        x = [1000, 2.5]
        for i in range(2):
            self.assertIs(x[i], x[i])

    def test_attribute(self):
        class C:
            pass
        o = C()
        o.x = 1000
        self.assertIs(o.x, o.x)
        value_setattr(o, 'y', 1000)
        self.assertIs(value_getattr(o, 'y')[0], o.y)

    def test_dunder_operand(self):
        big = 1000
        seen = []

        class C:
            def __radd__(self, other):
                seen.append(other)
            def __add__(self, other):
                seen.append(other)
            def __getitem__(self, key):
                seen.append(key)
            def __setitem__(self, key, value):
                seen.extend((key, value))
            def __contains__(self, key):
                seen.append(key)
                return True
            def __lt__(self, other):
                seen.append(other)
                return True

        c = C()
        for op in (lambda: big + c, lambda: c + big, lambda: c[big],
                   lambda: c.__setitem__(big, big), lambda: big in c,
                   lambda: c < big):
            seen.clear()
            op()
            for item in seen:
                self.assertIs(item, big)
        for f in (2.5, 1.5):
            seen.clear()
            f + c
            self.assertIs(seen[0], f)


class DescrGetDeletedTests(unittest.TestCase):
    def test_non_data_descriptor(self):
        class Desc:
            def __get__(self, obj, owner=None):
                return 1000
        class Owner:
            d = Desc()
        self.assertEqual(Owner().d, 1000)
        self.assertEqual(value_getattr(Owner(), 'd'), (1000, True))
        self.assertNotEqual(object_slot(Desc, 'descr_get'), 0)
        self.assertNotEqual(value_slot(Desc, 'descr_get'), 0)
        del Desc.__get__
        self.assertIsInstance(Owner().d, Desc)
        self.assertIsInstance(value_getattr(Owner(), 'd')[0], Desc)
        self.assertEqual(value_slot(Desc, 'descr_get'), 0)
        self.assertEqual(object_slot(Desc, 'descr_get'), 0)

    def test_data_descriptor(self):
        stored = []
        class Desc:
            def __get__(self, obj, owner=None):
                return 1000
            def __set__(self, obj, value):
                stored.append(value)
        class Owner:
            d = Desc()
        o = Owner()
        self.assertEqual(o.d, 1000)
        o.d = 5
        value_setattr(o, 'd', 2.5)
        self.assertEqual(stored, [5, 2.5])
        del Desc.__get__
        self.assertIs(o.d, Owner.__dict__['d'])
        self.assertIs(value_getattr(o, 'd')[0], Owner.__dict__['d'])
        self.assertEqual(value_slot(Desc, 'descr_get'), 0)
        self.assertEqual(object_slot(Desc, 'descr_get'), 0)
        self.assertNotEqual(value_slot(Desc, 'descr_set'), 0)
        self.assertNotEqual(object_slot(Desc, 'descr_set'), 0)
        o.d = 6
        self.assertEqual(stored, [5, 2.5, 6])


class ModuleAttributeTests(unittest.TestCase):
    def test_module_getattr(self):
        module = types.ModuleType('m2_module')
        def module_getattr(name):
            if name == 'answer':
                return 1000
            raise AttributeError(f'no {name}')
        module.__getattr__ = module_getattr
        module.present = 5
        self.assertEqual(module.answer, 1000)
        self.assertEqual(value_getattr(module, 'answer'), (1000, True))
        self.assertEqual(value_getattr(module, 'present'), (5, True))
        self.assertEqual(value_getoptionalattr(module, 'answer'), (1, 1000))
        self.assertEqual(value_getoptionalattr(module, 'other'), (0, None))
        with self.assertRaisesRegex(AttributeError, 'no other'):
            value_getattr(module, 'other')

    def test_math_pi(self):
        import math
        self.assertEqual(value_getattr(math, 'pi'), (math.pi, True))
        self.assertEqual(value_getoptionalattr(math, 'pi'), (1, math.pi))

    def test_missing_module_attribute(self):
        import math
        message = "module 'math' has no attribute 'nope'"
        with self.assertRaisesRegex(AttributeError, message) as cm:
            math.nope
        self.assertEqual(cm.exception.name, 'nope')
        with self.assertRaisesRegex(AttributeError, message):
            value_getattr(math, 'nope')
        self.assertEqual(value_getoptionalattr(math, 'nope'), (0, None))



class AttributeSlotWrapperTests(unittest.TestCase):
    # The slot descriptors of the attribute and descriptor rows; they call
    # the value side through M2-e's value wrappers once those land, and the
    # object side until then.

    def test_getattribute(self):
        self.assertEqual(object.__getattribute__(1000, 'real'), 1000)
        self.assertIs(type.__getattribute__(int, 'real'), int.real)
        with self.assertRaisesRegex(TypeError,
                                    "attribute name must be string, not 'int'"):
            object.__getattribute__(1, 5)
        with self.assertRaisesRegex(AttributeError,
                                    "'int' object has no attribute 'foo'"):
            object.__getattribute__(1, 'foo')

    def test_setattr_delattr(self):
        class C:
            pass
        c = C()
        object.__setattr__(c, 'x', 1000)
        self.assertEqual(object.__getattribute__(c, 'x'), 1000)
        object.__delattr__(c, 'x')
        self.assertFalse(hasattr(c, 'x'))
        with self.assertRaisesRegex(TypeError,
                                    "__setattr__ expected 2 arguments, got 1"):
            object.__setattr__(c, 'x')
        with self.assertRaisesRegex(
                AttributeError, "attribute 'real' of 'int' objects is not writable"):
            object.__delattr__(1, 'real')

    def test_setattr_hackcheck(self):
        with self.assertRaisesRegex(
                TypeError, "can't apply this __setattr__ to type object"):
            object.__setattr__(int, 'x', 1)

    def test_get(self):
        r = int.real
        self.assertEqual(r.__get__(5), 5)
        self.assertEqual(r.__get__(5, int), 5)
        self.assertIs(r.__get__(None, int), r)
        with self.assertRaisesRegex(TypeError, r"__get__\(None, None\) is invalid"):
            r.__get__(None, None)
        with self.assertRaisesRegex(
                TypeError, "__get__ expected at least 1 argument, got 0"):
            r.__get__()
        with self.assertRaisesRegex(
                TypeError, "__get__ expected at most 2 arguments, got 3"):
            r.__get__(1, 2, 3)

    def test_set_delete(self):
        descr = VGetSet.__dict__['x']
        o = VGetSet()
        descr.__set__(o, 7)
        self.assertEqual(o.x, 7)
        descr.__set__(o, 2.5)
        self.assertEqual(value_getattr(o, 'x'), (2.5, True))
        with self.assertRaisesRegex(AttributeError, 'cannot delete x'):
            descr.__delete__(o)


class PythonClassValueSideTests(unittest.TestCase):
    """Value sides of the slot wrappers of Python classes (V2a-1a-iii)."""

    OPERANDS = (3, -4, 2.5, 2**70)

    def test_pow(self):
        class C:
            def __pow__(self, other, mod=None):
                return ('pow', other, mod)
            def __rpow__(self, other, mod=None):
                return ('rpow', other, mod)
            def __ipow__(self, other, mod=None):
                return ('ipow', other, mod)
        c = C()
        for x in self.OPERANDS:
            with self.subTest(x=x):
                self.assertEqual(c ** x, ('pow', x, None))
                self.assertEqual(pow(c, x, 7), ('pow', x, 7))
                self.assertEqual(x ** c, ('rpow', x, None))
                if isinstance(x, int):
                    self.assertEqual(pow(x, c, 7), ('rpow', x, 7))
                d = c
                d **= x
                self.assertEqual(d, ('ipow', x, None))

    def test_ipow_modulus_is_rejected(self):
        class C:
            def __ipow__(self, other):
                return 'ipow'
        c = C()
        c **= 2
        self.assertEqual(c, 'ipow')
        with self.assertRaises(TypeError):
            pow(C(), 2, 5)

    def test_unary(self):
        class C:
            def __init__(self, v):
                self.v = v
            def __neg__(self): return -self.v
            def __pos__(self): return +self.v
            def __invert__(self): return ~self.v
            def __abs__(self): return abs(self.v)
        for v in (5, -5, 0, 2**70):
            c = C(v)
            for op in (operator.neg, operator.pos, operator.invert, abs):
                with self.subTest(v=v, op=op):
                    self.assertEqual(op(c), op(v))
        for v in (2.5, -2.5):
            c = C(v)
            for op in (operator.neg, operator.pos, abs):
                with self.subTest(v=v, op=op):
                    self.assertEqual(op(c), op(v))

    def test_bool(self):
        class C:
            def __init__(self, r):
                self.r = r
            def __bool__(self):
                return self.r
        self.assertIs(bool(C(True)), True)
        self.assertIs(bool(C(False)), False)
        for bad in (1, 0, 2.5, None, 'x', 2**70):
            with self.subTest(bad=bad):
                with self.assertRaisesRegex(TypeError, r'__bool__\(\) must return a bool'):
                    bool(C(bad))

    def test_hash(self):
        class C:
            def __init__(self, r):
                self.r = r
            def __hash__(self):
                return self.r
        # A result that fits Py_ssize_t is used as is (-1 becomes -2);
        # a larger one is hashed.
        for r, expected in ((0, 0), (5, 5), (-1, -2), (-2, -2),
                            (2**62, 2**62), (2**63, hash(2**63)),
                            (2**100, hash(2**100)), (True, 1)):
            with self.subTest(r=r):
                self.assertEqual(hash(C(r)), expected)
        for bad in (2.5, None, 'x'):
            with self.subTest(bad=bad):
                with self.assertRaisesRegex(TypeError, r'__hash__ method should return an integer'):
                    hash(C(bad))

    def test_repr_str_errors(self):
        class C:
            def __init__(self, r):
                self.r = r
            def __repr__(self):
                return self.r
        class D(C):
            def __str__(self):
                return self.r
        self.assertEqual(repr(C('x')), 'x')
        for bad in (5, 2.5, None):
            with self.subTest(bad=bad):
                with self.assertRaisesRegex(TypeError, r'__repr__\(\) must return a str'):
                    repr(C(bad))
                with self.assertRaisesRegex(TypeError, r'__str__\(\) must return a str'):
                    str(D(bad))

    def test_reflected_subclass_priority(self):
        class Base:
            def __add__(self, other): return 'base add'
            def __radd__(self, other): return 'base radd'
        class Sub(Base):
            def __radd__(self, other): return 'sub radd'
        class Plain(Base):
            pass
        self.assertEqual(Base() + Sub(), 'sub radd')
        self.assertEqual(Base() + Plain(), 'base add')
        self.assertEqual(Sub() + Sub(), 'base add')
        for x in self.OPERANDS:
            with self.subTest(x=x):
                self.assertEqual(x + Sub(), 'sub radd')
                self.assertEqual(Sub() + x, 'base add')

    def test_contains_fallbacks(self):
        class Iter:
            def __iter__(self):
                return iter((1, 2.5, 2**70))
        class Item:
            def __getitem__(self, i):
                if i >= 3:
                    raise IndexError
                return (1, 2.5, 2**70)[i]
        for cls in (Iter, Item):
            for x, expected in ((1, True), (2.5, True), (2**70, True),
                                (4, False), (0.5, False)):
                with self.subTest(cls=cls.__name__, x=x):
                    self.assertIs(x in cls(), expected)
                    self.assertIs(x not in cls(), not expected)

class IndexExactTests(unittest.TestCase):
    """PyNumber_Index() gives an exact int (a subclass instance is copied);
    _PyNumber_Index() may give the subclass. The value forms keep the
    difference: _PyValue_IndexExact() and _PyValue_Index()."""

    class MyInt(int):
        pass

    def test_operator_index(self):
        for value in (5, self.MyInt(5), True, 2**70, self.MyInt(2**70)):
            with self.subTest(value=value):
                result = operator.index(value)
                self.assertIs(type(result), int)
                self.assertEqual(result, int(value))

    def test_math_integer_functions(self):
        MyInt = self.MyInt
        self.assertIs(type(math.gcd(MyInt(12))), int)
        self.assertIs(type(math.gcd(MyInt(12), MyInt(18))), int)
        self.assertEqual(math.gcd(MyInt(12), MyInt(18)), 6)
        self.assertIs(type(math.lcm(MyInt(4))), int)
        self.assertEqual(math.lcm(MyInt(4), MyInt(6)), 12)
        self.assertEqual(math.comb(MyInt(5), MyInt(2)), 10)
        self.assertEqual(math.perm(MyInt(5), MyInt(2)), 20)
        self.assertEqual(math.comb(MyInt(2**70), MyInt(1)), 2**70)

    def test_nextafter_steps(self):
        MyInt = self.MyInt
        self.assertEqual(math.nextafter(1.0, 2.0, steps=MyInt(1)),
                         math.nextafter(1.0, 2.0))
        self.assertEqual(math.nextafter(1.0, 2.0, steps=MyInt(0)), 1.0)
        self.assertEqual(math.nextafter(1.0, 2.0, steps=MyInt(2**70)), 2.0)
        with self.assertRaisesRegex(ValueError, "non-negative"):
            math.nextafter(1.0, 2.0, steps=MyInt(-1))

    def test_enumerate_start(self):
        for start in (self.MyInt(2**70), self.MyInt(5)):
            with self.subTest(start=start):
                index, _ = next(enumerate('a', start))
                self.assertIs(type(index), int)
                self.assertEqual(index, int(start))

    def test_index_result_of_a_subclass(self):
        class Holder:
            def __index__(self):
                return IndexExactTests.MyInt(7)
        with self.assertWarns(DeprecationWarning):
            result = operator.index(Holder())
        self.assertIs(type(result), int)
        self.assertEqual(result, 7)


class ContainerSlotTests(unittest.TestCase):
    """The value slots of list, tuple, dict and str: an inline int or float
    key, index or operand reaches them unboxed, and the errors read as the
    object forms did."""

    def test_list(self):
        a = [10, 20.5, 30, 40]
        self.assertEqual((a[1], a[-1], a[1:3], a[::-2]), (20.5, 40, [20.5, 30], [40, 20.5]))
        self.assertEqual(a[True], 20.5)
        with self.assertRaisesRegex(TypeError, "list indices must be integers or slices, not float"):
            a[1.5]
        with self.assertRaisesRegex(TypeError, "list indices must be integers or slices, not str"):
            a["x"]
        with self.assertRaises(IndexError):
            a[4]
        with self.assertRaisesRegex(IndexError, "cannot fit 'int' into an index-sized integer"):
            a[2**70]
        a[0] = 11
        a[-1] = 4.5
        a[1:3] = (1, 2, 3)
        self.assertEqual(a, [11, 1, 2, 3, 4.5])
        del a[0]
        del a[-1]
        self.assertEqual(a, [1, 2, 3])
        with self.assertRaisesRegex(IndexError, "list assignment index out of range"):
            a[3] = 1
        with self.assertRaisesRegex(IndexError, "list assignment index out of range"):
            del a[3]
        with self.assertRaisesRegex(TypeError, "list indices must be integers or slices, not float"):
            a[1.5] = 1
        with self.assertRaisesRegex(TypeError, "must assign iterable to extended slice"):
            a[::2] = 5
        self.assertIn(2, a)
        self.assertNotIn(2.5, a)
        self.assertIn(2.0, a)
        self.assertEqual(a + [4], [1, 2, 3, 4])
        with self.assertRaisesRegex(TypeError, r'can only concatenate list \(not "int"\) to list'):
            a + 1
        with self.assertRaisesRegex(TypeError, r'can only concatenate list \(not "float"\) to list'):
            a + 1.5
        a += (7, 8)
        self.assertEqual(a, [1, 2, 3, 7, 8])
        with self.assertRaisesRegex(TypeError, "'int' object is not iterable"):
            a += 5
        self.assertEqual([1, 2, 3].pop(0), 1)
        b = [1, 2.5, 3]
        self.assertEqual((b.pop(), b.pop(0), b), (3, 1, [2.5]))

    def test_tuple(self):
        t = (10, 20.5, 30)
        self.assertEqual((t[1], t[-1], t[1:], t[::-1]), (20.5, 30, (20.5, 30), (30, 20.5, 10)))
        with self.assertRaisesRegex(TypeError, "tuple indices must be integers or slices, not float"):
            t[1.5]
        with self.assertRaisesRegex(IndexError, "tuple index out of range"):
            t[3]
        self.assertIn(20.5, t)
        self.assertNotIn(21, t)
        self.assertEqual(t + (1,), (10, 20.5, 30, 1))
        with self.assertRaisesRegex(TypeError, r'can only concatenate tuple \(not "int"\) to tuple'):
            t + 1

    def test_dict(self):
        d = {1: 'a', 2.5: 'b', 'k': 'c'}
        self.assertEqual((d[1], d[2.5], d['k'], d[True]), ('a', 'b', 'c', 'a'))
        for key in (3, 3.5, 2**70):
            with self.subTest(key=key):
                with self.assertRaises(KeyError) as cm:
                    d[key]
                self.assertEqual(cm.exception.args, (key,))
        d[3] = 'd'
        d[4.5] = 'e'
        del d[3]
        self.assertEqual(d, {1: 'a', 2.5: 'b', 'k': 'c', 4.5: 'e'})
        with self.assertRaises(KeyError):
            del d[3]
        self.assertIn(4.5, d)
        self.assertNotIn(5, d)
        class Missing(dict):
            def __missing__(self, key):
                return ('missing', key)
        self.assertEqual(Missing()[5], ('missing', 5))
        self.assertEqual(Missing()[2.5], ('missing', 2.5))
        with self.assertRaises(TypeError):
            d | 5
        with self.assertRaises(TypeError):
            5 | d
        self.assertEqual({1: 2} | {3: 4}, {1: 2, 3: 4})
        e = {1: 2}
        e |= [(3, 4)]
        self.assertEqual(e, {1: 2, 3: 4})
        with self.assertRaisesRegex(TypeError, "'int' object is not iterable"):
            e |= 5
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            e[[1]]

    def test_str(self):
        s = "abcd"
        self.assertEqual((s[1], s[-1], s[1:3], s[::-1]), ("b", "d", "bc", "dcba"))
        with self.assertRaisesRegex(TypeError, "string indices must be integers, not 'float'"):
            s[1.5]
        with self.assertRaisesRegex(IndexError, "string index out of range"):
            s[4]
        with self.assertRaisesRegex(TypeError, r'can only concatenate str \(not "int"\) to str'):
            s + 1
        with self.assertRaisesRegex(TypeError, "'in <string>' requires string as left operand, not int"):
            1 in s
        self.assertIn("bc", s)
        with self.assertRaises(TypeError):
            s - 1

    def test_str_format(self):
        cases = [
            ("%d", 5, "5"), ("%i", -7, "-7"), ("%u", 2**59, str(2**59)),
            ("%5d|%-5d|%05d", (3, 4, -5), "    3|4    |-0005"),
            ("%+d % d", (3, 4), "+3  4"), ("%.3d", 5, "005"),
            ("%x %X %o %#x %#X %#o", (255,) * 6, "ff FF 377 0xff 0XFF 0o377"),
            ("%x", -255, "-ff"), ("%#x", -255, "-0xff"), ("%#x", 0, "0x0"),
            ("%s %r %a", (5, 5, 5), "5 5 5"), ("%s %r", (2.5, 2.5), "2.5 2.5"),
            ("%s", 1e22, "1e+22"), ("%5s|%-5r|%.1s", (1, 2, 345), "    1|2    |3"),
            ("%f %e %g", (1.5, 1.5, 1.5), "1.500000 1.500000e+00 1.5"),
            ("%f %.2f %10.3f", (3, 3, 3), "3.000000 3.00      3.000"),
            ("%c %c", (65, "x"), "A x"), ("%5c", 66, "    B"),
            ("%(a)d %(b)s %(c).1f", {"a": 5, "b": 2.5, "c": 1}, "5 2.5 1.0"),
            ("%*d %.*f", (4, 5, 1, 2.25), "   5 2.2"),
            ("%d", 3.9, "3"), ("%x", True, "1"), ("%s", None, "None"),
        ]
        for fmt, args, expected in cases:
            with self.subTest(fmt=fmt, args=args):
                self.assertEqual(fmt % args, expected)
        with self.assertRaisesRegex(TypeError, "format argument: %x requires an integer, not float"):
            "%x" % 2.5
        with self.assertRaisesRegex(TypeError, "format argument: %c requires an integer or a unicode character, not float"):
            "%c" % 2.5
        with self.assertRaisesRegex(OverflowError, r"%c argument not in range\(0x110000\)"):
            "%c" % -1
        with self.assertRaisesRegex(TypeError, "not all arguments converted"):
            "%d" % (1, 2)
        with self.assertRaisesRegex(TypeError, "not enough arguments"):
            "%d %d" % 1
        with self.assertRaisesRegex(TypeError, "format requires a mapping, not int"):
            "%(a)d" % 1

class RareTypeSlotTests(unittest.TestCase):
    """The value slots of range, set, dict views, complex and the types whose
    compare and number slots decline an inline operand."""

    def test_range_index(self):
        r = range(10, 100, 7)
        self.assertEqual([r[i] for i in range(len(r))], list(r))
        self.assertEqual((r[-1], r[-len(r)], r[True]), (r[len(r) - 1], 10, 17))
        for i in (len(r), -len(r) - 1):
            with self.assertRaisesRegex(IndexError, "range object index out of range"):
                r[i]
        with self.assertRaisesRegex(TypeError, "range indices must be integers or slices, not float"):
            r[1.5]
        self.assertEqual(r[1:4], range(17, 38, 7))
        down = range(5, -20, -3)
        self.assertEqual([down[i] for i in range(len(down))], list(down))
        self.assertEqual(range(0)[0:0], range(0))
        with self.assertRaises(IndexError):
            range(0)[0]

    def test_range_large(self):
        r = range(2**70, 2**70 + 10, 3)
        self.assertEqual((r[0], r[-1]), (2**70, 2**70 + 9))
        with self.assertRaisesRegex(IndexError, "range object index out of range"):
            range(3)[2**70]
        self.assertIn(2**70 + 3, r)
        self.assertNotIn(2**70 + 4, r)
        self.assertEqual(range(0, 2**40, 2**20)[-1], 2**40 - 2**20)

    def test_range_contains(self):
        r = range(10, 100, 7)
        for v in range(-5, 110):
            with self.subTest(v=v):
                self.assertEqual(v in r, v in list(r))
        down = range(50, -50, -5)
        for v in range(-60, 60):
            with self.subTest(v=v):
                self.assertEqual(v in down, v in list(down))
        self.assertIn(17.0, r)
        self.assertNotIn(17.5, r)
        self.assertIn(True, range(3))
        self.assertNotIn("x", r)
        self.assertNotIn(1, range(0))
        self.assertNotIn(2**70, r)

    def test_set_contains(self):
        s = {1, 2.5, "a", (1, 2), frozenset({1})}
        self.assertIn(1, s)
        self.assertIn(2.5, s)
        self.assertIn(1.0, s)
        self.assertNotIn(3, s)
        self.assertIn(frozenset({1}), s)
        self.assertIn({1}, s)
        with self.assertRaisesRegex(TypeError, "unhashable type: 'list'"):
            [1] in s
        f = frozenset(s - {frozenset({1})})
        self.assertIn(2.5, f)
        self.assertNotIn(2.6, f)

    def test_set_operators(self):
        a, b = {1, 2, 3}, {2, 3, 4}
        self.assertEqual((a - b, a & b, a ^ b, a | b), ({1}, {2, 3}, {1, 4}, {1, 2, 3, 4}))
        fa = frozenset(a)
        self.assertEqual(type(fa | b), frozenset)
        c = set(a)
        c -= b
        c |= {9}
        c &= {1, 9}
        c ^= {9, 10}
        self.assertEqual(c, {1, 10})
        for op in (operator.sub, operator.and_, operator.xor, operator.or_):
            with self.subTest(op=op):
                with self.assertRaises(TypeError):
                    op(a, 1)
                with self.assertRaises(TypeError):
                    op(1, a)
        with self.assertRaises(TypeError):
            c |= 5
        self.assertTrue(a >= {1})
        self.assertFalse(a == 1)
        self.assertTrue(a != 1.5)

    def test_dict_views(self):
        d = {1: 'a', 2.5: 'b', 'k': (1, 2)}
        self.assertIn(1, d.keys())
        self.assertIn(2.5, d.keys())
        self.assertNotIn(3, d.keys())
        self.assertIn((1, 'a'), d.items())
        self.assertIn((2.5, 'b'), d.items())
        self.assertIn(('k', (1, 2)), d.items())
        self.assertNotIn((1, 'b'), d.items())
        self.assertNotIn(1, d.items())
        self.assertNotIn((1, 'a', 3), d.items())
        self.assertEqual(d.keys(), {1, 2.5, 'k'})
        self.assertNotEqual(d.keys(), 1)
        self.assertNotEqual(d.items(), 2.5)

    def test_complex_operators(self):
        z = 3 + 4j
        self.assertEqual((z + 1, 1 + z, z + 1.5, 1.5 + z), (4 + 4j, 4 + 4j, 4.5 + 4j, 4.5 + 4j))
        self.assertEqual((z - 1, 1 - z, z * 2, 2 * z), (2 + 4j, -2 - 4j, 6 + 8j, 6 + 8j))
        self.assertEqual((z / 2, z / 2.0), (1.5 + 2j, 1.5 + 2j))
        self.assertAlmostEqual(2 / z, 0.24 - 0.32j)
        self.assertEqual(z ** 2, -7 + 24j)
        self.assertEqual((-z, +z, abs(z), bool(z), bool(0j)), (-3 - 4j, z, 5.0, True, False))
        self.assertIs(type(abs(z)), float)
        self.assertEqual(z + 2**70, complex(3 + 2**70, 4))
        with self.assertRaisesRegex(OverflowError, "int too large to convert to float"):
            z + 10**400
        with self.assertRaisesRegex(ZeroDivisionError, "division by zero"):
            z / 0
        with self.assertRaisesRegex(ZeroDivisionError, "zero to a negative or complex power"):
            0j ** -1
        with self.assertRaisesRegex(ValueError, "complex modulo"):
            pow(z, 2, 5)
        with self.assertRaisesRegex(TypeError, "unsupported operand"):
            z + "a"

    def test_complex_compare(self):
        self.assertTrue(1 + 0j == 1)
        self.assertTrue(1 == 1 + 0j)
        self.assertTrue(1.5 + 0j == 1.5)
        self.assertTrue(1.5 != 1.5 + 1j)
        self.assertTrue(1j != 1)
        self.assertTrue(2**70 + 0j == 2**70)
        self.assertTrue(complex(2**53 + 1, 0) != 2**53 + 1)
        self.assertTrue(True == 1 + 0j)
        self.assertFalse(1j == "a")
        with self.assertRaisesRegex(TypeError, "'<' not supported"):
            1j < 1
        with self.assertRaisesRegex(TypeError, "'<' not supported"):
            1 < 1j

    def test_bytearray_from_list_of_ints(self):
        self.assertEqual(bytearray([0, 1, 255]), b'\x00\x01\xff')
        self.assertEqual(bytearray((7, 8)), b'\x07\x08')
        self.assertEqual(bytearray([True, 2, 3]), b'\x01\x02\x03')
        class Idx:
            def __index__(self):
                return 9
        self.assertEqual(bytearray([1, Idx()]), b'\x01\x09')
        for bad, exc in (([1, 256], ValueError), ([1, -1], ValueError),
                         ([1, 2**70], ValueError), ([1, 1.5], TypeError),
                         ([1, "a"], TypeError), ((1, None), TypeError)):
            with self.subTest(bad=bad):
                with self.assertRaises(exc):
                    bytearray(bad)

    def test_rare_compare_and_or(self):
        for obj in (slice(1), types.SimpleNamespace(a=1), range(3), (1,), [1],
                    {1: 2}, {1}, frozenset({1}), {}.keys(), {}.items(),
                    list[int], int | str, memoryview(b'a'),
                    collections.OrderedDict(), (lambda: 0).__code__):
            with self.subTest(obj=obj):
                self.assertFalse(obj == 1)
                self.assertTrue(obj != 2.5)
                self.assertTrue(obj == obj)
        self.assertEqual(int | str, typing.Union[int, str])
        self.assertEqual(list[int] | None, typing.Optional[list[int]])
        for bad in (1, 2.5):
            with self.assertRaises(TypeError):
                int | bad
            with self.assertRaises(TypeError):
                bad | int
            with self.assertRaises(TypeError):
                list[int] | bad


class IndexExactTests(unittest.TestCase):
    """PyNumber_Index() gives an exact int (a subclass instance is copied);
    _PyNumber_Index() may give the subclass. The value forms keep the
    difference: _PyValue_IndexExact() and _PyValue_Index()."""

    class MyInt(int):
        pass

    def test_operator_index(self):
        for value in (5, self.MyInt(5), True, 2**70, self.MyInt(2**70)):
            with self.subTest(value=value):
                result = operator.index(value)
                self.assertIs(type(result), int)
                self.assertEqual(result, int(value))

    def test_math_integer_functions(self):
        MyInt = self.MyInt
        self.assertIs(type(math.gcd(MyInt(12))), int)
        self.assertIs(type(math.gcd(MyInt(12), MyInt(18))), int)
        self.assertEqual(math.gcd(MyInt(12), MyInt(18)), 6)
        self.assertIs(type(math.lcm(MyInt(4))), int)
        self.assertEqual(math.lcm(MyInt(4), MyInt(6)), 12)
        self.assertEqual(math.comb(MyInt(5), MyInt(2)), 10)
        self.assertEqual(math.perm(MyInt(5), MyInt(2)), 20)
        self.assertEqual(math.comb(MyInt(2**70), MyInt(1)), 2**70)

    def test_nextafter_steps(self):
        MyInt = self.MyInt
        self.assertEqual(math.nextafter(1.0, 2.0, steps=MyInt(1)),
                         math.nextafter(1.0, 2.0))
        self.assertEqual(math.nextafter(1.0, 2.0, steps=MyInt(0)), 1.0)
        self.assertEqual(math.nextafter(1.0, 2.0, steps=MyInt(2**70)), 2.0)
        with self.assertRaisesRegex(ValueError, "non-negative"):
            math.nextafter(1.0, 2.0, steps=MyInt(-1))

    def test_enumerate_start(self):
        for start in (self.MyInt(2**70), self.MyInt(5)):
            with self.subTest(start=start):
                index, _ = next(enumerate('a', start))
                self.assertIs(type(index), int)
                self.assertEqual(index, int(start))

    def test_range_and_slice_arguments(self):
        MyInt = self.MyInt
        r = range(MyInt(2), MyInt(7), MyInt(1))
        for part in (r.start, r.stop, r.step):
            self.assertIs(type(part), int)
        self.assertIs(type(range(MyInt(5)).stop), int)
        self.assertEqual(range(10)[MyInt(2)], 2)
        self.assertEqual(slice(None).indices(MyInt(10)), (0, 10, 1))
        self.assertIs(type(slice(None).indices(MyInt(10))[1]), int)
        self.assertEqual([1, 2, 3][MyInt(1):MyInt(3)], [2, 3])

    def test_index_result_of_a_subclass(self):
        class Holder:
            def __index__(self):
                return IndexExactTests.MyInt(7)
        with self.assertWarns(DeprecationWarning):
            result = operator.index(Holder())
        self.assertIs(type(result), int)
        self.assertEqual(result, 7)


if __name__ == '__main__':
    unittest.main()
