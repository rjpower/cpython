#include "Python.h"
#include "opcode.h"

#include "pycore_code.h"          // _PyCodeConstructor
#include "pycore_function.h"      // _PyFunction_ClearCodeByVersion()
#include "pycore_hashtable.h"     // _Py_hashtable_t
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // PyInterpreterState.co_extra_freefuncs
#include "pycore_interpframe.h"   // FRAME_SPECIALS_SIZE
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_opcode_utils.h"  // RESUME_AT_FUNC_START
#include "pycore_pymem.h"         // _PyMem_FreeDelayed()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_setobject.h"     // _PySet_NextValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_InternImmortal()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()

#include "clinic/codeobject.c.h"
#include <stdbool.h>


#define INITIAL_SPECIALIZED_CODE_SIZE 16

static const char *
code_event_name(PyCodeEvent event) {
    switch (event) {
        #define CASE(op)                \
        case PY_CODE_EVENT_##op:         \
            return "PY_CODE_EVENT_" #op;
        PY_FOREACH_CODE_EVENT(CASE)
        #undef CASE
    }
    Py_UNREACHABLE();
}

static void
notify_code_watchers(PyCodeEvent event, PyCodeObject *co)
{
    assert(Py_REFCNT(co) > 0);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);
    uint8_t bits = interp->active_code_watchers;
    int i = 0;
    while (bits) {
        assert(i < CODE_MAX_WATCHERS);
        if (bits & 1) {
            PyCode_WatchCallback cb = interp->code_watchers[i];
            // callback must be non-null if the watcher bit is set
            assert(cb != NULL);
            if (cb(event, co) < 0) {
                PyErr_FormatUnraisable(
                    "Exception ignored in %s watcher callback for %R",
                    code_event_name(event), co);
            }
        }
        i++;
        bits >>= 1;
    }
}

int
PyCode_AddWatcher(PyCode_WatchCallback callback)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);

    for (int i = 0; i < CODE_MAX_WATCHERS; i++) {
        if (!interp->code_watchers[i]) {
            interp->code_watchers[i] = callback;
            interp->active_code_watchers |= (1 << i);
            return i;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "no more code watcher IDs available");
    return -1;
}

static inline int
validate_watcher_id(PyInterpreterState *interp, int watcher_id)
{
    if (watcher_id < 0 || watcher_id >= CODE_MAX_WATCHERS) {
        PyErr_Format(PyExc_ValueError, "Invalid code watcher ID %d", watcher_id);
        return -1;
    }
    if (!interp->code_watchers[watcher_id]) {
        PyErr_Format(PyExc_ValueError, "No code watcher set for ID %d", watcher_id);
        return -1;
    }
    return 0;
}

int
PyCode_ClearWatcher(int watcher_id)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->_initialized);
    if (validate_watcher_id(interp, watcher_id) < 0) {
        return -1;
    }
    interp->code_watchers[watcher_id] = NULL;
    interp->active_code_watchers &= ~(1 << watcher_id);
    return 0;
}

/******************
 * generic helpers
 ******************/

#define _PyCodeObject_CAST(op)  (assert(PyCode_Check(op)), (PyCodeObject *)(op))

int
_PyCode_ShouldInternString(PyObject *o)
{
    // compute if s matches [a-zA-Z0-9_]
    const unsigned char *s, *e;

    if (!PyUnicode_IS_ASCII(o))
        return 0;

    s = PyUnicode_1BYTE_DATA(o);
    e = s + PyUnicode_GET_LENGTH(o);
    for (; s != e; s++) {
        if (!Py_ISALNUM(*s) && *s != '_')
            return 0;
    }
    return 1;
}


static int
intern_strings(PyObject *tuple)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    Py_ssize_t i;

    for (i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyObject *v = PyTuple_GET_ITEM(tuple, i);
        if (v == NULL || !PyUnicode_CheckExact(v)) {
            PyErr_SetString(PyExc_SystemError,
                            "non-string found in code slot");
            return -1;
        }
        _PyUnicode_InternImmortal(interp, &_PyTuple_HEAP_ITEMS(tuple)[i]);
    }
    return 0;
}

/* Intern constants. In the default build, this interns selected string
   constants. In the free-threaded build, this also interns non-string
   constants. */
static int
intern_constants(PyObject *tuple, int *modified)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    for (Py_ssize_t i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyObject *v = PyTuple_GET_ITEM(tuple, i);
        if (PyUnicode_CheckExact(v)) {
            if (_PyCode_ShouldInternString(v)) {
                PyObject *w = v;
                _PyUnicode_InternMortal(interp, &v);
                if (w != v) {
                    PyTuple_SET_ITEM(tuple, i, v);
                    if (modified) {
                        *modified = 1;
                    }
                }
            }
        }
        else if (PyTuple_CheckExact(v)) {
            if (intern_constants(v, NULL) < 0) {
                return -1;
            }
        }
        else if (PyFrozenSet_CheckExact(v)) {
            PyObject *w = v;
            PyObject *tmp = PySequence_Tuple(v);
            if (tmp == NULL) {
                return -1;
            }
            int tmp_modified = 0;
            if (intern_constants(tmp, &tmp_modified) < 0) {
                Py_DECREF(tmp);
                return -1;
            }
            if (tmp_modified) {
                v = PyFrozenSet_New(tmp);
                if (v == NULL) {
                    Py_DECREF(tmp);
                    return -1;
                }

                PyTuple_SET_ITEM(tuple, i, v);
                Py_DECREF(w);
                if (modified) {
                    *modified = 1;
                }
            }
            Py_DECREF(tmp);
        }
    }
    return 0;
}

/* Return a shallow copy of a tuple that is
   guaranteed to contain exact strings, by converting string subclasses
   to exact strings and complaining if a non-string is found. */
static PyObject*
validate_and_copy_tuple(PyObject *tup)
{
    PyObject *newtuple;
    PyObject *item;
    Py_ssize_t i, len;

    len = PyTuple_GET_SIZE(tup);
    newtuple = PyTuple_New(len);
    if (newtuple == NULL)
        return NULL;

    for (i = 0; i < len; i++) {
        item = PyTuple_GET_ITEM(tup, i);
        if (PyUnicode_CheckExact(item)) {
            Py_INCREF(item);
        }
        else if (!PyUnicode_Check(item)) {
            PyErr_Format(
                PyExc_TypeError,
                "name tuples must contain only "
                "strings, not '%.500s'",
                Py_TYPE(item)->tp_name);
            Py_DECREF(newtuple);
            return NULL;
        }
        else {
            item = _PyUnicode_Copy(item);
            if (item == NULL) {
                Py_DECREF(newtuple);
                return NULL;
            }
        }
        PyTuple_SET_ITEM(newtuple, i, item);
    }

    return newtuple;
}

static int
init_co_cached(PyCodeObject *self)
{
    _PyCoCached *cached = FT_ATOMIC_LOAD_PTR(self->_co_cached);
    if (cached != NULL) {
        return 0;
    }

    Py_BEGIN_CRITICAL_SECTION(self);
    cached = self->_co_cached;
    if (cached == NULL) {
        cached = PyMem_New(_PyCoCached, 1);
        if (cached == NULL) {
            PyErr_NoMemory();
        }
        else {
            cached->_co_code = NULL;
            cached->_co_cellvars = NULL;
            cached->_co_freevars = NULL;
            cached->_co_varnames = NULL;
            FT_ATOMIC_STORE_PTR(self->_co_cached, cached);
        }
    }
    Py_END_CRITICAL_SECTION();
    return cached != NULL ? 0 : -1;
}

/******************
 * _PyCode_New()
 ******************/

// This is also used in compile.c.
void
_Py_set_localsplus_info(int offset, PyObject *name, _PyLocals_Kind kind,
                        PyObject *names, PyObject *kinds)
{
    PyTuple_SET_ITEM(names, offset, Py_NewRef(name));
    _PyLocals_SetKind(kinds, offset, kind);
}

static void
get_localsplus_counts(PyObject *names, PyObject *kinds,
                      int *pnlocals, int *pncellvars,
                      int *pnfreevars)
{
    int nlocals = 0;
    int ncellvars = 0;
    int nfreevars = 0;
    Py_ssize_t nlocalsplus = PyTuple_GET_SIZE(names);
    for (int i = 0; i < nlocalsplus; i++) {
        _PyLocals_Kind kind = _PyLocals_GetKind(kinds, i);
        if (kind & CO_FAST_LOCAL) {
            nlocals += 1;
            if (kind & CO_FAST_CELL) {
                ncellvars += 1;
            }
        }
        else if (kind & CO_FAST_CELL) {
            ncellvars += 1;
        }
        else if (kind & CO_FAST_FREE) {
            nfreevars += 1;
        }
    }
    if (pnlocals != NULL) {
        *pnlocals = nlocals;
    }
    if (pncellvars != NULL) {
        *pncellvars = ncellvars;
    }
    if (pnfreevars != NULL) {
        *pnfreevars = nfreevars;
    }
}

static PyObject *
get_localsplus_names(PyCodeObject *co, _PyLocals_Kind kind, int num)
{
    PyObject *names = PyTuple_New(num);
    if (names == NULL) {
        return NULL;
    }
    int index = 0;
    for (int offset = 0; offset < co->co_nlocalsplus; offset++) {
        _PyLocals_Kind k = _PyLocals_GetKind(co->co_localspluskinds, offset);
        if ((k & kind) == 0) {
            continue;
        }
        assert(index < num);
        PyObject *name = PyTuple_GET_ITEM(co->co_localsplusnames, offset);
        PyTuple_SET_ITEM(names, index, Py_NewRef(name));
        index += 1;
    }
    assert(index == num);
    return names;
}

int
_PyCode_Validate(struct _PyCodeConstructor *con)
{
    /* Check argument types */
    if (con->argcount < con->posonlyargcount || con->posonlyargcount < 0 ||
        con->kwonlyargcount < 0 ||
        con->stacksize < 0 || con->flags < 0 ||
        con->code == NULL || !PyBytes_Check(con->code) ||
        con->consts == NULL || !PyTuple_Check(con->consts) ||
        con->names == NULL || !PyTuple_Check(con->names) ||
        con->localsplusnames == NULL || !PyTuple_Check(con->localsplusnames) ||
        con->localspluskinds == NULL || !PyBytes_Check(con->localspluskinds) ||
        PyTuple_GET_SIZE(con->localsplusnames)
            != PyBytes_GET_SIZE(con->localspluskinds) ||
        con->name == NULL || !PyUnicode_Check(con->name) ||
        con->qualname == NULL || !PyUnicode_Check(con->qualname) ||
        con->filename == NULL || !PyUnicode_Check(con->filename) ||
        con->linetable == NULL || !PyBytes_Check(con->linetable) ||
        con->exceptiontable == NULL || !PyBytes_Check(con->exceptiontable)
        ) {
        PyErr_BadInternalCall();
        return -1;
    }

    /* Make sure that code is indexable with an int, this is
       a long running assumption in ceval.c and many parts of
       the interpreter. */
    if (PyBytes_GET_SIZE(con->code) > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "code: co_code larger than INT_MAX");
        return -1;
    }
    if (PyBytes_GET_SIZE(con->code) % sizeof(_PyIRWord) != 0) {
        PyErr_SetString(PyExc_ValueError, "code: co_code is malformed");
        return -1;
    }
    if (con->fbinit != NULL
        && (!PyBytes_Check(con->fbinit)
            || PyBytes_GET_SIZE(con->fbinit) % sizeof(uint32_t) != 0
            || PyBytes_GET_SIZE(con->fbinit) / sizeof(uint32_t) > IR_FIELD_MAX + 1))
    {
        PyErr_SetString(PyExc_ValueError, "code: co_fbinit is malformed");
        return -1;
    }

    /* Ensure that the co_varnames has enough names to cover the arg counts.
     * Note that totalargs = nlocals - nplainlocals.  We check nplainlocals
     * here to avoid the possibility of overflow (however remote). */
    int nlocals;
    get_localsplus_counts(con->localsplusnames, con->localspluskinds,
                          &nlocals, NULL, NULL);
    int nplainlocals = nlocals -
                       con->argcount -
                       con->kwonlyargcount -
                       ((con->flags & CO_VARARGS) != 0) -
                       ((con->flags & CO_VARKEYWORDS) != 0);
    if (nplainlocals < 0) {
        PyErr_SetString(PyExc_ValueError, "code: co_varnames is too small");
        return -1;
    }

    return 0;
}


