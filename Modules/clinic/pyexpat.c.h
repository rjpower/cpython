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
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(pyexpat_xmlparser_SetReparseDeferralEnabled__doc__,
"SetReparseDeferralEnabled($self, enabled, /)\n"
"--\n"
"\n"
"Enable/Disable reparse deferral; enabled by default with Expat >=2.6.0.");

#define PYEXPAT_XMLPARSER_SETREPARSEDEFERRALENABLED_METHODDEF    \
    {"SetReparseDeferralEnabled", _PyCFunction_CAST(pyexpat_xmlparser_SetReparseDeferralEnabled), METH_VALUE|METH_O, pyexpat_xmlparser_SetReparseDeferralEnabled__doc__},

static PyValue
pyexpat_xmlparser_SetReparseDeferralEnabled_impl(xmlparseobject *self,
                                                 int enabled);

static PyValue
pyexpat_xmlparser_SetReparseDeferralEnabled(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int enabled;

    enabled = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (enabled < 0) {
        goto exit;
    }
    return_value = pyexpat_xmlparser_SetReparseDeferralEnabled_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), enabled);

exit:
    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_GetReparseDeferralEnabled__doc__,
"GetReparseDeferralEnabled($self, /)\n"
"--\n"
"\n"
"Retrieve reparse deferral enabled status; always returns false with Expat <2.6.0.");

#define PYEXPAT_XMLPARSER_GETREPARSEDEFERRALENABLED_METHODDEF    \
    {"GetReparseDeferralEnabled", _PyCFunction_CAST(pyexpat_xmlparser_GetReparseDeferralEnabled), METH_VALUE|METH_NOARGS, pyexpat_xmlparser_GetReparseDeferralEnabled__doc__},

static PyValue
pyexpat_xmlparser_GetReparseDeferralEnabled_impl(xmlparseobject *self);

static PyValue
pyexpat_xmlparser_GetReparseDeferralEnabled(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pyexpat_xmlparser_GetReparseDeferralEnabled_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_Parse__doc__,
"Parse($self, data, isfinal=False, /)\n"
"--\n"
"\n"
"Parse XML data.\n"
"\n"
"\'data\' can be a bytes-like object or a string.  If it is a string,\n"
"the encoding declaration in the XML data is ignored.\n"
"\'isfinal\' should be true at end of input.");

#define PYEXPAT_XMLPARSER_PARSE_METHODDEF    \
    {"Parse", _PyCFunction_CAST(pyexpat_xmlparser_Parse), METH_VALUE|METH_METHOD|METH_KEYWORDS, pyexpat_xmlparser_Parse__doc__},

static PyValue
pyexpat_xmlparser_Parse_impl(xmlparseobject *self, PyTypeObject *cls,
                             PyValueBorrow data, int isfinal);

static PyValue
pyexpat_xmlparser_Parse(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow data;
    int isfinal = 0;

    if (!_PyArg_NoKwnames("Parse", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Parse", nargs, 1, 2)) {
        goto exit;
    }
    data = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    isfinal = _PyValue_IsTrue(PyValue_Borrow(args[1]));
    if (isfinal < 0) {
        goto exit;
    }
skip_optional:
    return_value = pyexpat_xmlparser_Parse_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), cls, data, isfinal);

exit:
    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_ParseFile__doc__,
"ParseFile($self, file, /)\n"
"--\n"
"\n"
"Parse XML data from a binary file-like object.\n"
"\n"
"Its read() method should return bytes.");

#define PYEXPAT_XMLPARSER_PARSEFILE_METHODDEF    \
    {"ParseFile", _PyCFunction_CAST(pyexpat_xmlparser_ParseFile), METH_VALUE|METH_METHOD|METH_KEYWORDS, pyexpat_xmlparser_ParseFile__doc__},

static PyValue
pyexpat_xmlparser_ParseFile_impl(xmlparseobject *self, PyTypeObject *cls,
                                 PyValueBorrow file);

static PyValue
pyexpat_xmlparser_ParseFile(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow file;

    if (!_PyArg_NoKwnames("ParseFile", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ParseFile", nargs, 1, 1)) {
        goto exit;
    }
    file = PyValue_Borrow(args[0]);
    return_value = pyexpat_xmlparser_ParseFile_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), cls, file);

