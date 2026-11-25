/* Rows of the attribute families' recipes (Python/register_ops/getattr.rop).

   A recipe is evidence for one site's name on one receiver type: the type
   version and the payload that go with it, read together from the site's
   feedback entry.  Under the GIL nothing runs between the read of the recipe
   and its use inside one arm, so the recipe is the live entry. */

#ifndef Py_INTERNAL_ATTR_RECIPE_H
#define Py_INTERNAL_ATTR_RECIPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_feedback.h"    // _PyFbAttr
#include "pycore_dict.h"        // _PyObject_Shape()
#include "pycore_function.h"    // _PyClassMethod_GetFunc()
#include "pycore_moduleobject.h" // PyModuleObject
#include "pycore_ir.h"          // _PyIR_FbBlock()
#include "pycore_descrobject.h"  // _PyGetSet_ValueRead()
#include "pycore_object.h"
#include "pycore_value_ops.h"     // _PyValue_GetAttr()      // _PyObject_ValueSlot()
#include "pycore_rop_metadata.h" // ROP_HANDLER_GetMethod_MethodValues

/* One recipe: the guards of a lookup path with the payload that goes with
   them, copied together from the site's entry or from one of its poly rows.
   A site that is installed or refreshed concurrently cannot show a version
   from one observation and an offset from another. */
typedef struct {
    uint32_t type_version;
    uint32_t aux;               // the owner's shape; a module's keys version; a class's version
    uint32_t offset;            // a slot's byte offset, a dict hint or index, a dict offset
    uint32_t read_kind;         // _Py_FB_READ_*
    uint32_t shape_to;          // a store that adds the attribute: the shape after it
    bool values_layout;         // a method row whose receiver has inline values
    _PyValueGuarded class_value;    // the cached method or class value
} _PyAttrRecipe;

static inline bool
_PyAttr_ValidInlineValues(PyObject *o)
{
    assert(Py_TYPE(o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    return FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(o)->valid) != 0;
}

/* The evidence rows of the adaptation macros (Python/specialize.c): the
   position of the case the site's receiver stands for among the cases of the
   row, and the recipe when that case has one.  The last case is "none". */
extern int _PyAttr_ClassifyGetAttr(PyValueBorrow owner, _PyInstr *site,
                                   const _PyFbAttr *fb, PyObject *name,
                                   _PyAttrRecipe *recipe);
extern int _PyAttr_ClassifyGetMethod(PyValueBorrow owner, _PyInstr *site,
                                     const _PyFbAttr *fb, PyObject *name,
                                     _PyAttrRecipe *recipe);
extern int _PyAttr_ClassifySetAttr(PyValueBorrow owner, _PyInstr *site,
                                   const _PyFbAttr *fb, PyObject *name,
                                   _PyAttrRecipe *recipe);
extern bool _PyAttr_PolyShapeReady(_PyInstr *site, const _PyFbAttr *fb);
extern bool _PyAttr_PolyMethodReady(_PyInstr *site, const _PyFbAttr *fb);

/* The site's recipe when its entry holds a live type version */
static inline bool
_PyAttr_AdmittedInstanceSlot(const _PyFbAttr *fb, PyObject *name,
                             _PyAttrRecipe *out)
{
    (void)name;
    if (fb->type_version == 0) {
        return false;
    }
    out->type_version = fb->type_version;
    out->offset = fb->offset;
    out->aux = 0;
    out->read_kind = 0;
    out->values_layout = false;
    return true;
}

/* The site's recipe when it names a method of a type with a live version.
   The entry's read_kind is not used by methods. */
static inline bool
_PyAttr_AdmittedMethod(const _PyFbAttr *fb, PyObject *name, _PyAttrRecipe *out)
{
    (void)name;
    if (fb->type_version == 0) {
        return false;
    }
    out->type_version = fb->type_version;
    out->offset = 0;
    out->aux = 0;
    out->read_kind = 0;
    out->values_layout = true;
    out->class_value = fb->class_value;
    return true;
}

/* The site's recipe for a shape read: the entry holds the type version and
   the shape of the owners it was nominated on */
static inline bool
_PyAttr_AdmittedShape(const _PyFbAttr *fb, PyObject *name, _PyAttrRecipe *out)
{
    (void)name;
#ifdef Py_SHAPES
    if (fb->type_version == 0 || fb->read_kind != _Py_FB_READ_SHAPE) {
        return false;
    }
    out->type_version = fb->type_version;
    out->aux = fb->shape;
    out->offset = fb->offset;
    out->read_kind = _Py_FB_READ_SHAPE;
    out->values_layout = false;
    return true;
#else
    (void)fb; (void)out;
    return false;
#endif
}

/* The site's recipe for a store under a shape: the entry holds the type
   version and the owner's shape, and for an addition the shape after it */
static inline bool
_PyAttr_AdmittedStoreShape(const _PyFbAttr *fb, PyObject *name, _PyAttrRecipe *out)
{
    (void)name;
#ifdef Py_SHAPES
    if (fb->type_version == 0) {
        return false;
    }
    out->type_version = fb->type_version;
    out->aux = fb->shape;
    out->offset = fb->offset;
    out->shape_to = fb->shape_to;
    out->read_kind = 0;
    out->values_layout = false;
    return true;
#else
    (void)fb; (void)out;
    return false;
#endif
}

/* The row of a poly table that records the type version, or -1.  A row
   holds one observation: version, auxiliary word, index and payload. */
static inline int
_PyAttr_PolyRow(const _PyInterpreterFrame *frame, const _PyFbAttr *fb,
                uint32_t version)
{
    if (version == 0 || fb->h.poly == 0) {
        return -1;
    }
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(frame->ir_words), &fb->h);
    for (int i = 0; i < FB_POLY_SIZE; i++) {
        if (poly->type_version[i] == version) {
            return i;
        }
    }
    return -1;
}

