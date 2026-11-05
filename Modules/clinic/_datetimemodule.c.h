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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(delta_new__doc__,
"timedelta(days=0, seconds=0, microseconds=0, milliseconds=0, minutes=0,\n"
"          hours=0, weeks=0)\n"
"--\n"
"\n"
"Difference between two datetime values.\n"
"\n"
"All arguments are optional and default to 0.\n"
"Arguments may be integers or floats, and may be positive or negative.");

static PyValue
delta_new_impl(PyTypeObject *type, PyValueBorrow days, PyValueBorrow seconds,
               PyValueBorrow microseconds, PyValueBorrow milliseconds,
               PyValueBorrow minutes, PyValueBorrow hours,
               PyValueBorrow weeks);

static PyValue
delta_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 7
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(days)), _PyValue_STATIC_IMMORTAL(&_Py_ID(seconds)), _PyValue_STATIC_IMMORTAL(&_Py_ID(microseconds)), _PyValue_STATIC_IMMORTAL(&_Py_ID(milliseconds)), _PyValue_STATIC_IMMORTAL(&_Py_ID(minutes)), _PyValue_STATIC_IMMORTAL(&_Py_ID(hours)), _PyValue_STATIC_IMMORTAL(&_Py_ID(weeks)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"days", "seconds", "microseconds", "milliseconds", "minutes", "hours", "weeks", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "timedelta",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[7];
    PyValueBorrow days = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow seconds = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow microseconds = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow milliseconds = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow minutes = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow hours = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow weeks = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 7, /*minkw*/ 0, /*varpos*/ 0, _idx, 7)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        days = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        seconds = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        microseconds = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        milliseconds = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        minutes = PyValue_Borrow(args[_idx[4]]);
    }
    if (_idx[5] >= 0) {
        hours = PyValue_Borrow(args[_idx[5]]);
    }
    if (_idx[6] >= 0) {
        weeks = PyValue_Borrow(args[_idx[6]]);
    }
    return_value = delta_new_impl(type, days, seconds, microseconds, milliseconds, minutes, hours, weeks);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
delta_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(delta_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = delta_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(datetime_date__doc__,
"date(year, month, day)\n"
"--\n"
"\n"
"Concrete date type.");

static PyValue
datetime_date_impl(PyTypeObject *type, int year, int month, int day);

static PyValue
datetime_date_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(year)), _PyValue_STATIC_IMMORTAL(&_Py_ID(month)), _PyValue_STATIC_IMMORTAL(&_Py_ID(day)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "month", "day", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "date",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    int year;
    int month;
    int day;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    year = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    month = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (month == -1 && PyErr_Occurred()) {
        goto exit;
    }
    day = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
    if (day == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_date_impl(type, year, month, day);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
datetime_date(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(datetime_date_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = datetime_date_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(datetime_date_today__doc__,
"today($type, /)\n"
"--\n"
"\n"
"Current date or datetime.\n"
"\n"
"Equivalent to fromtimestamp(time.time()).");

#define DATETIME_DATE_TODAY_METHODDEF    \
    {"today", _PyCFunction_CAST(datetime_date_today), METH_VALUE|METH_NOARGS|METH_CLASS, datetime_date_today__doc__},

static PyValue
datetime_date_today_impl(PyTypeObject *type);

static PyValue
datetime_date_today(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = datetime_date_today_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type));

    return return_value;
}

PyDoc_STRVAR(datetime_date_fromtimestamp__doc__,
"fromtimestamp($type, timestamp, /)\n"
"--\n"
"\n"
"Create a date from a POSIX timestamp.\n"
"\n"
"The timestamp is a number, e.g. created via time.time(), that is\n"
"interpreted as local time.");

#define DATETIME_DATE_FROMTIMESTAMP_METHODDEF    \
    {"fromtimestamp", _PyCFunction_CAST(datetime_date_fromtimestamp), METH_VALUE|METH_O|METH_CLASS, datetime_date_fromtimestamp__doc__},

static PyValue
datetime_date_fromtimestamp_impl(PyTypeObject *type, PyValueBorrow timestamp);

static PyValue
datetime_date_fromtimestamp(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow timestamp;

    timestamp = PyValue_Borrow(args[0]);
    return_value = datetime_date_fromtimestamp_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), timestamp);

    return return_value;
}

