/* Fixtures for the collector's stack walk (Python/gc_walk.c; note 11
 * section 3), x86-64 only: chains of C frames that hold known values in
 * callee-saved registers, alternating precise and opaque, and threads that
 * detach with a value only in a register. */

#include "parts.h"
#include "pycore_call.h"          // _PyStack_UnpackDictSlots()
#include "pycore_gc_trace.h"      // _PyGC_TraceLiveRootsOfHidden()
#include "pycore_gc_walk.h"
#include "pycore_lock.h"          // PyEvent
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value.h"         // _PyThreadState_PushValueSpan()

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

/* Reads the walked frames' words, redzones included, as the collector's
   scan does: not instrumented under ASan (Python/gc_trace.c) */
static void _Py_NO_SANITIZE_ADDRESS
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
     "slot_call"  make()'s object, repr'd;
     "late"       make()'s object, repr'd after a second collection;
     "store"      make()'s object, appended to keep, which is repr'd;
     "weakref"    make()'s object, which keep holds a weak reference to:
                  the counterexample, which kill clears and retain hands
                  back, a candidate that is no missed root;
     "release"    a list whose item array has a page of its own, collected
                  past the quarantine's horizon with the frame still holding
                  the array's address, so that the release that frees the
                  page and B's lookup of that word meet in one collection.
   In a debug build kill gives the dead object the trap type, so each use
   by its type aborts with the object's record. */
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
    int release = strcmp(kind, "release") == 0;
    if (release) {
        /* 32 MiB of items: past mimalloc's large size, so the array has a
           segment of its own, which its free releases at once */
        PyObject *one = PyList_New(0);
        if (one == NULL || PyList_Append(one, Py_None) < 0) {
            goto done;
        }
        obj = PySequence_Repeat(one, (Py_ssize_t)1 << 22);
        if (obj == NULL) {
            goto done;
        }
        items = ((PyListObject *)obj)->ob_item;
    }
    else if (field || interior) {
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
    if (strcmp(kind, "late") == 0) {
        r = PyObject_CallNoArgs(collect);
        if (r == NULL) {
            goto done;
        }
    }
    // Past the quarantine's 16 collections
    for (int i = 0; release && i < 20; i++) {
        r = PyObject_CallNoArgs(collect);
        if (r == NULL) {
            goto done;
        }
    }
    if (field) {
        result = PyLong_FromSsize_t(Py_SIZE(obj));
    }
    else if (strcmp(kind, "slot_call") == 0 || strcmp(kind, "late") == 0) {
        result = PyObject_Repr(obj);
    }
    else if (strcmp(kind, "store") == 0) {
        if (PyList_Append(keep, obj) == 0) {
            result = PyObject_Repr(keep);
        }
    }
    else if (interior) {
        result = PyLong_FromVoidPtr((void *)items[0].bits);
    }
    else if (release) {
        // The address only: the block is gone
        result = PyLong_FromVoidPtr((void *)items);
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

/* An __exit__, as a staticmethod (exc_type, exc, tb), that clears the
   exception's traceback, collects and then uses its tb argument.  Its frame
   is registered precise while it runs and keeps no copy of tb, so only the
   span of _PyEval_CallExitWithException, whose array args is, keeps the
   traceback through the collection: without that span, kill traps at the
   use.  Returns False, so that the exception propagates. */
static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
PyObject *
gc_exit_clears_traceback(PyObject *self, PyObject *const *args,
                         Py_ssize_t nargs)
{
    if (nargs != 3) {
        PyErr_SetString(PyExc_TypeError, "takes 3 arguments");
        return NULL;
    }
    _PyGCFrameDesc d;
    if (_PyGC_DecodeDescriptor((uintptr_t)&gc_exit_clears_traceback, &d) < 0
        || _PyGC_RegisterCodeRange(d.func_start, d.func_end,
                                   _PyGC_FRAME_PRECISE, _PyGC_RANGE_TEST,
                                   NULL) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "no stack walk");
        return NULL;
    }
    PyObject *result = NULL;
    if (PyException_SetTraceback(args[1], Py_None) < 0) {
        goto done;
    }
    PyGC_Collect();
    if (!PyTraceBack_Check(args[2])) {
        PyErr_SetString(PyExc_AssertionError, "tb is not a traceback");
        goto done;
    }
    result = Py_NewRef(Py_False);
done:
    _PyGC_UnregisterCodeRange(d.func_start);
    return result;
}


/* The span a slot binding boxes into (6d rung 2, test 5): a frame
   registered precise boxes an uncached inline int, holds the box only in
   a span, raw, or, as the control, canonical (the inline value, which
   holds no box), collects, and reads the box's type.  Under
   PYTHON_GCVERIFY=kill the control's box dies and the read aborts.  At its
   collection point the fixture walks the stack itself: the collection's
   walk does not report fallbacks in a test range.  Returns (the box is
   still an int, its frame was classed precise, the walk fell back,
   test-range fallbacks). */
struct span_walk {
    uintptr_t start, end;
    int cls;
    int fell_back;
    int64_t test_fallbacks;
};

static void
span_walk_scan(void *arg, uintptr_t start, uintptr_t end)
{
}

static void
span_walk_frame(void *arg, uintptr_t pc, uintptr_t sp, uintptr_t cfa,
                int cls)
{
    struct span_walk *f = arg;
    if (f->start <= pc && pc < f->end) {
        f->cls = cls;
    }
}

static struct span_walk last_span_walk;

static Py_NO_INLINE void
span_walk_here(struct span_walk *f)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    _PyGCWalk w = {
        .scan = span_walk_scan,
        .frame = span_walk_frame,
        .arg = f,
        .top = ts->c_stack_top,
    };
    static _Py_thread_local _PyGCContext ctx;
    _PyGC_CaptureCaller(&ctx);
    f->fell_back = _PyGC_WalkStack(&w, &ctx);
    f->test_fallbacks = w.test_fallbacks;
}