static int
init_code(PyCodeObject *co, struct _PyCodeConstructor *con)
{
    int nlocalsplus = (int)PyTuple_GET_SIZE(con->localsplusnames);
    int nlocals, ncellvars, nfreevars;
    get_localsplus_counts(con->localsplusnames, con->localspluskinds,
                          &nlocals, &ncellvars, &nfreevars);
    if (con->stacksize == 0) {
        con->stacksize = 1;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    co->co_filename = Py_NewRef(con->filename);
    co->co_name = Py_NewRef(con->name);
    co->co_qualname = Py_NewRef(con->qualname);
    _PyUnicode_InternMortal(interp, &co->co_filename);
    _PyUnicode_InternMortal(interp, &co->co_name);
    _PyUnicode_InternMortal(interp, &co->co_qualname);
    co->co_flags = con->flags;

    co->co_firstlineno = con->firstlineno;
    co->co_linetable = Py_NewRef(con->linetable);

    co->co_consts = Py_NewRef(con->consts);
    co->co_names = Py_NewRef(con->names);

    co->co_localsplusnames = Py_NewRef(con->localsplusnames);
    co->co_localspluskinds = Py_NewRef(con->localspluskinds);

    co->co_argcount = con->argcount;
    co->co_posonlyargcount = con->posonlyargcount;
    co->co_kwonlyargcount = con->kwonlyargcount;

    co->co_stacksize = con->stacksize;

    co->co_exceptiontable = Py_NewRef(con->exceptiontable);

    /* derived values */
    co->co_nlocalsplus = nlocalsplus;
    co->co_nlocals = nlocals;
    co->co_framesize = nlocalsplus + con->stacksize + FRAME_SPECIALS_SIZE;
    co->co_ncellvars = ncellvars;
    co->co_nfreevars = nfreevars;
    FT_MUTEX_LOCK(&interp->func_state.mutex);
    co->co_version = interp->func_state.next_version;
    if (interp->func_state.next_version != 0) {
        interp->func_state.next_version++;
    }
    FT_MUTEX_UNLOCK(&interp->func_state.mutex);
    co->_co_monitoring = NULL;
    co->_co_instrumentation_version = 0;
    /* not set */
    co->co_extra = NULL;
    co->_co_cached = NULL;

    memcpy(co->co_words, PyBytes_AS_STRING(con->code),
           PyBytes_GET_SIZE(con->code));
    co->co_nfeedback = 0;
    if (con->fbinit != NULL && PyBytes_GET_SIZE(con->fbinit)) {
        co->co_nfeedback = (int)(PyBytes_GET_SIZE(con->fbinit) / sizeof(uint32_t));
        memcpy((uint32_t *)_PyCode_FBINIT(co), PyBytes_AS_STRING(con->fbinit),
               PyBytes_GET_SIZE(con->fbinit));
    }
    co->_co_exec = NULL;
    int entry_point = 0;
    while (entry_point < Py_SIZE(co)) {
        if (_PyCode_InstrHandler(co, entry_point) == RESUME &&
           (_PyCode_InstrOparg(co, entry_point) & RESUME_OPARG_LOCATION_MASK) != RESUME_AT_GEN_EXPR_START
        ) {
            break;
        }
        entry_point++;
    }
    co->_co_firsttraceable = entry_point;

    notify_code_watchers(PY_CODE_EVENT_CREATE, co);
    return 0;
}

static int
scan_varint(const uint8_t *ptr)
{
    unsigned int read = *ptr++;
    unsigned int val = read & 63;
    unsigned int shift = 0;
    while (read & 64) {
        read = *ptr++;
        shift += 6;
        val |= (read & 63) << shift;
    }
    return val;
}

static int
scan_signed_varint(const uint8_t *ptr)
{
    unsigned int uval = scan_varint(ptr);
    if (uval & 1) {
        return -(int)(uval >> 1);
    }
    else {
        return uval >> 1;
    }
}

static int
get_line_delta(const uint8_t *ptr)
{
    int code = ((*ptr) >> 3) & 15;
    switch (code) {
        case PY_CODE_LOCATION_INFO_NONE:
            return 0;
        case PY_CODE_LOCATION_INFO_NO_COLUMNS:
        case PY_CODE_LOCATION_INFO_LONG:
            return scan_signed_varint(ptr+1);
        case PY_CODE_LOCATION_INFO_ONE_LINE0:
            return 0;
        case PY_CODE_LOCATION_INFO_ONE_LINE1:
            return 1;
        case PY_CODE_LOCATION_INFO_ONE_LINE2:
            return 2;
        default:
            /* Same line */
            return 0;
    }
}

static PyObject *
remove_column_info(PyObject *locations)
{
    Py_ssize_t offset = 0;
    const uint8_t *data = (const uint8_t *)PyBytes_AS_STRING(locations);
    PyBytesWriter *res = PyBytesWriter_Create(32);
    if (res == NULL) {
        return NULL;
    }
    uint8_t *output = (uint8_t *)PyBytesWriter_GetData(res);
    while (offset < PyBytes_GET_SIZE(locations)) {
        Py_ssize_t write_offset = output - (uint8_t *)PyBytesWriter_GetData(res);
        if (write_offset + 16 >= PyBytesWriter_GetSize(res)) {
            if (PyBytesWriter_Resize(res, PyBytesWriter_GetSize(res) * 2) < 0) {
                PyBytesWriter_Discard(res);
                return NULL;
            }
            output = (uint8_t *)PyBytesWriter_GetData(res) + write_offset;
        }
        int code = (data[offset] >> 3) & 15;
        if (code == PY_CODE_LOCATION_INFO_NONE) {
            *output++ = data[offset];
        }
        else {
            int blength = (data[offset] & 7)+1;
            output += write_location_entry_start(
                output, PY_CODE_LOCATION_INFO_NO_COLUMNS, blength);
            int ldelta = get_line_delta(&data[offset]);
            output += write_signed_varint(output, ldelta);
        }
        offset++;
        while (offset < PyBytes_GET_SIZE(locations) &&
            (data[offset] & 128) == 0) {
            offset++;
        }
    }
    return PyBytesWriter_FinishWithPointer(res, output);
}

static int
intern_code_constants(struct _PyCodeConstructor *con)
{
    if (intern_strings(con->names) < 0) {
        goto error;
    }
    if (intern_constants(con->consts, NULL) < 0) {
        goto error;
    }
    if (intern_strings(con->localsplusnames) < 0) {
        goto error;
    }
    FT_MUTEX_UNLOCK(&state->mutex);
    return 0;

error:
    FT_MUTEX_UNLOCK(&state->mutex);
    return -1;
}

/* The caller is responsible for ensuring that the given data is valid. */

PyCodeObject *
_PyCode_New(struct _PyCodeConstructor *con)
{
    if (intern_code_constants(con) < 0) {
        return NULL;
    }

    PyObject *replacement_locations = NULL;
    // Compact the linetable if we are opted out of debug
    // ranges.
    if (!_Py_GetConfig()->code_debug_ranges) {
        replacement_locations = remove_column_info(con->linetable);
        if (replacement_locations == NULL) {
            return NULL;
        }
        con->linetable = replacement_locations;
    }

    Py_ssize_t size = PyBytes_GET_SIZE(con->code) / sizeof(_PyIRWord);
    // The feedback table follows the words, two entries to an item
    Py_ssize_t nfb = con->fbinit != NULL
        ? PyBytes_GET_SIZE(con->fbinit) / (Py_ssize_t)sizeof(uint32_t) : 0;
    Py_ssize_t nitems = size + (nfb + 1) / 2;
    PyCodeObject *co;
    co = PyObject_NewVar(PyCodeObject, &PyCode_Type, nitems);
    if (co == NULL) {
        Py_XDECREF(replacement_locations);
        PyErr_NoMemory();
        return NULL;
    }
    Py_SET_SIZE(co, size);


    if (init_code(co, con) < 0) {
        Py_XDECREF(replacement_locations);
        Py_DECREF(co);
        return NULL;
    }

    Py_XDECREF(replacement_locations);
    return co;
}


/******************
 * the legacy "constructors"
 ******************/

/* Whether the inline constant handler of word fits the constant it names */
static int
inline_const_fits(_PyIRWord word, PyObject *consts)
{
    Py_ssize_t index = _PyIR_FieldOparg(word);
    if (index >= PyTuple_GET_SIZE(consts)) {
        return 0;
    }
    PyObject *value = PyTuple_GET_ITEM(consts, index);
    if (IR_HANDLER(word) == LOAD_CONST_INLINE_INT) {
        return _PyLong_CheckExactAndCompact(value);
    }
    return PyFloat_CheckExact(value) && !isnan(PyFloat_AS_DOUBLE(value));
}

/* Check the fields of words from outside the compiler and marshal that the
 * interpreter uses as indices without checking them: each handler exists,
 * each slot is in the frame, and each feedback index has an entry, which
 * for an operand kept in the entry is not the shared entry 0.  Returns the
 * words to use, a new reference: code itself, or a copy whose inline
 * constant handlers that no longer fit their constant run LOAD_CONST, as
 * after code.replace(co_consts=...). */
static PyObject *
check_words(PyObject *code, PyObject *fbinit, PyObject *consts, int nslots)
{
    PyObject *result = Py_NewRef(code);
    const char *bytes = PyBytes_AS_STRING(code);
    Py_ssize_t nwords = PyBytes_GET_SIZE(code) / (Py_ssize_t)sizeof(_PyIRWord);
    Py_ssize_t nfb = fbinit == NULL ? 0
        : PyBytes_GET_SIZE(fbinit) / (Py_ssize_t)sizeof(uint32_t);
    for (Py_ssize_t i = 0; i < nwords; i++) {
        _PyIRWord word;
        memcpy(&word, bytes + i * sizeof(_PyIRWord), sizeof(word));
        int handler = IR_HANDLER(word);
        if (handler >= IR_NUM_HANDLERS
            || !(_PyIR_Formats[handler].flags & _PyIR_VALID))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd has no handler %d", i, handler);
            goto error;
        }
        const _PyIRFormat *fmt = &_PyIR_Formats[handler];
        for (int k = 0; k < 4; k++) {
            int role = fmt->roles[k];
            uint32_t value = (uint32_t)(word >> (16 + 12 * k)) & IR_FIELD_MAX;
            if ((role == _PyIR_ROLE_DST || role == _PyIR_ROLE_BASE
                 || role >= _PyIR_ROLE_IN0) && value >= (uint32_t)nslots)
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd names slot %u of %d", i, value,
                             nslots);
                goto error;
            }
            if (role == _PyIR_ROLE_FB
                && (value >= (uint32_t)nfb
                    || (value == 0 && fmt->imm == _PyIR_LOC_FBARG
                        && !(handler >= _PyIR_BINARY_OP_FIRST
                             && handler <= _PyIR_BINARY_OP_FIRST + NB_OPARG_LAST))))
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd has no feedback entry %u", i,
                             value);
                goto error;
            }
        }
        if ((handler == LOAD_CONST_INLINE_INT || handler == LOAD_CONST_INLINE_FLOAT)
            && !inline_const_fits(word, consts))
        {
            if (result == code) {
                Py_SETREF(result, PyBytes_FromStringAndSize(
                    PyBytes_AS_STRING(code), PyBytes_GET_SIZE(code)));
                if (result == NULL) {
                    return NULL;
                }
            }
            _PyIR_SetHandler(&word, LOAD_CONST);
            memcpy(PyBytes_AS_STRING(result) + i * sizeof(_PyIRWord), &word,
                   sizeof(word));
        }
    }
    return result;

error:
    Py_DECREF(result);
    return NULL;
}

/* As PyUnstable_Code_NewWithPosOnlyArgs(), with the feedback table of the
 * words in fbinit, or NULL for none. */
