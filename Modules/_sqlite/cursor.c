/* cursor.c - the cursor type
 *
 * Copyright (C) 2004-2010 Gerhard Häring <gh@ghaering.de>
 *
 * This file is part of pysqlite.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#define Py_VALUE_CORE 1
#include "cursor.h"
#include "microprotocols.h"
#include "module.h"
#include "util.h"

#include "pycore_dict.h"          // PyDict_GetValue()
#include "pycore_list.h"          // PyList_AppendValue()
#include "pycore_long_value.h"    // _PyValue_AsDouble()
#include "pycore_pyerrors.h"      // _PyErr_FormatFromCause()
#include "pycore_tuple.h"         // PyTuple_GetValue()
#include "pycore_value.h"         // PyValue
#include "pycore_value_ops.h"     // _PyValue_GetIter()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()

typedef enum {
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_UNICODE,
    TYPE_BUFFER,
    TYPE_UNKNOWN
} parameter_type;

#define clinic_state() (pysqlite_get_state_by_type(Py_TYPE(self)))
#include "clinic/cursor.c.h"
#undef clinic_state

#define _pysqlite_Cursor_CAST(op)   ((pysqlite_Cursor *)(op))

static inline int
check_cursor_locked(pysqlite_Cursor *cur)
{
    if (cur->locked) {
        PyErr_SetString(cur->connection->ProgrammingError,
                        "Recursive use of cursors not allowed.");
        return 0;
    }
    return 1;
}

static pysqlite_state *
get_module_state_by_cursor(pysqlite_Cursor *cursor)
{
    if (cursor->connection != NULL && cursor->connection->state != NULL) {
        return cursor->connection->state;
    }
    return pysqlite_get_state_by_type(Py_TYPE(cursor));
}

static void
cursor_sqlite3_internal_error(pysqlite_Cursor *cursor,
                              const char *error_message,
                              int chain_exceptions)
{
    pysqlite_state *state = get_module_state_by_cursor(cursor);
    if (chain_exceptions) {
        assert(PyErr_Occurred());
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(state->InternalError, error_message);
        _PyErr_ChainExceptions1(exc);
    }
    else {
        assert(!PyErr_Occurred());
        PyErr_SetString(state->InternalError, error_message);
   }
}

static void
cursor_cannot_reset_stmt_error(pysqlite_Cursor *cursor, int chain_exceptions)
{
    cursor_sqlite3_internal_error(cursor,
                                  "cannot reset statement",
                                  chain_exceptions);
}

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _sqlite3
class _sqlite3.Cursor "pysqlite_Cursor *" "clinic_state()->CursorType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3c5b8115c5cf30f1]*/

/*[clinic input]
_sqlite3.Cursor.__init__ as pysqlite_cursor_init

    connection: object(type='pysqlite_Connection *', subclass_of='clinic_state()->ConnectionType')
    /

[clinic start generated code]*/

static int
pysqlite_cursor_init_impl(pysqlite_Cursor *self,
                          pysqlite_Connection *connection)
/*[clinic end generated code: output=ac59dce49a809ca8 input=23d4265b534989fb]*/
{
    if (!check_cursor_locked(self)) {
        return -1;
    }

    Py_INCREF(connection);
    Py_XSETREF(self->connection, connection);
    Py_CLEAR(self->statement);
    Py_CLEAR(self->row_cast_map);

    Py_INCREF(Py_None);
    Py_XSETREF(self->description, Py_None);

    Py_INCREF(Py_None);
    Py_XSETREF(self->lastrowid, Py_None);

    self->arraysize = 1;
    self->closed = 0;
    self->rowcount = -1L;

    Py_INCREF(Py_None);
    Py_XSETREF(self->row_factory, Py_None);

    if (!pysqlite_check_thread(self->connection)) {
        return -1;
    }

    self->initialized = 1;

    return 0;
}

static inline int
stmt_reset(pysqlite_Statement *self)
{
    int rc = SQLITE_OK;

    if (self->st != NULL) {
        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_reset(self->st);
        Py_END_ALLOW_THREADS
    }

    return rc;
}

static int
cursor_traverse(PyObject *op, visitproc visit, void *arg)
{
    pysqlite_Cursor *self = _pysqlite_Cursor_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->connection);
    Py_VISIT(self->description);
    Py_VISIT(self->row_cast_map);
    Py_VISIT(self->lastrowid);
    Py_VISIT(self->row_factory);
    Py_VISIT(self->statement);
    return 0;
}

static int
cursor_clear(PyObject *op)
{
    pysqlite_Cursor *self = _pysqlite_Cursor_CAST(op);
    Py_CLEAR(self->connection);
    Py_CLEAR(self->description);
    Py_CLEAR(self->row_cast_map);
    Py_CLEAR(self->lastrowid);
    Py_CLEAR(self->row_factory);
    if (self->statement) {
        /* Reset the statement if the user has not closed the cursor */
        int rc = stmt_reset(self->statement);
        Py_CLEAR(self->statement);
        if (rc != SQLITE_OK) {
            cursor_cannot_reset_stmt_error(self, 0);
            PyErr_FormatUnraisable("Exception ignored in cursor_clear()");
        }
    }

    return 0;
}