static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
PyObject *
span_box(int raw, int32_t value, PyObject *collect)
{
    _PyGCFrameDesc d;
    if (_PyGC_DecodeDescriptor((uintptr_t)&span_box, &d) < 0
        || _PyGC_RegisterCodeRange(d.func_start, d.func_end,
                                   _PyGC_FRAME_PRECISE, _PyGC_RANGE_TEST,
                                   NULL) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "no stack walk");
        return NULL;
    }
    PyObject *result = NULL;
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue held[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, held);
    PyObject *box = _PyValue_BoxInline(PyValue_FromInt32(value));
    if (box == NULL) {
        goto done;
    }
    if (raw) {
        _PyValueSpan_AppendObject(&span, box);
    }
    else {
        _PyValueSpan_Append(&span, PyValue_FromObject(box));
    }
    struct span_walk f = {.start = d.func_start, .end = d.func_end, .cls = -1};
    span_walk_here(&f);
    last_span_walk = f;
    PyObject *r = PyObject_CallNoArgs(collect);
    if (r == NULL) {
        goto done;
    }
    // The use: a dead box aborts here under kill, with its record.
    int is_int = Py_TYPE(box) == &PyLong_Type;
    result = Py_BuildValue("(OOOL)", is_int ? Py_True : Py_False,
                           f.cls == _PyGC_FRAME_PRECISE ? Py_True : Py_False,
                           f.fell_back ? Py_True : Py_False,
                           (long long)f.test_fallbacks);
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    _PyGC_UnregisterCodeRange(d.func_start);
    return result;
}

/* The walk of the last gc_span_box(), for its collect callable to report
   before the use, which faults in the control. */
static PyObject *
gc_span_box_walk(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    struct span_walk *f = &last_span_walk;
    return Py_BuildValue("(OOL)",
                         f->cls == _PyGC_FRAME_PRECISE ? Py_True : Py_False,
                         f->fell_back ? Py_True : Py_False,
                         (long long)f->test_fallbacks);
}

static PyObject *
gc_span_box(PyObject *self, PyObject *args)
{
    int raw, value;
    PyObject *collect;
    if (!PyArg_ParseTuple(args, "piO", &raw, &value, &collect)) {
        return NULL;
    }
    return span_box(raw, value, collect);
}

