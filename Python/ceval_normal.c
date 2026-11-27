/* The normal evaluator: the evaluator (Python/ceval_evaluator.h) over the
 * normal dispatch table (Python/ir_targets.h), which has no monitoring.  The
 * code of a code object that is not observed runs here; the observing
 * evaluator, Python/ceval_observing.c, is the same template compiled again.
 * The driver, ir_evaluate() in Python/ceval.c, enters it at
 * _PyEval_StartFrame() or _PyEval_ThrowFrame(); the observing evaluator
 * switches a frame to it at _PyEval_EnterNormal().
 *
 * This unit holds only the evaluator's functions and the C rows that they
 * inline, so that the build can compile them without frames of their own
 * (CFLAGS_EVALUATOR, Python/ceval_tail.h). */

#define IR_OBSERVING 0
#include "ceval_shared.h"

/* The C rows that the arms inline, with their static helpers.  They stay
 * extern: the observing evaluator, the function forms, the stubs and the
 * baseline JIT call them by symbol.  Here, in the unit of the arms, the
 * compiler inlines them into the arms that run them. */

/* The rows of Convert and Format (Python/register_ops/text.rop): the value
 * is borrowed, the result a new owner or NULL with an exception set. */
PyValue
_PyRop_ConvertValue(int kind, PyValueBorrow v)
{
    PyObject *res_o = kind == 0 ? (_PyValue_Str)(v)
                    : kind == 1 ? (_PyValue_Repr)(v)
                                : (_PyValue_ASCII)(v);
    return res_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res_o);
}

PyValue
_PyRop_FormatPlain(PyValueBorrow v)
{
    // An inline int or float formats as it prints: format(x, "") is str(x)
    PyObject *res_o = PyValue_IsObject(v) ? _PyValue_Format(v, PyValue_NULL)
                                          : (_PyValue_Str)(v);
    return res_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res_o);
}

PyValue
_PyRop_FormatValue(PyValueBorrow v, PyValueBorrow spec)
{
    PyObject *res_o = (_PyValue_Format)(v, spec);
    return res_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res_o);
}

/* The rows of Length and the patterns (Python/register_ops/patterns.rop):
 * the operands are borrowed, the result a new owner or NULL with an
 * exception set. */
PyValue
_PyRop_ValueLength(PyValueBorrow v)
{
    Py_ssize_t len = _PyValue_Size(v);
    if (len < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromInt64(len);
}

int
_PyRop_TypeHasFlag(int kind, PyValueBorrow v)
{
    unsigned long flag = kind == 0 ? Py_TPFLAGS_MAPPING : Py_TPFLAGS_SEQUENCE;
    return (PyValue_TYPE(v)->tp_flags & flag) != 0;
}

PyValue
_PyRop_MatchClass(int count, PyValueBorrow subject, PyValueBorrow type,
                  PyValueBorrow names)
{
    assert(PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(names)));
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *attrs_o = _PyEval_MatchClass(tstate, subject, type, count,
                                           PyValue_AsKnownObjectBorrow(names));
    if (attrs_o != NULL) {
        assert(PyTuple_CheckExact(attrs_o));
        return PyValue_FromObjectSteal(attrs_o);
    }
    if (_PyErr_Occurred(tstate)) {
        return PyValue_NULL;
    }
    return PyValue_None;
}

PyValue
_PyRop_MatchKeys(PyValueBorrow subject, PyValueBorrow keys)
{
    assert(PyValue_IsObject(subject));
    assert(PyValue_IsObject(keys));
    PyObject *values_o = _PyEval_MatchKeys(_PyThreadState_GET(),
        PyValue_AsKnownObjectBorrow(subject), PyValue_AsKnownObjectBorrow(keys));
    return values_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(values_o);
}

/* lhs op rhs for BINARY_OP's operation oparg; both operands are borrowed. */
PyValue
_PyEval_value_binary_op(int oparg, PyValueBorrow lhs, PyValueBorrow rhs)
{
    return _PyValue_BinaryOps[oparg](lhs, rhs);
}

/* The asynchronous iterator of obj, as GET_AITER pushes it, or the null value
 * with an exception set.  Consumes obj. */
