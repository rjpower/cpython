/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsLong()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder___init____doc__,
"RemoteUnwinder(pid, *, all_threads=False, only_active_thread=False,\n"
"               mode=0, debug=False, skip_non_matching_threads=True,\n"
"               native=False, gc=False, opcodes=False,\n"
"               cache_frames=False, stats=False)\n"
"--\n"
"\n"
"Initialize a new RemoteUnwinder object for debugging a remote Python process.\n"
"\n"
"Args:\n"
"    pid: Process ID of the target Python process to debug\n"
"    all_threads: If True, initialize state for all threads in the\n"
"        process.  If False, only initialize for the main thread.\n"
"    only_active_thread: If True, only sample the thread holding the GIL.\n"
"    mode: Profiling mode: 0=WALL (wall-time), 1=CPU (cpu-time), 2=GIL\n"
"        (gil-time).  Cannot be used together with all_threads=True.\n"
"    debug: If True, chain exceptions to explain the sequence of events\n"
"        that lead to the exception.\n"
"    skip_non_matching_threads: If True, skip threads that don\'t match\n"
"        the selected mode.  If False, include all threads regardless of\n"
"        mode.\n"
"    native: If True, include artificial \"<native>\" frames to denote\n"
"        calls to non-Python code.\n"
"    gc: If True, include artificial \"<GC>\" frames to denote active\n"
"        garbage collection.\n"
"    opcodes: If True, gather bytecode opcode information for\n"
"        instruction-level profiling.\n"
"    cache_frames: If True, enable frame caching optimization to avoid\n"
"        re-reading unchanged parent frames between samples.\n"
"    stats: If True, collect statistics about cache hits, memory reads,\n"
"        etc.  Use get_stats() to retrieve the collected statistics.\n"
"\n"
"The RemoteUnwinder provides functionality to inspect and debug a running\n"
"Python process, including examining thread states, stack frames and\n"
"other runtime data.\n"
"\n"
"Raises:\n"
"    PermissionError: If access to the target process is denied\n"
"    OSError: If unable to attach to the target process or access its\n"
"        memory\n"
"    RuntimeError: If unable to read debug information from the target\n"
"        process\n"
"    ValueError: If both all_threads and only_active_thread are True");

static int
_remote_debugging_RemoteUnwinder___init___impl(RemoteUnwinderObject *self,
                                               pid_t pid, int all_threads,
                                               int only_active_thread,
                                               int mode, int debug,
                                               int skip_non_matching_threads,
                                               int native, int gc,
                                               int opcodes, int cache_frames,
                                               int stats);

