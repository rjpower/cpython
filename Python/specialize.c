#include "Python.h"

#include "opcode.h"

#include "pycore_bytesobject.h"   // _PyBytes_Concat
#include "pycore_call.h"          // _Py_METH_CALL_FLAGS
#include "pycore_code.h"
#include "pycore_critical_section.h"
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_dict.h"          // DICT_KEYS_UNICODE
#include "pycore_function.h"      // _PyFunction_GetVersionForCurrentState()
#include "pycore_interpframe.h"   // FRAME_SPECIALS_SIZE
#include "pycore_lazyimportobject.h" // PyLazyImport_CheckExact
#include "pycore_list.h"          // _PyListIterObject, _PyList_Concat
#include "pycore_tuple.h"         // _PyTuple_Concat
#include "pycore_long.h"          // _PyLong_IsNonNegativeCompact()
#include "pycore_lookup.h"        // _PyShape_LookupPlan()
#include "pycore_moduleobject.h"
#include "pycore_object.h"
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_opcode_utils.h"  // RESUME_AT_FUNC_START
#include "pycore_pylifecycle.h"   // _PyOS_URandomNonblock()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_unicodeobject.h" // _PyUnicodeASCIIIter_Type

#include <stdlib.h> // rand()

/* For guidance on adding or extending families of instructions see
 * InternalDocs/interpreter.md `Specialization` section.
 */

#if Py_STATS
#define SPECIALIZATION_FAIL(opcode, kind) \
do { \
    PyStats *s = _PyStats_GET(); \
    if (s) { \
        int _kind = (kind); \
        assert(_kind < SPECIALIZATION_FAILURE_KINDS); \
        s->opcode_stats[opcode].specialization.failure_kinds[_kind]++; \
    } \
} while (0)
#else
#  define SPECIALIZATION_FAIL(opcode, kind) ((void)0)
#endif  // Py_STATS


#define SIMPLE_FUNCTION 0

/* Common */

#define SPEC_FAIL_OTHER 0
#define SPEC_FAIL_NO_DICT 1
#define SPEC_FAIL_OVERRIDDEN 2
#define SPEC_FAIL_OUT_OF_VERSIONS 3
#define SPEC_FAIL_OUT_OF_RANGE 4
#define SPEC_FAIL_EXPECTED_ERROR 5
#define SPEC_FAIL_WRONG_NUMBER_ARGUMENTS 6
#define SPEC_FAIL_CODE_COMPLEX_PARAMETERS 7
#define SPEC_FAIL_CODE_NOT_OPTIMIZED 8


#define SPEC_FAIL_LOAD_GLOBAL_NON_DICT 17
#define SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT 18

/* Super */

#define SPEC_FAIL_SUPER_BAD_CLASS 9
#define SPEC_FAIL_SUPER_SHADOWED 10

/* Attributes */

#define SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR 9
#define SPEC_FAIL_ATTR_NON_OVERRIDING_DESCRIPTOR 10
#define SPEC_FAIL_ATTR_NOT_DESCRIPTOR 11
#define SPEC_FAIL_ATTR_METHOD 12
#define SPEC_FAIL_ATTR_MUTABLE_CLASS 13
#define SPEC_FAIL_ATTR_PROPERTY 14
#define SPEC_FAIL_ATTR_NON_OBJECT_SLOT 15
#define SPEC_FAIL_ATTR_READ_ONLY 16
#define SPEC_FAIL_ATTR_AUDITED_SLOT 17
#define SPEC_FAIL_ATTR_NOT_MANAGED_DICT 18
#define SPEC_FAIL_ATTR_NON_STRING 19
#define SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND 20
#define SPEC_FAIL_ATTR_SHADOWED 21
#define SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD 22
#define SPEC_FAIL_ATTR_CLASS_METHOD_OBJ 23
#define SPEC_FAIL_ATTR_OBJECT_SLOT 24
#define SPEC_FAIL_ATTR_MODULE_LAZY_VALUE 25

#define SPEC_FAIL_ATTR_INSTANCE_ATTRIBUTE 26
#define SPEC_FAIL_ATTR_METACLASS_ATTRIBUTE 27
#define SPEC_FAIL_ATTR_PROPERTY_NOT_PY_FUNCTION 28
#define SPEC_FAIL_ATTR_OTHER_SHAPE 29
#define SPEC_FAIL_ATTR_NOT_IN_DICT 30
#define SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE 31
#define SPEC_FAIL_ATTR_CLASS_ATTR_DESCRIPTOR 32
#define SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD_OBJ 33
#define SPEC_FAIL_ATTR_METACLASS_OVERRIDDEN 34
#define SPEC_FAIL_ATTR_SPLIT_DICT 35
#define SPEC_FAIL_ATTR_DESCR_NOT_DEFERRED 36
#define SPEC_FAIL_ATTR_SLOT_AFTER_ITEMS 37

/* Binary subscr and store subscr */

#define SPEC_FAIL_SUBSCR_ARRAY_INT 9
#define SPEC_FAIL_SUBSCR_ARRAY_SLICE 10
#define SPEC_FAIL_SUBSCR_LIST_SLICE 11
#define SPEC_FAIL_SUBSCR_BUFFER_INT 12
#define SPEC_FAIL_SUBSCR_BUFFER_SLICE 13

/* Store subscr */
#define SPEC_FAIL_SUBSCR_BYTEARRAY_INT 18
#define SPEC_FAIL_SUBSCR_BYTEARRAY_SLICE 19
#define SPEC_FAIL_SUBSCR_PY_SIMPLE 20
#define SPEC_FAIL_SUBSCR_PY_OTHER 21
#define SPEC_FAIL_SUBSCR_DICT_SUBCLASS_NO_OVERRIDE 22
#define SPEC_FAIL_SUBSCR_NOT_HEAP_TYPE 23

/* Binary op */

#define SPEC_FAIL_BINARY_OP_ADD_DIFFERENT_TYPES          9
#define SPEC_FAIL_BINARY_OP_ADD_OTHER                   10
#define SPEC_FAIL_BINARY_OP_AND_DIFFERENT_TYPES         11
#define SPEC_FAIL_BINARY_OP_AND_INT                     12
#define SPEC_FAIL_BINARY_OP_AND_OTHER                   13
#define SPEC_FAIL_BINARY_OP_FLOOR_DIVIDE                14
#define SPEC_FAIL_BINARY_OP_LSHIFT                      15
#define SPEC_FAIL_BINARY_OP_MATRIX_MULTIPLY             16
#define SPEC_FAIL_BINARY_OP_MULTIPLY_DIFFERENT_TYPES    17
#define SPEC_FAIL_BINARY_OP_MULTIPLY_OTHER              18
#define SPEC_FAIL_BINARY_OP_OR                          19
#define SPEC_FAIL_BINARY_OP_POWER                       20
#define SPEC_FAIL_BINARY_OP_REMAINDER                   21
#define SPEC_FAIL_BINARY_OP_RSHIFT                      22
#define SPEC_FAIL_BINARY_OP_SUBTRACT_DIFFERENT_TYPES    23
#define SPEC_FAIL_BINARY_OP_SUBTRACT_OTHER              24
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_DIFFERENT_TYPES 25
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_FLOAT           26
#define SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_OTHER           27
#define SPEC_FAIL_BINARY_OP_XOR                         28
#define SPEC_FAIL_BINARY_OP_OR_INT                      29
#define SPEC_FAIL_BINARY_OP_OR_DIFFERENT_TYPES          30
#define SPEC_FAIL_BINARY_OP_XOR_INT                     31
#define SPEC_FAIL_BINARY_OP_XOR_DIFFERENT_TYPES         32
#define SPEC_FAIL_BINARY_OP_SUBSCR                      33
#define SPEC_FAIL_BINARY_OP_SUBSCR_LIST_SLICE           34
#define SPEC_FAIL_BINARY_OP_SUBSCR_TUPLE_SLICE          35
#define SPEC_FAIL_BINARY_OP_SUBSCR_STRING_SLICE         36
#define SPEC_FAIL_BINARY_OP_SUBSCR_NOT_HEAP_TYPE        37
#define SPEC_FAIL_BINARY_OP_SUBSCR_OTHER_SLICE          38
#define SPEC_FAIL_BINARY_OP_SUBSCR_MAPPINGPROXY         39
#define SPEC_FAIL_BINARY_OP_SUBSCR_RE_MATCH             40
#define SPEC_FAIL_BINARY_OP_SUBSCR_ARRAY                41
#define SPEC_FAIL_BINARY_OP_SUBSCR_DEQUE                42
#define SPEC_FAIL_BINARY_OP_SUBSCR_ENUMDICT             43
#define SPEC_FAIL_BINARY_OP_SUBSCR_STACKSUMMARY         44
#define SPEC_FAIL_BINARY_OP_SUBSCR_DEFAULTDICT          45
#define SPEC_FAIL_BINARY_OP_SUBSCR_COUNTER              46
#define SPEC_FAIL_BINARY_OP_SUBSCR_ORDEREDDICT          47
#define SPEC_FAIL_BINARY_OP_SUBSCR_BYTES                48
#define SPEC_FAIL_BINARY_OP_SUBSCR_STRUCTTIME           49
#define SPEC_FAIL_BINARY_OP_SUBSCR_RANGE                50

/* Calls */

#define SPEC_FAIL_CALL_INSTANCE_METHOD 11
#define SPEC_FAIL_CALL_CMETHOD 12
#define SPEC_FAIL_CALL_CFUNC_VARARGS 13
#define SPEC_FAIL_CALL_CFUNC_VARARGS_KEYWORDS 14
#define SPEC_FAIL_CALL_CFUNC_NOARGS 15
#define SPEC_FAIL_CALL_CFUNC_METHOD_FASTCALL_KEYWORDS 16
#define SPEC_FAIL_CALL_METH_DESCR_VARARGS 17
#define SPEC_FAIL_CALL_METH_DESCR_VARARGS_KEYWORDS 18
#define SPEC_FAIL_CALL_METH_DESCR_METHOD_FASTCALL_KEYWORDS 19
#define SPEC_FAIL_CALL_BAD_CALL_FLAGS 20
#define SPEC_FAIL_CALL_INIT_NOT_PYTHON 21
#define SPEC_FAIL_CALL_PEP_523 22
#define SPEC_FAIL_CALL_BOUND_METHOD 23
#define SPEC_FAIL_CALL_VECTORCALL 24
#define SPEC_FAIL_CALL_CLASS_MUTABLE 26
#define SPEC_FAIL_CALL_METHOD_WRAPPER 28
#define SPEC_FAIL_CALL_OPERATOR_WRAPPER 29
#define SPEC_FAIL_CALL_INIT_NOT_SIMPLE 30
#define SPEC_FAIL_CALL_METACLASS 31
#define SPEC_FAIL_CALL_INIT_NOT_INLINE_VALUES 32

/* COMPARE_OP */
#define SPEC_FAIL_COMPARE_OP_DIFFERENT_TYPES 12
#define SPEC_FAIL_COMPARE_OP_STRING 13
#define SPEC_FAIL_COMPARE_OP_BIG_INT 14
#define SPEC_FAIL_COMPARE_OP_BYTES 15
#define SPEC_FAIL_COMPARE_OP_TUPLE 16
#define SPEC_FAIL_COMPARE_OP_LIST 17
#define SPEC_FAIL_COMPARE_OP_SET 18
#define SPEC_FAIL_COMPARE_OP_BOOL 19
#define SPEC_FAIL_COMPARE_OP_BASEOBJECT 20
#define SPEC_FAIL_COMPARE_OP_FLOAT_LONG 21
#define SPEC_FAIL_COMPARE_OP_LONG_FLOAT 22

/* FOR_ITER and SEND */
#define SPEC_FAIL_ITER_GENERATOR 10
#define SPEC_FAIL_ITER_COROUTINE 11
#define SPEC_FAIL_ITER_ASYNC_GENERATOR 12
#define SPEC_FAIL_ITER_LIST 13
#define SPEC_FAIL_ITER_TUPLE 14
#define SPEC_FAIL_ITER_SET 15
#define SPEC_FAIL_ITER_STRING 16
#define SPEC_FAIL_ITER_BYTES 17
#define SPEC_FAIL_ITER_RANGE 18
#define SPEC_FAIL_ITER_ITERTOOLS 19
#define SPEC_FAIL_ITER_DICT_KEYS 20
#define SPEC_FAIL_ITER_DICT_ITEMS 21
#define SPEC_FAIL_ITER_DICT_VALUES 22
#define SPEC_FAIL_ITER_ENUMERATE 23
#define SPEC_FAIL_ITER_MAP 24
#define SPEC_FAIL_ITER_ZIP 25
#define SPEC_FAIL_ITER_SEQ_ITER 26
#define SPEC_FAIL_ITER_REVERSED_LIST 27
#define SPEC_FAIL_ITER_CALLABLE 28
#define SPEC_FAIL_ITER_ASCII_STRING 29
#define SPEC_FAIL_ITER_ASYNC_GENERATOR_SEND 30
#define SPEC_FAIL_ITER_SELF 31

// UNPACK_SEQUENCE

#define SPEC_FAIL_UNPACK_SEQUENCE_ITERATOR 9
#define SPEC_FAIL_UNPACK_SEQUENCE_SEQUENCE 10

// TO_BOOL
#define SPEC_FAIL_TO_BOOL_BYTEARRAY    9
#define SPEC_FAIL_TO_BOOL_BYTES       10
#define SPEC_FAIL_TO_BOOL_DICT        11
#define SPEC_FAIL_TO_BOOL_FLOAT       12
#define SPEC_FAIL_TO_BOOL_MAPPING     13
#define SPEC_FAIL_TO_BOOL_MEMORY_VIEW 14
#define SPEC_FAIL_TO_BOOL_NUMBER      15
#define SPEC_FAIL_TO_BOOL_SEQUENCE    16
#define SPEC_FAIL_TO_BOOL_SET         17
#define SPEC_FAIL_TO_BOOL_TUPLE       18

// CONTAINS_OP
#define SPEC_FAIL_CONTAINS_OP_STR        9
#define SPEC_FAIL_CONTAINS_OP_TUPLE      10
#define SPEC_FAIL_CONTAINS_OP_LIST       11
#define SPEC_FAIL_CONTAINS_OP_USER_CLASS 12

/* Instruction access.  instr is the instruction being specialised: a word
 * of the executing copy of the current frame's code object. */

