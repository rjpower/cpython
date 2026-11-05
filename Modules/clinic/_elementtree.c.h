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
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_elementtree_SubElement__doc__,
"SubElement($module, parent, tag, attrib={}, /, **extra)\n"
"--\n"
"\n"
"Create a new subelement of the parent element.");

#define _ELEMENTTREE_SUBELEMENT_METHODDEF    \
    {"SubElement", _PyCFunction_CAST(_elementtree_SubElement), METH_VALUE, _elementtree_SubElement__doc__},

static PyValue
_elementtree_SubElement_impl(PyObject *module, PyObject *parent,
                             PyValueBorrow tag, PyObject *attrib,
                             PyObject *extra);

static PyValue
_elementtree_SubElement(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *parent;
    PyValueBorrow tag;
    PyObject *attrib = NULL;
    PyObject *extra = NULL;

    if (!_PyArg_CheckPositional("SubElement", nargs, 2, 3)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), get_elementtree_state(module)->Element_Type)) {
        _PyArg_BadArgumentValue("SubElement", "argument 1", (get_elementtree_state(module)->Element_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    parent = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    tag = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[2])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("SubElement", "argument 3", "dict", PyValue_Borrow(args[2]));
        goto exit;
    }
    attrib = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2]));
skip_optional:
    extra = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (extra == NULL) {
        goto exit;
    }
    return_value = _elementtree_SubElement_impl(PyValue_AsKnownObjectBorrow(module), parent, tag, attrib, extra);

exit:
    /* Cleanup for extra */
    Py_XDECREF(extra);

    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_append__doc__,
"append($self, subelement, /)\n"
"--\n"
"\n"
"Add *subelement* to the end of this element.\n"
"\n"
"The new element will appear in document order after the last\n"
"existing subelement (or directly after the text, if it\'s the first\n"
"subelement), but before the end tag for this element.");

#define _ELEMENTTREE_ELEMENT_APPEND_METHODDEF    \
    {"append", _PyCFunction_CAST(_elementtree_Element_append), METH_VALUE|METH_METHOD, _elementtree_Element_append__doc__},

static PyValue
_elementtree_Element_append_impl(ElementObject *self, PyTypeObject *cls,
                                 PyObject *subelement);

static PyValue
_elementtree_Element_append(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *subelement;

    if (!_PyArg_NoKwnames("append", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("append", nargs, 1, 1)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), clinic_state()->Element_Type)) {
        _PyArg_BadArgumentValue("append", "argument 1", (clinic_state()->Element_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    subelement = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _elementtree_Element_append_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, subelement);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Reset element.\n"
"\n"
"This function removes all subelements, clears all attributes, and\n"
"sets the text and tail attributes to None.");

#define _ELEMENTTREE_ELEMENT_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(_elementtree_Element_clear), METH_VALUE|METH_NOARGS, _elementtree_Element_clear__doc__},

static PyValue
_elementtree_Element_clear_impl(ElementObject *self);

static PyValue
_elementtree_Element_clear(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _elementtree_Element_clear_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(_elementtree_Element___copy__), METH_VALUE|METH_METHOD, _elementtree_Element___copy____doc__},

static PyValue
_elementtree_Element___copy___impl(ElementObject *self, PyTypeObject *cls);

static PyValue
_elementtree_Element___copy__(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        goto exit;
    }
    return_value = _elementtree_Element___copy___impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(_elementtree_Element___deepcopy__), METH_VALUE|METH_O, _elementtree_Element___deepcopy____doc__},

static PyValue
_elementtree_Element___deepcopy___impl(ElementObject *self, PyObject *memo);

static PyValue
_elementtree_Element___deepcopy__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *memo;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("__deepcopy__", "argument", "dict", PyValue_Borrow(args[0]));
        goto exit;
    }
    memo = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _elementtree_Element___deepcopy___impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), memo);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(_elementtree_Element___sizeof__), METH_VALUE|METH_NOARGS, _elementtree_Element___sizeof____doc__},