PyValue
_PyEval_GetAIter(PyThreadState *tstate, PyValue obj)
{
    unaryfunc getter = NULL;
    // A number has no __aiter__
    if (!PyValue_IsObject(obj)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'async for' requires an object with "
                      "__aiter__ method, got %.100s",
                      PyValue_TYPE(obj)->tp_name);
        return PyValue_NULL;
    }
    PyObject *obj_o = PyValue_AsKnownObjectBorrow(obj);
    PyTypeObject *type = Py_TYPE(obj_o);

    if (type->tp_as_async != NULL) {
        getter = type->tp_as_async->am_aiter;
    }

    if (getter == NULL) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'async for' requires an object with "
                      "__aiter__ method, got %.100s",
                      type->tp_name);
        PyValue_CLOSE(obj);
        return PyValue_NULL;
    }

    PyObject *iter_o = (*getter)(obj_o);
    PyValue_CLOSE(obj);
    if (iter_o == NULL) {
        return PyValue_NULL;
    }

    if (Py_TYPE(iter_o)->tp_as_async == NULL ||
            Py_TYPE(iter_o)->tp_as_async->am_anext == NULL) {

        _PyErr_Format(tstate, PyExc_TypeError,
                      "'async for' received an object from __aiter__ "
                      "that does not implement __anext__: %.100s",
                      Py_TYPE(iter_o)->tp_name);
        Py_DECREF(iter_o);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(iter_o);
}

static Py_NO_INLINE PyValue
value_load_global(PyObject *globals, PyObject *builtins, PyObject *name)
{
    PyValue v = PyValue_NULL;
    _PyEval_LoadGlobalValue(globals, builtins, name, &v);
    return v;
}

static int
switch_push(_PyThreadStateImpl *impl, _PyInterpreterFrame *frame, _PyInstr *real)
{
    if (impl->eval_switches.count == impl->eval_switches.size) {
        int size = impl->eval_switches.size ? 2 * impl->eval_switches.size : 16;
        struct _PyEvalSwitch *items = PyMem_RawRealloc(
            impl->eval_switches.items, (size_t)size * sizeof(*items));
        if (items == NULL) {
            return -1;
        }
        impl->eval_switches.items = items;
        impl->eval_switches.size = size;
    }
    impl->eval_switches.items[impl->eval_switches.count++] =
        (struct _PyEvalSwitch){frame, real};
    frame->return_pc = &_PyEval_SwitchWord;
    return 0;
}

int
_PyEval_SwitchAt(PyThreadState *tstate, _PyInterpreterFrame *frame, _PyInstr *real)
{
    if (switch_push((_PyThreadStateImpl *)tstate, frame, real) < 0) {
        PyErr_NoMemory();
        return -1;
    }
    return 1;
}

/* The rows of the checked local families (Python/register_ops/values.rop) */
void
_PyRop_UnboundLocal(_PyInterpreterFrame *frame, PyValue *home)
{
    int index = (int)(home - frame->base);
    _PyEval_FormatExcCheckArg(
        _PyThreadState_GET(), PyExc_UnboundLocalError, UNBOUNDLOCAL_ERROR_MSG,
        _PyTuple_GET_OBJECT(_PyFrame_GetCode(frame)->co_localsplusnames, index));
}

PyValue
_PyRop_DeleteLocal(_PyInterpreterFrame *frame, PyValue *home)
{
    if (PyValue_IsNull(*home)) {
        _PyRop_UnboundLocal(frame, home);
        return PyValue_NULL;
    }
    return _PyFrame_TakeHome(frame, home);
}

