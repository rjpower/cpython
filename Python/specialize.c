#include "Python.h"

#include "opcode.h"

#include "pycore_attr_recipe.h"   // _PyAttrRecipe
#include "pycore_bytesobject.h"   // _PyBytes_Concat
#include "pycore_call.h"          // _Py_METH_CALL_FLAGS
#include "pycore_code.h"
#include "pycore_critical_section.h"
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_dict.h"          // DICT_KEYS_UNICODE
#include "pycore_function.h"      // _PyFunction_GetVersionForCurrentState()
#include "pycore_global_recipe.h" // _PyGlobalRecipe
#include "pycore_interpframe.h"   // FRAME_SPECIALS_SIZE
#include "pycore_lazyimportobject.h" // PyLazyImport_CheckExact
#include "pycore_list.h"          // _PyListIterObject, _PyList_Concat
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_tuple.h"         // _PyTuple_Concat
#include "pycore_long.h"          // _PyLong_IsNonNegativeCompact()
#include "pycore_moduleobject.h"
#include "pycore_object.h"
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_opcode_utils.h"  // RESUME_AT_FUNC_START
#include "pycore_pylifecycle.h"   // _PyOS_URandomNonblock()
#include "pycore_rop_rows.h"      // _PyRop_Install()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_unicodeobject.h" // _PyUnicodeASCIIIter_Type
#include "pycore_value_ops.h"     // _PyObject_GenericGetAttrValue()
#include "pycore_value_slots.h"   // _PyType_VALUE_SLOT()

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


typedef enum {
    SIMPLE_SIGNATURE,
    GENERAL_SIGNATURE,
    UNOPTIMIZED_SIGNATURE,
} FunctionSignature;

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
#define SPEC_FAIL_ATTR_NOT_IN_KEYS 29
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
#define SPEC_FAIL_TO_BOOL_MAPPING     13
#define SPEC_FAIL_TO_BOOL_NUMBER      15
#define SPEC_FAIL_TO_BOOL_SEQUENCE    16

/* Instruction access.  instr is the instruction being specialised: a word
 * of the executing copy of the current frame's code object. */

static inline _PyInterpreterFrame *
current_frame(void)
{
    return _PyThreadState_GET()->current_frame;
}

/* The stack op that the word's handler stands for, whether it is an old
 * handler, an IR-only one or a register instruction arm: the index for the
 * tables that are sized for stack ops (_PyOpcode_Deopt, _PyOpcode_Caches,
 * the statistics) */
static inline int
instr_base_opcode(_PyInstr *instr)
{
    return _PyIR_BaseOpcode(IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*instr)));
}

/* The opcode of the handler; for an IR-only handler, the instruction that
 * it stands for, as BINARY_OP for BINARY_OP_ADD. */
static inline int
instr_opcode(_PyInstr *instr)
{
    int handler = IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*instr));
    if (_PyIR_IsOnlyHandler(handler)) {
        handler = _PyIR_FormatOf(handler)->shadow;
        assert(!_PyIR_IsOnlyHandler(handler));
    }
    return handler;
}

static inline int
instr_oparg(_PyInstr *instr)
{
    _PyInterpreterFrame *frame = current_frame();
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int offset = _PyFrame_InstrOffset(frame, instr);
    int oparg = _PyCode_InstrOparg(code, offset);
    _PyIRWord word = _PyCode_IRWords(code)[offset];
    const _PyIRFormat *format = _PyIR_FormatOf(IR_HANDLER(word));
    if (format->operands == _PyIR_OPERANDS_CALL
        || format->operands == _PyIR_OPERANDS_KW)
    {
        // The count of a call is its operands: the arguments of the call are
        // the operands without the self slot and the names
        return _PyIR_CallSlotOperands(word) - _PyIR_CallHasSelf(word);
    }
    return oparg;
}

static inline _PyFbHeader *
instr_fb(_PyInstr *instr)
{
    _PyInterpreterFrame *frame = current_frame();
    return _PyIR_InstrFeedback(_PyFrame_GetCode(frame), _PyFrame_Exec(frame),
                               instr);
}

/* Whether h is entry 0, the header that the sites without an entry share and
 * which never specialises */
static inline int
is_shared_entry(_PyFbHeader *h)
{
    return h == _PyIR_FbEntry(_PyFrame_Exec(current_frame())->words, IR_FB_SHARED);
}

static inline void *
instr_entry(_PyInstr *instr)
{
    _PyFbHeader *h = instr_fb(instr);
    assert(h != NULL);
    // The specialisers write the fields past the header
    assert(!is_shared_entry(h));
    return h;
}

static inline int
set_opcode(_PyInstr *instr, int opcode)
{
    int previous = IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*instr));
    const _PyIRGeneratorBoundary *boundary =
        _PyIR_GeneratorBoundaryOf(previous);
    if (boundary != NULL && (boundary->flags & _PyIR_GEN_BOUNDARY_PLACED)) {
        const _PyIRGeneratorBoundary *target = _PyIR_GeneratorBoundaryOf(opcode);
        assert(target != NULL);
        int placed = _PyIR_GeneratorBoundaryHandler(
            target->ordinary, boundary->yielded_consumer,
            boundary->completed_consumer, boundary->resume_consumer);
        /* CLOSED is a cold source-family invariant. Never turn a supported
         * specialization into a placement-induced failure/backoff. */
        if (placed < 0) {
            Py_FatalError("missing closed generator specialization target");
        }
        opcode = placed;
    }
#ifdef Py_GIL_DISABLED
    _PyIRWord old = _Py_atomic_load_uint64_relaxed(instr);
    int handler = IR_HANDLER(old);
    if (_PyIR_HandlerFlags[handler] & _PyIR_H_INSTRUMENTED) {
        /* Lost race with instrumentation */
        return 0;
    }
    _PyIRWord new = (old & ~(_PyIRWord)IR_HANDLER_MASK) | (_PyIRWord)opcode;
    if (!_Py_atomic_compare_exchange_uint64(instr, &old, new)) {
        /* Lost race with instrumentation */
        return 0;
    }
    return 1;
#else
    _PyIR_SetHandler(instr, opcode);
    /* No unit is marked here: a unit is stale only when its code finds the
     * word's handler changed (Python/jit.c, resolve_status()) */
    return 1;
#endif
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

/* Whether the word runs a specialisation: any handler but the generic one
 * of the code object */
static inline int
instr_is_specialized(_PyInstr *instr)
{
    _PyInterpreterFrame *frame = current_frame();
    int generic = _PyCode_GenericHandler(_PyFrame_GetCode(frame),
                                         _PyFrame_InstrOffset(frame, instr));
    return IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*instr)) != generic;
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
    _PyFbPayload payload;
} fb_row;

static _PyFbBlock *
fb_block(_PyInstr *instr)
{
    _PyCodeExec *exec = _PyFrame_Exec(current_frame());
    assert(exec->words <= instr && instr < exec->words + exec->nwords);
    return _PyIR_FbBlock(exec->words);
}

