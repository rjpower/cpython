/* The machine lowerings of the value primitives; see pycore_jit_lower.h.
 *
 * Each lowering is the x86-64 form of one primitive's C definition (the
 * header named in Tools/cases_generator/ir_leaf.py), on the value encoding
 * of Include/internal/pycore_pyvalue.h.  Lib/test/test_jit_template.py runs
 * every lowering against its C definition (_testinternalcapi.leaf_primitive)
 * on generated values and register assignments. */

#include "Python.h"
#include "pycore_ceval.h"           // _PY_EVAL_EVENTS_MASK
#include "pycore_code.h"            // COMPARISON_LESS_THAN
#include "pycore_dict.h"            // PyDictKeysObject
#include "pycore_floatobject.h"     // _PyFloat_ExactDealloc()
#include "pycore_frame.h"           // _PyInterpreterFrame
#include "pycore_jit_lower.h"
#include "pycore_long.h"            // _PyLong_ExactDealloc()
#include "pycore_object.h"
#include "pycore_value.h"

#include <stddef.h>                 // offsetof

#ifdef _PY_JIT_SUPPORTED

#define A (cx->a)
#define REG(i) ((_PyJitReg)ops[i])
#define XMM(i) ((_PyJitXmm)ops[i])
#define AT(B, D) _PyJitMem_At((B), (D))
#define RAX _PyJitReg_RAX
#define RSP _PyJitReg_RSP

static const _PyJitReg abi_gprs[] = {
    _PyJitReg_RDI, _PyJitReg_RSI, _PyJitReg_RDX, _PyJitReg_RCX,
};

/* The cold section: the section current before the outermost ColdBegin is
   restored by its ColdEnd. */
void
_PyJitLower_ColdBegin(_PyJitWordCx *cx)
{
    if (cx->cold_depth++ == 0) {
        cx->cold_saved = A->current;
        _PyJitAsm_Section(A, _PyJitSection_COLD);
    }
}

void
_PyJitLower_ColdEnd(_PyJitWordCx *cx)
{
    assert(cx->cold_depth > 0);
    if (--cx->cold_depth == 0) {
        _PyJitAsm_Section(A, cx->cold_saved);
    }
}

void
_PyJitLower_Publish(_PyJitWordCx *cx, _PyJitReg scratch)
{
    _PyJitAsm_Lea(A, scratch,
                  AT(_PyJit_WORDS, (int32_t)(cx->index * sizeof(_PyIRWord))));
    _PyJitAsm_MovMR(A, AT(_PyJit_FRAME, offsetof(_PyInterpreterFrame, instr_ptr)),
                    scratch);
}

void
_PyJitLower_AluRK(_PyJitWordCx *cx, _PyJitAlu op, _PyJitReg dst, int64_t k,
                  _PyJitReg scratch)
{
    if (k == (int32_t)k) {
        _PyJitAsm_AluRI(A, op, dst, (int32_t)k);
        return;
    }
    _PyJitAsm_MovImm(A, scratch, k);
    _PyJitAsm_AluRR(A, op, dst, scratch);
}

/* The call saves every register it must keep, and every register that an
   argument comes from, in a block on the stack; it loads the arguments from
   there, so the moves need no ordering. */
void
_PyJitLower_Call(_PyJitWordCx *cx, const void *fn, int publish, int nargs,
                 const _PyJitArg *args, _PyJitArg result)
{
    uint32_t gprs = cx->live & _PyJit_GPR_POOL;
    uint32_t xmms = cx->live_xmm & _PyJit_XMM_POOL;
    for (int i = 0; i < nargs; i++) {
        if (args[i].kind == 0 && (_PyJit_GPR_POOL >> args[i].reg) & 1) {
            gprs |= 1u << args[i].reg;
        }
        else if (args[i].kind == 1) {
            xmms |= 1u << args[i].reg;
        }
    }
    assert(result.kind != 0 || !((cx->live >> result.reg) & 1));
    assert(result.kind != 1 || !((cx->live_xmm >> result.reg) & 1));
    int32_t slot[32];
    int32_t size = 0;
    for (int r = 0; r < 16; r++) {
        if ((gprs >> r) & 1) {
            slot[r] = size;
            size += 8;
        }
        if ((xmms >> r) & 1) {
            slot[16 + r] = size;
            size += 8;
        }
    }
    size = (size + 15) & ~15;
    if (size) {
        _PyJitAsm_AluRI(A, _PyJitAlu_SUB, RSP, size);
    }
    for (int r = 0; r < 16; r++) {
        if ((gprs >> r) & 1) {
            _PyJitAsm_MovMR(A, AT(RSP, slot[r]), (_PyJitReg)r);
        }
        if ((xmms >> r) & 1) {
            _PyJitAsm_MovsdMR(A, AT(RSP, slot[16 + r]), (_PyJitXmm)r);
        }
    }
    if (publish) {
        _PyJitLower_Publish(cx, _PyJitReg_R11);
    }
    int ngpr = 0, nxmm = 0;
    for (int i = 0; i < nargs; i++) {
        int r = args[i].reg;
        if (args[i].kind == 0) {
            assert(ngpr < 4);
            _PyJitReg to = abi_gprs[ngpr++];
            if ((gprs >> r) & 1) {
                _PyJitAsm_MovRM(A, to, AT(RSP, slot[r]));
            }
            else {
                _PyJitAsm_MovRR(A, to, (_PyJitReg)r);
            }
        }
        else {
            assert(args[i].kind == 1 && nxmm < 2);
            _PyJitAsm_MovsdRM(A, (_PyJitXmm)nxmm++, AT(RSP, slot[16 + r]));
        }
    }
    _PyJitAsm_CallAbs(A, fn);
    if (result.kind == 0 && result.reg != RAX) {
        _PyJitAsm_MovRR(A, (_PyJitReg)result.reg, RAX);
    }
    else if (result.kind == 1 && result.reg != 0) {
        _PyJitAsm_MovsdRR(A, (_PyJitXmm)result.reg, _PyJitXmm_0);
    }
    for (int r = 0; r < 16; r++) {
        if ((cx->live >> r) & 1) {
            _PyJitAsm_MovRM(A, (_PyJitReg)r, AT(RSP, slot[r]));
        }
        if ((cx->live_xmm >> r) & 1) {
            _PyJitAsm_MovsdRM(A, (_PyJitXmm)r, AT(RSP, slot[16 + r]));
        }
    }
    if (size) {
        _PyJitAsm_AluRI(A, _PyJitAlu_ADD, RSP, size);
    }
}