PyValue
_PyRop_MakeCell(_PyInterpreterFrame *frame, PyValue *home)
{
    (void)frame;
    // The cell holds its own count, so closing the binding cannot finalize it
    PyValue binding = *home;
    *home = PyValue_NULL;
    PyObject *cell = PyCell_NewValue(binding);
    PyValue_XCLOSE(binding);
    if (cell == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(cell);
}

/* GET_AWAITABLE's protocol arm: the iterator of x, which is closed on both
 * outcomes */
PyValue
_PyRop_GetAwaitable(PyValue x, uint32_t origin)
{
    PyObject *iter_o = _PyEval_GetAwaitable(x, (int)origin);
    PyValue_CLOSE(x);
    if (iter_o == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(iter_o);
}

/* GET_ANEXT: the awaitable of the next item of an iterator that GET_AITER
 * accepted, which stays borrowed */
PyValue
_PyRop_GetANext(PyValue *aiter_home)
{
    PyValue aiter = PyValue_DUP(PyValue_Borrow(*aiter_home));
    PyObject *awaitable_o = _PyEval_GetANext(PyValue_AsKnownObjectBorrow(PyValue_Borrow(aiter)));
    PyValue_CLOSE(aiter);
    if (awaitable_o == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(awaitable_o);
}

PyValue
_PyRop_LoadName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                uint32_t index)
{
    return _PyEval_LoadName(tstate, frame, _PyRop_NameAt(frame, index));
}

PyValue
_PyRop_LoadLocals(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    PyObject *l = frame->f_locals;
    if (l == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError, "no locals found");
        return PyValue_NULL;
    }
    return PyValue_DUP(PyValue_FromObjectBorrow(l));
}

PyValue
_PyRop_LoadBuildClass(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyStatusValue found = _PyValue_GetOptionalItem(
        PyValue_FromObjectBorrow(frame->f_builtins),
        PyValue_FromObjectBorrow(&_Py_ID(__build_class__)));
    if (found.status < 0) {
        return PyValue_NULL;
    }
    if (found.status == 0) {
        _PyErr_SetString(tstate, PyExc_NameError,
                         "__build_class__ not found");
        return PyValue_NULL;
    }
    return found.value;
}

PyValue
_PyRop_LoadGlobalOrBuiltin(_PyInterpreterFrame *frame, uint32_t index)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    return value_load_global(frame->f_globals, frame->f_builtins, name);
}

/* A NULL value deletes: _PyEval_StoreName's convention */
int
_PyRop_StoreName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                 uint32_t index, PyValue v)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    int error = _PyEval_StoreName(tstate, PyValue_Borrow(v), name,
                                  frame->f_locals);
    PyValue_XCLOSE(v);
    return error ? -1 : 0;
}

int
_PyRop_DeleteName(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  uint32_t index)
{
    return _PyRop_StoreName(tstate, frame, index, PyValue_NULL);
}

int
_PyRop_StoreGlobal(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   uint32_t index, PyValue v)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    int err = PyDict_SetValue(frame->f_globals, PyValue_FromObjectBorrow(name),
                              PyValue_Borrow(v));
    PyValue_XCLOSE(v);
    return err < 0 ? -1 : 0;
}

int
_PyRop_DeleteGlobal(PyThreadState *tstate, _PyInterpreterFrame *frame,
                    uint32_t index)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    int err = PyDict_PopValue(frame->f_globals, PyValue_FromObjectBorrow(name),
                              NULL);
    if (err == 0) {
        _PyEval_FormatExcCheckArg(tstate, PyExc_NameError, NAME_ERROR_MSG,
                                  name);
        return -1;
    }
    return err < 0 ? -1 : 0;
}

PyValue
_PyRop_ImportFrom(PyThreadState *tstate, _PyInterpreterFrame *frame,
                  PyValue module, uint32_t index)
{
    PyObject *name = _PyRop_NameAt(frame, index);
    // The module may be a number, if __import__ returned one
    PyObject *from_o = PyValue_ObjectOrNull(PyValue_Borrow(module));
    PyValue res;
    if (from_o != NULL && PyLazyImport_CheckExact(from_o)) {
        PyObject *res_o = _PyEval_LazyImportFrom(tstate, frame, from_o, name);
        res = res_o == NULL ? PyValue_NULL : PyValue_FromObjectSteal(res_o);
    }
    else {
        res = _PyEval_ImportFromValue(tstate, PyValue_Borrow(module), name);
    }
    PyValue_XCLOSE(module);
    return res;
}

/* The rows of the Move and Rotate arms (Python/register_ops/values.rop).
 * The lowering ordered the pairs of a Move: each destination is vacant when
 * its pair runs, so a pair is a take and a write. */
void
_PyRop_MoveInOrder(const _PyRopSpan *d, const _PyRopSpan *s)
{
    assert(s->count >= d->count);
    for (int i = 0; i < d->count; i++) {
        int from = _PyRopSpan_Slot(s, i);
        int to = _PyRopSpan_Slot(d, i);
        PyValue value = _PyFrame_TakeHome(d->frame, &d->frame->base[from]);
        assert(PyValue_IsNull(d->frame->base[to]));
        _PyFrame_WriteHome(d->frame, &d->frame->base[to], value);
    }
}

