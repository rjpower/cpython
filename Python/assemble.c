#include "Python.h"
#include "pycore_code.h"            // write_location_entry_start()
#include "pycore_compile.h"
#include "pycore_dict.h"          // _PyDict_NextValue()
#include "pycore_flowgraph.h"
#include "pycore_intrinsics.h"      // INTRINSIC_COMPARE_OP
#include "pycore_ir.h"                // IR_WORD()
#include "pycore_ir_source.h"
#include "pycore_list.h"              // PyList_GET_VALUE()
#include "pycore_long.h"              // _PyLong_CheckExactAndCompact()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_opcode_utils.h"    // IS_BACKWARDS_JUMP_OPCODE
#include "pycore_opcode_metadata.h" // _PyOpcode_num_popped
#include "pycore_regcfg.h"          // _PyRegCfg
#include "pycore_setobject.h"     // PySet_ContainsValue()
#include "pycore_symtable.h"        // _Py_SourceLocation
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()
#include "pycore_value.h"           // Canonical constant representation

#include <stdbool.h>

#define DEFAULT_LNOTAB_SIZE 16
#define DEFAULT_CNOTAB_SIZE 32

#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) < 0) {          \
        return ERROR;       \
    }

typedef _Py_SourceLocation location;

static inline bool
same_location(location a, location b)
{
    return a.lineno == b.lineno &&
           a.end_lineno == b.end_lineno &&
           a.col_offset == b.col_offset &&
           a.end_col_offset == b.end_col_offset;
}

/* The assembler encodes the ops of the register CFG (Python/regcfg.c),
 * whose slots compaction has assigned.  It sets each op's handler, the
 * generic one before relaxation; its feedback index, or -1; the index of
 * its word, or of the next for an op without one; and its number of words:
 * 1, 0 for an op that a pass deleted, or with the words that a relaxed loop
 * adds more. */
struct assembler {
    int a_nwords;
    int a_nfb;                 /* feedback entries, entry 0 included */
    PyObject *a_bytecode;      /* bytes containing the words */
    PyObject *a_fbinit;        /* bytes containing the feedback table */
    PyObject *a_constslots;    /* bytes: the constant-slot table */
    PyBytesWriter *a_except_table_writer; /* writer containing exception table */
    PyObject *a_except_table;  /* bytes containing exception table */
    int a_except_table_off;    /* offset into exception table */
    /* Location Info */
    int a_lineno;          /* lineno of last emitted instruction */
    PyBytesWriter *a_linetable_writer; /* writer containing location info */
    PyObject *a_linetable; /* bytes object containing location info */
    int a_location_off;    /* offset of last written location info frame */
};

static int
assemble_init(struct assembler *a, int firstlineno)
{
    a->a_lineno = firstlineno;
    a->a_linetable_writer = PyBytesWriter_Create(DEFAULT_CNOTAB_SIZE);
    if (a->a_linetable_writer == NULL) {
        goto error;
    }
    a->a_except_table_writer = PyBytesWriter_Create(DEFAULT_LNOTAB_SIZE);
    if (a->a_except_table_writer == NULL) {
        goto error;
    }
    return SUCCESS;
error:
    PyBytesWriter_Discard(a->a_linetable_writer);
    a->a_linetable_writer = NULL;
    PyBytesWriter_Discard(a->a_except_table_writer);
    a->a_except_table_writer = NULL;
    return ERROR;
}

static void
assemble_free(struct assembler *a)
{
    PyBytesWriter_Discard(a->a_linetable_writer);
    PyBytesWriter_Discard(a->a_except_table_writer);
    Py_XDECREF(a->a_bytecode);
    Py_XDECREF(a->a_fbinit);
    Py_XDECREF(a->a_constslots);
    Py_XDECREF(a->a_linetable);
    Py_XDECREF(a->a_except_table);
}

static inline void
write_except_byte(struct assembler *a, int byte) {
    unsigned char *p = PyBytesWriter_GetData(a->a_except_table_writer);
    p[a->a_except_table_off++] = byte;
}

#define CONTINUATION_BIT 64

static void
assemble_emit_exception_table_item(struct assembler *a, int value, int msb)
{
    assert ((msb | 128) == 128);
    assert(value >= 0 && value < (1 << 30));
    if (value >= 1 << 24) {
        write_except_byte(a, (value >> 24) | CONTINUATION_BIT | msb);
        msb = 0;
    }
    if (value >= 1 << 18) {
        write_except_byte(a, ((value >> 18)&0x3f) | CONTINUATION_BIT | msb);
        msb = 0;
    }
    if (value >= 1 << 12) {
        write_except_byte(a, ((value >> 12)&0x3f) | CONTINUATION_BIT | msb);
        msb = 0;
    }
    if (value >= 1 << 6) {
        write_except_byte(a, ((value >> 6)&0x3f) | CONTINUATION_BIT | msb);
        msb = 0;
    }
    write_except_byte(a, (value&0x3f) | msb);
}

/* See InternalDocs/exception_handling.md for details of layout */
#define MAX_SIZE_OF_ENTRY 20

static int
assemble_emit_exception_table_entry(struct assembler *a, int start, int end,
                                    _PyRegCfg *cfg, int h)
{
    Py_ssize_t len = PyBytesWriter_GetSize(a->a_except_table_writer);
    if (a->a_except_table_off + MAX_SIZE_OF_ENTRY >= len) {
        RETURN_IF_ERROR(PyBytesWriter_Resize(a->a_except_table_writer, len * 2));
    }
    int size = end-start;
    assert(end > start);
    _PyRegBlock *handler = &cfg->blocks[h];
    int target = handler->word;
    /* The handler starts with the exception on top of the stack, at depth
     * startdepth - 1, and lasti, if it keeps it, below: the entry names
     * the slot of the exception, whose temporary is canonical. */
    int exc_slot = cfg->nlocalsplus + handler->startdepth - 1;
    assert(exc_slot - handler->preserve_lasti >= cfg->nlocalsplus);
    int slot_lasti = (exc_slot<<1) | handler->preserve_lasti;
    assemble_emit_exception_table_item(a, start, (1<<7));
    assemble_emit_exception_table_item(a, size, 0);
    assemble_emit_exception_table_item(a, target, 0);
    assemble_emit_exception_table_item(a, slot_lasti, 0);
    return SUCCESS;
}

static int
assemble_exception_table(struct assembler *a, _PyRegCfg *cfg)
{
    int ioffset = 0;
    int handler = -1;
    int start = -1;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->except != handler) {
            if (handler >= 0 && ioffset > start) {
                RETURN_IF_ERROR(
                    assemble_emit_exception_table_entry(a, start, ioffset,
                                                        cfg, handler));
            }
            start = ioffset;
            handler = op->except;
        }
        ioffset += op->nwords;
    }
    if (handler >= 0 && ioffset > start) {
        RETURN_IF_ERROR(assemble_emit_exception_table_entry(a, start, ioffset,
                                                            cfg, handler));
    }
    return SUCCESS;
}


/* Code location emitting code. See locations.md for a description of the format. */

#define MSB 0x80

static void
write_location_byte(struct assembler* a, int val)
{
    uint8_t *linetable = PyBytesWriter_GetData(a->a_linetable_writer);
    linetable[a->a_location_off] = val & 255;
    a->a_location_off++;
}


static uint8_t *
location_pointer(struct assembler* a)
{
    uint8_t *linetable = PyBytesWriter_GetData(a->a_linetable_writer);
    return linetable + a->a_location_off;
}

static void
write_location_first_byte(struct assembler* a, int code, int length)
{
    a->a_location_off += write_location_entry_start(
        location_pointer(a), code, length);
}

static void
write_location_varint(struct assembler* a, unsigned int val)
{
    uint8_t *ptr = location_pointer(a);
    a->a_location_off += write_varint(ptr, val);
}