static void
cursor_dealloc(PyObject *op)
{
    pysqlite_Cursor *self = _pysqlite_Cursor_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    FT_CLEAR_WEAKREFS(op, self->in_weakreflist);
    (void)tp->tp_clear(op);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
_pysqlite_get_converter(pysqlite_state *state, const char *keystr,
                        Py_ssize_t keylen)
{
    PyObject *key;
    PyObject *upcase_key;
    PyObject *retval;

    key = PyUnicode_FromStringAndSize(keystr, keylen);
    if (!key) {
        return NULL;
    }
    upcase_key = PyObject_CallMethodNoArgs(key, state->str_upper);
    Py_DECREF(key);
    if (!upcase_key) {
        return NULL;
    }

    PyValue converter;
    int rc = PyDict_GetValue(state->converters, PyValue_FromObjectBorrow(upcase_key),
                             &converter);
    Py_DECREF(upcase_key);
    if (rc < 0) {
        return NULL;
    }
    if (PyValue_IsNull(converter)) {
        return NULL;
    }
    retval = _PyValue_AsObjectStealAt(converter, BOX_SITE_DICT);
    return retval;
}

static int
pysqlite_build_row_cast_map(pysqlite_Cursor* self)
{
    int i;
    const char* pos;
    const char* decltype;
    PyObject* converter;

    if (!self->connection->detect_types) {
        return 0;
    }

    Py_XSETREF(self->row_cast_map, PyList_New(0));
    if (!self->row_cast_map) {
        return -1;
    }

    for (i = 0; i < sqlite3_column_count(self->statement->st); i++) {
        converter = NULL;

        if (self->connection->detect_types & PARSE_COLNAMES) {
            const char *colname = sqlite3_column_name(self->statement->st, i);
            if (colname == NULL) {
                PyErr_NoMemory();
                Py_CLEAR(self->row_cast_map);
                return -1;
            }
            const char *type_start = NULL;
            for (pos = colname; *pos != 0; pos++) {
                if (*pos == '[') {
                    type_start = pos + 1;
                }
                else if (*pos == ']' && type_start != NULL) {
                    pysqlite_state *state = self->connection->state;
                    converter = _pysqlite_get_converter(state, type_start,
                                                        pos - type_start);
                    if (!converter && PyErr_Occurred()) {
                        Py_CLEAR(self->row_cast_map);
                        return -1;
                    }
                    break;
                }
            }
        }

        if (!converter && self->connection->detect_types & PARSE_DECLTYPES) {
            decltype = sqlite3_column_decltype(self->statement->st, i);
            if (decltype) {
                for (pos = decltype;;pos++) {
                    /* Converter names are split at '(' and blanks.
                     * This allows 'INTEGER NOT NULL' to be treated as 'INTEGER' and
                     * 'NUMBER(10)' to be treated as 'NUMBER', for example.
                     * In other words, it will work as people expect it to work.*/
                    if (*pos == ' ' || *pos == '(' || *pos == 0) {
                        pysqlite_state *state = self->connection->state;
                        converter = _pysqlite_get_converter(state, decltype,
                                                            pos - decltype);
                        if (!converter && PyErr_Occurred()) {
                            Py_CLEAR(self->row_cast_map);
                            return -1;
                        }
                        break;
                    }
                }
            }
        }

        int rc = PyList_AppendValue(
            self->row_cast_map,
            PyValue_FromObjectBorrow(converter ? converter : Py_None));
        Py_XDECREF(converter);
        if (rc != 0) {
            Py_CLEAR(self->row_cast_map);
            return -1;
        }
    }

    return 0;
}

static PyObject *
_pysqlite_build_column_name(pysqlite_Cursor *self, const char *colname)
{
    const char* pos;
    Py_ssize_t len;

    if (self->connection->detect_types & PARSE_COLNAMES) {
        for (pos = colname; *pos; pos++) {
            if (*pos == '[') {
                if ((pos != colname) && (*(pos-1) == ' ')) {
                    pos--;
                }
                break;
            }
        }
        len = pos - colname;
    }
    else {
        len = strlen(colname);
    }
    return PyUnicode_FromStringAndSize(colname, len);
}

/*
 * Returns a row from the currently active SQLite statement
 *
 * Precondidition:
 * - sqlite3_step() has been called before and it returned SQLITE_ROW.
 */
static PyObject *
_pysqlite_fetch_one_row(pysqlite_Cursor* self)
{
    int i, numcols;
    PyObject* row;
    int coltype;
    PyObject* converter = NULL;
    PyObject* converted;
    Py_ssize_t nbytes;
    char buf[200];
    const char* colname;
    PyObject* error_msg;

    Py_BEGIN_ALLOW_THREADS
    numcols = sqlite3_data_count(self->statement->st);
    Py_END_ALLOW_THREADS

    row = PyTuple_New(numcols);
    if (!row)
        return NULL;

    sqlite3 *db = self->connection->db;
    for (i = 0; i < numcols; i++) {
        if (self->connection->detect_types
                && self->row_cast_map != NULL
                && i < PyList_GET_SIZE(self->row_cast_map))
        {
            PyValue converter_v = PyList_GetValue(self->row_cast_map, i);
            if (PyValue_IsNull(converter_v)) {
                goto error;
            }
            converter = _PyValue_AsObjectStealAt(converter_v, BOX_SITE_LIST);
            if (converter == NULL) {
                goto error;
            }
        }
        else {
            converter = Py_NewRef(Py_None);
        }

        /*
         * Note, sqlite3_column_bytes() must come after sqlite3_column_blob()
         * or sqlite3_column_text().
         *
         * See https://sqlite.org/c3ref/column_blob.html for details.
         */
        if (converter != Py_None) {
            const void *blob = sqlite3_column_blob(self->statement->st, i);
            if (blob == NULL) {
                if (sqlite3_errcode(db) == SQLITE_NOMEM) {
                    PyErr_NoMemory();
                    goto error;
                }
                converted = Py_NewRef(Py_None);
            }
            else {
                nbytes = sqlite3_column_bytes(self->statement->st, i);
                PyObject *item = PyBytes_FromStringAndSize(blob, nbytes);
                if (item == NULL) {
                    goto error;
                }
                converted = PyObject_CallOneArg(converter, item);
                Py_DECREF(item);
            }
        } else {
            Py_BEGIN_ALLOW_THREADS
            coltype = sqlite3_column_type(self->statement->st, i);
            Py_END_ALLOW_THREADS
            if (coltype == SQLITE_NULL) {
                converted = Py_NewRef(Py_None);
            } else if (coltype == SQLITE_INTEGER) {
                converted = PyLong_FromLongLong(sqlite3_column_int64(self->statement->st, i));
            } else if (coltype == SQLITE_FLOAT) {
                converted = PyFloat_FromDouble(sqlite3_column_double(self->statement->st, i));
            } else if (coltype == SQLITE_TEXT) {
                const char *text = (const char*)sqlite3_column_text(self->statement->st, i);
                if (text == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
                    PyErr_NoMemory();
                    goto error;
                }

                nbytes = sqlite3_column_bytes(self->statement->st, i);
                if (self->connection->text_factory == (PyObject*)&PyUnicode_Type) {
                    converted = PyUnicode_FromStringAndSize(text, nbytes);
                    if (!converted && PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
                        PyErr_Clear();
                        colname = sqlite3_column_name(self->statement->st, i);
                        if (colname == NULL) {
                            PyErr_NoMemory();
                            goto error;
                        }
                        PyOS_snprintf(buf, sizeof(buf) - 1, "Could not decode to UTF-8 column '%s' with text '%s'",
                                     colname , text);
                        error_msg = PyUnicode_Decode(buf, strlen(buf), "ascii", "replace");

                        PyObject *exc = self->connection->OperationalError;
                        if (!error_msg) {
                            PyErr_SetString(exc, "Could not decode to UTF-8");
                        } else {
                            PyErr_SetObject(exc, error_msg);
                            Py_DECREF(error_msg);
                        }
                    }
                } else if (self->connection->text_factory == (PyObject*)&PyBytes_Type) {
                    converted = PyBytes_FromStringAndSize(text, nbytes);
                } else if (self->connection->text_factory == (PyObject*)&PyByteArray_Type) {
                    converted = PyByteArray_FromStringAndSize(text, nbytes);
                } else {
                    converted = PyObject_CallFunction(self->connection->text_factory, "y#", text, nbytes);
                }
            } else {
                /* coltype == SQLITE_BLOB */
                const void *blob = sqlite3_column_blob(self->statement->st, i);
                if (blob == NULL && sqlite3_errcode(db) == SQLITE_NOMEM) {
                    PyErr_NoMemory();
                    goto error;
                }

                nbytes = sqlite3_column_bytes(self->statement->st, i);
                converted = PyBytes_FromStringAndSize(blob, nbytes);
            }
        }

        Py_CLEAR(converter);
        if (!converted) {
            goto error;
        }
        PyTuple_SET_VALUE(row, i, PyValue_PrepareOwned(converted));
    }

    if (PyErr_Occurred())
        goto error;

    return row;

error:
    Py_XDECREF(converter);
    Py_DECREF(row);
    return NULL;
}

/*
 * Checks if a cursor object is usable.
 *
 * 0 => error; 1 => ok
 */
static int check_cursor(pysqlite_Cursor* cur)
{
    if (!cur->initialized) {
        pysqlite_state *state = pysqlite_get_state_by_type(Py_TYPE(cur));
        PyErr_SetString(state->ProgrammingError,
                        "Base Cursor.__init__ not called.");
        return 0;
    }

    assert(cur->connection != NULL);
    assert(cur->connection->state != NULL);

    if (cur->closed) {
        PyErr_SetString(cur->connection->state->ProgrammingError,
                        "Cannot operate on a closed cursor.");
        return 0;
    }

    return (pysqlite_check_thread(cur->connection)
            && pysqlite_check_connection(cur->connection)
            && check_cursor_locked(cur));
}

static int
begin_transaction(pysqlite_Connection *self)
{
    assert(self->isolation_level != NULL);
    int rc;

    Py_BEGIN_ALLOW_THREADS
    sqlite3_stmt *statement;
    char begin_stmt[16] = "BEGIN ";
#ifdef Py_DEBUG
    size_t len = strlen(self->isolation_level);
    assert(len <= 9);
#endif
    (void)strcat(begin_stmt, self->isolation_level);
    rc = sqlite3_prepare_v2(self->db, begin_stmt, -1, &statement, NULL);
    if (rc == SQLITE_OK) {
        (void)sqlite3_step(statement);
        rc = sqlite3_finalize(statement);
    }
    Py_END_ALLOW_THREADS

    if (rc != SQLITE_OK) {
        set_error_from_db(self->state, self->db);
        return -1;
    }

    return 0;
}

static PyObject *
get_statement_from_cache(pysqlite_Cursor *self, PyObject *operation)
{
    PyObject *args[] = { NULL, operation, };  // Borrowed ref.
    PyObject *cache = self->connection->statement_cache;
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return PyObject_Vectorcall(cache, args + 1, nargsf, NULL);
}

static inline int
stmt_step(sqlite3_stmt *statement)
{
    int rc;

    Py_BEGIN_ALLOW_THREADS
    rc = sqlite3_step(statement);
    Py_END_ALLOW_THREADS

    return rc;
}

static int
bind_param(pysqlite_state *state, pysqlite_Statement *self, int pos,
           PyValueBorrow parameter)
{
    int rc = SQLITE_OK;
    const char *string;
    Py_ssize_t buflen;
    parameter_type paramtype;

    if (PyValue_IsNone(parameter)) {
        rc = sqlite3_bind_null(self->st, pos);
        goto final;
    }

    if (PyValue_LongCheckExact(parameter)) {
        paramtype = TYPE_LONG;
    } else if (PyValue_TYPE(parameter) == &PyFloat_Type) {
        paramtype = TYPE_FLOAT;
    } else if (PyValue_TYPE(parameter) == &PyUnicode_Type) {
        paramtype = TYPE_UNICODE;
    } else if (PyValue_LongCheck(parameter)) {
        paramtype = TYPE_LONG;
    } else if (PyValue_FloatCheck(parameter)) {
        paramtype = TYPE_FLOAT;
    } else if (PyValue_IsObject(parameter) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(parameter))) {
        paramtype = TYPE_UNICODE;
    } else if (PyValue_IsObject(parameter) && PyObject_CheckBuffer(PyValue_AsKnownObjectBorrow(parameter))) {
        paramtype = TYPE_BUFFER;
    } else {
        paramtype = TYPE_UNKNOWN;
    }

    switch (paramtype) {
        case TYPE_LONG: {
            sqlite_int64 value = _pysqlite_value_as_int64(parameter);
            rc = (value == -1 && PyErr_Occurred())
                ? SQLITE_ERROR
                : sqlite3_bind_int64(self->st, pos, value);
            break;
        }
        case TYPE_FLOAT: {
            double value = _PyValue_AsDouble(parameter);
            rc = (value == -1 && PyErr_Occurred())
                ? SQLITE_ERROR
                : sqlite3_bind_double(self->st, pos, value);
            break;
        }
        case TYPE_UNICODE:
            string = PyUnicode_AsUTF8AndSize(PyValue_AsKnownObjectBorrow(parameter), &buflen);
            if (string == NULL) {
                return SQLITE_ERROR;
            }
            if (buflen > INT_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "string longer than INT_MAX bytes");
                return SQLITE_ERROR;
            }
            rc = sqlite3_bind_text(self->st, pos, string, (int)buflen, SQLITE_TRANSIENT);
            break;
        case TYPE_BUFFER: {
            Py_buffer view;
            if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(parameter), &view, PyBUF_SIMPLE) != 0) {
                return SQLITE_ERROR;
            }
            if (view.len > INT_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "BLOB longer than INT_MAX bytes");
                PyBuffer_Release(&view);
                return SQLITE_ERROR;
            }
            rc = sqlite3_bind_blob(self->st, pos, view.buf, (int)view.len, SQLITE_TRANSIENT);
            PyBuffer_Release(&view);
            break;
        }
        case TYPE_UNKNOWN:
            PyErr_Format(state->ProgrammingError,
                    "Error binding parameter %d: type '%s' is not supported",
                    pos, PyValue_TYPE(parameter)->tp_name);
            rc = SQLITE_ERROR;
    }