/* A call on a cold path that comes back: the lowering's slow path.  If
   fail is not negative, a call that gives NULL leaves for that label
   instead of coming back. */
static void
cold_call(_PyJitWordCx *cx, _PyJitLabel from, _PyJitLabel back, const void *fn,
          int publish, int nargs, const _PyJitArg *args, _PyJitArg result,
          _PyJitLabel fail)
{
    _PyJitLower_ColdBegin(cx);
    _PyJitAsm_Bind(A, from);
    _PyJitLower_Call(cx, fn, publish, nargs, args, result);
    if (fail >= 0) {
        _PyJitAsm_TestRR(A, (_PyJitReg)result.reg, (_PyJitReg)result.reg);
        _PyJitAsm_Jcc(A, _PyJitCond_E, fail);
    }
    _PyJitAsm_Jmp(A, back);
    _PyJitLower_ColdEnd(cx);
}

/* The label that a lowering with an exit leaves for (a constructor's NULL
   result, a guard that fails): aux is that label plus one, or NULL for none */
static _PyJitLabel
exit_label(const void *aux)
{
    return (_PyJitLabel)(intptr_t)aux - 1;
}

/* Shared shapes */

/* reg := imm64 + src, where reg may be src */
static void
add_k(_PyJitWordCx *cx, _PyJitReg dst, _PyJitReg src, int64_t k)
{
    if (k == (int32_t)k) {
        _PyJitAsm_Lea(A, dst, AT(src, (int32_t)k));
        return;
    }
    _PyJitAsm_MovImm(A, dst, k);
    _PyJitAsm_AluRR(A, _PyJitAlu_ADD, dst, src);
}

/* t := w >> shift (logical); the flags test the result */
static void
top_bits(_PyJitWordCx *cx, _PyJitReg t, _PyJitReg w, int shift)
{
    _PyJitAsm_MovRR(A, t, w);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHR, t, shift);
}

/* Flags for "w is an inline int", through t: E */
static _PyJitCond
inline_int_test(_PyJitWordCx *cx, _PyJitReg t, _PyJitReg w)
{
    _PyJitAsm_MovRR(A, t, w);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SAR, t, 64 - Py_INT_SHIFT);
    _PyJitAsm_AluRI(A, _PyJitAlu_CMP, t, -1);
    return _PyJitCond_E;
}

/* t := w - Py_DOUBLE_OFFSET, then flags for "w is an inline float", with k
   holding Py_INT_BASE - Py_DOUBLE_OFFSET: B */
static _PyJitCond
inline_float_test(_PyJitWordCx *cx, _PyJitReg t, _PyJitReg w, _PyJitReg k)
{
    add_k(cx, t, w, -(int64_t)Py_DOUBLE_OFFSET);
    _PyJitAsm_MovImm(A, k, (int64_t)(Py_INT_BASE - Py_DOUBLE_OFFSET));
    _PyJitAsm_AluRR(A, _PyJitAlu_CMP, t, k);
    return _PyJitCond_B;
}

/* The flags of a predicate computed with branches: true and false paths
   end at done with ZF set for true.  Returns E.  A predicate that has an
   exit jumps to it where it is false instead, and gives no flags. */
static _PyJitCond
join_bool(_PyJitWordCx *cx, _PyJitReg t, _PyJitLabel is_false, _PyJitLabel done)
{
    _PyJitAsm_Jmp(A, done);
    _PyJitAsm_Bind(A, is_false);
    _PyJitAsm_MovImm(A, t, 1);
    _PyJitAsm_TestRR(A, t, t);
    _PyJitAsm_Bind(A, done);
    return _PyJitCond_E;
}

static void
move(_PyJitWordCx *cx, _PyJitReg dst, _PyJitReg src)
{
    if (dst != src) {
        _PyJitAsm_MovRR(A, dst, src);
    }
}

/* t := the object of w, then flags: its type is tp (E), through k */
static _PyJitCond
type_is(_PyJitWordCx *cx, _PyJitReg t, _PyJitReg w, PyTypeObject *tp, _PyJitReg k)
{
    _PyJitAsm_MovRR(A, t, w);
    _PyJitAsm_AluRI(A, _PyJitAlu_AND, t, ~(int32_t)Py_TAG_REFCNT);
    _PyJitAsm_MovRM(A, t, AT(t, offsetof(PyObject, ob_type)));
    _PyJitAsm_MovImm(A, k, (int64_t)(uintptr_t)tp);
    _PyJitAsm_AluRR(A, _PyJitAlu_CMP, t, k);
    return _PyJitCond_E;
}

#define LOWERING(NAME) \
    int _PyJitLower_##NAME(_PyJitWordCx *cx, const uint8_t *ops, const void *aux)

/* Tag tests: [w, t] unless noted */

LOWERING(PyValue_IsNull)                        // [w]
{
    _PyJitAsm_TestRR(A, REG(0), REG(0));
    return _PyJitCond_E;
}

LOWERING(PyValue_IsPointerSpace)
{
    top_bits(cx, REG(1), REG(0), 48);
    return _PyJitCond_E;
}

LOWERING(PyValue_IsObject)
{
    _PyJitReg w = REG(0), t = REG(1);
    _PyJitLabel exit = exit_label(aux);
    _PyJitLabel is_false = exit >= 0 ? exit : _PyJitAsm_NewLabel(A);
    top_bits(cx, t, w, 48);
    _PyJitAsm_Jcc(A, _PyJitCond_NE, is_false);
    _PyJitAsm_AluRI(A, _PyJitAlu_CMP, w, 2);
    _PyJitAsm_Jcc(A, _PyJitCond_BE, is_false);
    _PyJitAsm_TestRI(A, w, Py_TAG_INVALID);
    if (exit >= 0) {
        _PyJitAsm_Jcc(A, _PyJitCond_NE, exit);
        return _PyJitCond_NONE;
    }
    return join_bool(cx, t, is_false, _PyJitAsm_NewLabel(A));
}