static size_t
_elementtree_Element___sizeof___impl(ElementObject *self);

static PyValue
_elementtree_Element___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    size_t _return_value;

    _return_value = _elementtree_Element___sizeof___impl((ElementObject *)PyValue_AsKnownObjectBorrow(self));
    if ((_return_value == (size_t)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = (uint64_t)_return_value <= INT64_MAX
        ? PyValue_FromInt64((int64_t)_return_value)
        : _PyValue_FromObjectResult(PyLong_FromUnsignedLongLong((unsigned long long)_return_value));

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___GETSTATE___METHODDEF    \
    {"__getstate__", _PyCFunction_CAST(_elementtree_Element___getstate__), METH_VALUE|METH_NOARGS, _elementtree_Element___getstate____doc__},

static PyValue
_elementtree_Element___getstate___impl(ElementObject *self);

static PyValue
_elementtree_Element___getstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _elementtree_Element___getstate___impl((ElementObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(_elementtree_Element___setstate__), METH_VALUE|METH_METHOD, _elementtree_Element___setstate____doc__},

static PyValue
_elementtree_Element___setstate___impl(ElementObject *self,
                                       PyTypeObject *cls,
                                       PyValueBorrow state);

static PyValue
_elementtree_Element___setstate__(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    if (!_PyArg_NoKwnames("__setstate__", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__setstate__", nargs, 1, 1)) {
        goto exit;
    }
    state = PyValue_Borrow(args[0]);
    return_value = _elementtree_Element___setstate___impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, state);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_extend__doc__,
"extend($self, elements, /)\n"
"--\n"
"\n"
"Append subelements from a sequence.\n"
"\n"
"*elements* is a sequence with zero or more elements.");

#define _ELEMENTTREE_ELEMENT_EXTEND_METHODDEF    \
    {"extend", _PyCFunction_CAST(_elementtree_Element_extend), METH_VALUE|METH_METHOD, _elementtree_Element_extend__doc__},

static PyValue
_elementtree_Element_extend_impl(ElementObject *self, PyTypeObject *cls,
                                 PyValueBorrow elements);

static PyValue
_elementtree_Element_extend(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow elements;

    if (!_PyArg_NoKwnames("extend", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("extend", nargs, 1, 1)) {
        goto exit;
    }
    elements = PyValue_Borrow(args[0]);
    return_value = _elementtree_Element_extend_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, elements);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_find__doc__,
"find($self, /, path, namespaces=None)\n"
"--\n"
"\n"
"Find first matching element by tag name or path.\n"
"\n"
"*path* is a string having either an element tag or an XPath,\n"
"*namespaces* is an optional mapping from namespace prefix to full\n"
"name.\n"
"\n"
"Return the first matching element, or None if no element was found.");

#define _ELEMENTTREE_ELEMENT_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(_elementtree_Element_find), METH_VALUE|METH_METHOD, _elementtree_Element_find__doc__},

static PyValue
_elementtree_Element_find_impl(ElementObject *self, PyTypeObject *cls,
                               PyValueBorrow path, PyValueBorrow namespaces);

static PyValue
_elementtree_Element_find(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(path)), _PyValue_STATIC_IMMORTAL(&_Py_ID(namespaces)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "find",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow path;
    PyValueBorrow namespaces = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    path = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        namespaces = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _elementtree_Element_find_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_findtext__doc__,
"findtext($self, /, path, default=None, namespaces=None)\n"
"--\n"
"\n"
"Find text for first matching element by tag name or path.\n"
"\n"
"*path* is a string having either an element tag or an XPath,\n"
"*default* is the value to return if the element was not found,\n"
"*namespaces* is an optional mapping from namespace prefix to full\n"
"name.\n"
"\n"
"Return text content of first matching element, or default value if\n"
"none was found. Note that if an element is found having no text\n"
"content, the empty string is returned.");

#define _ELEMENTTREE_ELEMENT_FINDTEXT_METHODDEF    \
    {"findtext", _PyCFunction_CAST(_elementtree_Element_findtext), METH_VALUE|METH_METHOD, _elementtree_Element_findtext__doc__},

static PyValue
_elementtree_Element_findtext_impl(ElementObject *self, PyTypeObject *cls,
                                   PyValueBorrow path,
                                   PyValueBorrow default_value,
                                   PyValueBorrow namespaces);

static PyValue
_elementtree_Element_findtext(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(path)), _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), _PyValue_STATIC_IMMORTAL(&_Py_ID(namespaces)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "default", "namespaces", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "findtext",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow path;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);
    PyValueBorrow namespaces = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    path = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        default_value = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        namespaces = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _elementtree_Element_findtext_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, path, default_value, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_findall__doc__,
"findall($self, /, path, namespaces=None)\n"
"--\n"
"\n"
"Find all matching subelements by tag name or path.\n"
"\n"
"*path* is a string having either an element tag or an XPath,\n"
"*namespaces* is an optional mapping from namespace prefix to full\n"
"name.\n"
"\n"
"Returns list containing all matching elements in document order.");

#define _ELEMENTTREE_ELEMENT_FINDALL_METHODDEF    \
    {"findall", _PyCFunction_CAST(_elementtree_Element_findall), METH_VALUE|METH_METHOD, _elementtree_Element_findall__doc__},

static PyValue
_elementtree_Element_findall_impl(ElementObject *self, PyTypeObject *cls,
                                  PyValueBorrow path,
                                  PyValueBorrow namespaces);

static PyValue
_elementtree_Element_findall(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(path)), _PyValue_STATIC_IMMORTAL(&_Py_ID(namespaces)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "findall",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow path;
    PyValueBorrow namespaces = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    path = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        namespaces = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _elementtree_Element_findall_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_iterfind__doc__,
"iterfind($self, /, path, namespaces=None)\n"
"--\n"
"\n"
"Find all matching subelements by tag name or path.\n"
"\n"
"*path* is a string having either an element tag or an XPath,\n"
"*namespaces* is an optional mapping from namespace prefix to full\n"
"name.\n"
"\n"
"Return an iterable yielding all matching elements in document order.");

#define _ELEMENTTREE_ELEMENT_ITERFIND_METHODDEF    \
    {"iterfind", _PyCFunction_CAST(_elementtree_Element_iterfind), METH_VALUE|METH_METHOD, _elementtree_Element_iterfind__doc__},

static PyValue
_elementtree_Element_iterfind_impl(ElementObject *self, PyTypeObject *cls,
                                   PyValueBorrow path,
                                   PyValueBorrow namespaces);

static PyValue
_elementtree_Element_iterfind(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(path)), _PyValue_STATIC_IMMORTAL(&_Py_ID(namespaces)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", "namespaces", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "iterfind",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow path;
    PyValueBorrow namespaces = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    path = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        namespaces = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _elementtree_Element_iterfind_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, path, namespaces);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_get__doc__,
"get($self, /, key, default=None)\n"
"--\n"
"\n"
"Get element attribute.\n"
"\n"
"Equivalent to attrib.get, but some implementations may handle this a\n"
"bit more efficiently. *key* is what attribute to look for, and\n"
"*default* is what to return if the attribute was not found.\n"
"\n"
"Returns a string containing the attribute value, or the default if\n"
"attribute was not found.");

