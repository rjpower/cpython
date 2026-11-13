#include "Python.h"
#include "opcode.h"

#include "pycore_code.h"          // _PyCodeConstructor
#include "pycore_dict.h"          // PyDict_ContainsValue()
#include "pycore_function.h"      // _PyFunction_ClearCodeByVersion()
#include "pycore_hashtable.h"     // _Py_hashtable_t
#include "pycore_index_pool.h"    // _PyIndexPool_Fini()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // PyInterpreterState.co_extra_freefuncs
#include "pycore_interpframe.h"   // FRAME_SPECIALS_SIZE
#include "pycore_ir_source.h"
#include "pycore_ir_outcomes.h"
#include "pycore_ir_exception.h"
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_opcode_utils.h"  // RESUME_AT_FUNC_START
#include "pycore_pymem.h"         // _PyMem_FreeDelayed()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_setobject.h"     // _PySet_NextValue()
#include "pycore_sliceobject.h"   // _PySlice_FromValues()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_InternImmortal()
#include "pycore_uniqueid.h"      // _PyObject_AssignUniqueId()
#include "pycore_value.h"         // _PyCode_ConstantValueKey()
#include "pycore_value_slots.h"  // _Py_VALUE_NOTIMPL_RICHCMP()
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
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
#ifdef Py_GIL_DISABLED
    // The free-threaded build interns (and immortalizes) all string constants
    return 1;
#else
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
#endif
}


/* Interning changes representatives, never typed equality or hash, including
 * for tuples already shared with callers or used as dictionary keys. */
static void
replace_equivalent_constant(PyObject *tuple, Py_ssize_t index, PyValue value)
{
    // a move: replaced below
    PyValue old = ((PyTupleObject *)tuple)->ob_item[index];
    ((PyTupleObject *)tuple)->ob_item[index] = value;
    PyValue_CLOSE(old);
}

static int
validate_code_name_tuple(PyObject *tuple)
{
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(tuple); i++) {
        PyValueBorrow value = PyTuple_GET_VALUE(tuple, i);
        if (!PyValue_IsObject(value) ||
            !PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(value))) {
            PyErr_SetString(PyExc_SystemError, "non-string found in code slot");
            return -1;
        }
    }
    return 0;
}

static int
intern_strings(PyObject *tuple)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    Py_ssize_t i;

    for (i = PyTuple_GET_SIZE(tuple); --i >= 0; ) {
        PyValue value = PyTuple_GetValue(tuple, i);
        if (!PyValue_IsObject(value) ||
            !PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(value))) {
            PyValue_XCLOSE(value);
            PyErr_SetString(PyExc_SystemError,
                            "non-string found in code slot");
            return -1;
        }
        PyObject *v = PyValue_AsKnownObjectSteal(value);
        _PyUnicode_InternImmortal(interp, &v);
        replace_equivalent_constant(tuple, i, PyValue_FromObjectSteal(v));
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
        PyValue held = PyTuple_GetValue(tuple, i);
        if (PyValue_IsNull(held)) {
            PyErr_SetString(PyExc_SystemError, "absent value in code constants");
            return -1;
        }
        if (!PyValue_IsObject(held)) {
            PyValue_CLOSE(held);
            continue;
        }
        PyObject *v = PyValue_AsKnownObjectBorrow(held);
        if (PyUnicode_CheckExact(v)) {
            if (_PyCode_ShouldInternString(v)) {
                PyObject *w = Py_NewRef(v);
                v = w;
                _PyUnicode_InternMortal(interp, &v);
                if (w != v) {
                    replace_equivalent_constant(tuple, i,
                                                PyValue_FromObjectSteal(v));
                    if (modified) {
                        *modified = 1;
                    }
                }
                else {
                    Py_DECREF(v);
                }
            }
        }
        else if (PyTuple_CheckExact(v)) {
            if (intern_constants(v, NULL) < 0) {
                PyValue_CLOSE(held);
                return -1;
            }
        }
        else if (PyFrozenSet_CheckExact(v)) {
            PyObject *tmp = PyTuple_New(PySet_GET_SIZE(v));
            if (tmp == NULL) {
                PyValue_CLOSE(held);
                return -1;
            }
            Py_ssize_t pos = 0, index = 0;
            PyValue item;
            Py_hash_t hash;
            int next;
            while ((next = _PySet_NextValue(v, &pos, &item, &hash)) > 0) {
                PyTuple_SET_VALUE(tmp, index++, item);
            }
            if (next < 0) {
                Py_DECREF(tmp);
                PyValue_CLOSE(held);
                return -1;
            }
            int tmp_modified = 0;
            if (intern_constants(tmp, &tmp_modified) < 0) {
                Py_DECREF(tmp);
                PyValue_CLOSE(held);
                return -1;
            }
            if (tmp_modified) {
                v = PyFrozenSet_New(NULL);
                if (v != NULL) {
                    for (Py_ssize_t j = 0; j < PyTuple_GET_SIZE(tmp); j++) {
                        PyValue item = PyTuple_GetValue(tmp, j);
                        int status = PySet_AddValue(v, item);
                        PyValue_CLOSE(item);
                        if (status < 0) {
                            Py_CLEAR(v);
                            break;
                        }
                    }
                }
                if (v == NULL) {
                    Py_DECREF(tmp);
                    PyValue_CLOSE(held);
                    return -1;
                }

                replace_equivalent_constant(tuple, i,
                                            PyValue_FromObjectSteal(v));
                if (modified) {
                    *modified = 1;
                }
            }
            Py_DECREF(tmp);
        }
        PyValue_CLOSE(held);
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
        PyValue value = PyTuple_GetValue(tup, i);
        item = PyValue_IsObject(value) ? PyValue_AsKnownObjectBorrow(value) : NULL;
        if (item == NULL || !PyUnicode_Check(item)) {
            PyErr_Format(
                PyExc_TypeError,
                "name tuples must contain only "
                "strings, not '%.500s'",
                PyValue_TYPE(value)->tp_name);
            PyValue_CLOSE(value);
            Py_DECREF(newtuple);
            return NULL;
        }
        if (PyUnicode_CheckExact(item)) {
            item = PyValue_AsKnownObjectSteal(value);
        }
        else {
            item = _PyUnicode_Copy(item);
            PyValue_CLOSE(value);
            if (item == NULL) {
                Py_DECREF(newtuple);
                return NULL;
            }
        }
        PyTuple_SET_VALUE(newtuple, i, PyValue_PrepareOwned(item));
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
    PyTuple_SET_VALUE(names, offset, PyValue_DUP(PyValue_FromObjectBorrow(name)));
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
        PyObject *name = _PyTuple_GET_OBJECT(co->co_localsplusnames, offset);
        PyTuple_SET_VALUE(names, index, PyValue_DUP(PyValue_FromObjectBorrow(name)));
        index += 1;
    }
    assert(index == num);
    return names;
}

/* The words against the layout of the frame, which the compiler's verifier
 * (Python/regcfg.c) guarantees for its own output: no word writes a
 * constant slot, through a field whose role is a destination or a local
 * written, and no window starts below the temporaries.  Words of unknown
 * handlers are for check_words(). */
static int
check_const_layout(struct _PyCodeConstructor *con, int first, int nconst)
{
    Py_ssize_t nwords = PyBytes_GET_SIZE(con->code) / (Py_ssize_t)sizeof(_PyIRWord);
    const char *bytes = PyBytes_AS_STRING(con->code);
    int nlocalsplus = (int)PyTuple_GET_SIZE(con->localsplusnames);
    for (Py_ssize_t i = 0; i < nwords; i++) {
        _PyIRWord word;
        memcpy(&word, bytes + i * sizeof(_PyIRWord), sizeof(word));
        int handler = IR_HANDLER(word);
        if (handler >= IR_NUM_HANDLERS
            || !(_PyIR_Formats[handler].flags & _PyIR_VALID))
        {
            continue;
        }
        const _PyIRFormat *fmt = &_PyIR_Formats[handler];
        for (int k = 0; k < 4; k++) {
            int role = fmt->roles[k];
            int value = (int)((word >> (16 + 12 * k)) & IR_FIELD_MAX);
            int writes = (role == _PyIR_ROLE_DST && fmt->dst != _PyIR_NO_DST)
                         || role == _PyIR_ROLE_LOCAL_OUT
                         || role == _PyIR_ROLE_LOCAL_CLEAR
                         || role == _PyIR_ROLE_LOCAL_MOVE;
            if (writes && value >= first && value < first + nconst) {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd writes constant slot %d", i, value);
                return -1;
            }
            if (role == _PyIR_ROLE_BASE && (fmt->flags & _PyIR_WINDOW)
                && value < nlocalsplus)
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd has a window at slot %d, below "
                             "the temporaries", i, value);
                return -1;
            }
        }
    }
    return 0;
}

/* The constant-slot table: pairs of (slot, co_consts index), in the
 * order of the constant slots of localsplusnames, which are consecutive
 * and have the kind CO_FAST_CONST | CO_FAST_HIDDEN, one per pair. */
static int
check_constslots(struct _PyCodeConstructor *con)
{
    Py_ssize_t nconst = 0;
    Py_ssize_t first = -1;
    Py_ssize_t n = PyBytes_GET_SIZE(con->localspluskinds);
    for (Py_ssize_t i = 0; i < n; i++) {
        _PyLocals_Kind kind = _PyLocals_GetKind(con->localspluskinds, (int)i);
        if (kind & CO_FAST_CONST) {
            if (kind != (CO_FAST_CONST | CO_FAST_HIDDEN)
                || (first >= 0 && i != first + nconst))
            {
                PyErr_SetString(PyExc_ValueError,
                                "code: malformed constant slot kinds");
                return -1;
            }
            if (first < 0) {
                first = i;
            }
            nconst++;
        }
    }
    if (con->constslots != NULL && !PyBytes_Check(con->constslots)) {
        PyErr_SetString(PyExc_ValueError, "code: co_constslots is malformed");
        return -1;
    }
    Py_ssize_t size = con->constslots == NULL ? 0
                      : PyBytes_GET_SIZE(con->constslots);
    if (size % (2 * sizeof(uint32_t)) != 0
        || size / (Py_ssize_t)(2 * sizeof(uint32_t)) != nconst)
    {
        PyErr_SetString(PyExc_ValueError, "code: co_constslots is malformed");
        return -1;
    }
    const uint32_t *table = size == 0 ? NULL
        : (const uint32_t *)PyBytes_AS_STRING(con->constslots);
    Py_ssize_t nconsts = PyTuple_GET_SIZE(con->consts);
    for (Py_ssize_t j = 0; j < nconst; j++) {
        if (table[2 * j] != first + j) {
            // The slots follow the cells, so a different number of
            // variables moves them
            PyErr_SetString(PyExc_ValueError,
                            "code: changing the number of variables of code "
                            "with constant slots is not supported "
                            "(co_constslots names other slots)");
            return -1;
        }
        if (table[2 * j + 1] >= nconsts) {
            PyErr_SetString(PyExc_ValueError,
                            "code: co_constslots is malformed");
            return -1;
        }
    }
    return check_const_layout(con, (int)first, (int)nconst);
}

/* The units that an entry of each kind takes at least (pycore_feedback.h) */
static const uint8_t fb_kind_units[] = {
    [FB_COUNTER] = sizeof(_PyFbHeader) / 8,
    [FB_BRANCH] = sizeof(_PyFbHeader) / 8,
    [FB_UNARY] = sizeof(_PyFbUnary) / 8,
    [FB_BINARY] = sizeof(_PyFbBinary) / 8,
    [FB_ATTR] = sizeof(_PyFbAttr) / 8,
    [FB_GLOBAL] = sizeof(_PyFbGlobal) / 8,
    [FB_CALL] = sizeof(_PyFbCall) / 8,
};

/* Whether fbinit, the feedback table of a code object, describes a vector:
 * entry 0 first, the others in order of unit, each with room for its kind,
 * all within the units that entry 0 gives. */
static int
fbinit_is_valid(PyObject *fbinit)
{
    Py_ssize_t size = PyBytes_GET_SIZE(fbinit);
    if (size % sizeof(uint32_t) != 0) {
        return 0;
    }
    Py_ssize_t n = size / (Py_ssize_t)sizeof(uint32_t);
    if (n == 0) {
        return 1;
    }
    uint32_t word;
    memcpy(&word, PyBytes_AS_STRING(fbinit), sizeof(word));
    uint32_t total = _PyIR_FBINIT_ARG(word);
    if (_PyIR_FBINIT_KIND(word) != FB_COUNTER || _PyIR_FBINIT_UNIT(word) != 0
        || total < IR_FB_FIRST_UNIT || total > IR_FB_UNITS_MAX)
    {
        return 0;
    }
    uint32_t next = IR_FB_FIRST_UNIT;
    for (Py_ssize_t i = 1; i < n; i++) {
        memcpy(&word, PyBytes_AS_STRING(fbinit) + i * sizeof(word), sizeof(word));
        int kind = _PyIR_FBINIT_KIND(word);
        if (kind <= FB_COUNTER || kind > FB_CALL
            || _PyIR_FBINIT_UNIT(word) < next)
        {
            return 0;
        }
        next = _PyIR_FBINIT_UNIT(word) + fb_kind_units[kind];
        if (next > total) {
            return 0;
        }
    }
    return 1;
}

/* Validate location payloads before the variable-length runtime readers see
 * them. Raw constructors may omit trailing locations; stored maps may not. */
static int
location_words(const _PyIRPositions *map, uint32_t *covered)
{
    const unsigned char *p = map->locations;
    const unsigned char *end = p + map->locations_size;
    uint64_t count = 0;
    while (p < end) {
        unsigned int header = *p++;
        if (!(header & 128)) {
            return -1;
        }
        count += (header & 7) + 1;
        int kind = (header >> 3) & 15;
        int fixed = kind < 10 ? 1 : kind < 13 ? 2 : 0;
        if (end - p < fixed) {
            return -1;
        }
        for (int j = 0; j < fixed; j++) {
            if (p[j] & 128) {
                return -1;
            }
        }
        p += fixed;
        int varints = kind == 13 ? 1 : kind == 14 ? 4 : 0;
        for (int k = 0; k < varints; k++) {
            int bytes = 0;
            unsigned int byte;
            do {
                if (p == end || ++bytes > 6 || (*p & 128)) {
                    return -1;
                }
                byte = *p++;
            } while (byte & 64);
        }
        if (count > UINT32_MAX) {
            return -1;
        }
    }
    *covered = (uint32_t)count;
    return 0;
}

static PyObject *
normalize_positions(const _PyIRPositions *map, uint32_t nwords)
{
    uint32_t covered;
    if (location_words(map, &covered) < 0 || covered > nwords) {
        PyErr_SetString(PyExc_ValueError, "code: malformed source map locations");
        return NULL;
    }
    uint32_t padding = (nwords - covered + 7) / 8;
    if ((uint64_t)map->locations_size + padding > UINT32_MAX) {
        return PyErr_NoMemory();
    }
    PyObject *locations = PyBytes_FromStringAndSize(NULL, (Py_ssize_t)map->locations_size + padding);
    if (locations == NULL) {
        return NULL;
    }
    unsigned char *p = (unsigned char *)PyBytes_AS_STRING(locations);
    memcpy(p, map->locations, map->locations_size);
    p += map->locations_size;
    while (covered < nwords) {
        uint32_t run = Py_MIN(nwords - covered, 8u);
        /* Location kind 15: no line or column, over run real words. */
        *p++ = (unsigned char)(0xF8 | (run - 1));
        covered += run;
    }
    PyObject *result = _PyIRPositions_Pack(
        (const unsigned char *)PyBytes_AS_STRING(locations),
        (uint32_t)PyBytes_GET_SIZE(locations), map->records, map->nrecords);
    Py_DECREF(locations);
    return result;
}

static int completion_handler_safe(struct _PyCodeConstructor *, int, int, int,
                                   int, int);
static int observer_handler_safe(struct _PyCodeConstructor *, _PyIRWord, int,
                                 int, int, int, int);

static int
validate_semantic_and_position_maps(struct _PyCodeConstructor *con)
{
    _PyIRPositions positions;
    int format = _PyIRPositions_Decode(PyBytes_AS_STRING(con->linetable),
                                    PyBytes_GET_SIZE(con->linetable), &positions);
    if (format < 0) {
        PyErr_SetString(PyExc_ValueError, "code: malformed source map");
        return -1;
    }
    int nwords = (int)(PyBytes_GET_SIZE(con->code) / sizeof(_PyIRWord));
    int nl = (int)PyTuple_GET_SIZE(con->localsplusnames);
    int nslots = nl + Py_MAX(con->stacksize, 1);
    uint32_t covered;
    if (location_words(&positions, &covered) < 0 || covered > (uint32_t)nwords ||
        (format && covered != (uint32_t)nwords))
    {
        PyErr_SetString(PyExc_ValueError, "code: source map locations do not fit code");
        return -1;
    }
    _PyIRSemanticMap map;
    if (_PyIRSemantic_Decode(PyBytes_AS_STRING(con->exceptiontable),
                             PyBytes_GET_SIZE(con->exceptiontable), &map) < 0 ||
        !_PyIR_ExceptionTableValid(map.exceptions,
                                  map.exceptions + map.exceptions_size,
                                  nwords, nl, nslots))
    {
        PyErr_SetString(PyExc_ValueError, "code: malformed semantic recovery map");
        return -1;
    }
    if (positions.nrecords && positions.nrecords != map.nrecords) {
        PyErr_SetString(PyExc_ValueError,
                        "code: position IDs do not match semantic recovery");
        return -1;
    }
    for (uint32_t i = 0; i < map.nrecords; i++) {
        _PyIRRecoveryEvent event;
        _PyIRSemantic_Event(&map, i, &event);
        if (event.from < 0 || event.from >= nwords ||
            event.to < 0 || event.to >= nwords ||
            event.edge < 0 || event.edge > _PyIR_EDGE_TAKEN ||
            event.handler < -1 || event.handler >= nwords ||
            (event.handler == -1 && (event.exc_slot != 0 || event.lasti != 0)) ||
            (event.handler >= 0 &&
             ((event.lasti != 0 && event.lasti != 1) ||
              event.exc_slot < PyTuple_GET_SIZE(con->localsplusnames) + event.lasti ||
              event.exc_slot >= nslots)))
        {
            PyErr_SetString(PyExc_ValueError, "code: semantic recovery does not fit code");
            return -1;
        }
        const _PyIRWord *words = (const _PyIRWord *)PyBytes_AS_STRING(con->code);
        if (_PyIR_HandlerBase[IR_HANDLER(words[event.to])] == RESUME) {
            PyErr_SetString(PyExc_ValueError,
                            "code: semantic recovery cannot precede an entry RESUME");
            return -1;
        }
        if (event.handler >= 0) {
            _PyIRWord from = words[event.from];
            int opcode = _PyIR_BaseOpcode(IR_HANDLER(from));
            bool completed = ((opcode == FOR_ITER || opcode == SEND) &&
                              event.edge == _PyIR_EDGE_TAKEN) ||
                             (opcode == CLEANUP_THROW &&
                              event.edge == _PyIR_EDGE_TAKEN);
            int observer = _PyIR_ObserverOutcome(from, event.edge);
            if (observer || completed) {
                int safe = observer
                    ? observer_handler_safe(con, from, event.edge,
                                            event.handler, event.exc_slot,
                                            event.lasti, 1)
                    : completion_handler_safe(con, event.handler,
                                              event.exc_slot, event.lasti,
                                              IR_B(from),
                                              _PyIR_CompletionWindow(opcode));
                if (safe <= 0) {
                    if (safe == 0) {
                        PyErr_SetString(PyExc_ValueError,
                                        "code: source handler reads a retired completion owner");
                    }
                    return -1;
                }
            }
        }
    }
    for (uint32_t i = 0; i < positions.nrecords; i++) {
        _PyIRPosition position;
        _PyIRPositions_Event(&positions, i, &position);
        if ((position.line == -1 &&
             (position.end_line != -1 || position.column != -1 ||
              position.end_column != -1)) ||
            position.line == 0 || position.line < -1 ||
            position.end_line < -1 ||
            (position.end_line != -1 && position.end_line < position.line) ||
            position.column < -1 || position.end_column < -1 ||
            ((position.column == -1) != (position.end_column == -1)) ||
            (position.line == position.end_line &&
             position.column > position.end_column))
        {
            PyErr_SetString(PyExc_ValueError, "code: malformed optional source position");
            return -1;
        }
    }
    return 0;
}

static int
kwargs_word_access(_PyIRWord word, int slot)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int reads = 0;
    for (int f = 0; f < 4; f++) {
        if (fields[f] != (uint32_t)slot) {
            continue;
        }
        int role = fmt->roles[f];
        if (role == _PyIR_ROLE_DST || role == _PyIR_ROLE_LOCAL_OUT ||
            role == _PyIR_ROLE_LOCAL_CLEAR || role == _PyIR_ROLE_LOCAL_MOVE ||
            role == _PyIR_ROLE_CELL)
        {
            return -1;
        }
        reads |= role == _PyIR_ROLE_LOCAL_IN || role >= _PyIR_ROLE_IN0;
    }
    if (fmt->flags & _PyIR_WINDOW) {
        int base = -1;
        for (int f = 0; f < 4; f++) {
            if (fmt->roles[f] == _PyIR_ROLE_BASE) {
                base = (int)fields[f];
            }
        }
        /* A feedback-encoded argument cannot put the local inside a window:
         * generated windows otherwise live above all ordinary locals. */
        if (base >= 0 && base <= slot) {
            int oparg = fmt->imm == _PyIR_LOC_FBARG ? IR_MAX_SLOTS
                                                 : _PyIR_FieldOparg(word);
            int writes = _PyIR_Extent(fmt->writes, oparg);
            if (slot < base + writes) {
                return -1;
            }
            reads |= slot < base + _PyIR_Extent(fmt->reads, oparg);
        }
    }
    return reads;
}

/* Prove that a pending kwargs home is immutable and cannot be read before a
 * materialization barrier. After a barrier the immutable binding stays real.
 * Exception handlers are conservative independent pending entry points. */
