/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_long.h"          // _PyLong_UnsignedLong_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

PyDoc_STRVAR(_overlapped_CreateIoCompletionPort__doc__,
"CreateIoCompletionPort($module, handle, port, key, concurrency, /)\n"
"--\n"
"\n"
"Create a completion port or register a handle with a port.");

#define _OVERLAPPED_CREATEIOCOMPLETIONPORT_METHODDEF    \
    {"CreateIoCompletionPort", _PyCFunction_CAST(_overlapped_CreateIoCompletionPort), METH_FASTCALL, _overlapped_CreateIoCompletionPort__doc__},

static PyObject *
_overlapped_CreateIoCompletionPort_impl(PyObject *module, HANDLE FileHandle,
                                        HANDLE ExistingCompletionPort,
                                        ULONG_PTR CompletionKey,
                                        DWORD NumberOfConcurrentThreads);

static PyValue
_overlapped_CreateIoCompletionPort(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE FileHandle;
    HANDLE ExistingCompletionPort;
    ULONG_PTR CompletionKey;
    DWORD NumberOfConcurrentThreads;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("CreateIoCompletionPort", nargs, 4, 4)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        FileHandle = PyLong_AsVoidPtr(_arg);
        if (!FileHandle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        ExistingCompletionPort = PyLong_AsVoidPtr(_arg);
        if (!ExistingCompletionPort && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        CompletionKey = (uintptr_t)PyLong_AsVoidPtr(_arg);
        if (!CompletionKey && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &NumberOfConcurrentThreads)) {
            goto exit;
        }
    }
    _return_value = _overlapped_CreateIoCompletionPort_impl(module, FileHandle, ExistingCompletionPort, CompletionKey, NumberOfConcurrentThreads);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_GetQueuedCompletionStatus__doc__,
"GetQueuedCompletionStatus($module, port, msecs, /)\n"
"--\n"
"\n"
"Get a message from completion port.\n"
"\n"
"Wait for up to msecs milliseconds.");

#define _OVERLAPPED_GETQUEUEDCOMPLETIONSTATUS_METHODDEF    \
    {"GetQueuedCompletionStatus", _PyCFunction_CAST(_overlapped_GetQueuedCompletionStatus), METH_FASTCALL, _overlapped_GetQueuedCompletionStatus__doc__},

static PyObject *
_overlapped_GetQueuedCompletionStatus_impl(PyObject *module,
                                           HANDLE CompletionPort,
                                           DWORD Milliseconds);

static PyValue
_overlapped_GetQueuedCompletionStatus(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE CompletionPort;
    DWORD Milliseconds;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("GetQueuedCompletionStatus", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        CompletionPort = PyLong_AsVoidPtr(_arg);
        if (!CompletionPort && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &Milliseconds)) {
            goto exit;
        }
    }
    _return_value = _overlapped_GetQueuedCompletionStatus_impl(module, CompletionPort, Milliseconds);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_PostQueuedCompletionStatus__doc__,
"PostQueuedCompletionStatus($module, port, bytes, key, address, /)\n"
"--\n"
"\n"
"Post a message to completion port.");

#define _OVERLAPPED_POSTQUEUEDCOMPLETIONSTATUS_METHODDEF    \
    {"PostQueuedCompletionStatus", _PyCFunction_CAST(_overlapped_PostQueuedCompletionStatus), METH_FASTCALL, _overlapped_PostQueuedCompletionStatus__doc__},

static PyObject *
_overlapped_PostQueuedCompletionStatus_impl(PyObject *module,
                                            HANDLE CompletionPort,
                                            DWORD NumberOfBytes,
                                            ULONG_PTR CompletionKey,
                                            OVERLAPPED *Overlapped);

