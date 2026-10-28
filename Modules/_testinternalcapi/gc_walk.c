/* Fixtures for the collector's stack walk (Python/gc_walk.c; note 11
 * section 3), x86-64 only: chains of C frames that hold known values in
 * callee-saved registers, alternating precise and opaque, and threads that
 * detach with a value only in a register. */

#include "parts.h"
#include "pycore_gc_trace.h"      // _PyGC_TraceLiveRootsOfHidden()
#include "pycore_gc_walk.h"
#include "pycore_lock.h"          // PyEvent
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tstate.h"        // _PyThreadStateImpl

#if defined(_PyGC_WALK) && defined(__x86_64__)

#define SENTINEL(i) ((uintptr_t)0x5a5a5a5a00000000ull | (uintptr_t)(i))
#define NLEVELS 5

/* --- the owners of registers -------------------------------------------- */

struct owners {
    uintptr_t *words;
    size_t n, cap;
    int failed;
    int64_t test_fallbacks;
    int fell_back;
};

static void
owners_scan(void *arg, uintptr_t start, uintptr_t end)
{
    struct owners *o = arg;
    for (uintptr_t p = start; p + sizeof(uintptr_t) <= end; p += sizeof(uintptr_t)) {
        if (o->n == o->cap) {
            size_t cap = o->cap ? 2 * o->cap : 1024;
            uintptr_t *w = realloc(o->words, cap * sizeof(*w));
            if (w == NULL) {
                o->failed = 1;
                return;
            }
            o->words = w;
            o->cap = cap;
        }
        o->words[o->n++] = *(uintptr_t *)p;
    }
}

static struct owners *current_owners;

static Py_NO_INLINE void
walk_from_here(void)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    _PyGCWalk w = {
        .scan = owners_scan,
        .arg = current_owners,
        .top = ts->c_stack_top,
        .activations = ts->gc_trace.activation,
    };
    // Not in this frame, whose words the walk scans
    static _Py_thread_local _PyGCContext ctx;
    _PyGC_CaptureCaller(&ctx);
    current_owners->fell_back = _PyGC_WalkStack(&w, &ctx);
    current_owners->test_fallbacks = w.test_fallbacks;
}

/* Each level keeps its sentinel in one callee-saved register across the
   call to the next: the asm operands place it there before and after the
   call, so the compiler must preserve it in that register or in a slot of
   its own frame, both of which this frame owns.  Not inlined, no sibling
   calls, and a use after the call. */
#define LEVEL(name, reg, i, next)                                           \
    static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls"))) \
    uintptr_t name(void)                                                    \
    {                                                                       \
        register uintptr_t v __asm__(reg) = SENTINEL(i);                    \
        __asm__ volatile("" : "+r"(v));                                     \
        uintptr_t r = next();                                               \
        __asm__ volatile("" : "+r"(v));                                     \
        return r + v;                                                       \
    }

static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
uintptr_t
level_innermost(void)
{
    walk_from_here();
    return 1;
}

LEVEL(level4, "r15", 4, level_innermost)
LEVEL(level3, "r14", 3, level4)
LEVEL(level2, "r13", 2, level3)
LEVEL(level1, "r12", 1, level2)
LEVEL(level0, "rbx", 0, level1)

static uintptr_t (*const levels[NLEVELS])(void) = {
    level0, level1, level2, level3, level4,
};

static int
register_function(uintptr_t (*f)(void), int cls, int flags)
{
    _PyGCFrameDesc d;
    if (_PyGC_DecodeDescriptor((uintptr_t)f, &d) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "no descriptor for a fixture");
        return -1;
    }
    if (_PyGC_RegisterCodeRange(d.func_start, d.func_end, cls, flags,
                                NULL) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "the code range table is full");
        return -1;
    }
    return 0;
}

/* Run the chain with the levels in precise (a string of '0' and '1', one
   per level) registered precise, and return, per level, whether the walk
   scanned its sentinel, with the walk's fallback flag. */
