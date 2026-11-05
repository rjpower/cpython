#ifndef Py_INTERNAL_VALUE_SLOTS_H
#define Py_INTERNAL_VALUE_SLOTS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_value.h"         // PyValue_NotImplemented
#include "pycore_value_api.h"     // _PyBorrow_TupleItemsFor()

/* Value slots (pyjit/design/value-api.md, section 2;
 * pyjit/design/proposals/value-api-v2a0.md, section 4).
 *
 * PyTypeObject.tp_value holds one value-form function beside each object
 * slot that has one, as opaque _Py_valuefunc storage (Include/cpython/
 * object.h). This header gives each field its typed signature. A value slot
 * takes its value arguments as borrows, which stay valid for the whole call,
 * including Python code that the slot runs: the caller holds the owner. It
 * returns an owned value, or PyValue_NULL with an exception set. Receivers
 * that are never inline (a container, an iterator, a descriptor) stay
 * PyObject *. */

typedef PyValue (*_Py_unaryvaluefunc)(PyValueBorrow);
typedef PyValue (*_Py_binaryvaluefunc)(PyValueBorrow, PyValueBorrow);
typedef PyValue (*_Py_ternaryvaluefunc)(PyValueBorrow, PyValueBorrow,
                                        PyValueBorrow);
typedef int (*_Py_inquiryvaluefunc)(PyValueBorrow);
typedef PyValue (*_Py_richcmpvaluefunc)(PyValueBorrow, PyValueBorrow, int);
typedef Py_hash_t (*_Py_hashvaluefunc)(PyValueBorrow);
/* repr and str: the result is a str object. */
typedef PyObject *(*_Py_reprvaluefunc)(PyValueBorrow);
typedef PyValue (*_Py_getattrovaluefunc)(PyValueBorrow, PyObject *);
/* A PyValue_NULL value deletes. */
typedef int (*_Py_setattrovaluefunc)(PyValueBorrow, PyObject *, PyValueBorrow);
typedef PyValue (*_Py_descrgetvaluefunc)(PyObject *, PyValueBorrow,
                                         PyObject *);
typedef int (*_Py_descrsetvaluefunc)(PyObject *, PyValueBorrow,
                                     PyValueBorrow);
typedef PyValue (*_Py_subscriptvaluefunc)(PyObject *, PyValueBorrow);
typedef int (*_Py_asssubscriptvaluefunc)(PyObject *, PyValueBorrow,
                                         PyValueBorrow);
typedef PyValue (*_Py_itemvaluefunc)(PyObject *, Py_ssize_t);
typedef int (*_Py_assitemvaluefunc)(PyObject *, Py_ssize_t, PyValueBorrow);
typedef int (*_Py_containsvaluefunc)(PyObject *, PyValueBorrow);
/* iternext and send return _PyStatusValue (pycore_value.h), as am_send
   does: PYGEN_NEXT with the item, PYGEN_RETURN with the return value,
   PYGEN_ERROR with PyValue_NULL and an exception set. An iternext slot
   returns PYGEN_RETURN with PyValue_NULL for a None return (R11); a send
   slot returns the None. */
typedef _PyStatusValue (*_Py_iternextvaluefunc)(PyObject *);
typedef _PyStatusValue (*_Py_sendvaluefunc)(PyObject *, PyValueBorrow);
/* The value vectorcall (value-api-m3.md, section 1.1): args is a window of
   words that the caller owns for the call; the callee borrows each one
   (PyValue_Borrow(args[i])) and takes a count (PyValue_DUP) to keep one.
   nargsf and kwnames are as PEP 590. The result is owned, or PyValue_NULL
   with an exception set. */
typedef PyValue (*_Py_vectorcallvaluefunc)(
    PyThreadState *tstate, PyObject *callable,
    const PyValue *args, size_t nargsf, PyObject *kwnames);
/* sq_concat and sq_inplace_concat: the receiver is the sequence. */
typedef PyValue (*_Py_concatvaluefunc)(PyObject *, PyValueBorrow);