#define _ELEMENTTREE_ELEMENT_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_elementtree_Element_get), METH_VALUE, _elementtree_Element_get__doc__},

static PyValue
_elementtree_Element_get_impl(ElementObject *self, PyValueBorrow key,
                              PyValueBorrow default_value);

static PyValue
_elementtree_Element_get(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        default_value = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _elementtree_Element_get_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_iter__doc__,
"iter($self, /, tag=None)\n"
"--\n"
"\n"
"Create tree iterator.\n"
"\n"
"The iterator loops over the element and all subelements in document\n"
"order, returning all elements with a matching tag.\n"
"\n"
"If the tree structure is modified during iteration, new or removed\n"
"elements may or may not be included. To get a stable set, use the\n"
"list() function on the iterator, and loop over the resulting list.\n"
"\n"
"*tag* is what tags to look for (default is to return all elements)\n"
"\n"
"Return an iterator containing all the matching elements.");

#define _ELEMENTTREE_ELEMENT_ITER_METHODDEF    \
    {"iter", _PyCFunction_CAST(_elementtree_Element_iter), METH_VALUE|METH_METHOD, _elementtree_Element_iter__doc__},

static PyValue
_elementtree_Element_iter_impl(ElementObject *self, PyTypeObject *cls,
                               PyValueBorrow tag);