/* Poly shape rows: the row of the owner's type, whose shape is the owner's */
static inline bool
_PyAttr_PolyShape(const _PyInterpreterFrame *frame, const _PyFbAttr *fb,
                  PyObject *o, _PyAttrRecipe *out)
{
#ifdef Py_SHAPES
    uint32_t version = FT_ATOMIC_LOAD_UINT_RELAXED(Py_TYPE(o)->tp_version_tag);
    int i = _PyAttr_PolyRow(frame, fb, version);
    if (i < 0) {
        return false;
    }
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(frame->ir_words), &fb->h);
    if (poly->payload[i].read_kind != _Py_FB_READ_SHAPE
        || _PyObject_Shape(o) != poly->aux[i])
    {
        return false;
    }
    out->type_version = version;
    out->aux = poly->aux[i];
    out->offset = poly->index[i];
    out->read_kind = _Py_FB_READ_SHAPE;
    out->values_layout = false;
    return true;
#else
    (void)frame; (void)fb; (void)o; (void)out;
    return false;
#endif
}

/* Poly method rows: the row of the owner's type.  A row observed on a
   receiver with inline values is shadowed by an instance attribute unless
   the receiver's values are still valid (_PyAttr_ValidReceiver). */
static inline bool
_PyAttr_PolyMethod(const _PyInterpreterFrame *frame, const _PyFbAttr *fb,
                   PyObject *o, _PyAttrRecipe *out)
{
    uint32_t version = FT_ATOMIC_LOAD_UINT_RELAXED(Py_TYPE(o)->tp_version_tag);
    int i = _PyAttr_PolyRow(frame, fb, version);
    if (i < 0) {
        return false;
    }
    const _PyFbPoly *poly = _PyFb_PolyOf(_PyIR_FbBlock(frame->ir_words), &fb->h);
    out->type_version = version;
    out->aux = 0;
    out->offset = 0;
    out->read_kind = 0;
    out->values_layout = poly->handler[i] == ROP_HANDLER_GetMethod_MethodValues;
    out->class_value = poly->payload[i].class_value;
    return true;
}

/* A method row that was observed on an inline-values receiver also needs
   this receiver's values to be valid */
static inline bool
_PyAttr_ValidReceiver(PyObject *o, _PyAttrRecipe r)
{
    return !r.values_layout || _PyAttr_ValidInlineValues(o);
}

static inline bool
_PyAttr_ShapeCurrent(PyObject *o, _PyAttrRecipe r)
{
#ifdef Py_SHAPES
    return _PyObject_Shape(o) == r.aux;
#else
    (void)o; (void)r;
    return false;
#endif
}

static inline PyValueBorrow
_PyAttr_BorrowMethod(_PyAttrRecipe r)
{
    return _PyValueGuarded_Load(r.class_value);
}

