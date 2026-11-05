#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#include "Python.h"
#include <stddef.h>               // offsetof()
#include "pycore_long_value.h"    // _PyValue_AsLong()
#include "pycore_object.h"
#include "pycore_tuple.h"         // PyTuple_GetValue()
#include "pycore_value.h"        // PyValue_AsKnownObjectBorrow()
#include "pycore_value_ops.h"     // _PyValue_GetAttr()
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()
#include "_iomodule.h"

/* Implementation note: the buffer is always at least one character longer
   than the enclosed string, for proper functioning of _PyIO_find_line_ending.
*/

#define STATE_REALIZED 1
#define STATE_ACCUMULATING 2

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _io
class _io.StringIO "stringio *" "clinic_state()->PyStringIO_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2693eada0658d470]*/

typedef struct {
    PyObject_HEAD
    Py_UCS4 *buf;
    Py_ssize_t pos;
    Py_ssize_t string_size;
    size_t buf_size;

    /* The stringio object can be in two states: accumulating or realized.
       In accumulating state, the internal buffer contains nothing and
       the contents are given by the embedded _PyUnicodeWriter structure.
       In realized state, the internal buffer is meaningful and the
       _PyUnicodeWriter is destroyed.
    */
    int state;
    PyUnicodeWriter *writer;

    char ok; /* initialized? */
    char closed;
    char readuniversal;
    char readtranslate;
    PyObject *decoder;
    PyObject *readnl;
    PyObject *writenl;

    PyObject *dict;
    PyObject *weakreflist;
    _PyIO_State *module_state;
} stringio;

#define stringio_CAST(op)   ((stringio *)(op))

#define clinic_state() (find_io_state_by_def(Py_TYPE(self)))
#include "clinic/stringio.c.h"
#undef clinic_state

static int _io_StringIO___init__(PyObject *self, PyObject *args, PyObject *kwargs);

#define CHECK_INITIALIZED(self, FAILURE) \
    if (self->ok <= 0) { \
        PyErr_SetString(PyExc_ValueError, \
            "I/O operation on uninitialized object"); \
        return FAILURE; \
    }

#define CHECK_CLOSED(self, FAILURE) \
    if (self->closed) { \
        PyErr_SetString(PyExc_ValueError, \
            "I/O operation on closed file"); \
        return FAILURE; \
    }

#define CHECK_INITIALIZED_INT(self) \
    if (self->ok <= 0) { \
        PyErr_SetString(PyExc_ValueError, \
            "I/O operation on uninitialized object"); \
        return -1; \
    }

#define CHECK_CLOSED_INT(self) \
    if (self->closed) { \
        PyErr_SetString(PyExc_ValueError, \
            "I/O operation on closed file"); \
        return -1; \
    }

#define ENSURE_REALIZED(self, FAILURE) \
    if (realize(self) < 0) { \
        return FAILURE; \
    }


/* Internal routine for changing the size, in terms of characters, of the
   buffer of StringIO objects.  The caller should ensure that the 'size'
   argument is non-negative.  Returns 0 on success, -1 otherwise. */
static int
resize_buffer(stringio *self, size_t size)
{
    /* Here, unsigned types are used to avoid dealing with signed integer
       overflow, which is undefined in C. */
    size_t alloc = self->buf_size;
    Py_UCS4 *new_buf = NULL;

    assert(self->buf != NULL);

    /* Reserve one more char for line ending detection. */
    size = size + 1;
    /* For simplicity, stay in the range of the signed type. Anyway, Python
       doesn't allow strings to be longer than this. */
    if (size > PY_SSIZE_T_MAX)
        goto overflow;

    if (size < alloc / 2) {
        /* Major downsize; resize down to exact size. */
        alloc = size + 1;
    }
    else if (size < alloc) {
        /* Within allocated size; quick exit */
        return 0;
    }
    else if (size <= alloc * 1.125) {
        /* Moderate upsize; overallocate similar to list_resize() */
        alloc = size + (size >> 3) + (size < 9 ? 3 : 6);
    }
    else {
        /* Major upsize; resize up to exact size */
        alloc = size + 1;
    }

    if (alloc > SIZE_MAX / sizeof(Py_UCS4))
        goto overflow;
    new_buf = (Py_UCS4 *)PyMem_Realloc(self->buf, alloc * sizeof(Py_UCS4));
    if (new_buf == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->buf_size = alloc;
    self->buf = new_buf;

    return 0;

  overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "new buffer size too large");
    return -1;
}