static PyValue
_elementtree_Element_iter(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(tag)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tag", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "iter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow tag = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        tag = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = _elementtree_Element_iter_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, tag);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_itertext__doc__,
"itertext($self, /)\n"
"--\n"
"\n"
"Create text iterator.\n"
"\n"
"The iterator loops over the element and all subelements in document\n"
"order, returning all inner text.");

#define _ELEMENTTREE_ELEMENT_ITERTEXT_METHODDEF    \
    {"itertext", _PyCFunction_CAST(_elementtree_Element_itertext), METH_VALUE|METH_METHOD, _elementtree_Element_itertext__doc__},

static PyValue
_elementtree_Element_itertext_impl(ElementObject *self, PyTypeObject *cls);

static PyValue
_elementtree_Element_itertext(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "itertext() takes no arguments");
        goto exit;
    }
    return_value = _elementtree_Element_itertext_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_insert__doc__,
"insert($self, index, subelement, /)\n"
"--\n"
"\n"
"Insert *subelement* at position *index*.");

#define _ELEMENTTREE_ELEMENT_INSERT_METHODDEF    \
    {"insert", _PyCFunction_CAST(_elementtree_Element_insert), METH_VALUE, _elementtree_Element_insert__doc__},

static PyValue
_elementtree_Element_insert_impl(ElementObject *self, Py_ssize_t index,
                                 PyObject *subelement);

static PyValue
_elementtree_Element_insert(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index;
    PyObject *subelement;

    if (!_PyArg_NoKwnamesValue("Element.insert()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), clinic_state()->Element_Type)) {
        _PyArg_BadArgumentValue("insert", "argument 2", (clinic_state()->Element_Type)->tp_name, PyValue_Borrow(args[1]));
        goto exit;
    }
    subelement = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _elementtree_Element_insert_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), index, subelement);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_items__doc__,
"items($self, /)\n"
"--\n"
"\n"
"Get element attributes as (name, value) pairs.\n"
"\n"
"Equivalent to attrib.items().");

#define _ELEMENTTREE_ELEMENT_ITEMS_METHODDEF    \
    {"items", _PyCFunction_CAST(_elementtree_Element_items), METH_VALUE|METH_NOARGS, _elementtree_Element_items__doc__},

static PyValue
_elementtree_Element_items_impl(ElementObject *self);

static PyValue
_elementtree_Element_items(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _elementtree_Element_items_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Get attribute names.\n"
"\n"
"Equivalent to attrib.keys()");

#define _ELEMENTTREE_ELEMENT_KEYS_METHODDEF    \
    {"keys", _PyCFunction_CAST(_elementtree_Element_keys), METH_VALUE|METH_NOARGS, _elementtree_Element_keys__doc__},

static PyValue
_elementtree_Element_keys_impl(ElementObject *self);

static PyValue
_elementtree_Element_keys(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _elementtree_Element_keys_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_makeelement__doc__,
"makeelement($self, tag, attrib, /)\n"
"--\n"
"\n"
"Create a new element with the same type.\n"
"\n"
"*tag* is a string containing the element name. *attrib* is a\n"
"dictionary containing the element attributes.\n"
"\n"
"Do not call this method, use the SubElement factory function\n"
"instead.");

#define _ELEMENTTREE_ELEMENT_MAKEELEMENT_METHODDEF    \
    {"makeelement", _PyCFunction_CAST(_elementtree_Element_makeelement), METH_VALUE|METH_METHOD, _elementtree_Element_makeelement__doc__},

