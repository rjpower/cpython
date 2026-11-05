/* ------------------------------------------------------------------------

   _codecs -- Provides access to the codec registry and the builtin
              codecs.

   This module should never be imported directly. The standard library
   module "codecs" wraps this builtin module for use within Python.

   The codec registry is accessible via:

     register(search_function) -> None

     lookup(encoding) -> CodecInfo object

   The builtin Unicode codecs use the following interface:

     <encoding>_encode(Unicode_object[,errors='strict']) ->
        (string object, bytes consumed)

     <encoding>_decode(char_buffer_obj[,errors='strict']) ->
        (Unicode object, bytes consumed)

   These <encoding>s are available: utf_8, unicode_escape,
   raw_unicode_escape, latin_1, ascii (7-bit), mbcs (on win32).


Written by Marc-Andre Lemburg (mal@lemburg.com).

Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_codecs.h"        // _PyCodec_Lookup()
#include "pycore_unicodeobject.h" // _PyUnicode_EncodeCharmap
#include "pycore_value.h"         // _PyValue_AsObjectAt()

#ifdef MS_WINDOWS
#include <windows.h>
#endif

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _codecs
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e1390e3da3cb9deb]*/

#include "pycore_runtime.h"
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()
#include "clinic/_codecsmodule.c.h"

/* --- Registry ----------------------------------------------------------- */

/*[clinic input]
_codecs.register
    search_function: object
    /

Register a codec search function.

Search functions are expected to take one argument, the encoding
name in all lower case letters, and either return None, or a tuple
of functions (encoder, decoder, stream_reader, stream_writer) (or
a CodecInfo object).
[clinic start generated code]*/

static PyValue
_codecs_register_impl(PyObject *module, PyValueBorrow search_function)
/*[clinic end generated code: output=4773503b14a5a9c9 input=2321d8c8c0420dfc]*/
{
    PyObject *func = _PyValue_AsObjectAt(search_function, BOX_SITE_PARSER);
    if (func == NULL)
        return PyValue_NULL;
    int rc = PyCodec_Register(func);
    Py_DECREF(func);
    if (rc)
        return PyValue_NULL;

    return PyValue_None;
}

/*[clinic input]
_codecs.unregister
    search_function: object
    /

Unregister a codec search function and clear the registry's cache.

If the search function is not registered, do nothing.
[clinic start generated code]*/

static PyValue
_codecs_unregister_impl(PyObject *module, PyValueBorrow search_function)
/*[clinic end generated code: output=aa9e0c9e5a2ce8e4 input=dd7c004c652d345e]*/
{
    PyObject *func = _PyValue_AsObjectAt(search_function, BOX_SITE_PARSER);
    if (func == NULL) {
        return PyValue_NULL;
    }
    int rc = PyCodec_Unregister(func);
    Py_DECREF(func);
    if (rc < 0) {
        return PyValue_NULL;
    }

    return PyValue_None;
}

/*[clinic input]
@permit_long_summary
_codecs.lookup
    encoding: str
    /

Looks up a codec tuple in the Python codec registry and returns a CodecInfo object.
[clinic start generated code]*/

static PyValue
_codecs_lookup_impl(PyObject *module, const char *encoding)
/*[clinic end generated code: output=436b8799792eceb4 input=02227d5429491ab3]*/
{
    return _PyValue_FromObjectResult(_PyCodec_Lookup(encoding));
}

/*[clinic input]
_codecs.encode
    obj: object
    encoding: str(c_default="NULL") = "utf-8"
    errors: str(c_default="NULL") = "strict"

Encodes obj using the codec registered for encoding.

The default encoding is 'utf-8'.  errors may be given to set a
different error handling scheme.  Default is 'strict' meaning that
encoding errors raise a ValueError.  Other possible values are 'ignore',
'replace' and 'backslashreplace' as well as any other name registered
with codecs.register_error that can handle ValueErrors.
[clinic start generated code]*/

static PyValue
_codecs_encode_impl(PyObject *module, PyValueBorrow obj,
                    const char *encoding, const char *errors)