static PyObject *
make_intermediate(stringio *self)
{
    PyObject *intermediate = PyUnicodeWriter_Finish(self->writer);
    self->writer = NULL;
    self->state = STATE_REALIZED;
    if (intermediate == NULL)
        return NULL;

    self->writer = PyUnicodeWriter_Create(0);
    if (self->writer == NULL) {
        Py_DECREF(intermediate);
        return NULL;
    }
    if (PyUnicodeWriter_WriteStr(self->writer, intermediate)) {
        Py_DECREF(intermediate);
        return NULL;
    }
    self->state = STATE_ACCUMULATING;
    return intermediate;
}

static int
realize(stringio *self)
{
    Py_ssize_t len;
    PyObject *intermediate;

    if (self->state == STATE_REALIZED)
        return 0;
    assert(self->state == STATE_ACCUMULATING);
    self->state = STATE_REALIZED;

    intermediate = PyUnicodeWriter_Finish(self->writer);
    self->writer = NULL;
    if (intermediate == NULL)
        return -1;

    /* Append the intermediate string to the internal buffer.
       The length should be equal to the current cursor position.
     */
    len = PyUnicode_GET_LENGTH(intermediate);
    if (resize_buffer(self, len) < 0) {
        Py_DECREF(intermediate);
        return -1;
    }
    if (!PyUnicode_AsUCS4(intermediate, self->buf, len, 0)) {
        Py_DECREF(intermediate);
        return -1;
    }

    Py_DECREF(intermediate);
    return 0;
}

/* Internal routine for writing a whole PyUnicode object to the buffer of a
   StringIO object. Returns 0 on success, or -1 on error. */
static Py_ssize_t
write_str(stringio *self, PyObject *obj)
{
    Py_ssize_t len;
    PyObject *decoded = NULL;

    assert(self->buf != NULL);
    assert(self->pos >= 0);

    if (self->decoder != NULL) {
        decoded = _PyIncrementalNewlineDecoder_decode(
            self->decoder, obj, 1 /* always final */);
    }
    else {
        decoded = Py_NewRef(obj);
    }
    if (self->writenl) {
        PyObject *translated = PyUnicode_Replace(
            decoded, _Py_LATIN1_CHR('\n'), self->writenl, -1);
        Py_SETREF(decoded, translated);
    }
    if (decoded == NULL)
        return -1;

    assert(PyUnicode_Check(decoded));
    len = PyUnicode_GET_LENGTH(decoded);
    assert(len >= 0);

    /* This overflow check is not strictly necessary. However, it avoids us to
       deal with funky things like comparing an unsigned and a signed
       integer. */
    if (self->pos > PY_SSIZE_T_MAX - len) {
        PyErr_SetString(PyExc_OverflowError,
                        "new position too large");
        goto fail;
    }

    if (self->state == STATE_ACCUMULATING) {
        if (self->string_size == self->pos) {
            // gh-149046: Avoid PyUnicodeWriter_WriteStr() which calls str(obj)
            // on str subclasses
            if (_PyUnicodeWriter_WriteStr((_PyUnicodeWriter*)self->writer, decoded))
                goto fail;
            goto success;
        }
        if (realize(self))
            goto fail;
    }

    if (self->pos + len > self->string_size) {
        if (resize_buffer(self, self->pos + len) < 0)
            goto fail;
    }

    if (self->pos > self->string_size) {
        /* In case of overseek, pad with null bytes the buffer region between
           the end of stream and the current position.

          0   lo      string_size                           hi
          |   |<---used--->|<----------available----------->|
          |   |            <--to pad-->|<---to write--->    |
          0   buf                   position

        */
        memset(self->buf + self->string_size, '\0',
               (self->pos - self->string_size) * sizeof(Py_UCS4));
    }

    /* Copy the data to the internal buffer, overwriting some of the
       existing data if self->pos < self->string_size. */
    if (!PyUnicode_AsUCS4(decoded,
                          self->buf + self->pos,
                          self->buf_size - self->pos,
                          0))
        goto fail;

success:
    /* Set the new length of the internal string if it has changed. */
    self->pos += len;
    if (self->string_size < self->pos)
        self->string_size = self->pos;

    Py_DECREF(decoded);
    return 0;

fail:
    Py_XDECREF(decoded);
    return -1;
}

/*[clinic input]
@critical_section
_io.StringIO.getvalue

Retrieve the entire contents of the object.
[clinic start generated code]*/

