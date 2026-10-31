"""What calls do to the collector: one table for the analyzer, root_check and
root_check_c (10a-1b.md item 2).

A call is of one of three kinds:

- "none": it neither allocates an object nor runs Python code, so it
  cannot start a collection;
- "alloc": it may allocate an object, and so collect, but it runs no Python
  code.  A collection runs no finalizer or callback inline (they are queued
  for the eval breaker), so across the call no edge of an existing object
  changes except those the call itself writes;
- "reentrant": anything else.  It may run Python code, which may change
  anything.

The analyzer's "escaping", which decides where a handler syncs its stack,
is the reentrant kind.  A collection can start at an "alloc" or a
"reentrant" call: those are the collecting calls.
"""

import re


NON_COLLECTING = frozenset({
    # Reference counting is gone (step 5c): these are no-ops or stores.
    "Py_INCREF", "Py_DECREF", "Py_XINCREF", "Py_XDECREF", "Py_CLEAR",
    "Py_SETREF", "Py_XSETREF", "Py_NewRef", "Py_XNewRef", "_Py_NewRef",
    "Py_REFCNT", "_Py_IsImmortal", "_Py_TryIncrefCompare",
    "_PyObject_IsUniquelyReferenced",
    # header, type and tracking predicates, and the shape
    "Py_TYPE", "Py_SIZE", "Py_IS_TYPE", "Py_SET_TYPE", "Py_SET_SIZE",
    "Py_Is", "Py_IsNone", "Py_IsTrue", "Py_IsFalse",
    "PyType_HasFeature", "_PyType_HasFeature", "PyType_IsSubtype",
    "PyObject_TypeCheck", "PyType_GetFlags", "PyExceptionInstance_Class",
    "_PyObject_SHAPE", "_PyObject_SetShape", "_PyObject_GC_IS_SHARED",
    "_PyObject_GC_IS_TRACKED", "_PyObject_GC_MAY_BE_TRACKED",
    "_PyObject_GC_TRACK", "PyObject_GC_UnTrack", "_Py_IsOwnedByCurrentThread",
    # values: tests, encodings and the immediates
    "PyValue_IsNull", "PyValue_IsObject", "PyValue_IsInline", "PyValue_Is",
    "PyValue_ObjectBorrow", "PyValue_FromObject", "PyValue_Type",
    "PyValue_IsInt", "PyValue_IsFloat", "PyValue_Bits",
    "PyValue_IsInlineInt", "PyValue_IsInlineFloat", "PyValue_FromInt32",
    "PyValue_Int32", "PyValue_Double",
    "_PyValue_None", "_PyValue_True", "_PyValue_False", "_PyValue_IsNone",
    "_PyValue_IsTrue", "_PyValue_IsFalse", "_PyValue_IsError",
    "_PyValue_IsValid", "_PyValue_Wrap", "_PyValue_Unwrap",
    "_PyValue_IsTaggedIndex", "_PyValue_TagIndex", "_PyValue_UntagIndex",
    "_PyValue_IncrementTaggedIndexNoOverflow", "_PyValue_IsPointerSpace",
    "_PyValue_IsPointerSpaceAt", "_PyValue_IsInlineAt", "_PyValue_FitsKind",
    "_PyValue_BothInlineInt", "_PyValue_FromBits", "_PyValue_DoubleBits",
    "_PyValue_IsCompactInt", "_PyValue_CompactInt", "_PyValue_FloatCheckExact",
    "_PyValue_FloatOperandsCheck", "_PyValue_NumberAsDouble",
    "_PyValue_FloatOperandValues", "_PyValue_UnrootObject",
    # an empty asm that hides a pointer's origin from the optimizer
    "_PyValue_HIDE_ORIGIN",
    # A new object's header (10a-1b.md item 3).  The reference tracer runs
    # with collection deferred (_PyReftracerTrack(), pycore_object.h); the
    # object map's chunks are mapped with mmap(); the root shape's fields
    # are plain stores; the debug heap check reads mimalloc's metadata.
    "_PyReftracerTrack", "_Py_NewReference", "_Py_NewReferenceNoTotal",
    "_PyGC_ObjectMapSet", "_PyGC_ObjectMapChunk", "_PyShape_InitRoot",
    "_PyObject_CheckObjectHeap",
    # a compiler intrinsic, and inline assembly that reads the stack pointer
    "__builtin_return_address", "_PyGC_StackAddress",
    # the verifier's trap in Py_TYPE() (debug builds): it reports and aborts
    "_PyGC_TrapUse",
    # arithmetic on the indices it is given
    "PySlice_AdjustIndices",
    # reads of a type's slot and of an object's layout: a data descriptor
    # test, and the address of a dict pointer (its assertions abort)
    "PyDescr_IsData", "_PyObject_ComputedDictPointer",
    # the dict's assertion (Objects/dictobject.c), which only reads the
    # dict.  Not true under DEBUG_PYDICT, where it hashes the keys and may
    # run Python code: a DEBUG_PYDICT build is outside the precise-roots
    # contract (note 11's platform matrix)
    "ASSERT_CONSISTENT",
    # an unpacked call's size, and its fill, which only stores into the
    # caller's span (Include/internal/pycore_call.h, 10a-2b.md section 1)
    "_PyStack_UnpackDictSlots", "_PyStack_UnpackDict",
    # pointer comparisons with the interpreter's import functions
    # (Python/import.c)
    "_PyImport_IsDefaultImportFunc", "_PyImport_IsDefaultLazyImportFunc",
    # stdio cannot reach the collector, and abort() does not return
    "fprintf", "fflush", "snprintf", "printf", "abort",
    # the value of an exact int that is not compact: PyLong_AsLongAndOverflow()
    # on an exact int calls no __index__ and allocates nothing
    # (Python/pyvalue.c)
    "_PyValue_FromLongObject",
    # ints
    "_PyLong_CompactValue", "_PyLong_DigitCount", "_PyLong_IsCompact",
    "_PyLong_IsNegative", "_PyLong_IsNonNegativeCompact", "_PyLong_IsZero",
    "_PyLong_BothAreCompact", "_PyLong_CheckExactAndCompact",
    # Container and cell readers and writers that do not box.  The object-
    # returning item and cell readers box an inline value (note 11 question
    # 12) and are allocating.  _PyList_AppendValue() resizes with
    # PyMem_Realloc(), which does not collect.
    "_PyDict_NextValue", "PyTuple_GET_VALUE", "PyList_GET_VALUE",
    "PyCell_GET_VALUE", "_PyCell_GetValue", "PyCell_SET_VALUE",
    "_PyCell_SwapValue", "PyTuple_SET_ITEM", "PyList_SET_ITEM",
    "PyTuple_SET_VALUE", "PyList_SET_VALUE", "_PyTuple_ITEMS",
    "_PyList_ITEMS", "_PyList_AppendValue", "_PyObject_DictAt",
    "_PyStolenTuple_Free", "_PyCode_CODE",
    "PyUnicode_READ_CHAR", "PyUnicode_IS_COMPACT_ASCII",
    "PyUnicode_1BYTE_DATA", "Py_Unicode_GET_LENGTH", "_PyUnicode_Equal",
    # functions, methods and builtins
    "PyFunction_GET_CODE", "PyFunction_GET_GLOBALS",
    "PyFunction_GET_BUILTINS", "PyMethod_GET_FUNCTION", "PyMethod_GET_SELF",
    "PyCFunction_GET_FUNCTION", "PyCFunction_GET_SELF",
    "PyCFunction_GET_FLAGS", "_PyFunction_SetVersion",
    # exceptions that only read or clear
    "PyException_GetCause", "PyException_GetContext",
    "PyException_GetTraceback", "_PyErr_ExceptionMatches",
    "PyErr_Occurred", "_PyErr_Occurred", "PyErr_Clear", "_PyErr_Clear",
    # casts
    "_PyObject_CAST", "_PyType_CAST", "_PyVarObject_CAST", "_Py_CAST",
    "_PyCFunction_CAST", "_PyFunction_CAST", "_PyTuple_CAST", "_PyList_CAST",
    # statics
    "_Py_ID", "_Py_STR", "_Py_LATIN1_CHR", "_PyLong_GetZero",
    "_PyLong_GetOne", "_Py_SINGLETON",
    # the thread state and the interpreter
    "_PyThreadState_GET", "PyThreadState_Get", "_PyInterpreterState_GET",
    "PyInterpreterState_Get", "_Py_EnsureTstateNotNULL",
    "_Py_set_eval_breaker_bit", "_Py_unset_eval_breaker_bit",
    "_Py_CHECK_EMSCRIPTEN_SIGNALS_PERIODICALLY",
    # frames: their fields, and the bookkeeping of pushing and popping them
    "_PyFrame_GetCode", "_PyFrame_GetFunction", "_PyFrame_GetGlobals",
    "_PyFrame_GetBuiltins", "_PyFrame_GetLocalsArray", "_PyFrame_GetBytecode",
    "_PyFrame_IsIncomplete", "_PyFrame_PushUnchecked",
    "_PyFrame_PushOverlapping", "_PyFrame_SetReturnValue", "_PyFrame_Slots",
    "_PyFrame_AssertTemporariesClear", "_PyFrame_Copy",
    "_PyThreadState_PopFrame", "_PyThreadState_UpdateLastProfiledFrame",
    "_PyThreadState_HasStackSpace", "_PyEval_GeneratorReturnTarget",
    "_PyGen_GetGeneratorFromFrame", "gen_try_set_executing",
    "_Py_LeaveRecursiveCallPy", "_Py_LeaveRecursiveCallTstate",
    "_Py_ReachedRecursionLimit",
    # Python/ir.c: decodes immutable code words/FBINIT through scalar field
    # extraction and the static format table; no allocation or callback.
    "_PyCode_InstrOparg",
    # feedback
    "_PyFb_Entry", "_PyFb_Header", "_PyFb_HasEntry", "_PyFrame_FbPoly",
    "_PyFb_ShapeRow", "advance_backoff_counter", "backoff_counter_triggers",
    "restart_backoff_counter",
    # assertions, control and libc
    "assert", "Py_UNREACHABLE", "Py_FatalError", "Py_UNUSED",
    "Py_ARRAY_LENGTH", "Py_MIN", "Py_MAX", "Py_ABS", "JUMP_TO_LABEL",
    "memcpy", "memmove", "memset", "memcmp", "strlen", "strcmp", "strncmp",
    "strchr", "strrchr", "isnan", "isfinite", "pow",
    # reads the stack pointer, with an intrinsic or an asm
    "_Py_get_machine_stack_pointer",
    "Py_BEGIN_CRITICAL_SECTION", "Py_END_CRITICAL_SECTION",
    "Py_BEGIN_CRITICAL_SECTION2", "Py_END_CRITICAL_SECTION2",
    # value roots and spans
    "_PyThreadState_PushValueRoot", "_PyThreadState_PopValueRoot",
    "_PyThreadState_PushValueSpan", "_PyValueSpan_Append",
    "_PyValueSpan_AppendObject", "_PyValueSpan_Rebase",
    # The mem domain counts its bytes for the trigger but never collects
    # (Objects/obmalloc.c, gc_count_alloc()): its callers do not expect a
    # collection.
    "PyMem_Malloc", "PyMem_Calloc", "PyMem_Realloc", "PyMem_Free",
    "PyMem_New", "PyMem_Resize", "PyMem_Del",
    "PyMem_RawMalloc", "PyMem_RawCalloc", "PyMem_RawRealloc", "PyMem_RawFree",
    # variable arguments: reads of the va_list, which holds no object of
    # its own
    "va_start", "va_copy", "va_arg", "va_end",
    # _Py_hashtable (Python/hashtable.c), by function: these call no
    # callback.  _Py_hashtable_new() allocates with the mem domain's
    # PyMem_Malloc() and stores, without calling, the hash and compare
    # functions it is given.  The rest of the family dispatches to the
    # table's configured callbacks (hash, compare, the key and value
    # destructors, the allocator) or to a caller's (foreach), so it is
    # unlisted and collects.
    "_Py_hashtable_hash_ptr", "_Py_hashtable_compare_direct",
    "_Py_hashtable_size", "_Py_hashtable_len", "_Py_hashtable_new",
    # statistics
    "CALL_STAT_INC", "EVAL_CALL_STAT_INC", "EVAL_CALL_STAT_INC_IF_FUNCTION",
    "OBJECT_STAT_INC", "VALUE_BOX_STAT_INC", "_Py_GatherStats_GetIter",
    # the cases generator's tests
    "doesnt_escape",
})