static void
write_location_signed_varint(struct assembler* a, int val)
{
    uint8_t *ptr = location_pointer(a);
    a->a_location_off += write_signed_varint(ptr, val);
}

static void
write_location_info_short_form(struct assembler* a, int length, int column, int end_column)
{
    assert(length > 0 &&  length <= 8);
    int column_low_bits = column & 7;
    int column_group = column >> 3;
    assert(column < 80);
    assert(end_column >= column);
    assert(end_column - column < 16);
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_SHORT0 + column_group, length);
    write_location_byte(a, (column_low_bits << 4) | (end_column - column));
}

static void
write_location_info_oneline_form(struct assembler* a, int length, int line_delta, int column, int end_column)
{
    assert(length > 0 &&  length <= 8);
    assert(line_delta >= 0 && line_delta < 3);
    assert(column < 128);
    assert(end_column < 128);
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_ONE_LINE0 + line_delta, length);
    write_location_byte(a, column);
    write_location_byte(a, end_column);
}

static void
write_location_info_long_form(struct assembler* a, location loc, int length)
{
    assert(length > 0 &&  length <= 8);
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_LONG, length);
    write_location_signed_varint(a, loc.lineno - a->a_lineno);
    assert(loc.end_lineno >= loc.lineno);
    write_location_varint(a, loc.end_lineno - loc.lineno);
    write_location_varint(a, loc.col_offset + 1);
    write_location_varint(a, loc.end_col_offset + 1);
}

static void
write_location_info_none(struct assembler* a, int length)
{
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_NONE, length);
}

static void
write_location_info_no_column(struct assembler* a, int length, int line_delta)
{
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_NO_COLUMNS, length);
    write_location_signed_varint(a, line_delta);
}

#define THEORETICAL_MAX_ENTRY_SIZE 25 /* 1 + 6 + 6 + 6 + 6 */


static int
write_location_info_entry(struct assembler* a, location loc, int isize)
{
    Py_ssize_t len = PyBytesWriter_GetSize(a->a_linetable_writer);
    if (a->a_location_off + THEORETICAL_MAX_ENTRY_SIZE >= len) {
        assert(len > THEORETICAL_MAX_ENTRY_SIZE);
        RETURN_IF_ERROR(PyBytesWriter_Resize(a->a_linetable_writer, len * 2));
    }
    if (loc.lineno == NO_LOCATION.lineno) {
        write_location_info_none(a, isize);
        return SUCCESS;
    }
    int line_delta = loc.lineno - a->a_lineno;
    int column = loc.col_offset;
    int end_column = loc.end_col_offset;
    if (column < 0 || end_column < 0) {
        if (loc.end_lineno == loc.lineno || loc.end_lineno < 0) {
            write_location_info_no_column(a, isize, line_delta);
            a->a_lineno = loc.lineno;
            return SUCCESS;
        }
    }
    else if (loc.end_lineno == loc.lineno) {
        if (line_delta == 0 && column < 80 && end_column - column < 16 && end_column >= column) {
            write_location_info_short_form(a, isize, column, end_column);
            return SUCCESS;
        }
        if (line_delta >= 0 && line_delta < 3 && column < 128 && end_column < 128) {
            write_location_info_oneline_form(a, isize, line_delta, column, end_column);
            a->a_lineno = loc.lineno;
            return SUCCESS;
        }
    }
    write_location_info_long_form(a, loc, isize);
    a->a_lineno = loc.lineno;
    return SUCCESS;
}

static int
assemble_emit_location(struct assembler* a, location loc, int isize)
{
    if (isize == 0) {
        return SUCCESS;
    }
    while (isize > 8) {
        RETURN_IF_ERROR(write_location_info_entry(a, loc, 8));
        isize -= 8;
    }
    return write_location_info_entry(a, loc, isize);
}

static int
assemble_location_info(struct assembler *a, _PyRegCfg *cfg, int firstlineno)
{
    a->a_lineno = firstlineno;
    location loc = NO_LOCATION;
    int size = 0;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (!same_location(loc, op->loc)) {
                RETURN_IF_ERROR(assemble_emit_location(a, loc, size));
                loc = op->loc;
                size = 0;
        }
        size += op->nwords;
    }
    RETURN_IF_ERROR(assemble_emit_location(a, loc, size));
    return SUCCESS;
}

/* Source-only operations occupy no word. Find the actual continuation first,
 * then record only this edge's eliminated source lines and exception state. */
static int
assemble_source_edge(PyBytesWriter *writer, _PyRegCfg *cfg, int from,
                     int edge, int block, int first)
{
    int b = block, i = first;
    while (b < cfg->nblocks) {
        if (i == cfg->blocks[b].end) {
            if (!cfg->blocks[b].falls_through || b + 1 == cfg->nblocks) {
                return SUCCESS;
            }
            i = cfg->blocks[++b].start;
            continue;
        }
        if (cfg->ops[i].nwords) {
            break;
        }
        i++;
    }
    if (b == cfg->nblocks) {
        return SUCCESS;
    }
    int to = cfg->ops[i].word;
    for (int j = first; j < i; j++) {
        _PyRegOp *op = &cfg->ops[j];
        if (!(op->flags & _PyReg_SOURCE_ORIGIN) || op->loc.lineno <= 0 ||
            !(op->opcode == NOP || (op->flags & _PyReg_ELIDED_NOP)))
        {
            continue;
        }
        _PyIRSourceEvent event = {
            .from = from, .to = to, .edge = edge,
            .line = op->loc.lineno, .end_line = op->loc.end_lineno,
            .column = op->loc.col_offset, .end_column = op->loc.end_col_offset,
            .handler = -1, .exc_slot = 0, .lasti = 0,
        };
        if (op->except >= 0) {
            _PyRegBlock *handler = &cfg->blocks[op->except];
            event.handler = handler->word;
            event.exc_slot = cfg->nlocalsplus + handler->startdepth - 1;
            event.lasti = handler->preserve_lasti;
        }
        unsigned char encoded[_PyIR_SOURCE_RECORD];
        _PyIRSource_EncodeEvent(encoded, &event);
        RETURN_IF_ERROR(PyBytesWriter_WriteBytes(writer, encoded, sizeof(encoded)));
    }
    return SUCCESS;
}

