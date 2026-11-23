/* Hooks for Lib/test/test_jit_template.py: the baseline JIT's primitive
   lowerings (Python/jit_lower.c) run on chosen registers and values. */

#include "parts.h"

#include "pycore_floatobject.h"   // _PyFloat_ExactDealloc()
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_jit_lower.h"
#include "pycore_jit_memory.h"
#include "pycore_long.h"          // _PyLong_ExactDealloc()
#include "pycore_pystate.h"       // _PyThreadState_GET()

#include <stddef.h>               // offsetof

#ifdef _PY_JIT_SUPPORTED

#define A (&a)
#define AT(B, D) _PyJitMem_At((B), (D))

/* The registers before and after the code under test */
typedef struct {
    uint64_t gpr_in[16];
    uint64_t xmm_in[16];
    uint64_t gpr_out[16];
    uint64_t xmm_out[16];
    uint64_t cond;
} RegState;

static int
is_frame_register(int r)
{
    return r == _PyJitReg_RSP || r == _PyJitReg_RBP;
}

static const _PyJitReg saved[] = {
    _PyJitReg_RBP, _PyJitReg_RBX, _PyJitReg_R12,
    _PyJitReg_R13, _PyJitReg_R14, _PyJitReg_R15,
};
#define NSAVED ((int)(sizeof(saved) / sizeof(saved[0])))

/* void f(RegState *s): save the callee-saved registers, keep s in rbp, load
   every other register from s, then the caller emits the code under test. */
static void
emit_prologue(_PyJitAsm *a)
{
    for (int i = 0; i < NSAVED; i++) {
        _PyJitAsm_Push(a, saved[i]);
    }
    _PyJitAsm_AluRI(a, _PyJitAlu_SUB, _PyJitReg_RSP, 8);  // 16-aligned
    _PyJitAsm_MovRR(a, _PyJitReg_RBP, _PyJitReg_RDI);
    for (int r = 0; r < 16; r++) {
        if (!is_frame_register(r)) {
            _PyJitAsm_MovRM(a, (_PyJitReg)r,
                            AT(_PyJitReg_RBP, offsetof(RegState, gpr_in) + 8 * r));
        }
        _PyJitAsm_MovsdRM(a, (_PyJitXmm)r,
                          AT(_PyJitReg_RBP, offsetof(RegState, xmm_in) + 8 * r));
    }
}

/* Store every register into s, restore and return */
static void
emit_epilogue(_PyJitAsm *a)
{
    for (int r = 0; r < 16; r++) {
        if (!is_frame_register(r)) {
            _PyJitAsm_MovMR(a, AT(_PyJitReg_RBP, offsetof(RegState, gpr_out) + 8 * r),
                            (_PyJitReg)r);
        }
        _PyJitAsm_MovsdMR(a, AT(_PyJitReg_RBP, offsetof(RegState, xmm_out) + 8 * r),
                          (_PyJitXmm)r);
    }
    _PyJitAsm_AluRI(a, _PyJitAlu_ADD, _PyJitReg_RSP, 8);
    for (int i = NSAVED - 1; i >= 0; i--) {
        _PyJitAsm_Pop(a, saved[i]);
    }
    _PyJitAsm_Ret(a);
}

static int
u64_items(PyObject *seq, uint64_t *out, Py_ssize_t n, const char *what)
{
    if (!PyTuple_Check(seq) || PyTuple_GET_SIZE(seq) != n) {
        PyErr_Format(PyExc_ValueError, "%s: a tuple of %zd ints", what, n);
        return -1;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PyTuple_GetItemRef(seq, i);
        if (item == NULL) {
            return -1;
        }
        out[i] = PyLong_AsUnsignedLongLongMask(item);
        Py_DECREF(item);
        if (out[i] == (uint64_t)-1 && PyErr_Occurred()) {
            return -1;
        }
    }
    return 0;
}

static PyObject *
u64_tuple(const uint64_t *items, int n)
{
    PyObject *t = PyTuple_New(n);
    if (t == NULL) {
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        PyObject *x = PyLong_FromUnsignedLongLong(items[i]);
        if (x == NULL) {
            Py_DECREF(t);
            return NULL;
        }
        PyTuple_SET_ITEM(t, i, x);
    }
    return t;
}

/* jit_lower(name, ops, gprs, xmms, aux, args_live) -> (cond, gprs_in, gprs, xmms)

   Runs the lowering of primitive `name` on the registers `ops` (its
   operands in the order of pycore_jit_lower.h).  gprs and xmms give every
   register's value before (16 ints each; xmm registers as their bits); the
   hook sets r12 to the thread state, r13 to the caller's frame, r14 to its
   words and r15 to its slots, as a unit holds them.  Every pool register
   that is not an operand is live across the lowering, and so are the
   arguments' registers if args_live (no result may then share one).  aux
   names the destructor of PyValue_CLOSE_SPECIALIZED.  Returns the predicate's value
   (None for a lowering that is not a predicate), the general registers as
   the code got them, and the registers after. */
