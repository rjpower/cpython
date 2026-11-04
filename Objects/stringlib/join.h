/* stringlib: bytes joining implementation */

#if STRINGLIB_IS_UNICODE
#error join.h only compatible with byte-wise strings
#endif

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(bytes_join_lock_held)(PyObject *sep, PyObject *seq)
{
    const char *sepstr = STRINGLIB_STR(sep);
    Py_ssize_t seplen = STRINGLIB_LEN(sep);
    PyObject *res = NULL;
    char *p;
    Py_ssize_t seqlen = PySequence_Fast_GET_SIZE(seq);
    Py_ssize_t sz = 0;
    Py_ssize_t i, nbufs = 0;
    PyObject *item = NULL;
    Py_buffer *buffers = NULL;
#define NB_STATIC_BUFFERS 10
    Py_buffer static_buffers[NB_STATIC_BUFFERS];
#define GIL_THRESHOLD 1048576
    int drop_gil = 1;
    PyThreadState *save = NULL;

    if (seqlen == 0) {
        res = STRINGLIB_NEW(NULL, 0);
        goto error;
    }
#if !STRINGLIB_MUTABLE
    if (seqlen == 1) {
        item = PyList_CheckExact(seq) ? PyList_GetItemRef(seq, 0) :
                                       PyTuple_GetItemRef(seq, 0);
        if (item == NULL || PyErr_Occurred()) goto error;
        if (STRINGLIB_CHECK_EXACT(item)) {
            res = item;
            item = NULL;
            return res;
        }
        Py_CLEAR(item);
        if (PyErr_Occurred()) goto error;
    }
#endif
    if (seqlen > NB_STATIC_BUFFERS) {
        buffers = PyMem_NEW(Py_buffer, seqlen);
        if (buffers == NULL) {
            PyErr_NoMemory();
            goto error;
        }
    }
    else {
        buffers = static_buffers;
    }

    for (i = 0; i < seqlen; i++) {
        Py_ssize_t itemlen;
        if (seqlen != PySequence_Fast_GET_SIZE(seq)) {
            PyErr_SetString(PyExc_RuntimeError,
                            "sequence changed size during iteration");
            goto error;
        }
        item = PyList_CheckExact(seq) ? PyList_GetItemRef(seq, i) :
                                       PyTuple_GetItemRef(seq, i);
        if (item == NULL || PyErr_Occurred()) goto error;
        if (PyBytes_CheckExact(item)) {
            buffers[i].obj = item;
            item = NULL;
            buffers[i].buf = PyBytes_AS_STRING(buffers[i].obj);
            buffers[i].len = PyBytes_GET_SIZE(buffers[i].obj);
            nbufs = i + 1;
        }
        else {
            buffers[i].obj = NULL;
            int status = PyObject_GetBuffer(item, &buffers[i], PyBUF_SIMPLE);
            if (status == 0) {
                nbufs = i + 1;
                Py_CLEAR(item);
                if (PyErr_Occurred()) goto error;
                drop_gil = 0;
            }
            else {
                PyObject *error_type = Py_NewRef((PyObject *)Py_TYPE(item));
                PyObject *error_name = NULL;
                if (PyType_HasFeature((PyTypeObject *)error_type,
                                     Py_TPFLAGS_HEAPTYPE)) {
                    error_name = Py_XNewRef(
                        ((PyHeapTypeObject *)error_type)->ht_name);
                }
                const char *type_name = ((PyTypeObject *)error_type)->tp_name;
                PyErr_Format(PyExc_TypeError,
                             "sequence item %zd: expected a bytes-like object, "
                             "%.80s found",
                             i, type_name);
                Py_CLEAR(error_name);
                Py_CLEAR(error_type);
                Py_CLEAR(item);
                goto error;
            }
        }
        if (PyErr_Occurred()) goto error;
        itemlen = buffers[i].len;
        if (itemlen > PY_SSIZE_T_MAX - sz) {
            PyErr_SetString(PyExc_OverflowError,
                            "join() result is too long");
            goto error;
        }
        sz += itemlen;
        if (i != 0) {
            if (seplen > PY_SSIZE_T_MAX - sz) {
                PyErr_SetString(PyExc_OverflowError,
                                "join() result is too long");
                goto error;
            }
            sz += seplen;
        }
        if (seqlen != PySequence_Fast_GET_SIZE(seq)) {
            PyErr_SetString(PyExc_RuntimeError,
                            "sequence changed size during iteration");
            goto error;
        }
    }

    res = STRINGLIB_NEW(NULL, sz);
    if (res == NULL || PyErr_Occurred()) goto error;

    p = STRINGLIB_STR(res);
    if (sz < GIL_THRESHOLD) {
        drop_gil = 0;
    }
    if (drop_gil) {
        save = PyEval_SaveThread();
    }
    if (!seplen) {
        for (i = 0; i < nbufs; i++) {
            Py_ssize_t n = buffers[i].len;
            char *q = buffers[i].buf;
            memcpy(p, q, n);
            p += n;
        }
    }
    else {
        Py_ssize_t n = buffers[0].len;
        char *q = buffers[0].buf;
        memcpy(p, q, n);
        p += n;
        for (i = 1; i < nbufs; i++) {
            memcpy(p, sepstr, seplen);
            p += seplen;
            n = buffers[i].len;
            q = buffers[i].buf;
            memcpy(p, q, n);
            p += n;
        }
    }
    if (drop_gil) {
        PyEval_RestoreThread(save);
    }

error:
    if (item != NULL) {
        Py_CLEAR(item);
    }
    for (i = 0; i < nbufs; i++)
        PyBuffer_Release(&buffers[i]);
    if (buffers != NULL && buffers != static_buffers)
        PyMem_Free(buffers);
    if (PyErr_Occurred()) {
        Py_CLEAR(res);
        return NULL;
    }
    return res;
}

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(bytes_join)(PyObject *sep, PyObject *iterable)
{
    PyObject *seq = NULL;
    PyObject *res = NULL;

    seq = PySequence_Fast(iterable, "can only join an iterable");
    if (seq == NULL) goto cleanup;

    Py_BEGIN_CRITICAL_SECTION_SEQUENCE_FAST(iterable);
    res = STRINGLIB(bytes_join_lock_held)(sep, seq);
    Py_END_CRITICAL_SECTION_SEQUENCE_FAST();

cleanup:
    if (seq != NULL) {
        Py_CLEAR(seq);
    }
    if (PyErr_Occurred()) {
        Py_CLEAR(res);
        return NULL;
    }
    return res;
}

#undef NB_STATIC_BUFFERS
#undef GIL_THRESHOLD
