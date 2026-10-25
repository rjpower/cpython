# Minimal tests for dis module

import ast
import contextlib
import dis
import functools
import io
import itertools
import opcode
import re
import sys
import tempfile
import textwrap
import types
import unittest
from test import support
from test.support import (captured_stdout, requires_debug_ranges,
                          requires_specialization, cpython_only,
                          os_helper, import_helper, reset_code)
from test.support.bytecode_helper import BytecodeTestCase, encode_words


CACHE = dis.opmap["CACHE"]

def get_tb():
    def _error():
        try:
            1 / 0
        except Exception as e:
            tb = e.__traceback__
        return tb

    tb = _error()
    while tb.tb_next:
        tb = tb.tb_next
    return tb

TRACEBACK_CODE = get_tb().tb_frame.f_code

class _C:
    def __init__(self, x):
        self.x = x == 1

    @staticmethod
    def sm(x):
        x = x == 1

    @classmethod
    def cm(cls, x):
        cls.x = x == 1

dis_c_instance_method = """\
%3d           RESUME                   0

%3d           LOAD_FAST_BORROW         1 (x)  r2 <-
              LOAD_SMALL_INT           1  r3 <-
              COMPARE_OP              72 (==)  r2 <- r2, r3
              STORE_ATTR               0 (x)  <- r0, r2
              LOAD_COMMON_CONSTANT     7 (None)  r2 <-
              RETURN_VALUE          <- r2
""" % (_C.__init__.__code__.co_firstlineno, _C.__init__.__code__.co_firstlineno + 1,)

dis_c_instance_method_bytes = """\
          RESUME                   0
          LOAD_FAST_BORROW         1  r2 <-
          LOAD_SMALL_INT           1  r3 <-
          COMPARE_OP            r2 <- r2, r3
          STORE_ATTR            <- r0, r2
          LOAD_COMMON_CONSTANT     7 (None)  r2 <-
          RETURN_VALUE          <- r2
"""

dis_c_class_method = """\
%3d           RESUME                   0

%3d           LOAD_FAST_BORROW         1 (x)  r2 <-
              LOAD_SMALL_INT           1  r3 <-
              COMPARE_OP              72 (==)  r2 <- r2, r3
              STORE_ATTR               0 (x)  <- r0, r2
              LOAD_COMMON_CONSTANT     7 (None)  r2 <-
              RETURN_VALUE          <- r2
""" % (_C.cm.__code__.co_firstlineno, _C.cm.__code__.co_firstlineno + 2,)

dis_c_static_method = """\
%3d           RESUME                   0

%3d           LOAD_FAST_BORROW         0 (x)  r1 <-
              LOAD_SMALL_INT           1  r2 <-
              COMPARE_OP              72 (==)  r0 <- r1, r2
              LOAD_COMMON_CONSTANT     7 (None)  r1 <-
              RETURN_VALUE          <- r1
""" % (_C.sm.__code__.co_firstlineno, _C.sm.__code__.co_firstlineno + 2,)

# Class disassembling info has an extra newline at end.
dis_c = """\
Disassembly of %s:
%s
Disassembly of %s:
%s
Disassembly of %s:
%s
""" % (_C.__init__.__name__, dis_c_instance_method,
       _C.cm.__name__, dis_c_class_method,
       _C.sm.__name__, dis_c_static_method)

def _f(a):
    print(a)
    return 1

dis_f = """\
%3d           RESUME                   0

%3d           LOAD_GLOBAL              1 (print + NULL)  r1 <-
              LOAD_FAST_BORROW         0 (a)  r3 <-
              CALL                     1  r1 <- r1
              POP_TOP               <- r1

%3d           LOAD_SMALL_INT           1  r1 <-
              RETURN_VALUE          <- r1
""" % (_f.__code__.co_firstlineno,
       _f.__code__.co_firstlineno + 1,
       _f.__code__.co_firstlineno + 2)

dis_f_with_offsets = """\
%3d          0       RESUME                   0

%3d          1       LOAD_GLOBAL              1 (print + NULL)  r1 <-
             2       LOAD_FAST_BORROW         0 (a)  r3 <-
             3       CALL                     1  r1 <- r1
             4       POP_TOP               <- r1

%3d          5       LOAD_SMALL_INT           1  r1 <-
             6       RETURN_VALUE          <- r1
""" % (_f.__code__.co_firstlineno,
       _f.__code__.co_firstlineno + 1,
       _f.__code__.co_firstlineno + 2)

dis_f_with_positions_format = f"""\
%-14s           RESUME                   0

%-14s           LOAD_GLOBAL              1 (print + NULL)  r1 <-
%-14s           LOAD_FAST_BORROW         0 (a)  r3 <-
%-14s           CALL                     1  r1 <- r1
%-14s           POP_TOP               <- r1

%-14s           LOAD_SMALL_INT           1  r1 <-
%-14s           RETURN_VALUE          <- r1
"""

dis_f_co_code = """\
          RESUME                   0
          LOAD_GLOBAL           r1 <-
          LOAD_FAST_BORROW         0  r3 <-
          CALL                     1  r1 <- r1
          POP_TOP               <- r1
          LOAD_SMALL_INT           1  r1 <-
          RETURN_VALUE          <- r1
"""

def bug708901():
    for res in range(1,
                     10):
        pass

dis_bug708901 = """\
%3d           RESUME                   0

%3d           LOAD_GLOBAL              1 (range + NULL)  r1 <-
              LOAD_SMALL_INT           1  r3 <-

%3d           LOAD_SMALL_INT          10  r4 <-

%3d           CALL                     2  r1 <- r1
              GET_ITER                 0  r1 <- r1
      L1:     FOR_ITER                 1 (to L2)  r0 <- r1

%3d           JUMP_BACKWARD            6 (to L1)

%3d   L2:     END_FOR               <- r1, r2, r3
              POP_ITER              <- r1, r2
              LOAD_COMMON_CONSTANT     7 (None)  r1 <-
              RETURN_VALUE          <- r1
""" % (bug708901.__code__.co_firstlineno,
       bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 2,
       bug708901.__code__.co_firstlineno + 1,
       bug708901.__code__.co_firstlineno + 3,
       bug708901.__code__.co_firstlineno + 1)


def bug1333982(x=[]):
    assert 0, ((s for s in x) +
              1)
    pass

dis_bug1333982 = """\
%3d           RESUME                   0

%3d           LOAD_COMMON_CONSTANT     0 (AssertionError)  r1 <-
              LOAD_CONST               1 (<code object <genexpr> at 0x..., file "%s", line %d>)  r2 <-
              MAKE_FUNCTION         r2 <- r2
              LOAD_FAST_BORROW         0 (x)  r3 <-
              CALL                     0  r2 <- r2

%3d           LOAD_SMALL_INT           1  r3 <-

%3d           BINARY_OP                0 (+)  r2 <- r2, r3
              CALL                     0  r1 <- r1
              RAISE_VARARGS            1  <- r1
""" % (bug1333982.__code__.co_firstlineno,
       bug1333982.__code__.co_firstlineno + 1,
       __file__,
       bug1333982.__code__.co_firstlineno + 1,
       bug1333982.__code__.co_firstlineno + 2,
       bug1333982.__code__.co_firstlineno + 1)


def bug42562():
    pass


# Set line number for 'pass' to None
bug42562.__code__ = bug42562.__code__.replace(co_linetable=b'\xf8')


dis_bug42562 = """\
          RESUME                   0
          LOAD_COMMON_CONSTANT     7 (None)  r0 <-
          RETURN_VALUE          <- r0
"""

# Constant indices past a byte, and past a 12-bit field, which take CD
code_bug_45757 = encode_words(
    ("NOP",),
    ("LOAD_CONST", 0, 0, 297 & 0xFFF, 297 >> 12),
    ("LOAD_CONST", 1, 0, 5001 & 0xFFF, 5001 >> 12),
    ("RETURN_VALUE", 0, 0),
)

dis_bug_45757 = """\
          NOP
          LOAD_CONST             297  r0 <-
          LOAD_CONST            5001  r1 <-
          RETURN_VALUE          <- r0
"""

# A jump to the word before it
bug46724 = encode_words(
    ("NOP",),
    ("JUMP_BACKWARD", 0, 0, 0, 0),
)


dis_bug46724 = """\
  L1:     NOP
          JUMP_BACKWARD            0 (to L1)
"""

def func_w_kwargs(a, b, **c):
    pass

def wrap_func_w_kwargs():
    func_w_kwargs(1, 2, c=5)

dis_kw_names = """\
%3d           RESUME                   0

%3d           LOAD_GLOBAL              1 (func_w_kwargs + NULL)  r0 <-
              LOAD_SMALL_INT           1  r2 <-
              LOAD_SMALL_INT           2  r3 <-
              LOAD_SMALL_INT           5  r4 <-
              LOAD_CONST               1 (('c',))  r5 <-
              CALL_KW                  3  r0 <- r0
              POP_TOP               <- r0
              LOAD_COMMON_CONSTANT     7 (None)  r0 <-
              RETURN_VALUE          <- r0
""" % (wrap_func_w_kwargs.__code__.co_firstlineno,
       wrap_func_w_kwargs.__code__.co_firstlineno + 1)

dis_intrinsic_1_2 = """\
  0           RESUME                   0

  1           LOAD_SMALL_INT           0  r0 <-
              LOAD_CONST               1 (('*',))  r1 <-
              IMPORT_NAME              2 (math + eager)  r0 <- r0
              CALL_INTRINSIC_1         2 (INTRINSIC_IMPORT_STAR)  r0 <- r0
              POP_TOP               <- r0
              LOAD_COMMON_CONSTANT     7 (None)  r0 <-
              RETURN_VALUE          <- r0
"""

dis_intrinsic_1_5 = """\
  0           RESUME                   0

  1           LOAD_NAME                0 (a)  r0 <-
              CALL_INTRINSIC_1         5 (INTRINSIC_UNARY_POSITIVE)  r0 <- r0
              RETURN_VALUE          <- r0
"""

dis_intrinsic_1_6 = """\
  0           RESUME                   0

  1           BUILD_LIST               0  r0 <- r0
              LOAD_NAME                0 (a)  r1 <-
              LIST_EXTEND              1  <- r0
              CALL_INTRINSIC_1         6 (INTRINSIC_LIST_TO_TUPLE)  r0 <- r0
              RETURN_VALUE          <- r0
"""

_BIG_LINENO_FORMAT = """\
  1           RESUME                   0

%3d           LOAD_GLOBAL              0 (spam)  r0 <-
              POP_TOP               <- r0
              LOAD_COMMON_CONSTANT     7 (None)  r0 <-
              RETURN_VALUE          <- r0
"""

_BIG_LINENO_FORMAT2 = """\
   1           RESUME                   0

%4d           LOAD_GLOBAL              0 (spam)  r0 <-
               POP_TOP               <- r0
               LOAD_COMMON_CONSTANT     7 (None)  r0 <-
               RETURN_VALUE          <- r0
"""

dis_module_expected_results = """\
Disassembly of f:
  4           RESUME                   0
              LOAD_COMMON_CONSTANT     7 (None)  r0 <-
              RETURN_VALUE          <- r0

Disassembly of g:
  5           RESUME                   0
              LOAD_COMMON_CONSTANT     7 (None)  r0 <-
              RETURN_VALUE          <- r0

"""

expr_str = "x + 1"

dis_expr_str = """\
  0           RESUME                   0

  1           LOAD_NAME                0 (x)  r0 <-
              LOAD_SMALL_INT           1  r1 <-
              BINARY_OP                0 (+)  r0 <- r0, r1
              RETURN_VALUE          <- r0
"""

simple_stmt_str = "x = x + 1"

dis_simple_stmt_str = """\
  0           RESUME                   0

  1           LOAD_NAME                0 (x)  r0 <-
              LOAD_SMALL_INT           1  r1 <-
              BINARY_OP                0 (+)  r0 <- r0, r1
              STORE_NAME               0 (x)  <- r0
              LOAD_COMMON_CONSTANT     7 (None)  r0 <-
              RETURN_VALUE          <- r0
"""

annot_stmt_str = """\

x: int = 1
y: fun(1)
lst[fun(0)]: int = 1
"""
# leading newline is for a reason (tests lineno)

dis_annot_stmt_str = """\
  --           MAKE_CELL                0 (__conditional_annotations__)

   0           RESUME                   0

   2           LOAD_CONST               1 (<code object __annotate__ at 0x..., file "<dis>", line 2>)  r1 <-
               MAKE_FUNCTION         r1 <- r1
               STORE_NAME               4 (__annotate__)  <- r1
               BUILD_SET                0  r1 <- r1
               STORE_NAME               0 (__conditional_annotations__)  <- r1
               LOAD_SMALL_INT           1  r1 <-
               STORE_NAME               1 (x)  <- r1
               LOAD_NAME                0 (__conditional_annotations__)  r1 <-
               LOAD_SMALL_INT           0  r2 <-
               CALL_INTRINSIC_2         6 (INTRINSIC_ADD_CONDITIONAL_ANNOTATION)  r1 <- r1, r2
               POP_TOP               <- r1

   3           LOAD_NAME                0 (__conditional_annotations__)  r1 <-
               LOAD_SMALL_INT           1  r2 <-
               CALL_INTRINSIC_2         6 (INTRINSIC_ADD_CONDITIONAL_ANNOTATION)  r1 <- r1, r2
               POP_TOP               <- r1

   4           LOAD_SMALL_INT           1  r1 <-
               LOAD_NAME                2 (lst)  r2 <-
               LOAD_NAME                3 (fun)  r3 <-
               PUSH_NULL             r4 <-
               LOAD_SMALL_INT           0  r5 <-
               CALL                     1  r3 <- r3
               STORE_SUBSCR          <- r2, r3, r1
               LOAD_COMMON_CONSTANT     7 (None)  r1 <-
               RETURN_VALUE          <- r1
"""

fn_with_annotate_str = """
def foo(a: int, b: str) -> str:
    return a * b
"""