static PyObject *
walk_owners(PyObject *arg)
{
    const char *precise = PyUnicode_AsUTF8(arg);
    if (precise == NULL) {
        return NULL;
    }
    if (strlen(precise) != NLEVELS) {
        PyErr_SetString(PyExc_ValueError, "one flag per level");
        return NULL;
    }
    int registered = 0;
    struct owners o = {0};
    PyObject *res = NULL;
    for (int i = 0; i < NLEVELS; i++) {
        if (precise[i] == '1') {
            if (register_function(levels[i], _PyGC_FRAME_PRECISE,
                                  _PyGC_RANGE_TEST) < 0) {
                goto unregister;
            }
            registered |= 1 << i;
        }
    }
    current_owners = &o;
    (void)level0();
    current_owners = NULL;
    if (o.failed) {
        PyErr_NoMemory();
        goto done;
    }
    res = PyTuple_New(NLEVELS + 1);
    if (res == NULL) {
        goto done;
    }
    for (int i = 0; i < NLEVELS; i++) {
        int found = 0;
        for (size_t k = 0; k < o.n; k++) {
            found |= o.words[k] == SENTINEL(i);
        }
        PyTuple_SET_ITEM(res, i, PyBool_FromLong(found));
    }
    PyTuple_SET_ITEM(res, NLEVELS, PyBool_FromLong(o.fell_back));
done:
    free(o.words);
unregister:
    for (int i = 0; i < NLEVELS; i++) {
        if (registered & (1 << i)) {
            _PyGCFrameDesc d;
            (void)_PyGC_DecodeDescriptor((uintptr_t)levels[i], &d);
            _PyGC_UnregisterCodeRange(d.func_start);
        }
    }
    return res;
}

static PyValue
gc_walk_owners(PyObject *self, PyValue arg)
{
    PyObject *op = PyValue_Object(arg);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(walk_owners(op));
}

/* --- a frame without unwind information: the walk falls back --------- */

/* A function with no CFI, so no FDE: it saves rbx, which holds a
   sentinel, and calls rdi. */
__asm__(
    ".text\n"
    ".p2align 4\n"
    "_testgc_nocfi:\n"
    "    push %rbx\n"
    "    movabs $0x5a5a5a5a000000ee, %rbx\n"
    "    call *%rdi\n"
    "    pop %rbx\n"
    "    ret\n"
    "_testgc_nocfi_end:\n"
);
extern void _testgc_nocfi(void (*)(void)) __asm__("_testgc_nocfi");
extern char _testgc_nocfi_end[] __asm__("_testgc_nocfi_end");

/* Walk through the frame without unwind information: (fell back, the
   fallbacks counted against test ranges, whether the words outward of it
   were scanned). */
static PyObject *
gc_walk_fallback(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    uintptr_t start = (uintptr_t)&_testgc_nocfi;
    if (_PyGC_RegisterCodeRange(start, (uintptr_t)_testgc_nocfi_end,
                                _PyGC_FRAME_CORE, _PyGC_RANGE_TEST,
                                NULL) < 0) {
        PyErr_SetString(PyExc_RuntimeError, "the code range table is full");
        return NULL;
    }
    struct owners o = {0};
    current_owners = &o;
    _testgc_nocfi(walk_from_here);
    current_owners = NULL;
    _PyGC_UnregisterCodeRange(start);
    int found = 0;
    for (size_t k = 0; k < o.n; k++) {
        found |= o.words[k] == SENTINEL(0xee);
    }
    free(o.words);
    if (o.failed) {
        return PyErr_NoMemory();
    }
    return Py_BuildValue("(iLi)", o.fell_back, (long long)o.test_fallbacks,
                         found);
}

/* --- detached threads -------------------------------------------------- */

static volatile uintptr_t hidden_addr;  // the object's address, XOR a mask
static PyThreadState *holder;    // the thread that holds it
#define HIDE 0x5555555555555555ull
static PyEvent published, release;

/* Detach with obj only in a callee-saved register, and block until
   released: how the thread detaches is mode's (0: Py_BEGIN_ALLOW_THREADS,
   pinned; 1: a helper that detaches and returns before this blocks, with
   this function registered precise, unpinned; 2: a swap to NULL in a
   function built -O0, unpinned). */
static Py_NO_INLINE PyThreadState *
detach_and_return(void)
{
    return PyEval_SaveThread();
}

static Py_NO_INLINE __attribute__((optimize("O0"))) PyThreadState *
swap_out_O0(void)
{
    return PyThreadState_Swap(NULL);
}