static PyValue
_overlapped_PostQueuedCompletionStatus(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE CompletionPort;
    DWORD NumberOfBytes;
    ULONG_PTR CompletionKey;
    OVERLAPPED *Overlapped;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("PostQueuedCompletionStatus", nargs, 4, 4)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        CompletionPort = PyLong_AsVoidPtr(_arg);
        if (!CompletionPort && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &NumberOfBytes)) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        CompletionKey = (uintptr_t)PyLong_AsVoidPtr(_arg);
        if (!CompletionKey && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        Overlapped = PyLong_AsVoidPtr(_arg);
        if (!Overlapped && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _overlapped_PostQueuedCompletionStatus_impl(module, CompletionPort, NumberOfBytes, CompletionKey, Overlapped);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_RegisterWaitWithQueue__doc__,
"RegisterWaitWithQueue($module, Object, CompletionPort, Overlapped,\n"
"                      Timeout, /)\n"
"--\n"
"\n"
"Register wait for Object; when complete CompletionPort is notified.");

#define _OVERLAPPED_REGISTERWAITWITHQUEUE_METHODDEF    \
    {"RegisterWaitWithQueue", _PyCFunction_CAST(_overlapped_RegisterWaitWithQueue), METH_FASTCALL, _overlapped_RegisterWaitWithQueue__doc__},

static PyObject *
_overlapped_RegisterWaitWithQueue_impl(PyObject *module, HANDLE Object,
                                       HANDLE CompletionPort,
                                       OVERLAPPED *Overlapped,
                                       DWORD Milliseconds);

static PyValue
_overlapped_RegisterWaitWithQueue(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE Object;
    HANDLE CompletionPort;
    OVERLAPPED *Overlapped;
    DWORD Milliseconds;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("RegisterWaitWithQueue", nargs, 4, 4)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        Object = PyLong_AsVoidPtr(_arg);
        if (!Object && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        CompletionPort = PyLong_AsVoidPtr(_arg);
        if (!CompletionPort && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        Overlapped = PyLong_AsVoidPtr(_arg);
        if (!Overlapped && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &Milliseconds)) {
            goto exit;
        }
    }
    _return_value = _overlapped_RegisterWaitWithQueue_impl(module, Object, CompletionPort, Overlapped, Milliseconds);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_UnregisterWait__doc__,
"UnregisterWait($module, WaitHandle, /)\n"
"--\n"
"\n"
"Unregister wait handle.");

#define _OVERLAPPED_UNREGISTERWAIT_METHODDEF    \
    {"UnregisterWait", _PyCFunction_CAST(_overlapped_UnregisterWait), METH_O, _overlapped_UnregisterWait__doc__},

static PyObject *
_overlapped_UnregisterWait_impl(PyObject *module, HANDLE WaitHandle);

static PyValue
_overlapped_UnregisterWait(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    HANDLE WaitHandle;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        WaitHandle = PyLong_AsVoidPtr(_arg);
        if (!WaitHandle && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _overlapped_UnregisterWait_impl(module, WaitHandle);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_UnregisterWaitEx__doc__,
"UnregisterWaitEx($module, WaitHandle, Event, /)\n"
"--\n"
"\n"
"Unregister wait handle.");

#define _OVERLAPPED_UNREGISTERWAITEX_METHODDEF    \
    {"UnregisterWaitEx", _PyCFunction_CAST(_overlapped_UnregisterWaitEx), METH_FASTCALL, _overlapped_UnregisterWaitEx__doc__},

static PyObject *
_overlapped_UnregisterWaitEx_impl(PyObject *module, HANDLE WaitHandle,
                                  HANDLE Event);

static PyValue
_overlapped_UnregisterWaitEx(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE WaitHandle;
    HANDLE Event;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("UnregisterWaitEx", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        WaitHandle = PyLong_AsVoidPtr(_arg);
        if (!WaitHandle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        Event = PyLong_AsVoidPtr(_arg);
        if (!Event && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _overlapped_UnregisterWaitEx_impl(module, WaitHandle, Event);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_CreateEvent__doc__,
"CreateEvent($module, EventAttributes, ManualReset, InitialState, Name,\n"
"            /)\n"
"--\n"
"\n"
"Create an event.\n"
"\n"
"EventAttributes must be None.");

#define _OVERLAPPED_CREATEEVENT_METHODDEF    \
    {"CreateEvent", _PyCFunction_CAST(_overlapped_CreateEvent), METH_FASTCALL, _overlapped_CreateEvent__doc__},

static PyObject *
_overlapped_CreateEvent_impl(PyObject *module, PyObject *EventAttributes,
                             BOOL ManualReset, BOOL InitialState,
                             const wchar_t *Name);

static PyValue
_overlapped_CreateEvent(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *EventAttributes;
    BOOL ManualReset;
    BOOL InitialState;
    const wchar_t *Name = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("CreateEvent", nargs, 4, 4)) {
        goto exit;
    }
    EventAttributes = PyValue_Object(args[0]);
    if (EventAttributes == NULL) {
        goto exit;
    }
    ManualReset = PyLong_AsIntValue(args[1]);
    if (ManualReset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    InitialState = PyLong_AsIntValue(args[2]);
    if (InitialState == -1 && PyErr_Occurred()) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            Name = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            Name = PyUnicode_AsWideCharString(_arg, NULL);
            if (Name == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("CreateEvent", "argument 4", "str or None", _arg);
            goto exit;
        }
    }
    _return_value = _overlapped_CreateEvent_impl(module, EventAttributes, ManualReset, InitialState, Name);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for Name */
    PyMem_Free((void *)Name);

    return return_value;
}

PyDoc_STRVAR(_overlapped_SetEvent__doc__,
"SetEvent($module, Handle, /)\n"
"--\n"
"\n"
"Set event.");

#define _OVERLAPPED_SETEVENT_METHODDEF    \
    {"SetEvent", _PyCFunction_CAST(_overlapped_SetEvent), METH_O, _overlapped_SetEvent__doc__},

static PyObject *
_overlapped_SetEvent_impl(PyObject *module, HANDLE Handle);

static PyValue
_overlapped_SetEvent(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    HANDLE Handle;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        Handle = PyLong_AsVoidPtr(_arg);
        if (!Handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _overlapped_SetEvent_impl(module, Handle);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_ResetEvent__doc__,
"ResetEvent($module, Handle, /)\n"
"--\n"
"\n"
"Reset event.");

#define _OVERLAPPED_RESETEVENT_METHODDEF    \
    {"ResetEvent", _PyCFunction_CAST(_overlapped_ResetEvent), METH_O, _overlapped_ResetEvent__doc__},

static PyObject *
_overlapped_ResetEvent_impl(PyObject *module, HANDLE Handle);

static PyValue
_overlapped_ResetEvent(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    HANDLE Handle;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        Handle = PyLong_AsVoidPtr(_arg);
        if (!Handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _overlapped_ResetEvent_impl(module, Handle);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_BindLocal__doc__,
"BindLocal($module, handle, family, /)\n"
"--\n"
"\n"
"Bind a socket handle to an arbitrary local port.\n"
"\n"
"family should be AF_INET or AF_INET6.");

#define _OVERLAPPED_BINDLOCAL_METHODDEF    \
    {"BindLocal", _PyCFunction_CAST(_overlapped_BindLocal), METH_FASTCALL, _overlapped_BindLocal__doc__},

static PyObject *
_overlapped_BindLocal_impl(PyObject *module, HANDLE Socket, int Family);

static PyValue
_overlapped_BindLocal(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE Socket;
    int Family;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("BindLocal", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        Socket = PyLong_AsVoidPtr(_arg);
        if (!Socket && PyErr_Occurred()) {
            goto exit;
        }
    }
    Family = PyLong_AsIntValue(args[1]);
    if (Family == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _overlapped_BindLocal_impl(module, Socket, Family);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_FormatMessage__doc__,
"FormatMessage($module, error_code, /)\n"
"--\n"
"\n"
"Return error message for an error code.");

#define _OVERLAPPED_FORMATMESSAGE_METHODDEF    \
    {"FormatMessage", _PyCFunction_CAST(_overlapped_FormatMessage), METH_O, _overlapped_FormatMessage__doc__},

static PyObject *
_overlapped_FormatMessage_impl(PyObject *module, DWORD code);

static PyValue
_overlapped_FormatMessage(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    DWORD code;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &code)) {
            goto exit;
        }
    }
    _return_value = _overlapped_FormatMessage_impl(module, code);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped__doc__,
"Overlapped(event=_overlapped.INVALID_HANDLE_VALUE)\n"
"--\n"
"\n"
"OVERLAPPED structure wrapper.");

static PyObject *
_overlapped_Overlapped_impl(PyTypeObject *type, HANDLE event);

static PyObject *
_overlapped_Overlapped(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(event), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"event", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Overlapped",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    HANDLE event = INVALID_HANDLE_VALUE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        PyObject *_arg = PyValue_Object(fastargs[0]);
        if (_arg == NULL) {
            goto exit;
        }
        event = PyLong_AsVoidPtr(_arg);
        if (!event && PyErr_Occurred()) {
            goto exit;
        }
    }
skip_optional_pos:
    return_value = _overlapped_Overlapped_impl(type, event);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_cancel__doc__,
"cancel($self, /)\n"
"--\n"
"\n"
"Cancel overlapped operation.");

#define _OVERLAPPED_OVERLAPPED_CANCEL_METHODDEF    \
    {"cancel", (PyCFunction)_overlapped_Overlapped_cancel, METH_NOARGS, _overlapped_Overlapped_cancel__doc__},

static PyObject *
_overlapped_Overlapped_cancel_impl(OverlappedObject *self);

static PyObject *
_overlapped_Overlapped_cancel(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _overlapped_Overlapped_cancel_impl((OverlappedObject *)self);
}

PyDoc_STRVAR(_overlapped_Overlapped_getresult__doc__,
"getresult($self, wait=False, /)\n"
"--\n"
"\n"
"Retrieve result of operation.\n"
"\n"
"If wait is true then it blocks until the operation is finished.  If\n"
"wait is false and the operation is still pending then an error is\n"
"raised.");

#define _OVERLAPPED_OVERLAPPED_GETRESULT_METHODDEF    \
    {"getresult", _PyCFunction_CAST(_overlapped_Overlapped_getresult), METH_FASTCALL, _overlapped_Overlapped_getresult__doc__},

static PyObject *
_overlapped_Overlapped_getresult_impl(OverlappedObject *self, BOOL wait);

static PyValue
_overlapped_Overlapped_getresult(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    BOOL wait = FALSE;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("getresult", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    wait = PyLong_AsIntValue(args[0]);
    if (wait == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = _overlapped_Overlapped_getresult_impl((OverlappedObject *)self, wait);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ReadFile__doc__,
"ReadFile($self, handle, size, /)\n"
"--\n"
"\n"
"Start overlapped read.");

#define _OVERLAPPED_OVERLAPPED_READFILE_METHODDEF    \
    {"ReadFile", _PyCFunction_CAST(_overlapped_Overlapped_ReadFile), METH_FASTCALL, _overlapped_Overlapped_ReadFile__doc__},

static PyObject *
_overlapped_Overlapped_ReadFile_impl(OverlappedObject *self, HANDLE handle,
                                     DWORD size);

static PyValue
_overlapped_Overlapped_ReadFile(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    DWORD size;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("ReadFile", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &size)) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_ReadFile_impl((OverlappedObject *)self, handle, size);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ReadFileInto__doc__,
"ReadFileInto($self, handle, buf, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_READFILEINTO_METHODDEF    \
    {"ReadFileInto", _PyCFunction_CAST(_overlapped_Overlapped_ReadFileInto), METH_FASTCALL, _overlapped_Overlapped_ReadFileInto__doc__},

static PyObject *
_overlapped_Overlapped_ReadFileInto_impl(OverlappedObject *self,
                                         HANDLE handle, Py_buffer *bufobj);

static PyValue
_overlapped_Overlapped_ReadFileInto(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("ReadFileInto", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &bufobj, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_ReadFileInto_impl((OverlappedObject *)self, handle, &bufobj);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSARecv__doc__,
"WSARecv($self, handle, size, flags=0, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_WSARECV_METHODDEF    \
    {"WSARecv", _PyCFunction_CAST(_overlapped_Overlapped_WSARecv), METH_FASTCALL, _overlapped_Overlapped_WSARecv__doc__},

static PyObject *
_overlapped_Overlapped_WSARecv_impl(OverlappedObject *self, HANDLE handle,
                                    DWORD size, DWORD flags);

static PyValue
_overlapped_Overlapped_WSARecv(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    DWORD size;
    DWORD flags = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("WSARecv", nargs, 2, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &size)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &flags)) {
            goto exit;
        }
    }
skip_optional:
    _return_value = _overlapped_Overlapped_WSARecv_impl((OverlappedObject *)self, handle, size, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSARecvInto__doc__,
"WSARecvInto($self, handle, buf, flags, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_WSARECVINTO_METHODDEF    \
    {"WSARecvInto", _PyCFunction_CAST(_overlapped_Overlapped_WSARecvInto), METH_FASTCALL, _overlapped_Overlapped_WSARecvInto__doc__},

static PyObject *
_overlapped_Overlapped_WSARecvInto_impl(OverlappedObject *self,
                                        HANDLE handle, Py_buffer *bufobj,
                                        DWORD flags);

static PyValue
_overlapped_Overlapped_WSARecvInto(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    DWORD flags;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("WSARecvInto", nargs, 3, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &bufobj, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &flags)) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_WSARecvInto_impl((OverlappedObject *)self, handle, &bufobj, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WriteFile__doc__,
"WriteFile($self, handle, buf, /)\n"
"--\n"
"\n"
"Start overlapped write.");

#define _OVERLAPPED_OVERLAPPED_WRITEFILE_METHODDEF    \
    {"WriteFile", _PyCFunction_CAST(_overlapped_Overlapped_WriteFile), METH_FASTCALL, _overlapped_Overlapped_WriteFile__doc__},

static PyObject *
_overlapped_Overlapped_WriteFile_impl(OverlappedObject *self, HANDLE handle,
                                      Py_buffer *bufobj);

static PyValue
_overlapped_Overlapped_WriteFile(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("WriteFile", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &bufobj, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_WriteFile_impl((OverlappedObject *)self, handle, &bufobj);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSASend__doc__,
"WSASend($self, handle, buf, flags, /)\n"
"--\n"
"\n"
"Start overlapped send.");

#define _OVERLAPPED_OVERLAPPED_WSASEND_METHODDEF    \
    {"WSASend", _PyCFunction_CAST(_overlapped_Overlapped_WSASend), METH_FASTCALL, _overlapped_Overlapped_WSASend__doc__},

static PyObject *
_overlapped_Overlapped_WSASend_impl(OverlappedObject *self, HANDLE handle,
                                    Py_buffer *bufobj, DWORD flags);

static PyValue
_overlapped_Overlapped_WSASend(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    DWORD flags;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("WSASend", nargs, 3, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &bufobj, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &flags)) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_WSASend_impl((OverlappedObject *)self, handle, &bufobj, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_AcceptEx__doc__,
"AcceptEx($self, listen_handle, accept_handle, /)\n"
"--\n"
"\n"
"Start overlapped wait for client to connect.");

#define _OVERLAPPED_OVERLAPPED_ACCEPTEX_METHODDEF    \
    {"AcceptEx", _PyCFunction_CAST(_overlapped_Overlapped_AcceptEx), METH_FASTCALL, _overlapped_Overlapped_AcceptEx__doc__},

static PyObject *
_overlapped_Overlapped_AcceptEx_impl(OverlappedObject *self,
                                     HANDLE ListenSocket,
                                     HANDLE AcceptSocket);

static PyValue
_overlapped_Overlapped_AcceptEx(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE ListenSocket;
    HANDLE AcceptSocket;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("AcceptEx", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        ListenSocket = PyLong_AsVoidPtr(_arg);
        if (!ListenSocket && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        AcceptSocket = PyLong_AsVoidPtr(_arg);
        if (!AcceptSocket && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_AcceptEx_impl((OverlappedObject *)self, ListenSocket, AcceptSocket);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ConnectEx__doc__,
"ConnectEx($self, client_handle, address_as_bytes, /)\n"
"--\n"
"\n"
"Start overlapped connect.\n"
"\n"
"client_handle should be unbound.");

#define _OVERLAPPED_OVERLAPPED_CONNECTEX_METHODDEF    \
    {"ConnectEx", _PyCFunction_CAST(_overlapped_Overlapped_ConnectEx), METH_FASTCALL, _overlapped_Overlapped_ConnectEx__doc__},

static PyObject *
_overlapped_Overlapped_ConnectEx_impl(OverlappedObject *self,
                                      HANDLE ConnectSocket,
                                      PyObject *AddressObj);

static PyValue
_overlapped_Overlapped_ConnectEx(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE ConnectSocket;
    PyObject *AddressObj;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("ConnectEx", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        ConnectSocket = PyLong_AsVoidPtr(_arg);
        if (!ConnectSocket && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("ConnectEx", "argument 2", "tuple", _arg);
            goto exit;
        }
        AddressObj = _arg;
    }
    _return_value = _overlapped_Overlapped_ConnectEx_impl((OverlappedObject *)self, ConnectSocket, AddressObj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_DisconnectEx__doc__,
"DisconnectEx($self, handle, flags, /)\n"
"--\n"
"\n");

#define _OVERLAPPED_OVERLAPPED_DISCONNECTEX_METHODDEF    \
    {"DisconnectEx", _PyCFunction_CAST(_overlapped_Overlapped_DisconnectEx), METH_FASTCALL, _overlapped_Overlapped_DisconnectEx__doc__},

static PyObject *
_overlapped_Overlapped_DisconnectEx_impl(OverlappedObject *self,
                                         HANDLE Socket, DWORD flags);

static PyValue
_overlapped_Overlapped_DisconnectEx(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE Socket;
    DWORD flags;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("DisconnectEx", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        Socket = PyLong_AsVoidPtr(_arg);
        if (!Socket && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &flags)) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_DisconnectEx_impl((OverlappedObject *)self, Socket, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_TransmitFile__doc__,
"TransmitFile($self, socket, file, offset, offset_high, count_to_write,\n"
"             count_per_send, flags, /)\n"
"--\n"
"\n"
"Transmit file data over a connected socket.");

#define _OVERLAPPED_OVERLAPPED_TRANSMITFILE_METHODDEF    \
    {"TransmitFile", _PyCFunction_CAST(_overlapped_Overlapped_TransmitFile), METH_FASTCALL, _overlapped_Overlapped_TransmitFile__doc__},

static PyObject *
_overlapped_Overlapped_TransmitFile_impl(OverlappedObject *self,
                                         HANDLE Socket, HANDLE File,
                                         DWORD offset, DWORD offset_high,
                                         DWORD count_to_write,
                                         DWORD count_per_send, DWORD flags);

static PyValue
_overlapped_Overlapped_TransmitFile(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE Socket;
    HANDLE File;
    DWORD offset;
    DWORD offset_high;
    DWORD count_to_write;
    DWORD count_per_send;
    DWORD flags;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("TransmitFile", nargs, 7, 7)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        Socket = PyLong_AsVoidPtr(_arg);
        if (!Socket && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        File = PyLong_AsVoidPtr(_arg);
        if (!File && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &offset)) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &offset_high)) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[4]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &count_to_write)) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[5]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &count_per_send)) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[6]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &flags)) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_TransmitFile_impl((OverlappedObject *)self, Socket, File, offset, offset_high, count_to_write, count_per_send, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ConnectNamedPipe__doc__,
"ConnectNamedPipe($self, handle, /)\n"
"--\n"
"\n"
"Start overlapped wait for a client to connect.");

#define _OVERLAPPED_OVERLAPPED_CONNECTNAMEDPIPE_METHODDEF    \
    {"ConnectNamedPipe", _PyCFunction_CAST(_overlapped_Overlapped_ConnectNamedPipe), METH_O, _overlapped_Overlapped_ConnectNamedPipe__doc__},

static PyObject *
_overlapped_Overlapped_ConnectNamedPipe_impl(OverlappedObject *self,
                                             HANDLE Pipe);

static PyValue
_overlapped_Overlapped_ConnectNamedPipe(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    HANDLE Pipe;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        Pipe = PyLong_AsVoidPtr(_arg);
        if (!Pipe && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_ConnectNamedPipe_impl((OverlappedObject *)self, Pipe);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_ConnectPipe__doc__,
"ConnectPipe($self, addr, /)\n"
"--\n"
"\n"
"Connect to the pipe for asynchronous I/O (overlapped).");

#define _OVERLAPPED_OVERLAPPED_CONNECTPIPE_METHODDEF    \
    {"ConnectPipe", _PyCFunction_CAST(_overlapped_Overlapped_ConnectPipe), METH_O, _overlapped_Overlapped_ConnectPipe__doc__},

static PyObject *
_overlapped_Overlapped_ConnectPipe_impl(OverlappedObject *self,
                                        const wchar_t *Address);

static PyValue
_overlapped_Overlapped_ConnectPipe(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const wchar_t *Address = NULL;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("ConnectPipe", "argument", "str", _arg);
            goto exit;
        }
        Address = PyUnicode_AsWideCharString(_arg, NULL);
        if (Address == NULL) {
            goto exit;
        }
    }
    _return_value = _overlapped_Overlapped_ConnectPipe_impl((OverlappedObject *)self, Address);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for Address */
    PyMem_Free((void *)Address);

    return return_value;
}

PyDoc_STRVAR(_overlapped_WSAConnect__doc__,
"WSAConnect($module, client_handle, address_as_bytes, /)\n"
"--\n"
"\n"
"Bind a remote address to a connectionless (UDP) socket.");

#define _OVERLAPPED_WSACONNECT_METHODDEF    \
    {"WSAConnect", _PyCFunction_CAST(_overlapped_WSAConnect), METH_FASTCALL, _overlapped_WSAConnect__doc__},

static PyObject *
_overlapped_WSAConnect_impl(PyObject *module, HANDLE ConnectSocket,
                            PyObject *AddressObj);

static PyValue
_overlapped_WSAConnect(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE ConnectSocket;
    PyObject *AddressObj;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("WSAConnect", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        ConnectSocket = PyLong_AsVoidPtr(_arg);
        if (!ConnectSocket && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("WSAConnect", "argument 2", "tuple", _arg);
            goto exit;
        }
        AddressObj = _arg;
    }
    _return_value = _overlapped_WSAConnect_impl(module, ConnectSocket, AddressObj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSASendTo__doc__,
"WSASendTo($self, handle, buf, flags, address_as_bytes, /)\n"
"--\n"
"\n"
"Start overlapped sendto over a connectionless (UDP) socket.");

#define _OVERLAPPED_OVERLAPPED_WSASENDTO_METHODDEF    \
    {"WSASendTo", _PyCFunction_CAST(_overlapped_Overlapped_WSASendTo), METH_FASTCALL, _overlapped_Overlapped_WSASendTo__doc__},

static PyObject *
_overlapped_Overlapped_WSASendTo_impl(OverlappedObject *self, HANDLE handle,
                                      Py_buffer *bufobj, DWORD flags,
                                      PyObject *AddressObj);

static PyValue
_overlapped_Overlapped_WSASendTo(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    DWORD flags;
    PyObject *AddressObj;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("WSASendTo", nargs, 4, 4)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &bufobj, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &flags)) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("WSASendTo", "argument 4", "tuple", _arg);
            goto exit;
        }
        AddressObj = _arg;
    }
    _return_value = _overlapped_Overlapped_WSASendTo_impl((OverlappedObject *)self, handle, &bufobj, flags, AddressObj);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSARecvFrom__doc__,
"WSARecvFrom($self, handle, size, flags=0, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_WSARECVFROM_METHODDEF    \
    {"WSARecvFrom", _PyCFunction_CAST(_overlapped_Overlapped_WSARecvFrom), METH_FASTCALL, _overlapped_Overlapped_WSARecvFrom__doc__},

static PyObject *
_overlapped_Overlapped_WSARecvFrom_impl(OverlappedObject *self,
                                        HANDLE handle, DWORD size,
                                        DWORD flags);

static PyValue
_overlapped_Overlapped_WSARecvFrom(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    DWORD size;
    DWORD flags = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("WSARecvFrom", nargs, 2, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &size)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &flags)) {
            goto exit;
        }
    }
