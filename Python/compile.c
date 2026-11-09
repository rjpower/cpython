/*
 * This file compiles an abstract syntax tree (AST) into Python bytecode.
 *
 * The primary entry point is _PyAST_Compile(), which returns a
 * PyCodeObject.  The compiler makes several passes to build the code
 * object:
 *   1. Checks for future statements.  See future.c
 *   2. Builds a symbol table.  See symtable.c.
 *   3. Generate an instruction sequence. See compiler_mod() in this file, which
 *      calls functions from codegen.c.
 *   4. Generate a control flow graph and run optimizations on it.  See flowgraph.c.
 *   5. Assemble the basic blocks into final code.  See optimize_and_assemble() in
 *      this file, and assembler.c.
 *
 */

#include "Python.h"
#include "pycore_ast.h"           // PyAST_Check()
#include "pycore_capsule.h"       // _PyCapsule_SetTraverse()
#include "pycore_code.h"
#include "pycore_compile.h"
#include "pycore_dict.h"
#include "pycore_flowgraph.h"     // _PyCfg_FromInstructionSequence()
#include "pycore_list.h"
#include "pycore_long_value.h"    // _PyValue_AsLong()
#include "pycore_regcfg.h"        // _PyRegCfg_Free()
#include "pycore_pystate.h"       // _Py_GetConfig()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_setobject.h"     // _PySet_NextValue()
#include "pycore_sliceobject.h"   // _PySlice_AsTuple()
#include "pycore_stats.h"
#include "pycore_tuple.h"         // _PyTuple_FromPair
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()
#include "pycore_value.h"
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()

#include "cpython/code.h"

#include <stdbool.h>


#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    do {                    \
        if ((X) == -1) {    \
            return ERROR;   \
        }                   \
    } while (0)

typedef _Py_SourceLocation location;
typedef _PyJumpTargetLabel jump_target_label;
typedef _PyInstructionSequence instr_sequence;
typedef struct _PyCfgBuilder cfg_builder;
typedef _PyCompile_FBlockInfo fblockinfo;
typedef enum _PyCompile_FBlockType fblocktype;

/* The following items change on entry and exit of code blocks.
   They must be saved and restored when returning to a block.
*/
struct compiler_unit {
    PySTEntryObject *u_ste;

    int u_scope_type;

    PyObject *u_private;            /* for private name mangling */
    PyObject *u_static_attributes;  /* for class: attributes accessed via self.X */
    PyObject *u_deferred_annotations; /* AnnAssign nodes deferred to the end of compilation */
    PyObject *u_conditional_annotation_indices;  /* indices of annotations that are conditionally executed (or -1 for unconditional annotations) */
    long u_next_conditional_annotation_index;  /* index of the next conditional annotation */

    instr_sequence *u_instr_sequence; /* codegen output */
    instr_sequence *u_stashed_instr_sequence; /* temporarily stashed parent instruction sequence */

    int u_nfblocks;
    int u_in_conditional_block;

    _PyCompile_FBlockInfo u_fblock[CO_MAXBLOCKS];

    _PyCompile_CodeUnitMetadata u_metadata;
};

/* This struct captures the global state of a compilation.

The u pointer points to the current compilation unit, while units
for enclosing blocks are stored in c_stack.     The u and c_stack are
managed by _PyCompile_EnterScope() and _PyCompile_ExitScope().

Note that we don't track recursion levels during compilation - the
task of detecting and rejecting excessive levels of nesting is
handled by the symbol analysis pass.

*/

typedef struct _PyCompiler {
    PyObject *c_filename;
    struct symtable *c_st;
    _PyFutureFeatures c_future;  /* module's __future__ */
    PyCompilerFlags c_flags;

    int c_optimize;              /* optimization level */
    int c_interactive;           /* true if in interactive mode */
    PyObject *c_const_cache;     /* Python dict holding all constants,
                                    including names tuple */
    struct compiler_unit *u;     /* compiler state for current block */
    PyObject *c_stack;           /* Python list holding compiler_unit ptrs */

    bool c_save_nested_seqs;     /* if true, construct recursive instruction sequences
                                  * (including instructions for nested code objects)
                                  */
    int c_disable_warning;
    PyObject *c_module;
} compiler;

static int
compiler_setup(compiler *c, mod_ty mod, PyObject *filename,
               PyCompilerFlags *flags, int optimize, PyArena *arena,
               PyObject *module)
{
    PyCompilerFlags local_flags = _PyCompilerFlags_INIT;

    c->c_const_cache = PyDict_New();
    if (!c->c_const_cache) {
        return ERROR;
    }

    c->c_stack = PyList_New(0);
    if (!c->c_stack) {
        return ERROR;
    }

    c->c_filename = Py_NewRef(filename);
    if (!_PyFuture_FromAST(mod, filename, &c->c_future)) {
        return ERROR;
    }
    c->c_module = Py_XNewRef(module);
    if (!flags) {
        flags = &local_flags;
    }
    int merged = c->c_future.ff_features | flags->cf_flags;
    c->c_future.ff_features = merged;
    flags->cf_flags = merged;
    c->c_flags = *flags;
    c->c_optimize = (optimize == -1) ? _Py_GetConfig()->optimization_level : optimize;
    c->c_save_nested_seqs = false;

    if (!_PyAST_Preprocess(mod, arena, filename, c->c_optimize, merged,
                           0, 1, module))
    {
        return ERROR;
    }
    c->c_st = _PySymtable_Build(mod, filename, &c->c_future);
    if (c->c_st == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError, "no symtable");
        }
        return ERROR;
    }
    return SUCCESS;
}

static void
compiler_free(compiler *c)
{
    if (c->c_st) {
        _PySymtable_Free(c->c_st);
    }
    Py_XDECREF(c->c_filename);
    Py_XDECREF(c->c_module);
    Py_XDECREF(c->c_const_cache);
    Py_XDECREF(c->c_stack);
    PyMem_Free(c);
}

