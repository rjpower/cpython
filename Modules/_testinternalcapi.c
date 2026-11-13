/*
 * C Extension module to test Python internal C APIs (Include/internal).
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

/* Always enable assertions */
#undef NDEBUG

#include "Python.h"
#include <string.h>
#include "pycore_bitutils.h"      // _Py_bswap32()
#include "pycore_bytesobject.h"   // _PyBytes_Find()
#include "pycore_ceval.h"         // _PyEval_AddPendingCall()
#include "pycore_code.h"          // _PyCode_GetTLBCFast()
#include "pycore_compile.h"       // _PyCompile_CodeGen()
#include "pycore_context.h"       // _PyContext_NewHamtForTests()
#include "pycore_dict.h"          // PyDictValues
#include "pycore_fileutils.h"     // _Py_normpath()
#include "pycore_flowgraph.h"     // _PyCompile_OptimizeCfg()
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_function.h"      // _PyFunction_GET_BUILTINS
#include "pycore_gc.h"            // PyGC_Head
#include "pycore_gc_trace.h"      // _PyGC_TraceVerify()
#include "pycore_hashtable.h"     // _Py_hashtable_new()
#include "pycore_import.h"        // _PyImport_ClearExtension()
#include "pycore_initconfig.h"    // _Py_GetConfigsAsDict()
#include "pycore_instruction_sequence.h"  // _PyInstructionSequence_New()
#include "pycore_ir_outcomes.h"    // _PyIR_NormalPhysicalTransfer()
#include "pycore_interpframe.h"   // _PyFrame_GetFunction()
#include "pycore_lock.h"          // PyEvent_WaitTimed()
#include "pycore_list.h"          // _PyList_SetItemTakeValue()
#include "pycore_object.h"        // _PyObject_IsFreed()
#include "pycore_opcode_metadata.h" // _PyOpcode_OpName
#include "pycore_opcode_utils.h"  // NUM_COMMON_CONSTANTS
#include "pycore_pathconfig.h"    // _PyPathConfig_ClearGlobal()
#include "pycore_pyerrors.h"      // _PyErr_ChainExceptions1()
#include "pycore_pylifecycle.h"   // _PyInterpreterConfig_InitFromDict()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_regcfg.h"        // _PyRegPass_SetKeepAcrossCode()
#include "pycore_runtime_structs.h" // _PY_NSMALLPOSINTS
#include "pycore_transition_roots.h" // _PyGC_TraceVisitTransitionRoots()
#include "pycore_unicodeobject.h" // _PyUnicode_TransformDecimalAndSpaceToASCII()
#include "pycore_value.h"         // _PyValue_RecheckHome()
#include "pycore_value_api.h"     // _PyValue_FromObjectKnown()
#include "pycore_tuple.h"       // _PyTuple_GET_OBJECT()

#include "clinic/_testinternalcapi.c.h"

// Include test definitions from _testinternalcapi/
#include "_testinternalcapi/parts.h"

#if defined(HAVE_DLADDR) && !defined(__wasi__)
#  include <dlfcn.h>
#endif
#if defined(HAVE_EXECINFO_H)
#  include <execinfo.h>
#endif
#ifdef MS_WINDOWS
#  include <windows.h>
#  include <intrin.h>
#  include <winnt.h>
#  include <wchar.h>
#endif

#define MODULE_NAME "_testinternalcapi"


static const uintptr_t min_frame_pointer_addr = 0x1000;
#define MAX_UNWIND_FRAMES 200

#ifdef __s390x__
// Linux's s390 "Stack Frame Layout" table documents that z/Architecture
// backchain frames start with the backchain at offset 0 and store "saved r14
// of caller function" at offset 112.  The same document's register table
// identifies r14 as the return-address register, so this backchain unwinder
// reads the return address from fp + 112.
// https://www.kernel.org/doc/html/v5.3/s390/debugging390.html#stack-frame-layout
//
// This is only for Linux s390x backchain frames.  The s390x ELF ABI does not
// generally mandate where RA and FP are saved, or whether they are saved at all.
// https://sourceware.org/binutils/docs/sframe-spec.html#s390x
#  define S390X_FRAME_RETURN_ADDRESS_OFFSET 112
#endif

// The generic manual unwinder treats the frame pointer as a two-word record:
// fp[0] is the previous frame pointer and fp[1] is the return address.  That is
// not true for every architecture, even with frame pointers enabled, so these
// offsets describe the actual slots used by each supported frame layout.
#if defined(__arm__) && !defined(__thumb__) && !defined(__clang__)
// GCC ARM mode keeps the caller's fp one word below fp and the saved LR at
// fp[0], so the return address is not in the generic fp[1] slot.
#  define FRAME_POINTER_NEXT_OFFSET (-1)
#  define FRAME_POINTER_RETURN_OFFSET 0
#elif defined(__s390x__)
// s390x backchain frames keep the previous frame pointer at fp[0], but save the
// return-address register in the ABI register save area rather than fp[1].
#  define FRAME_POINTER_NEXT_OFFSET 0
#  define FRAME_POINTER_RETURN_OFFSET \
    (S390X_FRAME_RETURN_ADDRESS_OFFSET / (Py_ssize_t)sizeof(uintptr_t))
#elif defined(__powerpc64__) || defined(__ppc64__)
// ppc64le puts the return address at fp[2]; it saves the Condition Register
// in fp[1]. See:
// https://refspecs.linuxfoundation.org/ELF/ppc64/PPC-elf64abi-1.9.html#STACK
#  define FRAME_POINTER_NEXT_OFFSET 0
#  define FRAME_POINTER_RETURN_OFFSET 2
#elif defined(__riscv)
// RISC-V saves the return address at fp[-1], and the previous frame pointer at fp[-2].
// See: https://riscv-non-isa.github.io/riscv-elf-psabi-doc/#_frame_pointer_convention
#  define FRAME_POINTER_NEXT_OFFSET -2
#  define FRAME_POINTER_RETURN_OFFSET -1
#elif defined(__loongarch__)
// On LoongArch, the frame pointer is the caller's stack pointer.
// The saved frame pointer is stored at fp[-2], and the return
// address is stored at fp[-1].
#  define FRAME_POINTER_NEXT_OFFSET -2
#  define FRAME_POINTER_RETURN_OFFSET -1
#else
#  define FRAME_POINTER_NEXT_OFFSET 0
#  define FRAME_POINTER_RETURN_OFFSET 1
#endif


static PyObject *
_get_current_module(void)
{
    // We ensured it was imported in _run_script().
    PyObject *name = PyUnicode_FromString(MODULE_NAME);
    if (name == NULL) {
        return NULL;
    }
    PyObject *mod = PyImport_GetModule(name);
    Py_DECREF(name);
    if (mod == NULL) {
        return NULL;
    }
    assert(mod != Py_None);
    return mod;
}


/* module state *************************************************************/

typedef struct {
    PyObject *record_list;
    PyObject *staged_code;
    PyObject *staged_observer;
    PyObject *staged_result;
    int staged_mode;
    int staged_observing;
    int staged_running;
} module_state;

static inline module_state *
get_module_state(PyObject *mod)
{
    assert(mod != NULL);
    module_state *state = PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static int
traverse_module_state(module_state *state, visitproc visit, void *arg)
{
    Py_VISIT(state->record_list);
    Py_VISIT(state->staged_code);
    Py_VISIT(state->staged_observer);
    Py_VISIT(state->staged_result);
    return 0;
}

static int
clear_module_state(module_state *state)
{
    Py_CLEAR(state->record_list);
    Py_CLEAR(state->staged_code);
    Py_CLEAR(state->staged_observer);
    Py_CLEAR(state->staged_result);
    return 0;
}


/* module functions *********************************************************/

/*[clinic input]
module _testinternalcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7bb583d8c9eb9a78]*/
static PyObject *
get_configs(PyObject *self, PyObject *Py_UNUSED(args))
{
    return _Py_GetConfigsAsDict();
}


static PyObject*
get_recursion_depth(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyThreadState *tstate = _PyThreadState_GET();

    return PyLong_FromLong(tstate->py_recursion_limit - tstate->py_recursion_remaining);
}


static PyObject*
get_c_recursion_remaining(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyThreadState *tstate = _PyThreadState_GET();
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    _PyThreadStateImpl *_tstate = (_PyThreadStateImpl *)tstate;
    int remaining = (int)((here_addr - _tstate->c_stack_soft_limit) / _PyOS_STACK_MARGIN_BYTES * 50);
    return PyLong_FromLong(remaining);
}

static PyObject*
get_stack_pointer(PyObject *self, PyObject *Py_UNUSED(args))
{
    uintptr_t here_addr = _Py_get_machine_stack_pointer();
    return PyLong_FromSize_t(here_addr);
}

static PyObject*
get_stack_margin(PyObject *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromSize_t(_PyOS_STACK_MARGIN_BYTES);
}

static PyObject *
test_stop_the_world(PyObject *self, PyObject *Py_UNUSED(args))
{
#ifdef Py_GIL_DISABLED
    PyInterpreterState *interp = _PyInterpreterState_GET();
    // Request consecutive pauses without running Python code between them.
    for (int i = 0; i < 100; i++) {
        _PyEval_StopTheWorld(interp);
        // Give detached threads time to try to reattach during the pause.
        PyEvent event = {0};
        PyEvent_WaitTimed(&event, 10 * 1000 * 1000, /*detach=*/0);
        _PyEval_StartTheWorld(interp);
    }
#endif
    Py_RETURN_NONE;
}

#ifdef MS_WINDOWS
static const char *
classify_address(uintptr_t addr)
{
    HMODULE module = NULL;
    if (GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
            | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)addr,
            &module)) {
        wchar_t path[MAX_PATH];
        DWORD len = GetModuleFileNameW(module, path, Py_ARRAY_LENGTH(path));
        if (len > 0 && len < Py_ARRAY_LENGTH(path)) {
            const wchar_t *base = wcsrchr(path, L'\\');
            base = base ? base + 1 : path;
            if (_wcsnicmp(base, L"python", 6) == 0) {
                return "python";
            }
            return "other";
        }
        /* Module resolved but path unavailable. */
        return "other";
    }
    return "other";
}
#elif defined(HAVE_DLADDR) && !defined(__wasi__)
static const char *
classify_address(uintptr_t addr)
{
    Dl_info info;
    if (dladdr((void *)addr, &info) != 0
        && info.dli_fname != NULL
        && info.dli_fname[0] != '\0') {
        const char *base = strrchr(info.dli_fname, '/');
        base = base ? base + 1 : info.dli_fname;
        if (strncmp(base, "python", 6) == 0) {
            return "python";
        }
#ifdef __CYGWIN__
        // Match Cygwin "cygpython3.16.dll"
        if (strncmp(base, "cygpython", 9) == 0) {
            return "python";
        }
#else
        // Match "libpython3.15.so.1.0"
        if (strncmp(base, "libpython", 9) == 0) {
            return "python";
        }
#endif
        return "other";
    }
    return "other";
}
#else
static const char *
classify_address(uintptr_t Py_UNUSED(addr))
{
    return "other";
}
#endif

static PyObject *
classify_stack_addresses(PyObject *self, PyObject *args)
{
    PyObject *seq = NULL;

    if (!PyArg_ParseTuple(args, "O:classify_stack_addresses", &seq)) {
        return NULL;
    }
    PyObject *fast = PySequence_Fast(seq, "addresses must be iterable");
    if (fast == NULL) {
        return NULL;
    }
    Py_ssize_t n = PySequence_Fast_GET_SIZE(fast);
    PyObject *labels = PyList_New(n);
    if (labels == NULL) {
        Py_DECREF(fast);
        return NULL;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue item = PyList_Check(fast) ? PyList_GetValue(fast, i)
                                         : PyTuple_GetValue(fast, i);
        if (PyValue_IsNull(item)) {
            Py_DECREF(labels);
            Py_DECREF(fast);
            return NULL;
        }
        PyObject *object = _PyValue_AsObjectAt(item, BOX_SITE_TEST);
        PyValue_XCLOSE(item);
        if (object == NULL) {
            Py_DECREF(labels);
            Py_DECREF(fast);
            return NULL;
        }
        unsigned long long value = PyLong_AsUnsignedLongLong(object);
        Py_DECREF(object);
        if (PyErr_Occurred()) {
            Py_DECREF(labels);
            Py_DECREF(fast);
            return NULL;
        }
        const char *label = classify_address((uintptr_t)value);
        PyObject *label_obj = PyUnicode_FromString(label);
        if (label_obj == NULL) {
            Py_DECREF(labels);
            Py_DECREF(fast);
            return NULL;
        }
        PyList_SET_ITEM(labels, i, label_obj);
    }
    Py_DECREF(fast);
    return labels;
}

static int
stack_address_is_valid(uintptr_t addr, uintptr_t stack_min, uintptr_t stack_max)
{
    if (addr < min_frame_pointer_addr) {
        return 0;
    }
    if (stack_min != 0 && (addr < stack_min || addr >= stack_max)) {
        return 0;
    }
    return 1;
}

static int
frame_pointer_slot_is_valid(uintptr_t *frame_pointer, Py_ssize_t offset,
                            uintptr_t stack_min, uintptr_t stack_max)
{
    uintptr_t fp_addr = (uintptr_t)frame_pointer;
    uintptr_t slot_addr;
    uintptr_t delta = (uintptr_t)Py_ABS(offset) * sizeof(uintptr_t);
    if (offset < 0) {
        if (fp_addr < delta) {
            return 0;
        }
        slot_addr = fp_addr - delta;
    }
    else {
        if (fp_addr > UINTPTR_MAX - delta) {
            return 0;
        }
        slot_addr = fp_addr + delta;
    }
    if (!stack_address_is_valid(slot_addr, stack_min, stack_max)) {
        return 0;
    }
    if (stack_max != 0) {
        if (slot_addr > UINTPTR_MAX - sizeof(uintptr_t)) {
            return 0;
        }
        if (slot_addr + sizeof(uintptr_t) > stack_max) {
            return 0;
        }
    }
    return 1;
}

static int
next_frame_pointer_is_valid(uintptr_t *frame_pointer, uintptr_t *next_fp,
                            uintptr_t stack_min, uintptr_t stack_max)
{
    uintptr_t fp_addr = (uintptr_t)frame_pointer;
    uintptr_t next_addr = (uintptr_t)next_fp;
    if (!stack_address_is_valid(next_addr, stack_min, stack_max)) {
        return 0;
    }
    if ((next_addr % sizeof(uintptr_t)) != 0) {
        return 0;
    }
#if _Py_STACK_GROWS_DOWN
    return next_addr > fp_addr;
#else
    return next_addr < fp_addr;
#endif
}

static PyObject * _Py_NO_SANITIZE_MEMORY
manual_unwind_from_fp(uintptr_t *frame_pointer)
{
    uintptr_t stack_min = 0;
    uintptr_t stack_max = 0;

#ifdef __s390x__
    Py_BUILD_ASSERT(S390X_FRAME_RETURN_ADDRESS_OFFSET % sizeof(uintptr_t) == 0);
#endif

    if (frame_pointer == NULL) {
        return PyList_New(0);
    }

    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate != NULL) {
        _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
#if _Py_STACK_GROWS_DOWN
        stack_min = tstate_impl->c_stack_hard_limit;
        stack_max = tstate_impl->c_stack_top;
#else
        stack_min = tstate_impl->c_stack_top;
        stack_max = tstate_impl->c_stack_hard_limit;
#endif
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }

    Py_ssize_t depth = 0;
    while (frame_pointer != NULL) {
        uintptr_t fp_addr = (uintptr_t)frame_pointer;
        if ((fp_addr % sizeof(uintptr_t)) != 0) {
            break;
        }
        if (depth >= MAX_UNWIND_FRAMES) {
            Py_DECREF(result);
            PyErr_Format(
                PyExc_RuntimeError,
                "manual frame pointer unwind returned more than %d frames",
                MAX_UNWIND_FRAMES);
            return NULL;
        }
        if (!stack_address_is_valid(fp_addr, stack_min, stack_max)) {
            break;
        }
        if (!frame_pointer_slot_is_valid(frame_pointer,
                                         FRAME_POINTER_NEXT_OFFSET,
                                         stack_min, stack_max)) {
            break;
        }
        if (!frame_pointer_slot_is_valid(frame_pointer,
                                         FRAME_POINTER_RETURN_OFFSET,
                                         stack_min, stack_max)) {
            break;
        }
        uintptr_t *next_fp = (uintptr_t *)frame_pointer[FRAME_POINTER_NEXT_OFFSET];
        uintptr_t return_addr = frame_pointer[FRAME_POINTER_RETURN_OFFSET];

        PyObject *addr_obj = PyLong_FromUnsignedLongLong(return_addr);
        if (addr_obj == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        if (PyList_Append(result, addr_obj) < 0) {
            Py_DECREF(addr_obj);
            Py_DECREF(result);
            return NULL;
        }
        Py_DECREF(addr_obj);
        depth++;

        if (!next_frame_pointer_is_valid(frame_pointer, next_fp,
                                         stack_min, stack_max)) {
            break;
        }
        frame_pointer = next_fp;
    }

    return result;
}

#if defined(HAVE_EXECINFO_H) && defined(HAVE_BACKTRACE)
static PyObject *
gnu_backtrace_unwind(PyObject *self, PyObject *Py_UNUSED(args))
{
    void *addresses[MAX_UNWIND_FRAMES + 1];
    int frame_count = backtrace(addresses, (int)Py_ARRAY_LENGTH(addresses));
    if (frame_count < 0) {
        PyErr_SetString(PyExc_RuntimeError, "backtrace() failed");
        return NULL;
    }
    if (frame_count > MAX_UNWIND_FRAMES) {
        PyErr_Format(
            PyExc_RuntimeError,
            "backtrace() returned more than %d frames",
            MAX_UNWIND_FRAMES);
        return NULL;
    }

    PyObject *result = PyList_New(frame_count);
    if (result == NULL) {
        return NULL;
    }
    for (int i = 0; i < frame_count; i++) {
        PyObject *addr_obj = PyLong_FromUnsignedLongLong((uintptr_t)addresses[i]);
        if (addr_obj == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyList_SET_ITEM(result, i, addr_obj);
    }
    return result;
}
#else
static PyObject *
gnu_backtrace_unwind(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_RuntimeError,
                    "gnu_backtrace_unwind is not supported on this platform");
    return NULL;
}
#endif

#if defined(__GNUC__) || defined(__clang__)
static PyObject *
manual_frame_pointer_unwind(PyObject *self, PyObject *args)
{
    uintptr_t *frame_pointer = (uintptr_t *)__builtin_frame_address(0);
    return manual_unwind_from_fp(frame_pointer);
}
#elif defined(MS_WINDOWS) && defined(_M_ARM64)
static PyObject *
manual_frame_pointer_unwind(PyObject *self, PyObject *args)
{
    CONTEXT ctx;
    uintptr_t *frame_pointer = NULL;

    RtlCaptureContext(&ctx);
    frame_pointer = (uintptr_t *)ctx.Fp;
    return manual_unwind_from_fp(frame_pointer);
}
#else
static PyObject *
manual_frame_pointer_unwind(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyErr_SetString(PyExc_RuntimeError,
                    "manual_frame_pointer_unwind is not supported on this platform");
    return NULL;
}
#endif

static PyObject*
test_bswap(PyObject *self, PyObject *Py_UNUSED(args))
{
    uint16_t u16 = _Py_bswap16(UINT16_C(0x3412));
    if (u16 != UINT16_C(0x1234)) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_bswap16(0x3412) returns %d", u16);
        return NULL;
    }

    uint32_t u32 = _Py_bswap32(UINT32_C(0x78563412));
    if (u32 != UINT32_C(0x12345678)) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_bswap32(0x78563412) returns %u", u32);
        return NULL;
    }

    uint64_t u64 = _Py_bswap64(UINT64_C(0xEFCDAB9078563412));
    if (u64 != UINT64_C(0x1234567890ABCDEF)) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_bswap64(0xEFCDAB9078563412) returns %llu", u64);
        return NULL;
    }

    Py_RETURN_NONE;
}


static int
check_popcount(uint32_t x, int expected)
{
    // Use volatile to prevent the compiler to optimize out the whole test
    volatile uint32_t u = x;
    int bits = _Py_popcount32(u);
    if (bits != expected) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_popcount32(%lu) returns %i, expected %i",
                     (unsigned long)x, bits, expected);
        return -1;
    }
    return 0;
}


static PyObject*
test_popcount(PyObject *self, PyObject *Py_UNUSED(args))
{
#define CHECK(X, RESULT) \
    do { \
        if (check_popcount(X, RESULT) < 0) { \
            return NULL; \
        } \
    } while (0)

    CHECK(0, 0);
    CHECK(1, 1);
    CHECK(0x08080808, 4);
    CHECK(0x10000001, 2);
    CHECK(0x10101010, 4);
    CHECK(0x10204080, 4);
    CHECK(0xDEADCAFE, 22);
    CHECK(0xFFFFFFFF, 32);
    Py_RETURN_NONE;

#undef CHECK
}


static int
check_bit_length(unsigned long x, int expected)
{
    // Use volatile to prevent the compiler to optimize out the whole test
    volatile unsigned long u = x;
    int len = _Py_bit_length(u);
    if (len != expected) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_bit_length(%lu) returns %i, expected %i",
                     x, len, expected);
        return -1;
    }
    return 0;
}


static PyObject*
test_bit_length(PyObject *self, PyObject *Py_UNUSED(args))
{
#define CHECK(X, RESULT) \
    do { \
        if (check_bit_length(X, RESULT) < 0) { \
            return NULL; \
        } \
    } while (0)

    CHECK(0, 0);
    CHECK(1, 1);
    CHECK(0x1000, 13);
    CHECK(0x1234, 13);
    CHECK(0x54321, 19);
    CHECK(0x7FFFFFFF, 31);
    CHECK(0xFFFFFFFF, 32);
    Py_RETURN_NONE;

#undef CHECK
}


#define TO_PTR(ch) ((void*)(uintptr_t)ch)
#define FROM_PTR(ptr) ((uintptr_t)ptr)
#define VALUE(key) (1 + ((int)(key) - 'a'))

static Py_uhash_t
hash_char(const void *key)
{
    char ch = (char)FROM_PTR(key);
    return ch;
}


static int
hashtable_cb(_Py_hashtable_t *table,
             const void *key_ptr, const void *value_ptr,
             void *user_data)
{
    int *count = (int *)user_data;
    char key = (char)FROM_PTR(key_ptr);
    int value = (int)FROM_PTR(value_ptr);
    assert(value == VALUE(key));
    *count += 1;
    return 0;
}


