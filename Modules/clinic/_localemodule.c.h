/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(_locale_setlocale__doc__,
"setlocale($module, category, locale=None, /)\n"
"--\n"
"\n"
"Activates/queries locale processing.");

#define _LOCALE_SETLOCALE_METHODDEF    \
    {"setlocale", _PyCFunction_CAST(_locale_setlocale), METH_FASTCALL, _locale_setlocale__doc__},

static PyObject *
_locale_setlocale_impl(PyObject *module, int category, const char *locale);

static PyValue
_locale_setlocale(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int category;
    const char *locale = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("setlocale", nargs, 1, 2)) {
        goto exit;
    }
    category = PyLong_AsIntValue(args[0]);
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            locale = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            locale = _PyUnicode_AsUTF8NoNUL(_arg);
            if (locale == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("setlocale", "argument 2", "str or None", _arg);
            goto exit;
        }
    }
skip_optional:
    _return_value = _locale_setlocale_impl(module, category, locale);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_locale_localeconv__doc__,
"localeconv($module, /)\n"
"--\n"
"\n"
"Returns numeric and monetary locale-specific parameters.");

#define _LOCALE_LOCALECONV_METHODDEF    \
    {"localeconv", (PyCFunction)_locale_localeconv, METH_NOARGS, _locale_localeconv__doc__},

static PyObject *
_locale_localeconv_impl(PyObject *module);

static PyObject *
_locale_localeconv(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _locale_localeconv_impl(module);
}

#if defined(HAVE_WCSCOLL)

PyDoc_STRVAR(_locale_strcoll__doc__,
"strcoll($module, os1, os2, /)\n"
"--\n"
"\n"
"Compares two strings according to the locale.");

#define _LOCALE_STRCOLL_METHODDEF    \
    {"strcoll", _PyCFunction_CAST(_locale_strcoll), METH_FASTCALL, _locale_strcoll__doc__},

static PyObject *
_locale_strcoll_impl(PyObject *module, PyObject *os1, PyObject *os2);

static PyValue
_locale_strcoll(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *os1;
    PyObject *os2;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("strcoll", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("strcoll", "argument 1", "str", _arg);
            goto exit;
        }
        os1 = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("strcoll", "argument 2", "str", _arg);
            goto exit;
        }
        os2 = _arg;
    }
    _return_value = _locale_strcoll_impl(module, os1, os2);
    return_value = PyValue_FromResult(_return_value);

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
    {"strxfrm", _PyCFunction_CAST(_locale_strxfrm), METH_O, _locale_strxfrm__doc__},

static PyObject *
_locale_strxfrm_impl(PyObject *module, PyObject *str);

static PyValue
_locale_strxfrm(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("strxfrm", "argument", "str", _arg);
            goto exit;
        }
        str = _arg;
    }
    _return_value = _locale_strxfrm_impl(module, str);
    return_value = PyValue_FromResult(_return_value);

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
    {"_getdefaultlocale", (PyCFunction)_locale__getdefaultlocale, METH_NOARGS, _locale__getdefaultlocale__doc__},

static PyObject *
_locale__getdefaultlocale_impl(PyObject *module);

static PyObject *
_locale__getdefaultlocale(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _locale__getdefaultlocale_impl(module);
}

#endif /* defined(MS_WINDOWS) */

#if defined(HAVE_LANGINFO_H)

PyDoc_STRVAR(_locale_nl_langinfo__doc__,
"nl_langinfo($module, key, /)\n"
"--\n"
"\n"
"Return the value for the locale information associated with key.");

#define _LOCALE_NL_LANGINFO_METHODDEF    \
    {"nl_langinfo", _PyCFunction_CAST(_locale_nl_langinfo), METH_O, _locale_nl_langinfo__doc__},

static PyObject *
_locale_nl_langinfo_impl(PyObject *module, int item);

