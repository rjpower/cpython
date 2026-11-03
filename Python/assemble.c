#include "Python.h"
#include "pycore_code.h"            // write_location_entry_start()
#include "pycore_compile.h"
#include "pycore_flowgraph.h"
#include "pycore_ir.h"                // IR_WORD()
#include "pycore_long.h"              // _PyLong_CheckExactAndCompact()
#include "pycore_opcode_utils.h"    // IS_BACKWARDS_JUMP_OPCODE
#include "pycore_opcode_metadata.h" // _PyOpcode_num_popped
#include "pycore_regcfg.h"          // _PyRegCfg
#include "pycore_symtable.h"        // _Py_SourceLocation

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
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_linetable));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_bytecode));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_fbinit));
    return SUCCESS;
}

static PyObject *
dict_keys_inorder(PyObject *dict, Py_ssize_t offset)
{
    PyObject *tuple, *k, *v;
    Py_ssize_t pos = 0, size = PyDict_GET_SIZE(dict);

    tuple = PyTuple_New(size);
    if (tuple == NULL)
        return NULL;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        Py_ssize_t i = PyLong_AsSsize_t(v);
        if (i == -1 && PyErr_Occurred()) {
            Py_DECREF(tuple);
            return NULL;
        }
        assert((i - offset) < size);
        assert((i - offset) >= 0);
        PyTuple_SET_ITEM(tuple, i - offset, Py_NewRef(k));
    }
    return tuple;
}

// This is in codeobject.c.
extern void _Py_set_localsplus_info(int, PyObject *, unsigned char,
                                   PyObject *, PyObject *);

static int
compute_localsplus_info(_PyCompile_CodeUnitMetadata *umd, int nlocalsplus,
                        int flags, PyObject *names, PyObject *kinds)
{
    PyObject *k, *v;
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
        while (pos < max && PyDict_Next(umd->u_varnames, &pos, &k, &v)) {
            int offset = PyLong_AsInt(v);
            if (offset == -1 && PyErr_Occurred()) {
                return ERROR;
            }
            assert(offset >= 0);
            assert(offset < nlocalsplus);

            _PyLocals_Kind kind = CO_FAST_LOCAL | argvarkinds[i].kind;

            if (umd->u_fasthidden != NULL) {
                int hidden = PySet_Contains(umd->u_fasthidden, k);
                RETURN_IF_ERROR(hidden);
                if (hidden) {
                    kind |= CO_FAST_HIDDEN;
                }
            }

            int has_key = PyDict_Contains(umd->u_cellvars, k);
            RETURN_IF_ERROR(has_key);
            if (has_key) {
                kind |= CO_FAST_CELL;
            }

            _Py_set_localsplus_info(offset, k, kind, names, kinds);
        }
    }
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);

    // This counter mirrors the fix done in fix_cell_offsets().
    int numdropped = 0, cellvar_offset = -1;
    pos = 0;
    while (PyDict_Next(umd->u_cellvars, &pos, &k, &v)) {
        int has_name = PyDict_Contains(umd->u_varnames, k);
        RETURN_IF_ERROR(has_name);
        if (has_name) {
            // Skip cells that are already covered by locals.
            numdropped += 1;
            continue;
        }

        cellvar_offset = PyLong_AsInt(v);
        if (cellvar_offset == -1 && PyErr_Occurred()) {
            return ERROR;
        }
        assert(cellvar_offset >= 0);
        cellvar_offset += nlocals - numdropped;
        assert(cellvar_offset < nlocalsplus);
        _Py_set_localsplus_info(cellvar_offset, k, CO_FAST_CELL, names, kinds);
    }

    pos = 0;
    while (PyDict_Next(umd->u_freevars, &pos, &k, &v)) {
        int offset = PyLong_AsInt(v);
        if (offset == -1 && PyErr_Occurred()) {
            return ERROR;
        }
        assert(offset >= 0);
        offset += nlocals - numdropped;
        assert(offset < nlocalsplus);
        /* XXX If the assertion below fails it is most likely because a freevar
           was added to u_freevars with the wrong index due to not taking into
           account cellvars already present, see gh-128632. */
        assert(offset > cellvar_offset);
        _Py_set_localsplus_info(offset, k, CO_FAST_FREE, names, kinds);
    }
    return SUCCESS;
}