static PyObject*
test_hashtable(PyObject *self, PyObject *Py_UNUSED(args))
{
    _Py_hashtable_t *table = _Py_hashtable_new(hash_char,
                                               _Py_hashtable_compare_direct);
    if (table == NULL) {
        return PyErr_NoMemory();
    }

    // Using an newly allocated table must not crash
    assert(table->nentries == 0);
    assert(table->nbuckets > 0);
    assert(_Py_hashtable_get(table, TO_PTR('x')) == NULL);

    // Test _Py_hashtable_set()
    char key;
    for (key='a'; key <= 'z'; key++) {
        int value = VALUE(key);
        if (_Py_hashtable_set(table, TO_PTR(key), TO_PTR(value)) < 0) {
            _Py_hashtable_destroy(table);
            return PyErr_NoMemory();
        }
    }
    assert(table->nentries == 26);
    assert(table->nbuckets > table->nentries);

    // Test _Py_hashtable_get_entry()
    for (key='a'; key <= 'z'; key++) {
        _Py_hashtable_entry_t *entry = _Py_hashtable_get_entry(table, TO_PTR(key));
        assert(entry != NULL);
        assert(entry->key == TO_PTR(key));
        assert(entry->value == TO_PTR(VALUE(key)));
    }

    // Test _Py_hashtable_get()
    for (key='a'; key <= 'z'; key++) {
        void *value_ptr = _Py_hashtable_get(table, TO_PTR(key));
        assert((int)FROM_PTR(value_ptr) == VALUE(key));
    }

    // Test _Py_hashtable_steal()
    key = 'p';
    void *value_ptr = _Py_hashtable_steal(table, TO_PTR(key));
    assert((int)FROM_PTR(value_ptr) == VALUE(key));
    assert(table->nentries == 25);
    assert(_Py_hashtable_get_entry(table, TO_PTR(key)) == NULL);

    // Test _Py_hashtable_foreach()
    int count = 0;
    int res = _Py_hashtable_foreach(table, hashtable_cb, &count);
    assert(res == 0);
    assert(count == 25);

    // Test _Py_hashtable_clear()
    _Py_hashtable_clear(table);
    assert(table->nentries == 0);
    assert(table->nbuckets > 0);
    assert(_Py_hashtable_get(table, TO_PTR('x')) == NULL);

    _Py_hashtable_destroy(table);
    Py_RETURN_NONE;
}


static PyObject *
test_reset_path_config(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(arg))
{
    _PyPathConfig_ClearGlobal();
    Py_RETURN_NONE;
}


static int
check_edit_cost(const char *a, const char *b, Py_ssize_t expected)
{
    int ret = -1;
    PyObject *a_obj = NULL;
    PyObject *b_obj = NULL;

    a_obj = PyUnicode_FromString(a);
    if (a_obj == NULL) {
        goto exit;
    }
    b_obj = PyUnicode_FromString(b);
    if (b_obj == NULL) {
        goto exit;
    }
    Py_ssize_t result = _Py_UTF8_Edit_Cost(a_obj, b_obj, -1);
    if (result != expected) {
        PyErr_Format(PyExc_AssertionError,
                     "Edit cost from '%s' to '%s' returns %zd, expected %zd",
                     a, b, result, expected);
        goto exit;
    }
    // Check that smaller max_edits thresholds are exceeded.
    Py_ssize_t max_edits = result;
    while (max_edits > 0) {
        max_edits /= 2;
        Py_ssize_t result2 = _Py_UTF8_Edit_Cost(a_obj, b_obj, max_edits);
        if (result2 <= max_edits) {
            PyErr_Format(PyExc_AssertionError,
                         "Edit cost from '%s' to '%s' (threshold %zd) "
                         "returns %zd, expected greater than %zd",
                         a, b, max_edits, result2, max_edits);
            goto exit;
        }
    }
    // Check that bigger max_edits thresholds don't change anything
    Py_ssize_t result3 = _Py_UTF8_Edit_Cost(a_obj, b_obj, result * 2 + 1);
    if (result3 != result) {
        PyErr_Format(PyExc_AssertionError,
                     "Edit cost from '%s' to '%s' (threshold %zd) "
                     "returns %zd, expected %zd",
                     a, b, result * 2, result3, result);
        goto exit;
    }
    ret = 0;
exit:
    Py_XDECREF(a_obj);
    Py_XDECREF(b_obj);
    return ret;
}

static PyObject *
test_edit_cost(PyObject *self, PyObject *Py_UNUSED(args))
{
    #define CHECK(a, b, n) do {              \
        if (check_edit_cost(a, b, n) < 0) {  \
            return NULL;                     \
        }                                    \
    } while (0)                              \

    CHECK("", "", 0);
    CHECK("", "a", 2);
    CHECK("a", "A", 1);
    CHECK("Apple", "Aple", 2);
    CHECK("Banana", "B@n@n@", 6);
    CHECK("Cherry", "Cherry!", 2);
    CHECK("---0---", "------", 2);
    CHECK("abc", "y", 6);
    CHECK("aa", "bb", 4);
    CHECK("aaaaa", "AAAAA", 5);
    CHECK("wxyz", "wXyZ", 2);
    CHECK("wxyz", "wXyZ123", 8);
    CHECK("Python", "Java", 12);
    CHECK("Java", "C#", 8);
    CHECK("AbstractFoobarManager", "abstract_foobar_manager", 3+2*2);
    CHECK("CPython", "PyPy", 10);
    CHECK("CPython", "pypy", 11);
    CHECK("AttributeError", "AttributeErrop", 2);
    CHECK("AttributeError", "AttributeErrorTests", 10);

    #undef CHECK
    Py_RETURN_NONE;
}


static int
check_bytes_find(const char *haystack0, const char *needle0,
                 Py_ssize_t offset, Py_ssize_t expected)
{
    Py_ssize_t len_haystack = strlen(haystack0);
    Py_ssize_t len_needle = strlen(needle0);
    Py_ssize_t result_1 = _PyBytes_Find(haystack0, len_haystack,
                                        needle0, len_needle, offset);
    if (result_1 != expected) {
        PyErr_Format(PyExc_AssertionError,
                    "Incorrect result_1: '%s' in '%s' (offset=%zd)",
                    needle0, haystack0, offset);
        return -1;
    }
    // Allocate new buffer with no NULL terminator.
    char *haystack = PyMem_Malloc(len_haystack);
    if (haystack == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    char *needle = PyMem_Malloc(len_needle);
    if (needle == NULL) {
        PyMem_Free(haystack);
        PyErr_NoMemory();
        return -1;
    }
    memcpy(haystack, haystack0, len_haystack);
    memcpy(needle, needle0, len_needle);
    Py_ssize_t result_2 = _PyBytes_Find(haystack, len_haystack,
                                        needle, len_needle, offset);
    PyMem_Free(haystack);
    PyMem_Free(needle);
    if (result_2 != expected) {
        PyErr_Format(PyExc_AssertionError,
                    "Incorrect result_2: '%s' in '%s' (offset=%zd)",
                    needle0, haystack0, offset);
        return -1;
    }
    return 0;
}

static int
check_bytes_find_large(Py_ssize_t len_haystack, Py_ssize_t len_needle,
                       const char *needle)
{
    char *zeros = PyMem_RawCalloc(len_haystack, 1);
    if (zeros == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    Py_ssize_t res = _PyBytes_Find(zeros, len_haystack, needle, len_needle, 0);
    PyMem_RawFree(zeros);
    if (res != -1) {
        PyErr_Format(PyExc_AssertionError,
                    "check_bytes_find_large(%zd, %zd) found %zd",
                    len_haystack, len_needle, res);
        return -1;
    }
    return 0;
}

static PyObject *
test_bytes_find(PyObject *self, PyObject *Py_UNUSED(args))
{
    #define CHECK(H, N, O, E) do {               \
        if (check_bytes_find(H, N, O, E) < 0) {  \
            return NULL;                         \
        }                                        \
    } while (0)

    CHECK("", "", 0, 0);
    CHECK("Python", "", 0, 0);
    CHECK("Python", "", 3, 3);
    CHECK("Python", "", 6, 6);
    CHECK("Python", "yth", 0, 1);
    CHECK("ython", "yth", 1, 1);
    CHECK("thon", "yth", 2, -1);
    CHECK("Python", "thon", 0, 2);
    CHECK("ython", "thon", 1, 2);
    CHECK("thon", "thon", 2, 2);
    CHECK("hon", "thon", 3, -1);
    CHECK("Pytho", "zz", 0, -1);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "ab", 0, -1);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "ba", 0, -1);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "bb", 0, -1);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab", "ab", 0, 30);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaba", "ba", 0, 30);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaabb", "bb", 0, 30);
    #undef CHECK

    // Hunt for segfaults
    // n, m chosen here so that (n - m) % (m + 1) == 0
    // This would make default_find in fastsearch.h access haystack[n].
    if (check_bytes_find_large(2048, 2, "ab") < 0) {
        return NULL;
    }
    if (check_bytes_find_large(4096, 16, "0123456789abcdef") < 0) {
        return NULL;
    }
    if (check_bytes_find_large(8192, 2, "ab") < 0) {
        return NULL;
    }
    if (check_bytes_find_large(16384, 4, "abcd") < 0) {
        return NULL;
    }
    if (check_bytes_find_large(32768, 2, "ab") < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
normalize_path(PyObject *self, PyObject *filename)
{
    Py_ssize_t size = -1;
    wchar_t *encoded = PyUnicode_AsWideCharString(filename, &size);
    if (encoded == NULL) {
        return NULL;
    }

    PyObject *result = PyUnicode_FromWideChar(_Py_normpath(encoded, size), -1);
    PyMem_Free(encoded);

    return result;
}

static PyObject *
get_getpath_codeobject(PyObject *self, PyObject *Py_UNUSED(args)) {
    return _Py_Get_Getpath_CodeObject();
}


// Test _Py_EncodeLocale()
static PyObject *
encode_locale(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    int current_locale = 0;
    PyObject *res = NULL;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|is", &unicode, &current_locale, &errors)) {
        return NULL;
    }

    // Accept embedded null characters
    Py_ssize_t unused_wlen;
    wchar_t *wstr = PyUnicode_AsWideCharString(unicode, &unused_wlen);
    if (wstr == NULL) {
        return NULL;
    }
    _Py_error_handler error_handler = _Py_GetErrorHandler(errors);

    const char *str_canary = (const char*)0x1234;
    char *str = (char*)str_canary;
    size_t error_pos_canary = (size_t)-123;
    size_t error_pos = error_pos_canary;
    const size_t output_length_canary = (size_t)-456;
    size_t output_length = output_length_canary;
    int ret = _Py_EncodeLocale(wstr,
                               &str, &output_length, &error_pos,
                               current_locale, error_handler);
    PyMem_Free(wstr);

    switch(ret) {
    case 0:
        assert(str != NULL && str != str_canary);
        assert(output_length != output_length_canary);
        assert(error_pos == error_pos_canary);
        res = PyBytes_FromStringAndSize(str, output_length);
        PyMem_RawFree(str);
        break;
    case _Py_CODEC_MEMORY_ERROR:
        assert(str == NULL);
        assert(output_length == 0);
        assert(error_pos == 0);
        PyErr_NoMemory();
        break;
    case _Py_CODEC_ENCODE_ERROR:
        assert(str == NULL);
        assert(output_length == 0);
        assert(error_pos != error_pos_canary);
        PyErr_Format(PyExc_RuntimeError, "encode error: pos=%zu", error_pos);
        break;
    case _Py_CODEC_UNSUPPORTED_ERROR_HANDLER:
        assert(str == NULL);
        assert(output_length == 0);
        assert(error_pos == 0);
        PyErr_SetString(PyExc_ValueError, "unsupported error handler");
        break;
    default:
        PyErr_SetString(PyExc_SystemError, "unknown error code");
        break;
    }
    return res;
}


// Test _Py_DecodeLocale()
static PyObject *
decode_locale(PyObject *self, PyObject *args)
{
    char *str;
    Py_ssize_t unused_len;
    int current_locale = 0;
    PyObject *res = NULL;
    const char *errors = NULL;
    // Accept embedded null bytes in str
    if (!PyArg_ParseTuple(args, "y#|is",
                          &str, &unused_len, &current_locale, &errors)) {
        return NULL;
    }
    _Py_error_handler error_handler = _Py_GetErrorHandler(errors);

    const wchar_t *wstr_canary = (const wchar_t*)0x12345;
    wchar_t *wstr = (wchar_t*)wstr_canary;
    const size_t wlen_canary = (size_t)-123;
    size_t wlen = wlen_canary;
    int ret = _Py_DecodeLocale(str, &wstr, &wlen,
                               current_locale, error_handler);

    switch(ret) {
    case 0:
        assert(wstr != NULL && wstr != wstr_canary);
        assert(wlen != wlen_canary);
        res = PyUnicode_FromWideChar(wstr, wlen);
        PyMem_RawFree(wstr);
        break;
    case _Py_CODEC_MEMORY_ERROR:
        assert(wstr == NULL);
        assert(wlen == 0);
        PyErr_NoMemory();
        break;
    case _Py_CODEC_DECODE_ERROR:
        assert(wstr == NULL);
        assert(wlen != wlen_canary);
        PyErr_Format(PyExc_RuntimeError, "decode error: pos=%zu", wlen);
        break;
    case _Py_CODEC_UNSUPPORTED_ERROR_HANDLER:
        assert(wstr == NULL);
        assert(wlen == 0);
        PyErr_SetString(PyExc_ValueError, "unsupported error handler");
        break;
    default:
        PyErr_SetString(PyExc_SystemError, "unknown error code");
        break;
    }
    return res;
}

static PyObject *
set_eval_frame_default(PyObject *self, PyObject *Py_UNUSED(args))
{
    module_state *state = get_module_state(self);
    _PyInterpreterState_SetEvalFrameFunc(_PyInterpreterState_GET(), NULL);
    Py_CLEAR(state->record_list);
    Py_RETURN_NONE;
}

static PyObject *
record_eval(PyThreadState *tstate, struct _PyInterpreterFrame *f, int exc)
{
    if (f->f_funcobj != NULL) {
        PyFunctionObject *func = _PyFrame_GetFunction(f);
        PyObject *module = _get_current_module();
        assert(module != NULL);
        module_state *state = get_module_state(module);
        Py_DECREF(module);
        int res = PyList_Append(state->record_list,
                                FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_name));
        if (res < 0) {
            return NULL;
        }
    }
    return _PyEval_EvalFrameDefault(tstate, f, exc);
}


static PyObject *
set_eval_frame_record(PyObject *self, PyObject *list)
{
    module_state *state = get_module_state(self);
    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a list");
        return NULL;
    }
    Py_XSETREF(state->record_list, Py_NewRef(list));
    _PyInterpreterState_SetEvalFrameFunc(_PyInterpreterState_GET(), record_eval);
    Py_RETURN_NONE;
}

enum {
    STAGED_EVAL_DEFAULT,
    STAGED_EVAL_RETURN,
    STAGED_EVAL_ERROR,
    STAGED_EVAL_RETIRE,
    STAGED_EVAL_ACTIVATE,
    STAGED_EVAL_ENTRY_ERROR,
    STAGED_EVAL_HEADERS,
};

static int
observe_staged_eval(module_state *state, const char *phase)
{
    PyObject *error = PyErr_GetRaisedException();
    state->staged_observing = 1;
    PyObject *observed = PyObject_CallFunction(state->staged_observer, "s", phase);
    if (observed == NULL) {
        state->staged_observing = 0;
        Py_XDECREF(error);
        return -1;
    }
    Py_DECREF(observed);
    state->staged_observing = 0;
    PyErr_SetRaisedException(error);
    return 0;
}

/* A real hook may execute callbacks both before and after a consuming default
 * evaluator, or return without delegating. Keep those paths distinct. */
static PyObject *
staged_eval(PyThreadState *tstate, _PyInterpreterFrame *frame, int exc)
{
    PyObject *module = _get_current_module();
    if (module == NULL) {
        return NULL;
    }
    module_state *state = get_module_state(module);
    if (state->staged_observing ||
        (PyObject *)frame->f_executable != state->staged_code)
    {
        Py_DECREF(module);
        return _PyEval_EvalFrameDefault(tstate, frame, exc);
    }
    PyObject *result = NULL;
    if (state->staged_mode == STAGED_EVAL_HEADERS) {
        PyObject *mapping = PyDict_New();
        if (mapping == NULL) {
            goto done;
        }
        Py_XSETREF(frame->f_locals, mapping);
        if (PyDict_SetItemString(state->staged_result, "mapping", mapping) < 0) {
            goto done;
        }
    }
    if (observe_staged_eval(state, "before") < 0) {
        goto done;
    }
    switch (state->staged_mode) {
        case STAGED_EVAL_DEFAULT:
        case STAGED_EVAL_HEADERS:
            result = _PyEval_EvalFrameDefault(tstate, frame, exc);
            break;
        case STAGED_EVAL_RETURN:
            result = Py_NewRef(state->staged_result);
            break;
        case STAGED_EVAL_ERROR:
            PyErr_SetString(PyExc_ValueError, "staged hook error");
            break;
        case STAGED_EVAL_RETIRE:
            if (_PyEval_RetireStagedFrame(tstate, frame) < 0) {
                goto done;
            }
            /* The strict API must reject a consumed token without reading the
             * old arena storage. This is intentionally the same old address. */
            if (_PyEval_RetireStagedFrame(tstate, frame) != -1 ||
                !PyErr_ExceptionMatches(PyExc_SystemError))
            {
                PyErr_SetString(PyExc_AssertionError,
                                "consumed staged token was accepted");
                goto done;
            }
            PyErr_Clear();
            if (_PyEval_RetireStagedFrame(tstate, NULL) != -1 ||
                !PyErr_ExceptionMatches(PyExc_SystemError))
            {
                PyErr_SetString(PyExc_AssertionError,
                                "empty staged node matched NULL");
                goto done;
            }
            PyErr_Clear();
            result = Py_NewRef(state->staged_result);
            break;
        case STAGED_EVAL_ACTIVATE: {
            _PyInterpreterFrame *previous = tstate->current_frame;
            frame->previous = previous;
            tstate->current_frame = frame;
            int activated = _PyEval_ActivateStagedFrame(tstate, frame);
            int observed = activated == 0
                ? observe_staged_eval(state, "active") : -1;
            tstate->current_frame = previous;
            if (activated < 0) {
                goto done;
            }
            PyObject *error = PyErr_GetRaisedException();
            _PyEval_FrameClearAndPop(tstate, frame, _PyIR_RETIRE_ERROR_UNWIND);
            PyErr_SetRaisedException(error);
            if (observed < 0) {
                goto done;
            }
            result = Py_NewRef(state->staged_result);
            break;
        }
        case STAGED_EVAL_ENTRY_ERROR: {
            _PyThreadStateImpl *impl = (_PyThreadStateImpl *)tstate;
            uintptr_t soft_limit = impl->c_stack_soft_limit;
            /* Leave the hard limit intact. Only the recoverable entry check
             * is forced; restore it before running any observer afterwards. */
#if _Py_STACK_GROWS_DOWN
            impl->c_stack_soft_limit = UINTPTR_MAX;
#else
            impl->c_stack_soft_limit = 1;
#endif
            result = _PyEval_EvalFrameDefault(tstate, frame, exc);
            impl->c_stack_soft_limit = soft_limit;
            break;
        }
    }
    if (observe_staged_eval(state, "after") < 0) {
        Py_CLEAR(result);
    }
done:
    Py_DECREF(module);
    return result;
}

static PyObject *
run_staged_provider(PyObject *module, PyObject *args)
{
    PyObject *function, *payload, *observer;
    const char *mode;
    int object_provider;
    if (!PyArg_ParseTuple(args, "OOOsp:run_staged_provider", &function,
                          &payload, &observer, &mode, &object_provider))
    {
        return NULL;
    }
    if (!PyFunction_Check(function)) {
        PyErr_SetString(PyExc_TypeError, "expected a Python function");
        return NULL;
    }
    module_state *state = get_module_state(module);
    if (state->staged_running) {
        PyErr_SetString(PyExc_RuntimeError, "staged provider probe is active");
        return NULL;
    }
    const char *modes[] = {"default", "return", "error", "retire", "activate",
                           "entry_error", "headers"};
    size_t selected;
    for (selected = 0; selected < Py_ARRAY_LENGTH(modes); selected++) {
        if (strcmp(mode, modes[selected]) == 0) {
            break;
        }
    }
    if (selected == Py_ARRAY_LENGTH(modes)) {
        PyErr_SetString(PyExc_ValueError, "unknown staged hook mode");
        return NULL;
    }
    if (selected == STAGED_EVAL_HEADERS && !PyDict_Check(payload)) {
        PyErr_SetString(PyExc_TypeError, "headers probe requires a dict payload");
        return NULL;
    }
    state->staged_running = 1;
    state->staged_mode = (int)selected;
    state->staged_code = Py_NewRef(PyFunction_GET_CODE(function));
    state->staged_observer = Py_NewRef(observer);
    state->staged_result = Py_NewRef(payload);
    PyThreadState *tstate = _PyThreadState_GET();
    _PyFrameEvalFunction previous = _PyInterpreterState_GetEvalFrameFunc(tstate->interp);
    int previous_allow = tstate->interp->eval_frame_allow_specialization;
    _PyInterpreterState_SetEvalFrameFunc(tstate->interp, staged_eval);
    PyObject *result;
    if (object_provider) {
        PyObject *locals = ((PyCodeObject *)state->staged_code)->co_flags & CO_OPTIMIZED
            ? NULL : PyFunction_GET_GLOBALS(function);
        result = _PyEval_Vector(tstate, (PyFunctionObject *)function, locals,
                               &payload, 1, NULL);
    }
    else {
        PyValue argument = PyValue_DUP(PyValue_FromObjectBorrow(payload));
        PyValue value = _PyEval_VectorValue(tstate, (PyFunctionObject *)function,
                                            &argument, 1, NULL);
        PyValue_CLOSE(argument);
        result = PyValue_IsNull(value) ? NULL : PyValue_AsObject(value);
        PyValue_XCLOSE(value);
    }
    _PyInterpreterState_SetEvalFrameFunc(tstate->interp, previous);
    _PyInterpreterState_SetEvalFrameAllowSpecialization(tstate->interp, previous_allow);
    Py_CLEAR(state->staged_code);
    Py_CLEAR(state->staged_observer);
    Py_CLEAR(state->staged_result);
    state->staged_running = 0;
    return result;
}

// Defined in interpreter.c
extern PyObject*
Test_EvalFrame(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag);
extern int Test_EvalFrame_Resumes, Test_EvalFrame_Loads;

static PyObject *
get_eval_frame_stats(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *res = PyDict_New();
    if (res == NULL) {
        return NULL;
    }
    PyObject *resumes = PyLong_FromLong(Test_EvalFrame_Resumes);
    if (resumes == NULL || PyDict_SetItemString(res, "resumes", resumes) < 0) {
        Py_XDECREF(resumes);
        Py_DECREF(res);
        return NULL;
    }
    Py_DECREF(resumes);
    PyObject *loads = PyLong_FromLong(Test_EvalFrame_Loads);
    if (loads == NULL || PyDict_SetItemString(res, "loads", loads) < 0) {
        Py_XDECREF(loads);
        Py_DECREF(res);
        return NULL;
    }
    Py_DECREF(loads);
    Test_EvalFrame_Resumes = Test_EvalFrame_Loads = 0;
    return res;
}

static PyObject *
record_eval_interp(PyThreadState *tstate, struct _PyInterpreterFrame *f, int exc)
{
    if (f->f_funcobj != NULL) {
        PyFunctionObject *func = _PyFrame_GetFunction(f);
        PyObject *module = _get_current_module();
        assert(module != NULL);
        module_state *state = get_module_state(module);
        Py_DECREF(module);
        int res = PyList_Append(state->record_list,
                                FT_ATOMIC_LOAD_PTR_ACQUIRE(func->func_name));
        if (res < 0) {
            return NULL;
        }
    }

    return Test_EvalFrame(tstate, f, exc);
}