LOWERING(PyValue_IsInline)
{
    top_bits(cx, REG(1), REG(0), 49);
    return _PyJitCond_NE;
}

LOWERING(PyValue_IsInlineInt)
{
    return inline_int_test(cx, REG(1), REG(0));
}

LOWERING(PyValue_IsInlineFloat)                 // [w, t, k]
{
    return inline_float_test(cx, REG(1), REG(0), REG(2));
}

LOWERING(PyValue_BothInlineInt)                 // [a, b, t]
{
    _PyJitAsm_MovRR(A, REG(2), REG(0));
    _PyJitAsm_AluRR(A, _PyJitAlu_AND, REG(2), REG(1));
    _PyJitAsm_ShiftRI(A, _PyJitShift_SAR, REG(2), 64 - Py_INT_SHIFT);
    _PyJitAsm_AluRI(A, _PyJitAlu_CMP, REG(2), -1);
    return _PyJitCond_E;
}

LOWERING(PyValue_IsTaggedIndex)
{
    add_k(cx, REG(1), REG(0), -(int64_t)Py_INDEX_BASE);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHR, REG(1), 48);
    return _PyJitCond_E;
}

LOWERING(PyValue_Is)                            // [a, b, t]
{
    _PyJitReg a = REG(0), b = REG(1), t = REG(2);
    _PyJitLabel exact = _PyJitAsm_NewLabel(A), done = _PyJitAsm_NewLabel(A);
    top_bits(cx, t, a, 48);
    _PyJitAsm_Jcc(A, _PyJitCond_NE, exact);
    /* In pointer space bit 0 is the no-count tag */
    _PyJitAsm_MovRR(A, t, a);
    _PyJitAsm_AluRR(A, _PyJitAlu_XOR, t, b);
    _PyJitAsm_AluRI(A, _PyJitAlu_AND, t, ~(int32_t)Py_TAG_REFCNT);
    _PyJitAsm_Jmp(A, done);
    _PyJitAsm_Bind(A, exact);
    _PyJitAsm_AluRR(A, _PyJitAlu_CMP, a, b);
    _PyJitAsm_Bind(A, done);
    return _PyJitCond_E;
}

LOWERING(_PyValue_NamesObject)                  // [w, p, t]
{
    _PyJitAsm_MovRR(A, REG(2), REG(0));
    _PyJitAsm_AluRI(A, _PyJitAlu_AND, REG(2), ~(int32_t)Py_TAG_REFCNT);
    _PyJitAsm_AluRR(A, _PyJitAlu_CMP, REG(2), REG(1));
    return _PyJitCond_E;
}

static _PyJitCond
is_word(_PyJitWordCx *cx, _PyJitReg w, _PyJitReg t, PyValue v)
{
    _PyJitAsm_MovImm(A, t, (int64_t)v.bits);
    _PyJitAsm_AluRR(A, _PyJitAlu_CMP, w, t);
    return _PyJitCond_E;
}

LOWERING(PyValue_IsTrue) { return is_word(cx, REG(0), REG(1), PyValue_True); }
LOWERING(PyValue_IsFalse) { return is_word(cx, REG(0), REG(1), PyValue_False); }
LOWERING(PyValue_IsNone) { return is_word(cx, REG(0), REG(1), PyValue_None); }

LOWERING(PyValue_BoolCheck)                     // [w, t, k]
{
    _PyJitLabel exit = exit_label(aux);
    _PyJitLabel is_false = exit >= 0 ? exit : _PyJitAsm_NewLabel(A);
    top_bits(cx, REG(1), REG(0), 48);
    _PyJitAsm_Jcc(A, _PyJitCond_NE, is_false);
    type_is(cx, REG(1), REG(0), &PyBool_Type, REG(2));
    if (exit >= 0) {
        _PyJitAsm_Jcc(A, _PyJitCond_NE, exit);
        return _PyJitCond_NONE;
    }
    return join_bool(cx, REG(1), is_false, _PyJitAsm_NewLabel(A));
}

LOWERING(PyValue_LongCheckExact)                // [w, t, k]
{
    _PyJitLabel exit = exit_label(aux);
    _PyJitLabel is_false = exit >= 0 ? exit : _PyJitAsm_NewLabel(A);
    _PyJitLabel done = _PyJitAsm_NewLabel(A);
    inline_int_test(cx, REG(1), REG(0));
    _PyJitAsm_Jcc(A, _PyJitCond_E, done);
    top_bits(cx, REG(1), REG(0), 48);
    _PyJitAsm_Jcc(A, _PyJitCond_NE, is_false);
    type_is(cx, REG(1), REG(0), &PyLong_Type, REG(2));
    if (exit >= 0) {
        _PyJitAsm_Jcc(A, _PyJitCond_NE, exit);
        _PyJitAsm_Bind(A, done);
        return _PyJitCond_NONE;
    }
    return join_bool(cx, REG(1), is_false, done);
}

/* Inline numbers: [dst, x] or [dst, x, t] */

LOWERING(_PyValue_IntScaled)
{
    move(cx, REG(0), REG(1));
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHL, REG(0), Py_INT_SHIFT);
    return _PyJitCond_NONE;
}

LOWERING(_PyValue_FromIntScaled)
{
    _PyJitAsm_MovImm(A, REG(2), (int64_t)Py_INT_BASE);
    move(cx, REG(0), REG(1));
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHR, REG(0), Py_INT_SHIFT);
    _PyJitAsm_AluRR(A, _PyJitAlu_OR, REG(0), REG(2));
    return _PyJitCond_NONE;
}

LOWERING(PyValue_InlineInt64)
{
    move(cx, REG(0), REG(1));
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHL, REG(0), Py_INT_SHIFT);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SAR, REG(0), Py_INT_SHIFT);
    return _PyJitCond_NONE;
}