/* PyObject_VectorcallMethod(name, [obj, *items], with
   PY_VECTORCALL_ARGUMENTS_OFFSET) from a frame registered precise.  holder
   is [obj, *items]; it is cleared before the call, so the argument array is
   the only holder of each.  The array is this frame's span, as its creator
   anchors it (10a-2.md rule 3).  A bound method found on obj prepends its
   self into the slot that holds obj, so while it runs the span holds the
   self and what keeps obj is the prepend's own span (rule 4).  Returns
   (the result, or the exception raised, and the name of the type of what
   the slot holds after the call, which must be obj again). */
static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
PyObject *
vectorcall_method_precise(PyObject *name, PyObject *holder)
{
    _PyGCFrameDesc d;
    if (_PyGC_DecodeDescriptor((uintptr_t)&vectorcall_method_precise, &d) < 0
        || _PyGC_RegisterCodeRange(d.func_start, d.func_end,
                                   _PyGC_FRAME_PRECISE, _PyGC_RANGE_TEST,
                                   NULL) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "no stack walk");
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue stack[16];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, stack);
    Py_ssize_t n = PyList_GET_SIZE(holder);
    for (Py_ssize_t i = 0; i < n; i++) {
        // The items are objects: the tests pass instances
        _PyValueSpan_AppendObject(&span, PyList_GET_ITEM(holder, i));
    }
    PyObject *res = NULL, *exc = NULL;
    const char *slot_type = NULL;
    if (PyList_SetSlice(holder, 0, n, NULL) == 0) {
        res = PyObject_VectorcallMethod(
            name, (PyObject *const *)stack,
            n | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
        exc = res == NULL ? PyErr_GetRaisedException() : NULL;
        // Kill traps here if the slot's object was not kept
        slot_type = Py_TYPE((PyObject *)stack[0].bits)->tp_name;
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    // Scanned again from here: res and exc are held while the tuple is made
    _PyGC_UnregisterCodeRange(d.func_start);
    if (slot_type == NULL) {
        return NULL;
    }
    return Py_BuildValue("(Os)", res != NULL ? res : exc, slot_type);
}

static PyObject *
gc_vectorcall_method(PyObject *self, PyObject *args)
{
    PyObject *name, *holder;
    if (!PyArg_ParseTuple(args, "UO!", &name, &PyList_Type, &holder)) {
        return NULL;
    }
    if (PyList_GET_SIZE(holder) < 1 || PyList_GET_SIZE(holder) > 16) {
        PyErr_SetString(PyExc_ValueError, "holder has 1 to 16 items");
        return NULL;
    }
    return vectorcall_method_precise(name, holder);
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

/* 10a rung 2b item 3 (10a-2b.md section 1): an unpacked call's arguments
   in its caller's span.  gc_unpack_consumer(d, collect, k=obj), called
   with kwargs d through gc_unpack_call(), registers its frame precise,
   clears d, collects and reads obj's type: only the caller's span, which
   the keyword values were unpacked into, holds obj then.  Under
   PYTHON_GCVERIFY=kill a missing anchor aborts at the read.  Returns (the
   type's name, its frame was classed precise, the walk fell back,
   test-range fallbacks). */
static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
PyObject *
gc_unpack_consumer(PyObject *self, PyObject *const *args, Py_ssize_t nargs,
                   PyObject *kwnames)
{
    if (nargs != 2 || !PyDict_Check(args[0]) || kwnames == NULL
        || PyTuple_GET_SIZE(kwnames) != 1)
    {
        PyErr_SetString(PyExc_TypeError, "takes (dict, collect, k=obj)");
        return NULL;
    }
    _PyGCFrameDesc d;
    if (_PyGC_DecodeDescriptor((uintptr_t)&gc_unpack_consumer, &d) < 0
        || _PyGC_RegisterCodeRange(d.func_start, d.func_end,
                                   _PyGC_FRAME_PRECISE, _PyGC_RANGE_TEST,
                                   NULL) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "no stack walk");
        return NULL;
    }
    PyObject *result = NULL;
    struct span_walk f = {.start = d.func_start, .end = d.func_end, .cls = -1};
    span_walk_here(&f);
    PyDict_Clear(args[0]);
    PyObject *r = PyObject_CallNoArgs(args[1]);
    if (r == NULL) {
        goto done;
    }
    // The use: a dead value aborts here under kill, with its record.
    const char *name = Py_TYPE(args[2])->tp_name;
    result = Py_BuildValue("(sOOL)", name,
                           f.cls == _PyGC_FRAME_PRECISE ? Py_True : Py_False,
                           f.fell_back ? Py_True : Py_False,
                           (long long)f.test_fallbacks);
done:
    _PyGC_UnregisterCodeRange(d.func_start);
    return result;
}

/* f(*args, **kwargs) from C: PyObject_Call(), whose keyword form is
   _PyVectorcall_Call()'s, or, with dict_form, PyObject_VectorcallDict().
   args' items are objects. */
static PyObject *
gc_unpack_call(PyObject *self, PyObject *a)
{
    PyObject *f, *args, *kwargs;
    int dict_form;
    if (!PyArg_ParseTuple(a, "OO!O!p", &f, &PyTuple_Type, &args,
                          &PyDict_Type, &kwargs, &dict_form)) {
        return NULL;
    }
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    for (Py_ssize_t i = 0; i < n; i++) {
        if (PyValue_IsInline(PyTuple_GET_VALUE(args, i))) {
            PyErr_SetString(PyExc_ValueError, "args' items are objects");
            return NULL;
        }
    }
    if (!dict_form) {
        return PyObject_Call(f, args, kwargs);
    }
    return PyObject_VectorcallDict(
        f, (PyObject *const *)_PyTuple_ITEMS(args), n, kwargs);
}

/* A callee that allocates nothing, so that a test that fails allocations
   counts only the unpacked call's own: the buffer and the names. */
static PyObject *
gc_unpack_sink(PyObject *self, PyObject *const *args, Py_ssize_t nargs,
               PyObject *kwnames)
{
    Py_RETURN_NONE;
}

/* _PyStack_UnpackDictSlots(prefix, nargs, nkwargs): the buffer's size, or
   -1 on overflow. */
static PyObject *
gc_unpack_dict_slots(PyObject *self, PyObject *a)
{
    Py_ssize_t prefix, nargs, nkwargs;
    if (!PyArg_ParseTuple(a, "nnn", &prefix, &nargs, &nkwargs)) {
        return NULL;
    }
    return PyLong_FromSsize_t(
        _PyStack_UnpackDictSlots(prefix, nargs, nkwargs));
}

/* The number of value roots this thread has pushed, so that a test can
   check that an error path pops what it pushed. */
static PyObject *
gc_value_roots_depth(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)_PyThreadState_GET();
    Py_ssize_t n = 0;
    for (_PyValueRoot *r = ts->gc_trace.value_roots; r != NULL; r = r->next) {
        n++;
    }
    return PyLong_FromSsize_t(n);
}