static int
validate_virtual_kwargs(struct _PyCodeConstructor *con)
{
    if (!(con->flags & CO_VIRTUAL_KWARGS)) {
        return 0;
    }
    int slot = con->argcount + con->kwonlyargcount + !!(con->flags & CO_VARARGS);
    if (!(con->flags & CO_VARKEYWORDS) ||
        con->flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR) ||
        slot >= PyBytes_GET_SIZE(con->localspluskinds) ||
        (_PyLocals_GetKind(con->localspluskinds, slot) & (CO_FAST_CELL | CO_FAST_FREE)))
    {
        goto invalid;
    }
    int nwords = (int)(PyBytes_GET_SIZE(con->code) / sizeof(_PyIRWord));
    const _PyIRWord *words = (const _PyIRWord *)PyBytes_AS_STRING(con->code);
    unsigned char *seen = PyMem_Calloc((size_t)Py_MAX(nwords, 1), 1);
    int *todo = PyMem_Malloc((size_t)Py_MAX(nwords, 1) * sizeof(int));
    if (seen == NULL || todo == NULL) {
        PyMem_Free(seen);
        PyMem_Free(todo);
        PyErr_NoMemory();
        return -1;
    }
    int count = 0;
    if (nwords) {
        seen[0] = 1;
        todo[count++] = 0;
    }
    for (int i = 0; i < nwords; i++) {
        int handler = IR_HANDLER(words[i]);
        if (handler >= IR_NUM_HANDLERS || !(_PyIR_FormatOf(handler)->flags & _PyIR_VALID) ||
            (handler != MATERIALIZE_KWARGS && kwargs_word_access(words[i], slot) < 0) ||
            (handler == MATERIALIZE_KWARGS && IR_A(words[i]) != (uint32_t)slot))
        {
            goto invalid_allocated;
        }
    }
    _PyIRSemanticMap semantic = _PyIRSemantic_Stored(con->exceptiontable);
    const unsigned char *p = semantic.exceptions;
    const unsigned char *end = p + semantic.exceptions_size;
    while (p < end) {
        unsigned int fields[4] = {0};
        for (int k = 0; k < 4; k++) {
            unsigned int byte;
            do {
                if (p == end || fields[k] > (UINT_MAX >> 6)) {
                    goto invalid_allocated;
                }
                byte = *p++;
                fields[k] = (fields[k] << 6) | (byte & 63);
            } while (byte & 64);
        }
        unsigned int target = fields[2];
        if (target >= (unsigned int)nwords) {
            goto invalid_allocated;
        }
        if (!seen[target]) {
            seen[target] = 1;
            todo[count++] = (int)target;
        }
    }
    _PyIRSemanticMap source;
    if (_PyIRSemantic_Decode(PyBytes_AS_STRING(con->exceptiontable),
                          PyBytes_GET_SIZE(con->exceptiontable), &source) < 0) {
        goto invalid_allocated;
    }
    for (uint32_t j = 0; j < source.nrecords; j++) {
        _PyIRRecoveryEvent event;
        _PyIRSemantic_Event(&source, j, &event);
        if (event.handler >= 0 && !seen[event.handler]) {
            seen[event.handler] = 1;
            todo[count++] = event.handler;
        }
    }
    while (count) {
        int i = todo[--count];
        _PyIRWord word = words[i];
        int handler = IR_HANDLER(word);
        if (handler == MATERIALIZE_KWARGS) {
            continue;
        }
        if (kwargs_word_access(word, slot)) {
            goto invalid_allocated;
        }
        const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
        int base = _PyIR_HandlerBase[_PyIR_BaseOpcode(handler)];
        int next[2] = {i + 1, -1};
        if (fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD)) {
            if (!(fmt->flags & _PyIR_NAMES_TARGET)) {
                next[1] = fmt->flags & (_PyIR_FORWARD | _PyIR_FUSED)
                    ? i + 1 + IR_DS(word) : (int)IR_CD(word);
            }
        }
        if (IS_SCOPE_EXIT_OPCODE(base) || (fmt->flags & _PyIR_JUMP_ALWAYS)) {
            next[0] = -1;
        }
        for (int k = 0; k < 2; k++) {
            int target = next[k];
            if (target < 0 || target == nwords) {
                continue;
            }
            if (target > nwords) {
                goto invalid_allocated;
            }
            if (!seen[target]) {
                seen[target] = 1;
                todo[count++] = target;
            }
        }
    }
    PyMem_Free(seen);
    PyMem_Free(todo);
    return 0;
invalid_allocated:
    PyMem_Free(seen);
    PyMem_Free(todo);
invalid:
    PyErr_SetString(PyExc_ValueError, "code: invalid virtual kwargs binding");
    return -1;
}


typedef struct {
    uint32_t *facts;
    int *queue;
    int n, head, tail, count;
    int homes[8], nhomes;
} completion_worklist;

enum {
    COMPLETION_REACHED = 1u << 24,
    COMPLETION_QUEUED = 1u << 25,
};

/* Low bits mean the original owner is unavailable. The next nhomes bits mean
 * that unavailable home may be non-NULL; OR joins preserve both possibilities. */
static int
completion_enqueue(completion_worklist *work, int target, unsigned int mask,
                   unsigned int nonnull)
{
    unsigned int limit = (1u << work->nhomes) - 1;
    unsigned int facts = (mask & limit) |
                         ((nonnull & mask & limit) << work->nhomes);
    if (target < 0 || target >= work->n) {
        return 0;
    }
    unsigned int old = work->facts[target];
    if ((old & COMPLETION_REACHED) && (old & facts) == facts) {
        return 1;
    }
    work->facts[target] |= facts | COMPLETION_REACHED;
    if (!(old & COMPLETION_QUEUED)) {
        work->facts[target] |= COMPLETION_QUEUED;
        work->queue[work->tail] = target;
        work->tail = (work->tail + 1) % work->n;
        work->count++;
    }
    return 1;
}

static unsigned int
completion_home_mask(int slot, const int *homes, int count)
{
    for (int k = 0; k < count; k++) {
        if (slot == homes[k]) {
            return 1u << k;
        }
    }
    return 0;
}

static int
completion_exception_edge(completion_worklist *work, int target, int slot,
                          int lasti, unsigned int mask,
                          unsigned int nonnull)
{
    if ((lasti != 0 && lasti != 1) || slot < lasti) {
        return 0;
    }
    int floor = slot - lasti;
    for (int k = 0; k < work->nhomes; k++) {
        if (work->homes[k] >= floor) {
            mask |= 1u << k;
            nonnull &= ~(1u << k);
        }
    }
    mask &= ~completion_home_mask(slot, work->homes, work->nhomes);
    if (lasti) {
        mask &= ~completion_home_mask(slot - 1, work->homes, work->nhomes);
    }
    return completion_enqueue(work, target, mask, nonnull);
}

/* A preserved prefix need not contain live semantic operands. Generator
 * StopIteration wrappers, for example, keep an iterator below their real
 * exception/lasti inputs. Prove that completed homes are not read as those
 * old values on any normal or exceptional handler path. This is the final
 * encoded proof for both compiled programs and public code constructors. */
static int
owner_handler_safe(struct _PyCodeConstructor *con, int entry, int exc_slot,
                   int lasti, const int *homes, int count,
                   unsigned int initial, unsigned int initial_nonnull)
{
    if (count <= 0 || count > 8) {
        return 0;
    }
    const _PyIRWord *words = (const _PyIRWord *)PyBytes_AS_STRING(con->code);
    int n = (int)(PyBytes_GET_SIZE(con->code) / sizeof(*words));
    _PyIRSemanticMap source;
    if (_PyIRSemantic_Decode(PyBytes_AS_STRING(con->exceptiontable),
                          PyBytes_GET_SIZE(con->exceptiontable), &source) < 0)
    {
        return 0;
    }
    completion_worklist work = {.n = n, .nhomes = count};
    memcpy(work.homes, homes, (size_t)count * sizeof(int));
    work.facts = PyMem_Calloc((size_t)n, sizeof(*work.facts));
    work.queue = PyMem_Malloc((size_t)n * sizeof(int));
    if (work.facts == NULL || work.queue == NULL) {
        PyMem_Free(work.facts);
        PyMem_Free(work.queue);
        PyErr_NoMemory();
        return -1;
    }
    /* Preserved homes may still be PRE owners. Only the actual unwind, not
     * POST's retirement alone, proves them NULL on both admitted paths. */
    int safe = completion_exception_edge(&work, entry, exc_slot, lasti,
                                         initial, initial_nonnull);
    while (safe && work.count) {
        int i = work.queue[work.head];
        work.head = (work.head + 1) % n;
        work.count--;
        work.facts[i] &= ~COMPLETION_QUEUED;
        unsigned int limit = (1u << count) - 1;
        unsigned int mask = work.facts[i] & limit;
        unsigned int nonnull = (work.facts[i] >> count) & limit;
        _PyIRWord word = words[i];
        int opcode = _PyIR_BaseOpcode(IR_HANDLER(word));
        const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
        int arg = fmt->imm == _PyIR_LOC_FBARG ? IR_MAX_SLOTS
                                              : _PyIR_FieldOparg(word);
        unsigned int reads = 0, defs = 0, nullable = 0;
        for (int k = 0; k < count; k++) {
            int home = work.homes[k];
            if (!(nonnull & (1u << k)) && _PyIR_NullableHome(word, arg, home)) {
                nullable |= 1u << k;
            }
            if (_PyIR_ReadsHome(word, arg, home)) {
                reads |= 1u << k;
            }
            if (_PyIR_DefinesHome(word, arg, home)) {
                defs |= 1u << k;
            }
        }
        /* RERAISE's implicit owner prefix is absent from its encoded reads.
         * Explicit retirement operands still require their original owners. */
        if (reads & mask & ~nullable) {
            PyErr_Format(PyExc_ValueError,
                "code: invalid semantic completion window or handler "
                "(%U:%d; entry=%d pc=%d opcode=%d base=%d count=%d "
                "unavailable=%u may_nonnull=%u reads=%u)", con->qualname, con->firstlineno,
                entry, i, opcode, work.homes[0], count, mask, nonnull, reads);
            safe = -1;
            break;
        }
        const unsigned char *cursor = source.exceptions;
        const unsigned char *end = cursor + source.exceptions_size;
        while (cursor < end) {
            int start, size, target, slot_lasti;
            if (_PyIR_ExceptionVarint(&cursor, end, &start) < 0 ||
                _PyIR_ExceptionVarint(&cursor, end, &size) < 0 ||
                _PyIR_ExceptionVarint(&cursor, end, &target) < 0 ||
                _PyIR_ExceptionVarint(&cursor, end, &slot_lasti) < 0)
            {
                safe = 0;
                break;
            }
            if (i >= start && i - start < size) {
                safe = completion_exception_edge(&work, target,
                    slot_lasti >> 1, slot_lasti & 1, mask, nonnull);
                break;
            }
        }
        if (!safe) {
            break;
        }
        bool jumps = (fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD)) &&
                     !(fmt->flags & _PyIR_NAMES_TARGET);
        for (int taken = 0; taken <= (int)jumps; taken++) {
            if (!taken && (IS_SCOPE_EXIT_OPCODE(_PyIR_HandlerBase[opcode]) ||
                           (fmt->flags & _PyIR_JUMP_ALWAYS)))
            {
                continue;
            }
            int next = !taken ? i + 1
                : fmt->flags & (_PyIR_FORWARD | _PyIR_FUSED)
                    ? i + 1 + IR_DS(word) : (int)IR_CD(word);
            unsigned int edge = mask;
            unsigned int edge_nonnull = nonnull;
            _PyIRSlotOutcome outcome = _PyIR_Outcome(word, taken);
            if (fmt->output_kind == _PyIR_OUTPUT_OUTCOME && !outcome.semantic) {
                safe = 0;
                break;
            }
            if (outcome.semantic) {
                /* Only actual produced values define a valid new input;
                 * a retired NULL home remains unavailable to readers. */
                for (int k = 0; k < count; k++) {
                    if (work.homes[k] >= outcome.first &&
                        work.homes[k] - outcome.first < outcome.count)
                    {
                        edge |= 1u << k;
                        edge_nonnull &= ~(1u << k);
                    }
                }
                edge &= ~completion_home_mask(outcome.dst, work.homes, count);
                if (opcode == FOR_ITER && !taken) {
                    edge &= ~completion_home_mask(IR_B(word) + 1, work.homes, count);
                }
                if (opcode == SEND && !taken) {
                    edge &= ~completion_home_mask(IR_B(word) + 2, work.homes, count);
                }
            }
            else {
                edge &= ~defs;
            }
            for (uint32_t j = 0; j < source.nrecords; j++) {
                _PyIRRecoveryEvent event;
                _PyIRSemantic_Event(&source, j, &event);
                if (event.from == i && event.edge == taken && event.handler >= 0 &&
                    !completion_exception_edge(&work, event.handler, event.exc_slot,
                                               event.lasti, edge, edge_nonnull))
                {
                    safe = 0;
                    break;
                }
            }
            if (!safe) {
                break;
            }
            if (!completion_enqueue(&work, next, edge, edge_nonnull)) {
                safe = 0;
                break;
            }
        }
    }
    PyMem_Free(work.facts);
    PyMem_Free(work.queue);
    return safe;
}

static int
completion_handler_safe(struct _PyCodeConstructor *con, int entry, int exc_slot,
                        int lasti, int base, int count)
{
    int homes[8];
    if (count <= 0 || count > 8) {
        return 0;
    }
    for (int k = 0; k < count; k++) {
        homes[k] = base + k;
    }
    unsigned int initial = (1u << count) - 1;
    return owner_handler_safe(con, entry, exc_slot, lasti, homes, count,
                              initial, initial);
}

static int
observer_handler_safe(struct _PyCodeConstructor *con, _PyIRWord word, int edge,
                      int entry, int exc_slot, int lasti, int post)
{
    int nl = (int)PyTuple_GET_SIZE(con->localsplusnames);
    int limit = nl + con->stacksize;
    _PyIRObservedHomes observed;
    if (!_PyIR_ObservedHomes(word, edge, nl, limit, &observed)) {
        return 0;
    }
    if (observed.count == 0) {
        return 1;
    }
    unsigned int unavailable = 0, may_nonnull = 0;
    for (int k = 0; k < observed.count; k++) {
        int home = observed.home[k];
        if (home < nl || home >= limit) {
            return 0;
        }
        for (int j = 0; j < k; j++) {
            if (observed.home[j] == home) {
                return 0;
            }
        }
        unsigned int bit = 1u << k;
        if (observed.old_cut[k] ||
            observed.physical[k] == _PyIR_OBSERVED_MISSING ||
            (!post && observed.physical[k] != _PyIR_OBSERVED_UNKNOWN))
        {
            unavailable |= bit;
            may_nonnull |= bit;
        }
        else if (observed.physical[k] == _PyIR_OBSERVED_NULL) {
            unavailable |= bit;
        }
    }
    return owner_handler_safe(con, entry, exc_slot, lasti, observed.home,
                              observed.count, unavailable, may_nonnull);
}

/* These words now own completion, rather than returning to frontend cleanup.
 * Validate their complete windows and the anchors their EH recipes preserve. */
static int
validate_completion_words(struct _PyCodeConstructor *con)
{
    const _PyIRWord *words = (const _PyIRWord *)PyBytes_AS_STRING(con->code);
    int n = (int)(PyBytes_GET_SIZE(con->code) / sizeof(*words));
    int nl = (int)PyTuple_GET_SIZE(con->localsplusnames);
    int limit = nl + con->stacksize;
    bool has_owner_proof = false;
    for (int i = 0; i < n; i++) {
        _PyIRWord word = words[i];
        int opcode = _PyIR_BaseOpcode(IR_HANDLER(word));
        int count = _PyIR_CompletionWindow(opcode);
        for (int edge = 0; edge < 2; edge++) {
            if (_PyIR_ObserverOutcome(word, edge)) {
                _PyIRObservedHomes observed;
                if (!_PyIR_ObservedHomes(word, edge, nl, limit, &observed)) {
                    goto invalid;
                }
                has_owner_proof = true;
            }
        }
        if (!count && opcode != RETURN_GENERATOR) {
            continue;
        }
        has_owner_proof = true;
        int base = (int)IR_B(word), dst = (int)IR_A(word);
        if ((count && (base < nl || base > limit - count)) ||
            (dst != IR_NO_DST && dst >= limit) ||
            (opcode == RETURN_GENERATOR && dst != IR_NO_DST) ||
            (opcode == FOR_ITER && dst >= nl && dst != IR_NO_DST &&
             dst != base + 2) ||
            (opcode == YIELD_VALUE && IR_C(word) == 1 && dst != base))
        {
            goto invalid;
        }
        if (opcode == FOR_ITER || opcode == SEND || opcode == CLEANUP_THROW) {
            int target = opcode == CLEANUP_THROW ? (int)IR_CD(word)
                                                : i + 1 + IR_DS(word);
            if (target < 0 || target >= n) {
                goto invalid;
            }
        }
    }
    if (!has_owner_proof) {
        return 0;
    }
    /* Named-local fields cannot borrow scratch homes. The handler proof
     * relies on that distinction when it reads the executable field roles. */
    for (int i = 0; i < n; i++) {
        const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(words[i]));
        uint32_t fields[] = {IR_A(words[i]), IR_B(words[i]),
                             IR_C(words[i]), IR_D(words[i])};
        for (int f = 0; f < 4; f++) {
            if (fmt->roles[f] >= _PyIR_ROLE_LOCAL_IN &&
                fmt->roles[f] <= _PyIR_ROLE_CELL && fields[f] >= (uint32_t)nl)
            {
                goto invalid;
            }
        }
    }
    _PyIRSemanticMap semantic = _PyIRSemantic_Stored(con->exceptiontable);
    const unsigned char *cursor = semantic.exceptions;
    const unsigned char *end = cursor + semantic.exceptions_size;
    if (!_PyIR_ExceptionTableValid(cursor, end, n, nl, limit)) {
        goto invalid;
    }
    cursor = semantic.exceptions;
    while (cursor < end) {
        int start, size, target, slot_lasti;
        if (_PyIR_ExceptionVarint(&cursor, end, &start) < 0 ||
            _PyIR_ExceptionVarint(&cursor, end, &size) < 0 ||
            _PyIR_ExceptionVarint(&cursor, end, &target) < 0 ||
            _PyIR_ExceptionVarint(&cursor, end, &slot_lasti) < 0 ||
            start > n || size > n - start || target >= n)
        {
            goto invalid;
        }
        int slot = slot_lasti >> 1;
        int floor = slot - (slot_lasti & 1);
        if (floor < nl || slot >= limit) {
            goto invalid;
        }
        for (int i = start; i < start + size; i++) {
            int opcode = _PyIR_BaseOpcode(IR_HANDLER(words[i]));
            for (int edge = 0; edge < 2; edge++) {
                if (!_PyIR_ObserverOutcome(words[i], edge)) {
                    continue;
                }
                int pre = observer_handler_safe(con, words[i], edge, target,
                                                slot, slot_lasti & 1, 0);
                int post = pre > 0
                    ? observer_handler_safe(con, words[i], edge, target,
                                            slot, slot_lasti & 1, 1)
                    : pre;
                if (pre < 0 || post < 0) {
                    return -1;
                }
                if (!pre || !post) {
                    goto invalid;
                }
            }
            if (opcode == FOR_ITER || opcode == SEND || opcode == CLEANUP_THROW) {
                int base = IR_B(words[i]), dst = IR_A(words[i]);
                int count = _PyIR_CompletionWindow(opcode);
                if (dst != IR_NO_DST && dst >= nl && dst < floor &&
                    (dst < base || dst >= base + count))
                {
                    goto invalid;
                }
                int safe = completion_handler_safe(con, target, slot,
                                                   slot_lasti & 1, base, count);
                if (safe < 0) {
                    return -1;
                }
                if (!safe) {
                    goto invalid;
                }
            }
        }
    }
    return 0;
invalid:
    PyErr_Format(PyExc_ValueError,
                 "code: invalid semantic completion window or handler (%U:%d)",
                 con->qualname, con->firstlineno);
    return -1;
}

static int
validate_code_structure(struct _PyCodeConstructor *con)
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
    Py_ssize_t nlocalsplus = PyTuple_GET_SIZE(con->localsplusnames);
    if (nlocalsplus > IR_MAX_SLOTS ||
        Py_MAX(con->stacksize, 1) > IR_MAX_SLOTS - nlocalsplus)
    {
        PyErr_SetString(PyExc_ValueError, "code: frame exceeds register limit");
        return -1;
    }
    if (con->fbinit != NULL
        && (!PyBytes_Check(con->fbinit) || !fbinit_is_valid(con->fbinit)))
    {
        PyErr_SetString(PyExc_ValueError, "code: co_fbinit is malformed");
        return -1;
    }
    if (check_constslots(con) < 0) {
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
    const _PyIRWord *words = (const _PyIRWord *)PyBytes_AS_STRING(con->code);
    Py_ssize_t nwords = PyBytes_GET_SIZE(con->code) / sizeof(_PyIRWord);
    for (Py_ssize_t i = 0; i < nwords; i++) {
        int handler = IR_HANDLER(words[i]);
        if (handler >= IR_NUM_HANDLERS || !(_PyIR_FormatOf(handler)->flags & _PyIR_VALID)) {
            PyErr_SetString(PyExc_ValueError, "code: invalid executable handler");
            return -1;
        }
        if (_PyIR_FormatOf(handler)->effects & _PyIR_NATIVE_ONLY) {
            PyErr_SetString(PyExc_ValueError, "code: native-only executable handler");
            return -1;
        }
        if (_PyIR_FormatOf(handler)->effects & _PyIR_LEARNED_ONLY) {
            PyErr_SetString(PyExc_ValueError, "code: learned-only executable handler");
            return -1;
        }
        if (handler == NOP) {
            PyErr_SetString(PyExc_ValueError,
                            "code: empty or release-only executable instruction");
            return -1;
        }
    }
    return 0;
}

#ifdef Py_GIL_DISABLED
static _PyCodeArray * _PyCodeArray_New(Py_ssize_t size);