static int
assemble_source_map(struct assembler *a, _PyRegCfg *cfg)
{
    PyBytesWriter *writer = PyBytesWriter_Create(0);
    if (writer == NULL) {
        return ERROR;
    }
    /* There is no executed predecessor before the real entry prologue.
     * Do not accept a leading source-only span without entry semantics. */
    for (int i = 0; i < cfg->nops && !cfg->ops[i].nwords; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if ((op->flags & _PyReg_SOURCE_ORIGIN) && op->loc.lineno > 0 &&
            (op->opcode == NOP || (op->flags & _PyReg_ELIDED_NOP)))
        {
            PyErr_SetString(PyExc_SystemError,
                            "source-only span before executable entry");
            goto error;
        }
    }
    for (int b = 0; b < cfg->nblocks; b++) {
        for (int i = cfg->blocks[b].start; i < cfg->blocks[b].end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (!op->nwords) {
                continue;
            }
            /* A fused word executes the compare's conditional jump too.
             * Use the same control operation as word encoding, while source
             * observers retain the actual emitted instruction's position. */
            int control_index = op->flags & _PyReg_FUSED ? op->fuse : i;
            _PyRegOp *control = &cfg->ops[control_index];
            assert(control_index >= i && control_index < cfg->blocks[b].end);
            assert(control == op || control->nwords == 0);
            const _PyIRFormat *fmt = _PyRegCfg_FormatOf(control->opcode);
            if (control->target >= 0 && !(fmt->flags & _PyIR_NAMES_TARGET)) {
                int target = control->target;
                if (assemble_source_edge(writer, cfg, op->word,
                                         _PyIR_SOURCE_TAKEN, target,
                                         cfg->blocks[target].start) < 0)
                {
                    goto error;
                }
            }
            if (!IS_NO_FALLTHROUGH_OPCODE(control->opcode) &&
                assemble_source_edge(writer, cfg, op->word,
                                     _PyIR_SOURCE_FALLTHROUGH, b, control_index + 1) < 0)
            {
                goto error;
            }
        }
    }
    PyObject *records = PyBytesWriter_Finish(writer);
    if (records == NULL) {
        return ERROR;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(records);
    PyObject *map = _PyIRSource_Pack(
        (const unsigned char *)PyBytes_AS_STRING(a->a_linetable),
        (uint32_t)PyBytes_GET_SIZE(a->a_linetable),
        (const unsigned char *)PyBytes_AS_STRING(records),
        (uint32_t)(size / _PyIR_SOURCE_RECORD));
    Py_DECREF(records);
    if (map == NULL) {
        return ERROR;
    }
    Py_SETREF(a->a_linetable, map);
    return SUCCESS;
error:
    PyBytesWriter_Discard(writer);
    return ERROR;
}

static int
assemble_emit(struct assembler *a, _PyRegCfg *cfg, int first_lineno,
              PyObject *const_cache)
{
    RETURN_IF_ERROR(assemble_init(a, first_lineno));

    RETURN_IF_ERROR(assemble_location_info(a, cfg, a->a_lineno));

    RETURN_IF_ERROR(assemble_exception_table(a, cfg));

    a->a_except_table = PyBytesWriter_FinishWithSize(a->a_except_table_writer,
                                                     a->a_except_table_off);
    a->a_except_table_writer = NULL;
    if (a->a_except_table == NULL) {
        return ERROR;
    }
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_except_table));

    a->a_linetable = PyBytesWriter_FinishWithSize(a->a_linetable_writer,
                                                  a->a_location_off);
    a->a_linetable_writer = NULL;
    if (a->a_linetable == NULL) {
        return ERROR;
    }
    RETURN_IF_ERROR(assemble_source_map(a, cfg));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_linetable));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_bytecode));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_fbinit));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_constslots));
    return SUCCESS;
}

static PyObject *
dict_keys_inorder(PyObject *dict, Py_ssize_t offset)
{
    PyObject *tuple;
    PyValue key = PyValue_NULL, value = PyValue_NULL;
    Py_ssize_t pos = 0, size = PyDict_GET_SIZE(dict);

    tuple = PyTuple_New(size);
    if (tuple == NULL)
        return NULL;
    if (PyDict_GET_SIZE(dict) != size) {
        PyErr_SetString(PyExc_RuntimeError,
                        "compiler names changed during tuple allocation");
        goto error;
    }
    Py_ssize_t count = 0;
    int status;
    while ((status = _PyDict_NextValue(dict, &pos, &key, &value, NULL)) > 0) {
        assert(PyValue_IsObject(key));
        assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(key)));
        assert(PyValue_LongCheckExact(value));
        Py_ssize_t i = PyValue_IsInlineInt(value) ? PyValue_InlineInt64(value) :
                       _PyValue_IntAsSsize_t(value);
        if (i == -1 && PyErr_Occurred()) {
            goto error;
        }
        if (i < offset || i - offset >= size ||
            !PyValue_IsNull(PyTuple_GET_VALUE(tuple, i - offset)))
        {
            PyErr_SetString(PyExc_SystemError, "invalid compiler name index");
            goto error;
        }
        PyTuple_SET_VALUE(tuple, i - offset, key);
        key = PyValue_NULL;
        PyValue_CLEAR(value);
        count++;
    }
    if (status < 0) {
        goto error;
    }
    if (count != size) {
        PyErr_SetString(PyExc_RuntimeError, "compiler names changed during iteration");
        goto error;
    }
    return tuple;

error:;
    PyValue_XCLOSE(key);
    PyValue_XCLOSE(value);
    Py_DECREF(tuple);
    return NULL;
}

// This is in codeobject.c.
extern void _Py_set_localsplus_info(int, PyObject *, unsigned char,
                                   PyObject *, PyObject *);

static int
compute_localsplus_info(_PyCompile_CodeUnitMetadata *umd, int nlocalsplus,
                        int nconst, int flags, PyObject *names,
                        PyObject *kinds)
{
    PyValue key = PyValue_NULL, value = PyValue_NULL;
    int status = 0;
    Py_ssize_t pos = 0;

    // Set the locals kinds.  Arg vars fill the first portion of the list.
    struct {
        int count;
        _PyLocals_Kind kind;
    }  argvarkinds[6] = {
        {(int)umd->u_posonlyargcount, CO_FAST_ARG_POS},
        {(int)umd->u_argcount, CO_FAST_ARG_POS | CO_FAST_ARG_KW},
        {(int)umd->u_kwonlyargcount, CO_FAST_ARG_KW},
        {!!(flags & CO_VARARGS), CO_FAST_ARG_VAR | CO_FAST_ARG_POS},
        {!!(flags & CO_VARKEYWORDS), CO_FAST_ARG_VAR | CO_FAST_ARG_KW},
        {-1, 0},  // the remaining local vars
    };
    int max = 0;
    for (int i = 0; i < 6; i++) {
        max = argvarkinds[i].count < 0
            ? INT_MAX
            : max + argvarkinds[i].count;
        while (pos < max &&
               (status = _PyDict_NextValue(umd->u_varnames, &pos,
                                            &key, &value, NULL)) > 0) {
            assert(PyValue_IsObject(key));
            assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(key)));
            assert(PyValue_LongCheckExact(value));
            PyObject *k = PyValue_AsKnownObjectBorrow(key);
            int offset = PyValue_IsInlineInt(value) ? (int)PyValue_InlineInt64(value) :
                         _PyValue_AsInt(value);
            if (offset == -1 && PyErr_Occurred()) {
                goto error;
            }
            if (offset < 0 || offset >= nlocalsplus) {
                PyErr_SetString(PyExc_SystemError, "invalid local variable index");
                goto error;
            }

            _PyLocals_Kind kind = CO_FAST_LOCAL | argvarkinds[i].kind;

            if (umd->u_fasthidden != NULL) {
                int hidden = PySet_ContainsValue(umd->u_fasthidden, PyValue_FromObjectBorrow(k));
                if (hidden < 0) {
                    goto error;
                }
                if (hidden) {
                    kind |= CO_FAST_HIDDEN;
                }
            }

            int has_key = PyDict_ContainsValue(umd->u_cellvars, PyValue_FromObjectBorrow(k));
            if (has_key < 0) {
                goto error;
            }
            if (has_key) {
                kind |= CO_FAST_CELL;
            }

            _Py_set_localsplus_info(offset, k, kind, names, kinds);
            PyValue_CLEAR(key);
            PyValue_CLEAR(value);
        }
        if (status < 0) {
            goto error;
        }
    }
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);

    // This counter mirrors the fix done in fix_cell_offsets().
    int numdropped = 0, cellvar_offset = -1;
    pos = 0;
    while ((status = _PyDict_NextValue(umd->u_cellvars, &pos,
                                      &key, &value, NULL)) > 0) {
        assert(PyValue_IsObject(key));
        assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(key)));
        assert(PyValue_LongCheckExact(value));
        PyObject *k = PyValue_AsKnownObjectBorrow(key);
        int has_name = PyDict_ContainsValue(umd->u_varnames, PyValue_FromObjectBorrow(k));
        if (has_name < 0) {
            goto error;
        }
        if (has_name) {
            // Skip cells that are already covered by locals.
            numdropped += 1;
            PyValue_CLEAR(key);
            PyValue_CLEAR(value);
            continue;
        }

        cellvar_offset = PyValue_IsInlineInt(value) ? (int)PyValue_InlineInt64(value) :
                         _PyValue_AsInt(value);
        if (cellvar_offset == -1 && PyErr_Occurred()) {
            goto error;
        }
        if (cellvar_offset < 0 || cellvar_offset >= nlocalsplus - nlocals + numdropped) {
            PyErr_SetString(PyExc_SystemError, "invalid cell variable index");
            goto error;
        }
        cellvar_offset += nlocals - numdropped;
        assert(cellvar_offset < nlocalsplus);
        _Py_set_localsplus_info(cellvar_offset, k, CO_FAST_CELL, names, kinds);
        PyValue_CLEAR(key);
        PyValue_CLEAR(value);
    }
    if (status < 0) {
        goto error;
    }

    pos = 0;
    while ((status = _PyDict_NextValue(umd->u_freevars, &pos,
                                      &key, &value, NULL)) > 0) {
        assert(PyValue_IsObject(key));
        assert(PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(key)));
        assert(PyValue_LongCheckExact(value));
        PyObject *k = PyValue_AsKnownObjectBorrow(key);
        int offset = PyValue_IsInlineInt(value) ? (int)PyValue_InlineInt64(value) :
                     _PyValue_AsInt(value);
        if (offset == -1 && PyErr_Occurred()) {
            goto error;
        }
        if (offset < 0 || offset >= nlocalsplus - nlocals + numdropped) {
            PyErr_SetString(PyExc_SystemError, "invalid free variable index");
            goto error;
        }
        // The constant slots come before the free variables
        offset += nlocals - numdropped + nconst;
        assert(offset < nlocalsplus);
        /* XXX If the assertion below fails it is most likely because a freevar
           was added to u_freevars with the wrong index due to not taking into
           account cellvars already present, see gh-128632. */
        assert(offset > cellvar_offset);
        _Py_set_localsplus_info(offset, k, CO_FAST_FREE, names, kinds);
        PyValue_CLEAR(key);
        PyValue_CLEAR(value);
    }
    if (status < 0) {
        return ERROR;
    }
    // The constant slots, named .k0, .k1, ... as the code constructor does
    int first = nlocalsplus - nconst - (int)PyDict_GET_SIZE(umd->u_freevars);
    for (int j = 0; j < nconst; j++) {
        PyObject *name = PyUnicode_FromFormat(".k%d", j);
        if (name == NULL) {
            return ERROR;
        }
        _Py_set_localsplus_info(first + j, name,
                                CO_FAST_CONST | CO_FAST_HIDDEN, names, kinds);
        Py_DECREF(name);
    }
    return SUCCESS;