static PyCodeObject *
code_new_with_fbinit(
                          int argcount, int posonlyargcount, int kwonlyargcount,
                          int nlocals, int stacksize, int flags,
                          PyObject *code, PyObject *fbinit,
                          PyObject *consts, PyObject *names,
                          PyObject *varnames, PyObject *freevars, PyObject *cellvars,
                          PyObject *filename, PyObject *name,
                          PyObject *qualname, int firstlineno,
                          PyObject *linetable,
                          PyObject *exceptiontable)
{
    PyCodeObject *co = NULL;
    PyObject *localsplusnames = NULL;
    PyObject *localspluskinds = NULL;
    PyObject *checked = NULL;
    PyObject *padded = NULL;

    if (varnames == NULL || !PyTuple_Check(varnames) ||
        cellvars == NULL || !PyTuple_Check(cellvars) ||
        freevars == NULL || !PyTuple_Check(freevars)
        ) {
        PyErr_BadInternalCall();
        return NULL;
    }

    // Extensions built for 2-byte code units, such as Cython's, pass zeroed
    // code that never runs and only spans the line table.  Pad it to whole
    // words, which are CACHE.
    if (code != NULL && PyBytes_Check(code)
        && PyBytes_GET_SIZE(code) % (Py_ssize_t)sizeof(_PyIRWord) != 0
        && PyBytes_GET_SIZE(code) % 2 == 0)
    {
        Py_ssize_t size = PyBytes_GET_SIZE(code);
        Py_ssize_t words_size = _Py_SIZE_ROUND_UP(size, sizeof(_PyIRWord));
        padded = PyBytes_FromStringAndSize(NULL, words_size);
        if (padded == NULL) {
            return NULL;
        }
        memcpy(PyBytes_AS_STRING(padded), PyBytes_AS_STRING(code), size);
        memset(PyBytes_AS_STRING(padded) + size, 0, words_size - size);
        code = padded;
    }

    // Set the "fast locals plus" info.
    int nvarnames = (int)PyTuple_GET_SIZE(varnames);
    int ncellvars = (int)PyTuple_GET_SIZE(cellvars);
    int nfreevars = (int)PyTuple_GET_SIZE(freevars);
    int nlocalsplus = nvarnames + ncellvars + nfreevars;
    localsplusnames = PyTuple_New(nlocalsplus);
    if (localsplusnames == NULL) {
        goto error;
    }
    localspluskinds = PyBytes_FromStringAndSize(NULL, nlocalsplus);
    if (localspluskinds == NULL) {
        goto error;
    }
    int  offset = 0;
    for (int i = 0; i < nvarnames; i++, offset++) {
        PyObject *name = PyTuple_GET_ITEM(varnames, i);
        _Py_set_localsplus_info(offset, name, CO_FAST_LOCAL,
                               localsplusnames, localspluskinds);
    }
    for (int i = 0; i < ncellvars; i++, offset++) {
        PyObject *name = PyTuple_GET_ITEM(cellvars, i);
        int argoffset = -1;
        for (int j = 0; j < nvarnames; j++) {
            int cmp = PyUnicode_Compare(PyTuple_GET_ITEM(varnames, j),
                                        name);
            assert(!PyErr_Occurred());
            if (cmp == 0) {
                argoffset = j;
                break;
            }
        }
        if (argoffset >= 0) {
            // Merge the localsplus indices.
            nlocalsplus -= 1;
            offset -= 1;
            _PyLocals_Kind kind = _PyLocals_GetKind(localspluskinds, argoffset);
            _PyLocals_SetKind(localspluskinds, argoffset, kind | CO_FAST_CELL);
            continue;
        }
        _Py_set_localsplus_info(offset, name, CO_FAST_CELL,
                               localsplusnames, localspluskinds);
    }
    for (int i = 0; i < nfreevars; i++, offset++) {
        PyObject *name = PyTuple_GET_ITEM(freevars, i);
        _Py_set_localsplus_info(offset, name, CO_FAST_FREE,
                               localsplusnames, localspluskinds);
    }

    // gh-110543: Make sure the CO_FAST_HIDDEN flag is set correctly.
    if (!(flags & CO_OPTIMIZED)) {
        Py_ssize_t nwords = PyBytes_GET_SIZE(code) / sizeof(_PyIRWord);
        const char *words = PyBytes_AS_STRING(code);
        for (Py_ssize_t i = 0; i < nwords; i++) {
            _PyIRWord word;
            memcpy(&word, words + i * sizeof(_PyIRWord), sizeof(word));
            if (IR_HANDLER(word) == LOAD_FAST_AND_CLEAR) {
                int oparg = _PyIR_FieldOparg(word);
                if (oparg >= nlocalsplus) {
                    PyErr_Format(PyExc_ValueError,
                                "code: LOAD_FAST_AND_CLEAR oparg %d out of range",
                                oparg);
                    goto error;
                }
                _PyLocals_Kind kind = _PyLocals_GetKind(localspluskinds, oparg);
                _PyLocals_SetKind(localspluskinds, oparg, kind | CO_FAST_HIDDEN);
            }
        }
    }

    // If any cells were args then nlocalsplus will have shrunk.
    if (nlocalsplus != PyTuple_GET_SIZE(localsplusnames)) {
        if (_PyTuple_Resize(&localsplusnames, nlocalsplus) < 0
                || _PyBytes_Resize(&localspluskinds, nlocalsplus) < 0) {
            goto error;
        }
    }

    struct _PyCodeConstructor con = {
        .filename = filename,
        .name = name,
        .qualname = qualname,
        .flags = flags,

        .code = code,
        .fbinit = fbinit,
        .firstlineno = firstlineno,
        .linetable = linetable,

        .consts = consts,
        .names = names,

        .localsplusnames = localsplusnames,
        .localspluskinds = localspluskinds,

        .argcount = argcount,
        .posonlyargcount = posonlyargcount,
        .kwonlyargcount = kwonlyargcount,

        .stacksize = stacksize,

        .exceptiontable = exceptiontable,
    };

    if (_PyCode_Validate(&con) < 0) {
        goto error;
    }
    checked = check_words(code, fbinit, consts, nlocalsplus + stacksize);
    if (checked == NULL) {
        goto error;
    }
    con.code = checked;
    if (nlocals != PyTuple_GET_SIZE(varnames)) {
        PyErr_SetString(PyExc_ValueError,
                        "code: co_nlocals != len(co_varnames)");
        goto error;
    }

    co = _PyCode_New(&con);
    if (co == NULL) {
        goto error;
    }

error:
    Py_XDECREF(localsplusnames);
    Py_XDECREF(localspluskinds);
    Py_XDECREF(checked);
    Py_XDECREF(padded);
    return co;
}

PyCodeObject *
PyUnstable_Code_NewWithPosOnlyArgs(
                          int argcount, int posonlyargcount, int kwonlyargcount,
                          int nlocals, int stacksize, int flags,
                          PyObject *code, PyObject *consts, PyObject *names,
                          PyObject *varnames, PyObject *freevars, PyObject *cellvars,
                          PyObject *filename, PyObject *name,
                          PyObject *qualname, int firstlineno,
                          PyObject *linetable,
                          PyObject *exceptiontable)
{
    return code_new_with_fbinit(argcount, posonlyargcount, kwonlyargcount,
                                nlocals, stacksize, flags, code, NULL, consts,
                                names, varnames, freevars, cellvars, filename,
                                name, qualname, firstlineno, linetable,
                                exceptiontable);
}

PyCodeObject *
PyUnstable_Code_New(int argcount, int kwonlyargcount,
           int nlocals, int stacksize, int flags,
           PyObject *code, PyObject *consts, PyObject *names,
           PyObject *varnames, PyObject *freevars, PyObject *cellvars,
           PyObject *filename, PyObject *name, PyObject *qualname,
           int firstlineno,
           PyObject *linetable,
           PyObject *exceptiontable)
{
    return PyCode_NewWithPosOnlyArgs(argcount, 0, kwonlyargcount, nlocals,
                                     stacksize, flags, code, consts, names,
                                     varnames, freevars, cellvars, filename,
                                     name, qualname, firstlineno,
                                     linetable,
                                     exceptiontable);
}

// NOTE: When modifying the construction of PyCode_NewEmpty, please also change
// test.test_code.CodeLocationTest.test_code_new_empty to keep it in sync!

// Slot 0 is the only temporary
static const _PyIRWord assert0[3] = {
    IR_WORD(RESUME, RESUME_AT_FUNC_START, 0, 0, 0),
    IR_WORD(LOAD_COMMON_CONSTANT, 0, CONSTANT_ASSERTIONERROR, 0, 0),
    IR_WORD(RAISE_VARARGS, 0, 1, 0, 0),
};

static const uint8_t linetable[2] = {
    (1 << 7)  // New entry.
    | (PY_CODE_LOCATION_INFO_NO_COLUMNS << 3)
    | (3 - 1),  // Three words.
    0,  // Offset from co_firstlineno.
};

PyCodeObject *
PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno)
{
    PyObject *nulltuple = NULL;
    PyObject *filename_ob = NULL;
    PyObject *funcname_ob = NULL;
    PyObject *code_ob = NULL;
    PyObject *linetable_ob = NULL;
    PyCodeObject *result = NULL;

    nulltuple = PyTuple_New(0);
    if (nulltuple == NULL) {
        goto failed;
    }
    funcname_ob = PyUnicode_FromString(funcname);
    if (funcname_ob == NULL) {
        goto failed;
    }
    filename_ob = PyUnicode_DecodeFSDefault(filename);
    if (filename_ob == NULL) {
        goto failed;
    }
    code_ob = PyBytes_FromStringAndSize((const char *)assert0, sizeof(assert0));
    if (code_ob == NULL) {
        goto failed;
    }
    linetable_ob = PyBytes_FromStringAndSize((const char *)linetable, 2);
    if (linetable_ob == NULL) {
        goto failed;
    }

#define emptystring (PyObject *)&_Py_SINGLETON(bytes_empty)
    struct _PyCodeConstructor con = {
        .filename = filename_ob,
        .name = funcname_ob,
        .qualname = funcname_ob,
        .code = code_ob,
        .firstlineno = firstlineno,
        .linetable = linetable_ob,
        .consts = nulltuple,
        .names = nulltuple,
        .localsplusnames = nulltuple,
        .localspluskinds = emptystring,
        .exceptiontable = emptystring,
        .stacksize = 1,
    };
    result = _PyCode_New(&con);

failed:
    Py_XDECREF(nulltuple);
    Py_XDECREF(funcname_ob);
    Py_XDECREF(filename_ob);
    Py_XDECREF(code_ob);
    Py_XDECREF(linetable_ob);
    return result;
}


/******************
 * source location tracking (co_lines/co_positions)
 ******************/

int
PyCode_Addr2Line(PyCodeObject *co, int addrq)
{
    if (addrq < 0) {
        return co->co_firstlineno;
    }
    _PyCoMonitoringData *data = _Py_atomic_load_ptr_acquire(&co->_co_monitoring);
    if (data) {
        _PyCoLineInstrumentationData *lines = _Py_atomic_load_ptr_acquire(&data->lines);
        if (lines) {
            return _Py_Instrumentation_GetLine(co, lines, addrq);
        }
    }
    assert(addrq >= 0 && addrq < Py_SIZE(co));
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(co, &bounds);
    return _PyCode_CheckLineNumber(addrq, &bounds);
}

int
_PyCode_SafeAddr2Line(PyCodeObject *co, int addrq)
{
    if (addrq < 0) {
        return co->co_firstlineno;
    }
    if (co->_co_monitoring && co->_co_monitoring->lines) {
        return _Py_Instrumentation_GetLine(co, co->_co_monitoring->lines, addrq);
    }
    if (!(addrq >= 0 && addrq < Py_SIZE(co))) {
        return -1;
    }
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(co, &bounds);
    return _PyCode_CheckLineNumber(addrq, &bounds);
}

void
_PyLineTable_InitAddressRange(const char *linetable, Py_ssize_t length, int firstlineno, PyCodeAddressRange *range)
{
    range->opaque.lo_next = (const uint8_t *)linetable;
    range->opaque.limit = range->opaque.lo_next + length;
    range->ar_start = -1;
    range->ar_end = 0;
    range->opaque.computed_line = firstlineno;
    range->ar_line = -1;
}

int
_PyCode_InitAddressRange(PyCodeObject* co, PyCodeAddressRange *bounds)
{
    assert(co->co_linetable != NULL);
    const char *linetable = PyBytes_AS_STRING(co->co_linetable);
    Py_ssize_t length = PyBytes_GET_SIZE(co->co_linetable);
    _PyLineTable_InitAddressRange(linetable, length, co->co_firstlineno, bounds);
    return bounds->ar_line;
}

/* Update *bounds to describe the first and one-past-the-last instructions in
   the same line as lasti.  Return the number of that line, or -1 if lasti is out of bounds. */
