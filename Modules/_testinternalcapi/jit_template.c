/* Hooks for Lib/test/test_jit_template.py: the baseline JIT's primitive
   lowerings (Python/jit_lower.c) run on chosen registers and values, and
   its templates (Python/jit_template.c) run on one word of a frame against
   the word's function form. */

#include "parts.h"

#include "pycore_ceval.h"         // _PyEval_FrameClearAndPop()
#include "pycore_interpframe.h"   // _PyFrame_PushUnchecked()
#include "pycore_ir.h"            // _PyIR_FormatOf()

#include "pycore_floatobject.h"   // _PyFloat_ExactDealloc()
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_jit.h"           // _PyEval_HandlerForms()
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
        .target = -1, .slow = -1, .leave = -1,
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

/* A form's or a unit's return: Python/ceval.h's _PyIRTransfer */
typedef struct {
    _PyInterpreterFrame *frame;
    _PyInstr *next;
} Transfer;

typedef Transfer (*Form)(PyThreadState *, _PyInterpreterFrame *, _PyInstr *, _PyIRWord);
typedef Transfer (*Unit)(PyThreadState *, _PyInterpreterFrame *);

/* Whether the last template run took its slow path */
static int64_t slow_taken;

/* The static target of word i, as the compiler finds it, or -1 */
static int
static_target(const _PyIRWord *words, uint32_t i)
{
    _PyIRWord w = words[i];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(w));
    if (fmt->flags & _PyIR_NAMES_TARGET) {
        return -1;
    }
    if (fmt->flags & _PyIR_JUMPS) {
        return (int)IR_CD(w);
    }
    if (fmt->flags & _PyIR_FORWARD) {
        return (int)i + 1 + IR_DS(w);
    }
    return -1;
}

/* rax:rdx := {frame, word index}, then leave */
static void
emit_transfer(_PyJitAsm *a, int index, _PyJitLabel leave)
{
    _PyJitAsm_MovRR(a, _PyJitReg_RAX, _PyJit_FRAME);
    _PyJitAsm_Lea(a, _PyJitReg_RDX, AT(_PyJit_WORDS, (int32_t)(index * sizeof(_PyIRWord))));
    _PyJitAsm_Jmp(a, leave);
}

/* A unit of one word, entered as Unit: its template, with the exits that a
   compiled unit gives it.  The slow path calls the word's form. */
static int
build_unit(_PyInterpreterFrame *frame, uint32_t index, _PyJitTemplate template,
           _PyJitCode *code, size_t *template_bytes)
{
    const _PyIRWord *words = (const _PyIRWord *)frame->ir_words;
    _PyIRWord w = words[index];
    _PyJitAsm a;
    _PyJitAsm_Init(&a);
    _PyJitWordCx cx = {
        .a = &a, .words = words, .word = w, .index = index,
        .target = _PyJitAsm_NewLabel(&a), .slow = _PyJitAsm_NewLabel(&a),
        .leave = _PyJitAsm_NewLabel(&a),
    };
    for (int i = 0; i < NSAVED; i++) {
        _PyJitAsm_Push(&a, saved[i]);
    }
    _PyJitAsm_AluRI(&a, _PyJitAlu_SUB, _PyJitReg_RSP, 8);
    _PyJitAsm_MovRR(&a, _PyJit_TSTATE, _PyJitReg_RDI);
    _PyJitAsm_MovRR(&a, _PyJit_FRAME, _PyJitReg_RSI);
    _PyJitAsm_MovRM(&a, _PyJit_WORDS, AT(_PyJit_FRAME, offsetof(_PyInterpreterFrame, ir_words)));
    _PyJitAsm_MovRM(&a, _PyJit_BASE, AT(_PyJit_FRAME, offsetof(_PyInterpreterFrame, base)));
    _PyJitAsm_MovImm(&a, _PyJit_UNIT, 0);
    /* The handler check of a word that specialisation may rewrite */
    _PyJitAsm_MovzxRM16(&a, _PyJitReg_RAX, AT(_PyJit_WORDS, (int32_t)(index * sizeof(_PyIRWord))));
    _PyJitAsm_AluRI(&a, _PyJitAlu_CMP, _PyJitReg_RAX, (int32_t)(w & 0xFFFF));
    _PyJitAsm_Jcc(&a, _PyJitCond_NE, cx.slow);
    size_t hot_before = a.sections[_PyJitSection_HOT].size;
    size_t cold_before = a.sections[_PyJitSection_COLD].size;
    template(&cx);
    assert(cx.cold_depth == 0);
    if (template_bytes != NULL) {
        template_bytes[0] = a.sections[_PyJitSection_HOT].size - hot_before;
        template_bytes[1] = a.sections[_PyJitSection_COLD].size - cold_before;
    }
    int target = static_target(words, index);
    emit_transfer(&a, (int)index + _PyIR_WordLength(w), cx.leave);
    _PyJitAsm_Bind(&a, cx.target);
    emit_transfer(&a, target >= 0 ? target : (int)index, cx.leave);
    _PyJitAsm_Bind(&a, cx.slow);
    _PyJitAsm_MovImm(&a, _PyJitReg_RAX, (int64_t)(uintptr_t)&slow_taken);
    _PyJitAsm_MovMI(&a, AT(_PyJitReg_RAX, 0), 1);
    _PyJitAsm_MovRR(&a, _PyJitReg_RDI, _PyJit_TSTATE);
    _PyJitAsm_MovRR(&a, _PyJitReg_RSI, _PyJit_FRAME);
    _PyJitAsm_Lea(&a, _PyJitReg_RDX, AT(_PyJit_WORDS, (int32_t)(index * sizeof(_PyIRWord))));
    _PyJitAsm_MovImm(&a, _PyJitReg_RCX, (int64_t)w);
    _PyJitAsm_CallAbs(&a, _PyEval_HandlerForms()[_PyJit_CompiledHandler(w)]);
    _PyJitAsm_Bind(&a, cx.leave);
    _PyJitAsm_AluRI(&a, _PyJitAlu_ADD, _PyJitReg_RSP, 8);
    for (int i = NSAVED - 1; i >= 0; i--) {
        _PyJitAsm_Pop(&a, saved[i]);
    }
    _PyJitAsm_Ret(&a);
    int placed = _PyJitAsm_Place(&a, code);
    _PyJitAsm_Fini(&a);
    return placed;
}