static PyValue
_elementtree_Element_makeelement_impl(ElementObject *self, PyTypeObject *cls,
                                      PyValueBorrow tag, PyObject *attrib);

static PyValue
_elementtree_Element_makeelement(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow tag;
    PyObject *attrib;

    if (!_PyArg_NoKwnames("makeelement", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("makeelement", nargs, 2, 2)) {
        goto exit;
    }
    tag = PyValue_Borrow(args[0]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("makeelement", "argument 2", "dict", PyValue_Borrow(args[1]));
        goto exit;
    }
    attrib = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _elementtree_Element_makeelement_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), cls, tag, attrib);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_remove__doc__,
"remove($self, subelement, /)\n"
"--\n"
"\n"
"Remove matching subelement.\n"
"\n"
"Unlike the find methods, this method compares elements based on\n"
"identity, NOT ON tag value or contents. To remove subelements by\n"
"other means, the easiest way is to use a list comprehension to\n"
"select what elements to keep, and then use slice assignment to\n"
"update the parent element.\n"
"\n"
"ValueError is raised if a matching element could not be found.");

#define _ELEMENTTREE_ELEMENT_REMOVE_METHODDEF    \
    {"remove", _PyCFunction_CAST(_elementtree_Element_remove), METH_VALUE|METH_O, _elementtree_Element_remove__doc__},

static PyValue
_elementtree_Element_remove_impl(ElementObject *self, PyObject *subelement);

static PyValue
_elementtree_Element_remove(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *subelement;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), clinic_state()->Element_Type)) {
        _PyArg_BadArgumentValue("remove", "argument", (clinic_state()->Element_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    subelement = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _elementtree_Element_remove_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), subelement);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element_set__doc__,
"set($self, key, value, /)\n"
"--\n"
"\n"
"Set element attribute.\n"
"\n"
"Equivalent to attrib[key] = value, but some implementations may\n"
"handle this a bit more efficiently. *key* is what attribute to set,\n"
"and *value* is the attribute value to set it to.");

#define _ELEMENTTREE_ELEMENT_SET_METHODDEF    \
    {"set", _PyCFunction_CAST(_elementtree_Element_set), METH_VALUE, _elementtree_Element_set__doc__},

static PyValue
_elementtree_Element_set_impl(ElementObject *self, PyValueBorrow key,
                              PyValueBorrow value);

static PyValue
_elementtree_Element_set(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow value;

    if (!_PyArg_NoKwnamesValue("Element.set()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    value = PyValue_Borrow(args[1]);
    return_value = _elementtree_Element_set_impl((ElementObject *)PyValue_AsKnownObjectBorrow(self), key, value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder___init____doc__,
"TreeBuilder(element_factory=None, *, comment_factory=None,\n"
"            pi_factory=None, insert_comments=False, insert_pis=False)\n"
"--\n"
"\n"
"Generic element structure builder.\n"
"\n"
"This builder converts a sequence of start, data, and end method\n"
"calls to a well-formed element structure.\n"
"\n"
"You can use this class to build an element structure using a custom\n"
"XML parser, or a parser for some other XML-like format.\n"
"\n"
"*element_factory* is an optional element factory which is called\n"
"to create new Element instances, as necessary.\n"
"\n"
"*comment_factory* is a factory to create comments to be used instead\n"
"of the standard factory.  If *insert_comments* is false (the\n"
"default), comments will not be inserted into the tree.\n"
"\n"
"*pi_factory* is a factory to create processing instructions to be\n"
"used instead of the standard factory.  If *insert_pis* is false (the\n"
"default), processing instructions will not be inserted into the tree.");

static int
_elementtree_TreeBuilder___init___impl(TreeBuilderObject *self,
                                       PyValueBorrow element_factory,
                                       PyValueBorrow comment_factory,
                                       PyValueBorrow pi_factory,
                                       int insert_comments, int insert_pis);

static int
_elementtree_TreeBuilder___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(element_factory)), _PyValue_STATIC_IMMORTAL(&_Py_ID(comment_factory)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pi_factory)), _PyValue_STATIC_IMMORTAL(&_Py_ID(insert_comments)), _PyValue_STATIC_IMMORTAL(&_Py_ID(insert_pis)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"element_factory", "comment_factory", "pi_factory", "insert_comments", "insert_pis", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "TreeBuilder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow element_factory = PyValue_Borrow(PyValue_None);
    PyValueBorrow comment_factory = PyValue_Borrow(PyValue_None);
    PyValueBorrow pi_factory = PyValue_Borrow(PyValue_None);
    int insert_comments = 0;
    int insert_pis = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        element_factory = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        comment_factory = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        pi_factory = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        insert_comments = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (insert_comments < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        insert_pis = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (insert_pis < 0) {
            goto exit;
        }
    }
    return_value = _elementtree_TreeBuilder___init___impl((TreeBuilderObject *)self, element_factory, comment_factory, pi_factory, insert_comments, insert_pis);

exit:
    return return_value;
}

static int
_elementtree_TreeBuilder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _elementtree_TreeBuilder___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _elementtree_TreeBuilder___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_elementtree__set_factories__doc__,
"_set_factories($module, comment_factory, pi_factory, /)\n"
"--\n"
"\n"
"Change the factories used to create comments and processing instructions.\n"
"\n"
"For internal use only.");