final:
    return rc;
}

/* returns 0 if the object is one of Python's internal ones that don't need to be adapted */
static inline int
need_adapt(pysqlite_state *state, PyValueBorrow obj)
{
    if (state->BaseTypeAdapted) {
        return 1;
    }

    if (PyValue_LongCheckExact(obj) || PyValue_TYPE(obj) == &PyFloat_Type
          || PyValue_TYPE(obj) == &PyUnicode_Type || PyValue_TYPE(obj) == &PyByteArray_Type) {
        return 0;
    } else {
        return 1;
    }
}

static void
bind_parameters(pysqlite_state *state, pysqlite_Statement *self,
                PyObject *parameters)
{
    PyValue current_param;
    PyValue adapted;
    const char* binding_name;
    int i;
    int rc;
    int num_params_needed;
    Py_ssize_t num_params;

    Py_BEGIN_ALLOW_THREADS
    num_params_needed = sqlite3_bind_parameter_count(self->st);
    Py_END_ALLOW_THREADS

    if (PyTuple_CheckExact(parameters) || PyList_CheckExact(parameters) || (!PyDict_Check(parameters) && _PyValue_SequenceCheck(PyValue_FromObjectBorrow(parameters)))) {
        /* parameters passed as sequence */
        if (PyTuple_CheckExact(parameters)) {
            num_params = PyTuple_GET_SIZE(parameters);
        } else if (PyList_CheckExact(parameters)) {
            num_params = PyList_GET_SIZE(parameters);
        } else {
            num_params = _PyValue_SequenceSize(PyValue_FromObjectBorrow(parameters));
            if (num_params == -1) {
                return;
            }
        }
        if (num_params != num_params_needed) {
            PyErr_Format(state->ProgrammingError,
                         "Incorrect number of bindings supplied. The current "
                         "statement uses %d, and there are %zd supplied.",
                         num_params_needed, num_params);
            return;
        }
        for (i = 0; i < num_params; i++) {
            const char *name = sqlite3_bind_parameter_name(self->st, i+1);
            if (name != NULL && name[0] != '?') {
                PyErr_Format(state->ProgrammingError,
                        "Binding %d ('%s') is a named parameter, but you "
                        "supplied a sequence which requires nameless (qmark) "
                        "placeholders.",
                        i+1, name);
                return;
            }

            PyValue param_v;
            if (PyTuple_CheckExact(parameters)) {
                param_v = PyTuple_GetValue(parameters, i);
            } else if (PyList_CheckExact(parameters)) {
                param_v = PyList_GetValue(parameters, i);
            } else {
                param_v = _PyValue_SequenceGetItem(
                    PyValue_FromObjectBorrow(parameters), i);
            }
            if (PyValue_IsNull(param_v)) {
                return;
            }
            current_param = param_v;
            if (!need_adapt(state, PyValue_Borrow(current_param))) {
                adapted = current_param;
            } else {
                PyValueBorrow protocol = PyValue_FromObjectBorrow(
                    (PyObject *)state->PrepareProtocolType);
                adapted = pysqlite_microprotocols_adapt(
                    state, PyValue_Borrow(current_param), protocol,
                    PyValue_Borrow(current_param));
                PyValue_CLOSE(current_param);
                if (PyValue_IsNull(adapted)) {
                    return;
                }
            }
            rc = bind_param(state, self, i + 1, PyValue_Borrow(adapted));
            PyValue_CLOSE(adapted);

            if (rc != SQLITE_OK) {
                PyObject *exc = PyErr_GetRaisedException();
                sqlite3 *db = sqlite3_db_handle(self->st);
                set_error_from_db(state, db);
                _PyErr_ChainExceptions1(exc);
                return;
            }
        }
    } else if (PyDict_Check(parameters)) {
        /* parameters passed as dictionary */
        for (i = 1; i <= num_params_needed; i++) {
            Py_BEGIN_ALLOW_THREADS
            binding_name = sqlite3_bind_parameter_name(self->st, i);
            Py_END_ALLOW_THREADS
            if (!binding_name) {
                PyErr_Format(state->ProgrammingError,
                             "Binding %d has no name, but you supplied a "
                             "dictionary (which has only names).", i);
                return;
            }

            binding_name++; /* skip first char (the colon) */
            _PyStatusValue found = _PyValue_MappingGetOptionalItemString(
                PyValue_FromObjectBorrow(parameters), binding_name);
            if (found.status < 0) {
                return;
            }
            if (found.status == 0) {
                PyErr_Format(state->ProgrammingError,
                             "You did not supply a value for binding "
                             "parameter :%s.", binding_name);
                return;
            }
            current_param = found.value;
            if (!need_adapt(state, PyValue_Borrow(current_param))) {
                adapted = current_param;
            } else {
                PyValueBorrow protocol = PyValue_FromObjectBorrow(
                    (PyObject *)state->PrepareProtocolType);
                adapted = pysqlite_microprotocols_adapt(
                    state, PyValue_Borrow(current_param), protocol,
                    PyValue_Borrow(current_param));
                PyValue_CLOSE(current_param);
                if (PyValue_IsNull(adapted)) {
                    return;
                }
            }
            rc = bind_param(state, self, i, PyValue_Borrow(adapted));
            PyValue_CLOSE(adapted);

            if (rc != SQLITE_OK) {
                PyObject *exc = PyErr_GetRaisedException();
                sqlite3 *db = sqlite3_db_handle(self->st);
                set_error_from_db(state, db);
                _PyErr_ChainExceptions1(exc);
                return;
           }
        }
    } else {
        PyErr_SetString(state->ProgrammingError,
                        "parameters are of unsupported type");
    }
}