NON_COLLECTING_PATTERNS = (
    re.compile(r"^_?Py\w*_Check(Exact)?$"),
    re.compile(r"^_?Py\w*_CHECK\w*$"),
    re.compile(r"^_?Py\w*_GET_(SIZE|LENGTH)$"),
    re.compile(r"^_?Py\w*_AS_\w+$"),
    re.compile(r"^_?Py_atomic_\w+$"),
    re.compile(r"^FT_ATOMIC_\w+$"),
)

ALLOCATING = frozenset({
    # numbers, and boxing an immediate (PyBool_FromLong() returns a
    # singleton but is listed with its kind)
    "PyLong_FromLong", "PyLong_FromSsize_t", "PyLong_FromSize_t",
    "PyLong_FromLongLong", "PyLong_FromUnsignedLong",
    "PyLong_FromUnsignedLongLong", "PyLong_FromDouble", "PyFloat_FromDouble",
    "PyBool_FromLong", "PyValue_FromDouble", "_PyValue_BoxInt64",
    "_PyValue_LshiftInt32",
    "PyValue_Object", "_PyValue_BoxInline", "PyValue_ObjectNew",
    "_PyValueRoot_Object", "_PyValueRoot_PushObject", "_PyValue_ObjectRooted",
    # readers that box an inline value (note 11 question 12)
    "PyTuple_GET_ITEM", "PyList_GET_ITEM", "PyCell_GET", "PyCell_GetRef",
    "PyTuple_GetItem", "PyList_GetItem", "PySequence_Fast_GET_ITEM",
    # containers.  PySet_New() and PyFrozenSet_New() are not here: they
    # iterate their argument, which may run Python code.
    "PyTuple_New", "PyList_New", "PyDict_New", "PyCell_New", "PyMethod_New",
    "PySlice_New", "_PyTuple_FromValues", "_PyTuple_FromArray",
    "_PyTuple_FromArraySteal", "_PyList_FromArraySteal", "PyTuple_Pack",
    # strings
    "PyUnicode_FromString", "PyUnicode_FromStringAndSize",
    "PyBytes_FromStringAndSize", "PyUnicode_Concat", "_PyUnicode_JoinArray",
    # raw allocation
    "PyType_GenericAlloc", "_PyType_AllocNoTrack", "PyObject_Malloc",
    "PyObject_Calloc", "PyObject_Realloc", "PyObject_New",
    "PyObject_NewVar", "PyObject_GC_New", "PyObject_GC_NewVar",
    "_PyObject_GC_New", "_PyObject_GC_NewVar",
    # errors that allocate their exception
    "_Py_EnterRecursiveCallTstate", "_Py_EnterRecursiveCall",
    "_PyErr_NoMemory", "PyErr_NoMemory",
})

assert not NON_COLLECTING & ALLOCATING


def kind(name: str) -> str | None:
    """"none", "alloc", or None for a name the table does not list, whose
    kind is "reentrant" unless a body says otherwise."""
    if name in ALLOCATING:
        return "alloc"
    if name in NON_COLLECTING or any(p.match(name) for p in NON_COLLECTING_PATTERNS):
        return "none"
    return None
