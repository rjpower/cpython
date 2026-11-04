/* Test helpers for the borrow stack: the owned references behind the
 * borrowed results of the PyArg_* parsers (Include/internal/pycore_borrow.h).
 * The tests are in Lib/test/test_capi/test_borrow_scope.py. */

#include "parts.h"

#include "pycore_borrow.h"        // _PyBorrow_Depth()
#include "pycore_ceval.h"         // _PyEval_EvalFrameDefault()
#include "pycore_interp.h"        // _PyInterpreterState_SetEvalFrameFunc()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_value.h"         // PyValue_MakeHeapSafe()


static Py_ssize_t
current_depth(void)
{
    return _PyBorrow_Depth(_PyThreadState_GET());
}

static PyObject *
borrow_depth(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    return PyLong_FromSsize_t(current_depth());
}

/* Collect, then allocate and free enough boxes that a box freed too early
   is reused and its contents overwritten. */
static int
churn(void)
{
    PyGC_Collect();
    for (int i = 0; i < 10000; i++) {
        PyObject *f = PyFloat_FromDouble(i + 0.25);
        if (f == NULL) {
            return -1;
        }
        PyObject *n = PyLong_FromLongLong(((long long)1 << 40) + i);
        if (n == NULL) {
            Py_DECREF(f);
            return -1;
        }
        Py_DECREF(n);
        Py_DECREF(f);
    }
    return 0;
}

#define MAX_OUTPUTS 8

typedef union {
    PyObject *o;
    int i;
} parse_output;

/* Build a tuple from the outputs of a format made of O and i codes, groups
   and an optional ':name' or ';message' suffix. */
static PyObject *
outputs_to_tuple(const char *format, parse_output *outputs)
{
    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }
    int k = 0;
    for (const char *p = format; *p && *p != ':' && *p != ';'; p++) {
        PyObject *item;
        if (*p == 'O') {
            item = Py_NewRef(outputs[k++].o);
        }
        else if (*p == 'i') {
            item = PyLong_FromLong(outputs[k++].i);
        }
        else {
            continue;
        }
        if (item == NULL || PyList_Append(result, item) < 0) {
            Py_XDECREF(item);
            Py_DECREF(result);
            return NULL;
        }
        Py_DECREF(item);
    }
    PyObject *tuple = PyList_AsTuple(result);
    Py_DECREF(result);
    return tuple;
}

static int
check_format(const char *format)
{
    int count = 0;
    for (const char *p = format; *p && *p != ':' && *p != ';'; p++) {
        if (*p == 'O' || *p == 'i') {
            count++;
        }
        else if (*p != '(' && *p != ')' && *p != '|') {
            PyErr_Format(PyExc_ValueError, "unsupported format code %c", *p);
            return -1;
        }
    }
    if (count > MAX_OUTPUTS) {
        PyErr_SetString(PyExc_ValueError, "too many format codes");
        return -1;
    }
    return 0;
}

/* Return (delta, result): delta is the number of entries the parse left on
   the stack, and result is either the tuple of outputs (read after churn())
   or the exception the parse raised. */
static PyObject *
report(Py_ssize_t before, int ok, const char *format, parse_output *outputs)
{
    Py_ssize_t delta = current_depth() - before;
    PyObject *result;
    if (!ok) {
        result = PyErr_GetRaisedException();
    }
    else {
        if (churn() < 0) {
            return NULL;
        }
        result = outputs_to_tuple(format, outputs);
        if (result == NULL) {
            return NULL;
        }
    }
    return Py_BuildValue("(nN)", delta, result);
}

