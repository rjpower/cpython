/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(unicodedata_UCD_decimal__doc__,
"decimal($self, chr, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Converts a Unicode character into its equivalent decimal value.\n"
"\n"
"Returns the decimal value assigned to the character chr as integer.\n"
"If no such value is defined, default is returned, or, if not given,\n"
"ValueError is raised.");

#define UNICODEDATA_UCD_DECIMAL_METHODDEF    \
    {"decimal", _PyCFunction_CAST(unicodedata_UCD_decimal), METH_FASTCALL, unicodedata_UCD_decimal__doc__},

static PyObject *
unicodedata_UCD_decimal_impl(PyObject *self, int chr,
                             PyObject *default_value);

static PyValue
unicodedata_UCD_decimal(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *default_value = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("decimal", nargs, 1, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("decimal", "argument 1", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "decimal(): argument 1 must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Object(args[1]);
    if (default_value == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = unicodedata_UCD_decimal_impl(self, chr, default_value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_digit__doc__,
"digit($self, chr, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Converts a Unicode character into its equivalent digit value.\n"
"\n"
"Returns the digit value assigned to the character chr as integer.\n"
"If no such value is defined, default is returned, or, if not given,\n"
"ValueError is raised.");

#define UNICODEDATA_UCD_DIGIT_METHODDEF    \
    {"digit", _PyCFunction_CAST(unicodedata_UCD_digit), METH_FASTCALL, unicodedata_UCD_digit__doc__},

static PyObject *
unicodedata_UCD_digit_impl(PyObject *self, int chr, PyObject *default_value);

static PyValue
unicodedata_UCD_digit(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *default_value = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("digit", nargs, 1, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("digit", "argument 1", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "digit(): argument 1 must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Object(args[1]);
    if (default_value == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = unicodedata_UCD_digit_impl(self, chr, default_value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_numeric__doc__,
"numeric($self, chr, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Converts a Unicode character into its equivalent numeric value.\n"
"\n"
"Returns the numeric value assigned to the character chr as float.\n"
"If no such value is defined, default is returned, or, if not given,\n"
"ValueError is raised.");

#define UNICODEDATA_UCD_NUMERIC_METHODDEF    \
    {"numeric", _PyCFunction_CAST(unicodedata_UCD_numeric), METH_FASTCALL, unicodedata_UCD_numeric__doc__},

static PyObject *
unicodedata_UCD_numeric_impl(PyObject *self, int chr,
                             PyObject *default_value);

static PyValue
unicodedata_UCD_numeric(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *default_value = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("numeric", nargs, 1, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("numeric", "argument 1", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "numeric(): argument 1 must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Object(args[1]);
    if (default_value == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = unicodedata_UCD_numeric_impl(self, chr, default_value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_category__doc__,
"category($self, chr, /)\n"
"--\n"
"\n"
"Returns the general category assigned to the character chr as string.");

#define UNICODEDATA_UCD_CATEGORY_METHODDEF    \
    {"category", _PyCFunction_CAST(unicodedata_UCD_category), METH_O, unicodedata_UCD_category__doc__},

static PyObject *
unicodedata_UCD_category_impl(PyObject *self, int chr);

static PyValue
unicodedata_UCD_category(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("category", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "category(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_UCD_category_impl(self, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_bidirectional__doc__,
"bidirectional($self, chr, /)\n"
"--\n"
"\n"
"Returns the bidirectional class assigned to the character chr as string.\n"
"\n"
"If no such value is defined, an empty string is returned.");

#define UNICODEDATA_UCD_BIDIRECTIONAL_METHODDEF    \
    {"bidirectional", _PyCFunction_CAST(unicodedata_UCD_bidirectional), METH_O, unicodedata_UCD_bidirectional__doc__},

static PyObject *
unicodedata_UCD_bidirectional_impl(PyObject *self, int chr);

static PyValue
unicodedata_UCD_bidirectional(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("bidirectional", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "bidirectional(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_UCD_bidirectional_impl(self, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_combining__doc__,
"combining($self, chr, /)\n"
"--\n"
"\n"
"Returns the canonical combining class assigned to the character chr as integer.\n"
"\n"
"Returns 0 if no combining class is defined.");

#define UNICODEDATA_UCD_COMBINING_METHODDEF    \
    {"combining", _PyCFunction_CAST(unicodedata_UCD_combining), METH_O, unicodedata_UCD_combining__doc__},

static int
unicodedata_UCD_combining_impl(PyObject *self, int chr);

static PyValue
unicodedata_UCD_combining(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    int _return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("combining", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "combining(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_UCD_combining_impl(self, chr);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_mirrored__doc__,
"mirrored($self, chr, /)\n"
"--\n"
"\n"
"Returns the mirrored property assigned to the character chr as integer.\n"
"\n"
"Returns 1 if the character has been identified as a \"mirrored\"\n"
"character in bidirectional text, 0 otherwise.");

#define UNICODEDATA_UCD_MIRRORED_METHODDEF    \
    {"mirrored", _PyCFunction_CAST(unicodedata_UCD_mirrored), METH_O, unicodedata_UCD_mirrored__doc__},

static int
unicodedata_UCD_mirrored_impl(PyObject *self, int chr);

static PyValue
unicodedata_UCD_mirrored(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    int _return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("mirrored", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "mirrored(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_UCD_mirrored_impl(self, chr);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_east_asian_width__doc__,
"east_asian_width($self, chr, /)\n"
"--\n"
"\n"
"Returns the east asian width assigned to the character chr as string.");

#define UNICODEDATA_UCD_EAST_ASIAN_WIDTH_METHODDEF    \
    {"east_asian_width", _PyCFunction_CAST(unicodedata_UCD_east_asian_width), METH_O, unicodedata_UCD_east_asian_width__doc__},

static PyObject *
unicodedata_UCD_east_asian_width_impl(PyObject *self, int chr);

static PyValue
unicodedata_UCD_east_asian_width(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("east_asian_width", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "east_asian_width(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_UCD_east_asian_width_impl(self, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_decomposition__doc__,
"decomposition($self, chr, /)\n"
"--\n"
"\n"
"Returns the character decomposition mapping assigned to the character chr as string.\n"
"\n"
"An empty string is returned in case no such mapping is defined.");

#define UNICODEDATA_UCD_DECOMPOSITION_METHODDEF    \
    {"decomposition", _PyCFunction_CAST(unicodedata_UCD_decomposition), METH_O, unicodedata_UCD_decomposition__doc__},

static PyObject *
unicodedata_UCD_decomposition_impl(PyObject *self, int chr);

static PyValue
unicodedata_UCD_decomposition(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("decomposition", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "decomposition(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_UCD_decomposition_impl(self, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_is_normalized__doc__,
"is_normalized($self, form, unistr, /)\n"
"--\n"
"\n"
"Return whether the Unicode string unistr is in the normal form \'form\'.\n"
"\n"
"Valid values for form are \'NFC\', \'NFKC\', \'NFD\', and \'NFKD\'.");

#define UNICODEDATA_UCD_IS_NORMALIZED_METHODDEF    \
    {"is_normalized", _PyCFunction_CAST(unicodedata_UCD_is_normalized), METH_FASTCALL, unicodedata_UCD_is_normalized__doc__},

static PyObject *
unicodedata_UCD_is_normalized_impl(PyObject *self, PyObject *form,
                                   PyObject *input);

static PyValue
unicodedata_UCD_is_normalized(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *form;
    PyObject *input;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("is_normalized", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("is_normalized", "argument 1", "str", _arg);
            goto exit;
        }
        form = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("is_normalized", "argument 2", "str", _arg);
            goto exit;
        }
        input = _arg;
    }
    _return_value = unicodedata_UCD_is_normalized_impl(self, form, input);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_normalize__doc__,
"normalize($self, form, unistr, /)\n"
"--\n"
"\n"
"Return the normal form \'form\' for the Unicode string unistr.\n"
"\n"
"Valid values for form are \'NFC\', \'NFKC\', \'NFD\', and \'NFKD\'.");

#define UNICODEDATA_UCD_NORMALIZE_METHODDEF    \
    {"normalize", _PyCFunction_CAST(unicodedata_UCD_normalize), METH_FASTCALL, unicodedata_UCD_normalize__doc__},

static PyObject *
unicodedata_UCD_normalize_impl(PyObject *self, PyObject *form,
                               PyObject *input);

static PyValue
unicodedata_UCD_normalize(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *form;
    PyObject *input;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("normalize", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("normalize", "argument 1", "str", _arg);
            goto exit;
        }
        form = _arg;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("normalize", "argument 2", "str", _arg);
            goto exit;
        }
        input = _arg;
    }
    _return_value = unicodedata_UCD_normalize_impl(self, form, input);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_name__doc__,
"name($self, chr, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Returns the name assigned to the character chr as a string.\n"
"\n"
"If no name is defined, default is returned, or, if not given,\n"
"ValueError is raised.");

#define UNICODEDATA_UCD_NAME_METHODDEF    \
    {"name", _PyCFunction_CAST(unicodedata_UCD_name), METH_FASTCALL, unicodedata_UCD_name__doc__},

static PyObject *
unicodedata_UCD_name_impl(PyObject *self, int chr, PyObject *default_value);

static PyValue
unicodedata_UCD_name(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *default_value = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("name", nargs, 1, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("name", "argument 1", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "name(): argument 1 must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Object(args[1]);
    if (default_value == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = unicodedata_UCD_name_impl(self, chr, default_value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_isxidstart__doc__,
"isxidstart($module, chr, /)\n"
"--\n"
"\n"
"Return True if the character has the XID_Start property, else False.");

#define UNICODEDATA_ISXIDSTART_METHODDEF    \
    {"isxidstart", _PyCFunction_CAST(unicodedata_isxidstart), METH_O, unicodedata_isxidstart__doc__},

static PyObject *
unicodedata_isxidstart_impl(PyObject *module, int chr);

static PyValue
unicodedata_isxidstart(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("isxidstart", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "isxidstart(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_isxidstart_impl(module, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_isxidcontinue__doc__,
"isxidcontinue($module, chr, /)\n"
"--\n"
"\n"
"Return True if the character has the XID_Continue property, else False.");

#define UNICODEDATA_ISXIDCONTINUE_METHODDEF    \
    {"isxidcontinue", _PyCFunction_CAST(unicodedata_isxidcontinue), METH_O, unicodedata_isxidcontinue__doc__},

static PyObject *
unicodedata_isxidcontinue_impl(PyObject *module, int chr);

static PyValue
unicodedata_isxidcontinue(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("isxidcontinue", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "isxidcontinue(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_isxidcontinue_impl(module, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_UCD_lookup__doc__,
"lookup($self, name, /)\n"
"--\n"
"\n"
"Look up character by name.\n"
"\n"
"If a character with the given name is found, return the\n"
"corresponding character.  If not found, KeyError is raised.");

#define UNICODEDATA_UCD_LOOKUP_METHODDEF    \
    {"lookup", _PyCFunction_CAST(unicodedata_UCD_lookup), METH_O, unicodedata_UCD_lookup__doc__},

static PyObject *
unicodedata_UCD_lookup_impl(PyObject *self, const char *name,
                            Py_ssize_t name_length);

static PyValue
unicodedata_UCD_lookup(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    const char *name;
    Py_ssize_t name_length;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyArg_Parse(_arg, "s#:lookup", &name, &name_length)) {
            goto exit;
        }
    }
    _return_value = unicodedata_UCD_lookup_impl(self, name, name_length);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_iter_graphemes__doc__,
"iter_graphemes($module, unistr, start=0, end=sys.maxsize, /)\n"
"--\n"
"\n"
"Returns an iterator to iterate over grapheme clusters.\n"
"\n"
"It uses extended grapheme cluster rules from TR29.");

#define UNICODEDATA_ITER_GRAPHEMES_METHODDEF    \
    {"iter_graphemes", _PyCFunction_CAST(unicodedata_iter_graphemes), METH_FASTCALL, unicodedata_iter_graphemes__doc__},

static PyObject *
unicodedata_iter_graphemes_impl(PyObject *module, PyObject *unistr,
                                Py_ssize_t start, Py_ssize_t end);

static PyValue
unicodedata_iter_graphemes(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *unistr;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("iter_graphemes", nargs, 1, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("iter_graphemes", "argument 1", "str", _arg);
            goto exit;
        }
        unistr = _arg;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        start = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[2]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        end = ival;
    }
skip_optional:
    _return_value = unicodedata_iter_graphemes_impl(module, unistr, start, end);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_block__doc__,
"block($module, chr, /)\n"
"--\n"
"\n"
"Return block assigned to the character chr.");

#define UNICODEDATA_BLOCK_METHODDEF    \
    {"block", _PyCFunction_CAST(unicodedata_block), METH_O, unicodedata_block__doc__},

static PyObject *
unicodedata_block_impl(PyObject *module, int chr);

static PyValue
unicodedata_block(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("block", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "block(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_block_impl(module, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_grapheme_cluster_break__doc__,
"grapheme_cluster_break($module, chr, /)\n"
"--\n"
"\n"
"Returns the Grapheme_Cluster_Break property assigned to the character.");

#define UNICODEDATA_GRAPHEME_CLUSTER_BREAK_METHODDEF    \
    {"grapheme_cluster_break", _PyCFunction_CAST(unicodedata_grapheme_cluster_break), METH_O, unicodedata_grapheme_cluster_break__doc__},

static PyObject *
unicodedata_grapheme_cluster_break_impl(PyObject *module, int chr);

static PyValue
unicodedata_grapheme_cluster_break(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("grapheme_cluster_break", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "grapheme_cluster_break(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_grapheme_cluster_break_impl(module, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_indic_conjunct_break__doc__,
"indic_conjunct_break($module, chr, /)\n"
"--\n"
"\n"
"Returns the Indic_Conjunct_Break property assigned to the character.");

#define UNICODEDATA_INDIC_CONJUNCT_BREAK_METHODDEF    \
    {"indic_conjunct_break", _PyCFunction_CAST(unicodedata_indic_conjunct_break), METH_O, unicodedata_indic_conjunct_break__doc__},

static PyObject *
unicodedata_indic_conjunct_break_impl(PyObject *module, int chr);

static PyValue
unicodedata_indic_conjunct_break(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("indic_conjunct_break", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "indic_conjunct_break(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_indic_conjunct_break_impl(module, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicodedata_extended_pictographic__doc__,
"extended_pictographic($module, chr, /)\n"
"--\n"
"\n"
"Returns the Extended_Pictographic property assigned to the character, as boolean.");

#define UNICODEDATA_EXTENDED_PICTOGRAPHIC_METHODDEF    \
    {"extended_pictographic", _PyCFunction_CAST(unicodedata_extended_pictographic), METH_O, unicodedata_extended_pictographic__doc__},

static PyObject *
unicodedata_extended_pictographic_impl(PyObject *module, int chr);

static PyValue
unicodedata_extended_pictographic(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    int chr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("extended_pictographic", "argument", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "extended_pictographic(): argument must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        chr = PyUnicode_READ_CHAR(_arg, 0);
    }
    _return_value = unicodedata_extended_pictographic_impl(module, chr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=37f5a248e1b746a3 input=a9049054013a1b77]*/