/* Free a thread-local copy of the bytecode of CO, with its poly tables. */
static void
free_bytecode_copy(PyCodeObject *co, char *bytecode)
{
    PyMem_Free(bytecode);
}
#endif

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
    co->co_weakreflist = NULL;
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
    co->co_nconstslots = 0;
    if (con->constslots != NULL && PyBytes_GET_SIZE(con->constslots)) {
        Py_ssize_t size = PyBytes_GET_SIZE(con->constslots);
        co->co_nconstslots = (int)(size / (2 * sizeof(uint32_t)));
        memcpy((uint32_t *)_PyCode_CONSTSLOTS(co),
               PyBytes_AS_STRING(con->constslots), size);
    }
    co->_co_exec = NULL;
#ifdef Py_GIL_DISABLED
    co->co_tlbc = _PyCodeArray_New(INITIAL_SPECIALIZED_CODE_SIZE);
    if (co->co_tlbc == NULL) {
        return -1;
    }
    co->co_tlbc->entries[0] = (char *)co->co_words;
#endif
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
    _PyIRPositions source;
    if (_PyIRPositions_Decode(PyBytes_AS_STRING(locations),
                          PyBytes_GET_SIZE(locations), &source) < 0)
    {
        PyErr_SetString(PyExc_ValueError, "code: malformed source map");
        return NULL;
    }
    const uint8_t *data = source.locations;
    PyBytesWriter *res = PyBytesWriter_Create(32);
    if (res == NULL) {
        return NULL;
    }
    uint8_t *output = (uint8_t *)PyBytesWriter_GetData(res);
    while (offset < source.locations_size) {
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
        while (offset < source.locations_size &&
            (data[offset] & 128) == 0) {
            offset++;
        }
    }
    PyObject *body = PyBytesWriter_FinishWithPointer(res, output);
    if (body == NULL) {
        return NULL;
    }
    PyObject *result = _PyIRPositions_Pack(
        (const unsigned char *)PyBytes_AS_STRING(body),
        (uint32_t)PyBytes_GET_SIZE(body), source.records, source.nrecords);
    Py_DECREF(body);
    if (result != NULL) {
        _PyIRPositions stripped = _PyIRPositions_Stored(result);
        for (uint32_t i = 0; i < stripped.nrecords; i++) {
            _PyIRPosition event;
            _PyIRPositions_Event(&stripped, i, &event);
            event.column = event.end_column = -1;
            _PyIRPositions_EncodeEvent((unsigned char *)stripped.records +
                                     i * _PyIR_POSITION_RECORD, &event);
        }
    }
    return result;
}