static compiler*
new_compiler(mod_ty mod, PyObject *filename, PyCompilerFlags *pflags,
             int optimize, PyArena *arena, PyObject *module)
{
    compiler *c = PyMem_Calloc(1, sizeof(compiler));
    if (c == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    if (compiler_setup(c, mod, filename, pflags, optimize, arena, module) < 0) {
        compiler_free(c);
        return NULL;
    }
    return c;
}

static void
compiler_unit_free(struct compiler_unit *u)
{
    Py_CLEAR(u->u_instr_sequence);
    Py_CLEAR(u->u_stashed_instr_sequence);
    Py_CLEAR(u->u_ste);
    Py_CLEAR(u->u_metadata.u_name);
    Py_CLEAR(u->u_metadata.u_qualname);
    Py_CLEAR(u->u_metadata.u_consts);
    Py_CLEAR(u->u_metadata.u_names);
    Py_CLEAR(u->u_metadata.u_varnames);
    Py_CLEAR(u->u_metadata.u_freevars);
    Py_CLEAR(u->u_metadata.u_cellvars);
    Py_CLEAR(u->u_metadata.u_fasthidden);
    Py_CLEAR(u->u_private);
    Py_CLEAR(u->u_static_attributes);
    Py_CLEAR(u->u_deferred_annotations);
    Py_CLEAR(u->u_conditional_annotation_indices);
    PyMem_Free(u);
}

#define CAPSULE_NAME "compile.c compiler unit"

/* The capsules on c_stack hold the units of the enclosing scopes, which are
   PyMem blocks that no traverse function reaches.  The capsule visits the
   unit's objects, so that a collection while a nested scope is compiled sees
   them (note 06, section 14.5). */
static int
compiler_unit_capsule_traverse(PyObject *capsule, visitproc visit, void *arg)
{
    // A capsule popped from c_stack has no name: the compiler may have
    // freed its unit (_PyCompile_ExitScope()).
    if (!PyCapsule_IsValid(capsule, CAPSULE_NAME)) {
        return 0;
    }
    struct compiler_unit *u = (struct compiler_unit *)PyCapsule_GetPointer(
        capsule, CAPSULE_NAME);
    Py_VISIT(u->u_instr_sequence);
    Py_VISIT(u->u_stashed_instr_sequence);
    Py_VISIT(u->u_ste);
    Py_VISIT(u->u_metadata.u_name);
    Py_VISIT(u->u_metadata.u_qualname);
    Py_VISIT(u->u_metadata.u_consts);
    Py_VISIT(u->u_metadata.u_names);
    Py_VISIT(u->u_metadata.u_varnames);
    Py_VISIT(u->u_metadata.u_freevars);
    Py_VISIT(u->u_metadata.u_cellvars);
    Py_VISIT(u->u_metadata.u_fasthidden);
    Py_VISIT(u->u_private);
    Py_VISIT(u->u_static_attributes);
    Py_VISIT(u->u_deferred_annotations);
    Py_VISIT(u->u_conditional_annotation_indices);
    return 0;
}

/* The compiler frees the unit itself in _PyCompile_ExitScope(). */
static int
compiler_unit_capsule_clear(PyObject *capsule)
{
    return 0;
}

/* Compiler stack, name and symbol-entry lists contain object metadata only. */
static PyObject *
compiler_metadata_item(PyObject *list, Py_ssize_t index)
{
    PyValueBorrow value = PyList_GET_VALUE(list, index);
    assert(PyValue_IsObject(value));
    return PyValue_AsKnownObjectBorrow(value);
}

int
_PyCompile_MaybeAddStaticAttributeToClass(compiler *c, expr_ty e)
{
    assert(e->kind == Attribute_kind);
    expr_ty attr_value = e->v.Attribute.value;
    if (attr_value->kind != Name_kind ||
        e->v.Attribute.ctx != Store ||
        !_PyUnicode_EqualToASCIIString(attr_value->v.Name.id, "self"))
    {
        return SUCCESS;
    }
    Py_ssize_t stack_size = PyList_GET_SIZE(c->c_stack);
    for (Py_ssize_t i = stack_size - 1; i >= 0; i--) {
        PyObject *capsule = compiler_metadata_item(c->c_stack, i);
        struct compiler_unit *u = (struct compiler_unit *)PyCapsule_GetPointer(
                                                              capsule, CAPSULE_NAME);
        assert(u);
        if (u->u_scope_type == COMPILE_SCOPE_CLASS) {
            assert(u->u_static_attributes);
            RETURN_IF_ERROR(PySet_AddValue(u->u_static_attributes, PyValue_FromObjectBorrow(e->v.Attribute.attr)));
            break;
        }
    }
    return SUCCESS;
}

int
_PyCompile_SetQualname(compiler *c)
{
    Py_ssize_t stack_size;
    struct compiler_unit *u = c->u;
    PyObject *name, *base;

    if (u->u_scope_type == COMPILE_SCOPE_MODULE) {
        return SUCCESS;
    }

    base = NULL;
    stack_size = PyList_GET_SIZE(c->c_stack);
    assert(stack_size >= 1);
    if (stack_size > 1) {
        int scope, force_global = 0;
        struct compiler_unit *parent;
        PyObject *mangled, *capsule;

        capsule = compiler_metadata_item(c->c_stack, stack_size - 1);
        parent = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        assert(parent);
        if (parent->u_scope_type == COMPILE_SCOPE_ANNOTATIONS) {
            /* The parent is an annotation scope, so we need to
               look at the grandparent. */
            if (stack_size == 2) {
                // If we're immediately within the module, we can skip
                // the rest and just set the qualname to be the same as name.
                u->u_metadata.u_qualname = Py_NewRef(u->u_metadata.u_name);
                return SUCCESS;
            }
            capsule = compiler_metadata_item(c->c_stack, stack_size - 2);
            parent = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
            assert(parent);
        }

        if (u->u_scope_type == COMPILE_SCOPE_FUNCTION
            || u->u_scope_type == COMPILE_SCOPE_ASYNC_FUNCTION
            || u->u_scope_type == COMPILE_SCOPE_CLASS) {
            assert(u->u_metadata.u_name);
            mangled = _Py_Mangle(parent->u_private, u->u_metadata.u_name);
            if (!mangled) {
                return ERROR;
            }

            scope = _PyST_GetScope(parent->u_ste, mangled);
            Py_DECREF(mangled);
            RETURN_IF_ERROR(scope);
            assert(scope != GLOBAL_IMPLICIT);
            if (scope == GLOBAL_EXPLICIT)
                force_global = 1;
        }

        if (!force_global) {
            if (parent->u_scope_type == COMPILE_SCOPE_FUNCTION
                || parent->u_scope_type == COMPILE_SCOPE_ASYNC_FUNCTION
                || parent->u_scope_type == COMPILE_SCOPE_LAMBDA)
            {
                _Py_DECLARE_STR(dot_locals, ".<locals>");
                base = PyUnicode_Concat(parent->u_metadata.u_qualname,
                                        &_Py_STR(dot_locals));
                if (base == NULL) {
                    return ERROR;
                }
            }
            else {
                base = Py_NewRef(parent->u_metadata.u_qualname);
            }
        }
        if (u->u_ste->ste_function_name != NULL) {
            PyObject *tmp = base;
            base = PyUnicode_FromFormat("%U.%U",
                base,
                u->u_ste->ste_function_name);
            Py_DECREF(tmp);
            if (base == NULL) {
                return ERROR;
            }
        }
    }
    else if (u->u_ste->ste_function_name != NULL) {
        base = Py_NewRef(u->u_ste->ste_function_name);
    }

    if (base != NULL) {
        name = PyUnicode_Concat(base, _Py_LATIN1_CHR('.'));
        Py_DECREF(base);
        if (name == NULL) {
            return ERROR;
        }
        PyUnicode_Append(&name, u->u_metadata.u_name);
        if (name == NULL) {
            return ERROR;
        }
    }
    else {
        name = Py_NewRef(u->u_metadata.u_name);
    }
    // The qualified name is interned when the code object is created,
    // whatever its content.  The body of a class also has it as a constant,
    // and intern_final_constants() only interns the constants selected by
    // _PyCode_ShouldInternString().  Intern the other qualified names now,
    // so that the same object is always used for both.
    if (!_PyCode_ShouldInternString(name)) {
        _PyUnicode_InternMortal(_PyInterpreterState_GET(), &name);
    }
    u->u_metadata.u_qualname = name;

    return SUCCESS;
}

/* Shared constants, including hashed cache keys, may change representative
 * only when typed equality and hash remain unchanged. Both recursive merging
 * and string interning below satisfy that contract. Consume replacement. */
static void
replace_equivalent_constant(PyObject *tuple, Py_ssize_t index, PyValue replacement)
{
    // a move: replaced below
    PyValue old = ((PyTupleObject *)tuple)->ob_item[index];
    ((PyTupleObject *)tuple)->ob_item[index] = replacement;
    PyValue_CLOSE(old);
}

static PyValue
constant_from_key(PyValue key)
{
    if (PyValue_IsObject(key) &&
        PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(key))) {
        return PyTuple_GetValue(PyValue_AsKnownObjectBorrow(key), 1);
    }
    return PyValue_DUP(key);
}

#define const_cache_insert(V0, V1, V2) \
    (const_cache_insert)(V0, PyValue_Borrow(V1), V2)

/* Merge constant and return an owned canonical key.
 * If recursive, insert all elements if o is a tuple or frozen set.
 */
static PyValue
(const_cache_insert)(PyObject *const_cache, PyValueBorrow constant, bool recursive)
{
    assert(PyDict_CheckExact(const_cache));
    PyValue input = PyValue_DUP(constant);
    PyObject *o = PyValue_IsObject(input) ? PyValue_AsKnownObjectBorrow(input) : NULL;
    if (o == Py_None || o == Py_Ellipsis) {
        return input;
    }

    PyValue key = _PyCode_ConstantValueKey(input);
    if (PyValue_IsNull(key)) {
        PyValue_CLOSE(input);
        return PyValue_NULL;
    }

    PyValue selected = PyValue_NULL;
    int res = _PyDict_SetDefaultValue(const_cache, key, key, &selected);
    if (res != 0) {
        PyValue_CLOSE(key);
        PyValue_CLOSE(input);
        return selected;
    }
    PyValue_CLOSE(selected);

    if (!recursive) {
        PyValue_CLOSE(input);
        return key;
    }

    if (o != NULL && PyTuple_CheckExact(o)) {
        Py_ssize_t len = PyTuple_GET_SIZE(o);
        for (Py_ssize_t i = 0; i < len; i++) {
            PyValue item = PyTuple_GetValue(o, i);
            PyValue child_key = const_cache_insert(const_cache, item, true);
            PyValue_CLOSE(item);
            if (PyValue_IsNull(child_key)) {
                goto error;
            }
            PyValue replacement = constant_from_key(child_key);
            PyValue_CLOSE(child_key);
            if (PyValue_IsNull(replacement)) {
                goto error;
            }
            replace_equivalent_constant(o, i, replacement);
        }
    }
    else if (o != NULL && PyFrozenSet_CheckExact(o)) {
        Py_ssize_t len = PySet_GET_SIZE(o);
        if (len != 0) {
            PyObject *merged = PyFrozenSet_New(NULL);
            if (merged == NULL) {
                goto error;
            }
            Py_ssize_t pos = 0;
            PyValue item;
            Py_hash_t hash;
            int next;
            while ((next = _PySet_NextValue(o, &pos, &item, &hash)) > 0) {
                PyValue child_key = const_cache_insert(const_cache, item, true);
                PyValue_CLOSE(item);
                if (PyValue_IsNull(child_key)) {
                    Py_DECREF(merged);
                    goto error;
                }
                PyValue replacement = constant_from_key(child_key);
                PyValue_CLOSE(child_key);
                if (PyValue_IsNull(replacement)) {
                    Py_DECREF(merged);
                    goto error;
                }
                int status = PySet_AddValue(merged, replacement);
                PyValue_CLOSE(replacement);
                if (status < 0) {
                    Py_DECREF(merged);
                    goto error;
                }
            }
            if (next < 0) {
                Py_DECREF(merged);
                goto error;
            }
            replace_equivalent_constant(PyValue_AsKnownObjectBorrow(key), 1,
                                        PyValue_FromObjectSteal(merged));
        }
    }

    PyValue_CLOSE(input);
    return key;
error:
    PyValue_CLOSE(key);
    PyValue_CLOSE(input);
    return PyValue_NULL;
}