PyObject *
_pysqlite_query_execute(pysqlite_Cursor* self, int multiple, PyObject* operation, PyObject* second_argument)
{
    PyObject* parameters_list = NULL;
    PyObject* parameters_iter = NULL;
    PyObject* parameters = NULL;
    int i;
    int rc;
    int numcols;
    PyObject* column_name;

    if (!check_cursor(self)) {
        goto error;
    }

    self->locked = 1;

    if (multiple) {
        if (PyIter_Check(second_argument)) {
            /* iterator */
            parameters_iter = Py_NewRef(second_argument);
        } else {
            /* sequence */
            parameters_iter = _PyValue_GetIter(PyValue_FromObjectBorrow(second_argument));
            if (!parameters_iter) {
                goto error;
            }
        }
    } else {
        parameters_list = PyList_New(0);
        if (!parameters_list) {
            goto error;
        }

        if (second_argument == NULL) {
            second_argument = PyTuple_New(0);
            if (!second_argument) {
                goto error;
            }
        } else {
            Py_INCREF(second_argument);
        }
        if (PyList_AppendValue(parameters_list,
                               PyValue_FromObjectBorrow(second_argument)) != 0) {
            Py_DECREF(second_argument);
            goto error;
        }
        Py_DECREF(second_argument);

        parameters_iter = _PyValue_GetIter(PyValue_FromObjectBorrow(parameters_list));
        if (!parameters_iter) {
            goto error;
        }
    }

    /* reset description */
    Py_INCREF(Py_None);
    Py_SETREF(self->description, Py_None);

    if (self->statement) {
        // Reset pending statements on this cursor.
        if (stmt_reset(self->statement) != SQLITE_OK) {
            goto reset_failure;
        }
    }

    PyObject *stmt = get_statement_from_cache(self, operation);
    Py_XSETREF(self->statement, (pysqlite_Statement *)stmt);
    if (!self->statement) {
        goto error;
    }

    pysqlite_state *state = self->connection->state;
    if (multiple && sqlite3_stmt_readonly(self->statement->st)) {
        PyErr_SetString(state->ProgrammingError,
                        "executemany() can only execute DML statements.");
        goto error;
    }

    if (sqlite3_stmt_busy(self->statement->st)) {
        Py_SETREF(self->statement,
                  pysqlite_statement_create(self->connection, operation));
        if (self->statement == NULL) {
            goto error;
        }
    }

    if (stmt_reset(self->statement) != SQLITE_OK) {
        goto reset_failure;
    }
    self->rowcount = self->statement->is_dml ? 0L : -1L;

    /* We start a transaction implicitly before a DML statement.
       SELECT is the only exception. See #9924. */
    if (self->connection->autocommit == AUTOCOMMIT_LEGACY
        && self->connection->isolation_level
        && self->statement->is_dml
        && sqlite3_get_autocommit(self->connection->db))
    {
        if (begin_transaction(self->connection) < 0) {
            goto error;
        }
    }

    assert(!sqlite3_stmt_busy(self->statement->st));
    while (1) {
        parameters = PyIter_Next(parameters_iter);
        if (!parameters) {
            break;
        }

        bind_parameters(state, self->statement, parameters);
        if (PyErr_Occurred()) {
            goto error;
        }

        rc = stmt_step(self->statement->st);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            if (PyErr_Occurred()) {
                /* there was an error that occurred in a user-defined callback */
                if (state->enable_callback_tracebacks) {
                    PyErr_Print();
                } else {
                    PyErr_Clear();
                }
            }
            set_error_from_db(state, self->connection->db);
            goto error;
        }

        if (pysqlite_build_row_cast_map(self) != 0) {
            _PyErr_FormatFromCause(state->OperationalError,
                                   "Error while building row_cast_map");
            goto error;
        }

        assert(rc == SQLITE_ROW || rc == SQLITE_DONE);
        Py_BEGIN_ALLOW_THREADS
        numcols = sqlite3_column_count(self->statement->st);
        Py_END_ALLOW_THREADS
        if (self->description == Py_None && numcols > 0) {
            Py_SETREF(self->description, PyTuple_New(numcols));
            if (!self->description) {
                goto error;
            }
            for (i = 0; i < numcols; i++) {
                const char *colname;
                colname = sqlite3_column_name(self->statement->st, i);
                if (colname == NULL) {
                    PyErr_NoMemory();
                    goto error;
                }
                column_name = _pysqlite_build_column_name(self, colname);
                if (column_name == NULL) {
                    goto error;
                }
                PyValueBorrow none = PyValue_Borrow(PyValue_None);
                PyObject *descriptor = _PyTuple_PackValues(
                    7, (PyValueBorrow[]){PyValue_FromObjectBorrow(column_name),
                                         none, none, none, none, none, none});
                Py_DECREF(column_name);
                if (descriptor == NULL) {
                    goto error;
                }
                PyTuple_SET_VALUE(self->description, i, PyValue_PrepareOwned(descriptor));
            }
        }

        if (rc == SQLITE_DONE) {
            if (self->statement->is_dml) {
                self->rowcount += (long)sqlite3_changes(self->connection->db);
            }
            if (stmt_reset(self->statement) != SQLITE_OK) {
                goto reset_failure;
            }
        }
        Py_XDECREF(parameters);
    }

    if (!multiple) {
        sqlite_int64 lastrowid;

        Py_BEGIN_ALLOW_THREADS
        lastrowid = sqlite3_last_insert_rowid(self->connection->db);
        Py_END_ALLOW_THREADS

        Py_SETREF(self->lastrowid, PyLong_FromLongLong(lastrowid));
        // Fall through on error.
    }