void
_PyRop_CopyHomes(const _PyRopSpan *d, const _PyRopSpan *s)
{
    assert(d->count == s->count);
    for (int i = 0; i < d->count; i++) {
        int from = _PyRopSpan_Slot(s, i);
        int to = _PyRopSpan_Slot(d, i);
        PyValue old = _PyFrame_TakeHome(d->frame, &d->frame->base[to]);
        assert(PyValue_IsNull(old));
        (void)old;
        _PyFrame_WriteHome(d->frame, &d->frame->base[to],
                           PyValue_DUP(PyValue_Borrow(d->frame->base[from])));
    }
}

int
_PyRop_ListAppend(PyValue *list, PyValue item)
{
    PyListObject *list_o = (PyListObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(*list));
    return _PyList_AppendTakeValueInline(list_o, item) < 0 ? -1 : 0;
}

int
_PyRop_SetAdd(PyValue *set, PyValue item)
{
    PySetObject *set_o = (PySetObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(*set));
    return _PySet_AddValueTakeRef(set_o, item) ? -1 : 0;
}

int
_PyRop_DictPut(PyValue *dict, PyValue key, PyValue value)
{
    PyObject *dict_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(*dict));
    assert(PyDict_CheckExact(dict_o));
    return _PyDict_SetValue_Take2((PyDictObject *)dict_o, key, value) != 0 ? -1 : 0;
}

/* Unpack and UnpackStar (Python/register_ops/displays.rop): the values of
 * the iterable, in target order, staged in a new tuple that the items hold,
 * so that a failure part-way leaves every home as it was.  The source stays
 * in its home and is held across the call. */
static int
rop_unpack(PyValue *source_home, int before, int after, Py_ssize_t count,
           _PyRopItems *out)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue seq = PyValue_DUP(PyValue_Borrow(*source_home));
    int ok = 0;
    // A number is not iterable
    if (!PyValue_IsObject(PyValue_Borrow(seq))) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "cannot unpack non-iterable %.200s object",
                      PyValue_TYPE(PyValue_Borrow(seq))->tp_name);
        goto done;
    }
    PyObject *staged = PyTuple_New(count);
    if (staged == NULL) {
        goto done;
    }
    PyObject *seq_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(seq));
    PyValue *items = _PyTuple_ITEMS(staged);
    if (!_PyEval_UnpackIterableValue(tstate, seq_o, before, after, items)) {
        // The items made are closed; the tuple holds none of them
        for (Py_ssize_t i = 0; i < count; i++) {
            items[i] = PyValue_NULL;
        }
        Py_DECREF(staged);
        goto done;
    }
    *out = (_PyRopItems){items, staged, count};
    ok = 1;
done:
    PyValue_CLOSE(seq);
    return ok ? 0 : -1;
}

int
_PyRop_UnpackIterable(PyValue *source_home, Py_ssize_t n, _PyRopItems *out)
{
    return rop_unpack(source_home, (int)n, -1, n, out);
}

int
_PyRop_UnpackStar(PyValue *source_home, Py_ssize_t shape, _PyRopItems *out)
{
    int before = shape & 0xFF, after = shape >> 8;
    return rop_unpack(source_home, before, after, before + 1 + after, out);
}

/* The specialized arms: the items of an exact tuple or list of n items.  The
 * guard has checked the type and length.  The list's length is checked
 * again under its lock.  In the free-threaded build, and for a list longer
 * than a park, the items are a tuple copied under the lock; otherwise no
 * code runs until the publish has written them.  A list that changed goes
 * the generic way. */
int
_PyRop_UnpackList(PyValue *source_home, Py_ssize_t n, _PyRopItems *out)
{
    PyObject *seq_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(*source_home));
    assert(PyList_CheckExact(seq_o));
    int same = 1, failed = 0;
    Py_BEGIN_CRITICAL_SECTION(seq_o);
    if (PyList_GET_SIZE(seq_o) != n) {
        same = 0;
    }
#ifndef Py_GIL_DISABLED
    else if (n <= _PY_ROP_PARK) {
        *out = (_PyRopItems){_PyList_ITEMS(seq_o), NULL, n};
    }
