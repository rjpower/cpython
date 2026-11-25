#include "Python.h"
#include "pycore_code.h"            // PyCodeObject
#include "pycore_flowgraph.h"
#include "pycore_jit.h"             // _PyJIT_OFF
#include "pycore_ir.h"              // _PyCodeIR, IR_WORD()
#include "pycore_ir_source.h"
#include "pycore_opcode_utils.h"    // IS_BACKWARDS_JUMP_OPCODE
#include "pycore_opcode_metadata.h" // _PyOpcode_num_popped
#include "pycore_regcfg.h"          // _PyRegCfg
#include "pycore_rop_metadata.h"    // ROP_HANDLER_Call_Generic, ROP_FIRST_HANDLER
#include "pycore_rop_rows.h"        // BRANCH_IF_FALSE
#include "pycore_symtable.h"        // _Py_SourceLocation
#include "pycore_tuple.h"           // PyTuple_GET_VALUE()
#include "pycore_value.h"           // Canonical constant representation

#include <stdbool.h>

#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) < 0) {          \
        return ERROR;       \
    }

/* The IR assembler lowers the ops of the register CFG (Python/regcfg.c),
 * whose slots compaction has assigned, to the _PyCodeIR of a code object.
 * It sets each op's handler, the generic one before relaxation; its feedback
 * index, or -1; the index of its word, or of the next for an op without one;
 * and its number of words: 1, 0 for an op that a pass deleted, or with the
 * words that a relaxed loop adds more. */

#define ALIGN8(N) (((N) + 7) & ~(size_t)7)

/* A growing byte array of scratch memory */
typedef struct {
    uint8_t *data;
    size_t size;
    size_t alloc;
} buffer;

struct assembler {
    int a_nwords;
    int a_nfb;                 /* feedback entries, entry 0 included */
    uint32_t *a_fbinit;        /* the feedback table, a_nfb entries */
    _PyIRWord *a_words;
    buffer a_exceptions;       /* the exception table, in word units */
    buffer a_records;          /* recovery events of the source-only spans */
    buffer a_positions;        /* the position of each */
    buffer a_recipes;          /* the ingress recipes, int32_t entries */
    uint32_t *a_pcmap;
};