error:
    Py_XDECREF(parameters);
    Py_XDECREF(parameters_iter);
    Py_XDECREF(parameters_list);

    self->locked = 0;

    if (PyErr_Occurred()) {
        if (self->statement) {
            sqlite3 *db = sqlite3_db_handle(self->statement->st);
            int sqlite3_state = sqlite3_errcode(db);
            // stmt_reset() may return a previously set exception,
            // either triggered because of Python or sqlite3.
            rc = stmt_reset(self->statement);
            Py_CLEAR(self->statement);
            if (sqlite3_state == SQLITE_OK && rc != SQLITE_OK) {
                cursor_cannot_reset_stmt_error(self, 1);
            }
        }
        self->rowcount = -1L;
        return NULL;
    }
    if (self->statement && !sqlite3_stmt_busy(self->statement->st)) {
        Py_CLEAR(self->statement);
    }
    return Py_NewRef((PyObject *)self);

reset_failure:
    /* suite to execute when stmt_reset() failed and no exception is set */
    assert(!PyErr_Occurred());

    Py_XDECREF(parameters);
    Py_XDECREF(parameters_iter);
    Py_XDECREF(parameters_list);

    self->locked = 0;
    self->rowcount = -1L;
    Py_CLEAR(self->statement);
    cursor_cannot_reset_stmt_error(self, 0);
    return NULL;
}