#define _Py_VALUE_KIND_UNARY _Py_unaryvaluefunc
#define _Py_VALUE_KIND_BINARY _Py_binaryvaluefunc
#define _Py_VALUE_KIND_TERNARY _Py_ternaryvaluefunc
#define _Py_VALUE_KIND_INQUIRY _Py_inquiryvaluefunc
#define _Py_VALUE_KIND_RICHCMP _Py_richcmpvaluefunc
#define _Py_VALUE_KIND_HASH _Py_hashvaluefunc
#define _Py_VALUE_KIND_REPR _Py_reprvaluefunc
#define _Py_VALUE_KIND_GETATTRO _Py_getattrovaluefunc
#define _Py_VALUE_KIND_SETATTRO _Py_setattrovaluefunc
#define _Py_VALUE_KIND_DESCRGET _Py_descrgetvaluefunc
#define _Py_VALUE_KIND_DESCRSET _Py_descrsetvaluefunc
#define _Py_VALUE_KIND_SUBSCRIPT _Py_subscriptvaluefunc
#define _Py_VALUE_KIND_ASSSUBSCRIPT _Py_asssubscriptvaluefunc
#define _Py_VALUE_KIND_ITEM _Py_itemvaluefunc
#define _Py_VALUE_KIND_ASSITEM _Py_assitemvaluefunc
#define _Py_VALUE_KIND_CONTAINS _Py_containsvaluefunc
#define _Py_VALUE_KIND_ITERNEXT _Py_iternextvaluefunc
#define _Py_VALUE_KIND_SEND _Py_sendvaluefunc
#define _Py_VALUE_KIND_VECTORCALL _Py_vectorcallvaluefunc
#define _Py_VALUE_KIND_CONCAT _Py_concatvaluefunc

/* The slot table: X(field, object slot, kind, wrapper), one row per field of
 * _PyValueSlots, in field order (hot fields first).
 *
 * - field: the _PyValueSlots field.
 * - object slot: the object slot beside it, as a member path of
 *   PyHeapTypeObject, so that offsetof() of it matches the offsets of
 *   slotdefs in Objects/typeobject.c.
 * - kind: the typed signature, _Py_VALUE_KIND_<kind>.
 * - wrapper: the family of value wrappers that the field's slotdefs use for
 *   descriptors such as int.__add__ (the left or reflected form comes from
 *   the slotdef), or NONE while the field's dispatch has no value side in
 *   V2a-0.
 *
 * The table drives the inheritance pass, the invariant check, the slotdef
 * pairs, the internal spec slot ids (Python/slots.toml) and the typed
 * accessors below. */