static inline _PyInterpreterFrame *
current_frame(void)
{
    return _PyThreadState_GET()->current_frame;
}

/* The opcode of the handler; for an IR-only handler, the instruction that
 * it stands for, as BINARY_OP for BINARY_OP_ADD. */
static inline int
instr_opcode(_PyInstr *instr)
{
    int handler = IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*instr));
    if (handler >= IR_FIRST_ONLY_HANDLER) {
        handler = _PyIR_FormatOf(handler)->shadow;
        assert(handler < IR_FIRST_ONLY_HANDLER);
    }
    return handler;
}

static inline int
instr_oparg(_PyInstr *instr)
{
    _PyInterpreterFrame *frame = current_frame();
    return _PyCode_InstrOparg(_PyFrame_GetCode(frame),
                              _PyFrame_InstrOffset(frame, instr));
}

static inline _PyFbHeader *
instr_fb(_PyInstr *instr)
{
    _PyInterpreterFrame *frame = current_frame();
    return _PyIR_InstrFeedback(_PyFrame_GetCode(frame), _PyFrame_Exec(frame),
                               instr);
}

/* Whether h is entry 0, which the sites past the IR_FIELD_MAX-th share and
 * which never specialises */
static inline int
is_shared_entry(_PyFbHeader *h)
{
    return (char *)h == _PyFrame_Exec(current_frame())->entries;
}

static inline void *
instr_entry(_PyInstr *instr)
{
    _PyFbHeader *h = instr_fb(instr);
    assert(h != NULL);
    return h;
}

static inline int
set_opcode(_PyInstr *instr, int opcode)
{
    _PyIR_SetHandler(instr, opcode);
    return 1;
}

static inline void
set_counter(_PyFbHeader *h, _Py_BackoffCounter value)
{
    FT_ATOMIC_STORE_UINT16_RELAXED(h->counter.value_and_backoff,
                                   value.value_and_backoff);
}

static inline _Py_BackoffCounter
load_counter(_PyFbHeader *h)
{
    _Py_BackoffCounter result = {
        .value_and_backoff =
            FT_ATOMIC_LOAD_UINT16_RELAXED(h->counter.value_and_backoff)};
    return result;
}

/* Polymorphic feedback.
 *
 * An entry describes the newest specialisation of its site.  When a
 * specialised site is about to be re-specialised, fb_begin() saves the
 * observation that the entry still describes in a poly table, and
 * specialize() then adds the new observation, up to FB_POLY_SIZE of them;
 * one more makes the site megamorphic.  fb_begin() also clears the entry
 * body, so the fields of a row are the ones its handler reads, plus the
 * operand versions recorded by fb_begin_unary() and friends, and zero
 * otherwise.  Poly tables belong to the feedback block of the current
 * frame's bytecode, since the specialiser always runs on behalf of that
 * frame. */

typedef struct {
    uint32_t type_version;
    uint32_t aux;
    uint32_t index;
    const void *descr;
    _PyShape *shape;
} fb_row;

static _PyFbBlock *
fb_block(_PyInstr *instr)
{
    _PyCodeExec *exec = _PyFrame_Exec(current_frame());
    assert(exec->words <= instr && instr < exec->words + exec->nwords);
    return exec->feedback;
}

static fb_row
fb_row_of(_PyFbHeader *h)
{
    fb_row row = {0};
    switch (h->kind) {
        case FB_UNARY:
        case FB_BRANCH:     // POP_JUMP_IF_FALSE and _TRUE
            row.type_version = ((_PyFbUnary *)h)->type_version;
            break;
        case FB_BINARY:
            row.type_version = ((_PyFbBinary *)h)->lhs_version;
            row.aux = ((_PyFbBinary *)h)->rhs_version;
            row.descr = ((_PyFbBinary *)h)->descr;
            break;
        case FB_ATTR:
            row.type_version = ((_PyFbAttr *)h)->type_version;
            row.aux = ((_PyFbAttr *)h)->dict_version;
            row.index = ((_PyFbAttr *)h)->index;
            row.descr = ((_PyFbAttr *)h)->descr;
            row.shape = ((_PyFbAttr *)h)->shape;
            break;
        case FB_CALL:
            row.type_version = ((_PyFbCall *)h)->func_version;
            row.aux = ((_PyFbCall *)h)->callable_type_version;
            break;
        default:
            Py_UNREACHABLE();
    }
    return row;
}

static void
fb_put_row(_PyFbPoly *poly, int i, fb_row row, int opcode)
{
    poly->type_version[i] = row.type_version;
    poly->aux[i] = row.aux;
    poly->index[i] = row.index;
    poly->descr[i] = row.descr;
    poly->shape[i] = row.shape;
    poly->handler[i] = (uint16_t)opcode;
}

/* Save the observation of a specialised MONO site in a new poly table.
 * Losing it to a failed allocation only loses feedback. */
static void
fb_save_mono(_PyInstr *instr, _PyFbHeader *h, int opcode)
{
    _PyFbBlock *block = fb_block(instr);
    uint32_t n = block->npoly;
    if ((n & (n - 1)) == 0) {
        size_t capacity = n ? 2 * (size_t)n : 1;
        _PyFbPoly *poly = _PyMem_NoScanRealloc(block->poly, capacity * sizeof(_PyFbPoly));
        if (poly == NULL) {
            return;
        }
        block->poly = poly;
    }
    memset(&block->poly[n], 0, sizeof(_PyFbPoly));
    fb_put_row(&block->poly[n], 0, fb_row_of(h), opcode);
    block->npoly = n + 1;
    h->poly = n + 1;
}

/* Called by each specialiser of a site that keeps poly rows, before it
 * rewrites the entry.  Every specialiser then replaces the opcode in memory,
 * through specialize(), unspecialize() or LOAD_ATTR's retry of an absent
 * attribute, so no specialised handler reads the cleared entry. */
static _PyFbHeader *
fb_begin(_PyInstr *instr)
{
    _PyFbHeader *h = instr_fb(instr);
    assert(h != NULL);
    uint8_t opcode = instr_opcode(instr);
    if (h->state == FB_MONO && h->poly == 0 && _PyOpcode_Deopt[opcode] != opcode) {
        fb_save_mono(instr, h, opcode);
    }
    // Constant sizes, so that the compiler inlines the stores
    switch (h->kind) {
        case FB_UNARY:
        case FB_BRANCH:     // POP_JUMP_IF_FALSE and _TRUE
            memset(h + 1, 0, sizeof(_PyFbUnary) - sizeof(_PyFbHeader));
            break;
        case FB_BINARY:
            memset(h + 1, 0, sizeof(_PyFbBinary) - sizeof(_PyFbHeader));
            break;
        case FB_ATTR:
            memset(h + 1, 0, sizeof(_PyFbAttr) - sizeof(_PyFbHeader));
            break;
        case FB_CALL:
            memset(h + 1, 0, sizeof(_PyFbCall) - sizeof(_PyFbHeader));
            break;
        default:
            Py_UNREACHABLE();
    }
    return h;
}

/* The operand types that UNARY, BINARY and CALL entries record, as type
 * versions, which are 0 for types that have none yet. */
static inline uint32_t
fb_type_version(PyTypeObject *tp)
{
    return FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag);
}

static void
fb_begin_unary(_PyInstr *instr, PyTypeObject *tp)
{
    _PyFbUnary *fb = (_PyFbUnary *)fb_begin(instr);
    fb->type_version = fb_type_version(tp);
}

static _PyFbBinary *
fb_begin_binary(_PyInstr *instr, PyTypeObject *lhs_type, PyTypeObject *rhs_type)
{
    _PyFbBinary *fb = (_PyFbBinary *)fb_begin(instr);
    fb->lhs_version = fb_type_version(lhs_type);
    fb->rhs_version = fb_type_version(rhs_type);
    return fb;
}

static void
fb_begin_call(_PyInstr *instr, PyTypeObject *callable_type)
{
    _PyFbCall *fb = (_PyFbCall *)fb_begin(instr);
    fb->callable_type_version = fb_type_version(callable_type);
}

/* Inline values

   An operand may be an inline int or float (see pycore_value.h). The
   specializer tests it through its PyValue, with PyValue_Type() and the
   inline-aware int checks, and fails where an int or float cannot be the
   operand, such as the owner of LOAD_ATTR, whose specialized forms guard on
   an object's type. BINARY_OP inspects objects deeply, so it boxes inline
   operands for the duration. */

/* Record the new specialisation OPCODE of a site in its poly table. */
static void
fb_record(_PyInstr *instr, _PyFbHeader *h, int opcode)
{
    if (h->poly == 0) {
        h->state = FB_MONO;
        return;
    }
    if (h->state == FB_MEGA) {
        return;
    }
    _PyFbPoly *poly = &fb_block(instr)->poly[h->poly - 1];
    fb_row row = fb_row_of(h);
    int i = 0;
    while (i < FB_POLY_SIZE && poly->handler[i] != 0 &&
           (poly->type_version[i] != row.type_version ||
            poly->aux[i] != row.aux || poly->shape[i] != row.shape ||
            poly->handler[i] != opcode)) {
        i++;
    }
    if (i == FB_POLY_SIZE) {
        h->state = FB_MEGA;
        return;
    }
    fb_put_row(poly, i, row, opcode);
    h->state = poly->handler[1] != 0 ? FB_POLY : FB_MONO;
}

static inline void
specialize(_PyInstr *instr, uint8_t specialized_opcode)
{
    assert(!PyErr_Occurred());
    if (!set_opcode(instr, specialized_opcode)) {
        STAT_INC(_PyOpcode_Deopt[specialized_opcode], failure);
        SPECIALIZATION_FAIL(_PyOpcode_Deopt[specialized_opcode],
                            SPEC_FAIL_OTHER);
        return;
    }
    STAT_INC(_PyOpcode_Deopt[specialized_opcode], success);
    _PyFbHeader *h = instr_fb(instr);
    if (h == NULL) {
        // RESUME specialises without an entry
        return;
    }
    assert(!is_shared_entry(h));
    set_counter(h, adaptive_counter_cooldown());
    fb_record(instr, h, specialized_opcode);
}

static inline void
unspecialize(_PyInstr *instr)
{
    assert(!PyErr_Occurred());
    STAT_INC(_PyOpcode_Deopt[instr_opcode(instr)], failure);
    // For BINARY_OP, the handler of the operation
    _PyInterpreterFrame *frame = current_frame();
    int generic = _PyCode_InstrHandler(_PyFrame_GetCode(frame),
                                       _PyFrame_InstrOffset(frame, instr));
    if (!set_opcode(instr, generic)) {
        SPECIALIZATION_FAIL(_PyOpcode_Deopt[instr_opcode(instr)], SPEC_FAIL_OTHER);
        return;
    }
    _PyFbHeader *h = instr_fb(instr);
    if (h != NULL) {
        assert(!is_shared_entry(h));
        set_counter(h, adaptive_counter_backoff(load_counter(h)));
    }
}

static int function_kind(PyCodeObject *code);
static bool function_check_args(PyObject *o, int expected_argcount, int opcode);
static uint32_t function_get_version(PyObject *o, int opcode);



static int
specialize_module_load_attr_lock_held(PyDictObject *dict, _PyInstr *instr, PyObject *name)
{
    _PyFbAttr *fb = instr_entry(instr);
    if (dict->ma_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NON_STRING);
        return -1;
    }
    PyValue value;
    Py_ssize_t index = _PyDict_LookupIndexAndValue(dict, name, &value);
    if (!PyValue_IsNull(value) && PyValue_Type(value) == &PyLazyImport_Type) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MODULE_LAZY_VALUE);
        return -1;
    }
    assert(index != DKIX_ERROR);
    if (index == DKIX_EMPTY) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND);
        return -1;
    }
    uint32_t keys_version = _PyDict_GetKeysVersionForCurrentState(
            _PyInterpreterState_GET(), dict);
    if (keys_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    fb->dict_version = keys_version;
    fb->index = (uint32_t)index;
    specialize(instr, LOAD_ATTR_MODULE);
    return 0;
}

static int
specialize_module_load_attr(
    PyObject *owner, _PyInstr *instr, PyObject *name)
{
    PyModuleObject *m = (PyModuleObject *)owner;
    assert((Py_TYPE(owner)->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);
    PyDictObject *dict = (PyDictObject *)m->md_dict;
    if (dict == NULL) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_NO_DICT);
        return -1;
    }
    int result;
    Py_BEGIN_CRITICAL_SECTION(dict);
    result = specialize_module_load_attr_lock_held(dict, instr, name);
    Py_END_CRITICAL_SECTION();
    return result;
}

/* Attribute specialization */

Py_NO_INLINE void
_Py_Specialize_LoadSuperAttr(PyValue global_super_st, PyValue cls_st, _PyInstr *instr, int load_method) {
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[LOAD_SUPER_ATTR] == INLINE_CACHE_ENTRIES_LOAD_SUPER_ATTR);
    fb_begin(instr);
    if (PyValue_IsInline(global_super_st) ||
        PyValue_ObjectBorrow(global_super_st) != (PyObject *)&PySuper_Type)
    {
        SPECIALIZATION_FAIL(LOAD_SUPER_ATTR, SPEC_FAIL_SUPER_SHADOWED);
        goto fail;
    }
    if (!PyType_FastSubclass(PyValue_Type(cls_st), Py_TPFLAGS_TYPE_SUBCLASS)) {
        SPECIALIZATION_FAIL(LOAD_SUPER_ATTR, SPEC_FAIL_SUPER_BAD_CLASS);
        goto fail;
    }
    uint8_t load_code = load_method ? LOAD_SUPER_ATTR_METHOD : LOAD_SUPER_ATTR_ATTR;
    specialize(instr, load_code);
    return;
fail:
    unspecialize(instr);
}

/* The hooks a type's tp_getattro may run around the generic protocol
 * (boil/design/09-object-protocol.md section 4.1).  A specialised handler
 * replaces the whole dispatch at a site, so the specialiser models the ones
 * it can: none, a __getattr__ (which only plans that cannot raise
 * AttributeError ignore), or a Python __getattribute__ without one. */
typedef struct {
    int overridden;             // a getattro the specialiser does not model
    PyObject *getattribute;     // a Python __getattribute__ (strong), or NULL
    unsigned int ga_version;    // the type version __getattribute__ was found under
    int has_getattr;
} hook_facts;