int
_PyCode_CheckLineNumber(int lasti, PyCodeAddressRange *bounds)
{
    while (bounds->ar_end <= lasti) {
        if (!_PyLineTable_NextAddressRange(bounds)) {
            return -1;
        }
    }
    while (bounds->ar_start > lasti) {
        if (!_PyLineTable_PreviousAddressRange(bounds)) {
            return -1;
        }
    }
    return bounds->ar_line;
}

static int
is_no_line_marker(uint8_t b)
{
    return (b >> 3) == 0x1f;
}


#define ASSERT_VALID_BOUNDS(bounds) \
    assert(bounds->opaque.lo_next <=  bounds->opaque.limit && \
        (bounds->ar_line == -1 || bounds->ar_line == bounds->opaque.computed_line) && \
        (bounds->opaque.lo_next == bounds->opaque.limit || \
        (*bounds->opaque.lo_next) & 128))

static int
next_code_delta(PyCodeAddressRange *bounds)
{
    assert((*bounds->opaque.lo_next) & 128);
    return ((*bounds->opaque.lo_next) & 7) + 1;
}

static int
previous_code_delta(PyCodeAddressRange *bounds)
{
    if (bounds->ar_start == 0) {
        // If we looking at the first entry, the
        // "previous" entry has an implicit length of 1.
        return 1;
    }
    const uint8_t *ptr = bounds->opaque.lo_next-1;
    while (((*ptr) & 128) == 0) {
        ptr--;
    }
    return ((*ptr) & 7) + 1;
}

static int
read_byte(PyCodeAddressRange *bounds)
{
    return *bounds->opaque.lo_next++;
}

static int
read_varint(PyCodeAddressRange *bounds)
{
    unsigned int read = read_byte(bounds);
    unsigned int val = read & 63;
    unsigned int shift = 0;
    while (read & 64) {
        read = read_byte(bounds);
        shift += 6;
        val |= (read & 63) << shift;
    }
    return val;
}

static int
read_signed_varint(PyCodeAddressRange *bounds)
{
    unsigned int uval = read_varint(bounds);
    if (uval & 1) {
        return -(int)(uval >> 1);
    }
    else {
        return uval >> 1;
    }
}

static void
retreat(PyCodeAddressRange *bounds)
{
    ASSERT_VALID_BOUNDS(bounds);
    assert(bounds->ar_start >= 0);
    do {
        bounds->opaque.lo_next--;
    } while (((*bounds->opaque.lo_next) & 128) == 0);
    bounds->opaque.computed_line -= get_line_delta(bounds->opaque.lo_next);
    bounds->ar_end = bounds->ar_start;
    bounds->ar_start -= previous_code_delta(bounds);
    if (is_no_line_marker(bounds->opaque.lo_next[-1])) {
        bounds->ar_line = -1;
    }
    else {
        bounds->ar_line = bounds->opaque.computed_line;
    }
    ASSERT_VALID_BOUNDS(bounds);
}

static void
advance(PyCodeAddressRange *bounds)
{
    ASSERT_VALID_BOUNDS(bounds);
    bounds->opaque.computed_line += get_line_delta(bounds->opaque.lo_next);
    if (is_no_line_marker(*bounds->opaque.lo_next)) {
        bounds->ar_line = -1;
    }
    else {
        bounds->ar_line = bounds->opaque.computed_line;
    }
    bounds->ar_start = bounds->ar_end;
    bounds->ar_end += next_code_delta(bounds);
    do {
        bounds->opaque.lo_next++;
    } while (bounds->opaque.lo_next < bounds->opaque.limit &&
        ((*bounds->opaque.lo_next) & 128) == 0);
    ASSERT_VALID_BOUNDS(bounds);
}

static void
advance_with_locations(PyCodeAddressRange *bounds, int *endline, int *column, int *endcolumn)
{
    ASSERT_VALID_BOUNDS(bounds);
    int first_byte = read_byte(bounds);
    int code = (first_byte >> 3) & 15;
    bounds->ar_start = bounds->ar_end;
    bounds->ar_end = bounds->ar_start + (first_byte & 7) + 1;
    switch(code) {
        case PY_CODE_LOCATION_INFO_NONE:
            bounds->ar_line = *endline = -1;
            *column =  *endcolumn = -1;
            break;
        case PY_CODE_LOCATION_INFO_LONG:
        {
            bounds->opaque.computed_line += read_signed_varint(bounds);
            bounds->ar_line = bounds->opaque.computed_line;
            *endline = bounds->ar_line + read_varint(bounds);
            *column = read_varint(bounds)-1;
            *endcolumn = read_varint(bounds)-1;
            break;
        }
        case PY_CODE_LOCATION_INFO_NO_COLUMNS:
        {
            /* No column */
            bounds->opaque.computed_line += read_signed_varint(bounds);
            *endline = bounds->ar_line = bounds->opaque.computed_line;
            *column = *endcolumn = -1;
            break;
        }
        case PY_CODE_LOCATION_INFO_ONE_LINE0:
        case PY_CODE_LOCATION_INFO_ONE_LINE1:
        case PY_CODE_LOCATION_INFO_ONE_LINE2:
        {
            /* one line form */
            int line_delta = code - 10;
            bounds->opaque.computed_line += line_delta;
            *endline = bounds->ar_line = bounds->opaque.computed_line;
            *column = read_byte(bounds);
            *endcolumn = read_byte(bounds);
            break;
        }
        default:
        {
            /* Short forms */
            int second_byte = read_byte(bounds);
            assert((second_byte & 128) == 0);
            *endline = bounds->ar_line = bounds->opaque.computed_line;
            *column = code << 3 | (second_byte >> 4);
            *endcolumn = *column + (second_byte & 15);
        }
    }
    ASSERT_VALID_BOUNDS(bounds);
}
int
PyCode_Addr2Location(PyCodeObject *co, int addrq,
                     int *start_line, int *start_column,
                     int *end_line, int *end_column)
{
    if (addrq < 0) {
        *start_line = *end_line = co->co_firstlineno;
        *start_column = *end_column = 0;
        return 1;
    }
    assert(addrq >= 0 && addrq < Py_SIZE(co));
    PyCodeAddressRange bounds;
    _PyCode_InitAddressRange(co, &bounds);
    _PyCode_CheckLineNumber(addrq, &bounds);
    retreat(&bounds);
    advance_with_locations(&bounds, end_line, start_column, end_column);
    *start_line = bounds.ar_line;
    return 1;
}


static inline int
at_end(PyCodeAddressRange *bounds) {
    return bounds->opaque.lo_next >= bounds->opaque.limit;
}

int
_PyLineTable_PreviousAddressRange(PyCodeAddressRange *range)
{
    if (range->ar_start <= 0) {
        return 0;
    }
    retreat(range);
    assert(range->ar_end > range->ar_start);
    return 1;
}

int
_PyLineTable_NextAddressRange(PyCodeAddressRange *range)
{
    if (at_end(range)) {
        return 0;
    }
    advance(range);
    assert(range->ar_end > range->ar_start);
    return 1;
}


typedef struct {
    PyObject_HEAD
    PyCodeObject *li_code;
    PyCodeAddressRange li_line;
} lineiterator;


static void
lineiter_dealloc(PyObject *self)
{
    lineiterator *li = (lineiterator*)self;
    Py_DECREF(li->li_code);
    Py_TYPE(li)->tp_free(li);
}

static PyObject *
_source_offset_converter(void *arg) {
    int *value = (int*)arg;
    if (*value == -1) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(*value);
}

static PyValue
lineiter_next(PyObject *self)
{
    lineiterator *li = (lineiterator*)self;
    PyCodeAddressRange *bounds = &li->li_line;
    if (!_PyLineTable_NextAddressRange(bounds)) {
        return PyValue_NULL;
    }
    int start = bounds->ar_start;
    int line = bounds->ar_line;
    // Merge overlapping entries:
    while (_PyLineTable_NextAddressRange(bounds)) {
        if (bounds->ar_line != line) {
            _PyLineTable_PreviousAddressRange(bounds);
            break;
        }
    }
    return PyValue_FromResult(Py_BuildValue("iiO&", start, bounds->ar_end,
                         _source_offset_converter, &line));
}

static int
lineiter_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((lineiterator *)self)->li_code);
    return 0;
}

PyTypeObject _PyLineIterator = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "line_iterator",                    /* tp_name */
    sizeof(lineiterator),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    lineiter_dealloc,                   /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
    0,                                  /* tp_doc */
    lineiter_traverse,                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    lineiter_next,                      /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    PyObject_Free,                      /* tp_free */
};

static lineiterator *
new_linesiterator(PyCodeObject *code)
{
    lineiterator *li = (lineiterator *)PyType_GenericAlloc(&_PyLineIterator, 0);
    if (li == NULL) {
        return NULL;
    }
    li->li_code = (PyCodeObject*)Py_NewRef(code);
    _PyCode_InitAddressRange(code, &li->li_line);
    return li;
}

/* co_positions iterator object. */
typedef struct {
    PyObject_HEAD
    PyCodeObject* pi_code;
    PyCodeAddressRange pi_range;
    int pi_offset;
    int pi_endline;
    int pi_column;
    int pi_endcolumn;
} positionsiterator;

static void
positionsiter_dealloc(PyObject *self)
{
    positionsiterator *pi = (positionsiterator*)self;
    Py_DECREF(pi->pi_code);
    Py_TYPE(pi)->tp_free(pi);
}

static PyValue
positionsiter_next(PyObject *self)
{
    positionsiterator *pi = (positionsiterator*)self;
    if (pi->pi_offset >= pi->pi_range.ar_end) {
        assert(pi->pi_offset == pi->pi_range.ar_end);
        if (at_end(&pi->pi_range)) {
            return PyValue_NULL;
        }
        advance_with_locations(&pi->pi_range, &pi->pi_endline, &pi->pi_column, &pi->pi_endcolumn);
    }
    pi->pi_offset += 1;
    return PyValue_FromResult(Py_BuildValue("(O&O&O&O&)",
        _source_offset_converter, &pi->pi_range.ar_line,
        _source_offset_converter, &pi->pi_endline,
        _source_offset_converter, &pi->pi_column,
        _source_offset_converter, &pi->pi_endcolumn));
}

static int
positionsiter_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((positionsiterator *)self)->pi_code);
    return 0;
}

PyTypeObject _PyPositionsIterator = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "positions_iterator",               /* tp_name */
    sizeof(positionsiterator),          /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    positionsiter_dealloc,              /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,       /* tp_flags */
    0,                                  /* tp_doc */
    positionsiter_traverse,             /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    positionsiter_next,                 /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    PyObject_Free,                      /* tp_free */
};

static PyObject*
code_positionsiterator(PyObject *self, PyObject* Py_UNUSED(args))
{
    PyCodeObject *code = (PyCodeObject*)self;
    positionsiterator* pi = (positionsiterator*)PyType_GenericAlloc(&_PyPositionsIterator, 0);
    if (pi == NULL) {
        return NULL;
    }
    pi->pi_code = (PyCodeObject*)Py_NewRef(code);
    _PyCode_InitAddressRange(code, &pi->pi_range);
    pi->pi_offset = pi->pi_range.ar_end;
    return (PyObject*)pi;
}


/******************
 * "extra" frame eval info (see PEP 523)
 ******************/

/* Holder for co_extra information */
typedef struct {
    Py_ssize_t ce_size;
    void *ce_extras[1];
} _PyCodeObjectExtra;


static inline size_t
code_extra_size(Py_ssize_t n)
{
    return sizeof(_PyCodeObjectExtra) + (n - 1) * sizeof(void *);
}

