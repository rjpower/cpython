#ifndef Py_INTERNAL_LOOKUP_H
#define Py_INTERNAL_LOOKUP_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Lookup plans (boil/design/09-object-protocol.md section 4).
 *
 * A plan says where the generic attribute protocol finds a name on objects
 * of one shape, and what guard keeps that true: the pair (shape,
 * type_version).  It describes PyObject_GenericGetAttr only; a type whose
 * tp_getattro is another function dispatches to it first, and that
 * function may run the same plan beneath it (object.__getattribute__, the
 * __getattr__ hook), never the other way round.
 *
 * The protocol has three phases.  Phase 1 finds the name in the MRO (the
 * type's cache, or find_name_in_mro()).  Phase 2, the classification in
 * this header, is pure: it allocates nothing, runs no user code and writes
 * no cache.  Phase 3 executes the plan (Objects/object.c).
 */

#include "pycore_shape.h"

typedef enum {
    _Py_LOOKUP_NONE = 0,        // a cache entry with only the MRO result
    _Py_LOOKUP_SLOT,            // a shape slot at byte `offset`
    _Py_LOOKUP_DICT,            // the instance dict; on a miss, `fallback`
    _Py_LOOKUP_DATA_DESCR,      // a data descriptor; MEMBER: the slot at `offset`
    _Py_LOOKUP_PROPERTY,        // a property; its getter is read at execution
    _Py_LOOKUP_METHOD,          // a function or method descriptor, not shadowed
    _Py_LOOKUP_DESCR,           // another non-data descriptor
    _Py_LOOKUP_CLASS_VALUE,     // a class attribute without __get__
    _Py_LOOKUP_ABSENT,          // AttributeError
} _PyLookupKind;

#define _Py_LOOKUP_CACHEABLE 1  // may be cached and specialised
#define _Py_LOOKUP_MEMBER 2     // DATA_DESCR: a __slots__ member at `offset`

typedef struct {
    uint8_t kind;               // _PyLookupKind
    uint8_t fallback;           // SLOT, DICT: the kind of steps 3 to 5
    uint8_t slot_kind;          // SLOT: _PySlotKind
    uint8_t flags;
    uint32_t type_version;      // the shape's version when classified; 0: none
    uint32_t offset;            // SLOT, MEMBER: bytes from the object start
    PyValue value;              // borrowed: the MRO result of the name
} _PyLookupPlan;

typedef enum {
    _Py_STORE_REPLACE = 1,      // the name is in the slot at `offset`
    _Py_STORE_GENERALISE,       // REPLACE, but the field's kind does not fit
    _Py_STORE_ADD,              // a new name in instance storage
    _Py_STORE_REMOVE,           // DELETE of a slot name
    _Py_STORE_DICT,             // the instance dict (or this rung's adapter)
    _Py_STORE_DATA_DESCR,       // descr's tp_descr_set; MEMBER: the slot at `offset`
    _Py_STORE_MISSING,          // DELETE of a name the object lacks
    _Py_STORE_NO_STORAGE,       // no instance storage and no setter
} _PyStoreOutcome;

typedef enum {
    _Py_STORE_SET = 0,
    _Py_STORE_DELETE = 1,
} _PyStoreOp;

typedef struct {
    uint8_t outcome;            // _PyStoreOutcome
    uint8_t flags;              // CACHEABLE, MEMBER
    uint8_t slot;               // REPLACE, GENERALISE, REMOVE: the field's slot; ADD: nslots
    uint8_t slot_kind;          // REPLACE, GENERALISE: the field's kind; ADD: shape_to's
                                // field's kind, or the value's if there is no edge yet
    uint32_t type_version;
    uint32_t offset;            // REPLACE, ADD, MEMBER: bytes from the object start
    _PyShape *shape_to;         // ADD, REMOVE: the next shape if its edge exists, or NULL
    PyValue value;              // borrowed: the MRO result of the name
} _PyStorePlan;

// The kind of DESCR, an MRO result, for a load: DATA_DESCR, PROPERTY,
// METHOD, DESCR, CLASS_VALUE or ABSENT; *MUTABLE is set if its type is
// mutable, so that the kind may change without a change to the receiver.
extern int _PyLookup_ClassifyLoad(PyValue descr, int *mutable);

// Phase 2.  DESCR is the MRO result of NAME on SHAPE's type; CACHEABLE says
// whether phase 1 allows caching (the plan may still withdraw it).
extern void _PyShape_ResolveLoad(_PyShape *shape, PyObject *name,
                                 PyValue descr, int cacheable,
                                 _PyLookupPlan *plan);
// VALUE_KIND is the kind of the value a SET stores (_PyValue_SlotKind()).
extern void _PyShape_ResolveStore(_PyShape *shape, PyObject *name,
                                  PyValue descr, int cacheable,
                                  _PyStoreOp op, uint8_t value_kind,
                                  _PyStorePlan *plan);

// Phases 1 and 2 with the type's (shape, name) cache; exported for
// _testinternalcapi.  NAME is a str;
// SHAPE's type is ready.  Never fails: an error in the MRO lookup is
// treated as a missing name, as _PyType_Lookup() does.
PyAPI_FUNC(void) _PyShape_LookupPlan(_PyShape *shape, PyObject *name,
                                _PyLookupPlan *plan);
PyAPI_FUNC(void) _PyShape_LookupStorePlan(_PyShape *shape, PyObject *name,
                                     _PyStoreOp op, uint8_t value_kind,
                                     _PyStorePlan *plan);

// The name of a kind or outcome, for _testinternalcapi and stats.
PyAPI_FUNC(const char *) _PyLookup_KindName(int kind);
PyAPI_FUNC(const char *) _PyLookup_OutcomeName(int outcome);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_LOOKUP_H */