/* An int in the inline range has its top Py_INT_SHIFT + 1 bits equal, so
   the region bits or into it: base | (i & ~base) is base | i. */
LOWERING(PyValue_FromInlineInt)
{
    _PyJitAsm_MovImm(A, REG(2), (int64_t)Py_INT_BASE);
    _PyJitAsm_AluRR(A, _PyJitAlu_OR, REG(2), REG(1));
    move(cx, REG(0), REG(2));
    return _PyJitCond_NONE;
}

LOWERING(PyValue_InlineFloat)                   // [xdst, w, t]
{
    add_k(cx, REG(2), REG(1), -(int64_t)Py_DOUBLE_OFFSET);
    _PyJitAsm_MovqXR(A, XMM(0), REG(2));
    return _PyJitCond_NONE;
}

LOWERING(PyValue_FromDoubleNotNaN)              // [dst, x, t]
{
    _PyJitAsm_MovImm(A, REG(2), (int64_t)Py_DOUBLE_OFFSET);
    _PyJitAsm_MovqRX(A, REG(0), XMM(1));
    _PyJitAsm_AluRR(A, _PyJitAlu_ADD, REG(0), REG(2));
    return _PyJitCond_NONE;
}

/* t := the inline int w as a double in x */
static void
int_to_double(_PyJitWordCx *cx, _PyJitXmm x, _PyJitReg w, _PyJitReg t)
{
    _PyJitAsm_MovRR(A, t, w);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHL, t, Py_INT_SHIFT);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SAR, t, Py_INT_SHIFT);
    _PyJitAsm_Cvtsi2sd(A, x, t);
}

/* [xa, xb, a, b, t, k]: true if a and b are two inline floats, or an
   inline float and an inline int; then xa and xb hold them as doubles. */
LOWERING(PyValue_FloatOperandValues)
{
    _PyJitXmm xa = XMM(0), xb = XMM(1);
    _PyJitReg a = REG(2), b = REG(3), t = REG(4), k = REG(5);
    _PyJitLabel a_int = _PyJitAsm_NewLabel(A), b_int = _PyJitAsm_NewLabel(A);
    _PyJitLabel exit = exit_label(aux);
    _PyJitLabel is_false = exit >= 0 ? exit : _PyJitAsm_NewLabel(A);
    _PyJitLabel is_true = _PyJitAsm_NewLabel(A);
    _PyJitAsm_Jcc(A, _PyJitCond_Not(inline_float_test(cx, t, a, k)), a_int);
    _PyJitAsm_MovqXR(A, xa, t);
    /* k still holds the float range */
    add_k(cx, t, b, -(int64_t)Py_DOUBLE_OFFSET);
    _PyJitAsm_AluRR(A, _PyJitAlu_CMP, t, k);
    _PyJitAsm_Jcc(A, _PyJitCond_AE, b_int);
    _PyJitAsm_MovqXR(A, xb, t);
    _PyJitAsm_Jmp(A, is_true);

    _PyJitAsm_Bind(A, b_int);
    _PyJitAsm_Jcc(A, _PyJitCond_Not(inline_int_test(cx, t, b)), is_false);
    int_to_double(cx, xb, b, t);
    _PyJitAsm_Jmp(A, is_true);

    _PyJitAsm_Bind(A, a_int);
    _PyJitAsm_Jcc(A, _PyJitCond_Not(inline_int_test(cx, t, a)), is_false);
    _PyJitAsm_Jcc(A, _PyJitCond_Not(inline_float_test(cx, t, b, k)), is_false);
    _PyJitAsm_MovqXR(A, xb, t);
    int_to_double(cx, xa, a, t);

    _PyJitAsm_Bind(A, is_true);
    if (exit >= 0) {
        return _PyJitCond_NONE;
    }
    _PyJitAsm_AluRR(A, _PyJitAlu_XOR, t, t);
    return join_bool(cx, t, is_false, _PyJitAsm_NewLabel(A));
}

LOWERING(PyValue_FromInt64)                     // [dst, i, t]
{
    _PyJitReg dst = REG(0), i = REG(1), t = REG(2);
    _PyJitLabel box = _PyJitAsm_NewLabel(A), done = _PyJitAsm_NewLabel(A);
    _PyJitAsm_MovRR(A, t, i);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHL, t, Py_INT_SHIFT);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SAR, t, Py_INT_SHIFT);
    _PyJitAsm_AluRR(A, _PyJitAlu_CMP, t, i);
    _PyJitAsm_Jcc(A, _PyJitCond_NE, box);
    _PyJitAsm_MovImm(A, t, (int64_t)Py_INT_BASE);
    _PyJitAsm_AluRR(A, _PyJitAlu_OR, t, i);
    _PyJitAsm_MovRR(A, dst, t);
    _PyJitAsm_Bind(A, done);
    _PyJitArg arg = _PyJitArg_GPR(i);
    cold_call(cx, box, done, (const void *)_PyValue_BoxInt64, 1, 1, &arg,
              _PyJitArg_GPR(dst), exit_label(aux));
    return _PyJitCond_NONE;
}

LOWERING(PyValue_FromDouble)                    // [dst, x, t]
{
    _PyJitReg dst = REG(0), t = REG(2);
    _PyJitXmm x = XMM(1);
    _PyJitLabel box = _PyJitAsm_NewLabel(A), done = _PyJitAsm_NewLabel(A);
    _PyJitAsm_SseRR(A, _PyJitSse_UCOMISD, x, x);
    _PyJitAsm_Jcc(A, _PyJitCond_P, box);
    _PyJitAsm_MovImm(A, t, (int64_t)Py_DOUBLE_OFFSET);
    _PyJitAsm_MovqRX(A, dst, x);
    _PyJitAsm_AluRR(A, _PyJitAlu_ADD, dst, t);
    _PyJitAsm_Bind(A, done);
    _PyJitArg arg = _PyJitArg_XMM(x);
    cold_call(cx, box, done, (const void *)_PyValue_BoxDouble, 1, 1, &arg,
              _PyJitArg_GPR(dst), exit_label(aux));
    return _PyJitCond_NONE;
}

