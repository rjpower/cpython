#define _GNU_SOURCE
#include "pyconfig.h"

#include "Python.h"
#include "internal/pycore_runtime.h"
#include "internal/pycore_ceval.h"

#if defined(Py_REMOTE_DEBUG) && defined(Py_SUPPORTS_REMOTE_DEBUG)
#include "remote_debug.h"

static int
init_proc_handle(proc_handle_t *handle, pid_t pid) {
    return _Py_RemoteDebug_InitProcHandle(handle, pid);
}

static void
cleanup_proc_handle(proc_handle_t *handle) {
    _Py_RemoteDebug_CleanupProcHandle(handle);
}

static int
read_memory(proc_handle_t *handle, uintptr_t remote_address, size_t len, void* dst)
{
    return _Py_RemoteDebug_ReadRemoteMemory(handle, remote_address, len, dst);
}

// Use the shared write function from remote_debug.h
static int
write_memory(proc_handle_t *handle, uintptr_t remote_address, size_t len, const void* src)
{
    return _Py_RemoteDebug_WriteRemoteMemory(handle, remote_address, len, src);
}

static int
ensure_debug_offset_compatibility(const _Py_DebugOffsets *debug_offsets)
{
    return _PyDebugOffsets_Validate(debug_offsets);
}

static int
read_offsets(
    proc_handle_t *handle,
    uintptr_t *runtime_start_address,
    _Py_DebugOffsets* debug_offsets
) {
    if (_Py_RemoteDebug_ReadDebugOffsets(handle, runtime_start_address, debug_offsets)) {
        return -1;
    }
    if (ensure_debug_offset_compatibility(debug_offsets)) {
        return -1;
    }
    return 0;
}

static int
send_exec_to_proc_handle(proc_handle_t *handle, int tid, const char *debugger_script_path)
{
    uintptr_t runtime_start_address;
    struct _Py_DebugOffsets debug_offsets;

    if (read_offsets(handle, &runtime_start_address, &debug_offsets)) {
        return -1;
    }

    uintptr_t interpreter_state_list_head = (uintptr_t)debug_offsets.runtime_state.interpreters_head;

    uintptr_t interpreter_state_addr;
    if (0 != read_memory(
            handle,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &interpreter_state_addr))
    {
        return -1;
    }

    if (interpreter_state_addr == 0) {
        PyErr_SetString(PyExc_RuntimeError, "Can't find a running interpreter in the remote process");
        return -1;
    }

    int is_remote_debugging_enabled = 0;
    if (0 != read_memory(
            handle,
            interpreter_state_addr + (uintptr_t)debug_offsets.debugger_support.remote_debugging_enabled,
            sizeof(int),
            &is_remote_debugging_enabled))
    {
        return -1;
    }

    if (is_remote_debugging_enabled != 1) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Remote debugging is not enabled in the remote process");
        return -1;
    }

    uintptr_t thread_state_addr;
    unsigned long this_tid = 0;

    if (tid != 0) {
        if (0 != read_memory(
                handle,
                interpreter_state_addr + (uintptr_t)debug_offsets.interpreter_state.threads_head,
                sizeof(void*),
                &thread_state_addr))
        {
            return -1;
        }
        while (thread_state_addr != 0) {
            if (0 != read_memory(
                    handle,
                    thread_state_addr + (uintptr_t)debug_offsets.thread_state.native_thread_id,
                    sizeof(this_tid),
                    &this_tid))
            {
                return -1;
            }

            if (this_tid == (unsigned long)tid) {
                break;
            }

            if (0 != read_memory(
                    handle,
                    thread_state_addr + (uintptr_t)debug_offsets.thread_state.next,
                    sizeof(void*),
                    &thread_state_addr))
            {
                return -1;
            }
        }

        if (thread_state_addr == 0) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "Can't find the specified thread in the remote process");
            return -1;
        }
    } else {
        if (0 != read_memory(
                handle,
                interpreter_state_addr + (uintptr_t)debug_offsets.interpreter_state.threads_main,
                sizeof(void*),
                &thread_state_addr))
        {
            return -1;
        }

        if (thread_state_addr == 0) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "Can't find the main thread in the remote process");
            return -1;
        }
    }

    // Ensure our path is not too long
    if (debug_offsets.debugger_support.debugger_script_path_size <= strlen(debugger_script_path)) {
        PyErr_SetString(PyExc_ValueError, "Debugger script path is too long");
        return -1;
    }

    uintptr_t debugger_script_path_addr = (uintptr_t)(
        thread_state_addr +
        debug_offsets.debugger_support.remote_debugger_support +
        debug_offsets.debugger_support.debugger_script_path);
    if (0 != write_memory(
            handle,
            debugger_script_path_addr,
            strlen(debugger_script_path) + 1,
            debugger_script_path))
    {
        return -1;
    }

    int pending_call = 1;
    uintptr_t debugger_pending_call_addr = (uintptr_t)(
        thread_state_addr +
        debug_offsets.debugger_support.remote_debugger_support +
        debug_offsets.debugger_support.debugger_pending_call);
    if (0 != write_memory(
            handle,
            debugger_pending_call_addr,
            sizeof(int),
            &pending_call))

    {
        return -1;
    }

    uintptr_t eval_breaker;
    if (0 != read_memory(
            handle,
            thread_state_addr + (uintptr_t)debug_offsets.debugger_support.eval_breaker,
            sizeof(uintptr_t),
            &eval_breaker))
    {
        return -1;
    }

    eval_breaker |= _PY_EVAL_PLEASE_STOP_BIT;

    if (0 != write_memory(
            handle,
            thread_state_addr + (uintptr_t)debug_offsets.debugger_support.eval_breaker,
            sizeof(uintptr_t),
            &eval_breaker))

    {
        return -1;
    }

    return 0;
}

#endif // defined(Py_REMOTE_DEBUG) && defined(Py_SUPPORTS_REMOTE_DEBUG)

int
_PySysRemoteDebug_SendExec(int pid, int tid, const char *debugger_script_path)
{
#if !defined(Py_SUPPORTS_REMOTE_DEBUG)
    PyErr_SetString(PyExc_RuntimeError, "Remote debugging is not supported on this platform");
    return -1;
#elif !defined(Py_REMOTE_DEBUG)
    PyErr_SetString(PyExc_RuntimeError, "Remote debugging support has not been compiled in");
    return -1;
#else

    PyThreadState *tstate = _PyThreadState_GET();
    const PyConfig *config = _PyInterpreterState_GetConfig(tstate->interp);
    if (config->remote_debug != 1) {
        PyErr_SetString(PyExc_RuntimeError, "Remote debugging is not enabled");
        return -1;
    }

    proc_handle_t handle;
    if (init_proc_handle(&handle, pid) < 0) {
        return -1;
    }

    int rc = send_exec_to_proc_handle(&handle, tid, debugger_script_path);
    cleanup_proc_handle(&handle);
    return rc;
#endif
}