static int
_remote_debugging_RemoteUnwinder___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 11
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(pid)), _PyValue_STATIC_IMMORTAL(&_Py_ID(all_threads)), _PyValue_STATIC_IMMORTAL(&_Py_ID(only_active_thread)), _PyValue_STATIC_IMMORTAL(&_Py_ID(mode)), _PyValue_STATIC_IMMORTAL(&_Py_ID(debug)), _PyValue_STATIC_IMMORTAL(&_Py_ID(skip_non_matching_threads)), _PyValue_STATIC_IMMORTAL(&_Py_ID(native)), _PyValue_STATIC_IMMORTAL(&_Py_ID(gc)), _PyValue_STATIC_IMMORTAL(&_Py_ID(opcodes)), _PyValue_STATIC_IMMORTAL(&_Py_ID(cache_frames)), _PyValue_STATIC_IMMORTAL(&_Py_ID(stats)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "all_threads", "only_active_thread", "mode", "debug", "skip_non_matching_threads", "native", "gc", "opcodes", "cache_frames", "stats", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "RemoteUnwinder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[11];
    pid_t pid;
    int all_threads = 0;
    int only_active_thread = 0;
    int mode = 0;
    int debug = 0;
    int skip_non_matching_threads = 1;
    int native = 0;
    int gc = 0;
    int opcodes = 0;
    int cache_frames = 0;
    int stats = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 11)) {
        goto exit;
    }
    #if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
    pid = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    #elif SIZEOF_PID_T == SIZEOF_LONG
    pid = _PyValue_AsLong(PyValue_Borrow(args[_idx[0]]));
    #else
    pid = _PyValue_AsLongLong(PyValue_Borrow(args[_idx[0]]));
    #endif
    if (pid == (pid_t)(-1) && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        all_threads = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (all_threads < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        only_active_thread = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (only_active_thread < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        mode = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        debug = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (debug < 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        skip_non_matching_threads = _PyValue_IsTrue(PyValue_Borrow(args[_idx[5]]));
        if (skip_non_matching_threads < 0) {
            goto exit;
        }
    }
    if (_idx[6] >= 0) {
        native = _PyValue_IsTrue(PyValue_Borrow(args[_idx[6]]));
        if (native < 0) {
            goto exit;
        }
    }
    if (_idx[7] >= 0) {
        gc = _PyValue_IsTrue(PyValue_Borrow(args[_idx[7]]));
        if (gc < 0) {
            goto exit;
        }
    }
    if (_idx[8] >= 0) {
        opcodes = _PyValue_IsTrue(PyValue_Borrow(args[_idx[8]]));
        if (opcodes < 0) {
            goto exit;
        }
    }
    if (_idx[9] >= 0) {
        cache_frames = _PyValue_IsTrue(PyValue_Borrow(args[_idx[9]]));
        if (cache_frames < 0) {
            goto exit;
        }
    }
    if (_idx[10] >= 0) {
        stats = _PyValue_IsTrue(PyValue_Borrow(args[_idx[10]]));
        if (stats < 0) {
            goto exit;
        }
    }
    return_value = _remote_debugging_RemoteUnwinder___init___impl((RemoteUnwinderObject *)self, pid, all_threads, only_active_thread, mode, debug, skip_non_matching_threads, native, gc, opcodes, cache_frames, stats);

exit:
    return return_value;
}

static int
_remote_debugging_RemoteUnwinder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _remote_debugging_RemoteUnwinder___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _remote_debugging_RemoteUnwinder___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_stack_trace__doc__,
"get_stack_trace($self, /)\n"
"--\n"
"\n"
"Returns stack traces for all interpreters and threads in process.\n"
"\n"
"Each element in the returned list is a tuple of (interpreter_id,\n"
"thread_list), where:\n"
"- interpreter_id is the interpreter identifier\n"
"- thread_list is a list of tuples (thread_id, frame_list) for\n"
"  threads in that interpreter\n"
"  - thread_id is the OS thread identifier\n"
"  - frame_list is a list of tuples (function_name, filename,\n"
"    line_number) representing the Python stack frames for that\n"
"    thread, ordered from most recent to oldest\n"
"\n"
"The threads returned depend on the initialization parameters:\n"
"- If only_active_thread was True: returns only the thread holding\n"
"  the GIL across all interpreters\n"
"- If all_threads was True: returns all threads across all\n"
"  interpreters\n"
"- Otherwise: returns only the main thread of each interpreter\n"
"\n"
"Example:\n"
"    [\n"
"        (0, [  # Main interpreter\n"
"            (1234, [\n"
"                (\'process_data\', \'worker.py\', 127),\n"
"                (\'run_worker\', \'worker.py\', 45),\n"
"                (\'main\', \'app.py\', 23)\n"
"            ]),\n"
"            (1235, [\n"
"                (\'handle_request\', \'server.py\', 89),\n"
"                (\'serve_forever\', \'server.py\', 52)\n"
"            ])\n"
"        ]),\n"
"        (1, [  # Sub-interpreter\n"
"            (1236, [\n"
"                (\'sub_worker\', \'sub.py\', 15)\n"
"            ])\n"
"        ])\n"
"    ]\n"
"\n"
"Raises:\n"
"    RuntimeError: If there is an error copying memory from the\n"
"        target process\n"
"    OSError: If there is an error accessing the target process\n"
"    PermissionError: If access to the target process is denied\n"
"    UnicodeDecodeError: If there is an error decoding strings from\n"
"        the target process");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_STACK_TRACE_METHODDEF    \
    {"get_stack_trace", _PyCFunction_CAST(_remote_debugging_RemoteUnwinder_get_stack_trace), METH_VALUE|METH_NOARGS, _remote_debugging_RemoteUnwinder_get_stack_trace__doc__},

static PyValue
_remote_debugging_RemoteUnwinder_get_stack_trace_impl(RemoteUnwinderObject *self);

static PyValue
_remote_debugging_RemoteUnwinder_get_stack_trace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _remote_debugging_RemoteUnwinder_get_stack_trace_impl((RemoteUnwinderObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_all_awaited_by__doc__,
"get_all_awaited_by($self, /)\n"
"--\n"
"\n"
"Get awaited_by relationships for tasks in the remote process.\n"
"\n"
"Returns:\n"
"    A list of AwaitedInfo objects, where each object contains:\n"
"\n"
"    - thread_id (int): Identifier of the thread, or 0 for tasks in the\n"
"      interpreter\'s fallback task list.\n"
"    - awaited_by (list[TaskInfo]): Tasks registered with this thread.\n"
"\n"
"Each TaskInfo contains:\n"
"    - task_id (int): Identifier of the task.\n"
"    - task_name (str): Name of the task.\n"
"    - coroutine_stack (list[CoroInfo]): Stack of coroutine frames.\n"
"    - awaited_by (list[CoroInfo]): Coroutine information for tasks or futures\n"
"      awaiting this task.\n"
"\n"
"Each CoroInfo contains:\n"
"    - call_stack (list[FrameInfo]): Call stack frames for the coroutine.\n"
"    - task_name (int): Identifier of the task or future.\n"
"\n"
"Raises:\n"
"    RuntimeError: If AsyncioDebug section is not available in the\n"
"        remote process\n"
"    MemoryError: If memory allocation fails\n"
"    OSError: If reading from the remote process fails\n"
"\n"
"Example output:\n"
"\n"
"    [\n"
"        AwaitedInfo(\n"
"            thread_id=12345,\n"
"            awaited_by=[\n"
"                TaskInfo(\n"
"                    task_id=1,\n"
"                    task_name=\"Task-1\",\n"
"                    coroutine_stack=[...],\n"
"                    awaited_by=[]\n"
"                )\n"
"            ]\n"
"        )\n"
"    ]");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_ALL_AWAITED_BY_METHODDEF    \
    {"get_all_awaited_by", _PyCFunction_CAST(_remote_debugging_RemoteUnwinder_get_all_awaited_by), METH_VALUE|METH_NOARGS, _remote_debugging_RemoteUnwinder_get_all_awaited_by__doc__},

static PyValue
_remote_debugging_RemoteUnwinder_get_all_awaited_by_impl(RemoteUnwinderObject *self);

static PyValue
_remote_debugging_RemoteUnwinder_get_all_awaited_by(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _remote_debugging_RemoteUnwinder_get_all_awaited_by_impl((RemoteUnwinderObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_async_stack_trace__doc__,
"get_async_stack_trace($self, /)\n"
"--\n"
"\n"
"Get the currently running async tasks and their dependency graphs from the remote process.\n"
"\n"
"This returns information about running tasks and all tasks that are\n"
"waiting for them, forming a complete dependency graph for each\n"
"thread\'s active task.\n"
"\n"
"For each thread with a running task, returns the running task plus\n"
"all tasks that transitively depend on it (tasks waiting for the\n"
"running task, tasks waiting for those tasks, etc.).\n"
"\n"
"Returns a list of per-thread results, where each thread result\n"
"contains:\n"
"- Thread ID\n"
"- List of task information for the running task and all its waiters\n"
"\n"
"Each task info contains:\n"
"- Task ID (memory address)\n"
"- Task name\n"
"- Call stack frames: List of (func_name, filename, lineno)\n"
"- List of tasks waiting for this task (recursive structure)\n"
"\n"
"Raises:\n"
"    RuntimeError: If AsyncioDebug section is not available in the\n"
"        target process\n"
"    MemoryError: If memory allocation fails\n"
"    OSError: If reading from the remote process fails\n"
"\n"
"Example output (similar structure to get_all_awaited_by but only for\n"
"running tasks):\n"
"[\n"
"    (140234, [\n"
"        (4345585712, \'main_task\',\n"
"         [(\"run_server\", \"server.py\", 127), (\"main\", \"app.py\", 23)],\n"
"         [\n"
"             (4345585800, \'worker_1\', [...], [...]),\n"
"             (4345585900, \'worker_2\', [...], [...])\n"
"         ])\n"
"    ])\n"
"]");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_ASYNC_STACK_TRACE_METHODDEF    \
    {"get_async_stack_trace", _PyCFunction_CAST(_remote_debugging_RemoteUnwinder_get_async_stack_trace), METH_VALUE|METH_NOARGS, _remote_debugging_RemoteUnwinder_get_async_stack_trace__doc__},

static PyValue
_remote_debugging_RemoteUnwinder_get_async_stack_trace_impl(RemoteUnwinderObject *self);

static PyValue
_remote_debugging_RemoteUnwinder_get_async_stack_trace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _remote_debugging_RemoteUnwinder_get_async_stack_trace_impl((RemoteUnwinderObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_stats__doc__,
"get_stats($self, /)\n"
"--\n"
"\n"
"Get collected statistics about profiling performance.\n"
"\n"
"Returns a dictionary containing statistics about cache performance,\n"
"memory reads, and other profiling metrics. Only available if the\n"
"RemoteUnwinder was created with stats=True.\n"
"\n"
"Returns:\n"
"    dict: A dictionary containing:\n"
"        - total_samples: Total number of get_stack_trace calls\n"
"        - frame_cache_hits: Full cache hits (entire stack unchanged)\n"
"        - frame_cache_misses: Cache misses requiring full walk\n"
"        - frame_cache_partial_hits: Partial hits (stopped at cached\n"
"          frame)\n"
"        - frames_read_from_cache: Total frames retrieved from cache\n"
"        - frames_read_from_memory: Total frames read from remote\n"
"          memory\n"
"        - memory_reads: Total remote memory read operations\n"
"        - memory_bytes_read: Total bytes read from remote memory\n"
"        - code_object_cache_hits: Code object cache hits\n"
"        - code_object_cache_misses: Code object cache misses\n"
"        - stale_cache_invalidations: Times stale cache entries were\n"
"          cleared\n"
"        - batched_read_attempts: Batched remote-read attempts\n"
"        - batched_read_successes: Attempts that read all requested\n"
"          segments\n"
"        - batched_read_misses: Attempts that fell back or partially\n"
"          read\n"
"        - batched_read_segments_requested: Segments requested by\n"
"          batched reads\n"
"        - batched_read_segments_completed: Segments completed by\n"
"          batched reads\n"
"        - frame_cache_hit_rate: Percentage of samples that hit the\n"
"          cache\n"
"        - code_object_cache_hit_rate: Percentage of code object\n"
"          lookups that hit cache\n"
"        - batched_read_success_rate: Percentage of batched reads\n"
"          that completed all segments\n"
"        - batched_read_segment_completion_rate: Percentage of\n"
"          requested segments read by batched reads\n"
"\n"
"Raises:\n"
"    RuntimeError: If stats collection was not enabled (stats=False)");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_STATS_METHODDEF    \
    {"get_stats", _PyCFunction_CAST(_remote_debugging_RemoteUnwinder_get_stats), METH_VALUE|METH_NOARGS, _remote_debugging_RemoteUnwinder_get_stats__doc__},

static PyValue
_remote_debugging_RemoteUnwinder_get_stats_impl(RemoteUnwinderObject *self);

static PyValue
_remote_debugging_RemoteUnwinder_get_stats(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _remote_debugging_RemoteUnwinder_get_stats_impl((RemoteUnwinderObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_pause_threads__doc__,
"pause_threads($self, /)\n"
"--\n"
"\n"
"Pause all threads in the target process.\n"
"\n"
"This stops all threads in the target process to allow for consistent\n"
"memory reads during sampling. Must be paired with a call to\n"
"resume_threads().\n"
"\n"
"Returns True if threads were successfully paused, False if they were\n"
"already paused.\n"
"\n"
"Raises:\n"
"    RuntimeError: If there is an error stopping the threads");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_PAUSE_THREADS_METHODDEF    \
    {"pause_threads", _PyCFunction_CAST(_remote_debugging_RemoteUnwinder_pause_threads), METH_VALUE|METH_NOARGS, _remote_debugging_RemoteUnwinder_pause_threads__doc__},

static PyValue
_remote_debugging_RemoteUnwinder_pause_threads_impl(RemoteUnwinderObject *self);

static PyValue
_remote_debugging_RemoteUnwinder_pause_threads(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _remote_debugging_RemoteUnwinder_pause_threads_impl((RemoteUnwinderObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_resume_threads__doc__,
"resume_threads($self, /)\n"
"--\n"
"\n"
"Resume all threads in the target process.\n"
"\n"
"This resumes threads that were previously paused with\n"
"pause_threads().\n"
"\n"
"Returns True if threads were successfully resumed, False if they\n"
"were not paused.");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_RESUME_THREADS_METHODDEF    \
    {"resume_threads", _PyCFunction_CAST(_remote_debugging_RemoteUnwinder_resume_threads), METH_VALUE|METH_NOARGS, _remote_debugging_RemoteUnwinder_resume_threads__doc__},

static PyValue
_remote_debugging_RemoteUnwinder_resume_threads_impl(RemoteUnwinderObject *self);

static PyValue
_remote_debugging_RemoteUnwinder_resume_threads(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _remote_debugging_RemoteUnwinder_resume_threads_impl((RemoteUnwinderObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_GCMonitor___init____doc__,
"GCMonitor(pid, *, debug=False)\n"
"--\n"
"\n"
"Initialize a new GCMonitor object for monitoring GC events from remote process.\n"
"\n"
"Args:\n"
"    pid: Process ID of the target Python process to monitor\n"
"    debug: If True, chain exceptions to explain the sequence of\n"
"           events that lead to the exception.\n"
"\n"
"The GCMonitor provides functionality to read GC statistics from\n"
"a running Python process.\n"
"\n"
"Raises:\n"
"    PermissionError: If access to the target process is denied\n"
"    OSError: If unable to attach to the target process or access\n"
"        its memory\n"
"    RuntimeError: If unable to read debug information from the\n"
"        target process");

static int
_remote_debugging_GCMonitor___init___impl(GCMonitorObject *self, pid_t pid,
                                          int debug);

static int
_remote_debugging_GCMonitor___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(pid)), _PyValue_STATIC_IMMORTAL(&_Py_ID(debug)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "debug", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "GCMonitor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    pid_t pid;
    int debug = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    #if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
    pid = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    #elif SIZEOF_PID_T == SIZEOF_LONG
    pid = _PyValue_AsLong(PyValue_Borrow(args[_idx[0]]));
    #else
    pid = _PyValue_AsLongLong(PyValue_Borrow(args[_idx[0]]));
    #endif
    if (pid == (pid_t)(-1) && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        debug = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (debug < 0) {
            goto exit;
        }
    }
    return_value = _remote_debugging_GCMonitor___init___impl((GCMonitorObject *)self, pid, debug);

exit:
    return return_value;
}

static int
_remote_debugging_GCMonitor___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _remote_debugging_GCMonitor___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _remote_debugging_GCMonitor___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_remote_debugging_GCMonitor_get_gc_stats__doc__,
"get_gc_stats($self, /, all_interpreters=False)\n"
"--\n"
"\n"
"Get garbage collector statistics from external Python process.\n"
"\n"
"  all_interpreters\n"
"    If True, return GC statistics from all interpreters.\n"
"    If False, return only from main interpreter.\n"
"\n"
"Returns a list of GCStatsInfo objects with GC statistics data.\n"
"\n"
"Returns:\n"
"    list of GCStatsInfo: A list of stats samples containing:\n"
"        - gen: GC generation number.\n"
"        - iid: Interpreter ID.\n"
"        - ts_start: Raw timestamp at collection start.\n"
"        - ts_stop: Raw timestamp at collection stop.\n"
"        - collections: Total number of collections.\n"
"        - collected: Total number of collected objects.\n"
"        - uncollectable: Total number of uncollectable objects.\n"
"        - candidates: Total objects considered and traversed.\n"
"        - heap_size: number of live objects.\n"
"        - duration: Total collection time, in seconds.\n"
"\n"
"Raises:\n"
"    RuntimeError: If the target process cannot be inspected or if\n"
"        its debug offsets or GC stats layout are incompatible.");

#define _REMOTE_DEBUGGING_GCMONITOR_GET_GC_STATS_METHODDEF    \
    {"get_gc_stats", _PyCFunction_CAST(_remote_debugging_GCMonitor_get_gc_stats), METH_VALUE|METH_KEYWORDS, _remote_debugging_GCMonitor_get_gc_stats__doc__},

static PyValue
_remote_debugging_GCMonitor_get_gc_stats_impl(GCMonitorObject *self,
                                              int all_interpreters);

static PyValue
_remote_debugging_GCMonitor_get_gc_stats(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(all_interpreters)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"all_interpreters", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_gc_stats",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int all_interpreters = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        all_interpreters = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (all_interpreters < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _remote_debugging_GCMonitor_get_gc_stats_impl((GCMonitorObject *)PyValue_AsKnownObjectBorrow(self), all_interpreters);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryWriter___init____doc__,
"BinaryWriter(filename, sample_interval_us, start_time_us, *,\n"
"             compression=0, mode=-1, capture_features=-1)\n"
"--\n"
"\n"
"High-performance binary writer for profiling data.\n"
"\n"
"Arguments:\n"
"    filename: Path to output file\n"
"    sample_interval_us: Sampling interval in microseconds\n"
"    start_time_us: Start timestamp in microseconds (from\n"
"        time.monotonic() * 1e6)\n"
"    compression: 0=none, 1=zstd (default: 0)\n"
"    mode: Profiling mode, or -1 if unknown (default: -1)\n"
"    capture_features: Capture feature bit mask, or -1 if unknown\n"
"        (default: -1)\n"
"\n"
"Use as a context manager or call finalize() when done.");

static int
_remote_debugging_BinaryWriter___init___impl(BinaryWriterObject *self,
                                             PyValueBorrow filename,
                                             unsigned long long sample_interval_us,
                                             unsigned long long start_time_us,
                                             int compression, int mode,
                                             int capture_features);

static int
_remote_debugging_BinaryWriter___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(filename)), _PyValue_STATIC_IMMORTAL(&_Py_ID(sample_interval_us)), _PyValue_STATIC_IMMORTAL(&_Py_ID(start_time_us)), _PyValue_STATIC_IMMORTAL(&_Py_ID(compression)), _PyValue_STATIC_IMMORTAL(&_Py_ID(mode)), _PyValue_STATIC_IMMORTAL(&_Py_ID(capture_features)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"filename", "sample_interval_us", "start_time_us", "compression", "mode", "capture_features", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BinaryWriter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyValueBorrow filename;
    unsigned long long sample_interval_us;
    unsigned long long start_time_us;
    int compression = 0;
    int mode = -1;
    int capture_features = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    filename = PyValue_Borrow(args[_idx[0]]);
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[1]]), &sample_interval_us, sizeof(unsigned long long),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long long)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned long long");
            goto exit;
        }
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[2]]), &start_time_us, sizeof(unsigned long long),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long long)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned long long");
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        compression = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (compression == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        mode = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        capture_features = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (capture_features == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _remote_debugging_BinaryWriter___init___impl((BinaryWriterObject *)self, filename, sample_interval_us, start_time_us, compression, mode, capture_features);

exit:
    return return_value;
}

static int
_remote_debugging_BinaryWriter___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _remote_debugging_BinaryWriter___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _remote_debugging_BinaryWriter___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_remote_debugging_BinaryWriter_write_sample__doc__,
"write_sample($self, /, stack_frames, timestamp_us)\n"
"--\n"
"\n"
"Write a sample to the binary file.\n"
"\n"
"Arguments:\n"
"    stack_frames: List of InterpreterInfo objects\n"
"    timestamp_us: Current timestamp in microseconds (from\n"
"        time.monotonic() * 1e6)");

#define _REMOTE_DEBUGGING_BINARYWRITER_WRITE_SAMPLE_METHODDEF    \
    {"write_sample", _PyCFunction_CAST(_remote_debugging_BinaryWriter_write_sample), METH_VALUE|METH_KEYWORDS, _remote_debugging_BinaryWriter_write_sample__doc__},

static PyValue
_remote_debugging_BinaryWriter_write_sample_impl(BinaryWriterObject *self,
                                                 PyValueBorrow stack_frames,
                                                 unsigned long long timestamp_us);

static PyValue
_remote_debugging_BinaryWriter_write_sample(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(stack_frames)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timestamp_us)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"stack_frames", "timestamp_us", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "write_sample",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow stack_frames;
    unsigned long long timestamp_us;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    stack_frames = PyValue_Borrow(args[_idx[0]]);
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[1]]), &timestamp_us, sizeof(unsigned long long),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long long)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned long long");
            goto exit;
        }
    }
    return_value = _remote_debugging_BinaryWriter_write_sample_impl((BinaryWriterObject *)PyValue_AsKnownObjectBorrow(self), stack_frames, timestamp_us);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryWriter_set_stats__doc__,