/* parse_then_churn(format, args): PyArg_ParseTuple(args, format, ...). */
static PyObject *
parse_then_churn(PyObject *Py_UNUSED(module), PyObject *const *args,
                 Py_ssize_t nargs)
{
    /* METH_FASTCALL, so the helper's own arguments push nothing and the
       delta counts only the parse under test. */
    if (nargs != 2 || !PyUnicode_Check(args[0]) || !PyTuple_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "parse_then_churn(format: str, args: tuple)");
        return NULL;
    }
    const char *format = PyUnicode_AsUTF8(args[0]);
    if (format == NULL || check_format(format) < 0) {
        return NULL;
    }
    parse_output outputs[MAX_OUTPUTS] = {0};
    Py_ssize_t before = current_depth();
    int ok = PyArg_ParseTuple(args[1], format,
                              &outputs[0], &outputs[1], &outputs[2],
                              &outputs[3], &outputs[4], &outputs[5],
                              &outputs[6], &outputs[7]);
    return report(before, ok, format, outputs);
}

/* parse_kw_then_churn(fast, args, kwargs): parse x and y with "OO" through
   PyArg_ParseTupleAndKeywords or _PyArg_ParseTupleAndKeywordsFast. */
static PyObject *
parse_kw_then_churn(PyObject *Py_UNUSED(module), PyObject *const *args,
                    Py_ssize_t nargs)
{
    static const char * const keywords[] = {"x", "y", NULL};
    static _PyArg_Parser parser = {
        .format = "OO:parse_kw_then_churn",
        .keywords = keywords,
    };
    if (nargs != 3 || !PyTuple_Check(args[1]) ||
        (args[2] != Py_None && !PyDict_Check(args[2])))
    {
        PyErr_SetString(PyExc_TypeError,
                        "parse_kw_then_churn(fast, args: tuple, kwargs: dict)");
        return NULL;
    }
    int fast = PyObject_IsTrue(args[0]);
    if (fast < 0) {
        return NULL;
    }
    PyObject *kwargs = args[2] == Py_None ? NULL : args[2];
    parse_output outputs[2] = {0};
    Py_ssize_t before = current_depth();
    int ok;
    if (fast) {
        ok = _PyArg_ParseTupleAndKeywordsFast(args[1], kwargs, &parser,
                                              &outputs[0], &outputs[1]);
    }
    else {
        ok = PyArg_ParseTupleAndKeywords(args[1], kwargs, "OO",
                                         (char **)keywords,
                                         &outputs[0], &outputs[1]);
    }
    return report(before, ok, "OO", outputs);
}

/* A METH_VARARGS function that parses its arguments and returns None. */
static PyObject *
varargs_OO(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *a, *b;
    if (!PyArg_ParseTuple(args, "OO:varargs_OO", &a, &b)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* parse_pairs(pairs, scoped): parse each element of the list with "OO",
   inside its own borrow scope if scoped, and return the number of entries
   left on the stack just before returning. */
static PyObject *
parse_pairs(PyObject *Py_UNUSED(module), PyObject *const *args,
            Py_ssize_t nargs)
{
    if (nargs != 2 || !PyList_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError, "parse_pairs(pairs: list, scoped)");
        return NULL;
    }
    int scoped = PyObject_IsTrue(args[1]);
    if (scoped < 0) {
        return NULL;
    }
    PyObject *pairs = args[0];
    Py_ssize_t before = current_depth();
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(pairs); i++) {
        PyObject *pair = PyList_GetItemRef(pairs, i);
        if (pair == NULL) {
            return NULL;
        }
        PyObject *a, *b;
        int ok;
        if (scoped) {
            Py_BEGIN_BORROW_SCOPE
            ok = PyArg_ParseTuple(pair, "OO", &a, &b);
            Py_END_BORROW_SCOPE
        }
        else {
            ok = PyArg_ParseTuple(pair, "OO", &a, &b);
        }
        Py_DECREF(pair);
        if (!ok) {
            return NULL;
        }
    }
    return PyLong_FromSsize_t(current_depth() - before);
}

/* parse_Oi(*args): "Oi", which boxes the first argument before the second
   can fail. */
static PyObject *
parse_Oi(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o;
    int i;
    if (!PyArg_ParseTuple(args, "Oi:parse_Oi", &o, &i)) {
        return NULL;
    }
    return Py_BuildValue("(Oi)", o, i);
}