static fb_row
fb_row_of(_PyFbHeader *h, int opcode)
{
    fb_row row = {0};
    switch (h->kind) {
        case FB_UNARY:
            row.type_version = ((_PyFbUnary *)h)->type_version;
            break;
        case FB_BINARY:
            row.type_version = ((_PyFbBinary *)h)->lhs_version;
            row.aux = ((_PyFbBinary *)h)->rhs_version;
            row.payload.descr = ((_PyFbBinary *)h)->descr;
            break;
        case FB_ATTR:
            row.type_version = ((_PyFbAttr *)h)->type_version;
            row.aux = ((_PyFbAttr *)h)->dict_version;
            row.index = ((_PyFbAttr *)h)->index;
            /* A historical observation copies metadata, never a live borrow.
             * The recorded handler identifies the active payload member. */
            switch (opcode) {
                case ROP_HANDLER_GetAttr_Shape:
                    row.payload.read_kind = ((_PyFbAttr *)h)->read_kind;
                    break;
                case ROP_HANDLER_GetAttr_GetSet:
                case ROP_HANDLER_GetMethod_MethodValues:
                case ROP_HANDLER_GetMethod_NoDict:
                case ROP_HANDLER_GetMethod_LazyDict:
                case ROP_HANDLER_GetMethod_ClassMethod:
                case ROP_HANDLER_GetAttr_Class:
                case ROP_HANDLER_GetAttr_NonDescriptorValues:
                case ROP_HANDLER_GetAttr_NonDescriptorNoDict:
                    row.payload.class_value = ((_PyFbAttr *)h)->class_value;
                    break;
                case STORE_ATTR_SHAPE_ADD:
                    row.payload.shape_to = ((_PyFbAttr *)h)->shape_to;
                    break;
                default:
                    break;
            }
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
    poly->payload[i] = row.payload;
    poly->handler[i] = (uint16_t)opcode;
}

/* Save the observation of a specialised MONO site in a new poly table.
 * Losing it to a failed allocation only loses feedback. */
static void
fb_save_mono(_PyInstr *instr, _PyFbHeader *h, int opcode, bool may_allocate)
{
    _PyFbBlock *block = fb_block(instr);
    uint32_t n = block->npoly;
    if ((n & (n - 1)) == 0) {
        if (!may_allocate) {
            return;
        }
        size_t capacity = n ? 2 * (size_t)n : 1;
        _PyFbPoly *poly = _PyMem_NoScanRealloc(block->poly, capacity * sizeof(_PyFbPoly));
        if (poly == NULL) {
            return;
        }
        block->poly = poly;
    }
    memset(&block->poly[n], 0, sizeof(_PyFbPoly));
    fb_put_row(&block->poly[n], 0, fb_row_of(h, opcode), opcode);
    block->npoly = n + 1;
    h->poly = (n + 1) * sizeof(_PyFbPoly);
}

/* Called by each specialiser of a site that keeps poly rows, before it
 * rewrites the entry.  Every specialiser then replaces the opcode in memory,
 * through specialize(), unspecialize() or LOAD_ATTR's retry of an absent
 * attribute, so no specialised handler reads the cleared entry. */
static _PyFbHeader *
fb_begin_ex(_PyInstr *instr, bool may_allocate)
{
    _PyFbHeader *h = instr_fb(instr);
    assert(h != NULL);
    // Entry 0 is a header and never specialises
    assert(!is_shared_entry(h));
    if (h->state == FB_MONO && h->poly == 0 && instr_is_specialized(instr)) {
        fb_save_mono(instr, h, IR_HANDLER(*instr), may_allocate);
    }
    // Constant sizes, so that the compiler inlines the stores
    switch (h->kind) {
        case FB_UNARY:
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

static _PyFbHeader *
fb_begin(_PyInstr *instr)
{
    return fb_begin_ex(instr, true);
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
fb_begin_call_ex(_PyInstr *instr, PyTypeObject *callable_type, bool may_allocate)
{
    _PyFbCall *fb = (_PyFbCall *)fb_begin_ex(instr, may_allocate);
    fb->callable_type_version = fb_type_version(callable_type);
}

static void
fb_begin_call(_PyInstr *instr, PyTypeObject *callable_type)
{
    fb_begin_call_ex(instr, callable_type, true);
}

/* Inline values

   An operand may be an inline int or float (see pycore_value.h). The
   specializer tests it through its value, with PyValue_TYPE() and the
   inline-aware int checks, and fails where an int or float cannot be the
   operand, such as the owner of LOAD_ATTR, whose specialized forms guard on
   an object's type. Binary nomination uses canonical type/compact predicates
   and only inspects object operands where those types require them. */



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
    _PyFbPoly *poly = _PyFb_PolyOf(fb_block(instr), h);
    fb_row row = fb_row_of(h, opcode);
    int i = 0;
    while (i < FB_POLY_SIZE && poly->handler[i] != 0 &&
           (poly->type_version[i] != row.type_version ||
            poly->aux[i] != row.aux || poly->handler[i] != opcode)) {
        i++;
    }
    if (i == FB_POLY_SIZE) {
        h->state = FB_MEGA;
        return;
    }
    fb_put_row(poly, i, row, opcode);
    h->state = poly->handler[1] != 0 ? FB_POLY : FB_MONO;
}

/* Nomination has already proved each offset against its exact type/name and
 * filled shape. Execution still checks the current type version and shape. */
static int
fb_shape_read_set(_PyInstr *instr, _PyFbHeader *h)
{
#ifdef Py_SHAPES
    if (h->state != FB_POLY || h->poly == 0) {
        return 0;
    }
    _PyFbPoly *poly = _PyFb_PolyOf(fb_block(instr), h);
    int count = 0;
    for (; count < FB_POLY_SIZE && poly->handler[count] != 0; count++) {
        if (poly->handler[count] != ROP_HANDLER_GetAttr_Shape ||
            poly->payload[count].read_kind != _Py_FB_READ_SHAPE ||
            poly->type_version[count] == 0 ||
            poly->aux[count] == _Py_SHAPE_NONE)
        {
            return 0;
        }
        for (int earlier = 0; earlier < count; earlier++) {
            if (poly->type_version[earlier] == poly->type_version[count]) {
                return 0;
            }
        }
    }
    return count >= 2;
#else
    (void)instr;
    (void)h;
    return 0;
#endif
}

/* Nomination of GetMethod.PolyMethod: the site has recorded two or more
 * receiver types, and every row (written by fb_record() through
 * fb_put_row(), as the shape rows are) is a method observation with a
 * distinct type version. */
static int
fb_method_poly_set(_PyInstr *instr, _PyFbHeader *h)
{
    if (h->state != FB_POLY || h->poly == 0) {
        return 0;
    }
    _PyFbPoly *poly = _PyFb_PolyOf(fb_block(instr), h);
    int count = 0;
    for (; count < FB_POLY_SIZE && poly->handler[count] != 0; count++) {
        if ((poly->handler[count] != ROP_HANDLER_GetMethod_MethodValues &&
             poly->handler[count] != ROP_HANDLER_GetMethod_NoDict) ||
            poly->type_version[count] == 0)
        {
            return 0;
        }
        for (int earlier = 0; earlier < count; earlier++) {
            if (poly->type_version[earlier] == poly->type_version[count]) {
                return 0;
            }
        }
    }
    return count >= 2;
}

/* The attribute specializer is the analysis behind the evidence rows of
 * GetAttr and GetMethod (classify_getattr and classify_getmethod,
 * Python/register_ops/getattr.rop).  It fills the site's entry and reports
 * the kind of access it found; the row answers with a case, and the
 * adaptation macro installs the arm it chooses. */
typedef enum {
    ATTR_NONE = 0,
    ATTR_INSTANCE_VALUE,
    ATTR_SHAPE,
    ATTR_MODULE,
    ATTR_WITH_HINT,
    ATTR_SLOT,
    ATTR_SLOT_POINTER,
    ATTR_CLASS,
    ATTR_CLASS_WITH_METACLASS_CHECK,
    ATTR_CLASSMETHOD,
    ATTR_NONDESCRIPTOR_WITH_VALUES,
    ATTR_NONDESCRIPTOR_NO_DICT,
    ATTR_GETSET,
    ATTR_METHOD_WITH_VALUES,
    ATTR_METHOD_NO_DICT,
    ATTR_METHOD_LAZY_DICT,
    ATTR_KIND_LIMIT,
} attr_kind;

/* Kinds share an int with opcodes in the helpers that LOAD_ATTR and STORE_ATTR
 * have in common: they start above every opcode and arm handler. */
#define ATTR_KIND_BASE 0x1000
#define ATTR_KIND(K) (ATTR_KIND_BASE + (K))

typedef struct {
    int kind;           // the attr_kind the specializer found, or 0
} attr_nomination;

/* The classification under way on this thread.  Subinterpreters with
 * their own GIL classify concurrently, so it is per thread. */
static _Py_thread_local attr_nomination *attr_collect;

/* Report the access found for the site: the form of specialized_opcode
 * (which is a store opcode or an attr kind). */
static inline void
specialize_handler(_PyInstr *instr, int specialized_opcode, int handler)
{
    assert(!PyErr_Occurred());
    if (specialized_opcode >= ATTR_KIND_BASE) {
        assert(attr_collect != NULL);
        attr_collect->kind = handler - ATTR_KIND_BASE;
        return;
    }
    if (!set_opcode(instr, handler)) {
        STAT_INC(_PyIR_BaseOpcode(specialized_opcode), failure);
        SPECIALIZATION_FAIL(_PyIR_BaseOpcode(specialized_opcode),
                            SPEC_FAIL_OTHER);
        return;
    }
    STAT_INC(_PyIR_BaseOpcode(specialized_opcode), success);
    _PyFbHeader *h = instr_fb(instr);
    if (h == NULL) {
        // RESUME specialises without an entry
        return;
    }
    assert(!is_shared_entry(h));
    set_counter(h, adaptive_counter_cooldown());
    fb_record(instr, h, handler);
}

static inline void
specialize(_PyInstr *instr, int specialized_opcode)
{
    specialize_handler(instr, specialized_opcode, specialized_opcode);
}

static inline void
unspecialize(_PyInstr *instr)
{
    assert(!PyErr_Occurred());
    if (attr_collect != NULL) {
        // The row reports the failure; the macro backs off
        return;
    }
    STAT_INC(instr_base_opcode(instr), failure);
    // For BINARY_OP, the handler of the operation
    _PyInterpreterFrame *frame = current_frame();
    int generic = _PyCode_GenericHandler(_PyFrame_GetCode(frame),
                                         _PyFrame_InstrOffset(frame, instr));
    if (!set_opcode(instr, generic)) {
        SPECIALIZATION_FAIL(instr_base_opcode(instr), SPEC_FAIL_OTHER);
        return;
    }
    _PyFbHeader *h = instr_fb(instr);
    if (h != NULL) {
        assert(!is_shared_entry(h));
        set_counter(h, adaptive_counter_backoff(load_counter(h)));
    }
}

/* The adaptation rows of the register instruction families
 * (Python/register_ops/core.rop; pycore_rop_rows.h). */

void
_PyRop_Backoff(_PyFbHeader *h)
{
    assert(!is_shared_entry(h));
    set_counter(h, adaptive_counter_backoff(load_counter(h)));
}

static int binary_op_extended_specialization(PyObject *lhs, PyObject *rhs, int oparg,
                                             _PyBinaryOpSpecializationDescr **descr);

bool
_PyRop_FindAddExtension(_PyFbBinary *fb, PyValueBorrow lhs_st, PyValueBorrow rhs_st)
{
    fb->descr = NULL;
    if (!PyValue_IsObject(lhs_st) || !PyValue_IsObject(rhs_st)) {
        return false;
    }
    _PyBinaryOpSpecializationDescr *descr;
    if (!binary_op_extended_specialization(PyValue_AsKnownObjectBorrow(lhs_st),
                                           PyValue_AsKnownObjectBorrow(rhs_st),
                                           NB_ADD, &descr)) {
        return false;
    }
    fb->descr = descr;
    return true;
}

int
_PyRop_Install(_PyInstr *instr, _PyFbHeader *h, int handler)
{
    if (is_shared_entry(h)) {
        return -1;
    }
    /* A nomination that does not change the word still consumes the
     * attempt, so the site does not sample again at once */
    set_counter(h, adaptive_counter_cooldown());
    int current = IR_HANDLER(FT_ATOMIC_LOAD_UINT64_RELAXED(*instr));
    /* An instrumented word (INSTRUMENTED_LINE runs the arm for its line
     * event) keeps its handler: installing over it would drop the event. */
    if (current == handler || (_PyIR_HandlerFlags[current] & _PyIR_H_INSTRUMENTED)) {
        return 0;
    }
    if (!set_opcode(instr, handler)) {
        return -1;
    }
    // A poly arm reads the rows that its own nomination found: it adds none
    if (handler != ROP_HANDLER_GetAttr_PolyShape &&
        handler != ROP_HANDLER_GetMethod_PolyMethod)
    {
        fb_record(instr, h, handler);
    }
    return 1;
}

static FunctionSignature function_signature(PyCodeObject *code);
static bool function_check_args(PyObject *o, int expected_argcount, int opcode);
static uint32_t function_get_version(PyObject *o, int opcode);

#ifdef Py_GIL_DISABLED
static void
maybe_enable_deferred_ref_count(PyObject *op)
{
    if (!_Py_IsOwnedByCurrentThread(op) && _PyObject_GC_IS_TRACKED(op)) {
        // For module level variables that are heavily used from multiple
        // threads, deferred reference counting provides good scaling
        // benefits.  The downside is that the object will only be deallocated
        // by a GC run.
        PyUnstable_Object_EnableDeferredRefcount(op);
    }
}
#endif


static int
specialize_module_load_attr_lock_held(PyDictObject *dict, _PyInstr *instr, PyObject *name)
{
    _PyFbAttr *fb = instr_entry(instr);
    if (dict->ma_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NON_STRING);
        return -1;
    }
    PyValue value = PyValue_NULL;
    Py_ssize_t index = _PyDict_LookupIndexAndOwnedValue(dict, name, &value);
    if (PyValue_IsObject(value) &&
        PyLazyImport_CheckExact(PyValue_AsKnownObjectBorrow(value))) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MODULE_LAZY_VALUE);
        goto fail;
    }
    assert(index != DKIX_ERROR);
    if (index == DKIX_EMPTY) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MODULE_ATTR_NOT_FOUND);
        goto fail;
    }
    uint32_t keys_version = _PyDict_GetKeysVersionForCurrentState(
            _PyInterpreterState_GET(), dict);
    if (keys_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        goto fail;
    }
    fb->dict_version = keys_version;
    fb->index = (uint32_t)index;
    specialize(instr, ATTR_KIND(ATTR_MODULE));
    PyValue_XCLOSE(value);
    return 0;
fail:
    PyValue_XCLOSE(value);
    return -1;
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
        PyValue_AsKnownObjectBorrow(global_super_st) != (PyObject *)&PySuper_Type)
    {
        SPECIALIZATION_FAIL(LOAD_SUPER_ATTR, SPEC_FAIL_SUPER_SHADOWED);
        goto fail;
    }
    if (!PyType_FastSubclass(PyValue_TYPE(cls_st), Py_TPFLAGS_TYPE_SUBCLASS)) {
        SPECIALIZATION_FAIL(LOAD_SUPER_ATTR, SPEC_FAIL_SUPER_BAD_CLASS);
        goto fail;
    }
    uint8_t load_code = load_method ? LOAD_SUPER_ATTR_METHOD : LOAD_SUPER_ATTR_ATTR;
    specialize(instr, load_code);
    return;
fail:
    unspecialize(instr);
}

typedef enum {
    OVERRIDING, /* Is an overriding descriptor, and will remain so. */
    METHOD, /* Attribute has Py_TPFLAGS_METHOD_DESCRIPTOR set */
    PROPERTY, /* Is a property */
    OBJECT_SLOT, /* Is a strong VALUE_EX slot descriptor */
    POINTER_SLOT, /* Is a descriptor of a PyObject * field (_Py_T_OBJECT) */
    OTHER_SLOT, /* Is a slot descriptor of another type */
    NON_OVERRIDING, /* Is another non-overriding descriptor, and is an instance of an immutable class*/
    BUILTIN_CLASSMETHOD, /* Builtin methods with METH_CLASS */
    PYTHON_CLASSMETHOD, /* Python classmethod(func) object */
    NON_DESCRIPTOR, /* Is not a descriptor, and is an instance of an immutable class */
    INLINE_VALUE, /* Canonical class scalar: no object descriptor protocol. */
    MUTABLE,   /* Instance of a mutable class; might, or might not, be a descriptor */
    ABSENT, /* Attribute is not present on the class */
    DUNDER_CLASS, /* __class__ attribute */
    GETSET_OVERRIDDEN, /* __getattribute__ or __setattr__ has been overridden */
    GETATTRIBUTE_IS_PYTHON_FUNCTION  /* Descriptor requires calling a Python __getattribute__ */
} DescriptorClassification;


static DescriptorClassification
classify_descriptor(PyValue value, bool has_getattr)
{
    if (PyValue_IsNull(value)) {
        return ABSENT;
    }
    if (!PyValue_IsObject(value)) {
        return INLINE_VALUE;
    }
    PyObject *descriptor = PyValue_AsKnownObjectBorrow(value);
    PyTypeObject *desc_cls = Py_TYPE(descriptor);
    if (!(desc_cls->tp_flags & Py_TPFLAGS_IMMUTABLETYPE)) {
        return MUTABLE;
    }
    if (desc_cls->tp_descr_set) {
        if (desc_cls == &PyMemberDescr_Type) {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descriptor;
            struct PyMemberDef *dmem = member->d_member;
            if (dmem->type == _Py_T_VALUE_EX) {
                return OBJECT_SLOT;
            }
            if (dmem->type == _Py_T_OBJECT) {
                return POINTER_SLOT;
            }
            return OTHER_SLOT;
        }
        if (desc_cls == &PyProperty_Type) {
            /* We can't detect at runtime whether an attribute exists
               with property. So that means we may have to call
               __getattr__. */
            return has_getattr ? GETSET_OVERRIDDEN : PROPERTY;
        }
        return OVERRIDING;
    }
    if (desc_cls->tp_descr_get) {
        if (desc_cls->tp_flags & Py_TPFLAGS_METHOD_DESCRIPTOR) {
            return METHOD;
        }
        if (Py_IS_TYPE(descriptor, &PyClassMethodDescr_Type)) {
            return BUILTIN_CLASSMETHOD;
        }
        if (Py_IS_TYPE(descriptor, &PyClassMethod_Type)) {
            return PYTHON_CLASSMETHOD;
        }
        return NON_OVERRIDING;
    }
    return NON_DESCRIPTOR;
}

static bool
descriptor_is_class(PyValue descriptor, PyObject *name)
{
    if (PyUnicode_CompareWithASCIIString(name, "__class__") != 0) {
        return false;
    }
    PyValue base = PyValue_NULL;
    unsigned int version;
    int eligible = _PyType_TryLookupValueNoAlloc(&PyBaseObject_Type, name, &base, &version);
    bool same = eligible == 1 && PyValue_Is(descriptor, base);
    PyValue_XCLOSE(base);
    return same;
}

