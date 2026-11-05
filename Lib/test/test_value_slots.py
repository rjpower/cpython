"""Value slots in PyTypeObject (V2a-0).

Covers pyjit/design/proposals/value-api-v2a0.md, sections 4.2-4.4 (the
tp_value field, the pairing invariant and inheritance in PyType_Ready) and
section 4.8 (the test types).
"""

import operator
import re
import sys
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
ObjAdd = _testcapi.ObjAdd
ObjIndex = _testcapi.ObjIndex
ObjNotImpl = _testcapi.ObjNotImpl

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

    def test_object_hash_of_inline_values(self):
        # object.__hash__ applied to an exact int or float calls the pair's
        # value wrapper, so _PyValue_GenericHash() gets an inline value. It
        # hashes a box of it, as PyObject_GenericHash() hashes the object
        # that it is given.
        generichash = _testcapi.object_generichash
        # A small int boxes to its cached object.
        self.assertEqual(object.__hash__(1), object.__hash__(1))
        self.assertEqual(object.__hash__(1), generichash(1))
        # Other inline values box to a new object each time, so the hash is
        # that of the box, as id() is.
        for x in (1000, -2**50, 2.5, -0.0):
            with self.subTest(x=x):
                self.assertIsInstance(object.__hash__(x), int)
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

    def test_int_has_no_value_side(self):
        self.assertEqual(wrapper_pair(int.__add__)[1], 0)

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

        self.assertEqual(value_slot(S, 'nb_subtract'), 0)
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
        self.assertEqual(value_slot(S, 'nb_subtract'), 0)
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
        self.assertEqual(value_slot(C, 'nb_subtract'), 0)
        self.assertEqual(C(1) - 1, 'B')
        del B.__sub__
        self.assertEqual(pair(C), self.split_pair)

    def test_reflected_conflict(self):
        class S(Split):
            __rsub__ = lambda self, o: 'r'

        self.assertEqual(value_slot(S, 'nb_subtract'), 0)
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


if __name__ == '__main__':
    unittest.main()