#endif
    else {
        PyObject *copy = _PyTuple_FromValueArray(_PyList_ITEMS(seq_o), n);
        failed = copy == NULL;
        *out = (_PyRopItems){copy ? _PyTuple_ITEMS(copy) : NULL, copy, n};
    }
    Py_END_CRITICAL_SECTION();
    if (!same) {
        return _PyRop_UnpackIterable(source_home, n, out);
    }
    return failed ? -1 : 0;
}

bool
_PyRop_TupleHasLength(PyValueBorrow v, Py_ssize_t n)
{
    return (PyValue_IsObject)(v)
           && PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(v))
           && PyTuple_GET_SIZE(PyValue_AsKnownObjectBorrow(v)) == n;
}

bool
_PyRop_ListHasLength(PyValueBorrow v, Py_ssize_t n)
{
    return (PyValue_IsObject)(v)
           && PyList_CheckExact(PyValue_AsKnownObjectBorrow(v))
           && PyList_GET_SIZE(PyValue_AsKnownObjectBorrow(v)) == n;
}

/* The case of classify_unpack: 0 an exact tuple of n items, 1 an exact list
 * of n items, 2 any other */
int
_PyRop_ClassifyUnpack(PyValueBorrow v, Py_ssize_t n)
{
    if (_PyRop_TupleHasLength(v, n)) {
        return 0;
    }
    return _PyRop_ListHasLength(v, n) ? 1 : 2;
}

PyValue
_PyRop_HoldKwargs(_PyInterpreterFrame *frame, const _PyIRWord *head)
{
    if (head[0] & IR_F_NO_KW) {
        return PyValue_NULL;
    }
    return PyValue_DUP(_PyFrame_Slots(frame)[_PyIR_CallOperand(head, 1)]);
}

PyValue
_PyRop_CallEventEx(PyThreadState *tstate, _PyInterpreterFrame *frame,
                   _PyInstr *site, PyValue fo, PyValue tup, _PyInstr *head)
{
    PyObject *callargs = rop_ex_callargs(tup);
    PyValue first = PyValue_NULL;
    if (PyTuple_GET_SIZE(callargs)) {
        first = _PyIR_AdoptTemp(PyTuple_GET_VALUE(callargs, 0));
    }
    if (_PyEval_call_monitor(tstate, frame, site, fo, first) < 0) {
        PyValue_XCLOSE(tup);
        return PyValue_NULL;
    }
    return tup;
}

bool
_PyRop_PythonTargetHeld(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        PyValue fo, const _PyIRWord *head)
{
    return _PyRop_PythonTarget(tstate, frame, &fo, head);
}

_PyInterpreterFrame *
_PyRop_PushPythonHeld(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      PyValue *fn, PyValue fo, _PyInstr *head)
{
    _PyInterpreterFrame *child = _PyRop_PushPython(tstate, frame, &fo, head);
    PyValue_XCLOSE(fo);
    return child;
}

bool
_PyRop_KwargsPending(_PyInterpreterFrame *frame, PyValue *local)
{
    return frame->pending_kwargs != IR_NO_DST;
}

PyValue
_PyRop_NewKwargs(_PyInterpreterFrame *frame, PyValue *local)
{
    uint16_t slot = frame->pending_kwargs;
    assert(slot != IR_NO_DST);
    assert(local == &_PyFrame_Slots(frame)[slot]);
    (void)slot;
    assert(PyValue_IsNull(*local));
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return PyValue_NULL;
    }
    frame->pending_kwargs = IR_NO_DST;
    return PyValue_FromObjectSteal(dict);
}

PyObject *
_PyEval_slice_of_bounds(PyValueBorrow start, PyValueBorrow stop)
{
    return _PySlice_FromValues(start, stop, PyValue_NULL);
}

void
_PyRop_ReraisePending(PyThreadState *tstate, _PyInterpreterFrame *frame,
                      _PyInstr *at)
{
    monitor_reraise(tstate, frame, at);
}

/* The exception in the home becomes the pending one, with a reference of its
 * own: the arm closes the home afterwards */
void
_PyRop_RestoreException(PyThreadState *tstate, _PyInterpreterFrame *frame,
                        _PyInstr *at, PyValue *home)
{
    // An exception being handled is an instance of BaseException
    PyObject *exc = PyValue_AsKnownObjectBorrow(*home);
    assert(exc && PyExceptionInstance_Check(exc));
    _PyErr_SetRaisedException(tstate, Py_NewRef(exc));
    monitor_reraise(tstate, frame, at);
}