#define _Py_VALUE_SLOT_TABLE(X) \
    X(nb_add, as_number.nb_add, BINARY, BINARY) \
    X(nb_subtract, as_number.nb_subtract, BINARY, BINARY) \
    X(nb_multiply, as_number.nb_multiply, BINARY, BINARY) \
    X(nb_true_divide, as_number.nb_true_divide, BINARY, BINARY) \
    X(nb_floor_divide, as_number.nb_floor_divide, BINARY, BINARY) \
    X(nb_remainder, as_number.nb_remainder, BINARY, BINARY) \
    X(richcompare, ht_type.tp_richcompare, RICHCMP, RICHCMP) \
    X(hash, ht_type.tp_hash, HASH, HASH) \
    X(nb_bool, as_number.nb_bool, INQUIRY, INQUIRY) \
    X(nb_index, as_number.nb_index, UNARY, UNARY) \
    X(getattro, ht_type.tp_getattro, GETATTRO, GETATTRO) \
    X(subscript, as_mapping.mp_subscript, SUBSCRIPT, SUBSCRIPT) \
    X(ass_subscript, as_mapping.mp_ass_subscript, ASSSUBSCRIPT, ASSSUBSCRIPT) \
    X(contains, as_sequence.sq_contains, CONTAINS, CONTAINS) \
    X(iternext, ht_type.tp_iternext, ITERNEXT, ITERNEXT) \
    X(call, ht_type.tp_call, VECTORCALL, CALL) \
    X(nb_power, as_number.nb_power, TERNARY, TERNARY) \
    X(nb_negative, as_number.nb_negative, UNARY, UNARY) \
    X(nb_positive, as_number.nb_positive, UNARY, UNARY) \
    X(nb_absolute, as_number.nb_absolute, UNARY, UNARY) \
    X(nb_invert, as_number.nb_invert, UNARY, UNARY) \
    X(nb_int, as_number.nb_int, UNARY, UNARY) \
    X(nb_float, as_number.nb_float, UNARY, UNARY) \
    X(nb_divmod, as_number.nb_divmod, BINARY, BINARY) \
    X(nb_lshift, as_number.nb_lshift, BINARY, BINARY) \
    X(nb_rshift, as_number.nb_rshift, BINARY, BINARY) \
    X(nb_and, as_number.nb_and, BINARY, BINARY) \
    X(nb_xor, as_number.nb_xor, BINARY, BINARY) \
    X(nb_or, as_number.nb_or, BINARY, BINARY) \
    X(nb_matrix_multiply, as_number.nb_matrix_multiply, BINARY, BINARY) \
    X(nb_inplace_add, as_number.nb_inplace_add, BINARY, BINARY) \
    X(nb_inplace_subtract, as_number.nb_inplace_subtract, BINARY, BINARY) \
    X(nb_inplace_multiply, as_number.nb_inplace_multiply, BINARY, BINARY) \
    X(nb_inplace_remainder, as_number.nb_inplace_remainder, BINARY, BINARY) \
    X(nb_inplace_power, as_number.nb_inplace_power, TERNARY, TERNARY) \
    X(nb_inplace_lshift, as_number.nb_inplace_lshift, BINARY, BINARY) \
    X(nb_inplace_rshift, as_number.nb_inplace_rshift, BINARY, BINARY) \
    X(nb_inplace_and, as_number.nb_inplace_and, BINARY, BINARY) \
    X(nb_inplace_xor, as_number.nb_inplace_xor, BINARY, BINARY) \
    X(nb_inplace_or, as_number.nb_inplace_or, BINARY, BINARY) \
    X(nb_inplace_floor_divide, as_number.nb_inplace_floor_divide, \
      BINARY, BINARY) \
    X(nb_inplace_true_divide, as_number.nb_inplace_true_divide, \
      BINARY, BINARY) \
    X(nb_inplace_matrix_multiply, as_number.nb_inplace_matrix_multiply, \
      BINARY, BINARY) \
    X(repr, ht_type.tp_repr, REPR, REPR) \
    X(str, ht_type.tp_str, REPR, REPR) \
    X(setattro, ht_type.tp_setattro, SETATTRO, SETATTRO) \
    X(descr_get, ht_type.tp_descr_get, DESCRGET, DESCRGET) \
    X(descr_set, ht_type.tp_descr_set, DESCRSET, DESCRSET) \
    X(item, as_sequence.sq_item, ITEM, ITEM) \
    X(ass_item, as_sequence.sq_ass_item, ASSITEM, ASSITEM) \
    X(send, as_async.am_send, SEND, NONE) \
    X(sq_concat, as_sequence.sq_concat, CONCAT, CONCAT) \
    X(sq_inplace_concat, as_sequence.sq_inplace_concat, CONCAT, CONCAT)

/* _Py_VALUE_SLOT_INDEX_<field>: the row of a field. */
#define _Py_VALUE_SLOT_ENUM(FIELD, PATH, KIND, WRAPPER) \
    _Py_VALUE_SLOT_INDEX_##FIELD,
enum {
    _Py_VALUE_SLOT_TABLE(_Py_VALUE_SLOT_ENUM)
    _Py_VALUE_SLOT_COUNT
};
#undef _Py_VALUE_SLOT_ENUM