/*[clinic input]
_sqlite3.Cursor.execute as pysqlite_cursor_execute

    sql: unicode
    parameters: object(c_default = 'NULL') = ()
    /

Executes an SQL statement.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_execute_impl(pysqlite_Cursor *self, PyObject *sql,
                             PyValueBorrow parameters)
/*[clinic end generated code: output=22322b53fb390d53 input=a8e0200a11627f94]*/
{
    PyObject *parameters_obj = NULL;
    if (!PyValue_IsNull(parameters)) {
        parameters_obj = _PyValue_AsObjectAt(parameters, BOX_SITE_PARSER);
        if (parameters_obj == NULL) {
            return PyValue_NULL;
        }
    }
    PyObject *res = _pysqlite_query_execute(self, 0, sql, parameters_obj);
    Py_XDECREF(parameters_obj);
    return _PyValue_FromObjectResult(res);
}

/*[clinic input]
_sqlite3.Cursor.executemany as pysqlite_cursor_executemany

    sql: unicode
    seq_of_parameters: object
    /

Repeatedly executes an SQL statement.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_executemany_impl(pysqlite_Cursor *self, PyObject *sql,
                                 PyValueBorrow seq_of_parameters)
/*[clinic end generated code: output=ca1c214fafefdf61 input=0d0a52e5eb7ccd35]*/
{
    PyObject *seq_obj = _PyValue_AsObjectAt(seq_of_parameters, BOX_SITE_PARSER);
    if (seq_obj == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = _pysqlite_query_execute(self, 1, sql, seq_obj);
    Py_DECREF(seq_obj);
    return _PyValue_FromObjectResult(res);
}

/*[clinic input]
_sqlite3.Cursor.executescript as pysqlite_cursor_executescript

    sql_script: str
    /

Executes multiple SQL statements at once.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_executescript_impl(pysqlite_Cursor *self,
                                   const char *sql_script)
/*[clinic end generated code: output=6bc781c8e36450f1 input=78f093be415a8a2c]*/
{
    if (!check_cursor(self)) {
        return PyValue_NULL;
    }

    size_t sql_len = strlen(sql_script);
    int max_length = sqlite3_limit(self->connection->db,
                                   SQLITE_LIMIT_SQL_LENGTH, -1);
    if (sql_len > (unsigned)max_length) {
        PyErr_SetString(self->connection->DataError,
                        "query string is too large");
        return PyValue_NULL;
    }

    // Commit if needed
    sqlite3 *db = self->connection->db;
    if (self->connection->autocommit == AUTOCOMMIT_LEGACY
        && !sqlite3_get_autocommit(db))
    {
        int rc = SQLITE_OK;

        Py_BEGIN_ALLOW_THREADS
        rc = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
        Py_END_ALLOW_THREADS

        if (rc != SQLITE_OK) {
            goto error;
        }
    }

    while (1) {
        int rc;
        const char *tail;

        Py_BEGIN_ALLOW_THREADS
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, sql_script, (int)sql_len + 1, &stmt,
                                &tail);
        if (rc == SQLITE_OK) {
            do {
                rc = sqlite3_step(stmt);
            } while (rc == SQLITE_ROW);
            rc = sqlite3_finalize(stmt);
        }
        Py_END_ALLOW_THREADS

        if (rc != SQLITE_OK) {
            goto error;
        }

        if (*tail == (char)0) {
            break;
        }
        sql_len -= (tail - sql_script);
        sql_script = tail;
    }

    return _PyValue_FromObjectResult(Py_NewRef((PyObject *)self));

error:
    set_error_from_db(self->connection->state, db);
    return PyValue_NULL;
}