PyDoc_STRVAR(datetime_date_fromordinal__doc__,
"fromordinal($type, ordinal, /)\n"
"--\n"
"\n"
"Construct a date from a proleptic Gregorian ordinal.\n"
"\n"
"January 1 of year 1 is day 1.  Only the year, month and day are\n"
"non-zero in the result.");

#define DATETIME_DATE_FROMORDINAL_METHODDEF    \
    {"fromordinal", _PyCFunction_CAST(datetime_date_fromordinal), METH_VALUE|METH_O|METH_CLASS, datetime_date_fromordinal__doc__},

static PyValue
datetime_date_fromordinal_impl(PyTypeObject *type, int ordinal);

static PyValue
datetime_date_fromordinal(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int ordinal;

    ordinal = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (ordinal == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_date_fromordinal_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), ordinal);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_fromisoformat__doc__,
"fromisoformat($type, string, /)\n"
"--\n"
"\n"
"Construct a date from a string in ISO 8601 format.");

#define DATETIME_DATE_FROMISOFORMAT_METHODDEF    \
    {"fromisoformat", _PyCFunction_CAST(datetime_date_fromisoformat), METH_VALUE|METH_O|METH_CLASS, datetime_date_fromisoformat__doc__},

static PyValue
datetime_date_fromisoformat_impl(PyTypeObject *type, PyObject *string);