error:;
    PyValue_XCLOSE(key);
    PyValue_XCLOSE(value);
    return ERROR;
}

static PyCodeObject *
makecode(_PyCompile_CodeUnitMetadata *umd, struct assembler *a, PyObject *const_cache,
         PyObject *constslist, int maxdepth, int nlocalsplus, int nconst,
         int code_flags,
         PyObject *filename)
{
    PyCodeObject *co = NULL;
    PyObject *names = NULL;
    PyObject *consts = NULL;
    PyObject *localsplusnames = NULL;
    PyObject *localspluskinds = NULL;
    names = dict_keys_inorder(umd->u_names, 0);
    if (!names) {
        goto error;
    }
    if (_PyCompile_ConstCacheMergeOne(const_cache, &names) < 0) {
        goto error;
    }

    consts = PyList_AsTuple(constslist); /* PyCode_New requires a tuple */
    if (consts == NULL) {
        goto error;
    }
    if (_PyCompile_ConstCacheMergeOne(const_cache, &consts) < 0) {
        goto error;
    }

    assert(umd->u_posonlyargcount < INT_MAX);
    assert(umd->u_argcount < INT_MAX);
    assert(umd->u_kwonlyargcount < INT_MAX);
    int posonlyargcount = (int)umd->u_posonlyargcount;
    int posorkwargcount = (int)umd->u_argcount;
    assert(INT_MAX - posonlyargcount - posorkwargcount > 0);
    int kwonlyargcount = (int)umd->u_kwonlyargcount;

    localsplusnames = PyTuple_New(nlocalsplus);
    if (localsplusnames == NULL) {
        goto error;
    }
    localspluskinds = PyBytes_FromStringAndSize(NULL, nlocalsplus);
    if (localspluskinds == NULL) {
        goto error;
    }
    if (compute_localsplus_info(
            umd, nlocalsplus, nconst, code_flags,
            localsplusnames, localspluskinds) == ERROR)
    {
        goto error;
    }

    struct _PyCodeConstructor con = {
        .filename = filename,
        .name = umd->u_name,
        .qualname = umd->u_qualname ? umd->u_qualname : umd->u_name,
        .flags = code_flags,

        .code = a->a_bytecode,
        .fbinit = a->a_fbinit,
        .constslots = a->a_constslots,
        .firstlineno = umd->u_firstlineno,
        .linetable = a->a_linetable,

        .consts = consts,
        .names = names,

        .localsplusnames = localsplusnames,
        .localspluskinds = localspluskinds,

        .argcount = posonlyargcount + posorkwargcount,
        .posonlyargcount = posonlyargcount,
        .kwonlyargcount = kwonlyargcount,

        .stacksize = maxdepth,

        .exceptiontable = a->a_except_table,
    };

    if (_PyCompile_ConstCacheMergeOne(const_cache, &localsplusnames) < 0) {
        goto error;
    }
    con.localsplusnames = localsplusnames;

    co = _PyCode_New(&con);
    if (co == NULL) {
        goto error;
    }

error:
    Py_XDECREF(names);
    Py_XDECREF(consts);
    Py_XDECREF(localsplusnames);
    Py_XDECREF(localspluskinds);
    return co;
}


/* Words: choosing handlers, feedback entries, layout and encoding */

/* The farthest exit of FOR_ITER and SEND that the assembler does not relax */
int _PyIR_MaxForward = IR_MAX_FORWARD;

/* Both outcomes name real continuations: fallthrough jumps over the exit
 * jump, and completion takes the exit jump. No cleanup word is a target. */
#define RELAXED_FOR_ITER_WORDS 3
#define RELAXED_SEND_WORDS 3

/* A code object past a register IR field limit: source can reach these, so
 * they are a SyntaxError, as for other compiler limits. */
static int
too_large(const char *what, int value)
{
    PyErr_Format(PyExc_SyntaxError,
                 "code object too large: %s %d exceeds the register IR limit",
                 what, value);
    return ERROR;
}

/* The generic handler of op: its opcode, a form that pushes its constant
 * inline, or for BINARY_OP the handler of its operation. */
static int
choose_handler(_PyRegOp *op, PyObject *consts)
{
    int opcode = op->opcode;
    int oparg = op->oparg;
    if (op->flags & _PyReg_CALL_NO_SELF) {
        switch (opcode) {
            case CALL:
                return CALL_NO_SELF;
            case CALL_KW:
                return CALL_KW_NO_SELF;
            case CALL_FUNCTION_EX:
                return op->flags & _PyReg_CALL_NO_KWARGS
                    ? CALL_EX_NO_SELF_NO_KW : CALL_EX_NO_SELF_KW;
            default:
                Py_UNREACHABLE();
        }
    }
    if (opcode == BINARY_OP) {
        if (oparg > NB_OPARG_LAST) {
            PyErr_Format(PyExc_SystemError,
                         "invalid BINARY_OP operation %d", oparg);
            return ERROR;
        }
        return _PyIR_BINARY_OP_FIRST + oparg;
    }
    // Canonical scalar constants have the same representation in every
    // supported interpreter.
    if (opcode == LOAD_SMALL_INT) {
        return LOAD_SMALL_INT_INLINE;
    }
    if (opcode == LOAD_CONST) {
        PyValueBorrow value = PyList_GET_VALUE(consts, oparg);
        if (PyValue_IsInlineInt(value)) {
            return LOAD_CONST_INLINE_INT;
        }
        if (PyValue_IsInlineFloat(value)) {
            return LOAD_CONST_INLINE_FLOAT;
        }
    }
    return opcode;
}