static DescriptorClassification
analyze_descriptor_load(PyTypeObject *type, PyObject *name,
                        PyValue *descr, unsigned int *tp_version)
{
    bool has_getattr = false;
    unsigned int ga_version = 0;
    getattrofunc slot = type->tp_getattro;
    if (slot == _Py_slot_tp_getattr_hook || slot == _Py_slot_tp_getattro) {
        PyValue getattribute = PyValue_NULL, getattr = PyValue_NULL;
        if (_PyType_TryLookupValueNoAlloc(type, &_Py_ID(__getattribute__),
                                        &getattribute, &ga_version) != 1 ||
            _PyType_TryLookupValueNoAlloc(type, &_Py_ID(__getattr__),
                                        &getattr, tp_version) != 1) {
            PyValue_XCLOSE(getattribute);
            PyValue_XCLOSE(getattr);
            return GETSET_OVERRIDDEN;
        }
        has_getattr = !PyValue_IsNull(getattr);
        PyValue_XCLOSE(getattr);
        bool custom = !PyValue_IsNull(getattribute) &&
            (!PyValue_IsObject(getattribute) ||
             PyValue_AsKnownObjectBorrow(getattribute) !=
                _PyInterpreterState_GET()->callable_cache.object__getattribute__);
        if (custom) {
            *tp_version = ga_version;
            if (!has_getattr && PyValue_IsObject(getattribute) &&
                PyFunction_Check(PyValue_AsKnownObjectBorrow(getattribute))) {
                *descr = getattribute;
                return GETATTRIBUTE_IS_PYTHON_FUNCTION;
            }
            PyValue_XCLOSE(getattribute);
            return GETSET_OVERRIDDEN;
        }
        PyValue_XCLOSE(getattribute);
    }
    else if (_PyType_VALUE_SLOT(type, getattro)
             != _PyObject_GenericGetAttrValue) {
        return GETSET_OVERRIDDEN;
    }
    if (_PyType_TryLookupValueNoAlloc(type, name, descr, tp_version) != 1) {
        return GETSET_OVERRIDDEN;
    }
    if (ga_version != 0) {
        *tp_version = ga_version;
    }
    if (descriptor_is_class(*descr, name)) {
        return DUNDER_CLASS;
    }
    return classify_descriptor(*descr, has_getattr);
}

static DescriptorClassification
analyze_descriptor_store(PyTypeObject *type, PyObject *name,
                         PyValue *descr, unsigned int *tp_version)
{
    if (_PyType_VALUE_SLOT(type, setattro) != _PyObject_GenericSetAttrValue ||
        _PyType_TryLookupValueNoAlloc(type, name, descr, tp_version) != 1) {
        return GETSET_OVERRIDDEN;
    }
    if (descriptor_is_class(*descr, name)) {
        return DUNDER_CLASS;
    }
    return classify_descriptor(*descr, false);
}

#ifdef Py_SHAPES
/* Whether the site specialised for another shape of the same type before,
 * which the poly table that fb_begin() filled records. */
static bool
fb_saw_other_shape(_PyInstr *instr, unsigned int tp_version, uint8_t shape)
{
    _PyFbHeader *h = instr_fb(instr);
    if (h->poly == 0) {
        return false;
    }
    _PyFbPoly *poly = _PyFb_PolyOf(fb_block(instr), h);
    for (int i = 0; i < FB_POLY_SIZE && poly->handler[i] != 0; i++) {
        int opcode = poly->handler[i];
        if ((opcode == ROP_HANDLER_GetAttr_Shape || opcode == STORE_ATTR_SHAPE ||
             opcode == STORE_ATTR_SHAPE_ADD) &&
            poly->type_version[i] == tp_version && poly->aux[i] != shape)
        {
            return true;
        }
    }
    return false;
}

/* Fill in the shape fields of FB for an access to slot INDEX of OWNER, and
 * return the shape opcode to use, or 0 if OWNER has no shape or, for a load,
 * lacks the slot.  A site that sees several shapes of one type, as with
 * attributes that only some instances set, keeps to the instance value
 * handlers, which check presence instead of the shape. */
static int
specialize_shape_access(PyObject *owner, _PyInstr *instr, _PyFbAttr *fb,
                        PyTypeObject *type, unsigned int tp_version,
                        Py_ssize_t index, int base_op)
{
    PyDictValues *values = _PyObject_InlineValues(owner);
    uint8_t shape = values->shape;
    if (shape == _Py_SHAPE_NONE || fb_saw_other_shape(instr, tp_version, shape)) {
        return 0;
    }
    int opcode;
    if (_PyShape_HasSlot(type, shape, index)) {
        opcode = base_op == LOAD_ATTR ? ATTR_KIND(ATTR_SHAPE) : STORE_ATTR_SHAPE;
        if (base_op == LOAD_ATTR) {
            fb->read_kind = _Py_FB_READ_SHAPE;
        }
    }
    else if (base_op == LOAD_ATTR) {
        return 0;
    }
    else {
        uint8_t shape_to = _PyShape_FindTransition(type, shape, index);
        if (shape_to == _Py_SHAPE_NONE) {
            return 0;
        }
        fb->shape_to = shape_to;
        opcode = STORE_ATTR_SHAPE_ADD;
    }
    fb->shape = shape;
    return opcode;
}
#endif

static int
specialize_dict_access_inline(
    PyObject *owner, _PyInstr *instr, PyTypeObject *type,
    PyObject *name, unsigned int tp_version,
    int base_op, int values_op)
{
    _PyFbAttr *fb = instr_entry(instr);
    PyDictKeysObject *keys = ((PyHeapTypeObject *)type)->ht_cached_keys;
    assert(PyUnicode_CheckExact(name));
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(owner);
    Py_ssize_t index = _PyDictKeys_StringLookupSplit(keys, name);
    assert (index != DKIX_ERROR);
    if (index == DKIX_EMPTY) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NOT_IN_KEYS);
        return 0;
    }
    assert(index >= 0);
    // Once a dict exists, its lock (not the owner's) guards the values, so
    // they may be invalidated concurrently. The instruction rechecks them.
    assert(_PyObject_GetManagedDict(owner) != NULL ||
           _PyObject_InlineValues(owner)->valid);
    char *value_addr = (char *)&_PyObject_InlineValues(owner)->values[index];
    fb->offset = (uint32_t)(value_addr - (char *)owner);
    fb->type_version = tp_version;
#ifdef Py_SHAPES
    int shape_op = specialize_shape_access(owner, instr, fb, type, tp_version,
                                           index, base_op);
    if (shape_op != 0) {
        specialize(instr, shape_op);
        return 1;
    }
#endif
    specialize(instr, values_op);
    return 1;
}

static int
specialize_dict_access_hint(
    PyDictObject *dict, _PyInstr *instr, PyTypeObject *type,
    PyObject *name, unsigned int tp_version,
    int base_op, int hint_op)
{
    _PyFbAttr *fb = instr_entry(instr);

    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(dict);

    // We found an instance with a __dict__.
    // STORE_ATTR_WITH_HINT can write the values of a split table, such as a
    // materialized dict that shares the inline values; ATTR_KIND(ATTR_WITH_HINT)
    // reads combined tables only.
    if (base_op == LOAD_ATTR && _PyDict_HasSplitTable(dict)) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_SPLIT_DICT);
        return 0;
    }
    /* The executing hint cannot use a general-key table. Declining here
     * also keeps metadata lookup native and nonescaping before cache writes. */
    if (dict->ma_keys->dk_kind == DICT_KEYS_GENERAL) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NON_STRING);
        return 0;
    }
    PyValue value = PyValue_NULL;
    Py_ssize_t index = _PyDict_LookupIndexAndOwnedValue(dict, name, &value);
    if (PyValue_IsNull(value) && index >= 0) {
        // A split table's shared keys can hold names this dict lacks.
        index = DKIX_EMPTY;
    }
    PyValue_XCLOSE(value);
    if (index < 0) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NOT_IN_DICT);
        return 0;
    }
    fb->hint = (uint32_t)index;
    fb->type_version = tp_version;
    specialize(instr, hint_op);
    return 1;
}


static int
specialize_dict_access(
    PyObject *owner, _PyInstr *instr, PyTypeObject *type,
    DescriptorClassification kind, PyObject *name, unsigned int tp_version,
    int base_op, int values_op, int hint_op)
{
    assert(kind == NON_OVERRIDING || kind == NON_DESCRIPTOR || kind == ABSENT ||
        kind == BUILTIN_CLASSMETHOD || kind == PYTHON_CLASSMETHOD ||
        kind == METHOD || kind == INLINE_VALUE);
    // No descriptor, or non overriding.
    if ((type->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        SPECIALIZATION_FAIL(base_op, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
        return 0;
    }
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES &&
        FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(owner)->valid) &&
        !(base_op == STORE_ATTR && _PyObject_GetManagedDict(owner) != NULL))
    {
        int res;
        Py_BEGIN_CRITICAL_SECTION(owner);
        PyDictObject *dict = _PyObject_GetManagedDict(owner);
        // A materialized dict shares the inline values, so loads can still
        // read them directly. Stores must go through the dict.
        if (dict == NULL || base_op == LOAD_ATTR) {
            res = specialize_dict_access_inline(owner, instr, type, name,
                                                tp_version, base_op, values_op);
        }
        else {
            // lost race and dict was created, fail specialization
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OTHER);
            res = 0;
        }
        Py_END_CRITICAL_SECTION();
        return res;
    }
    else {
        PyDictObject *dict = _PyObject_GetManagedDict(owner);
        if (dict == NULL || !PyDict_CheckExact(dict)) {
            SPECIALIZATION_FAIL(base_op, SPEC_FAIL_NO_DICT);
            return 0;
        }
        int res;
        Py_BEGIN_CRITICAL_SECTION(dict);
        // materialized managed dict
        res = specialize_dict_access_hint(dict, instr, type, name,
                                          tp_version, base_op, hint_op);
        Py_END_CRITICAL_SECTION();
        return res;
    }
}

static int
specialize_attr_loadclassattr(PyObject *owner, _PyInstr *instr,
                              PyObject *name, PyValueBorrow descriptor,
                              unsigned int tp_version,
                              DescriptorClassification kind, bool is_method,
                              uint32_t shared_keys_version);
static int specialize_class_load_attr(PyObject* owner, _PyInstr *instr, PyObject* name,
                                      bool method);

/* Returns true if instances of obj's class are
 * likely to have `name` in their __dict__.
 * For objects with inline values, we check in the shared keys.
 * For other objects, we check their actual dictionary.
 */
static bool
instance_has_key(PyObject *obj, PyObject *name, uint32_t *shared_keys_version)
{
    PyTypeObject *cls = Py_TYPE(obj);
    if ((cls->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return false;
    }
    if (cls->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        PyDictKeysObject *keys = ((PyHeapTypeObject *)cls)->ht_cached_keys;
        Py_ssize_t index =
            _PyDictKeys_StringLookupAndVersion(keys, name, shared_keys_version);
        return index >= 0;
    }
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL || !PyDict_CheckExact(dict)) {
        return false;
    }
    bool result;
    Py_BEGIN_CRITICAL_SECTION(dict);
    if (dict->ma_values) {
        result = false;
    }
    else if (dict->ma_keys->dk_kind == DICT_KEYS_UNICODE) {
        result = (_PyDict_LookupIndex(dict, name) >= 0);
    }
    else {
        result = false;
    }
    Py_END_CRITICAL_SECTION();
    return result;
}

/* ATTR_KIND(ATTR_GETSET): the descriptor is a getset with a value getter, which
 * the handler calls directly. The type version guard proves that the lookup
 * still finds this descriptor, and that the owner is an instance of the type
 * it applies to, which is checked here. */
static bool
specialize_value_getset(PyObject *owner, _PyInstr *instr, PyObject *descr,
                        unsigned int tp_version)
{
    if (descr == NULL || !Py_IS_TYPE(descr, &PyGetSetDescr_Type)) {
        return false;
    }
    PyGetSetDescrObject *getset = (PyGetSetDescrObject *)descr;
    if (getset->d_value == NULL ||
        !PyObject_TypeCheck(owner, getset->d_common.d_type)) {
        return false;
    }
    #ifdef Py_GIL_DISABLED
    if (!_PyObject_HasDeferredRefcount(descr)) {
        return false;
    }
    #endif
    _PyFbAttr *fb = instr_entry(instr);
    fb->type_version = tp_version;
    fb->class_value = _PyValueGuarded_FromBorrow(PyValue_FromObjectBorrow(descr));
    specialize(instr, ATTR_KIND(ATTR_GETSET));
    return true;
}