skip_optional:
    _return_value = _overlapped_Overlapped_WSARecvFrom_impl((OverlappedObject *)self, handle, size, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_overlapped_Overlapped_WSARecvFromInto__doc__,
"WSARecvFromInto($self, handle, buf, size, flags=0, /)\n"
"--\n"
"\n"
"Start overlapped receive.");

#define _OVERLAPPED_OVERLAPPED_WSARECVFROMINTO_METHODDEF    \
    {"WSARecvFromInto", _PyCFunction_CAST(_overlapped_Overlapped_WSARecvFromInto), METH_FASTCALL, _overlapped_Overlapped_WSARecvFromInto__doc__},

static PyObject *
_overlapped_Overlapped_WSARecvFromInto_impl(OverlappedObject *self,
                                            HANDLE handle, Py_buffer *bufobj,
                                            DWORD size, DWORD flags);

static PyValue
_overlapped_Overlapped_WSARecvFromInto(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    HANDLE handle;
    Py_buffer bufobj = {NULL, NULL};
    DWORD size;
    DWORD flags = 0;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("WSARecvFromInto", nargs, 3, 4)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        handle = PyLong_AsVoidPtr(_arg);
        if (!handle && PyErr_Occurred()) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &bufobj, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &size)) {
            goto exit;
        }
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &flags)) {
            goto exit;
        }
    }
skip_optional:
    _return_value = _overlapped_Overlapped_WSARecvFromInto_impl((OverlappedObject *)self, handle, &bufobj, size, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for bufobj */
    if (bufobj.obj) {
       PyBuffer_Release(&bufobj);
    }

    return return_value;
}
/*[clinic end generated code: output=d8ea9dc0ed342da8 input=a9049054013a1b77]*/
