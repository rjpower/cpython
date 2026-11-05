/* SHA1 module */

/* This module provides an interface to the SHA1 algorithm */

/* See below for information about the original code this module was
   based upon. Additional work performed by:

   Andrew Kuchling (amk@amk.ca)
   Greg Stein (gstein@lyra.org)
   Trevor Perrin (trevp@trevp.net)
   Bénédikt Tran (10796600+picnixz@users.noreply.github.com)

   Copyright (C) 2005-2007   Gregory P. Smith (greg@krypto.org)
   Licensed to PSF under a Contributor Agreement.

*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#define Py_VALUE_CORE 1
#include "Python.h"
#include "hashlib.h"
#include "pycore_object.h"        // _PyObject_VisitType()
#include "pycore_strhex.h"        // _Py_strhex()
#include "pycore_typeobject.h"    // _PyType_GetModuleState()
#include "pycore_value.h"         // _PyValue_AsObjectAt()
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()

#include "_hacl/Hacl_Hash_SHA1.h"

/* The SHA1 block size and message digest sizes, in bytes */

#define SHA1_BLOCKSIZE    64
#define SHA1_DIGESTSIZE   20

// --- Module objects ---------------------------------------------------------

typedef struct {
    HASHLIB_OBJECT_HEAD
    Hacl_Hash_SHA1_state_t *hash_state;
} SHA1object;

#define _SHA1object_CAST(op)    ((SHA1object *)(op))

// --- Module state -----------------------------------------------------------

typedef struct {
    PyTypeObject* sha1_type;
} SHA1State;

static inline SHA1State*
sha1_get_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (SHA1State *)state;
}

// --- Module clinic configuration --------------------------------------------

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _sha1
class SHA1Type "SHA1object *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3dc9a20d1becb759]*/

#include "clinic/sha1module.c.h"

// --- SHA-1 object interface configuration -----------------------------------

static SHA1object *
newSHA1object(SHA1State *st)
{
    SHA1object *sha = PyObject_GC_New(SHA1object, st->sha1_type);
    if (sha == NULL) {
        return NULL;
    }
    HASHLIB_INIT_MUTEX(sha);
    sha->hash_state = NULL; // PyObject_GC_New() does not zero memory

    PyObject_GC_Track(sha);
    return sha;
}


/* Internal methods for a hash object */
static void
SHA1_dealloc(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    SHA1object *ptr = _SHA1object_CAST(op);
    if (ptr->hash_state != NULL) {
        Hacl_Hash_SHA1_free(ptr->hash_state);
        ptr->hash_state = NULL;
    }
    PyObject_GC_Del(ptr);
    Py_DECREF(tp);
}


/* External methods for a hash object */

/*[clinic input]
SHA1Type.copy

    cls: defining_class

Return a copy of the hash object.
[clinic start generated code]*/

static PyValue
SHA1Type_copy_impl(SHA1object *self, PyTypeObject *cls)
/*[clinic end generated code: output=e21f40276948f989 input=6c22e66fcc34c58e]*/
{
    SHA1State *st = _PyType_GetModuleState(cls);

    SHA1object *newobj;
    if ((newobj = newSHA1object(st)) == NULL) {
        return PyValue_NULL;
    }

    HASHLIB_ACQUIRE_LOCK(self);
    newobj->hash_state = Hacl_Hash_SHA1_copy(self->hash_state);
    HASHLIB_RELEASE_LOCK(self);
    if (newobj->hash_state == NULL) {
        Py_DECREF(newobj);
        return _PyValue_FromObjectResult(PyErr_NoMemory());
    }
    return _PyValue_FromObjectResult((PyObject *)newobj);
}

/*[clinic input]
SHA1Type.digest

Return the digest value as a bytes object.
[clinic start generated code]*/