static int
do_specialize_instance_load_attr(PyObject* owner, _PyInstr *instr, PyObject* name,
                                 bool method, bool shadow, uint32_t shared_keys_version,
                                 DescriptorClassification kind, PyValue descriptor, unsigned int tp_version)
{
    PyObject *descr = PyValue_IsObject(descriptor) ? PyValue_AsKnownObjectBorrow(descriptor) : NULL;
    _PyFbAttr *fb = instr_entry(instr);
    PyTypeObject *type = Py_TYPE(owner);
    if (tp_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    switch(kind) {
        case OVERRIDING:
            if (specialize_value_getset(owner, instr, descr, tp_version)) {
                return 0;
            }
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            return -1;
        case METHOD:
        {
            if (shadow) {
                goto try_instance;
            }
            if (method) {
                if (specialize_attr_loadclassattr(owner, instr, name,
                                                  PyValue_Borrow(descriptor),
                                                  tp_version, kind, true,
                                                  shared_keys_version)) {
                    return 0;
                }
                else {
                    return -1;
                }
            }
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METHOD);
            return -1;
        }
        case PROPERTY:
            // A property pushes a frame: the site stays on the generic arm
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_PROPERTY);
            return -1;
        case OBJECT_SLOT:
        case POINTER_SLOT:
        {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descr;
            struct PyMemberDef *dmem = member->d_member;
            Py_ssize_t offset = dmem->offset;
            if (!PyObject_TypeCheck(owner, member->d_common.d_type)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                return -1;
            }
            if (dmem->flags & _Py_AFTER_ITEMS) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_SLOT_AFTER_ITEMS);
                return -1;
            }
            if (dmem->flags & Py_AUDIT_READ) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_AUDITED_SLOT);
                return -1;
            }
            if (offset != (uint32_t)offset) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                return -1;
            }
            assert(offset > 0);
            fb->index = (uint32_t)offset;
            fb->type_version = tp_version;
            if (kind == OBJECT_SLOT) {
                assert(dmem->type == _Py_T_VALUE_EX);
                specialize(instr, ATTR_KIND(ATTR_SLOT));
            }
            else {
                assert(dmem->type == _Py_T_OBJECT);
                specialize(instr, ATTR_KIND(ATTR_SLOT_POINTER));
            }
            return 0;
        }
        case DUNDER_CLASS:
            /* ob_type is a PyObject * field of every object: the same read,
             * found by the type's version, which fixes the owner's type. */
            fb->index = (uint32_t)offsetof(PyObject, ob_type);
            fb->type_version = tp_version;
            specialize(instr, ATTR_KIND(ATTR_SLOT_POINTER));
            return 0;
        case OTHER_SLOT:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NON_OBJECT_SLOT);
            return -1;
        case MUTABLE:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
            return -1;
        case GETSET_OVERRIDDEN:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OVERRIDDEN);
            return -1;
        case GETATTRIBUTE_IS_PYTHON_FUNCTION:
            // An overridden __getattribute__ pushes a frame, as a property does
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OVERRIDDEN);
            return -1;
        case BUILTIN_CLASSMETHOD:
        case PYTHON_CLASSMETHOD:
        case NON_OVERRIDING:
            if (shadow) {
                goto try_instance;
            }
            return -1;
        case INLINE_VALUE:
        case NON_DESCRIPTOR:
            if (shadow) {
                goto try_instance;
            }
            if (!method) {
                if (specialize_attr_loadclassattr(owner, instr, name,
                                                  PyValue_Borrow(descriptor),
                                                  tp_version, kind, false,
                                                  shared_keys_version)) {
                    return 0;
                }
            }
            return -1;
        case ABSENT:
            if (shadow) {
                goto try_instance;
            }
            // The row reports "none" and the macro backs off
            return 0;
    }
    Py_UNREACHABLE();
try_instance:
    if (specialize_dict_access(owner, instr, type, kind, name, tp_version,
                               LOAD_ATTR, ATTR_KIND(ATTR_INSTANCE_VALUE), ATTR_KIND(ATTR_WITH_HINT)))
    {
        return 0;
    }
    return -1;
}

static int
specialize_instance_load_attr(PyObject* owner, _PyInstr *instr, PyObject* name,
                              bool method)
{
    // 0 is not a valid version
    PyValue descr = PyValue_NULL;
    unsigned int tp_version = 0;
    PyTypeObject *type = Py_TYPE(owner);
    // Read the type version before the keys version, we could have a concurrent
    // modification of the split keys in which case we update the keys version and
    // then the type version, this ensures we will still deopt if that happens.
    DescriptorClassification kind = analyze_descriptor_load(type, name, &descr, &tp_version);
    uint32_t shared_keys_version = 0;
    bool shadow = instance_has_key(owner, name, &shared_keys_version);
    int result = do_specialize_instance_load_attr(owner, instr, name, method, shadow, shared_keys_version, kind, descr, tp_version);
    PyValue_XCLOSE(descr);
    return result;
}

/* Specialize the site of a GetAttr (method false) or GetMethod (true) word for
 * the attribute name of owner_st, reporting the kind found in attr_collect. */
static void
attr_specialize(PyValueBorrow owner_st, _PyInstr *instr, PyObject *name,
                bool method)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[LOAD_ATTR] == INLINE_CACHE_ENTRIES_LOAD_ATTR);
    if (!PyValue_IsObject(owner_st)) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *owner = PyValue_AsKnownObjectBorrow(owner_st);
    PyTypeObject *type = Py_TYPE(owner);
    PyValue proof = PyValue_NULL;
    unsigned int version;
    int eligible = _PyType_TryLookupValueNoAlloc(type, name, &proof, &version);
    PyValue_XCLOSE(proof);
    if (eligible != 1) {
        unspecialize(instr);
        return;
    }
    if (PyType_Check(owner)) {
        eligible = _PyType_TryLookupValueNoAlloc((PyTypeObject *)owner, name, &proof, &version);
        PyValue_XCLOSE(proof);
        if (eligible != 1) {
            unspecialize(instr);
            return;
        }
    }
    fb_begin(instr);
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
        fail = specialize_class_load_attr(owner, instr, name, method);
    }
    else {
        fail = specialize_instance_load_attr(owner, instr, name, method);
    }

    if (fail) {
        unspecialize(instr);
    }
}

Py_NO_INLINE void
(_Py_Specialize_StoreAttr)(PyValueBorrow owner_st, _PyInstr *instr, PyObject *name)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[STORE_ATTR] == INLINE_CACHE_ENTRIES_STORE_ATTR);
    PyValue descr = PyValue_NULL;
    _PyFbAttr *fb = NULL;
    if (!PyValue_IsObject(owner_st)) {
        SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *owner = PyValue_AsKnownObjectBorrow(owner_st);
    PyTypeObject *type = Py_TYPE(owner);
    PyValue proof = PyValue_NULL;
    unsigned int version;
    int eligible = _PyType_TryLookupValueNoAlloc(type, name, &proof, &version);
    PyValue_XCLOSE(proof);
    if (eligible != 1) {
        unspecialize(instr);
        return;
    }
    fb = (_PyFbAttr *)fb_begin(instr);
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
    unsigned int tp_version = 0;
    DescriptorClassification kind = analyze_descriptor_store(type, name, &descr, &tp_version);
    if (tp_version == 0) {
        goto fail;
    }
    PyObject *descr_o = PyValue_IsObject(descr) ? PyValue_AsKnownObjectBorrow(descr) : NULL;
    assert(!PyValue_IsNull(descr) || kind == ABSENT || kind == GETSET_OVERRIDDEN);
    switch(kind) {
        case OVERRIDING:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
            goto fail;
        case METHOD:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_METHOD);
            goto fail;
        case PROPERTY:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_PROPERTY);
            goto fail;
        case OBJECT_SLOT:
        {
            PyMemberDescrObject *member = (PyMemberDescrObject *)descr_o;
            struct PyMemberDef *dmem = member->d_member;
            Py_ssize_t offset = dmem->offset;
            if (!PyObject_TypeCheck(owner, member->d_common.d_type)) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_EXPECTED_ERROR);
                goto fail;
            }
            if (dmem->flags & _Py_AFTER_ITEMS) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_SLOT_AFTER_ITEMS);
                goto fail;
            }
            if (dmem->flags & Py_READONLY) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_READ_ONLY);
                goto fail;
            }
            if (offset != (uint32_t)offset) {
                SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                goto fail;
            }
            assert(dmem->type == _Py_T_VALUE_EX);
            assert(offset > 0);
            fb->index = (uint32_t)offset;
            fb->type_version = tp_version;
            specialize(instr, STORE_ATTR_SLOT);
            goto success;
        }
        case DUNDER_CLASS:
        case POINTER_SLOT:
        case OTHER_SLOT:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_NON_OBJECT_SLOT);
            goto fail;
        case MUTABLE:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
            goto fail;
        case GETATTRIBUTE_IS_PYTHON_FUNCTION:
        case GETSET_OVERRIDDEN:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_OVERRIDDEN);
            goto fail;
        case BUILTIN_CLASSMETHOD:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD_OBJ);
            goto fail;
        case PYTHON_CLASSMETHOD:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_CLASS_METHOD_OBJ);
            goto fail;
        case NON_OVERRIDING:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_CLASS_ATTR_DESCRIPTOR);
            goto fail;
        case INLINE_VALUE:
        case NON_DESCRIPTOR:
            SPECIALIZATION_FAIL(STORE_ATTR, SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE);
            goto fail;
        case ABSENT:
            if (specialize_dict_access(owner, instr, type, kind, name, tp_version,
                                       STORE_ATTR, STORE_ATTR_INSTANCE_VALUE,
                                       STORE_ATTR_WITH_HINT)) {
                goto success;
            }
    }
fail:
    PyValue_XCLOSE(descr);
    unspecialize(instr);
    return;
success:
    PyValue_XCLOSE(descr);
    return;
}

/* Each LOAD_ATTR handler the specializer can nominate, the arm of the
 * register families that replaces it and the position of that arm among the
 * cases of classify_getattr (GetAttr words) or classify_getmethod (GetMethod
 * words); -1 where the family has no such arm.  The last case of each row is
 * "none".  A nomination of a handler without an arm (a property, an
 * overridden __getattribute__, a getset in method position) is "none": the site stays generic. */
typedef struct {
    int kind;
    int arm;
    int getattr_case;
    int getmethod_case;
} attr_arm;

static const attr_arm attr_arms[] = {
    {ATTR_INSTANCE_VALUE, ROP_HANDLER_GetAttr_Instance, 0, -1},
    {ATTR_SHAPE, ROP_HANDLER_GetAttr_Shape, 1, -1},
    {ATTR_MODULE, ROP_HANDLER_GetAttr_Module, 2, -1},
    {ATTR_WITH_HINT, ROP_HANDLER_GetAttr_Hint, 3, -1},
    {ATTR_SLOT, ROP_HANDLER_GetAttr_ValueSlot, 4, -1},
    {ATTR_SLOT_POINTER, ROP_HANDLER_GetAttr_PointerSlot, 5, -1},
    {ATTR_CLASS, ROP_HANDLER_GetAttr_Class, 6, -1},
    {ATTR_CLASS_WITH_METACLASS_CHECK, ROP_HANDLER_GetAttr_Class, 6, -1},
    {ATTR_NONDESCRIPTOR_WITH_VALUES, ROP_HANDLER_GetAttr_NonDescriptorValues, 7, -1},
    {ATTR_NONDESCRIPTOR_NO_DICT, ROP_HANDLER_GetAttr_NonDescriptorNoDict, 8, -1},
    {ATTR_GETSET, ROP_HANDLER_GetAttr_GetSet, 9, -1},
    {ATTR_METHOD_WITH_VALUES, ROP_HANDLER_GetMethod_MethodValues, -1, 0},
    {ATTR_METHOD_NO_DICT, ROP_HANDLER_GetMethod_NoDict, -1, 1},
    {ATTR_METHOD_LAZY_DICT, ROP_HANDLER_GetMethod_LazyDict, -1, 2},
    {ATTR_CLASSMETHOD, ROP_HANDLER_GetMethod_ClassMethod, -1, 3},
};

static const attr_arm *
attr_arm_of(int kind, bool method)
{
    for (size_t i = 0; i < Py_ARRAY_LENGTH(attr_arms); i++) {
        const attr_arm *a = &attr_arms[i];
        if (a->kind == kind && (method ? a->getmethod_case : a->getattr_case) >= 0) {
            return a;
        }
    }
    return NULL;
}

/* Put back the payload of an entry, keeping its header: the counter and
 * the state belong to the adaptation. */
static void
attr_restore_entry(_PyFbAttr *fb, const _PyFbAttr *before)
{
    memcpy((char *)fb + sizeof(_PyFbHeader),
           (const char *)before + sizeof(_PyFbHeader),
           sizeof(_PyFbAttr) - sizeof(_PyFbHeader));
}