"set_stats($self, /, duration_sec, sample_rate, error_rate=None,\n"
"          missed_samples=None)\n"
"--\n"
"\n"
"Store measured profile statistics in the binary file.");

#define _REMOTE_DEBUGGING_BINARYWRITER_SET_STATS_METHODDEF    \
    {"set_stats", _PyCFunction_CAST(_remote_debugging_BinaryWriter_set_stats), METH_VALUE|METH_KEYWORDS, _remote_debugging_BinaryWriter_set_stats__doc__},

static PyValue
_remote_debugging_BinaryWriter_set_stats_impl(BinaryWriterObject *self,
                                              double duration_sec,
                                              double sample_rate,
                                              PyValueBorrow error_rate,
                                              PyValueBorrow missed_samples);

static PyValue
_remote_debugging_BinaryWriter_set_stats(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(duration_sec)), _PyValue_STATIC_IMMORTAL(&_Py_ID(sample_rate)), _PyValue_STATIC_IMMORTAL(&_Py_ID(error_rate)), _PyValue_STATIC_IMMORTAL(&_Py_ID(missed_samples)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"duration_sec", "sample_rate", "error_rate", "missed_samples", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_stats",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    double duration_sec;
    double sample_rate;
    PyValueBorrow error_rate = PyValue_Borrow(PyValue_None);
    PyValueBorrow missed_samples = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    duration_sec = _PyValue_AsDouble(PyValue_Borrow(args[_idx[0]]));
    if (duration_sec == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    sample_rate = _PyValue_AsDouble(PyValue_Borrow(args[_idx[1]]));
    if (sample_rate == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[2] >= 0) {
        error_rate = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        missed_samples = PyValue_Borrow(args[_idx[3]]);
    }
    return_value = _remote_debugging_BinaryWriter_set_stats_impl((BinaryWriterObject *)PyValue_AsKnownObjectBorrow(self), duration_sec, sample_rate, error_rate, missed_samples);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryWriter_finalize__doc__,
"finalize($self, /)\n"
"--\n"
"\n"
"Finalize and close the binary file.\n"
"\n"
"Writes string/frame tables, footer, and updates header.");

#define _REMOTE_DEBUGGING_BINARYWRITER_FINALIZE_METHODDEF    \
    {"finalize", _PyCFunction_CAST(_remote_debugging_BinaryWriter_finalize), METH_VALUE|METH_NOARGS, _remote_debugging_BinaryWriter_finalize__doc__},

static PyValue
_remote_debugging_BinaryWriter_finalize_impl(BinaryWriterObject *self);

static PyValue
_remote_debugging_BinaryWriter_finalize(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_BinaryWriter_finalize_impl((BinaryWriterObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryWriter_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the writer without finalizing (discards data).");

#define _REMOTE_DEBUGGING_BINARYWRITER_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_remote_debugging_BinaryWriter_close), METH_VALUE|METH_NOARGS, _remote_debugging_BinaryWriter_close__doc__},

static PyValue
_remote_debugging_BinaryWriter_close_impl(BinaryWriterObject *self);

static PyValue
_remote_debugging_BinaryWriter_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_BinaryWriter_close_impl((BinaryWriterObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryWriter___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Enter context manager.");

#define _REMOTE_DEBUGGING_BINARYWRITER___ENTER___METHODDEF    \
    {"__enter__", _PyCFunction_CAST(_remote_debugging_BinaryWriter___enter__), METH_VALUE|METH_NOARGS, _remote_debugging_BinaryWriter___enter____doc__},

static PyValue
_remote_debugging_BinaryWriter___enter___impl(BinaryWriterObject *self);

static PyValue
_remote_debugging_BinaryWriter___enter__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_BinaryWriter___enter___impl((BinaryWriterObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryWriter___exit____doc__,
"__exit__($self, /, exc_type=None, exc_val=None, exc_tb=None)\n"
"--\n"
"\n"
"Exit context manager, finalizing the file.");

#define _REMOTE_DEBUGGING_BINARYWRITER___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_remote_debugging_BinaryWriter___exit__), METH_VALUE|METH_KEYWORDS, _remote_debugging_BinaryWriter___exit____doc__},

static PyValue
_remote_debugging_BinaryWriter___exit___impl(BinaryWriterObject *self,
                                             PyValueBorrow exc_type,
                                             PyValueBorrow exc_val,
                                             PyValueBorrow exc_tb);

static PyValue
_remote_debugging_BinaryWriter___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(exc_type)), _PyValue_STATIC_IMMORTAL(&_Py_ID(exc_val)), _PyValue_STATIC_IMMORTAL(&_Py_ID(exc_tb)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"exc_type", "exc_val", "exc_tb", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__exit__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow exc_type = PyValue_Borrow(PyValue_None);
    PyValueBorrow exc_val = PyValue_Borrow(PyValue_None);
    PyValueBorrow exc_tb = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        exc_type = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        exc_val = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        exc_tb = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _remote_debugging_BinaryWriter___exit___impl((BinaryWriterObject *)PyValue_AsKnownObjectBorrow(self), exc_type, exc_val, exc_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryWriter_get_stats__doc__,
"get_stats($self, /)\n"
"--\n"
"\n"
"Get encoding statistics for the writer.\n"
"\n"
"Returns a dict with encoding statistics including\n"
"repeat/full/suffix/pop-push record counts, frames written/saved, and\n"
"compression ratio.");

#define _REMOTE_DEBUGGING_BINARYWRITER_GET_STATS_METHODDEF    \
    {"get_stats", _PyCFunction_CAST(_remote_debugging_BinaryWriter_get_stats), METH_VALUE|METH_NOARGS, _remote_debugging_BinaryWriter_get_stats__doc__},

static PyValue
_remote_debugging_BinaryWriter_get_stats_impl(BinaryWriterObject *self);

static PyValue
_remote_debugging_BinaryWriter_get_stats(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_BinaryWriter_get_stats_impl((BinaryWriterObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryReader___init____doc__,
"BinaryReader(filename)\n"
"--\n"
"\n"
"High-performance binary reader for profiling data.\n"
"\n"
"Arguments:\n"
"    filename: Path to input file\n"
"\n"
"Use as a context manager or call close() when done.");

static int
_remote_debugging_BinaryReader___init___impl(BinaryReaderObject *self,
                                             PyValueBorrow filename);

static int
_remote_debugging_BinaryReader___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(filename)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"filename", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BinaryReader",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow filename;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    filename = PyValue_Borrow(args[_idx[0]]);
    return_value = _remote_debugging_BinaryReader___init___impl((BinaryReaderObject *)self, filename);

exit:
    return return_value;
}

static int
_remote_debugging_BinaryReader___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _remote_debugging_BinaryReader___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _remote_debugging_BinaryReader___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_remote_debugging_BinaryReader_replay__doc__,
"replay($self, /, collector, progress_callback=None)\n"
"--\n"
"\n"
"Replay samples through a collector.\n"
"\n"
"Arguments:\n"
"    collector: Collector object with collect() method\n"
"    progress_callback: Optional callable(current, total)\n"
"\n"
"Returns:\n"
"    Number of samples replayed");

#define _REMOTE_DEBUGGING_BINARYREADER_REPLAY_METHODDEF    \
    {"replay", _PyCFunction_CAST(_remote_debugging_BinaryReader_replay), METH_VALUE|METH_KEYWORDS, _remote_debugging_BinaryReader_replay__doc__},

static PyValue
_remote_debugging_BinaryReader_replay_impl(BinaryReaderObject *self,
                                           PyValueBorrow collector,
                                           PyValueBorrow progress_callback);

static PyValue
_remote_debugging_BinaryReader_replay(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(collector)), _PyValue_STATIC_IMMORTAL(&_Py_ID(progress_callback)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"collector", "progress_callback", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replay",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow collector;
    PyValueBorrow progress_callback = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    collector = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        progress_callback = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _remote_debugging_BinaryReader_replay_impl((BinaryReaderObject *)PyValue_AsKnownObjectBorrow(self), collector, progress_callback);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryReader_get_info__doc__,
"get_info($self, /)\n"
"--\n"
"\n"
"Get metadata about the binary file.\n"
"\n"
"Returns:\n"
"    Dict with file metadata");

#define _REMOTE_DEBUGGING_BINARYREADER_GET_INFO_METHODDEF    \
    {"get_info", _PyCFunction_CAST(_remote_debugging_BinaryReader_get_info), METH_VALUE|METH_NOARGS, _remote_debugging_BinaryReader_get_info__doc__},

static PyValue
_remote_debugging_BinaryReader_get_info_impl(BinaryReaderObject *self);

static PyValue
_remote_debugging_BinaryReader_get_info(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_BinaryReader_get_info_impl((BinaryReaderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryReader_get_stats__doc__,
"get_stats($self, /)\n"
"--\n"
"\n"
"Get reconstruction statistics from replay.\n"
"\n"
"Returns a dict with statistics about record types decoded and\n"
"samples reconstructed during replay.");

#define _REMOTE_DEBUGGING_BINARYREADER_GET_STATS_METHODDEF    \
    {"get_stats", _PyCFunction_CAST(_remote_debugging_BinaryReader_get_stats), METH_VALUE|METH_NOARGS, _remote_debugging_BinaryReader_get_stats__doc__},

static PyValue
_remote_debugging_BinaryReader_get_stats_impl(BinaryReaderObject *self);

static PyValue
_remote_debugging_BinaryReader_get_stats(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_BinaryReader_get_stats_impl((BinaryReaderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryReader_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the reader and free resources.");

#define _REMOTE_DEBUGGING_BINARYREADER_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_remote_debugging_BinaryReader_close), METH_VALUE|METH_NOARGS, _remote_debugging_BinaryReader_close__doc__},

static PyValue
_remote_debugging_BinaryReader_close_impl(BinaryReaderObject *self);

static PyValue
_remote_debugging_BinaryReader_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_BinaryReader_close_impl((BinaryReaderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryReader___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Enter context manager.");

#define _REMOTE_DEBUGGING_BINARYREADER___ENTER___METHODDEF    \
    {"__enter__", _PyCFunction_CAST(_remote_debugging_BinaryReader___enter__), METH_VALUE|METH_NOARGS, _remote_debugging_BinaryReader___enter____doc__},

static PyValue
_remote_debugging_BinaryReader___enter___impl(BinaryReaderObject *self);

static PyValue
_remote_debugging_BinaryReader___enter__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_BinaryReader___enter___impl((BinaryReaderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_BinaryReader___exit____doc__,
"__exit__($self, /, exc_type=None, exc_val=None, exc_tb=None)\n"
"--\n"
"\n"
"Exit context manager, closing the file.");

#define _REMOTE_DEBUGGING_BINARYREADER___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_remote_debugging_BinaryReader___exit__), METH_VALUE|METH_KEYWORDS, _remote_debugging_BinaryReader___exit____doc__},

static PyValue
_remote_debugging_BinaryReader___exit___impl(BinaryReaderObject *self,
                                             PyValueBorrow exc_type,
                                             PyValueBorrow exc_val,
                                             PyValueBorrow exc_tb);

static PyValue
_remote_debugging_BinaryReader___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(exc_type)), _PyValue_STATIC_IMMORTAL(&_Py_ID(exc_val)), _PyValue_STATIC_IMMORTAL(&_Py_ID(exc_tb)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"exc_type", "exc_val", "exc_tb", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__exit__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow exc_type = PyValue_Borrow(PyValue_None);
    PyValueBorrow exc_val = PyValue_Borrow(PyValue_None);
    PyValueBorrow exc_tb = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        exc_type = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        exc_val = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        exc_tb = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _remote_debugging_BinaryReader___exit___impl((BinaryReaderObject *)PyValue_AsKnownObjectBorrow(self), exc_type, exc_val, exc_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_zstd_available__doc__,
"zstd_available($module, /)\n"
"--\n"
"\n"
"Check if zstd compression is available.\n"
"\n"
"Returns:\n"
"    True if zstd available, False otherwise");

#define _REMOTE_DEBUGGING_ZSTD_AVAILABLE_METHODDEF    \
    {"zstd_available", _PyCFunction_CAST(_remote_debugging_zstd_available), METH_VALUE|METH_NOARGS, _remote_debugging_zstd_available__doc__},

static PyValue
_remote_debugging_zstd_available_impl(PyObject *module);

static PyValue
_remote_debugging_zstd_available(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _remote_debugging_zstd_available_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_get_child_pids__doc__,
"get_child_pids($module, /, pid, *, recursive=True)\n"
"--\n"
"\n"
"Get all child process IDs of the given process.\n"
"\n"
"  pid\n"
"    Process ID of the parent process\n"
"  recursive\n"
"    If True, return all descendants (children, grandchildren, etc.).\n"
"    If False, return only direct children.\n"
"\n"
"Returns a list of child process IDs.  Returns an empty list if no\n"
"children are found.\n"
"\n"
"This function provides a snapshot of child processes at a moment in\n"
"time.  Child processes may exit or new ones may be created after the\n"
"list is returned.\n"
"\n"
"Raises:\n"
"    OSError: If unable to enumerate processes\n"
"    NotImplementedError: If not supported on this platform");

#define _REMOTE_DEBUGGING_GET_CHILD_PIDS_METHODDEF    \
    {"get_child_pids", _PyCFunction_CAST(_remote_debugging_get_child_pids), METH_VALUE|METH_KEYWORDS, _remote_debugging_get_child_pids__doc__},

static PyValue
_remote_debugging_get_child_pids_impl(PyObject *module, pid_t pid,
                                      int recursive);

static PyValue
_remote_debugging_get_child_pids(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(pid)), _PyValue_STATIC_IMMORTAL(&_Py_ID(recursive)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "recursive", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_child_pids",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    pid_t pid;
    int recursive = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    #if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
    pid = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    #elif SIZEOF_PID_T == SIZEOF_LONG
    pid = _PyValue_AsLong(PyValue_Borrow(args[_idx[0]]));
    #else
    pid = _PyValue_AsLongLong(PyValue_Borrow(args[_idx[0]]));
    #endif
    if (pid == (pid_t)(-1) && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        recursive = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (recursive < 0) {
            goto exit;
        }
    }
    return_value = _remote_debugging_get_child_pids_impl(PyValue_AsKnownObjectBorrow(module), pid, recursive);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_is_python_process__doc__,
"is_python_process($module, /, pid)\n"
"--\n"
"\n"
"Check if a process is a Python process.");

#define _REMOTE_DEBUGGING_IS_PYTHON_PROCESS_METHODDEF    \
    {"is_python_process", _PyCFunction_CAST(_remote_debugging_is_python_process), METH_VALUE|METH_KEYWORDS, _remote_debugging_is_python_process__doc__},

static PyValue
_remote_debugging_is_python_process_impl(PyObject *module, pid_t pid);

static PyValue
_remote_debugging_is_python_process(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(pid)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_python_process",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    pid_t pid;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    #if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
    pid = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    #elif SIZEOF_PID_T == SIZEOF_LONG
    pid = _PyValue_AsLong(PyValue_Borrow(args[_idx[0]]));
    #else
    pid = _PyValue_AsLongLong(PyValue_Borrow(args[_idx[0]]));
    #endif
    if (pid == (pid_t)(-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _remote_debugging_is_python_process_impl(PyValue_AsKnownObjectBorrow(module), pid);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_get_gc_stats__doc__,
"get_gc_stats($module, /, pid, *, all_interpreters=False)\n"
"--\n"
"\n"
"Get garbage collector statistics from external Python process.\n"
"\n"
"  all_interpreters\n"
"    If True, return GC statistics from all interpreters.\n"
"    If False, return only from main interpreter.\n"
"\n"
"Returns:\n"
"    list of GCStatsInfo: A list of stats samples containing:\n"
"        - gen: GC generation number.\n"
"        - iid: Interpreter ID.\n"
"        - ts_start: Raw timestamp at collection start.\n"
"        - ts_stop: Raw timestamp at collection stop.\n"
"        - collections: Total number of collections.\n"
"        - collected: Total number of collected objects.\n"
"        - uncollectable: Total number of uncollectable objects.\n"
"        - candidates: Total objects considered and traversed.\n"
"        - duration: Total collection time, in seconds.\n"
"\n"
"Raises:\n"
"    RuntimeError: If the target process cannot be inspected or if its\n"
"        debug offsets or GC stats layout are incompatible.");

#define _REMOTE_DEBUGGING_GET_GC_STATS_METHODDEF    \
    {"get_gc_stats", _PyCFunction_CAST(_remote_debugging_get_gc_stats), METH_VALUE|METH_KEYWORDS, _remote_debugging_get_gc_stats__doc__},

static PyValue
_remote_debugging_get_gc_stats_impl(PyObject *module, pid_t pid,
                                    int all_interpreters);

static PyValue
_remote_debugging_get_gc_stats(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(pid)), _PyValue_STATIC_IMMORTAL(&_Py_ID(all_interpreters)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "all_interpreters", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_gc_stats",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    pid_t pid;
    int all_interpreters = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    #if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
    pid = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    #elif SIZEOF_PID_T == SIZEOF_LONG
    pid = _PyValue_AsLong(PyValue_Borrow(args[_idx[0]]));
    #else
    pid = _PyValue_AsLongLong(PyValue_Borrow(args[_idx[0]]));
    #endif
    if (pid == (pid_t)(-1) && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        all_interpreters = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (all_interpreters < 0) {
            goto exit;
        }
    }
    return_value = _remote_debugging_get_gc_stats_impl(PyValue_AsKnownObjectBorrow(module), pid, all_interpreters);

exit:
    return return_value;
}
/*[clinic end generated code: output=16caed5e703e4e02 input=a9049054013a1b77]*/