/* END_ASYNC_FOR names its semantic SEND site for branch observation. */
static int
jump_target(_PyRegCfg *cfg, _PyRegOp *op)
{
    assert(op->target >= 0);
    _PyRegBlock *b = &cfg->blocks[op->target];
    if (op->opcode == END_ASYNC_FOR) {
        _PyRegOp *send = &cfg->ops[b->start];
        assert(send->opcode == SEND);
        return send->word;
    }
    return b->word;
}

/* The oparg that the handlers of op read: the word index of the target of
 * a jump, the distance from the next word to the exit of FOR_ITER and
 * SEND, otherwise the oparg of the stack instruction. */
static int
handler_oparg(_PyRegCfg *cfg, _PyRegOp *op)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
    if (fmt->flags & _PyIR_JUMPS) {
        return jump_target(cfg, op);
    }
    if (fmt->flags & _PyIR_FORWARD) {
        if (op->nwords > 1) {
            return 1;
        }
        // A fused word jumps where the jump after the compare did
        _PyRegOp *jump = (fmt->flags & _PyIR_FUSED) ? &cfg->ops[op->fuse] : op;
        return jump_target(cfg, jump) - (op->word + 1);
    }
    if (op->opcode == LOAD_GLOBAL) {
        // The NULL bit is only the stack model's: the slot is NULL already
        return op->oparg & ~1;
    }
    return op->oparg;
}

/* The order in which sites get feedback entries (pyjit/design/register-ir.md
 * G5): sites whose handler keeps its operation in the entry, which cannot
 * run without one; then, in a function-like body, sites inside a loop; then
 * the rest. */
enum {
    FB_CLASS_REQUIRED,
    FB_CLASS_LOOP,
    FB_CLASS_OTHER,
    FB_NCLASSES,
};

/* Whether the handler needs its entry for its operand: COMPARE_OP and
 * CONTAINS_OP, and the fused words, which keep the operation of their compare
 * and the sense of their jump there.  BINARY_OP has a handler per operation,
 * which runs at entry 0. */
static int
needs_entry(int handler)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(handler);
    return (fmt->imm == _PyIR_LOC_FBARG
            && !(handler >= _PyIR_BINARY_OP_FIRST
                 && handler <= _PyIR_BINARY_OP_FIRST + NB_OPARG_LAST))
           || (fmt->flags & _PyIR_FUSED);
}

/* The arg of the entry of a fused word: the operation of the compare and
 * the sense of the jump that follows. */
static int
fused_arg(_PyRegCfg *cfg, _PyRegOp *op)
{
    int arg = op->oparg & IR_FUSED_OP_MASK;
    if (cfg->ops[op->fuse].opcode == POP_JUMP_IF_FALSE) {
        arg |= _PyIR_FUSED_IF_FALSE;
    }
    return arg;
}

/* The generic handler of the fused pair that op starts */
static int
fused_handler(_PyRegOp *op)
{
    switch (op->opcode) {
        case COMPARE_OP:
            return COMPARE_OP_JUMP;
        case CONTAINS_OP:
            return CONTAINS_OP_JUMP;
        default:
            assert(op->opcode == IS_OP);
            return IS_OP_JUMP;
    }
}

/* Undo the fusion of the pair that op starts: the jump has a word and, in
 * place_entries(), an entry of its own again. */
static void
unfuse(_PyRegCfg *cfg, _PyRegOp *op)
{
    assert(op->flags & _PyReg_FUSED);
    op->flags &= ~_PyReg_FUSED;
    op->handler = op->opcode;
    cfg->ops[op->fuse].nwords = 1;
    op->fuse = 0;
}

/* The CALL_INTRINSIC_2 intrinsic that does what a COMPARE_OP or
 * CONTAINS_OP of oparg does, for a site that gets no entry. */
static int
fallback_intrinsic(int opcode, int oparg)
{
    if (opcode == CONTAINS_OP) {
        return INTRINSIC_CONTAINS_OP + (oparg & 1);
    }
    assert(opcode == COMPARE_OP);
    assert((oparg >> 5) <= Py_GE);
    return INTRINSIC_COMPARE_OP + (oparg >> 5) * 2 + ((oparg >> 4) & 1);
}

/* The sites inside a loop: the ops of the blocks of the natural loop of each
 * backward jump, which are its target and the blocks that reach the jump
 * without passing the target.  The flow graph has the edges of jumps and
 * fall-through, not of exceptions, so a handler laid out after the function
 * that jumps back into a loop does not put the code between them in it. */
static int
mark_loops(_PyRegCfg *cfg, char *in_loop)
{
    int n = cfg->nblocks;
    // Predecessors in compressed form: pred[start[b] .. start[b + 1] - 1]
    int *start = PyMem_Calloc(n + 2, sizeof(int));
    int *fill = PyMem_Calloc(n + 1, sizeof(int));
    int *seen = PyMem_Calloc(n + 1, sizeof(int));
    int *stack = PyMem_Calloc(n + 1, sizeof(int));
    int *pred = NULL;
    char *blk_in_loop = PyMem_Calloc(n + 1, 1);
    int res = ERROR;
    if (start == NULL || fill == NULL || seen == NULL || stack == NULL
        || blk_in_loop == NULL)
    {
        PyErr_NoMemory();
        goto done;
    }
    for (int pass = 0; pass < 2; pass++) {
        for (int b = 0; b < n; b++) {
            const _PyRegBlock *blk = &cfg->blocks[b];
            for (int i = blk->start; i <= blk->end; i++) {
                int succ = -1;
                if (i < blk->end) {
                    succ = cfg->ops[i].target;
                }
                else if (blk->falls_through && b + 1 < n) {
                    succ = b + 1;
                }
                if (succ < 0) {
                    continue;
                }
                if (pass == 0) {
                    start[succ + 1]++;
                }
                else {
                    pred[fill[succ]++] = b;
                }
            }
        }
        if (pass == 0) {
            for (int b = 0; b < n; b++) {
                start[b + 1] += start[b];
            }
            pred = PyMem_Malloc((start[n] + 1) * sizeof(int));
            if (pred == NULL) {
                PyErr_NoMemory();
                goto done;
            }
            memcpy(fill, start, n * sizeof(int));
        }
    }
    int stamp = 0;
    for (int b = 0; b < n; b++) {
        for (int i = cfg->blocks[b].start; i < cfg->blocks[b].end; i++) {
            int t = cfg->ops[i].target;
            if (t < 0 || t > b) {
                continue;
            }
            stamp++;
            int top = 0;
            seen[t] = stamp;
            blk_in_loop[t] = 1;
            if (seen[b] != stamp) {
                seen[b] = stamp;
                stack[top++] = b;
            }
            while (top > 0) {
                int x = stack[--top];
                blk_in_loop[x] = 1;
                for (int k = start[x]; k < start[x + 1]; k++) {
                    if (seen[pred[k]] != stamp) {
                        seen[pred[k]] = stamp;
                        stack[top++] = pred[k];
                    }
                }
            }
        }
    }
    for (int b = 0; b < n; b++) {
        for (int i = cfg->blocks[b].start; i < cfg->blocks[b].end; i++) {
            in_loop[i] = blk_in_loop[b];
        }
    }
    res = SUCCESS;
done:
    PyMem_Free(blk_in_loop);
    PyMem_Free(fill);
    PyMem_Free(start);
    PyMem_Free(seen);
    PyMem_Free(stack);
    PyMem_Free(pred);
    return res;
}

static int
next_frontend_op(_PyRegCfg *cfg, int first, int skip_resume)
{
    while (first < cfg->nops) {
        int opcode = cfg->ops[first].opcode;
        if (opcode != _PyReg_DEAD && opcode != NOP &&
            !(skip_resume && opcode == RESUME))
        {
            return first;
        }
        first++;
    }
    return -1;
}