dis_fn_with_annotate_str = """\
  0           RESUME                   0

  2           LOAD_CONST               0 (<code object __annotate__ at 0x..., file "<dis>", line 2>)  r0 <-
              MAKE_FUNCTION         r0 <- r0
              LOAD_CONST               1 (<code object foo at 0x..., file "<dis>", line 2>)  r1 <-
              MAKE_FUNCTION         r1 <- r1
              SET_FUNCTION_ATTRIBUTE  16 (annotate)  r0 <- r0, r1
              STORE_NAME               0 (foo)  <- r0
              LOAD_COMMON_CONSTANT     7 (None)  r0 <-
              RETURN_VALUE          <- r0
"""

compound_stmt_str = """\
x = 0
while 1:
    x += 1"""
# Trailing newline has been deliberately omitted

dis_compound_stmt_str = """\
  0           RESUME                   0

  1           LOAD_SMALL_INT           0  r0 <-
              STORE_NAME               0 (x)  <- r0

  2   L1:     NOP

  3           LOAD_NAME                0 (x)  r0 <-
              LOAD_SMALL_INT           1  r1 <-
              BINARY_OP               13 (+=)  r0 <- r0, r1
              STORE_NAME               0 (x)  <- r0
              JUMP_BACKWARD            3 (to L1)
"""

dis_traceback = """\
%4d           RESUME                   0

%4d           NOP

%4d   L1:     LOAD_SMALL_INT           1  r2 <-
               LOAD_SMALL_INT           0  r3 <-
           --> BINARY_OP               11 (/)  r2 <- r2, r3
               POP_TOP               <- r2

%4d   L2:     LOAD_FAST_CHECK          1 (tb)  r2 <-
               RETURN_VALUE          <- r2

  --   L3:     PUSH_EXC_INFO         r2 <- r2

%4d           LOAD_GLOBAL              0 (Exception)  r4 <-
               CHECK_EXC_MATCH       r4 <- r3, r4
               POP_JUMP_IF_FALSE       24 (to L7)  <- r4
               NOT_TAKEN
               STORE_FAST               0 (e)  <- r3

%4d   L4:     LOAD_FAST                0 (e)  r3 <-
               LOAD_ATTR                2 (__traceback__)  r1 <- r3
       L5:     POP_EXCEPT            <- r2
               LOAD_COMMON_CONSTANT     7 (None)  r0 <-
               DELETE_FAST              0 (e)

%4d           LOAD_FAST                1 (tb)  r2 <-
               RETURN_VALUE          <- r2

  --   L6:     LOAD_COMMON_CONSTANT     7 (None)  r0 <-
               DELETE_FAST              0 (e)
               RERAISE                  1  <- r3

%4d   L7:     RERAISE                  0  <- r3

  --   L8:     COPY                     3  <- r2
               POP_EXCEPT            <- r5
               RERAISE                  1  <- r3
ExceptionTable:
  L1 to L2 -> L3 [0]
  L3 to L4 -> L8 [1] lasti
  L4 to L5 -> L6 [1] lasti
  L6 to L8 -> L8 [1] lasti
""" % (TRACEBACK_CODE.co_firstlineno,
       TRACEBACK_CODE.co_firstlineno + 1,
       TRACEBACK_CODE.co_firstlineno + 2,
       TRACEBACK_CODE.co_firstlineno + 5,
       TRACEBACK_CODE.co_firstlineno + 3,
       TRACEBACK_CODE.co_firstlineno + 4,
       TRACEBACK_CODE.co_firstlineno + 5,
       TRACEBACK_CODE.co_firstlineno + 3)

def _fstring(a, b, c, d):
    return f'{a} {b:4} {c!r} {d!r:4}'

dis_fstring = """\
%3d           RESUME                   0

%3d           FORMAT_SIMPLE         r4 <- r0
              LOAD_CONST               0 (' ')  r5 <-
              LOAD_FAST_BORROW         1 (b)  r6 <-
              LOAD_CONST               1 ('4')  r7 <-
              FORMAT_WITH_SPEC      r6 <- r6, r7
              LOAD_CONST               0 (' ')  r7 <-
              CONVERT_VALUE            2 (repr)  r8 <- r2
              FORMAT_SIMPLE         r8 <- r8
              LOAD_CONST               0 (' ')  r9 <-
              CONVERT_VALUE            2 (repr)  r10 <- r3
              LOAD_CONST               1 ('4')  r11 <-
              FORMAT_WITH_SPEC      r10 <- r10, r11
              BUILD_STRING             7  r4 <- r4
              RETURN_VALUE          <- r4
""" % (_fstring.__code__.co_firstlineno, _fstring.__code__.co_firstlineno + 1)

def _with(c):
    with c:
        x = 1
    y = 2

dis_with = """\
%4d           RESUME                   0

%4d           LOAD_FAST_BORROW         0 (c)  r3 <-
               LOAD_SPECIAL             1 (__exit__)  <- r0
               SWAP                     2  <- r4
               SWAP                     3  <- r3
               LOAD_SPECIAL             0 (__enter__)  <- r5
               CALL                     0  r5 <- r5
       L1:     POP_TOP               <- r5

%4d           LOAD_SMALL_INT           1  r1 <-

%4d   L2:     LOAD_COMMON_CONSTANT     7 (None)  r5 <-
               LOAD_COMMON_CONSTANT     7 (None)  r6 <-
               LOAD_COMMON_CONSTANT     7 (None)  r7 <-
               CALL                     3  r3 <- r3
               POP_TOP               <- r3

%4d           LOAD_SMALL_INT           2  r2 <-
               LOAD_COMMON_CONSTANT     7 (None)  r3 <-
               RETURN_VALUE          <- r3

%4d   L3:     PUSH_EXC_INFO         r6 <- r6
               WITH_EXCEPT_START     r8 <- r3
               POP_JUMP_IF_TRUE        22 (to L4)  <- r8
               NOT_TAKEN
               RERAISE                  2  <- r5
       L4:     POP_TOP               <- r7
       L5:     POP_EXCEPT            <- r6
               POP_TOP               <- r5
               POP_TOP               <- r4
               POP_TOP               <- r3

%4d           LOAD_SMALL_INT           2  r2 <-
               LOAD_COMMON_CONSTANT     7 (None)  r3 <-
               RETURN_VALUE          <- r3

  --   L6:     COPY                     3  <- r6
               POP_EXCEPT            <- r9
               RERAISE                  1  <- r7
ExceptionTable:
  L1 to L2 -> L3 [2] lasti
  L3 to L5 -> L6 [4] lasti
""" % (_with.__code__.co_firstlineno,
       _with.__code__.co_firstlineno + 1,
       _with.__code__.co_firstlineno + 2,
       _with.__code__.co_firstlineno + 1,
       _with.__code__.co_firstlineno + 3,
       _with.__code__.co_firstlineno + 1,
       _with.__code__.co_firstlineno + 3,
       )

async def _asyncwith(c):
    async with c:
        x = 1
    y = 2

dis_asyncwith = """\
%4d            RETURN_GENERATOR      r3 <- r3
                POP_TOP               <- r3
        L1:     RESUME                   0

%4d            LOAD_FAST_BORROW         0 (c)  r3 <-
                LOAD_FAST                0 (c)  r4 <-
                LOAD_SPECIAL             3 (__aexit__)  <- r4
                SWAP                     2  <- r4
                SWAP                     3  <- r3
                LOAD_SPECIAL             2 (__aenter__)  <- r5
                CALL                     0  r5 <- r5
                GET_AWAITABLE            1  r5 <- r5
                PUSH_NULL             r6 <-
                LOAD_COMMON_CONSTANT     7 (None)  r7 <-
        L2:     SEND                     3 (to L5)  r7 <- r5
        L3:     YIELD_VALUE              1  r7 <- r7
        L4:     RESUME                   3
                JUMP_BACKWARD_NO_INTERRUPT 13 (to L2)
        L5:     END_SEND              r5 <- r5, r6, r7
        L6:     POP_TOP               <- r5

%4d            LOAD_SMALL_INT           1  r1 <-

%4d    L7:     LOAD_COMMON_CONSTANT     7 (None)  r5 <-
                LOAD_COMMON_CONSTANT     7 (None)  r6 <-
                LOAD_COMMON_CONSTANT     7 (None)  r7 <-
                CALL                     3  r3 <- r3
                GET_AWAITABLE            2  r3 <- r3
                PUSH_NULL             r4 <-
                LOAD_COMMON_CONSTANT     7 (None)  r5 <-
        L8:     SEND                     3 (to L11)  r5 <- r3
        L9:     YIELD_VALUE              1  r5 <- r5
       L10:     RESUME                   3
                JUMP_BACKWARD_NO_INTERRUPT 27 (to L8)
       L11:     END_SEND              r3 <- r3, r4, r5
                POP_TOP               <- r3

%4d            LOAD_SMALL_INT           2  r2 <-
                LOAD_COMMON_CONSTANT     7 (None)  r3 <-
                RETURN_VALUE          <- r3

%4d   L12:     CLEANUP_THROW         r5 <- r5
       L13:     JUMP_BACKWARD_NO_INTERRUPT 17 (to L5)
       L14:     CLEANUP_THROW         r3 <- r3
       L15:     JUMP_BACKWARD_NO_INTERRUPT 31 (to L11)
       L16:     PUSH_EXC_INFO         r6 <- r6
                WITH_EXCEPT_START     r8 <- r3
                GET_AWAITABLE            2  r8 <- r8
                PUSH_NULL             r9 <-
                LOAD_COMMON_CONSTANT     7 (None)  r10 <-
       L17:     SEND                     4 (to L21)  r10 <- r8
       L18:     YIELD_VALUE              1  r10 <- r10
       L19:     RESUME                   3
                JUMP_BACKWARD_NO_INTERRUPT 45 (to L17)
       L20:     CLEANUP_THROW         r8 <- r8
       L21:     END_SEND              r8 <- r8, r9, r10
                POP_JUMP_IF_TRUE        54 (to L22)  <- r8
                NOT_TAKEN
                RERAISE                  2  <- r5
       L22:     POP_TOP               <- r7
       L23:     POP_EXCEPT            <- r6
                POP_TOP               <- r5
                POP_TOP               <- r4
                POP_TOP               <- r3

%4d            LOAD_SMALL_INT           2  r2 <-
                LOAD_COMMON_CONSTANT     7 (None)  r3 <-
                RETURN_VALUE          <- r3

  --   L24:     COPY                     3  <- r6
                POP_EXCEPT            <- r9
                RERAISE                  1  <- r7
       L25:     CALL_INTRINSIC_1         3 (INTRINSIC_STOPITERATION_ERROR)  r4 <- r4
                RERAISE                  1  <- r3
ExceptionTable:
  L1 to L3 -> L25 [0] lasti
  L3 to L4 -> L12 [5]
  L4 to L6 -> L25 [0] lasti
  L6 to L7 -> L16 [2] lasti
  L7 to L9 -> L25 [0] lasti
  L9 to L10 -> L14 [3]
  L10 to L13 -> L25 [0] lasti
  L14 to L15 -> L25 [0] lasti
  L16 to L18 -> L24 [4] lasti
  L18 to L19 -> L20 [8]
  L19 to L23 -> L24 [4] lasti
  L23 to L25 -> L25 [0] lasti
""" % (_asyncwith.__code__.co_firstlineno,
       _asyncwith.__code__.co_firstlineno + 1,
       _asyncwith.__code__.co_firstlineno + 2,
       _asyncwith.__code__.co_firstlineno + 1,
       _asyncwith.__code__.co_firstlineno + 3,
       _asyncwith.__code__.co_firstlineno + 1,
       _asyncwith.__code__.co_firstlineno + 3,
       )


def _tryfinally(a, b):
    try:
        return a
    finally:
        b()

def _tryfinallyconst(b):
    try:
        return 1
    finally:
        b()

dis_tryfinally = """\
%4d           RESUME                   0

%4d           NOP

%4d   L1:     LOAD_FAST_BORROW         0 (a)  r2 <-

%4d   L2:     LOAD_FAST_BORROW         1 (b)  r3 <-
               PUSH_NULL             r4 <-
               CALL                     0  r3 <- r3
               POP_TOP               <- r3
               RETURN_VALUE          <- r2

  --   L3:     PUSH_EXC_INFO         r2 <- r2

%4d           LOAD_FAST                1 (b)  r4 <-
               PUSH_NULL             r5 <-
               CALL                     0  r4 <- r4
               POP_TOP               <- r4
               RERAISE                  0  <- r3

  --   L4:     COPY                     3  <- r2
               POP_EXCEPT            <- r5
               RERAISE                  1  <- r3
ExceptionTable:
  L1 to L2 -> L3 [0]
  L3 to L4 -> L4 [1] lasti
""" % (_tryfinally.__code__.co_firstlineno,
       _tryfinally.__code__.co_firstlineno + 1,
       _tryfinally.__code__.co_firstlineno + 2,
       _tryfinally.__code__.co_firstlineno + 4,
       _tryfinally.__code__.co_firstlineno + 4,
       )

dis_tryfinallyconst = """\
%4d           RESUME                   0

%4d           NOP

%4d           NOP

%4d           LOAD_FAST_BORROW         0 (b)  r1 <-
               PUSH_NULL             r2 <-
               CALL                     0  r1 <- r1
               POP_TOP               <- r1
               LOAD_SMALL_INT           1  r1 <-
               RETURN_VALUE          <- r1

  --   L1:     PUSH_EXC_INFO         r1 <- r1

%4d           LOAD_FAST                0 (b)  r1 <-
               PUSH_NULL             r1 <-
               CALL                     0  r1 <- r1
               POP_TOP               <- r1
               RERAISE                  0  <- r1

  --   L2:     COPY                     3  <- r1
               POP_EXCEPT            <- r4
               RERAISE                  1  <- r2
ExceptionTable:
  L1 to L2 -> L2 [1] lasti
""" % (_tryfinallyconst.__code__.co_firstlineno,
       _tryfinallyconst.__code__.co_firstlineno + 1,
       _tryfinallyconst.__code__.co_firstlineno + 2,
       _tryfinallyconst.__code__.co_firstlineno + 4,
       _tryfinallyconst.__code__.co_firstlineno + 4,
       )

def _g(x):
    yield x

async def _ag(x):
    yield x

async def _co(x):
    async for item in _ag(x):
        pass

def _h(y):
    def foo(x):
        '''funcdoc'''
        return list(x + z for z in y)
    return foo

