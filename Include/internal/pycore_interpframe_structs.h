/* Structures used by pycore_debug_offsets.h.
 *
 * See InternalDocs/frames.md for an explanation of the frame stack
 * including explanation of the PyFrameObject and _PyInterpreterFrame
 * structs.
 */

#ifndef Py_INTERNAL_INTERP_FRAME_STRUCTS_H
#define Py_INTERNAL_INTERP_FRAME_STRUCTS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_structs.h"       // PyValue
#include "pycore_typedefs.h"      // _PyInterpreterFrame

#ifdef __cplusplus
extern "C" {
#endif

enum _frameowner {
    FRAME_OWNED_BY_THREAD = 0,
    FRAME_OWNED_BY_GENERATOR = 1,
    FRAME_OWNED_BY_FRAME_OBJECT = 2,
    FRAME_OWNED_BY_INTERPRETER = 3,
};

struct _PyInterpreterFrame {
    PyValue f_executable; /* Deferred or strong reference (code object or None) */
    struct _PyInterpreterFrame *previous;
    /* Slot 0: co_nlocalsplus locals, cells and free variables, then
     * co_stacksize temporaries.  Every slot holds a value or NULL; a
     * temporary that no instruction has written or that an instruction
     * consumed is NULL.  The slots of a frame on the frame stack are on the
     * thread's value stack; a generator and a frame object keep them in
     * owned_slots. */
    PyValue *base;
    /* For a frame on the frame stack, the top of the value stack before it
     * was pushed, which popping it restores */
    PyValue *saved_top;
    PyValue f_funcobj; /* Deferred or strong reference. Only valid if not on C stack */
    PyObject *f_globals; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_builtins; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_locals; /* Strong reference, may be NULL. Only valid if not on C stack */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL. Only valid if not on C stack */
    _PyInstr *instr_ptr; /* Instruction currently executing (or about to begin) */
    /* Word 0 of the executing image, immediately after its _PyCodeExec
     * header, which also supplies the feedback base. */
    _PyInstr *ir_words;
    /* Real executable continuation while this frame awaits a child result. */
    _PyInstr *return_pc;
#ifdef Py_GIL_DISABLED
    /* Index of thread-local bytecode containing instr_ptr. */
    int32_t tlbc_index;
#endif
    /* The slot that receives the value when control comes back to this
     * frame: the dst of the call it is making, or for a suspended
     * generator the slot of the value it yielded, where the value sent in
     * goes. */
    uint16_t return_slot;
    /* Virtual empty **kwargs local, or IR_NO_DST when no binding is pending. */
    uint16_t pending_kwargs;
    char owner;
    uint8_t visited:1;
    uint8_t external:1; /* Set when the frame was pushed by something other than the VM */
    uint8_t pending_redirect:1; /* Commit a debugger continuation after child retirement */
#ifdef Py_DEBUG
    uint8_t lltrace:5;
#endif
    /* The slots of a frame that owns them, which base then points to */
    PyValue owned_slots[1];
};


/* _PyGenObject_HEAD defines the initial segment of generator
   and coroutine objects. */
#define _PyGenObject_HEAD(prefix)                                           \
    PyObject_HEAD                                                           \
    /* List of weak reference. */                                           \
    PyObject *prefix##_weakreflist;                                         \
    /* Name of the generator. */                                            \
    PyObject *prefix##_name;                                                \
    /* Qualified name of the generator. */                                  \
    PyObject *prefix##_qualname;                                            \
    _PyErr_StackItem prefix##_exc_state;                                    \
    PyObject *prefix##_origin_or_finalizer;                                 \
    int8_t prefix##_hooks_inited;                                           \
    int8_t prefix##_closed;                                                 \
    int8_t prefix##_running_async;                                          \
    /* The frame */                                                         \
    int8_t prefix##_frame_state;                                            \
    _PyInterpreterFrame prefix##_iframe;                                    \

struct _PyGenObject {
    /* The gi_ prefix is intended to remind of generator-iterator. */
    _PyGenObject_HEAD(gi)
};

struct _PyCoroObject {
    _PyGenObject_HEAD(cr)
};

struct _PyAsyncGenObject {
    _PyGenObject_HEAD(ag)
};

#undef _PyGenObject_HEAD


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_INTERP_FRAME_STRUCTS_H