static int
completion_error(_PyRegCfg *cfg, _PyRegOp *op, const char *detail)
{
    _PyRegSlots slots;
    _PyRegCfg_OpSlots(cfg, op, &slots);
    int handler_base = op->except < 0 ? cfg->nlocalsplus
        : cfg->nlocalsplus + cfg->blocks[op->except].startdepth - 1
          - cfg->blocks[op->except].preserve_lasti;
    int target = op->completion ? op->completion - 1
        : op->target >= 0 ? next_frontend_op(cfg, cfg->blocks[op->target].start, 0)
        : next_frontend_op(cfg, (int)(op - cfg->ops) + 1, 0);
    _PyRegOp *end = target >= 0 ? &cfg->ops[target] : NULL;
    const int *out = _PyReg_OUT(cfg, op);
    const int *in = end ? _PyReg_IN(cfg, end) : NULL;
    PyErr_Format(PyExc_SystemError,
        "invalid completion at line %d: %s "
        "(op=%d depth=%d nread=%d nout=%d base=%d dst=%d locals=%d "
        "handler=%d floor=%d target=%d target_op=%d target_nread=%d "
        "out=[%d,%d,%d] target_in=[%d,%d,%d])",
        op->loc.lineno, detail, op->opcode, op->depth, op->nread, op->nout,
        slots.base, slots.dst, cfg->nlocalsplus, op->except, handler_base,
        target, end ? end->opcode : -1, end ? end->nread : -1,
        op->nout > 0 ? out[0] : -1, op->nout > 1 ? out[1] : -1,
        op->nout > 2 ? out[2] : -1,
        end && end->nread > 0 ? in[0] : -1,
        end && end->nread > 1 ? in[1] : -1,
        end && end->nread > 2 ? in[2] : -1);
    return ERROR;
}

/* Allocation still sees the frontend's explicit ownership effects. Once its
 * physical homes are known, transfer each effect to its semantic producer.
 * Keep the frontend operations for allocation/verification; they get no word.
 * A shared END_SEND also has CLEANUP_THROW ingress, not just SEND ingress. */
static int
lower_completions(_PyRegCfg *cfg)
{
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->opcode == FOR_ITER || op->opcode == SEND) {
            int target = next_frontend_op(cfg, cfg->blocks[op->target].start, 0);
            int expected = op->opcode == FOR_ITER ? END_FOR : END_SEND;
            if (target < 0 || cfg->ops[target].opcode != expected || op->nout != 3) {
                return completion_error(cfg, op, "missing completion producer/target");
            }
            _PyRegOp *end = &cfg->ops[target];
            if (end->nread != 3 ||
                memcmp(_PyReg_OUT(cfg, op), _PyReg_IN(cfg, end), 3 * sizeof(int)))
            {
                return completion_error(cfg, op, "completion window differs from producer");
            }
            end->flags |= _PyReg_COMPLETED;
            op->completion = target + 1;
            if (op->opcode == FOR_ITER) {
                int pop = next_frontend_op(cfg, target + 1, 0);
                if (pop < 0 || cfg->ops[pop].opcode != POP_ITER ||
                    cfg->ops[pop].nread != 2 ||
                    memcmp(_PyReg_IN(cfg, &cfg->ops[pop]),
                           _PyReg_OUT(cfg, op), 2 * sizeof(int)))
                {
                    return completion_error(cfg, op, "missing iterator retirement");
                }
                cfg->ops[pop].flags |= _PyReg_COMPLETED;
            }
        }
        else if (op->opcode == CLEANUP_THROW) {
            int target = op->target < 0 ? -1
                : next_frontend_op(cfg, cfg->blocks[op->target].start, 0);
            if (target < 0 || cfg->ops[target].opcode != END_SEND ||
                op->nout != 3 || cfg->ops[target].nread != 3 ||
                memcmp(_PyReg_OUT(cfg, op), _PyReg_IN(cfg, &cfg->ops[target]),
                       3 * sizeof(int)))
            {
                return completion_error(cfg, op, "throw completion window differs");
            }
            op->completion = target + 1;
            cfg->ops[target].flags |= _PyReg_COMPLETED;
        }
        else if (op->opcode == RETURN_GENERATOR ||
                 (op->opcode == YIELD_VALUE && op->oparg == 0))
        {
            if (op->flags & _PyReg_DISCARD) {
                /* The ordinary fresh-result sink pass already proved the
                 * sole resume-input consumer and preserved its source span. */
                continue;
            }
            int pop = next_frontend_op(cfg, i + 1, 1);
            if (pop >= 0 && cfg->ops[pop].opcode == POP_TOP && op->nout == 1 &&
                cfg->ops[pop].nread == 1 &&
                _PyReg_IN(cfg, &cfg->ops[pop])[0] == _PyReg_OUT(cfg, op)[0])
            {
                op->flags |= _PyReg_DISCARD;
                cfg->ops[pop].flags |= _PyReg_COMPLETED | _PyReg_ELIDED_NOP;
            }
            else if (op->opcode == RETURN_GENERATOR) {
                return completion_error(cfg, op, "initial resume input is consumed");
            }
        }
    }
    /* Every entry into a removed completion block must have completed the
     * same operation already. Other iterator pops (e.g. any/all early exit)
     * remain explicit until their consuming branch contract is lowered. */
    for (int b = 0; b < cfg->nblocks; b++) {
        int first = next_frontend_op(cfg, cfg->blocks[b].start, 0);
        if (first < 0 || first >= cfg->blocks[b].end ||
            !(cfg->ops[first].flags & _PyReg_COMPLETED) ||
            (cfg->ops[first].opcode != END_FOR && cfg->ops[first].opcode != END_SEND))
        {
            continue;
        }
        for (int p = 0; p < cfg->nblocks; p++) {
            _PyRegBlock *pred = &cfg->blocks[p];
            for (int j = pred->start; j < pred->end; j++) {
                _PyRegOp *incoming = &cfg->ops[j];
                if (incoming->target == b && incoming->opcode != END_ASYNC_FOR &&
                    incoming->completion != first + 1)
                {
                    return completion_error(cfg, incoming, "uncompleted edge into completion");
                }
            }
            if (p + 1 == b && pred->falls_through && pred->reachable) {
                int last = pred->end - 1;
                while (last >= pred->start &&
                       (cfg->ops[last].opcode == NOP || cfg->ops[last].opcode == _PyReg_DEAD))
                {
                    last--;
                }
                if (last < pred->start || cfg->ops[last].completion != first + 1) {
                    return completion_error(cfg, &cfg->ops[first], "uncompleted fallthrough");
                }
            }
        }
    }
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (!op->completion) {
            continue;
        }
        _PyRegSlots slots;
        _PyRegCfg_OpSlots(cfg, op, &slots);
        int count = op->opcode == FOR_ITER ? 2 : op->opcode == SEND ? 3 : 4;
        int limit = cfg->nlocalsplus + cfg->ntemps;
        if (slots.base < cfg->nlocalsplus || slots.base > limit - count) {
            return completion_error(cfg, op, "physical completion window is outside frame");
        }
        int handler_base = cfg->nlocalsplus;
        if (op->except >= 0) {
            _PyRegBlock *handler = &cfg->blocks[op->except];
            handler_base += handler->startdepth - 1 - handler->preserve_lasti;
        }
        /* Final code construction proves the actual handler cannot read a
         * retired home before redefining it. A preserved exception prefix may
         * contain dead implicit unwind anchors, as in generator wrappers. */
        int dst = slots.dst;
        if (op->opcode != FOR_ITER) {
            _PyRegSlots completed;
            _PyRegCfg_OpSlots(cfg, &cfg->ops[op->completion - 1], &completed);
            dst = completed.dst;
        }
        if (dst != IR_NO_DST &&
            (dst < 0 || dst >= limit ||
             (dst >= cfg->nlocalsplus && dst < handler_base &&
              (dst < slots.base || dst >= slots.base + count)) ||
             (op->opcode == FOR_ITER && dst >= slots.base && dst < slots.base + 2)))
        {
            return completion_error(cfg, op, "completion destination aliases a live owner");
        }
    }
    return SUCCESS;
}