static int
attr_classify(PyValueBorrow owner, _PyInstr *site, const _PyFbAttr *fb,
              PyObject *name, bool method, _PyAttrRecipe *recipe)
{
    assert(attr_collect == NULL);
    attr_nomination nominated = {0};
    /* The attribute specializer writes its findings into the entry as it
     * goes, and an arm reads the entry.  A classification that installs
     * nothing must leave the entry as the site's arm expects it. */
    _PyFbAttr before = *fb;
    attr_collect = &nominated;
    attr_specialize(owner, site, name, method);
    attr_collect = NULL;
    int none = method ? 4 : 10;
    const attr_arm *chosen = attr_arm_of(nominated.kind, method);
    if (chosen == NULL) {
        attr_restore_entry((_PyFbAttr *)fb, &before);
        return none;
    }
    // The first poly observation saves what the entry described before
    fb_record(site, (_PyFbHeader *)fb, chosen->arm);
    int kind = method ? chosen->getmethod_case : chosen->getattr_case;
    bool admitted;
    switch (chosen->arm) {
        case ROP_HANDLER_GetAttr_Instance:
            admitted = _PyAttr_AdmittedInstanceSlot(fb, name, recipe);
            break;
        case ROP_HANDLER_GetAttr_Shape:
            admitted = _PyAttr_AdmittedShape(fb, name, recipe);
            break;
        case ROP_HANDLER_GetMethod_MethodValues:
            admitted = _PyAttr_AdmittedMethod(fb, name, recipe);
            break;
        default:
            admitted = _PyAttr_AdmittedRecipe(fb, name, recipe);
            break;
    }
    if (!admitted) {
        attr_restore_entry((_PyFbAttr *)fb, &before);
        return none;
    }
    return kind;
}

int
_PyAttr_ClassifyGetAttr(PyValueBorrow owner, _PyInstr *site, const _PyFbAttr *fb,
                        PyObject *name, _PyAttrRecipe *recipe)
{
    return attr_classify(owner, site, fb, name, false, recipe);
}

int
_PyAttr_ClassifyGetMethod(PyValueBorrow owner, _PyInstr *site, const _PyFbAttr *fb,
                          PyObject *name, _PyAttrRecipe *recipe)
{
    return attr_classify(owner, site, fb, name, true, recipe);
}

bool
_PyAttr_PolyShapeReady(_PyInstr *site, const _PyFbAttr *fb)
{
    return fb_shape_read_set(site, (_PyFbHeader *)fb) != 0;
}

bool
_PyAttr_PolyMethodReady(_PyInstr *site, const _PyFbAttr *fb)
{
    return fb_method_poly_set(site, (_PyFbHeader *)fb) != 0;
}

#ifdef Py_STATS
static int
load_attr_fail_kind(DescriptorClassification kind)
{
    switch (kind) {
        case OVERRIDING:
            return SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR;
        case METHOD:
            return SPEC_FAIL_ATTR_METHOD;
        case PROPERTY:
            return SPEC_FAIL_ATTR_PROPERTY;
        case OBJECT_SLOT:
            return SPEC_FAIL_ATTR_OBJECT_SLOT;
        case POINTER_SLOT:
        case OTHER_SLOT:
            return SPEC_FAIL_ATTR_NON_OBJECT_SLOT;
        case DUNDER_CLASS:
            return SPEC_FAIL_OTHER;
        case MUTABLE:
            return SPEC_FAIL_ATTR_MUTABLE_CLASS;
        case GETSET_OVERRIDDEN:
        case GETATTRIBUTE_IS_PYTHON_FUNCTION:
            return SPEC_FAIL_OVERRIDDEN;
        case BUILTIN_CLASSMETHOD:
            return SPEC_FAIL_ATTR_BUILTIN_CLASS_METHOD;
        case PYTHON_CLASSMETHOD:
            return SPEC_FAIL_ATTR_CLASS_METHOD_OBJ;
        case NON_OVERRIDING:
            return SPEC_FAIL_ATTR_NON_OVERRIDING_DESCRIPTOR;
        case INLINE_VALUE:
        case NON_DESCRIPTOR:
            return SPEC_FAIL_ATTR_NOT_DESCRIPTOR;
        case ABSENT:
            return SPEC_FAIL_ATTR_INSTANCE_ATTRIBUTE;
    }
    Py_UNREACHABLE();
}
#endif   // Py_STATS

static int
specialize_class_load_attr(PyObject *owner, _PyInstr *instr,
                             PyObject *name, bool method)
{
    assert(PyType_Check(owner));
    PyTypeObject *cls = (PyTypeObject *)owner;
    _PyFbAttr *fb = instr_entry(instr);
    if (_PyType_VALUE_SLOT(Py_TYPE(cls), getattro) != _Py_type_getattro_value) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METACLASS_OVERRIDDEN);
        return -1;
    }
    unsigned int meta_version = 0;
    PyValue metadescriptor = PyValue_NULL;
    if (_PyType_TryLookupValueNoAlloc(Py_TYPE(cls), name, &metadescriptor, &meta_version) != 1) {
        return -1;
    }
    DescriptorClassification metakind = classify_descriptor(metadescriptor, false);
    PyValue_XCLOSE(metadescriptor);
    switch (metakind) {
        case METHOD:
        case INLINE_VALUE:
        case NON_DESCRIPTOR:
        case NON_OVERRIDING:
        case BUILTIN_CLASSMETHOD:
        case PYTHON_CLASSMETHOD:
        case ABSENT:
            break;
        default:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_METACLASS_ATTRIBUTE);
            return -1;
    }
    PyValue descriptor = PyValue_NULL;
    DescriptorClassification kind = 0;
    unsigned int tp_version = 0;
    kind = analyze_descriptor_load(cls, name, &descriptor, &tp_version);
    PyObject *descr = PyValue_IsObject(descriptor) ? PyValue_AsKnownObjectBorrow(descriptor) : NULL;
    if (tp_version == 0) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
        PyValue_XCLOSE(descriptor);
        return -1;
    }
    bool metaclass_check = false;
    if ((Py_TYPE(cls)->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) == 0) {
        metaclass_check = true;
        if (meta_version == 0) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
            PyValue_XCLOSE(descriptor);
            return -1;
        }
    }
    int opcode = metaclass_check ? ATTR_KIND(ATTR_CLASS_WITH_METACLASS_CHECK)
                                    : ATTR_KIND(ATTR_CLASS);
    switch (kind) {
        case MUTABLE:
            // special case for enums which has Py_TYPE(descr) == cls
            // so guarding on type version is sufficient
            if (Py_TYPE(descr) != cls) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_MUTABLE_CLASS);
                PyValue_XCLOSE(descriptor);
                return -1;
            }
            if (Py_TYPE(descr)->tp_descr_get || Py_TYPE(descr)->tp_descr_set) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_OVERRIDING_DESCRIPTOR);
                PyValue_XCLOSE(descriptor);
                return -1;
            }
            _Py_FALLTHROUGH;
        case METHOD:
        case NON_DESCRIPTOR:
        case INLINE_VALUE:
#ifdef Py_GIL_DISABLED
            if (descr != NULL) {
                maybe_enable_deferred_ref_count(descr);
            }
#endif
            break;
        case PYTHON_CLASSMETHOD: {
#ifdef Py_GIL_DISABLED
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OTHER);
            PyValue_XCLOSE(descriptor);
            return -1;
#else
            if (!method || Py_TYPE(cls) != &PyType_Type) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_CLASS_METHOD_OBJ);
                PyValue_XCLOSE(descriptor);
                return -1;
            }
            assert(Py_IS_TYPE(descr, &PyClassMethod_Type));
            PyObject *target = _PyClassMethod_GetFunc(descr);
            if (target == NULL || !Py_IS_TYPE(target, &PyFunction_Type)) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_CLASS_METHOD_OBJ);
                PyValue_XCLOSE(descriptor);
                return -1;
            }
            opcode = ATTR_KIND(ATTR_CLASSMETHOD);
            break;
#endif
        }
#ifdef Py_STATS
        case ABSENT:
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_EXPECTED_ERROR);
            PyValue_XCLOSE(descriptor);
            return -1;
#endif
        default:
            SPECIALIZATION_FAIL(LOAD_ATTR, load_attr_fail_kind(kind));
            PyValue_XCLOSE(descriptor);
            return -1;
    }
    fb->class_value = _PyValueGuarded_FromBorrow(PyValue_Borrow(descriptor));
    fb->class_version = tp_version;
    if (metaclass_check) {
        fb->type_version = meta_version;
    }
    specialize(instr, opcode);
    PyValue_XCLOSE(descriptor);
    return 0;
}

// Please collect stats carefully before and after modifying. A subtle change
// can cause a significant drop in cache hits. A possible test is
// python.exe -m test_typing test_re test_dis test_zlib.
static int
specialize_attr_loadclassattr(PyObject *owner, _PyInstr *instr,
                              PyObject *name, PyValueBorrow descriptor,
                              unsigned int tp_version,
                              DescriptorClassification kind, bool is_method,
                              uint32_t shared_keys_version)
{
    _PyFbAttr *fb = instr_entry(instr);
    PyTypeObject *owner_cls = Py_TYPE(owner);

    assert(!PyValue_IsNull(descriptor));
    assert((is_method && kind == METHOD) ||
           (!is_method && (kind == NON_DESCRIPTOR || kind == INLINE_VALUE)));

    #ifdef Py_GIL_DISABLED
    if (PyValue_IsObject(descriptor) &&
        !_PyObject_HasDeferredRefcount(PyValue_AsKnownObjectBorrow(descriptor))) {
        SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_DESCR_NOT_DEFERRED);
        return 0;
    }
    #endif

    int opcode;
    unsigned long tp_flags = PyType_GetFlags(owner_cls);
    if (tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        #ifndef Py_GIL_DISABLED
        assert(_PyDictKeys_StringLookup(
                   ((PyHeapTypeObject *)owner_cls)->ht_cached_keys, name) < 0);
        #endif
        if (shared_keys_version == 0) {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_VERSIONS);
            return 0;
        }
        opcode = is_method ? ATTR_KIND(ATTR_METHOD_WITH_VALUES) : ATTR_KIND(ATTR_NONDESCRIPTOR_WITH_VALUES);
    }
    else {
        Py_ssize_t dictoffset;
        if (tp_flags & Py_TPFLAGS_MANAGED_DICT) {
            dictoffset = MANAGED_DICT_OFFSET;
        }
        else {
            dictoffset = owner_cls->tp_dictoffset;
            if (dictoffset < 0 || dictoffset > INT32_MAX) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_OUT_OF_RANGE);
                return 0;
            }
        }
        if (dictoffset == 0) {
            opcode = is_method ? ATTR_KIND(ATTR_METHOD_NO_DICT) : ATTR_KIND(ATTR_NONDESCRIPTOR_NO_DICT);
        }
        else if (is_method) {
            PyObject **addr = (PyObject **)((char *)owner + dictoffset);
            PyObject *dict = FT_ATOMIC_LOAD_PTR_ACQUIRE(*addr);
            if (dict) {
                SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_NOT_MANAGED_DICT);
                return 0;
            }
            /* Feedback fields are unsigned, so we offset the dictoffset by
             * MANAGED_DICT_OFFSET.
             * We do the reverse offset in ATTR_KIND(ATTR_METHOD_LAZY_DICT) */
            fb->dictoffset = (uint32_t)(dictoffset - MANAGED_DICT_OFFSET);
            opcode = ATTR_KIND(ATTR_METHOD_LAZY_DICT);
        }
        else {
            SPECIALIZATION_FAIL(LOAD_ATTR, SPEC_FAIL_ATTR_CLASS_ATTR_SIMPLE);
            return 0;
        }
    }
    /* The class or an MRO base owns this non-owning payload. Type mutation
     * invalidates the receiver's version before releasing that owner. Adding
     * an absent shared instance key also invalidates its owning type first.
     * A hit must acquire its result before receiver retirement can reenter. */
    fb->type_version = tp_version;
    fb->class_value = _PyValueGuarded_FromBorrow(descriptor);
    specialize(instr, opcode);
    return 1;
}

/* Evidence for GlobalRead (Python/register_ops/names.rop): which arm the
 * site's name calls for, and its recipe.  The entry takes the versions and
 * the index only when an arm is nominated.  Both dictionaries are watched, so
 * that a write to a global or a builtin invalidates the keys version. */
enum { GLOBAL_MODULE, GLOBAL_BUILTIN, GLOBAL_NONE };

