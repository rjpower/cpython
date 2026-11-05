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
#include "pycore_long_value.h"    // _PyValue_AsDouble()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(pysqlite_connect__doc__,
"connect($module, /, database, *, timeout=5.0, detect_types=0,\n"
"        isolation_level=\'\', check_same_thread=True,\n"
"        factory=ConnectionType, cached_statements=128, uri=False,\n"
"        autocommit=sqlite3.LEGACY_TRANSACTION_CONTROL)\n"
"--\n"
"\n"
"Open a connection to the SQLite database file \'database\'.\n"
"\n"
"You can use \":memory:\" to open a database connection to a database that\n"
"resides in RAM instead of on disk.");

#define PYSQLITE_CONNECT_METHODDEF    \
    {"connect", _PyCFunction_CAST(pysqlite_connect), METH_VALUE|METH_KEYWORDS, pysqlite_connect__doc__},
/*[clinic end generated code: output=0c09a317f1673d14 input=a9049054013a1b77]*/