static void
lookup_hook_facts(PyTypeObject *type, hook_facts *facts)
{
    *facts = (hook_facts){0};
    getattrofunc getattro_slot = type->tp_getattro;
    if (getattro_slot == PyObject_GenericGetAttr) {
        return;
    }
    if (getattro_slot != _Py_slot_tp_getattr_hook &&
        getattro_slot != _Py_slot_tp_getattro)
    {
        facts->overridden = 1;
        return;
    }
    /* One or both of __getattribute__ or __getattr__ may have been
       overridden.  See typeobject.c for why these functions are special. */
    PyObject *getattribute = _PyType_LookupRefAndVersion(
        type, &_Py_ID(__getattribute__), &facts->ga_version);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    bool custom = getattribute != NULL &&
        getattribute != interp->callable_cache.object__getattribute__;
    facts->has_getattr = _PyType_Lookup(type, &_Py_ID(__getattr__)) != NULL;
    if (custom) {
        if (!facts->has_getattr && Py_IS_TYPE(getattribute, &PyFunction_Type)) {
            facts->getattribute = getattribute;
            return;
        }
        /* Potentially both __getattr__ and __getattribute__ are set.
           Too complicated */
        facts->overridden = 1;
    }
    Py_XDECREF(getattribute);
}

/* Whether the site specialised for another shape of the same type before,
 * which the poly table that fb_begin() filled records. */
static bool
fb_saw_other_shape(_PyInstr *instr, unsigned int tp_version, _PyShape *shape)
{
    _PyFbHeader *h = instr_fb(instr);
    if (h->poly == 0) {
        return false;
    }
    _PyFbPoly *poly = &fb_block(instr)->poly[h->poly - 1];
    for (int i = 0; i < FB_POLY_SIZE && poly->handler[i] != 0; i++) {
        int opcode = poly->handler[i];
        if ((opcode == LOAD_ATTR_SHAPE || opcode == LOAD_ATTR_METHOD_SHAPE ||
             opcode == LOAD_ATTR_NONDESCRIPTOR_SHAPE ||
             opcode == STORE_ATTR_SHAPE || opcode == STORE_ATTR_SHAPE_TRANSITION) &&
            poly->type_version[i] == tp_version && poly->shape[i] != shape)
        {
            return true;
        }
    }
    return false;
}

/* The shape fields of FB for HANDLER, guarded by OWNER's shape, or 0 if
 * the site saw another shape of the type.  Until a polymorphic handler
 * takes such sites, they stay generic; the declined observation is
 * recorded, with the fields the caller set, so that the next
 * specialisation on the first shape sees the second as well. */
static int
set_shape_guard(PyObject *owner, _PyInstr *instr, _PyFbAttr *fb,
                unsigned int tp_version, int base_op, int handler)
{
    _PyShape *shape = _PyObject_SHAPE(owner);
    fb->shape = shape;
    fb->type_version = tp_version;
    if (fb_saw_other_shape(instr, tp_version, shape)) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_OTHER_SHAPE);
        fb_record(instr, instr_fb(instr), handler);
        return 0;
    }
    return 1;
}

/* A DICT plan: WITH_HINT if OWNER's instance dict holds NAME.  1 if
 * specialised; 0 if the dict lacks NAME, so that the class attribute
 * (steps 3 to 5) is the result; -1 to fail. */
static int
specialize_dict_hint(PyObject *owner, _PyInstr *instr, PyObject *name,
                     unsigned int tp_version, int base_op, int hint_op)
{
    _PyFbAttr *fb = instr_entry(instr);
    PyTypeObject *type = Py_TYPE(owner);
    Py_ssize_t dictoffset;
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        // Slot 0 in dictionary mode.
        assert(_PyShape_IsDictMode(_PyObject_SHAPE(owner)));
        dictoffset = type->tp_basicsize;
    }
    else {
        dictoffset = type->tp_dictoffset;
    }
    if (dictoffset == 0 || dictoffset > INT32_MAX || dictoffset < INT32_MIN) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_OUT_OF_RANGE);
        return -1;
    }
    PyDictObject *dict = (PyDictObject *)_PyObject_DictAt(owner, (int32_t)dictoffset);
    if (dict == NULL || !PyDict_CheckExact(dict)) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_NO_DICT);
        return -1;
    }
    int res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    if (_PyDict_HasSplitTable(dict)) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_SPLIT_DICT);
        res = -1;
    }
    else {
        PyValue value;
        Py_ssize_t index = _PyDict_LookupIndexAndValue(dict, name, &value);
        if (index < 0 || PyValue_IsNull(value)) {
            res = 0;
        }
        else {
            fb->hint = (uint32_t)index;
            res = 1;
        }
    }
    Py_END_CRITICAL_SECTION();
    if (res <= 0) {
        return res;
    }
    // The shape guard: the dictionary-mode shape, or a root shape.
    fb->shape = _PyObject_SHAPE(owner);
    fb->type_version = tp_version;
    fb->dictoffset = (int32_t)dictoffset;
    specialize(instr, hint_op);
    return 1;
}

static int
specialize_attr_loadclassattr(PyObject *owner, _PyInstr *instr,
                              PyObject *name, PyValue value,
                              unsigned int tp_version, bool is_method);
static int specialize_class_load_attr(PyObject* owner, _PyInstr *instr, PyObject* name);

static int
specialize_getattribute_overridden(_PyInstr *instr, PyObject *getattribute,
                                   unsigned int tp_version)
{
    _PyFbAttr *fb = instr_entry(instr);
    assert(Py_IS_TYPE(getattribute, &PyFunction_Type));
    if (tp_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    if (!function_check_args(getattribute, 2, LOAD_ATTR)) {
        return -1;
    }
    if (instr_oparg(instr) & 1) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
        return -1;
    }
    uint32_t version = function_get_version(getattribute, LOAD_ATTR);
    if (version == 0) {
        return -1;
    }
    /* Don't specialize if PEP 523 is active */
    if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
        return -1;
    }
    fb->func_version = version;
    fb->getattribute = getattribute;  /* borrowed */
    fb->type_version = tp_version;
    specialize(instr, LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN);
    return 0;
}

/* Take the plan, write the entry, pick the handler (note 09 section 4.8).
 * The handlers are today's; which of them a plan may use is set out in
 * boil/design/work/6c-rung2.md. */
static int
specialize_instance_load_attr(PyObject* owner, _PyInstr *instr, PyObject* name)
{
    _PyFbAttr *fb = instr_entry(instr);
    PyTypeObject *type = Py_TYPE(owner);
    uint8_t oparg = instr_oparg(instr);

    hook_facts hooks;
    lookup_hook_facts(type, &hooks);
    if (hooks.overridden) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OVERRIDDEN);
        return -1;
    }
    if (hooks.getattribute != NULL) {
        int res = specialize_getattribute_overridden(instr, hooks.getattribute,
                                                     hooks.ga_version);
        Py_DECREF(hooks.getattribute);
        return res;
    }

    _PyLookupPlan plan;
    _PyShape_LookupPlan(_PyObject_SHAPE(owner), name, &plan);
    unsigned int tp_version = plan.type_version;
    if (tp_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    if (!(plan.flags & _Py_LOOKUP_CACHEABLE)) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
        return -1;
    }
    PyObject *descr = PyValue_IsObject(plan.value)
                      ? PyValue_ObjectBorrow(plan.value) : NULL;

    switch (plan.kind) {
        case _Py_LOOKUP_DATA_DESCR:
            if (plan.flags & _Py_LOOKUP_MEMBER) {
                // An unbound member deopts, so a __getattr__ is still called.
                fb->index = plan.offset;
                fb->type_version = tp_version;
                specialize(instr, LOAD_ATTR_SLOT);
                return 0;
            }
            SPECIALIZATION_FAIL(LOAD_ATTR,
                Py_IS_TYPE(descr, &PyMemberDescr_Type)
                    ? SPEC_FAIL_ATTR_NON_OBJECT_SLOT
                    : SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            return -1;
        case _Py_LOOKUP_PROPERTY:
        {
            if (hooks.has_getattr) {
                /* We can't detect at runtime whether an attribute exists
                   with property. So that means we may have to call
                   __getattr__. */
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OVERRIDDEN);
                return -1;
            }
            PyObject *fget = ((_PyPropertyObject *)descr)->prop_get;
            if (fget == NULL) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                return -1;
            }
            if (!Py_IS_TYPE(fget, &PyFunction_Type)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_PROPERTY_NOT_PY_FUNCTION);
                return -1;
            }
            if (!function_check_args(fget, 1, LOAD_ATTR)) {
                return -1;
            }
            if (oparg & 1) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
                return -1;
            }
            /* Don't specialize if PEP 523 is active */
            if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
                return -1;
            }
            uint32_t func_version = function_get_version(fget, LOAD_ATTR);
            if (func_version == 0) {
                return -1;
            }
            fb->type_version = tp_version;
            fb->func_version = func_version;
            // The property, whose getter the handler reads and guards.
            fb->descr = descr;  /* borrowed */
            specialize(instr, LOAD_ATTR_PROPERTY);
            return 0;
        }
        default:
            break;
    }

    // Instance storage first, then the class (precedence steps 2 to 5).
    if (plan.kind == _Py_LOOKUP_SLOT) {
        fb->offset = plan.offset;
        if (!set_shape_guard(owner, instr, fb, tp_version, LOAD_ATTR,
                             LOAD_ATTR_SHAPE)) {
            return -1;
        }
        specialize(instr, LOAD_ATTR_SHAPE);
        return 0;
    }
    int kind = plan.kind;
    if (kind == _Py_LOOKUP_DICT) {
        int res = specialize_dict_hint(owner, instr, name, tp_version,
                                       LOAD_ATTR, LOAD_ATTR_WITH_HINT);
        if (res != 0) {
            return res > 0 ? 0 : -1;
        }
        // The dict lacks the name, but nothing guards against its gaining
        // it, so a class attribute is not specialised through a dict.
        if (plan.fallback != _Py_LOOKUP_ABSENT) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
            return -1;
        }
        kind = _Py_LOOKUP_ABSENT;
    }
    switch (kind) {
        case _Py_LOOKUP_METHOD:
            if (oparg & 1) {
                return specialize_attr_loadclassattr(owner, instr, name,
                                                     plan.value, tp_version,
                                                     true)
                       ? 0 : -1;
            }
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
            return -1;
        case _Py_LOOKUP_DESCR:
            SPECIALIZATION_FAIL(LOAD_ATTR,
                Py_IS_TYPE(descr, &PyClassMethod_Type)
                    ? SPEC_FAIL_ATTR_CLASS_METHOD_OBJ
                    : SPEC_FAIL_ATTR_NON_OVERRIDING_DESCRIPTOR);
            return -1;
        case _Py_LOOKUP_CLASS_VALUE:
            if ((oparg & 1) == 0) {
                if (specialize_attr_loadclassattr(owner, instr, name,
                                                  plan.value, tp_version,
                                                  false)) {
                    return 0;
                }
            }
            return -1;
        default:
            assert(kind == _Py_LOOKUP_ABSENT);
            // Retry after the cooldown.  A specialised opcode in memory
            // would read the entry that fb_begin() cleared.
            set_opcode(instr, LOAD_ATTR);
            set_counter(&fb->h, adaptive_counter_cooldown());
            return 0;
    }
}

Py_NO_INLINE void
_Py_Specialize_LoadAttr(PyValue owner_st, _PyInstr *instr, PyObject *name)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[LOAD_ATTR] == INLINE_CACHE_ENTRIES_LOAD_ATTR);
    fb_begin(instr);
    if (PyValue_IsInline(owner_st)) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *owner = PyValue_ObjectBorrow(owner_st);
    PyTypeObject *type = Py_TYPE(owner);
    bool fail;
    if (!_PyType_IsReady(type)) {
        // We *might* not really need this check, but we inherited it from
        // PyObject_GenericGetAttr and friends... and this way we still do the
        // right thing if someone forgets to call PyType_Ready(type):
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
        fail = true;
    }
    else if (PyModule_CheckExact(owner)) {
        fail = specialize_module_load_attr(owner, instr, name);
    }
    else if (PyType_Check(owner)) {
        fail = specialize_class_load_attr(owner, instr, name);
    }
    else {
        fail = specialize_instance_load_attr(owner, instr, name);
    }

    if (fail) {
        unspecialize(instr);
    }
}