static PyObject *
set_eval_frame_interp(PyObject *self, PyObject *args)
{
    if (PyTuple_GET_SIZE(args) == 1) {
        module_state *state = get_module_state(self);
        PyValueBorrow item = PyTuple_GET_VALUE(args, 0);
        if (!PyValue_IsObject(item) ||
            !PyList_Check(PyValue_AsKnownObjectBorrow(item))) {
            PyErr_SetString(PyExc_TypeError, "argument must be a list");
            return NULL;
        }
        PyObject *list = PyValue_AsKnownObjectBorrow(item);
        Py_XSETREF(state->record_list, Py_NewRef(list));
        _PyInterpreterState_SetEvalFrameFunc(_PyInterpreterState_GET(), record_eval_interp);
        _PyInterpreterState_SetEvalFrameAllowSpecialization(_PyInterpreterState_GET(), 1);
    } else {
        _PyInterpreterState_SetEvalFrameFunc(_PyInterpreterState_GET(), Test_EvalFrame);
        _PyInterpreterState_SetEvalFrameAllowSpecialization(_PyInterpreterState_GET(), 1);
    }

    Py_RETURN_NONE;
}

static PyObject *
is_specialization_enabled(PyObject *self, PyObject *Py_UNUSED(args))
{
    return PyBool_FromLong(
        _PyInterpreterState_IsSpecializationEnabled(_PyInterpreterState_GET()));
}

/*[clinic input]

_testinternalcapi.compiler_cleandoc -> object

    doc: unicode

C implementation of inspect.cleandoc().
[clinic start generated code]*/

static PyObject *
_testinternalcapi_compiler_cleandoc_impl(PyObject *module, PyObject *doc)
/*[clinic end generated code: output=2dd203a80feff5bc input=2de03fab931d9cdc]*/
{
    return _PyCompile_CleanDoc(doc);
}

/*[clinic input]

_testinternalcapi.new_instruction_sequence -> object

Return a new, empty InstructionSequence.
[clinic start generated code]*/

static PyObject *
_testinternalcapi_new_instruction_sequence_impl(PyObject *module)
/*[clinic end generated code: output=ea4243fddb9057fd input=1dec2591b173be83]*/
{
    return _PyInstructionSequence_New();
}

/*[clinic input]

_testinternalcapi.compiler_codegen -> object

  ast: object
  filename: object
  optimize: int
  compile_mode: int = 0

Apply compiler code generation to an AST.

Return (instruction_sequence, metadata).  metadata maps "argcount",
"posonlyargcount", "kwonlyargcount" to ints and "consts" to the list of
constants in LOAD_CONST index order (for use with optimize_cfg).
[clinic start generated code]*/

static PyObject *
_testinternalcapi_compiler_codegen_impl(PyObject *module, PyObject *ast,
                                        PyObject *filename, int optimize,
                                        int compile_mode)
/*[clinic end generated code: output=40a68f6e13951cc8 input=e0c65e5c80efe30e]*/
{
    PyCompilerFlags *flags = NULL;
    return _PyCompile_CodeGen(ast, filename, flags, optimize, compile_mode);
}


/*[clinic input]

_testinternalcapi.optimize_cfg -> object

  instructions: object
  consts: object
  nlocals: int

Apply compiler optimizations to an instruction list.

consts must be a list aligned with LOAD_CONST opargs (the "consts" entry
from the metadata dict returned by compiler_codegen for the same unit).
[clinic start generated code]*/

static PyObject *
_testinternalcapi_optimize_cfg_impl(PyObject *module, PyObject *instructions,
                                    PyObject *consts, int nlocals)
/*[clinic end generated code: output=57c53c3a3dfd1df0 input=905c3d935e063b27]*/
{
    return _PyCompile_OptimizeCfg(instructions, consts, nlocals);
}

static int
get_nonnegative_int_from_dict(PyObject *dict, const char *key) {
    PyObject *obj;
    if (PyDict_GetItemStringRef(dict, key, &obj) <= 0) {
        return -1;
    }
    int value = PyLong_AsLong(obj);
    Py_DECREF(obj);
    return value;
}

/*[clinic input]

_testinternalcapi.assemble_code_object -> object

  filename: object
  instructions: object
  metadata: object

Create a code object for the given instructions.
[clinic start generated code]*/

static PyObject *
_testinternalcapi_assemble_code_object_impl(PyObject *module,
                                            PyObject *filename,
                                            PyObject *instructions,
                                            PyObject *metadata)
/*[clinic end generated code: output=38003dc16a930f48 input=e713ad77f08fb3a8]*/

{
    assert(PyDict_Check(metadata));
    _PyCompile_CodeUnitMetadata umd = {0};
    PyObject *result = NULL;

    // Hold a strong reference to each entry used by umd.
    if (PyDict_GetItemStringRef(metadata, "name", &umd.u_name) < 0) {
        goto done;
    }
    if (PyDict_GetItemStringRef(metadata, "qualname", &umd.u_qualname) < 0) {
        goto done;
    }
    if (PyDict_GetItemStringRef(metadata, "consts", &umd.u_consts) < 0) {
        goto done;
    }
    if (PyDict_GetItemStringRef(metadata, "names", &umd.u_names) < 0) {
        goto done;
    }
    if (PyDict_GetItemStringRef(metadata, "varnames", &umd.u_varnames) < 0) {
        goto done;
    }
    if (PyDict_GetItemStringRef(metadata, "cellvars", &umd.u_cellvars) < 0) {
        goto done;
    }
    if (PyDict_GetItemStringRef(metadata, "freevars", &umd.u_freevars) < 0) {
        goto done;
    }
    if (PyDict_GetItemStringRef(metadata, "fasthidden", &umd.u_fasthidden) < 0) {
        goto done;
    }

    assert(PyUnicode_Check(umd.u_name));
    assert(PyUnicode_Check(umd.u_qualname));

    if (umd.u_fasthidden == Py_None) {
        Py_CLEAR(umd.u_fasthidden);
    }

    assert(PyDict_Check(umd.u_consts));
    assert(PyDict_Check(umd.u_names));
    assert(PyDict_Check(umd.u_varnames));
    assert(PyDict_Check(umd.u_cellvars));
    assert(PyDict_Check(umd.u_freevars));
    assert(umd.u_fasthidden == NULL || PySet_Check(umd.u_fasthidden));

    umd.u_argcount = get_nonnegative_int_from_dict(metadata, "argcount");
    umd.u_posonlyargcount = get_nonnegative_int_from_dict(metadata, "posonlyargcount");
    umd.u_kwonlyargcount = get_nonnegative_int_from_dict(metadata, "kwonlyargcount");
    umd.u_firstlineno = get_nonnegative_int_from_dict(metadata, "firstlineno");

    assert(umd.u_argcount >= 0);
    assert(umd.u_posonlyargcount >= 0);
    assert(umd.u_kwonlyargcount >= 0);
    assert(umd.u_firstlineno >= 0);

    result = (PyObject*)_PyCompile_Assemble(&umd, filename, instructions);

done:
    Py_XDECREF(umd.u_name);
    Py_XDECREF(umd.u_qualname);
    Py_XDECREF(umd.u_consts);
    Py_XDECREF(umd.u_names);
    Py_XDECREF(umd.u_varnames);
    Py_XDECREF(umd.u_cellvars);
    Py_XDECREF(umd.u_freevars);
    Py_XDECREF(umd.u_fasthidden);
    return result;
}


// Maybe this could be replaced by get_interpreter_config()?
static PyObject *
get_interp_settings(PyObject *self, PyObject *args)
{
    int interpid = -1;
    if (!PyArg_ParseTuple(args, "|i:get_interp_settings", &interpid)) {
        return NULL;
    }

    PyInterpreterState *interp = NULL;
    if (interpid < 0) {
        PyThreadState *tstate = _PyThreadState_GET();
        interp = tstate ? tstate->interp : _PyInterpreterState_Main();
    }
    else if (interpid == 0) {
        interp = _PyInterpreterState_Main();
    }
    else {
        PyErr_Format(PyExc_NotImplementedError,
                     "%d", interpid);
        return NULL;
    }
    assert(interp != NULL);

    PyObject *settings = PyDict_New();
    if (settings == NULL) {
        return NULL;
    }

    /* Add the feature flags. */
    PyObject *flags = PyLong_FromUnsignedLong(interp->feature_flags);
    if (flags == NULL) {
        Py_DECREF(settings);
        return NULL;
    }
    int res = PyDict_SetItemString(settings, "feature_flags", flags);
    Py_DECREF(flags);
    if (res != 0) {
        Py_DECREF(settings);
        return NULL;
    }

    /* "own GIL" */
    PyObject *own_gil = interp->ceval.own_gil ? Py_True : Py_False;
    if (PyDict_SetItemString(settings, "own_gil", own_gil) != 0) {
        Py_DECREF(settings);
        return NULL;
    }

    return settings;
}


static PyObject *
clear_extension(PyObject *self, PyObject *args)
{
    PyObject *name = NULL, *filename = NULL;
    if (!PyArg_ParseTuple(args, "OO:clear_extension", &name, &filename)) {
        return NULL;
    }
    if (_PyImport_ClearExtension(name, filename) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
write_perf_map_entry(PyObject *self, PyObject *args)
{
    PyObject *code_addr_v;
    const void *code_addr;
    PyObject *code_size_s;
    size_t code_size;
    const char *entry_name;

    if (!PyArg_ParseTuple(args, "OOs", &code_addr_v, &code_size_s, &entry_name))
        return NULL;
    code_addr = PyLong_AsVoidPtr(code_addr_v);
    if (code_addr == NULL) {
        return NULL;
    }

    code_size = PyLong_AsSize_t(code_size_s);
    if (code_size == (size_t)-1 && PyErr_Occurred()) {
        return NULL;
    }

    int ret = PyUnstable_WritePerfMapEntry(code_addr, code_size, entry_name);
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return PyLong_FromLong(ret);
}

static PyObject *
perf_map_state_teardown(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(ignored))
{
    PyUnstable_PerfMapState_Fini();
    Py_RETURN_NONE;
}

static PyObject *
iframe_getcode(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    struct _PyInterpreterFrame *f = ((PyFrameObject *)frame)->f_frame;
    return PyUnstable_InterpreterFrame_GetCode(f);
}

static PyObject *
iframe_getline(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    struct _PyInterpreterFrame *f = ((PyFrameObject *)frame)->f_frame;
    return PyLong_FromLong(PyUnstable_InterpreterFrame_GetLine(f));
}

static PyObject *
iframe_getlasti(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    struct _PyInterpreterFrame *f = ((PyFrameObject *)frame)->f_frame;
    return PyLong_FromLong(PyUnstable_InterpreterFrame_GetLasti(f));
}

static PyObject *
code_returns_only_none(PyObject *self, PyObject *arg)
{
    if (!PyCode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a code object");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)arg;
    int res = _PyCode_ReturnsOnlyNone(code);
    return PyBool_FromLong(res);
}

static PyObject *
get_co_framesize(PyObject *self, PyObject *arg)
{
    if (!PyCode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a code object");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)arg;
    return PyLong_FromLong(code->co_framesize);
}

/* The feedback entry of the instruction at word index I of the executing
 * copy of CODE, which this makes if CODE has never run: None if it has
 * none, else (kind,
 * state, first, second, rows), where first and second are the two 32-bit
 * fields after the header and rows lists the (type_version, aux, handler)
 * rows of its poly table. */
static PyObject *
get_feedback(PyObject *self, PyObject *args)
{
    static const char *const kinds[] = {
        "COUNTER", "BRANCH", "UNARY", "BINARY", "ATTR", "GLOBAL", "CALL",
    };
    static const char *const states[] = {"COLD", "MONO", "POLY", "MEGA", "OFF"};
    PyCodeObject *code;
    int i;
    if (!PyArg_ParseTuple(args, "O!i", &PyCode_Type, &code, &i)) {
        return NULL;
    }
    if (i < 0 || i >= Py_SIZE(code)) {
        PyErr_SetString(PyExc_ValueError, "index out of range");
        return NULL;
    }
    _PyCodeExec *exec = _PyCode_GetExec(code);
    if (exec == NULL) {
        return NULL;
    }
    if (_PyCode_InstrFbIndex(code, i) <= 0) {
        Py_RETURN_NONE;
    }
    _PyFbHeader *h = _PyIR_InstrFeedback(code, exec, &exec->words[i]);
    PyObject *rows = PyList_New(0);
    if (rows == NULL) {
        return NULL;
    }
    if (h->poly) {
        _PyFbPoly *poly = &exec->feedback->poly[h->poly - 1];
        for (int r = 0; r < FB_POLY_SIZE && poly->handler[r]; r++) {
            PyObject *row = Py_BuildValue("(IIi)", poly->type_version[r],
                                          poly->aux[r], poly->handler[r]);
            if (row == NULL || PyList_Append(rows, row) < 0) {
                Py_XDECREF(row);
                Py_DECREF(rows);
                return NULL;
            }
            Py_DECREF(row);
        }
    }
    uint32_t fields[2] = {0, 0};
    // Entries are not padded to a common size: read what the type has
    if (_PyIR_FormatOf(_PyCode_InstrHandler(code, i))->fb_size
            >= sizeof(_PyFbHeader) + sizeof(fields)) {
        memcpy(fields, h + 1, sizeof(fields));
    }
    return Py_BuildValue("(ssIIN)", kinds[h->kind], states[h->state],
                         fields[0], fields[1], rows);
}

/* The words of CODE as the compiler emitted them, or with ADAPTIVE those of
 * its executing copy, which specialisation and instrumentation rewrite:
 * per word a tuple (handler, a, b, c, d) of the name of its handler and its
 * fields.  The executing copy of code that has never run is co_code's. */
static PyObject *
get_ir(PyObject *self, PyObject *args)
{
    PyCodeObject *code;
    int adaptive = 0;
    if (!PyArg_ParseTuple(args, "O!|p", &PyCode_Type, &code, &adaptive)) {
        return NULL;
    }
    const _PyIRWord *words = _PyCode_WORDS(code);
    if (adaptive && code->_co_exec != NULL) {
        words = code->_co_exec->words;
    }
    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }
    for (Py_ssize_t w = 0; w < Py_SIZE(code); w++) {
        _PyIRWord word = FT_ATOMIC_LOAD_UINT64_RELAXED(words[w]);
        int handler = IR_HANDLER(word);
        const char *name = handler < IR_FIRST_ONLY_HANDLER
            ? _PyOpcode_OpName[handler]
            : _PyIR_OnlyNames[handler - IR_FIRST_ONLY_HANDLER];
        PyObject *item = Py_BuildValue(
            "(sIIII)", name, IR_A(word), IR_B(word), IR_C(word), IR_D(word));
        if (item == NULL || PyList_Append(result, item) < 0) {
            Py_XDECREF(item);
            Py_DECREF(result);
            return NULL;
        }
        Py_DECREF(item);
    }
    return result;
}

static PyObject *
ir_normal_physical_transfer(PyObject *self, PyObject *args)
{
    int handler, a, b, c, d, arg, edge, home, nl, limit;
    if (!PyArg_ParseTuple(args, "iiiiiiiiii", &handler, &a, &b, &c, &d,
                          &arg, &edge, &home, &nl, &limit)) {
        return NULL;
    }
    if (handler < 0 || handler >= IR_NUM_HANDLERS ||
        a < 0 || a > IR_FIELD_MAX || b < 0 || b > IR_FIELD_MAX ||
        c < 0 || c > IR_FIELD_MAX || d < 0 || d > IR_FIELD_MAX) {
        PyErr_SetString(PyExc_ValueError, "invalid IR word field");
        return NULL;
    }
    _PyIRWord word = IR_WORD(handler, a, b, c, d);
    _PyIRPhysicalTransfer result =
        _PyIR_NormalPhysicalTransfer(word, arg, edge, home, nl, limit);
    return Py_BuildValue("(ii)", result.kind, result.source);
}

/* Set the farthest exit of FOR_ITER and SEND that the assembler does not
 * relax, for code compiled from now on, and return the previous one. */
static PyObject *
set_ir_max_forward(PyObject *self, PyObject *arg)
{
    int value = PyLong_AsInt(arg);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (value < 0 || value > IR_MAX_FORWARD) {
        PyErr_SetString(PyExc_ValueError, "distance out of range");
        return NULL;
    }
    int old = _PyIR_MaxForward;
    _PyIR_MaxForward = value;
    return PyLong_FromLong(old);
}

/* Set the slot limit of the register allocator, for code compiled from now
 * on, and return the previous one. */
static PyObject *
set_ir_max_slots(PyObject *self, PyObject *arg)
{
    int value = PyLong_AsInt(arg);
    if (value == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (value < 0 || value > IR_MAX_SLOTS) {
        PyErr_SetString(PyExc_ValueError, "limit out of range");
        return NULL;
    }
    int old = _PyIR_MaxSlots;
    _PyIR_MaxSlots = value;
    return PyLong_FromLong(old);
}

static PyObject *
get_co_localskinds(PyObject *self, PyObject *arg)
{
    if (!PyCode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a code object");
        return NULL;
    }
    PyCodeObject *co = (PyCodeObject *)arg;

    PyObject *kinds = PyDict_New();
    if (kinds == NULL) {
        return NULL;
    }
    for (int offset = 0; offset < co->co_nlocalsplus; offset++) {
        PyObject *name = _PyTuple_GET_OBJECT(co->co_localsplusnames, offset);
        _PyLocals_Kind k = _PyLocals_GetKind(co->co_localspluskinds, offset);
        PyObject *kind = PyLong_FromLong(k);
        if (kind == NULL) {
            Py_DECREF(kinds);
            return NULL;
        }
        int res = PyDict_SetItem(kinds, name, kind);
        Py_DECREF(kind);
        if (res < 0) {
            Py_DECREF(kinds);
            return NULL;
        }
    }
    return kinds;
}

static PyObject *
get_code_var_counts(PyObject *self, PyObject *_args, PyObject *_kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *codearg;
    PyObject *globalnames = NULL;
    PyObject *attrnames = NULL;
    PyObject *globalsns = NULL;
    PyObject *builtinsns = NULL;
    static char *kwlist[] = {"code", "globalnames", "attrnames", "globalsns",
                             "builtinsns", NULL};
    if (!PyArg_ParseTupleAndKeywords(_args, _kwargs,
                    "O|OOO!O!:get_code_var_counts", kwlist,
                    &codearg, &globalnames, &attrnames,
                    &PyDict_Type, &globalsns, &PyDict_Type, &builtinsns))
    {
        return NULL;
    }
    if (PyFunction_Check(codearg)) {
        if (globalsns == NULL) {
            globalsns = PyFunction_GET_GLOBALS(codearg);
        }
        if (builtinsns == NULL) {
            builtinsns = _PyFunction_GET_BUILTINS(codearg);
        }
        codearg = PyFunction_GET_CODE(codearg);
    }
    else if (!PyCode_Check(codearg)) {
        PyErr_SetString(PyExc_TypeError,
                        "argument must be a code object or a function");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)codearg;

    _PyCode_var_counts_t counts = {0};
    _PyCode_GetVarCounts(code, &counts);
    if (_PyCode_SetUnboundVarCounts(
            tstate, code, &counts, globalnames, attrnames,
            globalsns, builtinsns) < 0)
    {
        return NULL;
    }

#define SET_COUNT(DICT, STRUCT, NAME) \
    do { \
        PyObject *count = PyLong_FromLong(STRUCT.NAME); \
        if (count == NULL) { \
            goto error; \
        } \
        int res = PyDict_SetItemString(DICT, #NAME, count); \
        Py_DECREF(count); \
        if (res < 0) { \
            goto error; \
        } \
    } while (0)

    PyObject *locals = NULL;
    PyObject *args = NULL;
    PyObject *cells = NULL;
    PyObject *hidden = NULL;
    PyObject *unbound = NULL;
    PyObject *globals = NULL;
    PyObject *countsobj = PyDict_New();
    if (countsobj == NULL) {
        return NULL;
    }
    SET_COUNT(countsobj, counts, total);

    // locals
    locals = PyDict_New();
    if (locals == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(countsobj, "locals", locals) < 0) {
        goto error;
    }
    SET_COUNT(locals, counts.locals, total);

    // locals.args
    args = PyDict_New();
    if (args == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(locals, "args", args) < 0) {
        goto error;
    }
    SET_COUNT(args, counts.locals.args, total);
    SET_COUNT(args, counts.locals.args, numposonly);
    SET_COUNT(args, counts.locals.args, numposorkw);
    SET_COUNT(args, counts.locals.args, numkwonly);
    SET_COUNT(args, counts.locals.args, varargs);
    SET_COUNT(args, counts.locals.args, varkwargs);

    // locals.numpure
    SET_COUNT(locals, counts.locals, numpure);

    // locals.cells
    cells = PyDict_New();
    if (cells == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(locals, "cells", cells) < 0) {
        goto error;
    }
    SET_COUNT(cells, counts.locals.cells, total);
    SET_COUNT(cells, counts.locals.cells, numargs);
    SET_COUNT(cells, counts.locals.cells, numothers);

    // locals.hidden
    hidden = PyDict_New();
    if (hidden == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(locals, "hidden", hidden) < 0) {
        goto error;
    }
    SET_COUNT(hidden, counts.locals.hidden, total);
    SET_COUNT(hidden, counts.locals.hidden, numpure);
    SET_COUNT(hidden, counts.locals.hidden, numcells);

    // numfree
    SET_COUNT(countsobj, counts, numfree);

    // unbound
    unbound = PyDict_New();
    if (unbound == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(countsobj, "unbound", unbound) < 0) {
        goto error;
    }
    SET_COUNT(unbound, counts.unbound, total);
    SET_COUNT(unbound, counts.unbound, numattrs);
    SET_COUNT(unbound, counts.unbound, numunknown);

    // unbound.globals
    globals = PyDict_New();
    if (globals == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(unbound, "globals", globals) < 0) {
        goto error;
    }
    SET_COUNT(globals, counts.unbound.globals, total);
    SET_COUNT(globals, counts.unbound.globals, numglobal);
    SET_COUNT(globals, counts.unbound.globals, numbuiltin);
    SET_COUNT(globals, counts.unbound.globals, numunknown);

#undef SET_COUNT

    Py_DECREF(locals);
    Py_DECREF(args);
    Py_DECREF(cells);
    Py_DECREF(hidden);
    Py_DECREF(unbound);
    Py_DECREF(globals);
    return countsobj;

error:
    Py_DECREF(countsobj);
    Py_XDECREF(locals);
    Py_XDECREF(args);
    Py_XDECREF(cells);
    Py_XDECREF(hidden);
    Py_XDECREF(unbound);
    Py_XDECREF(globals);
    return NULL;
}

static PyObject *
verify_stateless_code(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *codearg;
    PyObject *globalnames = NULL;
    PyObject *globalsns = NULL;
    PyObject *builtinsns = NULL;
    static char *kwlist[] = {"code", "globalnames",
                             "globalsns", "builtinsns", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                    "O|O!O!O!:get_code_var_counts", kwlist,
                    &codearg, &PySet_Type, &globalnames,
                    &PyDict_Type, &globalsns, &PyDict_Type, &builtinsns))
    {
        return NULL;
    }
    if (PyFunction_Check(codearg)) {
        if (globalsns == NULL) {
            globalsns = PyFunction_GET_GLOBALS(codearg);
        }
        if (builtinsns == NULL) {
            builtinsns = _PyFunction_GET_BUILTINS(codearg);
        }
        codearg = PyFunction_GET_CODE(codearg);
    }
    else if (!PyCode_Check(codearg)) {
        PyErr_SetString(PyExc_TypeError,
                        "argument must be a code object or a function");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)codearg;

    if (_PyCode_VerifyStateless(
                tstate, code, globalnames, globalsns, builtinsns) < 0)
    {
        return NULL;
    }
    Py_RETURN_NONE;
}

static int _pending_callback(void *arg)
{
    /* we assume the argument is callable object to which we own a reference */
    PyObject *callable = (PyObject *)arg;
    PyObject *r = PyObject_CallNoArgs(callable);
    Py_DECREF(callable);
    Py_XDECREF(r);
    return r != NULL ? 0 : -1;
}

/* The following requests n callbacks to _pending_callback.  It can be
 * run from any python thread.
 */
static PyObject *
pending_threadfunc(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable;
    unsigned int num = 1;
    int blocking = 0;
    int ensure_added = 0;
    static char *kwlist[] = {"callback", "num",
                             "blocking", "ensure_added", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O|I$pp:pending_threadfunc", kwlist,
                                     &callable, &num, &blocking, &ensure_added))
    {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();

    /* create the reference for the callbackwhile we hold the lock */
    for (unsigned int i = 0; i < num; i++) {
        Py_INCREF(callable);
    }

    PyThreadState *save_tstate = NULL;
    if (!blocking) {
        save_tstate = PyEval_SaveThread();
    }

    unsigned int num_added = 0;
    for (; num_added < num; num_added++) {
        if (ensure_added) {
            _Py_add_pending_call_result r;
            do {
                r = _PyEval_AddPendingCall(interp, &_pending_callback, callable, 0);
                assert(r == _Py_ADD_PENDING_SUCCESS
                       || r == _Py_ADD_PENDING_FULL);
            } while (r == _Py_ADD_PENDING_FULL);
        }
        else {
            if (_PyEval_AddPendingCall(interp, &_pending_callback, callable, 0) < 0) {
                break;
            }
        }
    }

    if (!blocking) {
        PyEval_RestoreThread(save_tstate);
    }

    for (unsigned int i = num_added; i < num; i++) {
        Py_DECREF(callable); /* unsuccessful add, destroy the extra reference */
    }

    /* The callable is decref'ed in _pending_callback() above. */
    return PyLong_FromUnsignedLong((unsigned long)num_added);
}


static struct {
    int64_t interpid;
} pending_identify_result;

static int
_pending_identify_callback(void *arg)
{
    PyThread_type_lock mutex = (PyThread_type_lock)arg;
    assert(pending_identify_result.interpid == -1);
    PyThreadState *tstate = PyThreadState_Get();
    pending_identify_result.interpid = PyInterpreterState_GetID(tstate->interp);
    PyThread_release_lock(mutex);
    return 0;
}

static PyObject *
pending_identify(PyObject *self, PyObject *args)
{
    PyObject *interpid;
    if (!PyArg_ParseTuple(args, "O:pending_identify", &interpid)) {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_LookUpIDObject(interpid);
    if (interp == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError, "interpreter not found");
        }
        return NULL;
    }

    pending_identify_result.interpid = -1;

    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return PyErr_NoMemory();
    }
    PyThread_acquire_lock(mutex, WAIT_LOCK);
    /* It gets released in _pending_identify_callback(). */

    _Py_add_pending_call_result r;
    do {
        Py_BEGIN_ALLOW_THREADS
        r = _PyEval_AddPendingCall(interp,
                                   &_pending_identify_callback, (void *)mutex,
                                   0);
        Py_END_ALLOW_THREADS
        assert(r == _Py_ADD_PENDING_SUCCESS
               || r == _Py_ADD_PENDING_FULL);
    } while (r == _Py_ADD_PENDING_FULL);

    /* Wait for the pending call to complete. */
    PyThread_acquire_lock(mutex, WAIT_LOCK);
    PyThread_release_lock(mutex);
    PyThread_free_lock(mutex);

    PyObject *res = PyLong_FromLongLong(pending_identify_result.interpid);
    pending_identify_result.interpid = -1;
    if (res == NULL) {
        return NULL;
    }
    return res;
}

static PyObject *
tracemalloc_get_traceback(PyObject *self, PyObject *args)
{
    unsigned int domain;
    PyObject *ptr_obj;

    if (!PyArg_ParseTuple(args, "IO", &domain, &ptr_obj)) {
        return NULL;
    }
    void *ptr = PyLong_AsVoidPtr(ptr_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }

    return _PyTraceMalloc_GetTraceback(domain, (uintptr_t)ptr);
}


// Test PyThreadState C API
static PyObject *
test_tstate_capi(PyObject *self, PyObject *Py_UNUSED(args))
{
    // PyThreadState_Get()
    PyThreadState *tstate = PyThreadState_Get();
    assert(tstate != NULL);

    // test _PyThreadState_GetDict()
    PyObject *dict = PyThreadState_GetDict();
    assert(dict != NULL);
    // dict is a borrowed reference

    PyObject *dict2 = _PyThreadState_GetDict(tstate);
    assert(dict2 == dict);
    // dict2 is a borrowed reference

    Py_RETURN_NONE;
}


/* Test _PyUnicode_TransformDecimalAndSpaceToASCII() */
static PyObject *
unicode_transformdecimalandspacetoascii(PyObject *self, PyObject *arg)
{
    if (arg == Py_None) {
        arg = NULL;
    }
    return _PyUnicode_TransformDecimalAndSpaceToASCII(arg);
}

static PyObject *
test_pyobject_is_freed(const char *test_name, PyObject *op)
{
    if (!_PyObject_IsFreed(op)) {
        PyErr_SetString(PyExc_AssertionError,
                        "object is not seen as freed");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
check_pyobject_null_is_freed(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *op = NULL;
    return test_pyobject_is_freed("check_pyobject_null_is_freed", op);
}


static PyObject *
check_pyobject_uninitialized_is_freed(PyObject *self,
                                      PyObject *Py_UNUSED(args))
{
    PyObject *op = (PyObject *)PyObject_Malloc(sizeof(PyObject));
    if (op == NULL) {
        return NULL;
    }
    /* Initialize reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* object fields like ob_type are uninitialized! */
    return test_pyobject_is_freed("check_pyobject_uninitialized_is_freed", op);
}


static PyObject *
check_pyobject_forbidden_bytes_is_freed(PyObject *self,
                                        PyObject *Py_UNUSED(args))
{
    /* Allocate an incomplete PyObject structure: truncate 'ob_type' field */
    PyObject *op = (PyObject *)PyObject_Malloc(offsetof(PyObject, ob_type));
    if (op == NULL) {
        return NULL;
    }
    /* Initialize reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* ob_type field is after the memory block: part of "forbidden bytes"
       when using debug hooks on memory allocators! */
    return test_pyobject_is_freed("check_pyobject_forbidden_bytes_is_freed", op);
}


static PyObject *
check_pyobject_freed_is_freed(PyObject *self, PyObject *Py_UNUSED(args))
{
    /* ASan, MSan or TSan would report an error. */
#if defined(_Py_ADDRESS_SANITIZER) || defined(_Py_THREAD_SANITIZER) || defined(_Py_MEMORY_SANITIZER)
    Py_RETURN_NONE;
#else
    PyObject *op = PyObject_CallNoArgs((PyObject *)&PyBaseObject_Type);
    if (op == NULL) {
        return NULL;
    }
    Py_TYPE(op)->tp_dealloc(op);
    /* Reset reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* object memory is freed! */
    return test_pyobject_is_freed("check_pyobject_freed_is_freed", op);
#endif
}


static PyObject *
test_pymem_getallocatorsname(PyObject *self, PyObject *args)
{
    const char *name = _PyMem_GetCurrentAllocatorName();
    if (name == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "cannot get allocators name");
        return NULL;
    }
    return PyUnicode_FromString(name);
}

static PyObject *
get_object_dict_values(PyObject *self, PyObject *obj)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (!_PyType_HasFeature(type, Py_TPFLAGS_INLINE_VALUES)) {
        Py_RETURN_NONE;
    }
    PyDictValues *values = _PyObject_InlineValues(obj);
    if (!values->valid) {
        Py_RETURN_NONE;
    }
    PyDictKeysObject *keys = ((PyHeapTypeObject *)type)->ht_cached_keys;
    assert(keys != NULL);
    int size = (int)keys->dk_nentries;
    assert(size >= 0);
    PyObject *res = PyTuple_New(size);
    if (res == NULL) {
        return NULL;
    }
    if (Py_TYPE(obj) != type ||
        ((PyHeapTypeObject *)type)->ht_cached_keys != keys ||
        keys->dk_nentries != size || !_PyObject_InlineValues(obj)->valid) {
        Py_DECREF(res);
        PyErr_SetString(PyExc_RuntimeError, "inline values changed while reading");
        return NULL;
    }
    values = _PyObject_InlineValues(obj);
    _Py_DECLARE_STR(anon_null, "<NULL>");
    for(int i = 0; i < size; i++) {
        PyValue item = values->values[i];
        PyTuple_SET_VALUE(res, i, PyValue_IsNull(item)
            ? PyValue_DUP(PyValue_FromObjectBorrow(&_Py_STR(anon_null)))
            : PyValue_XDUP(item));
    }
    return res;
}

#ifdef Py_SHAPES
// The shape index of an object with inline values, or None if it has none.
static PyObject *
get_object_shape(PyObject *self, PyObject *obj)
{
    if (!_PyType_HasFeature(Py_TYPE(obj), Py_TPFLAGS_INLINE_VALUES)) {
        Py_RETURN_NONE;
    }
    uint8_t shape = _PyObject_InlineValues(obj)->shape;
    if (shape == _Py_SHAPE_NONE) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(shape);
}

// The number of shapes of a type with inline values.
static PyObject *
get_type_shape_count(PyObject *self, PyObject *type)
{
    if (!PyType_Check(type) ||
        !_PyType_HasFeature((PyTypeObject *)type, Py_TPFLAGS_INLINE_VALUES))
    {
        PyErr_SetString(PyExc_TypeError, "expected a type with inline values");
        return NULL;
    }
    return PyLong_FromLong(_PyType_Shapes((PyTypeObject *)type)->nshapes);
}
#endif


static PyObject*
new_hamt(PyObject *self, PyObject *args)
{
    return _PyContext_NewHamtForTests();
}


static PyObject*
dict_getitem_knownhash(PyObject *self, PyObject *args)
{
    PyObject *mp, *key, *result;
    Py_ssize_t hash;

    if (!PyArg_ParseTuple(args, "OOn:dict_getitem_knownhash",
                          &mp, &key, &hash)) {
        return NULL;
    }
    if (!PyAnyDict_Check(mp)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    int rc = _PyDict_GetItemRef_KnownHash((PyDictObject *)mp, key,
                                          (Py_hash_t)hash, &result);
    if (rc == 0) {
        _PyErr_SetKeyError(key);
    }
    return result;
}


static int
_init_interp_config_from_object(PyInterpreterConfig *config, PyObject *obj)
{
    if (obj == NULL) {
        *config = (PyInterpreterConfig)_PyInterpreterConfig_INIT;
        return 0;
    }

    PyObject *dict = PyObject_GetAttrString(obj, "__dict__");
    if (dict == NULL) {
        PyErr_Format(PyExc_TypeError, "bad config %R", obj);
        return -1;
    }
    int res = _PyInterpreterConfig_InitFromDict(config, dict);
    Py_DECREF(dict);
    if (res < 0) {
        return -1;
    }
    return 0;
}

static PyInterpreterState *
_new_interpreter(PyInterpreterConfig *config, long whence)
{
    if (whence == _PyInterpreterState_WHENCE_XI) {
        return _PyXI_NewInterpreter(config, &whence, NULL, NULL);
    }
    PyObject *exc = NULL;
    PyInterpreterState *interp = NULL;
    if (whence == _PyInterpreterState_WHENCE_UNKNOWN) {
        assert(config == NULL);
        interp = PyInterpreterState_New();
    }
    else if (whence == _PyInterpreterState_WHENCE_CAPI
             || whence == _PyInterpreterState_WHENCE_LEGACY_CAPI)
    {
        PyThreadState *tstate = NULL;
        PyThreadState *save_tstate = PyThreadState_Swap(NULL);
        if (whence == _PyInterpreterState_WHENCE_LEGACY_CAPI) {
            assert(config == NULL);
            tstate = Py_NewInterpreter();
            PyThreadState_Swap(save_tstate);
        }
        else {
            PyStatus status = Py_NewInterpreterFromConfig(&tstate, config);
            PyThreadState_Swap(save_tstate);
            if (PyStatus_Exception(status)) {
                assert(tstate == NULL);
                _PyErr_SetFromPyStatus(status);
                exc = PyErr_GetRaisedException();
            }
        }
        if (tstate != NULL) {
            interp = PyThreadState_GetInterpreter(tstate);
            // Throw away the initial tstate.
            PyThreadState_Swap(tstate);
            PyThreadState_Clear(tstate);
            PyThreadState_Swap(save_tstate);
            PyThreadState_Delete(tstate);
        }
    }
    else {
        PyErr_Format(PyExc_ValueError,
                     "unsupported whence %ld", whence);
        return NULL;
    }

    if (interp == NULL) {
        PyErr_SetString(PyExc_InterpreterError,
                        "sub-interpreter creation failed");
        if (exc != NULL) {
            _PyErr_ChainExceptions1(exc);
        }
    }
    return interp;
}

// This exists mostly for testing the _interpreters module, as an
// alternative to _interpreters.create()
static PyObject *
create_interpreter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"config", "whence", NULL};
    PyObject *configobj = NULL;
    long whence = _PyInterpreterState_WHENCE_XI;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "|O$l:create_interpreter", kwlist,
                                     &configobj, &whence))
    {
        return NULL;
    }
    if (configobj == Py_None) {
        configobj = NULL;
    }

    // Resolve the config.
    PyInterpreterConfig *config = NULL;
    PyInterpreterConfig _config;
    if (whence == _PyInterpreterState_WHENCE_UNKNOWN
            || whence == _PyInterpreterState_WHENCE_LEGACY_CAPI)
    {
        if (configobj != NULL) {
            PyErr_SetString(PyExc_ValueError, "got unexpected config");
            return NULL;
        }
    }
    else {
        config = &_config;
        if (_init_interp_config_from_object(config, configobj) < 0) {
            return NULL;
        }
    }

    // Create the interpreter.
    PyInterpreterState *interp = _new_interpreter(config, whence);
    if (interp == NULL) {
        return NULL;
    }

    // Return the ID.
    PyObject *idobj = _PyInterpreterState_GetIDObject(interp);
    if (idobj == NULL) {
        _PyXI_EndInterpreter(interp, NULL, NULL);
        return NULL;
    }

    return idobj;
}