LOWERING(PyValue_TagIndex)                      // [dst, i, t]
{
    _PyJitAsm_MovImm(A, REG(2), (int64_t)Py_INDEX_BASE);
    move(cx, REG(0), REG(1));
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHL, REG(0), 16);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHR, REG(0), 16);
    _PyJitAsm_AluRR(A, _PyJitAlu_OR, REG(0), REG(2));
    return _PyJitCond_NONE;
}

LOWERING(PyValue_UntagIndex)                    // [dst, w]
{
    move(cx, REG(0), REG(1));
    _PyJitAsm_ShiftRI(A, _PyJitShift_SHL, REG(0), 16);
    _PyJitAsm_ShiftRI(A, _PyJitShift_SAR, REG(0), 16);
    return _PyJitCond_NONE;
}

LOWERING(PyValue_IncrementTaggedIndexNoOverflow) // [dst, w]
{
    _PyJitAsm_Lea(A, REG(0), AT(REG(1), 1));
    return _PyJitCond_NONE;
}

/* Objects of words.  The C definition of PyValue_AsKnownObjectBorrow()
   checks that the word is in pointer space; a template reaches it only
   after a guard that proved it, as the generator's IR_GUARD_* expansions
   do, so the lowering does not repeat the test. */

LOWERING(PyValue_AsKnownObjectBorrow)           // [dst, w]
{
    move(cx, REG(0), REG(1));
    _PyJitAsm_AluRI(A, _PyJitAlu_AND, REG(0), ~(int32_t)Py_TAG_REFCNT);
    return _PyJitCond_NONE;
}

LOWERING(_PyValue_AsKnownObjectBorrowPointer)   // [dst, w]
{
    move(cx, REG(0), REG(1));
    _PyJitAsm_AluRI(A, _PyJitAlu_AND, REG(0), ~(int32_t)Py_TAG_REFCNT);
    return _PyJitCond_NONE;
}

LOWERING(_PyValueGuarded_Load)                  // [dst, w]
{
    move(cx, REG(0), REG(1));
    return _PyJitCond_NONE;
}

LOWERING(_Py_EvalBreakerHasEvents)              // []
{
    _PyJitAsm_TestMI(A, AT(_PyJit_TSTATE, offsetof(PyThreadState, eval_breaker)),
                     _PY_EVAL_EVENTS_MASK);
    return _PyJitCond_NE;
}

/* Counts.  A word owns a count when it is not NULL and none of the bits of
   Py_OWNED_MASK is set.  In a build with Py_REF_DEBUG every count goes
   through its C definition, which keeps the reference totals. */

/* A release whose slow path may run Python, coming back to done on the hot
   path; returns the label its slow path goes on to (cx->release_tail), or
   done when the template has more releases than the tails record. */
static _PyJitLabel
note_release(_PyJitWordCx *cx, _PyJitLabel done)
{
    int k = cx->nreleases++;
    if (cx->cold_depth == 0) {
        cx->release_end = A->sections[_PyJitSection_HOT].size;
    }
    else {
        /* Not on the hot path: never the template's last code */
        cx->release_end = SIZE_MAX;
    }
    if (k >= _PyJit_MAX_RELEASES) {
        return done;
    }
    cx->release_tail[k] = _PyJitAsm_NewLabel(A);
    cx->release_back[k] = done;
    return cx->release_tail[k];
}

#ifdef Py_REF_DEBUG
static PyValue ref_dup(PyValue v) { return PyValue_DUP(v); }
static PyValue ref_xdup(PyValue v) { return PyValue_XDUP(v); }
static void ref_close(PyValue v) { PyValue_CLOSE(v); }
static void ref_xclose(PyValue v) { PyValue_XCLOSE(v); }
static void ref_close_long(PyValue v) { PyValue_CLOSE_SPECIALIZED(v, _PyLong_ExactDealloc); }
static void ref_close_float(PyValue v) { PyValue_CLOSE_SPECIALIZED(v, _PyFloat_ExactDealloc); }

static void
ref_call(_PyJitWordCx *cx, const void *fn, int publish, _PyJitReg w, _PyJitArg result)
{
    _PyJitArg arg = _PyJitArg_GPR(w);
    _PyJitLower_Call(cx, fn, publish, 1, &arg, result);
    if (fn == (const void *)ref_close || fn == (const void *)ref_xclose) {
        /* A release with no slow path of its own: no tail */
        cx->nreleases = _PyJit_MAX_RELEASES + 1;
    }
}
#else
/* t := the owned bits of w: E when w owns a count (and is not NULL) */
static _PyJitCond
owns_test(_PyJitWordCx *cx, _PyJitReg w, _PyJitReg t)
{
    _PyJitAsm_MovImm(A, t, (int64_t)Py_OWNED_MASK);
    _PyJitAsm_TestRR(A, w, t);
    return _PyJitCond_E;
}

/* ob_refcnt is the low 32 bits of the count word; the flags above it stay.
   The flags of the result: E when the count is now zero. */
static void
refcnt_step(_PyJitWordCx *cx, _PyJitReg w, _PyJitUnary op)
{
    _PyJitAsm_UnaryM32(A, op, AT(w, offsetof(PyObject, ob_refcnt)));
}

static void
dup_value(_PyJitWordCx *cx, _PyJitReg dst, _PyJitReg w, _PyJitReg t, int may_be_null)
{
    _PyJitLabel done = _PyJitAsm_NewLabel(A);
    if (may_be_null) {
        _PyJitAsm_TestRR(A, w, w);
        _PyJitAsm_Jcc(A, _PyJitCond_E, done);
    }
    _PyJitAsm_Jcc(A, _PyJitCond_Not(owns_test(cx, w, t)), done);
    refcnt_step(cx, w, _PyJitUnary_INC);
    _PyJitAsm_Bind(A, done);
    move(cx, dst, w);
}

/* The release of w's count; the last one calls destroy(w) or, when destroy
   is NULL, _Py_Dealloc(w), which may run Python. */
