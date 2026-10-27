/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(subprocess_fork_exec__doc__,
"fork_exec($module, args, executable_list, close_fds, pass_fds, cwd,\n"
"          env, p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite,\n"
"          errpipe_read, errpipe_write, restore_signals, call_setsid,\n"
"          pgid_to_set, gid, extra_groups, uid, child_umask, preexec_fn,\n"
"          /)\n"
"--\n"
"\n"
"Spawn a fresh new child process.\n"
"\n"
"Fork a child process, close parent file descriptors as appropriate in\n"
"the child and duplicate the few that are needed before calling exec() in\n"
"the child process.\n"
"\n"
"If close_fds is True, close file descriptors 3 and higher, except those\n"
"listed in the sorted tuple pass_fds.\n"
"\n"
"The preexec_fn, if supplied, will be called immediately before closing\n"
"file descriptors and exec.\n"
"\n"
"WARNING: preexec_fn is NOT SAFE if your application uses threads.\n"
"         It may trigger infrequent, difficult to debug deadlocks.\n"
"\n"
"If an error occurs in the child process before the exec, it is\n"
"serialized and written to the errpipe_write fd per subprocess.py.\n"
"\n"
"Returns: the child process\'s PID.\n"
"\n"
"Raises: Only on an error in the parent process.");

#define SUBPROCESS_FORK_EXEC_METHODDEF    \
    {"fork_exec", _PyCFunction_CAST(subprocess_fork_exec), METH_FASTCALL, subprocess_fork_exec__doc__},

static PyObject *
subprocess_fork_exec_impl(PyObject *module, PyObject *process_args,
                          PyObject *executable_list, int close_fds,
                          PyObject *py_fds_to_keep, PyObject *cwd_obj,
                          PyObject *env_list, int p2cread, int p2cwrite,
                          int c2pread, int c2pwrite, int errread,
                          int errwrite, int errpipe_read, int errpipe_write,
                          int restore_signals, int call_setsid,
                          pid_t pgid_to_set, PyObject *gid_object,
                          PyObject *extra_groups_packed,
                          PyObject *uid_object, int child_umask,
                          PyObject *preexec_fn);

static PyValue
subprocess_fork_exec(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *process_args;
    PyObject *executable_list;
    int close_fds;
    PyObject *py_fds_to_keep;
    PyObject *cwd_obj;
    PyObject *env_list;
    int p2cread;
    int p2cwrite;
    int c2pread;
    int c2pwrite;
    int errread;
    int errwrite;
    int errpipe_read;
    int errpipe_write;
    int restore_signals;
    int call_setsid;
    pid_t pgid_to_set;
    PyObject *gid_object;
    PyObject *extra_groups_packed;
    PyObject *uid_object;
    int child_umask;
    PyObject *preexec_fn;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("fork_exec", nargs, 22, 22)) {
        goto exit;
    }
    process_args = PyValue_Object(args[0]);
    if (process_args == NULL) {
        goto exit;
    }
    executable_list = PyValue_Object(args[1]);
    if (executable_list == NULL) {
        goto exit;
    }
    close_fds = PyObject_IsTrueValue(args[2]);
    if (close_fds < 0) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("fork_exec", "argument 4", "tuple", _arg);
            goto exit;
        }
        py_fds_to_keep = _arg;
    }
    cwd_obj = PyValue_Object(args[4]);
    if (cwd_obj == NULL) {
        goto exit;
    }
    env_list = PyValue_Object(args[5]);
    if (env_list == NULL) {
        goto exit;
    }
    p2cread = PyLong_AsIntValue(args[6]);
    if (p2cread == -1 && PyErr_Occurred()) {
        goto exit;
    }
    p2cwrite = PyLong_AsIntValue(args[7]);
    if (p2cwrite == -1 && PyErr_Occurred()) {
        goto exit;
    }
    c2pread = PyLong_AsIntValue(args[8]);
    if (c2pread == -1 && PyErr_Occurred()) {
        goto exit;
    }
    c2pwrite = PyLong_AsIntValue(args[9]);
    if (c2pwrite == -1 && PyErr_Occurred()) {
        goto exit;
    }
    errread = PyLong_AsIntValue(args[10]);
    if (errread == -1 && PyErr_Occurred()) {
        goto exit;
    }
    errwrite = PyLong_AsIntValue(args[11]);
    if (errwrite == -1 && PyErr_Occurred()) {
        goto exit;
    }
    errpipe_read = PyLong_AsIntValue(args[12]);
    if (errpipe_read == -1 && PyErr_Occurred()) {
        goto exit;
    }
    errpipe_write = PyLong_AsIntValue(args[13]);
    if (errpipe_write == -1 && PyErr_Occurred()) {
        goto exit;
    }
    restore_signals = PyObject_IsTrueValue(args[14]);
    if (restore_signals < 0) {
        goto exit;
    }
    call_setsid = PyObject_IsTrueValue(args[15]);
    if (call_setsid < 0) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[16]);
        if (_arg == NULL) {
            goto exit;
        }
        pgid_to_set = PyLong_AsPid(_arg);
        if (pgid_to_set == (pid_t)(-1) && PyErr_Occurred()) {
            goto exit;
        }
    }
    gid_object = PyValue_Object(args[17]);
    if (gid_object == NULL) {
        goto exit;
    }
    extra_groups_packed = PyValue_Object(args[18]);
    if (extra_groups_packed == NULL) {
        goto exit;
    }
    uid_object = PyValue_Object(args[19]);
    if (uid_object == NULL) {
        goto exit;
    }
    child_umask = PyLong_AsIntValue(args[20]);
    if (child_umask == -1 && PyErr_Occurred()) {
        goto exit;
    }
    preexec_fn = PyValue_Object(args[21]);
    if (preexec_fn == NULL) {
        goto exit;
    }
    _return_value = subprocess_fork_exec_impl(module, process_args, executable_list, close_fds, py_fds_to_keep, cwd_obj, env_list, p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite, errpipe_read, errpipe_write, restore_signals, call_setsid, pgid_to_set, gid_object, extra_groups_packed, uid_object, child_umask, preexec_fn);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=9149607dd1d1b271 input=a9049054013a1b77]*/