/*[clinic end generated code: output=d8d06186cf3a2ae8 input=e5271d443e391d7f]*/
{
    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Encode via the codec registry */
    PyObject *obj_obj = _PyValue_AsObjectAt(obj, BOX_SITE_PARSER);
    if (obj_obj == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = PyCodec_Encode(obj_obj, encoding, errors);
    Py_DECREF(obj_obj);
    return _PyValue_FromObjectResult(res);
}

/*[clinic input]
_codecs.decode
    obj: object
    encoding: str(c_default="NULL") = "utf-8"
    errors: str(c_default="NULL") = "strict"

Decodes obj using the codec registered for encoding.

Default encoding is 'utf-8'.  errors may be given to set a
different error handling scheme.  Default is 'strict' meaning that
encoding errors raise a ValueError.  Other possible values are 'ignore',
'replace' and 'backslashreplace' as well as any other name registered
with codecs.register_error that can handle ValueErrors.
[clinic start generated code]*/

static PyValue
_codecs_decode_impl(PyObject *module, PyValueBorrow obj,
                    const char *encoding, const char *errors)
/*[clinic end generated code: output=2c71e6b53ce558e7 input=3e6254628f9ca538]*/
{
    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Decode via the codec registry */
    PyObject *obj_obj = _PyValue_AsObjectAt(obj, BOX_SITE_PARSER);
    if (obj_obj == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = PyCodec_Decode(obj_obj, encoding, errors);
    Py_DECREF(obj_obj);
    return _PyValue_FromObjectResult(res);
}

/* --- Helpers ------------------------------------------------------------ */

static
PyObject *codec_tuple(PyObject *decoded,
                      Py_ssize_t len)
{
    if (decoded == NULL)
        return NULL;
    return Py_BuildValue("Nn", decoded, len);
}

/* --- String codecs ------------------------------------------------------ */
/*[clinic input]
_codecs.escape_decode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_escape_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors)
/*[clinic end generated code: output=0ae8d052c0a3b3ba input=77298a561c90bd82]*/
{
    PyObject *decoded = PyBytes_DecodeEscape(data->buf, data->len,
                                             errors, 0, NULL);
    return _PyValue_FromObjectResult(codec_tuple(decoded, data->len));
}

/*[clinic input]
_codecs.escape_encode
    data: object(subclass_of='&PyBytes_Type')
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_escape_encode_impl(PyObject *module, PyObject *data,
                           const char *errors)
/*[clinic end generated code: output=d0b93478394b3915 input=8f4b144799a94245]*/
{
    Py_ssize_t size = PyBytes_GET_SIZE(data);
    if (size > PY_SSIZE_T_MAX / 4) {
        PyErr_SetString(PyExc_OverflowError,
            "string is too large to encode");
            return PyValue_NULL;
    }
    Py_ssize_t newsize = 4*size;

    PyBytesWriter *writer = PyBytesWriter_Create(newsize);
    if (writer == NULL) {
        return PyValue_NULL;
    }
    char *p = PyBytesWriter_GetData(writer);

    for (Py_ssize_t i = 0; i < size; i++) {
        /* There's at least enough room for a hex escape */
        assert(newsize - (p - (char*)PyBytesWriter_GetData(writer)) >= 4);

        char c = PyBytes_AS_STRING(data)[i];
        if (c == '\'' || c == '\\') {
            *p++ = '\\'; *p++ = c;
        }
        else if (c == '\t') {
            *p++ = '\\'; *p++ = 't';
        }
        else if (c == '\n') {
            *p++ = '\\'; *p++ = 'n';
        }
        else if (c == '\r') {
            *p++ = '\\'; *p++ = 'r';
        }
        else if (c < ' ' || c >= 0x7f) {
            *p++ = '\\';
            *p++ = 'x';
            *p++ = Py_hexdigits[(c & 0xf0) >> 4];
            *p++ = Py_hexdigits[c & 0xf];
        }
        else {
            *p++ = c;
        }
    }

    PyObject *decoded = PyBytesWriter_FinishWithPointer(writer, p);
    return _PyValue_FromObjectResult(codec_tuple(decoded, size));
}

/* --- Decoder ------------------------------------------------------------ */
/*[clinic input]
_codecs.utf_7_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_7_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final)
/*[clinic end generated code: output=4ff8f4806eef7c42 input=dbf8c8998102dc7d]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF7Stateful(data->buf, data->len,
                                                     errors,
                                                     final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.utf_8_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_8_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final)
/*[clinic end generated code: output=ede7d8e18c8acce5 input=ca06bc8a9c970e25]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF8Stateful(data->buf, data->len,
                                                     errors,
                                                     final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.utf_16_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_16_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final)
/*[clinic end generated code: output=ee42cae5a439fad4 input=5b0f52071ba6cadc]*/
{
    int byteorder = 0;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF16Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.utf_16_le_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_16_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=6310a5ca705e521d input=115bd8c7b783d0bf]*/
{
    int byteorder = -1;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF16Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.utf_16_be_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_16_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=936ab8423864de08 input=63131422b01f9cb4]*/
{
    int byteorder = 1;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF16Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/* This non-standard version also provides access to the byteorder
   parameter of the builtin UTF-16 codec.

   It returns a tuple (unicode, bytesread, byteorder) with byteorder
   being the value in effect at the end of data.

*/
/*[clinic input]
_codecs.utf_16_ex_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    byteorder: int = 0
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_16_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final)
/*[clinic end generated code: output=ee0ee6b30730f72b input=f368a51cf384bf4c]*/
{
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;

    PyObject *decoded = PyUnicode_DecodeUTF16Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    if (decoded == NULL)
        return PyValue_NULL;
    return _PyValue_FromObjectResult(Py_BuildValue("Nni", decoded, consumed, byteorder));
}

/*[clinic input]
_codecs.utf_32_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_32_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final)
/*[clinic end generated code: output=0d0a275065160702 input=fcdf3658c5e9b5f3]*/
{
    int byteorder = 0;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF32Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.utf_32_le_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_32_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=854dc90565b6a0cf input=12220556e885f817]*/
{
    int byteorder = -1;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF32Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.utf_32_be_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_32_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=c73c3d31c8b5e629 input=2bc669b4781598db]*/
{
    int byteorder = 1;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF32Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/* This non-standard version also provides access to the byteorder
   parameter of the builtin UTF-32 codec.

   It returns a tuple (unicode, bytesread, byteorder) with byteorder
   being the value in effect at the end of data.

*/
/*[clinic input]
_codecs.utf_32_ex_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    byteorder: int = 0
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_32_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final)
/*[clinic end generated code: output=f56f8bdce4f38510 input=4a2323d0013620df]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF32Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    if (decoded == NULL)
        return PyValue_NULL;
    return _PyValue_FromObjectResult(Py_BuildValue("Nni", decoded, consumed, byteorder));
}

/*[clinic input]
_codecs.unicode_escape_decode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = None
    final: bool = True
    /
[clinic start generated code]*/

static PyValue
_codecs_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                   const char *errors, int final)
/*[clinic end generated code: output=fdc815f16f0fdc89 input=15019f081ffe272b]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = _PyUnicode_DecodeUnicodeEscapeStateful(data->buf, data->len,
                                                               errors,
                                                               final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.raw_unicode_escape_decode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = None
    final: bool = True
    /
[clinic start generated code]*/

static PyValue
_codecs_raw_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                       const char *errors, int final)
/*[clinic end generated code: output=180070973d14dbdd input=b93f823aa8c343ad]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = _PyUnicode_DecodeRawUnicodeEscapeStateful(data->buf, data->len,
                                                                  errors,
                                                                  final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.latin_1_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_latin_1_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors)
/*[clinic end generated code: output=c6c28177a828cbc5 input=76ca58fd6dcd08c7]*/
{
    PyObject *decoded = PyUnicode_DecodeLatin1(data->buf, data->len, errors);
    return _PyValue_FromObjectResult(codec_tuple(decoded, data->len));
}

/*[clinic input]
_codecs.ascii_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_ascii_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors)
/*[clinic end generated code: output=5fa4015f39e28840 input=e428a267a04b4481]*/
{
    PyObject *decoded = PyUnicode_DecodeASCII(data->buf, data->len, errors);
    return _PyValue_FromObjectResult(codec_tuple(decoded, data->len));
}

/*[clinic input]
_codecs.charmap_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    mapping: object = None
    /
[clinic start generated code]*/

static PyValue
_codecs_charmap_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors, PyValueBorrow mapping)
/*[clinic end generated code: output=edbe1bc5bd075cf2 input=15b69df43458eb40]*/
{
    PyObject *decoded;
    PyObject *mapping_obj = NULL;

    if (!PyValue_IsNone(mapping)) {
        mapping_obj = _PyValue_AsObjectAt(mapping, BOX_SITE_PARSER);
        if (mapping_obj == NULL)
            return PyValue_NULL;
    }

    decoded = PyUnicode_DecodeCharmap(data->buf, data->len, mapping_obj, errors);
    Py_XDECREF(mapping_obj);
    return _PyValue_FromObjectResult(codec_tuple(decoded, data->len));
}

#ifdef MS_WINDOWS

/*[clinic input]
_codecs.mbcs_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_mbcs_decode_impl(PyObject *module, Py_buffer *data,
                         const char *errors, int final)
/*[clinic end generated code: output=99f38b56b83f6e31 input=f144ad1ed6d8f5a6]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeMBCSStateful(data->buf, data->len,
            errors, final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.oem_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_oem_decode_impl(PyObject *module, Py_buffer *data,
                        const char *errors, int final)
/*[clinic end generated code: output=067394975b29ddf6 input=629bf87376d211b4]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeCodePageStateful(CP_OEMCP,
        data->buf, data->len, errors, final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

/*[clinic input]
_codecs.code_page_decode
    codepage: int
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_code_page_decode_impl(PyObject *module, int codepage,
                              Py_buffer *data, const char *errors, int final)
/*[clinic end generated code: output=5db9912e77365c68 input=6a32589b0658c277]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeCodePageStateful(codepage,
                                                         data->buf, data->len,
                                                         errors,
                                                         final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

#endif /* MS_WINDOWS */

#ifdef _Py_HAVE_ICONV

/*[clinic input]
_codecs.iconv_decode
    encoding: str
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool = False
    /
[clinic start generated code]*/

static PyValue
_codecs_iconv_decode_impl(PyObject *module, const char *encoding,
                          Py_buffer *data, const char *errors, int final)
/*[clinic end generated code: output=3f8d8b30c6cc591d input=d15a04d7d3a3e0cd]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = _PyUnicode_DecodeIconv(encoding, data->buf, data->len,
                                               errors,
                                               final ? NULL : &consumed);
    return _PyValue_FromObjectResult(codec_tuple(decoded, consumed));
}

#endif /* _Py_HAVE_ICONV */

/* --- Encoder ------------------------------------------------------------ */

/*[clinic input]
_codecs.readbuffer_encode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_readbuffer_encode_impl(PyObject *module, Py_buffer *data,
                               const char *errors)
/*[clinic end generated code: output=d0390c6cc7ccefa2 input=aa10cfdf252455c5]*/
{
    PyObject *result = PyBytes_FromStringAndSize(data->buf, data->len);
    return _PyValue_FromObjectResult(codec_tuple(result, data->len));
}

/*[clinic input]
_codecs.utf_7_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_7_encode_impl(PyObject *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=bb29fc1259ecae94 input=2546dbbb3fa53114]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_EncodeUTF7(str, errors),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.utf_8_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_8_encode_impl(PyObject *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=a719cea47f2721df input=a3e71ae01c3f93f3]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_AsUTF8String(str, errors),
                       PyUnicode_GET_LENGTH(str)));
}

/* This version provides access to the byteorder parameter of the
   builtin UTF-16 codecs as optional third argument. It defaults to 0
   which means: use the native byte order and prepend the data with a
   BOM mark.

*/

/*[clinic input]
_codecs.utf_16_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    byteorder: int = 0
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_16_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder)
/*[clinic end generated code: output=f7ffea151b5735bd input=68cdc2eb8338555d]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_EncodeUTF16(str, errors, byteorder),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.utf_16_le_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_16_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=5a696ec630fa3016 input=83d042706eed6798]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_EncodeUTF16(str, errors, -1),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.utf_16_be_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_16_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=fb2dee6809ba02b5 input=6f1e9e623b03071b]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_EncodeUTF16(str, errors, +1),
                       PyUnicode_GET_LENGTH(str)));
}

/* This version provides access to the byteorder parameter of the
   builtin UTF-32 codecs as optional third argument. It defaults to 0
   which means: use the native byte order and prepend the data with a
   BOM mark.

*/

/*[clinic input]
_codecs.utf_32_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    byteorder: int = 0
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_32_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder)
/*[clinic end generated code: output=3fb85f67493dcbd2 input=8ec4c64d983bc52b]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_EncodeUTF32(str, errors, byteorder),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.utf_32_le_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_32_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=b917d6694ed2b766 input=f0918d41de3eb1b1]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_EncodeUTF32(str, errors, -1),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.utf_32_be_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_utf_32_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=55cd812521a87551 input=967a99a95748b557]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_EncodeUTF32(str, errors, +1),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.unicode_escape_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                   const char *errors)
/*[clinic end generated code: output=16362fb888b85612 input=8c4de07597054e33]*/
{
    return _PyValue_FromObjectResult(codec_tuple(PyUnicode_AsUnicodeEscapeString(str),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.raw_unicode_escape_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_raw_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                       const char *errors)
/*[clinic end generated code: output=01a8795c26267466 input=4aa6f280d78e4574]*/
{
    return _PyValue_FromObjectResult(codec_tuple(PyUnicode_AsRawUnicodeEscapeString(str),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.latin_1_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_latin_1_encode_impl(PyObject *module, PyObject *str,
                            const char *errors)
/*[clinic end generated code: output=844a186912d293e7 input=ec3ef74bf85c5c5d]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_AsLatin1String(str, errors),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.ascii_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_ascii_encode_impl(PyObject *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=aad42857416ffe89 input=93e6e602838bd3de]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_AsASCIIString(str, errors),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.charmap_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    mapping: object = None
    /
[clinic start generated code]*/

static PyValue
_codecs_charmap_encode_impl(PyObject *module, PyObject *str,
                            const char *errors, PyValueBorrow mapping)
/*[clinic end generated code: output=97444a75dc855e8d input=2a98feae73dadce8]*/
{
    PyObject *mapping_obj = NULL;

    if (!PyValue_IsNone(mapping)) {
        mapping_obj = _PyValue_AsObjectAt(mapping, BOX_SITE_PARSER);
        if (mapping_obj == NULL)
            return PyValue_NULL;
    }

    PyObject *encoded = _PyUnicode_EncodeCharmap(str, mapping_obj, errors);
    Py_XDECREF(mapping_obj);
    return _PyValue_FromObjectResult(codec_tuple(encoded,
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.charmap_build
    map: unicode
    /
[clinic start generated code]*/

static PyValue
_codecs_charmap_build_impl(PyObject *module, PyObject *map)
/*[clinic end generated code: output=ed2bd132f8e14f14 input=d91a91d1717dbc6d]*/
{
    return _PyValue_FromObjectResult(PyUnicode_BuildEncodingMap(map));
}

#ifdef MS_WINDOWS

/*[clinic input]
_codecs.mbcs_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_mbcs_encode_impl(PyObject *module, PyObject *str, const char *errors)
/*[clinic end generated code: output=7e986d57fca00533 input=2e932fc289ea5a5b]*/
{
    return _PyValue_FromObjectResult(codec_tuple(PyUnicode_EncodeCodePage(CP_ACP, str, errors),
                       PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.oem_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_oem_encode_impl(PyObject *module, PyObject *str, const char *errors)
/*[clinic end generated code: output=ca8b3d02603a840f input=9eac86dc21eb14f2]*/
{
    return _PyValue_FromObjectResult(codec_tuple(PyUnicode_EncodeCodePage(CP_OEMCP, str, errors),
        PyUnicode_GET_LENGTH(str)));
}

/*[clinic input]
_codecs.code_page_encode
    code_page: int
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_code_page_encode_impl(PyObject *module, int code_page, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=7c594de8e49e5351 input=7d18a33bc8cd0f94]*/
{
    return _PyValue_FromObjectResult(codec_tuple(PyUnicode_EncodeCodePage(code_page, str, errors),
                       PyUnicode_GET_LENGTH(str)));
}

#endif /* MS_WINDOWS */

#ifdef _Py_HAVE_ICONV

/*[clinic input]
_codecs.iconv_encode
    encoding: str
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyValue
_codecs_iconv_encode_impl(PyObject *module, const char *encoding,
                          PyObject *str, const char *errors)
/*[clinic end generated code: output=06bcacad5680f497 input=53a1e2028c9e2b43]*/
{
    return _PyValue_FromObjectResult(codec_tuple(_PyUnicode_EncodeIconv(encoding, str, errors),
                       PyUnicode_GET_LENGTH(str)));
}

#endif /* _Py_HAVE_ICONV */

/* --- Error handler registry --------------------------------------------- */

/*[clinic input]
_codecs.register_error
    errors: str
    handler: object
    /

Register the specified error handler under the name errors.

handler must be a callable object, that will be called with an exception
instance containing information about the location of the
encoding/decoding error and must return a (replacement, new position)
tuple.
[clinic start generated code]*/

static PyValue
_codecs_register_error_impl(PyObject *module, const char *errors,
                            PyValueBorrow handler)
/*[clinic end generated code: output=4e2dfc3fb0156ac2 input=5bea01dfe835d9d8]*/
{
    PyObject *handler_obj = _PyValue_AsObjectAt(handler, BOX_SITE_PARSER);
    if (handler_obj == NULL)
        return PyValue_NULL;
    int rc = PyCodec_RegisterError(errors, handler_obj);
    Py_DECREF(handler_obj);
    if (rc)
        return PyValue_NULL;
    return PyValue_None;
}

/*[clinic input]
_codecs._unregister_error -> bool
    errors: str
    /

Un-register the specified error handler for the error handling `errors'.

Only custom error handlers can be un-registered. An exception is raised
if the error handling is a built-in one (e.g., 'strict'), or if an error
occurs.

Otherwise, this returns True if a custom handler has been successfully
un-registered, and False if no custom handler for the specified error
handling exists.

[clinic start generated code]*/

static int
_codecs__unregister_error_impl(PyObject *module, const char *errors)
/*[clinic end generated code: output=28c22be667465503 input=a63ab9e9ce1686d4]*/
{
    return _PyCodec_UnregisterError(errors);
}

/*[clinic input]
_codecs.lookup_error
    name: str
    /

lookup_error(errors) -> handler

Return the error handler for the specified error handling name or raise
a LookupError, if no handler exists under this name.
[clinic start generated code]*/

static PyValue
_codecs_lookup_error_impl(PyObject *module, const char *name)
/*[clinic end generated code: output=a79e30ef36e4a76f input=86cfb6a7a9c67113]*/
{
    return _PyValue_FromObjectResult(PyCodec_LookupError(name));
}

extern int _Py_normalize_encoding(const char *, char *, size_t, int);

/*[clinic input]
_codecs._normalize_encoding
    encoding: unicode

Normalize an encoding name *encoding*.

Used for encodings.normalize_encoding. Does not convert to lower case.
[clinic start generated code]*/

static PyValue
_codecs__normalize_encoding_impl(PyObject *module, PyObject *encoding)
/*[clinic end generated code: output=d2ee217dd8ebd8db input=3ff3f4d64995b988]*/
{
    Py_ssize_t len;
    const char *cstr = PyUnicode_AsUTF8AndSize(encoding, &len);
    if (cstr == NULL) {
        return PyValue_NULL;
    }

    if (len > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError, "encoding is too large");
        return PyValue_NULL;
    }

    char *normalized = PyMem_Malloc(len + 1);
    if (normalized == NULL) {
        return _PyValue_FromObjectResult(PyErr_NoMemory());
    }

    if (!_Py_normalize_encoding(cstr, normalized, len + 1, 0)) {
        PyMem_Free(normalized);
        return PyValue_NULL;
    }

    PyObject *result = PyUnicode_FromString(normalized);
    PyMem_Free(normalized);
    return _PyValue_FromObjectResult(result);
}

/* --- Module API --------------------------------------------------------- */

static PyMethodDef _codecs_functions[] = {
    _CODECS_REGISTER_METHODDEF
    _CODECS_UNREGISTER_METHODDEF
    _CODECS_LOOKUP_METHODDEF
    _CODECS_ENCODE_METHODDEF
    _CODECS_DECODE_METHODDEF
    _CODECS_ESCAPE_ENCODE_METHODDEF
    _CODECS_ESCAPE_DECODE_METHODDEF
    _CODECS_UTF_8_ENCODE_METHODDEF
    _CODECS_UTF_8_DECODE_METHODDEF
    _CODECS_UTF_7_ENCODE_METHODDEF
    _CODECS_UTF_7_DECODE_METHODDEF
    _CODECS_UTF_16_ENCODE_METHODDEF
    _CODECS_UTF_16_LE_ENCODE_METHODDEF
    _CODECS_UTF_16_BE_ENCODE_METHODDEF
    _CODECS_UTF_16_DECODE_METHODDEF
    _CODECS_UTF_16_LE_DECODE_METHODDEF
    _CODECS_UTF_16_BE_DECODE_METHODDEF
    _CODECS_UTF_16_EX_DECODE_METHODDEF
    _CODECS_UTF_32_ENCODE_METHODDEF
    _CODECS_UTF_32_LE_ENCODE_METHODDEF
    _CODECS_UTF_32_BE_ENCODE_METHODDEF
    _CODECS_UTF_32_DECODE_METHODDEF
    _CODECS_UTF_32_LE_DECODE_METHODDEF
    _CODECS_UTF_32_BE_DECODE_METHODDEF
    _CODECS_UTF_32_EX_DECODE_METHODDEF
    _CODECS_UNICODE_ESCAPE_ENCODE_METHODDEF
    _CODECS_UNICODE_ESCAPE_DECODE_METHODDEF
    _CODECS_RAW_UNICODE_ESCAPE_ENCODE_METHODDEF
    _CODECS_RAW_UNICODE_ESCAPE_DECODE_METHODDEF
    _CODECS_LATIN_1_ENCODE_METHODDEF
    _CODECS_LATIN_1_DECODE_METHODDEF
    _CODECS_ASCII_ENCODE_METHODDEF
    _CODECS_ASCII_DECODE_METHODDEF
    _CODECS_CHARMAP_ENCODE_METHODDEF
    _CODECS_CHARMAP_DECODE_METHODDEF
    _CODECS_CHARMAP_BUILD_METHODDEF
    _CODECS_READBUFFER_ENCODE_METHODDEF
    _CODECS_MBCS_ENCODE_METHODDEF
    _CODECS_MBCS_DECODE_METHODDEF
    _CODECS_OEM_ENCODE_METHODDEF
    _CODECS_OEM_DECODE_METHODDEF
    _CODECS_CODE_PAGE_ENCODE_METHODDEF
    _CODECS_CODE_PAGE_DECODE_METHODDEF
    _CODECS_ICONV_ENCODE_METHODDEF
    _CODECS_ICONV_DECODE_METHODDEF
    _CODECS_REGISTER_ERROR_METHODDEF
    _CODECS__UNREGISTER_ERROR_METHODDEF
    _CODECS_LOOKUP_ERROR_METHODDEF
    _CODECS__NORMALIZE_ENCODING_METHODDEF
    {NULL, NULL}                /* sentinel */
};

static PyModuleDef_Slot _codecs_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef codecsmodule = {
        PyModuleDef_HEAD_INIT,
        "_codecs",
        NULL,
        0,
        _codecs_functions,
        _codecs_slots,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__codecs(void)
{
    return PyModuleDef_Init(&codecsmodule);
}