/* RERAISE n names the word that raised the exception: the unwinder's tagged
 * index, which it wrote into the home below the exception */
void
_PyRop_SetOrigin(_PyInterpreterFrame *frame, PyValue *home)
{
    frame->instr_ptr = _PyFrame_InstrAt(frame, (int)PyValue_UntagIndex(*home));
}

/* Closes the operand in the home, if it is a temporary, with the pending
 * exception set aside; a local keeps its binding */
void
_PyRop_RetireFailedOperand(_PyInterpreterFrame *frame, PyValue *home)
{
    if (home - _PyFrame_Slots(frame) < _PyCode_IRFixed(_PyFrame_GetCode(frame))) {
        return;
    }
    PyObject *exc = PyErr_GetRaisedException();
    PyValue_XCLOSE(_PyFrame_TakeHome(frame, home));
    PyErr_SetRaisedException(exc);
}

/* The thread's handled exception (None when there is none) leaves the
 * exception state; the exception in the home takes its place, with a new
 * reference */
PyValue
_PyRop_SwapHandled(PyThreadState *tstate, PyValue *home)
{
    _PyErr_StackItem *exc_info = tstate->exc_info;
    PyObject *prev_o = _PyErr_StackItemTakeValue(exc_info);
    PyValue prev = prev_o != NULL ? PyValue_FromObjectSteal(prev_o)
                                  : PyValue_None;
    assert(PyValue_ExceptionInstanceCheck(*home));
    exc_info->exc_value = PyValue_AsPyObjectNew(*home);
    return prev;
}

/* The saved exception (or None) is the handled one again; the one it
 * replaces is released */
void
_PyRop_RestoreHandled(PyThreadState *tstate, PyValue x)
{
    _PyErr_StackItem *exc_info = tstate->exc_info;
    if (PyValue_IsNone(x)) {
        // Immortal: it owes no count
        Py_CLEAR(exc_info->exc_value);
        return;
    }
    Py_XSETREF(exc_info->exc_value, PyValue_AsKnownObjectSteal(x));
}

int
_PyRop_CheckExceptType(PyThreadState *tstate, PyValue *home)
{
    return _PyEval_CheckExceptTypeValidValue(tstate, PyValue_Borrow(*home));
}

bool
_PyRop_ExceptionMatches(PyValueBorrow exc, PyValueBorrow pattern)
{
    assert(PyValue_IsObject(exc) && PyValue_IsObject(pattern));
    return PyErr_GivenExceptionMatches(PyValue_AsKnownObjectBorrow(exc),
                                       PyValue_AsKnownObjectBorrow(pattern));
}

/* The exception that a handler holds is an instance of BaseException */
bool
_PyRop_IsStopIteration(PyValueBorrow exc)
{
    assert(PyValue_IsObject(exc));
    PyObject *o = PyValue_AsKnownObjectBorrow(exc);
    assert(PyExceptionInstance_Check(o));
    return PyErr_GivenExceptionMatches(o, PyExc_StopIteration);
}

PyValue
_PyRop_StopIterationValue(PyValueBorrow exc)
{
    return _PyStopIteration_GetValue(PyValue_AsKnownObjectBorrow(exc));
}

bool
_PyRop_IsStopAsyncIteration(PyValueBorrow exc)
{
    assert(PyValue_IsObject(exc));
    PyObject *o = PyValue_AsKnownObjectBorrow(exc);
    assert(PyExceptionInstance_Check(o));
    return PyErr_GivenExceptionMatches(o, PyExc_StopAsyncIteration);
}

/* exit_func(self, type(exc), exc, tb), or exit_func(type(exc), exc, tb) when
 * the exit function is bound.  exit_func is any attribute value of the context
 * manager's type, an inline scalar included, which the call rejects with
 * upstream's TypeError.  The words are borrowed; a missing traceback is None. */