static PyValue
_locale_nl_langinfo(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int item;
    PyObject *_return_value;

    item = PyLong_AsIntValue(arg);
    if (item == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _locale_nl_langinfo_impl(module, item);
    return_value = PyValue_FromResult(_return_value);

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
    {"gettext", _PyCFunction_CAST(_locale_gettext), METH_O, _locale_gettext__doc__},

static PyObject *
_locale_gettext_impl(PyObject *module, const char *in);

static PyValue
_locale_gettext(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *in;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("gettext", "argument", "str", _arg);
            goto exit;
        }
        in = _PyUnicode_AsUTF8NoNUL(_arg);
        if (in == NULL) {
            goto exit;
        }
    }
    _return_value = _locale_gettext_impl(module, in);
    return_value = PyValue_FromResult(_return_value);

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
    {"dgettext", _PyCFunction_CAST(_locale_dgettext), METH_FASTCALL, _locale_dgettext__doc__},

static PyObject *
_locale_dgettext_impl(PyObject *module, const char *domain, const char *in);

static PyValue
_locale_dgettext(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    const char *in;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("dgettext", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            domain = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            domain = _PyUnicode_AsUTF8NoNUL(_arg);
            if (domain == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("dgettext", "argument 1", "str or None", _arg);
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("dgettext", "argument 2", "str", _arg);
            goto exit;
        }
        in = _PyUnicode_AsUTF8NoNUL(_arg);
        if (in == NULL) {
            goto exit;
        }
    }
    _return_value = _locale_dgettext_impl(module, domain, in);
    return_value = PyValue_FromResult(_return_value);

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
    {"dcgettext", _PyCFunction_CAST(_locale_dcgettext), METH_FASTCALL, _locale_dcgettext__doc__},

static PyObject *
_locale_dcgettext_impl(PyObject *module, const char *domain,
                       const char *msgid, int category);

static PyValue
_locale_dcgettext(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    const char *msgid;
    int category;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("dcgettext", nargs, 3, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            domain = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            domain = _PyUnicode_AsUTF8NoNUL(_arg);
            if (domain == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("dcgettext", "argument 1", "str or None", _arg);
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("dcgettext", "argument 2", "str", _arg);
            goto exit;
        }
        msgid = _PyUnicode_AsUTF8NoNUL(_arg);
        if (msgid == NULL) {
            goto exit;
        }
    }
    category = PyLong_AsIntValue(args[2]);
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _locale_dcgettext_impl(module, domain, msgid, category);
    return_value = PyValue_FromResult(_return_value);

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
    {"textdomain", _PyCFunction_CAST(_locale_textdomain), METH_O, _locale_textdomain__doc__},

static PyObject *
_locale_textdomain_impl(PyObject *module, const char *domain);

static PyValue
_locale_textdomain(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            domain = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            domain = _PyUnicode_AsUTF8NoNUL(_arg);
            if (domain == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("textdomain", "argument", "str or None", _arg);
            goto exit;
        }
    }
    _return_value = _locale_textdomain_impl(module, domain);
    return_value = PyValue_FromResult(_return_value);

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
    {"bindtextdomain", _PyCFunction_CAST(_locale_bindtextdomain), METH_FASTCALL, _locale_bindtextdomain__doc__},

static PyObject *
_locale_bindtextdomain_impl(PyObject *module, const char *domain,
                            PyObject *dirname_obj);

static PyValue
_locale_bindtextdomain(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    PyObject *dirname_obj;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("bindtextdomain", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("bindtextdomain", "argument 1", "str", _arg);
            goto exit;
        }
        domain = _PyUnicode_AsUTF8NoNUL(_arg);
        if (domain == NULL) {
            goto exit;
        }
    }
    dirname_obj = PyValue_Object(args[1]);
    if (dirname_obj == NULL) {
        goto exit;
    }
    _return_value = _locale_bindtextdomain_impl(module, domain, dirname_obj);
    return_value = PyValue_FromResult(_return_value);

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
    {"bind_textdomain_codeset", _PyCFunction_CAST(_locale_bind_textdomain_codeset), METH_FASTCALL, _locale_bind_textdomain_codeset__doc__},

static PyObject *
_locale_bind_textdomain_codeset_impl(PyObject *module, const char *domain,
                                     const char *codeset);

static PyValue
_locale_bind_textdomain_codeset(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const char *domain;
    const char *codeset;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("bind_textdomain_codeset", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("bind_textdomain_codeset", "argument 1", "str", _arg);
            goto exit;
        }
        domain = _PyUnicode_AsUTF8NoNUL(_arg);
        if (domain == NULL) {
            goto exit;
        }
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            codeset = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            codeset = _PyUnicode_AsUTF8NoNUL(_arg);
            if (codeset == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("bind_textdomain_codeset", "argument 2", "str or None", _arg);
            goto exit;
        }
    }
    _return_value = _locale_bind_textdomain_codeset_impl(module, domain, codeset);
    return_value = PyValue_FromResult(_return_value);

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
    {"getencoding", (PyCFunction)_locale_getencoding, METH_NOARGS, _locale_getencoding__doc__},

static PyObject *
_locale_getencoding_impl(PyObject *module);

static PyObject *
_locale_getencoding(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _locale_getencoding_impl(module);
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
/*[clinic end generated code: output=f299f9c1b04482a1 input=a9049054013a1b77]*/