static int
buffer_reserve(buffer *b, size_t extra)
{
    if (b->size + extra <= b->alloc) {
        return SUCCESS;
    }
    size_t alloc = Py_MAX(b->alloc * 2, b->size + extra);
    uint8_t *data = PyMem_Realloc(b->data, alloc);
    if (data == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    b->data = data;
    b->alloc = alloc;
    return SUCCESS;
}

static int
buffer_write(buffer *b, const void *bytes, size_t n)
{
    RETURN_IF_ERROR(buffer_reserve(b, n));
    memcpy(b->data + b->size, bytes, n);
    b->size += n;
    return SUCCESS;
}

static void
assemble_free(struct assembler *a)
{
    PyMem_Free(a->a_fbinit);
    PyMem_Free(a->a_words);
    PyMem_Free(a->a_exceptions.data);
    PyMem_Free(a->a_records.data);
    PyMem_Free(a->a_positions.data);
    PyMem_Free(a->a_recipes.data);
    PyMem_Free(a->a_pcmap);
}


/* The caller has reserved room for the entry */
static inline void
write_except_byte(struct assembler *a, int byte) {
    a->a_exceptions.data[a->a_exceptions.size++] = byte;
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
#define MAX_SIZE_OF_ENTRY 25

/* The ingress recipe of op's exceptional edge, as an index + 1 in the
 * recipe table, or 0: the pairs of a source slot and a parameter's home that
 * compaction did not already make one.  Equal recipes share an entry. */
static int
assemble_recipe(struct assembler *a, _PyRegCfg *cfg, const _PyRegOp *op,
                int *index)
{
    *index = 0;
    const int *src = &cfg->operands[op->in + op->nread + op->nimplicit
                                    - op->nrecipe];
    if (op->nrecipe == 0) {
        return SUCCESS;
    }
    int32_t *entry = PyMem_Malloc((1 + 2 * (size_t)op->nrecipe) * sizeof(int32_t));
    if (entry == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    int n = 0;
    for (int q = 0; q < op->nrecipe; q++) {
        int from, home;
        if (op->flags & _PyReg_SLOT_RECIPE) {
            // A word of resolution names slots
            from = cfg->operands[op->recipe + 2 * q];
            home = cfg->operands[op->recipe + 2 * q + 1];
        }
        else {
            from = _PyRegCfg_Slot(cfg, src[q]);
            home = _PyRegCfg_Slot(
                cfg, _PyReg_CANON(cfg, cfg->operands[op->recipe + q]));
        }
        if (from != home) {
            entry[1 + 2 * n] = from;
            entry[2 + 2 * n] = home;
            n++;
        }
    }
    int res = SUCCESS;
    if (n == 0) {
        goto done;
    }
    entry[0] = n;
    size_t size = (size_t)(1 + 2 * n) * sizeof(int32_t);
    const int32_t *table = (const int32_t *)a->a_recipes.data;
    size_t count = a->a_recipes.size / sizeof(int32_t);
    for (size_t at = 0; at < count; at += 1 + 2 * (size_t)table[at]) {
        if (table[at] == n && memcmp(&table[at], entry, size) == 0) {
            *index = (int)at + 1;
            goto done;
        }
    }
    res = buffer_write(&a->a_recipes, entry, size);
    *index = res == SUCCESS ? (int)count + 1 : 0;
done:
    PyMem_Free(entry);
    return res;
}

static int
assemble_emit_exception_table_entry(struct assembler *a, int start, int end,
                                    _PyRegCfg *cfg, int h, int recipe)
{
    RETURN_IF_ERROR(buffer_reserve(&a->a_exceptions, MAX_SIZE_OF_ENTRY));
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
    assemble_emit_exception_table_item(a, recipe, 0);
    return SUCCESS;
}

/* An entry per range of words with one handler and one recipe */
static int
assemble_exception_table(struct assembler *a, _PyRegCfg *cfg)
{
    int ioffset = 0;
    int handler = -1, recipe = 0;
    int start = -1;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        int op_recipe = recipe;
        if (op->except >= 0 && op->nwords) {
            RETURN_IF_ERROR(assemble_recipe(a, cfg, op, &op_recipe));
        }
        if (op->except != handler || op_recipe != recipe) {
            if (handler >= 0 && ioffset > start) {
                RETURN_IF_ERROR(
                    assemble_emit_exception_table_entry(a, start, ioffset,
                                                        cfg, handler, recipe));
            }
            start = ioffset;
            handler = op->except;
            recipe = op_recipe;
        }
        ioffset += op->nwords;
    }
    if (handler >= 0 && ioffset > start) {
        RETURN_IF_ERROR(assemble_emit_exception_table_entry(a, start, ioffset,
                                                            cfg, handler,
                                                            recipe));
    }
    return SUCCESS;
}


/* Source-only operations occupy no word. Find the actual continuation first,
 * then record only this edge's eliminated source lines and exception state. */
static int
assemble_source_edge(struct assembler *a, _PyRegCfg *cfg, int from,
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
        if (!(op->flags & _PyReg_SOURCE_ORIGIN) ||
            !(op->opcode == NOP || (op->flags & _PyReg_ELIDED_NOP)))
        {
            continue;
        }
        _PyIRRecoveryEvent event = {
            .from = from, .to = to, .edge = edge,
            .handler = -1, .exc_slot = 0, .lasti = 0, .recipe = 0,
        };
        if (op->except >= 0) {
            _PyRegBlock *handler = &cfg->blocks[op->except];
            event.handler = handler->word;
            event.exc_slot = cfg->nlocalsplus + handler->startdepth - 1;
            event.lasti = handler->preserve_lasti;
            RETURN_IF_ERROR(assemble_recipe(a, cfg, op, &event.recipe));
        }
        unsigned char encoded[_PyIR_RECOVERY_RECORD];
        _PyIRSemantic_EncodeEvent(encoded, &event);
        RETURN_IF_ERROR(buffer_write(&a->a_records, encoded, sizeof(encoded)));
        _PyIRPosition position = {-1, -1, -1, -1, -1};
        if (op->loc.lineno > 0) {
            position = (_PyIRPosition){op->loc.lineno, op->loc.end_lineno,
                                      op->loc.col_offset, op->loc.end_col_offset,
                                      op->srcoff};
        }
        unsigned char location[_PyIR_POSITION_RECORD];
        _PyIRPositions_EncodeEvent(location, &position);
        RETURN_IF_ERROR(buffer_write(&a->a_positions, location, sizeof(location)));
    }
    return SUCCESS;
}

static int
assemble_source_map(struct assembler *a, _PyRegCfg *cfg)
{
    /* There is no executed predecessor before the real entry prologue.
     * Do not accept a leading source-only span without entry semantics. */
    for (int i = 0; i < cfg->nops && !cfg->ops[i].nwords; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if ((op->flags & _PyReg_SOURCE_ORIGIN) &&
            (op->opcode == NOP || (op->flags & _PyReg_ELIDED_NOP)))
        {
            PyErr_SetString(PyExc_SystemError,
                            "source-only span before executable entry");
            return ERROR;
        }
    }
    for (int b = 0; b < cfg->nblocks; b++) {
        for (int i = cfg->blocks[b].start; i < cfg->blocks[b].end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (!op->nwords) {
                continue;
            }
            const _PyIRFormat *fmt = _PyRegCfg_FormatOf(op->opcode, op->oparg);
            if (op->target >= 0 && !(fmt->flags & _PyIR_NAMES_TARGET)) {
                int target = op->target;
                RETURN_IF_ERROR(
                    assemble_source_edge(a, cfg, op->word, _PyIR_EDGE_TAKEN,
                                         target, cfg->blocks[target].start));
            }
            if (!_PyRegOp_EndsFlow(op->opcode)) {
                RETURN_IF_ERROR(
                    assemble_source_edge(a, cfg, op->word,
                                         _PyIR_EDGE_FALLTHROUGH, b,
                                         i + 1));
            }
        }
    }
    return SUCCESS;
}

/* The code-unit offset in co_code that each word stands for: the offset of
 * its op, which for a load or store that its consumer or producer absorbed
 * is the offset of the instruction that does the work. */
static int
assemble_pcmap(struct assembler *a, _PyRegCfg *cfg)
{
    a->a_pcmap = PyMem_Malloc(Py_MAX(a->a_nwords, 1) * sizeof(uint32_t));
    if (a->a_pcmap == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        for (int w = 0; w < op->nwords; w++) {
            a->a_pcmap[op->word + w] = (uint32_t)op->srcoff;
        }
    }
    return SUCCESS;
}

/* Words: choosing handlers, feedback entries, layout and encoding */

/* The farthest exit of FOR_ITER and SEND that the assembler does not relax */
int _PyIR_MaxForward = IR_MAX_FORWARD;

/* Both outcomes name real continuations: fallthrough jumps over the exit
 * jump, and completion takes the exit jump. No cleanup word is a target. */
#define RELAXED_FOR_ITER_WORDS 3
#define RELAXED_SEND_WORDS 3
#define RELAXED_BRANCH_WORDS 3

/* A code object past a register IR field limit.  The code is valid stack
 * bytecode, so the first call raises, and _PyIR_LowerCode() names the code. */
static int
too_large(const char *what, int value)
{
    PyErr_Format(PyExc_RuntimeError,
                 "code object too large: %s %d exceeds the register IR limit",
                 what, value);
    return ERROR;
}

/* The operand words of a call head with n operands */
static int
operand_word_count(int n)
{
    return (n + IR_OPERANDS_PER_WORD - 1) / IR_OPERANDS_PER_WORD;
}

/* The operands of a call (calls80, section 1), after the callable, which
 * the head names.  CALL and CALL_KW take the self input unless it is
 * virtual NULL, then the arguments, then for CALL_KW the names, which are
 * an immediate; CALL_FUNCTION_EX takes the positional arguments and, unless
 * it is virtual NULL, the keyword arguments. */
static int
call_has_self(const _PyRegOp *op)
{
    return op->opcode != CALL_FUNCTION_EX &&
           !(op->flags & _PyReg_CALL_NO_SELF);
}

static int
call_noperands(const _PyRegOp *op)
{
    if (op->opcode == CALL_FUNCTION_EX) {
        return 1 + !(op->flags & _PyReg_CALL_NO_KWARGS);
    }
    return op->nread - 1 - !call_has_self(op);
}

/* The field of operand k of op: a slot, or for the names of a CALL_KW the
 * index of their tuple in co_consts. */
static uint32_t
call_operand_field(_PyRegCfg *cfg, const _PyRegOp *op, int k, int n)
{
    if (op->opcode == CALL_KW && k == n - 1) {
        return op->kwnames > 0 ? (uint32_t)(op->kwnames - 1) : 0;
    }
    const int *in = _PyReg_IN(cfg, op);
    int first = op->opcode == CALL_FUNCTION_EX ? 2 : 2 - call_has_self(op);
    return (uint32_t)_PyRegCfg_Slot(cfg, in[first + k]);
}

/* The words that follow the head of op: fields A to D of word k name
 * operands 4k to 4k + 3, and an unused field holds IR_FIELD_MAX. */
static void
encode_operand_words(_PyRegCfg *cfg, _PyRegOp *op, _PyIRWord *words)
{
    int n = op->noperands;
    for (int k = 0; k < operand_word_count(n); k++) {
        uint32_t fields[IR_OPERANDS_PER_WORD];
        for (int f = 0; f < IR_OPERANDS_PER_WORD; f++) {
            int i = k * IR_OPERANDS_PER_WORD + f;
            fields[f] = i < n ? call_operand_field(cfg, op, i, n)
                              : IR_FIELD_MAX;
        }
        words[k] = IR_WORD(IR_OPERANDS, fields[0], fields[1], fields[2], fields[3]);
    }
}

/* The spans of a span instruction (Python/register_ops/values.rop and
 * displays.rop): the counts of its spans in declaration order, and their
 * slots in that order.  A Move has two spans: its destinations are its
 * outputs and its sources its inputs (resolution's moves hold slots,
 * _PyReg_MOVE_SLOTS).  A family with one span has a fixed destination
 * (Build: the span is its inputs) or fixed sources (its span is its
 * outputs). */
static int
span_elems(_PyRegCfg *cfg, const _PyRegOp *op, int counts[2], int *elems)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
    if (op->flags & _PyReg_MOVE_SLOTS) {
        const int *e = &cfg->operands[op->in];
        counts[0] = e[0];
        counts[1] = e[1];
        for (int k = 0; k < e[0] + e[1]; k++) {
            elems[k] = e[2 + k];
        }
        return e[0] + e[1];
    }
    int span_out = 0, nfixed_in = 0;
    for (int f = 0; f < 4; f++) {
        nfixed_in += fmt->roles[f] >= _PyIR_ROLE_IN0;
    }
    if (_PyIR_SpanCountFields(fmt) == 2) {
        counts[0] = op->nout;
        counts[1] = op->nread;
        span_out = 1;
    }
    else {
        // The span is the outputs unless the word has a destination of its own
        int has_dst = 0;
        for (int f = 0; f < 4; f++) {
            has_dst |= fmt->roles[f] == _PyIR_ROLE_DST;
        }
        span_out = !has_dst;
        counts[0] = span_out ? op->nout : op->nread - nfixed_in;
    }
    int n = 0;
    if (_PyIR_SpanCountFields(fmt) == 2) {
        for (int j = 0; j < op->nout; j++) {
            elems[n++] = _PyRegCfg_Slot(cfg, _PyReg_OUT(cfg, op)[j]);
        }
        for (int k = 0; k < op->nread; k++) {
            elems[n++] = _PyRegCfg_Slot(cfg, _PyReg_IN(cfg, op)[k]);
        }
    }
    else if (span_out) {
        for (int j = 0; j < op->nout; j++) {
            elems[n++] = _PyRegCfg_Slot(cfg, _PyReg_OUT(cfg, op)[j]);
        }
    }
    else {
        for (int k = nfixed_in; k < op->nread; k++) {
            elems[n++] = _PyRegCfg_Slot(cfg, _PyReg_IN(cfg, op)[k]);
        }
    }
    return n;
}