static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
uintptr_t
hold_detached(int mode)
{
    // Unmask the address in r14 itself, so that no other register or
    // stack slot of this thread holds it.
    register uintptr_t v __asm__("r14") = hidden_addr;
    __asm__ volatile("xor %1, %0" : "+r"(v) : "r"((uintptr_t)HIDE));
    holder = _PyThreadState_GET();
    PyThreadState *saved = NULL;
    _PyGCFrameDesc d;
    int precise = mode == 1
        && _PyGC_DecodeDescriptor((uintptr_t)&hold_detached, &d) == 0
        && _PyGC_RegisterCodeRange(d.func_start, d.func_end,
                                   _PyGC_FRAME_PRECISE, _PyGC_RANGE_TEST,
                                   NULL) == 0;
    if (mode == 0) {
        Py_BEGIN_ALLOW_THREADS
        __asm__ volatile("" : "+r"(v));
        _PyEvent_Notify(&published);
        PyEvent_Wait(&release);
        __asm__ volatile("" : "+r"(v));
        Py_END_ALLOW_THREADS
    }
    else {
        saved = mode == 1 ? detach_and_return() : swap_out_O0();
        __asm__ volatile("" : "+r"(v));
        _PyEvent_Notify(&published);
        PyEvent_Wait(&release);
        __asm__ volatile("" : "+r"(v));
        if (mode == 1) {
            PyEval_RestoreThread(saved);
        }
        else {
            PyThreadState_Swap(saved);
        }
    }
    if (precise) {
        _PyGC_UnregisterCodeRange(d.func_start);
    }
    __asm__ volatile("" : "+r"(v));
    return v;
}

/* Create [None] and publish its masked address in hidden_addr.  A
   function of its own, so that its caller's registers never hold it. */
static Py_NO_INLINE int
make_hidden(void)
{
    PyObject *obj = PyList_New(0);
    if (obj == NULL || PyList_Append(obj, Py_None) < 0) {
        Py_XDECREF(obj);
        return -1;
    }
    hidden_addr = (uintptr_t)obj ^ HIDE;
    return 0;
}

/* In a thread: create an object, hold it only in a register while
   detached until gc_walk_release(), and return it. */
static PyValue
gc_walk_hold_detached(PyObject *self, PyValue arg)
{
    if (!PyValue_IsInlineInt(arg)) {
        PyErr_SetString(PyExc_TypeError, "a mode, 0 to 2");
        return PyValue_NULL;
    }
    int mode = PyValue_Int32(arg);
    if (make_hidden() < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromObject((PyObject *)hold_detached(mode));
}

/* From another thread: wait until the object is published and return the
   _PyGC_TRACE_ROOT_* classes that reach it. */
static PyObject *
gc_walk_detached_roots(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_BEGIN_ALLOW_THREADS
    PyEvent_Wait(&published);
    Py_END_ALLOW_THREADS
    int roots = _PyGC_TraceLiveRootsOfHidden(_PyThreadState_GET(),
                                             &hidden_addr, HIDE);
    _PyGCDetachRecord *rec = &((_PyThreadStateImpl *)holder)->gc_trace.detach;
    return Py_BuildValue("(iii)", roots, rec->valid, rec->pinned);
}

/* Detach and reattach on this thread with obj only in a callee-saved
   register, and return whether the detach record still holds it: the
   record is stale once the thread attaches, and a scan that read it would
   keep obj. */
static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
int
detach_holding(PyObject *obj)
{
    register uintptr_t v __asm__("r14") = (uintptr_t)obj;
    __asm__ volatile("" : "+r"(v));
    Py_BEGIN_ALLOW_THREADS
    __asm__ volatile("" : "+r"(v));
    Py_END_ALLOW_THREADS
    __asm__ volatile("" : "+r"(v));
    _PyGCDetachRecord *rec =
        &((_PyThreadStateImpl *)_PyThreadState_GET())->gc_trace.detach;
    for (int i = 0; i < _PyGC_NSAVED; i++) {
        if (rec->cap.regs[i] == v) {
            return 1;
        }
    }
    return 0;
}

static PyValue
gc_walk_detach_holding(PyObject *self, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyBool_FromLong(detach_holding(obj)));
}