static PyValue
datetime_date_fromisoformat(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("fromisoformat", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("fromisoformat", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    string = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = datetime_date_fromisoformat_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_fromisocalendar__doc__,
"fromisocalendar($type, /, year, week, day)\n"
"--\n"
"\n"
"Construct a date from the ISO year, week number and weekday.\n"
"\n"
"This is the inverse of the date.isocalendar() function.");

#define DATETIME_DATE_FROMISOCALENDAR_METHODDEF    \
    {"fromisocalendar", _PyCFunction_CAST(datetime_date_fromisocalendar), METH_VALUE|METH_CLASS, datetime_date_fromisocalendar__doc__},

static PyValue
datetime_date_fromisocalendar_impl(PyTypeObject *type, int year, int week,
                                   int day);

static PyValue
datetime_date_fromisocalendar(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(year)), _PyValue_STATIC_IMMORTAL(&_Py_ID(week)), _PyValue_STATIC_IMMORTAL(&_Py_ID(day)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "week", "day", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fromisocalendar",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    int year;
    int week;
    int day;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    year = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    week = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (week == -1 && PyErr_Occurred()) {
        goto exit;
    }
    day = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
    if (day == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_date_fromisocalendar_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), year, week, day);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_strptime__doc__,
"strptime($type, string, format, /)\n"
"--\n"
"\n"
"Parse string according to the given date format (like time.strptime()).\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_DATE_STRPTIME_METHODDEF    \
    {"strptime", _PyCFunction_CAST(datetime_date_strptime), METH_VALUE|METH_CLASS, datetime_date_strptime__doc__},

static PyValue
datetime_date_strptime_impl(PyTypeObject *type, PyObject *string,
                            PyObject *format);

static PyValue
datetime_date_strptime(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;
    PyObject *format;

    if (!_PyArg_NoKwnamesValue("date.strptime()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("strptime", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("strptime", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("strptime", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    string = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("strptime", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("strptime", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    format = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = datetime_date_strptime_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_strftime__doc__,
"strftime($self, /, format)\n"
"--\n"
"\n"
"Format using strftime().\n"
"\n"
"Example: \"%d/%m/%Y, %H:%M:%S\".\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_DATE_STRFTIME_METHODDEF    \
    {"strftime", _PyCFunction_CAST(datetime_date_strftime), METH_VALUE, datetime_date_strftime__doc__},

static PyValue
datetime_date_strftime_impl(PyObject *self, PyObject *format);

static PyValue
datetime_date_strftime(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(format)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "strftime",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyObject *format;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("strftime", "argument 'format'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("strftime", "argument 'format'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    format = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    return_value = datetime_date_strftime_impl(PyValue_AsKnownObjectBorrow(self), format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date___format____doc__,
"__format__($self, format, /)\n"
"--\n"
"\n"
"Formats self with strftime.");

#define DATETIME_DATE___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(datetime_date___format__), METH_VALUE|METH_O, datetime_date___format____doc__},

static PyValue
datetime_date___format___impl(PyObject *self, PyObject *format);

static PyValue
datetime_date___format__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("__format__", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("__format__", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    format = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = datetime_date___format___impl(PyValue_AsKnownObjectBorrow(self), format);

exit:
    return return_value;
}

static PyValue
iso_calendar_date_new_impl(PyTypeObject *type, int year, int week,
                           int weekday);

static PyValue
iso_calendar_date_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(year)), _PyValue_STATIC_IMMORTAL(&_Py_ID(week)), _PyValue_STATIC_IMMORTAL(&_Py_ID(weekday)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "week", "weekday", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "IsoCalendarDate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    int year;
    int week;
    int weekday;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    year = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    week = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (week == -1 && PyErr_Occurred()) {
        goto exit;
    }
    weekday = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
    if (weekday == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = iso_calendar_date_new_impl(type, year, week, weekday);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
iso_calendar_date_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(iso_calendar_date_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = iso_calendar_date_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(datetime_date_replace__doc__,
"replace($self, /, year=unchanged, month=unchanged, day=unchanged)\n"
"--\n"
"\n"
"Return date with new specified fields.");

#define DATETIME_DATE_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(datetime_date_replace), METH_VALUE, datetime_date_replace__doc__},

static PyValue
datetime_date_replace_impl(PyDateTime_Date *self, int year, int month,
                           int day);

static PyValue
datetime_date_replace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(year)), _PyValue_STATIC_IMMORTAL(&_Py_ID(month)), _PyValue_STATIC_IMMORTAL(&_Py_ID(day)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "month", "day", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    int year = GET_YEAR(PyValue_AsKnownObjectBorrow(self));
    int month = GET_MONTH(PyValue_AsKnownObjectBorrow(self));
    int day = GET_DAY(PyValue_AsKnownObjectBorrow(self));

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        year = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (year == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        month = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (month == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        day = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (day == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = datetime_date_replace_impl((PyDateTime_Date *)PyValue_AsKnownObjectBorrow(self), year, month, day);

exit:
    return return_value;
}

PyDoc_STRVAR(timezone_new__doc__,
"timezone(offset, name=<unrepresentable>)\n"
"--\n"
"\n"
"Fixed offset from UTC implementation of tzinfo.");

static PyValue
timezone_new_impl(PyTypeObject *type, PyObject *offset, PyObject *name);

static PyValue
timezone_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(offset)), _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"offset", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "timezone",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyObject *offset;
    PyObject *name = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), DELTA_TYPE(NO_STATE))) {
        _PyArg_BadArgumentValue("timezone", "argument 'offset'", (DELTA_TYPE(NO_STATE))->tp_name, PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    offset = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("timezone", "argument 'name'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("timezone", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    }
    return_value = timezone_new_impl(type, offset, name);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
timezone_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(timezone_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = timezone_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(datetime_time__doc__,
"time(hour=0, minute=0, second=0, microsecond=0, tzinfo=None, *, fold=0)\n"
"--\n"
"\n"
"Time with time zone.\n"
"\n"
"All arguments are optional. tzinfo may be None, or an instance of\n"
"a tzinfo subclass. The remaining arguments may be ints.");

static PyValue
datetime_time_impl(PyTypeObject *type, int hour, int minute, int second,
                   int microsecond, PyValueBorrow tzinfo, int fold);

static PyValue
datetime_time_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(hour)), _PyValue_STATIC_IMMORTAL(&_Py_ID(minute)), _PyValue_STATIC_IMMORTAL(&_Py_ID(second)), _PyValue_STATIC_IMMORTAL(&_Py_ID(microsecond)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tzinfo)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fold)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hour", "minute", "second", "microsecond", "tzinfo", "fold", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "time",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    int hour = 0;
    int minute = 0;
    int second = 0;
    int microsecond = 0;
    PyValueBorrow tzinfo = PyValue_Borrow(PyValue_None);
    int fold = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        hour = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (hour == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        minute = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (minute == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        second = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (second == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        microsecond = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (microsecond == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        tzinfo = PyValue_Borrow(args[_idx[4]]);
    }
    if (_idx[5] >= 0) {
        fold = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (fold == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = datetime_time_impl(type, hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
datetime_time(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(datetime_time_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = datetime_time_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(datetime_time_strptime__doc__,
"strptime($type, string, format, /)\n"
"--\n"
"\n"
"Parse string according to the given time format (like time.strptime()).\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_TIME_STRPTIME_METHODDEF    \
    {"strptime", _PyCFunction_CAST(datetime_time_strptime), METH_VALUE|METH_CLASS, datetime_time_strptime__doc__},

static PyValue
datetime_time_strptime_impl(PyTypeObject *type, PyObject *string,
                            PyObject *format);

static PyValue
datetime_time_strptime(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;
    PyObject *format;

    if (!_PyArg_NoKwnamesValue("time.strptime()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("strptime", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("strptime", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("strptime", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    string = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("strptime", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("strptime", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    format = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = datetime_time_strptime_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_isoformat__doc__,
"isoformat($self, /, timespec=\'auto\')\n"
"--\n"
"\n"
"Return the time formatted according to ISO.\n"
"\n"
"The full format is \'HH:MM:SS.mmmmmm+zz:zz\'. By default, the\n"
"fractional part is omitted if self.microsecond == 0.\n"
"\n"
"The optional argument timespec specifies the number of additional\n"
"terms of the time to include. Valid options are \'auto\', \'hours\',\n"
"\'minutes\', \'seconds\', \'milliseconds\' and \'microseconds\'.");

#define DATETIME_TIME_ISOFORMAT_METHODDEF    \
    {"isoformat", _PyCFunction_CAST(datetime_time_isoformat), METH_VALUE, datetime_time_isoformat__doc__},

static PyValue
datetime_time_isoformat_impl(PyDateTime_Time *self, const char *timespec);

static PyValue
datetime_time_isoformat(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(timespec)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timespec", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "isoformat",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    const char *timespec = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("isoformat", "argument 'timespec'", "str", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            _PyArg_BadArgument("isoformat", "argument 'timespec'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
        timespec = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        if (timespec == NULL) {
            goto exit;
        }
    }
    return_value = datetime_time_isoformat_impl((PyDateTime_Time *)PyValue_AsKnownObjectBorrow(self), timespec);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_strftime__doc__,
"strftime($self, /, format)\n"
"--\n"
"\n"
"Format using strftime().\n"
"\n"
"The date part of the timestamp passed to underlying strftime should\n"
"not be used.\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_TIME_STRFTIME_METHODDEF    \
    {"strftime", _PyCFunction_CAST(datetime_time_strftime), METH_VALUE, datetime_time_strftime__doc__},

static PyValue
datetime_time_strftime_impl(PyDateTime_Time *self, PyObject *format);

static PyValue
datetime_time_strftime(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(format)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "strftime",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyObject *format;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("strftime", "argument 'format'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("strftime", "argument 'format'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    format = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    return_value = datetime_time_strftime_impl((PyDateTime_Time *)PyValue_AsKnownObjectBorrow(self), format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time___format____doc__,
"__format__($self, format, /)\n"
"--\n"
"\n"
"Formats self with strftime.");

#define DATETIME_TIME___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(datetime_time___format__), METH_VALUE|METH_O, datetime_time___format____doc__},

static PyValue
datetime_time___format___impl(PyObject *self, PyObject *format);

static PyValue
datetime_time___format__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("__format__", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("__format__", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    format = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = datetime_time___format___impl(PyValue_AsKnownObjectBorrow(self), format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_replace__doc__,
"replace($self, /, hour=unchanged, minute=unchanged, second=unchanged,\n"
"        microsecond=unchanged, tzinfo=unchanged, *, fold=unchanged)\n"
"--\n"
"\n"
"Return time with new specified fields.");

#define DATETIME_TIME_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(datetime_time_replace), METH_VALUE, datetime_time_replace__doc__},

static PyValue
datetime_time_replace_impl(PyDateTime_Time *self, int hour, int minute,
                           int second, int microsecond, PyValueBorrow tzinfo,
                           int fold);

static PyValue
datetime_time_replace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(hour)), _PyValue_STATIC_IMMORTAL(&_Py_ID(minute)), _PyValue_STATIC_IMMORTAL(&_Py_ID(second)), _PyValue_STATIC_IMMORTAL(&_Py_ID(microsecond)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tzinfo)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fold)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hour", "minute", "second", "microsecond", "tzinfo", "fold", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    int hour = TIME_GET_HOUR(PyValue_AsKnownObjectBorrow(self));
    int minute = TIME_GET_MINUTE(PyValue_AsKnownObjectBorrow(self));
    int second = TIME_GET_SECOND(PyValue_AsKnownObjectBorrow(self));
    int microsecond = TIME_GET_MICROSECOND(PyValue_AsKnownObjectBorrow(self));
    PyValueBorrow tzinfo = PyValue_FromObjectBorrow(HASTZINFO(PyValue_AsKnownObjectBorrow(self)) ? ((PyDateTime_Time *)PyValue_AsKnownObjectBorrow(self))->tzinfo : Py_None);
    int fold = TIME_GET_FOLD(PyValue_AsKnownObjectBorrow(self));

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        hour = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (hour == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        minute = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (minute == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        second = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (second == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        microsecond = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (microsecond == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        tzinfo = PyValue_Borrow(args[_idx[4]]);
    }
    if (_idx[5] >= 0) {
        fold = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (fold == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = datetime_time_replace_impl((PyDateTime_Time *)PyValue_AsKnownObjectBorrow(self), hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_fromisoformat__doc__,
"fromisoformat($type, string, /)\n"
"--\n"
"\n"
"Construct a time from a string in ISO 8601 format.");

#define DATETIME_TIME_FROMISOFORMAT_METHODDEF    \
    {"fromisoformat", _PyCFunction_CAST(datetime_time_fromisoformat), METH_VALUE|METH_O|METH_CLASS, datetime_time_fromisoformat__doc__},

static PyValue
datetime_time_fromisoformat_impl(PyTypeObject *type, PyObject *string);

static PyValue
datetime_time_fromisoformat(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("fromisoformat", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("fromisoformat", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    string = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = datetime_time_fromisoformat_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time___reduce_ex____doc__,
"__reduce_ex__($self, proto, /)\n"
"--\n"
"\n");

#define DATETIME_TIME___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", _PyCFunction_CAST(datetime_time___reduce_ex__), METH_VALUE|METH_O, datetime_time___reduce_ex____doc__},

static PyValue
datetime_time___reduce_ex___impl(PyDateTime_Time *self, int proto);

static PyValue
datetime_time___reduce_ex__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int proto;

    proto = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_time___reduce_ex___impl((PyDateTime_Time *)PyValue_AsKnownObjectBorrow(self), proto);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define DATETIME_TIME___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(datetime_time___reduce__), METH_VALUE|METH_NOARGS, datetime_time___reduce____doc__},

static PyValue
datetime_time___reduce___impl(PyDateTime_Time *self);

static PyValue
datetime_time___reduce__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = datetime_time___reduce___impl((PyDateTime_Time *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(datetime_datetime__doc__,
"datetime(year, month, day, hour=0, minute=0, second=0, microsecond=0,\n"
"         tzinfo=None, *, fold=0)\n"
"--\n"
"\n"
"A combination of a date and a time.\n"
"\n"
"The year, month and day arguments are required. tzinfo may be None, or\n"
"an instance of a tzinfo subclass. The remaining arguments may be ints.");

static PyValue
datetime_datetime_impl(PyTypeObject *type, int year, int month, int day,
                       int hour, int minute, int second, int microsecond,
                       PyValueBorrow tzinfo, int fold);

static PyValue
datetime_datetime_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(year)), _PyValue_STATIC_IMMORTAL(&_Py_ID(month)), _PyValue_STATIC_IMMORTAL(&_Py_ID(day)), _PyValue_STATIC_IMMORTAL(&_Py_ID(hour)), _PyValue_STATIC_IMMORTAL(&_Py_ID(minute)), _PyValue_STATIC_IMMORTAL(&_Py_ID(second)), _PyValue_STATIC_IMMORTAL(&_Py_ID(microsecond)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tzinfo)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fold)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "month", "day", "hour", "minute", "second", "microsecond", "tzinfo", "fold", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "datetime",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[9];
    int year;
    int month;
    int day;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int microsecond = 0;
    PyValueBorrow tzinfo = PyValue_Borrow(PyValue_None);
    int fold = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 8, /*minkw*/ 0, /*varpos*/ 0, _idx, 9)) {
        goto exit;
    }
    year = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    month = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (month == -1 && PyErr_Occurred()) {
        goto exit;
    }
    day = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
    if (day == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[3] >= 0) {
        hour = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (hour == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        minute = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (minute == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        second = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (second == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[6] >= 0) {
        microsecond = _PyValue_AsInt(PyValue_Borrow(args[_idx[6]]));
        if (microsecond == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[7] >= 0) {
        tzinfo = PyValue_Borrow(args[_idx[7]]);
    }
    if (_idx[8] >= 0) {
        fold = _PyValue_AsInt(PyValue_Borrow(args[_idx[8]]));
        if (fold == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = datetime_datetime_impl(type, year, month, day, hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
datetime_datetime(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(datetime_datetime_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = datetime_datetime_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(datetime_datetime_now__doc__,
"now($type, /, tz=None)\n"
"--\n"
"\n"
"Returns new datetime object representing current time local to tz.\n"
"\n"
"  tz\n"
"    Timezone object.\n"
"\n"
"If no tz is specified, uses local timezone.");

#define DATETIME_DATETIME_NOW_METHODDEF    \
    {"now", _PyCFunction_CAST(datetime_datetime_now), METH_VALUE|METH_CLASS, datetime_datetime_now__doc__},

static PyValue
datetime_datetime_now_impl(PyTypeObject *type, PyValueBorrow tz);

static PyValue
datetime_datetime_now(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(tz)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tz", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "now",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow tz = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        tz = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = datetime_datetime_now_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), tz);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_utcnow__doc__,
"utcnow($type, /)\n"
"--\n"
"\n"
"Return a new datetime representing UTC day and time.");

#define DATETIME_DATETIME_UTCNOW_METHODDEF    \
    {"utcnow", _PyCFunction_CAST(datetime_datetime_utcnow), METH_VALUE|METH_NOARGS|METH_CLASS, datetime_datetime_utcnow__doc__},

static PyValue
datetime_datetime_utcnow_impl(PyTypeObject *type);

static PyValue
datetime_datetime_utcnow(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = datetime_datetime_utcnow_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type));

    return return_value;
}

PyDoc_STRVAR(datetime_datetime_fromtimestamp__doc__,
"fromtimestamp($type, /, timestamp, tz=None)\n"
"--\n"
"\n"
"Create a datetime from a POSIX timestamp.\n"
"\n"
"The timestamp is a number, e.g. created via time.time(), that is\n"
"interpreted as local time.");

#define DATETIME_DATETIME_FROMTIMESTAMP_METHODDEF    \
    {"fromtimestamp", _PyCFunction_CAST(datetime_datetime_fromtimestamp), METH_VALUE|METH_CLASS, datetime_datetime_fromtimestamp__doc__},

static PyValue
datetime_datetime_fromtimestamp_impl(PyTypeObject *type,
                                     PyValueBorrow timestamp,
                                     PyValueBorrow tzinfo);

static PyValue
datetime_datetime_fromtimestamp(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(timestamp)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tz)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timestamp", "tz", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fromtimestamp",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow timestamp;
    PyValueBorrow tzinfo = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    timestamp = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        tzinfo = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = datetime_datetime_fromtimestamp_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), timestamp, tzinfo);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_utcfromtimestamp__doc__,
"utcfromtimestamp($type, timestamp, /)\n"
"--\n"
"\n"
"Create a naive UTC datetime from a POSIX timestamp.");

#define DATETIME_DATETIME_UTCFROMTIMESTAMP_METHODDEF    \
    {"utcfromtimestamp", _PyCFunction_CAST(datetime_datetime_utcfromtimestamp), METH_VALUE|METH_O|METH_CLASS, datetime_datetime_utcfromtimestamp__doc__},

static PyValue
datetime_datetime_utcfromtimestamp_impl(PyTypeObject *type,
                                        PyValueBorrow timestamp);

static PyValue
datetime_datetime_utcfromtimestamp(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow timestamp;

    timestamp = PyValue_Borrow(args[0]);
    return_value = datetime_datetime_utcfromtimestamp_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), timestamp);

    return return_value;
}

PyDoc_STRVAR(datetime_datetime_strptime__doc__,
"strptime($type, string, format, /)\n"
"--\n"
"\n"
"Parse string according to the given date and time format (like time.strptime()).\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_DATETIME_STRPTIME_METHODDEF    \
    {"strptime", _PyCFunction_CAST(datetime_datetime_strptime), METH_VALUE|METH_CLASS, datetime_datetime_strptime__doc__},

static PyValue
datetime_datetime_strptime_impl(PyTypeObject *type, PyObject *string,
                                PyObject *format);

static PyValue
datetime_datetime_strptime(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;
    PyObject *format;

    if (!_PyArg_NoKwnamesValue("datetime.strptime()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("strptime", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("strptime", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("strptime", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    string = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("strptime", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("strptime", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    format = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    return_value = datetime_datetime_strptime_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_combine__doc__,
"combine($type, /, date, time, tzinfo=<unrepresentable>)\n"
"--\n"
"\n"
"Construct a datetime from a given date and a given time.");

#define DATETIME_DATETIME_COMBINE_METHODDEF    \
    {"combine", _PyCFunction_CAST(datetime_datetime_combine), METH_VALUE|METH_CLASS, datetime_datetime_combine__doc__},

static PyValue
datetime_datetime_combine_impl(PyTypeObject *type, PyObject *date,
                               PyObject *time, PyValueBorrow tzinfo);

static PyValue
datetime_datetime_combine(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(date)), _PyValue_STATIC_IMMORTAL(&_Py_ID(time)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tzinfo)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"date", "time", "tzinfo", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "combine",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyObject *date;
    PyObject *time;
    PyValueBorrow tzinfo = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), DATE_TYPE(NO_STATE))) {
        _PyArg_BadArgumentValue("combine", "argument 'date'", (DATE_TYPE(NO_STATE))->tp_name, PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    date = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[1]])), TIME_TYPE(NO_STATE))) {
        _PyArg_BadArgumentValue("combine", "argument 'time'", (TIME_TYPE(NO_STATE))->tp_name, PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    time = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    if (_idx[2] >= 0) {
        tzinfo = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = datetime_datetime_combine_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), date, time, tzinfo);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_fromisoformat__doc__,
"fromisoformat($type, string, /)\n"
"--\n"
"\n"
"Construct a date from a string in ISO 8601 format.");

#define DATETIME_DATETIME_FROMISOFORMAT_METHODDEF    \
    {"fromisoformat", _PyCFunction_CAST(datetime_datetime_fromisoformat), METH_VALUE|METH_O|METH_CLASS, datetime_datetime_fromisoformat__doc__},

static PyValue
datetime_datetime_fromisoformat_impl(PyTypeObject *type, PyObject *string);

static PyValue
datetime_datetime_fromisoformat(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *string;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("fromisoformat", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("fromisoformat", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    string = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = datetime_datetime_fromisoformat_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), string);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_isoformat__doc__,
"isoformat($self, /, sep=\'T\', timespec=\'auto\')\n"
"--\n"
"\n"
"Return the time formatted according to ISO.\n"
"\n"
"The full format looks like \'YYYY-MM-DD HH:MM:SS.mmmmmm\'.\n"
"By default, the fractional part is omitted if self.microsecond == 0.\n"
"\n"
"If self.tzinfo is not None, the UTC offset is also attached, giving\n"
"a full format of \'YYYY-MM-DD HH:MM:SS.mmmmmm+HH:MM\'.\n"
"\n"
"Optional argument sep specifies the separator between date and\n"
"time, default \'T\'.\n"
"\n"
"The optional argument timespec specifies the number of additional\n"
"terms of the time to include. Valid options are \'auto\', \'hours\',\n"
"\'minutes\', \'seconds\', \'milliseconds\' and \'microseconds\'.");

#define DATETIME_DATETIME_ISOFORMAT_METHODDEF    \
    {"isoformat", _PyCFunction_CAST(datetime_datetime_isoformat), METH_VALUE, datetime_datetime_isoformat__doc__},

static PyValue
datetime_datetime_isoformat_impl(PyDateTime_DateTime *self, int sep,
                                 const char *timespec);

static PyValue
datetime_datetime_isoformat(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(sep)), _PyValue_STATIC_IMMORTAL(&_Py_ID(timespec)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "timespec", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "isoformat",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int sep = 'T';
    const char *timespec = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("isoformat", "argument 'sep'", "a unicode character", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            _PyArg_BadArgument("isoformat", "argument 'sep'", "a unicode character", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]))) != 1) {
            PyErr_Format(PyExc_TypeError,
                "isoformat(): argument 'sep' must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]))));
            goto exit;
        }
        sep = PyUnicode_READ_CHAR(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), 0);
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("isoformat", "argument 'timespec'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("isoformat", "argument 'timespec'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        timespec = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (timespec == NULL) {
            goto exit;
        }
    }
    return_value = datetime_datetime_isoformat_impl((PyDateTime_DateTime *)PyValue_AsKnownObjectBorrow(self), sep, timespec);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_replace__doc__,
"replace($self, /, year=unchanged, month=unchanged, day=unchanged,\n"
"        hour=unchanged, minute=unchanged, second=unchanged,\n"
"        microsecond=unchanged, tzinfo=unchanged, *, fold=unchanged)\n"
"--\n"
"\n"
"Return datetime with new specified fields.");

#define DATETIME_DATETIME_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(datetime_datetime_replace), METH_VALUE, datetime_datetime_replace__doc__},

static PyValue
datetime_datetime_replace_impl(PyDateTime_DateTime *self, int year,
                               int month, int day, int hour, int minute,
                               int second, int microsecond,
                               PyValueBorrow tzinfo, int fold);

static PyValue
datetime_datetime_replace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(year)), _PyValue_STATIC_IMMORTAL(&_Py_ID(month)), _PyValue_STATIC_IMMORTAL(&_Py_ID(day)), _PyValue_STATIC_IMMORTAL(&_Py_ID(hour)), _PyValue_STATIC_IMMORTAL(&_Py_ID(minute)), _PyValue_STATIC_IMMORTAL(&_Py_ID(second)), _PyValue_STATIC_IMMORTAL(&_Py_ID(microsecond)), _PyValue_STATIC_IMMORTAL(&_Py_ID(tzinfo)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fold)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "month", "day", "hour", "minute", "second", "microsecond", "tzinfo", "fold", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[9];
    int year = GET_YEAR(PyValue_AsKnownObjectBorrow(self));
    int month = GET_MONTH(PyValue_AsKnownObjectBorrow(self));
    int day = GET_DAY(PyValue_AsKnownObjectBorrow(self));
    int hour = DATE_GET_HOUR(PyValue_AsKnownObjectBorrow(self));
    int minute = DATE_GET_MINUTE(PyValue_AsKnownObjectBorrow(self));
    int second = DATE_GET_SECOND(PyValue_AsKnownObjectBorrow(self));
    int microsecond = DATE_GET_MICROSECOND(PyValue_AsKnownObjectBorrow(self));
    PyValueBorrow tzinfo = PyValue_FromObjectBorrow(HASTZINFO(PyValue_AsKnownObjectBorrow(self)) ? ((PyDateTime_DateTime *)PyValue_AsKnownObjectBorrow(self))->tzinfo : Py_None);
    int fold = DATE_GET_FOLD(PyValue_AsKnownObjectBorrow(self));

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 8, /*minkw*/ 0, /*varpos*/ 0, _idx, 9)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        year = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (year == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        month = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (month == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        day = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (day == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        hour = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (hour == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        minute = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (minute == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        second = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (second == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[6] >= 0) {
        microsecond = _PyValue_AsInt(PyValue_Borrow(args[_idx[6]]));
        if (microsecond == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[7] >= 0) {
        tzinfo = PyValue_Borrow(args[_idx[7]]);
    }
    if (_idx[8] >= 0) {
        fold = _PyValue_AsInt(PyValue_Borrow(args[_idx[8]]));
        if (fold == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = datetime_datetime_replace_impl((PyDateTime_DateTime *)PyValue_AsKnownObjectBorrow(self), year, month, day, hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_astimezone__doc__,
"astimezone($self, /, tz=None)\n"
"--\n"
"\n"
"Convert to local time in new timezone tz.");

#define DATETIME_DATETIME_ASTIMEZONE_METHODDEF    \
    {"astimezone", _PyCFunction_CAST(datetime_datetime_astimezone), METH_VALUE, datetime_datetime_astimezone__doc__},

static PyValue
datetime_datetime_astimezone_impl(PyDateTime_DateTime *self,
                                  PyValueBorrow tzinfo);

static PyValue
datetime_datetime_astimezone(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(tz)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tz", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "astimezone",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow tzinfo = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        tzinfo = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = datetime_datetime_astimezone_impl((PyDateTime_DateTime *)PyValue_AsKnownObjectBorrow(self), tzinfo);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime___reduce_ex____doc__,
"__reduce_ex__($self, proto, /)\n"
"--\n"
"\n");

#define DATETIME_DATETIME___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", _PyCFunction_CAST(datetime_datetime___reduce_ex__), METH_VALUE|METH_O, datetime_datetime___reduce_ex____doc__},

static PyValue
datetime_datetime___reduce_ex___impl(PyDateTime_DateTime *self, int proto);

static PyValue
datetime_datetime___reduce_ex__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int proto;

    proto = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_datetime___reduce_ex___impl((PyDateTime_DateTime *)PyValue_AsKnownObjectBorrow(self), proto);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define DATETIME_DATETIME___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(datetime_datetime___reduce__), METH_VALUE|METH_NOARGS, datetime_datetime___reduce____doc__},

static PyValue
datetime_datetime___reduce___impl(PyDateTime_DateTime *self);

static PyValue
datetime_datetime___reduce__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = datetime_datetime___reduce___impl((PyDateTime_DateTime *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}
/*[clinic end generated code: output=da08fee5cad32aa1 input=a9049054013a1b77]*/