exit:
    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_SetBase__doc__,
"SetBase($self, base, /)\n"
"--\n"
"\n"
"Set the base URL for the parser.");

#define PYEXPAT_XMLPARSER_SETBASE_METHODDEF    \
    {"SetBase", _PyCFunction_CAST(pyexpat_xmlparser_SetBase), METH_VALUE|METH_O, pyexpat_xmlparser_SetBase__doc__},

static PyValue
pyexpat_xmlparser_SetBase_impl(xmlparseobject *self, const char *base);

static PyValue
pyexpat_xmlparser_SetBase(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *base;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("SetBase", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("SetBase", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    base = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (base == NULL) {
        goto exit;
    }
    return_value = pyexpat_xmlparser_SetBase_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), base);

exit:
    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_GetBase__doc__,
"GetBase($self, /)\n"
"--\n"
"\n"
"Return base URL string for the parser.");

#define PYEXPAT_XMLPARSER_GETBASE_METHODDEF    \
    {"GetBase", _PyCFunction_CAST(pyexpat_xmlparser_GetBase), METH_VALUE|METH_NOARGS, pyexpat_xmlparser_GetBase__doc__},

static PyValue
pyexpat_xmlparser_GetBase_impl(xmlparseobject *self);

static PyValue
pyexpat_xmlparser_GetBase(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pyexpat_xmlparser_GetBase_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_GetSpecifiedAttributeCount__doc__,
"GetSpecifiedAttributeCount($self, /)\n"
"--\n"
"\n"
"Return the index just past the attributes given in the start tag.\n"
"\n"
"Attributes defaulted from the DTD follow the specified ones, so\n"
"attributes at lower indices in the list passed to\n"
"StartElementHandler were given in the start tag.  Each attribute\n"
"takes two items in that list, its name and its value.  Only\n"
"meaningful inside a StartElementHandler call, and only if\n"
"ordered_attributes is true.");

#define PYEXPAT_XMLPARSER_GETSPECIFIEDATTRIBUTECOUNT_METHODDEF    \
    {"GetSpecifiedAttributeCount", _PyCFunction_CAST(pyexpat_xmlparser_GetSpecifiedAttributeCount), METH_VALUE|METH_NOARGS, pyexpat_xmlparser_GetSpecifiedAttributeCount__doc__},

static PyValue
pyexpat_xmlparser_GetSpecifiedAttributeCount_impl(xmlparseobject *self);

static PyValue
pyexpat_xmlparser_GetSpecifiedAttributeCount(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pyexpat_xmlparser_GetSpecifiedAttributeCount_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_GetInputContext__doc__,
"GetInputContext($self, /)\n"
"--\n"
"\n"
"Return the input which generated the current event as bytes.\n"
"\n"
"The data is in the encoding of the entity which contains the text.\n"
"It extends to the end of the currently buffered input, therefore it\n"
"can contain also the data of the following events, and if the event\n"
"was generated by a large amount of text, not all of it may be\n"
"available.");

#define PYEXPAT_XMLPARSER_GETINPUTCONTEXT_METHODDEF    \
    {"GetInputContext", _PyCFunction_CAST(pyexpat_xmlparser_GetInputContext), METH_VALUE|METH_NOARGS, pyexpat_xmlparser_GetInputContext__doc__},

static PyValue
pyexpat_xmlparser_GetInputContext_impl(xmlparseobject *self);

static PyValue
pyexpat_xmlparser_GetInputContext(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = pyexpat_xmlparser_GetInputContext_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_ExternalEntityParserCreate__doc__,
"ExternalEntityParserCreate($self, context, encoding=<unrepresentable>,\n"
"                           /)\n"
"--\n"
"\n"
"Create a parser for parsing an external entity based on the information passed to the ExternalEntityRefHandler.");

#define PYEXPAT_XMLPARSER_EXTERNALENTITYPARSERCREATE_METHODDEF    \
    {"ExternalEntityParserCreate", _PyCFunction_CAST(pyexpat_xmlparser_ExternalEntityParserCreate), METH_VALUE|METH_METHOD|METH_KEYWORDS, pyexpat_xmlparser_ExternalEntityParserCreate__doc__},