static int
intern_code_constants(struct _PyCodeConstructor *con)
{
#ifdef Py_GIL_DISABLED
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _py_code_state *state = &interp->code_state;
    FT_MUTEX_LOCK(&state->mutex);
#endif
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

/* Only the common admission boundary below calls this allocator. Its private
 * constructor may be adjusted while normalizing locations and frame size. */
static PyCodeObject *
new_code(struct _PyCodeConstructor *con)
{
    if (intern_code_constants(con) < 0) {
        return NULL;
    }

    _PyIRPositions source;
    int source_format = _PyIRPositions_Decode(PyBytes_AS_STRING(con->linetable),
                                          PyBytes_GET_SIZE(con->linetable), &source);
    if (source_format < 0) {
        PyErr_SetString(PyExc_ValueError, "code: malformed source map");
        return NULL;
    }
    PyObject *replacement_locations = source_format
        ? Py_NewRef(con->linetable)
        : normalize_positions(&source,
            (uint32_t)(PyBytes_GET_SIZE(con->code) / sizeof(_PyIRWord)));
    if (replacement_locations == NULL) {
        return NULL;
    }
    con->linetable = replacement_locations;
    // Compact the linetable if we are opted out of debug
    // ranges.
    if (!_Py_GetConfig()->code_debug_ranges) {
        PyObject *stripped = remove_column_info(con->linetable);
        if (stripped == NULL) {
            Py_DECREF(replacement_locations);
            return NULL;
        }
        Py_SETREF(replacement_locations, stripped);
        con->linetable = replacement_locations;
    }

    Py_ssize_t size = PyBytes_GET_SIZE(con->code) / sizeof(_PyIRWord);
    // The feedback table follows the words, two entries to an item
    Py_ssize_t nfb = con->fbinit != NULL
        ? PyBytes_GET_SIZE(con->fbinit) / (Py_ssize_t)sizeof(uint32_t) : 0;
    // The constant-slot table follows, two entries to an item
    Py_ssize_t nconstslots = con->constslots != NULL
        ? PyBytes_GET_SIZE(con->constslots) / (Py_ssize_t)sizeof(uint32_t) / 2
        : 0;
    Py_ssize_t nitems = size + (nfb + 1) / 2 + nconstslots;
    PyCodeObject *co;
#ifdef Py_GIL_DISABLED
    co = PyObject_GC_NewVar(PyCodeObject, &PyCode_Type, nitems);
#else
    co = PyObject_NewVar(PyCodeObject, &PyCode_Type, nitems);
#endif
    if (co == NULL) {
        Py_XDECREF(replacement_locations);
        PyErr_NoMemory();
        return NULL;
    }
    Py_SET_SIZE(co, size);

#ifdef Py_GIL_DISABLED
    co->_co_unique_id = _Py_INVALID_UNIQUE_ID;
#endif

    if (init_code(co, con) < 0) {
        Py_XDECREF(replacement_locations);
        Py_DECREF(co);
        return NULL;
    }

#ifdef Py_GIL_DISABLED
    co->_co_unique_id = _PyObject_AssignUniqueId((PyObject *)co);
    _PyObject_GC_TRACK(co);
#endif
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
    PyValueBorrow value = PyTuple_GET_VALUE(consts, index);
    if (IR_HANDLER(word) == LOAD_CONST_INLINE_INT) {
        return PyValue_IsInlineInt(value);
    }
    return PyValue_IsInlineFloat(value);
}

static int
fb_field_is_valid(const _PyIRFormat *fmt, int handler, uint32_t unit,
                  const uint32_t *table, int nfb)
{
    if (nfb == 0) {
        return 0;
    }
    if (unit == 0) {
        // A fused word keeps its operation in its entry too
        return !((fmt->imm == _PyIR_LOC_FBARG
                  && !(handler >= _PyIR_BINARY_OP_FIRST
                       && handler <= _PyIR_BINARY_OP_FIRST + NB_OPARG_LAST))
                 || (fmt->flags & _PyIR_FUSED));
    }
    int i = _PyIR_FbFind(table, nfb, unit);
    if (i < 0) {
        return 0;
    }
    uint32_t end = i + 1 < nfb ? _PyIR_FBINIT_UNIT(table[i + 1])
                               : _PyIR_FBINIT_ARG(table[0]);
    return _PyIR_FBINIT_KIND(table[i]) == fmt->fb_kind &&
           (end - unit) * 8 >= fmt->fb_size;
}

/* Only after structural/feedback-domain admission. Pinned binary handlers
 * encode the operation in their handler number; other FBARG forms use the
 * immutable feedback initializer, never an execution-cache payload. */
static int
admitted_word_arg(_PyIRWord word, const uint32_t *table, int nfb)
{
    int handler = IR_HANDLER(word);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (fmt->imm != _PyIR_LOC_FBARG) {
        return _PyIR_FieldOparg(word);
    }
    if (handler >= _PyIR_BINARY_OP_FIRST &&
        handler <= _PyIR_BINARY_OP_FIRST + NB_OPARG_LAST)
    {
        return handler - _PyIR_BINARY_OP_FIRST;
    }
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int ordinal = _PyIR_FbFind(table, nfb, fields[fmt->fb - _PyIR_LOC_A]);
    assert(ordinal > 0);
    return (int)_PyIR_FBINIT_ARG(table[ordinal]);
}

static int
normal_window_result_offset(const _PyIRFormat *fmt)
{
    if (fmt->owner_events == 1) {
        const _PyIROwnershipEvent *event = _PyIR_OwnerPrograms + fmt->owner_program;
        if (event->kind == _PyIR_OWNER_NORMAL_CLEAR_INPUTS &&
            (event->flags & _PyIR_NORMAL_WINDOW_RESULT))
        {
            return event->src;
        }
    }
    return -1;
}

/* Check the fields of every admitted image that the
 * interpreter uses as indices without checking them: each handler exists,
 * each slot is in the frame, and each feedback index has an entry, which
 * for an operand kept in the entry is not the shared entry 0.  Returns the
 * words to use, a new reference: code itself, or a copy whose inline
 * constant handlers that no longer fit their constant run LOAD_CONST, as
 * after code.replace(co_consts=...). */
static PyObject *
check_words(struct _PyCodeConstructor *con)
{
    PyObject *code = con->code;
    PyObject *fbinit = con->fbinit;
    PyObject *consts = con->consts;
    int nl = (int)PyTuple_GET_SIZE(con->localsplusnames);
    int nslots = nl + Py_MAX(con->stacksize, 1);
    int nfreevars;
    get_localsplus_counts(con->localsplusnames, con->localspluskinds,
                          NULL, NULL, &nfreevars);
    PyObject *result = Py_NewRef(code);
    const char *bytes = PyBytes_AS_STRING(code);
    Py_ssize_t nwords = PyBytes_GET_SIZE(code) / (Py_ssize_t)sizeof(_PyIRWord);
    const uint32_t *table = fbinit == NULL ? NULL
        : (const uint32_t *)PyBytes_AS_STRING(fbinit);
    int nfb = fbinit == NULL ? 0
        : (int)(PyBytes_GET_SIZE(fbinit) / (Py_ssize_t)sizeof(uint32_t));
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
        int base = -1, dst = -1;
        if ((handler == YIELD_DELEGATED || handler == INSTRUMENTED_YIELD_DELEGATED) &&
            (i + 1 >= nwords || IR_D(word) == IR_A(word) || IR_D(word) == IR_B(word)))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd has invalid delegated suspension operands", i);
            goto error;
        }
        for (int k = 0; k < 4; k++) {
            int role = fmt->roles[k];
            uint32_t value = (uint32_t)(word >> (16 + 12 * k)) & IR_FIELD_MAX;
            if (role == _PyIR_ROLE_BASE) {
                base = (int)value;
            }
            else if (role == _PyIR_ROLE_DST) {
                dst = (int)value;
            }
            if (role == _PyIR_ROLE_DST && value == IR_NO_DST
                && (fmt->flags & (_PyIR_DISCARDS | _PyIR_UNPACKS)))
            {
                continue;
            }
            if ((role == _PyIR_ROLE_DST || role == _PyIR_ROLE_BASE
                 || role >= _PyIR_ROLE_IN0) && value >= (uint32_t)nslots)
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd names slot %u of %d", i, value,
                             nslots);
                goto error;
            }
            if (role >= _PyIR_ROLE_LOCAL_IN && role <= _PyIR_ROLE_CELL &&
                value >= (uint32_t)nl)
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd names local slot %u of %d", i,
                             value, nl);
                goto error;
            }
            if (role >= _PyIR_ROLE_IN0 && value < (uint32_t)nl &&
                !(fmt->borrow_mask & (1u << (role - _PyIR_ROLE_IN0))))
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd cannot borrow local slot %u", i, value);
                goto error;
            }
            // The field is the unit of an entry that fits the handler's
            // type; 0 is the shared header, which only a handler with no
            // operand in its entry can use.
            if (role == _PyIR_ROLE_FB && !fb_field_is_valid(fmt, handler, value,
                                                            table, nfb))
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd has no feedback entry %u", i,
                             value);
                goto error;
            }
        }
        int arg = admitted_word_arg(word, table, nfb);
        int reads = _PyIR_Extent(fmt->reads, arg);
        int writes = _PyIR_Extent(fmt->writes, arg);
        int through = _PyIR_Extent(fmt->through, arg);
        if (reads < 0 || writes < 0 || through < 0 || through > reads ||
            through > writes ||
            (base >= 0 && (base < nl ||
                          base > nslots - Py_MAX(reads, writes))))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd has an invalid register window", i);
            goto error;
        }
        int outputs = writes - through;
        int result_offset = normal_window_result_offset(fmt);
        if (result_offset >= 0 && dst >= nl && dst != IR_NO_DST &&
            dst != base + result_offset)
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd has an invalid window result destination", i);
            goto error;
        }
        if ((fmt->transfer_kind == _PyIR_TRANSFER_COPY && arg < 1) ||
            (fmt->transfer_kind == _PyIR_TRANSFER_SWAP && arg < 2))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd has invalid transfer operands", i);
            goto error;
        }
        if ((fmt->flags & _PyIR_UNPACKS) && dst != IR_NO_DST) {
            if (dst < 0 || dst >= nl || outputs > nl - dst) {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd has an invalid unpack local window", i);
                goto error;
            }
        }
        else if (fmt->output_kind == _PyIR_OUTPUT_REGISTER_DST && outputs > 0) {
            if ((dst == IR_NO_DST && outputs != 1) ||
                (dst != IR_NO_DST &&
                 (dst < 0 || outputs > nslots - dst ||
                  (dst < nl && (!(fmt->flags & _PyIR_STORE_DST) || outputs != 1)))))
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd has an invalid output window", i);
                goto error;
            }
        }
        if (fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD | _PyIR_NAMES_TARGET)) {
            Py_ssize_t target = fmt->flags & _PyIR_FORWARD ? i + 1 + arg : arg;
            if (target < 0 || target >= nwords) {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd has an invalid branch target", i);
                goto error;
            }
        }
        int opcode = _PyIR_BaseOpcode(handler);
        if ((opcode == YIELD_VALUE || opcode == INSTRUMENTED_YIELD_VALUE) &&
            handler != YIELD_DELEGATED &&
            handler != INSTRUMENTED_YIELD_DELEGATED && arg != 0)
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd needs an explicit delegated yield", i);
            goto error;
        }
        if (opcode == COPY_FREE_VARS && arg != nfreevars) {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd has an invalid free-variable count", i);
            goto error;
        }
        if ((opcode == LOAD_COMMON_CONSTANT && arg >= NUM_COMMON_CONSTANTS) ||
            (opcode == LOAD_SMALL_INT && arg >= _PY_NSMALLPOSINTS))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd has an invalid constant index", i);
            goto error;
        }
        if (OPCODE_HAS_CONST(opcode) &&
            (arg < 0 || arg >= PyTuple_GET_SIZE(consts)))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %zd has an invalid constant index", i);
            goto error;
        }
        if (OPCODE_HAS_NAME(opcode)) {
            int index = arg;
            if (opcode == LOAD_GLOBAL || opcode == LOAD_ATTR) {
                index >>= 1;
            }
            else if (opcode == LOAD_SUPER_ATTR || opcode == IMPORT_NAME) {
                index >>= 2;
            }
            if (index < 0 || index >= PyTuple_GET_SIZE(con->names)) {
                PyErr_Format(PyExc_ValueError,
                             "code: word %zd has an invalid name index", i);
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

typedef struct {
    const _PyIRWord *words;
    int n;
    int nl;
    int scratch;
    const uint32_t *fbinit;
    int nfb;
    _PyIRSemanticMap semantic;
} semantic_image;

enum { SEM_DELEGATED_NONE, SEM_DELEGATED_KNOWN, SEM_DELEGATED_UNKNOWN };

typedef struct {
    int receiver, outgoing, origin;
    unsigned char kind;
    unsigned char reserved[3];
} semantic_delegated;
static_assert(sizeof(semantic_delegated) == 16, "bounded delegated role tuple");

typedef struct {
    int start;
    int end;
    unsigned char reached;
    unsigned char queued;
    unsigned char complete;
    semantic_delegated delegated;
} semantic_block;

typedef struct {
    Py_uhash_t hash;
    uint32_t first, count;
    const _PyIRPhaseRange *ranges;
} semantic_shape;

typedef struct {
    _PyIRPhaseSite *sites;
    _PyIRPhaseRange *ranges, *scratch;
    uint32_t nsites, site_capacity, nranges, range_capacity;
    _Py_hashtable_t *shapes;
} semantic_phases;

typedef struct {
    unsigned int facts[4];
    uint32_t stamps[4];
    uint32_t published[4];
    unsigned int value;
    uint32_t value_stamp;
    uint8_t value_home, valid, widens_nonnull, value_ready;
} semantic_scalar_state;

typedef struct {
    int nblocks;
    int nwords;
    int head;
    int tail;
    int count;
    int *queue;
    semantic_block *blocks;
    uint64_t *facts;
    uint64_t *branch;
    semantic_phases *phases;
    semantic_scalar_state *error_nodes;
    uint32_t error_capacity;
    int created_empty;
    int completion_destination_empty;
    int normal_null_destination_empty;
    semantic_delegated delegated;
} semantic_worklist;

/* NULL, nonNULL and logical initialization are must-facts. Owner obligations
 * are may-facts: an owner on either incoming path still needs retirement.
 * Such a bit neither proves readability nor promises a mortal count; values
 * can be inline/immortal and another path can have an uninitialized home.
 * NO_HOME separately proves no logical value remains, including an inline or
 * index control value. Canonical NULL is empty; nonowning child header residue
 * need not be NULL. Unknown never grants this must-fact. */
enum {
    SEM_NULL, SEM_INITIALIZED, SEM_MAY_OWNER, SEM_NONNULL, SEM_NO_HOME, SEM_FACTS
};

static void
semantic_enqueue(semantic_worklist *work, int block, const uint64_t *state,
                 int complete)
{
    semantic_block *b = &work->blocks[block];
    uint64_t *stored = work->facts + (size_t)block * SEM_FACTS * work->nwords;
    int changed = !b->reached;
    semantic_delegated next = work->delegated;
    if (b->reached) {
        const semantic_delegated *old = &b->delegated;
        if (old->kind == SEM_DELEGATED_NONE && next.kind == SEM_DELEGATED_NONE) {
            next = *old;
        }
        else if (old->kind == SEM_DELEGATED_KNOWN && next.kind == SEM_DELEGATED_KNOWN &&
                 old->receiver == next.receiver && old->outgoing == next.outgoing)
        {
            if (old->origin != next.origin) {
                next.origin = -1;
            }
        }
        else {
            next = (semantic_delegated){.kind = SEM_DELEGATED_UNKNOWN};
        }
    }
    changed |= memcmp(&next, &b->delegated, sizeof(next)) != 0;
    b->delegated = next;
    complete = !b->reached ? complete : b->complete && complete;
    changed |= b->complete != complete;
    b->complete = (unsigned char)complete;
    for (int w = 0; w < SEM_FACTS * work->nwords; w++) {
        uint64_t next = !b->reached ? state[w]
            : w / work->nwords == SEM_MAY_OWNER ? stored[w] | state[w]
                                               : stored[w] & state[w];
        changed |= next != stored[w];
        stored[w] = next;
    }
    b->reached = 1;
    if (changed && !b->queued) {
        b->queued = 1;
        work->queue[work->tail] = block;
        work->tail = (work->tail + 1) % work->nblocks;
        work->count++;
    }
}

static void
semantic_exception_enqueue(semantic_worklist *work, int block,
                           const uint64_t *state)
{
    semantic_delegated normal = work->delegated;
    work->delegated = (semantic_delegated){.kind = SEM_DELEGATED_UNKNOWN};
    semantic_enqueue(work, block, state, 0);
    work->delegated = normal;
}

static unsigned int
semantic_home_facts(const semantic_worklist *work, const uint64_t *state,
                    int home, int nl)
{
    if (home < nl) {
        /* Named bindings are initialized but may be unbound. Their boundness
         * checks and local owner mutations remain runtime operations. */
        return (1u << SEM_INITIALIZED) | (1u << SEM_MAY_OWNER);
    }
    int bit = home - nl;
    unsigned int facts = 0;
    for (int f = 0; f < SEM_FACTS; f++) {
        facts |= ((state[f * work->nwords + bit / 64] >> (bit % 64)) & 1) << f;
    }
    return facts;
}

static Py_uhash_t
semantic_shape_hash(const void *key)
{
    return ((const semantic_shape *)key)->hash;
}

static int
semantic_shape_equal(const void *left, const void *right)
{
    const semantic_shape *a = left, *b = right;
    return a->count == b->count &&
        memcmp(a->ranges, b->ranges, (size_t)a->count * sizeof(*a->ranges)) == 0;
}

static void *
semantic_reserve(void *array, uint32_t *capacity, uint32_t count, size_t size)
{
    if (count <= *capacity) {
        return array;
    }
    uint32_t next = *capacity > UINT32_MAX / 2 ? UINT32_MAX : *capacity * 2;
    next = Py_MAX(Py_MAX(next, count), 8u);
    if (next > SIZE_MAX / size) {
        PyErr_NoMemory();
        return NULL;
    }
    void *result = PyMem_Realloc(array, (size_t)next * size);
    if (result == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    *capacity = next;
    return result;
}

static int
semantic_phase(const semantic_image *image, const semantic_worklist *work,
               const uint64_t *state, int site, uint8_t selector_class,
               uint8_t phase, int complete)
{
    semantic_phases *build = work->phases;
    if (build == NULL || !complete) {
        return 0;
    }
    if (build->scratch == NULL) {
        build->scratch = PyMem_Malloc((size_t)Py_MAX(image->scratch, 1) *
                                      sizeof(*build->scratch));
        build->shapes = _Py_hashtable_new_full(
            semantic_shape_hash, semantic_shape_equal, PyMem_Free, NULL, NULL);
        if (build->scratch == NULL || build->shapes == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }
    uint32_t nranges = 0;
    _PyIRHomeAuthority authority = _PyIR_HOME_MUST_READABLE;
    for (int bit = 0; bit < image->scratch; bit++) {
        unsigned int facts = semantic_home_facts(work, state, image->nl + bit, image->nl);
        uint8_t flags = facts & (1u << SEM_NONNULL) ? 0 : _PyIR_PHASE_RANGE_NULLABLE;
        if (!(facts & (1u << SEM_MAY_OWNER))) {
            if (selector_class == _PyIR_SELECTOR_ERROR) {
                if (!(facts & (1u << SEM_INITIALIZED))) {
                    if (!(facts & (1u << SEM_NO_HOME))) {
                        return 0;
                    }
                    continue;
                }
                /* A readable NULL/index control participates in a snapshot
                 * even though it carries no counted-owner obligation. */
                flags |= _PyIR_PHASE_RANGE_CONTROL;
            }
            else {
                if (!(facts & (1u << SEM_NULL))) {
                    return 0;
                }
                if (facts & (1u << SEM_INITIALIZED)) {
                    authority = _PyIR_HOME_SHADOW_CANDIDATE;
                }
                continue;
            }
        }
        if (!(facts & (1u << SEM_INITIALIZED))) {
            if (selector_class == _PyIR_SELECTOR_ERROR) {
                return 0;
            }
            authority = _PyIR_HOME_SHADOW_CANDIDATE;
        }
        if (nranges && build->scratch[nranges - 1].end == bit &&
            build->scratch[nranges - 1].flags == flags)
        {
            build->scratch[nranges - 1].end++;
        }
        else {
            build->scratch[nranges++] = (_PyIRPhaseRange){
                .begin = (uint16_t)bit, .end = (uint16_t)(bit + 1), .flags = flags};
        }
    }
    semantic_shape probe = {
        .count = nranges, .ranges = build->scratch,
        .hash = (Py_uhash_t)Py_HashBuffer(
            build->scratch, (Py_ssize_t)nranges * sizeof(*build->scratch)),
    };
#ifdef Py_DEBUG
    for (uint32_t i = 0; i < nranges; i++) {
        assert(build->scratch[i].begin < build->scratch[i].end);
        assert(build->scratch[i].end <= image->scratch);
        assert(i == 0 || build->scratch[i - 1].end <= build->scratch[i].begin);
        assert(!(build->scratch[i].flags & ~(_PyIR_PHASE_RANGE_NULLABLE | _PyIR_PHASE_RANGE_CONTROL)));
    }
#endif
    _Py_hashtable_entry_t *entry = _Py_hashtable_get_entry(build->shapes, &probe);
    uint32_t first;
    if (entry != NULL) {
        first = ((semantic_shape *)entry->key)->first;
    }
    else {
        if (nranges > UINT32_MAX - build->nranges) {
            PyErr_NoMemory();
            return -1;
        }
        first = build->nranges;
        if (nranges) {
            _PyIRPhaseRange *ranges = semantic_reserve(
                build->ranges, &build->range_capacity, first + nranges, sizeof(*ranges));
            if (ranges == NULL) {
                return -1;
            }
            build->ranges = ranges;
            memcpy(ranges + first, build->scratch, (size_t)nranges * sizeof(*ranges));
        }
        semantic_shape *key = PyMem_Malloc(sizeof(*key) +
            (size_t)nranges * sizeof(*build->scratch));
        if (key == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        *key = probe;
        key->first = first;
        key->ranges = (const _PyIRPhaseRange *)(key + 1);
        memcpy((void *)key->ranges, build->scratch,
               (size_t)nranges * sizeof(*build->scratch));
        if (_Py_hashtable_set(build->shapes, key, NULL) < 0) {
            PyMem_Free(key);
            PyErr_NoMemory();
            return -1;
        }
        build->nranges += nranges;
    }
    if (build->nsites == UINT32_MAX) {
        PyErr_NoMemory();
        return -1;
    }
    _PyIRPhaseSite *sites = semantic_reserve(
        build->sites, &build->site_capacity, build->nsites + 1, sizeof(*sites));
    if (sites == NULL) {
        return -1;
    }
    build->sites = sites;
    sites[build->nsites++] = (_PyIRPhaseSite){
        .site = (uint32_t)site, .phase = phase, .authority = (uint8_t)authority,
        .selector_class = selector_class,
        .first = first, .count = nranges};
    return 0;
}

static void semantic_set_facts(const semantic_worklist *, uint64_t *, int, int,
                               unsigned int);

/* Replay only the finite encoded scalar homes, coalescing physical aliases
 * before any SAVE or VACATE. Native-value facts are immutable snapshots;
 * callback widening changes frame homes, not an already owned C value. */
static int
semantic_scalar_actions(const semantic_image *image, semantic_worklist *work,
                        const uint64_t *incoming, _PyIRWord word,
                        const _PyIROwnershipEvent *header,
                        int homes[4], int *nhomes)
{
    uint32_t count = (uint32_t)header->src + 1;
    semantic_scalar_state *nodes = semantic_reserve(
        work->error_nodes, &work->error_capacity, count, sizeof(*nodes));
    if (nodes == NULL) {
        return -1;
    }
    work->error_nodes = nodes;
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    uint8_t aliases[4] = {4, 4, 4, 4};
    *nhomes = 0;
    nodes[0] = (semantic_scalar_state){.valid = 1, .value_home = 4};
    for (int field = 0; field < 4; field++) {
        int role = fmt->roles[field];
        if (role != _PyIR_ROLE_DST && role != _PyIR_ROLE_BASE &&
            role < _PyIR_ROLE_IN0)
        {
            continue;
        }
        int home = (int)((word >> (16 + 12 * field)) & IR_FIELD_MAX);
        if (home == IR_NO_DST) {
            continue;
        }
        int at = 0;
        while (at < *nhomes && homes[at] != home) {
            at++;
        }
        if (at == *nhomes) {
            homes[at] = home;
            nodes[0].facts[at] = semantic_home_facts(work, incoming, home, image->nl);
            (*nhomes)++;
        }
        aliases[field] = (uint8_t)at;
    }
    for (uint32_t at = 1; at < count; at++) {
        const _PyIROwnershipEvent *event = header + at;
        assert(event->dst < at);
        semantic_scalar_state *node = nodes + at;
        *node = nodes[event->dst];
        node->value = 0;
        node->value_ready = 0;
        node->value_home = 4;
        node->value_stamp = UINT32_MAX;
        if (event->kind == _PyIR_OWNER_STORAGE_JOIN) {
            assert(event->extent < at);
            const semantic_scalar_state *right = nodes + event->extent;
            node->valid &= right->valid;
            node->widens_nonnull |= right->widens_nonnull;
            unsigned int may = 1u << SEM_MAY_OWNER;
            for (int h = 0; h < *nhomes; h++) {
                node->facts[h] = ((node->facts[h] & right->facts[h]) & ~may) |
                                 ((node->facts[h] | right->facts[h]) & may);
                if (node->stamps[h] != right->stamps[h]) {
                    node->stamps[h] = UINT32_MAX;
                }
                if (node->published[h] != right->published[h]) {
                    node->published[h] = 0;
                }
            }
            continue;
        }
        if (event->kind == _PyIR_OWNER_STORAGE_CALLBACK) {
            if (event->flags & _PyIR_STORAGE_CALLBACK_DST) {
                assert(event->src < 4 && fmt->roles[event->src] == _PyIR_ROLE_DST);
                int dst = (int)((word >> (16 + 12 * event->src)) & IR_FIELD_MAX);
                if (dst >= image->nl && dst != IR_NO_DST) {
                    continue;
                }
            }
            int current_identity = !!(event->flags & _PyIR_STORAGE_CALLBACK_CURRENT_IDENTITY);
            node->widens_nonnull |= !current_identity;
            for (int h = 0; h < *nhomes; h++) {
                if (current_identity && homes[h] >= image->nl) {
                    continue;
                }
                node->facts[h] &= ~(1u << SEM_NONNULL);
                node->stamps[h] = at;
                node->published[h] = 0;
            }
            continue;
        }
        if (event->kind == _PyIR_OWNER_STORAGE_LITERAL) {
            node->value_ready = 1;
            node->value = (1u << SEM_INITIALIZED) |
                (event->flags & _PyIR_STORAGE_LITERAL_NONNULL ?
                    (1u << SEM_NONNULL) : (1u << SEM_NULL) | (1u << SEM_NO_HOME));
            continue;
        }
        if (event->kind == _PyIR_OWNER_STORAGE_RESULT) {
            /* A native result may be TEMP/private. Only the actual
             * normalizing publication can make it a canonical frame cell. */
            node->value_ready = 1;
            node->value = (1u << SEM_MAY_OWNER) |
                (event->flags & _PyIR_STORAGE_RESULT_CANONICAL ? 1u << SEM_INITIALIZED : 0) |
                (event->flags & _PyIR_STORAGE_RESULT_NONNULL ? 1u << SEM_NONNULL : 0);
            continue;
        }
        if (event->kind == _PyIR_OWNER_STORAGE_GUARD) {
            assert(event->extent > 0 && event->extent < at);
            const semantic_scalar_state *value = nodes + event->extent;
            node->valid &= value->valid && value->value_ready;
            node->value_ready = value->value_ready;
            node->value = event->flags & _PyIR_STORAGE_GUARD_NONNULL ?
                (value->value & ~((1u << SEM_NULL) | (1u << SEM_NO_HOME))) |
                    (1u << SEM_NONNULL) :
                (1u << SEM_INITIALIZED) | (1u << SEM_NULL) | (1u << SEM_NO_HOME);
            continue;
        }
        if (event->kind == _PyIR_OWNER_STORAGE_COPY) {
            assert(event->extent > 0 && event->extent < at);
            const semantic_scalar_state *value = nodes + event->extent;
            node->valid &= value->valid;
            if (!(event->flags & _PyIR_STORAGE_COPY_OWNED_SOURCE)) {
                node->valid &= value->value_home < 4;
                if (value->value_home < 4) {
                    node->valid &= value->value_stamp == node->stamps[value->value_home];
                }
            }
            node->value = value->value;
            node->value_ready = value->value_ready;
            /* DUP owns a distinct count. It is never permission to replace
             * an owner still stored in the source home. */
            continue;
        }
        assert(event->src < 4);
        if (event->kind == _PyIR_OWNER_STORAGE_PUBLISH) {
            assert(event->extent > 0 && event->extent < at);
            const semantic_scalar_state *value = nodes + event->extent;
            node->valid &= value->valid && value->value_ready &&
                (!(event->flags & _PyIR_STORAGE_PUBLISH_CANONICAL) ||
                 !!(value->value & (1u << SEM_INITIALIZED)));
        }
        int h = aliases[event->src];
        if (h == 4) {
            /* A discarded recipient consumes its native owner without
             * publishing a home. Other missing operands are uncovered. */
            node->valid &= event->kind == _PyIR_OWNER_STORAGE_PUBLISH &&
                           fmt->roles[event->src] == _PyIR_ROLE_DST;
            continue;
        }
        switch (event->kind) {
            case _PyIR_OWNER_STORAGE_SAVE:
                node->value = node->facts[h];
                node->value_ready = !!(node->value & (1u << SEM_INITIALIZED));
                node->value_home = (uint8_t)h;
                node->value_stamp = node->stamps[h];
                node->valid &= !!(node->value & (1u << SEM_INITIALIZED));
                node->valid &= !!(event->flags & _PyIR_STORAGE_READ_NULLABLE) ||
                               !!(node->value & (1u << SEM_NONNULL));
                break;
            case _PyIR_OWNER_STORAGE_VACATE:
                if (homes[h] >= image->nl) {
                    node->valid &= !(event->flags & _PyIR_STORAGE_TAKES_HOME) ||
                                   !(node->facts[h] & (1u << SEM_NO_HOME));
                    node->facts[h] = 1u << SEM_NO_HOME;
                    node->stamps[h] = at;
                    node->published[h] = 0;
                }
                break;
            case _PyIR_OWNER_STORAGE_PUBLISH: {
                assert(event->extent > 0 && event->extent < at);
                const semantic_scalar_state *value = nodes + event->extent;
                if (homes[h] < image->nl) {
                    break;
                }
                if (!(node->facts[h] & (1u << SEM_NO_HOME))) {
                    node->valid &= !!(event->flags & _PyIR_STORAGE_PUBLISH_SAME_VALUE) &&
                        (node->published[h] == event->extent ||
                         (value->value_home == h && value->value_stamp == node->stamps[h]));
                }
                node->facts[h] = value->value | (1u << SEM_INITIALIZED);
                node->stamps[h] = at;
                node->published[h] = event->extent;
                break;
            }
            default:
                node->valid = 0;
                break;
        }
    }
    return 0;
}

/* The suffix describes reached, post-flush snapshots emitted by the same
 * Storage instance as the C error transfer. SAVE captures native value facts
 * before later home writes; callbacks widen only frame-home facts. */
static int
semantic_error_snapshots(const semantic_image *image, semantic_worklist *work,
                         const uint64_t *state, int pc, int complete)
{
    _PyIRWord word = image->words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (!complete || work->phases == NULL || !(fmt->flags & _PyIR_ERROR_PROGRAM)) {
        return 0;
    }
    const _PyIROwnershipEvent *header =
        _PyIR_OwnerPrograms + fmt->owner_program + fmt->owner_events;
    assert(header->kind == _PyIR_OWNER_ERROR_PROGRAM);
    const _PyIROwnershipEvent *end = header + 1 + header->extent;
    assert(header->src <= header->extent);
    int homes[4], nhomes;
    if (semantic_scalar_actions(image, work, state, word, header, homes, &nhomes) < 0) {
        return -1;
    }
    for (const _PyIROwnershipEvent *event = header + 1 + header->src; event < end;) {
        if (event->kind == _PyIR_OWNER_STORAGE_NORMAL) {
            event++;
            continue;
        }
        assert(event->kind == _PyIR_OWNER_ERROR_CHECKPOINT);
        assert(event->src > 0 && event->src <= UINT8_MAX);
        assert(event->extent == 0);
        assert(event->dst <= header->src);
        const semantic_scalar_state *node = work->error_nodes + event->dst;
        int readable = node->valid;
        if (readable) {
            size_t bytes = (size_t)SEM_FACTS * work->nwords * sizeof(uint64_t);
            memcpy(work->branch, state, bytes);
            if (node->widens_nonnull || (event->flags & _PyIR_ERROR_CANONICAL_OR_NULL)) {
                memset(work->branch + SEM_NONNULL * work->nwords, 0,
                       (size_t)work->nwords * sizeof(uint64_t));
            }
            for (int h = 0; h < nhomes; h++) {
                if (homes[h] >= image->nl) {
                    unsigned int facts = node->facts[h];
                    if (event->flags & _PyIR_ERROR_CANONICAL_OR_NULL) {
                        facts &= ~(1u << SEM_NONNULL);
                    }
                    semantic_set_facts(work, work->branch, homes[h], image->nl, facts);
                }
            }
            if (semantic_phase(image, work, work->branch, pc, _PyIR_SELECTOR_ERROR,
                               (uint8_t)event->src, complete) < 0) {
                return -1;
            }
        }
        event++;
    }
    return 0;
}

static int
semantic_phase_compare(const void *left, const void *right)
{
    const _PyIRPhaseSite *a = left, *b = right;
    if (a->site != b->site) {
        return a->site < b->site ? -1 : 1;
    }
    if (a->selector_class != b->selector_class) {
        return (int)a->selector_class - (int)b->selector_class;
    }
    return (int)a->phase - (int)b->phase;
}

static void
semantic_set_facts(const semantic_worklist *work, uint64_t *state,
                   int home, int nl, unsigned int facts)
{
    int bit = home - nl;
    uint64_t mask = UINT64_C(1) << (bit % 64);
    for (int f = 0; f < SEM_FACTS; f++) {
        uint64_t *word = &state[f * work->nwords + bit / 64];
        *word = (*word & ~mask) | ((facts & (1u << f)) ? mask : 0);
    }
}

/* Apply only positively certified normal transfers. Unknown rows lose all
 * logical/owner proof. Existing physical NULL facts survive only where the
 * independent physical transfer API positively certifies preservation.
 * Inspect reads only after the fixed point, so an as-yet-unseen predecessor
 * cannot cause a false rejection. */
static int
semantic_leaf_step(const semantic_image *image, semantic_worklist *work,
              uint64_t *state, int pc, int inspect, int *complete)
{
    _PyIRWord word = image->words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    int nl = image->nl;
    if (fmt->transfer_kind == _PyIR_TRANSFER_UNKNOWN) {
        *complete = 0;
        int arg = admitted_word_arg(word, image->fbinit, image->nfb);
        for (int bit = 0; bit < image->scratch; bit++) {
            uint64_t mask = UINT64_C(1) << (bit % 64);
            _PyIRPhysicalTransfer physical = _PyIR_NormalPhysicalTransfer(
                word, arg, 0, nl + bit, nl, nl + image->scratch);
            if (physical.kind != _PyIR_PHYSICAL_PRESERVE) {
                state[SEM_NULL * work->nwords + bit / 64] &= ~mask;
            }
            int absence = _PyIR_NormalCallAbsence(
                word, arg, nl + bit, nl, nl + image->scratch);
            uint64_t *empty = &state[SEM_NO_HOME * work->nwords + bit / 64];
            if (absence == _PyIR_ABSENCE_SET_EMPTY) {
                *empty |= mask;
            }
            else if (absence == _PyIR_ABSENCE_SET_VALUE ||
                     (absence != _PyIR_ABSENCE_PRESERVE &&
                      physical.kind != _PyIR_PHYSICAL_PRESERVE))
            {
                *empty &= ~mask;
            }
        }
        memset(state + work->nwords, 0,
               (size_t)(SEM_NO_HOME - 1) * work->nwords * sizeof(uint64_t));
        return 0;
    }
    int arg = admitted_word_arg(word, image->fbinit, image->nfb);
    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
    int homes[8], nhomes = 0;
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role == _PyIR_ROLE_DST || role == _PyIR_ROLE_BASE ||
            role >= _PyIR_ROLE_IN0)
        {
            homes[nhomes++] = (int)fields[f];
        }
        if (role == _PyIR_ROLE_BASE && fmt->transfer_kind == _PyIR_TRANSFER_COPY) {
            homes[nhomes++] = (int)fields[f] + arg;
        }
        if (role == _PyIR_ROLE_BASE && fmt->transfer_kind == _PyIR_TRANSFER_SWAP) {
            homes[nhomes++] = (int)fields[f] + arg - 1;
        }
    }
    unsigned int next[8];
    for (int h = 0; h < nhomes; h++) {
        int home = homes[h];
        next[h] = 0;
        if (home < nl || home == IR_NO_DST) {
            continue;
        }
        unsigned int old = semantic_home_facts(work, state, home, nl);
        /* A certified NULL-safe retirement can close an empty home. This is
         * not permission for completion/observer recovery to read an old
         * retired owner; those existing provenance proofs remain separate. */
        int accepts_empty = IR_HANDLER(word) == POP_TOP ||
            (fmt->transfer_kind == _PyIR_TRANSFER_LOCAL_STORE &&
             (old & (1u << SEM_INITIALIZED)));
        if (_PyIR_ReadsHome(word, arg, home) && !accepts_empty &&
            !_PyIR_NullableHome(word, arg, home) &&
            !(old & (1u << SEM_INITIALIZED)))
        {
            *complete = 0;
        }
        if (inspect && (old & (1u << SEM_NULL)) &&
            _PyIR_ReadsHome(word, arg, home) && !accepts_empty &&
            !_PyIR_NullableHome(word, arg, home))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %d reads unavailable scratch home %d", pc, home);
            return -1;
        }
        _PyIRSemanticTransfer transfer = _PyIR_NormalSemanticTransfer(word, arg, home, nl);
        if (inspect && (old & (1u << SEM_MAY_OWNER)) &&
            (transfer.kind == _PyIR_SEMANTIC_FRESH ||
             transfer.kind == _PyIR_SEMANTIC_DUPLICATE ||
             (transfer.kind == _PyIR_SEMANTIC_MOVE && transfer.source < nl)))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %d overwrites scratch owner %d", pc, home);
            return -1;
        }
        switch (transfer.kind) {
            case _PyIR_SEMANTIC_PRESERVE:
                next[h] = old;
                break;
            case _PyIR_SEMANTIC_RELEASE:
                next[h] = (1u << SEM_NULL) | (1u << SEM_NO_HOME);
                break;
            case _PyIR_SEMANTIC_FRESH:
                next[h] = (1u << SEM_INITIALIZED) | (1u << SEM_MAY_OWNER) |
                          (1u << SEM_NONNULL);
                break;
            case _PyIR_SEMANTIC_NULL:
                next[h] = (1u << SEM_NULL) | (1u << SEM_INITIALIZED) |
                          (1u << SEM_NO_HOME);
                break;
            case _PyIR_SEMANTIC_DUPLICATE:
            case _PyIR_SEMANTIC_MOVE:
                next[h] = semantic_home_facts(work, state, transfer.source, nl);
                if (transfer.kind == _PyIR_SEMANTIC_DUPLICATE && transfer.source < nl) {
                    next[h] |= 1u << SEM_NONNULL;
                }
                break;
            default:
                Py_UNREACHABLE();
        }
    }
    for (int h = 0; h < nhomes; h++) {
        if (homes[h] >= nl && homes[h] != IR_NO_DST) {
            semantic_set_facts(work, state, homes[h], nl, next[h]);
        }
    }
    return 0;
}

static int
semantic_empty_return(int handler)
{
    return handler == RETURN_VALUE_EMPTY ||
           handler == INSTRUMENTED_RETURN_VALUE_EMPTY;
}

/* This is logical vacancy, independent of physical NULL/readable maps.
 * The fast return never examines scratch homes after transferring its result. */
static int
semantic_scratch_absent(const semantic_image *image,
                        const semantic_worklist *work, const uint64_t *state)
{
    for (int w = 0; w < work->nwords; w++) {
        int remaining = image->scratch - w * 64;
        uint64_t mask = remaining >= 64 ? UINT64_MAX :
            (UINT64_C(1) << remaining) - 1;
        if ((state[SEM_NO_HOME * work->nwords + w] & mask) != mask ||
            (state[SEM_MAY_OWNER * work->nwords + w] & mask))
        {
            return 0;
        }
    }
    return 1;
}

static int
semantic_input_home(_PyIRWord word, const _PyIRFormat *fmt, int input)
{
    for (int field = 0; field < 4; field++) {
        if (fmt->roles[field] == _PyIR_ROLE_IN0 + input) {
            return (int)((word >> (16 + field * 12)) & IR_FIELD_MAX);
        }
    }
    Py_UNREACHABLE();
}

/* The event census includes unreachable sites; only the fixed-point replay
 * can certify a reached site's incoming shape. */
static int
semantic_has_created_copy(const _PyIRFormat *fmt)
{
    for (int i = 0; i < fmt->owner_events; i++) {
        if (_PyIR_OwnerPrograms[fmt->owner_program + i].kind ==
            _PyIR_OWNER_CREATED_COPY)
        {
            return 1;
        }
    }
    return 0;
}

static int
semantic_empty_scratch(const semantic_image *image,
                       const semantic_worklist *work, const uint64_t *state)
{
    for (int bit = 0; bit < image->scratch; bit++) {
        uint64_t mask = UINT64_C(1) << (bit % 64);
        if (!(state[SEM_NULL * work->nwords + bit / 64] & mask) ||
            ((state[SEM_INITIALIZED * work->nwords + bit / 64] |
              state[SEM_MAY_OWNER * work->nwords + bit / 64]) & mask))
        {
            return 0;
        }
    }
    return 1;
}

/* One pending pair suffices for the frontend SEND -> delegated-yield
 * protocol. It denotes role homes, not object identity or a complete map. */
static int
semantic_delegated_step(const semantic_image *image, semantic_worklist *work,
                        const uint64_t *state, int pc, int inspect)
{
    _PyIRWord word = image->words[pc];
    int handler = IR_HANDLER(word);
    if (handler == YIELD_DELEGATED || handler == INSTRUMENTED_YIELD_DELEGATED) {
        semantic_delegated pair = work->delegated;
        int dst = (int)IR_A(word);
        int valid = pair.kind == SEM_DELEGATED_KNOWN &&
            pair.receiver == (int)IR_D(word) && pair.outgoing == (int)IR_B(word) &&
            (dst < image->nl || dst == IR_NO_DST || dst == pair.outgoing ||
             (semantic_home_facts(work, state, dst, image->nl) & (1u << SEM_NO_HOME)));
        work->delegated = (semantic_delegated){.kind = SEM_DELEGATED_NONE};
        if (inspect && !valid) {
            PyErr_Format(PyExc_ValueError,
                         "code: word %d has no delegated SEND role proof", pc);
            return -1;
        }
        return 0;
    }
    if (work->delegated.kind != SEM_DELEGATED_KNOWN) {
        return 0;
    }
    int arg = admitted_word_arg(word, image->fbinit, image->nfb);
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    if (fmt->transfer_kind == _PyIR_TRANSFER_SWAP) {
        int first = (int)IR_A(word), last = first + arg - 1;
        int *homes[] = {&work->delegated.receiver, &work->delegated.outgoing};
        for (int i = 0; i < 2; i++) {
            if (*homes[i] == first) {
                *homes[i] = last;
            }
            else if (*homes[i] == last) {
                *homes[i] = first;
            }
        }
        return 0;
    }
    int homes[] = {work->delegated.receiver, work->delegated.outgoing};
    for (int i = 0; i < 2; i++) {
        if (_PyIR_NormalPhysicalTransfer(word, arg, 0, homes[i], image->nl,
                image->nl + image->scratch).kind != _PyIR_PHYSICAL_PRESERVE)
        {
            work->delegated = (semantic_delegated){.kind = SEM_DELEGATED_UNKNOWN};
            break;
        }
    }
    return 0;
}

static int
semantic_step_core(const semantic_image *image, semantic_worklist *work,
              uint64_t *state, int pc, int inspect, int *complete)
{
    _PyIRWord word = image->words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (inspect && fmt->call_input_holes) {
        int base = (int)IR_B(word);
        int arg = admitted_word_arg(word, image->fbinit, image->nfb);
        for (int k = 0; k < _PyIR_Extent(fmt->reads, arg); k++) {
            if (_PyIR_CallInputRole(fmt, k) == _PyIR_CALL_INPUT_VIRTUAL_NULL &&
                !(semantic_home_facts(work, state, base + k, image->nl) &
                  (1u << SEM_NO_HOME)))
            {
                PyErr_Format(PyExc_ValueError,
                             "code: word %d has an unproved virtual call input %d",
                             pc, base + k);
                return -1;
            }
        }
        int count = _PyIR_Extent(fmt->reads, arg);
        int dst = (int)IR_A(word);
        if (dst >= image->nl && dst != IR_NO_DST &&
            (dst < base || dst >= base + count) &&
            !(semantic_home_facts(work, state, dst, image->nl) & (1u << SEM_NO_HOME)))
        {
            PyErr_Format(PyExc_ValueError,
                         "code: word %d overwrites an unproved call recipient %d", pc, dst);
            return -1;
        }
        if (fmt->effects & _PyIR_PUSHES_FRAME) {
            int first = base + count;
            for (int home = first; home < image->nl + image->scratch; home++) {
                if (home != (int)IR_A(word) &&
                    !(semantic_home_facts(work, state, home, image->nl) &
                      (1u << SEM_NO_HOME)))
                {
                    PyErr_Format(PyExc_ValueError,
                                 "code: word %d has an unproved child clobber home %d",
                                 pc, home);
                    return -1;
                }
            }
        }
    }
    if (fmt->owner_events == 0) {
        assert(fmt->transfer_kind == _PyIR_TRANSFER_UNKNOWN);
        return semantic_leaf_step(image, work, state, pc, inspect, complete);
    }
    const _PyIROwnershipEvent *program = _PyIR_OwnerPrograms + fmt->owner_program;
    if (program[0].kind == _PyIR_OWNER_LEAF) {
        assert(fmt->owner_events == 1 && program[0].src == fmt->transfer_kind);
        return semantic_leaf_step(image, work, state, pc, inspect, complete);
    }
    unsigned int owners[2] = {0};
    int nl = image->nl;
    int exact = *complete;
    int error = -1, error_exact = 0;
    for (int at = 0; at < fmt->owner_events; at++) {
        const _PyIROwnershipEvent *event = program + at;
        int home;
        unsigned int old;
        switch (event->kind) {
            case _PyIR_OWNER_READ:
                home = semantic_input_home(word, fmt, event->src);
                old = semantic_home_facts(work, state, home, nl);
                if (!(old & (1u << SEM_INITIALIZED))) {
                    exact = 0;
                }
                if (inspect && (old & (1u << SEM_NULL))) {
                    PyErr_Format(PyExc_ValueError,
                                 "code: word %d reads unavailable scratch home %d", pc, home);
                    return -1;
                }
                break;
            case _PyIR_OWNER_TAKE:
                assert(event->dst < Py_ARRAY_LENGTH(owners));
                home = semantic_input_home(word, fmt, event->src);
                owners[event->dst] = semantic_home_facts(work, state, home, nl);
                if (home >= nl) {
                    semantic_set_facts(work, state, home, nl,
                                       (1u << SEM_NULL) | (1u << SEM_NO_HOME));
                }
                break;
            case _PyIR_OWNER_RESTORE:
                assert(event->src < Py_ARRAY_LENGTH(owners));
                home = semantic_input_home(word, fmt, event->src);
                if (home >= nl) {
                    semantic_set_facts(work, state, home, nl, owners[event->src]);
                }
                break;
            case _PyIR_OWNER_FRESH:
                home = -1;
                for (int field = 0; field < 4; field++) {
                    if (fmt->roles[field] == _PyIR_ROLE_DST) {
                        home = (int)((word >> (16 + field * 12)) & IR_FIELD_MAX);
                    }
                }
                assert(home >= 0);
                if (home < nl || home == IR_NO_DST) {
                    break;
                }
                old = semantic_home_facts(work, state, home, nl);
                if (inspect && (old & (1u << SEM_MAY_OWNER))) {
                    PyErr_Format(PyExc_ValueError,
                                 "code: word %d overwrites scratch owner %d", pc, home);
                    return -1;
                }
                semantic_set_facts(work, state, home, nl,
                    (1u << SEM_INITIALIZED) | (1u << SEM_MAY_OWNER) |
                    (1u << SEM_NONNULL));
                break;
            case _PyIR_OWNER_PHASE:
                if (inspect && semantic_phase(image, work, state, pc,
                                              _PyIR_SELECTOR_ACTIVE, (uint8_t)event->src, exact) < 0) {
                    return -1;
                }
                break;
            case _PyIR_OWNER_ERROR_BRANCH:
                error = event->dst;
                error_exact = exact;
                if (inspect && work->phases != NULL) {
                    memcpy(work->branch, state,
                           (size_t)SEM_FACTS * work->nwords * sizeof(uint64_t));
                }
                break;
            case _PyIR_OWNER_NORMAL_EXIT:
                /* The generic transaction's normal transport is checked,
                 * but unconverted specializations do not yet close the
                 * whole canonical family. Never grant following exactness. */
                *complete = 0;
                if (inspect && work->phases != NULL && error >= 0) {
                    state = work->branch;
                    exact = error_exact;
                    at = error - 1;
                    error = -1;
                    break;
                }
                return 0;
            case _PyIR_OWNER_ERROR_EXIT:
                return 0;
            case _PyIR_OWNER_CALL_DRAIN:
                return semantic_leaf_step(image, work, state, pc, inspect, complete);
            case _PyIR_OWNER_YIELD_DELEGATED:
                /* The finite SEND role proof authorizes these inputs, not
                 * an arbitrary suspended scratch shape. The outgoing owner
                 * is detached; a later activation supplies the recipient. */
                semantic_set_facts(work, state, (int)IR_B(word), nl,
                                   (1u << SEM_NULL) | (1u << SEM_NO_HOME));
                home = (int)IR_A(word);
                if (home >= nl && home != IR_NO_DST) {
                    semantic_set_facts(work, state, home, nl, 0);
                }
                *complete = 0;
                return 0;
            case _PyIR_OWNER_NORMAL_COMPLETION:
                /* Completion transport belongs to its selected edge. FOR's
                 * body destination is not produced on completion: retain
                 * only its incoming absence. SEND produces an actual A. */
                home = (int)IR_A(word);
                work->completion_destination_empty =
                    !(event->flags & _PyIR_COMPLETION_HAS_RESULT) && home >= nl &&
                    home < nl + image->scratch &&
                    (semantic_home_facts(work, state, home, nl) & (1u << SEM_NO_HOME));
                return semantic_leaf_step(image, work, state, pc, inspect, complete);
            case _PyIR_OWNER_UNPACK_LOCAL_DRAIN:
            case _PyIR_OWNER_NORMAL_CLEAR_INPUTS: {
                int arg = admitted_word_arg(word, image->fbinit, image->nfb);
                /* The actual same-frame successful flush/vacate, with all
                 * family members covered, proves only consumed inputs empty.
                 * It does not make an output readable or grant phase coverage. */
                if (semantic_leaf_step(image, work, state, pc, inspect, complete) < 0) {
                    return -1;
                }
                for (int home = nl; home < nl + image->scratch; home++) {
                    if (_PyIR_ConsumesNormalInput(word, arg, home) &&
                        !_PyIR_DefinesHome(word, arg, home))
                    {
                        uint64_t mask = UINT64_C(1) << ((home - nl) % 64);
                        state[SEM_NO_HOME * work->nwords + (home - nl) / 64] |= mask;
                    }
                }
                int moved_offset = normal_window_result_offset(fmt);
                if (moved_offset >= 0) {
                    int dst = -1, base = -1;
                    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
                    for (int f = 0; f < 4; f++) {
                        if (fmt->roles[f] == _PyIR_ROLE_DST) {
                            dst = (int)fields[f];
                        }
                        else if (fmt->roles[f] == _PyIR_ROLE_BASE) {
                            base = (int)fields[f];
                        }
                    }
                    assert(dst >= 0 && base >= nl);
                    if (dst < nl ||
                        (dst == IR_NO_DST && (fmt->flags & _PyIR_DISCARDS)))
                    {
                        int home = base + moved_offset;
                        assert(home >= nl && home < nl + image->scratch);
                        uint64_t mask = UINT64_C(1) << ((home - nl) % 64);
                        state[SEM_NO_HOME * work->nwords + (home - nl) / 64] |= mask;
                    }
                }
                if (event->kind == _PyIR_OWNER_UNPACK_LOCAL_DRAIN) {
                    assert(fmt->flags & _PyIR_UNPACKS);
                    int dst = -1, base = -1;
                    uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
                    for (int f = 0; f < 4; f++) {
                        if (fmt->roles[f] == _PyIR_ROLE_DST) {
                            dst = (int)fields[f];
                        }
                        else if (fmt->roles[f] == _PyIR_ROLE_BASE) {
                            base = (int)fields[f];
                        }
                    }
                    /* The actual successful helper moves every output to
                     * the validated local run. Sentinel destinations retain
                     * their scratch outputs. Neither route proves values
                     * readable or supplies a physical NULL fact. */
                    if (dst >= 0 && dst < nl) {
                        int count = _PyIR_Extent(fmt->writes, arg);
                        assert(_PyIR_Extent(fmt->through, arg) == 0);
                        assert(base >= nl && base <= nl + image->scratch - count);
                        for (int home = base; home < base + count; home++) {
                            uint64_t mask = UINT64_C(1) << ((home - nl) % 64);
                            state[SEM_NO_HOME * work->nwords + (home - nl) / 64] |= mask;
                        }
                    }
                }
                return 0;
            }
            case _PyIR_OWNER_CREATED_COPY:
                work->created_empty = inspect && exact &&
                    semantic_empty_scratch(image, work, state);
                /* A future activation installs a new send home. It is not
                 * the ordinary successor of the created copy snapshot. */
                *complete = 0;
                /* The next activation supplies the explicit send recipient.
                 * The empty certificate describes the earlier copy only. */
                home = (int)IR_A(word);
                if (home >= nl && home != IR_NO_DST) {
                    semantic_set_facts(work, state, home, nl,
                        (1u << SEM_INITIALIZED) | (1u << SEM_MAY_OWNER));
                }
                return 0;
            default:
                Py_UNREACHABLE();
        }
    }
    Py_UNREACHABLE();
}

static int
semantic_step(const semantic_image *image, semantic_worklist *work,
              uint64_t *state, int pc, int inspect, int *complete)
{
    _PyIRWord word = image->words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    work->created_empty = 0;
    work->completion_destination_empty = 0;
    work->normal_null_destination_empty = 0;
    if (semantic_delegated_step(image, work, state, pc, inspect) < 0) {
        return -1;
    }
    if (fmt->owner_events == 1) {
        const _PyIROwnershipEvent *event = _PyIR_OwnerPrograms + fmt->owner_program;
        if (event->kind == _PyIR_OWNER_NORMAL_CLEAR_INPUTS) {
            int dst = (int)IR_A(word);
            int arg = admitted_word_arg(word, image->fbinit, image->nfb);
            work->normal_null_destination_empty =
                (event->flags & (_PyIR_NORMAL_NULL_FALLTHROUGH | _PyIR_NORMAL_NULL_TAKEN)) &&
                dst >= image->nl && dst != IR_NO_DST &&
                ((semantic_home_facts(work, state, dst, image->nl) & (1u << SEM_NO_HOME)) ||
                 _PyIR_ConsumesNormalInput(word, arg, dst));
        }
    }
    semantic_scalar_state normal = {0};
    int homes[4], nhomes = 0;
    if (fmt->flags & _PyIR_ERROR_PROGRAM) {
        const _PyIROwnershipEvent *header =
            _PyIR_OwnerPrograms + fmt->owner_program + fmt->owner_events;
        const _PyIROwnershipEvent *end = header + 1 + header->extent;
        for (const _PyIROwnershipEvent *event = header + 1 + header->src;
             event < end; event++)
        {
            if (event->kind != _PyIR_OWNER_STORAGE_NORMAL) {
                continue;
            }
            if (semantic_scalar_actions(image, work, state, word, header, homes, &nhomes) < 0) {
                return -1;
            }
            assert(event->dst <= header->src);
            normal = work->error_nodes[event->dst];
            break;
        }
    }
    if (normal.valid) {
        /* This complete emitted graph already checks its reads, native
         * owners and publications. Applying the conservative transfer too
         * would erase incoming facts for homes the graph never touches. */
        if (normal.widens_nonnull) {
            memset(state + SEM_NONNULL * work->nwords, 0,
                   (size_t)work->nwords * sizeof(uint64_t));
        }
        for (int h = 0; h < nhomes; h++) {
            if (homes[h] >= image->nl) {
                semantic_set_facts(work, state, homes[h], image->nl, normal.facts[h]);
            }
        }
        return 0;
    }
    return semantic_step_core(image, work, state, pc, inspect, complete);
}

static void
semantic_normal_edge(const semantic_image *image, semantic_worklist *work,
                     uint64_t *state, int pc, int edge)
{
    _PyIRWord word = image->words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    if (fmt->owner_events != 1) {
        return;
    }
    const _PyIROwnershipEvent *program = _PyIR_OwnerPrograms + fmt->owner_program;
    for (int at = 0; at < fmt->owner_events; at++) {
        const _PyIROwnershipEvent *event = program + at;
        if (event->kind == _PyIR_OWNER_NORMAL_CLEAR_INPUTS &&
            (event->flags & (_PyIR_NORMAL_NULL_FALLTHROUGH << edge)))
        {
            int dst = (int)IR_A(word);
            if (dst >= image->nl && dst != IR_NO_DST &&
                work->normal_null_destination_empty)
            {
                /* A reached final store produced NULL on this edge. It may
                 * replace a consumed input or an already absent home only. */
                semantic_set_facts(work, state, dst, image->nl,
                                   (1u << SEM_NULL) | (1u << SEM_NO_HOME));
            }
            continue;
        }
        if (event->kind != _PyIR_OWNER_NORMAL_COMPLETION) {
            continue;
        }
        if (edge == 0 && (event->flags & _PyIR_COMPLETION_SEND_YIELDED)) {
            assert(event->src == 3 && event->extent == 2);
            /* This is the actual yielded/body publication, independently
             * of A, which belongs to the SEND completion route. */
            work->delegated = (semantic_delegated){
                .receiver = (int)IR_B(word), .outgoing = (int)IR_B(word) + event->extent,
                .origin = pc, .kind = SEM_DELEGATED_KNOWN};
            continue;
        }
        if (event->dst != edge) {
            continue;
        }
        work->delegated = (semantic_delegated){.kind = SEM_DELEGATED_NONE};
        /* The actual completion consumes its full window before any result
         * publication. Logical absence does not imply physical NULL. */
        int has_result = event->flags & _PyIR_COMPLETION_HAS_RESULT;
        assert((has_result ? event->src == 3 || event->src == 4
                           : event->src == 2) && edge == 1);
        int base = (int)IR_B(word);
        assert(base >= image->nl &&
               base <= image->nl + image->scratch - event->src);
        for (int home = base; home < base + event->src; home++) {
            unsigned int old = semantic_home_facts(work, state, home, image->nl);
            semantic_set_facts(work, state, home, image->nl,
                (old & (1u << SEM_NULL)) | (1u << SEM_NO_HOME));
        }
        int dst = (int)IR_A(word);
        if (has_result && dst >= image->nl && dst != IR_NO_DST) {
            uint64_t mask = UINT64_C(1) << ((dst - image->nl) % 64);
            state[SEM_NO_HOME * work->nwords + (dst - image->nl) / 64] &= ~mask;
            state[SEM_NULL * work->nwords + (dst - image->nl) / 64] &= ~mask;
        }
        else if (!has_result && dst >= image->nl && dst != IR_NO_DST &&
            (dst < base || dst >= base + event->src) &&
            work->completion_destination_empty)
        {
            uint64_t mask = UINT64_C(1) << ((dst - image->nl) % 64);
            state[SEM_NO_HOME * work->nwords + (dst - image->nl) / 64] |= mask;
        }
    }
}

/* Common-image normal-edge hardening, still under NULL authority. Per-block
 * bitvectors cover the actual scratch extent. Allocation is O(words + blocks
 * * ceil(scratch/64)); each must-bit can disappear once and each owner
 * obligation can appear once at a block join.
 * Unsupported families/exception entries have explicit unknown state and
 * cannot authorize the later ownership representation switch. */
/* Zero-row plans are shared: exec teardown/size accounting frees and charges
 * only plans with actual site/phase rows. Cover every nonempty flag subset. */
static const _PyIRCodePlan all_normal_returns = {
    .flags = _PyIR_PLAN_ALL_RETURNS};
static const _PyIRCodePlan all_created_empty = {
    .flags = _PyIR_PLAN_ALL_CREATED_EMPTY};
static const _PyIRCodePlan all_returns_created_empty = {
    .flags = _PyIR_PLAN_ALL_RETURNS | _PyIR_PLAN_ALL_CREATED_EMPTY};

static const _PyIRCodePlan *
flag_only_plan(uint32_t flags)
{
    switch (flags) {
        case _PyIR_PLAN_ALL_RETURNS:
            return &all_normal_returns;
        case _PyIR_PLAN_ALL_CREATED_EMPTY:
            return &all_created_empty;
        default:
            assert(flags == (_PyIR_PLAN_ALL_RETURNS | _PyIR_PLAN_ALL_CREATED_EMPTY));
            return &all_returns_created_empty;
    }
}

static int
semantic_exact_return(const semantic_image *image, const semantic_worklist *work,
                      const uint64_t *state, int site, int complete)
{
    if (!complete || _PyIR_FormatOf(IR_HANDLER(image->words[site]))->
        transfer_kind != _PyIR_TRANSFER_EXIT)
    {
        return 0;
    }
    for (int bit = 0; bit < image->scratch; bit++) {
        uint64_t mask = UINT64_C(1) << (bit % 64);
        if (!(state[bit / 64] & mask) ||
            (state[SEM_MAY_OWNER * work->nwords + bit / 64] & mask))
        {
            return 0;
        }
    }
    return 1;
}

/* exception_unwind clears [slot-lasti,N), then installs exception at slot
 * and optional lasti below it. Retained parent prefix remains unknown. */
static void
semantic_handler_entry(const semantic_image *image,
                       const semantic_worklist *work, uint64_t *state,
                       int slot, int lasti)
{
    memset(state, 0, (size_t)SEM_FACTS * work->nwords * sizeof(uint64_t));
    for (int home = slot + 1; home < image->nl + image->scratch; home++) {
        semantic_set_facts(work, state, home, image->nl,
                           (1u << SEM_NULL) | (1u << SEM_NO_HOME));
    }
    for (int home = slot - lasti; home <= slot; home++) {
        semantic_set_facts(work, state, home, image->nl,
                           (1u << SEM_INITIALIZED) | (1u << SEM_MAY_OWNER) |
                           (1u << SEM_NONNULL));
    }
}

/* This is an abrupt footprint, independent of successful normal transfers.
 * A suspension can detach B and later install an arbitrary resume value at A.
 * The complete emitted contract covers monitoring, throw and bridge cleanup;
 * nothing at or above either scratch home is preserved by this capability. */
static int
semantic_abrupt_floor(const semantic_image *image, int pc)
{
    _PyIRWord word = image->words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    int base = (int)IR_B(word), dst = (int)IR_A(word);
    int limit = image->nl + image->scratch;
    int arg = admitted_word_arg(word, image->fbinit, image->nfb);
    if (fmt->flags & _PyIR_ABRUPT_OPERANDS) {
        uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
        int floor = limit;
        for (int field = 0; field < 4; field++) {
            int role = fmt->roles[field];
            if (role != _PyIR_ROLE_BASE && role != _PyIR_ROLE_DST &&
                role < _PyIR_ROLE_IN0)
            {
                continue;
            }
            int home = (int)fields[field];
            if (home >= image->nl && home != IR_NO_DST) {
                assert(home < limit);
                floor = Py_MIN(floor, home);
            }
        }
        return floor;
    }
    int ordinary = (fmt->flags & _PyIR_WINDOW) &&
        fmt->roles[1] == _PyIR_ROLE_BASE && fmt->roles[2] == _PyIR_ROLE_IMM &&
        fmt->roles[3] == _PyIR_ROLE_NONE && arg >= 0 && arg <= 1 &&
        _PyIR_Extent(fmt->reads, arg) == 1 && _PyIR_Extent(fmt->writes, arg) == 1;
    /* The explicit delegated form has sparse independent input fields.
     * Its retained D/cursor are not writes; only B and a scratch A set the
     * abrupt floor. The same flag still requires its own emitted receipt. */
    int delegated = !(fmt->flags & _PyIR_WINDOW) && fmt->ninputs == 3 &&
        fmt->roles[1] == _PyIR_ROLE_IN0 + 2 && fmt->roles[2] == _PyIR_ROLE_NONE &&
        fmt->roles[3] == _PyIR_ROLE_IN0 &&
        _PyIR_Extent(fmt->reads, arg) == 3 && _PyIR_Extent(fmt->writes, arg) == 3 &&
        _PyIR_Extent(fmt->through, arg) == 2;
    if (!(fmt->flags & _PyIR_ABRUPT_PREFIX) ||
        fmt->roles[0] != _PyIR_ROLE_DST || (!ordinary && !delegated) ||
        base < image->nl || base >= limit ||
        (dst != IR_NO_DST && dst >= limit))
    {
        return image->nl;
    }
    return dst >= image->nl && dst != IR_NO_DST ? Py_MIN(base, dst) : base;
}

static void
semantic_exception_entry(const semantic_image *image,
                         const semantic_worklist *work, uint64_t *out,
                         const uint64_t *incoming, int floor,
                         int slot, int lasti)
{
    semantic_handler_entry(image, work, out, slot, lasti);
    if (incoming == NULL) {
        return;
    }
    for (int home = image->nl; home < Py_MIN(floor, slot - lasti); home++) {
        int bit = home - image->nl;
        uint64_t mask = UINT64_C(1) << (bit % 64);
        out[SEM_NO_HOME * work->nwords + bit / 64] |=
            incoming[SEM_NO_HOME * work->nwords + bit / 64] & mask;
    }
}

/* Exception ranges are already validated sorted and disjoint. */
static const _PyIRExceptionEntry *
semantic_exception_at(const _PyIRExceptionEntry *entries, int count, int pc)
{
    int low = 0, high = count;
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (entries[mid].end <= pc) {
            low = mid + 1;
        }
        else {
            high = mid;
        }
    }
    return low < count && entries[low].start <= pc ? entries + low : NULL;
}

