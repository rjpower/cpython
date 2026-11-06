#ifndef Py_INTERNAL_TYPEOBJECT_H
#define Py_INTERNAL_TYPEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_interp_structs.h" // managed_static_type_state
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_structs.h"       // PyValue


/* state */


#define _Py_TYPE_VERSION_INT 1
#define _Py_TYPE_VERSION_FLOAT 2
#define _Py_TYPE_VERSION_LIST 3
#define _Py_TYPE_VERSION_TUPLE 4
#define _Py_TYPE_VERSION_STR 5
#define _Py_TYPE_VERSION_SET 6
#define _Py_TYPE_VERSION_FROZEN_SET 7
#define _Py_TYPE_VERSION_DICT 8
#define _Py_TYPE_VERSION_BYTEARRAY 9
#define _Py_TYPE_VERSION_BYTES 10
#define _Py_TYPE_VERSION_COMPLEX 11
#define _Py_TYPE_VERSION_FROZENDICT 12

#define _Py_TYPE_VERSION_NEXT 16


#define _Py_TYPE_BASE_VERSION_TAG (2<<16)
#define _Py_MAX_GLOBAL_TYPE_VERSION_TAG (_Py_TYPE_BASE_VERSION_TAG - 1)


/* _PyType_LookupRef[AndVersion] are owning, fallible object bridges: NULL can
   mean absence or a lookup/boxing/cleanup exception. Canonical lookup avoids
   boxing; advertised nonzero versions are revalidated after owner cleanup. */
/* One nonallocating admission rule for caches, guards and version producers.
   Zero is never an admitted expected version. Static admission also requires
   this interpreter's complete publication, independently of shared tags. */
extern unsigned int _PyType_GetVersionForCurrentState(
    PyInterpreterState *interp, PyTypeObject *type);
extern int _PyType_MatchesVersion(PyInterpreterState *interp,
                                  PyTypeObject *type, unsigned int expected);
extern void _Py_NO_RETURN _PyType_AbortClosedLookup(void);

/* Positive-refcount mutation receiver. Keep this C-local scope active through
   writes, rollback and all displaced-owner cleanup. Begin only closes admission;
   the actual writer invalidates before publishing live metadata. */
typedef struct {
    PyInterpreterState *interp;
    PyTypeObject *type;
    int active;
} _PyTypeMutation;

PyAPI_FUNC(int) _PyTypeMutation_Begin(_PyTypeMutation *, PyTypeObject *);
PyAPI_FUNC(void) _PyTypeMutation_End(_PyTypeMutation *);

/* runtime lifecycle */

extern PyStatus _PyTypes_InitTypes(PyInterpreterState *);
extern void _PyTypes_FiniTypes(PyInterpreterState *);
extern void _PyTypes_FiniExtTypes(PyInterpreterState *interp);
extern void _PyTypes_Fini(PyInterpreterState *);
extern void _PyTypes_FiniCachedDescriptors(PyInterpreterState *);

static inline PyObject **
_PyStaticType_GET_WEAKREFS_LISTPTR(managed_static_type_state *state)
{
    assert(state != NULL);
    return &state->tp_weaklist;
}

extern int _PyStaticType_InitBuiltin(
    PyInterpreterState *interp,
    PyTypeObject *type);
extern void _PyStaticType_FiniBuiltin(
    PyInterpreterState *interp,
    PyTypeObject *type);
extern void _PyStaticType_ClearWeakRefs(
    PyInterpreterState *interp,
    PyTypeObject *type);
// Exported for external JIT support
PyAPI_FUNC(managed_static_type_state *) _PyStaticType_GetState(
    PyInterpreterState *interp,
    PyTypeObject *type);

// Export for '_datetime' shared extension.
PyAPI_FUNC(int) _PyStaticType_InitForExtension(
    PyInterpreterState *interp,
     PyTypeObject *self);

// Export for _testinternalcapi extension.
PyAPI_FUNC(PyObject *) _PyStaticType_GetBuiltins(void);


/* Like PyType_GetModuleState, but skips verification
 * that type is a heap type with an associated module */
static inline void *
_PyType_GetModuleState(PyTypeObject *type)
{
    assert(PyType_Check(type));
    assert(type->tp_flags & Py_TPFLAGS_HEAPTYPE);
    PyHeapTypeObject *et = (PyHeapTypeObject *)type;
    assert(et->ht_module);
    PyModuleObject *mod = (PyModuleObject *)(et->ht_module);
    assert(mod != NULL);
    return mod->md_state;
}


// Export for 'math' shared extension, used via _PyType_IsReady() static inline
// function
PyAPI_FUNC(PyObject *) _PyType_GetDict(PyTypeObject *);
/* 1: eligible native lookup (NULL value means absent); 0: decline. Never fails. */
PyAPI_FUNC(int) _PyType_TryLookupValueNoAlloc(
    PyTypeObject *, PyObject *, PyValue *, unsigned int *);
// Visit what a static type refers to; for the tracing verifier.
extern int _PyType_TraverseStatic(PyTypeObject *, visitproc, void *);

PyAPI_FUNC(PyObject *) _PyType_LookupSubclasses(PyTypeObject *);
PyAPI_FUNC(PyObject *) _PyType_InitSubclasses(PyTypeObject *);

