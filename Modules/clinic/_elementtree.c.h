/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(_elementtree_SubElement__doc__,
"SubElement($module, parent, tag, attrib={}, /, **extra)\n"
"--\n"
"\n"
"Create a new subelement of the parent element.");

#define _ELEMENTTREE_SUBELEMENT_METHODDEF    \
    {"SubElement", _PyCFunction_CAST(_elementtree_SubElement), METH_VARARGS|METH_KEYWORDS, _elementtree_SubElement__doc__},

static PyObject *
_elementtree_SubElement_impl(PyObject *module, PyObject *parent,
                             PyObject *tag, PyObject *attrib,
                             PyObject *extra);

static PyObject *
_elementtree_SubElement(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *parent;
    PyObject *tag;
    PyObject *attrib = NULL;
    PyObject *extra = NULL;

    if (!_PyArg_CheckPositional("SubElement", PyTuple_GET_SIZE(args), 2, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 0));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyObject_TypeCheck(_arg, get_elementtree_state(module)->Element_Type)) {
            _PyArg_BadArgument("SubElement", "argument 1", (get_elementtree_state(module)->Element_Type)->tp_name, _arg);
            goto exit;
        }
        parent = _arg;
    }
    tag = PyValue_Object(PyTuple_GET_VALUE(args, 1));
    if (tag == NULL) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(PyTuple_GET_VALUE(args, 2));
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyDict_Check(_arg)) {
            _PyArg_BadArgument("SubElement", "argument 3", "dict", _arg);
            goto exit;
        }
        attrib = _arg;
    }
skip_optional:
    if (kwargs == NULL) {
        extra = PyDict_New();
        if (extra == NULL) {
            goto exit;
        }
    }
    else {
        extra = Py_NewRef(kwargs);
    }
    return_value = _elementtree_SubElement_impl(module, parent, tag, attrib, extra);

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
    {"append", _PyCFunction_CAST(_elementtree_Element_append), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_append__doc__},

static PyObject *
_elementtree_Element_append_impl(ElementObject *self, PyTypeObject *cls,
                                 PyObject *subelement);

static PyValue
_elementtree_Element_append(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *subelement;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("append", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("append", nargs, 1, 1)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyObject_TypeCheck(_arg, clinic_state()->Element_Type)) {
            _PyArg_BadArgument("append", "argument 1", (clinic_state()->Element_Type)->tp_name, _arg);
            goto exit;
        }
        subelement = _arg;
    }
    _return_value = _elementtree_Element_append_impl((ElementObject *)self, cls, subelement);
    return_value = PyValue_FromResult(_return_value);

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
    {"clear", (PyCFunction)_elementtree_Element_clear, METH_NOARGS, _elementtree_Element_clear__doc__},

static PyObject *
_elementtree_Element_clear_impl(ElementObject *self);

static PyObject *
_elementtree_Element_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_clear_impl((ElementObject *)self);
}

PyDoc_STRVAR(_elementtree_Element___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(_elementtree_Element___copy__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element___copy____doc__},

static PyObject *
_elementtree_Element___copy___impl(ElementObject *self, PyTypeObject *cls);

static PyValue
_elementtree_Element___copy__(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        return PyValue_NULL;
    }
    return PyValue_FromResult(_elementtree_Element___copy___impl((ElementObject *)self, cls));
}

PyDoc_STRVAR(_elementtree_Element___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(_elementtree_Element___deepcopy__), METH_O, _elementtree_Element___deepcopy____doc__},

static PyObject *
_elementtree_Element___deepcopy___impl(ElementObject *self, PyObject *memo);

static PyValue
_elementtree_Element___deepcopy__(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *memo;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyDict_Check(_arg)) {
            _PyArg_BadArgument("__deepcopy__", "argument", "dict", _arg);
            goto exit;
        }
        memo = _arg;
    }
    _return_value = _elementtree_Element___deepcopy___impl((ElementObject *)self, memo);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_elementtree_Element___sizeof__, METH_NOARGS, _elementtree_Element___sizeof____doc__},

static size_t
_elementtree_Element___sizeof___impl(ElementObject *self);