dis_nested_0 = """\
  --           MAKE_CELL                0 (y)

%4d           RESUME                   0

%4d           LOAD_FAST_BORROW         0 (y)  r2 <-
               BUILD_TUPLE              1  r2 <- r2
               LOAD_CONST               0 (<code object foo at 0x..., file "%s", line %d>)  r3 <-
               MAKE_FUNCTION         r3 <- r3
               SET_FUNCTION_ATTRIBUTE   8 (closure)  r2 <- r2, r3
               STORE_FAST               1 (foo)  <- r2

%4d           RETURN_VALUE          <- r1
""" % (_h.__code__.co_firstlineno,
       _h.__code__.co_firstlineno + 1,
       __file__,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 4,
)

dis_nested_1 = """\
%s
Disassembly of <code object foo at 0x..., file "%s", line %d>:
  --           COPY_FREE_VARS           1
               MAKE_CELL                0 (x)

%4d           RESUME                   0

%4d           LOAD_GLOBAL              0 (list)  r2 <-
               COPY                     1  <- r2
               LOAD_COMMON_CONSTANT     5 (list)  r4 <-
               IS_OP                    0 (is)  r3 <- r3, r4
               POP_JUMP_IF_FALSE       25 (to L3)  <- r3
               NOT_TAKEN
               POP_TOP               <- r2
               BUILD_LIST               0  r2 <- r2
               LOAD_FAST_BORROW         0 (x)  r3 <-
               BUILD_TUPLE              1  r3 <- r3
               LOAD_CONST               %d (<code object <genexpr> at 0x..., file "%s", line %d>)  r4 <-
               MAKE_FUNCTION         r4 <- r4
               SET_FUNCTION_ATTRIBUTE   8 (closure)  r3 <- r3, r4
               LOAD_DEREF               1 (y)  r4 <-
               CALL                     0  r3 <- r3
               PUSH_NULL             r4 <-
       L1:     FOR_ITER                 2 (to L2)  r5 <- r3
               LIST_APPEND              3  <- r2
               JUMP_BACKWARD           19 (to L1)
       L2:     END_FOR               <- r3, r4, r5
               POP_ITER              <- r3, r4
               RETURN_VALUE          <- r2
       L3:     PUSH_NULL             r3 <-
               LOAD_FAST_BORROW         0 (x)  r4 <-
               BUILD_TUPLE              1  r4 <- r4
               LOAD_CONST               1 (<code object <genexpr> at 0x..., file "%s", line %d>)  r5 <-
               MAKE_FUNCTION         r5 <- r5
               SET_FUNCTION_ATTRIBUTE   8 (closure)  r4 <- r4, r5
               LOAD_DEREF               1 (y)  r5 <-
               CALL                     0  r4 <- r4
               CALL                     1  r2 <- r2
               RETURN_VALUE          <- r2
""" % (dis_nested_0,
       __file__,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 1,
       _h.__code__.co_firstlineno + 3,
       1 if __debug__ else 0,
       __file__,
       _h.__code__.co_firstlineno + 3,
       __file__,
       _h.__code__.co_firstlineno + 3,
)

dis_nested_2 = """\
%s
Disassembly of <code object <genexpr> at 0x..., file "%s", line %d>:
  --           COPY_FREE_VARS           1
               RESUME                   4

%4d           LOAD_FAST                0 (.0)  r3 <-
               GET_ITER                 0  r3 <- r3
               RETURN_GENERATOR      r5 <- r5
               POP_TOP               <- r5
       L1:     RESUME                   0
       L2:     FOR_ITER                 6 (to L3)  r1 <- r3
               LOAD_DEREF               2 (x)  r5 <-
               BINARY_OP                0 (+)  r5 <- r5, r1
               YIELD_VALUE              0  r5 <- r5
               RESUME                   9
               POP_TOP               <- r5
               JUMP_BACKWARD            7 (to L2)
       L3:     END_FOR               <- r3, r4, r5
               POP_ITER              <- r3, r4
               LOAD_COMMON_CONSTANT     7 (None)  r3 <-
               RETURN_VALUE          <- r3

  --   L4:     CALL_INTRINSIC_1         3 (INTRINSIC_STOPITERATION_ERROR)  r6 <- r6
               RERAISE                  1  <- r5
ExceptionTable:
  L1 to L4 -> L4 [2] lasti
""" % (dis_nested_1,
       __file__,
       _h.__code__.co_firstlineno + 3,
       _h.__code__.co_firstlineno + 3,
)

def load_test(x, y=0):
    a, b = x, y
    return a, b

dis_load_test_quickened_code = """\
%3d           RESUME_CHECK             0

%3d           LOAD_FAST                0 (x)  r4 <-
              LOAD_FAST                1 (y)  r3 <-
              STORE_FAST               2 (a)  <- r4

%3d           LOAD_FAST_BORROW         2 (a)  r4 <-
              LOAD_FAST_BORROW         3 (b)  r5 <-
              BUILD_TUPLE              2  r4 <- r4
              RETURN_VALUE          <- r4
""" % (load_test.__code__.co_firstlineno,
       load_test.__code__.co_firstlineno + 1,
       load_test.__code__.co_firstlineno + 2)

def quickened_ints(disassembly):
    """Builds with tagged values quicken LOAD_SMALL_INT to the instruction
    that loads the int inline."""
    _testinternalcapi = import_helper.import_module("_testinternalcapi")
    if getattr(_testinternalcapi, "TAGGED_VALUES", False):
        return disassembly.replace("LOAD_SMALL_INT       ",
                                   "LOAD_SMALL_INT_INLINE")
    return disassembly

def loop_test():
    for i in [1, 2, 3] * 3:
        load_test(i)

dis_loop_test_quickened_code = """\
%3d           RESUME_CHECK             0

%3d           BUILD_LIST               0  r1 <- r1
              LOAD_CONST               1 ((1, 2, 3))  r2 <-
              LIST_EXTEND              1  <- r1
              LOAD_SMALL_INT           3  r2 <-
              BINARY_OP                5 (*)  r1 <- r1, r2
              GET_ITER_VIRTUAL         0  r1 <- r1
      L1:     FOR_ITER_LIST            5 (to L2)  r0 <- r1

%3d           LOAD_GLOBAL_MODULE       1 (load_test + NULL)  r3 <-
              LOAD_FAST_BORROW         0 (i)  r5 <-
              CALL_PY_GENERAL          1  r3 <- r3
              POP_TOP               <- r3
              JUMP_BACKWARD            7 (to L1)

%3d   L2:     END_FOR               <- r1, r2, r3
              POP_ITER              <- r1, r2
              LOAD_COMMON_CONSTANT     7 (None)  r1 <-
              RETURN_VALUE          <- r1
""" % (loop_test.__code__.co_firstlineno,
       loop_test.__code__.co_firstlineno + 1,
       loop_test.__code__.co_firstlineno + 2,
       loop_test.__code__.co_firstlineno + 1,)

def extended_arg_quick():
    *_, _ = ...

dis_extended_arg_quick_code = """\
%3d           RESUME                   0

%3d           LOAD_CONST               0 (Ellipsis)  r1 <-
              UNPACK_EX              256  <- r1
              POP_TOP               <- r2
              STORE_FAST               0 (_)  <- r1
              LOAD_COMMON_CONSTANT     7 (None)  r1 <-
              RETURN_VALUE          <- r1
"""% (extended_arg_quick.__code__.co_firstlineno,
      extended_arg_quick.__code__.co_firstlineno + 1,)

class DisTestBase(unittest.TestCase):
    "Common utilities for DisTests and TestDisTraceback"

    def strip_addresses(self, text):
        return re.sub(r'\b0x[0-9A-Fa-f]+\b', '0x...', text)

    def assert_exception_table_increasing(self, lines):
        prev_start, prev_end = -1, -1
        count = 0
        for line in lines:
            m = re.match(r'  L(\d+) to L(\d+) -> L\d+ \[\d+\]', line)
            start, end = [int(g) for g in m.groups()]
            self.assertGreaterEqual(end, start)
            self.assertGreaterEqual(start, prev_end)
            prev_start, prev_end = start, end
            count += 1
        return count

    def do_disassembly_compare(self, got, expected):
        if got != expected:
            got = self.strip_addresses(got)
        self.assertEqual(got, expected)