/* The index of key in dict, adding it with the next index if absent. It
   only reads key: the dict retains its own count. */
static Py_ssize_t
dict_add_value(PyObject *dict, PyValueBorrow key)
{
    Py_ssize_t index = PyDict_GET_SIZE(dict);
    PyValue proposed = PyValue_FromInt64(index);
    if (PyValue_IsNull(proposed)) {
        return ERROR;
    }
    PyValue selected = PyValue_NULL;
    int status = _PyDict_SetDefaultValue(dict, key, proposed, &selected);
    PyValue_CLOSE(proposed);
    if (status < 0) {
        return ERROR;
    }
    Py_ssize_t result = PyValue_IsInlineInt(selected)
                        ? PyValue_InlineInt64(selected)
                        : _PyValue_IntAsSsize_t(selected);
    PyValue_CLOSE(selected);
    return result;
}
#define dict_add_value(D, K) (dict_add_value)((D), PyValue_Borrow(K))

Py_ssize_t
_PyCompile_DictAddObj(PyObject *dict, PyObject *o)
{
    return dict_add_value(dict, PyValue_FromObjectBorrow(o));
}

Py_ssize_t
_PyCompile_AddConst(compiler *c, PyObject *o)
{
    PyValue key = const_cache_insert(c->c_const_cache,
                                   PyValue_FromObjectBorrow(o), true);
    if (PyValue_IsNull(key)) {
        return ERROR;
    }

    Py_ssize_t arg = dict_add_value(c->u->u_metadata.u_consts, key);
    PyValue_CLOSE(key);
    return arg;
}

Py_ssize_t
_PyCompile_AddConstValue(compiler *c, PyValueBorrow value)
{
    PyValue key = const_cache_insert(c->c_const_cache, value, true);
    if (PyValue_IsNull(key)) {
        return ERROR;
    }

    Py_ssize_t arg = dict_add_value(c->u->u_metadata.u_consts, key);
    PyValue_CLOSE(key);
    return arg;
}

static PyObject *
list2dict(PyObject *list)
{
    Py_ssize_t i, n;
    PyObject *v, *k;
    PyObject *dict = PyDict_New();
    if (!dict) return NULL;

    n = PyList_Size(list);
    for (i = 0; i < n; i++) {
        v = PyLong_FromSsize_t(i);
        if (!v) {
            Py_DECREF(dict);
            return NULL;
        }
        k = compiler_metadata_item(list, i);
        if (PyDict_SetValue(dict, PyValue_FromObjectBorrow(k), PyValue_FromObjectBorrow(v)) < 0) {
            Py_DECREF(v);
            Py_DECREF(dict);
            return NULL;
        }
        Py_DECREF(v);
    }
    return dict;
}

/* Return new dict containing names from src that match scope(s).

src is a symbol table dictionary.  If the scope of a name matches
either scope_type or flag is set, insert it into the new dict.  The
values are integers, starting at offset and increasing by one for
each key.
*/

static PyObject *
dictbytype(PyObject *src, int scope_type, int flag, Py_ssize_t offset)
{
    Py_ssize_t i = offset, num_keys, key_i;
    PyObject *k = NULL, *dest = PyDict_New();
    PyValue value = PyValue_NULL;
    PyObject *sorted_keys;

    assert(offset >= 0);
    if (dest == NULL)
        return NULL;

    /* Sort the keys so that we have a deterministic order on the indexes
       saved in the returned dictionary.  These indexes are used as indexes
       into the free and cell var storage.  Therefore if they aren't
       deterministic, then the generated bytecode is not deterministic.
    */
    sorted_keys = PyDict_Keys(src);
    if (sorted_keys == NULL) {
        Py_DECREF(dest);
        return NULL;
    }
    if (PyList_Sort(sorted_keys) != 0) {
        Py_DECREF(sorted_keys);
        Py_DECREF(dest);
        return NULL;
    }
    num_keys = PyList_GET_SIZE(sorted_keys);

    for (key_i = 0; key_i < num_keys; key_i++) {
        k = Py_NewRef(compiler_metadata_item(sorted_keys, key_i));
        int found = PyDict_GetValue(src, PyValue_FromObjectBorrow(k), &value);
        if (found <= 0) {
            if (found == 0) {
                PyErr_SetObject(PyExc_KeyError, k);
            }
            goto error;
        }
        assert(PyValue_LongCheckExact(value));
        long vi = PyValue_IsInlineInt(value) ? PyValue_InlineInt64(value) :
                  _PyValue_AsLong(value);
        if (vi == -1 && PyErr_Occurred()) {
            goto error;
        }
        PyValue_CLEAR(value);
        if (SYMBOL_TO_SCOPE(vi) == scope_type || vi & flag) {
            PyObject *item = PyLong_FromSsize_t(i);
            if (item == NULL) {
                goto error;
            }
            i++;
            if (PyDict_SetValue(dest, PyValue_FromObjectBorrow(k), PyValue_FromObjectBorrow(item)) < 0) {
                Py_DECREF(item);
                goto error;
            }
            Py_DECREF(item);
        }
        Py_CLEAR(k);
    }
    Py_DECREF(sorted_keys);
    return dest;

error:;
    PyValue_XCLOSE(value);
    Py_XDECREF(k);
    Py_DECREF(sorted_keys);
    Py_DECREF(dest);
    return NULL;
}

static int
add_cell_names_from_symbols(PyObject *symbols, PyObject *names)
{
    Py_ssize_t pos = 0;
    PyValue key = PyValue_NULL, value = PyValue_NULL;
    int status;
    while ((status = _PyDict_NextValue(symbols, &pos, &key, &value, NULL)) > 0) {
        assert(PyValue_IsObject(key));
        assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(key)));
        assert(PyValue_LongCheckExact(value));
        long flags = PyValue_IsInlineInt(value) ? PyValue_InlineInt64(value) :
                     _PyValue_AsLong(value);
        if (flags == -1 && PyErr_Occurred()) {
            goto error;
        }
        if (SYMBOL_TO_SCOPE(flags) == CELL) {
            if (PySet_AddValue(names, key) < 0) {
                goto error;
            }
        }
        PyValue_CLEAR(key);
        PyValue_CLEAR(value);
    }
    return status < 0 ? ERROR : SUCCESS;

error:;
    PyValue_XCLOSE(key);
    PyValue_XCLOSE(value);
    return ERROR;
}

static int
add_inlined_comprehension_cell_names(PySTEntryObject *ste, PyObject *names)
{
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(ste->ste_children); i++) {
        PySTEntryObject *child =
            (PySTEntryObject *)compiler_metadata_item(ste->ste_children, i);
        if (child->ste_type != InlinedComprehensionBlock) {
            continue;
        }
        if (add_cell_names_from_symbols(child->ste_symbols, names) < 0) {
            return ERROR;
        }
        if (add_inlined_comprehension_cell_names(child, names) < 0) {
            return ERROR;
        }
    }
    return SUCCESS;
}

/* Cells of the shared unit: this table's CELL names, plus cells that live
 * only on inlined comprehension children. */
static PyObject *
compiler_cellvars(PySTEntryObject *ste)
{
    PyObject *names = PySet_New(NULL);
    if (names == NULL) {
        return NULL;
    }
    if (add_cell_names_from_symbols(ste->ste_symbols, names) < 0) {
        Py_DECREF(names);
        return NULL;
    }
    if (add_inlined_comprehension_cell_names(ste, names) < 0) {
        Py_DECREF(names);
        return NULL;
    }
    PyObject *sorted = _PyValue_SequenceList(PyValue_FromObjectBorrow(names));
    Py_DECREF(names);
    if (sorted == NULL) {
        return NULL;
    }
    if (PyList_Sort(sorted) < 0) {
        Py_DECREF(sorted);
        return NULL;
    }
    PyObject *cellvars = list2dict(sorted);
    Py_DECREF(sorted);
    return cellvars;
}