static PyObject *
_elementtree_Element___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    size_t _return_value;

    _return_value = _elementtree_Element___sizeof___impl((ElementObject *)self);
    if ((_return_value == (size_t)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_Element___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___GETSTATE___METHODDEF    \
    {"__getstate__", (PyCFunction)_elementtree_Element___getstate__, METH_NOARGS, _elementtree_Element___getstate____doc__},

static PyObject *
_elementtree_Element___getstate___impl(ElementObject *self);

static PyObject *
_elementtree_Element___getstate__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element___getstate___impl((ElementObject *)self);
}

PyDoc_STRVAR(_elementtree_Element___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_ELEMENT___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(_elementtree_Element___setstate__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element___setstate____doc__},

static PyObject *
_elementtree_Element___setstate___impl(ElementObject *self,
                                       PyTypeObject *cls, PyObject *state);

static PyValue
_elementtree_Element___setstate__(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *state;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("__setstate__", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__setstate__", nargs, 1, 1)) {
        goto exit;
    }
    state = PyValue_Object(args[0]);
    if (state == NULL) {
        goto exit;
    }
    _return_value = _elementtree_Element___setstate___impl((ElementObject *)self, cls, state);
    return_value = PyValue_FromResult(_return_value);

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
    {"extend", _PyCFunction_CAST(_elementtree_Element_extend), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_extend__doc__},

static PyObject *
_elementtree_Element_extend_impl(ElementObject *self, PyTypeObject *cls,
                                 PyObject *elements);

static PyValue
_elementtree_Element_extend(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *elements;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("extend", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("extend", nargs, 1, 1)) {
        goto exit;
    }
    elements = PyValue_Object(args[0]);
    if (elements == NULL) {
        goto exit;
    }
    _return_value = _elementtree_Element_extend_impl((ElementObject *)self, cls, elements);
    return_value = PyValue_FromResult(_return_value);

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
    {"find", _PyCFunction_CAST(_elementtree_Element_find), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_find__doc__},

static PyObject *
_elementtree_Element_find_impl(ElementObject *self, PyTypeObject *cls,
                               PyObject *path, PyObject *namespaces);

static PyValue
_elementtree_Element_find(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), &_Py_ID(namespaces), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = PyValue_Object(args[0]);
    if (path == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = PyValue_Object(args[1]);
    if (namespaces == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _elementtree_Element_find_impl((ElementObject *)self, cls, path, namespaces);
    return_value = PyValue_FromResult(_return_value);

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
    {"findtext", _PyCFunction_CAST(_elementtree_Element_findtext), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_findtext__doc__},

static PyObject *
_elementtree_Element_findtext_impl(ElementObject *self, PyTypeObject *cls,
                                   PyObject *path, PyObject *default_value,
                                   PyObject *namespaces);

static PyValue
_elementtree_Element_findtext(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), &_Py_ID(default), &_Py_ID(namespaces), },
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
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *default_value = Py_None;
    PyObject *namespaces = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = PyValue_Object(args[0]);
    if (path == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        default_value = PyValue_Object(args[1]);
        if (default_value == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    namespaces = PyValue_Object(args[2]);
    if (namespaces == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _elementtree_Element_findtext_impl((ElementObject *)self, cls, path, default_value, namespaces);
    return_value = PyValue_FromResult(_return_value);

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
    {"findall", _PyCFunction_CAST(_elementtree_Element_findall), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_findall__doc__},

static PyObject *
_elementtree_Element_findall_impl(ElementObject *self, PyTypeObject *cls,
                                  PyObject *path, PyObject *namespaces);

static PyValue
_elementtree_Element_findall(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), &_Py_ID(namespaces), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = PyValue_Object(args[0]);
    if (path == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = PyValue_Object(args[1]);
    if (namespaces == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _elementtree_Element_findall_impl((ElementObject *)self, cls, path, namespaces);
    return_value = PyValue_FromResult(_return_value);

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
    {"iterfind", _PyCFunction_CAST(_elementtree_Element_iterfind), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_iterfind__doc__},

static PyObject *
_elementtree_Element_iterfind_impl(ElementObject *self, PyTypeObject *cls,
                                   PyObject *path, PyObject *namespaces);

static PyValue
_elementtree_Element_iterfind(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), &_Py_ID(namespaces), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *namespaces = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    path = PyValue_Object(args[0]);
    if (path == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    namespaces = PyValue_Object(args[1]);
    if (namespaces == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _elementtree_Element_iterfind_impl((ElementObject *)self, cls, path, namespaces);
    return_value = PyValue_FromResult(_return_value);

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
    {"get", _PyCFunction_CAST(_elementtree_Element_get), METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_get__doc__},

static PyObject *
_elementtree_Element_get_impl(ElementObject *self, PyObject *key,
                              PyObject *default_value);

static PyValue
_elementtree_Element_get(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(key), &_Py_ID(default), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *key;
    PyObject *default_value = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    key = PyValue_Object(args[0]);
    if (key == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    default_value = PyValue_Object(args[1]);
    if (default_value == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _elementtree_Element_get_impl((ElementObject *)self, key, default_value);
    return_value = PyValue_FromResult(_return_value);

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
    {"iter", _PyCFunction_CAST(_elementtree_Element_iter), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_iter__doc__},

static PyObject *
_elementtree_Element_iter_impl(ElementObject *self, PyTypeObject *cls,
                               PyObject *tag);

static PyValue
_elementtree_Element_iter(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(tag), },
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
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *tag = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tag = PyValue_Object(args[0]);
    if (tag == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _elementtree_Element_iter_impl((ElementObject *)self, cls, tag);
    return_value = PyValue_FromResult(_return_value);

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
    {"itertext", _PyCFunction_CAST(_elementtree_Element_itertext), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_itertext__doc__},

static PyObject *
_elementtree_Element_itertext_impl(ElementObject *self, PyTypeObject *cls);

static PyValue
_elementtree_Element_itertext(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "itertext() takes no arguments");
        return PyValue_NULL;
    }
    return PyValue_FromResult(_elementtree_Element_itertext_impl((ElementObject *)self, cls));
}

PyDoc_STRVAR(_elementtree_Element_insert__doc__,
"insert($self, index, subelement, /)\n"
"--\n"
"\n"
"Insert *subelement* at position *index*.");

#define _ELEMENTTREE_ELEMENT_INSERT_METHODDEF    \
    {"insert", _PyCFunction_CAST(_elementtree_Element_insert), METH_FASTCALL, _elementtree_Element_insert__doc__},

static PyObject *
_elementtree_Element_insert_impl(ElementObject *self, Py_ssize_t index,
                                 PyObject *subelement);

static PyValue
_elementtree_Element_insert(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index;
    PyObject *subelement;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyObject_TypeCheck(_arg, clinic_state()->Element_Type)) {
            _PyArg_BadArgument("insert", "argument 2", (clinic_state()->Element_Type)->tp_name, _arg);
            goto exit;
        }
        subelement = _arg;
    }
    _return_value = _elementtree_Element_insert_impl((ElementObject *)self, index, subelement);
    return_value = PyValue_FromResult(_return_value);

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
    {"items", (PyCFunction)_elementtree_Element_items, METH_NOARGS, _elementtree_Element_items__doc__},

static PyObject *
_elementtree_Element_items_impl(ElementObject *self);

static PyObject *
_elementtree_Element_items(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_items_impl((ElementObject *)self);
}

PyDoc_STRVAR(_elementtree_Element_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Get attribute names.\n"
"\n"
"Equivalent to attrib.keys()");

#define _ELEMENTTREE_ELEMENT_KEYS_METHODDEF    \
    {"keys", (PyCFunction)_elementtree_Element_keys, METH_NOARGS, _elementtree_Element_keys__doc__},

static PyObject *
_elementtree_Element_keys_impl(ElementObject *self);

static PyObject *
_elementtree_Element_keys(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_Element_keys_impl((ElementObject *)self);
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
    {"makeelement", _PyCFunction_CAST(_elementtree_Element_makeelement), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _elementtree_Element_makeelement__doc__},

static PyObject *
_elementtree_Element_makeelement_impl(ElementObject *self, PyTypeObject *cls,
                                      PyObject *tag, PyObject *attrib);

static PyValue
_elementtree_Element_makeelement(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *tag;
    PyObject *attrib;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("makeelement", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("makeelement", nargs, 2, 2)) {
        goto exit;
    }
    tag = PyValue_Object(args[0]);
    if (tag == NULL) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyDict_Check(_arg)) {
            _PyArg_BadArgument("makeelement", "argument 2", "dict", _arg);
            goto exit;
        }
        attrib = _arg;
    }
    _return_value = _elementtree_Element_makeelement_impl((ElementObject *)self, cls, tag, attrib);
    return_value = PyValue_FromResult(_return_value);

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
    {"remove", _PyCFunction_CAST(_elementtree_Element_remove), METH_O, _elementtree_Element_remove__doc__},

static PyObject *
_elementtree_Element_remove_impl(ElementObject *self, PyObject *subelement);

static PyValue
_elementtree_Element_remove(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *subelement;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyObject_TypeCheck(_arg, clinic_state()->Element_Type)) {
            _PyArg_BadArgument("remove", "argument", (clinic_state()->Element_Type)->tp_name, _arg);
            goto exit;
        }
        subelement = _arg;
    }
    _return_value = _elementtree_Element_remove_impl((ElementObject *)self, subelement);
    return_value = PyValue_FromResult(_return_value);

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
    {"set", _PyCFunction_CAST(_elementtree_Element_set), METH_FASTCALL, _elementtree_Element_set__doc__},

static PyObject *
_elementtree_Element_set_impl(ElementObject *self, PyObject *key,
                              PyObject *value);

static PyValue
_elementtree_Element_set(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *key;
    PyObject *value;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("set", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Object(args[0]);
    if (key == NULL) {
        goto exit;
    }
    value = PyValue_Object(args[1]);
    if (value == NULL) {
        goto exit;
    }
    _return_value = _elementtree_Element_set_impl((ElementObject *)self, key, value);
    return_value = PyValue_FromResult(_return_value);

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
                                       PyObject *element_factory,
                                       PyObject *comment_factory,
                                       PyObject *pi_factory,
                                       int insert_comments, int insert_pis);

static int
_elementtree_TreeBuilder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(element_factory), &_Py_ID(comment_factory), &_Py_ID(pi_factory), &_Py_ID(insert_comments), &_Py_ID(insert_pis), },
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
    PyValue argsbuf[5];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *element_factory = Py_None;
    PyObject *comment_factory = Py_None;
    PyObject *pi_factory = Py_None;
    int insert_comments = 0;
    int insert_pis = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(fastargs[0])) {
        element_factory = PyValue_Object(fastargs[0]);
        if (element_factory == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(fastargs[1])) {
        comment_factory = PyValue_Object(fastargs[1]);
        if (comment_factory == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[2])) {
        pi_factory = PyValue_Object(fastargs[2]);
        if (pi_factory == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(fastargs[3])) {
        insert_comments = PyObject_IsTrueValue(fastargs[3]);
        if (insert_comments < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    insert_pis = PyObject_IsTrueValue(fastargs[4]);
    if (insert_pis < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _elementtree_TreeBuilder___init___impl((TreeBuilderObject *)self, element_factory, comment_factory, pi_factory, insert_comments, insert_pis);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree__set_factories__doc__,
"_set_factories($module, comment_factory, pi_factory, /)\n"
"--\n"
"\n"
"Change the factories used to create comments and processing instructions.\n"
"\n"
"For internal use only.");

#define _ELEMENTTREE__SET_FACTORIES_METHODDEF    \
    {"_set_factories", _PyCFunction_CAST(_elementtree__set_factories), METH_FASTCALL, _elementtree__set_factories__doc__},

static PyObject *
_elementtree__set_factories_impl(PyObject *module, PyObject *comment_factory,
                                 PyObject *pi_factory);

static PyValue
_elementtree__set_factories(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *comment_factory;
    PyObject *pi_factory;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("_set_factories", nargs, 2, 2)) {
        goto exit;
    }
    comment_factory = PyValue_Object(args[0]);
    if (comment_factory == NULL) {
        goto exit;
    }
    pi_factory = PyValue_Object(args[1]);
    if (pi_factory == NULL) {
        goto exit;
    }
    _return_value = _elementtree__set_factories_impl(module, comment_factory, pi_factory);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_data__doc__,
"data($self, data, /)\n"
"--\n"
"\n"
"Add text to current element.");

#define _ELEMENTTREE_TREEBUILDER_DATA_METHODDEF    \
    {"data", _PyCFunction_CAST(_elementtree_TreeBuilder_data), METH_O, _elementtree_TreeBuilder_data__doc__},

static PyObject *
_elementtree_TreeBuilder_data_impl(TreeBuilderObject *self, PyObject *data);

static PyValue
_elementtree_TreeBuilder_data(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *data;
    PyObject *_return_value;

    data = PyValue_Object(arg);
    if (data == NULL) {
        goto exit;
    }
    _return_value = _elementtree_TreeBuilder_data_impl((TreeBuilderObject *)self, data);
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"end", _PyCFunction_CAST(_elementtree_TreeBuilder_end), METH_O, _elementtree_TreeBuilder_end__doc__},

static PyObject *
_elementtree_TreeBuilder_end_impl(TreeBuilderObject *self, PyObject *tag);

static PyValue
_elementtree_TreeBuilder_end(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *tag;
    PyObject *_return_value;

    tag = PyValue_Object(arg);
    if (tag == NULL) {
        goto exit;
    }
    _return_value = _elementtree_TreeBuilder_end_impl((TreeBuilderObject *)self, tag);
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"comment", _PyCFunction_CAST(_elementtree_TreeBuilder_comment), METH_O, _elementtree_TreeBuilder_comment__doc__},

static PyObject *
_elementtree_TreeBuilder_comment_impl(TreeBuilderObject *self,
                                      PyObject *text);

static PyValue
_elementtree_TreeBuilder_comment(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *text;
    PyObject *_return_value;

    text = PyValue_Object(arg);
    if (text == NULL) {
        goto exit;
    }
    _return_value = _elementtree_TreeBuilder_comment_impl((TreeBuilderObject *)self, text);
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"pi", _PyCFunction_CAST(_elementtree_TreeBuilder_pi), METH_FASTCALL, _elementtree_TreeBuilder_pi__doc__},

static PyObject *
_elementtree_TreeBuilder_pi_impl(TreeBuilderObject *self, PyObject *target,
                                 PyObject *text);

static PyValue
_elementtree_TreeBuilder_pi(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *target;
    PyObject *text = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("pi", nargs, 1, 2)) {
        goto exit;
    }
    target = PyValue_Object(args[0]);
    if (target == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    text = PyValue_Object(args[1]);
    if (text == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _elementtree_TreeBuilder_pi_impl((TreeBuilderObject *)self, target, text);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_TreeBuilder_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Flush builder buffers and return toplevel document Element.");

#define _ELEMENTTREE_TREEBUILDER_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_elementtree_TreeBuilder_close, METH_NOARGS, _elementtree_TreeBuilder_close__doc__},

static PyObject *
_elementtree_TreeBuilder_close_impl(TreeBuilderObject *self);

static PyObject *
_elementtree_TreeBuilder_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_TreeBuilder_close_impl((TreeBuilderObject *)self);
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
    {"start", _PyCFunction_CAST(_elementtree_TreeBuilder_start), METH_FASTCALL, _elementtree_TreeBuilder_start__doc__},

static PyObject *
_elementtree_TreeBuilder_start_impl(TreeBuilderObject *self, PyObject *tag,
                                    PyObject *attrs);

static PyValue
_elementtree_TreeBuilder_start(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *tag;
    PyObject *attrs;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("start", nargs, 2, 2)) {
        goto exit;
    }
    tag = PyValue_Object(args[0]);
    if (tag == NULL) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyDict_Check(_arg)) {
            _PyArg_BadArgument("start", "argument 2", "dict", _arg);
            goto exit;
        }
        attrs = _arg;
    }
    _return_value = _elementtree_TreeBuilder_start_impl((TreeBuilderObject *)self, tag, attrs);
    return_value = PyValue_FromResult(_return_value);

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
_elementtree_XMLParser___init___impl(XMLParserObject *self, PyObject *target,
                                     const char *encoding);

static int
_elementtree_XMLParser___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(target), &_Py_ID(encoding), },
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
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *target = Py_None;
    const char *encoding = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(fastargs[0])) {
        target = PyValue_Object(fastargs[0]);
        if (target == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    {
        PyObject *_arg = PyValue_Object(fastargs[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            encoding = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            encoding = _PyUnicode_AsUTF8NoNUL(_arg);
            if (encoding == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("XMLParser", "argument 'encoding'", "str or None", _arg);
            goto exit;
        }
    }
skip_optional_kwonly:
    return_value = _elementtree_XMLParser___init___impl((XMLParserObject *)self, target, encoding);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Finish feeding data to parser and return element structure.");

#define _ELEMENTTREE_XMLPARSER_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_elementtree_XMLParser_close, METH_NOARGS, _elementtree_XMLParser_close__doc__},

static PyObject *
_elementtree_XMLParser_close_impl(XMLParserObject *self);

static PyObject *
_elementtree_XMLParser_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_XMLParser_close_impl((XMLParserObject *)self);
}

PyDoc_STRVAR(_elementtree_XMLParser_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_elementtree_XMLParser_flush, METH_NOARGS, _elementtree_XMLParser_flush__doc__},

static PyObject *
_elementtree_XMLParser_flush_impl(XMLParserObject *self);

static PyObject *
_elementtree_XMLParser_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _elementtree_XMLParser_flush_impl((XMLParserObject *)self);
}

PyDoc_STRVAR(_elementtree_XMLParser_feed__doc__,
"feed($self, data, /)\n"
"--\n"
"\n"
"Feed encoded data to parser.");

#define _ELEMENTTREE_XMLPARSER_FEED_METHODDEF    \
    {"feed", _PyCFunction_CAST(_elementtree_XMLParser_feed), METH_O, _elementtree_XMLParser_feed__doc__},

static PyObject *
_elementtree_XMLParser_feed_impl(XMLParserObject *self, PyObject *data);

static PyValue
_elementtree_XMLParser_feed(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *data;
    PyObject *_return_value;

    data = PyValue_Object(arg);
    if (data == NULL) {
        goto exit;
    }
    _return_value = _elementtree_XMLParser_feed_impl((XMLParserObject *)self, data);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser__parse_whole__doc__,
"_parse_whole($self, file, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__PARSE_WHOLE_METHODDEF    \
    {"_parse_whole", _PyCFunction_CAST(_elementtree_XMLParser__parse_whole), METH_O, _elementtree_XMLParser__parse_whole__doc__},

static PyObject *
_elementtree_XMLParser__parse_whole_impl(XMLParserObject *self,
                                         PyObject *file);

static PyValue
_elementtree_XMLParser__parse_whole(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *file;
    PyObject *_return_value;

    file = PyValue_Object(arg);
    if (file == NULL) {
        goto exit;
    }
    _return_value = _elementtree_XMLParser__parse_whole_impl((XMLParserObject *)self, file);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_elementtree_XMLParser__setevents__doc__,
"_setevents($self, events_queue, events_to_report=None, /)\n"
"--\n"
"\n");

#define _ELEMENTTREE_XMLPARSER__SETEVENTS_METHODDEF    \
    {"_setevents", _PyCFunction_CAST(_elementtree_XMLParser__setevents), METH_FASTCALL, _elementtree_XMLParser__setevents__doc__},

static PyObject *
_elementtree_XMLParser__setevents_impl(XMLParserObject *self,
                                       PyObject *events_queue,
                                       PyObject *events_to_report);

static PyValue
_elementtree_XMLParser__setevents(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *events_queue;
    PyObject *events_to_report = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("_setevents", nargs, 1, 2)) {
        goto exit;
    }
    events_queue = PyValue_Object(args[0]);
    if (events_queue == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    events_to_report = PyValue_Object(args[1]);
    if (events_to_report == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _elementtree_XMLParser__setevents_impl((XMLParserObject *)self, events_queue, events_to_report);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=2d3908b28b18da3d input=a9049054013a1b77]*/