static Py_NO_INLINE PyValue
with_exit_vectorcall(PyValueBorrow exit_func, PyValueBorrow exit_self,
                     PyValueBorrow exc)
{
    PyObject *val_o = PyValue_AsKnownObjectBorrow(exc);
    assert(val_o && PyExceptionInstance_Check(val_o));
    PyObject *type_o = PyExceptionInstance_Class(val_o);
    // A new reference or NULL
    PyObject *original_tb = PyException_GetTraceback(val_o);
    PyValue stack[4];
    int has_self = !PyValue_IsNull(exit_self);
    _PyValue_WindowPut(&stack[0], exit_self);
    _PyValue_WindowPut(&stack[1], PyValue_FromObjectBorrow(type_o));
    _PyValue_WindowPut(&stack[2], PyValue_FromObjectBorrow(val_o));
    _PyValue_WindowPut(&stack[3], PyValue_FromObjectBorrow(
        original_tb == NULL ? Py_None : original_tb));
    PyValue res = _PyValue_Vectorcall(exit_func, stack + 1 - has_self,
                                      3 + has_self, NULL);
    Py_XDECREF(original_tb);
    return res;
}

PyValue
_PyRop_WithExitCall(PyValueBorrow fn, PyValueBorrow self, PyValueBorrow exc)
{
    return with_exit_vectorcall(fn, self, exc);
}

PyValue
_PyRop_WithExitBound(PyValueBorrow fn, PyValueBorrow exc)
{
    return with_exit_vectorcall(fn, PyValue_Borrow(PyValue_NULL), exc);
}

int
_PyRop_CheckExceptStarType(PyThreadState *tstate, PyValue *home)
{
    return _PyEval_CheckExceptStarTypeValidValue(tstate, PyValue_Borrow(*home));
}

/* The Unary row (Python/register_ops/numeric.rop): closes x on both outcomes.
 * Only -Py_INT_MIN leaves the inline int range, and negating a float that is
 * not NaN gives one that is not. */
PyValue
_PyRop_UnaryOp(uint32_t op, PyValue x)
{
    PyValue value = x;
    PyValue res;
    if (op == ROP_UNARYOP_NEGATIVE) {
        if (PyValue_IsInlineInt(value)) {
            res = PyValue_FromInt64(-PyValue_InlineInt64(value));
        }
        else if (PyValue_IsInlineFloat(value)) {
            res = PyValue_FromDoubleNotNaN(-PyValue_InlineFloat(value));
        }
        else {
            res = _PyValue_Negative(value);
        }
    }
    else if (op == ROP_UNARYOP_POSITIVE) {
        res = _PyValue_Positive(value);
    }
    else {
        assert(op == ROP_UNARYOP_INVERT);
        res = _PyValue_Invert(value);
    }
    PyValue_XCLOSE(value);
    return res;
}

/* x is the deeper stack value, the function's first argument */
PyValue
_PyRop_IntrinsicBinary(PyThreadState *tstate, uint32_t id, PyValue x, PyValue y)
{
    PyValue held2 = x;
    PyValue held1 = y;
    PyValue res = _PyEval_CallIntrinsic2(tstate, rop_intrinsic_number[id],
                                         held2, held1);
    PyValue_XCLOSE(held1);
    PyValue_XCLOSE(held2);
    return res;
}

PyValue
_PyRop_IntrinsicAttr(PyThreadState *tstate, uint32_t id, PyValue x, PyValue y)
{
    PyValue held2 = x;
    PyValue held1 = y;
    PyValue res;
    if (rop_intrinsic_number[id] == INTRINSIC_GETATTR) {
        // The owner is a value; the name is a str
        PyObject *name = PyValue_ObjectOrNull(held1);
        assert(name != NULL && PyUnicode_CheckExact(name));
        res = _PyValue_GetAttr(held2, name);
    }
    else {
        // held2 is the value, NULL for a deletion; held1 is the tuple
        // (owner, name)
        assert(rop_intrinsic_number[id] == INTRINSIC_SETATTR);
        PyObject *target = PyValue_ObjectOrNull(held1);
        assert(target != NULL && PyTuple_CheckExact(target)
               && PyTuple_GET_SIZE(target) == 2);
        PyValueBorrow owner = PyTuple_GET_VALUE(target, 0);
        PyObject *attr_name = _PyTuple_GET_OBJECT(target, 1);
        int err = _PyValue_SetAttr(owner, attr_name, held2);
        res = err < 0 ? PyValue_NULL : PyValue_None;
    }
    PyValue_XCLOSE(held1);
    PyValue_XCLOSE(held2);
    return res;
}

#include "ceval_evaluator.h"