int
_PyCompile_EnterScope(compiler *c, identifier name, int scope_type,
                       void *key, int lineno, PyObject *private,
                      _PyCompile_CodeUnitMetadata *umd)
{
    struct compiler_unit *u;
    u = (struct compiler_unit *)PyMem_Calloc(1, sizeof(struct compiler_unit));
    if (!u) {
        PyErr_NoMemory();
        return ERROR;
    }
    u->u_scope_type = scope_type;
    if (umd != NULL) {
        u->u_metadata = *umd;
    }
    else {
        u->u_metadata.u_argcount = 0;
        u->u_metadata.u_posonlyargcount = 0;
        u->u_metadata.u_kwonlyargcount = 0;
    }
    u->u_ste = _PySymtable_Lookup(c->c_st, key);
    if (!u->u_ste) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_name = Py_NewRef(name);
    u->u_metadata.u_varnames = list2dict(u->u_ste->ste_varnames);
    if (!u->u_metadata.u_varnames) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_cellvars = compiler_cellvars(u->u_ste);
    if (!u->u_metadata.u_cellvars) {
        compiler_unit_free(u);
        return ERROR;
    }
    if (u->u_ste->ste_needs_class_closure) {
        /* Cook up an implicit __class__ cell. */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILE_SCOPE_CLASS);
        res = _PyCompile_DictAddObj(u->u_metadata.u_cellvars, &_Py_ID(__class__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    if (u->u_ste->ste_needs_classdict) {
        /* Cook up an implicit __classdict__ cell. */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILE_SCOPE_CLASS);
        res = _PyCompile_DictAddObj(u->u_metadata.u_cellvars, &_Py_ID(__classdict__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    if (u->u_ste->ste_has_conditional_annotations) {
        /* Cook up an implicit __conditional_annotations__ cell */
        Py_ssize_t res;
        assert(u->u_scope_type == COMPILE_SCOPE_CLASS || u->u_scope_type == COMPILE_SCOPE_MODULE);
        res = _PyCompile_DictAddObj(u->u_metadata.u_cellvars, &_Py_ID(__conditional_annotations__));
        if (res < 0) {
            compiler_unit_free(u);
            return ERROR;
        }
    }

    u->u_metadata.u_freevars = dictbytype(u->u_ste->ste_symbols, FREE, DEF_FREE_CLASS,
                               PyDict_GET_SIZE(u->u_metadata.u_cellvars));
    if (!u->u_metadata.u_freevars) {
        compiler_unit_free(u);
        return ERROR;
    }

    if (scope_type == COMPILE_SCOPE_MODULE || scope_type == COMPILE_SCOPE_CLASS) {
        u->u_metadata.u_fasthidden = PySet_New(NULL);
        if (!u->u_metadata.u_fasthidden) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    else {
        u->u_metadata.u_fasthidden = NULL;
    }

    u->u_nfblocks = 0;
    u->u_metadata.u_firstlineno = lineno;
    u->u_metadata.u_consts = PyDict_New();
    if (!u->u_metadata.u_consts) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_metadata.u_names = PyDict_New();
    if (!u->u_metadata.u_names) {
        compiler_unit_free(u);
        return ERROR;
    }

    u->u_deferred_annotations = NULL;
    u->u_conditional_annotation_indices = NULL;
    u->u_next_conditional_annotation_index = 0;
    if (scope_type == COMPILE_SCOPE_CLASS) {
        u->u_static_attributes = PySet_New(0);
        if (!u->u_static_attributes) {
            compiler_unit_free(u);
            return ERROR;
        }
    }
    else {
        u->u_static_attributes = NULL;
    }

    u->u_instr_sequence = (instr_sequence*)_PyInstructionSequence_New();
    if (!u->u_instr_sequence) {
        compiler_unit_free(u);
        return ERROR;
    }
    u->u_stashed_instr_sequence = NULL;

    /* Push the old compiler_unit on the stack. */
    if (c->u) {
        PyObject *capsule = PyCapsule_New(c->u, CAPSULE_NAME, NULL);
        if (!capsule
            || _PyCapsule_SetTraverse(capsule, compiler_unit_capsule_traverse,
                                      compiler_unit_capsule_clear) < 0
            || PyList_AppendValue(c->c_stack, PyValue_FromObjectBorrow(capsule)) < 0)
        {
            Py_XDECREF(capsule);
            compiler_unit_free(u);
            return ERROR;
        }
        Py_DECREF(capsule);
        if (private == NULL) {
            private = c->u->u_private;
        }
    }

    u->u_private = Py_XNewRef(private);

    c->u = u;
    return SUCCESS;
}

void
_PyCompile_ExitScope(compiler *c)
{
    // Don't call PySequence_DelItem() with an exception raised
    PyObject *exc = PyErr_GetRaisedException();

    instr_sequence *nested_seq = NULL;
    if (c->c_save_nested_seqs) {
        nested_seq = c->u->u_instr_sequence;
        Py_INCREF(nested_seq);
    }
    compiler_unit_free(c->u);
    /* Restore c->u to the parent unit. */
    Py_ssize_t n = PyList_GET_SIZE(c->c_stack) - 1;
    if (n >= 0) {
        PyObject *capsule = compiler_metadata_item(c->c_stack, n);
        c->u = (struct compiler_unit *)PyCapsule_GetPointer(capsule, CAPSULE_NAME);
        assert(c->u);
        // The capsule can outlive the unit, which its traverse must then
        // not read; cannot fail for a valid capsule.
        (void)PyCapsule_SetName(capsule, NULL);
        /* we are deleting from a list so this really shouldn't fail */
        if (_PyValue_SequenceDelItem(PyValue_FromObjectBorrow(c->c_stack), n) < 0) {
            PyErr_FormatUnraisable("Exception ignored while removing "
                                   "the last compiler stack item");
        }
        if (nested_seq != NULL) {
            if (_PyInstructionSequence_AddNested(c->u->u_instr_sequence, nested_seq) < 0) {
                PyErr_FormatUnraisable("Exception ignored while appending "
                                       "nested instruction sequence");
            }
        }
    }
    else {
        c->u = NULL;
    }
    Py_XDECREF(nested_seq);

    PyErr_SetRaisedException(exc);
}

/*
 * Frame block handling functions
 */

int
_PyCompile_PushFBlock(compiler *c, location loc,
                     fblocktype t, jump_target_label block_label,
                     jump_target_label exit, void *datum)
{
    fblockinfo *f;
    if (c->u->u_nfblocks >= CO_MAXBLOCKS) {
        return _PyCompile_Error(c, loc, "too many statically nested blocks");
    }
    f = &c->u->u_fblock[c->u->u_nfblocks++];
    f->fb_type = t;
    f->fb_block = block_label;
    f->fb_loc = loc;
    f->fb_exit = exit;
    f->fb_datum = datum;
    if (t == COMPILE_FBLOCK_FINALLY_END) {
        c->c_disable_warning++;
    }
    return SUCCESS;
}

void
_PyCompile_PopFBlock(compiler *c, fblocktype t, jump_target_label block_label)
{
    struct compiler_unit *u = c->u;
    assert(u->u_nfblocks > 0);
    u->u_nfblocks--;
    assert(u->u_fblock[u->u_nfblocks].fb_type == t);
    assert(SAME_JUMP_TARGET_LABEL(u->u_fblock[u->u_nfblocks].fb_block, block_label));
    if (t == COMPILE_FBLOCK_FINALLY_END) {
        c->c_disable_warning--;
    }
}

fblockinfo *
_PyCompile_TopFBlock(compiler *c)
{
    if (c->u->u_nfblocks == 0) {
        return NULL;
    }
    return &c->u->u_fblock[c->u->u_nfblocks - 1];
}

bool
_PyCompile_InExceptionHandler(compiler *c)
{
    for (Py_ssize_t i = 0; i < c->u->u_nfblocks; i++) {
        fblockinfo *block = &c->u->u_fblock[i];
        switch (block->fb_type) {
            case COMPILE_FBLOCK_TRY_EXCEPT:
            case COMPILE_FBLOCK_FINALLY_TRY:
            case COMPILE_FBLOCK_FINALLY_END:
            case COMPILE_FBLOCK_EXCEPTION_HANDLER:
            case COMPILE_FBLOCK_EXCEPTION_GROUP_HANDLER:
            case COMPILE_FBLOCK_HANDLER_CLEANUP:
                return true;
            default:
                break;
        }
    }
    return false;
}

void
_PyCompile_DeferredAnnotations(compiler *c,
                               PyObject **deferred_annotations,
                               PyObject **conditional_annotation_indices)
{
    *deferred_annotations = Py_XNewRef(c->u->u_deferred_annotations);
    *conditional_annotation_indices = Py_XNewRef(c->u->u_conditional_annotation_indices);
}

static location
start_location(asdl_stmt_seq *stmts)
{
    if (asdl_seq_LEN(stmts) > 0) {
        /* Set current line number to the line number of first statement.
         * This way line number for SETUP_ANNOTATIONS will always
         * coincide with the line number of first "real" statement in module.
         * If body is empty, then lineno will be set later in the assembly stage.
         */
        stmt_ty st = (stmt_ty)asdl_seq_GET(stmts, 0);
        return SRC_LOCATION_FROM_AST(st);
    }
    return (const _Py_SourceLocation){1, 1, 0, 0};
}

static int
compiler_codegen(compiler *c, mod_ty mod)
{
    RETURN_IF_ERROR(_PyCodegen_EnterAnonymousScope(c, mod));
    assert(c->u->u_scope_type == COMPILE_SCOPE_MODULE);
    switch (mod->kind) {
    case Module_kind: {
        asdl_stmt_seq *stmts = mod->v.Module.body;
        RETURN_IF_ERROR(_PyCodegen_Module(c, start_location(stmts), stmts, false));
        break;
    }
    case Interactive_kind: {
        c->c_interactive = 1;
        asdl_stmt_seq *stmts = mod->v.Interactive.body;
        RETURN_IF_ERROR(_PyCodegen_Module(c, start_location(stmts), stmts, true));
        break;
    }
    case Expression_kind: {
        RETURN_IF_ERROR(_PyCodegen_Expression(c, mod->v.Expression.body));
        break;
    }
    default: {
        PyErr_Format(PyExc_SystemError,
                     "module kind %d should not be possible",
                     mod->kind);
        return ERROR;
    }}
    return SUCCESS;
}

static PyCodeObject *
compiler_mod(compiler *c, mod_ty mod)
{
    PyCodeObject *co = NULL;
    int addNone = mod->kind != Expression_kind;
    assert(c->u == NULL);
    if (compiler_codegen(c, mod) < 0) {
        goto finally;
    }
    co = _PyCompile_OptimizeAndAssemble(c, addNone);
finally:
    if (c->u != NULL) {
        _PyCompile_ExitScope(c);
    }
    return co;
}

/* Inlined comprehensions are compiled in the enclosing unit. If a name is
 * FREE in the comprehension, or is absent from its table (scope 0), resolve
 * it in enclosing tables until it is bound. Stop if the next table is a class:
 * nested scopes (including inlined comprehensions) do not see class locals, so
 * the name stays FREE. __class__ and friends are not allowed to be free
 * through a class; treat those loads as implicit globals.
 *
 * Names with no entry (scope 0) include loads synthesized by codegen, such as
 * the implicit receiver for zero-arg super(). */
static int
compiler_resolve_inlined_free(PySTEntryObject **ste, PyObject *name)
{
    int scope = _PyST_GetScope(*ste, name);
    RETURN_IF_ERROR(scope);
    while ((*ste)->ste_type == InlinedComprehensionBlock &&
           (scope == FREE || scope == 0)) {
        PySTEntryObject *parent = (*ste)->ste_parent;
        assert(parent != NULL);
        if (parent->ste_type == ClassBlock) {
            if (_PyST_IsClassClosureName(name)) {
                return GLOBAL_IMPLICIT;
            }
            break;
        }
        *ste = parent;
        scope = _PyST_GetScope(*ste, name);
        RETURN_IF_ERROR(scope);
    }
    return scope;
}

int
_PyCompile_GetRefType(compiler *c, PyObject *name)
{
    if (c->u->u_scope_type == COMPILE_SCOPE_CLASS && _PyST_IsClassClosureName(name)) {
        return CELL;
    }
    PySTEntryObject *ste = c->u->u_ste;
    int scope = compiler_resolve_inlined_free(&ste, name);
    RETURN_IF_ERROR(scope);
    if (scope == 0) {
        PyErr_Format(PyExc_SystemError,
                     "_PyST_GetScope(name=%R) failed: "
                     "unknown scope in unit %S (%R); "
                     "symbols: %R; locals: %R; "
                     "globals: %R",
                     name,
                     c->u->u_metadata.u_name, ste->ste_id,
                     ste->ste_symbols, c->u->u_metadata.u_varnames,
                     c->u->u_metadata.u_names);
        return ERROR;
    }
    return scope;
}

static int
dict_lookup_arg(PyObject *dict, PyObject *name)
{
    PyValue value;
    if (PyDict_GetValue(dict, PyValue_FromObjectBorrow(name), &value) <= 0) {
        return ERROR;
    }
    assert(PyValue_LongCheckExact(value));
    long result = PyValue_IsInlineInt(value) ? PyValue_InlineInt64(value) :
                  _PyValue_AsLong(value);
    PyValue_CLOSE(value);
    return result;
}

int
_PyCompile_LookupCellvar(compiler *c, PyObject *name)
{
    assert(c->u->u_metadata.u_cellvars);
    return dict_lookup_arg(c->u->u_metadata.u_cellvars, name);
}

int
_PyCompile_LookupArg(compiler *c, PyCodeObject *co, PyObject *name)
{
    /* Special case: If a class contains a method with a
     * free variable that has the same name as a method,
     * the name will be considered free *and* local in the
     * class.  It should be handled by the closure, as
     * well as by the normal name lookup logic.
     */
    int reftype = _PyCompile_GetRefType(c, name);
    if (reftype == -1) {
        return ERROR;
    }
    int arg;
    if (reftype == CELL) {
        arg = dict_lookup_arg(c->u->u_metadata.u_cellvars, name);
    }
    else {
        arg = dict_lookup_arg(c->u->u_metadata.u_freevars, name);
    }
    if (arg == -1 && !PyErr_Occurred()) {
        PyObject *freevars = _PyCode_GetFreevars(co);
        if (freevars == NULL) {
            PyErr_Clear();
        }
        PyErr_Format(PyExc_SystemError,
            "compiler_lookup_arg(name=%R) with reftype=%d failed in %S; "
            "freevars of code %S: %R",
            name,
            reftype,
            c->u->u_metadata.u_name,
            co->co_name,
            freevars);
        Py_XDECREF(freevars);
        return ERROR;
    }
    return arg;
}

PyObject *
_PyCompile_StaticAttributesAsTuple(compiler *c)
{
    assert(c->u->u_static_attributes);
    PyObject *static_attributes_unsorted = _PyValue_SequenceList(PyValue_FromObjectBorrow(c->u->u_static_attributes));
    if (static_attributes_unsorted == NULL) {
        return NULL;
    }
    if (PyList_Sort(static_attributes_unsorted) != 0) {
        Py_DECREF(static_attributes_unsorted);
        return NULL;
    }
    PyObject *static_attributes = _PyValue_SequenceTuple(PyValue_FromObjectBorrow(static_attributes_unsorted));
    Py_DECREF(static_attributes_unsorted);
    return static_attributes;
}

int
_PyCompile_ResolveNameop(compiler *c, PyObject *mangled,
                          _PyCompile_optype *optype, Py_ssize_t *arg)
{
    PyObject *dict = c->u->u_metadata.u_names;
    *optype = COMPILE_OP_NAME;

    PySTEntryObject *ste = c->u->u_ste;
    assert(ste != NULL);

    int scope = compiler_resolve_inlined_free(&ste, mangled);
    RETURN_IF_ERROR(scope);

    switch (scope) {
    case FREE:
        dict = c->u->u_metadata.u_freevars;
        *optype = COMPILE_OP_DEREF;
        break;
    case CELL:
        dict = c->u->u_metadata.u_cellvars;
        *optype = COMPILE_OP_DEREF;
        break;
    case LOCAL:
        /* Inlined comprehensions isolate their locals as FAST, even when
         * nested in class or module scope. */
        if (_PyST_IsFunctionLike(ste) || ste->ste_type == InlinedComprehensionBlock) {
            *optype = COMPILE_OP_FAST;
        }
        break;
    case GLOBAL_IMPLICIT: {
        /* Opcode depends on the enclosing non-inlined scope. */
        PySTEntryObject *enclosing = ste;
        while (enclosing->ste_parent != NULL) {
            assert(enclosing->ste_type == InlinedComprehensionBlock);
            enclosing = enclosing->ste_parent;
        }
        if (_PyST_IsFunctionLike(enclosing)) {
            *optype = COMPILE_OP_GLOBAL;
        }
        break;
    }
    case GLOBAL_EXPLICIT:
        *optype = COMPILE_OP_GLOBAL;
        break;
    default:
        /* scope can be 0 */
        break;
    }
    /* XXX Handle __doc__ and the like better */
    assert(scope || PyUnicode_READ_CHAR(mangled, 0) == '_');
    if (*optype != COMPILE_OP_FAST) {
        *arg = _PyCompile_DictAddObj(dict, mangled);
        RETURN_IF_ERROR(*arg);
    }
    return scope;
}

int
_PyCompile_EnterInlinedComprehensionScope(compiler *c, PySTEntryObject *entry,
                                          _PyCompile_InlinedComprehensionState *state)
{
    assert(state->saved_ste == NULL);
    state->saved_ste = c->u->u_ste;
    c->u->u_ste = (PySTEntryObject *)Py_NewRef(entry);
    return SUCCESS;
}

int
_PyCompile_ExitInlinedComprehensionScope(compiler *c,
                                        _PyCompile_InlinedComprehensionState *state)
{
    assert(state->saved_ste != NULL);
    Py_DECREF(c->u->u_ste);
    c->u->u_ste = state->saved_ste;
    state->saved_ste = NULL;
    return SUCCESS;
}

void
_PyCompile_EnterConditionalBlock(struct _PyCompiler *c)
{
    c->u->u_in_conditional_block++;
}

void
_PyCompile_LeaveConditionalBlock(struct _PyCompiler *c)
{
    assert(c->u->u_in_conditional_block > 0);
    c->u->u_in_conditional_block--;
}

int
_PyCompile_AddDeferredAnnotation(compiler *c, stmt_ty s,
                                 PyObject **conditional_annotation_index)
{
    if (c->u->u_deferred_annotations == NULL) {
        c->u->u_deferred_annotations = PyList_New(0);
        if (c->u->u_deferred_annotations == NULL) {
            return ERROR;
        }
    }
    if (c->u->u_conditional_annotation_indices == NULL) {
        c->u->u_conditional_annotation_indices = PyList_New(0);
        if (c->u->u_conditional_annotation_indices == NULL) {
            return ERROR;
        }
    }
    PyObject *ptr = PyLong_FromVoidPtr((void *)s);
    if (ptr == NULL) {
        return ERROR;
    }
    if (PyList_AppendValue(c->u->u_deferred_annotations, PyValue_FromObjectBorrow(ptr)) < 0) {
        Py_DECREF(ptr);
        return ERROR;
    }
    Py_DECREF(ptr);
    PyObject *index;
    if (c->u->u_scope_type == COMPILE_SCOPE_MODULE || c->u->u_in_conditional_block) {
        index = PyLong_FromLong(c->u->u_next_conditional_annotation_index);
        if (index == NULL) {
            return ERROR;
        }
        *conditional_annotation_index = Py_NewRef(index);
        c->u->u_next_conditional_annotation_index++;
    }
    else {
        index = PyLong_FromLong(-1);
        if (index == NULL) {
            return ERROR;
        }
    }
    int rc = PyList_AppendValue(c->u->u_conditional_annotation_indices, PyValue_FromObjectBorrow(index));
    Py_DECREF(index);
    RETURN_IF_ERROR(rc);
    return SUCCESS;
}

/* Raises a SyntaxError and returns ERROR.
 * If something goes wrong, a different exception may be raised.
 */
int
_PyCompile_Error(compiler *c, location loc, const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (msg == NULL) {
        return ERROR;
    }
    _PyErr_RaiseSyntaxError(msg, c->c_filename, loc.lineno, loc.col_offset + 1,
                            loc.end_lineno, loc.end_col_offset + 1);
    Py_DECREF(msg);
    return ERROR;
}

/* Emits a SyntaxWarning and returns 0 on success.
   If a SyntaxWarning raised as error, replaces it with a SyntaxError
   and returns -1.
*/
int
_PyCompile_Warn(compiler *c, location loc, const char *format, ...)
{
    if (c->c_disable_warning) {
        return 0;
    }
    va_list vargs;
    va_start(vargs, format);
    PyObject *msg = PyUnicode_FromFormatV(format, vargs);
    va_end(vargs);
    if (msg == NULL) {
        return ERROR;
    }
    int ret = _PyErr_EmitSyntaxWarning(msg, c->c_filename, loc.lineno, loc.col_offset + 1,
                                       loc.end_lineno, loc.end_col_offset + 1,
                                       c->c_module);
    Py_DECREF(msg);
    return ret;
}

PyObject *
_PyCompile_Mangle(compiler *c, PyObject *name)
{
    return _Py_Mangle(c->u->u_private, name);
}

PyObject *
_PyCompile_MaybeMangle(compiler *c, PyObject *name)
{
    return _Py_MaybeMangle(c->u->u_private, c->u->u_ste, name);
}

instr_sequence *
_PyCompile_InstrSequence(compiler *c)
{
    return c->u->u_instr_sequence;
}

int
_PyCompile_StartAnnotationSetup(struct _PyCompiler *c)
{
    instr_sequence *new_seq = (instr_sequence *)_PyInstructionSequence_New();
    if (new_seq == NULL) {
        return ERROR;
    }
    assert(c->u->u_stashed_instr_sequence == NULL);
    c->u->u_stashed_instr_sequence = c->u->u_instr_sequence;
    c->u->u_instr_sequence = new_seq;
    return SUCCESS;
}

int
_PyCompile_EndAnnotationSetup(struct _PyCompiler *c)
{
    assert(c->u->u_stashed_instr_sequence != NULL);
    instr_sequence *parent_seq = c->u->u_stashed_instr_sequence;
    instr_sequence *anno_seq = c->u->u_instr_sequence;
    c->u->u_stashed_instr_sequence = NULL;
    c->u->u_instr_sequence = parent_seq;
    if (_PyInstructionSequence_SetAnnotationsCode(parent_seq, anno_seq) == ERROR) {
        Py_DECREF(anno_seq);
        return ERROR;
    }
    return SUCCESS;
}


int
_PyCompile_FutureFeatures(compiler *c)
{
    return c->c_future.ff_features;
}

struct symtable *
_PyCompile_Symtable(compiler *c)
{
    return c->c_st;
}

PySTEntryObject *
_PyCompile_SymtableEntry(compiler *c)
{
    return c->u->u_ste;
}

int
_PyCompile_OptimizationLevel(compiler *c)
{
    return c->c_optimize;
}

int
_PyCompile_IsInteractiveTopLevel(compiler *c)
{
    assert(c->c_stack != NULL);
    assert(PyList_CheckExact(c->c_stack));
    bool is_nested_scope = PyList_GET_SIZE(c->c_stack) > 0;
    return c->c_interactive && !is_nested_scope;
}

int
_PyCompile_ScopeType(compiler *c)
{
    return c->u->u_scope_type;
}

PyObject *
_PyCompile_Qualname(compiler *c)
{
    assert(c->u->u_metadata.u_qualname);
    return c->u->u_metadata.u_qualname;
}

_PyCompile_CodeUnitMetadata *
_PyCompile_Metadata(compiler *c)
{
    return &c->u->u_metadata;
}

// Replace an owned value with its canonical constant, without recursion.
int
_PyCompile_ConstCacheMergeValue(PyObject *const_cache, PyValue *value)
{
    PyValue key = const_cache_insert(const_cache, *value, false);
    if (PyValue_IsNull(key)) {
        return ERROR;
    }
    PyValue replacement = constant_from_key(key);
    PyValue_CLOSE(key);
    if (PyValue_IsNull(replacement)) {
        return ERROR;
    }
    PyValue_CLOSE(*value);
    *value = replacement;
    return SUCCESS;
}

// Merge *obj* with constant cache, without recursion.
int
_PyCompile_ConstCacheMergeOne(PyObject *const_cache, PyObject **obj)
{
    PyValue key = const_cache_insert(const_cache,
                                   PyValue_FromObjectBorrow(*obj), false);
    if (PyValue_IsNull(key)) {
        return ERROR;
    }
    PyValue value = constant_from_key(key);
    PyValue_CLOSE(key);
    if (PyValue_IsNull(value)) {
        return ERROR;
    }
    PyObject *replacement = _PyValue_AsObjectAt(value, BOX_SITE_COMPILER);
    PyValue_CLOSE(value);
    if (replacement == NULL) {
        return ERROR;
    }
    Py_SETREF(*obj, replacement);
    return SUCCESS;
}

static PyObject *
consts_dict_keys_inorder(PyObject *dict)
{
    Py_ssize_t pos = 0, size = PyDict_GET_SIZE(dict);

    PyObject *consts = PyList_New(size);
    if (consts == NULL)
        return NULL;
    PyValue key, index;
    int next;
    while ((next = _PyDict_NextValue(dict, &pos, &key, &index, NULL)) > 0) {
        Py_ssize_t i = PyValue_IsInlineInt(index)
                       ? PyValue_InlineInt64(index)
                       : _PyValue_IntAsSsize_t(index);
        PyValue_CLOSE(index);
        if (i < 0 || i >= size) {
            PyValue_CLOSE(key);
            Py_DECREF(consts);
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_SystemError, "invalid compiler constant index");
            }
            return NULL;
        }
        PyValue constant = constant_from_key(key);
        PyValue_CLOSE(key);
        if (PyValue_IsNull(constant)) {
            Py_DECREF(consts);
            return NULL;
        }
        PyList_SET_VALUE(consts, i, constant);
    }
    if (next < 0) {
        Py_DECREF(consts);
        return NULL;
    }
    return consts;
}

/* Intern the strings of the constant *o* which intern_constants() in
 * codeobject.c would intern when a code object is created.
 *
 * The strings of a slice are interned in all builds, although
 * intern_constants() only looks into slices in the free-threaded build,
 * so that the constants of a compilation are the same objects in both
 * builds.
 *
 * Return a new reference to the constant to use instead of *o*.  The items
 * of a tuple are replaced in place.  A frozenset or a slice with an item
 * to replace cannot be modified: a new object is created and stored in its
 * entry of *const_cache*, which is where the other users of *o* find it.
 */
static PyValue
intern_constant(PyObject *const_cache, PyValue constant)
{
    PyValue input = PyValue_DUP(constant);
    if (!PyValue_IsObject(input)) {
        return input;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(input);
    if (PyUnicode_CheckExact(o)) {
        o = PyValue_AsKnownObjectSteal(input);
        if (_PyCode_ShouldInternString(o)) {
            _PyUnicode_InternMortal(_PyInterpreterState_GET(), &o);
        }
        return PyValue_FromObjectSteal(o);
    }
    if (PyTuple_CheckExact(o)) {
        Py_ssize_t len = PyTuple_GET_SIZE(o);
        for (Py_ssize_t i = 0; i < len; i++) {
            PyValue item = PyTuple_GetValue(o, i);
            PyValue replacement = intern_constant(const_cache, item);
            PyValue_CLOSE(item);
            if (PyValue_IsNull(replacement)) {
                PyValue_CLOSE(input);
                return PyValue_NULL;
            }
            replace_equivalent_constant(o, i, replacement);
        }
        return input;
    }
    if (!PyFrozenSet_CheckExact(o) && !PySlice_Check(o)) {
        return input;
    }

    // The key of a frozenset or of a slice is a pair whose second item is
    // the object which stands for all the equal constants.
    // See _PyCode_ConstantKey().
    PyValue key = const_cache_insert(const_cache, input, false);
    PyValue_CLOSE(input);
    if (PyValue_IsNull(key)) {
        return PyValue_NULL;
    }
    PyValue selected = constant_from_key(key);
    if (PyValue_IsNull(selected)) {
        PyValue_CLOSE(key);
        return PyValue_NULL;
    }
    o = PyValue_AsKnownObjectBorrow(selected);

    PyObject *items;
    if (PyFrozenSet_CheckExact(o)) {
        items = PyTuple_New(PySet_GET_SIZE(o));
        if (items != NULL) {
            Py_ssize_t pos = 0, i = 0;
            PyValue item;
            Py_hash_t hash;
            int next;
            while ((next = _PySet_NextValue(o, &pos, &item, &hash)) > 0) {
                PyTuple_SET_VALUE(items, i++, item);
            }
            if (next < 0) {
                Py_CLEAR(items);
            }
        }
    }
    else {
        items = _PySlice_AsTuple((PySliceObject *)o);
    }
    if (items == NULL) {
        PyValue_CLOSE(selected);
        PyValue_CLOSE(key);
        return PyValue_NULL;
    }
    bool modified = false;
    Py_ssize_t len = PyTuple_GET_SIZE(items);
    for (Py_ssize_t i = 0; i < len; i++) {
        PyValue item = PyTuple_GetValue(items, i);
        PyValue replacement = intern_constant(const_cache, item);
        if (PyValue_IsNull(replacement)) {
            PyValue_CLOSE(item);
            Py_DECREF(items);
            PyValue_CLOSE(selected);
            PyValue_CLOSE(key);
            return PyValue_NULL;
        }
        if (!PyValue_Is(item, replacement)) {
            modified = true;
        }
        PyValue_CLOSE(item);
        replace_equivalent_constant(items, i, replacement);
    }
    if (modified) {
        PyObject *new;
        if (PyFrozenSet_CheckExact(o)) {
            new = PyFrozenSet_New(NULL);
            if (new != NULL) {
                for (Py_ssize_t i = 0; i < len; i++) {
                    PyValue item = PyTuple_GetValue(items, i);
                    int status = PySet_AddValue(new, item);
                    PyValue_CLOSE(item);
                    if (status < 0) {
                        Py_CLEAR(new);
                        break;
                    }
                }
            }
        }
        else {
            PyValue fields[3];
            for (int i = 0; i < 3; i++) {
                fields[i] = PyTuple_GetValue(items, i);
            }
            new = _PySlice_FromValues(fields[0], fields[1], fields[2]);
            for (int i = 0; i < 3; i++) {
                PyValue_CLOSE(fields[i]);
            }
        }
        if (new == NULL) {
            Py_DECREF(items);
            PyValue_CLOSE(selected);
            PyValue_CLOSE(key);
            return PyValue_NULL;
        }
        PyValue replacement = PyValue_FromObjectSteal(new);
        replace_equivalent_constant(PyValue_AsKnownObjectBorrow(key), 1,
                                    PyValue_DUP(replacement));
        PyValue_CLOSE(selected);
        selected = replacement;
    }
    Py_DECREF(items);
    PyValue_CLOSE(key);
    return selected;
}

/* Intern the strings of the constants of a code unit, once the optimizer
 * has removed the constants which are not used.
 *
 * intern_constants() in codeobject.c interns the string constants of each
 * new code object.  When it has to replace a string of a frozenset (or, in
 * the free-threaded build, of a slice) by a string which is already
 * interned, it makes a copy of the container for that code object only.
 * The constants of a code object would then be shared with the other code
 * objects of the compilation or not depending on which strings happen to
 * be interned in the process, and so would the output of marshal
 * (gh-129724).  Replace the strings here instead, where all the code units
 * get the same container from the constant cache.
 */
static int
intern_final_constants(PyObject *const_cache, PyObject *consts)
{
    assert(PyList_CheckExact(consts));
    Py_ssize_t len = PyList_GET_SIZE(consts);
    for (Py_ssize_t i = 0; i < len; i++) {
        PyValue item = PyList_GetValue(consts, i);
        PyValue replacement = intern_constant(const_cache, item);
        PyValue_CLOSE(item);
        if (PyValue_IsNull(replacement)) {
            return ERROR;
        }
        if (_PyList_SetItemTakeValue((PyListObject *)consts, i, replacement) < 0) {
            return ERROR;
        }
    }
    return SUCCESS;
}

static int
compute_code_flags(compiler *c)
{
    PySTEntryObject *ste = c->u->u_ste;
    int flags = 0;
    if (_PyST_IsFunctionLike(ste)) {
        flags |= CO_NEWLOCALS | CO_OPTIMIZED;
        if (ste->ste_nested)
            flags |= CO_NESTED;
        if (ste->ste_generator && !ste->ste_coroutine)
            flags |= CO_GENERATOR;
        if (ste->ste_generator && ste->ste_coroutine)
            flags |= CO_ASYNC_GENERATOR;
        if (ste->ste_varargs)
            flags |= CO_VARARGS;
        if (ste->ste_varkeywords)
            flags |= CO_VARKEYWORDS;
        if (ste->ste_has_docstring)
            flags |= CO_HAS_DOCSTRING;
        if (ste->ste_method)
            flags |= CO_METHOD;
    }

    if (ste->ste_coroutine && !ste->ste_generator) {
        flags |= CO_COROUTINE;
    }

    /* (Only) inherit compilerflags in PyCF_MASK */
    flags |= (c->c_flags.cf_flags & PyCF_MASK);

    return flags;
}

static PyCodeObject *
optimize_and_assemble_code_unit(struct compiler_unit *u, PyObject *const_cache,
                                int code_flags, PyObject *filename)
{
    cfg_builder *g = NULL;
    _PyRegCfg *regcfg = NULL;

    PyCodeObject *co = NULL;
    PyObject *consts = consts_dict_keys_inorder(u->u_metadata.u_consts);
    if (consts == NULL) {
        goto error;
    }
    g = _PyCfg_FromInstructionSequence(u->u_instr_sequence);
    if (g == NULL) {
        goto error;
    }
    int nlocals = (int)PyDict_GET_SIZE(u->u_metadata.u_varnames);
    int nparams = (int)PyList_GET_SIZE(u->u_ste->ste_varnames);
    assert(u->u_metadata.u_firstlineno);

    if (_PyCfg_OptimizeCodeUnit(g, consts, const_cache, nlocals,
                                nparams, u->u_metadata.u_firstlineno) < 0) {
        goto error;
    }

    int stackdepth;
    int nlocalsplus;
    if (_PyCfg_OptimizedCfgToRegCfg(g, &u->u_metadata, code_flags, &stackdepth,
                                    &nlocalsplus, &regcfg) < 0) {
        goto error;
    }

    if (intern_final_constants(const_cache, consts) < 0) {
        goto error;
    }

    /** Assembly **/
    co = _PyAssemble_MakeCodeObject(&u->u_metadata, const_cache, consts,
                                    regcfg, nlocalsplus, code_flags, filename);

error:
    Py_XDECREF(consts);
    _PyRegCfg_Free(regcfg);
    _PyCfgBuilder_Free(g);
    return co;
}


PyCodeObject *
_PyCompile_OptimizeAndAssemble(compiler *c, int addNone)
{
    struct compiler_unit *u = c->u;
    PyObject *const_cache = c->c_const_cache;
    PyObject *filename = c->c_filename;

    int code_flags = compute_code_flags(c);
    assert(code_flags >= 0);
    if (_PyCodegen_AddReturnAtEnd(c, addNone) < 0) {
        return NULL;
    }

    return optimize_and_assemble_code_unit(u, const_cache, code_flags, filename);
}

PyCodeObject *
_PyAST_Compile(mod_ty mod, PyObject *filename, PyCompilerFlags *pflags,
               int optimize, PyArena *arena, PyObject *module)
{
    assert(!PyErr_Occurred());
    compiler *c = new_compiler(mod, filename, pflags, optimize, arena, module);
    if (c == NULL) {
        return NULL;
    }

    PyCodeObject *co = compiler_mod(c, mod);
    compiler_free(c);
    assert(co || PyErr_Occurred());
    return co;
}

int
_PyCompile_AstPreprocess(mod_ty mod, PyObject *filename, PyCompilerFlags *cf,
                         int optimize, PyArena *arena, int no_const_folding,
                         PyObject *module)
{
    _PyFutureFeatures future;
    if (!_PyFuture_FromAST(mod, filename, &future)) {
        return -1;
    }
    int flags = future.ff_features | cf->cf_flags;
    if (optimize == -1) {
        optimize = _Py_GetConfig()->optimization_level;
    }
    if (!_PyAST_Preprocess(mod, arena, filename, optimize, flags,
                           no_const_folding, 0, module))
    {
        return -1;
    }
    return 0;
}

// C implementation of inspect.cleandoc()
//
// Difference from inspect.cleandoc():
// - Do not remove leading and trailing blank lines to keep lineno.
PyObject *
_PyCompile_CleanDoc(PyObject *doc)
{
    doc = PyObject_CallMethod(doc, "expandtabs", NULL);
    if (doc == NULL) {
        return NULL;
    }

    Py_ssize_t doc_size;
    const char *doc_utf8 = PyUnicode_AsUTF8AndSize(doc, &doc_size);
    if (doc_utf8 == NULL) {
        Py_DECREF(doc);
        return NULL;
    }
    const char *p = doc_utf8;
    const char *pend = p + doc_size;

    // First pass: find minimum indentation of any non-blank lines
    // after first line.
    while (p < pend && *p++ != '\n') {
    }

    Py_ssize_t margin = PY_SSIZE_T_MAX;
    while (p < pend) {
        const char *s = p;
        while (*p == ' ') p++;
        if (p < pend && *p != '\n') {
            margin = Py_MIN(margin, p - s);
        }
        while (p < pend && *p++ != '\n') {
        }
    }
    if (margin == PY_SSIZE_T_MAX) {
        margin = 0;
    }

    // Second pass: write cleandoc into buff.

    // copy first line without leading spaces.
    p = doc_utf8;
    while (*p == ' ') {
        p++;
    }
    if (p == doc_utf8 && margin == 0 ) {
        // doc is already clean.
        return doc;
    }

    char *buff = PyMem_Malloc(doc_size);
    if (buff == NULL){
        Py_DECREF(doc);
        PyErr_NoMemory();
        return NULL;
    }

    char *w = buff;

    while (p < pend) {
        int ch = *w++ = *p++;
        if (ch == '\n') {
            break;
        }
    }

    // copy subsequent lines without margin.
    while (p < pend) {
        for (Py_ssize_t i = 0; i < margin; i++, p++) {
            if (*p != ' ') {
                assert(*p == '\n' || *p == '\0');
                break;
            }
        }
        while (p < pend) {
            int ch = *w++ = *p++;
            if (ch == '\n') {
                break;
            }
        }
    }

    Py_DECREF(doc);
    PyObject *res = PyUnicode_FromStringAndSize(buff, w - buff);
    PyMem_Free(buff);
    return res;
}

/* Access to compiler optimizations for unit tests.
 *
 * _PyCompile_CodeGen takes an AST, applies code-gen and
 * returns the unoptimized CFG as an instruction list.
 *
 */
PyObject *
_PyCompile_CodeGen(PyObject *ast, PyObject *filename, PyCompilerFlags *pflags,
                   int optimize, int compile_mode)
{
    PyObject *res = NULL;
    PyObject *metadata = NULL;
    PyObject *consts_list = NULL;

    if (!PyAST_Check(ast)) {
        PyErr_SetString(PyExc_TypeError, "expected an AST");
        return NULL;
    }

    PyArena *arena = _PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty mod = PyAST_obj2mod(ast, arena, compile_mode);
    if (mod == NULL || !_PyAST_Validate(mod)) {
        _PyArena_Free(arena);
        return NULL;
    }

    compiler *c = new_compiler(mod, filename, pflags, optimize, arena, NULL);
    if (c == NULL) {
        _PyArena_Free(arena);
        return NULL;
    }
    c->c_save_nested_seqs = true;

    metadata = PyDict_New();
    if (metadata == NULL) {
        goto finally;
    }

    if (compiler_codegen(c, mod) < 0) {
        goto finally;
    }

    _PyCompile_CodeUnitMetadata *umd = &c->u->u_metadata;

#define SET_METADATA_INT(key, value) do { \
        PyObject *v = PyLong_FromLong((long)value); \
        if (v == NULL) goto finally; \
        int res = PyDict_SetValueString(metadata, key, PyValue_FromObjectBorrow(v)); \
        Py_XDECREF(v); \
        if (res < 0) goto finally; \
    } while (0);

    SET_METADATA_INT("argcount", umd->u_argcount);
    SET_METADATA_INT("posonlyargcount", umd->u_posonlyargcount);
    SET_METADATA_INT("kwonlyargcount", umd->u_kwonlyargcount);
#undef SET_METADATA_INT

    int addNone = mod->kind != Expression_kind;
    if (_PyCodegen_AddReturnAtEnd(c, addNone) < 0) {
        goto finally;
    }

    if (_PyInstructionSequence_ApplyLabelMap(_PyCompile_InstrSequence(c)) < 0) {
        goto finally;
    }

    /* After AddReturnAtEnd: co_consts indices match the final instruction stream. */
    consts_list = consts_dict_keys_inorder(umd->u_consts);
    if (consts_list == NULL) {
        goto finally;
    }
    if (PyDict_SetValueString(metadata, "consts", PyValue_FromObjectBorrow(consts_list)) < 0) {
        goto finally;
    }

    /* Allocate a copy of the instruction sequence on the heap */
    res = _PyTuple_PackValues(2, (PyValueBorrow[]){PyValue_FromObjectBorrow((PyObject *)_PyCompile_InstrSequence(c)), PyValue_FromObjectBorrow(metadata)});

finally:
    Py_XDECREF(consts_list);
    Py_XDECREF(metadata);
    if (c->u != NULL) {
        _PyCompile_ExitScope(c);
    }
    compiler_free(c);
    _PyArena_Free(arena);
    return res;
}

int _PyCfg_JumpLabelsToTargets(cfg_builder *g);

PyCodeObject *
_PyCompile_Assemble(_PyCompile_CodeUnitMetadata *umd, PyObject *filename,
                    PyObject *seq)
{
    if (!_PyInstructionSequence_Check(seq)) {
        PyErr_SetString(PyExc_TypeError, "expected an instruction sequence");
        return NULL;
    }
    cfg_builder *g = NULL;
    PyCodeObject *co = NULL;
    _PyRegCfg *regcfg = NULL;

    PyObject *const_cache = PyDict_New();
    if (const_cache == NULL) {
        return NULL;
    }

    g = _PyCfg_FromInstructionSequence((instr_sequence*)seq);
    if (g == NULL) {
        goto error;
    }

    if (_PyCfg_JumpLabelsToTargets(g) < 0) {
        goto error;
    }

    int code_flags = 0;
    int stackdepth, nlocalsplus;
    if (_PyCfg_OptimizedCfgToRegCfg(g, umd, code_flags, &stackdepth, &nlocalsplus,
                                    &regcfg) < 0) {
        goto error;
    }

    PyObject *consts = consts_dict_keys_inorder(umd->u_consts);
    if (consts == NULL) {
        goto error;
    }
    co = _PyAssemble_MakeCodeObject(umd, const_cache, consts, regcfg,
                                    nlocalsplus, code_flags, filename);
    Py_DECREF(consts);

error:
    Py_DECREF(const_cache);
    _PyCfgBuilder_Free(g);
    _PyRegCfg_Free(regcfg);
    return co;
}

/* Retained for API compatibility.
 * Optimization is now done in _PyCfg_OptimizeCodeUnit */

PyObject *
PyCode_Optimize(PyObject *code, PyObject* Py_UNUSED(consts),
                PyObject *Py_UNUSED(names), PyObject *Py_UNUSED(lnotab_obj))
{
    return Py_NewRef(code);
}