/* Reentrancy: an O& converter that calls back into Python. */

typedef struct {
    PyObject *result;
    Py_ssize_t depth_before;
    Py_ssize_t depth_after;
} callback_record;

static int
call_back_converter(PyObject *callback, void *address)
{
    callback_record *record = address;
    record->depth_before = current_depth();
    record->result = PyObject_CallNoArgs(callback);
    record->depth_after = current_depth();
    return record->result != NULL;
}

/* parse_reentrant(args): parse args with "OO&O", where the converter calls
   args[1].  Return (a, b, depth_before, depth_after, callback_result), with
   a and b read after churn(). */
static PyObject *
parse_reentrant(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *a, *b;
    callback_record record = {NULL, -1, -1};
    int ok = PyArg_ParseTuple(args, "OO&O:parse_reentrant",
                              &a, call_back_converter, &record, &b);
    if (!ok) {
        Py_XDECREF(record.result);
        return NULL;
    }
    if (churn() < 0) {
        Py_DECREF(record.result);
        return NULL;
    }
    return Py_BuildValue("(OOnnN)", a, b, record.depth_before,
                         record.depth_after, record.result);
}


/* A type whose nb_add parses an internal tuple with "O" and optionally calls
   a Python function.  BINARY_OP calls nb_add directly, with no native call
   boundary, so the parse leaves its entry above the eval floor.  Calling an
   instance parses the same way through its own vectorcall function, which
   has no trampoline: only the caller's call-protocol scope releases it. */

typedef struct {
    PyObject_HEAD
    PyObject *items;
    PyObject *callback;
    vectorcallfunc vectorcall;
} ParseOnAddObject;

static PyObject *
parse_on_add_vectorcall(PyObject *op, PyObject *const *args, size_t nargsf,
                        PyObject *kwnames)
{
    ParseOnAddObject *self = (ParseOnAddObject *)op;
    PyObject *item;
    if (!PyArg_ParseTuple(self->items, "O", &item)) {
        return NULL;
    }
    return Py_NewRef(item);
}

static PyObject *
parse_on_add_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *items, *callback = Py_None;
    if (!PyArg_ParseTuple(args, "O!|O:ParseOnAdd", &PyTuple_Type, &items,
                          &callback)) {
        return NULL;
    }
    ParseOnAddObject *self = (ParseOnAddObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->items = Py_NewRef(items);
    self->callback = Py_NewRef(callback);
    self->vectorcall = parse_on_add_vectorcall;
    return (PyObject *)self;
}

static void
parse_on_add_dealloc(PyObject *op)
{
    ParseOnAddObject *self = (ParseOnAddObject *)op;
    PyTypeObject *type = Py_TYPE(op);
    Py_XDECREF(self->items);
    Py_XDECREF(self->callback);
    type->tp_free(op);
    Py_DECREF(type);
}

static PyObject *
parse_on_add_add(PyObject *left, PyObject *right)
{
    PyObject *op =
        (binaryfunc)PyType_GetSlot(Py_TYPE(left), Py_nb_add) == parse_on_add_add
        ? left : right;
    ParseOnAddObject *self = (ParseOnAddObject *)op;
    PyObject *item;
    if (!PyArg_ParseTuple(self->items, "O", &item)) {
        return NULL;
    }
    if (self->callback != Py_None) {
        PyObject *res = PyObject_CallNoArgs(self->callback);
        if (res == NULL) {
            return NULL;
        }
        Py_DECREF(res);
    }
    return Py_NewRef(item);
}

static PyMemberDef parse_on_add_members[] = {
    {"__vectorcalloffset__", Py_T_PYSSIZET,
     offsetof(ParseOnAddObject, vectorcall), Py_READONLY},
    {NULL},
};

static PyType_Slot parse_on_add_slots[] = {
    {Py_tp_new, parse_on_add_new},
    {Py_tp_dealloc, parse_on_add_dealloc},
    {Py_tp_call, PyVectorcall_Call},
    {Py_tp_members, parse_on_add_members},
    {Py_nb_add, parse_on_add_add},
    {0, NULL},
};