// This exists mostly for testing the _interpreters module, as an
// alternative to _interpreters.destroy()
static PyObject *
destroy_interpreter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"id", "basic", NULL};
    PyObject *idobj = NULL;
    int basic = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O|p:destroy_interpreter", kwlist,
                                     &idobj, &basic))
    {
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_LookUpIDObject(idobj);
    if (interp == NULL) {
        return NULL;
    }

    if (basic)
    {
        // Test the basic Py_EndInterpreter with weird out of order thread states
        PyThreadState *t1, *t2;
        PyThreadState *prev;
        t1 = interp->threads.head;
        if (t1 == NULL) {
            t1 = PyThreadState_New(interp);
        }
        t2 = PyThreadState_New(interp);
        prev = PyThreadState_Swap(t2);
        // t1 is deliberately left alive; Py_EndInterpreter() must clean it up.
        Py_EndInterpreter(t2);
        PyThreadState_Swap(prev);
    }
    else
    {
        // use the cross interpreter _PyXI_EndInterpreter normally
        _PyXI_EndInterpreter(interp, NULL, NULL);
    }
    Py_RETURN_NONE;
}

// This exists mostly for testing the _interpreters module, as an
// alternative to _interpreters.destroy()
static PyObject *
exec_interpreter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"id", "code", "main", NULL};
    PyObject *idobj;
    const char *code;
    int runningmain = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "Os|$p:exec_interpreter", kwlist,
                                     &idobj, &code, &runningmain))
    {
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_LookUpIDObject(idobj);
    if (interp == NULL) {
        return NULL;
    }

    PyObject *res = NULL;
    PyThreadState *tstate =
        _PyThreadState_NewBound(interp, _PyThreadState_WHENCE_EXEC);

    PyThreadState *save_tstate = PyThreadState_Swap(tstate);

    if (runningmain) {
       if (_PyInterpreterState_SetRunningMain(interp) < 0) {
           goto finally;
       }
    }

    /* only initialise 'cflags.cf_flags' to test backwards compatibility */
    PyCompilerFlags cflags = {0};
    int r = PyRun_SimpleStringFlags(code, &cflags);
    if (PyErr_Occurred()) {
        PyErr_PrintEx(0);
    }

    if (runningmain) {
        _PyInterpreterState_SetNotRunningMain(interp);
    }

    res = PyLong_FromLong(r);

finally:
    PyThreadState_Clear(tstate);
    PyThreadState_Swap(save_tstate);
    PyThreadState_Delete(tstate);
    return res;
}


/* To run some code in a sub-interpreter.

Generally you can use the interpreters module,
but we keep this helper as a distinct implementation.
That's especially important for testing the interpreters module.
*/
static PyObject *
run_in_subinterp_with_config(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *code;
    PyObject *configobj;
    int xi = 0;
    static char *kwlist[] = {"code", "config", "xi", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                    "sO|$p:run_in_subinterp_with_config", kwlist,
                    &code, &configobj, &xi))
    {
        return NULL;
    }

    PyInterpreterConfig config;
    if (_init_interp_config_from_object(&config, configobj) < 0) {
        return NULL;
    }

    /* only initialise 'cflags.cf_flags' to test backwards compatibility */
    PyCompilerFlags cflags = {0};

    int r;
    if (xi) {
        PyThreadState *save_tstate;
        PyThreadState *tstate;

        /* Create an interpreter, staying switched to it. */
        PyInterpreterState *interp = \
                _PyXI_NewInterpreter(&config, NULL, &tstate, &save_tstate);
        if (interp == NULL) {
            return NULL;
        }

        /* Exec the code in the new interpreter. */
        r = PyRun_SimpleStringFlags(code, &cflags);

        /* clean up post-exec. */
        _PyXI_EndInterpreter(interp, tstate, &save_tstate);
    }
    else {
        PyThreadState *substate;
        PyThreadState *mainstate = PyThreadState_Swap(NULL);

        /* Create an interpreter, staying switched to it. */
        PyStatus status = Py_NewInterpreterFromConfig(&substate, &config);
        if (PyStatus_Exception(status)) {
            /* Since no new thread state was created, there is no exception to
               propagate; raise a fresh one after swapping in the old thread
               state. */
            PyThreadState_Swap(mainstate);
            _PyErr_SetFromPyStatus(status);
            PyObject *exc = PyErr_GetRaisedException();
            PyErr_SetString(PyExc_InterpreterError,
                            "sub-interpreter creation failed");
            _PyErr_ChainExceptions1(exc);
            return NULL;
        }

        /* Exec the code in the new interpreter. */
        r = PyRun_SimpleStringFlags(code, &cflags);

        /* clean up post-exec. */
        Py_EndInterpreter(substate);
        PyThreadState_Swap(mainstate);
    }

    return PyLong_FromLong(r);
}


static PyObject *
normalize_interp_id(PyObject *self, PyObject *idobj)
{
    int64_t interpid = _PyInterpreterState_ObjectToID(idobj);
    if (interpid < 0) {
        return NULL;
    }
    return PyLong_FromLongLong(interpid);
}

static PyObject *
next_interpreter_id(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int64_t interpid = _PyRuntime.interpreters.next_id;
    return PyLong_FromLongLong(interpid);
}

static PyObject *
unused_interpreter_id(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    int64_t interpid = INT64_MAX;
    assert(interpid > _PyRuntime.interpreters.next_id);
    return PyLong_FromLongLong(interpid);
}

static PyObject *
interpreter_exists(PyObject *self, PyObject *idobj)
{
    PyInterpreterState *interp = _PyInterpreterState_LookUpIDObject(idobj);
    if (interp == NULL) {
        if (PyErr_ExceptionMatches(PyExc_InterpreterNotFoundError)) {
            PyErr_Clear();
            Py_RETURN_FALSE;
        }
        assert(PyErr_Occurred());
        return NULL;
    }
    Py_RETURN_TRUE;
}

static PyObject *
get_interpreter_refcount(PyObject *self, PyObject *idobj)
{
    PyInterpreterState *interp = _PyInterpreterState_LookUpIDObject(idobj);
    if (interp == NULL) {
        return NULL;
    }
    return PyLong_FromLongLong(interp->id_refcount);
}

