"""This module includes tests of the code object representation.

>>> def f(x):
...     def g(y):
...         return x + y
...     return g
...

>>> dump(f.__code__)
name: f
argcount: 1
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('x', 'g')
cellvars: ('x',)
freevars: ()
nlocals: 2
flags: 3
consts: ('<code object g>',)

>>> dump(f(4).__code__)
name: g
argcount: 1
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('y',)
cellvars: ()
freevars: ('x',)
nlocals: 1
flags: 19
consts: ('None',)

>>> def h(x, y):
...     a = x + y
...     b = x - y
...     c = a * b
...     return c
...

>>> dump(h.__code__)
name: h
argcount: 2
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('x', 'y', 'a', 'b', 'c')
cellvars: ()
freevars: ()
nlocals: 5
flags: 3
consts: ('None',)

>>> def attrs(obj):
...     print(obj.attr1)
...     print(obj.attr2)
...     print(obj.attr3)

>>> dump(attrs.__code__)
name: attrs
argcount: 1
posonlyargcount: 0
kwonlyargcount: 0
names: ('print', 'attr1', 'attr2', 'attr3')
varnames: ('obj',)
cellvars: ()
freevars: ()
nlocals: 1
flags: 3
consts: ('None',)

>>> def optimize_away():
...     'doc string'
...     'not a docstring'
...     53
...     0x53

>>> dump(optimize_away.__code__)
name: optimize_away
argcount: 0
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ()
cellvars: ()
freevars: ()
nlocals: 0
flags: 67108867
consts: ("'doc string'",)

>>> def keywordonly_args(a,b,*,k1):
...     return a,b,k1
...

>>> dump(keywordonly_args.__code__)
name: keywordonly_args
argcount: 2
posonlyargcount: 0
kwonlyargcount: 1
names: ()
varnames: ('a', 'b', 'k1')
cellvars: ()
freevars: ()
nlocals: 3
flags: 3
consts: ('None',)

>>> def posonly_args(a,b,/,c):
...     return a,b,c
...

>>> dump(posonly_args.__code__)
name: posonly_args
argcount: 3
posonlyargcount: 2
kwonlyargcount: 0
names: ()
varnames: ('a', 'b', 'c')
cellvars: ()
freevars: ()
nlocals: 3
flags: 3
consts: ('None',)

>>> def has_docstring(x: str):
...     'This is a one-line doc string'
...     x += x
...     x += "hello world"
...     # co_flags should be 0x4000003 = 67108867
...     return x

>>> dump(has_docstring.__code__)
name: has_docstring
argcount: 1
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('x',)
cellvars: ()
freevars: ()
nlocals: 1
flags: 67108867
consts: ("'This is a one-line doc string'", "'hello world'")

>>> async def async_func_docstring(x: str, y: str):
...     "This is a docstring from async function"
...     import asyncio
...     await asyncio.sleep(1)
...     # co_flags should be 0x4000083 = 67108995
...     return x + y

>>> dump(async_func_docstring.__code__)
name: async_func_docstring
argcount: 2
posonlyargcount: 0
kwonlyargcount: 0
names: ('asyncio', 'sleep')
varnames: ('x', 'y', 'asyncio')
cellvars: ()
freevars: ()
nlocals: 3
flags: 67108995
consts: ("'This is a docstring from async function'",)

>>> def no_docstring(x, y, z):
...     return x + "hello" + y + z + "world"

>>> dump(no_docstring.__code__)
name: no_docstring
argcount: 3
posonlyargcount: 0
kwonlyargcount: 0
names: ()
varnames: ('x', 'y', 'z')
cellvars: ()
freevars: ()
nlocals: 3
flags: 3
consts: ("'hello'", "'world'")

>>> class class_with_docstring:
...     '''This is a docstring for class'''
...     '''This line is not docstring'''
...     pass

>>> print(class_with_docstring.__doc__)
This is a docstring for class

>>> class class_without_docstring:
...     pass

>>> print(class_without_docstring.__doc__)
None
"""

import copy
import types
import inspect
import sys
import threading
import doctest
import unittest
import warnings
import textwrap
import weakref
from test.support.ir_semantics import (observation_tables, semantic_parts,
                                       semantic_table)
import dis
import struct

try:
    import ctypes
except ImportError:
    ctypes = None
from test.support import (cpython_only,
                          check_impl_detail, requires_debug_ranges,
                          gc_collect, Py_GIL_DISABLED, late_deletion)
from test.support.script_helper import assert_python_ok
from test.support import threading_helper, import_helper
from test.support.bytecode_helper import encode_words, instructions_with_positions
from opcode import opmap, opname
try:
    import _testinternalcapi
except ModuleNotFoundError:
    _testinternalcapi = None
import test._code_definitions as defs

COPY_FREE_VARS = opmap['COPY_FREE_VARS']


def consts(t):
    """Yield a doctest-safe sequence of object reprs."""
    for elt in t:
        r = repr(elt)
        if r.startswith("<code object"):
            yield "<code object %s>" % elt.co_name
        else:
            yield r

def dump(co):
    """Print out a text representation of a code object."""
    for attr in ["name", "argcount", "posonlyargcount",
                 "kwonlyargcount", "names", "varnames",
                 "cellvars", "freevars", "nlocals", "flags"]:
        print("%s: %s" % (attr, getattr(co, "co_" + attr)))
    print("consts:", tuple(consts(co.co_consts)))

# Needed for test_closure_injection below
# Defined at global scope to avoid implicitly closing over __class__
def external_getitem(self, i):
    return f"Foreign getitem: {super().__getitem__(i)}"