static int
code_extra_grow_gil(PyCodeObject *co, _PyCodeObjectExtra *old_co_extra,
                    Py_ssize_t old_ce_size, Py_ssize_t new_ce_size,
                    Py_ssize_t index, void *extra)
{
    _PyCodeObjectExtra *new_co_extra = PyMem_Realloc(
        old_co_extra, code_extra_size(new_ce_size));
    if (new_co_extra == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    for (Py_ssize_t i = old_ce_size; i < new_ce_size; i++) {
        new_co_extra->ce_extras[i] = NULL;
    }
    new_co_extra->ce_size = new_ce_size;
    new_co_extra->ce_extras[index] = extra;
    co->co_extra = new_co_extra;
    return 0;
}

int
PyUnstable_Code_GetExtra(PyObject *code, Py_ssize_t index, void **extra)
{
    if (!PyCode_Check(code)) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyCodeObject *co = (PyCodeObject *)code;
    *extra = NULL;

    if (index < 0) {
        return 0;
    }

    // Lock-free read; pairs with release stores in SetExtra.
    _PyCodeObjectExtra *co_extra = FT_ATOMIC_LOAD_PTR_ACQUIRE(co->co_extra);
    if (co_extra != NULL && index < co_extra->ce_size) {
        *extra = FT_ATOMIC_LOAD_PTR_ACQUIRE(co_extra->ce_extras[index]);
    }

    return 0;
}


int
PyUnstable_Code_SetExtra(PyObject *code, Py_ssize_t index, void *extra)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

    // co_extra_user_count is monotonically increasing and published with
    // release store in RequestCodeExtraIndex, so once an index is valid
    // it stays valid.
    Py_ssize_t user_count = FT_ATOMIC_LOAD_SSIZE_ACQUIRE(
        interp->co_extra_user_count);

    if (!PyCode_Check(code) || index < 0 || index >= user_count) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyCodeObject *co = (PyCodeObject *)code;
    int result = 0;
    void *old_slot_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(co);

    _PyCodeObjectExtra *old_co_extra = (_PyCodeObjectExtra *)co->co_extra;
    Py_ssize_t old_ce_size = (old_co_extra == NULL)
        ? 0 : old_co_extra->ce_size;

    // Fast path: slot already exists, update in place.
    if (index < old_ce_size) {
        old_slot_value = old_co_extra->ce_extras[index];
        FT_ATOMIC_STORE_PTR_RELEASE(old_co_extra->ce_extras[index], extra);
        goto done;
    }

    // Slow path: buffer needs to grow.
    Py_ssize_t new_ce_size = user_count;
    // GIL build: grow with realloc.
    result = code_extra_grow_gil(
        co, old_co_extra, old_ce_size, new_ce_size, index, extra);

done:;
    Py_END_CRITICAL_SECTION();
    if (old_slot_value != NULL) {
        // Free the old slot value if a free function was registered.
        // The caller must ensure no other thread can still access the old
        // value after this overwrite.
        freefunc free_extra = interp->co_extra_freefuncs[index];
        if (free_extra != NULL) {
            free_extra(old_slot_value);
        }
    }

    return result;
}


/******************
 * other PyCodeObject accessor functions
 ******************/

static PyObject *
get_cached_locals(PyCodeObject *co, PyObject **cached_field,
    _PyLocals_Kind kind, int num)
{
    assert(cached_field != NULL);
    assert(co->_co_cached != NULL);
    PyObject *varnames = FT_ATOMIC_LOAD_PTR(*cached_field);
    if (varnames != NULL) {
        return Py_NewRef(varnames);
    }

    Py_BEGIN_CRITICAL_SECTION(co);
    varnames = *cached_field;
    if (varnames == NULL) {
        varnames = get_localsplus_names(co, kind, num);
        if (varnames != NULL) {
            FT_ATOMIC_STORE_PTR(*cached_field, varnames);
        }
    }
    Py_END_CRITICAL_SECTION();
    return Py_XNewRef(varnames);
}

PyObject *
_PyCode_GetVarnames(PyCodeObject *co)
{
    if (init_co_cached(co)) {
        return NULL;
    }
    return get_cached_locals(co, &co->_co_cached->_co_varnames, CO_FAST_LOCAL, co->co_nlocals);
}

PyObject *
PyCode_GetVarnames(PyCodeObject *code)
{
    return _PyCode_GetVarnames(code);
}

PyObject *
_PyCode_GetCellvars(PyCodeObject *co)
{
    if (init_co_cached(co)) {
        return NULL;
    }
    return get_cached_locals(co, &co->_co_cached->_co_cellvars, CO_FAST_CELL, co->co_ncellvars);
}

PyObject *
PyCode_GetCellvars(PyCodeObject *code)
{
    return _PyCode_GetCellvars(code);
}

PyObject *
_PyCode_GetFreevars(PyCodeObject *co)
{
    if (init_co_cached(co)) {
        return NULL;
    }
    return get_cached_locals(co, &co->_co_cached->_co_freevars, CO_FAST_FREE, co->co_nfreevars);
}

PyObject *
PyCode_GetFreevars(PyCodeObject *code)
{
    return _PyCode_GetFreevars(code);
}


// We may want to move these macros to pycore_opcode_utils.h
// and use them in Python/bytecodes.c.
#define LOAD_GLOBAL_NAME_INDEX(oparg) ((oparg)>>1)
#define LOAD_ATTR_NAME_INDEX(oparg) ((oparg)>>1)

#ifndef Py_DEBUG
#define GETITEM(v, i) PyTuple_GET_ITEM((v), (i))
#else
static inline PyObject *
GETITEM(PyObject *v, Py_ssize_t i)
{
    assert(PyTuple_Check(v));
    assert(i >= 0);
    assert(i < PyTuple_GET_SIZE(v));
    assert(PyTuple_GET_ITEM(v, i) != NULL);
    return PyTuple_GET_ITEM(v, i);
}
#endif

static int
identify_unbound_names(PyThreadState *tstate, PyCodeObject *co,
                       PyObject *globalnames, PyObject *attrnames,
                       PyObject *globalsns, PyObject *builtinsns,
                       struct co_unbound_counts *counts, int *p_numdupes)
{
    // This function is inspired by inspect.getclosurevars().
    // It would be nicer if we had something similar to co_localspluskinds,
    // but for co_names.
    assert(globalnames != NULL);
    assert(PySet_Check(globalnames));
    assert(PySet_GET_SIZE(globalnames) == 0 || counts != NULL);
    assert(attrnames != NULL);
    assert(PySet_Check(attrnames));
    assert(PySet_GET_SIZE(attrnames) == 0 || counts != NULL);
    assert(globalsns == NULL || PyAnyDict_Check(globalsns));
    assert(builtinsns == NULL || PyDict_Check(builtinsns));
    assert(counts == NULL || counts->total == 0);
    struct co_unbound_counts unbound = {0};
    int numdupes = 0;
    Py_ssize_t len = Py_SIZE(co);
    for (int i = 0; i < len; i++) {
        int opcode = _PyIR_BaseOpcode(_PyCode_InstrHandler(co, i));
        if (opcode == LOAD_ATTR) {
            int oparg = _PyCode_InstrOparg(co, i);
            int index = LOAD_ATTR_NAME_INDEX(oparg);
            PyObject *name = GETITEM(co->co_names, index);
            if (PySet_Contains(attrnames, name)) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                continue;
            }
            unbound.total += 1;
            unbound.numattrs += 1;
            if (PySet_Add(attrnames, name) < 0) {
                return -1;
            }
            if (PySet_Contains(globalnames, name)) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                numdupes += 1;
            }
        }
        else if (opcode == LOAD_GLOBAL) {
            int oparg = _PyCode_InstrOparg(co, i);
            int index = LOAD_ATTR_NAME_INDEX(oparg);
            PyObject *name = GETITEM(co->co_names, index);
            if (PySet_Contains(globalnames, name)) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                continue;
            }
            unbound.total += 1;
            unbound.globals.total += 1;
            if (globalsns != NULL && PyDict_Contains(globalsns, name)) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                unbound.globals.numglobal += 1;
            }
            else if (builtinsns != NULL && PyDict_Contains(builtinsns, name)) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                unbound.globals.numbuiltin += 1;
            }
            else {
                unbound.globals.numunknown += 1;
            }
            if (PySet_Add(globalnames, name) < 0) {
                return -1;
            }
            if (PySet_Contains(attrnames, name)) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                numdupes += 1;
            }
        }
    }
    if (counts != NULL) {
        *counts = unbound;
    }
    if (p_numdupes != NULL) {
        *p_numdupes = numdupes;
    }
    return 0;
}


void
_PyCode_GetVarCounts(PyCodeObject *co, _PyCode_var_counts_t *counts)
{
    assert(counts != NULL);

    // Count the locals, cells, and free vars.
    struct co_locals_counts locals = {0};
    int numfree = 0;
    PyObject *kinds = co->co_localspluskinds;
    Py_ssize_t numlocalplusfree = PyBytes_GET_SIZE(kinds);
    for (int i = 0; i < numlocalplusfree; i++) {
        _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);
        if (kind & CO_FAST_FREE) {
            assert(!(kind & CO_FAST_LOCAL));
            assert(!(kind & CO_FAST_HIDDEN));
            assert(!(kind & CO_FAST_ARG));
            numfree += 1;
        }
        else {
            // Apparently not all non-free vars a CO_FAST_LOCAL.
            assert(kind);
            locals.total += 1;
            if (kind & CO_FAST_ARG) {
                locals.args.total += 1;
                if (kind & CO_FAST_ARG_VAR) {
                    if (kind & CO_FAST_ARG_POS) {
                        assert(!(kind & CO_FAST_ARG_KW));
                        assert(!locals.args.varargs);
                        locals.args.varargs = 1;
                    }
                    else {
                        assert(kind & CO_FAST_ARG_KW);
                        assert(!locals.args.varkwargs);
                        locals.args.varkwargs = 1;
                    }
                }
                else if (kind & CO_FAST_ARG_POS) {
                    if (kind & CO_FAST_ARG_KW) {
                        locals.args.numposorkw += 1;
                    }
                    else {
                        locals.args.numposonly += 1;
                    }
                }
                else {
                    assert(kind & CO_FAST_ARG_KW);
                    locals.args.numkwonly += 1;
                }
                if (kind & CO_FAST_CELL) {
                    locals.cells.total += 1;
                    locals.cells.numargs += 1;
                }
                // Args are never hidden currently.
                assert(!(kind & CO_FAST_HIDDEN));
            }
            else {
                if (kind & CO_FAST_CELL) {
                    locals.cells.total += 1;
                    locals.cells.numothers += 1;
                    if (kind & CO_FAST_HIDDEN) {
                        locals.hidden.total += 1;
                        locals.hidden.numcells += 1;
                    }
                }
                else {
                    locals.numpure += 1;
                    if (kind & CO_FAST_HIDDEN) {
                        locals.hidden.total += 1;
                        locals.hidden.numpure += 1;
                    }
                }
            }
        }
    }
    assert(locals.args.total == (
            co->co_argcount + co->co_kwonlyargcount
            + !!(co->co_flags & CO_VARARGS)
            + !!(co->co_flags & CO_VARKEYWORDS)));
    assert(locals.args.numposonly == co->co_posonlyargcount);
    assert(locals.args.numposonly + locals.args.numposorkw == co->co_argcount);
    assert(locals.args.numkwonly == co->co_kwonlyargcount);
    assert(locals.cells.total == co->co_ncellvars);
    assert(locals.args.total + locals.numpure == co->co_nlocals);
    assert(locals.total + locals.cells.numargs == co->co_nlocals + co->co_ncellvars);
    assert(locals.total + numfree == co->co_nlocalsplus);
    assert(numfree == co->co_nfreevars);

    // Get the unbound counts.
    assert(PyTuple_GET_SIZE(co->co_names) >= 0);
    assert(PyTuple_GET_SIZE(co->co_names) < INT_MAX);
    int numunbound = (int)PyTuple_GET_SIZE(co->co_names);
    struct co_unbound_counts unbound = {
        .total = numunbound,
        // numglobal and numattrs can be set later
        // with _PyCode_SetUnboundVarCounts().
        .numunknown = numunbound,
    };

    // "Return" the result.
    *counts = (_PyCode_var_counts_t){
        .total = locals.total + numfree + unbound.total,
        .locals = locals,
        .numfree = numfree,
        .unbound = unbound,
    };
}

int
_PyCode_SetUnboundVarCounts(PyThreadState *tstate,
                            PyCodeObject *co, _PyCode_var_counts_t *counts,
                            PyObject *globalnames, PyObject *attrnames,
                            PyObject *globalsns, PyObject *builtinsns)
{
    int res = -1;
    PyObject *globalnames_owned = NULL;
    PyObject *attrnames_owned = NULL;

    // Prep the name sets.
    if (globalnames == NULL) {
        globalnames_owned = PySet_New(NULL);
        if (globalnames_owned == NULL) {
            goto finally;
        }
        globalnames = globalnames_owned;
    }
    else if (!PySet_Check(globalnames)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                     "expected a set for \"globalnames\", got %R", globalnames);
        goto finally;
    }
    if (attrnames == NULL) {
        attrnames_owned = PySet_New(NULL);
        if (attrnames_owned == NULL) {
            goto finally;
        }
        attrnames = attrnames_owned;
    }
    else if (!PySet_Check(attrnames)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                     "expected a set for \"attrnames\", got %R", attrnames);
        goto finally;
    }

    // Fill in unbound.globals and unbound.numattrs.
    struct co_unbound_counts unbound = {0};
    int numdupes = 0;
    Py_BEGIN_CRITICAL_SECTION(co);
    res = identify_unbound_names(
            tstate, co, globalnames, attrnames, globalsns, builtinsns,
            &unbound, &numdupes);
    Py_END_CRITICAL_SECTION();
    if (res < 0) {
        goto finally;
    }
    assert(unbound.numunknown == 0);
    assert(unbound.total - numdupes <= counts->unbound.total);
    assert(counts->unbound.numunknown == counts->unbound.total);
    // There may be a name that is both a global and an attr.
    int totalunbound = counts->unbound.total + numdupes;
    unbound.numunknown = totalunbound - unbound.total;
    unbound.total = totalunbound;
    counts->unbound = unbound;
    counts->total += numdupes;
    res = 0;