static PyValue
_io_StringIO_getvalue_impl(stringio *self)
/*[clinic end generated code: output=0076474127d2599e input=fb5dee06b8d467f3]*/
{
    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);
    if (self->state == STATE_ACCUMULATING)
        return _PyValue_FromObjectResult(make_intermediate(self));
    return _PyValue_FromObjectResult(PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, self->buf,
                                     self->string_size));
}

/*[clinic input]
@critical_section
_io.StringIO.tell

Tell the current file position.
[clinic start generated code]*/

static PyValue
_io_StringIO_tell_impl(stringio *self)
/*[clinic end generated code: output=f2a67301ff66ec64 input=98a08f3e2dae3550]*/
{
    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);
    return PyValue_FromInt64(self->pos);
}

/*[clinic input]
@critical_section
_io.StringIO.read
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /

Read at most size characters, returned as a string.

If the argument is negative or omitted, read until EOF
is reached. Return an empty string at EOF.
[clinic start generated code]*/

static PyValue
_io_StringIO_read_impl(stringio *self, Py_ssize_t size)
/*[clinic end generated code: output=41ae609c955c5a54 input=9fbef45d8aece8e7]*/
{
    Py_ssize_t n;
    Py_UCS4 *output;

    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);

    /* adjust invalid sizes */
    n = self->string_size - self->pos;
    if (size < 0 || size > n) {
        size = n;
        if (size < 0)
            size = 0;
    }

    /* Optimization for seek(0); read() */
    if (self->state == STATE_ACCUMULATING && self->pos == 0 && size == n) {
        PyObject *result = make_intermediate(self);
        self->pos = self->string_size;
        return _PyValue_FromObjectResult(result);
    }

    ENSURE_REALIZED(self, PyValue_NULL);
    output = self->buf + self->pos;
    self->pos += size;
    return _PyValue_FromObjectResult(PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, output, size));
}

/* Internal helper, used by stringio_readline and stringio_iternext */
static PyObject *
_stringio_readline(stringio *self, Py_ssize_t limit)
{
    Py_UCS4 *start, *end, old_char;
    Py_ssize_t len, consumed;

    /* In case of overseek, return the empty string */
    if (self->pos >= self->string_size)
        return Py_GetConstant(Py_CONSTANT_EMPTY_STR);

    start = self->buf + self->pos;
    if (limit < 0 || limit > self->string_size - self->pos)
        limit = self->string_size - self->pos;

    end = start + limit;
    old_char = *end;
    *end = '\0';
    len = _PyIO_find_line_ending(
        self->readtranslate, self->readuniversal, self->readnl,
        PyUnicode_4BYTE_KIND, (char*)start, (char*)end, &consumed);
    *end = old_char;
    /* If we haven't found any line ending, we just return everything
       (`consumed` is ignored). */
    if (len < 0)
        len = limit;
    self->pos += len;
    return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, start, len);
}

/*[clinic input]
@critical_section
_io.StringIO.readline
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /

Read until newline or EOF.

Returns an empty string if EOF is hit immediately.
[clinic start generated code]*/

static PyValue
_io_StringIO_readline_impl(stringio *self, Py_ssize_t size)
/*[clinic end generated code: output=98999240aa896337 input=4d14b8495dea1d98]*/
{
    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);
    ENSURE_REALIZED(self, PyValue_NULL);

    return _PyValue_FromObjectResult(_stringio_readline(self, size));
}

/*[clinic input]
@critical_section
_io.StringIO.__next__ as stringio_iternext
[clinic start generated code]*/

static PyObject *
stringio_iternext_impl(stringio *self)
/*[clinic end generated code: output=51eb3991b1c79c41 input=b6cf3e73fab6b42e]*/
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(self);

    PyObject *line;

    CHECK_INITIALIZED(self, NULL);
    CHECK_CLOSED(self, NULL);
    ENSURE_REALIZED(self, NULL);

    if (Py_IS_TYPE(self, self->module_state->PyStringIO_Type)) {
        /* Skip method call overhead for speed */
        line = _stringio_readline(self, -1);
    }
    else {
        /* XXX is subclassing StringIO really supported? */
        line = PyObject_CallMethodNoArgs((PyObject *)self, &_Py_ID(readline));
        if (line && !PyUnicode_Check(line)) {
            PyErr_Format(PyExc_OSError,
                         "readline() should have returned a str object, "
                         "not '%.200s'", Py_TYPE(line)->tp_name);
            Py_DECREF(line);
            return NULL;
        }
    }

    if (line == NULL)
        return NULL;

    if (PyUnicode_GET_LENGTH(line) == 0) {
        /* Reached EOF */
        Py_DECREF(line);
        return NULL;
    }

    return line;
}