static int
global_classify_lock_held(PyObject *globals, PyObject *builtins, _PyFbGlobal *fb,
                          PyObject *name, _PyGlobalRecipe *recipe)
{
    PyValue value = PyValue_NULL;
    assert(PyUnicode_CheckExact(name));
    int result = GLOBAL_NONE;
    if (!PyDict_CheckExact(globals)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_DICT);
        goto done;
    }
    PyDictKeysObject * globals_keys = ((PyDictObject *)globals)->ma_keys;
    if (globals_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto done;
    }
    Py_ssize_t index = _PyDict_LookupIndexAndOwnedValue((PyDictObject *)globals, name, &value);
    if (index == DKIX_ERROR) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_EXPECTED_ERROR);
        goto done;
    }
    if (PyValue_IsObject(value) &&
        PyLazyImport_CheckExact(PyValue_AsKnownObjectBorrow(value))) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_ATTR_MODULE_LAZY_VALUE);
        goto done;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (index != DKIX_EMPTY) {
        uint32_t keys_version = _PyDict_GetKeysVersionForCurrentState(
                interp, (PyDictObject*) globals);
        if (keys_version == 0) {
            SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
            goto done;
        }
        PyDict_Watch(MODULE_WATCHER_ID, globals);
        fb->index = (uint32_t)index;
        fb->globals_version = keys_version;
        fb->builtins_version = 0;
        result = GLOBAL_MODULE;
        goto done;
    }
    if (!PyDict_CheckExact(builtins)) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_DICT);
        goto done;
    }
    PyDictKeysObject * builtin_keys = ((PyDictObject *)builtins)->ma_keys;
    if (builtin_keys->dk_kind != DICT_KEYS_UNICODE) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_LOAD_GLOBAL_NON_STRING_OR_SPLIT);
        goto done;
    }
    assert(PyValue_IsNull(value));
    index = _PyDict_LookupIndexAndOwnedValue((PyDictObject *)builtins, name, &value);
    if (index == DKIX_ERROR) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_EXPECTED_ERROR);
        goto done;
    }
    if (PyValue_IsObject(value) &&
        PyLazyImport_CheckExact(PyValue_AsKnownObjectBorrow(value))) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_ATTR_MODULE_LAZY_VALUE);
        goto done;
    }
    if (index == DKIX_EMPTY) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_EXPECTED_ERROR);
        goto done;
    }
    uint32_t globals_version = _PyDict_GetKeysVersionForCurrentState(
            interp, (PyDictObject*) globals);
    if (globals_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto done;
    }
    PyDict_Watch(MODULE_WATCHER_ID, globals);
    uint32_t builtins_version = _PyDict_GetKeysVersionForCurrentState(
            interp, (PyDictObject*) builtins);
    if (builtins_version == 0) {
        SPECIALIZATION_FAIL(LOAD_GLOBAL, SPEC_FAIL_OUT_OF_VERSIONS);
        goto done;
    }
    PyDict_Watch(MODULE_WATCHER_ID, builtins);
    fb->index = (uint32_t)index;
    fb->globals_version = globals_version;
    fb->builtins_version = builtins_version;
    result = GLOBAL_BUILTIN;
done:
    PyValue_XCLOSE(value);
    if (result == GLOBAL_MODULE) {
        (void)_PyGlobal_AdmittedModule(fb, recipe);
    }
    else if (result == GLOBAL_BUILTIN) {
        (void)_PyGlobal_AdmittedBuiltin(fb, recipe);
    }
    return result;
}

int
_PyGlobal_Classify(_PyInterpreterFrame *frame, _PyInstr *site,
                   const _PyFbGlobal *fb, PyObject *name, _PyGlobalRecipe *recipe)
{
    (void)site;
    PyObject *globals = frame->f_globals;
    PyObject *builtins = frame->f_builtins;
    int result;
#ifdef Py_GIL_DISABLED
    if (PyMutex_IsLocked(&globals->ob_mutex) || PyMutex_IsLocked(&builtins->ob_mutex)) {
        // Skip specialization if either dictionary is locked to avoid lock
        // contention.
        return GLOBAL_NONE;
    }
#endif
    Py_BEGIN_CRITICAL_SECTION2(globals, builtins);
    result = global_classify_lock_held(globals, builtins, (_PyFbGlobal *)fb,
                                       name, recipe);
    Py_END_CRITICAL_SECTION2();
    return result;
}

static FunctionSignature
function_signature(PyCodeObject *code)
{
    int flags = code->co_flags;
    if ((flags & (CO_VARKEYWORDS | CO_VARARGS)) || code->co_kwonlyargcount) {
        return GENERAL_SIGNATURE;
    }
    if ((flags & CO_OPTIMIZED) == 0) {
        return UNOPTIMIZED_SIGNATURE;
    }
    return SIMPLE_SIGNATURE;
}

/* A call of a generator function creates its object: no frame runs, so no
 * specialization may push one. */
static bool
code_creates_generator(PyCodeObject *code)
{
    return (code->co_flags &
            (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) != 0;
}

/* An optional specialization may fail without failing the current operation.
 * Cache nomination still requires a positive execution-image readiness proof. */
static bool
prepare_function_exec(PyCodeObject *code)
{
    if (_PyCode_GetExec(code) == NULL) {
        PyErr_Clear();
        return false;
    }
    return true;
}

/* The same for a specialization that pushes the frame of the function. */
static bool
prepare_function_for_inline(PyCodeObject *code)
{
    return !code_creates_generator(code) && prepare_function_exec(code);
}

/* Returning false indicates a failure. */
static bool
function_check_args(PyObject *o, int expected_argcount, int opcode)
{
    assert(Py_IS_TYPE(o, &PyFunction_Type));
    PyFunctionObject *func = (PyFunctionObject *)o;
    PyCodeObject *fcode = (PyCodeObject *)func->func_code;
    FunctionSignature signature = function_signature(fcode);
    if (signature != SIMPLE_SIGNATURE) {
        SPECIALIZATION_FAIL(opcode, signature == GENERAL_SIGNATURE
                            ? SPEC_FAIL_CODE_COMPLEX_PARAMETERS
                            : SPEC_FAIL_CODE_NOT_OPTIMIZED);
        return false;
    }
    if (fcode->co_argcount != expected_argcount) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_WRONG_NUMBER_ARGUMENTS);
        return false;
    }
    if (!prepare_function_for_inline(fcode)) {
        SPECIALIZATION_FAIL(opcode, SPEC_FAIL_OTHER);
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
store_subscr_fail_kind(PyObject *container, PyValueBorrow sub)
{
    PyTypeObject *container_type = Py_TYPE(container);
    bool sub_is_int = PyValue_LongCheckExact(sub);
    bool sub_is_slice = PyValue_TYPE(sub) == &PySlice_Type;
    PyMappingMethods *as_mapping = container_type->tp_as_mapping;
    if (as_mapping && (as_mapping->mp_ass_subscript
                       == PyDict_Type.tp_as_mapping->mp_ass_subscript)) {
        return SPEC_FAIL_SUBSCR_DICT_SUBCLASS_NO_OVERRIDE;
    }
    if (PyObject_CheckBuffer(container)) {
        if (sub_is_int &&
            !(PyValue_IsInlineInt(sub) && PyValue_InlineInt64(sub) >= 0))
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
    PyObject *descriptor = _PyType_LookupRef(container_type, &_Py_ID(__setitem__));
    int fail_kind = SPEC_FAIL_OTHER;
    if (descriptor && Py_TYPE(descriptor) == &PyFunction_Type) {
        PyFunctionObject *func = (PyFunctionObject *)descriptor;
        PyCodeObject *code = (PyCodeObject *)func->func_code;
        if (function_signature(code) == SIMPLE_SIGNATURE) {
            fail_kind = SPEC_FAIL_SUBSCR_PY_SIMPLE;
        }
        else {
            fail_kind = SPEC_FAIL_SUBSCR_PY_OTHER;
        }
    }
    Py_XDECREF(descriptor);
    return fail_kind;
}
#endif

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
    (void)_PyType_GetVersionForCurrentState(_PyInterpreterState_GET(), tp);
    PyValue init_value;
    unsigned int version = _PyType_LookupValueAndVersion(
        tp, &_Py_ID(__init__), &init_value);
    *tp_version = version;
    if (PyValue_IsNull(init_value) || !PyValue_IsObject(init_value) ||
        !PyFunction_Check(PyValue_AsKnownObjectBorrow(init_value))) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_INIT_NOT_PYTHON);
        PyValue_XCLOSE(init_value);
        return NULL;
    }
    PyObject *init = PyValue_AsKnownObjectSteal(init_value);
    PyCodeObject *code = (PyCodeObject *)PyFunction_GET_CODE(init);
    if (function_signature(code) != SIMPLE_SIGNATURE) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_INIT_NOT_SIMPLE);
        Py_DECREF(init);
        return NULL;
    }
    if (!prepare_function_for_inline(code)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OTHER);
        Py_DECREF(init);
        return NULL;
    }
    return init;
}

/* See pycore_feedback.h.  The slots a keyword fills are not defaults, and
 * the positional parameters that no operand and no keyword fills are the
 * ones that __defaults__ must fill: its length is the only fact about it
 * that a call needs, and the function version does not guard it. */
int
_PyBindPlan_Init(_PyBindPlan *plan, PyCodeObject *code, int positional,
                 int skip, int lead, const uint16_t *dests, int kwcount)
{
    memset(plan, 0, sizeof(*plan));
    int flags = code->co_flags;
    int argcount = code->co_argcount;
    int kwonly = code->co_kwonlyargcount;
    if (!(flags & CO_OPTIMIZED) || code->_co_exec == NULL ||
        argcount > _Py_CALL_BIND_SLOTS_MAX ||
        kwonly > _Py_CALL_BIND_SLOTS_MAX ||
        positional > UINT8_MAX || kwcount > UINT8_MAX)
    {
        return 0;
    }
    uint16_t bits = 0;
    if (flags & (CO_GENERATOR | CO_COROUTINE | CO_ASYNC_GENERATOR)) {
        /* The call creates the object and binds into its frame; its
         * prologue (free variables, cells, the iterator of a generator
         * expression) runs after the binding */
        bits |= _Py_CALL_BIND_GENERATOR;
    }
    int filled = positional < argcount ? positional : argcount;
    int extras = positional - filled;
    if (extras > 0 && (!(flags & CO_VARARGS) ||
                       extras > _Py_CALL_BIND_EXTRAS_MAX || lead > argcount))
    {
        return 0;
    }
    uint32_t positional_covered = 0;
    uint32_t kwonly_covered = 0;
    int routed = 0;
    for (int i = 0; i < kwcount; i++) {
        if (dests[i] == _Py_CALL_BIND_TO_KWARGS) {
            routed++;
        }
        else if (dests[i] < argcount) {
            positional_covered |= 1u << dests[i];
        }
        else {
            kwonly_covered |= 1u << (dests[i] - argcount);
        }
    }
    uint32_t defaulted = ((1u << argcount) - (1u << filled)) & ~positional_covered;
    plan->default_mask = (uint16_t)defaulted;
    plan->need = defaulted ? (uint8_t)(argcount - __builtin_ctz(defaulted)) : 0;
    plan->kwonly_mask = (uint16_t)(((1u << kwonly) - 1) & ~kwonly_covered);
    plan->positional = (uint8_t)positional;
    plan->filled = (uint8_t)filled;
    plan->keywords = (uint8_t)kwcount;
    if (skip) {
        bits |= _Py_CALL_BIND_SKIP;
    }
    if (lead) {
        bits |= _Py_CALL_BIND_LEAD;
    }
    if (flags & CO_VARARGS) {
        bits |= _Py_CALL_BIND_VARARGS;
    }
    if (flags & CO_VARKEYWORDS) {
        bits |= routed == 0 && (code->_co_ir->flags & _PyIR_CODE_VIRTUAL_KWARGS)
            ? _Py_CALL_BIND_VIRTUAL_KWARGS : _Py_CALL_BIND_NEW_KWARGS;
    }
    if (bits & (_Py_CALL_BIND_VARARGS | _Py_CALL_BIND_NEW_KWARGS |
                _Py_CALL_BIND_GENERATOR) || plan->kwonly_mask || extras) {
        bits |= _Py_CALL_BIND_GENERAL;
    }
    plan->flags = bits | _Py_CALL_BIND_VALID;
    return 1;
}

int
_PyBindPlan_ForCall(_PyBindPlan *plan, PyCodeObject *code, int positional,
                    int skip, int lead)
{
    _PyCodeExec *exec = code->_co_exec;
    if (exec == NULL || exec == &_PyIR_InitCleanupExec || positional < 0 ||
        positional > UINT8_MAX)
    {
        return _PyBindPlan_Init(plan, code, positional, skip, lead, NULL, 0);
    }
    uint64_t shape = 1 + ((uint64_t)positional | (uint64_t)skip << 8 |
                          (uint64_t)lead << 9);
    if (exec->call_plan_shape != shape) {
        _PyBindPlan fresh;
        _PyBindPlan_Init(&fresh, code, positional, skip, lead, NULL, 0);
        exec->call_plan = fresh;
        exec->call_plan_shape = shape;
    }
    *plan = exec->call_plan;
    return (plan->flags & _Py_CALL_BIND_VALID) != 0;
}

/* Cache only destinations: names, defaults and keyword-only defaults remain
 * owned by the guarded function, which the call reads each time.  A name that
 * is no parameter goes to **kwargs: it must be interned, as every parameter
 * name is, so that no other spelling of it names a parameter.  The binding
 * rests on the code, which every function of the guarded version shares. */