finally:
    Py_XDECREF(globalnames_owned);
    Py_XDECREF(attrnames_owned);
    return res;
}


int
_PyCode_CheckNoInternalState(PyCodeObject *co, const char **p_errmsg)
{
    const char *errmsg = NULL;
    // We don't worry about co_instrumentation or co_monitoring.
    // They are essentially ephemeral.
    if (co->co_extra != NULL) {
        errmsg = "only basic code objects are supported";
    }

    if (errmsg != NULL) {
        if (p_errmsg != NULL) {
            *p_errmsg = errmsg;
        }
        return 0;
    }
    return 1;
}

int
_PyCode_CheckNoExternalState(PyCodeObject *co, _PyCode_var_counts_t *counts,
                             const char **p_errmsg)
{
    const char *errmsg = NULL;
    if (counts->numfree > 0) {  // It's a closure.
        errmsg = "closures not supported";
    }
    else if (counts->unbound.globals.numglobal > 0) {
        errmsg = "globals not supported";
    }
    else if (counts->unbound.globals.numbuiltin > 0
             && counts->unbound.globals.numunknown > 0)
    {
        errmsg = "globals not supported";
    }
    // Otherwise we don't check counts.unbound.globals.numunknown since we can't
    // distinguish beween globals and builtins here.

    if (errmsg != NULL) {
        if (p_errmsg != NULL) {
            *p_errmsg = errmsg;
        }
        return 0;
    }
    return 1;
}

int
_PyCode_VerifyStateless(PyThreadState *tstate,
                        PyCodeObject *co, PyObject *globalnames,
                        PyObject *globalsns, PyObject *builtinsns)
{
    const char *errmsg;
   _PyCode_var_counts_t counts = {0};
    _PyCode_GetVarCounts(co, &counts);
    if (_PyCode_SetUnboundVarCounts(
                            tstate, co, &counts, globalnames, NULL,
                            globalsns, builtinsns) < 0)
    {
        return -1;
    }
    // We may consider relaxing the internal state constraints
    // if it becomes a problem.
    if (!_PyCode_CheckNoInternalState(co, &errmsg)) {
        _PyErr_SetString(tstate, PyExc_ValueError, errmsg);
        return -1;
    }
    if (builtinsns != NULL) {
        // Make sure the next check will fail for globals,
        // even if there aren't any builtins.
        counts.unbound.globals.numbuiltin += 1;
    }
    if (!_PyCode_CheckNoExternalState(co, &counts, &errmsg)) {
        _PyErr_SetString(tstate, PyExc_ValueError, errmsg);
        return -1;
    }
    // Note that we don't check co->co_flags & CO_NESTED for anything here.
    return 0;
}


int
_PyCode_CheckPureFunction(PyCodeObject *co, const char **p_errmsg)
{
    const char *errmsg = NULL;
    if (co->co_flags & CO_GENERATOR) {
        errmsg = "generators not supported";
    }
    else if (co->co_flags & CO_COROUTINE) {
        errmsg = "coroutines not supported";
    }
    else if (co->co_flags & CO_ITERABLE_COROUTINE) {
        errmsg = "coroutines not supported";
    }
    else if (co->co_flags & CO_ASYNC_GENERATOR) {
        errmsg = "generators not supported";
    }

    if (errmsg != NULL) {
        if (p_errmsg != NULL) {
            *p_errmsg = errmsg;
        }
        return 0;
    }
    return 1;
}

/* Here "value" means a non-None value, since a bare return is identical
 * to returning None explicitly.  Likewise a missing return statement
 * at the end of the function is turned into "return None". */
static int
code_returns_only_none(PyCodeObject *co)
{
    if (!_PyCode_CheckPureFunction(co, NULL)) {
        return 0;
    }
    int len = (int)Py_SIZE(co);
    assert(len > 0);

    // Look up None in co_consts.
    Py_ssize_t nconsts = PyTuple_Size(co->co_consts);
    int none_index = 0;
    for (; none_index < nconsts; none_index++) {
        if (PyTuple_GET_ITEM(co->co_consts, none_index) == Py_None) {
            break;
        }
    }
    for (int i = 0; i < len; i++) {
        if (!IS_RETURN_OPCODE(_PyIR_BaseOpcode(_PyCode_InstrHandler(co, i)))) {
            continue;
        }
        assert(i != 0);
        int prev = _PyIR_BaseOpcode(_PyCode_InstrHandler(co, i - 1));
        int prev_oparg = _PyCode_InstrOparg(co, i - 1);
        if (prev == LOAD_COMMON_CONSTANT && prev_oparg == CONSTANT_NONE) {
            continue;
        }
        if (none_index < nconsts && prev == LOAD_CONST
            && prev_oparg == none_index)
        {
            continue;
        }
        return 0;
    }
    return 1;
}

int
_PyCode_ReturnsOnlyNone(PyCodeObject *co)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(co);
    res = code_returns_only_none(co);
    Py_END_CRITICAL_SECTION();
    return res;
}


PyObject *
_PyCode_GetCode(PyCodeObject *co)
{
    if (init_co_cached(co)) {
        return NULL;
    }

    _PyCoCached *cached = co->_co_cached;
    PyObject *code = FT_ATOMIC_LOAD_PTR(cached->_co_code);
    if (code != NULL) {
        return Py_NewRef(code);
    }

    Py_BEGIN_CRITICAL_SECTION(co);
    code = cached->_co_code;
    if (code == NULL) {
        code = PyBytes_FromStringAndSize((const char *)co->co_words,
                                         _PyCode_NBYTES(co));
        if (code != NULL) {
            assert(cached->_co_code == NULL);
            FT_ATOMIC_STORE_PTR(cached->_co_code, code);
        }
    }
    Py_END_CRITICAL_SECTION();
    return Py_XNewRef(code);
}

PyObject *
PyCode_GetCode(PyCodeObject *co)
{
    return _PyCode_GetCode(co);
}

/******************
 * PyCode_Type
 ******************/

/*[clinic input]
class code "PyCodeObject *" "&PyCode_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=78aa5d576683bb4b]*/

/*[clinic input]
@classmethod
code.__new__ as code_new

    argcount: int
    posonlyargcount: int
    kwonlyargcount: int
    nlocals: int
    stacksize: int
    flags: int
    codestring as code: object(subclass_of="&PyBytes_Type")
    constants as consts: object(subclass_of="&PyTuple_Type")
    names: object(subclass_of="&PyTuple_Type")
    varnames: object(subclass_of="&PyTuple_Type")
    filename: unicode
    name: unicode
    qualname: unicode
    firstlineno: int
    linetable: object(subclass_of="&PyBytes_Type")
    exceptiontable: object(subclass_of="&PyBytes_Type")
    freevars: object(subclass_of="&PyTuple_Type", c_default="NULL") = ()
    cellvars: object(subclass_of="&PyTuple_Type", c_default="NULL") = ()
    fbinit: object(subclass_of="&PyBytes_Type", c_default="NULL") = b''
    /

Create a code object.  Not for the faint of heart.
[clinic start generated code]*/

static PyObject *
code_new_impl(PyTypeObject *type, int argcount, int posonlyargcount,
              int kwonlyargcount, int nlocals, int stacksize, int flags,
              PyObject *code, PyObject *consts, PyObject *names,
              PyObject *varnames, PyObject *filename, PyObject *name,
              PyObject *qualname, int firstlineno, PyObject *linetable,
              PyObject *exceptiontable, PyObject *freevars,
              PyObject *cellvars, PyObject *fbinit)
/*[clinic end generated code: output=9e0481ed272aa222 input=9534933ed7e73334]*/
{
    PyObject *co = NULL;
    PyObject *ournames = NULL;
    PyObject *ourvarnames = NULL;
    PyObject *ourfreevars = NULL;
    PyObject *ourcellvars = NULL;

    if (PySys_Audit("code.__new__", "OOOiiiiii",
                    code, filename, name, argcount, posonlyargcount,
                    kwonlyargcount, nlocals, stacksize, flags) < 0) {
        goto cleanup;
    }

    if (argcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: argcount must not be negative");
        goto cleanup;
    }

    if (posonlyargcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: posonlyargcount must not be negative");
        goto cleanup;
    }

    if (kwonlyargcount < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: kwonlyargcount must not be negative");
        goto cleanup;
    }
    if (nlocals < 0) {
        PyErr_SetString(
            PyExc_ValueError,
            "code: nlocals must not be negative");
        goto cleanup;
    }

    ournames = validate_and_copy_tuple(names);
    if (ournames == NULL)
        goto cleanup;
    ourvarnames = validate_and_copy_tuple(varnames);
    if (ourvarnames == NULL)
        goto cleanup;
    if (freevars)
        ourfreevars = validate_and_copy_tuple(freevars);
    else
        ourfreevars = PyTuple_New(0);
    if (ourfreevars == NULL)
        goto cleanup;
    if (cellvars)
        ourcellvars = validate_and_copy_tuple(cellvars);
    else
        ourcellvars = PyTuple_New(0);
    if (ourcellvars == NULL)
        goto cleanup;

    co = (PyObject *)code_new_with_fbinit(argcount, posonlyargcount,
                                          kwonlyargcount,
                                          nlocals, stacksize, flags,
                                          code, fbinit, consts, ournames,
                                          ourvarnames, ourfreevars,
                                          ourcellvars, filename,
                                          name, qualname, firstlineno,
                                          linetable,
                                          exceptiontable
                                         );
  cleanup:
    Py_XDECREF(ournames);
    Py_XDECREF(ourvarnames);
    Py_XDECREF(ourfreevars);
    Py_XDECREF(ourcellvars);
    return co;
}

static void
free_monitoring_data(_PyCoMonitoringData *data)
{
    if (data == NULL) {
        return;
    }
    if (data->tools) {
        PyMem_Free(data->tools);
    }
    if (data->lines) {
        PyMem_Free(data->lines);
    }
    if (data->line_tools) {
        PyMem_Free(data->line_tools);
    }
    if (data->per_instruction_opcodes) {
        PyMem_Free(data->per_instruction_opcodes);
    }
    if (data->per_instruction_tools) {
        PyMem_Free(data->per_instruction_tools);
    }
    PyMem_Free(data);
}

static void
free_code_extras(PyCodeObject *co)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyCodeObjectExtra *co_extra = co->co_extra;
    co->co_extra = NULL;
    for (Py_ssize_t i = 0; i < co_extra->ce_size; i++) {
        freefunc free_extra = interp->co_extra_freefuncs[i];

        if (free_extra != NULL) {
            free_extra(co_extra->ce_extras[i]);
        }
    }
    PyMem_Free(co_extra);
}