Py_NO_INLINE void
_Py_Specialize_StoreAttr(PyValue owner_st, _PyInstr *instr, PyObject *name)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[STORE_ATTR] == INLINE_CACHE_ENTRIES_STORE_ATTR);
    _PyFbAttr *fb = (_PyFbAttr *)fb_begin(instr);
    if (PyValue_IsInline(owner_st)) {
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *owner = PyValue_ObjectBorrow(owner_st);
    PyTypeObject *type = Py_TYPE(owner);
    if (!_PyType_IsReady(type)) {
        // We *might* not really need this check, but we inherited it from
        // PyObject_GenericSetAttr and friends... and this way we still do the
        // right thing if someone forgets to call PyType_Ready(type):
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OTHER);
        goto fail;
    }
    if (PyModule_CheckExact(owner)) {
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OVERRIDDEN);
        goto fail;
    }
    if (type->tp_setattro != PyObject_GenericSetAttr) {
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OVERRIDDEN);
        goto fail;
    }
    _PyStorePlan plan;
    _PyShape_LookupStorePlan(_PyObject_SHAPE(owner), name, _Py_STORE_SET, &plan);
    unsigned int tp_version = plan.type_version;
    if (tp_version == 0) {
        goto fail;
    }
    if (!(plan.flags & _Py_LOOKUP_CACHEABLE)) {
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
        goto fail;
    }
    PyObject *d = PyValue_IsObject(plan.value)
                  ? PyValue_ObjectBorrow(plan.value) : NULL;
    (void)d;  // read only by SPECIALIZATION_FAIL
    if (plan.outcome == _Py_STORE_DATA_DESCR) {
        if (plan.flags & _Py_LOOKUP_MEMBER) {
            fb->index = plan.offset;
            fb->type_version = tp_version;
            specialize(instr, STORE_ATTR_SLOT);
            goto success;
        }
        SPECIALIZATION_FAIL(STORE_ATTR,
            Py_IS_TYPE(d, &PyProperty_Type) ? SPEC_FAIL_ATTR_PROPERTY
            : Py_IS_TYPE(d, &PyMemberDescr_Type) ? SPEC_FAIL_ATTR_NON_OBJECT_SLOT
            : SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
        goto fail;
    }
    if (!PyValue_IsNull(plan.value)) {
        // A store that shadows a class attribute is not specialised.
        SPECIALIZATION_FAIL(STORE_ATTR,
            d == NULL ? SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE
            : Py_IS_TYPE(d, &PyClassMethod_Type) ? SPEC_FAIL_ATTR_CLASS_METHOD_OBJ
            : _PyType_HasFeature(Py_TYPE(d), Py_TPFLAGS_METHOD_DESCRIPTOR) ? SPEC_FAIL_ATTR_METHOD
            : Py_TYPE(d)->tp_descr_get ? SPEC_FAIL_ATTR_CLASS_ATTR_DESCRIPTOR
            : SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE);
        goto fail;
    }
    switch (plan.outcome) {
        case _Py_STORE_REPLACE:
            fb->offset = plan.offset;
            if (!set_shape_guard(owner, instr, fb, tp_version, STORE_ATTR,
                                 STORE_ATTR_SHAPE)) {
                goto fail;
            }
            specialize(instr, STORE_ATTR_SHAPE);
            goto success;
        case _Py_STORE_ADD:
        {
            fb->offset = plan.offset;
            if (!set_shape_guard(owner, instr, fb, tp_version, STORE_ATTR,
                                 STORE_ATTR_SHAPE_TRANSITION)) {
                goto fail;
            }
            _PyShape *shape_to = plan.shape_to;
            if (shape_to == NULL) {
                /* Prepare the edge now, so that the first instance through
                   a new transition can be specialised.  The generic store
                   that follows prepares again and reports a failure, so a
                   MemoryError here is the specialiser's own. */
                PyObject *key = name;
                _PyUnicode_InternMortal(_PyInterpreterState_GET(), &key);
                _PyShapePrepare prep = PyUnicode_CHECK_INTERNED(key)
                    ? _PyShape_PrepareAdd(_PyObject_SHAPE(owner), key, &shape_to)
                    : _Py_SHAPE_PREPARE_LIMIT;
                if (prep != _Py_SHAPE_PREPARE_OK) {
                    if (prep == _Py_SHAPE_PREPARE_NOMEMORY) {
                        PyErr_Clear();
                    }
                    SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                    goto fail;
                }
            }
            fb->shape_to = shape_to;
            specialize(instr, STORE_ATTR_SHAPE_TRANSITION);
            goto success;
        }
        case _Py_STORE_DICT:
            if (specialize_dict_hint(owner, instr, name, tp_version,
                                     STORE_ATTR, STORE_ATTR_WITH_HINT) > 0) {
                goto success;
            }
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_NOT_IN_DICT);
            goto fail;
        default:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OTHER);
            goto fail;
    }
fail:
    unspecialize(instr);
    return;
success:
    return;
}

#ifdef Py_STATS
static int
load_attr_fail_kind(int kind, PyValue value)
{
    PyObject *d = PyValue_IsObject(value) ? PyValue_ObjectBorrow(value) : NULL;
    switch (kind) {
        case _Py_LOOKUP_DATA_DESCR:
            return Py_IS_TYPE(d, &PyMemberDescr_Type)
                   ? SPEC_FAIL_ATTR_NON_OBJECT_SLOT
                   : SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR;
        case _Py_LOOKUP_PROPERTY:
            return SPEC_FAIL_ATTR_PROPERTY;
        case _Py_LOOKUP_METHOD:
            return SPEC_FAIL_ATTR_METHOD;
        case _Py_LOOKUP_DESCR:
            if (Py_IS_TYPE(d, &PyClassMethodDescr_Type)) {
                return SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD;
            }
            if (Py_IS_TYPE(d, &PyClassMethod_Type)) {
                return SPEC_FAIL_ATTR_CLASS_METHOD_OBJ;
            }
            return SPEC_FAIL_ATTR_NON_OVERRIDING_DESCRIPTOR;
        case _Py_LOOKUP_CLASS_VALUE:
            return SPEC_FAIL_ATTR_NOT_DESCRIPTOR;
        case _Py_LOOKUP_ABSENT:
            return SPEC_FAIL_EXPECTED_ERROR;
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS

/* A load from a class: type.__getattribute__ over the class's MRO, with
 * the metaclass's attribute first.  Only class values and functions, which
 * the class returns as they are, are specialised. */
static int
specialize_class_load_attr(PyObject *owner, _PyInstr *instr,
                             PyObject *name)
{
    assert(PyType_Check(owner));
    PyTypeObject *cls = (PyTypeObject *)owner;
    _PyFbAttr *fb = instr_entry(instr);
    if (Py_TYPE(cls)->tp_getattro != _Py_type_getattro) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METACLASS_OVERRIDDEN);
        return -1;
    }
    PyValue meta = PyValue_NULL;
    unsigned int meta_version = _PyType_LookupValueAndVersion(Py_TYPE(cls), name, &meta);
    int mutable;
    int metakind = _PyLookup_ClassifyLoad(meta, &mutable);
    if (mutable || !(metakind == _Py_LOOKUP_METHOD ||
                     metakind == _Py_LOOKUP_CLASS_VALUE ||
                     metakind == _Py_LOOKUP_DESCR ||
                     metakind == _Py_LOOKUP_ABSENT))
    {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METACLASS_ATTRIBUTE);
        return -1;
    }
    hook_facts hooks;
    lookup_hook_facts(cls, &hooks);
    if (hooks.overridden || hooks.getattribute != NULL) {
        Py_XDECREF(hooks.getattribute);
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OVERRIDDEN);
        return -1;
    }
    PyValue value = PyValue_NULL;
    unsigned int tp_version = _PyType_LookupValueAndVersion(cls, name, &value);
    if (tp_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    bool metaclass_check = false;
    if ((Py_TYPE(cls)->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) == 0) {
        metaclass_check = true;
        if (meta_version == 0) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
            return -1;
        }
    }
    int kind = _PyLookup_ClassifyLoad(value, &mutable);
    if (mutable) {
        // An enum member's type is the class, so the class's version
        // guards it, provided the member is not a descriptor.
        PyObject *d = PyValue_ObjectBorrow(value);
        if (Py_TYPE(d) != cls) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
            return -1;
        }
        if (kind != _Py_LOOKUP_CLASS_VALUE) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            return -1;
        }
    }
    if (kind != _Py_LOOKUP_METHOD && kind != _Py_LOOKUP_CLASS_VALUE) {
        SPECIALIZATION_FAIL(LOAD_ATTR, load_attr_fail_kind(kind, value));
        return -1;
    }
    // The value, not a box of it: the class holds the value, and a box of
    // an inline value that only the entry held would die.
    fb->attr_value = value;
    fb->class_version = tp_version;
    if (metaclass_check) {
        fb->type_version = meta_version;
        specialize(instr, LOAD_ATTR_CLASS_WITH_METACLASS_CHECK);
    }
    else {
        specialize(instr, LOAD_ATTR_CLASS);
    }
    return 0;
}

// Please collect stats carefully before and after modifying. A subtle change
// can cause a significant drop in cache hits. A possible test is
// python.exe -m test_typing test_re test_dis test_zlib.
static int
specialize_attr_loadclassattr(PyObject *owner, _PyInstr *instr,
                              PyObject *name, PyValue value,
                              unsigned int tp_version, bool is_method)
{
    _PyFbAttr *fb = instr_entry(instr);
    PyTypeObject *owner_cls = Py_TYPE(owner);

    assert(!PyValue_IsNull(value));

    /* `descr` is borrowed. This is safe for methods (even inherited ones from
    *  super classes!) as long as tp_version_tag is validated for two main reasons:
    *
    *  1. The class will always hold a reference to the method so it will
    *  usually not be GC-ed. Should it be deleted in Python, e.g.
    *  `del obj.meth`, tp_version_tag will be invalidated, because of reason 2.
    *
    *  2. The pre-existing type method cache (MCACHE) uses the same principles
    *  of caching a borrowed descriptor. The MCACHE infrastructure does all the
    *  heavy lifting for us. E.g. it invalidates tp_version_tag on any MRO
    *  modification, on any type object change along said MRO, etc. (see
    *  PyType_Modified usages in typeobject.c). The MCACHE has been
    *  working since Python 2.6 and it's battle-tested.
    */
    fb->type_version = tp_version;
    if (is_method) {
        fb->descr = PyValue_ObjectBorrow(value);
    }
    else {
        // As for LOAD_ATTR_CLASS, the value rather than a box of it
        fb->attr_value = value;
    }
    int opcode;
    if (owner_cls->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        // A DICT plan does not reach here, so the object is in shape mode,
        // and its shape proves that no slot shadows the class attribute.
        assert(!_PyShape_IsDictMode(_PyObject_SHAPE(owner)));
        assert(_PyShape_SlotOf(_PyObject_SHAPE(owner), name) < 0);
        opcode = is_method ? LOAD_ATTR_METHOD_SHAPE : LOAD_ATTR_NONDESCRIPTOR_SHAPE;
        if (!set_shape_guard(owner, instr, fb, tp_version, LOAD_ATTR, opcode)) {
            return 0;
        }
    }
    else if (owner_cls->tp_dictoffset == 0) {
        opcode = is_method ? LOAD_ATTR_METHOD_NO_DICT : LOAD_ATTR_NONDESCRIPTOR_NO_DICT;
    }
    else {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
        return 0;
    }
    specialize(instr, opcode);
    return 1;
}

static void
specialize_load_global_lock_held(
    PyObject *globals, PyObject *builtins,
    _PyInstr *instr, PyObject *name)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[LOAD_GLOBAL] == INLINE_CACHE_ENTRIES_LOAD_GLOBAL);
    _PyFbGlobal *fb = instr_entry(instr);
    assert(PyUnicode_CheckExact(name));
    if (!PyDict_CheckExact(globals)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_DICT);
        goto fail;
    }
    PyDictKeysObject * globals_keys = ((PyDictObject *)globals)->ma_keys;
    if (globals_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto fail;
    }
    PyValue value;
    Py_ssize_t index = _PyDict_LookupIndexAndValue((PyDictObject *)globals, name, &value);
    if (index == DKIX_ERROR) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_EXPECTED_ERROR);
        goto fail;
    }
    if (!PyValue_IsNull(value) && PyValue_Type(value) == &PyLazyImport_Type) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_ATTR_MODULE_LAZY_VALUE);
        goto fail;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (index != DKIX_EMPTY) {
        uint32_t keys_version = _PyDict_GetKeysVersionForCurrentState(
                interp, (PyDictObject*) globals);
        if (keys_version == 0) {
            SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
            goto fail;
        }
        PyDict_Watch(MODULE_WATCHER_ID, globals);
        fb->index = (uint32_t)index;
        fb->globals_version = keys_version;
        specialize(instr, LOAD_GLOBAL_MODULE);
        return;
    }
    if (!PyDict_CheckExact(builtins)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_DICT);
        goto fail;
    }
    PyDictKeysObject * builtin_keys = ((PyDictObject *)builtins)->ma_keys;
    if (builtin_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto fail;
    }
    index = _PyDict_LookupIndexAndValue((PyDictObject *)builtins, name, &value);
    if (index == DKIX_ERROR) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_EXPECTED_ERROR);
        goto fail;
    }
    if (!PyValue_IsNull(value) && PyValue_Type(value) == &PyLazyImport_Type) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_ATTR_MODULE_LAZY_VALUE);
        goto fail;
    }
    if (index == DKIX_EMPTY) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_EXPECTED_ERROR);
        goto fail;
    }
    uint32_t globals_version = _PyDict_GetKeysVersionForCurrentState(
            interp, (PyDictObject*) globals);
    if (globals_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto fail;
    }
    PyDict_Watch(MODULE_WATCHER_ID, globals);
    uint32_t builtins_version = _PyDict_GetKeysVersionForCurrentState(
            interp, (PyDictObject*) builtins);
    if (builtins_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto fail;
    }
    PyDict_Watch(MODULE_WATCHER_ID, builtins);
    fb->index = (uint32_t)index;
    fb->globals_version = globals_version;
    fb->builtins_version = builtins_version;
    specialize(instr, LOAD_GLOBAL_BUILTIN);
    return;
fail:
    unspecialize(instr);
}

Py_NO_INLINE void
_Py_Specialize_LoadGlobal(
    PyObject *globals, PyObject *builtins,
    _PyInstr *instr, PyObject *name)
{
    Py_BEGIN_CRITICAL_SECTION2(globals, builtins);
    specialize_load_global_lock_held(globals, builtins, instr, name);
    Py_END_CRITICAL_SECTION2();
}

static int
function_kind(PyCodeObject *code) {
    int flags = code->co_flags;
    if ((flags & (CO_VARKEYWORDS | CO_VARARGS)) || code->co_kwonlyargcount) {
        return SPEC_FAIL_CODE_COMPLEX_PARAMETERS;
    }
    if ((flags & CO_OPTIMIZED) == 0) {
        return SPEC_FAIL_CODE_NOT_OPTIMIZED;
    }
    // The specialisations of simple functions push the callee's frame
    // inline, where allocating its executing copy could not fail
    if (_PyCode_GetExec(code) == NULL) {
        PyErr_Clear();
        return SPEC_FAIL_OTHER;
    }
    return SIMPLE_FUNCTION;
}

/* Returning false indicates a failure. */
static bool
function_check_args(PyObject *o, int expected_argcount, int opcode)
{
    assert(Py_IS_TYPE(o, &PyFunction_Type));
    PyFunctionObject *func = (PyFunctionObject *)o;
    PyCodeObject *fcode = (PyCodeObject *)func->func_code;
    int kind = function_kind(fcode);
    if (kind != SIMPLE_FUNCTION) {
        SPECIALIZATION_FAIL(opcode, kind);
        return false;
    }
    if (fcode->co_argcount != expected_argcount) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
        return false;
    }
    return true;
}

/* Returning 0 indicates a failure. */
static uint32_t
function_get_version(PyObject *o, int opcode)
{
    assert(Py_IS_TYPE(o, &PyFunction_Type));
    PyFunctionObject *func = (PyFunctionObject *)o;
    uint32_t version = _PyFunction_GetVersionForCurrentState(func);
    if (!_PyFunction_IsVersionValid(version)) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_OUT_OF_VERSIONS);
        return 0;
    }
    return version;
}