static PyValue
pyexpat_xmlparser_ExternalEntityParserCreate_impl(xmlparseobject *self,
                                                  PyTypeObject *cls,
                                                  const char *context,
                                                  const char *encoding);

static PyValue
pyexpat_xmlparser_ExternalEntityParserCreate(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *context;
    const char *encoding = NULL;

    if (!_PyArg_NoKwnames("ExternalEntityParserCreate", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ExternalEntityParserCreate", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("ExternalEntityParserCreate", "argument 1", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        context = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        context = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (context == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("ExternalEntityParserCreate", "argument 1", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("ExternalEntityParserCreate", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("ExternalEntityParserCreate", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (encoding == NULL) {
        goto exit;
    }
skip_optional:
    return_value = pyexpat_xmlparser_ExternalEntityParserCreate_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), cls, context, encoding);

exit:
    return return_value;
}

PyDoc_STRVAR(pyexpat_xmlparser_SetParamEntityParsing__doc__,
"SetParamEntityParsing($self, flag, /)\n"
"--\n"
"\n"
"Controls parsing of parameter entities (including the external DTD subset).\n"
"\n"
"Possible flag values are XML_PARAM_ENTITY_PARSING_NEVER,\n"
"XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE and\n"
"XML_PARAM_ENTITY_PARSING_ALWAYS. Returns true if setting the flag\n"
"was successful.");

#define PYEXPAT_XMLPARSER_SETPARAMENTITYPARSING_METHODDEF    \
    {"SetParamEntityParsing", _PyCFunction_CAST(pyexpat_xmlparser_SetParamEntityParsing), METH_VALUE|METH_O, pyexpat_xmlparser_SetParamEntityParsing__doc__},

static PyValue
pyexpat_xmlparser_SetParamEntityParsing_impl(xmlparseobject *self, int flag);

static PyValue
pyexpat_xmlparser_SetParamEntityParsing(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag;

    flag = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pyexpat_xmlparser_SetParamEntityParsing_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), flag);

exit:
    return return_value;
}

#if (XML_COMBINED_VERSION >= 19505)

PyDoc_STRVAR(pyexpat_xmlparser_UseForeignDTD__doc__,
"UseForeignDTD($self, flag=True, /)\n"
"--\n"
"\n"
"Allows the application to provide an artificial external subset if one is not specified as part of the document instance.\n"
"\n"
"This readily allows the use of a \'default\' document type controlled\n"
"by the application, while still getting the advantage of providing\n"
"document type information to the parser.  \'flag\' defaults to True if\n"
"not provided.");

#define PYEXPAT_XMLPARSER_USEFOREIGNDTD_METHODDEF    \
    {"UseForeignDTD", _PyCFunction_CAST(pyexpat_xmlparser_UseForeignDTD), METH_VALUE|METH_METHOD|METH_KEYWORDS, pyexpat_xmlparser_UseForeignDTD__doc__},

static PyValue
pyexpat_xmlparser_UseForeignDTD_impl(xmlparseobject *self, PyTypeObject *cls,
                                     int flag);