static PyObject *
jit_lower(PyObject *module, PyObject *args)
{
    const char *name;
    PyObject *ops_seq, *gprs, *xmms, *aux_name;
    int args_live;
    if (!PyArg_ParseTuple(args, "sOOOOp", &name, &ops_seq, &gprs, &xmms, &aux_name,
                          &args_live))
    {
        return NULL;
    }
    const _PyJitLoweringInfo *info = NULL;
    for (int i = 0; i < _PyJit_NumLowerings; i++) {
        if (strcmp(_PyJit_Lowerings[i].name, name) == 0) {
            info = &_PyJit_Lowerings[i];
        }
    }
    if (info == NULL) {
        PyErr_Format(PyExc_ValueError, "no lowering of %s", name);
        return NULL;
    }
    Py_ssize_t nops = (Py_ssize_t)strlen(info->kinds);
    uint64_t ops64[32];
    if (nops > 32 || u64_items(ops_seq, ops64, nops, "ops") < 0) {
        return NULL;
    }
    uint8_t ops[32];
    uint32_t used = 0, used_xmm = 0;
    for (Py_ssize_t i = 0; i < nops; i++) {
        ops[i] = (uint8_t)ops64[i];
        int is_arg = i >= info->nresults && i < info->nresults + info->nargs;
        if (args_live && is_arg) {
            continue;
        }
        if (info->kinds[i] == 'x') {
            used_xmm |= 1u << ops[i];
        }
        else if (!((_PyJit_GPR_POOL >> ops[i]) & 1)) {
            PyErr_Format(PyExc_ValueError, "register %d is not allocatable", ops[i]);
            return NULL;
        }
        else {
            used |= 1u << ops[i];
        }
    }
    const void *aux = NULL;
    if (aux_name != Py_None) {
        const char *d = PyUnicode_AsUTF8(aux_name);
        if (d == NULL) {
            return NULL;
        }
        aux = strcmp(d, "_PyFloat_ExactDealloc") == 0
            ? (const void *)_PyFloat_ExactDealloc : (const void *)_PyLong_ExactDealloc;
    }

    RegState s = {0};
    if (u64_items(gprs, s.gpr_in, 16, "gprs") < 0
        || u64_items(xmms, s.xmm_in, 16, "xmms") < 0)
    {
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    _PyInterpreterFrame *frame = tstate->current_frame;
    s.gpr_in[_PyJit_TSTATE] = (uintptr_t)tstate;
    s.gpr_in[_PyJit_FRAME] = (uintptr_t)frame;
    s.gpr_in[_PyJit_WORDS] = (uintptr_t)frame->ir_words;
    s.gpr_in[_PyJit_BASE] = (uintptr_t)frame->base;

    _PyJitAsm a;
    _PyJitAsm_Init(&a);
    /* The word is the caller's current one, so a lowering that publishes
       writes the instr_ptr it already has. */
    _PyJitWordCx cx = {
        .a = &a,
        .words = (const _PyIRWord *)frame->ir_words,
        .index = (uint32_t)(frame->instr_ptr - frame->ir_words),
        .live = _PyJit_GPR_POOL & ~used,
        .live_xmm = _PyJit_XMM_POOL & ~used_xmm,
        .next = -1, .target = -1, .slow = -1, .resume = -1,
        .leave = -1, .backedge = -1,
    };
    emit_prologue(&a);
    int cond = info->lower(&cx, ops, aux);
    assert(cx.cold_depth == 0);
    if (info->predicate) {
        _PyJitLabel holds = _PyJitAsm_NewLabel(&a);
        _PyJitAsm_MovMI(&a, AT(_PyJitReg_RBP, offsetof(RegState, cond)), 1);
        _PyJitAsm_Jcc(&a, (_PyJitCond)cond, holds);
        _PyJitAsm_MovMI(&a, AT(_PyJitReg_RBP, offsetof(RegState, cond)), 0);
        _PyJitAsm_Bind(&a, holds);
    }
    else if (cond != _PyJitCond_NONE) {
        _PyJitAsm_Fini(&a);
        PyErr_Format(PyExc_AssertionError, "%s returned a condition", name);
        return NULL;
    }
    emit_epilogue(&a);
    _PyJitCode code;
    int placed = _PyJitAsm_Place(&a, &code);
    _PyJitAsm_Fini(&a);
    if (placed < 0) {
        PyErr_SetString(PyExc_MemoryError, "cannot place the lowering");
        return NULL;
    }
    ((void (*)(RegState *))(uintptr_t)code.start)(&s);
    _PyJitMemory_Free(&code);

    PyObject *gin = u64_tuple(s.gpr_in, 16);
    PyObject *gout = u64_tuple(s.gpr_out, 16);
    PyObject *xout = u64_tuple(s.xmm_out, 16);
    PyObject *result = NULL;
    if (gin != NULL && gout != NULL && xout != NULL) {
        result = Py_BuildValue("(OOOO)",
                               info->predicate ? (s.cond ? Py_True : Py_False) : Py_None,
                               gin, gout, xout);
    }
    Py_XDECREF(gin);
    Py_XDECREF(gout);
    Py_XDECREF(xout);
    return result;
}

/* jit_lowerings() -> ((name, nresults, nargs, ntemps, nxtemps, kinds,
   predicate), ...) */
static PyObject *
jit_lowerings(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *t = PyTuple_New(_PyJit_NumLowerings);
    if (t == NULL) {
        return NULL;
    }
    for (int i = 0; i < _PyJit_NumLowerings; i++) {
        const _PyJitLoweringInfo *l = &_PyJit_Lowerings[i];
        PyObject *item = Py_BuildValue("(siiiisO)", l->name, l->nresults, l->nargs,
                                       l->ntemps, l->nxtemps, l->kinds,
                                       l->predicate ? Py_True : Py_False);
        if (item == NULL) {
            Py_DECREF(t);
            return NULL;
        }
        PyTuple_SET_ITEM(t, i, item);
    }
    return t;
}

static PyMethodDef test_methods[] = {
    {"jit_lower", jit_lower, METH_VARARGS},
    {"jit_lowerings", jit_lowerings, METH_NOARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_JitTemplate(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}

#else

int
_PyTestInternalCapi_Init_JitTemplate(PyObject *module)
{
    return 0;
}

#endif  // _PY_JIT_SUPPORTED