#ifdef Py_STATS
static int
store_subscr_fail_kind(PyObject *container, PyValue sub)
{
    PyTypeObject *container_type = Py_TYPE(container);
    bool sub_is_int = (PyValue_Type(sub) == &PyLong_Type);
    bool sub_is_slice = PyValue_Type(sub) == &PySlice_Type;
    PyMappingMethods *as_mapping = container_type->tp_as_mapping;
    if (as_mapping && (as_mapping->mp_ass_subscript
                       == PyDict_Type.tp_as_mapping->mp_ass_subscript)) {
        return SPEC_FAIL_SUBSCR_DICT_SUBCLASS_NO_OVERRIDE;
    }
    if (PyObject_CheckBuffer(container)) {
        if (sub_is_int &&
            !(_PyValue_IsCompactInt(sub) && _PyValue_CompactInt(sub) >= 0))
        {
            return SPEC_FAIL_OUT_OF_RANGE;
        }
        else if (strcmp(container_type->tp_name, "array.array") == 0) {
            if (sub_is_int) {
                return SPEC_FAIL_SUBSCR_ARRAY_INT;
            }
            else if (sub_is_slice) {
                return SPEC_FAIL_SUBSCR_ARRAY_SLICE;
            }
            else {
                return SPEC_FAIL_OTHER;
            }
        }
        else if (PyByteArray_CheckExact(container)) {
            if (sub_is_int) {
                return SPEC_FAIL_SUBSCR_BYTEARRAY_INT;
            }
            else if (sub_is_slice) {
                return SPEC_FAIL_SUBSCR_BYTEARRAY_SLICE;
            }
            else {
                return SPEC_FAIL_OTHER;
            }
        }
        else {
            if (sub_is_int) {
                return SPEC_FAIL_SUBSCR_BUFFER_INT;
            }
            else if (sub_is_slice) {
                return SPEC_FAIL_SUBSCR_BUFFER_SLICE;
            }
            else {
                return SPEC_FAIL_OTHER;
            }
        }
        return SPEC_FAIL_OTHER;
    }
    PyObject *descriptor = _PyType_Lookup(container_type, &_Py_ID(__setitem__));
    if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
        PyFunctionObject *func = (PyFunctionObject *)descriptor;
        PyCodeObject *code = (PyCodeObject *)func->func_code;
        int kind = function_kind(code);
        if (kind == SIMPLE_FUNCTION) {
            return SPEC_FAIL_SUBSCR_PY_SIMPLE;
        }
        else {
            return SPEC_FAIL_SUBSCR_PY_OTHER;
        }
    }
    return SPEC_FAIL_OTHER;
}
#endif

Py_NO_INLINE void
_Py_Specialize_StoreSubscr(PyValue container_st, PyValue sub_st, _PyInstr *instr)
{
    assert(ENABLE_SPECIALIZATION);
    fb_begin_binary(instr, PyValue_Type(container_st), PyValue_Type(sub_st));
    if (PyValue_IsInline(container_st)) {
        SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *container = PyValue_ObjectBorrow(container_st);
    PyTypeObject *container_type = Py_TYPE(container);
    if (container_type == &PyList_Type) {
        if (PyValue_Type(sub_st) == &PyLong_Type) {
            if (_PyValue_IsCompactInt(sub_st)
                && _PyValue_CompactInt(sub_st) >= 0
                && _PyValue_CompactInt(sub_st) < PyList_GET_SIZE(container))
            {
                specialize(instr, STORE_SUBSCR_LIST_INT);
                return;
            }
            else {
                SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OUT_OF_RANGE);
                unspecialize(instr);
                return;
            }
        }
        else if (PyValue_Type(sub_st) == &PySlice_Type) {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_SUBSCR_LIST_SLICE);
            unspecialize(instr);
            return;
        }
        else {
            SPECIALIZATION_FAIL(STORE_SUBSCR, SPEC_FAIL_OTHER);
            unspecialize(instr);
            return;
        }
    }
    if (container_type->tp_as_mapping != NULL &&
        container_type->tp_as_mapping->mp_ass_subscript == _PyDict_StoreSubscript)
    {
        specialize(instr, STORE_SUBSCR_DICT);
        return;
    }
    SPECIALIZATION_FAIL(STORE_SUBSCR, store_subscr_fail_kind(container, sub_st));
    unspecialize(instr);
}

/* Returns a strong reference. */
static PyObject *
get_init_for_simple_managed_python_class(PyTypeObject *tp, unsigned int *tp_version)
{
    assert(tp->tp_new == PyBaseObject_Type.tp_new);
    if (tp->tp_alloc != PyType_GenericAlloc) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OVERRIDDEN);
        return NULL;
    }
    unsigned long tp_flags = PyType_GetFlags(tp);
    if (!(tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        /* Is this possible? */
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_EXPECTED_ERROR);
        return NULL;
    }
    PyObject *init = _PyType_LookupRefAndVersion(tp, &_Py_ID(__init__), tp_version);
    if (init == NULL || !PyFunction_Check(init)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_INIT_NOT_PYTHON);
        Py_XDECREF(init);
        return NULL;
    }
    int kind = function_kind((PyCodeObject *)PyFunction_GET_CODE(init));
    if (kind != SIMPLE_FUNCTION) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_INIT_NOT_SIMPLE);
        Py_DECREF(init);
        return NULL;
    }
    return init;
}

static int
specialize_class_call(PyObject *callable, _PyInstr *instr, int nargs)
{
    assert(PyType_Check(callable));
    PyTypeObject *tp = _PyType_CAST(callable);
    if (tp->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) {
        int oparg = instr_oparg(instr);
        if (nargs == 1 && oparg == 1) {
            if (tp == &PyUnicode_Type) {
                specialize(instr, CALL_STR_1);
                return 0;
            }
            else if (tp == &PyType_Type) {
                specialize(instr, CALL_TYPE_1);
                return 0;
            }
            else if (tp == &PyTuple_Type) {
                specialize(instr, CALL_TUPLE_1);
                return 0;
            }
        }
        if (tp->tp_vectorcall != NULL) {
            specialize(instr, CALL_BUILTIN_CLASS);
            return 0;
        }
        goto generic;
    }
    if (Py_TYPE(tp) != &PyType_Type) {
        goto generic;
    }
    if (tp->tp_new == PyBaseObject_Type.tp_new) {
        unsigned int tp_version = 0;
        PyObject *init = get_init_for_simple_managed_python_class(tp, &tp_version);
        if (!tp_version) {
            SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
            Py_XDECREF(init);
            return -1;
        }
        if (init != NULL && _PyType_CacheInitForSpecialization(
                                (PyHeapTypeObject *)tp, init, tp_version)) {
            ((_PyFbCall *)instr_entry(instr))->type_version = tp_version;
            specialize(instr, CALL_ALLOC_AND_ENTER_INIT);
            Py_DECREF(init);
            return 0;
        }
        Py_XDECREF(init);
    }
generic:
    specialize(instr, CALL_NON_PY_GENERAL);
    return 0;
}

static int
specialize_method_descriptor(PyMethodDescrObject *descr, PyValue self_or_null,
                             _PyInstr *instr, int nargs)
{
    switch (descr->d_method->ml_flags & _Py_METH_CALL_FLAGS) {
        case METH_NOARGS: {
            if (nargs != 1) {
                SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return -1;
            }
            specialize(instr, CALL_METHOD_DESCRIPTOR_NOARGS);
            return 0;
        }
        case METH_O: {
            if (nargs != 2) {
                SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return -1;
            }
            PyInterpreterState *interp = _PyInterpreterState_GET();
            PyObject *list_append = interp->callable_cache.list_append;
            int oparg = instr_oparg(instr);
            if ((PyObject *)descr == list_append && oparg == 1) {
                assert(!PyValue_IsNull(self_or_null));
                if (PyValue_Type(self_or_null) == &PyList_Type) {
                    specialize(instr, CALL_LIST_APPEND);
                    return 0;
                }
            }
            specialize(instr, CALL_METHOD_DESCRIPTOR_O);
            return 0;
        }
        case METH_FASTCALL: {
            specialize(instr, CALL_METHOD_DESCRIPTOR_FAST);
            return 0;
        }
        case METH_FASTCALL | METH_KEYWORDS: {
            specialize(instr, CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS);
            return 0;
        }
    }
    specialize(instr, CALL_NON_PY_GENERAL);
    return 0;
}

static int
specialize_py_call(PyFunctionObject *func, _PyInstr *instr, int nargs,
                   bool bound_method)
{
    _PyFbCall *fb = instr_entry(instr);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int kind = function_kind(code);
    /* Don't specialize if PEP 523 is active */
    if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_PEP_523);
        return -1;
    }
    if (func->vectorcall != _PyFunction_Vectorcall) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_VECTORCALL);
        return -1;
    }
    int argcount = -1;
    if (kind == SPEC_FAIL_CODE_NOT_OPTIMIZED) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CODE_NOT_OPTIMIZED);
        return -1;
    }
    if (kind == SIMPLE_FUNCTION) {
        argcount = code->co_argcount;
    }
    int version = _PyFunction_GetVersionForCurrentState(func);
    if (!_PyFunction_IsVersionValid(version)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    fb->func_version = version;
    uint8_t opcode;
    if (argcount == nargs + bound_method) {
        opcode =
            bound_method ? CALL_BOUND_METHOD_EXACT_ARGS : CALL_PY_EXACT_ARGS;
    }
    else {
        opcode = bound_method ? CALL_BOUND_METHOD_GENERAL : CALL_PY_GENERAL;
    }
    specialize(instr, opcode);
    return 0;
}


static int
specialize_py_call_kw(PyFunctionObject *func, _PyInstr *instr, int nargs,
                   bool bound_method)
{
    _PyFbCall *fb = instr_entry(instr);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    int kind = function_kind(code);
    /* Don't specialize if PEP 523 is active */
    if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_PEP_523);
        return -1;
    }
    if (func->vectorcall != _PyFunction_Vectorcall) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_VECTORCALL);
        return -1;
    }
    if (kind == SPEC_FAIL_CODE_NOT_OPTIMIZED) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CODE_NOT_OPTIMIZED);
        return -1;
    }
    int version = _PyFunction_GetVersionForCurrentState(func);
    if (!_PyFunction_IsVersionValid(version)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    fb->func_version = version;
    specialize(instr, bound_method ? CALL_KW_BOUND_METHOD : CALL_KW_PY);
    return 0;
}

static int
specialize_c_call(PyObject *callable, _PyInstr *instr, int nargs)
{
    if (PyCFunction_GET_FUNCTION(callable) == NULL) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OTHER);
        return 1;
    }
    switch (PyCFunction_GET_FLAGS(callable) & _Py_METH_CALL_FLAGS) {
        case METH_O: {
            if (nargs != 1) {
                SPECIALIZATION_FAIL(CALL, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
                return 1;
            }
            /* len(o) */
            PyInterpreterState *interp = _PyInterpreterState_GET();
            if (callable == interp->callable_cache.len && instr_oparg(instr) == 1) {
                specialize(instr, CALL_LEN);
                return 0;
            }
            specialize(instr, CALL_BUILTIN_O);
            return 0;
        }
        case METH_FASTCALL: {
            if (nargs == 2) {
                /* isinstance(o1, o2) */
                PyInterpreterState *interp = _PyInterpreterState_GET();
                if (callable == interp->callable_cache.isinstance && instr_oparg(instr) == 2) {
                    specialize(instr, CALL_ISINSTANCE);
                    return 0;
                }
            }
            specialize(instr, CALL_BUILTIN_FAST);
            return 0;
        }
        case METH_FASTCALL | METH_KEYWORDS: {
            specialize(instr, CALL_BUILTIN_FAST_WITH_KEYWORDS);
            return 0;
        }
        default:
            specialize(instr, CALL_NON_PY_GENERAL);
            return 0;
    }
}

Py_NO_INLINE void
_Py_Specialize_Call(PyValue callable_st, PyValue self_or_null_st, _PyInstr *instr, int nargs)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[CALL] == INLINE_CACHE_ENTRIES_CALL);
    assert(instr_opcode(instr) != INSTRUMENTED_CALL);
    fb_begin_call(instr, PyValue_Type(callable_st));
    if (PyValue_IsInline(callable_st)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *callable = PyValue_ObjectBorrow(callable_st);
    int fail;
    if (PyCFunction_CheckExact(callable)) {
        fail = specialize_c_call(callable, instr, nargs);
    }
    else if (PyFunction_Check(callable)) {
        fail = specialize_py_call((PyFunctionObject *)callable, instr, nargs, false);
    }
    else if (PyType_Check(callable)) {
        fail = specialize_class_call(callable, instr, nargs);
    }
    else if (Py_IS_TYPE(callable, &PyMethodDescr_Type)) {
        fail = specialize_method_descriptor((PyMethodDescrObject *)callable,
                                            self_or_null_st, instr, nargs);
    }
    else if (PyMethod_Check(callable)) {
        PyObject *func = ((PyMethodObject *)callable)->im_func;
        if (PyFunction_Check(func)) {
            fail = specialize_py_call((PyFunctionObject *)func, instr, nargs, true);
        }
        else {
            SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_BOUND_METHOD);
            fail = -1;
        }
    }
    else {
        specialize(instr, CALL_NON_PY_GENERAL);
        fail = 0;
    }
    if (fail) {
        unspecialize(instr);
    }
}