/* Only a complete emitted route census can remove an ordinary EH edge.
 * Observer/source recovery is joined separately, even for an inert word. */
static int
semantic_has_exceptional_predecessor(const semantic_image *image, int pc)
{
    _PyIRWord word = image->words[pc];
    const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(word));
    unsigned int routes = fmt->error_routes;
    unsigned int known = _PyIR_ERROR_COMPLETE | _PyIR_ERROR_DIRECT |
                         _PyIR_ERROR_BRIDGE | _PyIR_ERROR_LOCAL_DST;
    if (!(routes & _PyIR_ERROR_COMPLETE) || (routes & ~known) ||
        (routes & (_PyIR_ERROR_DIRECT | _PyIR_ERROR_BRIDGE)))
    {
        return 1;
    }
    if (routes & _PyIR_ERROR_LOCAL_DST) {
        uint32_t fields[] = {IR_A(word), IR_B(word), IR_C(word), IR_D(word)};
        int found = 0;
        for (int f = 0; f < 4; f++) {
            if (fmt->roles[f] == _PyIR_ROLE_DST) {
                found++;
                if (fields[f] < (uint32_t)image->nl) {
                    return 1;
                }
            }
        }
        /* A malformed conditional capability cannot certify an inert site. */
        if (found != 1) {
            return 1;
        }
    }
    return 0;
}