static void
specialize_keyword_binding(PyFunctionObject *func, _PyFbCall *fb, _PyInstr *instr,
                           int nargs, bool has_self, PyValueBorrow names)
{
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    if (!(code->co_flags & CO_OPTIMIZED) ||
        !PyValue_IsObject(names) ||
        !PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(names)))
    {
        return;
    }
    PyObject *kwnames = PyValue_AsKnownObjectBorrow(names);
    Py_ssize_t kwcount = PyTuple_GET_SIZE(kwnames);
    if (kwcount < 1 || kwcount > _Py_CALL_BIND_KEYWORD_CAPACITY) {
        return;
    }
    int positional = nargs + has_self - (int)kwcount;
    int total = code->co_argcount + code->co_kwonlyargcount;
    if (positional < 0 || total >= _Py_CALL_BIND_TO_KWARGS) {
        return;
    }
    uint16_t destinations[_Py_CALL_BIND_KEYWORD_CAPACITY];
    for (int i = 0; i < kwcount; i++) {
        PyValueBorrow name = PyTuple_GET_VALUE(kwnames, i);
        if (!PyValue_IsObject(name) ||
            !PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(name)))
        {
            return;
        }
        int destination;
        for (destination = code->co_posonlyargcount; destination < total;
             destination++)
        {
            PyObject *parameter =
                _PyTuple_GET_OBJECT(code->co_localsplusnames, destination);
            if (PyValue_AsKnownObjectBorrow(name) == parameter) {
                break;
            }
            if (!PyUnicode_CHECK_INTERNED(parameter) ||
                !PyUnicode_CHECK_INTERNED(PyValue_AsKnownObjectBorrow(name)))
            {
                return;
            }
        }
        if (destination == total) {
            if (!(code->co_flags & CO_VARKEYWORDS)) {
                return;
            }
            destination = _Py_CALL_BIND_TO_KWARGS;
        }
        else if (destination < positional && destination < code->co_argcount) {
            return;
        }
        for (int j = 0; j < i; j++) {
            if (destinations[j] == destination) {
                return;
            }
        }
        destinations[i] = (uint16_t)destination;
    }
    int skip = !has_self && (int)IR_C(*instr) - 1 > nargs;
    if (!_PyBindPlan_Init(&fb->plan, code, positional, skip, 0, destinations,
                          (int)kwcount))
    {
        return;
    }
    for (int i = 0; i < kwcount; i++) {
        fb->plan.keyword_dest[i] = destinations[i];
    }
}

static int
specialize_py_call_kw(PyFunctionObject *func, _PyInstr *instr, int nargs,
                     bool bound_method, bool has_self, PyValueBorrow kwnames)
{
    _PyFbCall *fb = instr_entry(instr);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    FunctionSignature signature = function_signature(code);
    /* Don't specialize if PEP 523 is active */
    if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET())) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_PEP_523);
        return -1;
    }
    if (func->vectorcall != _PyFunction_Vectorcall) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CALL_VECTORCALL);
        return -1;
    }
    if (signature == UNOPTIMIZED_SIGNATURE) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_CODE_NOT_OPTIMIZED);
        return -1;
    }
    // A generator function's call binds into its object (_Py_CALL_BIND_GENERATOR)
    if (!prepare_function_exec(code)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OTHER);
        return -1;
    }
    int version = _PyFunction_GetVersionForCurrentState(func);
    if (!_PyFunction_IsVersionValid(version)) {
        SPECIALIZATION_FAIL(CALL, SPEC_FAIL_OUT_OF_VERSIONS);
        return -1;
    }
    fb->func_version = version;
    memset(&fb->plan, 0, sizeof(fb->plan));
    if (!bound_method) {
        specialize_keyword_binding(func, fb, instr, nargs, has_self, kwnames);
    }
    specialize(instr, bound_method ? CALL_KW_BOUND_METHOD : CALL_KW_PY);
    return 0;
}

/* The evidence rows of GetItem and SetItem (classify_getitem and
 * classify_setitem, Python/register_ops/items.rop): the arm that a subscript
 * of v by k stands for, as the index of its case in the declared order, with
 * the site's entry holding the operand type versions.  Every arm guards what
 * it relies on, so the entry records evidence and guards nothing. */
enum { GETITEM_LIST_INDEX, GETITEM_TUPLE_INDEX, GETITEM_ASCII, GETITEM_UNICODE_CHAR,
       GETITEM_DICT, GETITEM_LIST_SLICE, GETITEM_NONE };
enum { SETITEM_LIST_INDEX, SETITEM_DICT, SETITEM_NONE };

static bool
item_record(_PyFbBinary *fb, PyValueBorrow v, PyValueBorrow k)
{
    if (is_shared_entry(&fb->h) || !PyValue_IsObject(v) ||
        !_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET()))
    {
        return false;
    }
    memset(&fb->h + 1, 0, sizeof(_PyFbBinary) - sizeof(_PyFbHeader));
    fb->lhs_version = fb_type_version(PyValue_TYPE(v));
    fb->rhs_version = fb_type_version(PyValue_TYPE(k));
    return true;
}

int
_PyRop_ClassifyGetItem(_PyFbBinary *fb, PyValueBorrow v, PyValueBorrow k)
{
    if (!item_record(fb, v, k)) {
        return GETITEM_NONE;
    }
    PyObject *container = PyValue_AsKnownObjectBorrow(v);
    PyTypeObject *tp = Py_TYPE(container);
    bool int_key = PyValue_IsInlineInt(k);
    if (tp == &PyList_Type) {
        if (int_key) {
            return GETITEM_LIST_INDEX;
        }
        return PyValue_TYPE(k) == &PySlice_Type ? GETITEM_LIST_SLICE : GETITEM_NONE;
    }
    if (tp == &PyTuple_Type) {
        return int_key ? GETITEM_TUPLE_INDEX : GETITEM_NONE;
    }
    if (tp == &PyUnicode_Type) {
        if (!int_key) {
            return GETITEM_NONE;
        }
        return PyUnicode_IS_COMPACT_ASCII(container)
            ? GETITEM_ASCII : GETITEM_UNICODE_CHAR;
    }
    if (tp->tp_as_mapping != NULL &&
        tp->tp_as_mapping->mp_subscript == _PyDict_Subscript)
    {
        return GETITEM_DICT;
    }
    return GETITEM_NONE;
}

int
_PyRop_ClassifySetItem(_PyFbBinary *fb, PyValueBorrow v, PyValueBorrow k)
{
    if (!item_record(fb, v, k)) {
        return SETITEM_NONE;
    }
    PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(v));
    if (tp == &PyList_Type) {
        return PyValue_IsInlineInt(k) ? SETITEM_LIST_INDEX : SETITEM_NONE;
    }
    if (tp->tp_as_mapping != NULL &&
        tp->tp_as_mapping->mp_ass_subscript == _PyDict_StoreSubscript)
    {
        return SETITEM_DICT;
    }
    return SETITEM_NONE;
}

/* The evidence row of Call (classify_call, Python/register_ops/calls.rop):
 * the arm that a call of fn at site stands for, as the index of its case
 * (exact, bound_exact, init, other), with the site's entry filled for that
 * arm: a simple Python function, a bound method of one, or a heap class whose
 * __init__ is one; every other callable is "other", which runs in
 * Call.Generic. */
enum { CALL_CASE_EXACT, CALL_CASE_BOUND_EXACT, CALL_CASE_INIT, CALL_CASE_OTHER };

static bool
call_classify_function(_PyFbCall *fb, PyFunctionObject *func, int nargs)
{
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    if (!_PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET()) ||
        func->vectorcall != _PyFunction_Vectorcall ||
        function_signature(code) != SIMPLE_SIGNATURE ||
        code->co_argcount != nargs || code_creates_generator(code) ||
        !prepare_function_exec(code))
    {
        return false;
    }
    int version = _PyFunction_GetVersionForCurrentState(func);
    if (!_PyFunction_IsVersionValid(version)) {
        return false;
    }
    fb->func_version = version;
    return true;
}

static bool
call_classify_class(_PyFbCall *fb, PyTypeObject *tp)
{
    if ((tp->tp_flags & Py_TPFLAGS_IMMUTABLETYPE) ||
        Py_TYPE(tp) != &PyType_Type || tp->tp_new != PyBaseObject_Type.tp_new) {
        return false;
    }
    unsigned int tp_version = 0;
    PyObject *init = get_init_for_simple_managed_python_class(tp, &tp_version);
    bool cached = tp_version != 0 && init != NULL &&
        _PyType_CacheInitForSpecialization((PyHeapTypeObject *)tp, init, tp_version);
    Py_XDECREF(init);
    if (cached) {
        fb->type_version = tp_version;
    }
    return cached;
}

int
_PyRop_ClassifyCall(_PyInstr *site, _PyFbCall *fb, PyValueBorrow fn,
                    const _PyIRWord *head)
{
    assert(site == head);
    (void)site;
    if (is_shared_entry(&fb->h) || !PyValue_IsObject(fn)) {
        return CALL_CASE_OTHER;
    }
    memset(&fb->h + 1, 0, sizeof(_PyFbCall) - sizeof(_PyFbHeader));
    _PyIRWord word = head[0];
    // The arguments the callee receives, a self that is present among them
    int self_slot = (word & IR_F_NO_SELF) == 0;
    int self_present = self_slot && !PyValue_IsNull(
        _PyFrame_Slots(current_frame())[_PyIR_CallOperand(head, 0)]);
    int nargs = (int)IR_C(word) - (self_slot && !self_present);
    PyObject *callable = PyValue_AsKnownObjectBorrow(fn);
    fb->callable_type_version = fb_type_version(Py_TYPE(callable));
    if (PyFunction_Check(callable)) {
        return call_classify_function(fb, (PyFunctionObject *)callable, nargs)
            ? CALL_CASE_EXACT : CALL_CASE_OTHER;
    }
    if (self_present) {
        return CALL_CASE_OTHER;
    }
    if (PyMethod_Check(callable)) {
        PyObject *func = ((PyMethodObject *)callable)->im_func;
        return PyFunction_Check(func) &&
            call_classify_function(fb, (PyFunctionObject *)func, nargs + 1)
            ? CALL_CASE_BOUND_EXACT : CALL_CASE_OTHER;
    }
    if (PyType_Check(callable)) {
        return call_classify_class(fb, (PyTypeObject *)callable)
            ? CALL_CASE_INIT : CALL_CASE_OTHER;
    }
    return CALL_CASE_OTHER;
}