Py_NO_INLINE void
_Py_Specialize_CallKw(PyValue callable_st, _PyInstr *instr, int nargs)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[CALL_KW] == INLINE_CACHE_ENTRIES_CALL_KW);
    assert(instr_opcode(instr) != INSTRUMENTED_CALL_KW);
    fb_begin_call(instr, PyValue_Type(callable_st));
    if (PyValue_IsInline(callable_st)) {
        SPECIALIZATION_FAIL(CALL_KW, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *callable = PyValue_ObjectBorrow(callable_st);
    int fail;
    if (PyFunction_Check(callable)) {
        fail = specialize_py_call_kw((PyFunctionObject *)callable, instr, nargs, false);
    }
    else if (PyMethod_Check(callable)) {
        PyObject *func = ((PyMethodObject *)callable)->im_func;
        if (PyFunction_Check(func)) {
            fail = specialize_py_call_kw((PyFunctionObject *)func, instr, nargs, true);
        }
        else {
            SPECIALIZATION_FAIL(CALL_KW, SPEC_FAIL_CALL_BOUND_METHOD);
            fail = -1;
        }
    }
    else {
        specialize(instr, CALL_KW_NON_PY);
        fail = 0;
    }
    if (fail) {
        unspecialize(instr);
    }
}

#ifdef Py_STATS
static int
binary_op_fail_kind(int oparg, PyValue lhs_st, PyValue rhs_st)
{
    PyTypeObject *lhs_type = PyValue_Type(lhs_st);
    PyTypeObject *rhs_type = PyValue_Type(rhs_st);
    // rhs is an int that is not a non-negative compact one
    bool rhs_other_int = rhs_type == &PyLong_Type &&
        !(_PyValue_IsCompactInt(rhs_st) && _PyValue_CompactInt(rhs_st) >= 0);
    switch (oparg) {
        case NB_ADD:
        case NB_INPLACE_ADD:
            if (lhs_type != rhs_type) {
                return SPEC_FAIL_BINARY_OP_ADD_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_ADD_OTHER;
        case NB_AND:
        case NB_INPLACE_AND:
            if (lhs_type != rhs_type) {
                return SPEC_FAIL_BINARY_OP_AND_DIFFERENT_TYPES;
            }
            if (lhs_type == &PyLong_Type) {
                return SPEC_FAIL_BINARY_OP_AND_INT;
            }
            return SPEC_FAIL_BINARY_OP_AND_OTHER;
        case NB_FLOOR_DIVIDE:
        case NB_INPLACE_FLOOR_DIVIDE:
            return SPEC_FAIL_BINARY_OP_FLOOR_DIVIDE;
        case NB_LSHIFT:
        case NB_INPLACE_LSHIFT:
            return SPEC_FAIL_BINARY_OP_LSHIFT;
        case NB_MATRIX_MULTIPLY:
        case NB_INPLACE_MATRIX_MULTIPLY:
            return SPEC_FAIL_BINARY_OP_MATRIX_MULTIPLY;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            if (lhs_type != rhs_type) {
                return SPEC_FAIL_BINARY_OP_MULTIPLY_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_MULTIPLY_OTHER;
        case NB_OR:
        case NB_INPLACE_OR:
            if (lhs_type != rhs_type) {
                return SPEC_FAIL_BINARY_OP_OR_DIFFERENT_TYPES;
            }
            if (lhs_type == &PyLong_Type) {
                return SPEC_FAIL_BINARY_OP_OR_INT;
            }
            return SPEC_FAIL_BINARY_OP_OR;
        case NB_POWER:
        case NB_INPLACE_POWER:
            return SPEC_FAIL_BINARY_OP_POWER;
        case NB_REMAINDER:
        case NB_INPLACE_REMAINDER:
            return SPEC_FAIL_BINARY_OP_REMAINDER;
        case NB_RSHIFT:
        case NB_INPLACE_RSHIFT:
            return SPEC_FAIL_BINARY_OP_RSHIFT;
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            if (lhs_type != rhs_type) {
                return SPEC_FAIL_BINARY_OP_SUBTRACT_DIFFERENT_TYPES;
            }
            return SPEC_FAIL_BINARY_OP_SUBTRACT_OTHER;
        case NB_TRUE_DIVIDE:
        case NB_INPLACE_TRUE_DIVIDE:
            if (lhs_type != rhs_type) {
                return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_DIFFERENT_TYPES;
            }
            if (lhs_type == &PyFloat_Type) {
                return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_FLOAT;
            }
            return SPEC_FAIL_BINARY_OP_TRUE_DIVIDE_OTHER;
        case NB_XOR:
        case NB_INPLACE_XOR:
            if (lhs_type != rhs_type) {
                return SPEC_FAIL_BINARY_OP_XOR_DIFFERENT_TYPES;
            }
            if (lhs_type == &PyLong_Type) {
                return SPEC_FAIL_BINARY_OP_XOR_INT;
            }
            return SPEC_FAIL_BINARY_OP_XOR;
        case NB_SUBSCR:
            if (lhs_type == &PyList_Type) {
                if (rhs_other_int) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (rhs_type == &PySlice_Type) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_LIST_SLICE;
                }
            }
            if (lhs_type == &PyTuple_Type) {
                if (rhs_other_int) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (rhs_type == &PySlice_Type) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_TUPLE_SLICE;
                }
            }
            if (lhs_type == &PyUnicode_Type) {
                if (rhs_other_int) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (rhs_type == &PySlice_Type) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_STRING_SLICE;
                }
            }
            unsigned int tp_version;
            PyTypeObject *container_type = lhs_type;
            PyObject *descriptor = _PyType_LookupRefAndVersion(container_type, &_Py_ID(__getitem__), &tp_version);
            if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
                if (!(container_type->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
                    Py_DECREF(descriptor);
                    return SPEC_FAIL_BINARY_OP_SUBSCR_NOT_HEAP_TYPE;
                }
                PyFunctionObject *func = (PyFunctionObject *)descriptor;
                PyCodeObject *fcode = (PyCodeObject *)func->func_code;
                int kind = function_kind(fcode);
                if (kind != SIMPLE_FUNCTION) {
                    Py_DECREF(descriptor);
                    return kind;
                }
                if (fcode->co_argcount != 2) {
                    Py_DECREF(descriptor);
                    return SPEC_FAIL_WRONG_NUMBER_ARGUMENTS;
                }

                if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
                    /* Don't specialize if PEP 523 is active */
                    Py_DECREF(descriptor);
                    return SPEC_FAIL_OTHER;
                }
            }
            Py_XDECREF(descriptor);

            if (PyType_IsSubtype(lhs_type, &PyDictProxy_Type)) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_MAPPINGPROXY;
            }

            if (PyType_IsSubtype(lhs_type, &PyBytes_Type)) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_BYTES;
            }

            if (PyType_IsSubtype(lhs_type, &PyRange_Type)) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_RANGE;
            }

            if (strcmp(container_type->tp_name, "array.array") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_ARRAY;
            }

            if (strcmp(container_type->tp_name, "re.Match") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_RE_MATCH;
            }

            if (strcmp(container_type->tp_name, "collections.deque") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_DEQUE;
            }

            if (strcmp(_PyType_Name(container_type), "EnumDict") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_ENUMDICT;
            }

            if (strcmp(container_type->tp_name, "StackSummary") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_STACKSUMMARY;
            }

            if (strcmp(container_type->tp_name, "collections.defaultdict") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_DEFAULTDICT;
            }

            if (strcmp(container_type->tp_name, "Counter") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_COUNTER;
            }

            if (strcmp(container_type->tp_name, "collections.OrderedDict") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_ORDEREDDICT;
            }

            if (strcmp(container_type->tp_name, "time.struct_time") == 0) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_STRUCTTIME;
            }

            if (rhs_type == &PySlice_Type) {
                return SPEC_FAIL_BINARY_OP_SUBSCR_OTHER_SLICE;
            }
            return SPEC_FAIL_BINARY_OP_SUBSCR;
    }
    Py_UNREACHABLE();
}
#endif

/** Binary Op Specialization Extensions */

/* The actions of descriptors whose operands are neither int nor float. */
#define OBJECT_ACTION(NAME, FUNC) \
    static PyObject * \
    (NAME)(PyValue lhs, PyValue rhs) \
    { \
        return FUNC(PyValue_ObjectBorrow(lhs), PyValue_ObjectBorrow(rhs)); \
    }
OBJECT_ACTION(list_concat,  _PyList_Concat)
OBJECT_ACTION(tuple_concat, _PyTuple_Concat)
OBJECT_ACTION(bytes_concat, _PyBytes_Concat)
OBJECT_ACTION(dict_or,      _PyDict_Or)
OBJECT_ACTION(dict_ior,     _PyDict_IOr)
#undef OBJECT_ACTION

// The repeat count of an exact int: inline, or an int object outside int32.
static Py_ssize_t
repeat_count(PyValue count)
{
    if (PyValue_IsInlineInt(count)) {
        return PyValue_Int32(count);
    }
    return PyLong_AsSsize_t(PyValue_ObjectBorrow(count));
}

#define SEQ_INT_MULTIPLY_ACTION(NAME, REPEAT, SEQ, COUNT) \
    static PyObject * \
    (NAME)(PyValue lhs, PyValue rhs) \
    { \
        Py_ssize_t count = repeat_count(COUNT); \
        if (count == -1 && PyErr_Occurred()) { \
            return NULL; \
        } \
        return REPEAT(PyValue_ObjectBorrow(SEQ), count); \
    }
SEQ_INT_MULTIPLY_ACTION(str_int_multiply,   _PyUnicode_Repeat, lhs, rhs)
SEQ_INT_MULTIPLY_ACTION(int_str_multiply,   _PyUnicode_Repeat, rhs, lhs)
SEQ_INT_MULTIPLY_ACTION(bytes_int_multiply, _PyBytes_Repeat,   lhs, rhs)
SEQ_INT_MULTIPLY_ACTION(int_bytes_multiply, _PyBytes_Repeat,   rhs, lhs)
SEQ_INT_MULTIPLY_ACTION(tuple_int_multiply, _PyTuple_Repeat,   lhs, rhs)
SEQ_INT_MULTIPLY_ACTION(int_tuple_multiply, _PyTuple_Repeat,   rhs, lhs)
#undef SEQ_INT_MULTIPLY_ACTION

static _PyBinaryOpSpecializationDescr binaryop_extend_descrs[] = {
    /* list-list concatenation: _PyList_Concat always allocates a new list */
    {NB_ADD, NULL, list_concat, &PyList_Type, 1, &PyList_Type, &PyList_Type},
    /* tuple-tuple concatenation: _PyTuple_Concat has a zero-length shortcut
       that can return one of the operands, so the result is not guaranteed
       to be a freshly allocated object. */
    {NB_ADD, NULL, tuple_concat, &PyTuple_Type, 0, &PyTuple_Type, &PyTuple_Type},

    /* str * int / int * str: call _PyUnicode_Repeat directly.
       _PyUnicode_Repeat returns the original when n == 1. */
    {NB_MULTIPLY, NULL, str_int_multiply, &PyUnicode_Type, 0, &PyUnicode_Type, &PyLong_Type},
    {NB_MULTIPLY, NULL, int_str_multiply, &PyUnicode_Type, 0, &PyLong_Type, &PyUnicode_Type},
    {NB_INPLACE_MULTIPLY, NULL, str_int_multiply, &PyUnicode_Type, 0, &PyUnicode_Type, &PyLong_Type},
    {NB_INPLACE_MULTIPLY, NULL, int_str_multiply, &PyUnicode_Type, 0, &PyLong_Type, &PyUnicode_Type},

    /* bytes + bytes: bytes_concat may return an operand when one side
       is empty, so result is not always unique. */
    {NB_ADD, NULL, bytes_concat, &PyBytes_Type, 0, &PyBytes_Type, &PyBytes_Type},
    {NB_INPLACE_ADD, NULL, bytes_concat, &PyBytes_Type, 0, &PyBytes_Type, &PyBytes_Type},

    /* bytes * int / int * bytes: call _PyBytes_Repeat directly.
       _PyBytes_Repeat returns the original when n == 1. */
    {NB_MULTIPLY, NULL, bytes_int_multiply, &PyBytes_Type, 0, &PyBytes_Type, &PyLong_Type},
    {NB_MULTIPLY, NULL, int_bytes_multiply, &PyBytes_Type, 0, &PyLong_Type, &PyBytes_Type},
    {NB_INPLACE_MULTIPLY, NULL, bytes_int_multiply, &PyBytes_Type, 0, &PyBytes_Type, &PyLong_Type},
    {NB_INPLACE_MULTIPLY, NULL, int_bytes_multiply, &PyBytes_Type, 0, &PyLong_Type, &PyBytes_Type},

    /* tuple * int / int * tuple: call _PyTuple_Repeat directly.
       _PyTuple_Repeat returns the original when n == 1. */
    {NB_MULTIPLY, NULL, tuple_int_multiply, &PyTuple_Type, 0, &PyTuple_Type, &PyLong_Type},
    {NB_MULTIPLY, NULL, int_tuple_multiply, &PyTuple_Type, 0, &PyLong_Type, &PyTuple_Type},
    {NB_INPLACE_MULTIPLY, NULL, tuple_int_multiply, &PyTuple_Type, 0, &PyTuple_Type, &PyLong_Type},
    {NB_INPLACE_MULTIPLY, NULL, int_tuple_multiply, &PyTuple_Type, 0, &PyLong_Type, &PyTuple_Type},

    /* dict | dict */
    {NB_OR, NULL, dict_or, &PyDict_Type, 1, &PyDict_Type, &PyDict_Type},
    {NB_INPLACE_OR, NULL, dict_ior, &PyDict_Type, 0, &PyDict_Type, &PyDict_Type},
};

static int
binary_op_extended_specialization(PyValue lhs, PyValue rhs, int oparg,
                                  _PyBinaryOpSpecializationDescr **descr)
{
    size_t n = sizeof(binaryop_extend_descrs)/sizeof(_PyBinaryOpSpecializationDescr);
    for (size_t i = 0; i < n; i++) {
        _PyBinaryOpSpecializationDescr *d = &binaryop_extend_descrs[i];
        if (d->oparg != oparg) {
            continue;
        }
        int match = (d->guard != NULL)
            ? d->guard(lhs, rhs)
            : (PyValue_Type(lhs) == d->lhs_type && PyValue_Type(rhs) == d->rhs_type);
        if (match) {
            *descr = d;
            return 1;
        }
    }
    return 0;
}