class DisTests(DisTestBase):

    maxDiff = None

    def get_disassembly(self, func, lasti=-1, wrapper=True, **kwargs):
        # We want to test the default printing behaviour, not the file arg
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            if wrapper:
                dis.dis(func, **kwargs)
            else:
                dis.disassemble(func, lasti, **kwargs)
        return output.getvalue()

    def get_disassemble_as_string(self, func, lasti=-1):
        return self.get_disassembly(func, lasti, False)

    def do_disassembly_test(self, func, expected, **kwargs):
        self.maxDiff = None
        got = self.get_disassembly(func, depth=0, **kwargs)
        self.do_disassembly_compare(got, expected)
        # Add checks for dis.disco
        if hasattr(func, '__code__'):
            got_disco = io.StringIO()
            with contextlib.redirect_stdout(got_disco):
                dis.disco(func.__code__, **kwargs)
            self.do_disassembly_compare(got_disco.getvalue(), expected)

    def test_opmap(self):
        self.assertEqual(dis.opmap["CACHE"], 0)
        self.assertIn(dis.opmap["LOAD_CONST"], dis.hasconst)
        self.assertIn(dis.opmap["STORE_NAME"], dis.hasname)

    def test_opname(self):
        self.assertEqual(dis.opname[dis.opmap["LOAD_FAST"]], "LOAD_FAST")

    def test_boundaries(self):
        self.assertEqual(dis.opmap["EXTENDED_ARG"], dis.EXTENDED_ARG)

    def test_widths(self):
        long_opcodes = set(['JUMP_BACKWARD_NO_INTERRUPT',
                            'LOAD_FAST_BORROW_LOAD_FAST_BORROW',
                            'INSTRUMENTED_CALL_FUNCTION_EX',
                            'YIELD_FROM_CORO_CHECK',
                            'ANNOTATIONS_PLACEHOLDER'])
        for op, opname in enumerate(dis.opname):
            if opname in long_opcodes or opname.startswith("INSTRUMENTED"):
                continue
            if opname in opcode._specialized_opmap:
                continue
            # Like specialisations, the IR's own handlers only show in the
            # adaptive disassembly
            if opname in opcode._ir_opmap:
                continue
            with self.subTest(opname=opname):
                width = dis._OPNAME_WIDTH
                if op in dis.hasarg:
                    width += 1 + dis._OPARG_WIDTH
                self.assertLessEqual(len(opname), width)

    def test_dis(self):
        self.do_disassembly_test(_f, dis_f)

    def test_dis_with_offsets(self):
        self.do_disassembly_test(_f, dis_f_with_offsets, show_offsets=True)

    @requires_debug_ranges()
    def test_dis_with_all_positions(self):
        def format_instr_positions(instr):
            values = tuple('?' if p is None else p for p in instr.positions)
            return '%s:%s-%s:%s' % (values[0], values[2], values[1], values[3])

        instrs = list(dis.get_instructions(_f))
        for instr in instrs:
            with self.subTest(instr=instr):
                self.assertTrue(all(p is not None for p in instr.positions))
        positions = tuple(map(format_instr_positions, instrs))
        expected = dis_f_with_positions_format % positions
        self.do_disassembly_test(_f, expected, show_positions=True)

    @requires_debug_ranges()
    def test_dis_with_some_positions(self):
        code = ("def f():\n"
                "   try: pass\n"
                "   finally:pass")
        f = compile(ast.parse(code), "?", "exec").co_consts[0]

        expect = '\n'.join([
            '1:0-1:0              RESUME                   0',
            '',
            '2:3-3:15             NOP',
            '',
            '3:11-3:15            LOAD_COMMON_CONSTANT     7 (None)  r0 <-',
            '3:11-3:15            RETURN_VALUE          <- r0',
            '',
            '  --         L1:     PUSH_EXC_INFO         r0 <- r0',
            '',
            '3:11-3:15            RERAISE                  0  <- r0',
            '',
            '  --         L2:     COPY                     3  <- r0',
            '  --                 POP_EXCEPT            <- r3',
            '  --                 RERAISE                  1  <- r1',
            'ExceptionTable:',
            '  L1 to L2 -> L2 [1] lasti',
            '',
        ])
        self.do_disassembly_test(f, expect, show_positions=True)

    @requires_debug_ranges()
    def test_dis_with_linenos_but_no_columns(self):
        code = "def f():\n\tx = 1"
        tree = ast.parse(code)
        func = tree.body[0]
        ass_x = func.body[0].targets[0]
        # remove columns information but keep line information
        ass_x.col_offset = ass_x.end_col_offset = -1
        f = compile(tree, "?", "exec").co_consts[0]

        expect = '\n'.join([
            '1:0-1:0            RESUME                   0',
            '',
            '2:5-2:6            LOAD_SMALL_INT           1  r0 <-',
            '2:?-2:?            LOAD_COMMON_CONSTANT     7 (None)  r1 <-',
            '2:?-2:?            RETURN_VALUE          <- r1',
            '',
        ])
        self.do_disassembly_test(f, expect, show_positions=True)

    def test_dis_with_no_positions(self):
        def f():
            pass

        f.__code__ = f.__code__.replace(co_linetable=b'')
        expect = '\n'.join([
            '          RESUME                   0',
            '          LOAD_COMMON_CONSTANT     7 (None)  r0 <-',
            '          RETURN_VALUE          <- r0',
            '',
        ])
        self.do_disassembly_test(f, expect, show_positions=True)

    def test_bug_708901(self):
        self.do_disassembly_test(bug708901, dis_bug708901)

    def test_bug_1333982(self):
        # This one is checking bytecodes generated for an `assert` statement,
        # so fails if the tests are run with -O.  Skip this test then.
        if not __debug__:
            self.skipTest('need asserts, run without -O')

        self.do_disassembly_test(bug1333982, dis_bug1333982)

    def test_bug_42562(self):
        self.do_disassembly_test(bug42562, dis_bug42562)

    def test_bug_45757(self):
        # Wide constant indices
        self.do_disassembly_test(code_bug_45757, dis_bug_45757)

    def test_bug_46724(self):
        # Test that a backward jump gets a label
        self.do_disassembly_test(bug46724, dis_bug46724)

    def test_kw_names(self):
        # Test that value is displayed for keyword argument names:
        self.do_disassembly_test(wrap_func_w_kwargs, dis_kw_names)

    def test_intrinsic_1(self):
        # Test that argrepr is displayed for CALL_INTRINSIC_1
        self.do_disassembly_test("from math import *", dis_intrinsic_1_2)
        self.do_disassembly_test("+a", dis_intrinsic_1_5)
        self.do_disassembly_test("(*a,)", dis_intrinsic_1_6)

    def test_intrinsic_2(self):
        self.assertIn("CALL_INTRINSIC_2         1 (INTRINSIC_PREP_RERAISE_STAR)",
                      self.get_disassembly("try: pass\nexcept* Exception: x"))

    def test_big_linenos(self):
        def func(count):
            namespace = {}
            func = "def foo():\n " + "".join(["\n "] * count + ["spam\n"])
            exec(func, namespace)
            return namespace['foo']

        # Test all small ranges
        for i in range(1, 300):
            expected = _BIG_LINENO_FORMAT % (i + 2)
            self.do_disassembly_test(func(i), expected)

        # Test some larger ranges too
        for i in range(300, 1000, 10):
            expected = _BIG_LINENO_FORMAT % (i + 2)
            self.do_disassembly_test(func(i), expected)

        for i in range(1000, 5000, 10):
            expected = _BIG_LINENO_FORMAT2 % (i + 2)
            self.do_disassembly_test(func(i), expected)

        from test import dis_module
        self.do_disassembly_test(dis_module, dis_module_expected_results)

    def test_disassemble_str(self):
        self.do_disassembly_test(expr_str, dis_expr_str)
        self.do_disassembly_test(simple_stmt_str, dis_simple_stmt_str)
        self.do_disassembly_test(annot_stmt_str, dis_annot_stmt_str)
        self.do_disassembly_test(fn_with_annotate_str, dis_fn_with_annotate_str)
        self.do_disassembly_test(compound_stmt_str, dis_compound_stmt_str)

    def test_disassemble_bytes(self):
        self.do_disassembly_test(_f.__code__.co_code, dis_f_co_code)

    def test_disassemble_class(self):
        self.do_disassembly_test(_C, dis_c)

    def test_disassemble_instance_method(self):
        self.do_disassembly_test(_C(1).__init__, dis_c_instance_method)

    def test_disassemble_instance_method_bytes(self):
        method_bytecode = _C(1).__init__.__code__.co_code
        self.do_disassembly_test(method_bytecode, dis_c_instance_method_bytes)

    def test_disassemble_static_method(self):
        self.do_disassembly_test(_C.sm, dis_c_static_method)

    def test_disassemble_class_method(self):
        self.do_disassembly_test(_C.cm, dis_c_class_method)

    def test_disassemble_generator(self):
        gen_func_disas = self.get_disassembly(_g)  # Generator function
        gen_disas = self.get_disassembly(_g(1))  # Generator iterator
        self.assertEqual(gen_disas, gen_func_disas)

    def test_disassemble_async_generator(self):
        agen_func_disas = self.get_disassembly(_ag)  # Async generator function
        agen_disas = self.get_disassembly(_ag(1))  # Async generator iterator
        self.assertEqual(agen_disas, agen_func_disas)

    def test_disassemble_coroutine(self):
        coro_func_disas = self.get_disassembly(_co)  # Coroutine function
        coro = _co(1)  # Coroutine object
        coro.close()  # Avoid a RuntimeWarning (never awaited)
        coro_disas = self.get_disassembly(coro)
        self.assertEqual(coro_disas, coro_func_disas)

    def test_disassemble_fstring(self):
        self.do_disassembly_test(_fstring, dis_fstring)

    def test_disassemble_with(self):
        self.do_disassembly_test(_with, dis_with)

    def test_disassemble_asyncwith(self):
        self.do_disassembly_test(_asyncwith, dis_asyncwith)

    def test_disassemble_try_finally(self):
        self.do_disassembly_test(_tryfinally, dis_tryfinally)
        self.do_disassembly_test(_tryfinallyconst, dis_tryfinallyconst)

    def test_dis_none(self):
        try:
            del sys.last_exc
        except AttributeError:
            pass
        try:
            del sys.last_traceback
        except AttributeError:
            pass
        self.assertRaises(RuntimeError, dis.dis, None)

    def test_dis_traceback(self):
        self.maxDiff = None
        try:
            del sys.last_traceback
        except AttributeError:
            pass

        try:
            1/0
        except Exception as e:
            tb = e.__traceback__
            sys.last_exc = e

        tb_dis = self.get_disassemble_as_string(tb.tb_frame.f_code, tb.tb_lasti)
        self.do_disassembly_test(None, tb_dis)

    def test_dis_object(self):
        self.assertRaises(TypeError, dis.dis, object())

    def test_disassemble_recursive(self):
        def check(expected, **kwargs):
            dis = self.get_disassembly(_h, **kwargs)
            dis = self.strip_addresses(dis)
            self.assertEqual(dis, expected)

        check(dis_nested_0, depth=0)
        check(dis_nested_1, depth=1)
        check(dis_nested_2, depth=2)
        check(dis_nested_2, depth=3)
        check(dis_nested_2, depth=None)
        check(dis_nested_2)

    def test__try_compile_no_context_exc_on_error(self):
        # see gh-102114
        try:
            dis._try_compile(")", "")
        except Exception as e:
            self.assertIsNone(e.__context__)

    def test_async_for_presentation(self):

        async def afunc():
            async for letter in async_iter1:
                l2
            l3

        disassembly =  self.get_disassembly(afunc)
        for line in disassembly.split("\n"):
            if "END_ASYNC_FOR" in line:
                break
        else:
            self.fail("No END_ASYNC_FOR in disassembly of async for")
        self.assertNotIn("to", line)
        self.assertIn("from", line)


    @staticmethod
    def code_quicken(f):
        _testinternalcapi = import_helper.import_module("_testinternalcapi")
        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            f()

    @cpython_only
    @requires_specialization
    def test_super_instructions(self):
        self.code_quicken(lambda: load_test(0, 0))
        got = self.get_disassembly(load_test, adaptive=True)
        self.do_disassembly_compare(got, dis_load_test_quickened_code)

    @cpython_only
    @requires_specialization
    def test_load_attr_specialize(self):
        load_attr_quicken = """\
  0           RESUME_CHECK             0

  1           LOAD_CONST               0 ('a')  r0 <-
              LOAD_ATTR_SLOT           0 (__class__)  r0 <- r0
              RETURN_VALUE          <- r0
"""
        co = compile("'a'.__class__", "", "eval")
        self.code_quicken(lambda: exec(co, {}, {}))
        got = self.get_disassembly(co, adaptive=True)
        self.do_disassembly_compare(got, load_attr_quicken)

    @cpython_only
    @requires_specialization
    def test_call_specialize(self):
        call_quicken = """\
  0           RESUME_CHECK             0

  1           LOAD_NAME                0 (str)  r0 <-
              PUSH_NULL             r1 <-
              LOAD_SMALL_INT           1  r2 <-
              CALL_STR_1               1  r0 <- r0
              RETURN_VALUE          <- r0
"""
        co = compile("str(1)", "", "eval")
        self.code_quicken(lambda: exec(co, {}, {}))
        got = self.get_disassembly(co, adaptive=True)
        self.do_disassembly_compare(got, quickened_ints(call_quicken))

    @cpython_only
    @requires_specialization
    def test_loop_quicken(self):
        # Loop can trigger a quicken where the loop is located
        self.code_quicken(loop_test)
        got = self.get_disassembly(loop_test, adaptive=True)
        self.do_disassembly_compare(got, quickened_ints(dis_loop_test_quickened_code))

    @cpython_only
    @requires_specialization
    def test_loop_with_conditional_at_end_is_quickened(self):
        _testinternalcapi = import_helper.import_module("_testinternalcapi")
        # PYTHON_TAGGED_VALUES=0 leaves range loops unspecialized.
        range_loops = getattr(_testinternalcapi, "TAGGED_VALUES", True)
        def for_loop_true(x):
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                if x:
                    pass

        for_loop_true(True)
        if range_loops:
            self.assertIn('FOR_ITER_RANGE',
                          self.get_disassembly(for_loop_true, adaptive=True))

        def for_loop_false(x):
            for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
                if x:
                    pass

        for_loop_false(False)
        if range_loops:
            self.assertIn('FOR_ITER_RANGE',
                          self.get_disassembly(for_loop_false, adaptive=True))

        def while_loop():
            i = 0
            while i < _testinternalcapi.SPECIALIZATION_THRESHOLD:
                i += 1

        while_loop()
        self.assertIn('COMPARE_OP_INT',
                      self.get_disassembly(while_loop, adaptive=True))

    @cpython_only
    def test_extended_arg_quick(self):
        # An oparg past a byte fits in a field: no EXTENDED_ARG
        got = self.get_disassembly(extended_arg_quick)
        self.do_disassembly_compare(got, dis_extended_arg_quick_code)

    def get_feedback(self, quickened, adaptive):
        def f():
            l = []
            for i in range(42):
                l.append(i)
        if quickened:
            self.code_quicken(f)
        else:
            # "copy" the code to un-quicken it:
            reset_code(f)
        instructions = list(dis.get_instructions(f, show_caches=True,
                                                 adaptive=adaptive))
        text = self.get_disassembly(f, show_caches=True, adaptive=adaptive)
        return instructions, text

    @cpython_only
    def test_show_caches(self):
        # show_caches shows the feedback entry of each instruction that has
        # one: its kind and static operand, and for the executing copy also
        # its state and counters
        fields = {
            ("kind", "arg"),
            ("kind", "state", "counter", "arg", "first", "second"),
        }
        for quickened in (False, True):
            for adaptive in (False, True):
                with self.subTest(f"{quickened=}, {adaptive=}"):
                    instructions, text = self.get_feedback(quickened, adaptive)
                    with_fb = [instr for instr in instructions
                               if instr.fb is not None]
                    self.assertEqual([instr.fb for instr in with_fb],
                                     list(range(1, 7)))
                    for instr in instructions:
                        self.assertEqual(instr.cache_info is not None,
                                         instr.fb is not None)
                    for instr in with_fb:
                        names = tuple(name for name, _, _ in instr.cache_info)
                        self.assertIn(names, fields)
                    self.assertEqual(text.count(" FEEDBACK "), 6)
                    # The free-threaded build does not specialise the IR
                    if quickened and adaptive and not support.Py_GIL_DISABLED:
                        states = [dict((name, data) for name, _, data
                                       in instr.cache_info)["state"]
                                  for instr in with_fb]
                        self.assertIn(b"\x01", states)  # MONO

    @cpython_only
    def test_show_currinstr_with_cache(self):
        """
        Make sure that the current instruction is marked once, also when its
        feedback entry is shown
        """
        def f():
            print(a)
        load = [instr for instr in dis.get_instructions(f)
                if instr.opname == "LOAD_GLOBAL"][0]
        self.assertIsNotNone(load.fb)
        for show_caches in (False, True):
            assem = self.get_disassembly(f.__code__, lasti=load.offset,
                                         wrapper=False, show_caches=show_caches)
            self.assertRegex(assem, r"--> LOAD_GLOBAL")
            self.assertEqual(assem.count("-->"), 1)


class DisWithFileTests(DisTests):

    # Run the tests again, using the file arg instead of print
    def get_disassembly(self, func, lasti=-1, wrapper=True, **kwargs):
        output = io.StringIO()
        if wrapper:
            dis.dis(func, file=output, **kwargs)
        else:
            dis.disassemble(func, lasti, file=output, **kwargs)
        return output.getvalue()


if dis.code_info.__doc__ is None:
    code_info_consts = "0: None"
else:
    code_info_consts = "0: 'Formatted details of methods, functions, or code.'"

code_info_code_info = f"""\
Name:              code_info
Filename:          (.*)
Argument count:    1
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  1
Stack size:        \\d+
Flags:             OPTIMIZED, NEWLOCALS(, HAS_DOCSTRING)?
Constants:
   {code_info_consts}
Names:
   0: _format_code_info
   1: _get_code_object
Variable names:
   0: x"""


@staticmethod
def tricky(a, b, /, x, y, z=True, *args, c, d, e=[], **kwds):
    def f(c=c):
        print(a, b, x, y, z, c, d, e, f)
    yield a, b, x, y, z, c, d, e, f

code_info_tricky = """\
Name:              tricky
Filename:          (.*)
Argument count:    5
Positional-only arguments: 2
Kw-only arguments: 3
Number of locals:  10
Stack size:        \\d+
Flags:             OPTIMIZED, NEWLOCALS, VARARGS, VARKEYWORDS, GENERATOR
Constants:
   0: <code object f at (.*), file "(.*)", line (.*)>
Variable names:
   0: a
   1: b
   2: x
   3: y
   4: z
   5: c
   6: d
   7: e
   8: args
   9: kwds
Cell variables:
   0: [abedfxyz]
   1: [abedfxyz]
   2: [abedfxyz]
   3: [abedfxyz]
   4: [abedfxyz]
   5: [abedfxyz]
   6: [abedfxyz]
   7: [abedfxyz]"""
# NOTE: the order of the cell variables above depends on dictionary order!

co_tricky_nested_f = tricky.__func__.__code__.co_consts[0]

code_info_tricky_nested_f = """\
Filename:          (.*)
Argument count:    1
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  1
Stack size:        \\d+
Flags:             OPTIMIZED, NEWLOCALS, NESTED
Constants:
   0: None
Names:
   0: print
Variable names:
   0: c
Free variables:
   0: [abedfxyz]
   1: [abedfxyz]
   2: [abedfxyz]
   3: [abedfxyz]
   4: [abedfxyz]
   5: [abedfxyz]"""

code_info_expr_str = """\
Name:              <module>
Filename:          <disassembly>
Argument count:    0
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  0
Stack size:        \\d+
Flags:             0x0
Constants:
   0: 1
Names:
   0: x"""

code_info_simple_stmt_str = """\
Name:              <module>
Filename:          <disassembly>
Argument count:    0
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  0
Stack size:        \\d+
Flags:             0x0
Constants:
   0: 1
Names:
   0: x"""

code_info_compound_stmt_str = """\
Name:              <module>
Filename:          <disassembly>
Argument count:    0
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  0
Stack size:        \\d+
Flags:             0x0
Constants:
   0: 0
Names:
   0: x"""


async def async_def():
    await 1
    async for a in b: pass
    async with c as d: pass