static PyType_Spec parse_on_add_spec = {
    .name = "_testinternalcapi.ParseOnAdd",
    .basicsize = sizeof(ParseOnAddObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_HAVE_VECTORCALL),
    .slots = parse_on_add_slots,
};


/* A PEP 523 frame evaluator that parses a tuple before it evaluates each
   function frame and appends the parsed object to a list afterwards.  The
   eval loop's floor sits above the hook's entry, so releases in the loop
   leave it alone. */

static PyObject *hook_items;
static PyObject *hook_record;

static PyObject *
parse_eval(PyThreadState *tstate, struct _PyInterpreterFrame *frame, int exc)
{
    PyObject *item = NULL;
    if (hook_items != NULL && !PyArg_ParseTuple(hook_items, "O", &item)) {
        return NULL;
    }
    PyObject *result = _PyEval_EvalFrameDefault(tstate, frame, exc);
    if (result != NULL && item != NULL) {
        if (churn() < 0 || PyList_Append(hook_record, item) < 0) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

/* set_eval_frame_parse(items, record), or set_eval_frame_parse() to remove
   the hook. */
static PyObject *
set_eval_frame_parse(PyObject *Py_UNUSED(module), PyObject *const *args,
                     Py_ssize_t nargs)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (nargs == 0) {
        _PyInterpreterState_SetEvalFrameFunc(interp, NULL);
        Py_CLEAR(hook_items);
        Py_CLEAR(hook_record);
        Py_RETURN_NONE;
    }
    if (nargs != 2 || !PyTuple_Check(args[0]) || !PyList_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "set_eval_frame_parse(items: tuple, record: list)");
        return NULL;
    }
    Py_XSETREF(hook_items, Py_NewRef(args[0]));
    Py_XSETREF(hook_record, Py_NewRef(args[1]));
    _PyInterpreterState_SetEvalFrameFunc(interp, parse_eval);
    Py_RETURN_NONE;
}


/* call_loop(callable, args, n): call callable(*args) n times through
   PyObject_Call() and n times through PyObject_Vectorcall(), and return the
   number of entries left on the stack just before returning.  Only the call
   boundaries' scopes can release entries here: no eval-loop instruction
   boundary runs in this C loop. */
static PyObject *
call_loop(PyObject *Py_UNUSED(module), PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs != 3 || !PyTuple_Check(args[1]) ||
        PyTuple_GET_SIZE(args[1]) > MAX_OUTPUTS)
    {
        PyErr_SetString(PyExc_TypeError,
                        "call_loop(callable, args: tuple, n: int)");
        return NULL;
    }
    PyObject *callable = args[0], *tuple = args[1];
    Py_ssize_t n = PyLong_AsSsize_t(args[2]);
    if (n == -1 && PyErr_Occurred()) {
        return NULL;
    }
    Py_ssize_t size = PyTuple_GET_SIZE(tuple);
    PyObject *vector[MAX_OUTPUTS];
    for (Py_ssize_t i = 0; i < size; i++) {
        vector[i] = PyTuple_GetItemRef(tuple, i);
        if (vector[i] == NULL) {
            while (--i >= 0) {
                Py_DECREF(vector[i]);
            }
            return NULL;
        }
    }
    Py_ssize_t before = current_depth();
    PyObject *result = NULL;
    for (Py_ssize_t i = 0; i < 2 * n; i++) {
        PyObject *res = i < n
            ? PyObject_Call(callable, tuple, NULL)
            : PyObject_Vectorcall(callable, vector, size, NULL);
        if (res == NULL) {
            goto done;
        }
        Py_DECREF(res);
    }
    result = PyLong_FromSsize_t(current_depth() - before);
done:
    for (Py_ssize_t i = 0; i < size; i++) {
        Py_DECREF(vector[i]);
    }
    return result;
}

/* A raw allocator that fails every allocation: the borrow stack's chunks
   come from the raw domain. */