#define _ELEMENTTREE__SET_FACTORIES_METHODDEF    \
    {"_set_factories", _PyCFunction_CAST(_elementtree__set_factories), METH_VALUE, _elementtree__set_factories__doc__},

static PyValue
_elementtree__set_factories_impl(PyObject *module,
                                 PyValueBorrow comment_factory,
                                 PyValueBorrow pi_factory);

static PyValue
_elementtree__set_factories(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow comment_factory;
    PyValueBorrow pi_factory;

    if (!_PyArg_NoKwnamesValue("_elementtree._set_factories()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_set_factories", nargs, 2, 2)) {
        goto exit;
    }
    comment_factory = PyValue_Borrow(args[0]);
    pi_factory = PyValue_Borrow(args[1]);
    return_value = _elementtree__set_factories_impl(PyValue_AsKnownObjectBorrow(module), comment_factory, pi_factory);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_data__doc__,
"data($self, data, /)\n"
"--\n"
"\n"
"Add text to current element.");

#define _ELEMENTTREE_TREEBUILDER_DATA_METHODDEF    \
    {"data", _PyCFunction_CAST(_elementtree_TreeBuilder_data), METH_VALUE|METH_O, _elementtree_TreeBuilder_data__doc__},

static PyValue
_elementtree_TreeBuilder_data_impl(TreeBuilderObject *self,
                                   PyValueBorrow data);

static PyValue
_elementtree_TreeBuilder_data(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow data;

    data = PyValue_Borrow(args[0]);
    return_value = _elementtree_TreeBuilder_data_impl((TreeBuilderObject *)PyValue_AsKnownObjectBorrow(self), data);

    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_end__doc__,
"end($self, tag, /)\n"
"--\n"
"\n"
"Close and return current Element.\n"
"\n"
"*tag* is the element name.");

#define _ELEMENTTREE_TREEBUILDER_END_METHODDEF    \
    {"end", _PyCFunction_CAST(_elementtree_TreeBuilder_end), METH_VALUE|METH_O, _elementtree_TreeBuilder_end__doc__},

static PyValue
_elementtree_TreeBuilder_end_impl(TreeBuilderObject *self, PyValueBorrow tag);

static PyValue
_elementtree_TreeBuilder_end(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow tag;

    tag = PyValue_Borrow(args[0]);
    return_value = _elementtree_TreeBuilder_end_impl((TreeBuilderObject *)PyValue_AsKnownObjectBorrow(self), tag);

    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_comment__doc__,
"comment($self, text, /)\n"
"--\n"
"\n"
"Create a comment using the comment_factory.\n"
"\n"
"*text* is the text of the comment.");