code_info_async_def = """\
Name:              async_def
Filename:          (.*)
Argument count:    0
Positional-only arguments: 0
Kw-only arguments: 0
Number of locals:  2
Stack size:        \\d+
Flags:             OPTIMIZED, NEWLOCALS, COROUTINE
Constants:
   0: 1
Names:
   0: b
   1: c
Variable names:
   0: a
   1: d"""

class CodeInfoTests(unittest.TestCase):
    test_pairs = [
      (dis.code_info, code_info_code_info),
      (tricky, code_info_tricky),
      (co_tricky_nested_f, code_info_tricky_nested_f),
      (expr_str, code_info_expr_str),
      (simple_stmt_str, code_info_simple_stmt_str),
      (compound_stmt_str, code_info_compound_stmt_str),
      (async_def, code_info_async_def)
    ]

    def test_code_info(self):
        self.maxDiff = 1000
        for x, expected in self.test_pairs:
            self.assertRegex(dis.code_info(x), expected)

    def test_show_code(self):
        self.maxDiff = 1000
        for x, expected in self.test_pairs:
            with captured_stdout() as output:
                dis.show_code(x)
            self.assertRegex(output.getvalue(), expected+"\n")
            output = io.StringIO()
            dis.show_code(x, file=output)
            self.assertRegex(output.getvalue(), expected)

    def test_code_info_object(self):
        self.assertRaises(TypeError, dis.code_info, object())

    def test_pretty_flags_no_flags(self):
        self.assertEqual(dis.pretty_flags(0), '0x0')


# Fodder for instruction introspection tests
#   Editing any of these may require recalculating the expected output
def outer(a=1, b=2):
    def f(c=3, d=4):
        def inner(e=5, f=6):
            print(a, b, c, d, e, f)
        print(a, b, c, d)
        return inner
    print(a, b, '', 1, [], {}, "Hello world!")
    return f

def jumpy():
    # This won't actually run (but that's OK, we only disassemble it)
    for i in range(10):
        print(i)
        if i < 4:
            continue
        if i > 6:
            break
    else:
        print("I can haz else clause?")
    while i:
        print(i)
        i -= 1
        if i > 6:
            continue
        if i < 4:
            break
    else:
        print("Who let lolcatz into this test suite?")
    try:
        1 / 0
    except ZeroDivisionError:
        print("Here we go, here we go, here we go...")
    else:
        with i as dodgy:
            print("Never reach this")
    finally:
        print("OK, now we're done")

# End fodder for opinfo generation tests
expected_outer_line = 1
_line_offset = outer.__code__.co_firstlineno - 1
code_object_f = outer.__code__.co_consts[1]
expected_f_line = code_object_f.co_firstlineno - _line_offset
code_object_inner = code_object_f.co_consts[1]
expected_inner_line = code_object_inner.co_firstlineno - _line_offset
expected_jumpy_line = 1

# The following lines are useful to regenerate the expected results after
# either the fodder is modified or the bytecode generation changes
# After regeneration, update the references to code_object_f and
# code_object_inner before rerunning the tests

def _stringify_instruction(instr):
    # Since postions offsets change a lot for these test cases, ignore them.
    base = (
        f"  make_inst(opname={instr.opname!r}, arg={instr.arg!r}, argval={instr.argval!r}, " +
        f"argrepr={instr.argrepr!r}, offset={instr.offset}, start_offset={instr.start_offset}, " +
        f"starts_line={instr.starts_line!r}, line_number={instr.line_number}"
    )
    if instr.label is not None:
        base += f", label={instr.label!r}"
    if instr.handler != instr.opname:
        base += f", handler={instr.handler!r}"
    if instr.dst is not None:
        base += f", dst={instr.dst!r}"
    if instr.srcs:
        base += f", srcs={instr.srcs!r}"
    if instr.fb is not None:
        base += f", fb={instr.fb!r}"
    if instr.cache_info:
        base += f", cache_info={instr.cache_info!r}"
    return base + "),"

def _prepare_test_cases():
    ignore = io.StringIO()
    with contextlib.redirect_stdout(ignore):
        f = outer()
        inner = f()
    _instructions_outer = dis.get_instructions(outer, first_line=expected_outer_line)
    _instructions_f = dis.get_instructions(f, first_line=expected_f_line)
    _instructions_inner = dis.get_instructions(inner, first_line=expected_inner_line)
    _instructions_jumpy = dis.get_instructions(jumpy, first_line=expected_jumpy_line)
    result = "\n".join(
        [
            "expected_opinfo_outer = [",
            *map(_stringify_instruction, _instructions_outer),
            "]",
            "",
            "expected_opinfo_f = [",
            *map(_stringify_instruction, _instructions_f),
            "]",
            "",
            "expected_opinfo_inner = [",
            *map(_stringify_instruction, _instructions_inner),
            "]",
            "",
            "expected_opinfo_jumpy = [",
            *map(_stringify_instruction, _instructions_jumpy),
            "]",
        ]
    )
    result = result.replace(repr(repr(code_object_f)), "repr(code_object_f)")
    result = result.replace(repr(code_object_f), "code_object_f")
    result = result.replace(repr(repr(code_object_inner)), "repr(code_object_inner)")
    result = result.replace(repr(code_object_inner), "code_object_inner")
    print(result)

# from test.test_dis import _prepare_test_cases; _prepare_test_cases()

make_inst = dis.Instruction.make

expected_opinfo_outer = [
  make_inst(opname='MAKE_CELL', arg=0, argval='a', argrepr='a', offset=0, start_offset=0, starts_line=True, line_number=None),
  make_inst(opname='MAKE_CELL', arg=1, argval='b', argrepr='b', offset=1, start_offset=1, starts_line=False, line_number=None),
  make_inst(opname='RESUME', arg=0, argval=0, argrepr='', offset=2, start_offset=2, starts_line=True, line_number=1),
  make_inst(opname='LOAD_CONST', arg=3, argval=(3, 4), argrepr='(3, 4)', offset=3, start_offset=3, starts_line=True, line_number=2, dst=3),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='a', argrepr='a', offset=4, start_offset=4, starts_line=False, line_number=2, dst=4),
  make_inst(opname='LOAD_FAST_BORROW', arg=1, argval='b', argrepr='b', offset=5, start_offset=5, starts_line=False, line_number=2, dst=5),
  make_inst(opname='BUILD_TUPLE', arg=2, argval=2, argrepr='', offset=6, start_offset=6, starts_line=False, line_number=2, dst=4, srcs=(4,)),
  make_inst(opname='LOAD_CONST', arg=1, argval=code_object_f, argrepr=repr(code_object_f), offset=7, start_offset=7, starts_line=False, line_number=2, dst=5),
  make_inst(opname='MAKE_FUNCTION', arg=None, argval=None, argrepr='', offset=8, start_offset=8, starts_line=False, line_number=2, dst=5, srcs=(5,)),
  make_inst(opname='SET_FUNCTION_ATTRIBUTE', arg=8, argval=8, argrepr='closure', offset=9, start_offset=9, starts_line=False, line_number=2, dst=4, srcs=(4, 5)),
  make_inst(opname='SET_FUNCTION_ATTRIBUTE', arg=1, argval=1, argrepr='defaults', offset=10, start_offset=10, starts_line=False, line_number=2, dst=3, srcs=(3, 4)),
  make_inst(opname='STORE_FAST', arg=2, argval='f', argrepr='f', offset=11, start_offset=11, starts_line=False, line_number=2, srcs=(3,)),
  make_inst(opname='LOAD_GLOBAL', arg=1, argval='print', argrepr='print + NULL', offset=12, start_offset=12, starts_line=True, line_number=7, dst=3, fb=1, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='LOAD_DEREF', arg=0, argval='a', argrepr='a', offset=13, start_offset=13, starts_line=False, line_number=7, dst=5),
  make_inst(opname='LOAD_DEREF', arg=1, argval='b', argrepr='b', offset=14, start_offset=14, starts_line=False, line_number=7, dst=6),
  make_inst(opname='LOAD_COMMON_CONSTANT', arg=8, argval='', argrepr="''", offset=15, start_offset=15, starts_line=False, line_number=7, dst=7),
  make_inst(opname='LOAD_SMALL_INT', arg=1, argval=1, argrepr='', offset=16, start_offset=16, starts_line=False, line_number=7, handler='LOAD_SMALL_INT_INLINE', dst=8),
  make_inst(opname='BUILD_LIST', arg=0, argval=0, argrepr='', offset=17, start_offset=17, starts_line=False, line_number=7, dst=9, srcs=(9,)),
  make_inst(opname='BUILD_MAP', arg=0, argval=0, argrepr='', offset=18, start_offset=18, starts_line=False, line_number=7, dst=10, srcs=(10,)),
  make_inst(opname='LOAD_CONST', arg=2, argval='Hello world!', argrepr="'Hello world!'", offset=19, start_offset=19, starts_line=False, line_number=7, dst=11),
  make_inst(opname='CALL', arg=7, argval=7, argrepr='', offset=20, start_offset=20, starts_line=False, line_number=7, dst=3, srcs=(3,), fb=2, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x07\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=21, start_offset=21, starts_line=False, line_number=7, srcs=(3,)),
  make_inst(opname='RETURN_VALUE', arg=None, argval=None, argrepr='', offset=22, start_offset=22, starts_line=True, line_number=8, srcs=(2,)),
]

expected_opinfo_f = [
  make_inst(opname='COPY_FREE_VARS', arg=2, argval=2, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=None),
  make_inst(opname='MAKE_CELL', arg=0, argval='c', argrepr='c', offset=1, start_offset=1, starts_line=False, line_number=None),
  make_inst(opname='MAKE_CELL', arg=1, argval='d', argrepr='d', offset=2, start_offset=2, starts_line=False, line_number=None),
  make_inst(opname='RESUME', arg=0, argval=0, argrepr='', offset=3, start_offset=3, starts_line=True, line_number=2),
  make_inst(opname='LOAD_CONST', arg=2, argval=(5, 6), argrepr='(5, 6)', offset=4, start_offset=4, starts_line=True, line_number=3, dst=5),
  make_inst(opname='LOAD_FAST_BORROW', arg=3, argval='a', argrepr='a', offset=5, start_offset=5, starts_line=False, line_number=3, dst=6),
  make_inst(opname='LOAD_FAST_BORROW', arg=4, argval='b', argrepr='b', offset=6, start_offset=6, starts_line=False, line_number=3, dst=7),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='c', argrepr='c', offset=7, start_offset=7, starts_line=False, line_number=3, dst=8),
  make_inst(opname='LOAD_FAST_BORROW', arg=1, argval='d', argrepr='d', offset=8, start_offset=8, starts_line=False, line_number=3, dst=9),
  make_inst(opname='BUILD_TUPLE', arg=4, argval=4, argrepr='', offset=9, start_offset=9, starts_line=False, line_number=3, dst=6, srcs=(6,)),
  make_inst(opname='LOAD_CONST', arg=1, argval=code_object_inner, argrepr=repr(code_object_inner), offset=10, start_offset=10, starts_line=False, line_number=3, dst=7),
  make_inst(opname='MAKE_FUNCTION', arg=None, argval=None, argrepr='', offset=11, start_offset=11, starts_line=False, line_number=3, dst=7, srcs=(7,)),
  make_inst(opname='SET_FUNCTION_ATTRIBUTE', arg=8, argval=8, argrepr='closure', offset=12, start_offset=12, starts_line=False, line_number=3, dst=6, srcs=(6, 7)),
  make_inst(opname='SET_FUNCTION_ATTRIBUTE', arg=1, argval=1, argrepr='defaults', offset=13, start_offset=13, starts_line=False, line_number=3, dst=5, srcs=(5, 6)),
  make_inst(opname='STORE_FAST', arg=2, argval='inner', argrepr='inner', offset=14, start_offset=14, starts_line=False, line_number=3, srcs=(5,)),
  make_inst(opname='LOAD_GLOBAL', arg=1, argval='print', argrepr='print + NULL', offset=15, start_offset=15, starts_line=True, line_number=5, dst=5, fb=1, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='LOAD_DEREF', arg=3, argval='a', argrepr='a', offset=16, start_offset=16, starts_line=False, line_number=5, dst=7),
  make_inst(opname='LOAD_DEREF', arg=4, argval='b', argrepr='b', offset=17, start_offset=17, starts_line=False, line_number=5, dst=8),
  make_inst(opname='LOAD_DEREF', arg=0, argval='c', argrepr='c', offset=18, start_offset=18, starts_line=False, line_number=5, dst=9),
  make_inst(opname='LOAD_DEREF', arg=1, argval='d', argrepr='d', offset=19, start_offset=19, starts_line=False, line_number=5, dst=10),
  make_inst(opname='CALL', arg=4, argval=4, argrepr='', offset=20, start_offset=20, starts_line=False, line_number=5, dst=5, srcs=(5,), fb=2, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x04\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=21, start_offset=21, starts_line=False, line_number=5, srcs=(5,)),
  make_inst(opname='RETURN_VALUE', arg=None, argval=None, argrepr='', offset=22, start_offset=22, starts_line=True, line_number=6, srcs=(2,)),
]

expected_opinfo_inner = [
  make_inst(opname='COPY_FREE_VARS', arg=4, argval=4, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=None),
  make_inst(opname='RESUME', arg=0, argval=0, argrepr='', offset=1, start_offset=1, starts_line=True, line_number=3),
  make_inst(opname='LOAD_GLOBAL', arg=1, argval='print', argrepr='print + NULL', offset=2, start_offset=2, starts_line=True, line_number=4, dst=6, fb=1, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='LOAD_DEREF', arg=2, argval='a', argrepr='a', offset=3, start_offset=3, starts_line=False, line_number=4, dst=8),
  make_inst(opname='LOAD_DEREF', arg=3, argval='b', argrepr='b', offset=4, start_offset=4, starts_line=False, line_number=4, dst=9),
  make_inst(opname='LOAD_DEREF', arg=4, argval='c', argrepr='c', offset=5, start_offset=5, starts_line=False, line_number=4, dst=10),
  make_inst(opname='LOAD_DEREF', arg=5, argval='d', argrepr='d', offset=6, start_offset=6, starts_line=False, line_number=4, dst=11),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='e', argrepr='e', offset=7, start_offset=7, starts_line=False, line_number=4, dst=12),
  make_inst(opname='LOAD_FAST_BORROW', arg=1, argval='f', argrepr='f', offset=8, start_offset=8, starts_line=False, line_number=4, dst=13),
  make_inst(opname='CALL', arg=6, argval=6, argrepr='', offset=9, start_offset=9, starts_line=False, line_number=4, dst=6, srcs=(6,), fb=2, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x06\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=10, start_offset=10, starts_line=False, line_number=4, srcs=(6,)),
  make_inst(opname='LOAD_COMMON_CONSTANT', arg=7, argval=None, argrepr='None', offset=11, start_offset=11, starts_line=False, line_number=4, dst=6),
  make_inst(opname='RETURN_VALUE', arg=None, argval=None, argrepr='', offset=12, start_offset=12, starts_line=False, line_number=4, srcs=(6,)),
]