/*[clinic input]
@critical_section
_io.StringIO.truncate
    pos: object = None
    /

Truncate size to pos.

The pos argument defaults to the current file position, as
returned by tell().  The current file position is unchanged.
Returns the new absolute position.
[clinic start generated code]*/

static PyValue
_io_StringIO_truncate_impl(stringio *self, PyValueBorrow pos)
/*[clinic end generated code: output=dbe05c838cf64c38 input=d59fd2ee49757ae6]*/
{
    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);

    Py_ssize_t size;
    if (PyValue_IsNone(pos)) {
        size = self->pos;
    }
    else {
        size = _PyValue_AsLong(pos);
        if (size == -1 && PyErr_Occurred()) {
            return PyValue_NULL;
        }
        if (size < 0) {
            PyErr_Format(PyExc_ValueError,
                         "negative pos value %zd", size);
            return PyValue_NULL;
        }
    }

    if (size < self->string_size) {
        ENSURE_REALIZED(self, PyValue_NULL);
        if (resize_buffer(self, size) < 0)
            return PyValue_NULL;
        self->string_size = size;
    }

    return PyValue_FromInt64(size);
}

/*[clinic input]
@critical_section
_io.StringIO.seek
    pos: Py_ssize_t
    whence: int = 0
    /

Change stream position.

Seek to character offset pos relative to position indicated by
whence:
    0  Start of stream (the default).  pos should be >= 0;
    1  Current position - pos must be 0;
    2  End of stream - pos must be 0.
Returns the new absolute position.
[clinic start generated code]*/

static PyValue
_io_StringIO_seek_impl(stringio *self, Py_ssize_t pos, int whence)
/*[clinic end generated code: output=e2061cc74cbd254d input=ffef24668fd71a5d]*/
{
    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);

    if (whence != 0 && whence != 1 && whence != 2) {
        PyErr_Format(PyExc_ValueError,
                     "Invalid whence (%i, should be 0, 1 or 2)", whence);
        return PyValue_NULL;
    }
    else if (pos < 0 && whence == 0) {
        PyErr_Format(PyExc_ValueError,
                     "Negative seek position %zd", pos);
        return PyValue_NULL;
    }
    else if (whence != 0 && pos != 0) {
        PyErr_SetString(PyExc_OSError,
                        "Can't do nonzero cur-relative seeks");
        return PyValue_NULL;
    }

    /* whence = 0: offset relative to beginning of the string.
       whence = 1: no change to current position.
       whence = 2: change position to end of file. */
    if (whence == 1) {
        pos = self->pos;
    }
    else if (whence == 2) {
        pos = self->string_size;
    }

    self->pos = pos;

    return PyValue_FromInt64(self->pos);
}

/*[clinic input]
@critical_section
_io.StringIO.write
    s as obj: object
    /

Write string to file.

Returns the number of characters written, which is always equal to
the length of the string.
[clinic start generated code]*/

static PyValue
_io_StringIO_write_impl(stringio *self, PyValueBorrow obj)
/*[clinic end generated code: output=a3fda62aca3eef5b input=1561272c0da4651f]*/
{
    Py_ssize_t size;

    CHECK_INITIALIZED(self, PyValue_NULL);
    if (!(PyValue_IsObject(obj) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(obj)))) {
        PyErr_Format(PyExc_TypeError, "string argument expected, got '%s'",
                     PyValue_TYPE(obj)->tp_name);
        return PyValue_NULL;
    }
    CHECK_CLOSED(self, PyValue_NULL);
    size = PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(obj));

    if (size > 0 && write_str(self, PyValue_AsKnownObjectBorrow(obj)) < 0)
        return PyValue_NULL;

    return PyValue_FromInt64(size);
}

/*[clinic input]
@critical_section
_io.StringIO.close

Close the IO object.

Attempting any further operation after the object is closed
will raise a ValueError.

This method has no effect if the file is already closed.
[clinic start generated code]*/

static PyValue
_io_StringIO_close_impl(stringio *self)
/*[clinic end generated code: output=bd394299d979dbd2 input=305d19aa29cc40b9]*/
{
    self->closed = 1;
    /* Free up some memory */
    if (resize_buffer(self, 0) < 0)
        return PyValue_NULL;
    PyUnicodeWriter_Discard(self->writer);
    self->writer = NULL;
    Py_CLEAR(self->readnl);
    Py_CLEAR(self->writenl);
    Py_CLEAR(self->decoder);
    return PyValue_None;
}