static void
close_value(_PyJitWordCx *cx, _PyJitReg w, _PyJitReg t, int may_be_null,
            const void *destroy)
{
    _PyJitLabel done = _PyJitAsm_NewLabel(A), last = _PyJitAsm_NewLabel(A);
    if (may_be_null) {
        _PyJitAsm_TestRR(A, w, w);
        _PyJitAsm_Jcc(A, _PyJitCond_E, done);
    }
    _PyJitAsm_Jcc(A, _PyJitCond_Not(owns_test(cx, w, t)), done);
    refcnt_step(cx, w, _PyJitUnary_DEC);
    _PyJitAsm_Jcc(A, _PyJitCond_E, last);
    _PyJitAsm_Bind(A, done);
    _PyJitArg arg = _PyJitArg_GPR(w);
    _PyJitLabel back = done;
    if (destroy == NULL) {
        /* _Py_Dealloc may run Python: the compiler decides what follows */
        back = note_release(cx, done);
    }
    cold_call(cx, last, back, destroy ? destroy : (const void *)_Py_Dealloc,
              destroy == NULL, 1, &arg, _PyJitArg_NONE, -1);
}

/* The last count of an exact int or float: its destructor, which runs no
   Python, after the reference tracer as Py_DECREF_MORTAL_SPECIALIZED() */
static void
destroy_long(PyObject *op)
{
    _PyReftracerTrack(op, PyRefTracer_DESTROY);
    _PyLong_ExactDealloc(op);
}

static void
destroy_float(PyObject *op)
{
    _PyReftracerTrack(op, PyRefTracer_DESTROY);
    _PyFloat_ExactDealloc(op);
}
#endif

LOWERING(PyValue_DUP)                           // [dst, w, t]
{
#ifdef Py_REF_DEBUG
    ref_call(cx, (const void *)ref_dup, 0, REG(1), _PyJitArg_GPR(REG(0)));
#else
    dup_value(cx, REG(0), REG(1), REG(2), 0);
#endif
    return _PyJitCond_NONE;
}

LOWERING(PyValue_XDUP)                          // [dst, w, t]
{
#ifdef Py_REF_DEBUG
    ref_call(cx, (const void *)ref_xdup, 0, REG(1), _PyJitArg_GPR(REG(0)));
#else
    dup_value(cx, REG(0), REG(1), REG(2), 1);
#endif
    return _PyJitCond_NONE;
}

LOWERING(PyValue_CLOSE)                         // [w, t]
{
#ifdef Py_REF_DEBUG
    ref_call(cx, (const void *)ref_close, 1, REG(0), _PyJitArg_NONE);
#else
    close_value(cx, REG(0), REG(1), 0, NULL);
#endif
    return _PyJitCond_NONE;
}

LOWERING(PyValue_XCLOSE)                        // [w, t]
{
#ifdef Py_REF_DEBUG
    ref_call(cx, (const void *)ref_xclose, 1, REG(0), _PyJitArg_NONE);
#else
    close_value(cx, REG(0), REG(1), 1, NULL);
#endif
    return _PyJitCond_NONE;
}

/* [w, t]; aux is the destructor: _PyLong_ExactDealloc or
   _PyFloat_ExactDealloc, the two that leaf uops name */
LOWERING(PyValue_CLOSE_SPECIALIZED)
{
    int is_long = aux == (const void *)_PyLong_ExactDealloc;
    assert(is_long || aux == (const void *)_PyFloat_ExactDealloc);
#ifdef Py_REF_DEBUG
    ref_call(cx, is_long ? (const void *)ref_close_long : (const void *)ref_close_float,
             0, REG(0), _PyJitArg_NONE);
#else
    close_value(cx, REG(0), REG(1), 0,
                is_long ? (const void *)destroy_long : (const void *)destroy_float);
#endif
    return _PyJitCond_NONE;
}

/* Objects and types.  Operands are in the order of pycore_jit_lower.h; a
   pointer is the object's address.  The primitives that the generator
   reaches only through a guard (the C definitions assert it) do not repeat
   the test. */

static_assert(sizeof(PyValue) == 8, "the element scale of the lists and tuples");

LOWERING(Py_TYPE)                               // [dst, o]
{
    _PyJitAsm_MovRM(A, REG(0), AT(REG(1), offsetof(PyObject, ob_type)));
    return _PyJitCond_NONE;
}

/* Flags: the type of o is tp (E), through t */
static _PyJitCond
type_is_exactly(_PyJitWordCx *cx, _PyJitReg o, _PyJitReg t, PyTypeObject *tp)
{
    _PyJitAsm_MovImm(A, t, (int64_t)(uintptr_t)tp);
    _PyJitAsm_AluMR(A, _PyJitAlu_CMP, AT(o, offsetof(PyObject, ob_type)), t);
    return _PyJitCond_E;
}

#define CHECK_EXACT(NAME, TYPE) \
    LOWERING(NAME)                                      /* [o, t] */ \
    { \
        return type_is_exactly(cx, REG(0), REG(1), &TYPE); \
    }

CHECK_EXACT(PyList_CheckExact, PyList_Type)
CHECK_EXACT(PyTuple_CheckExact, PyTuple_Type)
CHECK_EXACT(PyDict_CheckExact, PyDict_Type)
CHECK_EXACT(PyModule_CheckExact, PyModule_Type)
CHECK_EXACT(PyUnicode_CheckExact, PyUnicode_Type)
CHECK_EXACT(PyFunction_Check, PyFunction_Type)

LOWERING(PyList_GET_SIZE)                       // [dst, o]
{
    _PyJitAsm_MovRM(A, REG(0), AT(REG(1), offsetof(PyVarObject, ob_size)));
    return _PyJitCond_NONE;
}

LOWERING(PyTuple_GET_SIZE)                      // [dst, o]
{
    _PyJitAsm_MovRM(A, REG(0), AT(REG(1), offsetof(PyVarObject, ob_size)));
    return _PyJitCond_NONE;
}

LOWERING(_PyList_ITEMS)                         // [dst, o]
{
    _PyJitAsm_MovRM(A, REG(0), AT(REG(1), offsetof(PyListObject, ob_item)));
    return _PyJitCond_NONE;
}