expected_opinfo_jumpy = [
  make_inst(opname='RESUME', arg=0, argval=0, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=1),
  make_inst(opname='LOAD_GLOBAL', arg=1, argval='range', argrepr='range + NULL', offset=1, start_offset=1, starts_line=True, line_number=3, dst=2, fb=1, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='LOAD_SMALL_INT', arg=10, argval=10, argrepr='', offset=2, start_offset=2, starts_line=False, line_number=3, handler='LOAD_SMALL_INT_INLINE', dst=4),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=3, start_offset=3, starts_line=False, line_number=3, dst=2, srcs=(2,), fb=2, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='GET_ITER', arg=0, argval=0, argrepr='', offset=4, start_offset=4, starts_line=False, line_number=3, dst=2, srcs=(2,), fb=3, cache_info=[('kind', 1, b'\x02'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='FOR_ITER', arg=19, argval=25, argrepr='to L4', offset=5, start_offset=5, starts_line=False, line_number=3, label=1, dst=0, srcs=(2,), fb=4, cache_info=[('kind', 1, b'\x02'), ('arg', 3, b'\x13\x00\x00')]),
  make_inst(opname='LOAD_GLOBAL', arg=3, argval='print', argrepr='print + NULL', offset=6, start_offset=6, starts_line=True, line_number=4, dst=4, fb=5, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='i', argrepr='i', offset=7, start_offset=7, starts_line=False, line_number=4, dst=6),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=8, start_offset=8, starts_line=False, line_number=4, dst=4, srcs=(4,), fb=6, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=9, start_offset=9, starts_line=False, line_number=4, srcs=(4,)),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='i', argrepr='i', offset=10, start_offset=10, starts_line=True, line_number=5, dst=4),
  make_inst(opname='LOAD_SMALL_INT', arg=4, argval=4, argrepr='', offset=11, start_offset=11, starts_line=False, line_number=5, handler='LOAD_SMALL_INT_INLINE', dst=5),
  make_inst(opname='COMPARE_OP', arg=18, argval='<', argrepr='bool(<)', offset=12, start_offset=12, starts_line=False, line_number=5, dst=4, srcs=(4, 5), fb=7, cache_info=[('kind', 1, b'\x03'), ('arg', 3, b'\x12\x00\x00')]),
  make_inst(opname='POP_JUMP_IF_FALSE', arg=16, argval=16, argrepr='to L2', offset=13, start_offset=13, starts_line=False, line_number=5, srcs=(4,), fb=8, cache_info=[('kind', 1, b'\x01'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='NOT_TAKEN', arg=None, argval=None, argrepr='', offset=14, start_offset=14, starts_line=False, line_number=5),
  make_inst(opname='JUMP_BACKWARD', arg=5, argval=5, argrepr='to L1', offset=15, start_offset=15, starts_line=True, line_number=6),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='i', argrepr='i', offset=16, start_offset=16, starts_line=True, line_number=7, label=2, dst=4),
  make_inst(opname='LOAD_SMALL_INT', arg=6, argval=6, argrepr='', offset=17, start_offset=17, starts_line=False, line_number=7, handler='LOAD_SMALL_INT_INLINE', dst=5),
  make_inst(opname='COMPARE_OP', arg=148, argval='>', argrepr='bool(>)', offset=18, start_offset=18, starts_line=False, line_number=7, dst=4, srcs=(4, 5), fb=9, cache_info=[('kind', 1, b'\x03'), ('arg', 3, b'\x94\x00\x00')]),
  make_inst(opname='POP_JUMP_IF_TRUE', arg=22, argval=22, argrepr='to L3', offset=19, start_offset=19, starts_line=False, line_number=7, srcs=(4,), fb=10, cache_info=[('kind', 1, b'\x01'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='NOT_TAKEN', arg=None, argval=None, argrepr='', offset=20, start_offset=20, starts_line=False, line_number=7),
  make_inst(opname='JUMP_BACKWARD', arg=5, argval=5, argrepr='to L1', offset=21, start_offset=21, starts_line=False, line_number=7),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=22, start_offset=22, starts_line=True, line_number=8, label=3, srcs=(3,)),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=23, start_offset=23, starts_line=False, line_number=8, srcs=(2,)),
  make_inst(opname='JUMP_FORWARD', arg=31, argval=31, argrepr='to L5', offset=24, start_offset=24, starts_line=False, line_number=8),
  make_inst(opname='END_FOR', arg=None, argval=None, argrepr='', offset=25, start_offset=25, starts_line=True, line_number=3, label=4, srcs=(2, 3, 4)),
  make_inst(opname='POP_ITER', arg=None, argval=None, argrepr='', offset=26, start_offset=26, starts_line=False, line_number=3, srcs=(2, 3)),
  make_inst(opname='LOAD_GLOBAL', arg=3, argval='print', argrepr='print + NULL', offset=27, start_offset=27, starts_line=True, line_number=10, dst=2, fb=11, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='LOAD_CONST', arg=1, argval='I can haz else clause?', argrepr="'I can haz else clause?'", offset=28, start_offset=28, starts_line=False, line_number=10, dst=4),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=29, start_offset=29, starts_line=False, line_number=10, dst=2, srcs=(2,), fb=12, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=30, start_offset=30, starts_line=False, line_number=10, srcs=(2,)),
  make_inst(opname='LOAD_FAST_CHECK', arg=0, argval='i', argrepr='i', offset=31, start_offset=31, starts_line=True, line_number=11, label=5, dst=2),
  make_inst(opname='POP_JUMP_IF_FALSE', arg=54, argval=54, argrepr='to L8', offset=32, start_offset=32, starts_line=False, line_number=11, srcs=(2,), fb=13, cache_info=[('kind', 1, b'\x01'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='NOT_TAKEN', arg=None, argval=None, argrepr='', offset=33, start_offset=33, starts_line=False, line_number=11),
  make_inst(opname='LOAD_GLOBAL', arg=3, argval='print', argrepr='print + NULL', offset=34, start_offset=34, starts_line=True, line_number=12, dst=2, fb=14, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='i', argrepr='i', offset=35, start_offset=35, starts_line=False, line_number=12, dst=4),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=36, start_offset=36, starts_line=False, line_number=12, dst=2, srcs=(2,), fb=15, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=37, start_offset=37, starts_line=False, line_number=12, srcs=(2,)),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='i', argrepr='i', offset=38, start_offset=38, starts_line=True, line_number=13, dst=2),
  make_inst(opname='LOAD_SMALL_INT', arg=1, argval=1, argrepr='', offset=39, start_offset=39, starts_line=False, line_number=13, handler='LOAD_SMALL_INT_INLINE', dst=3),
  make_inst(opname='BINARY_OP', arg=23, argval=23, argrepr='-=', offset=40, start_offset=40, starts_line=False, line_number=13, handler='BINARY_OP_INPLACE_SUBTRACT', dst=0, srcs=(2, 3), fb=16, cache_info=[('kind', 1, b'\x03'), ('arg', 3, b'\x17\x00\x00')]),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='i', argrepr='i', offset=41, start_offset=41, starts_line=True, line_number=14, dst=2),
  make_inst(opname='LOAD_SMALL_INT', arg=6, argval=6, argrepr='', offset=42, start_offset=42, starts_line=False, line_number=14, handler='LOAD_SMALL_INT_INLINE', dst=3),
  make_inst(opname='COMPARE_OP', arg=148, argval='>', argrepr='bool(>)', offset=43, start_offset=43, starts_line=False, line_number=14, dst=2, srcs=(2, 3), fb=17, cache_info=[('kind', 1, b'\x03'), ('arg', 3, b'\x94\x00\x00')]),
  make_inst(opname='POP_JUMP_IF_FALSE', arg=47, argval=47, argrepr='to L6', offset=44, start_offset=44, starts_line=False, line_number=14, srcs=(2,), fb=18, cache_info=[('kind', 1, b'\x01'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='NOT_TAKEN', arg=None, argval=None, argrepr='', offset=45, start_offset=45, starts_line=False, line_number=14),
  make_inst(opname='JUMP_BACKWARD', arg=31, argval=31, argrepr='to L5', offset=46, start_offset=46, starts_line=True, line_number=15),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='i', argrepr='i', offset=47, start_offset=47, starts_line=True, line_number=16, label=6, dst=2),
  make_inst(opname='LOAD_SMALL_INT', arg=4, argval=4, argrepr='', offset=48, start_offset=48, starts_line=False, line_number=16, handler='LOAD_SMALL_INT_INLINE', dst=3),
  make_inst(opname='COMPARE_OP', arg=18, argval='<', argrepr='bool(<)', offset=49, start_offset=49, starts_line=False, line_number=16, dst=2, srcs=(2, 3), fb=19, cache_info=[('kind', 1, b'\x03'), ('arg', 3, b'\x12\x00\x00')]),
  make_inst(opname='POP_JUMP_IF_TRUE', arg=53, argval=53, argrepr='to L7', offset=50, start_offset=50, starts_line=False, line_number=16, srcs=(2,), fb=20, cache_info=[('kind', 1, b'\x01'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='NOT_TAKEN', arg=None, argval=None, argrepr='', offset=51, start_offset=51, starts_line=False, line_number=16),
  make_inst(opname='JUMP_BACKWARD', arg=31, argval=31, argrepr='to L5', offset=52, start_offset=52, starts_line=False, line_number=16),
  make_inst(opname='JUMP_FORWARD', arg=58, argval=58, argrepr='to L9', offset=53, start_offset=53, starts_line=True, line_number=17, label=7),
  make_inst(opname='LOAD_GLOBAL', arg=3, argval='print', argrepr='print + NULL', offset=54, start_offset=54, starts_line=True, line_number=19, label=8, dst=2, fb=21, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='LOAD_CONST', arg=2, argval='Who let lolcatz into this test suite?', argrepr="'Who let lolcatz into this test suite?'", offset=55, start_offset=55, starts_line=False, line_number=19, dst=4),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=56, start_offset=56, starts_line=False, line_number=19, dst=2, srcs=(2,), fb=22, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=57, start_offset=57, starts_line=False, line_number=19, srcs=(2,)),
  make_inst(opname='NOP', arg=None, argval=None, argrepr='', offset=58, start_offset=58, starts_line=True, line_number=20, label=9),
  make_inst(opname='LOAD_SMALL_INT', arg=1, argval=1, argrepr='', offset=59, start_offset=59, starts_line=True, line_number=21, handler='LOAD_SMALL_INT_INLINE', dst=2),
  make_inst(opname='LOAD_SMALL_INT', arg=0, argval=0, argrepr='', offset=60, start_offset=60, starts_line=False, line_number=21, handler='LOAD_SMALL_INT_INLINE', dst=3),
  make_inst(opname='BINARY_OP', arg=11, argval=11, argrepr='/', offset=61, start_offset=61, starts_line=False, line_number=21, handler='BINARY_OP_TRUE_DIVIDE', dst=2, srcs=(2, 3), fb=23, cache_info=[('kind', 1, b'\x03'), ('arg', 3, b'\x0b\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=62, start_offset=62, starts_line=False, line_number=21, srcs=(2,)),
  make_inst(opname='LOAD_FAST_BORROW', arg=0, argval='i', argrepr='i', offset=63, start_offset=63, starts_line=True, line_number=25, dst=2),
  make_inst(opname='LOAD_SPECIAL', arg=1, argval=1, argrepr='__exit__', offset=64, start_offset=64, starts_line=False, line_number=25, srcs=(0,)),
  make_inst(opname='SWAP', arg=2, argval=2, argrepr='', offset=65, start_offset=65, starts_line=False, line_number=25, srcs=(3,)),
  make_inst(opname='SWAP', arg=3, argval=3, argrepr='', offset=66, start_offset=66, starts_line=False, line_number=25, srcs=(2,)),
  make_inst(opname='LOAD_SPECIAL', arg=0, argval=0, argrepr='__enter__', offset=67, start_offset=67, starts_line=False, line_number=25, srcs=(4,)),
  make_inst(opname='CALL', arg=0, argval=0, argrepr='', offset=68, start_offset=68, starts_line=False, line_number=25, dst=4, srcs=(4,), fb=24, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='STORE_FAST', arg=1, argval='dodgy', argrepr='dodgy', offset=69, start_offset=69, starts_line=False, line_number=25, srcs=(4,)),
  make_inst(opname='LOAD_GLOBAL', arg=3, argval='print', argrepr='print + NULL', offset=70, start_offset=70, starts_line=True, line_number=26, dst=4, fb=25, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='LOAD_CONST', arg=3, argval='Never reach this', argrepr="'Never reach this'", offset=71, start_offset=71, starts_line=False, line_number=26, dst=6),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=72, start_offset=72, starts_line=False, line_number=26, dst=4, srcs=(4,), fb=26, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=73, start_offset=73, starts_line=False, line_number=26, srcs=(4,)),
  make_inst(opname='LOAD_COMMON_CONSTANT', arg=7, argval=None, argrepr='None', offset=74, start_offset=74, starts_line=True, line_number=25, dst=4),
  make_inst(opname='LOAD_COMMON_CONSTANT', arg=7, argval=None, argrepr='None', offset=75, start_offset=75, starts_line=False, line_number=25, dst=5),
  make_inst(opname='LOAD_COMMON_CONSTANT', arg=7, argval=None, argrepr='None', offset=76, start_offset=76, starts_line=False, line_number=25, dst=6),
  make_inst(opname='CALL', arg=3, argval=3, argrepr='', offset=77, start_offset=77, starts_line=False, line_number=25, dst=2, srcs=(2,), fb=27, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=78, start_offset=78, starts_line=False, line_number=25, srcs=(2,)),
  make_inst(opname='LOAD_GLOBAL', arg=3, argval='print', argrepr='print + NULL', offset=79, start_offset=79, starts_line=True, line_number=28, label=10, dst=2, fb=28, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='LOAD_CONST', arg=5, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=80, start_offset=80, starts_line=False, line_number=28, dst=4),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=81, start_offset=81, starts_line=False, line_number=28, dst=2, srcs=(2,), fb=29, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=82, start_offset=82, starts_line=False, line_number=28, srcs=(2,)),
  make_inst(opname='LOAD_COMMON_CONSTANT', arg=7, argval=None, argrepr='None', offset=83, start_offset=83, starts_line=False, line_number=28, dst=2),
  make_inst(opname='RETURN_VALUE', arg=None, argval=None, argrepr='', offset=84, start_offset=84, starts_line=False, line_number=28, srcs=(2,)),
  make_inst(opname='PUSH_EXC_INFO', arg=None, argval=None, argrepr='', offset=85, start_offset=85, starts_line=True, line_number=25, dst=5, srcs=(5,)),
  make_inst(opname='WITH_EXCEPT_START', arg=None, argval=None, argrepr='', offset=86, start_offset=86, starts_line=False, line_number=25, dst=7, srcs=(2,)),
  make_inst(opname='POP_JUMP_IF_TRUE', arg=90, argval=90, argrepr='to L11', offset=87, start_offset=87, starts_line=False, line_number=25, srcs=(7,), fb=30, cache_info=[('kind', 1, b'\x01'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='NOT_TAKEN', arg=None, argval=None, argrepr='', offset=88, start_offset=88, starts_line=False, line_number=25),
  make_inst(opname='RERAISE', arg=2, argval=2, argrepr='', offset=89, start_offset=89, starts_line=False, line_number=25, srcs=(4,)),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=90, start_offset=90, starts_line=False, line_number=25, label=11, srcs=(6,)),
  make_inst(opname='POP_EXCEPT', arg=None, argval=None, argrepr='', offset=91, start_offset=91, starts_line=False, line_number=25, srcs=(5,)),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=92, start_offset=92, starts_line=False, line_number=25, srcs=(4,)),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=93, start_offset=93, starts_line=False, line_number=25, srcs=(3,)),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=94, start_offset=94, starts_line=False, line_number=25, srcs=(2,)),
  make_inst(opname='JUMP_BACKWARD_NO_INTERRUPT', arg=79, argval=79, argrepr='to L10', offset=95, start_offset=95, starts_line=False, line_number=25),
  make_inst(opname='COPY', arg=3, argval=3, argrepr='', offset=96, start_offset=96, starts_line=True, line_number=None, srcs=(5,)),
  make_inst(opname='POP_EXCEPT', arg=None, argval=None, argrepr='', offset=97, start_offset=97, starts_line=False, line_number=None, srcs=(8,)),
  make_inst(opname='RERAISE', arg=1, argval=1, argrepr='', offset=98, start_offset=98, starts_line=False, line_number=None, srcs=(6,)),
  make_inst(opname='PUSH_EXC_INFO', arg=None, argval=None, argrepr='', offset=99, start_offset=99, starts_line=False, line_number=None, dst=2, srcs=(2,)),
  make_inst(opname='LOAD_GLOBAL', arg=4, argval='ZeroDivisionError', argrepr='ZeroDivisionError', offset=100, start_offset=100, starts_line=True, line_number=22, dst=4, fb=31, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x04\x00\x00')]),
  make_inst(opname='CHECK_EXC_MATCH', arg=None, argval=None, argrepr='', offset=101, start_offset=101, starts_line=False, line_number=22, dst=4, srcs=(3, 4)),
  make_inst(opname='POP_JUMP_IF_FALSE', arg=111, argval=111, argrepr='to L12', offset=102, start_offset=102, starts_line=False, line_number=22, srcs=(4,), fb=32, cache_info=[('kind', 1, b'\x01'), ('arg', 3, b'\x00\x00\x00')]),
  make_inst(opname='NOT_TAKEN', arg=None, argval=None, argrepr='', offset=103, start_offset=103, starts_line=False, line_number=22),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=104, start_offset=104, starts_line=False, line_number=22, srcs=(3,)),
  make_inst(opname='LOAD_GLOBAL', arg=3, argval='print', argrepr='print + NULL', offset=105, start_offset=105, starts_line=True, line_number=23, dst=3, fb=33, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='LOAD_CONST', arg=4, argval='Here we go, here we go, here we go...', argrepr="'Here we go, here we go, here we go...'", offset=106, start_offset=106, starts_line=False, line_number=23, dst=5),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=107, start_offset=107, starts_line=False, line_number=23, dst=3, srcs=(3,), fb=34, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=108, start_offset=108, starts_line=False, line_number=23, srcs=(3,)),
  make_inst(opname='POP_EXCEPT', arg=None, argval=None, argrepr='', offset=109, start_offset=109, starts_line=False, line_number=23, srcs=(2,)),
  make_inst(opname='JUMP_BACKWARD_NO_INTERRUPT', arg=79, argval=79, argrepr='to L10', offset=110, start_offset=110, starts_line=False, line_number=23),
  make_inst(opname='RERAISE', arg=0, argval=0, argrepr='', offset=111, start_offset=111, starts_line=True, line_number=22, label=12, srcs=(3,)),
  make_inst(opname='COPY', arg=3, argval=3, argrepr='', offset=112, start_offset=112, starts_line=True, line_number=None, srcs=(2,)),
  make_inst(opname='POP_EXCEPT', arg=None, argval=None, argrepr='', offset=113, start_offset=113, starts_line=False, line_number=None, srcs=(5,)),
  make_inst(opname='RERAISE', arg=1, argval=1, argrepr='', offset=114, start_offset=114, starts_line=False, line_number=None, srcs=(3,)),
  make_inst(opname='PUSH_EXC_INFO', arg=None, argval=None, argrepr='', offset=115, start_offset=115, starts_line=False, line_number=None, dst=2, srcs=(2,)),
  make_inst(opname='LOAD_GLOBAL', arg=3, argval='print', argrepr='print + NULL', offset=116, start_offset=116, starts_line=True, line_number=28, dst=4, fb=35, cache_info=[('kind', 1, b'\x05'), ('arg', 3, b'\x03\x00\x00')]),
  make_inst(opname='LOAD_CONST', arg=5, argval="OK, now we're done", argrepr='"OK, now we\'re done"', offset=117, start_offset=117, starts_line=False, line_number=28, dst=6),
  make_inst(opname='CALL', arg=1, argval=1, argrepr='', offset=118, start_offset=118, starts_line=False, line_number=28, dst=4, srcs=(4,), fb=36, cache_info=[('kind', 1, b'\x06'), ('arg', 3, b'\x01\x00\x00')]),
  make_inst(opname='POP_TOP', arg=None, argval=None, argrepr='', offset=119, start_offset=119, starts_line=False, line_number=28, srcs=(4,)),
  make_inst(opname='RERAISE', arg=0, argval=0, argrepr='', offset=120, start_offset=120, starts_line=False, line_number=28, srcs=(3,)),
  make_inst(opname='COPY', arg=3, argval=3, argrepr='', offset=121, start_offset=121, starts_line=True, line_number=None, srcs=(2,)),
  make_inst(opname='POP_EXCEPT', arg=None, argval=None, argrepr='', offset=122, start_offset=122, starts_line=False, line_number=None, srcs=(5,)),
  make_inst(opname='RERAISE', arg=1, argval=1, argrepr='', offset=123, start_offset=123, starts_line=False, line_number=None, srcs=(3,)),
]