static int
stringio_traverse(PyObject *op, visitproc visit, void *arg)
{
    stringio *self = stringio_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->readnl);
    Py_VISIT(self->writenl);
    Py_VISIT(self->decoder);
    Py_VISIT(self->dict);
    // The accumulating writer lives in PyMem memory, which the tracing
    // mark does not scan; the string it builds is not a cycle member.
    if (self->writer != NULL) {
        Py_VISIT(((_PyUnicodeWriter *)self->writer)->buffer);
    }
    return 0;
}

static int
stringio_clear(PyObject *op)
{
    stringio *self = stringio_CAST(op);
    Py_CLEAR(self->readnl);
    Py_CLEAR(self->writenl);
    Py_CLEAR(self->decoder);
    Py_CLEAR(self->dict);
    return 0;
}

static void
stringio_dealloc(PyObject *op)
{
    stringio *self = stringio_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    _PyObject_GC_UNTRACK(self);
    self->ok = 0;
    if (self->buf) {
        PyMem_Free(self->buf);
        self->buf = NULL;
    }
    PyUnicodeWriter_Discard(self->writer);
    (void)stringio_clear(op);
    FT_CLEAR_WEAKREFS(op, self->weakreflist);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
stringio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    stringio *self;

    assert(type != NULL && type->tp_alloc != NULL);
    self = (stringio *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    /* tp_alloc initializes all the fields to zero. So we don't have to
       initialize them here. */

    self->buf = (Py_UCS4 *)PyMem_Malloc(0);
    if (self->buf == NULL) {
        Py_DECREF(self);
        return PyErr_NoMemory();
    }

    return (PyObject *)self;
}

/*[clinic input]
_io.StringIO.__init__
    initial_value as value: object(c_default="NULL") = ''
    newline as newline_obj: object(c_default="NULL") = '\n'

Text I/O implementation using an in-memory buffer.

The initial_value argument sets the value of object.  The newline
argument is like the one of TextIOWrapper's constructor.
[clinic start generated code]*/

static int
_io_StringIO___init___impl(stringio *self, PyValueBorrow value,
                           PyValueBorrow newline_obj)
/*[clinic end generated code: output=c98d465a95e99491 input=cee2d9181b2577a3]*/
{
    const char *newline = "\n";
    Py_ssize_t value_len;

    /* Parse the newline argument. We only want to allow unicode objects or
       None. */
    if (PyValue_IsNone(newline_obj)) {
        newline = NULL;
    }
    else if (!PyValue_IsNull(newline_obj)) {
        if (!(PyValue_IsObject(newline_obj) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(newline_obj)))) {
            PyErr_Format(PyExc_TypeError,
                         "newline must be str or None, not %.200s",
                         PyValue_TYPE(newline_obj)->tp_name);
            return -1;
        }
        newline = PyUnicode_AsUTF8(PyValue_AsKnownObjectBorrow(newline_obj));
        if (newline == NULL)
            return -1;
    }

    if (newline && newline[0] != '\0'
        && !(newline[0] == '\n' && newline[1] == '\0')
        && !(newline[0] == '\r' && newline[1] == '\0')
        && !(newline[0] == '\r' && newline[1] == '\n' && newline[2] == '\0')) {
        PyErr_Format(PyExc_ValueError,
                     "illegal newline value: %R",
                     PyValue_AsKnownObjectBorrow(newline_obj));
        return -1;
    }
    if (!PyValue_IsNull(value) && !PyValue_IsNone(value) && !(PyValue_IsObject(value) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(value)))) {
        PyErr_Format(PyExc_TypeError,
                     "initial_value must be str or None, not %.200s",
                     PyValue_TYPE(value)->tp_name);
        return -1;
    }

    self->ok = 0;

    PyUnicodeWriter_Discard(self->writer);
    self->writer = NULL;
    Py_CLEAR(self->readnl);
    Py_CLEAR(self->writenl);
    Py_CLEAR(self->decoder);

    assert((newline != NULL && !PyValue_IsNone(newline_obj)) ||
           (newline == NULL && PyValue_IsNone(newline_obj)));

    if (newline) {
        self->readnl = PyUnicode_FromString(newline);
        if (self->readnl == NULL)
            return -1;
    }
    self->readuniversal = (newline == NULL || newline[0] == '\0');
    self->readtranslate = (newline == NULL);
    /* If newline == "", we don't translate anything.
       If newline == "\n" or newline == None, we translate to "\n", which is
       a no-op.
       (for newline == None, TextIOWrapper translates to os.linesep, but it
       is pointless for StringIO)
    */
    if (newline != NULL && newline[0] == '\r') {
        self->writenl = Py_NewRef(self->readnl);
    }

    _PyIO_State *module_state = find_io_state_by_def(Py_TYPE(self));
    if (self->readuniversal) {
        self->decoder = PyObject_CallFunctionObjArgs(
            (PyObject *)module_state->PyIncrementalNewlineDecoder_Type,
            Py_None, self->readtranslate ? Py_True : Py_False, NULL);
        if (self->decoder == NULL)
            return -1;
    }

    /* Now everything is set up, resize buffer to size of initial value,
       and copy it */
    self->string_size = 0;
    if (!PyValue_IsNull(value) && !PyValue_IsNone(value))
        value_len = PyUnicode_GetLength(PyValue_AsKnownObjectBorrow(value));
    else
        value_len = 0;
    if (value_len > 0) {
        /* This is a heuristic, for newline translation might change
           the string length. */
        if (resize_buffer(self, 0) < 0)
            return -1;
        self->state = STATE_REALIZED;
        self->pos = 0;
        if (write_str(self, PyValue_AsKnownObjectBorrow(value)) < 0)
            return -1;
    }
    else {
        /* Empty stringio object, we can start by accumulating */
        if (resize_buffer(self, 0) < 0)
            return -1;
        self->writer = PyUnicodeWriter_Create(0);
        if (self->writer == NULL) {
            return -1;
        }
        self->state = STATE_ACCUMULATING;
    }
    self->pos = 0;
    self->module_state = module_state;
    self->closed = 0;
    self->ok = 1;
    return 0;
}