static int
normal_transfer_proof(const semantic_image *image,
                      const _PyIRCodePlan **plan, PyObject **normalized)
{
    assert(plan == NULL || normalized == NULL);
    if (normalized != NULL) {
        *normalized = NULL;
    }
    if (plan != NULL) {
        *plan = NULL;
    }
    const _PyIRWord *words = image->words;
    int n = image->n;
    if (n == 0) {
        return 0;
    }
    int nl = image->nl;
    const unsigned char *exceptions = image->semantic.exceptions;
    const unsigned char *exception_end = exceptions + image->semantic.exceptions_size;
    int scratch = image->scratch;
    if (!_PyIR_ExceptionTableValid(exceptions, exception_end, n, nl, nl + scratch)) {
        PyErr_SetString(PyExc_ValueError, "code: invalid exception table");
        return -1;
    }
    int *block_at = PyMem_Calloc((size_t)n, sizeof(int));
    if (block_at == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    block_at[0] = 1;
    for (int i = 0; i < n; i++) {
        const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(words[i]));
        if ((fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD)) &&
            !(fmt->flags & _PyIR_NAMES_TARGET))
        {
            int target = fmt->flags & _PyIR_FORWARD ? i + 1 + IR_DS(words[i]) : (int)IR_CD(words[i]);
            block_at[target] = 1;
            if (i + 1 < n) {
                block_at[i + 1] = 1;
            }
        }
        if (IS_SCOPE_EXIT_OPCODE(_PyIR_HandlerBase[
                _PyIR_BaseOpcode(IR_HANDLER(words[i]))]) && i + 1 < n)
        {
            block_at[i + 1] = 1;
        }
    }
    int nexceptions = 0;
    const unsigned char *cursor = exceptions;
    while (cursor < exception_end) {
        _PyIRExceptionEntry entry = {0};
        int err = _PyIR_ExceptionNext(&cursor, exception_end, &entry);
        assert(err == 0);
        (void)err;
        block_at[entry.target] = 1;
        nexceptions++;
    }
    _PyIRSemanticMap source = image->semantic;
    int source_recovery = 0;
    for (uint32_t i = 0; i < source.nrecords; i++) {
        _PyIRRecoveryEvent event;
        _PyIRSemantic_Event(&source, i, &event);
        if (event.handler >= 0) {
            block_at[event.handler] = 1;
            source_recovery = 1;
        }
    }
    int nblocks = 0;
    for (int i = 0; i < n; i++) {
        nblocks += block_at[i];
    }
    semantic_worklist work = {.nblocks = nblocks,
                              .nwords = Py_MAX(1, (scratch + 63) / 64)};
    work.blocks = PyMem_Calloc((size_t)nblocks, sizeof(*work.blocks));
    work.queue = PyMem_Malloc((size_t)nblocks * sizeof(*work.queue));
    size_t state_words = (size_t)SEM_FACTS * work.nwords;
    int has_abrupt = nexceptions != 0 || source_recovery;
    size_t vectors = (size_t)nblocks + 2 + has_abrupt;
    if (vectors > SIZE_MAX / sizeof(uint64_t) / state_words) {
        PyMem_Free(work.blocks);
        PyMem_Free(work.queue);
        PyMem_Free(block_at);
        PyErr_NoMemory();
        return -1;
    }
    work.facts = PyMem_Calloc(vectors * state_words, sizeof(uint64_t));
    if (work.blocks == NULL || work.queue == NULL || work.facts == NULL) {
        PyMem_Free(work.blocks);
        PyMem_Free(work.queue);
        PyMem_Free(work.facts);
        PyMem_Free(block_at);
        PyErr_NoMemory();
        return -1;
    }
    int block = -1;
    for (int i = 0; i < n; i++) {
        if (block_at[i]) {
            if (block >= 0) {
                work.blocks[block].end = i;
            }
            work.blocks[++block].start = i;
        }
        block_at[i] = block;
    }
    work.blocks[block].end = n;
    uint64_t *state = work.facts + (size_t)nblocks * state_words;
    /* Reuse this vector for exceptional phase inspection and independent
     * normal successors; those operations occur in separate proof passes. */
    work.branch = state + state_words;
    uint64_t *abrupt = has_abrupt ? state + 2 * state_words : NULL;
    _PyIRExceptionEntry *handlers = NULL;
    int *source_links = NULL;
    int result = 0;
    semantic_phases phases = {0};
    if (nexceptions) {
        if ((size_t)nexceptions > SIZE_MAX / sizeof(*handlers) ||
            (handlers = PyMem_Malloc((size_t)nexceptions * sizeof(*handlers))) == NULL)
        {
            PyErr_NoMemory();
            result = -1;
            goto done;
        }
        cursor = exceptions;
        for (int i = 0; i < nexceptions; i++) {
            int err = _PyIR_ExceptionNext(&cursor, exception_end, &handlers[i]);
            assert(err == 0);
            (void)err;
        }
    }
    if (source_recovery) {
        if (source.nrecords > INT_MAX ||
            (size_t)source.nrecords > SIZE_MAX - (size_t)nblocks)
        {
            PyErr_NoMemory();
            result = -1;
            goto done;
        }
        size_t links = (size_t)nblocks + source.nrecords;
        if (links > SIZE_MAX / sizeof(*source_links) ||
            (source_links = PyMem_Malloc(links * sizeof(*source_links))) == NULL)
        {
            PyErr_NoMemory();
            result = -1;
            goto done;
        }
        for (int i = 0; i < nblocks; i++) {
            source_links[i] = -1;
        }
        for (uint32_t i = 0; i < source.nrecords; i++) {
            _PyIRRecoveryEvent event;
            _PyIRSemantic_Event(&source, i, &event);
            if (event.handler >= 0) {
                int b = block_at[event.from];
                source_links[(size_t)nblocks + i] = source_links[b];
                source_links[b] = (int)i;
            }
        }
    }
    for (int bit = 0; bit < scratch; bit++) {
        uint64_t mask = UINT64_C(1) << (bit % 64);
        state[SEM_NULL * work.nwords + bit / 64] |= mask;
        state[SEM_NO_HOME * work.nwords + bit / 64] |= mask;
    }
    semantic_enqueue(&work, 0, state, 1);
    /* First propagate real exceptional predecessors. Only after that fixed
     * point, seed unknown ingress for unvisited protected/source code. It
     * must remain checked, but an upfront unknown seed would erase every
     * reached positive prefix. The second pass can only remove must-facts. */
    for (int pass = 0; pass < (has_abrupt ? 2 : 1); pass++) {
        if (pass) {
            for (int i = 0; i < nexceptions; i++) {
                const _PyIRExceptionEntry *entry = &handlers[i];
                for (int pc = entry->start; pc < entry->end; pc++) {
                    if (!work.blocks[block_at[pc]].reached)
                    {
                        semantic_exception_entry(image, &work, abrupt, NULL, 0,
                                                 entry->slot, entry->lasti);
                        semantic_exception_enqueue(&work, block_at[entry->target], abrupt);
                        break;
                    }
                }
            }
            if (source_recovery) {
                for (uint32_t i = 0; i < source.nrecords; i++) {
                    _PyIRRecoveryEvent event;
                    _PyIRSemantic_Event(&source, i, &event);
                    if (event.handler >= 0 &&
                        !work.blocks[block_at[event.from]].reached)
                    {
                        semantic_exception_entry(image, &work, abrupt, NULL, 0,
                                                 event.exc_slot, event.lasti);
                        semantic_exception_enqueue(&work, block_at[event.handler], abrupt);
                    }
                }
            }
        }
        while (work.count) {
            int b = work.queue[work.head];
            work.head = (work.head + 1) % nblocks;
            work.count--;
            work.blocks[b].queued = 0;
            memcpy(state, work.facts + (size_t)b * state_words, state_words * sizeof(uint64_t));
            int complete = work.blocks[b].complete;
            work.delegated = work.blocks[b].delegated;
            /* Source recovery has no abrupt footprint receipt yet. Its actual
             * from-block contributes an unknown prefix, independently of EH. */
            if (source_links != NULL) {
                for (int i = source_links[b]; i >= 0; i = source_links[(size_t)nblocks + i]) {
                    _PyIRRecoveryEvent event;
                    _PyIRSemantic_Event(&source, (uint32_t)i, &event);
                    semantic_exception_entry(image, &work, abrupt, NULL, 0,
                                             event.exc_slot, event.lasti);
                    semantic_exception_enqueue(&work, block_at[event.handler], abrupt);
                }
            }
            int end = work.blocks[b].end;
            for (int i = work.blocks[b].start; i < end; i++) {
                const _PyIRExceptionEntry *entry =
                    semantic_exception_at(handlers, nexceptions, i);
                if (entry != NULL) {
                    /* LINE/INSTRUCTION can fail before an inert body runs.
                     * Their shared checked wrappers may vacate scratch, but
                     * never populate an incoming absent home. This PRE edge
                     * exists even when its source receipt is uncovered. */
                    int floor = _PyIR_OBSERVER_ENTRY_ABSENCE ? nl + scratch : nl;
                    semantic_exception_entry(image, &work, abrupt, state, floor,
                                             entry->slot, entry->lasti);
                    semantic_exception_enqueue(&work, block_at[entry->target], abrupt);
                }
                if (entry != NULL &&
                    semantic_has_exceptional_predecessor(image, i))
                {
                    semantic_exception_entry(image, &work, abrupt, state,
                                             semantic_abrupt_floor(image, i),
                                             entry->slot, entry->lasti);
                    semantic_exception_enqueue(&work, block_at[entry->target], abrupt);
                }
                (void)semantic_step(image, &work, state, i, 0, &complete);
            }
            int last = end - 1;
            const _PyIRFormat *fmt = _PyIR_FormatOf(IR_HANDLER(words[last]));
            int opcode = _PyIR_BaseOpcode(IR_HANDLER(words[last]));
            memcpy(work.branch, state, state_words * sizeof(uint64_t));
            semantic_delegated outgoing = work.delegated;
            if (end < n && !(fmt->flags & _PyIR_JUMP_ALWAYS) &&
                !IS_SCOPE_EXIT_OPCODE(_PyIR_HandlerBase[opcode]))
            {
                semantic_normal_edge(image, &work, state, last, 0);
                semantic_enqueue(&work, block_at[end], state, complete);
            }
            if ((fmt->flags & (_PyIR_JUMPS | _PyIR_FORWARD)) &&
                !(fmt->flags & _PyIR_NAMES_TARGET))
            {
                int target = fmt->flags & _PyIR_FORWARD
                    ? last + 1 + IR_DS(words[last]) : (int)IR_CD(words[last]);
                /* Each edge starts at the common post-transfer state,
                 * including the finite protocol tuple. Fallthrough facts
                 * must never become facts of the taken successor. */
                memcpy(state, work.branch, state_words * sizeof(uint64_t));
                work.delegated = outgoing;
                semantic_normal_edge(image, &work, state, last, 1);
                semantic_enqueue(&work, block_at[target], state, complete);
            }
        }
    }
    /* Allocate persistent storage only for mixed coverage. First count exact
     * sites without retaining a per-word map or another scratch-state array. */
    work.phases = plan != NULL ? &phases : NULL;
    uint32_t nreturns = 0, ncertified = 0, ncreated = 0, ncreated_empty = 0;
    if (plan != NULL) {
        for (int i = 0; i < n; i++) {
            nreturns += _PyIR_FormatOf(IR_HANDLER(words[i]))->transfer_kind ==
                _PyIR_TRANSFER_EXIT;
            ncreated += semantic_has_created_copy(_PyIR_FormatOf(IR_HANDLER(words[i])));
        }
    }
    for (int b = 0; b < nblocks && result == 0; b++) {
        if (!work.blocks[b].reached) {
            for (int i = work.blocks[b].start; i < work.blocks[b].end; i++) {
                if (semantic_empty_return(IR_HANDLER(words[i]))) {
                    PyErr_Format(PyExc_ValueError,
                                 "code: word %d has an unproved empty return", i);
                    result = -1;
                    break;
                }
                if (IR_HANDLER(words[i]) == YIELD_DELEGATED ||
                    IR_HANDLER(words[i]) == INSTRUMENTED_YIELD_DELEGATED) {
                    PyErr_Format(PyExc_ValueError,
                                 "code: word %d has no delegated entry proof", i);
                    result = -1;
                    break;
                }
                if (_PyIR_FormatOf(IR_HANDLER(words[i]))->call_input_holes) {
                    PyErr_Format(PyExc_ValueError,
                                 "code: word %d has no virtual call entry proof", i);
                    result = -1;
                    break;
                }
            }
            continue;
        }
        memcpy(state, work.facts + (size_t)b * state_words, state_words * sizeof(uint64_t));
        int complete = work.blocks[b].complete;
        work.delegated = work.blocks[b].delegated;
        for (int i = work.blocks[b].start; i < work.blocks[b].end; i++) {
            if (semantic_error_snapshots(image, &work, state, i, complete) < 0 ||
                semantic_step(image, &work, state, i, 1, &complete) < 0) {
                result = -1;
                break;
            }
            int handler = IR_HANDLER(words[i]);
            if (semantic_empty_return(handler) || handler == RETURN_VALUE ||
                handler == INSTRUMENTED_RETURN_VALUE)
            {
                int empty = semantic_scratch_absent(image, &work, state);
                if (semantic_empty_return(handler) && !empty) {
                    PyErr_Format(PyExc_ValueError,
                                 "code: word %d has an unproved empty return", i);
                    result = -1;
                    break;
                }
                if (normalized != NULL && !semantic_empty_return(handler) && empty) {
                    /* Constructor bytes may be borrowed from the caller.
                     * Copy only when a stable proof first selects a fast form;
                     * all remaining proofs still inspect the original image. */
                    if (*normalized == NULL) {
                        *normalized = PyBytes_FromStringAndSize((const char *)words,
                            (Py_ssize_t)n * sizeof(*words));
                        if (*normalized == NULL) {
                            result = -1;
                            break;
                        }
                    }
                    int selected = handler == RETURN_VALUE ? RETURN_VALUE_EMPTY :
                        INSTRUMENTED_RETURN_VALUE_EMPTY;
                    _PyIRWord selected_word = words[i];
                    _PyIR_SetHandler(&selected_word, selected);
                    memcpy(PyBytes_AS_STRING(*normalized) + (size_t)i * sizeof(*words),
                           &selected_word, sizeof(selected_word));
                }
            }
            if (plan != NULL) {
                ncertified += semantic_exact_return(image, &work, state, i, complete);
                ncreated_empty += work.created_empty;
            }
        }
    }
    uint32_t flags =
        (ncertified && ncertified == nreturns ? _PyIR_PLAN_ALL_RETURNS : 0) |
        (ncreated && ncreated == ncreated_empty ? _PyIR_PLAN_ALL_CREATED_EMPTY : 0);
    if (result == 0 && plan != NULL && (ncertified || phases.nsites || flags)) {
        uint32_t nsites = flags & _PyIR_PLAN_ALL_RETURNS ? 0 : ncertified;
        if (nsites == 0 && phases.nsites == 0) {
            *plan = flag_only_plan(flags);
        }
        else {
            size_t bytes = sizeof(_PyIRCodePlan);
            uint32_t counts[] = {nsites, phases.nsites, phases.nranges};
            size_t sizes[] = {sizeof(uint32_t), sizeof(_PyIRPhaseSite), sizeof(_PyIRPhaseRange)};
            for (size_t i = 0; i < Py_ARRAY_LENGTH(counts); i++) {
                if (counts[i] > (SIZE_MAX - bytes) / sizes[i]) {
                    PyErr_NoMemory();
                    result = -1;
                    break;
                }
                bytes += (size_t)counts[i] * sizes[i];
            }
            _PyIRCodePlan *sparse = result == 0 ? PyMem_Malloc(bytes) : NULL;
            if (sparse == NULL) {
                PyErr_NoMemory();
                result = -1;
            }
            else {
                *sparse = (_PyIRCodePlan){
                    .nsites = nsites, .nphases = phases.nsites,
                    .nranges = phases.nranges,
                    .flags = flags};
                uint32_t next = 0;
                for (int b = 0; b < nblocks && nsites; b++) {
                    if (!work.blocks[b].reached) {
                        continue;
                    }
                    memcpy(state, work.facts + (size_t)b * state_words,
                           state_words * sizeof(uint64_t));
                    int complete = work.blocks[b].complete;
                    work.delegated = work.blocks[b].delegated;
                    for (int i = work.blocks[b].start;
                         i < work.blocks[b].end; i++)
                    {
                        (void)semantic_step(image, &work, state, i, 0, &complete);
                        if (semantic_exact_return(image, &work, state, i, complete)) {
                            assert(next < nsites);
                            sparse->sites[next++] = (uint32_t)i;
                        }
                    }
                }
                assert(next == nsites);
                if (phases.nsites) {
                    qsort(phases.sites, phases.nsites, sizeof(*phases.sites),
                          semantic_phase_compare);
                    memcpy((void *)_PyIR_PlanPhases(sparse), phases.sites,
                           (size_t)phases.nsites * sizeof(*phases.sites));
                }
                if (phases.nranges) {
                    memcpy((void *)_PyIR_PlanRanges(sparse), phases.ranges,
                           (size_t)phases.nranges * sizeof(*phases.ranges));
                }
                *plan = sparse;
            }
        }
    }
