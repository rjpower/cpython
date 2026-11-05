/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_locale_setlocale__doc__,
"setlocale($module, category, locale=None, /)\n"
"--\n"
"\n"
"Activates/queries locale processing.");

#define _LOCALE_SETLOCALE_METHODDEF    \
    {"setlocale", _PyCFunction_CAST(_locale_setlocale), METH_VALUE, _locale_setlocale__doc__},

static PyValue
_locale_setlocale_impl(PyObject *module, int category, const char *locale);

static PyValue
_locale_setlocale(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int category;
    const char *locale = NULL;

    if (!_PyArg_CheckPositional("setlocale", nargs, 1, 2)) {
        goto exit;
    }
    category = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("setlocale", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        locale = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        locale = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (locale == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("setlocale", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _locale_setlocale_impl(PyValue_AsKnownObjectBorrow(module), category, locale);

exit:
    return return_value;
}

PyDoc_STRVAR(_locale_localeconv__doc__,
"localeconv($module, /)\n"
"--\n"
"\n"
"Returns numeric and monetary locale-specific parameters.");

#define _LOCALE_LOCALECONV_METHODDEF    \
    {"localeconv", _PyCFunction_CAST(_locale_localeconv), METH_VALUE|METH_NOARGS, _locale_localeconv__doc__},

static PyValue
_locale_localeconv_impl(PyObject *module);

static PyValue
_locale_localeconv(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _locale_localeconv_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(HAVE_WCSCOLL)

PyDoc_STRVAR(_locale_strcoll__doc__,
"strcoll($module, os1, os2, /)\n"
"--\n"
"\n"
"Compares two strings according to the locale.");

#define _LOCALE_STRCOLL_METHODDEF    \
    {"strcoll", _PyCFunction_CAST(_locale_strcoll), METH_VALUE, _locale_strcoll__doc__},

static PyValue
_locale_strcoll_impl(PyObject *module, PyObject *os1, PyObject *os2);

static PyValue
_locale_strcoll(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *os1;
    PyObject *os2;

    if (!_PyArg_CheckPositional("strcoll", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("strcoll", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("strcoll", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    os1 = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("strcoll", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("strcoll", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    os2 = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = _locale_strcoll_impl(PyValue_AsKnownObjectBorrow(module), os1, os2);

exit:
    return return_value;
}

#endif /* defined(HAVE_WCSCOLL) */

#if defined(HAVE_WCSXFRM)

PyDoc_STRVAR(_locale_strxfrm__doc__,
"strxfrm($module, string, /)\n"
"--\n"
"\n"
"Return a string that can be used as a key for locale-aware comparisons.");

#define _LOCALE_STRXFRM_METHODDEF    \
    {"strxfrm", _PyCFunction_CAST(_locale_strxfrm), METH_VALUE|METH_O, _locale_strxfrm__doc__},

static PyValue
_locale_strxfrm_impl(PyObject *module, PyObject *str);

static PyValue
_locale_strxfrm(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("strxfrm", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("strxfrm", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _locale_strxfrm_impl(PyValue_AsKnownObjectBorrow(module), str);

exit:
    return return_value;
}

#endif /* defined(HAVE_WCSXFRM) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_locale__getdefaultlocale__doc__,
"_getdefaultlocale($module, /)\n"
"--\n"
"\n");

#define _LOCALE__GETDEFAULTLOCALE_METHODDEF    \
    {"_getdefaultlocale", _PyCFunction_CAST(_locale__getdefaultlocale), METH_VALUE|METH_NOARGS, _locale__getdefaultlocale__doc__},

static PyValue
_locale__getdefaultlocale_impl(PyObject *module);

static PyValue
_locale__getdefaultlocale(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _locale__getdefaultlocale_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(HAVE_LANGINFO_H)

PyDoc_STRVAR(_locale_nl_langinfo__doc__,
"nl_langinfo($module, key, /)\n"
"--\n"
"\n"
"Return the value for the locale information associated with key.");

#define _LOCALE_NL_LANGINFO_METHODDEF    \
    {"nl_langinfo", _PyCFunction_CAST(_locale_nl_langinfo), METH_VALUE|METH_O, _locale_nl_langinfo__doc__},

static PyValue
_locale_nl_langinfo_impl(PyObject *module, int item);

static PyValue
_locale_nl_langinfo(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int item;

    item = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (item == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _locale_nl_langinfo_impl(PyValue_AsKnownObjectBorrow(module), item);

exit:
    return return_value;
}

#endif /* defined(HAVE_LANGINFO_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_gettext__doc__,
"gettext($module, msg, /)\n"
"--\n"
"\n"
"gettext(msg) -> string\n"
"\n"
"Return translation of msg.");

#define _LOCALE_GETTEXT_METHODDEF    \
    {"gettext", _PyCFunction_CAST(_locale_gettext), METH_VALUE|METH_O, _locale_gettext__doc__},

static PyValue
_locale_gettext_impl(PyObject *module, const char *in);

static PyValue
_locale_gettext(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *in;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("gettext", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("gettext", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    in = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (in == NULL) {
        goto exit;
    }
    return_value = _locale_gettext_impl(PyValue_AsKnownObjectBorrow(module), in);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_dgettext__doc__,
"dgettext($module, domain, msg, /)\n"
"--\n"
"\n"
"dgettext(domain, msg) -> string\n"
"\n"
"Return translation of msg in domain.");

#define _LOCALE_DGETTEXT_METHODDEF    \
    {"dgettext", _PyCFunction_CAST(_locale_dgettext), METH_VALUE, _locale_dgettext__doc__},

static PyValue
_locale_dgettext_impl(PyObject *module, const char *domain, const char *in);

static PyValue
_locale_dgettext(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    const char *in;

    if (!_PyArg_CheckPositional("dgettext", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("dgettext", "argument 1", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        domain = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        domain = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (domain == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("dgettext", "argument 1", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("dgettext", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("dgettext", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    in = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (in == NULL) {
        goto exit;
    }
    return_value = _locale_dgettext_impl(PyValue_AsKnownObjectBorrow(module), domain, in);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_dcgettext__doc__,
"dcgettext($module, domain, msg, category, /)\n"
"--\n"
"\n"
"Return translation of msg in domain and category.");

#define _LOCALE_DCGETTEXT_METHODDEF    \
    {"dcgettext", _PyCFunction_CAST(_locale_dcgettext), METH_VALUE, _locale_dcgettext__doc__},

static PyValue
_locale_dcgettext_impl(PyObject *module, const char *domain,
                       const char *msgid, int category);

static PyValue
_locale_dcgettext(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    const char *msgid;
    int category;

    if (!_PyArg_CheckPositional("dcgettext", nargs, 3, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("dcgettext", "argument 1", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        domain = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        domain = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (domain == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("dcgettext", "argument 1", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("dcgettext", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("dcgettext", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    msgid = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (msgid == NULL) {
        goto exit;
    }
    category = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _locale_dcgettext_impl(PyValue_AsKnownObjectBorrow(module), domain, msgid, category);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_textdomain__doc__,
"textdomain($module, domain, /)\n"
"--\n"
"\n"
"Set the C library\'s textdmain to domain, returning the new domain.");

#define _LOCALE_TEXTDOMAIN_METHODDEF    \
    {"textdomain", _PyCFunction_CAST(_locale_textdomain), METH_VALUE|METH_O, _locale_textdomain__doc__},

static PyValue
_locale_textdomain_impl(PyObject *module, const char *domain);

static PyValue
_locale_textdomain(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("textdomain", "argument", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        domain = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        domain = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (domain == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("textdomain", "argument", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    return_value = _locale_textdomain_impl(PyValue_AsKnownObjectBorrow(module), domain);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_bindtextdomain__doc__,
"bindtextdomain($module, domain, dir, /)\n"
"--\n"
"\n"
"Bind the C library\'s domain to dir.");

#define _LOCALE_BINDTEXTDOMAIN_METHODDEF    \
    {"bindtextdomain", _PyCFunction_CAST(_locale_bindtextdomain), METH_VALUE, _locale_bindtextdomain__doc__},

static PyValue
_locale_bindtextdomain_impl(PyObject *module, const char *domain,
                            PyValueBorrow dirname_obj);

static PyValue
_locale_bindtextdomain(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    PyValueBorrow dirname_obj;

    if (!_PyArg_CheckPositional("bindtextdomain", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("bindtextdomain", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("bindtextdomain", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    domain = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (domain == NULL) {
        goto exit;
    }
    dirname_obj = PyValue_Borrow(args[1]);
    return_value = _locale_bindtextdomain_impl(PyValue_AsKnownObjectBorrow(module), domain, dirname_obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H) && defined(HAVE_BIND_TEXTDOMAIN_CODESET)

PyDoc_STRVAR(_locale_bind_textdomain_codeset__doc__,
"bind_textdomain_codeset($module, domain, codeset, /)\n"
"--\n"
"\n"
"Bind the C library\'s domain to codeset.");

#define _LOCALE_BIND_TEXTDOMAIN_CODESET_METHODDEF    \
    {"bind_textdomain_codeset", _PyCFunction_CAST(_locale_bind_textdomain_codeset), METH_VALUE, _locale_bind_textdomain_codeset__doc__},

static PyValue
_locale_bind_textdomain_codeset_impl(PyObject *module, const char *domain,
                                     const char *codeset);

static PyValue
_locale_bind_textdomain_codeset(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    const char *codeset;

    if (!_PyArg_CheckPositional("bind_textdomain_codeset", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("bind_textdomain_codeset", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("bind_textdomain_codeset", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    domain = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (domain == NULL) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("bind_textdomain_codeset", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        codeset = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        codeset = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (codeset == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("bind_textdomain_codeset", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    return_value = _locale_bind_textdomain_codeset_impl(PyValue_AsKnownObjectBorrow(module), domain, codeset);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) && defined(HAVE_BIND_TEXTDOMAIN_CODESET) */

PyDoc_STRVAR(_locale_getencoding__doc__,
"getencoding($module, /)\n"
"--\n"
"\n"
"Get the current locale encoding.");

#define _LOCALE_GETENCODING_METHODDEF    \
    {"getencoding", _PyCFunction_CAST(_locale_getencoding), METH_VALUE|METH_NOARGS, _locale_getencoding__doc__},

static PyValue
_locale_getencoding_impl(PyObject *module);

static PyValue
_locale_getencoding(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _locale_getencoding_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#ifndef _LOCALE_STRCOLL_METHODDEF
    #define _LOCALE_STRCOLL_METHODDEF
#endif /* !defined(_LOCALE_STRCOLL_METHODDEF) */

#ifndef _LOCALE_STRXFRM_METHODDEF
    #define _LOCALE_STRXFRM_METHODDEF
#endif /* !defined(_LOCALE_STRXFRM_METHODDEF) */

#ifndef _LOCALE__GETDEFAULTLOCALE_METHODDEF
    #define _LOCALE__GETDEFAULTLOCALE_METHODDEF
#endif /* !defined(_LOCALE__GETDEFAULTLOCALE_METHODDEF) */

#ifndef _LOCALE_NL_LANGINFO_METHODDEF
    #define _LOCALE_NL_LANGINFO_METHODDEF
#endif /* !defined(_LOCALE_NL_LANGINFO_METHODDEF) */

#ifndef _LOCALE_GETTEXT_METHODDEF
    #define _LOCALE_GETTEXT_METHODDEF
#endif /* !defined(_LOCALE_GETTEXT_METHODDEF) */

#ifndef _LOCALE_DGETTEXT_METHODDEF
    #define _LOCALE_DGETTEXT_METHODDEF
#endif /* !defined(_LOCALE_DGETTEXT_METHODDEF) */

#ifndef _LOCALE_DCGETTEXT_METHODDEF
    #define _LOCALE_DCGETTEXT_METHODDEF
#endif /* !defined(_LOCALE_DCGETTEXT_METHODDEF) */

#ifndef _LOCALE_TEXTDOMAIN_METHODDEF
    #define _LOCALE_TEXTDOMAIN_METHODDEF
#endif /* !defined(_LOCALE_TEXTDOMAIN_METHODDEF) */

#ifndef _LOCALE_BINDTEXTDOMAIN_METHODDEF
    #define _LOCALE_BINDTEXTDOMAIN_METHODDEF
#endif /* !defined(_LOCALE_BINDTEXTDOMAIN_METHODDEF) */

#ifndef _LOCALE_BIND_TEXTDOMAIN_CODESET_METHODDEF
    #define _LOCALE_BIND_TEXTDOMAIN_CODESET_METHODDEF
#endif /* !defined(_LOCALE_BIND_TEXTDOMAIN_CODESET_METHODDEF) */
/*[clinic end generated code: output=faac59eebb0f9fca input=a9049054013a1b77]*/