LOWERING(_PyTuple_ITEMS)                        // [dst, o]
{
    _PyJitAsm_Lea(A, REG(0), AT(REG(1), offsetof(PyTupleObject, ob_item)));
    return _PyJitCond_NONE;
}

/* The items array is loaded before the index is used, and the result may
   share the register of either argument, so the array goes through t. */
LOWERING(PyList_GET_VALUE)                      // [dst, o, i, t]
{
    _PyJitAsm_MovRM(A, REG(3), AT(REG(1), offsetof(PyListObject, ob_item)));
    _PyJitAsm_MovRM(A, REG(0), _PyJitMem_Indexed(REG(3), REG(2), 3, 0));
    return _PyJitCond_NONE;
}

LOWERING(PyTuple_GET_VALUE)                     // [dst, o, i]
{
    _PyJitAsm_MovRM(A, REG(0), _PyJitMem_Indexed(REG(1), REG(2), 3,
                                                 offsetof(PyTupleObject, ob_item)));
    return _PyJitCond_NONE;
}

/* t := the offset of o's inline values from o: its type's basic size */
static void
inline_values_offset(_PyJitWordCx *cx, _PyJitReg o, _PyJitReg t)
{
    _PyJitAsm_MovRM(A, t, AT(o, offsetof(PyObject, ob_type)));
    _PyJitAsm_MovRM(A, t, AT(t, offsetof(PyTypeObject, tp_basicsize)));
}

LOWERING(_PyObject_InlineValues)                // [dst, o, t]
{
    inline_values_offset(cx, REG(1), REG(2));
    _PyJitAsm_Lea(A, REG(0), _PyJitMem_Indexed(REG(1), REG(2), 0, 0));
    return _PyJitCond_NONE;
}

LOWERING(_PyObject_Shape)                       // [dst, o, t]
{
    inline_values_offset(cx, REG(1), REG(2));
    _PyJitAsm_MovzxRM8(A, REG(0), _PyJitMem_Indexed(REG(1), REG(2), 0,
                                                    offsetof(PyDictValues, shape)));
    return _PyJitCond_NONE;
}

LOWERING(_PyObject_ValueSlot)                   // [dst, o, offset]
{
    _PyJitAsm_Lea(A, REG(0), _PyJitMem_Indexed(REG(1), REG(2), 0, 0));
    return _PyJitCond_NONE;
}

/* The slot is filled (the type and shape guards prove it), so the count is
   PyValue_DUP()'s, without the test for NULL. */
LOWERING(_PyObject_ShapeReadDup)                // [dst, o, offset, t]
{
    _PyJitReg dst = REG(0);
    _PyJitAsm_MovRM(A, dst, _PyJitMem_Indexed(REG(1), REG(2), 0, 0));
#ifdef Py_REF_DEBUG
    ref_call(cx, (const void *)ref_dup, 0, dst, _PyJitArg_GPR(dst));
#else
    dup_value(cx, dst, dst, REG(3), 0);
#endif
    return _PyJitCond_NONE;
}

/* 1 << n for the index sizes of a dict's keys: the shift count would have
   to be in cl, which a lowering may not take. */
static const uint64_t pow2[64] = {
    0x1, 0x2, 0x4, 0x8,
    0x10, 0x20, 0x40, 0x80,
    0x100, 0x200, 0x400, 0x800,
    0x1000, 0x2000, 0x4000, 0x8000,
    0x10000, 0x20000, 0x40000, 0x80000,
    0x100000, 0x200000, 0x400000, 0x800000,
    0x1000000, 0x2000000, 0x4000000, 0x8000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000,
    0x100000000, 0x200000000, 0x400000000, 0x800000000,
    0x1000000000, 0x2000000000, 0x4000000000, 0x8000000000,
    0x10000000000, 0x20000000000, 0x40000000000, 0x80000000000,
    0x100000000000, 0x200000000000, 0x400000000000, 0x800000000000,
    0x1000000000000, 0x2000000000000, 0x4000000000000, 0x8000000000000,
    0x10000000000000, 0x20000000000000, 0x40000000000000, 0x80000000000000,
    0x100000000000000, 0x200000000000000, 0x400000000000000, 0x800000000000000,
    0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000,
};

LOWERING(DK_UNICODE_ENTRIES)                    // [dst, keys, t, k]
{
    _PyJitReg keys = REG(1), t = REG(2), k = REG(3);
    _PyJitAsm_MovzxRM8(A, k, AT(keys, offsetof(PyDictKeysObject, dk_log2_index_bytes)));
    _PyJitAsm_MovImm(A, t, (int64_t)(uintptr_t)pow2);
    _PyJitAsm_MovRM(A, t, _PyJitMem_Indexed(t, k, 3, 0));
    _PyJitAsm_Lea(A, REG(0), _PyJitMem_Indexed(keys, t, 0,
                                               offsetof(PyDictKeysObject, dk_indices)));
    return _PyJitCond_NONE;
}

/* The u8 fields of a dict's values: the insertion order array follows the
   values, capacity of them. */

LOWERING(_PyDictValues_SetShape)                // [values, shape]
{
    _PyJitAsm_MovMR8(A, AT(REG(0), offsetof(PyDictValues, shape)), REG(1));
    return _PyJitCond_NONE;
}

LOWERING(_PyDictValues_AddToInsertionOrder)     // [values, ix, array, size]
{
    _PyJitReg values = REG(0), ix = REG(1), array = REG(2), size = REG(3);
    _PyJitAsm_MovzxRM8(A, array, AT(values, offsetof(PyDictValues, capacity)));
    _PyJitAsm_Lea(A, array, _PyJitMem_Indexed(values, array, 3,
                                              offsetof(PyDictValues, values)));
    _PyJitAsm_MovzxRM8(A, size, AT(values, offsetof(PyDictValues, size)));
    _PyJitAsm_MovMR8(A, _PyJitMem_Indexed(array, size, 0, 0), ix);
    _PyJitAsm_Lea(A, size, AT(size, 1));
    _PyJitAsm_MovMR8(A, AT(values, offsetof(PyDictValues, size)), size);
    return _PyJitCond_NONE;
}