#define _ELEMENTTREE_TREEBUILDER_COMMENT_METHODDEF    \
    {"comment", _PyCFunction_CAST(_elementtree_TreeBuilder_comment), METH_VALUE|METH_O, _elementtree_TreeBuilder_comment__doc__},

static PyValue
_elementtree_TreeBuilder_comment_impl(TreeBuilderObject *self,
                                      PyValueBorrow text);

static PyValue
_elementtree_TreeBuilder_comment(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow text;

    text = PyValue_Borrow(args[0]);
    return_value = _elementtree_TreeBuilder_comment_impl((TreeBuilderObject *)PyValue_AsKnownObjectBorrow(self), text);

    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_pi__doc__,
"pi($self, target, text=None, /)\n"
"--\n"
"\n"
"Create a processing instruction using the pi_factory.\n"
"\n"
"*target* is the target name of the processing instruction. *text* is\n"
"the data of the processing instruction, or \'\'.");

#define _ELEMENTTREE_TREEBUILDER_PI_METHODDEF    \
    {"pi", _PyCFunction_CAST(_elementtree_TreeBuilder_pi), METH_VALUE, _elementtree_TreeBuilder_pi__doc__},

static PyValue
_elementtree_TreeBuilder_pi_impl(TreeBuilderObject *self,
                                 PyValueBorrow target, PyValueBorrow text);