/* Properties and pseudo-properties */

/*[clinic input]
@critical_section
_io.StringIO.readable

Returns True if the IO object can be read.
[clinic start generated code]*/

static PyValue
_io_StringIO_readable_impl(stringio *self)
/*[clinic end generated code: output=c9fb9b3b717a5887 input=6cd2ffd65a8e8763]*/
{
    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);
    return PyValue_True;
}

/*[clinic input]
@critical_section
_io.StringIO.writable

Returns True if the IO object can be written.
[clinic start generated code]*/

static PyValue
_io_StringIO_writable_impl(stringio *self)
/*[clinic end generated code: output=63c1414112a1ecaa input=1b3c63dbaa761c69]*/
{
    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);
    return PyValue_True;
}

/*[clinic input]
@critical_section
_io.StringIO.seekable

Returns True if the IO object can be seeked.
[clinic start generated code]*/

static PyValue
_io_StringIO_seekable_impl(stringio *self)
/*[clinic end generated code: output=686337a1184d9fbd input=a820fad2cf085fc3]*/
{
    CHECK_INITIALIZED(self, PyValue_NULL);
    CHECK_CLOSED(self, PyValue_NULL);
    return PyValue_True;
}

/* Pickling support.

   The implementation of __getstate__ is similar to the one for BytesIO,
   except that we also save the newline parameter. For __setstate__ and unlike
   BytesIO, we call __init__ to restore the object's state. Doing so allows us
   to avoid decoding the complex newline state while keeping the object
   representation compact.

   See comment in bytesio.c regarding why only pickle protocols and onward are
   supported.
*/

/*[clinic input]
@critical_section
_io.StringIO.__getstate__

[clinic start generated code]*/

static PyValue
_io_StringIO___getstate___impl(stringio *self)
/*[clinic end generated code: output=cbf008249bbb4cfb input=76f27255ef83bb92]*/
{
    PyValue initvalue_v = _io_StringIO_getvalue_impl(self);
    PyObject *dict;
    PyObject *state;

    if (PyValue_IsNull(initvalue_v))
        return PyValue_NULL;
    PyObject *initvalue = PyValue_AsKnownObjectBorrow(initvalue_v);  // a str
    if (self->dict == NULL) {
        dict = Py_NewRef(Py_None);
    }
    else {
        dict = PyDict_Copy(self->dict);
        if (dict == NULL) {
            PyValue_CLOSE(initvalue_v);
            return PyValue_NULL;
        }
    }

    state = Py_BuildValue("(OOnN)", initvalue,
                          self->readnl ? self->readnl : Py_None,
                          self->pos, dict);
    PyValue_CLOSE(initvalue_v);
    return _PyValue_FromObjectResult(state);
}