static int
span_nelems(_PyRegCfg *cfg, const _PyRegOp *op)
{
    if (op->flags & _PyReg_MOVE_SLOTS) {
        return cfg->operands[op->in] + cfg->operands[op->in + 1];
    }
    const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
    if (_PyIR_SpanCountFields(fmt) == 2) {
        return op->nout + op->nread;
    }
    int nfixed_in = 0, has_dst = 0;
    for (int f = 0; f < 4; f++) {
        nfixed_in += fmt->roles[f] >= _PyIR_ROLE_IN0;
        has_dst |= fmt->roles[f] == _PyIR_ROLE_DST;
    }
    return has_dst ? op->nread - nfixed_in : op->nout;
}

/* The words of a span instruction (pycore_ir.h, _PyIR_SpanElement()): the
 * counts in the head's COUNT fields, the first elements in its ELEMENT
 * fields, the rest in operand words of four fields each. */
static int
span_word_count(const _PyIRFormat *fmt, int nelems)
{
    int nhead = 0;
    for (int f = 0; f < 4; f++) {
        nhead += fmt->roles[f] == _PyIR_ROLE_ELEMENT;
    }
    return 1 + fmt->fixed_words
           + operand_word_count(nelems > nhead ? nelems - nhead : 0);
}

static int
encode_spans(_PyRegCfg *cfg, _PyRegOp *op, _PyIRWord *words)
{
    const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
    int n = span_nelems(cfg, op);
    int counts[2] = {0, 0};
    int *elems = PyMem_Malloc(Py_MAX(n, 1) * sizeof(int));
    if (elems == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    span_elems(cfg, op, counts, elems);
    _PyRegSlots slots;
    _PyRegCfg_OpSlots(cfg, op, &slots);
    uint32_t fields[4];
    int c = 0, e = 0, fixed_in = 0, ndst = 0;
    for (int f = 0; f < 4; f++) {
        int role = fmt->roles[f];
        if (role == _PyIR_ROLE_COUNT) {
            fields[f] = (uint32_t)counts[c++];
        }
        else if (role == _PyIR_ROLE_ELEMENT) {
            fields[f] = e < n ? (uint32_t)elems[e++] : IR_FIELD_MAX;
        }
        else if (role == _PyIR_ROLE_DST) {
            fields[f] = (uint32_t)slots.out[ndst++];
        }
        else if (role >= _PyIR_ROLE_IN0) {
            fields[f] = (uint32_t)_PyRegCfg_Slot(cfg, _PyReg_IN(cfg, op)[fixed_in++]);
        }
        else if (role == _PyIR_ROLE_IMM) {
            // A scalar next to the spans (Function's `present`): the op's oparg
            fields[f] = (uint32_t)op->oparg;
        }
        else if (role == _PyIR_ROLE_FB) {
            // The sites past the vector's limit share entry 0
            fields[f] = (uint32_t)(op->fb > 0 ? op->fb : IR_FB_SHARED);
        }
        else {
            fields[f] = 0;
        }
    }
    words[0] = IR_WORD(op->handler, fields[0], fields[1], fields[2], fields[3]);
    for (int w = 0; e < n; w++) {
        for (int f = 0; f < IR_OPERANDS_PER_WORD; f++) {
            fields[f] = e < n ? (uint32_t)elems[e++] : IR_FIELD_MAX;
        }
        words[1 + fmt->fixed_words + w] = IR_WORD(IR_OPERANDS, fields[0],
                                                  fields[1], fields[2], fields[3]);
    }
    PyMem_Free(elems);
    return SUCCESS;
}

/* The call-role flags of op's word, which the CallRoles pass proved
 * (Python/regcfg_passes.c): IR_F_NO_SELF for CALL and CALL_KW,
 * IR_F_NO_KW for CALL_FUNCTION_EX. */
static int
call_role_flags(const _PyRegOp *op)
{
    if (op->opcode == CALL_FUNCTION_EX) {
        return (op->flags & _PyReg_CALL_NO_KWARGS) ? IR_F_NO_KW : 0;
    }
    if (op->opcode == ROP_HANDLER_Call_Generic || op->opcode == CALL_KW) {
        int flags = (op->flags & _PyReg_CALL_NO_SELF) ? IR_F_NO_SELF : 0;
        if (op->opcode == ROP_HANDLER_Call_Generic &&
            (op->flags & _PyReg_CALL_UNINTERRUPTED)) {
            flags |= IR_F_UNINTERRUPTED;
        }
        return flags;
    }
    assert(!(op->flags & (_PyReg_CALL_NO_SELF | _PyReg_CALL_NO_KWARGS |
                          _PyReg_CALL_UNINTERRUPTED)));
    return 0;
}

/* The generic handler of op: its opcode, or for BINARY_OP the handler of
 * its operation. */
static int
choose_handler(_PyRegOp *op, PyObject *consts)
{
    int opcode = op->opcode;
    int oparg = op->oparg;
    /* A rule that emitted a register family's arm named its handler */
    if (opcode >= IR_NUM_OLD_HANDLERS) {
        return opcode;
    }
    if (opcode == YIELD_VALUE && oparg == 1) {
        return YIELD_DELEGATED;
    }
    /* With the baseline JIT on, frame entries and resumes are its warm-up
     * events and hand-offs (Python/jit.c): a choice made here, so that with
     * the JIT off the words are what they were.  A backedge's is the Jump
     * family's (the translator's rule). */
    if (opcode == RESUME && _PyInterpreterState_GET()->ceval.jit != _PyJIT_OFF) {
        return RESUME_JIT;
    }
    if (opcode == BINARY_OP) {
        // NB_SUBSCR is the GetItem family's, never a BINARY_OP handler
        if (oparg >= NB_SUBSCR) {
            PyErr_Format(PyExc_SystemError,
                         "invalid BINARY_OP operation %d", oparg);
            return ERROR;
        }
        return _PyIR_BINARY_OP_FIRST + oparg;
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
    if ((fmt->flags & _PyIR_FORWARD) && fmt->target != _PyIR_LOC_NONE) {
        // A register instruction: the oparg is its immediate; the target
        // is written by encode()
        return op->oparg;
    }
    if (fmt->flags & _PyIR_FORWARD) {
        if (op->nwords > 1) {
            return 1;
        }
        return jump_target(cfg, op) - (op->word + 1);
    }
    if (_PyIR_CALL_SHAPED(fmt->operands)) {
        return call_noperands(op);
    }
    return op->oparg;
}

/* The order in which sites get feedback entries (pyjit/design/register-ir.md
 * G5): in a function-like body, sites inside a loop, then the rest. */
enum {
    FB_CLASS_LOOP,
    FB_CLASS_OTHER,
    FB_NCLASSES,
};

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
        slots.base, slots.out[0], cfg->nlocalsplus, op->except, handler_base,
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
            // The retirement of the resume input alone (rule_pop_top)
            int pop = next_frontend_op(cfg, i + 1, 1);
            if (pop >= 0 && cfg->ops[pop].opcode == ROP_SELECT_Move_retire &&
                op->nout == 1 && cfg->ops[pop].nout == 0 &&
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
        int dst = slots.out[0];
        if (op->opcode != FOR_ITER) {
            _PyRegSlots completed;
            _PyRegCfg_OpSlots(cfg, &cfg->ops[op->completion - 1], &completed);
            dst = completed.out[0];
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

/* Choose the handlers. */
static int
choose_handlers(_PyRegCfg *cfg, PyObject *consts)
{
    RETURN_IF_ERROR(lower_completions(cfg));
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        op->fb = -1;
        if (op->opcode == _PyReg_DEAD) {
            op->nwords = 0;
            continue;
        }
        // A register instruction arm is its own handler
        assert(op->opcode >= ROP_FIRST_HANDLER ||
               (!IS_PSEUDO_INSTR(op->opcode) && IS_VALID_OPCODE(op->opcode)));
        if (op->opcode == NOP || (op->flags & (_PyReg_ELIDED | _PyReg_COMPLETED))) {
            op->handler = NOP;
            op->nwords = 0;
            continue;
        }
        op->handler = choose_handler(op, consts);
        if (op->handler < 0) {
            return ERROR;
        }
        op->nwords = 1 + _PyIR_FormatOf(op->handler)->fixed_words;
        if (_PyIR_CALL_SHAPED(_PyIR_FormatOf(op->handler)->operands)) {
            op->noperands = call_noperands(op);
            if (op->noperands > IR_FIELD_MAX) {
                return too_large("call operands", op->noperands);
            }
            if (op->opcode == CALL_KW && op->kwnames - 1 >= IR_FIELD_MAX) {
                return too_large("call keyword names constant", op->kwnames - 1);
            }
            op->nwords += operand_word_count(op->noperands);
        }
        else if (_PyIR_FormatOf(op->handler)->operands == _PyIR_OPERANDS_SPANS) {
            int n = span_nelems(cfg, op);
            if (n > 2 * IR_FIELD_MAX) {
                return too_large("parallel move", n);
            }
            op->nwords = span_word_count(_PyIR_FormatOf(op->handler), n);
        }
    }
    return SUCCESS;
}

/* Place the feedback entries, in the units of pycore_ir.h: entry 0 is the
 * header, the others follow in the order of FB_CLASS_*, each in source
 * order, as long as they end by IR_FB_UNITS_MAX.  Sites past that share
 * entry 0, which never specialises and keeps no operand; their arms run
 * the generic path.  a_fbinit gets one word per entry, in order of address.
 * The entries are placed in order from the start of the vector, then
 * counted up toward entry 0 (IR_FB_SHARED), which ends it. */
static int
place_entries(struct assembler *a, _PyRegCfg *cfg, int code_flags)
{
    char *cls = PyMem_Calloc(cfg->nops + 1, 2);
    if (cls == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    char *in_loop = cls + cfg->nops + 1;
    int res = ERROR;
    PyMem_Free(a->a_fbinit);
    a->a_fbinit = NULL;
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
        // An arm may keep its entry in an operand word
        if (fmt->fb == _PyIR_LOC_NONE && fmt->fixed_words == 0) {
            continue;
        }
        // Only entry 0, which the sites without an entry share, is a counter
        assert(fmt->fb_size != 0 && fmt->fb_kind != FB_COUNTER);
        cls[i] = in_loop[i] ? FB_CLASS_LOOP : FB_CLASS_OTHER;
        nsites++;
    }
    a->a_fbinit = PyMem_Malloc((nsites + 1) * sizeof(uint32_t));
    if (a->a_fbinit == NULL) {
        PyErr_NoMemory();
        goto done;
    }
    uint32_t *fbinit = a->a_fbinit;
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
                if (fmt->imm == _PyIR_LOC_FBARG) {
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
            op->fb = IR_FB_SHARED;
        }
    }
    // Entry 0 exists only with others
    if (nfb > 1) {
        uint32_t total = unit;
        fbinit[0] = _PyIR_FBINIT(FB_COUNTER, IR_FB_SHARED, total);
        for (int i = 1; i < nfb; i++) {
            uint32_t word = fbinit[i];
            fbinit[i] = _PyIR_FBINIT(_PyIR_FBINIT_KIND(word),
                                     _PyIR_FBINIT_FIELD(word) - total
                                     + IR_FB_SHARED,
                                     _PyIR_FBINIT_ARG(word));
        }
        for (int i = 0; i < cfg->nops; i++) {
            _PyRegOp *op = &cfg->ops[i];
            if (op->fb > 0 && op->fb < IR_FB_SHARED) {
                op->fb += IR_FB_SHARED - (int)total;
            }
        }
    }
    a->a_nfb = nfb > 1 ? nfb : 0;
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
            if (word <= _PyIR_MaxForward) {
                continue;
            }
            const _PyIRFormat *fmt = _PyIR_FormatOf(op->handler);
            if (fmt->target == _PyIR_LOC_C || fmt->target == _PyIR_LOC_D) {
                // A conditional register instruction whose target a
                // 12-bit distance cannot reach: it jumps to a word that
                // jumps to the target, and the fall-through word jumps over
                // that one (RELAXED_BRANCH_WORDS)
                int distance = jump_target(cfg, op) - (op->word + 1);
                if (op->nwords == 1 + fmt->fixed_words
                    && (distance > _PyIR_MaxForward
                        || distance < IR_MIN_DISTANCE)) {
                    op->nwords = RELAXED_BRANCH_WORDS + fmt->fixed_words;
                    changed = 1;
                }
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
        slots.out[0] = completed.out[0];
    }
    int oparg = handler_oparg(cfg, op);
    // Every role's value, then a pick per field.  DST is meaningless when
    // fmt->dst is _PyIR_NO_DST.
    int values[_PyIR_ROLE_IN0 + 4] = {
        [_PyIR_ROLE_NONE] = 0,
        [_PyIR_ROLE_DST] = slots.out[0],
        [_PyIR_ROLE_BASE] = slots.base,
        [_PyIR_ROLE_FB] = op->fb > 0 ? op->fb : IR_FB_SHARED,
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
    int ndst = 0;
    for (int r = 0; r < 4; r++) {
        int role = fmt->roles[r];
        assert(role < _PyIR_ROLE_IN0 + 4);
        int value = values[role];
        if (role == _PyIR_ROLE_DST) {
            value = slots.out[ndst++];
        }
        if (!_PyIR_RoleIsImm(role) && (value < 0 || value > IR_FIELD_MAX)) {
            // Only unreachable code names slots past the frame, since the
            // slot count is checked first.
            assert(!reachable);
            value = 0;
        }
        fields[r] = (uint32_t)value;
    }
    if (fmt->target != _PyIR_LOC_NONE) {
        // A register instruction's target, a signed distance in the fields
        // its encoding gives it; the oparg, if any, has its own field
        // A relaxed branch takes the second jump, which follows the
        // operand words and the first jump
        int distance = op->nwords > 1 + fmt->fixed_words
            ? 1 + fmt->fixed_words : jump_target(cfg, op) - (op->word + 1);
        int bits = fmt->target == _PyIR_LOC_AB ? 24 : 12;
        if (distance < -(1 << (bits - 1)) || distance >= (1 << (bits - 1))) {
            if (reachable) {
                return too_large("distance", distance);
            }
            distance = 0;
        }
        if (fmt->target == _PyIR_LOC_AB) {
            fields[0] = (uint32_t)distance & IR_FIELD_MAX;
            fields[1] = ((uint32_t)distance >> 12) & IR_FIELD_MAX;
        }
        else {
            fields[fmt->target - _PyIR_LOC_A] = (uint32_t)distance & IR_FIELD_MAX;
        }
        if (fmt->imm >= _PyIR_LOC_A && fmt->imm <= _PyIR_LOC_D) {
            if (oparg < 0 || oparg > IR_FIELD_MAX) {
                return too_large("oparg", oparg);
            }
            fields[fmt->imm - _PyIR_LOC_A] = (uint32_t)oparg;
        }
    }
    else if (fmt->imm == _PyIR_LOC_CD || fmt->imm == _PyIR_LOC_BC ||
             fmt->imm == _PyIR_LOC_AB) {
        if (oparg < 0 || oparg > IR_CD_MAX) {
            if (reachable) {
                return too_large("oparg", oparg);
            }
            oparg = 0;
        }
        int low = fmt->imm == _PyIR_LOC_CD ? 2 : fmt->imm == _PyIR_LOC_BC ? 1 : 0;
        fields[low] = oparg & IR_FIELD_MAX;
        fields[low + 1] = oparg >> 12;
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
    int flags = call_role_flags(op);
    *word = IR_WORD(op->handler | flags,
                    fields[0], fields[1], fields[2], fields[3]);
    return SUCCESS;
}

static inline _PyIRWord
jump_word(int target)
{
    return IR_WORD(IR_JUMP, 0, 0, target & IR_FIELD_MAX, target >> 12);
}

/* Word entries keep their original homed IO. Only a private native outcome
 * consumes a payload, so initial None and debugger/native materialization
 * remain ordinary ingress. Use the same bounded physical-edge predicates as
 * public admission, after logical completion lowering and allocation. */
static void
place_generator_boundaries(_PyIRWord *words, int nwords, int nlocalsplus)
{
#ifdef Py_GIL_DISABLED
    return;
#else
    // An arm's operand words are not instructions: step over them
    for (int pc = 0; pc < nwords; pc += _PyIR_WordLength(words[pc])) {
        int handler = _PyIR_GeneratorBoundaryCandidate(
            words, nwords, nlocalsplus, pc);
        if (handler >= 0) {
            words[pc] = (words[pc] & ~(_PyIRWord)IR_HANDLER_MASK) | (_PyIRWord)handler;
        }
    }
    /* Only removals follow nomination. A missing throw/join/loop sibling
     * therefore removes its connected shapes and cannot oscillate. */
    int changed;
    do {
        changed = 0;
        for (int pc = 0; pc < nwords; pc += _PyIR_WordLength(words[pc])) {
            int handler = IR_HANDLER(words[pc]);
            const _PyIRGeneratorBoundary *boundary =
                _PyIR_GeneratorBoundaryOf(handler);
            if (boundary == NULL ||
                !(boundary->flags & _PyIR_GEN_BOUNDARY_PLACED) ||
                _PyIR_CheckGeneratorBoundary(words, nwords, nlocalsplus,
                                            pc, handler))
            {
                continue;
            }
            words[pc] = (words[pc] & ~(_PyIRWord)IR_HANDLER_MASK) | boundary->ordinary;
            changed = 1;
        }
    } while (changed);
#endif
}

/* The words and the feedback table, in a->a_words and a->a_fbinit */
static int
assemble_words(struct assembler *a, _PyRegCfg *cfg, PyObject *consts,
               int nfixed, int nslots, int code_flags)
{
    if (nslots > IR_MAX_SLOTS) {
        PyErr_Format(PyExc_RuntimeError,
                     "too many local variables and temporaries in function "
                     "(needs %d slots, limit %d)", nslots, IR_MAX_SLOTS);
        return ERROR;
    }
    RETURN_IF_ERROR(choose_handlers(cfg, consts));
    RETURN_IF_ERROR(place_entries(a, cfg, code_flags));
    RETURN_IF_ERROR(layout(a, cfg));

    a->a_words = PyMem_Calloc(Py_MAX(a->a_nwords, 1), sizeof(_PyIRWord));
    if (a->a_words == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    _PyIRWord *words = a->a_words;
    for (int b = 0; b < cfg->nblocks; b++) {
        _PyRegBlock *blk = &cfg->blocks[b];
        for (int i = blk->start; i < blk->end; i++) {
            _PyRegOp *op = &cfg->ops[i];
            int w = op->word;
            if (op->nwords == 0) {
                continue;
            }
            if (_PyIR_FormatOf(op->handler)->operands == _PyIR_OPERANDS_SPANS) {
                RETURN_IF_ERROR(encode_spans(cfg, op, &words[w]));
                continue;
            }
            RETURN_IF_ERROR(encode(cfg, op, blk->reachable, &words[w]));
            if (_PyIR_CALL_SHAPED(_PyIR_FormatOf(op->handler)->operands)) {
                encode_operand_words(cfg, op, &words[w + 1]);
            }
            else if (_PyIR_FormatOf(op->handler)->fixed_words) {
                // P0 of the first operand word is the feedback entry
                words[w + 1] = (_PyIRWord)(op->fb > 0 ? op->fb : IR_FB_SHARED);
                if (op->nwords > 1 + _PyIR_FormatOf(op->handler)->fixed_words) {
                    // A relaxed branch: its fall-through word jumps over
                    // the second jump, which goes to the target
                    int k = _PyIR_FormatOf(op->handler)->fixed_words;
                    words[w + 1 + k] = jump_word(w + RELAXED_BRANCH_WORDS + k);
                    words[w + 2 + k] = jump_word(jump_target(cfg, op));
                }
            }
            else if (op->nwords > 1) {
                int exit = jump_target(cfg, op);
                if (op->opcode == FOR_ITER) {
                    words[w + 1] = jump_word(w + RELAXED_FOR_ITER_WORDS);
                    words[w + 2] = jump_word(exit);
                }
                else if (_PyIR_FormatOf(op->handler)->target != _PyIR_LOC_NONE) {
                    // The branch reaches the second jump; the word after the
                    // branch skips it
                    words[w + 1] = jump_word(w + RELAXED_BRANCH_WORDS);
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
    place_generator_boundaries(words, a->a_nwords, nfixed);
    return SUCCESS;
}

/* Move the arrays of a into one allocation, the _PyCodeIR of the code */
static int
make_ir(struct assembler *a, _PyRegCfg *cfg, int nfixed, int nslots,
        _PyCodeIR **result)
{
    // The size and count limits of the fields are far below 2**32
    size_t nwords = (size_t)a->a_nwords;
    size_t nfb = (size_t)a->a_nfb;
    size_t nconst = (size_t)cfg->nconst;
    size_t nexc = a->a_exceptions.size;
    size_t nrecords = a->a_records.size / _PyIR_RECOVERY_RECORD;
    size_t nrecipes = a->a_recipes.size / sizeof(int32_t);
    size_t size = ALIGN8(sizeof(_PyCodeIR))
        + ALIGN8(nwords * sizeof(_PyIRWord))
        + ALIGN8(nconst * sizeof(PyValueBorrow))
        + ALIGN8(nwords * sizeof(uint32_t))
        + ALIGN8(nfb * sizeof(uint32_t))
        + ALIGN8(nexc)
        + ALIGN8(nrecords * _PyIR_RECOVERY_RECORD)
        + ALIGN8(nrecords * _PyIR_POSITION_RECORD)
        + ALIGN8(nrecipes * sizeof(int32_t));
    char *block = PyMem_Malloc(size);
    if (block == NULL) {
        PyErr_NoMemory();
        return ERROR;
    }
    _PyCodeIR *ir = (_PyCodeIR *)block;
    char *p = block + ALIGN8(sizeof(_PyCodeIR));
    ir->words = (_PyIRWord *)p;
    memcpy(p, a->a_words, nwords * sizeof(_PyIRWord));
    p += ALIGN8(nwords * sizeof(_PyIRWord));
    ir->const_values = (PyValueBorrow *)p;
    if (nconst) {
        memcpy(p, cfg->const_values, nconst * sizeof(PyValueBorrow));
    }
    p += ALIGN8(nconst * sizeof(PyValueBorrow));
    ir->pcmap = (uint32_t *)p;
    memcpy(p, a->a_pcmap, nwords * sizeof(uint32_t));
    p += ALIGN8(nwords * sizeof(uint32_t));
    ir->fbinit = (uint32_t *)p;
    if (nfb) {
        memcpy(p, a->a_fbinit, nfb * sizeof(uint32_t));
    }
    p += ALIGN8(nfb * sizeof(uint32_t));
    ir->exceptions = (uint8_t *)p;
    if (nexc) {
        memcpy(p, a->a_exceptions.data, nexc);
    }
    p += ALIGN8(nexc);
    ir->records = (uint8_t *)p;
    if (nrecords) {
        memcpy(p, a->a_records.data, nrecords * _PyIR_RECOVERY_RECORD);
    }
    p += ALIGN8(nrecords * _PyIR_RECOVERY_RECORD);
    ir->positions = (uint8_t *)p;
    if (nrecords) {
        memcpy(p, a->a_positions.data, nrecords * _PyIR_POSITION_RECORD);
    }
    p += ALIGN8(nrecords * _PyIR_POSITION_RECORD);
    ir->recipes = (int32_t *)p;
    if (nrecipes) {
        memcpy(p, a->a_recipes.data, nrecipes * sizeof(int32_t));
    }
    ir->nrecipes = (uint32_t)nrecipes;
    ir->nwords = (uint32_t)nwords;
    ir->nfixed = (uint32_t)nfixed;
    ir->nslots = (uint32_t)nslots;
    ir->nfeedback = (uint32_t)nfb;
    ir->nconst = (uint32_t)nconst;
    ir->const_first = (uint32_t)cfg->const_first;
    ir->nexceptions = (uint32_t)nexc;
    ir->nrecords = (uint32_t)nrecords;
    ir->nbytes = (uint32_t)size;
    ir->flags = cfg->virtual_kwargs ? _PyIR_CODE_VIRTUAL_KWARGS : 0;
    /* The entry RESUME starts the code's execution, past the prefix of a
     * generator */
    ir->first_traceable = ir->nwords;
    for (int i = 0; i < cfg->nops; i++) {
        _PyRegOp *op = &cfg->ops[i];
        if (op->nwords && op->opcode == RESUME &&
            (op->oparg & RESUME_OPARG_LOCATION_MASK) != RESUME_AT_GEN_EXPR_START)
        {
            ir->first_traceable = (uint32_t)op->word;
            break;
        }
    }
    *result = ir;
    return SUCCESS;
}

int
_PyIR_Assemble(PyCodeObject *co, _PyRegCfg *cfg, _PyCodeIR **result)
{
    struct assembler a;
    memset(&a, 0, sizeof(a));
    PyObject *consts = co->co_consts;
    int res = _PyRegCfg_AssignConstSlots(cfg, consts);
    // The hidden constant slots and the discard slot are fixed, with the
    // locals, cells and free variables
    int nfixed = cfg->nlocalsplus;
    // A frame has a scratch slot even where nothing is pushed
    int nslots = nfixed + Py_MAX(cfg->ntemps, 1);
    if (res == SUCCESS) {
        res = assemble_words(&a, cfg, consts, nfixed, nslots, co->co_flags);
    }
    if (res == SUCCESS) {
        res = assemble_exception_table(&a, cfg);
    }
    if (res == SUCCESS) {
        res = assemble_source_map(&a, cfg);
    }
    if (res == SUCCESS) {
        res = assemble_pcmap(&a, cfg);
    }
    if (res == SUCCESS) {
        res = make_ir(&a, cfg, nfixed, nslots, result);
    }
    assemble_free(&a);
    return res;
}