static PyCodeObject *
makecode(_PyCompile_CodeUnitMetadata *umd, struct assembler *a, PyObject *const_cache,
         PyObject *constslist, int maxdepth, int nlocalsplus, int code_flags,
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
            umd, nlocalsplus, code_flags,
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

   if (_PyCode_Validate(&con) < 0) {
        goto error;
    }

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

static const uint8_t fb_kind[256] = {
#define FB_KIND(OP, TYPE, KIND) [OP] = KIND,
    _PyFb_FAMILIES(FB_KIND)
    _PyFb_BRANCHES(FB_KIND)
#undef FB_KIND
};

#ifndef NDEBUG
static const uint8_t has_fb_entry[256] = {
#define FB_HAS(OP, TYPE, KIND) [OP] = 1,
    _PyFb_FAMILIES(FB_HAS)
    _PyFb_BRANCHES(FB_HAS)
#undef FB_HAS
};
#endif

/* The farthest exit of FOR_ITER and SEND that the assembler does not relax */
int _PyIR_MaxForward = IR_MAX_FORWARD;

/* The words of a relaxed FOR_ITER and of a relaxed SEND, its own included.
 * A relaxed FOR_ITER at word w exits by distance 1, so that
 *
 *     w + 1   IR_JUMP to w + 4, the body
 *     w + 2   IR_JUMP to END_FOR, where a generator returns
 *     w + 3   IR_JUMP past END_FOR, where an exhausted iterator goes
 *
 * and a relaxed SEND at w has w + 1, to w + 3, and w + 2, to END_SEND. */
#define RELAXED_FOR_ITER_WORDS 4
#define RELAXED_SEND_WORDS 3

static int
too_large(const char *what, int value)
{
    PyErr_Format(PyExc_SystemError,
                 "%s %d does not fit in the register IR", what, value);
    return ERROR;
}

/* The generic handler of op: its opcode, a form that pushes its constant
 * inline, or for BINARY_OP the handler of its operation. */
static int
choose_handler(_PyRegOp *op, PyObject *consts)
{
    int opcode = op->opcode;
    int oparg = op->oparg;
    if (opcode == BINARY_OP) {
        if (oparg > NB_OPARG_LAST) {
            return too_large("BINARY_OP operation", oparg);
        }
        return _PyIR_BINARY_OP_FIRST + oparg;
    }
    // The executing copy runs the boxing forms instead in a build without
    // inline values or under PYTHON_TAGGED_VALUES=0 (Python/ir.c), so the
    // words do not depend on the build.
    if (opcode == LOAD_SMALL_INT) {
        return LOAD_SMALL_INT_INLINE;
    }
    if (opcode == LOAD_CONST) {
        PyObject *value = PyList_GET_ITEM(consts, oparg);
        if (_PyLong_CheckExactAndCompact(value)) {
            return LOAD_CONST_INLINE_INT;
        }
        if (PyFloat_CheckExact(value) && !isnan(PyFloat_AS_DOUBLE(value))) {
            return LOAD_CONST_INLINE_FLOAT;
        }
    }
    return opcode;
}

/* The word that op jumps to.  END_ASYNC_FOR names the SEND of its loop,
 * which starts its block; its target is the END_SEND where that SEND
 * exits, which sys.monitoring needs. */
static int
jump_target(_PyRegCfg *cfg, _PyRegOp *op)
{
    assert(op->target >= 0);
    _PyRegBlock *b = &cfg->blocks[op->target];
    if (op->opcode == END_ASYNC_FOR) {
        _PyRegOp *send = &cfg->ops[b->start];
        assert(send->opcode == SEND);
        return cfg->blocks[send->target].word;
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
        return jump_target(cfg, op) - (op->word + 1);
    }
    return op->oparg;
}

/* Choose the handlers and place the feedback entries.  Sites past the
 * IR_FIELD_MAX-th share entry 0, which never specialises; those whose
 * handler would read its static operand from its entry take the NOFB form,
 * which carries it in the word. */
static int
assign_handlers(struct assembler *a, _PyRegCfg *cfg, PyObject *consts)
{
    a->a_nfb = 1;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        op->fb = -1;
        if (op->opcode == _PyReg_DEAD) {
            op->nwords = 0;
            continue;
        }
        assert(!IS_PSEUDO_INSTR(op->opcode));
        assert(IS_VALID_OPCODE(op->opcode));
        op->handler = choose_handler(op, consts);
        if (op->handler < 0) {
            return ERROR;
        }
        op->nwords = 1;
        const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
        if (fmt->fb == _PyIR_LOC_NONE) {
            continue;
        }
        assert(has_fb_entry[op->opcode]);
        assert(fb_kind[op->opcode] != FB_COUNTER);
        if (a->a_nfb <= IR_FIELD_MAX) {
            op->fb = a->a_nfb++;
            continue;
        }
        op->fb = 0;
        if (fmt->imm == _PyIR_LOC_FBARG && fmt->nofb != 0) {
            op->handler = fmt->nofb;
            op->fb = -1;
        }
    }
    return SUCCESS;
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
        if (word <= _PyIR_MaxForward) {
            return SUCCESS;
        }
        int changed = 0;
        for (int i = 0; i < cfg->nops; i++) {
            _PyRegOp *op = &cfg->ops[i];
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
    int oparg = handler_oparg(cfg, op);
    // Every role's value, then a pick per field.  DST is meaningless when
    // fmt->dst is _PyIR_NO_DST.
    int values[_PyIR_ROLE_IN0 + 4] = {
        [_PyIR_ROLE_NONE] = 0,
        [_PyIR_ROLE_DST] = slots.dst,
        [_PyIR_ROLE_BASE] = slots.base,
        [_PyIR_ROLE_FB] = op->fb > 0 ? op->fb : 0,
        [_PyIR_ROLE_IMM] = oparg,
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
        if (role != _PyIR_ROLE_IMM && (value < 0 || value > IR_FIELD_MAX)) {
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
        if (oparg < 0 || oparg > IR_FIELD_MAX) {
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
               int nlocalsplus, int stacksize)
{
    if (nlocalsplus + stacksize > IR_MAX_SLOTS) {
        PyErr_Format(PyExc_SystemError,
                     "the code needs %d slots for its locals, cells, free "
                     "variables and stack, and the register IR has at most %d",
                     nlocalsplus + stacksize, IR_MAX_SLOTS);
        return ERROR;
    }
    RETURN_IF_ERROR(assign_handlers(a, cfg, consts));
    RETURN_IF_ERROR(layout(a, cfg));

    a->a_bytecode = PyBytes_FromStringAndSize(NULL,
                                              a->a_nwords * sizeof(_PyIRWord));
    // Entry 0 exists only with others
    int nfb = a->a_nfb > 1 ? a->a_nfb : 0;
    a->a_fbinit = PyBytes_FromStringAndSize(NULL, nfb * sizeof(uint32_t));
    if (a->a_bytecode == NULL || a->a_fbinit == NULL) {
        return ERROR;
    }
    _PyIRWord *words = (_PyIRWord *)PyBytes_AS_STRING(a->a_bytecode);
    uint32_t *fbinit = (uint32_t *)PyBytes_AS_STRING(a->a_fbinit);
    if (nfb) {
        fbinit[0] = _PyIR_FBINIT(FB_COUNTER, 0);
    }
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            int w = op->word;
            if (op->nwords == 0) {
                continue;
            }
            RETURN_IF_ERROR(encode(cfg, op, blk->reachable, &words[w]));
            if (op->fb > 0) {
                int kind = fb_kind[op->opcode];
                int arg = 0;
                if (kind != FB_BRANCH) {
                    arg = handler_oparg(cfg, op);
                    if (arg < 0 || arg > IR_CD_MAX) {
                        return too_large("oparg", arg);
                    }
                }
                fbinit[op->fb] = _PyIR_FBINIT(kind, arg);
            }
            if (op->nwords > 1) {
                int exit = jump_target(cfg, op);
                if (op->opcode == FOR_ITER) {
                    words[w + 1] = jump_word(w + RELAXED_FOR_ITER_WORDS);
                    words[w + 2] = jump_word(exit);
                    words[w + 3] = jump_word(exit + 1);
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

PyCodeObject *
_PyAssemble_MakeCodeObject(_PyCompile_CodeUnitMetadata *umd, PyObject *const_cache,
                           PyObject *consts, _PyRegCfg *cfg,
                           int nlocalsplus, int code_flags, PyObject *filename)
{
    PyCodeObject *co = NULL;
    // co_stacksize keeps its meaning for tools: the slots for temporaries,
    // which compaction may make fewer than the deepest stack
    int stacksize = cfg->ntemps;
    struct assembler a;
    memset(&a, 0, sizeof(a));
    int res = assemble_words(&a, cfg, consts, nlocalsplus, stacksize);
    if (res == SUCCESS) {
        res = assemble_emit(&a, cfg, umd->u_firstlineno, const_cache);
    }
    if (res == SUCCESS) {
        co = makecode(umd, &a, const_cache, consts, stacksize, nlocalsplus,
                      code_flags, filename);
    }
    assemble_free(&a);
    return co;
}
