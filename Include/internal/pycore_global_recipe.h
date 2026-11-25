/* Rows of the GlobalRead family's recipes (Python/register_ops/names.rop).

   A recipe is the evidence of one site: the keys versions of the frame's
   globals (and builtins) with the index of the name in the keys.  A keys
   object has one owner and a version that is never reused, so a match names
   the dictionary the specializer saw. */

#ifndef Py_INTERNAL_GLOBAL_RECIPE_H
#define Py_INTERNAL_GLOBAL_RECIPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_dict.h"          // DK_UNICODE_ENTRIES()
#include "pycore_feedback.h"      // _PyFbGlobal
#include "pycore_frame.h"         // _PyInterpreterFrame

typedef struct {
    uint32_t globals_version;
    uint32_t builtins_version;     // zero for a module-level name
    uint32_t index;
} _PyGlobalRecipe;

/* The position of the case among the cases of classify_global: module,
   builtin, none */
extern int _PyGlobal_Classify(_PyInterpreterFrame *frame, _PyInstr *site,
                              const _PyFbGlobal *fb, PyObject *name,
                              _PyGlobalRecipe *recipe);

/* The site's recipe when its entry names a module-level binding */
static inline bool
_PyGlobal_AdmittedModule(const _PyFbGlobal *fb, _PyGlobalRecipe *out)
{
    if (fb->globals_version == 0) {
        return false;
    }
    out->globals_version = fb->globals_version;
    out->builtins_version = 0;
    out->index = fb->index;
    return true;
}

/* ... and a builtin one, which needs both versions */
static inline bool
_PyGlobal_AdmittedBuiltin(const _PyFbGlobal *fb, _PyGlobalRecipe *out)
{
    if (fb->globals_version == 0 || fb->builtins_version == 0) {
        return false;
    }
    out->globals_version = fb->globals_version;
    out->builtins_version = fb->builtins_version;
    out->index = fb->index;
    return true;
}

/* Globals are always a dict in a word that these rows reach: a word of
   another namespace has another keys object, whose version differs */
static inline PyDictKeysObject *
_PyGlobal_Keys(PyObject *dict)
{
    return FT_ATOMIC_LOAD_PTR_ACQUIRE(((PyDictObject *)dict)->ma_keys);
}

static inline bool
_PyGlobal_GlobalsCurrent(_PyInterpreterFrame *frame, _PyGlobalRecipe r)
{
    PyDictKeysObject *keys = _PyGlobal_Keys(frame->f_globals);
    return FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) == r.globals_version;
}

/* Builtins can be any object: only an exact dict has the keys */
static inline bool
_PyGlobal_BuiltinsCurrent(_PyInterpreterFrame *frame, _PyGlobalRecipe r)
{
    PyObject *builtins = frame->f_builtins;
    if (!PyDict_CheckExact(builtins)) {
        return false;
    }
    PyDictKeysObject *keys = _PyGlobal_Keys(builtins);
    return FT_ATOMIC_LOAD_UINT32_RELAXED(keys->dk_version) == r.builtins_version;
}

/* The value at the recipe's index, NULL (absent) when it is past the
   entries or deleted */
static inline PyValueBorrow
_PyGlobal_Slot(PyObject *dict, _PyGlobalRecipe r)
{
    PyDictKeysObject *keys = _PyGlobal_Keys(dict);
    assert(keys->dk_kind == DICT_KEYS_UNICODE);
    if (r.index >= (size_t)FT_ATOMIC_LOAD_SSIZE_RELAXED(keys->dk_nentries)) {
        return PyValue_Borrow(PyValue_NULL);
    }
    return PyValue_Borrow(DK_UNICODE_ENTRIES(keys)[r.index].me_value);
}

static inline PyValueBorrow
_PyGlobal_GlobalsSlot(_PyInterpreterFrame *frame, _PyGlobalRecipe r)
{
    return _PyGlobal_Slot(frame->f_globals, r);
}

static inline PyValueBorrow
_PyGlobal_BuiltinsSlot(_PyInterpreterFrame *frame, _PyGlobalRecipe r)
{
    return _PyGlobal_Slot(frame->f_builtins, r);
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_RECIPE_H */