static PyObject *
link_interpreter_refcount(PyObject *self, PyObject *idobj)
{
    PyInterpreterState *interp = _PyInterpreterState_LookUpIDObject(idobj);
    if (interp == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    _PyInterpreterState_RequireIDRef(interp, 1);
    Py_RETURN_NONE;
}

static PyObject *
unlink_interpreter_refcount(PyObject *self, PyObject *idobj)
{
    PyInterpreterState *interp = _PyInterpreterState_LookUpIDObject(idobj);
    if (interp == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    _PyInterpreterState_RequireIDRef(interp, 0);
    Py_RETURN_NONE;
}

static PyObject *
interpreter_refcount_linked(PyObject *self, PyObject *idobj)
{
    PyInterpreterState *interp = _PyInterpreterState_LookUpIDObject(idobj);
    if (interp == NULL) {
        return NULL;
    }
    if (_PyInterpreterState_RequiresIDRef(interp)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


static void
_xid_capsule_destructor(PyObject *capsule)
{
    _PyXIData_t *xidata = (_PyXIData_t *)PyCapsule_GetPointer(capsule, NULL);
    if (xidata != NULL) {
        assert(_PyXIData_Release(xidata) == 0);
        _PyXIData_Free(xidata);
    }
}

static PyObject *
get_crossinterp_data(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *obj = NULL;
    PyObject *modeobj = NULL;
    static char *kwlist[] = {"obj", "mode", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                    "O|O:get_crossinterp_data", kwlist,
                    &obj, &modeobj))
    {
        return NULL;
    }
    const char *mode = NULL;
    if (modeobj == NULL || modeobj == Py_None) {
        mode = "xidata";
    }
    else if (!PyUnicode_Check(modeobj)) {
        PyErr_Format(PyExc_TypeError, "expected mode str, got %R", modeobj);
        return NULL;
    }
    else {
        mode = PyUnicode_AsUTF8(modeobj);
        if (mode == NULL) {
            return NULL;
        }
        if (strlen(mode) == 0) {
            mode = "xidata";
        }
    }

    PyThreadState *tstate = _PyThreadState_GET();
    _PyXIData_t *xidata = _PyXIData_New();
    if (xidata == NULL) {
        return NULL;
    }
    if (strcmp(mode, "xidata") == 0) {
        if (_PyObject_GetXIDataNoFallback(tstate, obj, xidata) != 0) {
            goto error;
        }
    }
    else if (strcmp(mode, "fallback") == 0) {
        xidata_fallback_t fallback = _PyXIDATA_FULL_FALLBACK;
        if (_PyObject_GetXIData(tstate, obj, fallback, xidata) != 0)
        {
            goto error;
        }
    }
    else if (strcmp(mode, "pickle") == 0) {
        if (_PyPickle_GetXIData(tstate, obj, xidata) != 0) {
            goto error;
        }
    }
    else if (strcmp(mode, "marshal") == 0) {
        if (_PyMarshal_GetXIData(tstate, obj, xidata) != 0) {
            goto error;
        }
    }
    else if (strcmp(mode, "code") == 0) {
        if (_PyCode_GetXIData(tstate, obj, xidata) != 0) {
            goto error;
        }
    }
    else if (strcmp(mode, "func") == 0) {
        if (_PyFunction_GetXIData(tstate, obj, xidata) != 0) {
            goto error;
        }
    }
    else if (strcmp(mode, "script") == 0) {
        if (_PyCode_GetScriptXIData(tstate, obj, xidata) != 0) {
            goto error;
        }
    }
    else if (strcmp(mode, "script-pure") == 0) {
        if (_PyCode_GetPureScriptXIData(tstate, obj, xidata) != 0) {
            goto error;
        }
    }
    else {
        PyErr_Format(PyExc_ValueError, "unsupported mode %R", modeobj);
        goto error;
    }
    PyObject *capsule = PyCapsule_New(xidata, NULL, _xid_capsule_destructor);
    if (capsule == NULL) {
        assert(_PyXIData_Release(xidata) == 0);
        goto error;
    }
    return capsule;

error:
    _PyXIData_Free(xidata);
    return NULL;
}

static PyObject *
restore_crossinterp_data(PyObject *self, PyObject *args)
{
    PyObject *capsule = NULL;
    if (!PyArg_ParseTuple(args, "O:restore_crossinterp_data", &capsule)) {
        return NULL;
    }

    _PyXIData_t *xidata = (_PyXIData_t *)PyCapsule_GetPointer(capsule, NULL);
    if (xidata == NULL) {
        return NULL;
    }
    return _PyXIData_NewObject(xidata);
}


static PyObject *
raiseTestError(const char* test_name, const char* msg)
{
    PyErr_Format(PyExc_AssertionError, "%s: %s", test_name, msg);
    return NULL;
}


/*[clinic input]
_testinternalcapi.test_long_numbits
[clinic start generated code]*/

static PyObject *
_testinternalcapi_test_long_numbits_impl(PyObject *module)
/*[clinic end generated code: output=745d62d120359434 input=f14ca6f638e44dad]*/
{
    struct triple {
        long input;
        uint64_t nbits;
        int sign;
    } testcases[] = {{0, 0, 0},
                     {1L, 1, 1},
                     {-1L, 1, -1},
                     {2L, 2, 1},
                     {-2L, 2, -1},
                     {3L, 2, 1},
                     {-3L, 2, -1},
                     {4L, 3, 1},
                     {-4L, 3, -1},
                     {0x7fffL, 15, 1},          /* one Python int digit */
             {-0x7fffL, 15, -1},
             {0xffffL, 16, 1},
             {-0xffffL, 16, -1},
             {0xfffffffL, 28, 1},
             {-0xfffffffL, 28, -1}};
    size_t i;

    for (i = 0; i < Py_ARRAY_LENGTH(testcases); ++i) {
        uint64_t nbits;
        int sign = -7;
        PyObject *plong;

        plong = PyLong_FromLong(testcases[i].input);
        if (plong == NULL)
            return NULL;
        nbits = _PyLong_NumBits(plong);
        (void)PyLong_GetSign(plong, &sign);

        Py_DECREF(plong);
        if (nbits != testcases[i].nbits)
            return raiseTestError("test_long_numbits",
                            "wrong result for _PyLong_NumBits");
        if (sign != testcases[i].sign)
            return raiseTestError("test_long_numbits",
                            "wrong result for PyLong_GetSign()");
    }
    Py_RETURN_NONE;
}

static PyObject *
compile_perf_trampoline_entry(PyObject *self, PyObject *args)
{
    PyObject *co;
    if (!PyArg_ParseTuple(args, "O!", &PyCode_Type, &co)) {
        return NULL;
    }
    int ret = PyUnstable_PerfTrampoline_CompileCode((PyCodeObject *)co);
    if (ret != 0) {
        PyErr_SetString(PyExc_AssertionError, "Failed to compile trampoline");
        return NULL;
    }
    return PyLong_FromLong(ret);
}

static PyObject *
perf_trampoline_set_persist_after_fork(PyObject *self, PyObject *args)
{
    int enable;
    if (!PyArg_ParseTuple(args, "i", &enable)) {
        return NULL;
    }
    int ret = PyUnstable_PerfTrampoline_SetPersistAfterFork(enable);
    if (ret == 0) {
        PyErr_SetString(PyExc_AssertionError, "Failed to set persist_after_fork");
        return NULL;
    }
    return PyLong_FromLong(ret);
}


static PyObject *
get_rare_event_counters(PyObject *self, PyObject *type)
{
    PyInterpreterState *interp = PyInterpreterState_Get();

    return Py_BuildValue(
        "{sksksksksk}",
        "set_class", (unsigned long)interp->rare_events.set_class,
        "set_bases", (unsigned long)interp->rare_events.set_bases,
        "set_eval_frame_func", (unsigned long)interp->rare_events.set_eval_frame_func,
        "builtin_dict", (unsigned long)interp->rare_events.builtin_dict,
        "func_modification", (unsigned long)interp->rare_events.func_modification
    );
}

static PyObject *
reset_rare_event_counters(PyObject *self, PyObject *Py_UNUSED(type))
{
    PyInterpreterState *interp = PyInterpreterState_Get();

    interp->rare_events.set_class = 0;
    interp->rare_events.set_bases = 0;
    interp->rare_events.set_eval_frame_func = 0;
    interp->rare_events.builtin_dict = 0;
    interp->rare_events.func_modification = 0;

    return Py_None;
}


#ifdef Py_GIL_DISABLED
static PyObject *
get_py_thread_id(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    uintptr_t tid = _Py_ThreadId();
    Py_BUILD_ASSERT(sizeof(unsigned long long) >= sizeof(tid));
    return PyLong_FromUnsignedLongLong(tid);
}

static PyCodeObject *
get_code(PyObject *obj)
{
    if (PyCode_Check(obj)) {
        return (PyCodeObject *)obj;
    }
    else if (PyFunction_Check(obj)) {
        return (PyCodeObject *)PyFunction_GetCode(obj);
    }
    return (PyCodeObject *)PyErr_Format(
        PyExc_TypeError, "expected function or code object, got %T", obj);
}

static PyObject *
get_tlbc(PyObject *Py_UNUSED(module), PyObject *obj)
{
    PyCodeObject *code = get_code(obj);
    if (code == NULL) {
        return NULL;
    }
    _Py_CODEUNIT *bc = _PyCode_GetTLBCFast(PyThreadState_GET(), code);
    if (bc == NULL) {
        Py_RETURN_NONE;
    }
    return PyBytes_FromStringAndSize((const char *)bc, _PyCode_NBYTES(code));
}

static PyObject *
get_tlbc_id(PyObject *Py_UNUSED(module), PyObject *obj)
{
    PyCodeObject *code = get_code(obj);
    if (code == NULL) {
        return NULL;
    }
    _Py_CODEUNIT *bc = _PyCode_GetTLBCFast(PyThreadState_GET(), code);
    if (bc == NULL) {
        Py_RETURN_NONE;
    }
    return PyLong_FromVoidPtr(bc);
}

static PyObject *
get_long_lived_total(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromInt64(PyInterpreterState_Get()->gc.long_lived_total);
}

#endif

static PyObject *
has_inline_values(PyObject *self, PyObject *obj)
{
    if ((Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) &&
        _PyObject_InlineValues(obj)->valid) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
has_split_table(PyObject *self, PyObject *obj)
{
    if (PyDict_Check(obj) && _PyDict_HasSplitTable((PyDictObject *)obj)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

// Circumvents standard version assignment machinery - use with caution and only on
// short-lived heap types
static PyObject *
type_assign_specific_version_unsafe(PyObject *self, PyObject *args)
{
    PyTypeObject *type;
    unsigned int version;
    if (!PyArg_ParseTuple(args, "Oi:type_assign_specific_version_unsafe", &type, &version)) {
        return NULL;
    }
    assert(!PyType_HasFeature(type, Py_TPFLAGS_IMMUTABLETYPE));
    _PyType_SetVersion(type, version);
    Py_RETURN_NONE;
}

/*[clinic input]
gh_119213_getargs

    spam: object = None

Test _PyArg_Parser.kwtuple
[clinic start generated code]*/

static PyObject *
gh_119213_getargs_impl(PyObject *module, PyObject *spam)
/*[clinic end generated code: output=d8d9c95d5b446802 input=65ef47511da80fc2]*/
{
    // It must never have been called in the main interprer
    assert(!_Py_IsMainInterpreter(PyInterpreterState_Get()));
    return Py_NewRef(spam);
}

/*[clinic input]
get_next_dict_keys_version
[clinic start generated code]*/

static PyObject *
get_next_dict_keys_version_impl(PyObject *module)
/*[clinic end generated code: output=e5405a509cf9d423 input=bd1cee7c6b9d3a3c]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    uint32_t keys_version = interp->dict_state.next_keys_version;
    return PyLong_FromLong(keys_version);
}

static PyObject *
get_static_builtin_types(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _PyStaticType_GetBuiltins();
}


static PyObject *
identify_type_slot_wrappers(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _PyType_GetSlotWrapperNames();
}


static PyObject *
has_deferred_refcount(PyObject *self, PyObject *op)
{
    return PyBool_FromLong(_PyObject_HasDeferredRefcount(op));
}

static PyObject *
get_tracked_heap_size(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromInt64(PyInterpreterState_Get()->gc.heap_size);
}

static PyObject *
is_static_immortal(PyObject *self, PyObject *op)
{
    if (_Py_IsStaticImmortal(op)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
incref_decref_delayed(PyObject *self, PyObject *op)
{
    _PyObject_XDecRefDelayed(Py_NewRef(op));
    Py_RETURN_NONE;
}

// Encode an exact int51 or non-NaN float as an inline stack value, check that
// the operations valid on any value treat it as a value, then box it.
static PyObject *
value_box_roundtrip(PyObject *self, PyObject *arg)
{
    PyValue ref;
    if (PyLong_CheckExact(arg)) {
        int64_t i;
        if (PyLong_AsInt64(arg, &i) < 0) {
            return NULL;
        }
        if (!_PyValue_IntFitsInline(i)) {
            PyErr_SetString(PyExc_OverflowError, "int has no inline encoding");
            return NULL;
        }
        ref = PyValue_FromInlineInt(i);
        if (!PyValue_IsInlineInt(ref) || PyValue_InlineInt64(ref) != i
            || _PyValue_FromIntScaled(_PyValue_IntScaled(ref)).bits != ref.bits
            || !PyValue_BothInlineInt(ref, ref) || !PyValue_LongCheck(ref)
            || !PyValue_IsLanguageValue(ref))
        {
            PyErr_SetString(PyExc_AssertionError, "bad inline int");
            return NULL;
        }
    }
    else if (PyFloat_CheckExact(arg)) {
        double d = PyFloat_AS_DOUBLE(arg);
        if (isnan(d)) {
            PyErr_SetString(PyExc_ValueError, "NaN has no inline encoding");
            return NULL;
        }
        ref = PyValue_FromDoubleNotNaN(d);
        double back = PyValue_InlineFloat(ref);
        if (!PyValue_IsInlineFloat(ref) || memcmp(&back, &d, sizeof(d)) != 0
            || PyValue_IsInlineInt(ref) || !PyValue_FloatCheck(ref))
        {
            PyErr_SetString(PyExc_AssertionError, "bad inline float");
            return NULL;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "expected an exact int or float");
        return NULL;
    }
    PyValue dup = PyValue_DUP(ref);
    if (!PyValue_IsInline(ref) || PyValue_IsPointerSpace(ref)
        || PyValue_IsTaggedIndex(ref) || PyValue_IsNull(ref)
        || !PyValue_IsValid(ref)
        || PyValue_RefcountOnObject(ref)
        || PyValue_TYPE(ref) != Py_TYPE(arg)
        || !PyValue_Is(dup, ref) || !PyValue_Is(PyValue_Borrow(ref), ref)
        || !PyValue_Is(PyValue_Borrow(ref), ref)
        || !PyValue_IsInlineAt(&ref) || PyValue_IsPointerSpaceAt(&ref)
        || PyValue_Is(ref, PyValue_None))
    {
        PyErr_SetString(PyExc_AssertionError, "inline value predicates disagree");
        return NULL;
    }
    PyValue_CLOSE(dup);
    PyValue boxed = _PyValue_BoxInlineValue(ref);
    if (PyValue_IsInline(boxed)) {
        return NULL;
    }
    if (!PyValue_IsPointerSpaceAt(&boxed) || PyValue_IsInlineAt(&boxed)) {
        PyValue_CLOSE(boxed);
        PyErr_SetString(PyExc_AssertionError, "box is not a pointer");
        return NULL;
    }
    return PyValue_AsKnownObjectSteal(boxed);
}

// The names of the frame's local variables that hold inline values.
static PyObject *
inline_locals(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    _PyInterpreterFrame *f = ((PyFrameObject *)frame)->f_frame;
    PyCodeObject *co = _PyFrame_GetCode(f);
    PyObject *names = PySet_New(NULL);
    if (names == NULL) {
        return NULL;
    }
    for (int i = 0; i < co->co_nlocalsplus; i++) {
        if (PyValue_IsInline(f->base[i]) &&
            !(_PyLocals_GetKind(co->co_localspluskinds, i) & CO_FAST_CONST) &&
            PySet_Add(names, _PyTuple_GET_OBJECT(co->co_localsplusnames, i)) < 0)
        {
            Py_DECREF(names);
            return NULL;
        }
    }
    return names;
}

// The slots of the frame (locals and temporaries) that hold a mortal
// object with the no-count tag: a borrow, which no frame slot may hold
// (pyjit/design/pyvalue.md D4).
static PyObject *
tagged_mortal_slots(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    _PyInterpreterFrame *f = ((PyFrameObject *)frame)->f_frame;
    PyValue *slots = _PyFrame_Slots(f);
    PyObject *found = PyList_New(0);
    if (found == NULL) {
        return NULL;
    }
    for (int i = 0; i < _PyFrame_NumSlots(f); i++) {
        PyValue v = slots[i];
        if (!PyValue_IsObject(v) || (v.bits & Py_TAG_REFCNT) == 0
            || _Py_IsImmortal(PyValue_AsKnownObjectBorrow(v)))
        {
            continue;
        }
        PyObject *index = PyLong_FromLong(i);
        if (index == NULL || PyList_Append(found, index) < 0) {
            Py_XDECREF(index);
            Py_DECREF(found);
            return NULL;
        }
        Py_DECREF(index);
    }
    return found;
}

/* Exercise the public owning bridge, including canonicalization of an owned
 * object result, which releases a scalar box at once. */
static PyObject *
pyvalue_contract(PyObject *self, PyObject *arg)
{
    PyValue value = PyValue_PrepareOwned(Py_NewRef(arg));
    const char *kind = PyValue_IsInlineInt(value) ? "int51" :
                       PyValue_IsInlineFloat(value) ? "float64" : "object";
    PyValue duplicate = PyValue_XDUP(PyValue_Borrow(value));
    assert(PyValue_Is(value, duplicate));
    assert(PyValue_IsLanguageValue(value));
    PyObject *object = _PyValue_AsObjectAt(duplicate, BOX_SITE_TEST);
    PyValue_XCLOSE(duplicate);
    PyValue_XCLOSE(value);
    if (object == NULL) {
        return NULL;
    }
    return Py_BuildValue("sN", kind, object);
}

/* The count of values found in a home in a non-canonical form (debug builds),
 * and the call sites that reported them. A violation is a fatal error unless
 * _PYTHON_VALUE_HOME_REPORT is set in the environment (see
 * _PyValue_ReportHome()). */
static PyObject *
pyvalue_home_violations(PyObject *self, PyObject *unused)
{
    return PyLong_FromSsize_t(_PyValue_HomeViolationCount());
}

static PyObject *
pyvalue_home_violation_sites(PyObject *self, PyObject *unused)
{
    return _PyValue_HomeViolationSites();
}

/* Append value, an exact int or float with an inline form, to list as a box:
 * a non-canonical store, which the home check rejects. */
static PyObject *
pyvalue_append_box(PyObject *self, PyObject *args)
{
    PyObject *list, *value;
    if (!PyArg_ParseTuple(args, "O!O", &PyList_Type, &list, &value)) {
        return NULL;
    }
    PyValueBorrow ref = PyValue_FromObjectBorrow(value);
    if (!PyValue_IsInline(ref)) {
        PyErr_SetString(PyExc_ValueError, "the value has no inline form");
        return NULL;
    }
    PyValue box = _PyValue_BoxInlineValue(ref);
    if (PyValue_IsInline(box)) {
        return NULL;
    }
    if (_PyList_AppendTakeValue((PyListObject *)list, box) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/* Value API packet V1 (pyjit/design/value-api.md): entry primitives and
 * value format conversions. */

static const char *
pyvalue_kind(PyValueBorrow value)
{
    if (PyValue_IsInlineInt(value)) {
        return "int";
    }
    return PyValue_IsInlineFloat(value) ? "float" : "object";
}

/* (kind, round trip, same bits): the value that the inline borrowed entry
 * makes of obj, the object it boxes back to, and whether its bits equal
 * those of the out-of-line PyValue_FromObjectBorrow(). */
static PyObject *
pyvalue_from_object_borrow_inline(PyObject *self, PyObject *obj)
{
    PyValueBorrow value = PyValue_FromObjectBorrow(obj);
    PyValueBorrow expected = PyValue_FromObjectBorrow(obj);
    PyObject *back = _PyValue_AsObjectAt(value, BOX_SITE_TEST);
    if (back == NULL) {
        return NULL;
    }
    return Py_BuildValue("(sNO)", pyvalue_kind(value), back,
                         value.bits == expected.bits ? Py_True : Py_False);
}

/* The round trip of obj through the typed entry, which must give a borrow
 * of obj itself with an owner's word: tagged only if obj is immortal (B1).
 * Debug builds abort when obj is an exact int or float. */
static PyObject *
pyvalue_from_object_known(PyObject *self, PyObject *obj)
{
    PyValueBorrow value = _PyValue_FromObjectKnown(obj);
    uintptr_t tag = _Py_IsImmortal(obj) ? Py_TAG_REFCNT : 0;
    if (value.bits != ((uintptr_t)obj | tag)) {
        PyErr_SetString(PyExc_AssertionError, "not an owner's word");
        return NULL;
    }
    return _PyValue_AsObjectAt(value, BOX_SITE_TEST);
}

static PyObject *
pyvalue_entry_null(PyObject *self, PyObject *unused)
{
    if (!PyValue_IsNull(_PyValue_FromObjectKnown(NULL))
        || !PyValue_IsNull(PyValue_FromObjectBorrow(NULL)))
    {
        PyErr_SetString(PyExc_AssertionError, "NULL entry is not NULL");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
check_frame_code_pin(PyObject *self, PyObject *arg)
{
    if (!PyCode_Check(arg) || _Py_IsImmortal(arg)) {
        PyErr_SetString(PyExc_ValueError, "expected a mortal code object");
        return NULL;
    }
    Py_ssize_t before = Py_REFCNT(arg);
    _PyInterpreterFrame frame = {
        .f_executable = (PyCodeObject *)Py_NewRef(arg),
    };
    int ok = _PyFrame_GetCode(&frame) == (PyCodeObject *)arg
             && Py_REFCNT(arg) == before + 1;
    Py_CLEAR(frame.f_executable);
    ok = ok && Py_REFCNT(arg) == before;

    PyCodeObject *immortal = (PyCodeObject *)&_Py_InitCleanup;
    frame.f_executable = (PyCodeObject *)Py_NewRef(immortal);
    ok = ok && _Py_IsImmortal((PyObject *)immortal)
         && _PyFrame_GetCode(&frame) == immortal;
    Py_CLEAR(frame.f_executable);
    _PyInterpreterFrame sentinel = {.owner = FRAME_OWNED_BY_INTERPRETER};
    PyObject *sentinel_code = PyUnstable_InterpreterFrame_GetCode(&sentinel);
    ok = ok && sentinel_code == Py_None;
    Py_XDECREF(sentinel_code);
    if (!ok) {
        PyErr_SetString(PyExc_AssertionError, "incorrect frame code ownership");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
check_frame_function_owner(PyObject *self, PyObject *arg)
{
    if (!PyFunction_Check(arg) || _Py_IsImmortal(arg)) {
        PyErr_SetString(PyExc_ValueError, "expected a mortal function");
        return NULL;
    }
    Py_ssize_t before = Py_REFCNT(arg);
    PyValue owner = PyValue_DUP(PyValue_FromObjectBorrow(arg));
    _PyInterpreterFrame frame = {
        .f_funcobj = _PyValue_FunctionOwnerSteal(owner),
    };
    int ok = (PyObject *)_PyFrame_GetFunction(&frame) == arg
             && Py_REFCNT(arg) == before + 1;
    Py_CLEAR(frame.f_funcobj);
    if (!ok || Py_REFCNT(arg) != before) {
        PyErr_SetString(PyExc_AssertionError, "incorrect frame function ownership");
        return NULL;
    }
    Py_RETURN_NONE;
}

/* PyUnicode_FromFormat(format, value) for the canonical value of obj, or
 * PyUnicode_FromFormat(format, value, other) when other is given. */
static PyObject *
pyvalue_format(PyObject *self, PyObject *args)
{
    const char *format;
    PyObject *obj, *other = NULL;
    if (!PyArg_ParseTuple(args, "sO|O", &format, &obj, &other)) {
        return NULL;
    }
    PyValueBorrow value = PyValue_Borrow(PyValue_FromObjectBorrow(obj));
    if (other == NULL) {
        return PyUnicode_FromFormat(format, value);
    }
    return PyUnicode_FromFormat(format, value, other);
}

static PyObject *
pyvalue_format_inline(PyObject *self, PyObject *Py_UNUSED(args))
{
    return PyUnicode_FromFormat(
        "%vR|%vS|%vA",
        PyValue_Borrow(PyValue_FromInt64(1000)),
        PyValue_Borrow(PyValue_FromDouble(-0.0)),
        PyValue_Borrow(PyValue_FromInt64(1000)));
}

static PyObject *
pyvalue_format_list_item(PyObject *self, PyObject *list)
{
    if (!PyList_CheckExact(list) || PyList_GET_SIZE(list) != 1) {
        PyErr_SetString(PyExc_TypeError, "expected a one-item list");
        return NULL;
    }
    PyValueBorrow value = PyList_GET_VALUE(list, 0);
    return PyUnicode_FromFormat("%vR", value);
}

/* PyErr_Format(exception, format, value) for the canonical value of obj. */
static PyObject *
pyvalue_err_format(PyObject *self, PyObject *args)
{
    PyObject *exception, *obj;
    const char *format;
    if (!PyArg_ParseTuple(args, "OsO", &exception, &format, &obj)) {
        return NULL;
    }
    return PyErr_Format(exception, format,
                        PyValue_Borrow(PyValue_FromObjectBorrow(obj)));
}

/* PyValue_AsObject() of a tagged index, which is no language value: an
 * assertion failure in a debug build. */
static PyObject *
pyvalue_as_object_index(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _PyValue_AsObjectAt(PyValue_TagIndex(7), BOX_SITE_TEST);
}

/* PyValue_AsKnownObjectBorrow(), or PyValue_AsKnownObjectSteal() if steal
 * is true, of an inline int: a fatal error in every build. */
static PyObject *
pyvalue_as_known_object_inline(PyObject *self, PyObject *steal)
{
    int take = PyObject_IsTrue(steal);
    if (take < 0) {
        return NULL;
    }
    PyValue value = PyValue_FromInlineInt(5);
    if (take) {
        return PyValue_AsKnownObjectSteal(value);
    }
    return Py_NewRef(PyValue_AsKnownObjectBorrow(value));
}

static PyObject *
pyvalue_internal_contract(PyObject *self, PyObject *unused)
{
    PyValue absent = PyValue_PrepareOwned(NULL);
    assert(PyValue_IsNull(absent));
    PyValue invalid[] = {
        PyValue_NULL, PyValue_TagIndex(7),
#ifdef Py_DEBUG
        PyValue_Wrap((void *)0x1000),
#endif
    };
    for (size_t i = 0; i < Py_ARRAY_LENGTH(invalid); i++) {
        // PyValue_AsObject() asserts this.
        assert(!PyValue_IsLanguageValue(invalid[i]));
    }
    Py_RETURN_NONE;
}

static PyObject *
pyvalue_try_xclose(PyObject *self, PyObject *obj)
{
    PyValue owner = PyValue_FromObjectSteal(Py_NewRef(obj));
    PyObject *primary = PyObject_CallNoArgs(PyExc_RuntimeError);
    if (primary == NULL) {
        PyValue_XCLOSE(owner);
        return NULL;
    }
    PyErr_SetRaisedException(Py_NewRef(primary));
    Py_ssize_t before = Py_REFCNT(obj);
    int closed = _PyValue_TryXCloseNoReentry(owner);
    Py_ssize_t after = Py_REFCNT(obj);
    PyObject *actual = PyErr_GetRaisedException();
    int preserved = actual == primary;
    Py_XDECREF(actual);
    Py_DECREF(primary);
    if (!closed) {
        PyValue_XCLOSE(owner);
    }
    if (!preserved) {
        PyErr_SetString(PyExc_AssertionError, "close changed the raised exception");
        return NULL;
    }
    return Py_BuildValue("(inn)", closed, before, after);
}

static PyObject *
pyvalue_try_xclose_last(PyObject *self, PyObject *factory)
{
    PyObject *obj = PyObject_CallNoArgs(factory);
    if (obj == NULL) {
        return NULL;
    }
    PyValue owner = PyValue_FromObjectSteal(obj);
    if (!PyValue_RefcountOnObject(owner) || Py_REFCNT(obj) != 1) {
        PyValue_XCLOSE(owner);
        PyErr_SetString(PyExc_ValueError, "factory must return a unique counted owner");
        return NULL;
    }
    int closed = _PyValue_TryXCloseNoReentry(owner);
    if (closed) {
        /* The object may already be freed if the ownership contract broke. */
        PyErr_SetString(PyExc_AssertionError, "close consumed the last count");
        return NULL;
    }
    Py_ssize_t remaining = Py_REFCNT(obj);
    PyValue_XCLOSE(owner);
    if (remaining != 1) {
        PyErr_SetString(PyExc_AssertionError, "failed close changed its owner");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
pyvalue_try_xclose_internal(PyObject *self, PyObject *Py_UNUSED(unused))
{
    PyValue no_count[] = {
        PyValue_NULL, PyValue_TagIndex(7), PyValue_FromInlineInt(-7),
        PyValue_FromDoubleNotNaN(-0.0), PyValue_None, PyValue_True,
#ifdef Py_DEBUG
        PyValue_Wrap((void *)0x1000),
#endif
    };
    for (size_t i = 0; i < Py_ARRAY_LENGTH(no_count); i++) {
        if (!_PyValue_TryXCloseNoReentry(no_count[i])) {
            PyErr_SetString(PyExc_AssertionError, "no-count close failed");
            return NULL;
        }
    }
    /* A release-build Wrap has the encoding of a counted pointer; a debug
     * Wrap has an invalid tag. Neither is admitted to a frame home here. */
    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }
    Py_INCREF(list);
    PyValue wrapped = PyValue_Wrap(list);
    int counted = PyValue_RefcountOnObject(wrapped);
    int closed = _PyValue_TryXCloseNoReentry(wrapped);
    int ok = Py_REFCNT(list) == 2 - (closed && counted);
#ifdef Py_GIL_DISABLED
    ok = ok && closed == !counted;
#else
    ok = ok && closed;
#endif
    if (!closed || !counted) {
        Py_DECREF(list);
    }
    Py_DECREF(list);
    if (!ok) {
        PyErr_SetString(PyExc_AssertionError, "incorrect wrapped pointer close");
        return NULL;
    }
#ifndef Py_GIL_DISABLED
    /* Like test_py_set_immortal, use an unregistered stack object so this
     * test neither leaks an immortal allocation nor alters ref-debug totals.
     * The untagged owner predates the change to dynamic immortality. */
    PyObject object = {0};
    object.ob_refcnt = 1;
    object.ob_type = &PyBaseObject_Type;
    PyValue owner = PyValue_FromObjectSteal(&object);
    if (!PyUnstable_SetImmortal(&object)) {
        PyErr_SetString(PyExc_AssertionError, "could not immortalize test object");
        return NULL;
    }
    Py_ssize_t before = Py_REFCNT(&object);
    if (!_PyValue_TryXCloseNoReentry(owner)
        || Py_REFCNT(&object) != before - 1 || !_Py_IsImmortal(&object)) {
        PyErr_SetString(PyExc_AssertionError, "incorrect immortal owner close");
        return NULL;
    }
#endif
    Py_RETURN_NONE;
}

#ifdef __EMSCRIPTEN__
#include "emscripten.h"

EM_JS(int, emscripten_set_up_async_input_device_js, (void), {
    let idx = 0;
    const encoder = new TextEncoder();
    const bufs = [
        encoder.encode("ab\n"),
        encoder.encode("fi\n"),
        encoder.encode("xy\n"),
    ];
    function sleep(t) {
        return new Promise(res => setTimeout(res, t));
    }
    FS.createAsyncInputDevice("/dev", "blah", async () => {
        await sleep(5);
        return bufs[(idx ++) % 3];
    });
    return !!Module._Py_EmscriptenStackSwitching;
});

static PyObject *
emscripten_set_up_async_input_device(PyObject *self, PyObject *Py_UNUSED(ignored)) {
    if (emscripten_set_up_async_input_device_js()) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
#endif

static PyObject *
simple_pending_call(PyObject *self, PyObject *callable)
{
    if (_PyEval_AddPendingCall(_PyInterpreterState_GET(), _pending_callback, Py_NewRef(callable), 0) < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
vectorcall_nop(PyObject *callable, PyObject *const *args,
               size_t nargsf, PyObject *kwnames)
{
    Py_RETURN_NONE;
}

static PyObject *
set_vectorcall_nop(PyObject *self, PyObject *func)
{
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "expected function");
        return NULL;
    }

    PyFunction_SetVectorcall((PyFunctionObject *)func, vectorcall_nop);
    Py_RETURN_NONE;
}

static PyObject *
module_get_gc_hooks(PyObject *self, PyObject *arg)
{
    PyModuleObject *mod = (PyModuleObject *)arg;
    PyObject *traverse = NULL;
    PyObject *clear = NULL;
    PyObject *free = NULL;
    PyObject *result = NULL;
    traverse = PyLong_FromVoidPtr(mod->md_state_traverse);
    if (!traverse) {
        goto finally;
    }
    clear = PyLong_FromVoidPtr(mod->md_state_clear);
    if (!clear) {
        goto finally;
    }
    free = PyLong_FromVoidPtr(mod->md_state_free);
    if (!free) {
        goto finally;
    }
    result = PyTuple_FromArray((PyObject*[]){ traverse, clear, free }, 3);
finally:
    Py_XDECREF(traverse);
    Py_XDECREF(clear);
    Py_XDECREF(free);
    return result;
}


static void
check_threadstate_set_stack_protection(PyThreadState *tstate,
                                       void *start, size_t size)
{
    assert(PyUnstable_ThreadState_SetStackProtection(tstate, start, size) == 0);
    assert(!PyErr_Occurred());

    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    assert(ts->c_stack_top == (uintptr_t)start + size);
    assert(ts->c_stack_hard_limit <= ts->c_stack_soft_limit);
    assert(ts->c_stack_soft_limit < ts->c_stack_top);
}


static PyObject *
test_threadstate_set_stack_protection(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyThreadState *tstate = PyThreadState_GET();
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    assert(!PyErr_Occurred());

    uintptr_t init_base = ts->c_stack_init_base;
    size_t init_top = ts->c_stack_init_top;

    // Test the minimum stack size
    size_t size = _PyOS_MIN_STACK_SIZE;
    void *start = (void*)(_Py_get_machine_stack_pointer() - size);
    check_threadstate_set_stack_protection(tstate, start, size);

    // Test a larger size
    size = 7654321;
    assert(size > _PyOS_MIN_STACK_SIZE);
    start = (void*)(_Py_get_machine_stack_pointer() - size);
    check_threadstate_set_stack_protection(tstate, start, size);

    // Test invalid size (too small)
    size = 5;
    start = (void*)(_Py_get_machine_stack_pointer() - size);
    assert(PyUnstable_ThreadState_SetStackProtection(tstate, start, size) == -1);
    assert(PyErr_ExceptionMatches(PyExc_ValueError));
    PyErr_Clear();

    // Test PyUnstable_ThreadState_ResetStackProtection()
    PyUnstable_ThreadState_ResetStackProtection(tstate);
    assert(ts->c_stack_init_base == init_base);
    assert(ts->c_stack_init_top == init_top);

    Py_RETURN_NONE;
}

#define NUM_GUARDS 100

static PyObject *
test_interp_guard_countdown(PyObject *self, PyObject *unused)
{
    PyThreadState *save_tstate = PyThreadState_Swap(NULL);

    // This test assumes that the interpreter has no guards active.
    // While this is currently true for the main interpreter as of writing,
    // this won't necessarily be true in the future. For the sake of
    // maintainance, we create a new interpreter to be sure that there aren't
    // any other guards.
    PyThreadState *interp_tstate = Py_NewInterpreter();
    assert(interp_tstate != NULL);
    PyInterpreterState *interp = PyInterpreterState_Get();
    assert(_PyInterpreterState_GuardCountdown(interp) == 0);

    PyInterpreterGuard *guards[NUM_GUARDS];
    for (int i = 0; i < NUM_GUARDS; ++i) {
        guards[i] = PyInterpreterGuard_FromCurrent();
        assert(guards[i] != 0);
        assert(_PyInterpreterState_GuardCountdown(interp) == i + 1);
    }

    for (int i = 0; i < NUM_GUARDS; ++i) {
        PyInterpreterGuard_Close(guards[i]);
        assert(_PyInterpreterState_GuardCountdown(interp) == (NUM_GUARDS - i - 1));
    }

    Py_EndInterpreter(interp_tstate);
    PyThreadState_Swap(save_tstate);
    Py_RETURN_NONE;
}

static PyObject *
test_interp_view_countdown(PyObject *self, PyObject *unused)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    PyInterpreterView *view = PyInterpreterView_FromCurrent();
    if (view == NULL) {
        return NULL;
    }
    assert(_PyInterpreterState_GuardCountdown(interp) == 0);

    PyInterpreterGuard *guards[NUM_GUARDS];

    for (int i = 0; i < NUM_GUARDS; ++i) {
        guards[i] = PyInterpreterGuard_FromView(view);
        assert(guards[i] != 0);
        assert(_PyInterpreterGuard_GetInterpreter(guards[i]) == interp);
        assert(_PyInterpreterState_GuardCountdown(interp) == i + 1);
    }

    for (int i = 0; i < NUM_GUARDS; ++i) {
        PyInterpreterGuard_Close(guards[i]);
        assert(_PyInterpreterState_GuardCountdown(interp) == (NUM_GUARDS - i - 1));
    }

    PyInterpreterView_Close(view);
    Py_RETURN_NONE;
}

#undef NUM_LOCKS

static PyObject *
_pyerr_setkeyerror(PyObject *self, PyObject *arg)
{
    // Test that _PyErr_SetKeyError() overrides the current exception
    // if an exception is set
    PyErr_NoMemory();

    _PyErr_SetKeyError(arg);

    assert(PyErr_Occurred());
    return NULL;
}

static PyObject *
test_thread_state_ensure_from_view_interp_switch(PyObject *self, PyObject *unused)
{
    /* The main tstate is already attached and was NOT created by
     * PyThreadState_Ensure, so delete_on_release == 0. */
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp != NULL);
    PyInterpreterView *view = PyInterpreterView_FromCurrent();
    assert(view != NULL);

    /* First Ensure/Release pair on this pre-existing tstate. */
    assert(_PyThreadState_GET() != NULL);
    PyThreadStateToken *t1 = PyThreadState_EnsureFromView(view);
    assert(t1 != NULL);
    assert(_PyInterpreterState_GuardCountdown(interp) == 1);
    PyThreadState_Release(t1);
    assert(_PyInterpreterState_GuardCountdown(interp) == 0);
    assert(_PyThreadState_GET() != NULL);

    /* tstate->ensure.owned_guard now points at the freed guard. */

    /* Re-attach: Bug B detaches us as a side effect (separate repro). */
    PyThreadState *save = PyThreadState_Swap(NULL);

    PyThreadStateToken *t2 = PyThreadState_EnsureFromView(view);
    assert(_PyInterpreterState_GuardCountdown(interp) == 1);
    assert(t2 != NULL);
    PyThreadState_Release(t2);
    assert(_PyInterpreterState_GuardCountdown(interp) == 0);
    assert(_PyThreadState_GET() == NULL);

    PyThreadState_Swap(save);

    /* In a release build (no assertion) the second Ensure silently
     * skipped storing its guard and Release decremented the global
     * counter from 0, wrapping it to GUARDS_NOT_ALLOWED.  All future
     * guard acquisitions then fail: */
    PyInterpreterGuard *g = PyInterpreterGuard_FromCurrent();
    assert(g != NULL);
    assert(_PyInterpreterState_GuardCountdown(interp) == 1);
    PyInterpreterGuard_Close(g);
    assert(_PyInterpreterState_GuardCountdown(interp) == 0);

    PyInterpreterView_Close(view);
    Py_RETURN_NONE;
}

static PyObject *
unicodewriter_overflow(PyObject *self, PyObject *unused)
{
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }
    if (PyUnicodeWriter_WriteASCII(writer, "hello", -1) < 0) {
        PyUnicodeWriter_Discard(writer);
        return NULL;
    }

    _PyUnicodeWriter *impl = (_PyUnicodeWriter*)writer;
    PyObject *buffer = impl->buffer;
    Py_ssize_t index = PyUnicode_GET_LENGTH(buffer);
    PyUnicode_WRITE(impl->kind, impl->data, index, '#');  // overflow!

    // Spoiler: the function doesn't return if an overflow is detected
    // in debug mode
    return PyUnicodeWriter_Finish(writer);
}

/* Self interrupting context manager */

typedef struct {
    PyObject_HEAD
    int within;
} SelfInterruptingContextManagerObject;

static PyObject *
new_self_interrupting(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SelfInterruptingContextManagerObject *self =
        (SelfInterruptingContextManagerObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->within = 0;
    }
    return (PyObject *)self;
}

static PyObject *
self_interrupting_enter(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    ((SelfInterruptingContextManagerObject *)op)->within = 1;
    PyThreadState *tstate = PyThreadState_Get();
    PyObject *ki = Py_NewRef(PyExc_KeyboardInterrupt);
    PyObject *old_exc = _Py_atomic_exchange_ptr(&tstate->async_exc, ki);
    _Py_set_eval_breaker_bit(tstate, _PY_ASYNC_EXCEPTION_BIT);
    Py_XDECREF(old_exc);

    return Py_NewRef(op);
}

static PyObject *
self_interrupting_within(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    return PyBool_FromLong(((SelfInterruptingContextManagerObject *)op)->within);
}

static PyObject *
self_interrupting_exit(PyObject *op, PyObject *Py_UNUSED(args)) {
    ((SelfInterruptingContextManagerObject *)op)->within = 0;
    Py_RETURN_NONE;
}

static PyMethodDef self_interrupting_methods[] = {
    {"__enter__", self_interrupting_enter, METH_NOARGS, NULL},
    {"within", self_interrupting_within, METH_NOARGS, NULL},
    {"__exit__", self_interrupting_exit, METH_VARARGS, NULL},
    {NULL, NULL} /* sentinel */
};

static PyTypeObject SelfInterruptingContextManager_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_testcapi.SelfInterruptingContextManager",
    sizeof(SelfInterruptingContextManagerObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_new = new_self_interrupting,
    .tp_methods = self_interrupting_methods,
};


/* The tracing verifier (Python/gc_trace.c) */

static PyObject *
verify_and_get_holes(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (_PyGC_TraceVerify(tstate) < 0) {
        Py_RETURN_NONE;
    }
    return _PyGC_TraceGetHoles(tstate->interp);
}

/* Run a full collection and verify it whatever the verification mode.
   Return the list of holes, or None if the verifier cannot run. */
static PyObject *
gc_verify(PyObject *self, PyObject *Py_UNUSED(args))
{
    return verify_and_get_holes();
}

/* The holes found by the last verification. */
static PyObject *
gc_verify_holes(PyObject *self, PyObject *Py_UNUSED(args))
{
    return _PyGC_TraceGetHoles(_PyInterpreterState_GET());
}

/* The number of verifications that ran in this interpreter. */
static PyObject *
gc_verify_count(PyObject *self, PyObject *Py_UNUSED(args))
{
    return PyLong_FromSsize_t(_PyGC_TraceCount(_PyInterpreterState_GET()));
}


/* Call factory() and keep the result only in a C local across a verified
   collection.  Return (holes, id of the object). */
static PyObject *
gc_verify_with_c_local(PyObject *self, PyObject *factory)
{
    PyObject *obj = PyObject_CallNoArgs(factory);
    if (obj == NULL) {
        return NULL;
    }
    PyObject *holes = verify_and_get_holes();
    PyObject *res = NULL;
    if (holes != NULL) {
        res = Py_BuildValue("(NK)", holes, (unsigned long long)(uintptr_t)obj);
    }
    Py_DECREF(obj);
    return res;
}

/* Call factory() and keep the result only in a C local while calling
   func(id of the object), which may block in another thread.  Return the
   result of func. */
static PyObject *
call_with_c_local(PyObject *self, PyObject *args)
{
    PyObject *factory, *func;
    if (!PyArg_ParseTuple(args, "OO", &factory, &func)) {
        return NULL;
    }
    PyObject *obj = PyObject_CallNoArgs(factory);
    if (obj == NULL) {
        return NULL;
    }
    PyObject *res = PyObject_CallFunction(func, "K",
                                          (unsigned long long)(uintptr_t)obj);
    Py_DECREF(obj);
    return res;
}

/* The array's address, masked so that no word of memory holds it: the
   verifier scans the PyMem blocks that the C stack points to. */
static uintptr_t hidden_array;
#define HIDE_MASK ((uintptr_t)0x5a5a5a5a5a5a5a5aULL)

/* Store factory() in a new array, keeping its address only in
   hidden_array.  The registers that held it are restored on return. */
static Py_NO_INLINE int
store_in_hidden_array(PyObject *factory)
{
    PyObject **array = PyMem_Calloc(4, sizeof(PyObject *));
    if (array == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    array[2] = PyObject_CallNoArgs(factory);
    if (array[2] == NULL) {
        PyMem_Free(array);
        return -1;
    }
    hidden_array = (uintptr_t)array ^ HIDE_MASK;
    return 0;
}

/* Overwrite the dead stack below the caller, where the call to the
   factory may have left the object's address. */
static Py_NO_INLINE void
clear_dead_stack(void)
{
    // Volatile stores, which the compiler may not remove as dead.
    volatile char buf[16384];
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = 0;
    }
}

/* Call factory() and keep the result only in a PyMem_Malloc() array across
   a verified collection: the pattern that a tracing collector cannot see,
   which the verifier must report.  Return (holes, the object). */
static PyObject *
gc_verify_with_pymem_array(PyObject *self, PyObject *factory)
{
    if (store_in_hidden_array(factory) < 0) {
        return NULL;
    }
    clear_dead_stack();
    PyObject *holes = verify_and_get_holes();
    PyObject **array = (PyObject **)(hidden_array ^ HIDE_MASK);
    hidden_array = 0;
    PyObject *res = NULL;
    if (holes != NULL) {
        // Pass on the array's reference, so that the caller can keep the
        // object and with it its address.
        res = Py_BuildValue("(NN)", holes, array[2]);
    }
    else {
        Py_DECREF(array[2]);
    }
    PyMem_Free(array);
    return res;
}

static PyObject *
frame_has_virtual_kwargs(PyObject *self, PyObject *Py_UNUSED(args))
{
    _PyInterpreterFrame *frame = _PyThreadState_GET()->current_frame;
    return PyBool_FromLong(frame != NULL && frame->pending_kwargs != IR_NO_DST);
}

static PyObject *
common_constant_storage(PyObject *module, PyObject *Py_UNUSED(args))
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    int inline_count = 0;
    for (int i = 0; i < NUM_COMMON_CONSTANTS; i++) {
        PyValue value = interp->common_consts[i];
        assert(PyValue_IsLanguageValue(value));
        _PyValue_RecheckHome(value);
        inline_count += PyValue_IsInline(value);
        if (PyValue_IsObject(value)) {
            assert(_Py_IsImmortal(PyValue_AsKnownObjectBorrow(value)));
        }
    }
    PyValue minus_one = interp->common_consts[CONSTANT_MINUS_ONE];
    return Py_BuildValue("(iiii)", NUM_COMMON_CONSTANTS, inline_count,
                         PyValue_IsInlineInt(minus_one),
                         PyValue_IsInlineInt(minus_one) &&
                         PyValue_InlineInt64(minus_one) == -1);
}

static PyObject *
dict_vm_storage(PyObject *module, PyObject *arg)
{
    if (!PyDict_CheckExact(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected an exact dict");
        return NULL;
    }
    Py_ssize_t pos = 0, count = 0, inline_keys = 0, inline_values = 0;
    PyValue key, value;
    int status;
    while ((status = _PyDict_NextValue(arg, &pos, &key, &value, NULL)) > 0) {
        assert(PyValue_IsLanguageValue(key) && PyValue_IsLanguageValue(value));
        _PyValue_RecheckHome(key);
        _PyValue_RecheckHome(value);
        count++;
        inline_keys += PyValue_IsInline(key);
        inline_values += PyValue_IsInline(value);
        /* The dict still owns both: these independent iterator owners
         * cannot release the last reference during storage inspection. */
        PyValue_CLOSE(value);
        PyValue_CLOSE(key);
    }
    if (status < 0) {
        return NULL;
    }
    return Py_BuildValue("(nnn)", count, inline_keys, inline_values);
}

typedef struct {
    int closed;
    int saw_publication;
    PyObject *list;
} list_value_owner_probe;

static void
list_value_owner_closed(PyObject *capsule)
{
    list_value_owner_probe *probe = PyCapsule_GetPointer(
        capsule, "list-value-owner");
    assert(probe != NULL);
    probe->closed++;
    if (probe->list != NULL) {
        PyValueBorrow value = PyList_GET_VALUE(probe->list, 0);
        probe->saw_publication = PyValue_IsInlineInt(value) &&
                                 PyValue_InlineInt64(value) == 37;
    }
}

static PyObject *
test_list_setitem_take_value(PyObject *module, PyObject *Py_UNUSED(args))
{
    list_value_owner_probe probe = {0};
    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }
    PyObject *owner = PyCapsule_New(&probe, "list-value-owner",
                                    list_value_owner_closed);
    if (owner == NULL) {
        Py_DECREF(list);
        return NULL;
    }
    int result = _PyList_SetItemTakeValue((PyListObject *)list, 1,
                                         PyValue_FromObjectSteal(owner));
    if (result != -1 || !PyErr_ExceptionMatches(PyExc_IndexError) ||
        probe.closed != 1) {
        Py_DECREF(list);
        PyErr_SetString(PyExc_AssertionError, "invalid index did not consume owner");
        return NULL;
    }
    PyErr_Clear();
    Py_DECREF(list);

    list = PyList_New(1);
    if (list == NULL) {
        return NULL;
    }
    owner = PyCapsule_New(&probe, "list-value-owner", list_value_owner_closed);
    if (owner == NULL) {
        Py_DECREF(list);
        return NULL;
    }
    result = _PyList_SetItemTakeValue((PyListObject *)list, 0,
                                      PyValue_FromObjectSteal(owner));
    assert(result == 0);
    PyValue alias = PyList_GetValue(list, 0);
    assert(!PyValue_IsNull(alias));
    result = _PyList_SetItemTakeValue((PyListObject *)list, 0, alias);
    assert(result == 0);
    if (probe.closed != 1) {
        Py_DECREF(list);
        PyErr_SetString(PyExc_AssertionError, "aliased replacement lost owner");
        return NULL;
    }
    probe.list = list;
    result = _PyList_SetItemTakeValue((PyListObject *)list, 0,
                                      PyValue_FromInlineInt(37));
    assert(result == 0);
    probe.list = NULL;
    Py_DECREF(list);
    if (probe.closed != 2 || !probe.saw_publication) {
        PyErr_SetString(PyExc_AssertionError, "old owner closed before publication");
        return NULL;
    }
    Py_RETURN_NONE;
}

typedef struct {
    PyMemAllocatorEx saved;
    int called;
} constant_key_allocation_probe;

static void *
constant_key_fail_malloc(void *context, size_t size)
{
    constant_key_allocation_probe *probe = context;
    probe->called++;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &probe->saved);
    return NULL;
}

static void *
constant_key_fail_calloc(void *context, size_t count, size_t size)
{
    return constant_key_fail_malloc(context, 0);
}

static void *
constant_key_fail_realloc(void *context, void *pointer, size_t size)
{
    return constant_key_fail_malloc(context, 0);
}

static void
constant_key_fail_free(void *context, void *pointer)
{
    constant_key_allocation_probe *probe = context;
    probe->saved.free(probe->saved.ctx, pointer);
}

static PyObject *
constant_value_key(PyObject *module, PyObject *args)
{
    Py_ssize_t count = PyTuple_GET_SIZE(args);
    if (count < 1 || count > 2) {
        PyErr_SetString(PyExc_TypeError, "expected a constant and optional bool");
        return NULL;
    }
    int fail = 0;
    if (count == 2) {
        PyValueBorrow flag = PyTuple_GET_VALUE(args, 1);
        if (!PyValue_BoolCheck(flag)) {
            PyErr_SetString(PyExc_TypeError, "failure flag must be bool");
            return NULL;
        }
        fail = PyValue_AsKnownObjectBorrow(flag) == Py_True;
    }
    PyValue input = PyTuple_GetValue(args, 0);
    assert(!PyValue_IsNull(input));
    if (fail) {
        constant_key_allocation_probe probe = {0};
        PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &probe.saved);
        PyMemAllocatorEx failing = {
            .ctx = &probe, .malloc = constant_key_fail_malloc,
            .calloc = constant_key_fail_calloc, .realloc = constant_key_fail_realloc,
            .free = constant_key_fail_free,
        };
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &failing);
        PyValue failed = _PyCode_ConstantValueKey(input);
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &probe.saved);
        int valid = probe.called == 1 && PyValue_IsNull(failed) &&
                    PyErr_ExceptionMatches(PyExc_MemoryError);
        PyValue_XCLOSE(failed);
        if (!valid) {
            PyValue_CLOSE(input);
            PyErr_SetString(PyExc_AssertionError, "constant-key OOM was not observed");
            return NULL;
        }
        PyErr_Clear();
    }
    PyValue key = _PyCode_ConstantValueKey(input);
    if (PyValue_IsNull(key)) {
        PyValue_CLOSE(input);
        return NULL;
    }
    PyObject *object = _PyValue_AsObjectAt(input, BOX_SITE_TEST);
    PyValue_CLOSE(input);
    if (object == NULL) {
        PyValue_CLOSE(key);
        return NULL;
    }
    PyObject *legacy = _PyCode_ConstantKey(object);
    Py_DECREF(object);
    if (legacy == NULL) {
        PyValue_CLOSE(key);
        return NULL;
    }
    PyObject *result = PyTuple_New(3);
    if (result == NULL) {
        Py_DECREF(legacy);
        PyValue_CLOSE(key);
        return NULL;
    }
    PyTuple_SET_VALUE(result, 0, PyValue_FromInlineInt(PyValue_IsInlineInt(key)));
    PyTuple_SET_VALUE(result, 1, key);
    PyTuple_SET_ITEM(result, 2, legacy);
    return result;
}

static PyObject *
repr_guard_probe(PyObject *module, PyObject *args)
{
    if (!PyTuple_Check(args) || PyTuple_GET_SIZE(args) != 2) {
        PyErr_SetString(PyExc_TypeError, "expected two repr-guard operands");
        return NULL;
    }
    PyObject *left = PyTuple_GetItemRef(args, 0);
    if (left == NULL) {
        return NULL;
    }
    PyObject *right = PyTuple_GetItemRef(args, 1);
    if (right == NULL) {
        Py_DECREF(left);
        return NULL;
    }
    int left_entered = 0, right_entered = 0;
    PyObject *result = NULL;
    int first = Py_ReprEnter(left);
    if (first < 0) {
        goto done;
    }
    if (first != 0) {
        PyErr_SetString(PyExc_AssertionError, "operand already guarded at entry");
        goto done;
    }
    left_entered = 1;
    int repeated = Py_ReprEnter(left);
    if (repeated < 0) {
        goto done;
    }
    if (repeated == 0) {
        Py_ReprLeave(left);
        PyErr_SetString(PyExc_AssertionError, "duplicate operand registered twice");
        goto done;
    }
    int other = Py_ReprEnter(right);
    if (other < 0) {
        goto done;
    }
    right_entered = other == 0;
    if (right_entered) {
        Py_ReprLeave(right);
        right_entered = 0;
    }
    PyObject *primary = PyObject_CallNoArgs(PyExc_RuntimeError);
    if (primary == NULL) {
        goto done;
    }
    PyErr_SetRaisedException(Py_NewRef(primary));
    Py_ReprLeave(left);
    left_entered = 0;
    PyObject *actual = PyErr_GetRaisedException();
    int preserved = actual == primary;
    Py_XDECREF(actual);
    Py_DECREF(primary);
    int again = Py_ReprEnter(left);
    if (again < 0) {
        goto done;
    }
    left_entered = again == 0;
    result = Py_BuildValue("(iiiii)", first, repeated, other, again, preserved);
done:
    if (right_entered) {
        Py_ReprLeave(right);
    }
    if (left_entered) {
        Py_ReprLeave(left);
    }
    Py_DECREF(right);
    Py_DECREF(left);
    return result;
}

/* Compile source with copy propagation's kill at code that may run Python
 * turned off, so that test_ir_contract can show that the verifier rejects
 * the fold that results */
static PyObject *
compile_without_copyprop_kill(PyObject *self, PyObject *args)
{
    const char *source, *filename;
    if (!PyArg_ParseTuple(args, "ss:compile_without_copyprop_kill",
                          &source, &filename)) {
        return NULL;
    }
    int old = _PyRegPass_SetKeepAcrossCode(1);
    PyObject *code = Py_CompileString(source, filename, Py_file_input);
    _PyRegPass_SetKeepAcrossCode(old);
    return code;
}

static PyObject *
set_inline_exception_value(PyObject *self, PyObject *exception)
{
    _PyErr_SetObjectValue(_PyThreadState_GET(), exception,
                          PyValue_FromInlineInt(42));
    return NULL;
}

typedef struct {
    PyObjectArenaAllocator original;
    int allocations;
    void *memory[2];
    size_t sizes[2];
} DirtyFrameArena;

static void *
dirty_frame_arena_alloc(void *context, size_t size)
{
    DirtyFrameArena *arena = context;
    void *memory = arena->original.alloc(arena->original.ctx, size);
    if (memory != NULL) {
        memset(memory, 0xa5, size);
        assert(arena->allocations < 2);
        arena->memory[arena->allocations] = memory;
        arena->sizes[arena->allocations++] = size;
    }
    return memory;
}

static void
dirty_frame_arena_free(void *context, void *memory, size_t size)
{
    DirtyFrameArena *arena = context;
    arena->original.free(arena->original.ctx, memory, size);
}

static void
assert_frame_arena_null(_PyInterpreterFrame *frame, int count)
{
    for (int i = 0; i < count; i++) {
        assert(PyValue_IsNull(frame->base[i]));
    }
}

/* Run only in an isolated subprocess: the allocator hook is process-global.
 * No Python callback occurs between installing it and restoring it. */
static PyObject *
test_frame_arena(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyThreadState state = {0};
    DirtyFrameArena arena = {0};
    PyObject_GetArenaAllocator(&arena.original);
    PyObjectArenaAllocator dirty = {
        &arena, dirty_frame_arena_alloc, dirty_frame_arena_free
    };
    PyObject_SetArenaAllocator(&dirty);
    _PyInterpreterFrame *frame = _PyThreadState_PushFrame(
        &state, FRAME_SPECIALS_SIZE + 1);
    PyObject_SetArenaAllocator(&arena.original);
    if (frame == NULL) {
        return PyErr_NoMemory();
    }
    assert(arena.allocations == 1);
    assert_frame_arena_null(frame, 1);
    _PyThreadState_PopFrame(&state, frame);

    _PyStackChunk *root = state.datastack_chunk;
    int large = (int)(state.datastack_limit - state.datastack_top) -
                FRAME_SPECIALS_SIZE - 1;
    assert(_PyThreadState_HasStackSpace(&state, large + FRAME_SPECIALS_SIZE));
    frame = _PyThreadState_PushFrameUnchecked(&state, large);
    assert_frame_arena_null(frame, large);
    _PyThreadState_PopFrame(&state, frame);
    PyObject **high = root->initialized;

    _PyInterpreterFrame *headers[8];
    for (int i = 0; i < 8; i++) {
        assert(_PyThreadState_HasStackSpace(&state, FRAME_SPECIALS_SIZE + 1));
        headers[i] = _PyThreadState_PushFrameUnchecked(&state, 1);
        assert_frame_arena_null(headers[i], 1);
    }
    assert(root->initialized < high);
    for (int i = 8; i-- > 0;) {
        _PyThreadState_PopFrame(&state, headers[i]);
    }
    frame = _PyThreadState_PushFrameUnchecked(&state, large);
    assert_frame_arena_null(frame, large);
    assert(root->initialized == _PyDataStack_Address(frame->base + large));
    _PyThreadState_PopFrame(&state, frame);

    _PyInterpreterFrame *anchor = _PyThreadState_PushFrameUnchecked(&state, 1);
    for (int repeat = 0; repeat < 2; repeat++) {
        _PyStackChunk *cached = state.datastack_cached_chunk;
        PyObject_SetArenaAllocator(&dirty);
        frame = _PyThreadState_PushFrame(&state,
            FRAME_SPECIALS_SIZE + 2 * large);
        PyObject_SetArenaAllocator(&arena.original);
        if (frame == NULL) {
            _PyThreadState_PopFrame(&state, anchor);
            arena.original.free(arena.original.ctx, root, arena.sizes[0]);
            if (cached != NULL) {
                arena.original.free(arena.original.ctx, cached, arena.sizes[1]);
            }
            return PyErr_NoMemory();
        }
        if (repeat) {
            assert(state.datastack_chunk == cached);
        }
        assert_frame_arena_null(frame, 2 * large);
        _PyThreadState_PopFrame(&state, frame);
        assert(state.datastack_chunk == root);
    }
    _PyThreadState_PopFrame(&state, anchor);
    _PyStackChunk *cached = state.datastack_cached_chunk;
    assert(arena.allocations == 2);
    assert(arena.memory[0] == root && arena.memory[1] == cached);
    arena.original.free(arena.original.ctx, cached, arena.sizes[1]);
    arena.original.free(arena.original.ctx, root, arena.sizes[0]);
    Py_RETURN_NONE;
}

/* Read-only addresses for isolated remote-reader corruption tests. */
static PyObject *
remote_debug_layout(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("{s:K,s:n,s:n,s:n,s:i}",
        "descriptor", (unsigned long long)(uintptr_t)&_PyRuntime.debug_offsets,
        "chunk_section", (Py_ssize_t)offsetof(_Py_DebugOffsets, stack_chunk),
        "generator_frame", (Py_ssize_t)offsetof(PyGenObject, gi_iframe),
        "return_slot", (Py_ssize_t)offsetof(_PyInterpreterFrame, return_slot),
        "no_dst", IR_NO_DST);
}

static int
append_transition_root_id(PyObject *object, void *arg)
{
    PyObject *address = PyLong_FromVoidPtr(object);
    if (address == NULL) {
        return -1;
    }
    int status = PyList_Append((PyObject *)arg, address);
    Py_DECREF(address);
    return status;
}

/* Bypass conservative arena/C-stack roots so tests exercise the precise bridge. */
static PyObject *
get_transition_root_ids(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *ids = PyList_New(0);
    if (ids == NULL) {
        return NULL;
    }
    if (_PyGC_TraceVisitTransitionRoots(_PyThreadState_GET(),
                                       append_transition_root_id, ids) < 0)
    {
        Py_DECREF(ids);
        return NULL;
    }
    return ids;
}

/* Observe every node, including prefix-only and disarmed provider records.
 * Code and frame geometry may already be gone; do not rerun a shadow check. */
static PyObject *
get_transition_shadow_status(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *states = PyList_New(0);
    if (states == NULL) {
        return NULL;
    }
    for (_PyTransitionRoots *roots =
             ((_PyThreadStateImpl *)_PyThreadState_GET())->transition_roots;
         roots != NULL; roots = roots->previous)
    {
        PyObject *state = Py_BuildValue("(iiiO)", roots->kind,
                                       roots->retire_reason, roots->shadow_status,
                                       roots->frame != NULL ? Py_True : Py_False);
        if (state == NULL) {
            Py_DECREF(states);
            return NULL;
        }
        int status = PyList_Append(states, state);
        Py_DECREF(state);
        if (status < 0) {
            Py_DECREF(states);
            return NULL;
        }
    }
    return states;
}

/* Read only the published marker. This probe does not infer a receiver or
 * bypass the generator state/read claim required to acquire its value. */
static PyObject *
has_delegated_frame_phase(PyObject *self, PyObject *frame_object)
{
    if (!PyFrame_Check(frame_object)) {
        PyErr_SetString(PyExc_TypeError, "expected a frame");
        return NULL;
    }
    _PyInterpreterFrame *frame = ((PyFrameObject *)frame_object)->f_frame;
    uint16_t phase = frame->phase_state;
    return PyBool_FromLong(phase == _PyIR_PHASE_YIELD_DELEGATED ||
                          phase == (_PyIR_PHASE_CAPTURED | _PyIR_PHASE_YIELD_DELEGATED));
}

/* Observe all active frames head-to-outer. This never starts a comparison;
 * the stored status must come from an actual reached collector frame walk. */
static PyObject *
get_ir_frame_phases(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *states = PyList_New(0);
    if (states == NULL) {
        return NULL;
    }
    for (_PyInterpreterFrame *frame = _PyThreadState_GET()->current_frame;
         frame != NULL; frame = frame->previous)
    {
        PyObject *code = frame->f_executable != NULL
            ? (PyObject *)frame->f_executable : Py_None;
        PyObject *state = Py_BuildValue("(Oi)", code, frame->phase_state);
        if (state == NULL) {
            Py_DECREF(states);
            return NULL;
        }
        int status = PyList_Append(states, state);
        Py_DECREF(state);
        if (status < 0) {
            Py_DECREF(states);
            return NULL;
        }
    }
    return states;
}

/* Probe the cold query with initialized control homes, never executable
 * malformed code. The argument pins the borrowed code in this unpublished frame. */
static PyObject *
check_ir_retirement(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *code_object;
    int site, reason, dirty_home = -1, unaligned = 0;
    static char *keywords[] = {
        "code", "site", "reason", "dirty_home", "unaligned", NULL,
    };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oii|ip:check_ir_retirement",
                                    keywords, &code_object, &site, &reason,
                                    &dirty_home, &unaligned))
    {
        return NULL;
    }
    if (!PyCode_Check(code_object)) {
        PyErr_SetString(PyExc_TypeError, "expected a code object");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)code_object;
    int nslots = _PyFrame_NumSlotsForCodeObject(code);
    if (site < 0 || site > Py_SIZE(code) ||
        reason < _PyIR_RETIRE_NONE || reason > _PyIR_RETIRE_STAGED_FAILURE ||
        (dirty_home != -1 &&
         (dirty_home < code->co_nlocalsplus || dirty_home >= nslots)))
    {
        PyErr_SetString(PyExc_ValueError, "invalid retirement control probe");
        return NULL;
    }
    _PyCodeExec *exec = _PyCode_GetExec(code);
    if (exec == NULL) {
        return NULL;
    }
    PyValue *homes = PyMem_New(PyValue, Py_MAX(nslots, 1));
    if (homes == NULL) {
        return PyErr_NoMemory();
    }
    for (int i = 0; i < nslots; i++) {
        homes[i] = PyValue_NULL;
    }
    if (dirty_home >= 0) {
        homes[dirty_home] = PyValue_FromInlineInt(17);
    }
    uintptr_t pc = (uintptr_t)exec->words + (size_t)site * sizeof(*exec->words)
                   + unaligned;
    _PyInterpreterFrame frame = {
        .f_executable = code,
        .base = homes,
        .ir_words = exec->words,
        .instr_ptr = (_PyInstr *)pc,
        .owner = FRAME_OWNED_BY_THREAD,
    };
    uint8_t status = _PyIR_ShadowRetirement(&frame, (uint8_t)reason);
    PyMem_Free(homes);
    return PyLong_FromLong(status);
}

/* Initialize a covered control shape without executing the image or
 * publishing the fake frame. Only inline values enter the scratch homes. */
static PyObject *
check_ir_frame_phase(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *code_object;
    int site, phase, dirty_home = -1, missing_home = -1;
    int unaligned = 0, wrong_image = 0, authority_only = 0;
    static char *keywords[] = {
        "code", "site", "phase", "dirty_home", "missing_home",
        "unaligned", "wrong_image", "authority_only", NULL,
    };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oii|$iippp:check_ir_frame_phase",
                                    keywords, &code_object, &site, &phase,
                                    &dirty_home, &missing_home, &unaligned,
                                    &wrong_image, &authority_only))
    {
        return NULL;
    }
    if (!PyCode_Check(code_object)) {
        PyErr_SetString(PyExc_TypeError, "expected a code object");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)code_object;
    int nslots = _PyFrame_NumSlotsForCodeObject(code);
    int nl = code->co_nlocalsplus;
    if (site < 0 || site > Py_SIZE(code) || phase < 0 || phase > UINT16_MAX ||
        nl < 0 || nslots < nl || nslots > IR_MAX_SLOTS ||
        (dirty_home != -1 && (dirty_home < nl || dirty_home >= nslots)) ||
        (missing_home != -1 && (missing_home < nl || missing_home >= nslots)))
    {
        PyErr_SetString(PyExc_ValueError, "invalid frame phase control probe");
        return NULL;
    }
    _PyCodeExec *exec = _PyCode_GetExec(code);
    if (exec == NULL) {
        return NULL;
    }
    PyValue *homes = PyMem_New(PyValue, Py_MAX(nslots, 1));
    if (homes == NULL) {
        return PyErr_NoMemory();
    }
    for (int i = 0; i < nslots; i++) {
        homes[i] = PyValue_NULL;
    }
    uintptr_t pc = (uintptr_t)exec->words + (size_t)site * sizeof(*exec->words);
    _PyInterpreterFrame frame = {
        .f_executable = code,
        .base = homes,
        .ir_words = exec->words,
        .instr_ptr = (_PyInstr *)pc,
        .owner = FRAME_OWNED_BY_THREAD,
        .phase_state = (uint16_t)phase,
    };
    _PyIRHomeView view;
    _PyIRHomeAuthority covered = _PyIR_GetFrameHomeView(&frame, &view);
    int dirty_selected = 0, missing_required = 0;
    if (covered) {
        for (uint32_t r = 0; r < view.nranges; r++) {
            const _PyIRPhaseRange *range = view.ranges + r;
            for (int bit = range->begin; bit < range->end; bit++) {
                int home = view.nbindings + bit;
                homes[home] = PyValue_FromInlineInt(17);
                dirty_selected |= home == dirty_home;
                missing_required |= home == missing_home &&
                    !(range->flags & _PyIR_PHASE_RANGE_NULLABLE);
            }
        }
    }
    if ((dirty_home >= 0 && (!covered || dirty_selected)) ||
        (missing_home >= 0 && (!covered || !missing_required)))
    {
        PyMem_Free(homes);
        PyErr_SetString(PyExc_ValueError,
                        "dirty home must be absent and missing home must be required");
        return NULL;
    }
    if (dirty_home >= 0) {
        homes[dirty_home] = PyValue_FromInlineInt(17);
    }
    if (missing_home >= 0) {
        homes[missing_home] = PyValue_NULL;
    }
    _PyIRWord other_image = 0;
    if (wrong_image) {
        frame.ir_words = &other_image;
    }
    frame.instr_ptr = (_PyInstr *)(pc + unaligned);
    int status = authority_only ? (int)_PyIR_GetFrameHomeView(&frame, &view)
                               : _PyIR_ShadowFramePhase(&frame);
    PyMem_Free(homes);
    return PyLong_FromLong(status);
}

static PyObject *
get_frame_home_view(PyObject *self, PyObject *arg)
{
    if (!PyFrame_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected a frame");
        return NULL;
    }
    _PyIRHomeView view = {0};
    _PyIRHomeAuthority authority = _PyFrame_GetHomeView(
        ((PyFrameObject *)arg)->f_frame, &view);
    return Py_BuildValue("(iiii)", authority, view.nbindings,
                         view.scratch_limit, view.nranges);
}

static PyObject *
check_created_home_view_invalidation(PyObject *self, PyObject *arg)
{
    if (!PyFrame_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected a frame");
        return NULL;
    }
    _PyInterpreterFrame *frame = ((PyFrameObject *)arg)->f_frame;
    _PyIRHomeView view;
    if (frame->phase_state != _PyIR_PHASE_CREATED ||
        _PyFrame_GetHomeView(frame, &view) != _PyIR_HOME_MUST_READABLE)
    {
        PyErr_SetString(PyExc_ValueError, "expected certified created frame");
        return NULL;
    }
    /* No callback or home mutation occurs during this invalidation probe. */
    frame->phase_state = 0;
    int denied = _PyFrame_GetHomeView(frame, &view) == _PyIR_HOME_NONE;
    frame->phase_state = _PyIR_PHASE_CREATED;
    if (!denied) {
        PyErr_SetString(PyExc_AssertionError, "created authority was reacquired");
        return NULL;
    }
    Py_RETURN_NONE;
}

/* A cold protocol probe: inactive words are never exposed as Python values,
 * executed, or closed. Only the selected home owns payload. */
#ifndef Py_GIL_DISABLED
static int
count_frame_probe_owners(PyObject *object, void *arg)
{
    (*(int *)arg)++;
    return 0;
}
#endif

static PyObject *
check_frame_copy(PyObject *args, int mode)
{
    int created = mode == 1;
    int delegated = mode >= 2;
    int clipped = mode == 3;
    PyObject *code_object, *payload;
    if (!PyArg_ParseTuple(args, "O!O:check_frame_selected_copy", &PyCode_Type,
                          &code_object, &payload))
    {
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)code_object;
    int n = _PyFrame_NumSlotsForCodeObject(code);
    int nl = code->co_nlocalsplus;
    if ((!created && n - nl < 2) || (created && nl == 0)) {
        PyErr_SetString(PyExc_ValueError, "insufficient probe homes");
        return NULL;
    }
    _PyCodeExec *exec = NULL;
    int site = 0;
    if (created || delegated) {
        exec = _PyCode_GetExec(code);
        if (exec == NULL) {
            return NULL;
        }
        if (created && (exec->semantic_plan == NULL ||
            !(exec->semantic_plan->flags & _PyIR_PLAN_ALL_CREATED_EMPTY)))
        {
            PyErr_SetString(PyExc_ValueError, "probe needs certified empty creation");
            return NULL;
        }
        int opcode = created ? RETURN_GENERATOR : YIELD_DELEGATED;
        while (site < exec->nwords &&
               _PyIR_BaseOpcode(IR_HANDLER(exec->words[site])) != opcode)
        {
            site++;
        }
        if (site == exec->nwords) {
            PyErr_SetString(PyExc_ValueError, "probe needs the actual copy/yield site");
            return NULL;
        }
    }
    size_t size = offsetof(_PyInterpreterFrame, owned_slots) +
        (size_t)n * sizeof(PyValue);
    _PyInterpreterFrame *src = PyMem_Calloc(1, size);
    /* The selectors derive their owner container from the embedded frame.
     * Private untracked storage never executes or invokes an object destructor. */
    size_t offset = created ? offsetof(PyGenObject, gi_iframe) : delegated
        ? offsetof(PyFrameObject, _f_frame_data) : 0;
    void *dst_storage = PyMem_Calloc(1, offset + size);
    _PyInterpreterFrame *dst = dst_storage == NULL ? NULL
        : (_PyInterpreterFrame *)((char *)dst_storage + offset);
    if (src == NULL || dst == NULL) {
        PyMem_Free(src);
        PyMem_Free(dst_storage);
        return PyErr_NoMemory();
    }
    src->f_executable = (PyCodeObject *)Py_NewRef(code);
    src->base = src->owned_slots;
    const uint64_t poison = UINT64_C(0xdeadbeefdeadbeef);
    for (int i = nl; i < n; i++) {
        src->base[i].bits = poison;
    }
    PyValue value = PyValue_DUP(PyValue_FromObjectBorrow(payload));
    int selected_home = delegated ? IR_D(exec->words[site]) : created ? 0 : nl + 1;
    assert(selected_home >= 0 && selected_home < n);
    assert(!delegated || selected_home >= nl);
    src->base[selected_home] = _PyValue_Publish(&value);
    int scratch_home = delegated ? selected_home - nl : 1;
    const _PyIRPhaseRange selected = {
        .begin = (uint16_t)scratch_home,
        .end = (uint16_t)(scratch_home + 1),
    };
    const _PyIRHomeView view = {
        .nbindings = (uint16_t)nl,
        .scratch_limit = (uint16_t)(clipped ? 0 : selected.end),
        .nranges = !clipped, .ranges = &selected,
    };
    if (delegated) {
        src->owner = FRAME_OWNED_BY_GENERATOR;
        src->ir_words = exec->words;
        src->instr_ptr = exec->words + site + 1;
        src->return_slot = IR_A(exec->words[site]);
        _PyFrame_PublishDelegatedSuspension(src);
        assert(_PyIR_DelegatedReceiverHome(src) == selected_home);
    }
    if (created) {
        src->ir_words = exec->words;
        src->instr_ptr = exec->words + site;
        _PyFrame_CopyCreatedGenerator(src, dst);
    }
    else {
        _PyFrame_Copy(src, dst, &view, _PyIR_HOME_MUST_READABLE);
    }
    int correct = clipped
        ? PyValue_IsNull(dst->base[selected_home]) &&
          src->base[selected_home].bits == value.bits
        : dst->base[selected_home].bits == value.bits &&
          PyValue_IsNull(src->base[selected_home]);
    if (delegated) {
        correct &= src->phase_state == 0 &&
            dst->phase_state == (clipped ? 0 : _PyIR_PHASE_YIELD_DELEGATED);
        PyFrameObject *capture = dst_storage;
        dst->owner = FRAME_OWNED_BY_FRAME_OBJECT;
        capture->f_frame = dst;
        capture->f_capture = view;
        capture->f_capture_authority = _PyIR_HOME_MUST_READABLE;
        dst->phase_state |= _PyIR_PHASE_CAPTURED;
        _PyIRHomeView selected_view;
        correct &= _PyFrame_GetHomeView(dst, &selected_view) == _PyIR_HOME_MUST_READABLE;
        correct &= selected_view.scratch_limit == view.scratch_limit;
        correct &= _PyIR_DelegatedReceiverHome(dst) == (clipped ? -1 : selected_home);
    }
    for (int i = nl; i < n; i++) {
        if (i != selected_home) {
            correct &= src->base[i].bits == poison && PyValue_IsNull(dst->base[i]);
        }
    }
#ifndef Py_GIL_DISABLED
    if (created) {
        ((PyGenObject *)dst_storage)->gi_frame_state = FRAME_CREATED;
        dst->owner = FRAME_OWNED_BY_GENERATOR;
        for (int i = nl; i < n; i++) {
            dst->base[i].bits = poison;
        }
        int visited = 0;
        int status = _PyGC_VisitFrameStack(dst, count_frame_probe_owners, &visited);
        correct &= status == 0 && visited == PyValue_IsObject(value);
    }
#endif
    PyValue_XCLOSE(dst->base[selected_home]);
    if (clipped) {
        PyValue_XCLOSE(src->base[selected_home]);
    }
    Py_DECREF(dst->f_executable);
    PyMem_Free(src);
    PyMem_Free(dst_storage);
    if (!correct) {
        PyErr_SetString(PyExc_AssertionError, "copy touched an inactive home");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
check_frame_selected_copy(PyObject *self, PyObject *args)
{
    return check_frame_copy(args, 0);
}

static PyObject *
check_created_frame_copy(PyObject *self, PyObject *args)
{
    return check_frame_copy(args, 1);
}

static PyObject *
check_delegated_frame_copy(PyObject *self, PyObject *args)
{
    return check_frame_copy(args, 2);
}

static PyObject *
check_clipped_delegated_frame_copy(PyObject *self, PyObject *args)
{
    return check_frame_copy(args, 3);
}

static PyObject *
has_empty_created_copy_plan(PyObject *self, PyObject *arg)
{
    if (!PyCode_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "expected code");
        return NULL;
    }
    _PyCodeExec *exec = _PyCode_GetExec((PyCodeObject *)arg);
    if (exec == NULL) {
        return NULL;
    }
    return PyBool_FromLong(exec->semantic_plan != NULL &&
        (exec->semantic_plan->flags & _PyIR_PLAN_ALL_CREATED_EMPTY));
}

static PyMethodDef module_functions[] = {
    {"check_created_home_view_invalidation", check_created_home_view_invalidation, METH_O},
    {"has_empty_created_copy_plan", has_empty_created_copy_plan, METH_O},
    {"check_created_frame_copy", check_created_frame_copy, METH_VARARGS},
    {"get_frame_home_view", get_frame_home_view, METH_O},
    {"check_frame_selected_copy", check_frame_selected_copy, METH_VARARGS},
    {"test_frame_arena", test_frame_arena, METH_NOARGS},
    {"get_transition_root_ids", get_transition_root_ids, METH_NOARGS},
    {"get_transition_shadow_status", get_transition_shadow_status, METH_NOARGS},
    {"check_delegated_frame_copy", check_delegated_frame_copy, METH_VARARGS},
    {"check_clipped_delegated_frame_copy", check_clipped_delegated_frame_copy, METH_VARARGS},
    {"has_delegated_frame_phase", has_delegated_frame_phase, METH_O},
    {"get_ir_frame_phases", get_ir_frame_phases, METH_NOARGS},
    {"check_ir_retirement", _PyCFunction_CAST(check_ir_retirement),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"check_ir_frame_phase", _PyCFunction_CAST(check_ir_frame_phase),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"remote_debug_layout", remote_debug_layout, METH_NOARGS},
    {"set_inline_exception_value", set_inline_exception_value, METH_O},
    {"compile_without_copyprop_kill", compile_without_copyprop_kill,
     METH_VARARGS, NULL},
    {"repr_guard_probe", repr_guard_probe, METH_O},
    {"constant_value_key", constant_value_key, METH_VARARGS},
    {"test_list_setitem_take_value", test_list_setitem_take_value, METH_NOARGS},
    {"dict_vm_storage", dict_vm_storage, METH_O},
    {"common_constant_storage", common_constant_storage, METH_NOARGS},
    {"frame_has_virtual_kwargs", frame_has_virtual_kwargs, METH_NOARGS},
    {"pyvalue_contract", pyvalue_contract, METH_O, NULL},
    {"pyvalue_internal_contract", pyvalue_internal_contract, METH_NOARGS, NULL},
    {"pyvalue_try_xclose", pyvalue_try_xclose, METH_O, NULL},
    {"pyvalue_try_xclose_last", pyvalue_try_xclose_last, METH_O, NULL},
    {"pyvalue_try_xclose_internal", pyvalue_try_xclose_internal, METH_NOARGS, NULL},
    {"pyvalue_home_violations", pyvalue_home_violations, METH_NOARGS, NULL},
    {"pyvalue_home_violation_sites", pyvalue_home_violation_sites, METH_NOARGS, NULL},
    {"pyvalue_append_box", pyvalue_append_box, METH_VARARGS, NULL},
    {"pyvalue_from_object_borrow_inline", pyvalue_from_object_borrow_inline, METH_O, NULL},
    {"pyvalue_from_object_known", pyvalue_from_object_known, METH_O, NULL},
    {"pyvalue_entry_null", pyvalue_entry_null, METH_NOARGS, NULL},
    {"check_frame_code_pin", check_frame_code_pin, METH_O, NULL},
    {"check_frame_function_owner", check_frame_function_owner, METH_O, NULL},
    {"pyvalue_format", pyvalue_format, METH_VARARGS, NULL},
    {"pyvalue_format_inline", pyvalue_format_inline, METH_NOARGS, NULL},
    {"pyvalue_format_list_item", pyvalue_format_list_item, METH_O, NULL},
    {"pyvalue_err_format", pyvalue_err_format, METH_VARARGS, NULL},
    {"pyvalue_as_object_index", pyvalue_as_object_index, METH_NOARGS, NULL},
    {"pyvalue_as_known_object_inline", pyvalue_as_known_object_inline, METH_O, NULL},
    {"get_configs", get_configs, METH_NOARGS},
    {"gc_verify", gc_verify, METH_NOARGS},
    {"gc_verify_holes", gc_verify_holes, METH_NOARGS},
    {"gc_verify_count", gc_verify_count, METH_NOARGS},
    {"gc_verify_with_c_local", gc_verify_with_c_local, METH_O},
    {"gc_verify_with_pymem_array", gc_verify_with_pymem_array, METH_O},
    {"call_with_c_local", call_with_c_local, METH_VARARGS},
    {"get_eval_frame_stats", get_eval_frame_stats, METH_NOARGS, NULL},
    {"get_recursion_depth", get_recursion_depth, METH_NOARGS},
    {"get_c_recursion_remaining", get_c_recursion_remaining, METH_NOARGS},
    {"get_stack_pointer", get_stack_pointer, METH_NOARGS},
    {"get_stack_margin", get_stack_margin, METH_NOARGS},
    {"test_stop_the_world", test_stop_the_world, METH_NOARGS},
    {"classify_stack_addresses", classify_stack_addresses, METH_VARARGS},
    {"gnu_backtrace_unwind", gnu_backtrace_unwind, METH_NOARGS},
    {"manual_frame_pointer_unwind", manual_frame_pointer_unwind, METH_NOARGS},
    {"test_bswap", test_bswap, METH_NOARGS},
    {"test_popcount", test_popcount, METH_NOARGS},
    {"test_bit_length", test_bit_length, METH_NOARGS},
    {"test_hashtable", test_hashtable, METH_NOARGS},
    {"reset_path_config", test_reset_path_config, METH_NOARGS},
    {"test_edit_cost", test_edit_cost, METH_NOARGS},
    {"test_bytes_find", test_bytes_find, METH_NOARGS},
    {"normalize_path", normalize_path, METH_O, NULL},
    {"get_getpath_codeobject", get_getpath_codeobject, METH_NOARGS, NULL},
    {"encode_locale", encode_locale, METH_VARARGS},
    {"decode_locale", decode_locale, METH_VARARGS},
    {"set_eval_frame_default", set_eval_frame_default, METH_NOARGS, NULL},
    {"set_eval_frame_interp", set_eval_frame_interp, METH_VARARGS, NULL},
    {"set_eval_frame_record", set_eval_frame_record, METH_O, NULL},
    {"run_staged_provider", run_staged_provider, METH_VARARGS, NULL},
    {"is_specialization_enabled", is_specialization_enabled, METH_NOARGS, NULL},
    _TESTINTERNALCAPI_COMPILER_CLEANDOC_METHODDEF
    _TESTINTERNALCAPI_NEW_INSTRUCTION_SEQUENCE_METHODDEF
    _TESTINTERNALCAPI_COMPILER_CODEGEN_METHODDEF
    _TESTINTERNALCAPI_OPTIMIZE_CFG_METHODDEF
    _TESTINTERNALCAPI_ASSEMBLE_CODE_OBJECT_METHODDEF
    {"get_interp_settings", get_interp_settings, METH_VARARGS, NULL},
    {"clear_extension", clear_extension, METH_VARARGS, NULL},
    {"write_perf_map_entry", write_perf_map_entry, METH_VARARGS},
    {"perf_map_state_teardown", perf_map_state_teardown, METH_NOARGS},
    {"iframe_getcode", iframe_getcode, METH_O, NULL},
    {"iframe_getline", iframe_getline, METH_O, NULL},
    {"iframe_getlasti", iframe_getlasti, METH_O, NULL},
    {"code_returns_only_none", code_returns_only_none, METH_O, NULL},
    {"get_co_framesize", get_co_framesize, METH_O, NULL},
    {"get_co_localskinds", get_co_localskinds, METH_O, NULL},
    {"get_feedback", get_feedback, METH_VARARGS, NULL},
    {"get_ir", get_ir, METH_VARARGS, NULL},
    {"ir_normal_physical_transfer", ir_normal_physical_transfer,
     METH_VARARGS, NULL},
    {"set_ir_max_forward", set_ir_max_forward, METH_O, NULL},
    {"set_ir_max_slots", set_ir_max_slots, METH_O, NULL},
    {"get_code_var_counts", _PyCFunction_CAST(get_code_var_counts),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"verify_stateless_code", _PyCFunction_CAST(verify_stateless_code),
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"pending_threadfunc", _PyCFunction_CAST(pending_threadfunc),
     METH_VARARGS | METH_KEYWORDS},
    {"pending_identify", pending_identify, METH_VARARGS, NULL},
    {"_PyTraceMalloc_GetTraceback", tracemalloc_get_traceback, METH_VARARGS},
    {"test_tstate_capi", test_tstate_capi, METH_NOARGS, NULL},
    {"_PyUnicode_TransformDecimalAndSpaceToASCII", unicode_transformdecimalandspacetoascii, METH_O},
    {"check_pyobject_forbidden_bytes_is_freed",
                            check_pyobject_forbidden_bytes_is_freed, METH_NOARGS},
    {"check_pyobject_freed_is_freed", check_pyobject_freed_is_freed, METH_NOARGS},
    {"check_pyobject_null_is_freed",  check_pyobject_null_is_freed,  METH_NOARGS},
    {"check_pyobject_uninitialized_is_freed",
                              check_pyobject_uninitialized_is_freed, METH_NOARGS},
    {"pymem_getallocatorsname", test_pymem_getallocatorsname, METH_NOARGS},
    {"get_object_dict_values", get_object_dict_values, METH_O},
#ifdef Py_SHAPES
    {"get_object_shape", get_object_shape, METH_O},
    {"get_type_shape_count", get_type_shape_count, METH_O},
#endif
    {"hamt", new_hamt, METH_NOARGS},
    {"dict_getitem_knownhash",  dict_getitem_knownhash,          METH_VARARGS},
    {"create_interpreter", _PyCFunction_CAST(create_interpreter),
     METH_VARARGS | METH_KEYWORDS},
    {"destroy_interpreter", _PyCFunction_CAST(destroy_interpreter),
     METH_VARARGS | METH_KEYWORDS},
    {"exec_interpreter", _PyCFunction_CAST(exec_interpreter),
     METH_VARARGS | METH_KEYWORDS},
    {"run_in_subinterp_with_config",
     _PyCFunction_CAST(run_in_subinterp_with_config),
     METH_VARARGS | METH_KEYWORDS},
    {"normalize_interp_id", normalize_interp_id, METH_O},
    {"next_interpreter_id", next_interpreter_id, METH_NOARGS},
    {"unused_interpreter_id", unused_interpreter_id, METH_NOARGS},
    {"interpreter_exists", interpreter_exists, METH_O},
    {"get_interpreter_refcount", get_interpreter_refcount, METH_O},
    {"link_interpreter_refcount", link_interpreter_refcount,     METH_O},
    {"unlink_interpreter_refcount", unlink_interpreter_refcount, METH_O},
    {"interpreter_refcount_linked", interpreter_refcount_linked, METH_O},
    {"compile_perf_trampoline_entry", compile_perf_trampoline_entry, METH_VARARGS},
    {"perf_trampoline_set_persist_after_fork", perf_trampoline_set_persist_after_fork, METH_VARARGS},
    {"get_crossinterp_data",    _PyCFunction_CAST(get_crossinterp_data),
     METH_VARARGS | METH_KEYWORDS},
    {"restore_crossinterp_data", restore_crossinterp_data,       METH_VARARGS},
    _TESTINTERNALCAPI_TEST_LONG_NUMBITS_METHODDEF
    {"get_rare_event_counters", get_rare_event_counters, METH_NOARGS},
    {"reset_rare_event_counters", reset_rare_event_counters, METH_NOARGS},
    {"has_inline_values", has_inline_values, METH_O},
    {"has_split_table", has_split_table, METH_O},
    {"type_assign_specific_version_unsafe", type_assign_specific_version_unsafe, METH_VARARGS,
     PyDoc_STR("forcefully assign type->tp_version_tag")},

#ifdef Py_GIL_DISABLED
    {"py_thread_id", get_py_thread_id, METH_NOARGS},
    {"get_tlbc", get_tlbc, METH_O, NULL},
    {"get_tlbc_id", get_tlbc_id, METH_O, NULL},
    {"get_long_lived_total", get_long_lived_total, METH_NOARGS},
#endif
    GH_119213_GETARGS_METHODDEF
    {"get_static_builtin_types", get_static_builtin_types, METH_NOARGS},
    {"identify_type_slot_wrappers", identify_type_slot_wrappers, METH_NOARGS},
    {"has_deferred_refcount", has_deferred_refcount, METH_O},
    {"get_tracked_heap_size", get_tracked_heap_size, METH_NOARGS},
    {"is_static_immortal", is_static_immortal, METH_O},
    {"incref_decref_delayed", incref_decref_delayed, METH_O},
    {"value_box_roundtrip", value_box_roundtrip, METH_O},
    {"inline_locals", inline_locals, METH_O},
    {"tagged_mortal_slots", tagged_mortal_slots, METH_O},
    GET_NEXT_DICT_KEYS_VERSION_METHODDEF
#ifdef __EMSCRIPTEN__
    {"emscripten_set_up_async_input_device", emscripten_set_up_async_input_device, METH_NOARGS},
#endif
    {"simple_pending_call", simple_pending_call, METH_O},
    {"set_vectorcall_nop", set_vectorcall_nop, METH_O},
    {"module_get_gc_hooks", module_get_gc_hooks, METH_O},
    {"test_threadstate_set_stack_protection",
     test_threadstate_set_stack_protection, METH_NOARGS},
    {"_pyerr_setkeyerror", _pyerr_setkeyerror, METH_O},
    {"test_interp_guard_countdown", test_interp_guard_countdown, METH_NOARGS},
    {"test_interp_view_countdown", test_interp_view_countdown, METH_NOARGS},
    {"test_thread_state_ensure_from_view_interp_switch", test_thread_state_ensure_from_view_interp_switch, METH_NOARGS},
    {"unicodewriter_overflow", unicodewriter_overflow, METH_NOARGS},
    {NULL, NULL} /* sentinel */
};


/* initialization function */

static int
module_exec(PyObject *module)
{
    if (_PyTestInternalCapi_Init_Lock(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_PyTime(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_Set(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_Slots(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_Complex(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_CriticalSection(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_Tokenizer(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_Tuple(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_List(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_TypeCache(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_NativeScope(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_Dict(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_BorrowScope(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_ValueSlots(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_ValueCall(module) < 0) {
        return 1;
    }

    Py_ssize_t sizeof_gc_head = 0;
#ifndef Py_GIL_DISABLED
    sizeof_gc_head = sizeof(PyGC_Head);
#endif

    if (PyModule_Add(module, "SIZEOF_PYGC_HEAD",
                        PyLong_FromSsize_t(sizeof_gc_head)) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SIZEOF_MANAGED_PRE_HEADER",
                        PyLong_FromSsize_t(2 * sizeof(PyObject*))) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SIZEOF_PYOBJECT",
                        PyLong_FromSsize_t(sizeof(PyObject))) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SIZEOF_TIME_T",
                        PyLong_FromSsize_t(sizeof(time_t))) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SPECIALIZATION_THRESHOLD",
                        PyLong_FromLong(ADAPTIVE_WARMUP_VALUE + 1)) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SPECIALIZATION_COOLDOWN",
                        PyLong_FromLong(ADAPTIVE_COOLDOWN_VALUE + 1)) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SHARED_KEYS_MAX_SIZE",
                        PyLong_FromLong(SHARED_KEYS_MAX_SIZE)) < 0) {
        return 1;
    }

    if (PyModule_AddIntMacro(module, _PY_NSMALLPOSINTS) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "TAGGED_VALUES", PyBool_FromLong(1)) < 0) {
        return 1;
    }

#ifdef Py_SHAPES
    if (PyModule_AddIntMacro(module, _Py_SHAPE_MAX_PER_TYPE) < 0) {
        return 1;
    }
#endif

#ifdef _Py_WITH_FRAME_POINTERS
    if (PyModule_AddIntMacro(module, _Py_WITH_FRAME_POINTERS) < 0) {
        return 1;
    }
#endif

    if (PyType_Ready(&SelfInterruptingContextManager_Type) < 0) {
        return 1;
    }
    PyModule_AddObject(module, "SelfInterruptingContextManager", (PyObject *)&SelfInterruptingContextManager_Type);

    if (PyModule_AddIntMacro(module, _Py_MAX_UNICODE) < 0) {
        return 1;
    }

    return 0;
}

PyABIInfo_VAR(abi_info);

static struct PyModuleDef_Slot module_slots[] = {
    {Py_mod_abi, &abi_info},
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static int
module_traverse(PyObject *module, visitproc visit, void *arg)
{
    module_state *state = get_module_state(module);
    assert(state != NULL);
    traverse_module_state(state, visit, arg);
    return 0;
}

static int
module_clear(PyObject *module)
{
    module_state *state = get_module_state(module);
    assert(state != NULL);
    (void)clear_module_state(state);
    return 0;
}

static void
module_free(void *module)
{
    module_state *state = get_module_state(module);
    assert(state != NULL);
    (void)clear_module_state(state);
}

static struct PyModuleDef _testcapimodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME,
    .m_doc = NULL,
    .m_size = sizeof(module_state),
    .m_methods = module_functions,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = module_free,
};


PyMODINIT_FUNC
PyInit__testinternalcapi(void)
{
    return PyModuleDef_Init(&_testcapimodule);
}