done:
    if (result < 0 && normalized != NULL) {
        Py_CLEAR(*normalized);
    }
    PyMem_Free(source_links);
    PyMem_Free(handlers);
    if (phases.shapes != NULL) {
        _Py_hashtable_destroy(phases.shapes);
    }
    PyMem_Free(phases.sites);
    PyMem_Free(phases.ranges);
    PyMem_Free(phases.scratch);
    PyMem_Free(work.error_nodes);
    PyMem_Free(work.facts);
    PyMem_Free(work.queue);
    PyMem_Free(work.blocks);
    PyMem_Free(block_at);
    return result;
}

static int
validate_normal_transfers(struct _PyCodeConstructor *con, PyObject **normalized)
{
    semantic_image image = {
        .words = (const _PyIRWord *)PyBytes_AS_STRING(con->code),
        .n = (int)(PyBytes_GET_SIZE(con->code) / sizeof(_PyIRWord)),
        .nl = (int)PyTuple_GET_SIZE(con->localsplusnames),
        .scratch = Py_MAX(con->stacksize, 1),
        .fbinit = con->fbinit == NULL ? NULL :
            (const uint32_t *)PyBytes_AS_STRING(con->fbinit),
        .nfb = con->fbinit == NULL ? 0 :
            (int)(PyBytes_GET_SIZE(con->fbinit) / sizeof(uint32_t)),
        .semantic = _PyIRSemantic_Stored(con->exceptiontable),
    };
    int result = normal_transfer_proof(&image, NULL, normalized);
    if (result < 0 && PyErr_ExceptionMatches(PyExc_ValueError)) {
        _PyErr_FormatNote("while admitting %U (%U)", con->qualname, con->filename);
    }
    return result;
}

int
_PyCode_BuildSemanticPlan(PyCodeObject *co, const _PyIRCodePlan **plan)
{
    *plan = NULL;
    int nl = co->co_nlocalsplus;
    int scratch = Py_MAX(co->co_stacksize, 1);
    int limit = _PyFrame_NumSlotsForCodeObject(co);
    /* Public zero-stack images reserve one physical scratch home. Keep that
     * same domain in the builder; a differently shaped static image has no
     * certificate rather than silently discarding its physical tail. */
    if (nl < 0 || nl > IR_MAX_SLOTS || scratch > IR_MAX_SLOTS - nl ||
        limit != nl + scratch)
    {
        return 0;
    }
    semantic_image image = {
        .words = _PyCode_WORDS(co),
        .n = (int)Py_SIZE(co),
        .nl = nl,
        .scratch = scratch,
        .fbinit = _PyCode_FBINIT(co),
        .nfb = co->co_nfeedback,
        .semantic = _PyIRSemantic_Stored(co->co_exceptiontable),
    };
    return normal_transfer_proof(&image, plan, NULL);
}

/* The normalized words and all semantic proofs refer to the same immutable
 * image. A caller's constructor and borrowed input references stay unchanged. */
static PyObject *
validate_semantic_image(struct _PyCodeConstructor *con)
{
    if (validate_code_structure(con) < 0) {
        return NULL;
    }
    PyObject *words = check_words(con);
    if (words == NULL) {
        return NULL;
    }
    struct _PyCodeConstructor checked = *con;
    checked.code = words;
    PyObject *normalized = NULL;
    if (validate_semantic_and_position_maps(&checked) < 0 ||
        validate_completion_words(&checked) < 0 || validate_virtual_kwargs(&checked) < 0 ||
        validate_normal_transfers(&checked, &normalized) < 0)
    {
        Py_DECREF(words);
        return NULL;
    }
    if (normalized != NULL) {
        Py_SETREF(words, normalized);
    }
    return words;
}

PyCodeObject *
_PyCode_New(struct _PyCodeConstructor *con)
{
    PyObject *words = validate_semantic_image(con);
    if (words == NULL) {
        return NULL;
    }
    struct _PyCodeConstructor checked = *con;
    checked.code = words;
    PyCodeObject *co = new_code(&checked);
    Py_DECREF(words);
    return co;
}

/* As PyUnstable_Code_NewWithPosOnlyArgs(), with the feedback table of the
 * words in fbinit, or NULL for none, and the constant-slot table in
 * constslots, or NULL for none: the constructor adds its hidden slots to
 * localsplus, between the cells and the free variables. */