extern PyObject * _PyType_GetBases(PyTypeObject *type);
extern PyObject* _PyType_GetSubclasses(PyTypeObject *);
extern int _PyType_HasSubclasses(PyTypeObject *);

// Export for _testinternalcapi extension.
PyAPI_FUNC(PyObject *) _PyType_GetSlotWrapperNames(void);

// PyType_Ready() must be called if _PyType_IsReady() is false.
// See also the Py_TPFLAGS_READY flag.
static inline int
_PyType_IsReady(PyTypeObject *type)
{
    return _PyType_GetDict(type) != NULL;
}

/* type's getattro: the value slot, its object thunk, and the body, which
   can suppress a missing attribute's AttributeError. */
// Export the pair for '_testinternalcapi' shared extension
PyAPI_FUNC(PyValue) _Py_type_getattro_value(PyValueBorrow type, PyObject *name);
PyAPI_FUNC(PyObject*) _Py_type_getattro(PyObject *type, PyObject *name);
extern PyValue _Py_type_getattro_impl(PyTypeObject *type, PyObject *name,
                                      int *suppress_missing_attribute);

Py_VALUE_ENTRY extern PyObject* _Py_BaseObject_RichCompare(PyObject* self, PyObject* other, int op);

// Export the pairs for '_testinternalcapi' shared extension
int _Py_object_init(PyObject *, PyObject *, PyObject *);
int _Py_object_init_value(PyValueBorrow, const PyValue *, Py_ssize_t, PyObject *);
int _Py_slot_tp_init(PyObject *, PyObject *, PyObject *);
int _Py_slot_tp_init_value(PyValueBorrow, const PyValue *, Py_ssize_t, PyObject *);
PyObject *_Py_slot_tp_new(PyTypeObject *, PyObject *, PyObject *);
PyValue _Py_slot_tp_new_value(PyTypeObject *, const PyValue *, Py_ssize_t, PyObject *);

PyAPI_FUNC(PyObject*) _Py_slot_tp_getattro(PyObject *self, PyObject *name);
PyAPI_FUNC(PyObject*) _Py_slot_tp_getattr_hook(PyObject *self, PyObject *name);
/* Their value sides; the pair table names the first, which the hook
   installs in place of itself. */
PyAPI_FUNC(PyValue) _Py_slot_tp_getattro_value(PyValueBorrow self,
                                               PyObject *name);
PyAPI_FUNC(PyValue) _Py_slot_tp_getattr_hook_value(PyValueBorrow self,
                                                   PyObject *name);

extern int _PyType_HasSlotTpIternext(PyTypeObject *type);

extern PyTypeObject _PyBufferWrapper_Type;

PyAPI_FUNC(PyObject*) _PySuper_Lookup(PyTypeObject *su_type, PyObject *su_obj,
                                 PyObject *name, int *meth_found);

extern PyObject *_PySuper_LookupDescr(PyTypeObject *su_type,
                                     PyTypeObject *su_obj_type,
                                     PyObject *name);

extern PyObject* _PyType_GetFullyQualifiedName(PyTypeObject *type, char sep);

// Perform the following operation, in a thread-safe way when required by the
// build mode.
//
// self->tp_flags = (self->tp_flags & ~mask) | flags;
extern void _PyType_SetFlags(PyTypeObject *self, unsigned long mask,
                             unsigned long flags);
extern int _PyType_AddMethod(PyTypeObject *, PyMethodDef *);

// Like _PyType_SetFlags(), but apply the operation to self and any of its
// subclasses without Py_TPFLAGS_IMMUTABLETYPE set.
extern void _PyType_SetFlagsRecursive(PyTypeObject *self, unsigned long mask,
                                      unsigned long flags);

// Raise PyType_Modified with the type lock already held.
extern void _PyType_Modified_Unlocked(PyTypeObject *type);

PyAPI_FUNC(void) _PyType_SetVersion(PyTypeObject *tp, unsigned int version);
PyTypeObject *_PyType_LookupByVersion(unsigned int version);

// Function pointer type for user-defined validation function that will be
// called by _PyType_Validate().
// It should return 0 if the validation is passed, otherwise it will return -1.
typedef int (*_py_validate_type)(PyTypeObject *);

// It will verify the ``ty`` through user-defined validation function ``validate``,
// and if the validation is passed, it will set the ``tp_version`` as valid
// tp_version_tag from the ``ty``.
// Exported for external JIT support
int _PyType_Validate(PyTypeObject *ty, _py_validate_type validate, unsigned int *tp_version);
int _PyType_CacheGetItemForSpecialization(PyHeapTypeObject *ht, PyObject *descriptor, uint32_t tp_version);

// Precalculates count of non-unique slots and fills wrapperbase.name_count.
extern int _PyType_InitSlotDefs(PyInterpreterState *interp);

// Like PyType_GetBaseByToken, but does not modify refcounts.
// Cannot fail; arguments must be valid.
PyAPI_FUNC(int)
_PyType_GetBaseByToken_Borrow(PyTypeObject *type, void *token, PyTypeObject **result);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TYPEOBJECT_H */