/* The value of a slot for Python: an int for a word with no object (an
   inline number, an index, NULL), else a new reference to its object */
static PyObject *
slot_object(PyValue v)
{
    if (PyValue_IsNull(v) || !PyValue_IsPointerSpace(v)) {
        return PyLong_FromUnsignedLongLong((unsigned long long)v.bits);
    }
    return Py_NewRef(PyValue_AsKnownObjectBorrow(v));
}

/* A frame leaves with its temporaries empty: close them, then pop it with
   its locals and constants */
static void
clear_and_pop(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    int nfixed = (int)(((const _PyCodeExec *)frame->ir_words) - 1)->nfixed;
    for (int i = nfixed; i < _PyFrame_NumSlots(frame); i++) {
        PyValue v = frame->base[i];
        frame->base[i] = PyValue_NULL;
        PyValue_XCLOSE(v);
    }
    _PyEval_FrameClearAndPop(tstate, frame);
}

/* jit_template_run(func, index, slots, template) -> (next, slow, exc, slots)

   Pushes a frame of func (whose code has run, so that it has an executing
   copy), sets the slots given by the dict slots (index: object, or index:
   int for a raw word, which the frame then owns as it is), and runs word
   index by its template if template, else by its form.  Returns the index
   of the next word (None if the word raised), whether the template took its
   slow path, the exception it raised, and every slot after it as
   slot_object() gives it.  The frame is then cleared and popped. */