/* Choose the handlers.  A compare marked by _PyRegPass_FuseJump starts
 * fused with its jump, which then has no word of its own; place_entries()
 * unfuses the pairs whose compare gets no entry, and layout() those whose
 * target is too far. */
static int
choose_handlers(_PyRegCfg *cfg, PyObject *consts)
{
    RETURN_IF_ERROR(lower_completions(cfg));
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        op->fb = -1;
        op->flags &= ~_PyReg_FUSED;
        if (op->opcode == _PyReg_DEAD) {
            op->nwords = 0;
            continue;
        }
        assert(!IS_PSEUDO_INSTR(op->opcode));
        assert(IS_VALID_OPCODE(op->opcode));
        if (op->opcode == NOP || (op->flags & (_PyReg_ELIDED | _PyReg_COMPLETED))) {
            op->handler = NOP;
            op->nwords = 0;
            continue;
        }
        op->handler = choose_handler(op, consts);
        if (op->handler < 0) {
            return ERROR;
        }
        op->nwords = 1;
    }
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->opcode != _PyReg_DEAD && op->fuse) {
            op->flags |= _PyReg_FUSED;
            op->handler = fused_handler(op);
            cfg->ops[op->fuse].nwords = 0;
        }
    }
    return SUCCESS;
}

/* Place the feedback entries, in the units of pycore_ir.h: entry 0 is the
 * header, the others follow in the order of FB_CLASS_*, each in source
 * order, as long as they end by IR_FB_UNITS_MAX.  Sites past that share
 * entry 0, which never specialises and keeps no operand; a COMPARE_OP or
 * CONTAINS_OP among them becomes the intrinsic call, and a fused pair among
 * them is two words again, so a pair is fused only if its compare keeps an
 * entry.  a_fbinit gets one word per entry, in unit order.
 *
 * It runs again after layout() unfused pairs by distance, which add entries
 * of classes after the required one and so change none of the others. */
static int
place_entries(struct assembler *a, _PyRegCfg *cfg, int code_flags,
              int after_layout)
{
    char *cls = PyMem_Calloc(cfg->nops + 1, 2);
    if (cls == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    char *in_loop = cls + cfg->nops + 1;
    int res = ERROR;
    Py_CLEAR(a->a_fbinit);
    // A module or class body runs once, and ranks no loops
    if (code_flags & CO_OPTIMIZED) {
        if (mark_loops(cfg, in_loop) < 0) {
            goto done;
        }
    }
    int nsites = 0;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        op->fb = -1;
        cls[i] = -1;
        if (op->nwords == 0) {
            continue;
        }
        const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
        if (fmt->fb == _PyIR_LOC_NONE) {
            continue;
        }
        // Only entry 0, which the sites without an entry share, is a counter
        assert(fmt->fb_size != 0 && fmt->fb_kind != FB_COUNTER);
        cls[i] = needs_entry(op->handler) ? FB_CLASS_REQUIRED
                 : in_loop[i] ? FB_CLASS_LOOP : FB_CLASS_OTHER;
        nsites++;
    }
    // Room for the entries of the jumps that a fused pair may get back
    a->a_fbinit = PyBytes_FromStringAndSize(NULL, (nsites + 1 + cfg->nops) * sizeof(uint32_t));
    if (a->a_fbinit == NULL) {
        goto done;
    }
    uint32_t *fbinit = (uint32_t *)PyBytes_AS_STRING(a->a_fbinit);
    uint32_t unit = IR_FB_FIRST_UNIT;
    int nfb = 1;
    for (int c = 0; c < FB_NCLASSES; c++) {
        for (int i = 0; i < cfg->nops; i++) {
            if (cls[i] != c) {
                continue;
            }
            _PyRegOp *op = &cfg->ops[i];
            const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
            uint32_t size = fmt->fb_size >> IR_FB_UNIT_SHIFT;
            if (unit + size <= IR_FB_UNITS_MAX) {
                int arg = 0;
                if (op->flags & _PyReg_FUSED) {
                    arg = fused_arg(cfg, op);
                }
                else if (fmt->imm == _PyIR_LOC_FBARG) {
                    arg = handler_oparg(cfg, op);
                    if (arg < 0 || arg > 0xFFFF) {
                        too_large("oparg", arg);
                        goto done;
                    }
                }
                fbinit[nfb++] = _PyIR_FBINIT(fmt->fb_kind, unit, arg);
                op->fb = (int)unit;
                unit += size;
                continue;
            }
            if (c != FB_CLASS_REQUIRED) {
                op->fb = 0;
                continue;
            }
            if (op->flags & _PyReg_FUSED) {
                assert(!after_layout);
                int jump = op->fuse;
                unfuse(cfg, op);
                cls[jump] = in_loop[jump] ? FB_CLASS_LOOP : FB_CLASS_OTHER;
                if (op->opcode == IS_OP) {
                    continue;
                }
            }
            op->oparg = fallback_intrinsic(op->opcode, op->oparg);
            op->opcode = CALL_INTRINSIC_2;
            op->handler = CALL_INTRINSIC_2;
        }
    }
    // Entry 0 exists only with others
    if (nfb > 1) {
        fbinit[0] = _PyIR_FBINIT(FB_COUNTER, 0, unit);
    }
    a->a_nfb = nfb > 1 ? nfb : 0;
    if (_PyBytes_Resize(&a->a_fbinit, a->a_nfb * sizeof(uint32_t)) < 0) {
        goto done;
    }
    res = SUCCESS;
done:
    PyMem_Free(cls);
    return res;
}

/* Place the words, relaxing each FOR_ITER and SEND whose exit is too far,
 * until none is: relaxing only moves exits further away. */
static int
layout(struct assembler *a, _PyRegCfg *cfg)
{
    for (;;) {
        int word = 0;
        for (int b = 0; b < cfg->nblocks; b++) {
            _PyRegBlock *blk = &cfg->blocks[b];
            blk->word = word;
            for (int i = blk->start; i < blk->end; i++) {
                cfg->ops[i].word = word;
                word += cfg->ops[i].nwords;
            }
        }
        a->a_nwords = word;
        if (word > IR_CD_MAX) {
            // Jump targets would not fit
            return too_large("word count", word);
        }
        int changed = 0;
        for (int i = 0; i < cfg->nops; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->flags & _PyReg_FUSED) {
                // Forward only, within the range of a 12-bit distance
                int distance = jump_target(cfg, &cfg->ops[op->fuse])
                               - (op->word + 1);
                if (distance < 0 || distance > _PyIR_MaxForward) {
                    unfuse(cfg, op);
                    changed = 1;
                }
                continue;
            }
            if (word <= _PyIR_MaxForward) {
                continue;
            }
            if ((op->opcode != FOR_ITER && op->opcode != SEND) || op->nwords > 1) {
                continue;
            }
            int distance = jump_target(cfg, op) - (op->word + 1);
            if (distance > _PyIR_MaxForward) {
                op->nwords = op->opcode == FOR_ITER
                    ? RELAXED_FOR_ITER_WORDS : RELAXED_SEND_WORDS;
                changed = 1;
            }
        }
        if (!changed) {
            return SUCCESS;
        }
    }
}

/* The word of op.  Unreachable code never runs, and fields that do not fit
 * there become 0. */