# One last piece of inspect fodder to check the default line number handling
def simple(): pass
expected_opinfo_simple = [
  make_inst(opname='RESUME', arg=0, argval=0, argrepr='', offset=0, start_offset=0, starts_line=True, line_number=simple.__code__.co_firstlineno),
  make_inst(opname='LOAD_COMMON_CONSTANT', arg=7, argval=None, argrepr='None', offset=1, start_offset=1, starts_line=False, line_number=simple.__code__.co_firstlineno, dst=0),
  make_inst(opname='RETURN_VALUE', arg=None, argval=None, argrepr='', offset=2, start_offset=2, starts_line=False, line_number=simple.__code__.co_firstlineno, srcs=(0,)),
]


class InstructionTestCase(BytecodeTestCase):

    def assertInstructionsEqual(self, instrs_1, instrs_2, /):
        instrs_1 = [instr_1._replace(positions=None, cache_info=None) for instr_1 in instrs_1]
        instrs_2 = [instr_2._replace(positions=None, cache_info=None) for instr_2 in instrs_2]
        self.assertEqual(instrs_1, instrs_2)

class InstructionTests(InstructionTestCase):

    def __init__(self, *args):
        super().__init__(*args)
        self.maxDiff = None

    def test_instruction_str(self):
        # smoke test for __str__
        instrs = dis.get_instructions(simple)
        for instr in instrs:
            str(instr)

    def test_default_first_line(self):
        actual = dis.get_instructions(simple)
        self.assertInstructionsEqual(list(actual), expected_opinfo_simple)

    def test_first_line_set_to_None(self):
        actual = dis.get_instructions(simple, first_line=None)
        self.assertInstructionsEqual(list(actual), expected_opinfo_simple)

    def test_outer(self):
        actual = dis.get_instructions(outer, first_line=expected_outer_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_outer)

    def test_nested(self):
        with captured_stdout():
            f = outer()
        actual = dis.get_instructions(f, first_line=expected_f_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_f)

    def test_doubly_nested(self):
        with captured_stdout():
            inner = outer()()
        actual = dis.get_instructions(inner, first_line=expected_inner_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_inner)

    def test_jumpy(self):
        actual = dis.get_instructions(jumpy, first_line=expected_jumpy_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_jumpy)

    @requires_debug_ranges()
    def test_co_positions(self):
        code = compile('f(\n  x, y, z\n)', '<test>', 'exec')
        positions = [
            instr.positions
            for instr in dis.get_instructions(code)
        ]
        expected = [
            (0, 1, 0, 0),
            (1, 1, 0, 1),
            (1, 1, 0, 1),
            (2, 2, 2, 3),
            (2, 2, 5, 6),
            (2, 2, 8, 9),
            (1, 3, 0, 1),
            (1, 3, 0, 1),
            (1, 3, 0, 1),
            (1, 3, 0, 1)
        ]
        self.assertEqual(positions, expected)

        named_positions = [
            (pos.lineno, pos.end_lineno, pos.col_offset, pos.end_col_offset)
            for pos in positions
        ]
        self.assertEqual(named_positions, expected)

    @requires_debug_ranges()
    def test_co_positions_missing_info(self):
        code = compile('x, y, z', '<test>', 'exec')
        code_without_location_table = code.replace(co_linetable=b'')
        actual = dis.get_instructions(code_without_location_table)
        for instruction in actual:
            with self.subTest(instruction=instruction):
                positions = instruction.positions
                self.assertEqual(len(positions), 4)
                if instruction.opname == "RESUME":
                    continue
                self.assertIsNone(positions.lineno)
                self.assertIsNone(positions.end_lineno)
                self.assertIsNone(positions.col_offset)
                self.assertIsNone(positions.end_col_offset)

    @requires_debug_ranges()
    def test_co_positions_per_word(self):
        # co_positions has an entry per word, and dis an instruction per word
        def roots(a, b, c):
            d = b**2 - 4 * a * c
            yield (-b - cmath.sqrt(d)) / (2 * a)
            if d:
                yield (-b + cmath.sqrt(d)) / (2 * a)
        code = roots.__code__
        for show_caches in (False, True):
            for adaptive in (False, True):
                with self.subTest(f"{adaptive=}, {show_caches=}"):
                    dis_positions = [
                        None if instruction.positions is None else (
                            instruction.positions.lineno,
                            instruction.positions.end_lineno,
                            instruction.positions.col_offset,
                            instruction.positions.end_col_offset,
                        )
                        for instruction in dis.get_instructions(
                            code, adaptive=adaptive, show_caches=show_caches)
                    ]
                    self.assertEqual(list(code.co_positions()), dis_positions)

    def test_oparg_alias(self):
        instruction = make_inst(opname="NOP", arg=None, argval=None,
                                  argrepr='', offset=10, start_offset=10, starts_line=True, line_number=1, label=None,
                                  positions=None)
        self.assertEqual(instruction.arg, instruction.oparg)

    def test_show_caches_with_label(self):
        def f(x, y, z):
            if x:
                res = y
            else:
                res = z
            return res

        output = io.StringIO()
        dis.dis(f.__code__, file=output, show_caches=True)
        self.assertIn("L1:", output.getvalue())

    def test_is_op_format(self):
        output = io.StringIO()
        dis.dis("a is b", file=output, show_caches=True)
        self.assertIn("IS_OP                    0 (is)", output.getvalue())

        output = io.StringIO()
        dis.dis("a is not b", file=output, show_caches=True)
        self.assertIn("IS_OP                    1 (is not)", output.getvalue())

    def test_contains_op_format(self):
        output = io.StringIO()
        dis.dis("a in b", file=output, show_caches=True)
        self.assertIn("CONTAINS_OP              0 (in)", output.getvalue())

        output = io.StringIO()
        dis.dis("a not in b", file=output, show_caches=True)
        self.assertIn("CONTAINS_OP              1 (not in)", output.getvalue())

    def test_baseopname_and_baseopcode(self):
        # Standard instructions
        for name in dis.opmap:
            instruction = make_inst(opname=name, arg=None, argval=None, argrepr='', offset=0,
                                    start_offset=0, starts_line=True, line_number=1, label=None, positions=None)
            baseopname = instruction.baseopname
            baseopcode = instruction.baseopcode
            self.assertIsNotNone(baseopname)
            self.assertIsNotNone(baseopcode)
            self.assertEqual(name, baseopname)
            self.assertEqual(instruction.opcode, baseopcode)

        # Specialized instructions
        for name in opcode._specialized_opmap:
            instruction = make_inst(opname=name, arg=None, argval=None, argrepr='',
                                    offset=0, start_offset=0, starts_line=True, line_number=1, label=None, positions=None)
            baseopname = instruction.baseopname
            baseopcode = instruction.baseopcode
            self.assertIn(name, opcode._specializations[baseopname])
            self.assertEqual(opcode.opmap[baseopname], baseopcode)

    def test_jump_target(self):
        # Non-jump instructions should return None
        instruction = make_inst(opname="NOP", arg=None, argval=None,
                                  argrepr='', offset=10, start_offset=10, starts_line=True, line_number=1, label=None,
                                  positions=None)
        self.assertIsNone(instruction.jump_target)

        # Jumps name the word index of their target
        target = 100
        for opname, offset in (("JUMP_FORWARD", 10), ("JUMP_BACKWARD", 200),
                               ("POP_JUMP_IF_FALSE", 10)):
            instruction = make_inst(opname=opname, arg=target, argval=target,
                                    argrepr='', offset=offset, start_offset=offset,
                                    starts_line=True, line_number=1, label=None,
                                    positions=None)
            self.assertEqual(target, instruction.jump_target)

        # FOR_ITER and SEND exit by a distance from the word after them
        delta = 100
        for opname in ("FOR_ITER", "SEND"):
            instruction = make_inst(opname=opname, arg=delta, argval=delta,
                                    argrepr='', offset=10, start_offset=10,
                                    starts_line=True, line_number=1, label=None,
                                    positions=None)
            self.assertEqual(10 + 1 + 100, instruction.jump_target)

    def test_argval_argrepr(self):
        def f(opcode, oparg, offset, *init_args):
            arg_resolver = dis.ArgResolver(*init_args)
            return arg_resolver.get_argval_argrepr(opcode, oparg, offset)

        offset = 42
        co_consts = (0, 1, 2, 3)
        names = {1: 'a', 2: 'b'}
        varname_from_oparg = lambda i : names[i]
        labels_map = {24: 1}
        args = (offset, co_consts, names, varname_from_oparg, labels_map)
        self.assertEqual(f(opcode.opmap["POP_TOP"], None, *args), (None, ''))
        self.assertEqual(f(opcode.opmap["LOAD_CONST"], 1, *args), (1, '1'))
        self.assertEqual(f(opcode.opmap["LOAD_GLOBAL"], 2, *args), ('a', 'a'))
        self.assertEqual(f(opcode.opmap["JUMP_BACKWARD"], 24, *args), (24, 'to L1'))
        self.assertEqual(f(opcode.opmap["COMPARE_OP"], 3, *args), ('<', '<'))
        self.assertEqual(f(opcode.opmap["SET_FUNCTION_ATTRIBUTE"], 2, *args), (2, 'kwdefaults'))
        self.assertEqual(f(opcode.opmap["BINARY_OP"], 3, *args), (3, '<<'))
        self.assertEqual(f(opcode.opmap["CALL_INTRINSIC_1"], 2, *args), (2, 'INTRINSIC_IMPORT_STAR'))

    def test_custom_arg_resolver(self):
        class MyArgResolver(dis.ArgResolver):
            def offset_from_jump_arg(self, op, arg, offset):
                return arg + 1

            def get_label_for_offset(self, offset):
                return 2 * offset

        def f(opcode, oparg, offset, *init_args):
            arg_resolver = MyArgResolver(*init_args)
            return arg_resolver.get_argval_argrepr(opcode, oparg, offset)
        offset = 42
        self.assertEqual(f(opcode.opmap["JUMP_BACKWARD"], 1, offset), (2, 'to L4'))
        self.assertEqual(f(opcode.opmap["SETUP_FINALLY"], 2, offset), (3, 'to L6'))


    def get_instructions(self, code):
        return dis._get_instructions_bytes(code)

    def test_start_offset(self):
        # An instruction is one word, with no EXTENDED_ARG before it, so
        # start_offset is offset
        many_consts = compile(
            "".join(f"a = {i}.5\n" for i in range(300)), "<test>", "exec")
        for code in (_f, many_consts):
            instructions = list(dis.Bytecode(code))
            self.assertNotIn("EXTENDED_ARG",
                             [instr.opname for instr in instructions])
            for instruction in instructions:
                self.assertEqual(instruction.offset, instruction.start_offset)

    def test_cache_offset_and_end_offset(self):
        # No inline cache follows an instruction: both offsets are the
        # index of the next word, and the start of the next instruction
        instructions = list(dis.Bytecode(self.test_cache_offset_and_end_offset))
        for instr in instructions:
            self.assertEqual(instr.cache_offset, instr.offset + 1)
            self.assertEqual(instr.end_offset, instr.offset + 1)
        for prev, curr in zip(instructions, instructions[1:]):
            self.assertEqual(prev.end_offset, curr.start_offset)