static PyValue
_elementtree_TreeBuilder_pi(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow target;
    PyValueBorrow text = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("TreeBuilder.pi()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("pi", nargs, 1, 2)) {
        goto exit;
    }
    target = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    text = PyValue_Borrow(args[1]);
skip_optional:
    return_value = _elementtree_TreeBuilder_pi_impl((TreeBuilderObject *)PyValue_AsKnownObjectBorrow(self), target, text);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Flush builder buffers and return toplevel document Element.");

#define _ELEMENTTREE_TREEBUILDER_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_elementtree_TreeBuilder_close), METH_VALUE|METH_NOARGS, _elementtree_TreeBuilder_close__doc__},

static PyValue
_elementtree_TreeBuilder_close_impl(TreeBuilderObject *self);

static PyValue
_elementtree_TreeBuilder_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _elementtree_TreeBuilder_close_impl((TreeBuilderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_start__doc__,
"start($self, tag, attrs, /)\n"
"--\n"
"\n"
"Open new element and return it.\n"
"\n"
"*tag* is the element name, *attrs* is a dict containing element\n"
"attributes.");

#define _ELEMENTTREE_TREEBUILDER_START_METHODDEF    \
    {"start", _PyCFunction_CAST(_elementtree_TreeBuilder_start), METH_VALUE, _elementtree_TreeBuilder_start__doc__},

static PyValue
_elementtree_TreeBuilder_start_impl(TreeBuilderObject *self,
                                    PyValueBorrow tag, PyObject *attrs);

static PyValue
_elementtree_TreeBuilder_start(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow tag;
    PyObject *attrs;

    if (!_PyArg_NoKwnamesValue("TreeBuilder.start()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("start", nargs, 2, 2)) {
        goto exit;
    }
    tag = PyValue_Borrow(args[0]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("start", "argument 2", "dict", PyValue_Borrow(args[1]));
        goto exit;
    }
    attrs = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _elementtree_TreeBuilder_start_impl((TreeBuilderObject *)PyValue_AsKnownObjectBorrow(self), tag, attrs);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser___init____doc__,
"XMLParser(*, target=None, encoding=None)\n"
"--\n"
"\n"
"Element structure builder for XML source data based on the expat parser.\n"
"\n"
"*target* is an optional target object which defaults to an instance\n"
"of the standard TreeBuilder class, *encoding* is an optional encoding\n"
"string which if given, overrides the encoding specified in the XML\n"
"file: http://www.iana.org/assignments/character-sets");

static int
_elementtree_XMLParser___init___impl(XMLParserObject *self,
                                     PyValueBorrow target,
                                     const char *encoding);

static int
_elementtree_XMLParser___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(target)), _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"target", "encoding", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "XMLParser",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow target = PyValue_Borrow(PyValue_None);
    const char *encoding = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        target = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("XMLParser", "argument 'encoding'", "str or None", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])) == Py_None) {
            encoding = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            if (encoding == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("XMLParser", "argument 'encoding'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
    }
    return_value = _elementtree_XMLParser___init___impl((XMLParserObject *)self, target, encoding);

exit:
    return return_value;
}

static int
_elementtree_XMLParser___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _elementtree_XMLParser___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _elementtree_XMLParser___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_elementtree_XMLParser_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Finish feeding data to parser and return element structure.");

#define _ELEMENTTREE_XMLPARSER_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_elementtree_XMLParser_close), METH_VALUE|METH_NOARGS, _elementtree_XMLParser_close__doc__},

static PyValue
_elementtree_XMLParser_close_impl(XMLParserObject *self);

static PyValue
_elementtree_XMLParser_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _elementtree_XMLParser_close_impl((XMLParserObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_elementtree_XMLParser_flush), METH_VALUE|METH_NOARGS, _elementtree_XMLParser_flush__doc__},

static PyValue
_elementtree_XMLParser_flush_impl(XMLParserObject *self);

static PyValue
_elementtree_XMLParser_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _elementtree_XMLParser_flush_impl((XMLParserObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser_feed__doc__,
"feed($self, data, /)\n"
"--\n"
"\n"
"Feed encoded data to parser.");

#define _ELEMENTTREE_XMLPARSER_FEED_METHODDEF    \
    {"feed", _PyCFunction_CAST(_elementtree_XMLParser_feed), METH_VALUE|METH_O, _elementtree_XMLParser_feed__doc__},

static PyValue
_elementtree_XMLParser_feed_impl(XMLParserObject *self, PyValueBorrow data);

static PyValue
_elementtree_XMLParser_feed(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow data;

    data = PyValue_Borrow(args[0]);
    return_value = _elementtree_XMLParser_feed_impl((XMLParserObject *)PyValue_AsKnownObjectBorrow(self), data);

    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser__parse_whole__doc__,
"_parse_whole($self, file, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__PARSE_WHOLE_METHODDEF    \
    {"_parse_whole", _PyCFunction_CAST(_elementtree_XMLParser__parse_whole), METH_VALUE|METH_O, _elementtree_XMLParser__parse_whole__doc__},

static PyValue
_elementtree_XMLParser__parse_whole_impl(XMLParserObject *self,
                                         PyValueBorrow file);

static PyValue
_elementtree_XMLParser__parse_whole(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow file;

    file = PyValue_Borrow(args[0]);
    return_value = _elementtree_XMLParser__parse_whole_impl((XMLParserObject *)PyValue_AsKnownObjectBorrow(self), file);

    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser__setevents__doc__,
"_setevents($self, events_queue, events_to_report=None, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__SETEVENTS_METHODDEF    \
    {"_setevents", _PyCFunction_CAST(_elementtree_XMLParser__setevents), METH_VALUE, _elementtree_XMLParser__setevents__doc__},

static PyValue
_elementtree_XMLParser__setevents_impl(XMLParserObject *self,
                                       PyValueBorrow events_queue,
                                       PyValueBorrow events_to_report);

static PyValue
_elementtree_XMLParser__setevents(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow events_queue;
    PyValueBorrow events_to_report = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("XMLParser._setevents()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_setevents", nargs, 1, 2)) {
        goto exit;
    }
    events_queue = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    events_to_report = PyValue_Borrow(args[1]);
skip_optional:
    return_value = _elementtree_XMLParser__setevents_impl((XMLParserObject *)PyValue_AsKnownObjectBorrow(self), events_queue, events_to_report);

exit:
    return return_value;
}
/*[clinic end generated code: output=b2a6d6261db2106b input=a9049054013a1b77]*/