LOWERING(_PyLong_IsZero)                        // [o, t]
{
    _PyJitReg t = REG(1);
    _PyJitAsm_MovRM(A, t, AT(REG(0), offsetof(PyLongObject, long_value.lv_tag)));
    _PyJitAsm_AluRI(A, _PyJitAlu_AND, t, SIGN_MASK);
    _PyJitAsm_AluRI(A, _PyJitAlu_CMP, t, SIGN_ZERO);
    return _PyJitCond_E;
}

/* The frame, in r13; its slots, in r15 (pycore_jit_lower.h) */

LOWERING(GLOBALS)                               // [dst]
{
    _PyJitAsm_MovRM(A, REG(0), AT(_PyJit_FRAME, offsetof(_PyInterpreterFrame, f_globals)));
    return _PyJitCond_NONE;
}

LOWERING(BUILTINS)                              // [dst]
{
    _PyJitAsm_MovRM(A, REG(0), AT(_PyJit_FRAME, offsetof(_PyInterpreterFrame, f_builtins)));
    return _PyJitCond_NONE;
}

LOWERING(FRAME_CO_CONSTS)                       // [dst]
{
    _PyJitReg dst = REG(0);
    _PyJitAsm_MovRM(A, dst, AT(_PyJit_FRAME, offsetof(_PyInterpreterFrame, f_executable)));
    _PyJitAsm_MovRM(A, dst, AT(dst, offsetof(PyCodeObject, co_consts)));
    return _PyJitCond_NONE;
}

LOWERING(IR_SLOTS)                              // [dst]
{
    _PyJitAsm_MovRR(A, REG(0), _PyJit_BASE);
    return _PyJitCond_NONE;
}

/* The index is an i32 held sign-extended in its register */
LOWERING(GETLOCAL)                              // [dst, i]
{
    _PyJitAsm_MovRM(A, REG(0), _PyJitMem_Indexed(_PyJit_BASE, REG(1), 3, 0));
    return _PyJitCond_NONE;
}

/* COMPARISON_BIT(x, y) & mask != 0, the compares' test, as a condition.
   mask is a set of COMPARISON_* bits fixed when the code is made.  For
   integers, which are never unordered, after `cmp x, y`: */
int
_PyJitLower_ComparisonInt(_PyJitWordCx *cx, const uint8_t *ops, int mask)  // [x, y]
{
    _PyJitAsm_AluRR(A, _PyJitAlu_CMP, REG(0), REG(1));
    switch (mask & (COMPARISON_LESS_THAN | COMPARISON_GREATER_THAN | COMPARISON_EQUALS)) {
        case COMPARISON_LESS_THAN: return _PyJitCond_L;
        case COMPARISON_GREATER_THAN: return _PyJitCond_G;
        case COMPARISON_EQUALS: return _PyJitCond_E;
        case COMPARISON_LESS_THAN | COMPARISON_EQUALS: return _PyJitCond_LE;
        case COMPARISON_GREATER_THAN | COMPARISON_EQUALS: return _PyJitCond_GE;
        case COMPARISON_LESS_THAN | COMPARISON_GREATER_THAN: return _PyJitCond_NE;
    }
    Py_UNREACHABLE();
}

/* For doubles.  `ucomisd x, y` sets CF for less, ZF for equal, and all of
   ZF, PF and CF for unordered; swapping the operands swaps less and
   greater.  Every set of outcomes is one condition of one of the two orders
   but ordered-equal and its complement, which take two. */
int
_PyJitLower_ComparisonDouble(_PyJitWordCx *cx, const uint8_t *ops, int mask)  // [x, y, t, u]
{
    enum { U = COMPARISON_UNORDERED, L = COMPARISON_LESS_THAN,
           G = COMPARISON_GREATER_THAN, E = COMPARISON_EQUALS };
    int swap = 0, cond;
    switch (mask & (U | L | G | E)) {
        case G: cond = _PyJitCond_A; break;
        case G | E: cond = _PyJitCond_AE; break;
        case U | L: cond = _PyJitCond_B; break;
        case U | L | E: cond = _PyJitCond_BE; break;
        case L: swap = 1; cond = _PyJitCond_A; break;
        case L | E: swap = 1; cond = _PyJitCond_AE; break;
        case U | G: swap = 1; cond = _PyJitCond_B; break;
        case U | G | E: swap = 1; cond = _PyJitCond_BE; break;
        case L | G: cond = _PyJitCond_NE; break;
        case U | E: cond = _PyJitCond_E; break;
        case U: cond = _PyJitCond_P; break;
        case L | G | E: cond = _PyJitCond_NP; break;
        case E:
        case U | L | G:
            _PyJitAsm_AluRR(A, _PyJitAlu_XOR, REG(2), REG(2));
            _PyJitAsm_AluRR(A, _PyJitAlu_XOR, REG(3), REG(3));
            _PyJitAsm_SseRR(A, _PyJitSse_UCOMISD, XMM(0), XMM(1));
            if (mask & E) {
                _PyJitAsm_Setcc(A, _PyJitCond_E, REG(2));
                _PyJitAsm_Setcc(A, _PyJitCond_NP, REG(3));
                _PyJitAsm_TestRR(A, REG(2), REG(3));
            }
            else {
                _PyJitAsm_Setcc(A, _PyJitCond_NE, REG(2));
                _PyJitAsm_Setcc(A, _PyJitCond_P, REG(3));
                _PyJitAsm_AluRR(A, _PyJitAlu_OR, REG(2), REG(3));
            }
            return _PyJitCond_NE;
        default:
            Py_UNREACHABLE();   // never and always: no compare has them
    }
    _PyJitAsm_SseRR(A, _PyJitSse_UCOMISD, swap ? XMM(1) : XMM(0), swap ? XMM(0) : XMM(1));
    return cond;
}

#define _PY_JIT_LOWERINGS_TABLE
#include "jit_lowerings.h"

#endif  // _PY_JIT_SUPPORTED