// The operands are values: the types and int tests read them without a box.
static void
specialize_binary_op(PyValue lhs_st, PyValue rhs_st,
                     _PyInstr *instr, int oparg, PyValue *locals)
{
    PyTypeObject *lhs_type = PyValue_Type(lhs_st);
    PyTypeObject *rhs_type = PyValue_Type(rhs_st);
    _PyFbBinary *fb = fb_begin_binary(instr, lhs_type, rhs_type);
    fb->descr = NULL;
    bool compact_ints =
        _PyValue_IsCompactInt(lhs_st) && _PyValue_IsCompactInt(rhs_st);
    // Two floats, or a float and a compact int.
    bool floats = _PyValue_FloatOperandsCheck(lhs_st, rhs_st);

    switch (oparg) {
        case NB_ADD:
        case NB_INPLACE_ADD:
            if (compact_ints) {
                specialize(instr, BINARY_OP_ADD_INT);
                return;
            }
            if (floats) {
                specialize(instr, BINARY_OP_ADD_FLOAT);
                return;
            }
            if (lhs_type != rhs_type) {
                break;
            }
            if (lhs_type == &PyUnicode_Type) {
                // A STORE_FAST folded into the word: its dst names the local
                _PyInterpreterFrame *frame = current_frame();
                int local = _PyCode_InstrStoredLocal(
                    _PyFrame_GetCode(frame), _PyFrame_InstrOffset(frame, instr));
                bool to_store = local >= 0;
                if (to_store && PyValue_Is(locals[local], lhs_st)) {
                    specialize(instr, BINARY_OP_INPLACE_ADD_UNICODE);
                    return;
                }
                specialize(instr, BINARY_OP_ADD_UNICODE);
                return;
            }
            break;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            if (compact_ints) {
                specialize(instr, BINARY_OP_MULTIPLY_INT);
                return;
            }
            if (floats) {
                specialize(instr, BINARY_OP_MULTIPLY_FLOAT);
                return;
            }
            break;
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            if (compact_ints) {
                specialize(instr, BINARY_OP_SUBTRACT_INT);
                return;
            }
            if (floats) {
                specialize(instr, BINARY_OP_SUBTRACT_FLOAT);
                return;
            }
            break;
        case NB_TRUE_DIVIDE:
        case NB_INPLACE_TRUE_DIVIDE:
            if (floats && _PyValue_NumberAsDouble(rhs_st) != 0.0) {
                specialize(instr, BINARY_OP_TRUEDIV_FLOAT);
                return;
            }
            break;
        case NB_FLOOR_DIVIDE:
        case NB_INPLACE_FLOOR_DIVIDE:
            if (compact_ints && _PyValue_CompactInt(rhs_st) != 0) {
                specialize(instr, BINARY_OP_FLOOR_DIVIDE_INT);
                return;
            }
            break;
        case NB_REMAINDER:
        case NB_INPLACE_REMAINDER:
            if (compact_ints && _PyValue_CompactInt(rhs_st) != 0) {
                specialize(instr, BINARY_OP_REMAINDER_INT);
                return;
            }
            break;
        case NB_AND:
        case NB_INPLACE_AND:
            if (compact_ints) {
                specialize(instr, BINARY_OP_AND_INT);
                return;
            }
            break;
        case NB_OR:
        case NB_INPLACE_OR:
            if (compact_ints) {
                specialize(instr, BINARY_OP_OR_INT);
                return;
            }
            break;
        case NB_XOR:
        case NB_INPLACE_XOR:
            if (compact_ints) {
                specialize(instr, BINARY_OP_XOR_INT);
                return;
            }
            break;
        case NB_LSHIFT:
        case NB_INPLACE_LSHIFT:
            if (compact_ints && _PyValue_CompactInt(rhs_st) >= 0) {
                specialize(instr, BINARY_OP_LSHIFT_INT);
                return;
            }
            break;
        case NB_RSHIFT:
        case NB_INPLACE_RSHIFT:
            if (compact_ints && _PyValue_CompactInt(rhs_st) >= 0) {
                specialize(instr, BINARY_OP_RSHIFT_INT);
                return;
            }
            break;
        case NB_SUBSCR:
            if (_PyValue_IsCompactInt(rhs_st) && _PyValue_CompactInt(rhs_st) >= 0) {
                if (lhs_type == &PyList_Type) {
                    specialize(instr, BINARY_OP_SUBSCR_LIST_INT);
                    return;
                }
                if (lhs_type == &PyTuple_Type) {
                    specialize(instr, BINARY_OP_SUBSCR_TUPLE_INT);
                    return;
                }
                if (lhs_type == &PyUnicode_Type) {
                    if (PyUnicode_IS_COMPACT_ASCII(PyValue_ObjectBorrow(lhs_st))) {
                        specialize(instr, BINARY_OP_SUBSCR_STR_INT);
                        return;
                    } else {
                        specialize(instr, BINARY_OP_SUBSCR_USTR_INT);
                        return;
                    }
                }
            }
            if (lhs_type->tp_as_mapping != NULL &&
                lhs_type->tp_as_mapping->mp_subscript == _PyDict_Subscript)
            {
                specialize(instr, BINARY_OP_SUBSCR_DICT);
                return;
            }
            if (lhs_type == &PyList_Type && rhs_type == &PySlice_Type) {
                specialize(instr, BINARY_OP_SUBSCR_LIST_SLICE);
                return;
            }
            unsigned int tp_version;
            PyTypeObject *container_type = lhs_type;
            PyObject *descriptor = _PyType_LookupRefAndVersion(container_type, &_Py_ID(__getitem__), &tp_version);
            if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type &&
                container_type->tp_flags & Py_TPFLAGS_HEAPTYPE)
            {
                PyFunctionObject *func = (PyFunctionObject *)descriptor;
                PyCodeObject *fcode = (PyCodeObject *)func->func_code;
                int kind = function_kind(fcode);
                PyHeapTypeObject *ht = (PyHeapTypeObject *)container_type;
                if (kind == SIMPLE_FUNCTION &&
                    fcode->co_argcount == 2 &&
                    _PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET()) && /* Don't specialize if PEP 523 is active */
                    _PyType_CacheGetItemForSpecialization(ht, descriptor, (uint32_t)tp_version))
                {
                    specialize(instr, BINARY_OP_SUBSCR_GETITEM);
                    Py_DECREF(descriptor);
                    return;
                }
            }
            Py_XDECREF(descriptor);
            break;
    }

    _PyBinaryOpSpecializationDescr *descr;
    if (binary_op_extended_specialization(lhs_st, rhs_st, oparg, &descr)) {
        fb->descr = descr;
        specialize(instr, BINARY_OP_EXTEND);
        return;
    }

    SPECIALIZATION_FAIL(BINARY_OP, binary_op_fail_kind(oparg, lhs_st, rhs_st));
    unspecialize(instr);
}

Py_NO_INLINE void
_Py_Specialize_BinaryOp(PyValue lhs_st, PyValue rhs_st, _PyInstr *instr,
                        int oparg, PyValue *locals)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[BINARY_OP] == INLINE_CACHE_ENTRIES_BINARY_OP);
    specialize_binary_op(lhs_st, rhs_st, instr, oparg, locals);
}


#ifdef Py_STATS
static int
compare_op_fail_kind(PyTypeObject *lhs, PyTypeObject *rhs)
{
    if (lhs != rhs) {
        if (lhs == &PyFloat_Type && rhs == &PyLong_Type) {
            return SPEC_FAIL_COMPARE_OP_FLOAT_LONG;
        }
        if (lhs == &PyLong_Type && rhs == &PyFloat_Type) {
            return SPEC_FAIL_COMPARE_OP_LONG_FLOAT;
        }
        return SPEC_FAIL_COMPARE_OP_DIFFERENT_TYPES;
    }
    if (lhs == &PyBytes_Type) {
        return SPEC_FAIL_COMPARE_OP_BYTES;
    }
    if (lhs == &PyTuple_Type) {
        return SPEC_FAIL_COMPARE_OP_TUPLE;
    }
    if (lhs == &PyList_Type) {
        return SPEC_FAIL_COMPARE_OP_LIST;
    }
    if (lhs == &PySet_Type || lhs == &PyFrozenSet_Type) {
        return SPEC_FAIL_COMPARE_OP_SET;
    }
    if (lhs == &PyBool_Type) {
        return SPEC_FAIL_COMPARE_OP_BOOL;
    }
    if (lhs->tp_richcompare == PyBaseObject_Type.tp_richcompare) {
        return SPEC_FAIL_COMPARE_OP_BASEOBJECT;
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS

Py_NO_INLINE void
_Py_Specialize_CompareOp(PyValue lhs_st, PyValue rhs_st, _PyInstr *instr,
                         int oparg)
{
    PyTypeObject *lhs_type = PyValue_Type(lhs_st);
    PyTypeObject *rhs_type = PyValue_Type(rhs_st);
    uint8_t specialized_op;

    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[COMPARE_OP] == INLINE_CACHE_ENTRIES_COMPARE_OP);
    fb_begin_binary(instr, lhs_type, rhs_type);
    // All of these specializations compute boolean values, so they're all valid
    // regardless of the fifth-lowest oparg bit.
    if (lhs_type != rhs_type) {
        SPECIALIZATION_FAIL(COMPARE_OP, compare_op_fail_kind(lhs_type, rhs_type));
        goto failure;
    }
    if (lhs_type == &PyFloat_Type) {
        specialized_op = COMPARE_OP_FLOAT;
        goto success;
    }
    if (lhs_type == &PyLong_Type) {
        if (_PyValue_IsCompactInt(lhs_st) && _PyValue_IsCompactInt(rhs_st)) {
            specialized_op = COMPARE_OP_INT;
            goto success;
        }
        else {
            SPECIALIZATION_FAIL(COMPARE_OP, SPEC_FAIL_COMPARE_OP_BIG_INT);
            goto failure;
        }
    }
    if (lhs_type == &PyUnicode_Type) {
        int cmp = oparg >> 5;
        if (cmp != Py_EQ && cmp != Py_NE) {
            SPECIALIZATION_FAIL(COMPARE_OP, SPEC_FAIL_COMPARE_OP_STRING);
            goto failure;
        }
        else {
            specialized_op = COMPARE_OP_STR;
            goto success;
        }
    }
    SPECIALIZATION_FAIL(COMPARE_OP, compare_op_fail_kind(lhs_type, rhs_type));
failure:
    unspecialize(instr);
    return;
success:
    specialize(instr, specialized_op);
}

#ifdef Py_STATS
static int
unpack_sequence_fail_kind(PyObject *seq)
{
    if (PySequence_Check(seq)) {
        return SPEC_FAIL_UNPACK_SEQUENCE_SEQUENCE;
    }
    if (PyIter_Check(seq)) {
        return SPEC_FAIL_UNPACK_SEQUENCE_ITERATOR;
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS

Py_NO_INLINE void
_Py_Specialize_UnpackSequence(PyValue seq_st, _PyInstr *instr, int oparg)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[UNPACK_SEQUENCE] ==
           INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE);
    fb_begin_unary(instr, PyValue_Type(seq_st));
    if (PyValue_IsInline(seq_st)) {
        SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *seq = PyValue_ObjectBorrow(seq_st);
    if (PyTuple_CheckExact(seq)) {
        if (PyTuple_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            unspecialize(instr);
            return;
        }
        if (PyTuple_GET_SIZE(seq) == 2) {
            specialize(instr, UNPACK_SEQUENCE_TWO_TUPLE);
            return;
        }
        specialize(instr, UNPACK_SEQUENCE_TUPLE);
        return;
    }
    if (PyList_CheckExact(seq)) {
        if (PyList_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            unspecialize(instr);
            return;
        }
        specialize(instr, UNPACK_SEQUENCE_LIST);
        return;
    }
    SPECIALIZATION_FAIL(UNPACK_SEQUENCE, unpack_sequence_fail_kind(seq));
    unspecialize(instr);
}

#ifdef Py_STATS
int
 _PySpecialization_ClassifyIterator(PyObject *iter)
{
    if (PyGen_CheckExact(iter)) {
        return SPEC_FAIL_ITER_GENERATOR;
    }
    if (PyCoro_CheckExact(iter)) {
        return SPEC_FAIL_ITER_COROUTINE;
    }
    if (PyAsyncGen_CheckExact(iter)) {
        return SPEC_FAIL_ITER_ASYNC_GENERATOR;
    }
    if (PyAsyncGenASend_CheckExact(iter)) {
        return SPEC_FAIL_ITER_ASYNC_GENERATOR_SEND;
    }
    PyTypeObject *t = Py_TYPE(iter);
    if (t == &PyListIter_Type) {
        return SPEC_FAIL_ITER_LIST;
    }
    if (t == &PyTupleIter_Type) {
        return SPEC_FAIL_ITER_TUPLE;
    }
    if (t == &PyDictIterKey_Type) {
        return SPEC_FAIL_ITER_DICT_KEYS;
    }
    if (t == &PyDictIterValue_Type) {
        return SPEC_FAIL_ITER_DICT_VALUES;
    }
    if (t == &PyDictIterItem_Type) {
        return SPEC_FAIL_ITER_DICT_ITEMS;
    }
    if (t == &PySetIter_Type) {
        return SPEC_FAIL_ITER_SET;
    }
    if (t == &PyUnicodeIter_Type) {
        return SPEC_FAIL_ITER_STRING;
    }
    if (t == &PyBytesIter_Type) {
        return SPEC_FAIL_ITER_BYTES;
    }
    if (t == &PyRangeIter_Type) {
        return SPEC_FAIL_ITER_RANGE;
    }
    if (t == &PyEnum_Type) {
        return SPEC_FAIL_ITER_ENUMERATE;
    }
    if (t == &PyMap_Type) {
        return SPEC_FAIL_ITER_MAP;
    }
    if (t == &PyZip_Type) {
        return SPEC_FAIL_ITER_ZIP;
    }
    if (t == &PySeqIter_Type) {
        return SPEC_FAIL_ITER_SEQ_ITER;
    }
    if (t == &PyListRevIter_Type) {
        return SPEC_FAIL_ITER_REVERSED_LIST;
    }
    if (t == &_PyUnicodeASCIIIter_Type) {
        return SPEC_FAIL_ITER_ASCII_STRING;
    }
    const char *name = t->tp_name;
    if (strncmp(name, "itertools", 9) == 0) {
        return SPEC_FAIL_ITER_ITERTOOLS;
    }
    if (strncmp(name, "callable_iterator", 17) == 0) {
        return SPEC_FAIL_ITER_CALLABLE;
    }
    return SPEC_FAIL_OTHER;
}
#endif   // Py_STATS

Py_NO_INLINE void
_Py_Specialize_ForIter(PyValue iter, PyValue null_or_index, _PyInstr *instr, int oparg)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[FOR_ITER] == INLINE_CACHE_ENTRIES_FOR_ITER);
    PyObject *iter_o = PyValue_ObjectBorrow(iter);
    PyTypeObject *tp = Py_TYPE(iter_o);
    fb_begin_unary(instr, tp);

    if (PyValue_IsNull(null_or_index)) {
        if (tp == &PyRangeIter_Type) {
            specialize(instr, FOR_ITER_RANGE);
            return;
        }
        else if (tp == &PyGen_Type && oparg <= SHRT_MAX) {
            // The generator returns to END_FOR, or for a relaxed FOR_ITER
            // to a jump there
            assert(IR_HANDLER(instr[1 + oparg]) == IR_JUMP ||
                   instr_opcode(instr + 1 + oparg) == END_FOR ||
                   instr_opcode(instr + 1 + oparg) == INSTRUMENTED_END_FOR);
            /* Don't specialize if PEP 523 is active */
            if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
                goto failure;
            }
            specialize(instr, FOR_ITER_GEN);
            return;
        }
    }
    else {
        if (tp->_tp_iteritem != NULL) {
            if (tp == &PyList_Type) {
                specialize(instr, FOR_ITER_LIST);
                return;
            }
            else if (tp == &PyTuple_Type) {
                specialize(instr, FOR_ITER_TUPLE);
                return;
            }
        }
        specialize(instr, FOR_ITER_VIRTUAL);
        return;
    }