Py_NO_INLINE void
_Py_Specialize_CallKw(PyValue callable_st, PyValue self_or_null,
                     PyValue kwnames, _PyInstr *instr, int nargs)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[CALL_KW] == INLINE_CACHE_ENTRIES_CALL_KW);
    assert(instr_opcode(instr) != INSTRUMENTED_CALL_KW);
    bool native_or_inline = PyValue_IsInline(callable_st) ||
        (PyValue_IsObject(callable_st) &&
         PyCFunction_Check(PyValue_AsKnownObjectBorrow(callable_st)));
    fb_begin_call_ex(instr, PyValue_TYPE(callable_st), !native_or_inline);
    if (PyValue_IsInline(callable_st)) {
        SPECIALIZATION_FAIL(CALL_KW, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *callable = PyValue_AsKnownObjectBorrow(callable_st);
    int fail;
    if (PyFunction_Check(callable)) {
        fail = specialize_py_call_kw((PyFunctionObject *)callable, instr, nargs,
                                    false, !PyValue_IsNull(self_or_null),
                                    PyValue_Borrow(kwnames));
    }
    else if (PyMethod_Check(callable)) {
        PyObject *func = ((PyMethodObject *)callable)->im_func;
        if (PyFunction_Check(func)) {
            fail = specialize_py_call_kw((PyFunctionObject *)func, instr, nargs,
                                        true, false, PyValue_Borrow(kwnames));
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
binary_op_fail_kind(int oparg, PyValueBorrow lhs_st, PyValueBorrow rhs_st)
{
    PyTypeObject *lhs_type = PyValue_TYPE(lhs_st);
    PyTypeObject *rhs_type = PyValue_TYPE(rhs_st);
    switch (oparg) {
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
                if (rhs_type == &PyLong_Type &&
                    (!PyValue_IsInlineInt(rhs_st) || PyValue_InlineInt64(rhs_st) < 0)) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (rhs_type == &PySlice_Type) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_LIST_SLICE;
                }
            }
            if (lhs_type == &PyTuple_Type) {
                if (rhs_type == &PyLong_Type &&
                    (!PyValue_IsInlineInt(rhs_st) || PyValue_InlineInt64(rhs_st) < 0)) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (rhs_type == &PySlice_Type) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_TUPLE_SLICE;
                }
            }
            if (lhs_type == &PyUnicode_Type) {
                if (rhs_type == &PyLong_Type &&
                    (!PyValue_IsInlineInt(rhs_st) || PyValue_InlineInt64(rhs_st) < 0)) {
                    return SPEC_FAIL_OUT_OF_RANGE;
                }
                if (rhs_type == &PySlice_Type) {
                    return SPEC_FAIL_BINARY_OP_SUBSCR_STRING_SLICE;
                }
            }
            PyTypeObject *container_type = lhs_type;
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

            if (strcmp(container_type->tp_name, "EnumDict") == 0) {
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

#define SEQ_INT_MULTIPLY_ACTION(NAME, REPEAT, SEQ, COUNT) \
    static PyObject * \
    (NAME)(PyObject *lhs, PyObject *rhs) \
    { \
        Py_ssize_t count = _PyValue_IntAsSsize_t(PyValue_FromObjectBorrow(COUNT)); \
        if (count == -1 && PyErr_Occurred()) { \
            return NULL; \
        } \
        return REPEAT(SEQ, count); \
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
    {NB_ADD, _PyList_Concat, &PyList_Type, 1, &PyList_Type, &PyList_Type},
    /* tuple-tuple concatenation: _PyTuple_Concat has a zero-length shortcut
       that can return one of the operands, so the result is not guaranteed
       to be a freshly allocated object. */
    {NB_ADD, _PyTuple_Concat, &PyTuple_Type, 0, &PyTuple_Type, &PyTuple_Type},

    /* str * int / int * str: call _PyUnicode_Repeat directly.
       _PyUnicode_Repeat returns the original when n == 1. */
    {NB_MULTIPLY, str_int_multiply, &PyUnicode_Type, 0, &PyUnicode_Type, &PyLong_Type},
    {NB_MULTIPLY, int_str_multiply, &PyUnicode_Type, 0, &PyLong_Type, &PyUnicode_Type},
    {NB_INPLACE_MULTIPLY, str_int_multiply, &PyUnicode_Type, 0, &PyUnicode_Type, &PyLong_Type},
    {NB_INPLACE_MULTIPLY, int_str_multiply, &PyUnicode_Type, 0, &PyLong_Type, &PyUnicode_Type},

    /* bytes + bytes: bytes_concat may return an operand when one side
       is empty, so result is not always unique. */
    {NB_ADD, _PyBytes_Concat, &PyBytes_Type, 0, &PyBytes_Type, &PyBytes_Type},
    {NB_INPLACE_ADD, _PyBytes_Concat, &PyBytes_Type, 0, &PyBytes_Type, &PyBytes_Type},

    /* bytes * int / int * bytes: call _PyBytes_Repeat directly.
       _PyBytes_Repeat returns the original when n == 1. */
    {NB_MULTIPLY, bytes_int_multiply, &PyBytes_Type, 0, &PyBytes_Type, &PyLong_Type},
    {NB_MULTIPLY, int_bytes_multiply, &PyBytes_Type, 0, &PyLong_Type, &PyBytes_Type},
    {NB_INPLACE_MULTIPLY, bytes_int_multiply, &PyBytes_Type, 0, &PyBytes_Type, &PyLong_Type},
    {NB_INPLACE_MULTIPLY, int_bytes_multiply, &PyBytes_Type, 0, &PyLong_Type, &PyBytes_Type},

    /* tuple * int / int * tuple: call _PyTuple_Repeat directly.
       _PyTuple_Repeat returns the original when n == 1. */
    {NB_MULTIPLY, tuple_int_multiply, &PyTuple_Type, 0, &PyTuple_Type, &PyLong_Type},
    {NB_MULTIPLY, int_tuple_multiply, &PyTuple_Type, 0, &PyLong_Type, &PyTuple_Type},
    {NB_INPLACE_MULTIPLY, tuple_int_multiply, &PyTuple_Type, 0, &PyTuple_Type, &PyLong_Type},
    {NB_INPLACE_MULTIPLY, int_tuple_multiply, &PyTuple_Type, 0, &PyLong_Type, &PyTuple_Type},

    /* dict | dict */
    {NB_OR, _PyDict_Or, &PyDict_Type, 1, &PyDict_Type, &PyDict_Type},
    {NB_INPLACE_OR, _PyDict_IOr, &PyDict_Type, 0, &PyDict_Type, &PyDict_Type},
};

static int
binary_op_extended_specialization(PyObject *lhs, PyObject *rhs, int oparg,
                                  _PyBinaryOpSpecializationDescr **descr)
{
    size_t n = sizeof(binaryop_extend_descrs)/sizeof(_PyBinaryOpSpecializationDescr);
    for (size_t i = 0; i < n; i++) {
        _PyBinaryOpSpecializationDescr *d = &binaryop_extend_descrs[i];
        if (d->oparg != oparg) {
            continue;
        }
        int match = Py_TYPE(lhs) == d->lhs_type && Py_TYPE(rhs) == d->rhs_type;
        if (match) {
            *descr = d;
            return 1;
        }
    }
    return 0;
}

/* Binary nomination cannot escape: evaluated operands remain in their
 * canonical homes until the semantic operation captures independent owners.
 * Scalar types and existing compact predicates need no temporary boxes. */
static void
specialize_binary_op(PyObject *lhs, PyObject *rhs,
                     PyValueBorrow lhs_st, PyValueBorrow rhs_st,
                     _PyInstr *instr, int oparg, PyValue *locals)
{
    PyTypeObject *lhs_type = PyValue_TYPE(lhs_st);
    PyTypeObject *rhs_type = PyValue_TYPE(rhs_st);
    _PyFbBinary *fb = fb_begin_binary(instr, lhs_type, rhs_type);
    fb->descr = NULL;
    bool producers = true;
    // The specialized forms take inline operands only.
    bool inline_ints = producers && PyValue_BothInlineInt(lhs_st, rhs_st);
    // Two floats, or a float and an int.
    bool floats = producers && PyValue_FloatOperandsCheck(lhs_st, rhs_st);

    switch (oparg) {
        /* NB_ADD is the Add family's (add.rop adapt_before_use_add) */
        case NB_INPLACE_ADD:
            if (inline_ints) {
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
                if (to_store && PyValue_IsObject(locals[local]) &&
                    PyValue_AsKnownObjectBorrow(locals[local]) == lhs)
                {
                    specialize(instr, BINARY_OP_INPLACE_ADD_UNICODE);
                    return;
                }
                specialize(instr, BINARY_OP_ADD_UNICODE);
                return;
            }
            break;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            if (inline_ints) {
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
            if (inline_ints) {
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
            if (floats && PyValue_NumberAsDouble(rhs_st) != 0.0) {
                specialize(instr, BINARY_OP_TRUEDIV_FLOAT);
                return;
            }
            break;
        case NB_FLOOR_DIVIDE:
        case NB_INPLACE_FLOOR_DIVIDE:
            if (inline_ints && PyValue_InlineInt64(rhs_st) != 0) {
                specialize(instr, BINARY_OP_FLOOR_DIVIDE_INT);
                return;
            }
            break;
        case NB_REMAINDER:
        case NB_INPLACE_REMAINDER:
            if (inline_ints && PyValue_InlineInt64(rhs_st) != 0) {
                specialize(instr, BINARY_OP_REMAINDER_INT);
                return;
            }
            break;
        case NB_AND:
        case NB_INPLACE_AND:
            if (inline_ints) {
                specialize(instr, BINARY_OP_AND_INT);
                return;
            }
            break;
        case NB_OR:
        case NB_INPLACE_OR:
            if (inline_ints) {
                specialize(instr, BINARY_OP_OR_INT);
                return;
            }
            break;
        case NB_XOR:
        case NB_INPLACE_XOR:
            if (inline_ints) {
                specialize(instr, BINARY_OP_XOR_INT);
                return;
            }
            break;
        case NB_LSHIFT:
        case NB_INPLACE_LSHIFT:
            if (inline_ints && PyValue_InlineInt64(rhs_st) >= 0) {
                specialize(instr, BINARY_OP_LSHIFT_INT);
                return;
            }
            break;
        case NB_RSHIFT:
        case NB_INPLACE_RSHIFT:
            if (inline_ints && PyValue_InlineInt64(rhs_st) >= 0) {
                specialize(instr, BINARY_OP_RSHIFT_INT);
                return;
            }
            break;
    }

    _PyBinaryOpSpecializationDescr *descr;
    if (lhs != NULL && rhs != NULL &&
        binary_op_extended_specialization(lhs, rhs, oparg, &descr)) {
        fb->descr = descr;
        specialize(instr, BINARY_OP_EXTEND);
        return;
    }

    SPECIALIZATION_FAIL(BINARY_OP, binary_op_fail_kind(oparg, lhs_st, rhs_st));
    unspecialize(instr);
}

Py_NO_INLINE void
(_Py_Specialize_BinaryOp)(PyValueBorrow lhs_st, PyValueBorrow rhs_st, _PyInstr *instr,
                        int oparg, PyValue *locals)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[BINARY_OP] == INLINE_CACHE_ENTRIES_BINARY_OP);
    PyObject *lhs = PyValue_IsObject(lhs_st) ? PyValue_AsKnownObjectBorrow(lhs_st) : NULL;
    PyObject *rhs = PyValue_IsObject(rhs_st) ? PyValue_AsKnownObjectBorrow(rhs_st) : NULL;
    specialize_binary_op(lhs, rhs, lhs_st, rhs_st, instr, oparg, locals);
}


#ifdef Py_STATS
static int
unpack_sequence_fail_kind(PyObject *seq)
{
    if (_PyValue_SequenceCheck(PyValue_FromObjectBorrow(seq))) {
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
    fb_begin_unary(instr, PyValue_TYPE(seq_st));
    if (PyValue_IsInline(seq_st)) {
        SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *seq = PyValue_AsKnownObjectBorrow(seq_st);
    if (PyTuple_CheckExact(seq)) {
        if (PyTuple_GET_SIZE(seq) != oparg) {
            SPECIALIZATION_FAIL(UNPACK_SEQUENCE, SPEC_FAIL_EXPECTED_ERROR);
            unspecialize(instr);
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
_Py_Specialize_Send(PyValue receiver_st, _PyInstr *instr)
{
    PyObject *receiver = PyValue_AsKnownObjectBorrow(receiver_st);

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
_Py_Specialize_CallFunctionEx(PyValue func_st, PyValue args_st, _PyInstr *instr)
{
    assert(ENABLE_SPECIALIZATION);
    assert(_PyOpcode_Caches[CALL_FUNCTION_EX] == INLINE_CACHE_ENTRIES_CALL_FUNCTION_EX);
    fb_begin_call(instr, PyValue_TYPE(func_st));
    if (PyValue_IsInline(func_st)) {
        SPECIALIZATION_FAIL(CALL_FUNCTION_EX, SPEC_FAIL_OTHER);
        unspecialize(instr);
        return;
    }
    PyObject *func = PyValue_AsKnownObjectBorrow(func_st);

    /* The binding plan of the arity and the code that the site sees: the
     * call uses it for that code and arity (func_version holds the code's
     * version), and derives one for any other. */
    _PyFbCall *fb = instr_entry(instr);
    memset(&fb->plan, 0, sizeof(fb->plan));
    fb->func_version = 0;
    int lead = 0;
    if (PyMethod_Check(func)) {
        func = ((PyMethodObject *)func)->im_func;
        lead = 1;
    }
    if (PyFunction_Check(func) && PyValue_IsObject(args_st) &&
        PyTuple_CheckExact(PyValue_AsKnownObjectBorrow(args_st)))
    {
        PyCodeObject *code = (PyCodeObject *)((PyFunctionObject *)func)->func_code;
        Py_ssize_t count = lead + PyTuple_GET_SIZE(PyValue_AsKnownObjectBorrow(args_st));
        if (count <= UINT8_MAX &&
            _PyBindPlan_Init(&fb->plan, code, (int)count, 0, lead, NULL, 0))
        {
            fb->func_version = code->co_version;
        }
    }
    func = PyValue_AsKnownObjectBorrow(func_st);
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

static int
type_valid(PyTypeObject *Py_UNUSED(ty))
{
    return 0;
}

/* The truth rows' adaptation (Python/register_ops/truth.rop): the entry
 * takes the operand's type version, and a type whose version the arm may
 * guard is validated, which can assign the version. */
void
_PyRop_BeginTruth(_PyFbUnary *fb, PyValueBorrow v)
{
    fb->type_version = fb_type_version(PyValue_TYPE(v));
}

bool
_PyRop_ValidatePlain(_PyFbUnary *fb, PyValueBorrow v)
{
    PyTypeObject *tp = PyValue_TYPE(v);
    unsigned int version = 0;
    if (!PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE)
        || _PyType_Validate(tp, check_type_always_true, &version) != 0) {
        return false;
    }
    assert(version);
    fb->type_version = version;
    return true;
}

bool
_PyRop_ValidateSlot(_PyFbUnary *fb, PyValueBorrow v)
{
    unsigned int version = 0;
    if (_PyType_Validate(PyValue_TYPE(v), type_valid, &version) != 0) {
        return false;
    }
    assert(version);
    fb->type_version = version;
    return true;
}



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

#define NO_LOC_2 (128 | (PY_CODE_LOCATION_INFO_NONE << 3) | 1)

/* Static code uses the same normalized positions envelope as allocated code.
 * Its semantic container remains the shared empty bytes object. */
static const struct _PyStaticPositionsBytes {
    PyObject_VAR_HEAD
    Py_hash_t ob_shash;
    char ob_sval[18];
} no_location = {
    PyVarObject_HEAD_INIT(&PyBytes_Type, 17)
    .ob_shash = -1,
    .ob_sval = {'I', 'R', 'S', 'M', 2, 0, 0, 0,
                1, 0, 0, 0, 0, 0, 0, 0, NO_LOC_2, 0}
};
static_assert(offsetof(PyBytesObject, ob_sval) ==
              offsetof(struct _PyStaticPositionsBytes, ob_sval),
              "static positions bytes must have the PyBytesObject layout");


const struct _PyCode4 _Py_InitCleanup = {
    _PyVarObject_HEAD_INIT(&PyCode_Type, 2),
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
    .co_stacksize = 2,
    .co_framesize = 2 + FRAME_SPECIALS_SIZE,
    // The register form of these two instructions is _PyIR_InitCleanupExec
    // (Python/ir.c), which this static code object holds already lowered
    ._co_ir = &_PyIR_InitCleanupIR,
    ._co_exec = &_PyIR_InitCleanupExec,
    .co_code_adaptive = {
        EXIT_INIT_CHECK, 0,
        RETURN_VALUE, 0,
    }
};