static PyCodeObject *
code_new_with_fbinit(
                          int argcount, int posonlyargcount, int kwonlyargcount,
                          int nlocals, int stacksize, int flags,
                          PyObject *code, PyObject *fbinit,
                          PyObject *constslots,
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
    PyObject *padded = NULL;

    if (varnames == NULL || !PyTuple_Check(varnames) ||
        cellvars == NULL || !PyTuple_Check(cellvars) ||
        freevars == NULL || !PyTuple_Check(freevars)
        ) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (validate_code_name_tuple(varnames) < 0 ||
        validate_code_name_tuple(cellvars) < 0 ||
        validate_code_name_tuple(freevars) < 0) {
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
    if (constslots != NULL && !PyBytes_Check(constslots)) {
        PyErr_SetString(PyExc_ValueError, "code: co_constslots is malformed");
        goto error;
    }
    int nconstslots = constslots == NULL ? 0
        : (int)(PyBytes_GET_SIZE(constslots) / (2 * sizeof(uint32_t)));
    int nlocalsplus = nvarnames + ncellvars + nconstslots + nfreevars;
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
        PyObject *name = _PyTuple_GET_OBJECT(varnames, i);
        _Py_set_localsplus_info(offset, name, CO_FAST_LOCAL,
                               localsplusnames, localspluskinds);
    }
    for (int i = 0; i < ncellvars; i++, offset++) {
        PyObject *name = _PyTuple_GET_OBJECT(cellvars, i);
        int argoffset = -1;
        for (int j = 0; j < nvarnames; j++) {
            int cmp = PyUnicode_Compare(_PyTuple_GET_OBJECT(varnames, j),
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
    for (int i = 0; i < nconstslots; i++, offset++) {
        PyObject *name = PyUnicode_FromFormat(".k%d", i);
        if (name == NULL) {
            goto error;
        }
        _Py_set_localsplus_info(offset, name, CO_FAST_CONST | CO_FAST_HIDDEN,
                               localsplusnames, localspluskinds);
        Py_DECREF(name);
    }
    for (int i = 0; i < nfreevars; i++, offset++) {
        PyObject *name = _PyTuple_GET_OBJECT(freevars, i);
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
        .constslots = constslots,
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
                                nlocals, stacksize, flags, code, NULL, NULL, consts,
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
    _PyIRPositions source;
    int valid = _PyIRPositions_Decode(linetable, length, &source);
    assert(valid >= 0);
    if (valid < 0) {
        source = (_PyIRPositions){(const unsigned char *)linetable, 0, NULL, 0};
    }
    range->opaque.lo_next = source.locations;
    range->opaque.limit = range->opaque.lo_next + source.locations_size;
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
    int offset;
    int line;
} line_source_row;

typedef struct {
    PyObject_HEAD
    PyCodeObject *li_code;
    PyCodeAddressRange li_line;
    line_source_row *li_sources;
    uint32_t li_source_count;
    uint32_t li_source_next;
    int li_offset;
    int li_range_end;
    int li_range_line;
    int li_last_line;
} lineiterator;

static void
lineiter_dealloc(PyObject *self)
{
    lineiterator *li = (lineiterator*)self;
    PyMem_Free(li->li_sources);
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

static PyObject *
lineiter_next(PyObject *self)
{
    lineiterator *li = (lineiterator*)self;
    int next_source = INT_MAX;
    while (li->li_source_next < li->li_source_count) {
        line_source_row row = li->li_sources[li->li_source_next];
        if (row.offset > li->li_offset) {
            next_source = row.offset;
            break;
        }
        li->li_source_next++;
        if (row.offset == li->li_offset && row.line != li->li_last_line) {
            li->li_last_line = row.line;
            return Py_BuildValue("iii", row.offset, row.offset, row.line);
        }
    }
    if (li->li_offset >= Py_SIZE(li->li_code)) {
        return NULL;
    }
    if (li->li_offset >= li->li_range_end) {
        PyCodeAddressRange *bounds = &li->li_line;
        int line = _PyCode_CheckLineNumber(li->li_offset, bounds);
        int end = bounds->ar_end;
        while (_PyLineTable_NextAddressRange(bounds)) {
            if (bounds->ar_line != line) {
                _PyLineTable_PreviousAddressRange(bounds);
                break;
            }
            end = bounds->ar_end;
        }
        /* A source row can split this range. Retain the merged end so the
         * next split never seeks the raw location cursor backwards. */
        li->li_range_end = end;
        li->li_range_line = line;
    }
    int start = li->li_offset;
    int end = Py_MIN(li->li_range_end, next_source);
    int line = li->li_range_line;
    li->li_offset = end;
    li->li_last_line = line;
    return Py_BuildValue("iiO&", start, end,
                         _source_offset_converter, &line);
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

static int
compare_source_rows(const void *left, const void *right)
{
    const line_source_row *a = left, *b = right;
    if (a->offset != b->offset) {
        return (a->offset > b->offset) - (a->offset < b->offset);
    }
    return (a->line > b->line) - (a->line < b->line);
}

static lineiterator *
new_linesiterator(PyCodeObject *code)
{
    lineiterator *li = (lineiterator *)PyType_GenericAlloc(&_PyLineIterator, 0);
    if (li == NULL) {
        return NULL;
    }
    li->li_code = (PyCodeObject*)Py_NewRef(code);
    li->li_sources = NULL;
    li->li_source_count = li->li_source_next = 0;
    li->li_offset = li->li_range_end = 0;
    li->li_range_line = -1;
    li->li_last_line = INT_MIN;
    _PyCode_InitAddressRange(code, &li->li_line);
    _PyIRObservationMap source = _PyIRObservation_Stored(code);
    if (source.nrecords) {
        li->li_sources = PyMem_Calloc(source.nrecords, sizeof(line_source_row));
        if (li->li_sources == NULL) {
            PyErr_NoMemory();
            Py_DECREF(li);
            return NULL;
        }
        for (uint32_t i = 0; i < source.nrecords; i++) {
            _PyIRObservationEvent event;
            _PyIRObservation_Event(&source, i, &event);
            li->li_sources[i] = (line_source_row){event.to, event.line};
        }
        qsort(li->li_sources, source.nrecords, sizeof(line_source_row), compare_source_rows);
        /* Static co_lines rows are sorted independently of runtime edge
         * order. Filter redundant rows with one forward location-table walk. */
        PyCodeAddressRange future;
        _PyCode_InitAddressRange(code, &future);
        for (uint32_t i = 0; i < source.nrecords; i++) {
            line_source_row row = li->li_sources[i];
            if (row.line < 0 || row.line == _PyCode_CheckLineNumber(row.offset, &future)) {
                continue;
            }
            uint32_t count = li->li_source_count;
            if (count && li->li_sources[count - 1].offset == row.offset &&
                li->li_sources[count - 1].line == row.line)
            {
                continue;
            }
            li->li_sources[li->li_source_count++] = row;
        }
    }
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

static PyObject*
positionsiter_next(PyObject *self)
{
    positionsiterator *pi = (positionsiterator*)self;
    if (pi->pi_offset >= pi->pi_range.ar_end) {
        assert(pi->pi_offset == pi->pi_range.ar_end);
        if (at_end(&pi->pi_range)) {
            return NULL;
        }
        advance_with_locations(&pi->pi_range, &pi->pi_endline, &pi->pi_column, &pi->pi_endcolumn);
    }
    pi->pi_offset += 1;
    return Py_BuildValue("(O&O&O&O&)",
        _source_offset_converter, &pi->pi_range.ar_line,
        _source_offset_converter, &pi->pi_endline,
        _source_offset_converter, &pi->pi_column,
        _source_offset_converter, &pi->pi_endcolumn);
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

#ifdef Py_GIL_DISABLED
static int
code_extra_grow_ft(PyCodeObject *co, _PyCodeObjectExtra *old_co_extra,
                   Py_ssize_t old_ce_size, Py_ssize_t new_ce_size,
                   Py_ssize_t index, void *extra)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(co);
    _PyCodeObjectExtra *new_co_extra = PyMem_Malloc(
        code_extra_size(new_ce_size));
    if (new_co_extra == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    if (old_ce_size > 0) {
        memcpy(new_co_extra->ce_extras, old_co_extra->ce_extras,
               old_ce_size * sizeof(void *));
    }
    for (Py_ssize_t i = old_ce_size; i < new_ce_size; i++) {
        new_co_extra->ce_extras[i] = NULL;
    }
    new_co_extra->ce_size = new_ce_size;
    new_co_extra->ce_extras[index] = extra;

    // Publish new buffer and its contents to lock-free readers.
    FT_ATOMIC_STORE_PTR_RELEASE(co->co_extra, new_co_extra);
    if (old_co_extra != NULL) {
        // QSBR: defer old-buffer free until lock-free readers quiesce.
        _PyMem_FreeDelayed(old_co_extra, code_extra_size(old_ce_size));
    }
    return 0;
}
#else
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
#endif

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
#ifdef Py_GIL_DISABLED
    // FT build: allocate new buffer and swap; QSBR reclaims the old one.
    result = code_extra_grow_ft(
        co, old_co_extra, old_ce_size, new_ce_size, index, extra);
#else
    // GIL build: grow with realloc.
    result = code_extra_grow_gil(
        co, old_co_extra, old_ce_size, new_ce_size, index, extra);
#endif

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
#define GETITEM(v, i) _PyTuple_GET_OBJECT((v), (i))
#else
static inline PyObject *
GETITEM(PyObject *v, Py_ssize_t i)
{
    assert(PyTuple_Check(v));
    assert(i >= 0);
    assert(i < PyTuple_GET_SIZE(v));
    return _PyTuple_GET_OBJECT(v, i);
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
            if (PySet_ContainsValue(attrnames, PyValue_FromObjectBorrow(name))) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                continue;
            }
            unbound.total += 1;
            unbound.numattrs += 1;
            if (PySet_AddValue(attrnames, PyValue_FromObjectBorrow(name)) < 0) {
                return -1;
            }
            if (PySet_ContainsValue(globalnames, PyValue_FromObjectBorrow(name))) {
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
            if (PySet_ContainsValue(globalnames, PyValue_FromObjectBorrow(name))) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                continue;
            }
            unbound.total += 1;
            unbound.globals.total += 1;
            if (globalsns != NULL && PyDict_ContainsValue(globalsns, PyValue_FromObjectBorrow(name))) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                unbound.globals.numglobal += 1;
            }
            else if (builtinsns != NULL && PyDict_ContainsValue(builtinsns, PyValue_FromObjectBorrow(name))) {
                if (_PyErr_Occurred(tstate)) {
                    return -1;
                }
                unbound.globals.numbuiltin += 1;
            }
            else {
                unbound.globals.numunknown += 1;
            }
            if (PySet_AddValue(globalnames, PyValue_FromObjectBorrow(name)) < 0) {
                return -1;
            }
            if (PySet_ContainsValue(attrnames, PyValue_FromObjectBorrow(name))) {
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
    int numconst = 0;
    PyObject *kinds = co->co_localspluskinds;
    Py_ssize_t numlocalplusfree = PyBytes_GET_SIZE(kinds);
    for (int i = 0; i < numlocalplusfree; i++) {
        _PyLocals_Kind kind = _PyLocals_GetKind(co->co_localspluskinds, i);
        if (kind & CO_FAST_CONST) {
            // A constant slot is none of these
            numconst += 1;
        }
        else if (kind & CO_FAST_FREE) {
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
    assert(numconst == co->co_nconstslots);
    assert(locals.total + numfree + numconst == co->co_nlocalsplus);
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
        if (PyValue_IsNone(PyTuple_GET_VALUE(co->co_consts, none_index))) {
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
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

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
    constslots: object(subclass_of="&PyBytes_Type", c_default="NULL") = b''
    /

Create a code object.  Not for the faint of heart.
[clinic start generated code]*/

static PyValue
code_new_impl(PyTypeObject *type, int argcount, int posonlyargcount,
              int kwonlyargcount, int nlocals, int stacksize, int flags,
              PyObject *code, PyObject *consts, PyObject *names,
              PyObject *varnames, PyObject *filename, PyObject *name,
              PyObject *qualname, int firstlineno, PyObject *linetable,
              PyObject *exceptiontable, PyObject *freevars,
              PyObject *cellvars, PyObject *fbinit, PyObject *constslots)
/*[clinic end generated code: output=d98fa2ec432ba30e input=c95e5e3b3712cb94]*/
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
                                          code, fbinit, constslots, consts, ournames,
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
    return _PyValue_FromObjectResult(co);
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
    if (data->left_tools) {
        PyMem_Free(data->left_tools);
    }
    PyMem_Free(data->stop_tools);
    if (data->lines) {
        PyMem_Free(data->lines);
    }
    if (data->line_tools) {
        PyMem_Free(data->line_tools);
    }
    PyMem_Free(data->source_line_tools);
    PyMem_Free(data->source_forced);
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


static void
code_dealloc(PyObject *self)
{
    PyThreadState *tstate = PyThreadState_GET();
    _Py_atomic_add_uint64(&tstate->interp->_code_object_generation, 1);
    PyCodeObject *co = _PyCodeObject_CAST(self);
    _PyObject_ResurrectStart(self);
    notify_code_watchers(PY_CODE_EVENT_DESTROY, co);
    if (_PyObject_ResurrectEnd(self)) {
        return;
    }

#ifdef Py_GIL_DISABLED
    PyObject_GC_UnTrack(co);
#endif

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
#ifdef Py_GIL_DISABLED
    assert(co->_co_unique_id == _Py_INVALID_UNIQUE_ID);
#endif
    if (co->_co_cached != NULL) {
        Py_XDECREF(co->_co_cached->_co_code);
        Py_XDECREF(co->_co_cached->_co_cellvars);
        Py_XDECREF(co->_co_cached->_co_freevars);
        Py_XDECREF(co->_co_cached->_co_varnames);
        PyMem_Free(co->_co_cached);
    }
    FT_CLEAR_WEAKREFS(self, co->co_weakreflist);
    free_monitoring_data(co->_co_monitoring);
    _PyCode_ClearExec(co);
#ifdef Py_GIL_DISABLED
    if (co->co_tlbc != NULL) {
        // The first element always points to the mutable bytecode at the end of
        // the code object, which will be freed when the code object is freed.
        for (Py_ssize_t i = 1; i < co->co_tlbc->size; i++) {
            char *entry = co->co_tlbc->entries[i];
            if (entry != NULL) {
                free_bytecode_copy(co, entry);
            }
        }
        PyMem_Free(co->co_tlbc);
    }
#endif
    PyObject_Free(co);
}

static int
code_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyCodeObject *co = _PyCodeObject_CAST(self);
    Py_VISIT(co->co_consts);
#ifndef Py_GIL_DISABLED
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
#endif
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

    eq = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(co->co_name), PyValue_FromObjectBorrow(cp->co_name), Py_EQ);
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
                  co->co_nfeedback * sizeof(uint32_t)) == 0
        && co->co_nconstslots == cp->co_nconstslots
        && memcmp(_PyCode_CONSTSLOTS(co), _PyCode_CONSTSLOTS(cp),
                  co->co_nconstslots * 2 * sizeof(uint32_t)) == 0;
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
    eq = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(consts1), PyValue_FromObjectBorrow(consts2), Py_EQ);
    Py_DECREF(consts1);
    Py_DECREF(consts2);
    if (eq <= 0) goto unequal;

    eq = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(co->co_names), PyValue_FromObjectBorrow(cp->co_names), Py_EQ);
    if (eq <= 0) goto unequal;
    eq = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(co->co_localsplusnames), PyValue_FromObjectBorrow(cp->co_localsplusnames), Py_EQ);
    if (eq <= 0) goto unequal;
    eq = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(co->co_linetable), PyValue_FromObjectBorrow(cp->co_linetable), Py_EQ);
    if (eq <= 0) {
        goto unequal;
    }
    eq = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(co->co_exceptiontable), PyValue_FromObjectBorrow(cp->co_exceptiontable), Py_EQ);
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
        Py_hash_t h = _PyValue_Hash(PyValue_FromObjectBorrow(EXPR));  \
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
    for (int i = 0; i < 2 * co->co_nconstslots; i++) {
        SCRAMBLE_IN(_PyCode_CONSTSLOTS(co)[i]);
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


static PyObject *
code_getvarnames(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return _PyCode_GetVarnames(code);
}

static PyObject *
code_getcellvars(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return _PyCode_GetCellvars(code);
}

static PyObject *
code_getfreevars(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return _PyCode_GetFreevars(code);
}

/* The words of the executing copy, whose handlers specialisation and
 * instrumentation rewrite, or co_code before the first call. */
static PyObject *
code_getcodeadaptive(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(code->_co_exec);
    if (exec == NULL) {
        return _PyCode_GetCode(code);
    }
    return PyBytes_FromStringAndSize((const char *)exec->words,
                                     exec->nwords * sizeof(_PyIRWord));
}

/* The feedback entries of the executing copy, in units of 8 bytes with
 * entry 0 first, or None before the first call or without entries. */
static PyObject *
code_getfeedback(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    _PyCodeExec *exec = FT_ATOMIC_LOAD_PTR_ACQUIRE(code->_co_exec);
    if (exec == NULL || exec->entries == NULL) {
        Py_RETURN_NONE;
    }
    return PyBytes_FromStringAndSize(exec->entries,
                                     (Py_ssize_t)_PyCode_FbUnits(code)
                                     << IR_FB_UNIT_SHIFT);
}

/* The feedback table: per feedback entry, its kind and static operand */
static PyObject *
code_getfbinit(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return PyBytes_FromStringAndSize((const char *)_PyCode_FBINIT(code),
                                     code->co_nfeedback * sizeof(uint32_t));
}

/* The constant-slot table: per constant slot, the slot and the index in
 * co_consts of its value, two 32-bit integers each */
static PyObject *
code_getconstslots(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return PyBytes_FromStringAndSize((const char *)_PyCode_CONSTSLOTS(code),
                                     code->co_nconstslots * 2 * sizeof(uint32_t));
}

static PyObject *
code_getcode(PyObject *self, void *closure)
{
    PyCodeObject *code = _PyCodeObject_CAST(self);
    return _PyCode_GetCode(code);
}

static PyGetSetDef code_getsetlist[] = {
    {"_co_code_adaptive", code_getcodeadaptive, NULL, NULL},
    {"co_fbinit",         code_getfbinit,       NULL, NULL},
    {"co_constslots",     code_getconstslots,   NULL, NULL},
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
    // The feedback table is not counted (follow-up); the constant slots are
    res += (size_t)co->co_nconstslots * 2 * sizeof(uint32_t);
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
    co_argcount: int(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_argcount") = unchanged
    co_posonlyargcount: int(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_posonlyargcount") = unchanged
    co_kwonlyargcount: int(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_kwonlyargcount") = unchanged
    co_nlocals: int(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_nlocals") = unchanged
    co_stacksize: int(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_stacksize") = unchanged
    co_flags: int(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_flags") = unchanged
    co_firstlineno: int(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_firstlineno") = unchanged
    co_code: object(subclass_of="&PyBytes_Type", c_default="NULL") = unchanged
    co_consts: object(subclass_of="&PyTuple_Type", c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_consts") = unchanged
    co_names: object(subclass_of="&PyTuple_Type", c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_names") = unchanged
    co_varnames: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_freevars: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_cellvars: object(subclass_of="&PyTuple_Type", c_default="NULL") = unchanged
    co_filename: unicode(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_filename") = unchanged
    co_name: unicode(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_name") = unchanged
    co_qualname: unicode(c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_qualname") = unchanged
    co_linetable: object(subclass_of="&PyBytes_Type", c_default="((PyCodeObject *)PyValue_AsKnownObjectBorrow(self))->co_linetable") = unchanged
    co_exceptiontable: object(subclass_of="&PyBytes_Type", c_default="NULL") = unchanged
    co_fbinit: object(subclass_of="&PyBytes_Type", c_default="NULL") = unchanged
    co_constslots: object(subclass_of="&PyBytes_Type", c_default="NULL") = unchanged

Return a copy of the code object with new values for the specified fields.
[clinic start generated code]*/

static PyValue
code_replace_impl(PyCodeObject *self, int co_argcount,
                  int co_posonlyargcount, int co_kwonlyargcount,
                  int co_nlocals, int co_stacksize, int co_flags,
                  int co_firstlineno, PyObject *co_code, PyObject *co_consts,
                  PyObject *co_names, PyObject *co_varnames,
                  PyObject *co_freevars, PyObject *co_cellvars,
                  PyObject *co_filename, PyObject *co_name,
                  PyObject *co_qualname, PyObject *co_linetable,
                  PyObject *co_exceptiontable, PyObject *co_fbinit,
                  PyObject *co_constslots)
/*[clinic end generated code: output=44d80c045ef0ae97 input=04e92ba26bb63103]*/
{
#define CHECK_INT_ARG(ARG) \
        if (ARG < 0) { \
            PyErr_SetString(PyExc_ValueError, \
                            #ARG " must be a positive integer"); \
            return PyValue_NULL; \
        }

    CHECK_INT_ARG(co_argcount);
    CHECK_INT_ARG(co_posonlyargcount);
    CHECK_INT_ARG(co_kwonlyargcount);
    CHECK_INT_ARG(co_nlocals);
    CHECK_INT_ARG(co_stacksize);
    CHECK_INT_ARG(co_flags);
    CHECK_INT_ARG(co_firstlineno);

#undef CHECK_INT_ARG

    bool structural_change =
        co_argcount != self->co_argcount ||
        co_posonlyargcount != self->co_posonlyargcount ||
        co_kwonlyargcount != self->co_kwonlyargcount ||
        co_nlocals != self->co_nlocals || co_stacksize != self->co_stacksize ||
        (co_flags & ~CO_VIRTUAL_KWARGS) != (self->co_flags & ~CO_VIRTUAL_KWARGS) ||
        co_varnames != NULL || co_freevars != NULL || co_cellvars != NULL;
    bool recovery_geometry_change =
        co_nlocals != self->co_nlocals ||
        Py_MAX(co_stacksize, 1) != Py_MAX(self->co_stacksize, 1) ||
        (co_varnames != NULL && PyTuple_GET_SIZE(co_varnames) != self->co_nlocals) ||
        (co_freevars != NULL && PyTuple_GET_SIZE(co_freevars) != self->co_nfreevars) ||
        (co_cellvars != NULL && PyTuple_GET_SIZE(co_cellvars) != self->co_ncellvars);
    if (co_code != NULL) {
        bool changed_words = PyBytes_GET_SIZE(co_code) != Py_SIZE(self) * (Py_ssize_t)sizeof(_PyIRWord) ||
            memcmp(PyBytes_AS_STRING(co_code), self->co_words,
                   (size_t)Py_MIN(PyBytes_GET_SIZE(co_code),
                                 Py_SIZE(self) * (Py_ssize_t)sizeof(_PyIRWord))) != 0;
        structural_change |= changed_words;
        recovery_geometry_change |= changed_words;
    }
    if (recovery_geometry_change && co_exceptiontable == NULL) {
        _PyIRSemanticMap semantic = _PyIRSemantic_Stored(self->co_exceptiontable);
        if (semantic.nrecords) {
            PyErr_SetString(PyExc_ValueError,
                            "code.replace: changed code requires matching semantic recovery");
            return PyValue_NULL;
        }
    }
    if (co_exceptiontable == NULL) {
        co_exceptiontable = self->co_exceptiontable;
    }
    if (structural_change) {
        co_flags &= ~CO_VIRTUAL_KWARGS;
    }
    PyObject *code = NULL;
    if (co_code == NULL) {
        code = _PyCode_GetCode(self);
        if (code == NULL) {
            return PyValue_NULL;
        }
        co_code = code;
    }

    if (PySys_Audit("code.__new__", "OOOiiiiii",
                    co_code, co_filename, co_name, co_argcount,
                    co_posonlyargcount, co_kwonlyargcount, co_nlocals,
                    co_stacksize, co_flags) < 0) {
        Py_XDECREF(code);
        return PyValue_NULL;
    }

    PyCodeObject *co = NULL;
    PyObject *varnames = NULL;
    PyObject *cellvars = NULL;
    PyObject *freevars = NULL;
    PyObject *fbinit = NULL;
    PyObject *constslots = NULL;
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
    if (co_constslots == NULL) {
        constslots = PyBytes_FromStringAndSize(
            (const char *)_PyCode_CONSTSLOTS(self),
            self->co_nconstslots * 2 * sizeof(uint32_t));
        if (constslots == NULL) {
            goto error;
        }
        co_constslots = constslots;
    }
    co = code_new_with_fbinit(
        co_argcount, co_posonlyargcount, co_kwonlyargcount, co_nlocals,
        co_stacksize, co_flags, co_code, co_fbinit, co_constslots,
        co_consts, co_names,
        co_varnames, co_freevars, co_cellvars, co_filename, co_name,
        co_qualname, co_firstlineno,
        co_linetable, co_exceptiontable);

error:
    Py_XDECREF(fbinit);
    Py_XDECREF(constslots);
    Py_XDECREF(code);
    Py_XDECREF(varnames);
    Py_XDECREF(cellvars);
    Py_XDECREF(freevars);
    return _PyValue_FromObjectResult((PyObject *)co);
}

/*[clinic input]
code._varname_from_oparg

    oparg: int

(internal-only) Return the local variable name for the given oparg.

WARNING: this method is for internal use only and may change or go
away.
[clinic start generated code]*/

static PyValue
code__varname_from_oparg_impl(PyCodeObject *self, int oparg)
/*[clinic end generated code: output=cdd1cd7ce711272c input=6ba7d6df0d566463]*/
{
    PyValue name = PyTuple_GetValue(self->co_localsplusnames, oparg);
    PyObject *result = PyValue_IsNull(name) ? NULL : _PyValue_AsObjectAt(name, BOX_SITE_TUPLE);
    PyValue_XCLOSE(name);
    return _PyValue_FromObjectResult(result);
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


_Py_VALUE_NOTIMPL_RICHCMP(code_richcompare_value, code_richcompare)

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
#ifdef Py_GIL_DISABLED
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
#endif
    code_new__doc__,                    /* tp_doc */
    code_traverse,                      /* tp_traverse */
    0,                                  /* tp_clear */
    code_richcompare,                   /* tp_richcompare */
    offsetof(PyCodeObject, co_weakreflist),     /* tp_weaklistoffset */
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
    .tp_value = {
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, code_richcompare_value),
    },
};


/******************
 * other API
 ******************/

static PyValue constant_key_tuple(PyValueBorrow first, PyValueBorrow second,
                                  PyValueBorrow tag);
#define constant_key_tuple(V0, V1, V2) \
    (constant_key_tuple)(PyValue_Borrow(V0), PyValue_Borrow(V1), PyValue_Borrow(V2))

static PyValue
(constant_key_tuple)(PyValueBorrow first, PyValueBorrow second,
                     PyValueBorrow tag)
{
    /* The private array owns every element across tuple allocation. NULL1
       selects a pair; a language None is the distinct negative-zero tag. */
    PyValue items[3] = {PyValue_DUP(first), PyValue_DUP(second),
                        PyValue_XDUP(tag)};
    Py_ssize_t count = PyValue_IsNull(tag) ? 2 : 3;
    PyObject *tuple = _PyTuple_FromValueArray(items, count);
    for (Py_ssize_t i = 0; i < count; i++) {
        PyValue_CLOSE(items[i]);
    }
    return tuple == NULL ? PyValue_NULL : PyValue_FromObjectSteal(tuple);
}

static PyValue constant_value_key(PyValueBorrow constant);
#define constant_value_key(V0) (constant_value_key)(PyValue_Borrow(V0))

static PyValue
(constant_value_key)(PyValueBorrow constant)
{
    assert(PyValue_IsLanguageValue(constant));
    PyValue value = PyValue_DUP(constant);
    PyValue key = PyValue_NULL;
    PyObject *op = PyValue_IsObject(value) ? PyValue_AsKnownObjectBorrow(value) : NULL;

    if (PyValue_IsInlineInt(value) ||
        (op != NULL && (op == Py_None || op == Py_Ellipsis ||
                        PyLong_CheckExact(op) || PyUnicode_CheckExact(op) ||
                        PyCode_Check(op)))) {
        return value;
    }
    if (op != NULL && (PyBool_Check(op) || PyBytes_CheckExact(op))) {
        key = constant_key_tuple(PyValue_FromObjectBorrow((PyObject *)Py_TYPE(op)),
                                 value, PyValue_NULL);
    }
    // An inline float, or a NaN, which is boxed.
    else if (PyValue_IsInlineFloat(value) || (op != NULL && PyFloat_CheckExact(op))) {
        double d = op != NULL ? PyFloat_AS_DOUBLE(op) : PyValue_InlineFloat(value);
        PyValueBorrow tag = d == 0.0 && copysign(1.0, d) < 0.0
                      ? PyValue_FromObjectBorrow(Py_None)
                      : PyValue_Borrow(PyValue_NULL);
        key = constant_key_tuple(
            PyValue_FromObjectBorrow((PyObject *)&PyFloat_Type), value, tag);
    }
    else if (PyComplex_CheckExact(op)) {
        Py_complex z = PyComplex_AsCComplex(op);
        bool real_negative = z.real == 0.0 && copysign(1.0, z.real) < 0.0;
        bool imag_negative = z.imag == 0.0 && copysign(1.0, z.imag) < 0.0;
        PyValueBorrow tag = PyValue_Borrow(PyValue_NULL);
        if (real_negative && imag_negative) {
            tag = PyValue_FromObjectBorrow(Py_True);
        }
        else if (imag_negative) {
            tag = PyValue_FromObjectBorrow(Py_False);
        }
        else if (real_negative) {
            tag = PyValue_FromObjectBorrow(Py_None);
        }
        key = constant_key_tuple(PyValue_FromObjectBorrow((PyObject *)Py_TYPE(op)),
                                 value, tag);
    }
    else if (PyTuple_CheckExact(op)) {
        Py_ssize_t len = PyTuple_GET_SIZE(op);
        PyObject *tuple = PyTuple_New(len);
        if (tuple == NULL) {
            goto done;
        }
        for (Py_ssize_t i = 0; i < len; i++) {
            PyValue item = PyTuple_GetValue(op, i);
            PyValue item_key = constant_value_key(item);
            PyValue_CLOSE(item);
            if (PyValue_IsNull(item_key)) {
                Py_DECREF(tuple);
                goto done;
            }
            PyTuple_SET_VALUE(tuple, i, item_key);
        }
        key = constant_key_tuple(PyValue_FromObjectBorrow(tuple), value,
                                 PyValue_NULL);
        Py_DECREF(tuple);
    }
    else if (PyFrozenSet_CheckExact(op)) {
        PyObject *set = PyFrozenSet_New(NULL);
        if (set == NULL) {
            goto done;
        }
        Py_ssize_t pos = 0;
        PyValue item;
        int status;
        while ((status = _PySet_NextValue(op, &pos, &item, NULL)) > 0) {
            PyValue item_key = constant_value_key(item);
            PyValue_CLOSE(item);
            if (PyValue_IsNull(item_key)) {
                Py_DECREF(set);
                goto done;
            }
            int added = PySet_AddValue(set, item_key);
            PyValue_CLOSE(item_key);
            if (added < 0) {
                Py_DECREF(set);
                goto done;
            }
        }
        if (status < 0) {
            Py_DECREF(set);
            goto done;
        }
        key = constant_key_tuple(PyValue_FromObjectBorrow(set), value,
                                 PyValue_NULL);
        Py_DECREF(set);
    }
    else if (PySlice_Check(op)) {
        PySliceObject *slice = (PySliceObject *)op;
        PyValue keys[3] = {PyValue_NULL, PyValue_NULL, PyValue_NULL};
        PyValue *fields[3] = {&slice->start, &slice->stop, &slice->step};
        for (int i = 0; i < 3; i++) {
            keys[i] = constant_value_key(*fields[i]);
            if (PyValue_IsNull(keys[i])) {
                goto slice_done;
            }
        }
        PyObject *slice_key = _PySlice_FromValues(keys[0], keys[1], keys[2]);
        if (slice_key != NULL) {
            key = constant_key_tuple(PyValue_FromObjectBorrow(slice_key), value,
                                     PyValue_NULL);
            Py_DECREF(slice_key);
        }
slice_done:
        for (int i = 0; i < 3; i++) {
            PyValue_XCLOSE(keys[i]);
        }
    }
    else {
        PyObject *obj_id = PyLong_FromVoidPtr(op);
        if (obj_id != NULL) {
            key = constant_key_tuple(PyValue_FromObjectBorrow(obj_id), value,
                                     PyValue_NULL);
            Py_DECREF(obj_id);
        }
    }
done:
    PyValue_CLOSE(value);
    return key;
}

PyValue
_PyCode_ConstantValueKey(PyValue constant)
{
    return constant_value_key(constant);
}

PyObject *
_PyCode_ConstantKey(PyObject *op)
{
    PyValue key = constant_value_key(PyValue_FromObjectBorrow(op));
    if (PyValue_IsNull(key)) {
        return NULL;
    }
    PyObject *result = _PyValue_AsObjectAt(key, BOX_SITE_COMPILER);
    PyValue_CLOSE(key);
    return result;
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

#ifdef Py_GIL_DISABLED

// Thread-local bytecode (TLBC)
//
// Each thread specializes a thread-local copy of the bytecode, created on the
// first RESUME, in free-threaded builds. All copies of the bytecode for a code
// object are stored in the `co_tlbc` array. Threads reserve a globally unique
// index identifying its copy of the bytecode in all `co_tlbc` arrays at thread
// creation and release the index at thread destruction. The first entry in
// every `co_tlbc` array always points to the "main" copy of the bytecode that
// is stored at the end of the code object. This ensures that no bytecode is
// copied for programs that do not use threads.
//
// Thread-local bytecode can be disabled at runtime by providing either `-X
// tlbc=0` or `PYTHON_TLBC=0`. Disabling thread-local bytecode also disables
// specialization. All threads share the main copy of the bytecode when
// thread-local bytecode is disabled.
//
// Concurrent modifications to the bytecode made by the specializing
// interpreter and instrumentation use atomics, with specialization taking care
// not to overwrite an instruction that was instrumented concurrently.

int32_t
_Py_ReserveTLBCIndex(PyInterpreterState *interp)
{
    if (interp->config.tlbc_enabled) {
        return _PyIndexPool_AllocIndex(&interp->tlbc_indices);
    }
    // All threads share the main copy of the bytecode when TLBC is disabled
    return 0;
}

void
_Py_UnreserveTLBCIndex(PyInterpreterState *interp, int32_t index)
{
    if (interp->config.tlbc_enabled) {
        _PyIndexPool_FreeIndex(&interp->tlbc_indices, index);
    }
}

void
_Py_ClearTLBCIndex(_PyThreadStateImpl *tstate)
{
    PyInterpreterState *interp = ((PyThreadState *)tstate)->interp;
    _Py_UnreserveTLBCIndex(interp, tstate->tlbc_index);
}

static _PyCodeArray *
_PyCodeArray_New(Py_ssize_t size)
{
    _PyCodeArray *arr = PyMem_Calloc(
        1, offsetof(_PyCodeArray, entries) + sizeof(void *) * size);
    if (arr == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    arr->size = size;
    return arr;
}


// My kingdom for a bitset
struct flag_set {
    uint8_t *flags;
    Py_ssize_t size;
};

static inline int
flag_is_set(struct flag_set *flags, Py_ssize_t idx)
{
    assert(idx >= 0);
    return (idx < flags->size) && flags->flags[idx];
}

// Set the flag for each tlbc index in use
static int
get_indices_in_use(PyInterpreterState *interp, struct flag_set *in_use)
{
    assert(interp->stoptheworld.world_stopped);
    assert(in_use->flags == NULL);
    int32_t max_index = 0;
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        int32_t idx = ((_PyThreadStateImpl *) p)->tlbc_index;
        if (idx > max_index) {
            max_index = idx;
        }
    }
    _Py_FOR_EACH_TSTATE_END(interp);
    in_use->size = (size_t) max_index + 1;
    in_use->flags = PyMem_Calloc(in_use->size, sizeof(*in_use->flags));
    if (in_use->flags == NULL) {
        return -1;
    }
    _Py_FOR_EACH_TSTATE_BEGIN(interp, p) {
        in_use->flags[((_PyThreadStateImpl *) p)->tlbc_index] = 1;
    }
    _Py_FOR_EACH_TSTATE_END(interp);
    return 0;
}

struct get_code_args {
    _PyObjectStack code_objs;
    struct flag_set indices_in_use;
    int err;
};

static void
clear_get_code_args(struct get_code_args *args)
{
    if (args->indices_in_use.flags != NULL) {
        PyMem_Free(args->indices_in_use.flags);
        args->indices_in_use.flags = NULL;
    }
    _PyObjectStack_Clear(&args->code_objs);
}

static inline int
is_bytecode_unused(_PyCodeArray *tlbc, Py_ssize_t idx,
                   struct flag_set *indices_in_use)
{
    assert(idx > 0 && idx < tlbc->size);
    return tlbc->entries[idx] != NULL && !flag_is_set(indices_in_use, idx);
}

static int
get_code_with_unused_tlbc(PyObject *obj, void *data)
{
    struct get_code_args *args = (struct get_code_args *) data;
    if (!PyCode_Check(obj)) {
        return 1;
    }
    PyCodeObject *co = (PyCodeObject *) obj;
    _PyCodeArray *tlbc = co->co_tlbc;
    // The first index always points at the main copy of the bytecode embedded
    // in the code object.
    for (Py_ssize_t i = 1; i < tlbc->size; i++) {
        if (is_bytecode_unused(tlbc, i, &args->indices_in_use)) {
            if (_PyObjectStack_Push(&args->code_objs, obj) < 0) {
                args->err = -1;
                return 0;
            }
            return 1;
        }
    }
    return 1;
}

static void
free_unused_bytecode(PyCodeObject *co, struct flag_set *indices_in_use)
{
    _PyCodeArray *tlbc = co->co_tlbc;
    // The first index always points at the main copy of the bytecode embedded
    // in the code object.
    for (Py_ssize_t i = 1; i < tlbc->size; i++) {
        if (is_bytecode_unused(tlbc, i, indices_in_use)) {
            free_bytecode_copy(co, tlbc->entries[i]);
            tlbc->entries[i] = NULL;
        }
    }
}

int
_Py_ClearUnusedTLBC(PyInterpreterState *interp)
{
    struct get_code_args args = {
        .code_objs = {NULL},
        .indices_in_use = {NULL, 0},
        .err = 0,
    };
    _PyEval_StopTheWorld(interp);
    // Collect in-use tlbc indices
    if (get_indices_in_use(interp, &args.indices_in_use) < 0) {
        goto err;
    }
    // Collect code objects that have bytecode not in use by any thread
    _PyGC_VisitObjectsWorldStopped(
        interp, get_code_with_unused_tlbc, &args);
    if (args.err < 0) {
        goto err;
    }
    // Free unused bytecode. This must happen outside of gc_visit_heaps; it is
    // unsafe to allocate or free any mimalloc managed memory when it's
    // running.
    PyObject *obj;
    while ((obj = _PyObjectStack_Pop(&args.code_objs)) != NULL) {
        free_unused_bytecode((PyCodeObject*) obj, &args.indices_in_use);
    }
    _PyEval_StartTheWorld(interp);
    clear_get_code_args(&args);
    return 0;

err:
    _PyEval_StartTheWorld(interp);
    clear_get_code_args(&args);
    PyErr_NoMemory();
    return -1;
}

#endif