failure:
    SPECIALIZATION_FAIL(FOR_ITER,
                        _PySpecialization_ClassifyIterator(iter_o));
    unspecialize(instr);
}

Py_NO_INLINE void
_Py_Specialize_Send(PyValue receiver_st, _PyInstr *instr)
{
    PyObject *receiver = PyValue_ObjectBorrow(receiver_st);

    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[SEND] == INLINE_CACHE_ENTRIES_SEND);
    PyTypeObject *tp = Py_TYPE(receiver);
    fb_begin_unary(instr, tp);
    if (tp == &PyGen_Type || tp == &PyCoro_Type) {
        /* Don't specialize if PEP 523 is active */
        if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
            SPECIALIZATION_FAIL(SEND, SPEC_FAIL_OTHER);
            goto failure;
        }
        specialize(instr, SEND_GEN);
        return;
    }
    if (tp->_tp_iteritem != NULL) {
        specialize(instr, SEND_VIRTUAL);
        return;
    }
    if (tp == &_PyAsyncGenASend_Type) {
        specialize(instr, SEND_ASYNC_GEN);
        return;
    }
    SPECIALIZATION_FAIL(SEND,
                        _PySpecialization_ClassifyIterator(receiver));
failure:
    unspecialize(instr);
}

Py_NO_INLINE void
_Py_Specialize_CallFunctionEx(PyValue func_st, _PyInstr *instr)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[CALL_FUNCTION_EX] == INLINE_CACHE_ENTRIES_CALL_FUNCTION_EX);
    fb_begin_call(instr, PyValue_Type(func_st));
    if (PyValue_IsInline(func_st)) {
        SPECIALIZATION_FAIL(CALL_FUNCTION_EX, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *func = PyValue_ObjectBorrow(func_st);

    if (Py_TYPE(func) == &PyFunction_Type &&
        ((PyFunctionObject *)func)->vectorcall == _PyFunction_Vectorcall) {
        if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
            goto failure;
        }
        specialize(instr, CALL_EX_PY);
        return;
    }
    specialize(instr, CALL_EX_NON_PY_GENERAL);
    return;
failure:
    unspecialize(instr);
}

#ifdef Py_STATS
static int
to_bool_fail_kind(PyObject *value)
{
    if (PyByteArray_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_BYTEARRAY;
    }
    if (PyBytes_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_BYTES;
    }
    if (PyDict_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_DICT;
    }
    if (PyFloat_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_FLOAT;
    }
    if (PyMemoryView_Check(value)) {
        return SPEC_FAIL_TO_BOOL_MEMORY_VIEW;
    }
    if (PyAnySet_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_SET;
    }
    if (PyTuple_CheckExact(value)) {
        return SPEC_FAIL_TO_BOOL_TUPLE;
    }
    return SPEC_FAIL_OTHER;
}
#endif  // Py_STATS

static int
check_type_always_true(PyTypeObject *ty)
{
    PyNumberMethods *nb = ty->tp_as_number;
    if (nb && nb->nb_bool) {
        return SPEC_FAIL_TO_BOOL_NUMBER;
    }
    PyMappingMethods *mp = ty->tp_as_mapping;
    if (mp && mp->mp_length) {
        return SPEC_FAIL_TO_BOOL_MAPPING;
    }
    PySequenceMethods *sq = ty->tp_as_sequence;
    if (sq && sq->sq_length) {
      return SPEC_FAIL_TO_BOOL_SEQUENCE;
    }
    return 0;
}

/* The specialisations of a truth test, in the order of truth_ops below */
enum { TRUTH_BOOL, TRUTH_INT, TRUTH_LIST, TRUTH_NONE, TRUTH_STR,
       TRUTH_ALWAYS_TRUE, TRUTH_COUNT };

/* TO_BOOL and the conditional jumps, which test the truth of a value the
 * same way, choose a specialisation from their own row of ops. */
static void
specialize_truth(PyValue value_o, _PyInstr *instr, const uint8_t *ops)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[TO_BOOL] == INLINE_CACHE_ENTRIES_TO_BOOL);
    fb_begin_unary(instr, PyValue_Type(value_o));
    if (PyValue_IsInlineInt(value_o)) {
        specialize(instr, ops[TRUTH_INT]);
        return;
    }
    if (PyValue_IsInlineFloat(value_o)) {
        SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_TO_BOOL_FLOAT);
        unspecialize(instr);
        return;
    }
    PyObject *value = PyValue_ObjectBorrow(value_o);
    uint8_t specialized_op;
    if (PyBool_Check(value)) {
        specialized_op = ops[TRUTH_BOOL];
        goto success;
    }
    if (PyLong_CheckExact(value)) {
        specialized_op = ops[TRUTH_INT];
        goto success;
    }
    if (PyList_CheckExact(value)) {
        specialized_op = ops[TRUTH_LIST];
        goto success;
    }
    if (Py_IsNone(value)) {
        specialized_op = ops[TRUTH_NONE];
        goto success;
    }
    if (PyUnicode_CheckExact(value)) {
        specialized_op = ops[TRUTH_STR];
        goto success;
    }
    if (PyType_HasFeature(Py_TYPE(value), Py_TPFLAGS_HEAPTYPE)) {
        unsigned int version = 0;
        int err = _PyType_Validate(Py_TYPE(value), check_type_always_true, &version);
        if (err < 0) {
            SPECIALIZATION_FAIL(TO_BOOL, SPEC_FAIL_OUT_OF_VERSIONS);
            goto failure;
        }
        else if (err > 0) {
            SPECIALIZATION_FAIL(TO_BOOL, err);
            goto failure;
        }

        assert(err == 0);
        assert(version);
        ((_PyFbUnary *)instr_entry(instr))->type_version = version;
        specialized_op = ops[TRUTH_ALWAYS_TRUE];
        goto success;
    }

    SPECIALIZATION_FAIL(TO_BOOL, to_bool_fail_kind(value));
failure:
    unspecialize(instr);
    return;
success:
    specialize(instr, specialized_op);
}

Py_NO_INLINE void
_Py_Specialize_ToBool(PyValue value_o, _PyInstr *instr)
{
    static const uint8_t ops[TRUTH_COUNT] = {
        TO_BOOL_BOOL, TO_BOOL_INT, TO_BOOL_LIST, TO_BOOL_NONE, TO_BOOL_STR,
        TO_BOOL_ALWAYS_TRUE,
    };
    specialize_truth(value_o, instr, ops);
}

Py_NO_INLINE void
_Py_Specialize_PopJumpIf(PyValue value_o, _PyInstr *instr)
{
    static const uint8_t if_false[TRUTH_COUNT] = {
        POP_JUMP_IF_FALSE_BOOL, POP_JUMP_IF_FALSE_INT, POP_JUMP_IF_FALSE_LIST,
        POP_JUMP_IF_FALSE_NONE, POP_JUMP_IF_FALSE_STR,
        POP_JUMP_IF_FALSE_ALWAYS_TRUE,
    };
    static const uint8_t if_true[TRUTH_COUNT] = {
        POP_JUMP_IF_TRUE_BOOL, POP_JUMP_IF_TRUE_INT, POP_JUMP_IF_TRUE_LIST,
        POP_JUMP_IF_TRUE_NONE, POP_JUMP_IF_TRUE_STR,
        POP_JUMP_IF_TRUE_ALWAYS_TRUE,
    };
    int opcode = _PyOpcode_Deopt[instr_opcode(instr)];
    assert(opcode == POP_JUMP_IF_FALSE || opcode == POP_JUMP_IF_TRUE);
    specialize_truth(value_o, instr,
                     opcode == POP_JUMP_IF_FALSE ? if_false : if_true);
}

#ifdef Py_STATS
static int
containsop_fail_kind(PyObject *value) {
    if (PyUnicode_CheckExact(value)) {
        return SPEC_FAIL_CONTAINS_OP_STR;
    }
    if (PyList_CheckExact(value)) {
        return SPEC_FAIL_CONTAINS_OP_LIST;
    }
    if (PyTuple_CheckExact(value)) {
        return SPEC_FAIL_CONTAINS_OP_TUPLE;
    }
    if (PyType_Check(value)) {
        return SPEC_FAIL_CONTAINS_OP_USER_CLASS;
    }
    return SPEC_FAIL_OTHER;
}
#endif

Py_NO_INLINE void
_Py_Specialize_ContainsOp(PyValue item_st, PyValue value_st, _PyInstr *instr)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[CONTAINS_OP] == INLINE_CACHE_ENTRIES_COMPARE_OP);
    fb_begin_binary(instr, PyValue_Type(item_st), PyValue_Type(value_st));
    if (PyValue_IsInline(value_st)) {
        SPECIALIZATION_FAIL(CONTAINS_OP, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *value = PyValue_ObjectBorrow(value_st);
    if (PyAnyDict_CheckExact(value)) {
        specialize(instr, CONTAINS_OP_DICT);
        return;
    }
    if (PySet_CheckExact(value) || PyFrozenSet_CheckExact(value)) {
        specialize(instr, CONTAINS_OP_SET);
        return;
    }

    SPECIALIZATION_FAIL(CONTAINS_OP, containsop_fail_kind(value));
    unspecialize(instr);
    return;
}

void
_Py_Specialize_GetIter(PyValue iterable, _PyInstr *instr)
{
    assert(ENABLE_SPECIALIZATION);
    PyTypeObject *tp = PyValue_Type(iterable);
    fb_begin_unary(instr, tp);
    if (tp->_tp_iteritem != NULL) {
        specialize(instr, GET_ITER_VIRTUAL);
        return;
    }
    if (tp->tp_iter == PyObject_SelfIter) {
        specialize(instr, GET_ITER_SELF);
        return;
    }
    SPECIALIZATION_FAIL(GET_ITER,
        tp == &PyCoro_Type ? SPEC_FAIL_ITER_COROUTINE : SPEC_FAIL_OTHER);
    unspecialize(instr);
}


#ifdef Py_STATS
void
_Py_GatherStats_GetIter(PyValue iterable)
{
    PyTypeObject *tp = PyValue_Type(iterable);
    int kind = SPEC_FAIL_OTHER;
    if (tp == &PyTuple_Type) {
        kind = SPEC_FAIL_ITER_TUPLE;
    }
    else if (tp == &PyList_Type) {
        kind = SPEC_FAIL_ITER_LIST;
    }
    else if (tp == &PyDict_Type) {
        kind = SPEC_FAIL_ITER_DICT_KEYS;
    }
    else if (tp == &PySet_Type) {
        kind = SPEC_FAIL_ITER_SET;
    }
    else if (tp == &PyBytes_Type) {
        kind = SPEC_FAIL_ITER_BYTES;
    }
    else if (tp == &PyEnum_Type) {
        kind = SPEC_FAIL_ITER_ENUMERATE;
    }
    else if (tp == &PyUnicode_Type) {
        kind = SPEC_FAIL_ITER_STRING;
    }
    else if (tp == &PyGen_Type) {
        kind = SPEC_FAIL_ITER_GENERATOR;
    }
    else if (tp == &PyCoro_Type) {
        kind = SPEC_FAIL_ITER_COROUTINE;
    }
    else if (tp == &PyAsyncGen_Type) {
        kind = SPEC_FAIL_ITER_ASYNC_GENERATOR;
    }
    else if (tp == &_PyAsyncGenASend_Type) {
        kind = SPEC_FAIL_ITER_ASYNC_GENERATOR_SEND;
    }
    else if (tp->tp_iter == PyObject_SelfIter) {
        kind = SPEC_FAIL_ITER_SELF;
    }
    SPECIALIZATION_FAIL(GET_ITER, kind);
}
#endif


/* Code init cleanup.
 * CALL_ALLOC_AND_ENTER_INIT will set up
 * the frame to execute the EXIT_INIT_CHECK
 * instruction.
 * Starts with a NOP, which never runs: the shim's instr_ptr stays there, and
 * __init__ returns to the word after it.
 * This is used as a plain code object, not a function,
 * so must not access globals or builtins.
 * There are a few other constraints imposed on the code
 * by the free-threaded build:
 *
 * 1. The RESUME instruction must not be executed. Otherwise we may attempt to
 *    free the statically allocated TLBC array.
 * 2. It must contain no specializable instructions. Specializing multiple
 *    copies of the same bytecode is not thread-safe in free-threaded builds.
 *
 * This should be dynamically allocated if either of those restrictions need to
 * be lifted.
 */

#define NO_LOC_4 (128 | (PY_CODE_LOCATION_INFO_NONE << 3) | 3)

static const PyBytesObject no_location = {
    PyVarObject_HEAD_INIT(&PyBytes_Type, 1)
    .ob_sval = { NO_LOC_4 }
};


const struct _PyCode3 _Py_InitCleanup = {
    _PyVarObject_HEAD_INIT(&PyCode_Type, 3),
    .co_consts = (PyObject *)&_Py_SINGLETON(tuple_empty),
    .co_names = (PyObject *)&_Py_SINGLETON(tuple_empty),
    .co_exceptiontable = (PyObject *)&_Py_SINGLETON(bytes_empty),
    .co_flags = CO_OPTIMIZED | CO_NO_MONITORING_EVENTS,
    .co_localsplusnames = (PyObject *)&_Py_SINGLETON(tuple_empty),
    .co_localspluskinds = (PyObject *)&_Py_SINGLETON(bytes_empty),
    .co_filename = &_Py_ID(__init__),
    .co_name = &_Py_ID(__init__),
    .co_qualname = &_Py_ID(__init__),
    .co_linetable = (PyObject *)&no_location,
    ._co_firsttraceable = 3,
    .co_stacksize = 2,
    .co_framesize = 2 + FRAME_SPECIALS_SIZE,
    // The same words as _PyIR_InitCleanupExec (Python/ir.c), which this
    // static code object starts with as its executing copy
    ._co_exec = &_PyIR_InitCleanupExec,
    .co_words = {
        IR_WORD(NOP, 0, 0, 0, 0),
        IR_WORD(EXIT_INIT_CHECK, 1, 0, 0, 0),
        IR_WORD(RETURN_VALUE, 0, 0, 0, 0),
    }
};