static PyObject *
pysqlite_cursor_iternext(PyObject *op)
{
    pysqlite_Cursor *self = _pysqlite_Cursor_CAST(op);
    if (!check_cursor(self)) {
        return NULL;
    }

    if (self->statement == NULL) {
        return NULL;
    }

    sqlite3_stmt *stmt = self->statement->st;
    assert(stmt != NULL);
    assert(sqlite3_data_count(stmt) != 0);

    self->locked = 1;  // GH-80254: Prevent recursive use of cursors.
    PyObject *row = _pysqlite_fetch_one_row(self);
    self->locked = 0;
    if (row == NULL) {
        return NULL;
    }
    int rc = stmt_step(stmt);
    if (rc == SQLITE_DONE) {
        if (self->statement->is_dml) {
            self->rowcount = (long)sqlite3_changes(self->connection->db);
        }
        rc = stmt_reset(self->statement);
        Py_CLEAR(self->statement);
        if (rc != SQLITE_OK) {
            goto reset_failure;
        }
    }
    else if (rc != SQLITE_ROW) {
        rc = set_error_from_db(self->connection->state, self->connection->db);
        int reset_rc = stmt_reset(self->statement);
        Py_CLEAR(self->statement);
        Py_DECREF(row);
        if (rc == SQLITE_OK && reset_rc != SQLITE_OK) {
            goto reset_failure;
        }
        return NULL;
    }
    if (!Py_IsNone(self->row_factory)) {
        PyObject *factory = self->row_factory;
        PyObject *args[] = { op, row, };
        PyObject *new_row = PyObject_Vectorcall(factory, args, 2, NULL);
        Py_SETREF(row, new_row);
    }
    return row;

reset_failure:
    cursor_cannot_reset_stmt_error(self, 0);
    return NULL;
}

/*[clinic input]
_sqlite3.Cursor.fetchone as pysqlite_cursor_fetchone

Fetches one row from the resultset.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_fetchone_impl(pysqlite_Cursor *self)
/*[clinic end generated code: output=cb5c15e32d37c9e3 input=e78294ec5980fdba]*/
{
    PyObject* row;

    row = pysqlite_cursor_iternext((PyObject *)self);
    if (!row && !PyErr_Occurred()) {
        return PyValue_None;
    }

    return _PyValue_FromObjectResult(row);
}

/*[clinic input]
_sqlite3.Cursor.fetchmany as pysqlite_cursor_fetchmany

    size as maxrows: uint32(c_default='((pysqlite_Cursor *)PyValue_AsKnownObjectBorrow(self))->arraysize') = 1
        The default value is set by the Cursor.arraysize attribute.

Fetches several rows from the resultset.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_fetchmany_impl(pysqlite_Cursor *self, uint32_t maxrows)
/*[clinic end generated code: output=5147c7c64dc36886 input=3b61411a66234f03]*/
{
    PyObject* row;
    PyObject* list;

    list = PyList_New(0);
    if (!list) {
        return PyValue_NULL;
    }

    while (maxrows > 0 && (row = pysqlite_cursor_iternext((PyObject *)self))) {
        int rc = PyList_AppendValue(list, PyValue_FromObjectBorrow(row));
        Py_DECREF(row);
        if (rc < 0) {
            break;
        }
        maxrows--;
    }

    if (PyErr_Occurred()) {
        Py_DECREF(list);
        return PyValue_NULL;
    } else {
        return _PyValue_FromObjectResult(list);
    }
}

/*[clinic input]
_sqlite3.Cursor.fetchall as pysqlite_cursor_fetchall

Fetches all rows from the resultset.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_fetchall_impl(pysqlite_Cursor *self)
/*[clinic end generated code: output=09794bb8fb398a91 input=f5d401086a8df25a]*/
{
    PyObject* row;
    PyObject* list;

    list = PyList_New(0);
    if (!list) {
        return PyValue_NULL;
    }

    while ((row = pysqlite_cursor_iternext((PyObject *)self))) {
        if (PyList_AppendValue(list, PyValue_FromObjectBorrow(row)) < 0) {
            Py_DECREF(row);
            break;
        }
        Py_DECREF(row);
    }

    if (PyErr_Occurred()) {
        Py_DECREF(list);
        return PyValue_NULL;
    } else {
        return _PyValue_FromObjectResult(list);
    }
}

/*[clinic input]
_sqlite3.Cursor.setinputsizes as pysqlite_cursor_setinputsizes

    sizes: object
    /

Required by DB-API. Does nothing in sqlite3.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_setinputsizes_impl(pysqlite_Cursor *self,
                                   PyValueBorrow sizes)
/*[clinic end generated code: output=4437be8d019f9fbd input=de7950a3aec79bdf]*/
{
    return PyValue_None;
}