static_assert(_Py_VALUE_SLOT_COUNT == 53, "53 value slot fields");
static_assert(sizeof(_PyValueSlots) ==
              _Py_VALUE_SLOT_COUNT * sizeof(_Py_valuefunc),
              "_PyValueSlots has one field per row of the slot table");
#define _Py_VALUE_SLOT_CHECK_ORDER(FIELD, PATH, KIND, WRAPPER) \
    static_assert(offsetof(_PyValueSlots, FIELD) == \
                  _Py_VALUE_SLOT_INDEX_##FIELD * sizeof(_Py_valuefunc), \
                  "_PyValueSlots." #FIELD " is out of table order");
_Py_VALUE_SLOT_TABLE(_Py_VALUE_SLOT_CHECK_ORDER)
#undef _Py_VALUE_SLOT_CHECK_ORDER

/* _Py_VALUE_SLOT_TYPE_<field>: the typed signature of a field. */
#define _Py_VALUE_SLOT_TYPEDEF(FIELD, PATH, KIND, WRAPPER) \
    typedef _Py_VALUE_KIND_##KIND _Py_VALUE_SLOT_TYPE_##FIELD;
_Py_VALUE_SLOT_TABLE(_Py_VALUE_SLOT_TYPEDEF)
#undef _Py_VALUE_SLOT_TYPEDEF

/* The value slot of type t, typed, or NULL. */
#define _PyType_VALUE_SLOT(t, field) \
    ((_Py_VALUE_SLOT_TYPE_##field)(t)->tp_value.field)

/* fn, which must have the typed signature of the field (a compile error
 * otherwise), as _Py_valuefunc storage. It is a constant expression, for
 * static initialisers: .tp_value.nb_add = _Py_VALUE_SLOT_INIT(nb_add, fn). */
#define _Py_VALUE_SLOT_INIT(field, fn) \
    _Generic((fn), _Py_VALUE_SLOT_TYPE_##field: (_Py_valuefunc)(fn))

/* Store fn, checked as for _Py_VALUE_SLOT_INIT, in a value slot of t. */
#define _PyType_SET_VALUE_SLOT(t, field, fn) \
    ((t)->tp_value.field = _Py_VALUE_SLOT_INIT(field, fn))

/* The value field of row (_Py_VALUE_SLOT_INDEX_<field>) of type t, as an
 * lvalue; the fields are laid out in table order (the static asserts
 * above). */
#define _PyType_VALUE_FIELD_AT(t, row) \
    (((_Py_valuefunc *)&(t)->tp_value)[row])

/* The pair table (value-api-v2a0.md, section 4.3): object functions that
 * many types share, with their value sides. A type whose object slot of
 * row is object, and whose value field is 0, gets value in PyType_Ready.
 * It lives in Objects/value_pairs.c, the one unit that may name a marked
 * function (value-api-m1b-m2.md, A8). A row whose value side is NULL waits
 * for the step that writes it: M1b (iteration), M3 (calls). */
typedef struct {
    int row;
    void *object;
    _Py_valuefunc value;
} _PyValueSlotPair;
extern const _PyValueSlotPair _PyValueSlot_Pairs[];
extern const size_t _PyValueSlot_PairCount;

#ifdef Py_DEBUG
/* Check that the value field of row pairs with the object slot beside it
 * (section 4.4); the value dispatchers call it, through
 * _PyType_CHECK_VALUE_ROW(), before each value-slot call. A failure is
 * fatal. */
extern void _PyType_CheckValueRow(PyTypeObject *type, int row);
#  define _PyType_CHECK_VALUE_ROW(type, row) _PyType_CheckValueRow(type, row)
#else
#  define _PyType_CHECK_VALUE_ROW(type, row) ((void)0)
#endif

/* The offset in PyTypeObject of the value field beside the object slot at
 * offset, an offset of slotdefs (from PyHeapTypeObject), or -1 if that
 * object slot has none. */
extern Py_ssize_t _PyType_ValueSlotFromObjectOffset(Py_ssize_t offset);

/* The value wrapper shape (value-api-m3.md, section 1.11; D17): self is a
 * borrow whose owner the caller holds (a window word or the method-wrapper's
 * self home) and is never boxed; args, nargs and kwnames are the D2 window,
 * read under window-read; wrapped_value is the descriptor's d_wrapped_value,
 * cast to the row's _Py_*valuefunc. The wrapper checks arity only, with
 * check_num_args's message; wrapperdescr_value_raw_call makes the keyword
 * check once. The result is owned, or PyValue_NULL with an exception set;
 * a wrapper over a three-state slot turns "absent" into the exception. */
typedef PyValue (*_Py_wrappervaluefunc)(
    PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
    PyObject *kwnames, void *wrapped_value);

/* The value wrapper that a slot descriptor of base calls with its
 * d_wrapped_value, or NULL if base is not a slotdef or its row has none
 * (value-api-v2a0.md, section 4.5). */
extern _Py_wrappervaluefunc _PyType_ValueWrapper(
    const struct wrapperbase *base);

#ifdef Py_DEBUG
/* Check that every value slot of type pairs with its object slot
 * (value-api-v2a0.md, section 4.4). A failure is fatal. */
extern void _PyType_CheckValueInvariant(PyTypeObject *type);
#endif


/* Conversions at the edges of a value slot. */

/* The owned value of an object-slot result: NULL gives PyValue_NULL, with
 * the exception that the slot set. An exact small int or float becomes
 * inline. */
static inline PyValue
_PyValue_FromObjectResult(PyObject *obj)
{
    return obj == NULL ? PyValue_NULL : PyValue_FromObjectSteal(obj);
}

/* The object that a thunk returns for the owned value result: the object,
 * a new box of an inline value, or NULL with an exception set.
 *
 * The THUNK forms (pyjit/design/proposals/value-api-g.md, section 1.2): a
 * compatibility entry, marked Py_VALUE_ENTRY, boxes for its caller. In
 * stats builds the macro captures the entry's caller (_Py_BOX_CALLER()) where
 * it expands, in the entry's own body, so the box is of kind THUNK, owned by
 * that caller, under the family's site class. Only the bodies of marked
 * functions may use them (compat-capture in Tools/build/check_value_api.py).
 * _PyValue_THUNK_NEW() and _PyValue_THUNK_AT() are in pycore_value_api.h. */
static inline PyObject *
_PyValue_THUNK_EXIT(PyValue result _PyBox_AT_PARAM)
{
    if (PyValue_IsNull(result)) {
        return NULL;
    }
    if (PyValue_IsInline(result)) {
        // An inline value owns nothing, so there is nothing to release.
        return _PyValue_AsObjectFor(result, at);
    }
    return PyValue_AsKnownObjectSteal(result);
}
#define _PyValue_THUNK_EXIT(RESULT, SITE) \
    (_PyValue_THUNK_EXIT)((RESULT) _PyBox_AT_ARG(_PyValue_THUNK_AT(SITE)))

/* The object result and status of a thunk over a _PyStatusValue: r's value
   becomes *result as for _PyValue_THUNK_EXIT() (PyValue_NULL stays NULL),
   and the status is r's, or -1 if boxing the value fails. For the object
   API's own signatures (PyIter_Send(), PyMapping_GetOptionalItem()). */
/* _PyValue_THUNK_EXIT() with the box argument already made. */
static inline PyObject *
_PyValue_ThunkResult(PyValue result _PyBox_AT_PARAM)
{
    if (PyValue_IsNull(result)) {
        return NULL;
    }
    if (PyValue_IsInline(result)) {
        return _PyValue_AsObjectFor(result, at);
    }
    return PyValue_AsKnownObjectSteal(result);
}

static inline int
_PyValue_THUNK_EXIT_STATUS(_PyStatusValue r, PyObject **result _PyBox_AT_PARAM)
{
    int is_null = PyValue_IsNull(r.value);
    *result = _PyValue_ThunkResult(r.value _PyBox_AT_ARG(at));
    return (*result == NULL && !is_null) ? -1 : r.status;
}
#define _PyValue_THUNK_EXIT_STATUS(R, RESULT, SITE) \
    (_PyValue_THUNK_EXIT_STATUS)((R), (RESULT) \
                                 _PyBox_AT_ARG(_PyValue_THUNK_AT(SITE)))

/* The object result of a thunk over an iternext value slot (R11): the item
   for PYGEN_NEXT; for PYGEN_RETURN, NULL with StopIteration(value) set if
   the value is not PyValue_NULL (the value is closed), else NULL with
   nothing set; NULL with the exception for PYGEN_ERROR. */
PyAPI_FUNC(PyObject *) (_PyValue_IterNextThunkExit)(_PyStatusValue r
                                                    _PyBox_AT_PARAM);
#define _PyValue_IterNextThunkExit(R) \
    (_PyValue_IterNextThunkExit)((R) \
        _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_ITERATION)))

/* The borrowed value of an object argument of a thunk. */
#define _Py_VALUE_ARG(OBJ) PyValue_FromObjectBorrow(OBJ)

/* The adapter (D9): the object argument of an object slot that a value
 * dispatcher calls because the value slot of the row is 0. An argument that
 * holds an object lends it; an inline one is boxed, of kind SLOT_ADAPTER
 * under site class SITE, owned by the object slot SLOTFN, and
 * _PyValue_AdapterRelease() releases the box after the call. A failed box
 * gives NULL with the exception set. */
static inline PyObject *
_PyValue_AdapterBox(PyValueBorrow v _PyBox_AT_PARAM)
{
    if (!PyValue_IsInline(v)) {
        return PyValue_AsKnownObjectBorrow(v);
    }
    return _PyValue_AsObjectFor(v, at);
}

static inline void
_PyValue_AdapterRelease(PyValueBorrow v, PyObject *obj)
{
    if (PyValue_IsInline(v)) {
        Py_DECREF(obj);
    }
}
#define _PyValue_ADAPTER_AT(SITE, SLOTFN) \
    _PyBox_AT((SITE), BOX_KIND_SLOT_ADAPTER, _Py_FUNC_ADDR(SLOTFN))
#define _PyValue_AdapterBox(V, SITE, SLOTFN) \
    (_PyValue_AdapterBox)(PyValue_Borrow(V) \
                          _PyBox_AT_ARG(_PyValue_ADAPTER_AT(SITE, SLOTFN)))
#define _PyValue_AdapterRelease(V, OBJ) \
    (_PyValue_AdapterRelease)(PyValue_Borrow(V), OBJ)


/* Thunks (D12): _Py_VALUE_THUNK_<kind>(name, valuefn) defines name, the
 * object slot of a core type whose value slot is valuefn. It converts its
 * object arguments at entry with _Py_VALUE_ARG(), which makes an exact int
 * or float inline (so the result of an exact int operation is boxed again at
 * exit: Q8), calls valuefn and converts the result with
 * _PyValue_THUNK_EXIT(). A type that defines
 * both names valuefn in tp_value and name in the object slot, so that the
 * two form a pair. */

#define _Py_VALUE_THUNK_UNARY(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *self) \
    { \
        return _PyValue_THUNK_EXIT((valuefn)(_Py_VALUE_ARG(self)), \
                                   BOX_SITE_NUMBER); \
    }

#define _Py_VALUE_THUNK_BINARY(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *left, PyObject *right) \
    { \
        return _PyValue_THUNK_EXIT( \
            (valuefn)(_Py_VALUE_ARG(left), _Py_VALUE_ARG(right)), \
            BOX_SITE_NUMBER); \
    }

#define _Py_VALUE_THUNK_TERNARY(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *base, PyObject *exponent, PyObject *modulus) \
    { \
        return _PyValue_THUNK_EXIT( \
            (valuefn)(_Py_VALUE_ARG(base), _Py_VALUE_ARG(exponent), \
                      _Py_VALUE_ARG(modulus)), BOX_SITE_NUMBER); \
    }

#define _Py_VALUE_THUNK_INQUIRY(name, valuefn) \
    Py_VALUE_ENTRY static int \
    name(PyObject *self) \
    { \
        return (valuefn)(_Py_VALUE_ARG(self)); \
    }

#define _Py_VALUE_THUNK_RICHCMP(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *left, PyObject *right, int op) \
    { \
        return _PyValue_THUNK_EXIT( \
            (valuefn)(_Py_VALUE_ARG(left), _Py_VALUE_ARG(right), op), \
            BOX_SITE_COMPARE); \
    }

#define _Py_VALUE_THUNK_HASH(name, valuefn) \
    Py_VALUE_ENTRY static Py_hash_t \
    name(PyObject *self) \
    { \
        return (valuefn)(_Py_VALUE_ARG(self)); \
    }

#define _Py_VALUE_THUNK_REPR(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *self) \
    { \
        return (valuefn)(_Py_VALUE_ARG(self)); \
    }

#define _Py_VALUE_THUNK_CONCAT(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *self, PyObject *other) \
    { \
        return _PyValue_THUNK_EXIT((valuefn)(self, _Py_VALUE_ARG(other)), \
                                   BOX_SITE_SEQUENCE); \
    }

/* The receivers of the subscript, item, contains, iternext and send
   families stay objects (value-api-m1b-m2.md, section 1.1). A NULL value
   argument of ASSSUBSCRIPT or ASSITEM, which deletes, becomes
   PyValue_NULL. */

#define _Py_VALUE_THUNK_SUBSCRIPT(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *self, PyObject *key) \
    { \
        return _PyValue_THUNK_EXIT((valuefn)(self, _Py_VALUE_ARG(key)), \
                                   BOX_SITE_SEQUENCE); \
    }

#define _Py_VALUE_THUNK_ITEM(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *self, Py_ssize_t i) \
    { \
        return _PyValue_THUNK_EXIT((valuefn)(self, i), BOX_SITE_SEQUENCE); \
    }

#define _Py_VALUE_THUNK_ASSSUBSCRIPT(name, valuefn) \
    Py_VALUE_ENTRY static int \
    name(PyObject *self, PyObject *key, PyObject *value) \
    { \
        return (valuefn)(self, _Py_VALUE_ARG(key), _Py_VALUE_ARG(value)); \
    }

#define _Py_VALUE_THUNK_ASSITEM(name, valuefn) \
    Py_VALUE_ENTRY static int \
    name(PyObject *self, Py_ssize_t i, PyObject *value) \
    { \
        return (valuefn)(self, i, _Py_VALUE_ARG(value)); \
    }

#define _Py_VALUE_THUNK_CONTAINS(name, valuefn) \
    Py_VALUE_ENTRY static int \
    name(PyObject *self, PyObject *item) \
    { \
        return (valuefn)(self, _Py_VALUE_ARG(item)); \
    }

#define _Py_VALUE_THUNK_ITERNEXT(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *self) \
    { \
        return _PyValue_IterNextThunkExit((valuefn)(self)); \
    }

#define _Py_VALUE_THUNK_SEND(name, valuefn) \
    Py_VALUE_ENTRY static PySendResult \
    name(PyObject *self, PyObject *arg, PyObject **result) \
    { \
        return (PySendResult)_PyValue_THUNK_EXIT_STATUS( \
            (valuefn)(self, _Py_VALUE_ARG(arg)), result, \
            BOX_SITE_ITERATION); \
    }


/* The attribute thunks (value-api-m1b-m2.md, section 2.2). A NULL obj
   (class access) or value (deletion) becomes PyValue_NULL. */

#define _Py_VALUE_THUNK_GETATTRO(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *self, PyObject *attr) \
    { \
        return _PyValue_THUNK_EXIT((valuefn)(_Py_VALUE_ARG(self), attr), \
                                   BOX_SITE_ATTRIBUTE); \
    }

#define _Py_VALUE_THUNK_SETATTRO(name, valuefn) \
    Py_VALUE_ENTRY static int \
    name(PyObject *self, PyObject *attr, PyObject *value) \
    { \
        return (valuefn)(_Py_VALUE_ARG(self), attr, _Py_VALUE_ARG(value)); \
    }

#define _Py_VALUE_THUNK_DESCRGET(name, valuefn) \
    Py_VALUE_ENTRY static PyObject * \
    name(PyObject *descr, PyObject *obj, PyObject *type) \
    { \
        return _PyValue_THUNK_EXIT( \
            (valuefn)(descr, _Py_VALUE_ARG(obj), type), \
            BOX_SITE_ATTRIBUTE); \
    }

#define _Py_VALUE_THUNK_DESCRSET(name, valuefn) \
    Py_VALUE_ENTRY static int \
    name(PyObject *descr, PyObject *obj, PyObject *value) \
    { \
        return (valuefn)(descr, _Py_VALUE_ARG(obj), _Py_VALUE_ARG(value)); \
    }

/* NotImplemented sides (D12, script class N): _Py_VALUE_NOTIMPL_<kind>(name,
 * objfn) defines name, the value side of a binary, ternary or compare object
 * slot objfn that returns NotImplemented unless its operands are of its own
 * type, so never for an exact int or float. An inline operand gives
 * NotImplemented without a call; otherwise name calls objfn with the
 * objects. A ternary slot's modulus is not tested: an inline modulus is
 * boxed for the call. ternary_op also calls the modulus's own slot, so a
 * ternary objfn must also return NotImplemented for a modulus that is not
 * of its type, as class N requires of all three operands. */

#define _Py_VALUE_NOTIMPL_BINARY(name, objfn) \
    static PyValue \
    name(PyValueBorrow left, PyValueBorrow right) \
    { \
        if (PyValue_IsInline(left) || PyValue_IsInline(right)) { \
            return PyValue_NotImplemented; \
        } \
        return _PyValue_FromObjectResult( \
            (objfn)(PyValue_AsKnownObjectBorrow(left), \
                    PyValue_AsKnownObjectBorrow(right))); \
    }

#define _Py_VALUE_NOTIMPL_RICHCMP(name, objfn) \
    static PyValue \
    name(PyValueBorrow left, PyValueBorrow right, int op) \
    { \
        if (PyValue_IsInline(left) || PyValue_IsInline(right)) { \
            return PyValue_NotImplemented; \
        } \
        return _PyValue_FromObjectResult( \
            (objfn)(PyValue_AsKnownObjectBorrow(left), \
                    PyValue_AsKnownObjectBorrow(right), op)); \
    }

#define _Py_VALUE_NOTIMPL_TERNARY(name, objfn) \
    static PyValue \
    name(PyValueBorrow base, PyValueBorrow exponent, PyValueBorrow modulus) \
    { \
        if (PyValue_IsInline(base) || PyValue_IsInline(exponent)) { \
            return PyValue_NotImplemented; \
        } \
        PyObject *modulus_object = _PyValue_AsObjectFor( \
            modulus, _PyValue_ADAPTER_AT(BOX_SITE_UNATTRIBUTED, (objfn))); \
        if (modulus_object == NULL) { \
            return PyValue_NULL; \
        } \
        PyObject *result = (objfn)(PyValue_AsKnownObjectBorrow(base), \
                                   PyValue_AsKnownObjectBorrow(exponent), \
                                   modulus_object); \
        Py_DECREF(modulus_object); \
        return _PyValue_FromObjectResult(result); \
    }

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_VALUE_SLOTS_H */