/*[clinic input]
@critical_section
_io.StringIO.__setstate__

    state: object
    /
[clinic start generated code]*/

static PyValue
_io_StringIO___setstate___impl(stringio *self, PyValueBorrow state)
/*[clinic end generated code: output=2fc2d0405602ded7 input=8a27784b11b82e47]*/
{
    PyObject *initarg;
    PyValue position_obj;
    PyValue dict;
    Py_ssize_t pos;

    assert(!PyValue_IsNull(state));
    CHECK_CLOSED(self, PyValue_NULL);

    /* We allow the state tuple to be longer than 4, because we may need
       someday to extend the object's state without breaking
       backward-compatibility. */
    if (!(PyValue_IsObject(state) && PyTuple_Check(PyValue_AsKnownObjectBorrow(state))) || PyTuple_GET_SIZE(PyValue_AsKnownObjectBorrow(state)) < 4) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s.__setstate__ argument should be 4-tuple, got %.200s",
                     Py_TYPE(self)->tp_name, PyValue_TYPE(state)->tp_name);
        return PyValue_NULL;
    }

    PyObject *state_obj = PyValue_AsKnownObjectBorrow(state);  // a tuple

    /* Initialize the object's state. */
    initarg = PyTuple_GetSlice(state_obj, 0, 2);
    if (initarg == NULL)
        return PyValue_NULL;
    if (_io_StringIO___init__((PyObject *)self, initarg, NULL) < 0) {
        Py_DECREF(initarg);
        return PyValue_NULL;
    }
    Py_DECREF(initarg);

    /* Restore the buffer state. Even if __init__ did initialize the buffer,
       we have to initialize it again since __init__ may translate the
       newlines in the initial_value string. We clearly do not want that
       because the string value in the state tuple has already been translated
       once by __init__. So we do not take any chance and replace object's
       buffer completely. */
    {
        PyValue item = PyTuple_GetValue(state_obj, 0);
        if (PyValue_IsNull(item)) {
            return PyValue_NULL;
        }
        if (PyValue_IsObject(item) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(item))) {
            PyObject *item_str = PyValue_AsKnownObjectBorrow(item);
            Py_UCS4 *buf = PyUnicode_AsUCS4Copy(item_str);
            if (buf == NULL) {
                PyValue_CLOSE(item);
                return PyValue_NULL;
            }
            Py_ssize_t bufsize = PyUnicode_GET_LENGTH(item_str);

            if (resize_buffer(self, bufsize) < 0) {
                PyMem_Free(buf);
                PyValue_CLOSE(item);
                return PyValue_NULL;
            }
            memcpy(self->buf, buf, bufsize * sizeof(Py_UCS4));
            PyMem_Free(buf);
            self->string_size = bufsize;
        }
        else {
            assert(PyValue_IsNone(item));
            self->string_size = 0;
        }
        PyValue_CLOSE(item);
    }

    /* Set carefully the position value. Alternatively, we could use the seek
       method instead of modifying self->pos directly to better protect the
       object internal state against erroneous (or malicious) inputs. */
    position_obj = PyTuple_GetValue(state_obj, 2);
    if (PyValue_IsNull(position_obj)) {
        return PyValue_NULL;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(position_obj), &PyLong_Type)) {
        PyErr_Format(PyExc_TypeError,
                     "third item of state must be an integer, got %.200s",
                     PyValue_TYPE(position_obj)->tp_name);
        PyValue_CLOSE(position_obj);
        return PyValue_NULL;
    }
    pos = _PyValue_AsSsize_t(position_obj, PyExc_OverflowError);
    PyValue_CLOSE(position_obj);
    if (pos == -1 && PyErr_Occurred())
        return PyValue_NULL;
    if (pos < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "position value cannot be negative");
        return PyValue_NULL;
    }
    self->pos = pos;

    /* Set the dictionary of the instance variables. */
    dict = PyTuple_GetValue(state_obj, 3);
    if (PyValue_IsNull(dict)) {
        return PyValue_NULL;
    }
    if (!PyValue_IsNone(dict)) {
        if (!(PyValue_IsObject(dict) && PyDict_Check(PyValue_AsKnownObjectBorrow(dict)))) {
            PyErr_Format(PyExc_TypeError,
                         "fourth item of state should be a dict, got a %.200s",
                         PyValue_TYPE(dict)->tp_name);
            PyValue_CLOSE(dict);
            return PyValue_NULL;
        }
        if (self->dict) {
            /* Alternatively, we could replace the internal dictionary
               completely. However, it seems more practical to just update it. */
            if (PyDict_Update(self->dict, PyValue_AsKnownObjectBorrow(dict)) < 0) {
                PyValue_CLOSE(dict);
                return PyValue_NULL;
            }
        }
        else {
            self->dict = Py_NewRef(PyValue_AsKnownObjectBorrow(dict));
        }
    }
    PyValue_CLOSE(dict);

    return PyValue_None;
}