/* The missed-root demonstration (note 11 8.3): a function that registers
   itself precise, so that the collector does not scan its frame, holds an
   object only there across collect() and then uses it.  Under
   PYTHON_GCVERIFY=kill the object dies and its blocks are quarantined, so
   each use sees PYMEM_DEADBYTE, or traps under ASan:
     "field"      the list's ob_size, read after the collection;
     "interior"   its first item, read through ob_item cached before;
     "finalizer"  make()'s object, whose finalizer runs, read by its type;
     "weakref"    make()'s object, which keep holds a weak reference to:
                  the counterexample, which kill clears and retain hands
                  back, a candidate that is no missed root. */
static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
PyObject *
missed_root(const char *kind, PyObject *make, PyObject *collect,
            PyObject *keep)
{
    _PyGCFrameDesc d;
    if (_PyGC_DecodeDescriptor((uintptr_t)&missed_root, &d) < 0
        || _PyGC_RegisterCodeRange(d.func_start, d.func_end,
                                   _PyGC_FRAME_PRECISE, _PyGC_RANGE_TEST,
                                   NULL) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "no stack walk");
        return NULL;
    }
    PyObject *result = NULL;
    PyObject *obj = NULL;
    PyValue *items = NULL;
    PyObject *wr = NULL;
    int field = strcmp(kind, "field") == 0;
    int interior = strcmp(kind, "interior") == 0;
    if (field || interior) {
        obj = PyList_New(0);
        PyObject *item = PyLong_FromLong(123456789);
        if (obj == NULL || item == NULL || PyList_Append(obj, item) < 0) {
            goto done;
        }
        items = ((PyListObject *)obj)->ob_item;
    }
    else {
        obj = PyObject_CallNoArgs(make);
        if (obj == NULL) {
            goto done;
        }
        if (strcmp(kind, "weakref") == 0) {
            wr = PyWeakref_NewRef(obj, NULL);
            if (wr == NULL || PyList_Append(keep, wr) < 0) {
                goto done;
            }
        }
    }
    PyObject *r = PyObject_CallNoArgs(collect);
    if (r == NULL) {
        goto done;
    }
    if (field) {
        result = PyLong_FromSsize_t(Py_SIZE(obj));
    }
    else if (interior) {
        result = PyLong_FromVoidPtr((void *)items[0].bits);
    }
    else if (wr != NULL) {
        PyObject *ref;
        if (PyWeakref_GetRef(wr, &ref) < 0) {
            goto done;
        }
        result = ref != NULL ? ref : Py_None;
    }
    else {
        result = PyLong_FromVoidPtr(Py_TYPE(obj));
    }
done:
    _PyGC_UnregisterCodeRange(d.func_start);
    return result;
}

static PyObject *
gc_verify_missed_root(PyObject *self, PyObject *args)
{
    const char *kind;
    PyObject *make, *collect, *keep;
    if (!PyArg_ParseTuple(args, "sOOO!", &kind, &make, &collect,
                          &PyList_Type, &keep)) {
        return NULL;
    }
    return missed_root(kind, make, collect, keep);
}

static PyObject *
gc_walk_release(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyEvent_Notify(&release);
    Py_RETURN_NONE;
}

/* Reset the events between runs. */
static PyObject *
gc_walk_reset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    published = (PyEvent){0};
    release = (PyEvent){0};
    hidden_addr = 0;
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"gc_walk_owners", _PyCFunction_CAST(gc_walk_owners), METH_O},
    {"gc_walk_fallback", gc_walk_fallback, METH_NOARGS},
    {"gc_walk_hold_detached", _PyCFunction_CAST(gc_walk_hold_detached), METH_O},
    {"gc_verify_missed_root", gc_verify_missed_root, METH_VARARGS},
    {"gc_walk_detached_roots", gc_walk_detached_roots, METH_NOARGS},
    {"gc_walk_detach_holding", _PyCFunction_CAST(gc_walk_detach_holding), METH_O},
    {"gc_walk_release", gc_walk_release, METH_NOARGS},
    {"gc_walk_reset", gc_walk_reset, METH_NOARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_GCWalk(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}

#else

int
_PyTestInternalCapi_Init_GCWalk(PyObject *m)
{
    return 0;
}

#endif