static PyMemAllocatorEx saved_raw;

static void *
fail_raw_malloc(void *ctx, size_t size)
{
    return NULL;
}

static void *
fail_raw_calloc(void *ctx, size_t nelem, size_t elsize)
{
    return NULL;
}

static void *
fail_raw_realloc(void *ctx, void *ptr, size_t size)
{
    return NULL;
}

static void
fail_raw_free(void *ctx, void *ptr)
{
    saved_raw.free(saved_raw.ctx, ptr);
}

/* borrow_grow_oom(args, vector=False): in a scope, push all of args while
   the raw allocator fails, as one tuple run, or with vector true as an
   argument vector (_PyBorrow_ValueVector).  More entries than a chunk holds
   need a new chunk, so the push fails.  Return (delta, exception) as
   parse_then_churn() does, with delta read before the scope exits. */
static PyObject *
borrow_grow_oom(PyObject *Py_UNUSED(module), PyObject *const *args,
                Py_ssize_t nargs)
{
    if (nargs < 1 || nargs > 2 || !PyTuple_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "borrow_grow_oom(args: tuple, vector=False)");
        return NULL;
    }
    int vector = nargs == 2 ? PyObject_IsTrue(args[1]) : 0;
    if (vector < 0) {
        return NULL;
    }
    PyObject *tuple = args[0];
    Py_ssize_t n = PyTuple_GET_SIZE(tuple);
    PyValue *values = NULL;
    if (vector) {
        values = PyMem_New(PyValue, n);
        if (values == NULL) {
            return PyErr_NoMemory();
        }
        for (Py_ssize_t i = 0; i < n; i++) {
            values[i] = PyValue_MakeHeapSafe(PyTuple_GET_VALUE(tuple, i));
        }
    }
    PyMemAllocatorEx failing = {
        .malloc = fail_raw_malloc,
        .calloc = fail_raw_calloc,
        .realloc = fail_raw_realloc,
        .free = fail_raw_free,
    };
    PyThreadState *tstate = _PyThreadState_GET();
    Py_ssize_t before = current_depth();
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &saved_raw);
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &failing);
    const void *pushed = vector
        ? (const void *)_PyBorrow_ValueVector(values, n)
        : (const void *)_PyBorrow_TupleItems(tuple, n);
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &saved_raw);
    PyObject *result;
    if (pushed != NULL) {
        _PyBorrowScope_Exit(tstate, mark);
        PyErr_SetString(PyExc_AssertionError, "the push did not need a chunk");
        result = NULL;
    }
    else {
        result = report(before, 0, "", NULL);
        _PyBorrowScope_Exit(tstate, mark);
    }
    /* The tuple still owns every item, so closing frees nothing. */
    for (Py_ssize_t i = 0; values != NULL && i < n; i++) {
        PyValue_CLOSE(values[i]);
    }
    PyMem_Free(values);
    return result;
}

/* borrow_release_requested(*args): whether this thread's eval breaker asks
   for a borrow release.  It ignores its arguments, so map() can call it. */
static PyObject *
borrow_release_requested(PyObject *Py_UNUSED(module),
                         PyObject *const *Py_UNUSED(args),
                         Py_ssize_t Py_UNUSED(nargs))
{
    return PyBool_FromLong(_Py_eval_breaker_bit_is_set(
        _PyThreadState_GET(), _PY_BORROW_RELEASE_BIT));
}

/* borrow_hold_and_wait(args, seconds): parse args with "O", then release the
   GIL and check the borrowed object's type every 10 ms for `seconds`.  Run
   in a daemon thread while the main thread finalizes: the box must stay
   alive while its thread is still inside the call. */