static inline bool
_PyAttr_InstanceVersion(PyObject *o, _PyAttrRecipe r)
{
    return FT_ATOMIC_LOAD_UINT_RELAXED(Py_TYPE(o)->tp_version_tag) == r.type_version;
}

static inline PyValueBorrow
_PyAttr_BorrowValueSlotFilled(PyObject *o, _PyAttrRecipe r)
{
    PyValueBorrow found = PyValue_Borrow(*_PyObject_ValueSlot(o, r.offset));
    assert(!PyValue_IsNull(found));
    return found;
}

static inline PyValueBorrow
_PyAttr_BorrowValueSlot(PyObject *o, _PyAttrRecipe r)
{
    return PyValue_Borrow(*_PyObject_ValueSlot(o, r.offset));
}

/* Stores.  The guards and the store of an arm run under the GIL with nothing
   between them; the free-threaded build's object lock of the old handlers is
   not carried over. */

/* The owner has no dictionary, its inline values are valid and, under
   shapes, a store that fills a vacant slot has its transition */
static inline bool
_PyAttr_StoreValuesReady(PyObject *o, _PyAttrRecipe r)
{
    assert(Py_TYPE(o)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    if (_PyObject_GetManagedDict(o) || !_PyAttr_ValidInlineValues(o)) {
        return false;
    }
#ifdef Py_SHAPES
    PyValue *slot = _PyObject_ValueSlot(o, r.offset);
    if (PyValue_IsNull(*slot)) {
        PyDictValues *values = _PyObject_InlineValues(o);
        if (_PyShape_TransitionMissing(Py_TYPE(o), values->shape,
                                       slot - values->values))
        {
            return false;
        }
    }
#else
    (void)r;
#endif
    return true;
}

/* The owner's shape is the recipe's and the recipe's slot is filled (a
   store replaces it) or vacant (a store adds it).  The shape proves this
   for an entry written for this handler; the test of the slot keeps a
   store from filling a filled slot when the entry was rewritten for
   another nomination. */
static inline bool
_PyAttr_StoreShapeFilled(PyObject *o, _PyAttrRecipe r)
{
    return _PyAttr_ShapeCurrent(o, r)
        && !PyValue_IsNull(*_PyObject_ValueSlot(o, r.offset));
}

static inline bool
_PyAttr_StoreShapeVacant(PyObject *o, _PyAttrRecipe r)
{
    return _PyAttr_ShapeCurrent(o, r)
        && PyValue_IsNull(*_PyObject_ValueSlot(o, r.offset));
}

/* The managed dict has an entry at the recipe's hint whose key is the name
   and which holds a value, in split or combined keys, and the dict has no
   watchers (their notification may run Python). */
static inline bool
_PyAttr_HintStoreReady(PyObject *o, PyObject *name, _PyAttrRecipe r)
{
    assert(Py_TYPE(o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictObject *dict = _PyObject_GetManagedDict(o);
    if (dict == NULL) {
        return false;
    }
    assert(PyDict_CheckExact((PyObject *)dict));
    PyDictKeysObject *dk = dict->ma_keys;
    if (r.offset >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_nentries) ||
        dk->dk_kind == DICT_KEYS_GENERAL)
    {
        return false;
    }
    PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dk) + r.offset;
    if (FT_ATOMIC_LOAD_PTR_RELAXED(ep->me_key) != name) {
        return false;
    }
    // A split table keeps its values, such as an object's inline values,
    // apart from its keys.
    PyValue *slot = dict->ma_values == NULL ?
        &ep->me_value : &dict->ma_values->values[r.offset];
    return !PyValue_IsNull(*slot) && !(dict->_ma_watcher_tag & DICT_WATCHER_MASK);
}

/* The slot of the hinted entry: the guards of _PyAttr_HintStoreReady hold */
static inline PyValue *
_PyAttr_HintedSlot(PyObject *o, _PyAttrRecipe r)
{
    PyDictObject *dict = _PyObject_GetManagedDict(o);
    PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + r.offset;
    return dict->ma_values == NULL ? &ep->me_value : &dict->ma_values->values[r.offset];
}

/* true with the previous owner when the slot was filled; false when it was
   vacant, which adds the attribute: insertion order and shape follow */
static inline bool
_PyAttr_StoreInstanceValue(PyValueBorrow owner, _PyAttrRecipe r, PyValue x,
                           PyValue *old)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(owner);
    PyValue *slot = _PyObject_ValueSlot(o, r.offset);
    PyValue previous = _PyValueSlot_Exchange(slot, x);
    if (!PyValue_IsNull(previous)) {
        *old = previous;
        return true;
    }
    PyDictValues *values = _PyObject_InlineValues(o);
    Py_ssize_t index = slot - values->values;
    _PyDictValues_AddToInsertionOrder(values, index);
#ifdef Py_SHAPES
    _PyDictValues_SetShape(values,
        _PyShape_FindTransition(Py_TYPE(o), values->shape, index));
#endif
    return false;
}