static PyMethodDef test_methods[] = {
    {"gc_walk_owners", _PyCFunction_CAST(gc_walk_owners), METH_O},
    {"gc_walk_fallback", gc_walk_fallback, METH_NOARGS},
    {"gc_walk_hold_detached", _PyCFunction_CAST(gc_walk_hold_detached), METH_O},
    {"gc_verify_missed_root", gc_verify_missed_root, METH_VARARGS},
    {"gc_exit_clears_traceback", _PyCFunction_CAST(gc_exit_clears_traceback),
     METH_FASTCALL},
    {"gc_span_box", gc_span_box, METH_VARARGS},
    {"gc_span_box_walk", gc_span_box_walk, METH_NOARGS},
    {"gc_vectorcall_method", gc_vectorcall_method, METH_VARARGS},
    {"gc_unpack_consumer", _PyCFunction_CAST(gc_unpack_consumer),
     METH_FASTCALL | METH_KEYWORDS},
    {"gc_unpack_call", gc_unpack_call, METH_VARARGS},
    {"gc_unpack_sink", _PyCFunction_CAST(gc_unpack_sink),
     METH_FASTCALL | METH_KEYWORDS},
    {"gc_unpack_dict_slots", gc_unpack_dict_slots, METH_VARARGS},
    {"gc_value_roots_depth", gc_value_roots_depth, METH_NOARGS},
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