static PyValue
pyexpat_xmlparser_UseForeignDTD(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag = 1;

    if (!_PyArg_NoKwnames("UseForeignDTD", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("UseForeignDTD", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
skip_optional:
    return_value = pyexpat_xmlparser_UseForeignDTD_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), cls, flag);

exit:
    return return_value;
}

#endif /* (XML_COMBINED_VERSION >= 19505) */

#if (XML_COMBINED_VERSION >= 20400)

PyDoc_STRVAR(pyexpat_xmlparser_SetBillionLaughsAttackProtectionActivationThreshold__doc__,
"SetBillionLaughsAttackProtectionActivationThreshold($self, threshold, /)\n"
"--\n"
"\n"
"Sets the number of output bytes needed to activate protection against billion laughs attacks.\n"
"\n"
"The number of output bytes includes amplification from entity\n"
"expansion and reading DTD files.\n"
"\n"
"Parser objects usually have a protection activation threshold of\n"
"8 MiB, but the actual default value depends on the underlying Expat\n"
"library.\n"
"\n"
"Activation thresholds below 4 MiB are known to break support for\n"
"DITA 1.3 payload and are hence not recommended.");

#define PYEXPAT_XMLPARSER_SETBILLIONLAUGHSATTACKPROTECTIONACTIVATIONTHRESHOLD_METHODDEF    \
    {"SetBillionLaughsAttackProtectionActivationThreshold", _PyCFunction_CAST(pyexpat_xmlparser_SetBillionLaughsAttackProtectionActivationThreshold), METH_VALUE|METH_METHOD|METH_KEYWORDS, pyexpat_xmlparser_SetBillionLaughsAttackProtectionActivationThreshold__doc__},

static PyValue
pyexpat_xmlparser_SetBillionLaughsAttackProtectionActivationThreshold_impl(xmlparseobject *self,
                                                                           PyTypeObject *cls,
                                                                           unsigned long long threshold);

static PyValue
pyexpat_xmlparser_SetBillionLaughsAttackProtectionActivationThreshold(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned long long threshold;

    if (!_PyArg_NoKwnames("SetBillionLaughsAttackProtectionActivationThreshold", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("SetBillionLaughsAttackProtectionActivationThreshold", nargs, 1, 1)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &threshold, sizeof(unsigned long long),
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
    return_value = pyexpat_xmlparser_SetBillionLaughsAttackProtectionActivationThreshold_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), cls, threshold);

exit:
    return return_value;
}

#endif /* (XML_COMBINED_VERSION >= 20400) */

#if (XML_COMBINED_VERSION >= 20400)

PyDoc_STRVAR(pyexpat_xmlparser_SetBillionLaughsAttackProtectionMaximumAmplification__doc__,
"SetBillionLaughsAttackProtectionMaximumAmplification($self, max_factor,\n"
"                                                     /)\n"
"--\n"
"\n"
"Sets the maximum tolerated amplification factor for protection against billion laughs attacks.\n"
"\n"
"The amplification factor is calculated as \"(direct + indirect) /\n"
"direct\" while parsing, where \"direct\" is the number of bytes read\n"
"from the primary document in parsing and \"indirect\" is the number of\n"
"bytes added by expanding entities and reading external DTD files,\n"
"combined.\n"
"\n"
"The \'max_factor\' value must be a non-NaN floating point value\n"
"greater than or equal to 1.0.  Amplification factors greater than\n"
"30,000 can be observed in the middle of parsing even with benign\n"
"files in practice.  In particular, the activation threshold should\n"
"be carefully chosen to avoid false positives.\n"
"\n"
"Parser objects usually have a maximum amplification factor of 100,\n"
"but the actual default value depends on the underlying Expat\n"
"library.");

#define PYEXPAT_XMLPARSER_SETBILLIONLAUGHSATTACKPROTECTIONMAXIMUMAMPLIFICATION_METHODDEF    \
    {"SetBillionLaughsAttackProtectionMaximumAmplification", _PyCFunction_CAST(pyexpat_xmlparser_SetBillionLaughsAttackProtectionMaximumAmplification), METH_VALUE|METH_METHOD|METH_KEYWORDS, pyexpat_xmlparser_SetBillionLaughsAttackProtectionMaximumAmplification__doc__},

static PyValue
pyexpat_xmlparser_SetBillionLaughsAttackProtectionMaximumAmplification_impl(xmlparseobject *self,
                                                                            PyTypeObject *cls,
                                                                            float max_factor);

static PyValue
pyexpat_xmlparser_SetBillionLaughsAttackProtectionMaximumAmplification(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    float max_factor;

    if (!_PyArg_NoKwnames("SetBillionLaughsAttackProtectionMaximumAmplification", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("SetBillionLaughsAttackProtectionMaximumAmplification", nargs, 1, 1)) {
        goto exit;
    }
    max_factor = (float) _PyValue_AsDouble(PyValue_Borrow(args[0]));
    if (max_factor == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pyexpat_xmlparser_SetBillionLaughsAttackProtectionMaximumAmplification_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), cls, max_factor);

exit:
    return return_value;
}

#endif /* (XML_COMBINED_VERSION >= 20400) */

#if (XML_COMBINED_VERSION >= 20702)

PyDoc_STRVAR(pyexpat_xmlparser_SetAllocTrackerActivationThreshold__doc__,
"SetAllocTrackerActivationThreshold($self, threshold, /)\n"
"--\n"
"\n"
"Sets the number of allocated bytes of dynamic memory needed to activate protection against disproportionate use of RAM.\n"
"\n"
"Parser objects usually have an allocation activation threshold of\n"
"64 MiB, but the actual default value depends on the underlying Expat\n"
"library.");

#define PYEXPAT_XMLPARSER_SETALLOCTRACKERACTIVATIONTHRESHOLD_METHODDEF    \
    {"SetAllocTrackerActivationThreshold", _PyCFunction_CAST(pyexpat_xmlparser_SetAllocTrackerActivationThreshold), METH_VALUE|METH_METHOD|METH_KEYWORDS, pyexpat_xmlparser_SetAllocTrackerActivationThreshold__doc__},

static PyValue
pyexpat_xmlparser_SetAllocTrackerActivationThreshold_impl(xmlparseobject *self,
                                                          PyTypeObject *cls,
                                                          unsigned long long threshold);

static PyValue
pyexpat_xmlparser_SetAllocTrackerActivationThreshold(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned long long threshold;

    if (!_PyArg_NoKwnames("SetAllocTrackerActivationThreshold", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("SetAllocTrackerActivationThreshold", nargs, 1, 1)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &threshold, sizeof(unsigned long long),
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
    return_value = pyexpat_xmlparser_SetAllocTrackerActivationThreshold_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), cls, threshold);

exit:
    return return_value;
}

#endif /* (XML_COMBINED_VERSION >= 20702) */

#if (XML_COMBINED_VERSION >= 20702)

PyDoc_STRVAR(pyexpat_xmlparser_SetAllocTrackerMaximumAmplification__doc__,
"SetAllocTrackerMaximumAmplification($self, max_factor, /)\n"
"--\n"
"\n"
"Sets the maximum amplification factor between direct input and bytes of dynamic memory allocated.\n"
"\n"
"The amplification factor is calculated as \"allocated / direct\" while\n"
"parsing, where \"direct\" is the number of bytes read from the primary\n"
"document in parsing and \"allocated\" is the number of bytes of\n"
"dynamic memory allocated in the parser hierarchy.\n"
"\n"
"The \'max_factor\' value must be a non-NaN floating point value\n"
"greater than or equal to 1.0.  Amplification factors greater than\n"
"100.0 can be observed near the start of parsing even with benign\n"
"files in practice.  In particular, the activation threshold should\n"
"be carefully chosen to avoid false positives.\n"
"\n"
"Parser objects usually have a maximum amplification factor of 100,\n"
"but the actual default value depends on the underlying Expat\n"
"library.");

#define PYEXPAT_XMLPARSER_SETALLOCTRACKERMAXIMUMAMPLIFICATION_METHODDEF    \
    {"SetAllocTrackerMaximumAmplification", _PyCFunction_CAST(pyexpat_xmlparser_SetAllocTrackerMaximumAmplification), METH_VALUE|METH_METHOD|METH_KEYWORDS, pyexpat_xmlparser_SetAllocTrackerMaximumAmplification__doc__},

static PyValue
pyexpat_xmlparser_SetAllocTrackerMaximumAmplification_impl(xmlparseobject *self,
                                                           PyTypeObject *cls,
                                                           float max_factor);

static PyValue
pyexpat_xmlparser_SetAllocTrackerMaximumAmplification(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    float max_factor;

    if (!_PyArg_NoKwnames("SetAllocTrackerMaximumAmplification", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("SetAllocTrackerMaximumAmplification", nargs, 1, 1)) {
        goto exit;
    }
    max_factor = (float) _PyValue_AsDouble(PyValue_Borrow(args[0]));
    if (max_factor == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pyexpat_xmlparser_SetAllocTrackerMaximumAmplification_impl((xmlparseobject *)PyValue_AsKnownObjectBorrow(self), cls, max_factor);

exit:
    return return_value;
}

#endif /* (XML_COMBINED_VERSION >= 20702) */

PyDoc_STRVAR(pyexpat_ParserCreate__doc__,
"ParserCreate($module, /, encoding=None, namespace_separator=None,\n"
"             intern=<unrepresentable>)\n"
"--\n"
"\n"
"Return a new XML parser object.");

#define PYEXPAT_PARSERCREATE_METHODDEF    \
    {"ParserCreate", _PyCFunction_CAST(pyexpat_ParserCreate), METH_VALUE|METH_KEYWORDS, pyexpat_ParserCreate__doc__},

static PyValue
pyexpat_ParserCreate_impl(PyObject *module, const char *encoding,
                          const char *namespace_separator,
                          PyValueBorrow intern);

static PyValue
pyexpat_ParserCreate(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), _PyValue_STATIC_IMMORTAL(&_Py_ID(namespace_separator)), _PyValue_STATIC_IMMORTAL(&_Py_ID(intern)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"encoding", "namespace_separator", "intern", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ParserCreate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    const char *encoding = NULL;
    const char *namespace_separator = NULL;
    PyValueBorrow intern = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("ParserCreate", "argument 'encoding'", "str or None", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])) == Py_None) {
            encoding = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            if (encoding == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("ParserCreate", "argument 'encoding'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("ParserCreate", "argument 'namespace_separator'", "str or None", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])) == Py_None) {
            namespace_separator = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            namespace_separator = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            if (namespace_separator == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("ParserCreate", "argument 'namespace_separator'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        intern = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = pyexpat_ParserCreate_impl(PyValue_AsKnownObjectBorrow(module), encoding, namespace_separator, intern);

exit:
    return return_value;
}

PyDoc_STRVAR(pyexpat_ErrorString__doc__,
"ErrorString($module, code, /)\n"
"--\n"
"\n"
"Returns string error for given number.");

#define PYEXPAT_ERRORSTRING_METHODDEF    \
    {"ErrorString", _PyCFunction_CAST(pyexpat_ErrorString), METH_VALUE|METH_O, pyexpat_ErrorString__doc__},

static PyValue
pyexpat_ErrorString_impl(PyObject *module, long code);

static PyValue
pyexpat_ErrorString(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    long code;

    code = _PyValue_AsLong(PyValue_Borrow(args[0]));
    if (code == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = pyexpat_ErrorString_impl(PyValue_AsKnownObjectBorrow(module), code);

exit:
    return return_value;
}

#ifndef PYEXPAT_XMLPARSER_USEFOREIGNDTD_METHODDEF
    #define PYEXPAT_XMLPARSER_USEFOREIGNDTD_METHODDEF
#endif /* !defined(PYEXPAT_XMLPARSER_USEFOREIGNDTD_METHODDEF) */

#ifndef PYEXPAT_XMLPARSER_SETBILLIONLAUGHSATTACKPROTECTIONACTIVATIONTHRESHOLD_METHODDEF
    #define PYEXPAT_XMLPARSER_SETBILLIONLAUGHSATTACKPROTECTIONACTIVATIONTHRESHOLD_METHODDEF
#endif /* !defined(PYEXPAT_XMLPARSER_SETBILLIONLAUGHSATTACKPROTECTIONACTIVATIONTHRESHOLD_METHODDEF) */

#ifndef PYEXPAT_XMLPARSER_SETBILLIONLAUGHSATTACKPROTECTIONMAXIMUMAMPLIFICATION_METHODDEF
    #define PYEXPAT_XMLPARSER_SETBILLIONLAUGHSATTACKPROTECTIONMAXIMUMAMPLIFICATION_METHODDEF
#endif /* !defined(PYEXPAT_XMLPARSER_SETBILLIONLAUGHSATTACKPROTECTIONMAXIMUMAMPLIFICATION_METHODDEF) */

#ifndef PYEXPAT_XMLPARSER_SETALLOCTRACKERACTIVATIONTHRESHOLD_METHODDEF
    #define PYEXPAT_XMLPARSER_SETALLOCTRACKERACTIVATIONTHRESHOLD_METHODDEF
#endif /* !defined(PYEXPAT_XMLPARSER_SETALLOCTRACKERACTIVATIONTHRESHOLD_METHODDEF) */

#ifndef PYEXPAT_XMLPARSER_SETALLOCTRACKERMAXIMUMAMPLIFICATION_METHODDEF
    #define PYEXPAT_XMLPARSER_SETALLOCTRACKERMAXIMUMAMPLIFICATION_METHODDEF
#endif /* !defined(PYEXPAT_XMLPARSER_SETALLOCTRACKERMAXIMUMAMPLIFICATION_METHODDEF) */
/*[clinic end generated code: output=0a3f6e421ca182a9 input=a9049054013a1b77]*/