int
_PyCode_HasExtraFree(PyObject *op)
{
    PyCodeObject *co = _PyCodeObject_CAST(op);
    if (co->co_extra == NULL) {
        return 0;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyCodeObjectExtra *co_extra = co->co_extra;
    for (Py_ssize_t i = 0; i < co_extra->ce_size; i++) {
        if (interp->co_extra_freefuncs[i] != NULL) {
            return 1;
        }
    }
    return 0;
}

void
_PyCode_Destroy(PyObject *op)
{
    PyCodeObject *co = _PyCodeObject_CAST(op);
    notify_code_watchers(PY_CODE_EVENT_DESTROY, co);
    // A watcher that keeps co leaves it alive without its extras; the free
    // functions are arbitrary C and may run code, so they cannot wait for
    // the sweep.
    if (co->co_extra != NULL) {
        free_code_extras(co);
    }
}

static void
code_dealloc(PyObject *self)
{
    PyThreadState *tstate = PyThreadState_GET();
    _Py_atomic_add_uint64(&tstate->interp->_code_object_generation, 1);
    PyCodeObject *co = _PyCodeObject_CAST(self);


    _PyFunction_ClearCodeByVersion(co->co_version);
    if (co->co_extra != NULL) {
        free_code_extras(co);
    }

    Py_XDECREF(co->co_consts);
    Py_XDECREF(co->co_names);
    Py_XDECREF(co->co_localsplusnames);
    Py_XDECREF(co->co_localspluskinds);
    Py_XDECREF(co->co_filename);
    Py_XDECREF(co->co_name);
    Py_XDECREF(co->co_qualname);
    Py_XDECREF(co->co_linetable);
    Py_XDECREF(co->co_exceptiontable);
    if (co->_co_cached != NULL) {
        Py_XDECREF(co->_co_cached->_co_code);
        Py_XDECREF(co->_co_cached->_co_cellvars);
        Py_XDECREF(co->_co_cached->_co_freevars);
        Py_XDECREF(co->_co_cached->_co_varnames);
        PyMem_Free(co->_co_cached);
    }
    FT_CLEAR_WEAKREFS(self);
    free_monitoring_data(co->_co_monitoring);
    _PyCode_ClearExec(co);
    PyObject_Free(co);
}

static int
code_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyCodeObject *co = _PyCodeObject_CAST(self);
    Py_VISIT(co->co_consts);
    /* Code objects are not GC objects in the default build, so the cycle
       collector never calls this; the tracing collector (Python/gc_trace.c)
       does, and must see every reference that the code object owns. */
    Py_VISIT(co->co_names);
    Py_VISIT(co->co_localsplusnames);
    Py_VISIT(co->co_localspluskinds);
    Py_VISIT(co->co_filename);
    Py_VISIT(co->co_name);
    Py_VISIT(co->co_qualname);
    Py_VISIT(co->co_linetable);
    Py_VISIT(co->co_exceptiontable);
    if (co->_co_cached != NULL) {
        Py_VISIT(co->_co_cached->_co_code);
        Py_VISIT(co->_co_cached->_co_cellvars);
        Py_VISIT(co->_co_cached->_co_freevars);
        Py_VISIT(co->_co_cached->_co_varnames);
    }
    return 0;
}

static PyObject *
code_repr(PyObject *self)
{
    PyCodeObject *co = _PyCodeObject_CAST(self);
    int lineno;
    if (co->co_firstlineno != 0)
        lineno = co->co_firstlineno;
    else
        lineno = -1;
    if (co->co_filename && PyUnicode_Check(co->co_filename)) {
        return PyUnicode_FromFormat(
            "<code object %U at %p, file \"%U\", line %d>",
            co->co_name, co, co->co_filename, lineno);
    } else {
        return PyUnicode_FromFormat(
            "<code object %U at %p, file ???, line %d>",
            co->co_name, co, lineno);
    }
}

static PyObject *
code_richcompare(PyObject *self, PyObject *other, int op)
{
    PyCodeObject *co, *cp;
    int eq;
    PyObject *consts1, *consts2;
    PyObject *res;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyCode_Check(self) ||
        !PyCode_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    co = (PyCodeObject *)self;
    cp = (PyCodeObject *)other;

    eq = PyObject_RichCompareBool(co->co_name, cp->co_name, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = co->co_argcount == cp->co_argcount;
    if (!eq) goto unequal;
    eq = co->co_posonlyargcount == cp->co_posonlyargcount;
    if (!eq) goto unequal;
    eq = co->co_kwonlyargcount == cp->co_kwonlyargcount;
    if (!eq) goto unequal;
    eq = co->co_flags == cp->co_flags;
    if (!eq) goto unequal;
    eq = co->co_firstlineno == cp->co_firstlineno;
    if (!eq) goto unequal;
    eq = Py_SIZE(co) == Py_SIZE(cp);
    if (!eq) {
        goto unequal;
    }
    eq = co->co_nfeedback == cp->co_nfeedback
        && memcmp(co->co_words, cp->co_words, _PyCode_NBYTES(co)) == 0
        && memcmp(_PyCode_FBINIT(co), _PyCode_FBINIT(cp),
                  co->co_nfeedback * sizeof(uint32_t)) == 0;
    if (!eq) {
        goto unequal;
    }

    /* compare constants */
    consts1 = _PyCode_ConstantKey(co->co_consts);
    if (!consts1)
        return NULL;
    consts2 = _PyCode_ConstantKey(cp->co_consts);
    if (!consts2) {
        Py_DECREF(consts1);
        return NULL;
    }
    eq = PyObject_RichCompareBool(consts1, consts2, Py_EQ);
    Py_DECREF(consts1);
    Py_DECREF(consts2);
    if (eq <= 0) goto unequal;

    eq = PyObject_RichCompareBool(co->co_names, cp->co_names, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = PyObject_RichCompareBool(co->co_localsplusnames,
                                  cp->co_localsplusnames, Py_EQ);
    if (eq <= 0) goto unequal;
    eq = PyObject_RichCompareBool(co->co_linetable, cp->co_linetable, Py_EQ);
    if (eq <= 0) {
        goto unequal;
    }
    eq = PyObject_RichCompareBool(co->co_exceptiontable,
                                  cp->co_exceptiontable, Py_EQ);
    if (eq <= 0) {
        goto unequal;
    }

    if (op == Py_EQ)
        res = Py_True;
    else
        res = Py_False;
    goto done;

  unequal:
    if (eq < 0)
        return NULL;
    if (op == Py_NE)
        res = Py_True;
    else
        res = Py_False;

  done:
    return Py_NewRef(res);
}

static Py_hash_t
code_hash(PyObject *self)
{
    PyCodeObject *co = _PyCodeObject_CAST(self);
    Py_uhash_t uhash = 20221211;
    #define SCRAMBLE_IN(H) do {       \
        uhash ^= (Py_uhash_t)(H);     \
        uhash *= PyHASH_MULTIPLIER;  \
    } while (0)
    #define SCRAMBLE_IN_HASH(EXPR) do {     \
        Py_hash_t h = PyObject_Hash(EXPR);  \
        if (h == -1) {                      \
            return -1;                      \
        }                                   \
        SCRAMBLE_IN(h);                     \
    } while (0)

    SCRAMBLE_IN_HASH(co->co_name);
    SCRAMBLE_IN_HASH(co->co_consts);
    SCRAMBLE_IN_HASH(co->co_names);
    SCRAMBLE_IN_HASH(co->co_localsplusnames);
    SCRAMBLE_IN_HASH(co->co_linetable);
    SCRAMBLE_IN_HASH(co->co_exceptiontable);
    SCRAMBLE_IN(co->co_argcount);
    SCRAMBLE_IN(co->co_posonlyargcount);
    SCRAMBLE_IN(co->co_kwonlyargcount);
    SCRAMBLE_IN(co->co_flags);
    SCRAMBLE_IN(co->co_firstlineno);
    SCRAMBLE_IN(Py_SIZE(co));
    for (int i = 0; i < Py_SIZE(co); i++) {
        SCRAMBLE_IN(_PyCode_WORDS(co)[i]);
    }
    for (int i = 0; i < co->co_nfeedback; i++) {
        SCRAMBLE_IN(_PyCode_FBINIT(co)[i]);
    }
    if ((Py_hash_t)uhash == -1) {
        return -2;
    }
    return (Py_hash_t)uhash;
}


#define OFF(x) offsetof(PyCodeObject, x)

static PyMemberDef code_memberlist[] = {
    {"co_argcount",        Py_T_INT,     OFF(co_argcount),        Py_READONLY},
    {"co_posonlyargcount", Py_T_INT,     OFF(co_posonlyargcount), Py_READONLY},
    {"co_kwonlyargcount",  Py_T_INT,     OFF(co_kwonlyargcount),  Py_READONLY},
    {"co_stacksize",       Py_T_INT,     OFF(co_stacksize),       Py_READONLY},
    {"co_flags",           Py_T_INT,     OFF(co_flags),           Py_READONLY},
    {"co_nlocals",         Py_T_INT,     OFF(co_nlocals),         Py_READONLY},
    {"co_consts",          _Py_T_OBJECT, OFF(co_consts),          Py_READONLY},
    {"co_names",           _Py_T_OBJECT, OFF(co_names),           Py_READONLY},
    {"co_filename",        _Py_T_OBJECT, OFF(co_filename),        Py_READONLY},
    {"co_name",            _Py_T_OBJECT, OFF(co_name),            Py_READONLY},
    {"co_qualname",        _Py_T_OBJECT, OFF(co_qualname),        Py_READONLY},
    {"co_firstlineno",     Py_T_INT,     OFF(co_firstlineno),     Py_READONLY},
    {"co_linetable",       _Py_T_OBJECT, OFF(co_linetable),       Py_READONLY},
    {"co_exceptiontable",  _Py_T_OBJECT, OFF(co_exceptiontable),  Py_READONLY},
    {NULL}      /* Sentinel */
};


static PyValue
code_getvarnames(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return PyValue_FromResult(_PyCode_GetVarnames(code));
}

static PyValue
code_getcellvars(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return PyValue_FromResult(_PyCode_GetCellvars(code));
}

static PyValue
code_getfreevars(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return PyValue_FromResult(_PyCode_GetFreevars(code));
}

/* The words of the executing copy, whose handlers specialisation and
 * instrumentation rewrite, or co_code before the first call. */
static PyValue
code_getcodeadaptive(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(code->_co_exec);
    if (exec == NULL) {
        return PyValue_FromResult(_PyCode_GetCode(code));
    }
    return PyValue_FromResult(PyBytes_FromStringAndSize((const char *)exec->words,
                                     exec->nwords * sizeof(_PyIRWord)));
}

/* The feedback entries of the executing copy, FB_STRIDE bytes each and
 * entry 0 first, or None before the first call or without entries. */
static PyValue
code_getfeedback(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(code->_co_exec);
    if (exec == NULL || exec->entries == NULL) {
        return PyValue_FromObject(Py_None);
    }
    return PyValue_FromResult(PyBytes_FromStringAndSize(exec->entries,
                                     code->co_nfeedback * FB_STRIDE));
}

/* The feedback table: per feedback entry, its kind and static operand */
static PyValue
code_getfbinit(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return PyValue_FromResult(PyBytes_FromStringAndSize((const char *)_PyCode_FBINIT(code),
                                     code->co_nfeedback * sizeof(uint32_t)));
}

static PyValue
code_getcode(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return PyValue_FromResult(_PyCode_GetCode(code));
}

static PyGetSetDef code_getsetlist[] = {
    {"_co_code_adaptive", code_getcodeadaptive, NULL, NULL},
    {"co_fbinit",         code_getfbinit,       NULL, NULL},
    {"_co_feedback",      code_getfeedback,     NULL, NULL},
    // The following old names are kept for backward compatibility.
    {"co_varnames",       code_getvarnames,     NULL, NULL},
    {"co_cellvars",       code_getcellvars,     NULL, NULL},
    {"co_freevars",       code_getfreevars,     NULL, NULL},
    {"co_code",           code_getcode,         NULL, NULL},
    {0}
};


static PyObject *
code_sizeof(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyCodeObject *co = _PyCodeObject_CAST(self);
    size_t res = _PyObject_VAR_SIZE(Py_TYPE(co), Py_SIZE(co));
    res += _PyCode_ExecSizeOf(co);
    _PyCodeObjectExtra *co_extra = (_PyCodeObjectExtra*) co->co_extra;
    if (co_extra != NULL) {
        res += sizeof(_PyCodeObjectExtra);
        res += ((size_t)co_extra->ce_size - 1) * sizeof(co_extra->ce_extras[0]);
    }
    return PyLong_FromSize_t(res);
}

static PyObject *
code_linesiterator(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return (PyObject *)new_linesiterator(code);
}

static PyObject *
code_branchesiterator(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return _PyInstrumentation_BranchesIterator(code);
}