class CodeTest(unittest.TestCase):

    @cpython_only
    def test_newempty(self):
        _testcapi = import_helper.import_module("_testcapi")
        co = _testcapi.code_newempty("filename", "funcname", 15)
        self.assertEqual(co.co_filename, "filename")
        self.assertEqual(co.co_name, "funcname")
        self.assertEqual(co.co_firstlineno, 15)
        #Empty code object should raise, but not crash the VM
        with self.assertRaises(Exception):
            exec(co)

    @cpython_only
    def test_closure_injection(self):
        # From https://bugs.python.org/issue32176
        from types import FunctionType

        def create_closure(__class__):
            return (lambda: __class__).__closure__

        def shift_stack_slots(c, by):
            # A new free variable moves the evaluation stack up by one slot,
            # so every word that names a stack slot must follow it.
            import _opcode
            contract = _opcode.get_ir_contract()
            words = []
            for i in range(0, len(c.co_code), 8):
                word = int.from_bytes(c.co_code[i:i+8], 'little')
                handler = word & 0xFFFF
                row = contract[handler]
                roles = row.roles
                self.assertFalse(row.flags & _opcode.IR_JUMPS)
                fields = [(word >> shift) & 0xFFF for shift in (16, 28, 40, 52)]
                for k, role in enumerate(roles):
                    is_slot = role in (_opcode.IR_ROLE_DST, _opcode.IR_ROLE_BASE) \
                              or role >= _opcode.IR_ROLE_IN0
                    if is_slot and fields[k] != 4095 and fields[k] >= c.co_nlocals:
                        fields[k] += by
                words.append((handler, *fields))
            return encode_words(*words)

        def new_code(c):
            '''A new code object with a __class__ cell added to freevars'''
            self.assertEqual(c.co_freevars, ())
            return c.replace(co_freevars=('__class__',),
                             co_code=encode_words((COPY_FREE_VARS, 1))
                                     + shift_stack_slots(c, 1),
                             co_linetable=b'')

        def add_foreign_method(cls, name, f):
            code = new_code(f.__code__)
            assert not f.__closure__
            closure = create_closure(cls)
            defaults = f.__defaults__
            setattr(cls, name, FunctionType(code, globals(), name, defaults, closure))

        class List(list):
            pass

        add_foreign_method(List, "__getitem__", external_getitem)

        # Ensure the closure injection actually worked
        function = List.__getitem__
        class_ref = function.__closure__[0].cell_contents
        self.assertIs(class_ref, List)

        # Ensure the zero-arg super() call in the injected method works
        obj = List([1, 2, 3])
        self.assertEqual(obj[0], "Foreign getitem: 1")

    def test_constructor(self):
        def func(): pass
        co = func.__code__
        CodeType = type(co)

        # test code constructor
        CodeType(co.co_argcount,
                        co.co_posonlyargcount,
                        co.co_kwonlyargcount,
                        co.co_nlocals,
                        co.co_stacksize,
                        co.co_flags,
                        co.co_code,
                        co.co_consts,
                        co.co_names,
                        co.co_varnames,
                        co.co_filename,
                        co.co_name,
                        co.co_qualname,
                        co.co_firstlineno,
                        co.co_linetable,
                        co.co_exceptiontable,
                        co.co_freevars,
                        co.co_cellvars)

    def test_qualname(self):
        self.assertEqual(
            CodeTest.test_qualname.__code__.co_qualname,
            CodeTest.test_qualname.__qualname__
        )

    def test_replace(self):
        def func():
            x = 1
            return x
        code = func.__code__

        # Different co_name, co_varnames, co_consts.
        # Must have the same number of constants and
        # variables or we get crashes.
        def func2():
            y = 2
            return y
        code2 = func2.__code__

        for attr, value in (
            ("co_argcount", 0),
            ("co_posonlyargcount", 0),
            ("co_kwonlyargcount", 0),
            ("co_nlocals", 1),
            ("co_stacksize", 1),
            ("co_flags", code.co_flags | inspect.CO_COROUTINE),
            ("co_firstlineno", 100),
            ("co_code", code2.co_code),
            ("co_consts", code2.co_consts),
            ("co_names", ("myname",)),
            ("co_varnames", ('spam',)),
            ("co_freevars", ("freevar",)),
            ("co_cellvars", ("cellvar",)),
            ("co_filename", "newfilename"),
            ("co_name", "newname"),
            ("co_linetable", code2.co_linetable),
        ):
            with self.subTest(attr=attr, value=value):
                new_code = code.replace(**{attr: value})
                self.assertEqual(getattr(new_code, attr), value)
                new_code = copy.replace(code, **{attr: value})
                self.assertEqual(getattr(new_code, attr), value)

        new_code = code.replace(co_varnames=code2.co_varnames,
                                co_nlocals=code2.co_nlocals)
        self.assertEqual(new_code.co_varnames, code2.co_varnames)
        self.assertEqual(new_code.co_nlocals, code2.co_nlocals)
        new_code = copy.replace(code, co_varnames=code2.co_varnames,
                                co_nlocals=code2.co_nlocals)
        self.assertEqual(new_code.co_varnames, code2.co_varnames)
        self.assertEqual(new_code.co_nlocals, code2.co_nlocals)

    def test_nlocals_mismatch(self):
        def func():
            x = 1
            return x
        co = func.__code__
        assert co.co_nlocals > 0;

        # First we try the constructor.
        CodeType = type(co)
        for diff in (-1, 1):
            with self.assertRaises(ValueError):
                CodeType(co.co_argcount,
                         co.co_posonlyargcount,
                         co.co_kwonlyargcount,
                         # This is the only change.
                         co.co_nlocals + diff,
                         co.co_stacksize,
                         co.co_flags,
                         co.co_code,
                         co.co_consts,
                         co.co_names,
                         co.co_varnames,
                         co.co_filename,
                         co.co_name,
                         co.co_qualname,
                         co.co_firstlineno,
                         co.co_linetable,
                         co.co_exceptiontable,
                         co.co_freevars,
                         co.co_cellvars,
                         )
        # Then we try the replace method.
        with self.assertRaises(ValueError):
            co.replace(co_nlocals=co.co_nlocals - 1)
        with self.assertRaises(ValueError):
            co.replace(co_nlocals=co.co_nlocals + 1)

    def test_shrinking_localsplus(self):
        # Check that PyCode_NewWithPosOnlyArgs resizes both
        # localsplusnames and localspluskinds, if an argument is a cell.
        def func(arg):
            return lambda: arg
        code = func.__code__
        newcode = code.replace(co_name="func")  # Should not raise SystemError
        self.assertEqual(code, newcode)

    def test_empty_linetable(self):
        def func():
            pass
        new_code = code = func.__code__.replace(co_linetable=b'')
        self.assertEqual(list(new_code.co_lines()),
                         [(0, len(new_code.co_code) // 8, None)])

    @unittest.skipIf(_testinternalcapi is None, '_testinternalcapi is missing')
    def test_returns_only_none(self):
        value = True

        def spam1():
            pass
        def spam2():
            return
        def spam3():
            return None
        def spam4():
            if not value:
                return
            ...
        def spam5():
            if not value:
                return None
            ...
        lambda1 = (lambda: None)
        for func in [
            spam1,
            spam2,
            spam3,
            spam4,
            spam5,
            lambda1,
        ]:
            with self.subTest(func):
                res = _testinternalcapi.code_returns_only_none(func.__code__)
                self.assertTrue(res)

        def spam6():
            return True
        def spam7():
            return value
        def spam8():
            if value:
                return None
            return True
        def spam9():
            if value:
                return True
            return None
        lambda2 = (lambda: True)
        for func in [
            spam6,
            spam7,
            spam8,
            spam9,
            lambda2,
        ]:
            with self.subTest(func):
                res = _testinternalcapi.code_returns_only_none(func.__code__)
                self.assertFalse(res)

    def test_invalid_bytecode(self):
        def foo():
            pass

        unused = next(i for i, name in enumerate(opname[:233])
                      if name == f'<{i}>')

        # Change the first handler to an unused number; the constructor
        # rejects words whose handler does not exist
        with self.assertRaisesRegex(ValueError, "invalid executable handler"):
            foo.__code__.replace(
                co_code=unused.to_bytes(2, 'little') + foo.__code__.co_code[2:])

    def test_code_units_padded(self):
        # Cython passes zeroed code in 2-byte units that only spans its line
        # table; the constructor pads it to whole words of CACHE
        def foo():
            pass

        code = foo.__code__.replace(co_code=bytes(12), co_linetable=b'')
        self.assertEqual(code.co_code, bytes(16))
        with self.assertRaisesRegex(ValueError, "co_code is malformed"):
            foo.__code__.replace(co_code=bytes(3))

    def test_replace_inline_constant(self):
        # The assembler loads a compact int or a float with an inline
        # constant handler; replacing the constant keeps the code valid
        def f():
            x = 1000
            y = 2.5
            return x, y

        consts = tuple("spam" if c == 1000 else b"eggs" if c == 2.5 else c
                       for c in f.__code__.co_consts)
        f.__code__ = f.__code__.replace(co_consts=consts)
        self.assertEqual(f(), ("spam", b"eggs"))

    @requires_debug_ranges()
    def test_co_positions_artificial_instructions(self):
        import dis

        namespace = {}
        exec(textwrap.dedent("""\
        try:
            1/0
        except Exception as e:
            exc = e
        """), namespace)

        exc = namespace['exc']
        traceback = exc.__traceback__
        code = traceback.tb_frame.f_code

        artificial_instructions = []
        for instr, positions in instructions_with_positions(
            dis.get_instructions(code), code.co_positions()
        ):
            # If any of the positions is None, then all have to
            # be None as well for the case above. There are still
            # some places in the compiler, where the artificial instructions
            # get assigned the first_lineno but they don't have other positions.
            # There is no easy way of inferring them at that stage, so for now
            # we don't support it.
            self.assertIn(positions.count(None), [0, 3, 4])

            if not any(positions):
                artificial_instructions.append(instr)

        self.assertEqual(
            [
                (instruction.opname, instruction.argval)
                for instruction in artificial_instructions
            ],
            [
                ("PUSH_EXC_INFO", None),
                ("LOAD_COMMON_CONSTANT", None), # artificial 'None'
                ("STORE_NAME", "e"),  # XX: we know the location for this
                ("STORE_NAME", "e"),
                ("RERAISE", 1),
                ("COPY", 3),
                ("POP_EXCEPT", None),
                ("RERAISE", 1)
            ]
        )

    def test_endline_and_columntable_none_when_no_debug_ranges(self):
        # Make sure that if `-X no_debug_ranges` is used, there is
        # minimal debug info
        code = textwrap.dedent("""
            def f():
                pass

            positions = f.__code__.co_positions()
            for line, end_line, column, end_column in positions:
                assert line == end_line
                assert column is None
                assert end_column is None
            """)
        assert_python_ok('-X', 'no_debug_ranges', '-c', code)

    def test_endline_and_columntable_none_when_no_debug_ranges_env(self):
        # Same as above but using the environment variable opt out.
        code = textwrap.dedent("""
            def f():
                pass

            positions = f.__code__.co_positions()
            for line, end_line, column, end_column in positions:
                assert line == end_line
                assert column is None
                assert end_column is None
            """)
        assert_python_ok('-c', code, PYTHONNODEBUGRANGES='1')

    # co_positions behavior when info is missing.

    @requires_debug_ranges()
    def test_co_positions_empty_linetable(self):
        def func():
            x = 1
        new_code = func.__code__.replace(co_linetable=b'')
        positions = list(new_code.co_positions())
        self.assertEqual(len(positions), len(new_code.co_code) // 8)
        for line, end_line, column, end_column in positions:
            self.assertIsNone(line)
            self.assertIsNone(end_line)
            self.assertIsNone(column)
            self.assertIsNone(end_column)

    def test_code_equality(self):
        def f():
            try:
                a()
            except:
                b()
            else:
                c()
            finally:
                d()
        code_a = f.__code__
        code_b = code_a.replace(co_linetable=b"")
        exceptions, recovery = semantic_parts(code_a.co_exceptiontable)
        self.assertTrue(exceptions)
        # A leading zero varint group keeps every admitted exception edge
        # while changing only the semantic bytes used by equality.
        equivalent = bytes((0xc0, exceptions[0] & 0x7f)) + exceptions[1:]
        alternate = semantic_table(equivalent, recovery)
        code_c = code_a.replace(co_exceptiontable=alternate)
        code_d = code_b.replace(co_exceptiontable=alternate)
        self.assertEqual(dis.Bytecode(code_c).exception_entries,
                         dis.Bytecode(code_a).exception_entries)
        self.assertEqual(code_c.co_linetable, code_a.co_linetable)
        self.assertNotEqual(code_a, code_b)
        self.assertNotEqual(code_a, code_c)
        self.assertNotEqual(code_a, code_d)
        self.assertNotEqual(code_b, code_c)
        self.assertNotEqual(code_b, code_d)
        self.assertNotEqual(code_c, code_d)

    def test_code_hash_uses_firstlineno(self):
        c1 = (lambda: 1).__code__
        c2 = (lambda: 1).__code__
        self.assertNotEqual(c1, c2)
        self.assertNotEqual(hash(c1), hash(c2))
        c3 = c1.replace(co_firstlineno=17)
        self.assertNotEqual(c1, c3)
        self.assertNotEqual(hash(c1), hash(c3))

    def test_code_hash_uses_order(self):
        # Swapping posonlyargcount and kwonlyargcount should change the hash.
        c = (lambda x, y, *, z=1, w=1: 1).__code__
        self.assertEqual(c.co_argcount, 2)
        self.assertEqual(c.co_posonlyargcount, 0)
        self.assertEqual(c.co_kwonlyargcount, 2)
        swapped = c.replace(co_posonlyargcount=2, co_kwonlyargcount=0)
        self.assertNotEqual(c, swapped)
        self.assertNotEqual(hash(c), hash(swapped))

    def test_code_hash_uses_bytecode(self):
        c = (lambda x, y: x + y).__code__
        d = (lambda x, y: x * y).__code__
        c1 = c.replace(co_code=d.co_code)
        self.assertNotEqual(c, c1)
        self.assertNotEqual(hash(c), hash(c1))

    @cpython_only
    def test_code_equal_with_instrumentation(self):
        """ GH-109052

        Make sure the instrumentation doesn't affect the code equality
        The validity of this test relies on the fact that "x is x" and
        "x in x" have only one different instruction and the instructions
        have the same argument.

        """
        code1 = compile("x is x", "example.py", "eval")
        code2 = compile("x in x", "example.py", "eval")
        sys._getframe().f_trace_opcodes = True
        sys.settrace(lambda *args: None)
        exec(code1, {'x': []})
        exec(code2, {'x': []})
        self.assertNotEqual(code1, code2)
        sys.settrace(None)

    @unittest.skipIf(_testinternalcapi is None, "missing _testinternalcapi")
    def test_local_kinds(self):
        CO_FAST_ARG_POS = 0x02
        CO_FAST_ARG_KW = 0x04
        CO_FAST_ARG_VAR = 0x08
        CO_FAST_HIDDEN = 0x10
        CO_FAST_LOCAL = 0x20
        CO_FAST_CELL = 0x40
        CO_FAST_FREE = 0x80

        POSONLY = CO_FAST_LOCAL | CO_FAST_ARG_POS
        POSORKW = CO_FAST_LOCAL | CO_FAST_ARG_POS | CO_FAST_ARG_KW
        KWONLY = CO_FAST_LOCAL | CO_FAST_ARG_KW
        VARARGS = CO_FAST_LOCAL | CO_FAST_ARG_VAR | CO_FAST_ARG_POS
        VARKWARGS = CO_FAST_LOCAL | CO_FAST_ARG_VAR | CO_FAST_ARG_KW

        funcs = {
            defs.simple_script: {},
            defs.complex_script: {
                'obj': CO_FAST_LOCAL,
                'pickle': CO_FAST_LOCAL,
                'spam_minimal': CO_FAST_LOCAL,
                'data': CO_FAST_LOCAL,
                'res': CO_FAST_LOCAL,
            },
            defs.script_with_globals: {
                'obj1': CO_FAST_LOCAL,
                'obj2': CO_FAST_LOCAL,
            },
            defs.script_with_explicit_empty_return: {},
            defs.script_with_return: {},
            defs.spam_minimal: {},
            defs.spam_with_builtins: {
                'x': CO_FAST_LOCAL,
                'values': CO_FAST_LOCAL,
                'checks': CO_FAST_LOCAL,
                'res': CO_FAST_LOCAL,
            },
            defs.spam_with_globals_and_builtins: {
                'func1': CO_FAST_LOCAL,
                'func2': CO_FAST_LOCAL,
                'funcs': CO_FAST_LOCAL,
                'checks': CO_FAST_LOCAL,
                'res': CO_FAST_LOCAL,
            },
            defs.spam_with_global_and_attr_same_name: {},
            defs.spam_full_args: {
                'a': POSONLY,
                'b': POSONLY,
                'c': POSORKW,
                'd': POSORKW,
                'e': KWONLY,
                'f': KWONLY,
                'args': VARARGS,
                'kwargs': VARKWARGS,
            },
            defs.spam_full_args_with_defaults: {
                'a': POSONLY,
                'b': POSONLY,
                'c': POSORKW,
                'd': POSORKW,
                'e': KWONLY,
                'f': KWONLY,
                'args': VARARGS,
                'kwargs': VARKWARGS,
            },
            defs.spam_args_attrs_and_builtins: {
                'a': POSONLY,
                'b': POSONLY,
                'c': POSORKW,
                'd': POSORKW,
                'e': KWONLY,
                'f': KWONLY,
                'args': VARARGS,
                'kwargs': VARKWARGS,
            },
            defs.spam_returns_arg: {
                'x': POSORKW,
            },
            defs.spam_raises: {},
            defs.spam_with_inner_not_closure: {
                'eggs': CO_FAST_LOCAL,
            },
            defs.spam_with_inner_closure: {
                'x': CO_FAST_CELL,
                'eggs': CO_FAST_LOCAL,
            },
            defs.spam_annotated: {
                'a': POSORKW,
                'b': POSORKW,
                'c': POSORKW,
            },
            defs.spam_full: {
                'a': POSONLY,
                'b': POSONLY,
                'c': POSORKW,
                'd': POSORKW,
                'e': KWONLY,
                'f': KWONLY,
                'args': VARARGS,
                'kwargs': VARKWARGS,
                'x': CO_FAST_LOCAL,
                'y': CO_FAST_LOCAL,
                'z': CO_FAST_LOCAL,
                'extras': CO_FAST_LOCAL,
            },
            defs.spam: {
                'x': POSORKW,
            },
            defs.spam_N: {
                'x': POSORKW,
                'eggs_nested': CO_FAST_LOCAL,
            },
            defs.spam_C: {
                'x': POSORKW | CO_FAST_CELL,
                'a': CO_FAST_CELL,
                'eggs_closure': CO_FAST_LOCAL,
            },
            defs.spam_NN: {
                'x': POSORKW,
                'eggs_nested_N': CO_FAST_LOCAL,
            },
            defs.spam_NC: {
                'x': POSORKW | CO_FAST_CELL,
                'a': CO_FAST_CELL,
                'eggs_nested_C': CO_FAST_LOCAL,
            },
            defs.spam_CN: {
                'x': POSORKW | CO_FAST_CELL,
                'a': CO_FAST_CELL,
                'eggs_closure_N': CO_FAST_LOCAL,
            },
            defs.spam_CC: {
                'x': POSORKW | CO_FAST_CELL,
                'a': CO_FAST_CELL,
                'eggs_closure_C': CO_FAST_LOCAL,
            },
            defs.eggs_nested: {
                'y': POSORKW,
            },
            defs.eggs_closure: {
                'y': POSORKW,
                'x': CO_FAST_FREE,
                'a': CO_FAST_FREE,
            },
            defs.eggs_nested_N: {
                'y': POSORKW,
                'ham_nested': CO_FAST_LOCAL,
            },
            defs.eggs_nested_C: {
                'y': POSORKW | CO_FAST_CELL,
                'x': CO_FAST_FREE,
                'a': CO_FAST_FREE,
                'ham_closure': CO_FAST_LOCAL,
            },
            defs.eggs_closure_N: {
                'y': POSORKW,
                'x': CO_FAST_FREE,
                'a': CO_FAST_FREE,
                'ham_C_nested': CO_FAST_LOCAL,
            },
            defs.eggs_closure_C: {
                'y': POSORKW | CO_FAST_CELL,
                'b': CO_FAST_CELL,
                'x': CO_FAST_FREE,
                'a': CO_FAST_FREE,
                'ham_C_closure': CO_FAST_LOCAL,
            },
            defs.ham_nested: {
                'z': POSORKW,
            },
            defs.ham_closure: {
                'z': POSORKW,
                'y': CO_FAST_FREE,
                'x': CO_FAST_FREE,
                'a': CO_FAST_FREE,
            },
            defs.ham_C_nested: {
                'z': POSORKW,
            },
            defs.ham_C_closure: {
                'z': POSORKW,
                'y': CO_FAST_FREE,
                'b': CO_FAST_FREE,
                'x': CO_FAST_FREE,
                'a': CO_FAST_FREE,
            },
        }
        assert len(funcs) == len(defs.FUNCTIONS)
        for func in defs.FUNCTIONS:
            with self.subTest(func):
                expected = funcs[func]
                kinds = _testinternalcapi.get_co_localskinds(func.__code__)
                self.assertEqual(kinds, expected)

    @unittest.skipIf(_testinternalcapi is None, "missing _testinternalcapi")
    def test_var_counts(self):
        self.maxDiff = None
        def new_var_counts(*,
                           posonly=0,
                           posorkw=0,
                           kwonly=0,
                           varargs=0,
                           varkwargs=0,
                           purelocals=0,
                           argcells=0,
                           othercells=0,
                           freevars=0,
                           globalvars=0,
                           attrs=0,
                           unknown=0,
                           ):
            nargvars = posonly + posorkw + kwonly + varargs + varkwargs
            nlocals = nargvars + purelocals + othercells
            if isinstance(globalvars, int):
                globalvars = {
                    'total': globalvars,
                    'numglobal': 0,
                    'numbuiltin': 0,
                    'numunknown': globalvars,
                }
            else:
                g_numunknown = 0
                if isinstance(globalvars, dict):
                    numglobal = globalvars['numglobal']
                    numbuiltin = globalvars['numbuiltin']
                    size = 2
                    if 'numunknown' in globalvars:
                        g_numunknown = globalvars['numunknown']
                        size += 1
                    assert len(globalvars) == size, globalvars
                else:
                    assert not isinstance(globalvars, str), repr(globalvars)
                    try:
                        numglobal, numbuiltin = globalvars
                    except ValueError:
                        numglobal, numbuiltin, g_numunknown = globalvars
                globalvars = {
                    'total': numglobal + numbuiltin + g_numunknown,
                    'numglobal': numglobal,
                    'numbuiltin': numbuiltin,
                    'numunknown': g_numunknown,
                }
            unbound = globalvars['total'] + attrs + unknown
            return {
                'total': nlocals + freevars + unbound,
                'locals': {
                    'total': nlocals,
                    'args': {
                        'total': nargvars,
                        'numposonly': posonly,
                        'numposorkw': posorkw,
                        'numkwonly': kwonly,
                        'varargs': varargs,
                        'varkwargs': varkwargs,
                    },
                    'numpure': purelocals,
                    'cells': {
                        'total': argcells + othercells,
                        'numargs': argcells,
                        'numothers': othercells,
                    },
                    'hidden': {
                        'total': 0,
                        'numpure': 0,
                        'numcells': 0,
                    },
                },
                'numfree': freevars,
                'unbound': {
                    'total': unbound,
                    'globals': globalvars,
                    'numattrs': attrs,
                    'numunknown': unknown,
                },
            }

        funcs = {
            defs.simple_script: new_var_counts(),
            defs.complex_script: new_var_counts(
                purelocals=5,
                globalvars=1,
                attrs=2,
            ),
            defs.script_with_globals: new_var_counts(
                purelocals=2,
                globalvars=1,
            ),
            defs.script_with_explicit_empty_return: new_var_counts(),
            defs.script_with_return: new_var_counts(),
            defs.spam_minimal: new_var_counts(),
            defs.spam_minimal: new_var_counts(),
            defs.spam_with_builtins: new_var_counts(
                purelocals=4,
                globalvars=4,
            ),
            defs.spam_with_globals_and_builtins: new_var_counts(
                purelocals=5,
                globalvars=6,
            ),
            defs.spam_with_global_and_attr_same_name: new_var_counts(
                globalvars=2,
                attrs=1,
            ),
            defs.spam_full_args: new_var_counts(
                posonly=2,
                posorkw=2,
                kwonly=2,
                varargs=1,
                varkwargs=1,
            ),
            defs.spam_full_args_with_defaults: new_var_counts(
                posonly=2,
                posorkw=2,
                kwonly=2,
                varargs=1,
                varkwargs=1,
            ),
            defs.spam_args_attrs_and_builtins: new_var_counts(
                posonly=2,
                posorkw=2,
                kwonly=2,
                varargs=1,
                varkwargs=1,
                attrs=1,
            ),
            defs.spam_returns_arg: new_var_counts(
                posorkw=1,
            ),
            defs.spam_raises: new_var_counts(
                globalvars=1,
            ),
            defs.spam_with_inner_not_closure: new_var_counts(
                purelocals=1,
            ),
            defs.spam_with_inner_closure: new_var_counts(
                othercells=1,
                purelocals=1,
            ),
            defs.spam_annotated: new_var_counts(
                posorkw=3,
            ),
            defs.spam_full: new_var_counts(
                posonly=2,
                posorkw=2,
                kwonly=2,
                varargs=1,
                varkwargs=1,
                purelocals=4,
                globalvars=3,
                attrs=1,
            ),
            defs.spam: new_var_counts(
                posorkw=1,
            ),
            defs.spam_N: new_var_counts(
                posorkw=1,
                purelocals=1,
            ),
            defs.spam_C: new_var_counts(
                posorkw=1,
                purelocals=1,
                argcells=1,
                othercells=1,
            ),
            defs.spam_NN: new_var_counts(
                posorkw=1,
                purelocals=1,
            ),
            defs.spam_NC: new_var_counts(
                posorkw=1,
                purelocals=1,
                argcells=1,
                othercells=1,
            ),
            defs.spam_CN: new_var_counts(
                posorkw=1,
                purelocals=1,
                argcells=1,
                othercells=1,
            ),
            defs.spam_CC: new_var_counts(
                posorkw=1,
                purelocals=1,
                argcells=1,
                othercells=1,
            ),
            defs.eggs_nested: new_var_counts(
                posorkw=1,
            ),
            defs.eggs_closure: new_var_counts(
                posorkw=1,
                freevars=2,
            ),
            defs.eggs_nested_N: new_var_counts(
                posorkw=1,
                purelocals=1,
            ),
            defs.eggs_nested_C: new_var_counts(
                posorkw=1,
                purelocals=1,
                argcells=1,
                freevars=2,
            ),
            defs.eggs_closure_N: new_var_counts(
                posorkw=1,
                purelocals=1,
                freevars=2,
            ),
            defs.eggs_closure_C: new_var_counts(
                posorkw=1,
                purelocals=1,
                argcells=1,
                othercells=1,
                freevars=2,
            ),
            defs.ham_nested: new_var_counts(
                posorkw=1,
            ),
            defs.ham_closure: new_var_counts(
                posorkw=1,
                freevars=3,
            ),
            defs.ham_C_nested: new_var_counts(
                posorkw=1,
            ),
            defs.ham_C_closure: new_var_counts(
                posorkw=1,
                freevars=4,
            ),
        }
        assert len(funcs) == len(defs.FUNCTIONS), (len(funcs), len(defs.FUNCTIONS))
        for func in defs.FUNCTIONS:
            with self.subTest(func):
                expected = funcs[func]
                counts = _testinternalcapi.get_code_var_counts(func.__code__)
                self.assertEqual(counts, expected)

        func = defs.spam_with_globals_and_builtins
        with self.subTest(f'{func} code'):
            expected = new_var_counts(
                purelocals=5,
                globalvars=6,
            )
            counts = _testinternalcapi.get_code_var_counts(func.__code__)
            self.assertEqual(counts, expected)

        with self.subTest(f'{func} with own globals and builtins'):
            expected = new_var_counts(
                purelocals=5,
                globalvars=(2, 4),
            )
            counts = _testinternalcapi.get_code_var_counts(func)
            self.assertEqual(counts, expected)

        with self.subTest(f'{func} without globals'):
            expected = new_var_counts(
                purelocals=5,
                globalvars=(0, 4, 2),
            )
            counts = _testinternalcapi.get_code_var_counts(func, globalsns={})
            self.assertEqual(counts, expected)

        with self.subTest(f'{func} without both'):
            expected = new_var_counts(
                purelocals=5,
                globalvars=6,
            )
            counts = _testinternalcapi.get_code_var_counts(func, globalsns={},
                  builtinsns={})
            self.assertEqual(counts, expected)

        with self.subTest(f'{func} without builtins'):
            expected = new_var_counts(
                purelocals=5,
                globalvars=(2, 0, 4),
            )
            counts = _testinternalcapi.get_code_var_counts(func, builtinsns={})
            self.assertEqual(counts, expected)

    @unittest.skipIf(_testinternalcapi is None, "missing _testinternalcapi")
    def test_stateless(self):
        self.maxDiff = None

        STATELESS_FUNCTIONS = [
            *defs.STATELESS_FUNCTIONS,
            # stateless with defaults
            defs.spam_full_args_with_defaults,
        ]

        for func in defs.STATELESS_CODE:
            with self.subTest((func, '(code)')):
                _testinternalcapi.verify_stateless_code(func.__code__)
        for func in STATELESS_FUNCTIONS:
            with self.subTest((func, '(func)')):
                _testinternalcapi.verify_stateless_code(func)

        for func in defs.FUNCTIONS:
            if func not in defs.STATELESS_CODE:
                with self.subTest((func, '(code)')):
                    with self.assertRaises(Exception):
                        _testinternalcapi.verify_stateless_code(func.__code__)

            if func not in STATELESS_FUNCTIONS:
                with self.subTest((func, '(func)')):
                    with self.assertRaises(Exception):
                        _testinternalcapi.verify_stateless_code(func)

    def test_code_richcompare_raise_exception(self):
        class BadStr(str):
            def __eq__(self, _):
                raise RuntimeError("Poison!")

            __hash__ = str.__hash__

        c1 = compile("pass", "test", "exec")
        c2 = c1.replace(co_name=BadStr("poison"))
        c3 = compile("pass", "poison", "exec")
        with self.assertRaises(RuntimeError):
            c2 == c3

def isinterned(s):
    return s is sys.intern(('_' + s + '_')[1:-1])

types_FunctionType = type(lambda: None)


class ConstSlotsTest(unittest.TestCase):
    """Inline scalar constants read in a loop live in hidden slots."""

    @staticmethod
    def loop(n):
        total = 0
        for i in range(n):
            total += i * 3 + 1.5
            if total > 100:
                total -= 7
        return total

    def slots(self, code):
        data = code.co_constslots
        return [(int.from_bytes(data[i:i + 4], sys.byteorder),
                 int.from_bytes(data[i + 4:i + 8], sys.byteorder))
                for i in range(0, len(data), 8)]

    def test_table(self):
        code = self.loop.__code__
        slots = self.slots(code)
        self.assertEqual(len(slots), 4)
        values = [code.co_consts[index] for _, index in slots]
        self.assertEqual(sorted(values), [1.5, 3, 7, 100])
        self.assertEqual([slot for slot, _ in slots],
                         list(range(code.co_nlocals,
                                    code.co_nlocals + len(slots))))
        self.assertEqual(code.co_varnames, ('n', 'total', 'i'))
        self.assertEqual(code.co_nlocals, 3)
        self.assertEqual(self.loop(10), 136.0)

    def test_no_loop_no_slots(self):
        def f(x):
            return x * 3 + 1.5
        self.assertEqual(f.__code__.co_constslots, b'')

    def test_not_in_f_locals(self):
        def f(n):
            seen = None
            for i in range(n):
                seen = sys._getframe().f_locals
                i += 1000
            return seen
        self.assertTrue(f.__code__.co_constslots)
        self.assertEqual(sorted(f(2)), ['i', 'n', 'seen'])

    def test_locals_in_unoptimized_code(self):
        ns = {}
        exec("x = 0\nfor i in range(3):\n    x += 5\nloc = dict(locals())",
             ns)
        self.assertEqual(ns['x'], 15)
        self.assertEqual(sorted(k for k in ns['loc'] if k != '__builtins__'),
                         ['i', 'x'])

    def test_f_locals_write_refused(self):
        def f(n):
            for i in range(n):
                frame = sys._getframe()
                frame.f_locals['.k0'] = 99
                i += 1
            return i
        self.assertEqual(f(3), 3)

    def test_replace_keeps_table(self):
        code = self.loop.__code__
        same = code.replace(co_name='other')
        self.assertEqual(same.co_constslots, code.co_constslots)
        self.assertEqual(same.co_varnames, code.co_varnames)
        self.assertEqual(same.co_nlocals, code.co_nlocals)
        self.assertEqual(same.co_freevars, code.co_freevars)
        self.assertEqual(code.replace(), code)
        self.assertNotEqual(same, code)
        # The words name the slots, so the table cannot go
        with self.assertRaises(ValueError):
            code.replace(co_constslots=b'')

    def test_replace_consts(self):
        code = self.loop.__code__
        slots = self.slots(code)
        consts = list(code.co_consts)
        for _, index in slots:
            if consts[index] == 3:
                consts[index] = 5
        new = code.replace(co_consts=tuple(consts))
        f = types_FunctionType(new, globals())
        self.assertEqual(f(4), 4 * 0 + (0 + 1.5) + (5 + 1.5) + (10 + 1.5)
                         + (15 + 1.5))

    def test_replace_to_mortal_constant(self):
        # A constant that is no scalar any more is held by the frame
        code = self.loop.__code__
        consts = list(code.co_consts)
        for _, index in self.slots(code):
            if consts[index] == 7:
                consts[index] = 2 ** 70 + 7
        new = code.replace(co_consts=tuple(consts))
        f = types_FunctionType(new, globals())
        mortal = [c for c in new.co_consts if type(c) is int
                  and c > 2 ** 69][0]
        before = sys.getrefcount(mortal)
        for _ in range(3):
            self.assertEqual(f(60), self.loop_with_seven(60, 2 ** 70 + 7))
        self.assertEqual(sys.getrefcount(mortal), before)

    def test_mortal_constant_after_overlapping_calls(self):
        class Constant:
            def __rsub__(self, left):
                return left - 7

        code = self.loop.__code__
        consts = list(code.co_consts)
        mortal = Constant()
        indices = [index for _, index in self.slots(code)
                   if consts[index] == 7]
        self.assertEqual(len(indices), 1)
        consts[indices[0]] = mortal
        function = types_FunctionType(code.replace(co_consts=tuple(consts)), globals())

        def caller():
            return function(60)

        expected = self.loop(60)
        before = sys.getrefcount(mortal)
        for _ in range(50):
            self.assertEqual(caller(), expected)
        self.assertEqual(sys.getrefcount(mortal), before)

    @staticmethod
    def loop_with_seven(n, seven):
        total = 0
        for i in range(n):
            total += i * 3 + 1.5
            if total > 100:
                total -= seven
        return total

    def test_malformed_table(self):
        code = self.loop.__code__
        slots = self.slots(code)
        bad = code.co_constslots[:-4]
        with self.assertRaises(ValueError):
            code.replace(co_constslots=bad)
        index = len(code.co_consts)
        data = bytearray(code.co_constslots)
        data[4:8] = index.to_bytes(4, sys.byteorder)
        with self.assertRaises(ValueError):
            code.replace(co_constslots=bytes(data))
        data = bytearray(code.co_constslots)
        data[0:4] = (slots[0][0] + 1).to_bytes(4, sys.byteorder)
        with self.assertRaises(ValueError):
            code.replace(co_constslots=bytes(data))

    def test_table_may_not_add_a_slot_over_temporaries(self):
        import struct
        code = self.loop.__code__
        n = len(self.slots(code))
        extra = code.co_constslots + struct.pack('II', code.co_nlocals + n, 0)
        with self.assertRaisesRegex(ValueError, "writes constant slot|window"):
            code.replace(co_constslots=extra)
        with self.assertRaisesRegex(ValueError, "writes constant slot|window"):
            types.CodeType(
                code.co_argcount, code.co_posonlyargcount,
                code.co_kwonlyargcount, code.co_nlocals, code.co_stacksize,
                code.co_flags, code.co_code, code.co_consts, code.co_names,
                code.co_varnames, code.co_filename, code.co_name,
                code.co_qualname, code.co_firstlineno, code.co_linetable,
                code.co_exceptiontable, code.co_freevars, code.co_cellvars,
                code.co_fbinit, extra)

    def test_table_with_a_mortal_slot_over_temporaries(self):
        import struct
        code = self.loop.__code__
        n = len(self.slots(code))
        extra = code.co_constslots + struct.pack('II', code.co_nlocals + n,
                                                 len(code.co_consts))
        with self.assertRaises(ValueError):
            code.replace(co_consts=code.co_consts + ([1, 2],),
                         co_constslots=extra)

    def test_changing_variable_count(self):
        code = self.loop.__code__
        with self.assertRaisesRegex(
                ValueError, "changing the number of variables"):
            code.replace(co_nlocals=code.co_nlocals + 1,
                         co_varnames=code.co_varnames + ('extra',))
        with self.assertRaisesRegex(
                ValueError, "changing the number of variables"):
            code.replace(co_nlocals=2, co_varnames=code.co_varnames[:2])
        renamed = code.replace(co_varnames=('a', 'b', 'c'))
        self.assertEqual(renamed.co_constslots, code.co_constslots)

    def test_malformed_marshal_table(self):
        import marshal
        import struct
        code = self.loop.__code__
        data = marshal.dumps(code)
        table = code.co_constslots
        at = data.find(table)
        self.assertGreater(at, 0)
        index = len(code.co_consts)
        for bad in (struct.pack('II', *struct.unpack('II', table[:8])[:1]
                                + (index,)) + table[8:],
                    struct.pack('II', 2, 0) + table[8:],
                    table[:8] + table[:8] + table[16:]):
            patched = data[:at] + bad + data[at + len(bad):]
            with self.assertRaises(ValueError):
                marshal.loads(patched)
        self.assertEqual(marshal.loads(data), code)

    def test_async_generator(self):
        async def agen(n):
            total = 0
            for i in range(n):
                total += 2
                yield total * 3
        self.assertTrue(agen.__code__.co_constslots)
        async def collect():
            return [x async for x in agen(3)]
        c = collect()
        try:
            c.send(None)
        except StopIteration as e:
            self.assertEqual(e.value, [6, 12, 18])

    def test_class_body(self):
        class C:
            total = 0
            for i in range(3):
                total += 5
            seen = dict(locals())
        self.assertEqual(C.total, 15)
        self.assertEqual(sorted(C.seen), ['__firstlineno__', '__module__',
                                          '__qualname__', 'i', 'total'])
        self.assertEqual([k for k in vars(C) if k.startswith('.')], [])

    def test_exec_with_closure(self):
        def outer():
            x = 10
            def inner():
                total = 0
                for i in range(3):
                    total += x + 4
                return total
            return inner
        inner = outer()
        self.assertTrue(inner.__code__.co_constslots)
        cell = inner.__closure__
        ns = {'range': range}
        code = inner.__code__
        f = types_FunctionType(code, ns, closure=cell)
        self.assertEqual(f(), 42)

    def test_function_locals_and_vars(self):
        def f(n):
            total = 0
            for i in range(n):
                total += 7
            return locals(), vars(), dict(sys._getframe().f_locals)
        for got in f(2):
            self.assertEqual(got, {'n': 2, 'total': 14, 'i': 1})

    def test_capture_locals(self):
        import traceback
        def f(n):
            total = 0
            for i in range(n):
                total += 7
            1 / 0
        try:
            f(2)
        except ZeroDivisionError as e:
            tb = traceback.TracebackException.from_exception(
                e, capture_locals=True)
            text = "".join(tb.format())
        self.assertIn("total = 14", text)
        self.assertNotIn(".k", text)

    def test_settrace_pathwise_null_input_after_loop(self):
        def f():
            x = y = 2
            if not x:
                return 4
            for i in range(3):
                x + 6
            checkpoint = 8
            return x + y + checkpoint

        first = f.__code__.co_firstlineno
        names = [row[0] for row in _testinternalcapi.get_ir(f.__code__, True)]
        self.assertIn("COPY", names)
        self.assertIn("STORE_FAST", names)
        self.assertTrue(any(name.startswith("FOR_ITER") for name in names))
        original = f.__code__.co_code
        for warm in (0, 20):
            with self.subTest(warm=warm):
                for _ in range(warm):
                    self.assertEqual(f(), 12)
                attempts = []
                events = 0
                def tracer(frame, event, arg):
                    nonlocal events
                    if frame.f_code is f.__code__ and event == "line":
                        events += 1
                        self.assertLess(events, 80)
                        if frame.f_lineno == first + 6 and not attempts:
                            word = _testinternalcapi.get_ir(f.__code__, True)[frame.f_lasti]
                            self.assertEqual(word[0], "INSTRUMENTED_LINE")
                            attempts.append(frame.f_lasti)
                            frame.f_lineno = first + 2
                    return tracer

                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    self.assertEqual(f(), 12)
                finally:
                    sys.settrace(previous)
                self.assertEqual(len(attempts), 1)
                self.assertEqual(f.__code__.co_code, original)

    def test_settrace_pathwise_null_rechecks_warning_redirect(self):
        def f():
            x = y = 2
            if not x:
                return 4
            for i in range(3):
                x + 6
            del x
            checkpoint = 8
            return 42

        first = f.__code__.co_firstlineno
        for nested in (False, True):
            with self.subTest(nested=nested):
                frames, seen = [], []
                def tracer(frame, event, arg):
                    if (frame.f_code is f.__code__ and event == "line" and
                            frame.f_lineno == first + 7 and not frames):
                        frames.append(frame)
                        frame.f_lineno = first + 2
                    return tracer

                def showwarning(*args, **kwargs):
                    frame = frames[0]
                    seen.append(frame.f_lineno)
                    self.assertEqual(len(seen), 1)
                    self.assertEqual(str(args[0]),
                                     "assigning None to 2 unbound locals")
                    if nested:
                        frame.f_locals["x"] = 5
                        frame.f_locals["checkpoint"] = 8
                        frame.f_lineno = first + 8

                previous = sys.gettrace()
                with warnings.catch_warnings():
                    warnings.simplefilter("always", RuntimeWarning)
                    warnings.showwarning = showwarning
                    sys.settrace(tracer)
                    try:
                        self.assertEqual(f(), 42 if nested else 4)
                    finally:
                        sys.settrace(previous)
                self.assertEqual(seen, [first + 7])
                self.assertEqual(len(frames), 1)
                frames.clear()

    def test_settrace_null_input_cannot_replace_method_owner(self):
        calls = []
        class Receiver:
            def method(self, value):
                calls.append(value)
                return value

        def f(obj, go):
            if go:
                return obj.method(
                    7,
                )
            for item in range(2):
                pass
            checkpoint = 1
            return "done"

        first = f.__code__.co_firstlineno
        receiver = Receiver()
        for warm in (0, 20):
            with self.subTest(warm=warm):
                for _ in range(warm):
                    self.assertEqual(f(receiver, False), "done")
                attempts = []
                events = 0
                def tracer(frame, event, arg):
                    nonlocal events
                    if frame.f_code is f.__code__ and event == "line":
                        events += 1
                        self.assertLess(events, 60)
                        if frame.f_lineno == first + 7 and not attempts:
                            before = frame.f_lasti
                            attempts.append(before)
                            with self.assertRaisesRegex(ValueError, "incoming temporary"):
                                frame.f_lineno = first + 3
                            self.assertEqual(frame.f_lasti, before)
                            self.assertIs(frame.f_locals["obj"], receiver)
                    return tracer

                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    self.assertEqual(f(receiver, False), "done")
                finally:
                    sys.settrace(previous)
                self.assertEqual(len(attempts), 1)
                self.assertEqual(calls, [])

    def test_settrace_null_requirements_keep_both_join_obligations(self):
        def lost_null(flag):
            x = y = 2
            if flag:
                left = right = 3
            else:
                left = 0
            for i in range(3):
                x + 6
            checkpoint = 8
            return 42

        def mixed(flag):
            x = y = 2
            if flag:
                left = right = 3
            else:
                for i in range(3):
                    x + 6
            checkpoint = 8
            return 42

        for original, stop, mode in ((lost_null, 8, "lost-null"),
                                     (mixed, 7, "mixed")):
            with self.subTest(mode=mode):
                code = original.__code__
                first = code.co_firstlineno
                rows = _testinternalcapi.get_ir(code)
                calls = [row for row in rows if row[0] == "CALL_NO_SELF"]
                self.assertEqual(len(calls), 1)
                optional_self = calls[0][2] + 1
                self.assertGreaterEqual(optional_self, code.co_nlocals)
                branch_words = [instruction for instruction in dis.get_instructions(code)
                                if instruction.positions.lineno == first + 3]
                copy = next(instruction for instruction in branch_words
                            if instruction.opname == "COPY")
                self.assertEqual(rows[copy.offset][1] + 1, optional_self)
                words = list(memoryview(code.co_code).cast("Q"))
                if mode == "lost-null":
                    # The untaken arm leaves the optional-self home occupied.
                    # Its join with the ordinary NULL arm cannot prove NULL.
                    store = next(instruction for instruction in branch_words
                                 if instruction.opname == "STORE_FAST" and
                                 rows[instruction.offset][1] == optional_self)
                    pc, source = store.offset, optional_self - 1
                else:
                    # The untaken arm requires this incoming owner; the other
                    # arm's CALL requires the same home specifically NULL.
                    pc, source = copy.offset, optional_self
                words[pc] = (words[pc] & ~(4095 << 16)) | (source << 16)
                matching_recovery = memoryview(code.co_exceptiontable).tobytes()
                self.assertEqual(matching_recovery, code.co_exceptiontable)
                self.assertIsNot(matching_recovery, code.co_exceptiontable)
                # The changed arm reads a home already consumed by STORE_FAST
                # (or a NULL COPY input). Admission now rejects this witness
                # before an observer can consider its joined requirements.
                with self.assertRaisesRegex(ValueError, "reads unavailable scratch"):
                    code.replace(co_code=b"".join(
                        word.to_bytes(8, sys.byteorder) for word in words),
                        co_exceptiontable=matching_recovery)
                # Keep a runtime counterpart with valid transfers: the NULL
                # call input is reconstructed by reexecuting the original arm.
                f = types.FunctionType(code, original.__globals__)
                attempts = []
                def tracer(frame, event, arg):
                    if (frame.f_code is code and event == "line" and
                            frame.f_lineno == first + stop and not attempts):
                        before = frame.f_lasti
                        attempts.append(before)
                        unbound = 2 if mode == "lost-null" else 3
                        with self.assertWarnsRegex(
                                RuntimeWarning,
                                f"assigning None to {unbound} unbound locals"):
                            frame.f_lineno = first + 2
                        self.assertNotEqual(frame.f_lasti, before)
                        self.assertIs(frame.f_locals["flag"], False)
                    return tracer

                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    self.assertEqual(f(False), 42)
                finally:
                    sys.settrace(previous)
                self.assertEqual(len(attempts), 1)

    def test_settrace_lineno_jump(self):
        def f():
            total = 0
            for i in range(3):
                total += 5
                total += 6
            return total
        lines = []
        def tracer(frame, event, arg):
            if frame.f_code is not f.__code__:
                return None
            if event == 'line':
                lines.append(frame.f_lineno)
                if len(lines) == 4:
                    # Jump back to the start of the loop body
                    frame.f_lineno = lines[2]
            return tracer
        sys.settrace(tracer)
        try:
            result = f()
        finally:
            sys.settrace(None)
        self.assertIsInstance(result, int)
        self.assertGreater(len(lines), 6)

    def test_settrace_replays_body_with_current_iterator_anchor(self):
        for values in (range(3), iter((0, 1, 2))):
            with self.subTest(values=type(values)):
                def f(values):
                    total = 0
                    for i in values:
                        total += 5
                        total += 6
                    return total

                applied = False
                def tracer(frame, event, arg):
                    nonlocal applied
                    if (frame.f_code is f.__code__ and event == 'line' and
                            frame.f_lineno == f.__code__.co_firstlineno + 4 and
                            not applied):
                        applied = True
                        frame.f_lineno = f.__code__.co_firstlineno + 3
                    return tracer

                old_trace = sys.gettrace()
                sys.settrace(tracer)
                try:
                    self.assertEqual(f(values), 38)
                finally:
                    sys.settrace(old_trace)
                self.assertTrue(applied)

    def test_settrace_cannot_reuse_an_exhausted_loop_anchor(self):
        def f():
            total = 0
            for item in (1,):
                total += 5
            for item in (2,):
                total += 6
            return total

        rejected = False
        def tracer(frame, event, arg):
            nonlocal rejected
            if (frame.f_code is f.__code__ and event == 'line' and
                    frame.f_lineno == f.__code__.co_firstlineno + 5):
                with self.assertRaisesRegex(ValueError, 'incoming temporary'):
                    frame.f_lineno = f.__code__.co_firstlineno + 3
                rejected = True
            return tracer

        old_trace = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(f(), 11)
        finally:
            sys.settrace(old_trace)
        self.assertTrue(rejected)

    def test_settrace_cannot_enter_body_before_iterator_creation(self):
        def f():
            total = 0
            for item in (1,):
                total += 5
            return total

        rejected = False
        def tracer(frame, event, arg):
            nonlocal rejected
            if (frame.f_code is f.__code__ and event == 'line' and
                    frame.f_lineno == f.__code__.co_firstlineno + 1):
                with self.assertRaisesRegex(ValueError, 'incoming temporary'):
                    frame.f_lineno = f.__code__.co_firstlineno + 3
                rejected = True
            return tracer

        old_trace = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(f(), 5)
        finally:
            sys.settrace(old_trace)
        self.assertTrue(rejected)

    def test_settrace_pre_cancellation_commits_before_owner_callbacks(self):
        frames, finalized, calls = [], [], []
        class Callable:
            def __call__(self, value):
                calls.append(value)
            def __del__(inner):
                frame = frames[0]
                finalized.append(frame.f_lineno)
                frame.f_lineno = fallback_line
        def factory():
            return Callable()
        def f(value):
            result = 0
            factory()(
                value)  # cancel
            result = 7  # target
            return result  # fallback

        first = f.__code__.co_firstlineno
        cancel_line, target_line, fallback_line = first + 3, first + 4, first + 5
        redirected = False
        def tracer(frame, event, arg):
            nonlocal redirected
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == cancel_line and not redirected):
                redirected = True
                self.assertEqual(list(f.__code__.co_positions())[frame.f_lasti][0],
                                 cancel_line)
                frames.append(frame)
                frame.f_lineno = target_line
            return tracer

        previous = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(f(object()), 0)
        finally:
            sys.settrace(previous)
            frames.clear()
        self.assertTrue(redirected)
        self.assertEqual(calls, [])
        self.assertEqual(finalized, [target_line])

    def test_settrace_cancellation_after_comprehension_restoration(self):
        def f(values, prebound):
            marker = "outer"
            if not prebound:
                del marker
            result = [
                marker + 1  # active
                for marker in values
            ]
            len(
                result)  # cancel
            return marker, result  # target

        first = f.__code__.co_firstlineno
        cancel_line, target_line = first + 9, first + 10
        for prebound in (True, False):
            with self.subTest(prebound=prebound):
                redirected = False
                def tracer(frame, event, arg):
                    nonlocal redirected
                    if frame.f_code is not f.__code__ or event != "line":
                        return tracer
                    if frame.f_lineno == cancel_line and not redirected:
                        redirected = True
                        if prebound:
                            self.assertEqual(frame.f_locals["marker"], "outer")
                        else:
                            self.assertNotIn("marker", frame.f_locals)
                        self.assertEqual(list(f.__code__.co_positions())[frame.f_lasti][0],
                                         cancel_line)
                        frame.f_lineno = target_line
                    return tracer

                previous = sys.gettrace()
                with warnings.catch_warnings(record=True) as caught:
                    warnings.simplefilter("always", RuntimeWarning)
                    sys.settrace(tracer)
                    try:
                        self.assertEqual(f([1, 2], prebound),
                                         ("outer" if prebound else None, [2, 3]))
                    finally:
                        sys.settrace(previous)
                self.assertEqual(len(caught), 0 if prebound else 1)
                self.assertTrue(all(issubclass(w.category, RuntimeWarning) for w in caught))
                self.assertTrue(redirected)

    def test_settrace_restores_saved_bound_and_unbound_local(self):
        def f(values, prebound):
            marker = "outer"
            if not prebound:
                del marker
            result = None
            result = [
                marker + 1  # active
                for marker in values
            ]
            return marker  # target

        first = f.__code__.co_firstlineno
        active, target = first + 6, first + 9
        self.assertTrue(any(i.opname == "LOAD_FAST_CHECK" and
                            i.positions.lineno == target
                            for i in dis.get_instructions(f)))
        for prebound in (True, False):
            with self.subTest(prebound=prebound):
                redirected = False
                def tracer(frame, event, arg):
                    nonlocal redirected
                    if (frame.f_code is f.__code__ and event == "line" and
                            frame.f_lineno == active and not redirected):
                        redirected = True
                        frame.f_lineno = target
                        if prebound:
                            self.assertEqual(frame.f_locals["marker"], "outer")
                        else:
                            self.assertNotIn("marker", frame.f_locals)
                    return tracer

                previous = sys.gettrace()
                with warnings.catch_warnings():
                    warnings.simplefilter("error", RuntimeWarning)
                    sys.settrace(tracer)
                    try:
                        if prebound:
                            self.assertEqual(f([1, 2], prebound), "outer")
                        else:
                            with self.assertRaisesRegex(UnboundLocalError, "marker"):
                                f([1, 2], prebound)
                    finally:
                        sys.settrace(previous)
                self.assertTrue(redirected)

    def _monitor_branch(self, f, event, callback):
        monitoring = sys.monitoring
        tool = 3
        monitoring.use_tool_id(tool, "test_code committed branch")
        self.addCleanup(monitoring.free_tool_id, tool)
        self.addCleanup(monitoring.register_callback, tool, event, None)
        self.addCleanup(monitoring.set_local_events, tool, f.__code__, 0)
        monitoring.register_callback(tool, event, callback)
        return tool

    def test_monitoring_committed_conditional_redirect(self):
        def f(flag):
            if bool(flag):
                return "original"
            return "redirected"

        first = f.__code__.co_firstlineno
        attempts = []
        def callback(code, src, dst):
            frame = sys._getframe(1)
            self.assertIs(frame.f_code, code)
            self.assertEqual(frame.f_lasti, src)
            word = _testinternalcapi.get_ir(code, True)[src]
            self.assertIn(word[0], ("INSTRUMENTED_POP_JUMP_IF_FALSE",
                                    "INSTRUMENTED_POP_JUMP_IF_TRUE"))
            self.assertEqual(dst, src + 1)
            attempts.append(src)
            self.assertLessEqual(len(attempts), 1)
            frame.f_lineno = first + 3
            self.assertEqual(frame.f_lineno, first + 3)

        tool = self._monitor_branch(f, sys.monitoring.events.BRANCH_LEFT, callback)
        for warm in (0, 20):
            with self.subTest(warm=warm):
                sys.monitoring.set_local_events(tool, f.__code__, 0)
                for _ in range(warm):
                    self.assertEqual(f(True), "original")
                attempts.clear()
                sys.monitoring.set_local_events(tool, f.__code__,
                                                sys.monitoring.events.BRANCH_LEFT)
                self.assertEqual(f(True), "redirected")
                self.assertEqual(len(attempts), 1)

    def test_monitoring_committed_conditional_requires_target_owners(self):
        def f(values, flag):
            for outer in values:
                if bool(flag):
                    checkpoint = outer
            return "done"

        first = f.__code__.co_firstlineno
        self.assertIn("FOR_ITER", [word[0] for word in
                                   _testinternalcapi.get_ir(f.__code__)])
        attempts = []
        def callback(code, src, dst):
            frame = sys._getframe(1)
            self.assertIs(frame.f_code, code)
            if frame.f_lineno != first + 2 or attempts:
                return
            self.assertIn(_testinternalcapi.get_ir(code, True)[src][0],
                          ("INSTRUMENTED_POP_JUMP_IF_FALSE",
                           "INSTRUMENTED_POP_JUMP_IF_TRUE"))
            self.assertNotEqual(dst, src + 1)
            attempts.append(src)
            before = dict(frame.f_locals)
            with self.assertRaisesRegex(ValueError, "incoming temporary register owner"):
                frame.f_lineno = first + 3
            self.assertEqual(frame.f_lasti, src)
            self.assertEqual(dict(frame.f_locals), before)

        tool = self._monitor_branch(f, sys.monitoring.events.BRANCH_RIGHT, callback)
        for warm in (0, 20):
            with self.subTest(warm=warm):
                sys.monitoring.set_local_events(tool, f.__code__, 0)
                for _ in range(warm):
                    self.assertEqual(f([1], True), "done")
                attempts.clear()
                sys.monitoring.set_local_events(tool, f.__code__,
                                                sys.monitoring.events.BRANCH_RIGHT)
                self.assertEqual(f([1], True), "done")
                self.assertEqual(len(attempts), 1)

    def test_monitoring_committed_conditional_keeps_saved_binding_obligation(self):
        def f(values):
            marker = "outer"
            result = [marker for marker in values if bool(marker)]
            return marker, result

        first = f.__code__.co_firstlineno
        self.assertIn("LOAD_FAST_AND_CLEAR", [word[0] for word in
                                              _testinternalcapi.get_ir(f.__code__)])
        attempts = []
        def callback(code, src, dst):
            frame = sys._getframe(1)
            self.assertIs(frame.f_code, code)
            word = _testinternalcapi.get_ir(code, True)[src]
            if attempts or word[0] not in ("INSTRUMENTED_POP_JUMP_IF_FALSE",
                                           "INSTRUMENTED_POP_JUMP_IF_TRUE"):
                return
            self.assertEqual(frame.f_lineno, first + 2)
            self.assertNotEqual(dst, src + 1)
            attempts.append(src)
            self.assertEqual(frame.f_locals["marker"], "inner")
            with self.assertRaisesRegex(ValueError, "incoming temporary register owner"):
                frame.f_lineno = first + 3
            self.assertEqual(frame.f_lasti, src)
            self.assertEqual(frame.f_locals["marker"], "inner")

        tool = self._monitor_branch(f, sys.monitoring.events.BRANCH_RIGHT, callback)
        for warm in (0, 20):
            with self.subTest(warm=warm):
                sys.monitoring.set_local_events(tool, f.__code__, 0)
                for _ in range(warm):
                    self.assertEqual(f(["inner"]), ("outer", ["inner"]))
                attempts.clear()
                sys.monitoring.set_local_events(tool, f.__code__,
                                                sys.monitoring.events.BRANCH_RIGHT)
                self.assertEqual(f(["inner"]), ("outer", ["inner"]))
                self.assertEqual(len(attempts), 1)

    def test_monitoring_committed_redirect_honors_close_time_redirect(self):
        frames, closed = [], []
        def sink(*args):
            self.fail("cancelled call executed")
        class Guard:
            def __del__(inner):
                frame = frames[0]
                closed.append((frame.f_lineno, frame.f_locals["value"]))
                frame.f_lineno = first + 5
        def f(flag):
            value = 0
            sink(Guard(),
                 "yes" if bool(flag) else "no")
            value = 7
            return value

        first = f.__code__.co_firstlineno
        attempts = []
        def callback(code, src, dst):
            frame = sys._getframe(1)
            self.assertIs(frame.f_code, code)
            frames.append(frame)
            attempts.append(src)
            self.assertLessEqual(len(attempts), 1)
            frame.f_lineno = first + 4

        tool = self._monitor_branch(f, sys.monitoring.events.BRANCH_LEFT, callback)
        sys.monitoring.register_callback(tool, sys.monitoring.events.LINE,
                                         lambda *args: None)
        self.addCleanup(sys.monitoring.register_callback, tool,
                        sys.monitoring.events.LINE, None)
        sys.monitoring.set_local_events(tool, f.__code__,
                                        sys.monitoring.events.BRANCH_LEFT |
                                        sys.monitoring.events.LINE)
        # This witnesses our explicit close transaction, not a language promise
        # of prompt finalization in another implementation.
        self.assertEqual(f(True), 0)
        self.assertEqual(len(attempts), 1)
        self.assertEqual(closed, [(first + 4, 0)])
        frames.clear()

    def test_monitoring_committed_redirect_rechecks_warning_state(self):
        def f(flag, bind):
            if bind:
                missing = 1
            if bool(flag):
                return "original"
            return "redirected", missing

        first = f.__code__.co_firstlineno
        attempts, seen, frames = [], [], []
        def callback(code, src, dst):
            frame = sys._getframe(1)
            self.assertIs(frame.f_code, code)
            if frame.f_lineno != first + 3:
                return
            attempts.append(src)
            self.assertLessEqual(len(attempts), 1)
            frames.append(frame)
            frame.f_lineno = first + 5
        def showwarning(*args, **kwargs):
            frame = frames[0]
            seen.append(frame.f_lineno)  # Public getter changes only its cache.
            self.assertEqual(len(seen), 1)
            if nested:
                frame.f_locals["missing"] = 5
                frame.f_lineno = first + 4

        tool = self._monitor_branch(f, sys.monitoring.events.BRANCH_LEFT, callback)
        for nested in (False, True):
            with self.subTest(nested=nested):
                attempts.clear()
                seen.clear()
                frames.clear()
                sys.monitoring.set_local_events(tool, f.__code__,
                                                sys.monitoring.events.BRANCH_LEFT)
                with warnings.catch_warnings():
                    warnings.simplefilter("always", RuntimeWarning)
                    warnings.showwarning = showwarning
                    self.assertEqual(f(True, False),
                                     "original" if nested else ("redirected", None))
                self.assertEqual(len(attempts), 1)
                self.assertEqual(seen, [first + 3])
        frames.clear()

    def test_settrace_restores_binding_from_committed_backedge(self):
        def f(values, prebound):
            marker = "outer"
            if not prebound:
                del marker
            result = [marker for marker in values]
            return marker

        first = f.__code__.co_firstlineno
        target = first + 5
        for prebound in (True, False):
            for warm in (0, 20):
                with self.subTest(prebound=prebound, warm=warm):
                    for _ in range(warm):
                        try:
                            f([1, 2], prebound)
                        except UnboundLocalError:
                            self.assertFalse(prebound)
                    redirected = []
                    def tracer(frame, event, arg):
                        if frame.f_code is f.__code__ and event == "line":
                            word = _testinternalcapi.get_ir(f.__code__, True)[frame.f_lasti]
                            if word[0] == "INSTRUMENTED_JUMP_BACKWARD" and not redirected:
                                redirected.append(frame.f_lasti)
                                self.assertEqual(frame.f_lineno, first + 4)
                                frame.f_lineno = target
                                if prebound:
                                    self.assertEqual(frame.f_locals["marker"], "outer")
                                else:
                                    self.assertNotIn("marker", frame.f_locals)
                        return tracer

                    previous = sys.gettrace()
                    with warnings.catch_warnings():
                        warnings.simplefilter("ignore", RuntimeWarning)
                        sys.settrace(tracer)
                        try:
                            if prebound:
                                self.assertEqual(f([1, 2], prebound), "outer")
                            else:
                                with self.assertRaisesRegex(UnboundLocalError, "marker"):
                                    f([1, 2], prebound)
                        finally:
                            sys.settrace(previous)
                    self.assertEqual(len(redirected), 1)

    def test_settrace_committed_backedge_requires_target_owners(self):
        def f(values):
            for outer in values:
                checkpoint = outer
            result = [marker for marker in values]
            return result

        first = f.__code__.co_firstlineno
        attempted = []
        def tracer(frame, event, arg):
            if frame.f_code is f.__code__ and event == "line":
                word = _testinternalcapi.get_ir(f.__code__, True)[frame.f_lasti]
                if (word[0] == "INSTRUMENTED_JUMP_BACKWARD" and
                        frame.f_lineno == first + 3 and not attempted):
                    attempted.append(frame.f_lasti)
                    with self.assertRaisesRegex(
                            ValueError, "incoming temporary register owner"):
                        frame.f_lineno = first + 2
                    self.assertEqual(frame.f_lasti, attempted[0])
            return tracer

        previous = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(f([1, 2]), [1, 2])
        finally:
            sys.settrace(previous)
        self.assertEqual(len(attempted), 1)

    def test_settrace_committed_backedge_requires_saved_origin(self):
        def f(values):
            marker = "outer"
            result = [marker for marker in values]
            return marker

        instruction = next(i for i in dis.get_instructions(f)
                           if i.opname == "LOAD_FAST_AND_CLEAR")
        code = f.__code__
        words = list(struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code))
        words[instruction.offset] = ((words[instruction.offset] & ~(4095 << 16)) |
                                      (4095 << 16))
        with self.assertRaisesRegex(ValueError, "reads unavailable scratch"):
            code.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                         co_exceptiontable=memoryview(code.co_exceptiontable).tobytes())
        attempted = []
        def tracer(frame, event, arg):
            if frame.f_code is f.__code__ and event == "line":
                word = _testinternalcapi.get_ir(f.__code__, True)[frame.f_lasti]
                if word[0] == "INSTRUMENTED_JUMP_BACKWARD" and not attempted:
                    attempted.append(frame.f_lasti)
                    # The valid image still needs the actual saved-origin
                    # recipe to restore the outer binding at this backedge.
                    with self.assertWarnsRegex(
                            RuntimeWarning, "assigning None to 1 unbound local"):
                        frame.f_lineno = code.co_firstlineno + 3
                    self.assertEqual(frame.f_locals["marker"], "outer")
            return tracer

        previous = sys.gettrace()
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            sys.settrace(tracer)
            try:
                self.assertEqual(f([1, 2]), "outer")
            finally:
                sys.settrace(previous)
        self.assertEqual(len(attempted), 1)

    def test_settrace_backedge_restore_honors_close_time_redirect(self):
        frames, closed, errors = [], [], []
        def consume(value):
            return 0
        class Item:
            def __del__(self):
                frame = frames[0]
                closed.append((frame.f_locals["marker"], frame.f_lineno))
                try:
                    frame.f_lineno = frame.f_code.co_firstlineno + 5
                except Exception as error:
                    errors.append(error)
        def values():
            yield Item()
        def f(items):
            marker = "outer"
            checkpoint = 0
            result = [consume(marker) for marker in items]
            if checkpoint:
                return marker, checkpoint
            checkpoint = 7
            return marker, checkpoint

        first = f.__code__.co_firstlineno
        redirected = []
        def tracer(frame, event, arg):
            if frame.f_code is f.__code__:
                if event == "call":
                    frames.append(frame)
                elif event == "line":
                    word = _testinternalcapi.get_ir(f.__code__, True)[frame.f_lasti]
                    if word[0] == "INSTRUMENTED_JUMP_BACKWARD" and not redirected:
                        redirected.append(frame.f_lasti)
                        frame.f_lineno = first + 6
            return tracer

        previous = sys.gettrace()
        # This checks our explicit close transaction, not a Python guarantee
        # that another interpreter promptly destroys the iteration value.
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", RuntimeWarning)
            sys.settrace(tracer)
            try:
                self.assertEqual(f(values()), ("outer", 0))
            finally:
                sys.settrace(previous)
        self.assertEqual(len(redirected), 1)
        self.assertEqual(closed, [("outer", first + 6)])
        self.assertEqual(errors, [])
        frames.clear()

    def test_settrace_saved_null_refuses_unchecked_target_binding(self):
        def raw(flag, values, obj):
            result = None
            if flag:
                marker = 1
                return marker  # target: compiler-proven binding
            result = [
                marker + 1  # active
                for marker in values
            ]
            return result

        def folded(flag, values, obj):
            result = None
            if flag:
                marker = 1
                return marker + 1  # target: folded local input
            result = [
                marker + 1  # active
                for marker in values
            ]
            return result

        def attribute(flag, values, obj):
            result = None
            if flag:
                marker = 1
                obj.attr = marker  # target: NULL would delete
                return marker
            result = [
                marker + 1  # active
                for marker in values
            ]
            return result

        class Owner:
            pass

        for f, bound_result, active_offset in ((raw, 1, 6), (folded, 2, 6),
                                               (attribute, 1, 7)):
            with self.subTest(function=f.__name__):
                first = f.__code__.co_firstlineno
                target, active = first + 4, first + active_offset
                obj = Owner()
                self.assertEqual(f(True, [1, 2], obj), bound_result)
                sentinel = object()
                obj.attr = sentinel
                target_ops = [i for i in dis.get_instructions(f)
                              if i.positions.lineno == target]
                self.assertTrue(target_ops)
                self.assertFalse(any(i.opname == "LOAD_FAST_CHECK"
                                     for i in target_ops))
                attempted = False
                def tracer(frame, event, arg):
                    nonlocal attempted
                    if (frame.f_code is f.__code__ and event == "line" and
                            frame.f_lineno == active and not attempted):
                        attempted = True
                        pc = frame.f_lasti
                        binding = frame.f_locals["marker"]
                        with self.assertRaisesRegex(
                                ValueError, "incoming temporary register owner"):
                            frame.f_lineno = target
                        self.assertEqual(frame.f_lasti, pc)
                        self.assertEqual(frame.f_lineno, active)
                        self.assertIs(frame.f_locals["marker"], binding)
                    return tracer

                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    self.assertEqual(f(False, [1, 2], obj), [2, 3])
                finally:
                    sys.settrace(previous)
                self.assertTrue(attempted)
                self.assertIs(obj.attr, sentinel)

    def test_settrace_restores_distinct_locals_before_closing(self):
        def f(values):
            left, right = "left", "right"
            result = None
            result = [
                left + right  # active
                for left, right in values
            ]
            return left, right, result  # target

        first = f.__code__.co_firstlineno
        redirected = False
        def tracer(frame, event, arg):
            nonlocal redirected
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == first + 4 and not redirected):
                redirected = True
                frame.f_lineno = first + 7
                self.assertEqual((frame.f_locals["left"], frame.f_locals["right"]),
                                 ("left", "right"))
            return tracer

        previous = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(f([(1, 2)]), ("left", "right", None))
        finally:
            sys.settrace(previous)
        self.assertTrue(redirected)

    def test_settrace_restores_oldest_same_local_binding(self):
        def f(values):
            left, right = "outer", "right"
            result = None
            result = [
                left + right  # active
                for left, right in values
            ]
            return left, right, result  # target

        original = f.__code__
        instructions = list(dis.get_instructions(f))
        saves = [i for i in instructions if i.opname == "LOAD_FAST_AND_CLEAR"]
        self.assertEqual([i.argval for i in saves], ["left", "right"])
        restores = [i for i in instructions if i.offset > saves[-1].offset and
                    i.opname == "STORE_FAST" and i.argval in ("left", "right")]
        outer, inner = restores[:2]
        self.assertEqual([outer.argval, inner.argval], ["left", "right"])
        words = list(struct.unpack(f'<{len(original.co_code) // 8}Q', original.co_code))
        local = (words[saves[0].offset] >> 28) & 4095
        outer_home = (words[outer.offset] >> 16) & 4095
        inner_home = (words[inner.offset] >> 16) & 4095
        self.assertNotEqual(outer_home, inner_home)
        # Both noncyclic SAVE producers now name left. The second saves its
        # genuine NULL binding. Restore inner then outer on normal and EH
        # paths, rather than imposing the original distinct-local order.
        words[saves[1].offset] = ((words[saves[1].offset] & ~(4095 << 28)) |
                                 (local << 28))
        for instruction in restores:
            words[instruction.offset] = ((words[instruction.offset] & ~(4095 << 28)) |
                                         (local << 28))
        words[outer.offset] = ((words[outer.offset] & ~(4095 << 16)) |
                               (inner_home << 16))
        words[inner.offset] = ((words[inner.offset] & ~(4095 << 16)) |
                               (outer_home << 16))
        self.assertEqual({(words[i.offset] >> 28) & 4095 for i in saves}, {local})
        f.__code__ = original.replace(
            co_code=struct.pack(f'<{len(words)}Q', *words),
            co_exceptiontable=memoryview(original.co_exceptiontable).tobytes())
        self.assertEqual(f([(1, 2)]), ("outer", 2, [3]))

        first = f.__code__.co_firstlineno
        redirected = False
        def tracer(frame, event, arg):
            nonlocal redirected
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == first + 4 and not redirected):
                redirected = True
                frame.f_lineno = first + 7
            return tracer

        previous = sys.gettrace()
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            sys.settrace(tracer)
            try:
                self.assertEqual(f([(1, 2)]), ("outer", 2, None))
            finally:
                sys.settrace(previous)
        self.assertTrue(redirected)

    def test_settrace_restoration_commit_precedes_nested_finalizer_redirect(self):
        frames, finalized, calls = [], [], []
        class Item:
            def __del__(self):
                frame = frames[0]
                finalized.append((frame.f_locals["marker"], frame.f_lineno))
                frame.f_lineno = nested

        def values():
            yield Item()

        def consume(item):
            calls.append(item)

        def f():
            marker = "outer"
            checkpoint = 0
            result = None
            result = [
                consume(marker)  # active
                for marker in values()
            ]
            checkpoint = 7  # target
            return marker, checkpoint  # nested

        first = f.__code__.co_firstlineno
        active, target, nested = first + 5, first + 8, first + 9
        redirected = False
        def tracer(frame, event, arg):
            nonlocal redirected
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == active and not redirected):
                redirected = True
                frames.append(frame)
                frame.f_lineno = target
            return tracer

        previous = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(f(), ("outer", 0))
        finally:
            sys.settrace(previous)
            frames.clear()
        self.assertTrue(redirected)
        self.assertEqual(calls, [])
        self.assertEqual(finalized, [("outer", target)])

    def test_settrace_cyclic_saved_generation_requires_witness(self):
        def f(values):
            left = "outer"
            result = None
            result = [
                [left + 1 for left in left]  # active
                for left in values
            ]
            return left, result  # target

        first = f.__code__.co_firstlineno
        saves = [i for i in dis.get_instructions(f) if i.opname == "LOAD_FAST_AND_CLEAR"]
        self.assertEqual(len(saves), 2)
        inner_save = saves[-1].offset
        refused = False
        def tracer(frame, event, arg):
            nonlocal refused
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == first + 4 and frame.f_lasti > inner_save and
                    not refused):
                # The inner SAVE producer is reachable again through the
                # outer loop. Static producer identity is not its generation.
                refused = True
                with self.assertRaisesRegex(ValueError, "incoming temporary"):
                    frame.f_lineno = first + 7
            return tracer

        previous = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(f([(1, 2)]), ("outer", [[2, 3]]))
        finally:
            sys.settrace(previous)
        self.assertTrue(refused)

    def test_settrace_saved_cell_requires_restoration_recipe(self):
        def f(values):
            marker = "outer"
            result = None
            result = [
                lambda: marker  # active
                for marker in values
            ]
            return marker, result  # target

        self.assertIn("marker", f.__code__.co_cellvars)
        self.assertTrue(any(i.opname == "LOAD_FAST_AND_CLEAR" and i.argval == "marker"
                            for i in dis.get_instructions(f)))
        first = f.__code__.co_firstlineno
        refused = False
        def tracer(frame, event, arg):
            nonlocal refused
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == first + 4 and not refused):
                refused = True
                with self.assertRaisesRegex(ValueError, "incoming temporary"):
                    frame.f_lineno = first + 7
            return tracer

        previous = sys.gettrace()
        sys.settrace(tracer)
        try:
            marker, result = f([1, 2])
        finally:
            sys.settrace(previous)
        self.assertTrue(refused)
        self.assertEqual(marker, "outer")
        self.assertEqual([item() for item in result], [2, 2])

    def test_settrace_lost_saved_origin_remains_an_obligation(self):
        def f(value):
            copy = value
            checkpoint = 1
            return copy

        instruction = next(i for i in dis.get_instructions(f)
                           if i.opname in ("LOAD_FAST", "LOAD_FAST_BORROW") and
                           i.argval == "value")
        words = list(struct.unpack(f'<{len(f.__code__.co_code) // 8}Q', f.__code__.co_code))
        # Discard the saved value on purpose. The proof must not confuse a
        # missing token home with a completed named-local restoration.
        words[instruction.offset] = (dis.opmap["LOAD_FAST_AND_CLEAR"] |
                                      (4095 << 16) |
                                      (f.__code__.co_varnames.index("value") << 28))
        f.__code__ = f.__code__.replace(
            co_code=struct.pack(f'<{len(words)}Q', *words),
            co_exceptiontable=f.__code__.co_exceptiontable)
        class Finished(Exception):
            pass
        refused = False
        first = f.__code__.co_firstlineno
        def tracer(frame, event, arg):
            nonlocal refused
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == first + 2):
                with self.assertRaisesRegex(ValueError, "incoming temporary"):
                    frame.f_lineno = first + 3
                refused = True
                # Do not execute the deliberately uninitialized copy load.
                raise Finished
            return tracer

        previous = sys.gettrace()
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            sys.settrace(tracer)
            try:
                with self.assertRaises(Finished):
                    f(object())
            finally:
                sys.settrace(previous)
        self.assertTrue(refused)

    def test_settrace_each_distinct_local_requires_restoration(self):
        def f(run):
            left = 10
            right = 20
            result = None
            checkpoint = 0
            if run:
                result = [(left, right) for left, right in [(1, 2)]]
            checkpoint = 1
            return checkpoint

        instructions = list(dis.get_instructions(f))
        saves = [i for i in instructions if i.opname == "LOAD_FAST_AND_CLEAR"]
        self.assertEqual(len(saves), 2)
        first_name, second_name = (i.argval for i in saves)
        second_restore = next(i for i in instructions
                           if i.offset > saves[-1].offset and
                           i.opname == "STORE_FAST" and i.argval == second_name)
        first_restore = next(i for i in instructions
                             if i.offset > saves[-1].offset and
                             i.opname == "STORE_FAST" and i.argval == first_name)
        original = f.__code__
        words = list(struct.unpack(f'<{len(original.co_code) // 8}Q', original.co_code))
        first_word = words[first_restore.offset]
        # Another write to the first local cannot replace restoration of
        # the distinct second local. That obligation remains active.
        self.assertNotEqual((words[second_restore.offset] >> 16) & 4095,
                            (first_word >> 16) & 4095)
        # Preserve the second saved value's input home; consuming the first
        # home again would fail image admission before restoration is tested.
        words[second_restore.offset] = (
            (words[second_restore.offset] & ~(4095 << 28)) |
            (first_word & (4095 << 28)))
        bad = original.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                               co_exceptiontable=original.co_exceptiontable)
        first = original.co_firstlineno
        for invalid in (False, True):
            with self.subTest(invalid=invalid):
                f.__code__ = bad if invalid else original
                visited = False
                def tracer(frame, event, arg):
                    nonlocal visited
                    if (frame.f_code is f.__code__ and event == "line" and
                            frame.f_lineno == first + 7 and not visited):
                        visited = True
                        if invalid:
                            with self.assertRaisesRegex(ValueError, "incoming temporary"):
                                frame.f_lineno = first + 8
                        else:
                            frame.f_lineno = first + 8
                    return tracer
                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    # The malformed branch is never executed. Both static
                    # ingresses must nevertheless agree before cancellation.
                    self.assertEqual(f(False), 1 if invalid else 0)
                finally:
                    sys.settrace(previous)
                self.assertTrue(visited)

    def test_settrace_same_local_ancestor_cannot_skip_younger_save(self):
        def f(run):
            left = 10
            result = None
            checkpoint = 0
            if run:
                result = [[left for left in left] for left in [(1, 2)]]
            checkpoint = 1
            return checkpoint, left, result

        original = f.__code__
        instructions = list(dis.get_instructions(f))
        saves = [i for i in instructions if i.opname == "LOAD_FAST_AND_CLEAR"]
        self.assertEqual(len(saves), 2)
        self.assertEqual([i.argval for i in saves], ["left", "left"])
        restores = [i for i in instructions
                    if i.offset > saves[-1].offset and
                    i.opname == "STORE_FAST" and i.argval == "left"]
        self.assertGreaterEqual(len(restores), 2)
        inner, outer = restores[:2]
        words = list(struct.unpack(f'<{len(original.co_code) // 8}Q', original.co_code))
        self.assertEqual(len({(words[i.offset] >> 28) & 4095 for i in saves}), 1)
        inner_home = (words[inner.offset] >> 16) & 4095
        outer_home = (words[outer.offset] >> 16) & 4095
        self.assertNotEqual(inner_home, outer_home)
        # The younger same-local save is still active. Supplying its ancestor
        # token must not be accepted as a restoration of the younger binding.
        words[inner.offset] = ((words[inner.offset] & ~(4095 << 16)) |
                                (outer_home << 16))
        # This changed restoration can retain scratch owners until return.
        # Reprove the general form instead of inheriting an EMPTY certificate.
        return_forms = {
            dis._all_opmap["RETURN_VALUE_EMPTY"]: dis._all_opmap["RETURN_VALUE"],
            dis._all_opmap["INSTRUMENTED_RETURN_VALUE_EMPTY"]:
                dis._all_opmap["INSTRUMENTED_RETURN_VALUE"],
        }
        words = [(word & ~65535) | return_forms.get(word & 65535, word & 65535)
                 for word in words]
        matching_recovery = memoryview(original.co_exceptiontable).tobytes()
        self.assertEqual(matching_recovery, original.co_exceptiontable)
        self.assertIsNot(matching_recovery, original.co_exceptiontable)
        bad = original.replace(co_code=struct.pack(f'<{len(words)}Q', *words),
                               co_exceptiontable=matching_recovery)
        self.assertEqual(f(True), (1, 10, [[1, 2]]))
        first = original.co_firstlineno
        for invalid in (False, True):
            with self.subTest(invalid=invalid):
                f.__code__ = bad if invalid else original
                visited = False
                def tracer(frame, event, arg):
                    nonlocal visited
                    if (frame.f_code is f.__code__ and event == "line" and
                            frame.f_lineno == first + 6 and not visited):
                        visited = True
                        if invalid:
                            with self.assertRaisesRegex(ValueError, "incoming temporary"):
                                frame.f_lineno = first + 7
                        else:
                            frame.f_lineno = first + 7
                    return tracer
                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    # Exercise the well-formed ingress only; the malformed
                    # branch contributes solely to the cold proof's join.
                    self.assertEqual(f(False), (1 if invalid else 0, 10, None))
                finally:
                    sys.settrace(previous)
                self.assertTrue(visited)

    def test_settrace_inherited_protected_redirects(self):
        for entering in (False, True):
            with self.subTest(entering=entering):
                source = """def f():
    value = 0
    value += 10  # outside
    try:
        value += 1  # target
        value += 2  # inside
    except LookupError:
        value = -1
    return value
"""
                namespace = {}
                exec(source, namespace)
                f = namespace["f"]
                source_line = next(i for i, line in enumerate(source.splitlines(), 1)
                                   if ("# outside" if entering else "# inside") in line)
                target_line = next(i for i, line in enumerate(source.splitlines(), 1)
                                   if "# target" in line)
                redirected = False
                def tracer(frame, event, arg):
                    nonlocal redirected
                    if (frame.f_code is f.__code__ and event == "line" and
                            frame.f_lineno == source_line and not redirected):
                        redirected = True
                        frame.f_lineno = target_line
                    return tracer

                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    result = f()
                finally:
                    sys.settrace(previous)
                self.assertTrue(redirected)
                self.assertEqual(result, 3 if entering else 14)

    def test_settrace_same_caught_redirect_preserves_exception_owners(self):
        source = """def f():
    value = 0
    before = after = None
    try:
        1 / 0
    except ZeroDivisionError as error:
        before = sys.exception()
        value += 1  # target
        value += 2  # source
        after = sys.exception()
    return value, before, after
"""
        namespace = {"sys": sys}
        exec(source, namespace)
        f = namespace["f"]
        target_line = next(i for i, line in enumerate(source.splitlines(), 1)
                           if "# target" in line)
        source_line = next(i for i, line in enumerate(source.splitlines(), 1)
                           if "# source" in line)
        redirected = False
        def tracer(frame, event, arg):
            nonlocal redirected
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == source_line and not redirected):
                redirected = True
                inherited = sys.exception()
                self.assertIs(inherited, frame.f_locals["error"])
                try:
                    raise LookupError("callback owned")
                except LookupError as callback:
                    frame.f_lineno = target_line
                    self.assertIs(sys.exception(), callback)
                self.assertIs(sys.exception(), inherited)
            return tracer

        previous = sys.gettrace()
        sys.settrace(tracer)
        try:
            value, before, after = f()
        finally:
            sys.settrace(previous)
        self.assertTrue(redirected)
        self.assertEqual(value, 4)
        self.assertIsInstance(before, ZeroDivisionError)
        self.assertIs(before, after)

    def test_settrace_forward_caught_redirect_preserves_exception_owners(self):
        source = """def f():
    value = 0
    before = after = None
    try:
        1 / 0
    except ZeroDivisionError as error:
        before = sys.exception()
        value += 1
        value += 2  # source
        value += 4  # target
        after = sys.exception()
    return value, before, after
"""
        namespace = {"sys": sys}
        exec(source, namespace)
        f = namespace["f"]
        target_line = next(i for i, line in enumerate(source.splitlines(), 1)
                           if "# target" in line)
        source_line = next(i for i, line in enumerate(source.splitlines(), 1)
                           if "# source" in line)
        for warm in (0, 20):
            with self.subTest(warm=warm):
                for _ in range(warm):
                    self.assertEqual(f()[0], 7)
                observed = []
                events = 0
                def tracer(frame, event, arg):
                    nonlocal events
                    if frame.f_code is f.__code__ and event == "line":
                        events += 1
                        self.assertLess(events, 80)
                        if frame.f_lineno == source_line and not observed:
                            inherited = sys.exception()
                            observed.append(inherited)
                            self.assertIs(inherited, frame.f_locals["error"])
                            try:
                                raise LookupError("callback owned")
                            except LookupError as callback:
                                frame.f_lineno = target_line
                                self.assertIs(sys.exception(), callback)
                            self.assertIs(sys.exception(), inherited)
                    return tracer

                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    value, before, after = f()
                finally:
                    sys.settrace(previous)
                self.assertEqual(value, 5)
                self.assertEqual(len(observed), 1)
                self.assertIs(before, observed[0])
                self.assertIs(after, before)

    def test_settrace_forward_caught_refuses_missing_loop_owner(self):
        source = """def f():
    value = 0
    try:
        1 / 0
    except ZeroDivisionError as error:
        value += 1  # source
        for item in (2,):
            value += item  # target
    return value
"""
        namespace = {}
        exec(source, namespace)
        f = namespace["f"]
        source_line = next(i for i, line in enumerate(source.splitlines(), 1)
                           if "# source" in line)
        target_line = next(i for i, line in enumerate(source.splitlines(), 1)
                           if "# target" in line)
        instructions = list(dis.get_instructions(f))
        loops = [inst for inst in instructions
                 if inst.opname.startswith("FOR_ITER")]
        self.assertEqual(len(loops), 1)
        loop = loops[0]
        words = _testinternalcapi.get_ir(f.__code__)
        nlocalsplus = len(_testinternalcapi.get_co_localskinds(f.__code__))
        self.assertGreaterEqual(words[loop.offset][2], nlocalsplus)
        target_instructions = [inst for inst in instructions
                               if inst.positions.lineno == target_line]
        self.assertTrue(target_instructions)
        for inst in target_instructions:
            self.assertGreater(inst.offset, loop.offset)
            self.assertLess(inst.offset, loop.argval)
        backedges = [inst for inst in target_instructions
                     if inst.opname.startswith("JUMP_BACKWARD")]
        self.assertEqual(len(backedges), 1)
        self.assertEqual(backedges[0].argval, loop.offset)
        refused = []
        def tracer(frame, event, arg):
            if (frame.f_code is f.__code__ and event == "line" and
                    frame.f_lineno == source_line and not refused):
                before = frame.f_lasti
                inherited = sys.exception()
                refused.append(before)
                with self.assertRaisesRegex(ValueError, "incoming temporary"):
                    frame.f_lineno = target_line
                self.assertEqual(frame.f_lasti, before)
                self.assertEqual(frame.f_locals["value"], 0)
                self.assertIs(frame.f_locals["error"], inherited)
                self.assertIs(sys.exception(), inherited)
            return tracer

        previous = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(f(), 3)
        finally:
            sys.settrace(previous)
        self.assertEqual(len(refused), 1)

    def test_settrace_cannot_manufacture_or_exit_caught_context(self):
        source = """def f(reraise=False):
    value = 0
    value += 1  # root
    try:
        1 / 0
    except ZeroDivisionError:
        value += 2  # caught
        if reraise:
            raise  # bare
    return value  # exit
"""
        for exiting in (False, True):
            with self.subTest(exiting=exiting):
                namespace = {}
                exec(source, namespace)
                f = namespace["f"]
                target = "# exit" if exiting else "# bare"
                stopped = "# caught" if exiting else "# root"
                target_line = next(i for i, line in enumerate(source.splitlines(), 1)
                                   if target in line)
                source_line = next(i for i, line in enumerate(source.splitlines(), 1)
                                   if stopped in line)
                refused = False
                def tracer(frame, event, arg):
                    nonlocal refused
                    if (frame.f_code is f.__code__ and event == "line" and
                            frame.f_lineno == source_line and not refused):
                        refused = True
                        with self.assertRaisesRegex(ValueError, "incoming temporary"):
                            frame.f_lineno = target_line
                    return tracer

                previous = sys.gettrace()
                sys.settrace(tracer)
                try:
                    self.assertEqual(f(), 3)
                finally:
                    sys.settrace(previous)
                self.assertTrue(refused)

    def test_settrace_same_real_line_preserves_protected_state(self):
        sources = {
            "try": """def f(manager):
    value = 1
    try:
        value += 2  # identity
    except ZeroDivisionError:
        value = -1
    return value, None, None
""",
            "except": """def f(manager):
    try:
        1 / 0
    except ZeroDivisionError as error:
        before = sys.exception()
        value = 1
        value += 2  # identity
        after = sys.exception()
    return value, before, after
""",
            "with": """def f(manager):
    with manager as value:
        value += 2  # identity
    return value, None, None
""",
        }
        class Manager:
            def __init__(self):
                self.events = []
            def __enter__(self):
                self.events.append("enter")
                return 1
            def __exit__(self, *args):
                self.events.append("exit")

        for context, source in sources.items():
            with self.subTest(context=context):
                namespace = {"sys": sys}
                exec(source, namespace)
                f = namespace["f"]
                line = next(i for i, text in enumerate(source.splitlines(), 1)
                            if "# identity" in text)
                positions = list(f.__code__.co_positions())
                applied = False
                def tracer(frame, event, arg):
                    nonlocal applied
                    if (frame.f_code is f.__code__ and event == "line" and
                            frame.f_lineno == line and not applied):
                        applied = True
                        self.assertEqual(positions[frame.f_lasti][0], line)
                        before = dict(frame.f_locals)
                        inherited = sys.exception()
                        try:
                            raise LookupError("callback-owned")
                        except LookupError as callback:
                            frame.f_lineno = line
                            self.assertIs(sys.exception(), callback)
                        self.assertIs(sys.exception(), inherited)
                        self.assertEqual(dict(frame.f_locals), before)
                    return tracer

                manager = Manager()
                old_trace = sys.gettrace()
                with warnings.catch_warnings():
                    warnings.simplefilter("error", RuntimeWarning)
                    sys.settrace(tracer)
                    try:
                        result, before, after = f(manager)
                    finally:
                        sys.settrace(old_trace)
                self.assertTrue(applied)
                self.assertEqual(result, 3)
                if context == "except":
                    self.assertIsInstance(before, ZeroDivisionError)
                    self.assertIs(after, before)
                else:
                    self.assertIsNone(before)
                    self.assertIsNone(after)
                self.assertEqual(manager.events,
                                 ["enter", "exit"] if context == "with" else [])

    def test_settrace_running_line_does_not_reenter_with(self):
        for warm in (0, 20):
            with self.subTest(warm=warm):
                events = []
                class Manager:
                    def __call__(self, *args):
                        events.append("call")
                    def __enter__(self):
                        events.append("enter")
                    def __exit__(self, *args):
                        events.append("exit")

                def body():
                    events.append("body")

                def f(a, b):
                    a(b if b else a, b)
                    with b:
                        body()

                manager = Manager()
                for _ in range(warm):
                    f(manager, manager)
                events.clear()
                stops = []
                applied = []
                def tracer(frame, event, arg):
                    if frame.f_code is not f.__code__:
                        return None
                    if event == "line":
                        self.assertLess(len(stops), 40, "running-line replay loop")
                        before, line = frame.f_lasti, frame.f_lineno
                        stops.append((before, line))
                        try:
                            frame.f_lineno = line
                        except ValueError:
                            # Source-only phases still require their own recipe.
                            pass
                        else:
                            self.assertEqual(frame.f_lasti, before)
                            applied.append((before, line))
                    return tracer

                old_trace = sys.gettrace()
                sys.settrace(tracer)
                try:
                    self.assertIsNone(f(manager, manager))
                finally:
                    sys.settrace(old_trace)
                self.assertEqual(events, ["call", "enter", "body", "exit"])
                header_pcs = {pc for pc, line in applied
                              if line == f.__code__.co_firstlineno + 2}
                self.assertGreaterEqual(len(header_pcs), 2)

    def test_marshal_copy(self):
        import marshal
        code = self.loop.__code__
        for new in (marshal.loads(marshal.dumps(code)), copy.copy(code),
                    copy.replace(code)):
            self.assertEqual(new, code)
            self.assertEqual(new.co_constslots, code.co_constslots)
            self.assertEqual(types_FunctionType(new, globals())(10), 136.0)

    def test_pickle_refused(self):
        import pickle
        with self.assertRaises(TypeError):
            pickle.dumps(self.loop.__code__)

    def test_generator_and_coroutine(self):
        def gen(n):
            total = 0
            for i in range(n):
                total += 2.5
                yield total + 1
        self.assertTrue(gen.__code__.co_constslots)
        self.assertEqual(list(gen(3)), [3.5, 6.0, 8.5])

        async def coro(n):
            total = 0
            for i in range(n):
                total += 4
                total = total - 1
            return total
        self.assertTrue(coro.__code__.co_constslots)
        c = coro(5)
        try:
            c.send(None)
        except StopIteration as e:
            self.assertEqual(e.value, 15)

    def test_closure_with_constants(self):
        def outer(k):
            def inner(n):
                total = 0
                for i in range(n):
                    total += k + 3
                return total
            return inner
        inner = outer(2)
        self.assertTrue(inner.__code__.co_constslots)
        self.assertEqual(inner.__code__.co_freevars, ('k',))
        self.assertEqual(inner(4), 20)

    def test_cell_and_free_around_constants(self):
        def f(n):
            cell = 1
            def g():
                return cell
            total = 0
            for i in range(n):
                cell += 1
                total += g() + 2
            return total
        self.assertTrue(f.__code__.co_constslots)
        self.assertEqual(f(3), 2 + 2 + 3 + 2 + 4 + 2)

    def test_recursion_and_overlap(self):
        def fib(n):
            if n < 2:
                return n
            total = 0
            for i in range(1):
                total += fib(n - 1) + fib(n - 2)
            return total
        self.assertTrue(fib.__code__.co_constslots)
        self.assertEqual(fib(15), 610)

    def test_call_without_arguments_from_generator_frame(self):
        # A call that the interpreter specializes and that a generator or
        # coroutine makes pushes its callee's frame apart from the caller's
        # window.  With no self and no arguments, the callee's first slot is
        # a constant slot, which the push must leave alone.
        counter = 0

        def callee():
            nonlocal counter
            counter = 0
            while counter < 10:
                counter += 1
            return counter

        code = callee.__code__
        self.assertEqual(code.co_nlocals, 0)
        self.assertEqual(self.slots(code)[0][0], 0)

        def gen():
            for _ in range(50):
                yield callee()
        self.assertEqual(sum(gen()), 500)

        async def inner():
            nonlocal counter
            counter = 0
            await suspend()
            while counter < 10:
                counter += 1
            return counter

        async def outer():
            total = 0
            for _ in range(50):
                total += await inner()
            return total

        @types.coroutine
        def suspend():
            yield

        coro = outer()
        try:
            while True:
                coro.send(None)
        except StopIteration as e:
            self.assertEqual(e.value, 500)

    def test_slot_limit(self):
        # More distinct constants than the frame can hold: the rest keep
        # their loads
        n = 5000
        body = "".join(f"        t += {i}.5\n" for i in range(n))
        ns = {}
        exec("def f(m):\n    t = 0\n    for i in range(m):\n" + body
             + "    return t\n", ns)
        f = ns['f']
        slots = self.slots(f.__code__)
        self.assertTrue(0 < len(slots) < n)
        code = f.__code__
        self.assertLessEqual(code.co_nlocals + len(slots) + code.co_stacksize,
                             4095)
        self.assertEqual(f(2), 2 * sum(i + 0.5 for i in range(n)))

    def test_exception_in_loop_with_constants(self):
        def f(n):
            total = 0
            for i in range(n):
                try:
                    total += 10 // (i - 3)
                except ZeroDivisionError:
                    total += 100
            return total
        self.assertEqual(f(6), 10 // -3 + 10 // -2 + 10 // -1 + 100
                         + 10 // 1 + 10 // 2)




class CodeConstsTest(unittest.TestCase):

    def find_const(self, consts, value):
        for v in consts:
            if v == value:
                return v
        self.assertIn(value, consts)  # raises an exception
        self.fail('Should never be reached')

    def assertIsInterned(self, s):
        if not isinterned(s):
            self.fail('String %r is not interned' % (s,))

    def assertIsNotInterned(self, s):
        if isinterned(s):
            self.fail('String %r is interned' % (s,))

    @cpython_only
    def test_interned_string(self):
        co = compile('res = "str_value"', '?', 'exec')
        v = self.find_const(co.co_consts, 'str_value')
        self.assertIsInterned(v)

    @cpython_only
    def test_interned_string_in_tuple(self):
        co = compile('res = ("str_value",)', '?', 'exec')
        v = self.find_const(co.co_consts, ('str_value',))
        self.assertIsInterned(v[0])

    @cpython_only
    def test_interned_string_in_frozenset(self):
        co = compile('res = a in {"str_value"}', '?', 'exec')
        v = self.find_const(co.co_consts, frozenset(('str_value',)))
        self.assertIsInterned(tuple(v)[0])

    @cpython_only
    def test_interned_string_default(self):
        def f(a='str_value'):
            return a
        self.assertIsInterned(f())

    @cpython_only
    @unittest.skipIf(Py_GIL_DISABLED, "free-threaded build interns all string constants")
    def test_interned_string_with_null(self):
        co = compile(r'res = "str\0value!"', '?', 'exec')
        v = self.find_const(co.co_consts, 'str\0value!')
        self.assertIsNotInterned(v)

    @cpython_only
    @unittest.skipUnless(Py_GIL_DISABLED, "does not intern all constants")
    def test_interned_constants(self):
        # compile separately to avoid compile time de-duping

        globals = {}
        exec(textwrap.dedent("""
            def func1():
                return (0.0, (1, 2, "hello"))
        """), globals)

        exec(textwrap.dedent("""
            def func2():
                return (0.0, (1, 2, "hello"))
        """), globals)

        self.assertTrue(globals["func1"]() is globals["func2"]())

    @cpython_only
    def test_unusual_constants(self):
        # gh-130851: Code objects constructed with constants that are not
        # types generated by the bytecode compiler should not crash the
        # interpreter.
        class Unhashable:
            def __hash__(self):
                raise TypeError("unhashable type")

        class MyInt(int):
            pass

        class MyComplex(complex):
            pass

        code = compile("a = 1", "<string>", "exec")
        code = code.replace(co_consts=(1, Unhashable(), MyInt(1), MyInt(1),
                                       MyComplex(1j)))
        self.assertIsInstance(code.co_consts[1], Unhashable)
        self.assertEqual(code.co_consts[2], code.co_consts[3])
        self.assertFalse(sys._is_immortal(code.co_consts[4]))


class CodeWeakRefTest(unittest.TestCase):

    def test_basic(self):
        # Create a code object in a clean environment so that we know we have
        # the only reference to it left.
        namespace = {}
        exec("def f(): pass", globals(), namespace)
        f = namespace["f"]
        del namespace

        self.called = False
        def callback(code):
            self.called = True

        # f is now the last reference to the function, and through it, the code
        # object.  While we hold it, check that we can create a weakref and
        # deref it.  Then delete it, and check that the callback gets called and
        # the reference dies.
        coderef = weakref.ref(f.__code__, callback)
        self.assertTrue(bool(coderef()))
        del f
        gc_collect()  # For PyPy or other GCs.
        self.assertFalse(bool(coderef()))
        self.assertTrue(self.called)

# Python implementation of location table parsing algorithm
def read(it):
    return next(it)

def read_varint(it):
    b = read(it)
    val = b & 63;
    shift = 0;
    while b & 64:
        b = read(it)
        shift += 6
        val |= (b&63) << shift
    return val

def read_signed_varint(it):
    uval = read_varint(it)
    if uval & 1:
        return -(uval >> 1)
    else:
        return uval >> 1

def parse_location_table(code):
    line = code.co_firstlineno
    locations = code.co_linetable
    if locations[:4] == b"IRSM":
        if len(locations) < 16:
            raise ValueError("truncated source map")
        version = int.from_bytes(locations[4:8], "little")
        size = int.from_bytes(locations[8:12], "little")
        records = int.from_bytes(locations[12:16], "little")
        if version != 2 or 16 + size + records * 16 != len(locations):
            raise ValueError("malformed source map")
        locations = locations[16:16 + size]
    it = iter(locations)
    while True:
        try:
            first_byte = read(it)
        except StopIteration:
            return
        code = (first_byte >> 3) & 15
        length = (first_byte & 7) + 1
        if code == 15:
            yield (code, length, None, None, None, None)
        elif code == 14:
            line_delta = read_signed_varint(it)
            line += line_delta
            end_line = line + read_varint(it)
            col = read_varint(it)
            if col == 0:
                col = None
            else:
                col -= 1
            end_col = read_varint(it)
            if end_col == 0:
                end_col = None
            else:
                end_col -= 1
            yield (code, length, line, end_line, col, end_col)
        elif code == 13: # No column
            line_delta = read_signed_varint(it)
            line += line_delta
            yield (code, length, line, line, None, None)
        elif code in (10, 11, 12): # new line
            line_delta = code - 10
            line += line_delta
            column = read(it)
            end_column = read(it)
            yield (code, length, line, line, column, end_column)
        else:
            assert (0 <= code < 10)
            second_byte = read(it)
            column = code << 3 | (second_byte >> 4)
            yield (code, length, line, line, column, column + (second_byte & 15))

def positions_from_location_table(code):
    for _, length, line, end_line, col, end_col in parse_location_table(code):
        for _ in range(length):
            yield (line, end_line, col, end_col)

def dedup(lst, prev=object()):
    for item in lst:
        if item != prev:
            yield item
            prev = item

def lines_from_postions(positions):
    return dedup(l for (l, _, _, _) in positions)

def misshappen():
    """





    """
    x = (


        4

        +

        y

    )
    y = (
        a
        +
            b
                +

                d
        )
    return q if (

        x

        ) else p

def bug93662():
    example_report_generation_message= (
            """
            """
    ).strip()
    raise ValueError()


class CodeLocationTest(unittest.TestCase):

    def check_positions(self, func):
        pos1 = list(func.__code__.co_positions())
        pos2 = list(positions_from_location_table(func.__code__))
        for l1, l2 in zip(pos1, pos2):
            self.assertEqual(l1, l2)
        self.assertEqual(len(pos1), len(pos2))

    def test_positions(self):
        self.check_positions(parse_location_table)
        self.check_positions(misshappen)
        self.check_positions(bug93662)

    def check_lines(self, func):
        co = func.__code__
        ranges = list(co.co_lines())
        lines1 = [line for _, _, line in ranges]
        self.assertEqual(lines1, list(dedup(lines1)))
        # Source-only lines have zero-width ranges; co_positions describes
        # real words. Compare their word coverage independently of those
        # extra source records.
        lines1 = list(dedup(line for start, end, line in ranges if start < end))
        lines2 = list(lines_from_postions(positions_from_location_table(co)))
        for l1, l2 in zip(lines1, lines2):
            self.assertEqual(l1, l2)
        self.assertEqual(len(lines1), len(lines2))

    def test_lines(self):
        self.check_lines(parse_location_table)
        self.check_lines(misshappen)
        self.check_lines(bug93662)

    def test_large_source_line_iterator_keeps_complete_word_coverage(self):
        namespace = {}
        source = 'def func(owner):\n' + ''.join(
            f'    owner.value = {index}\n    {index}\n' for index in range(256))
        exec(compile(source, '<source-line-iterator>', 'exec'), namespace)
        code = namespace['func'].__code__
        original = code.co_linetable
        ranges = list(code.co_lines())
        self.assertGreaterEqual(len(ranges), 256)
        self.assertTrue(any(start == end for start, end, _ in ranges))
        self.assertEqual(ranges[0][0], 0)
        self.assertEqual(ranges[-1][1], len(code.co_code) // 8)
        self.assertEqual([end for _, end, _ in ranges[:-1]],
                         [start for start, _, _ in ranges[1:]])
        self.assertEqual([line for start, end, line in ranges for _ in range(start, end)],
                         [position[0] for position in code.co_positions()])
        self.assertEqual(ranges, list(code.co_lines()))
        self.assertEqual(code.co_linetable, original)

    def test_source_line_rows_at_one_landing_are_sorted_and_deduplicated(self):
        def func():
            return None

        code = func.__code__
        target = list(dis.get_instructions(func))[-1].offset
        real_line = list(code.co_positions())[target][0]
        raw = code.co_linetable
        version, size, _ = struct.unpack_from('<III', raw, 4)
        source_lines = [real_line + 30, real_line + 10, real_line,
                        real_line + 20, real_line + 10]
        records = [(target - 1, target, 0, line, line, 0, 1, -1, 0, 0)
                   for line in source_lines]
        exceptions, _ = semantic_parts(code.co_exceptiontable)
        code = code.replace(**observation_tables(raw[16:16 + size], records, exceptions))
        original = code.co_linetable
        ranges = list(code.co_lines())
        self.assertEqual([(start, line) for start, end, line in ranges if start == end],
                         [(target, real_line + offset) for offset in (10, 20, 30)])
        self.assertEqual([line for start, end, line in ranges for _ in range(start, end)],
                         [position[0] for position in code.co_positions()])
        self.assertEqual(code.co_linetable, original)

    @cpython_only
    def test_code_new_empty(self):
        # If this test fails, it means that the construction of PyCode_NewEmpty
        # needs to be modified! Please update this test *and* PyCode_NewEmpty,
        # so that they both stay in sync.
        def f():
            pass
        PY_CODE_LOCATION_INFO_NO_COLUMNS = 13
        f.__code__ = f.__code__.replace(
            co_stacksize=1,
            co_firstlineno=42,
            co_code=encode_words(
                ("RESUME", 0),
                ("LOAD_COMMON_CONSTANT", 0, 0),
                ("RAISE_VARARGS", 0, 1),
            ),
            co_linetable=bytes(
                [
                    (1 << 7)
                    | (PY_CODE_LOCATION_INFO_NO_COLUMNS << 3)
                    | (3 - 1),
                    0,
                ]
            ),
        )
        self.assertRaises(AssertionError, f)
        self.assertEqual(
            list(f.__code__.co_positions()),
            3 * [(42, 42, None, None)],
        )

    @cpython_only
    def test_docstring_under_o2(self):
        code = textwrap.dedent('''
            def has_docstring(x, y):
                """This is a first-line doc string"""
                """This is a second-line doc string"""
                a = x + y
                b = x - y
                return a, b


            def no_docstring(x):
                def g(y):
                    return x + y
                return g


            async def async_func():
                """asynf function doc string"""
                pass


            for func in [has_docstring, no_docstring(4), async_func]:
                assert(func.__doc__ is None)
            ''')

        rc, out, err = assert_python_ok('-OO', '-c', code)

    def test_co_branches(self):
        _testcapi = import_helper.import_module("_testcapi")
        code_offset_to_line = _testcapi.code_offset_to_line

        def get_line_branches(func):
            code = func.__code__
            base = code.co_firstlineno
            return [
                (
                    code_offset_to_line(code, src) - base,
                    code_offset_to_line(code, left) - base,
                    code_offset_to_line(code, right) - base
                ) for (src, left, right) in
                code.co_branches()
            ]

        def simple(x):
            if x:
                A
            else:
                B

        self.assertEqual(
            get_line_branches(simple),
            [(1,2,4)])

        def with_extended_args(x):
            if x:
                A.x; A.x; A.x; A.x; A.x; A.x;
                A.x; A.x; A.x; A.x; A.x; A.x;
                A.x; A.x; A.x; A.x; A.x; A.x;
                A.x; A.x; A.x; A.x; A.x; A.x;
                A.x; A.x; A.x; A.x; A.x; A.x;
            else:
                B

        self.assertEqual(
            get_line_branches(with_extended_args),
            [(1,2,8)])

        async def afunc():
            async for letter in async_iter1:
                2
            3

        self.assertEqual(
            get_line_branches(afunc),
            [(1,1,3)])

if check_impl_detail(cpython=True) and ctypes is not None:
    import ctypes.util
    freefunc = ctypes.CFUNCTYPE(None, ctypes.c_voidp)

    @ctypes.util.wrap_dll_function(ctypes.pythonapi)
    def PyUnstable_Eval_RequestCodeExtraIndex(free: freefunc) -> ctypes.c_ssize_t:
        pass

    @ctypes.util.wrap_dll_function(ctypes.pythonapi)
    def PyUnstable_Code_SetExtra(code: ctypes.py_object,
                                 index: ctypes.c_ssize_t,
                                 extra: ctypes.c_voidp) -> ctypes.c_int:
        pass
    SetExtra = PyUnstable_Code_SetExtra

    @ctypes.util.wrap_dll_function(ctypes.pythonapi)
    def PyUnstable_Code_GetExtra(code: ctypes.py_object,
                                 index: ctypes.c_ssize_t,
                                 extra: ctypes.POINTER(ctypes.c_voidp)) -> ctypes.c_int:
        pass
    GetExtra = PyUnstable_Code_GetExtra

    LAST_FREED = None
    def myfree(ptr):
        global LAST_FREED
        LAST_FREED = ptr

    FREE_FUNC = freefunc(myfree)
    FREE_INDEX = PyUnstable_Eval_RequestCodeExtraIndex(FREE_FUNC)
    # Make sure myfree sticks around at least as long as the interpreter,
    # since we (currently) can't unregister the function and leaving a
    # dangling pointer will cause a crash on deallocation of code objects if
    # something else uses co_extras, like test_capi.test_misc.
    late_deletion(myfree)

    class CoExtra(unittest.TestCase):
        def get_func(self):
            # Defining a function causes the containing function to have a
            # reference to the code object.  We need the code objects to go
            # away, so we eval a lambda.
            return eval('lambda:42')

        def test_get_non_code(self):
            f = self.get_func()

            self.assertRaises(SystemError, SetExtra, 42, FREE_INDEX,
                              ctypes.c_voidp(100))
            self.assertRaises(SystemError, GetExtra, 42, FREE_INDEX,
                              ctypes.c_voidp(100))

        def test_bad_index(self):
            f = self.get_func()
            self.assertRaises(SystemError, SetExtra, f.__code__,
                              FREE_INDEX+100, ctypes.c_voidp(100))
            self.assertEqual(GetExtra(f.__code__, FREE_INDEX+100,
                              ctypes.c_voidp(100)), 0)

        def test_free_called(self):
            # Verify that the provided free function gets invoked
            # when the code object is cleaned up.
            f = self.get_func()

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(100))
            del f
            gc_collect()  # For free-threaded build
            self.assertEqual(LAST_FREED, 100)

        def test_get_set(self):
            # Test basic get/set round tripping.
            f = self.get_func()

            extra = ctypes.c_voidp()

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(200))
            # reset should free...
            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(300))
            self.assertEqual(LAST_FREED, 200)

            extra = ctypes.c_voidp()
            GetExtra(f.__code__, FREE_INDEX, extra)
            self.assertEqual(extra.value, 300)
            del f

        @threading_helper.requires_working_threading()
        def test_free_different_thread(self):
            # Freeing a code object on a different thread then
            # where the co_extra was set should be safe.
            f = self.get_func()
            class ThreadTest(threading.Thread):
                def __init__(self, f, test):
                    super().__init__()
                    self.f = f
                    self.test = test
                def run(self):
                    del self.f
                    gc_collect()
                    # gh-117683: In the free-threaded build, the code object's
                    # destructor may still be running concurrently in the main
                    # thread.
                    if not Py_GIL_DISABLED:
                        self.test.assertEqual(LAST_FREED, 500)

            SetExtra(f.__code__, FREE_INDEX, ctypes.c_voidp(500))
            tt = ThreadTest(f, self)
            del f
            tt.start()
            tt.join()
            gc_collect()  # For free-threaded build
            self.assertEqual(LAST_FREED, 500)


def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