/*[clinic input]
_sqlite3.Cursor.setoutputsize as pysqlite_cursor_setoutputsize

    size: object
    column: object = None
    /

Required by DB-API. Does nothing in sqlite3.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_setoutputsize_impl(pysqlite_Cursor *self, PyValueBorrow size,
                                   PyValueBorrow column)
/*[clinic end generated code: output=94ac584c2bee94f4 input=607a6bece8bbb273]*/
{
    return PyValue_None;
}

/*[clinic input]
_sqlite3.Cursor.close as pysqlite_cursor_close

Closes the cursor.
[clinic start generated code]*/

static PyValue
pysqlite_cursor_close_impl(pysqlite_Cursor *self)
/*[clinic end generated code: output=89608b93f895fde6 input=08b36552dbb9a986]*/
{
    if (!check_cursor_locked(self)) {
        return PyValue_NULL;
    }

    if (!self->connection) {
        PyTypeObject *tp = Py_TYPE(self);
        pysqlite_state *state = pysqlite_get_state_by_type(tp);
        PyErr_SetString(state->ProgrammingError,
                        "Base Cursor.__init__ not called.");
        return PyValue_NULL;
    }
    if (!pysqlite_check_thread(self->connection) || !pysqlite_check_connection(self->connection)) {
        return PyValue_NULL;
    }

    if (self->statement) {
        int rc = stmt_reset(self->statement);
        // Force self->statement to be NULL even if stmt_reset() may have
        // failed to avoid a possible double-free if someone calls close()
        // twice as a leak here would be better than a double-free.
        Py_CLEAR(self->statement);
        if (rc != SQLITE_OK) {
            cursor_cannot_reset_stmt_error(self, 0);
            return PyValue_NULL;
        }
    }

    self->closed = 1;

    return PyValue_None;
}

/*[clinic input]
@getter
_sqlite3.Cursor.arraysize
[clinic start generated code]*/

static PyObject *
_sqlite3_Cursor_arraysize_get_impl(pysqlite_Cursor *self)
/*[clinic end generated code: output=e0919d97175e6c50 input=3278f8d3ecbd90e3]*/
{
    return PyLong_FromUInt32(self->arraysize);
}

/*[clinic input]
@setter
_sqlite3.Cursor.arraysize
    value: uint32
[clinic start generated code]*/

static int
_sqlite3_Cursor_arraysize_set_impl(pysqlite_Cursor *self, uint32_t value)
/*[clinic end generated code: output=465c2db6df904802 input=80234ca4ec5cfb7c]*/
{
    self->arraysize = value;
    return 0;
}

static PyMethodDef cursor_methods[] = {
    PYSQLITE_CURSOR_CLOSE_METHODDEF
    PYSQLITE_CURSOR_EXECUTEMANY_METHODDEF
    PYSQLITE_CURSOR_EXECUTESCRIPT_METHODDEF
    PYSQLITE_CURSOR_EXECUTE_METHODDEF
    PYSQLITE_CURSOR_FETCHALL_METHODDEF
    PYSQLITE_CURSOR_FETCHMANY_METHODDEF
    PYSQLITE_CURSOR_FETCHONE_METHODDEF
    PYSQLITE_CURSOR_SETINPUTSIZES_METHODDEF
    PYSQLITE_CURSOR_SETOUTPUTSIZE_METHODDEF
    {NULL, NULL}
};

static struct PyMemberDef cursor_members[] =
{
    {"connection", _Py_T_OBJECT, offsetof(pysqlite_Cursor, connection), Py_READONLY},
    {"description", _Py_T_OBJECT, offsetof(pysqlite_Cursor, description), Py_READONLY},
    {"lastrowid", _Py_T_OBJECT, offsetof(pysqlite_Cursor, lastrowid), Py_READONLY},
    {"rowcount", Py_T_LONG, offsetof(pysqlite_Cursor, rowcount), Py_READONLY},
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(pysqlite_Cursor, in_weakreflist), Py_READONLY},
    {NULL}
};

static PyObject *
cursor_get_row_factory(PyObject *op, void *Py_UNUSED(closure))
{
    pysqlite_Cursor *self = _pysqlite_Cursor_CAST(op);
    return Py_NewRef(self->row_factory);
}

static int
cursor_set_row_factory(PyObject *op, PyObject *value, void *Py_UNUSED(closure))
{
    pysqlite_Cursor *self = _pysqlite_Cursor_CAST(op);
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError,
                        "cannot delete row_factory attribute");
        return -1;
    }
    Py_XSETREF(self->row_factory, Py_NewRef(value));
    return 0;
}

static struct PyGetSetDef cursor_getsets[] = {
    _SQLITE3_CURSOR_ARRAYSIZE_GETSETDEF
    {"row_factory", cursor_get_row_factory, cursor_set_row_factory},
    {NULL},
};

static const char cursor_doc[] =
PyDoc_STR("SQLite database cursor class.");

static PyType_Slot cursor_slots[] = {
    {Py_tp_dealloc, cursor_dealloc},
    {Py_tp_doc, (void *)cursor_doc},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, pysqlite_cursor_iternext},
    {Py_tp_methods, cursor_methods},
    {Py_tp_members, cursor_members},
    {Py_tp_getset, cursor_getsets},
    {Py_tp_init, pysqlite_cursor_init},
    {Py_tp_traverse, cursor_traverse},
    {Py_tp_clear, cursor_clear},
    {0, NULL},
};

static PyType_Spec cursor_spec = {
    .name = MODULE_NAME ".Cursor",
    .basicsize = sizeof(pysqlite_Cursor),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = cursor_slots,
};

int
pysqlite_cursor_setup_types(PyObject *module)
{
    PyObject *type = PyType_FromModuleAndSpec(module, &cursor_spec, NULL);
    if (type == NULL) {
        return -1;
    }
    pysqlite_state *state = pysqlite_get_state(module);
    state->CursorType = (PyTypeObject *)type;
    return 0;
}