/*[clinic input]
@permit_long_summary
@text_signature "($self, /, **changes)"
code.replace

    *
    co_argcount: int(c_default="((PyCodeObject *)self)->co_argcount") = unchanged
    co_posonlyargcount: int(c_default="((PyCodeObject *)self)->co_posonlyargcount") = unchanged
    co_kwonlyargcount: int(c_default="((PyCodeObject *)self)->co_kwonlyargcount") = unchanged
    co_nlocals: int(c_default="((PyCodeObject *)self)->co_nlocals") = unchanged
    co_stacksize: int(c_default="((PyCodeObject *)self)->co_stacksize") = unchanged
    co_flags: int(c_default="((PyCodeObject *)self)->co_flags") = unchanged
    co_firstlineno: int(c_default="((PyCodeObject *)self)->co_firstlineno") = unchanged
    co_code: object(subclass_of="&PyBytes_Type", c_default="NULL") = unchanged
    co_consts: object(subclass_of="&PyTuple_Type", c_default="((PyCodeObject *)self)->co_consts") = unchanged
    co_names: object(subclass_of="&PyTuple_Type", c_default="((PyCodeObject *)self)->co_names") = unchanged
    co_varnames: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_freevars: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_cellvars: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_filename: unicode(c_default="((PyCodeObject *)self)->co_filename") = unchanged
    co_name: unicode(c_default="((PyCodeObject *)self)->co_name") = unchanged
    co_qualname: unicode(c_default="((PyCodeObject *)self)->co_qualname") = unchanged
    co_linetable: object(subclass_of="&PyBytes_Type", c_default="((PyCodeObject *)self)->co_linetable") = unchanged
    co_exceptiontable: object(subclass_of="&PyBytes_Type", c_default="((PyCodeObject *)self)->co_exceptiontable") = unchanged
    co_fbinit: object(subclass_of="&PyBytes_Type", c_default="NULL") = unchanged

Return a copy of the code object with new values for the specified fields.
[clinic start generated code]*/

static PyObject *
code_replace_impl(PyCodeObject *self, int co_argcount,
                  int co_posonlyargcount, int co_kwonlyargcount,
                  int co_nlocals, int co_stacksize, int co_flags,
                  int co_firstlineno, PyObject *co_code, PyObject *co_consts,
                  PyObject *co_names, PyObject *co_varnames,
                  PyObject *co_freevars, PyObject *co_cellvars,
                  PyObject *co_filename, PyObject *co_name,
                  PyObject *co_qualname, PyObject *co_linetable,
                  PyObject *co_exceptiontable, PyObject *co_fbinit)
/*[clinic end generated code: output=ca4997a8f05a288c input=f2236468f8933e03]*/
{
#define CHECK_INT_ARG(ARG) \
        if (ARG < 0) { \
            PyErr_SetString(PyExc_ValueError, \
                            #ARG " must be a positive integer"); \
            return NULL; \
        }

    CHECK_INT_ARG(co_argcount);
    CHECK_INT_ARG(co_posonlyargcount);
    CHECK_INT_ARG(co_kwonlyargcount);
    CHECK_INT_ARG(co_nlocals);
    CHECK_INT_ARG(co_stacksize);
    CHECK_INT_ARG(co_flags);
    CHECK_INT_ARG(co_firstlineno);

#undef CHECK_INT_ARG

    PyObject *code = NULL;
    if (co_code == NULL) {
        code = _PyCode_GetCode(self);
        if (code == NULL) {
            return NULL;
        }
        co_code = code;
    }

    if (PySys_Audit("code.__new__", "OOOiiiiii",
                    co_code, co_filename, co_name, co_argcount,
                    co_posonlyargcount, co_kwonlyargcount, co_nlocals,
                    co_stacksize, co_flags) < 0) {
        Py_XDECREF(code);
        return NULL;
    }

    PyCodeObject *co = NULL;
    PyObject *varnames = NULL;
    PyObject *cellvars = NULL;
    PyObject *freevars = NULL;
    PyObject *fbinit = NULL;
    if (co_varnames == NULL) {
        varnames = get_localsplus_names(self, CO_FAST_LOCAL, self->co_nlocals);
        if (varnames == NULL) {
            goto error;
        }
        co_varnames = varnames;
    }
    if (co_cellvars == NULL) {
        cellvars = get_localsplus_names(self, CO_FAST_CELL, self->co_ncellvars);
        if (cellvars == NULL) {
            goto error;
        }
        co_cellvars = cellvars;
    }
    if (co_freevars == NULL) {
        freevars = get_localsplus_names(self, CO_FAST_FREE, self->co_nfreevars);
        if (freevars == NULL) {
            goto error;
        }
        co_freevars = freevars;
    }

    if (co_fbinit == NULL) {
        fbinit = PyBytes_FromStringAndSize((const char *)_PyCode_FBINIT(self),
                                           self->co_nfeedback * sizeof(uint32_t));
        if (fbinit == NULL) {
            goto error;
        }
        co_fbinit = fbinit;
    }
    co = code_new_with_fbinit(
        co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals,
        co_stacksize, co_flags, co_code, co_fbinit, co_consts, co_names,
        co_varnames, co_freevars, co_cellvars, co_filename, co_name,
        co_qualname, co_firstlineno,
        co_linetable, co_exceptiontable);

error:
    Py_XDECREF(fbinit);
    Py_XDECREF(code);
    Py_XDECREF(varnames);
    Py_XDECREF(cellvars);
    Py_XDECREF(freevars);
    return (PyObject *)co;
}

/*[clinic input]
code._varname_from_oparg

    oparg: int

(internal-only) Return the local variable name for the given oparg.

WARNING: this method is for internal use only and may change or go
away.
[clinic start generated code]*/

static PyObject *
code__varname_from_oparg_impl(PyCodeObject *self, int oparg)
/*[clinic end generated code: output=1fd1130413184206 input=6ba7d6df0d566463]*/
{
    PyObject *name = PyTuple_GetItem(self->co_localsplusnames, oparg);
    if (name == NULL) {
        return NULL;
    }
    return Py_NewRef(name);
}

/* XXX code objects need to participate in GC? */

static struct PyMethodDef code_methods[] = {
    {"__sizeof__", code_sizeof, METH_NOARGS},
    {"co_lines", code_linesiterator, METH_NOARGS},
    {"co_branches", code_branchesiterator, METH_NOARGS},
    {"co_positions", code_positionsiterator, METH_NOARGS},
    CODE_REPLACE_METHODDEF
    CODE__VARNAME_FROM_OPARG_METHODDEF
    {"__replace__", _PyCFunction_CAST(code_replace), METH_FASTCALL|METH_KEYWORDS,
     PyDoc_STR("__replace__($self, /, **changes)\n--\n\nThe same as replace().")},
    {NULL, NULL}                /* sentinel */
};


PyTypeObject PyCode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "code",
    offsetof(PyCodeObject, co_words),
    sizeof(_PyIRWord),
    code_dealloc,                       /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    code_repr,                          /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    code_hash,                          /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_MANAGED_WEAKREF, /* tp_flags */
    code_new__doc__,                    /* tp_doc */
    code_traverse,                      /* tp_traverse */
    0,                                  /* tp_clear */
    code_richcompare,                   /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    code_methods,                       /* tp_methods */
    code_memberlist,                    /* tp_members */
    code_getsetlist,                    /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    code_new,                           /* tp_new */
};


/******************
 * other API
 ******************/

// _PyCode_ConstantKey() of a value, such as a bound of a slice.
static PyObject *
constant_key_value(PyValue v)
{
    PyObject *op = PyValue_Object(v);
    if (op == NULL) {
        return NULL;
    }
    return _PyCode_ConstantKey(op);
}

PyObject*
_PyCode_ConstantKey(PyObject *op)
{
    PyObject *key;

    /* Py_None and Py_Ellipsis are singletons. */
    if (op == Py_None || op == Py_Ellipsis
       || PyLong_CheckExact(op)
       || PyUnicode_CheckExact(op)
          /* code_richcompare() uses _PyCode_ConstantKey() internally */
       || PyCode_Check(op))
    {
        /* Objects of these types are always different from object of other
         * type and from tuples. */
        key = Py_NewRef(op);
    }
    else if (PyBool_Check(op) || PyBytes_CheckExact(op)) {
        /* Make booleans different from integers 0 and 1.
         * Avoid BytesWarning from comparing bytes with strings. */
        key = _PyTuple_FromPair((PyObject *)Py_TYPE(op), op);
    }
    else if (PyFloat_CheckExact(op)) {
        double d = PyFloat_AS_DOUBLE(op);
        /* all we need is to make the tuple different in either the 0.0
         * or -0.0 case from all others, just to avoid the "coercion".
         */
        if (d == 0.0 && copysign(1.0, d) < 0.0)
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_None);
        else
            key = _PyTuple_FromPair((PyObject *)Py_TYPE(op), op);
    }
    else if (PyComplex_CheckExact(op)) {
        Py_complex z;
        int real_negzero, imag_negzero;
        /* For the complex case we must make complex(x, 0.)
           different from complex(x, -0.) and complex(0., y)
           different from complex(-0., y), for any x and y.
           All four complex zeros must be distinguished.*/
        z = PyComplex_AsCComplex(op);
        real_negzero = z.real == 0.0 && copysign(1.0, z.real) < 0.0;
        imag_negzero = z.imag == 0.0 && copysign(1.0, z.imag) < 0.0;
        /* use True, False and None singleton as tags for the real and imag
         * sign, to make tuples different */
        if (real_negzero && imag_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_True);
        }
        else if (imag_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_False);
        }
        else if (real_negzero) {
            key = PyTuple_Pack(3, Py_TYPE(op), op, Py_None);
        }
        else {
            key = _PyTuple_FromPair((PyObject *)Py_TYPE(op), op);
        }
    }
    else if (PyTuple_CheckExact(op)) {
        Py_ssize_t i, len;
        PyObject *tuple;

        len = PyTuple_GET_SIZE(op);
        tuple = PyTuple_New(len);
        if (tuple == NULL)
            return NULL;

        for (i=0; i < len; i++) {
            PyObject *item, *item_key;

            item = PyTuple_GET_ITEM(op, i);
            item_key = _PyCode_ConstantKey(item);
            if (item_key == NULL) {
                Py_DECREF(tuple);
                return NULL;
            }

            PyTuple_SET_ITEM(tuple, i, item_key);
        }

        key = _PyTuple_FromPair(tuple, op);
        Py_DECREF(tuple);
    }
    else if (PyFrozenSet_CheckExact(op)) {
        Py_ssize_t pos = 0;
        PyValue item;
        Py_hash_t hash;
        Py_ssize_t i, len;
        PyObject *tuple, *set;

        len = PySet_GET_SIZE(op);
        tuple = PyTuple_New(len);
        if (tuple == NULL)
            return NULL;

        i = 0;
        while (_PySet_NextValue(op, &pos, &item, &hash)) {
            PyObject *item_key = NULL;
            PyObject *item_o = PyValue_Object(item);
            if (item_o != NULL) {
                item_key = _PyCode_ConstantKey(item_o);
            }
            if (item_key == NULL) {
                Py_DECREF(tuple);
                return NULL;
            }

            assert(i < len);
            PyTuple_SET_ITEM(tuple, i, item_key);
            i++;
        }
        set = PyFrozenSet_New(tuple);
        Py_DECREF(tuple);
        if (set == NULL)
            return NULL;

        key = _PyTuple_FromPair(set, op);
        Py_DECREF(set);
        return key;
    }
    else if (PySlice_Check(op)) {
        PySliceObject *slice = (PySliceObject *)op;
        PyObject *start_key = NULL;
        PyObject *stop_key = NULL;
        PyObject *step_key = NULL;
        key = NULL;

        start_key = constant_key_value(slice->start);
        if (start_key == NULL) {
            goto slice_exit;
        }

        stop_key = constant_key_value(slice->stop);
        if (stop_key == NULL) {
            goto slice_exit;
        }

        step_key = constant_key_value(slice->step);
        if (step_key == NULL) {
            goto slice_exit;
        }

        PyObject *slice_key = PySlice_New(start_key, stop_key, step_key);
        if (slice_key == NULL) {
            goto slice_exit;
        }

        key = _PyTuple_FromPair(slice_key, op);
        Py_DECREF(slice_key);
    slice_exit:
        Py_XDECREF(start_key);
        Py_XDECREF(stop_key);
        Py_XDECREF(step_key);
    }
    else {
        /* for other types, use the object identifier as a unique identifier
         * to ensure that they are seen as unequal. */
        PyObject *obj_id = PyLong_FromVoidPtr(op);
        if (obj_id == NULL)
            return NULL;

        key = _PyTuple_FromPair(obj_id, op);
        Py_DECREF(obj_id);
    }
    return key;
}


PyStatus
_PyCode_Init(PyInterpreterState *interp)
{
    return _PyStatus_OK();
}

void
_PyCode_Fini(PyInterpreterState *interp)
{
}