/* The shape proves the slot filled */
static inline PyValue
_PyAttr_StoreShapeSlot(PyValueBorrow owner, _PyAttrRecipe r, PyValue x)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(owner);
    return _PyValueSlot_ExchangeFilled(_PyObject_ValueSlot(o, r.offset), x);
}

/* The shape proves the slot vacant */
static inline void
_PyAttr_StoreShapeAdd(PyValueBorrow owner, _PyAttrRecipe r, PyValue x)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(owner);
    PyValue *slot = _PyObject_ValueSlot(o, r.offset);
    _PyValueSlot_Fill(slot, x);
    PyDictValues *values = _PyObject_InlineValues(o);
    _PyDictValues_AddToInsertionOrder(values, slot - values->values);
    _PyDictValues_SetShape(values, (uint8_t)r.shape_to);
}

static inline PyValue
_PyAttr_StoreHinted(PyValueBorrow owner, _PyAttrRecipe r, PyValueBorrow x)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(owner);
    return _PyValueSlot_ExchangeFilled(_PyAttr_HintedSlot(o, r), PyValue_DUP(x));
}

/* A member slot: true with the previous owner when it held one */
static inline bool
_PyAttr_StoreMemberSlot(PyValueBorrow owner, _PyAttrRecipe r, PyValue x,
                        PyValue *old)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(owner);
    PyValue previous = _PyValueSlot_Exchange(_PyObject_ValueSlot(o, r.offset), x);
    *old = previous;
    return !PyValue_IsNull(previous);
}

/* The recipe of an entry that an arm of the lookups installed: every field
   of the entry's payload, copied together.  An entry that holds neither a
   type version nor a class or keys version was cleared. */
static inline bool
_PyAttr_AdmittedRecipe(const _PyFbAttr *fb, PyObject *name, _PyAttrRecipe *out)
{
    (void)name;
    if (fb->type_version == 0 && fb->class_version == 0) {
        return false;
    }
    out->type_version = fb->type_version;
    out->aux = fb->class_version;
    out->offset = fb->offset;
    out->read_kind = fb->read_kind;
    out->values_layout = false;
    out->class_value = fb->class_value;
    return true;
}

static inline bool
_PyAttr_IsModule(PyObject *o)
{
    return PyModule_CheckExact(o);
}

/* The module's entry at the recipe's index while its keys have the recipe's
   version; NULL (absent) otherwise */
static inline PyValueBorrow
_PyAttr_ModuleEntry(PyObject *o, _PyAttrRecipe r)
{
    PyDictObject *dict = (PyDictObject *)((PyModuleObject *)o)->md_dict;
    assert(dict != NULL);
    PyDictKeysObject *keys = FT_ATOMIC_LOAD_PTR_ACQUIRE(dict->ma_keys);
    if (FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) != r.aux) {
        return PyValue_Borrow(PyValue_NULL);
    }
    assert(keys->dk_kind == DICT_KEYS_UNICODE);
    if (r.offset >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(keys->dk_nentries)) {
        return PyValue_Borrow(PyValue_NULL);
    }
    return PyValue_Borrow(DK_UNICODE_ENTRIES(keys)[r.offset].me_value);
}

/* The managed dict's entry at the recipe's hint when its key is the name;
   NULL (absent) otherwise */
static inline PyValueBorrow
_PyAttr_HintedEntry(PyObject *o, PyObject *name, _PyAttrRecipe r)
{
    PyValueBorrow none = PyValue_Borrow(PyValue_NULL);
    assert(Py_TYPE(o)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictObject *dict = _PyObject_GetManagedDict(o);
    if (dict == NULL) {
        return none;
    }
    PyDictKeysObject *dk = FT_ATOMIC_LOAD_PTR(dict->ma_keys);
    assert(PyDict_CheckExact((PyObject *)dict));
#ifdef Py_GIL_DISABLED
    if (!_Py_IsOwnedByCurrentThread((PyObject *)dict) && !_PyObject_GC_IS_SHARED(dict)) {
        return none;
    }
#endif
    if (r.offset >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_nentries)
        || dk->dk_kind != DICT_KEYS_UNICODE)
    {
        return none;
    }
    PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dk) + r.offset;
    if (FT_ATOMIC_LOAD_PTR_RELAXED(ep->me_key) != name) {
        return none;
    }
    return PyValue_Borrow(ep->me_value);
}