/*[clinic input]
@critical_section
@getter
_io.StringIO.closed -> bool
[clinic start generated code]*/

static int
_io_StringIO_closed_get_impl(stringio *self)
/*[clinic end generated code: output=754068c44422cafa input=ea05e89b945e721c]*/
{
    CHECK_INITIALIZED_INT(self);
    return self->closed;
}

/*[clinic input]
@critical_section
@getter
_io.StringIO.line_buffering -> bool
[clinic start generated code]*/

static int
_io_StringIO_line_buffering_get_impl(stringio *self)
/*[clinic end generated code: output=56c0edde9001fb37 input=326f8b3bd1feb699]*/
{
    CHECK_INITIALIZED_INT(self);
    CHECK_CLOSED_INT(self);
    return 0;
}

/*[clinic input]
@critical_section
@getter
_io.StringIO.newlines
[clinic start generated code]*/

static PyObject *
_io_StringIO_newlines_get_impl(stringio *self)
/*[clinic end generated code: output=35d7c0b66d7e0160 input=092a14586718244b]*/
{
    CHECK_INITIALIZED(self, NULL);
    CHECK_CLOSED(self, NULL);
    if (self->decoder == NULL) {
        Py_RETURN_NONE;
    }
    PyValue newlines = _PyValue_GetAttr(PyValue_FromObjectBorrow(self->decoder),
                                        &_Py_ID(newlines));
    if (PyValue_IsNull(newlines)) {
        return NULL;
    }
    return _PyValue_AsObjectStealAt(newlines, BOX_SITE_ATTRIBUTE);
}

static struct PyMethodDef stringio_methods[] = {
    _IO_STRINGIO_CLOSE_METHODDEF
    _IO_STRINGIO_GETVALUE_METHODDEF
    _IO_STRINGIO_READ_METHODDEF
    _IO_STRINGIO_READLINE_METHODDEF
    _IO_STRINGIO_TELL_METHODDEF
    _IO_STRINGIO_TRUNCATE_METHODDEF
    _IO_STRINGIO_SEEK_METHODDEF
    _IO_STRINGIO_WRITE_METHODDEF

    _IO_STRINGIO_SEEKABLE_METHODDEF
    _IO_STRINGIO_READABLE_METHODDEF
    _IO_STRINGIO_WRITABLE_METHODDEF

    _IO_STRINGIO___GETSTATE___METHODDEF
    _IO_STRINGIO___SETSTATE___METHODDEF
    {NULL, NULL}        /* sentinel */
};

static PyGetSetDef stringio_getset[] = {
    _IO_STRINGIO_CLOSED_GETSETDEF
    _IO_STRINGIO_NEWLINES_GETSETDEF
    /*  (following comments straight off of the original Python wrapper:)
        XXX Cruft to support the TextIOWrapper API. This would only
        be meaningful if StringIO supported the buffer attribute.
        Hopefully, a better solution, than adding these pseudo-attributes,
        will be found.
    */
    _IO_STRINGIO_LINE_BUFFERING_GETSETDEF
    {NULL}
};

static struct PyMemberDef stringio_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(stringio, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(stringio, dict), Py_READONLY},
    {NULL},
};

static PyType_Slot stringio_slots[] = {
    {Py_tp_dealloc, stringio_dealloc},
    {Py_tp_doc, (void *)_io_StringIO___init____doc__},
    {Py_tp_traverse, stringio_traverse},
    {Py_tp_clear, stringio_clear},
    {Py_tp_iternext, stringio_iternext},
    {Py_tp_methods, stringio_methods},
    {Py_tp_members, stringio_members},
    {Py_tp_getset, stringio_getset},
    {Py_tp_init, _io_StringIO___init__},
    {Py_tp_new, stringio_new},
    {0, NULL},
};

PyType_Spec _Py_stringio_spec = {
    .name = "_io.StringIO",
    .basicsize = sizeof(stringio),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = stringio_slots,
};