static PyValue
SHA1Type_digest_impl(SHA1object *self)
/*[clinic end generated code: output=e4327ef09649d9a3 input=13824b35407444bd]*/
{
    unsigned char digest[SHA1_DIGESTSIZE];
    HASHLIB_ACQUIRE_LOCK(self);
    Hacl_Hash_SHA1_digest(self->hash_state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return _PyValue_FromObjectResult(PyBytes_FromStringAndSize((const char *)digest, SHA1_DIGESTSIZE));
}

/*[clinic input]
SHA1Type.hexdigest

Return the digest value as a string of hexadecimal digits.
[clinic start generated code]*/

static PyValue
SHA1Type_hexdigest_impl(SHA1object *self)
/*[clinic end generated code: output=21b577ebf71bf84e input=97691055c0c74ab0]*/
{
    unsigned char digest[SHA1_DIGESTSIZE];
    HASHLIB_ACQUIRE_LOCK(self);
    Hacl_Hash_SHA1_digest(self->hash_state, digest);
    HASHLIB_RELEASE_LOCK(self);
    return _PyValue_FromObjectResult(_Py_strhex((const char *)digest, SHA1_DIGESTSIZE));
}

static void
update(Hacl_Hash_SHA1_state_t *state, uint8_t *buf, Py_ssize_t len)
{
    /*
     * Note: we explicitly ignore the error code on the basis that it would
     * take more than 1 billion years to overflow the maximum admissible length
     * for SHA-1 (2^61 - 1).
     */
#if PY_SSIZE_T_MAX > UINT32_MAX
    while (len > UINT32_MAX) {
        (void)Hacl_Hash_SHA1_update(state, buf, UINT32_MAX);
        len -= UINT32_MAX;
        buf += UINT32_MAX;
    }
#endif
    /* cast to uint32_t is now safe */
    (void)Hacl_Hash_SHA1_update(state, buf, (uint32_t)len);
}

/*[clinic input]
SHA1Type.update

    obj: object
    /

Update this hash object's state with the provided string.
[clinic start generated code]*/

static PyValue
SHA1Type_update_impl(SHA1object *self, PyValueBorrow obj)
/*[clinic end generated code: output=86f0ea4d49c04a0a input=aad8e07812edbba3]*/
{
    Py_buffer buf;
    PyObject *obj_o = _PyValue_AsObjectAt(obj, BOX_SITE_PARSER);
    if (obj_o == NULL) {
        return PyValue_NULL;
    }
    GET_BUFFER_VIEW_OR_ERROR(obj_o, &buf, {Py_DECREF(obj_o); return PyValue_NULL;});
    Py_DECREF(obj_o);
    HASHLIB_EXTERNAL_INSTRUCTIONS_LOCKED(
        self, buf.len,
        update(self->hash_state, buf.buf, buf.len)
    );
    PyBuffer_Release(&buf);
    return PyValue_None;
}

static PyMethodDef SHA1_methods[] = {
    SHA1TYPE_COPY_METHODDEF
    SHA1TYPE_DIGEST_METHODDEF
    SHA1TYPE_HEXDIGEST_METHODDEF
    SHA1TYPE_UPDATE_METHODDEF
    {NULL,        NULL}         /* sentinel */
};

static PyObject *
SHA1_get_block_size(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyLong_FromLong(SHA1_BLOCKSIZE);
}

static PyObject *
SHA1_get_name(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyUnicode_FromStringAndSize("sha1", 4);
}

static PyObject *
sha1_get_digest_size(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyLong_FromLong(SHA1_DIGESTSIZE);
}

static PyGetSetDef SHA1_getseters[] = {
    {"block_size", SHA1_get_block_size, NULL, NULL, NULL},
    {"name", SHA1_get_name, NULL, NULL, NULL},
    {"digest_size", sha1_get_digest_size, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};

static PyType_Slot sha1_type_slots[] = {
    {Py_tp_dealloc, SHA1_dealloc},
    {Py_tp_methods, SHA1_methods},
    {Py_tp_getset, SHA1_getseters},
    {Py_tp_traverse, _PyObject_VisitType},
    {0,0}
};

static PyType_Spec sha1_type_spec = {
    .name = "_sha1.sha1",
    .basicsize =  sizeof(SHA1object),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC),
    .slots = sha1_type_slots
};

/* The single module-level function: new() */

/*[clinic input]
_sha1.sha1

    data: object(c_default="NULL") = b''
    *
    usedforsecurity: bool = True
    string as string_obj: object(c_default="NULL") = None

Return a new SHA1 hash object; optionally initialized with a string.
[clinic start generated code]*/

static PyValue
_sha1_sha1_impl(PyObject *module, PyValueBorrow data, int usedforsecurity,
                PyValueBorrow string_obj)
/*[clinic end generated code: output=b446c6aaf3c148a3 input=807f25264e0ac656]*/
{
    SHA1object *new;
    Py_buffer buf = {NULL, NULL};
    PyObject *data_o = NULL, *string_o = NULL;
    if (!PyValue_IsNull(data)) {
        data_o = _PyValue_AsObjectAt(data, BOX_SITE_PARSER);
        if (data_o == NULL) {
            return PyValue_NULL;
        }
    }
    if (!PyValue_IsNull(string_obj)) {
        string_o = _PyValue_AsObjectAt(string_obj, BOX_SITE_PARSER);
        if (string_o == NULL) {
            Py_XDECREF(data_o);
            return PyValue_NULL;
        }
    }
    PyObject *string;
    if (_Py_hashlib_data_argument(&string, data_o, string_o) < 0) {
        Py_XDECREF(data_o);
        Py_XDECREF(string_o);
        return PyValue_NULL;
    }

    // buf.obj is NULL for an exporter that fills the view without one.
    int have_string = (string != NULL);
    if (string) {
        GET_BUFFER_VIEW_OR_ERROR(string, &buf, {
            Py_XDECREF(data_o);
            Py_XDECREF(string_o);
            return PyValue_NULL;
        });
    }
    // buf keeps its own reference to the object, so the boxes can go.
    Py_XDECREF(data_o);
    Py_XDECREF(string_o);

    SHA1State *st = sha1_get_state(module);
    if ((new = newSHA1object(st)) == NULL) {
        if (have_string) {
            PyBuffer_Release(&buf);
        }
        return PyValue_NULL;
    }

    new->hash_state = Hacl_Hash_SHA1_malloc();

    if (new->hash_state == NULL) {
        Py_DECREF(new);
        if (have_string) {
            PyBuffer_Release(&buf);
        }
        return _PyValue_FromObjectResult(PyErr_NoMemory());
    }
    if (have_string) {
        /* Do not use self->mutex here as this is the constructor
         * where it is not yet possible to have concurrent access. */
        HASHLIB_EXTERNAL_INSTRUCTIONS_UNLOCKED(
            buf.len,
            update(new->hash_state, buf.buf, buf.len)
        );
        PyBuffer_Release(&buf);
    }

    return _PyValue_FromObjectResult((PyObject *)new);
}


/* List of functions exported by this module */

static struct PyMethodDef SHA1_functions[] = {
    _SHA1_SHA1_METHODDEF
    {NULL,      NULL}            /* Sentinel */
};

static int
_sha1_traverse(PyObject *module, visitproc visit, void *arg)
{
    SHA1State *state = sha1_get_state(module);
    Py_VISIT(state->sha1_type);
    return 0;
}

static int
_sha1_clear(PyObject *module)
{
    SHA1State *state = sha1_get_state(module);
    Py_CLEAR(state->sha1_type);
    return 0;
}

static void
_sha1_free(void *module)
{
    (void)_sha1_clear((PyObject *)module);
}

static int
_sha1_exec(PyObject *module)
{
    SHA1State* st = sha1_get_state(module);

    st->sha1_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &sha1_type_spec, NULL);
    if (PyModule_AddObjectRef(module,
                              "SHA1Type",
                              (PyObject *)st->sha1_type) < 0)
    {
        return -1;
    }
    if (PyModule_AddIntConstant(module,
                                "_GIL_MINSIZE",
                                HASHLIB_GIL_MINSIZE) < 0)
    {
        return -1;
    }

    return 0;
}


/* Initialize this module. */

static PyModuleDef_Slot _sha1_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, _sha1_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _sha1module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_sha1",
    .m_size = sizeof(SHA1State),
    .m_methods = SHA1_functions,
    .m_slots = _sha1_slots,
    .m_traverse = _sha1_traverse,
    .m_clear = _sha1_clear,
    .m_free = _sha1_free
};

PyMODINIT_FUNC
PyInit__sha1(void)
{
    return PyModuleDef_Init(&_sha1module);
}