static PyObject *
jit_template_run(PyObject *module, PyObject *args)
{
    PyObject *func, *slots;
    unsigned int index;
    int use_template;
    if (!PyArg_ParseTuple(args, "O!IO!p", &PyFunction_Type, &func, &index,
                          &PyDict_Type, &slots, &use_template))
    {
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)((PyFunctionObject *)func)->func_code;
    if (code->_co_exec == NULL) {
        PyErr_SetString(PyExc_ValueError, "the code has not run");
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    if (!_PyThreadState_HasStackSpace(tstate, code->co_framesize)) {
        PyErr_NoMemory();
        return NULL;
    }
    _PyInterpreterFrame *previous = tstate->current_frame;
    _PyInterpreterFrame *frame = _PyFrame_PushUnchecked(
        tstate, PyValue_FromObjectSteal(Py_NewRef(func)), previous);
    int nslots = _PyFrame_NumSlots(frame);
    const _PyIRWord *words = (const _PyIRWord *)frame->ir_words;
    _PyIRWord w = words[index];
    frame->instr_ptr = (_PyInstr *)&words[index];
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_NextEntryRef(slots, &pos, &key, &value)) {
        long i = PyLong_AsLong(key);
        Py_DECREF(key);
        if (i < 0 || i >= nslots) {
            Py_DECREF(value);
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_IndexError, "no such slot");
            }
            clear_and_pop(tstate, frame);
            return NULL;
        }
        PyValue v;
        PyObject *tag = NULL;
        if (PyTuple_CheckExact(value) && PyTuple_GET_SIZE(value) == 2) {
            tag = PyTuple_GetItemRef(value, 0);
        }
        int raw = tag != NULL && PyUnicode_Check(tag) && PyUnicode_EqualToUTF8(tag, "raw");
        Py_XDECREF(tag);
        if (raw) {
            /* ("raw", bits): a word that is no object, such as a tagged index */
            PyObject *bits = PyTuple_GetItemRef(value, 1);
            v.bits = bits ? (uintptr_t)PyLong_AsUnsignedLongLong(bits) : 0;
            Py_XDECREF(bits);
            Py_DECREF(value);
            if (PyErr_Occurred()) {
                clear_and_pop(tstate, frame);
                return NULL;
            }
        }
        else {
            v = PyValue_FromObjectSteal(value);
        }
        PyValue_XCLOSE(frame->base[i]);
        frame->base[i] = v;
    }
    tstate->current_frame = frame;
    Transfer t;
    slow_taken = 0;
    if (use_template) {
        _PyJitTemplate template = _PyJit_Templates[_PyJit_CompiledHandler(w)];
        _PyJitCode unit;
        if (template == NULL || build_unit(frame, index, template, &unit, NULL) < 0) {
            tstate->current_frame = previous;
            clear_and_pop(tstate, frame);
            PyErr_SetString(PyExc_ValueError, "no template for the word, or no code memory");
            return NULL;
        }
        t = ((Unit)(uintptr_t)unit.start)(tstate, frame);
        _PyJitMemory_Free(&unit);
    }
    else {
        Form form = (Form)_PyEval_HandlerForms()[_PyJit_CompiledHandler(w)];
        t = form(tstate, frame, (_PyInstr *)&words[index], w);
    }
    tstate->current_frame = previous;
    PyObject *exc = PyErr_GetRaisedException();
    PyObject *out = PyTuple_New(nslots);
    for (int i = 0; out != NULL && i < nslots; i++) {
        PyObject *o = slot_object(frame->base[i]);
        if (o == NULL) {
            Py_CLEAR(out);
            break;
        }
        PyTuple_SET_ITEM(out, i, o);
    }
    assert(t.frame == frame || t.next == NULL);
    PyObject *next = t.next == NULL ? Py_NewRef(Py_None)
        : PyLong_FromLong((long)(t.next - (_PyInstr *)words));
    clear_and_pop(tstate, frame);
    if (out == NULL || next == NULL) {
        Py_XDECREF(out);
        Py_XDECREF(next);
        Py_XDECREF(exc);
        return NULL;
    }
    return Py_BuildValue("(NONN)", next, slow_taken ? Py_True : Py_False,
                         exc ? exc : Py_None, out);
}

/* jit_template_size(func, index) -> (hot, cold)

   The bytes of the template of word index of func, in the hot and in the
   cold section, as compiled for that word (the slots that it names are the
   word's own). */
static PyObject *
jit_template_size(PyObject *module, PyObject *args)
{
    PyObject *func;
    unsigned int index;
    if (!PyArg_ParseTuple(args, "O!I", &PyFunction_Type, &func, &index)) {
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)((PyFunctionObject *)func)->func_code;
    if (code->_co_exec == NULL) {
        PyErr_SetString(PyExc_ValueError, "the code has not run");
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    if (!_PyThreadState_HasStackSpace(tstate, code->co_framesize)) {
        PyErr_NoMemory();
        return NULL;
    }
    _PyInterpreterFrame *frame = _PyFrame_PushUnchecked(
        tstate, PyValue_FromObjectSteal(Py_NewRef(func)), tstate->current_frame);
    _PyIRWord w = ((const _PyIRWord *)frame->ir_words)[index];
    _PyJitTemplate template = _PyJit_Templates[_PyJit_CompiledHandler(w)];
    _PyJitCode unit;
    size_t bytes[2];
    int built = template == NULL ? -1 : build_unit(frame, index, template, &unit, bytes);
    if (built >= 0) {
        _PyJitMemory_Free(&unit);
    }
    clear_and_pop(tstate, frame);
    if (built < 0) {
        PyErr_SetString(PyExc_ValueError, "no template for the word, or no code memory");
        return NULL;
    }
    return Py_BuildValue("(nn)", (Py_ssize_t)bytes[0], (Py_ssize_t)bytes[1]);
}

static PyMethodDef test_methods[] = {
    {"jit_template_run", jit_template_run, METH_VARARGS},
    {"jit_template_size", jit_template_size, METH_VARARGS},
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