/* A _Py_T_OBJECT member or __class__: a PyObject * field of the owner; NULL
   (absent) when the field is NULL */
static inline PyValueBorrow
_PyAttr_BorrowPointerSlot(PyObject *o, _PyAttrRecipe r)
{
    PyObject *field = *(PyObject **)((char *)o + r.offset);
    if (field == NULL) {
        return PyValue_Borrow(PyValue_NULL);
    }
    return PyValue_Borrow(PyValue_FromObjectBorrow(field));
}

/* The owner is a class with the recipe's own version, and a metaclass with
   the recipe's version when the nomination required one */
static inline bool
_PyAttr_ClassCurrent(PyObject *o, _PyAttrRecipe r)
{
    if (!PyType_Check(o)) {
        return false;
    }
    assert(r.aux != 0);
    if (FT_ATOMIC_LOAD_UINT_RELAXED(((PyTypeObject *)o)->tp_version_tag) != r.aux) {
        return false;
    }
    return r.type_version == 0
        || FT_ATOMIC_LOAD_UINT_RELAXED(Py_TYPE(o)->tp_version_tag) == r.type_version;
}

static inline bool
_PyAttr_BuiltinMetaclass(PyObject *o)
{
#ifdef Py_GIL_DISABLED
    (void)o;
    return false;
#else
    return Py_TYPE(o) == &PyType_Type;
#endif
}

/* The receiver has a dictionary slot that holds no dictionary yet */
static inline bool
_PyAttr_LazyDictAbsent(PyObject *o, _PyAttrRecipe r)
{
    char *ptr = (char *)o + MANAGED_DICT_OFFSET + r.offset;
    return FT_ATOMIC_LOAD_PTR_ACQUIRE(*(PyObject **)ptr) == NULL;
}

/* The function that the recipe's classmethod wraps; NULL (absent) unless it
   is a Python function.  __init__ can replace the wrapped callable without
   invalidating the class version, so the arm reads it on every hit. */
static inline PyValueBorrow
_PyAttr_ClassMethodFunction(_PyAttrRecipe r)
{
    PyValueBorrow none = PyValue_Borrow(PyValue_NULL);
    PyObject *cm = PyValue_AsKnownObjectBorrow(_PyValueGuarded_LoadObject(r.class_value));
    if (!Py_IS_TYPE(cm, &PyClassMethod_Type)) {
        return none;
    }
    PyObject *target = _PyClassMethod_GetFunc(cm);
    if (target == NULL || !Py_IS_TYPE(target, &PyFunction_Type)) {
        return none;
    }
    return PyValue_Borrow(PyValue_FromObjectBorrow(target));
}

static inline bool
_PyValue_BorrowPresent(PyValueBorrow b)
{
    return !PyValue_IsNull(b);
}

#ifdef __cplusplus
}
#endif
/* _PyValue_GetMethod with the receiver made an owner when the callable is
   an unbound method; *self is NULL when the attribute itself was found. */
static inline PyValue
_PyAttr_GetMethod(PyValueBorrow obj, PyObject *name, PyValue *self)
{
    _PyStatusValue r = (_PyValue_GetMethod)(obj, name);
    *self = PyValue_NULL;
    if (r.status < 0) {
        return PyValue_NULL;
    }
    if (r.status == 1) {
        *self = PyValue_DUP(obj);
    }
    return r.value;
}

static inline void
_PyRop_DiscardMaybe(PyValue z)
{
    PyValue_XCLOSE(z);
}

static inline PyValue
_PyAttr_GetSetRead(_PyAttrRecipe r, PyValueBorrow owner)
{
    PyValueBorrow descr = _PyValueGuarded_LoadObject(r.class_value);
    return (_PyGetSet_ValueRead)(PyValue_AsKnownObjectBorrow(descr), owner);
}

#endif /* !Py_INTERNAL_ATTR_RECIPE_H */