# get_instructions has its own tests above, so can rely on it to validate
# the object oriented API
class BytecodeTests(InstructionTestCase, DisTestBase):

    def test_instantiation(self):
        # Test with function, method, code string and code object
        for obj in [_f, _C(1).__init__, "a=1", _f.__code__]:
            with self.subTest(obj=obj):
                b = dis.Bytecode(obj)
                self.assertIsInstance(b.codeobj, types.CodeType)

        self.assertRaises(TypeError, dis.Bytecode, object())

    def test_iteration(self):
        for obj in [_f, _C(1).__init__, "a=1", _f.__code__]:
            with self.subTest(obj=obj):
                via_object = list(dis.Bytecode(obj))
                via_generator = list(dis.get_instructions(obj))
                self.assertInstructionsEqual(via_object, via_generator)

    def test_explicit_first_line(self):
        actual = dis.Bytecode(outer, first_line=expected_outer_line)
        self.assertInstructionsEqual(list(actual), expected_opinfo_outer)

    def test_source_line_in_disassembly(self):
        # Use the line in the source code
        actual = dis.Bytecode(simple).dis()
        actual = actual.strip().partition(" ")[0]  # extract the line no
        expected = str(simple.__code__.co_firstlineno)
        self.assertEqual(actual, expected)
        # Use an explicit first line number
        actual = dis.Bytecode(simple, first_line=350).dis()
        actual = actual.strip().partition(" ")[0]  # extract the line no
        self.assertEqual(actual, "350")

    def test_info(self):
        self.maxDiff = 1000
        for x, expected in CodeInfoTests.test_pairs:
            b = dis.Bytecode(x)
            self.assertRegex(b.info(), expected)

    def test_disassembled(self):
        actual = dis.Bytecode(_f).dis()
        self.do_disassembly_compare(actual, dis_f)

    def test_from_traceback(self):
        tb = get_tb()
        b = dis.Bytecode.from_traceback(tb)
        while tb.tb_next: tb = tb.tb_next

        self.assertEqual(b.current_offset, tb.tb_lasti)

    def test_from_traceback_dis(self):
        self.maxDiff = None
        tb = get_tb()
        b = dis.Bytecode.from_traceback(tb)
        self.assertEqual(b.dis(), dis_traceback)

    @requires_debug_ranges()
    def test_bytecode_co_positions(self):
        bytecode = dis.Bytecode("a=1")
        it = bytecode.codeobj.co_positions()
        for instr, positions in zip(bytecode, it, strict=True):
            self.assertEqual(instr.positions, positions)

class TestBytecodeTestCase(BytecodeTestCase):
    def test_assert_not_in_with_op_not_in_bytecode(self):
        code = compile("a = 1", "<string>", "exec")
        self.assertInBytecode(code, "LOAD_SMALL_INT", 1)
        self.assertNotInBytecode(code, "LOAD_NAME")
        self.assertNotInBytecode(code, "LOAD_NAME", "a")

    def test_assert_not_in_with_arg_not_in_bytecode(self):
        code = compile("a = 1", "<string>", "exec")
        self.assertInBytecode(code, "LOAD_SMALL_INT")
        self.assertInBytecode(code, "LOAD_SMALL_INT", 1)
        self.assertNotInBytecode(code, "LOAD_CONST", 2)

    def test_assert_not_in_with_arg_in_bytecode(self):
        code = compile("a = 1", "<string>", "exec")
        with self.assertRaises(AssertionError):
            self.assertNotInBytecode(code, "LOAD_SMALL_INT", 1)

class TestFinderMethods(unittest.TestCase):
    def test__find_imports(self):
        cases = [
            ("import a.b.c", ('a.b.c', 0, None)),
            ("from a.b import c", ('a.b', 0, ('c',))),
            ("from a.b import c as d", ('a.b', 0, ('c',))),
            ("from a.b import *", ('a.b', 0, ('*',))),
            ("from ...a.b import c as d", ('a.b', 3, ('c',))),
            ("from ..a.b import c as d, e as f", ('a.b', 2, ('c', 'e'))),
            ("from ..a.b import *", ('a.b', 2, ('*',))),
        ]
        for src, expected in cases:
            with self.subTest(src=src):
                code = compile(src, "<string>", "exec")
                res = tuple(dis._find_imports(code))
                self.assertEqual(len(res), 1)
                self.assertEqual(res[0], expected)

    def test__find_store_names(self):
        cases = [
            ("x+y", ()),
            ("x=y=1", ('x', 'y')),
            ("x+=y", ('x',)),
            ("global x\nx=y=1", ('x', 'y')),
            ("global x\nz=x", ('z',)),
        ]
        for src, expected in cases:
            with self.subTest(src=src):
                code = compile(src, "<string>", "exec")
                res = tuple(dis._find_store_names(code))
                self.assertEqual(res, expected)

    def test_findlabels(self):
        labels = dis.findlabels(jumpy.__code__.co_code)
        jumps = [
            instr.offset
            for instr in expected_opinfo_jumpy
            if instr.is_jump_target
        ]

        self.assertEqual(sorted(labels), sorted(jumps))

    def test_findlinestarts(self):
        def func():
            pass

        code = func.__code__
        offsets = [linestart[0] for linestart in dis.findlinestarts(code)]
        self.assertEqual(offsets, [0, 1])


class TestDisTraceback(DisTestBase):
    def setUp(self) -> None:
        try:  # We need to clean up existing tracebacks
            del sys.last_exc
        except AttributeError:
            pass
        try:  # We need to clean up existing tracebacks
            del sys.last_traceback
        except AttributeError:
            pass
        return super().setUp()

    def get_disassembly(self, tb):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            dis.distb(tb)
        return output.getvalue()

    def test_distb_empty(self):
        with self.assertRaises(RuntimeError):
            dis.distb()

    def test_distb_last_traceback(self):
        self.maxDiff = None
        # We need to have an existing last traceback in `sys`:
        tb = get_tb()
        sys.last_traceback = tb

        self.do_disassembly_compare(self.get_disassembly(None), dis_traceback)

    def test_distb_explicit_arg(self):
        self.maxDiff = None
        tb = get_tb()

        self.do_disassembly_compare(self.get_disassembly(tb), dis_traceback)


class TestDisTracebackWithFile(TestDisTraceback):
    # Run the `distb` tests again, using the file arg instead of print
    def get_disassembly(self, tb):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            dis.distb(tb, file=output)
        return output.getvalue()

class TestDisCLI(unittest.TestCase):

    def setUp(self):
        self.filename = tempfile.mktemp()
        self.addCleanup(os_helper.unlink, self.filename)

    @staticmethod
    def text_normalize(string):
        """Dedent *string* and strip it from its surrounding whitespaces.

        This method is used by the other utility functions so that any
        string to write or to match against can be freely indented.
        """
        return textwrap.dedent(string).strip()

    def set_source(self, content):
        with open(self.filename, 'w') as fp:
            fp.write(self.text_normalize(content))

    def invoke_dis(self, *flags):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            dis.main(args=[*flags, self.filename])
        return self.text_normalize(output.getvalue())

    def check_output(self, source, expect, *flags):
        with self.subTest(source=source, flags=flags):
            self.set_source(source)
            res = self.invoke_dis(*flags)
            expect = self.text_normalize(expect)
            self.assertListEqual(res.splitlines(), expect.splitlines())

    def test_invocation(self):
        # test various combinations of parameters
        base_flags = [
            ('-C', '--show-caches'),
            ('-O', '--show-offsets'),
            ('-P', '--show-positions'),
            ('-S', '--specialized'),
        ]

        self.set_source('''
            def f():
                print(x)
                return None
        ''')

        for r in range(1, len(base_flags) + 1):
            for choices in itertools.combinations(base_flags, r=r):
                for args in itertools.product(*choices):
                    with self.subTest(args=args[1:]):
                        _ = self.invoke_dis(*args)

        with self.assertRaises(SystemExit):
            # suppress argparse error message
            with contextlib.redirect_stderr(io.StringIO()):
                _ = self.invoke_dis('--unknown')

    def test_show_cache(self):
        # test 'python -m dis -C/--show-caches'
        source = 'print()'
        expect = '''
            0           RESUME                   0

            1           LOAD_NAME                0 (print)  r0 <-
                        PUSH_NULL             r1 <-
                        CALL                     0  r0 <- r0
                        FEEDBACK                 1 (kind: CALL, arg: 0)
                        POP_TOP               <- r0
                        LOAD_COMMON_CONSTANT     7 (None)  r0 <-
                        RETURN_VALUE          <- r0
        '''
        for flag in ['-C', '--show-caches']:
            self.check_output(source, expect, flag)

    def test_show_offsets(self):
        # test 'python -m dis -O/--show-offsets'
        source = 'pass'
        expect = '''
            0          0       RESUME                   0

            1          1       LOAD_COMMON_CONSTANT     7 (None)  r0 <-
                       2       RETURN_VALUE          <- r0
        '''
        for flag in ['-O', '--show-offsets']:
            self.check_output(source, expect, flag)

    def test_show_positions(self):
        # test 'python -m dis -P/--show-positions'
        source = 'pass'
        expect = '''
            0:0-1:0            RESUME                   0

            1:0-1:4            LOAD_COMMON_CONSTANT     7 (None)  r0 <-
            1:0-1:4            RETURN_VALUE          <- r0
        '''
        for flag in ['-P', '--show-positions']:
            self.check_output(source, expect, flag)

    def test_specialized_code(self):
        # test 'python -m dis -S/--specialized'
        source = 'pass'
        expect = '''
            0           RESUME                   0

            1           LOAD_COMMON_CONSTANT     7 (None)  r0 <-
                        RETURN_VALUE          <- r0
        '''
        for flag in ['-S', '--specialized']:
            self.check_output(source, expect, flag)


if __name__ == "__main__":
    unittest.main()