static PyObject *
borrow_hold_and_wait(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *items, *object;
    double seconds;
    if (!PyArg_ParseTuple(args, "O!d", &PyTuple_Type, &items, &seconds) ||
        !PyArg_ParseTuple(items, "O", &object))
    {
        return NULL;
    }
    /* A held lock: each timed acquire is a portable 10 ms sleep. */
    PyThread_type_lock lock = PyThread_allocate_lock();
    if (lock == NULL) {
        return PyErr_NoMemory();
    }
    PyThread_acquire_lock(lock, WAIT_LOCK);
    PyTypeObject *type = Py_TYPE(object);
    int alive = 1;
    Py_BEGIN_ALLOW_THREADS
    for (double waited = 0; waited < seconds && alive; waited += 0.01) {
        /* Racy by design: the reader holds no GIL, as in a real
           Py_BEGIN_ALLOW_THREADS section that reads its parsed input. */
        alive = Py_TYPE(object) == type;
        (void)PyThread_acquire_lock_timed(lock, 10 * 1000, 0);
    }
    if (!alive) {
        fprintf(stderr, "borrowed object freed under its thread\n");
        abort();
    }
    Py_END_ALLOW_THREADS
    PyThread_release_lock(lock);
    PyThread_free_lock(lock);
    Py_RETURN_NONE;
}

/* Exit a scope with a mark above the top.  Debug builds fail fatally. */
static PyObject *
borrow_bad_exit(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
#ifdef Py_DEBUG
    PyThreadState *tstate = _PyThreadState_GET();
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    _PyBorrowScope_Exit(tstate, mark + 1);
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_RuntimeError, "requires a debug build");
    return NULL;
#endif
}

/* METH_FASTCALL: the reference count of each argument, read before
   anything else is allocated. */
static PyObject *
argument_refcounts(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    Py_ssize_t counts[8];
    if (nargs > (Py_ssize_t)Py_ARRAY_LENGTH(counts)) {
        PyErr_SetString(PyExc_TypeError, "at most 8 arguments");
        return NULL;
    }
    for (Py_ssize_t i = 0; i < nargs; i++) {
        counts[i] = Py_REFCNT(args[i]);
    }
    PyObject *result = PyList_New(0);
    for (Py_ssize_t i = 0; result != NULL && i < nargs; i++) {
        PyObject *count = PyLong_FromSsize_t(counts[i]);
        if (count == NULL || PyList_Append(result, count) < 0) {
            Py_CLEAR(result);
        }
        Py_XDECREF(count);
    }
    return result;
}

static PyMethodDef methods[] = {
    {"borrow_depth", borrow_depth, METH_NOARGS},
    {"parse_then_churn", _PyCFunction_CAST(parse_then_churn), METH_FASTCALL},
    {"parse_kw_then_churn", _PyCFunction_CAST(parse_kw_then_churn), METH_FASTCALL},
    {"varargs_OO", varargs_OO, METH_VARARGS},
    {"parse_pairs", _PyCFunction_CAST(parse_pairs), METH_FASTCALL},
    {"parse_Oi", parse_Oi, METH_VARARGS},
    {"parse_reentrant", parse_reentrant, METH_VARARGS},
    {"set_eval_frame_parse", _PyCFunction_CAST(set_eval_frame_parse), METH_FASTCALL},
    {"borrow_bad_exit", borrow_bad_exit, METH_NOARGS},
    {"call_loop", _PyCFunction_CAST(call_loop), METH_FASTCALL},
    {"borrow_grow_oom", _PyCFunction_CAST(borrow_grow_oom), METH_FASTCALL},
    {"borrow_release_requested", _PyCFunction_CAST(borrow_release_requested), METH_FASTCALL},
    {"borrow_hold_and_wait", borrow_hold_and_wait, METH_VARARGS},
    {"argument_refcounts", _PyCFunction_CAST(argument_refcounts), METH_FASTCALL},
    {NULL},
};

int
_PyTestInternalCapi_Init_BorrowScope(PyObject *module)
{
    if (PyModule_AddFunctions(module, methods) < 0) {
        return -1;
    }
    PyObject *type = PyType_FromModuleAndSpec(module, &parse_on_add_spec, NULL);
    if (type == NULL) {
        return -1;
    }
    return PyModule_Add(module, "ParseOnAdd", type);
}