static int
encode(_PyRegCfg *cfg, _PyRegOp *op, int reachable, _PyIRWord *word)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
    _PyRegSlots slots;
    _PyRegCfg_OpSlots(cfg, op, &slots);
    if ((op->opcode == SEND || op->opcode == CLEANUP_THROW) && op->completion) {
        _PyRegSlots completed;
        _PyRegCfg_OpSlots(cfg, &cfg->ops[op->completion - 1], &completed);
        slots.dst = completed.dst;
    }
    int oparg = handler_oparg(cfg, op);
    // Every role's value, then a pick per field.  DST is meaningless when
    // fmt->dst is _PyIR_NO_DST.
    int values[_PyIR_ROLE_IN0 + 4] = {
        [_PyIR_ROLE_NONE] = 0,
        [_PyIR_ROLE_DST] = slots.dst,
        [_PyIR_ROLE_BASE] = slots.base,
        [_PyIR_ROLE_FB] = op->fb > 0 ? op->fb : 0,
        [_PyIR_ROLE_IMM] = oparg,
        [_PyIR_ROLE_LOCAL_IN] = oparg,
        [_PyIR_ROLE_LOCAL_OUT] = oparg,
        [_PyIR_ROLE_LOCAL_CLEAR] = oparg,
        [_PyIR_ROLE_LOCAL_MOVE] = oparg,
        [_PyIR_ROLE_CELL] = oparg,
        [_PyIR_ROLE_IN0] = slots.in[0],
        [_PyIR_ROLE_IN0 + 1] = slots.in[1],
        [_PyIR_ROLE_IN0 + 2] = slots.in[2],
        [_PyIR_ROLE_IN0 + 3] = slots.in[3],
    };
    uint32_t fields[4];
    for (int r = 0; r < 4; r++) {
        int role = fmt->roles[r];
        assert(role < _PyIR_ROLE_IN0 + 4);
        int value = values[role];
        if (!_PyIR_RoleIsImm(role) && (value < 0 || value > IR_FIELD_MAX)) {
            // Only unreachable code names slots past the frame, since the
            // slot count is checked first.
            assert(!reachable);
            value = 0;
        }
        fields[r] = (uint32_t)value;
    }
    if (fmt->imm == _PyIR_LOC_CD) {
        if (oparg < 0 || oparg > IR_CD_MAX) {
            if (reachable) {
                return too_large("oparg", oparg);
            }
            oparg = 0;
        }
        fields[2] = oparg & IR_FIELD_MAX;
        fields[3] = oparg >> 12;
    }
    else if (fmt->imm >= _PyIR_LOC_A && fmt->imm <= _PyIR_LOC_D) {
        int r = fmt->imm - _PyIR_LOC_A;
        if (fmt->flags & _PyIR_FORWARD) {
            // A signed distance
            if (oparg < IR_MIN_DISTANCE || oparg > IR_MAX_FORWARD) {
                if (reachable) {
                    return too_large("distance", oparg);
                }
                oparg = 0;
            }
            fields[r] = (uint32_t)oparg & IR_FIELD_MAX;
        }
        else if (oparg < 0 || oparg > IR_FIELD_MAX) {
            if (reachable) {
                return too_large("oparg", oparg);
            }
            fields[r] = 0;
        }
    }
    *word = IR_WORD(op->handler, fields[0], fields[1], fields[2], fields[3]);
    return SUCCESS;
}

static inline _PyIRWord
jump_word(int target)
{
    return IR_WORD(IR_JUMP, 0, 0, target & IR_FIELD_MAX, target >> 12);
}

/* The words and the feedback table, in a->a_bytecode and a->a_fbinit */
static int
assemble_words(struct assembler *a, _PyRegCfg *cfg, PyObject *consts,
               int nlocalsplus, int stacksize, int code_flags)
{
    if (nlocalsplus + stacksize > IR_MAX_SLOTS) {
        PyErr_Format(PyExc_SyntaxError,
                     "too many local variables and temporaries in function "
                     "(needs %d slots, limit %d)",
                     nlocalsplus + stacksize, IR_MAX_SLOTS);
        return ERROR;
    }
    RETURN_IF_ERROR(choose_handlers(cfg, consts));
    RETURN_IF_ERROR(place_entries(a, cfg, code_flags, 0));
    int nfused = 0;
    for (int i = 0; i < cfg->nops; i++) {
        nfused += (cfg->ops[i].flags & _PyReg_FUSED) != 0;
    }
    RETURN_IF_ERROR(layout(a, cfg));
    for (int i = 0; i < cfg->nops; i++) {
        nfused -= (cfg->ops[i].flags & _PyReg_FUSED) != 0;
    }
    if (nfused != 0) {
        // Pairs unfused by distance need entries for their jumps
        RETURN_IF_ERROR(place_entries(a, cfg, code_flags, 1));
    }

    a->a_bytecode = PyBytes_FromStringAndSize(NULL,
                                              a->a_nwords * sizeof(_PyIRWord));
    if (a->a_bytecode == NULL) {
        return ERROR;
    }
    _PyIRWord *words = (_PyIRWord *)PyBytes_AS_STRING(a->a_bytecode);
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            int w = op->word;
            if (op->nwords == 0) {
                continue;
            }
            RETURN_IF_ERROR(encode(cfg, op, blk->reachable, &words[w]));
            if (op->nwords > 1) {
                int exit = jump_target(cfg, op);
                if (op->opcode == FOR_ITER) {
                    words[w + 1] = jump_word(w + RELAXED_FOR_ITER_WORDS);
                    words[w + 2] = jump_word(exit);
                }
                else {
                    assert(op->opcode == SEND);
                    words[w + 1] = jump_word(w + RELAXED_SEND_WORDS);
                    words[w + 2] = jump_word(exit);
                }
            }
        }
    }
    return SUCCESS;
}

/* The constant-slot table: (slot, co_consts index) pairs */
static int
make_constslots(struct assembler *a, _PyRegCfg *cfg)
{
    a->a_constslots = PyBytes_FromStringAndSize(
        NULL, cfg->nconst * 2 * sizeof(uint32_t));
    if (a->a_constslots == NULL) {
        return ERROR;
    }
    uint32_t *table = (uint32_t *)PyBytes_AS_STRING(a->a_constslots);
    for (int j = 0; j < cfg->nconst; j++) {
        table[2 * j] = (uint32_t)(cfg->const_first + j);
        table[2 * j + 1] = (uint32_t)cfg->const_index[j];
    }
    return SUCCESS;
}

PyCodeObject *
_PyAssemble_MakeCodeObject(_PyCompile_CodeUnitMetadata *umd, PyObject *const_cache,
                           PyObject *consts, _PyRegCfg *cfg,
                           int nlocalsplus, int code_flags, PyObject *filename)
{
    PyCodeObject *co = NULL;
    // co_stacksize keeps its meaning for tools: the slots for temporaries,
    // which compaction may make fewer than the deepest stack
    struct assembler a;
    memset(&a, 0, sizeof(a));
    code_flags &= ~CO_VIRTUAL_KWARGS;
    if (cfg->virtual_kwargs) {
        code_flags |= CO_VIRTUAL_KWARGS;
    }
    int res = _PyRegCfg_AssignConstSlots(
        cfg, consts, (int)PyDict_GET_SIZE(umd->u_freevars));
    // The hidden constant slots are part of the frame
    nlocalsplus = cfg->nlocalsplus;
    int stacksize = cfg->ntemps;
    if (res == SUCCESS) {
        res = make_constslots(&a, cfg);
    }
    if (res == SUCCESS) {
        res = assemble_words(&a, cfg, consts, nlocalsplus, stacksize,
                             code_flags);
    }
    if (res == SUCCESS) {
        res = assemble_emit(&a, cfg, umd->u_firstlineno, const_cache);
    }
    if (res == SUCCESS) {
        co = makecode(umd, &a, const_cache, consts, stacksize, nlocalsplus,
                      cfg->nconst, code_flags, filename);
    }
    assemble_free(&a);
    return co;
}
