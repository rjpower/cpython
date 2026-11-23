/* Hooks for Lib/test/test_jit_asm.py: the JIT's assembler and code memory.

   A program is a list of tuples, ("name", argument...).  An argument is an
   int (a register, a condition, an immediate, a label number or an address)
   or a tuple (base, disp) or (base, index, scale, disp) for a memory
   operand.  The names are the table of `ops` below. */

#include "parts.h"

#include "pycore_jit_asm.h"

#ifdef _PY_JIT_SUPPORTED

#define MAX_LABELS 256
#define MAX_INTS 4
#define MAX_MEMS 2

typedef struct {
    _PyJitAsm a;
    _PyJitLabel labels[MAX_LABELS];   // by the program's label numbers
    int created[MAX_LABELS];
} Program;

static _PyJitLabel
label_of(Program *p, int64_t number)
{
    assert(number >= 0 && number < MAX_LABELS);
    if (!p->created[number]) {
        p->labels[number] = _PyJitAsm_NewLabel(&p->a);
        p->created[number] = 1;
    }
    return p->labels[number];
}

#define A (&p->a)
#define L(i) label_of(p, v[i])
#define M(i) m[i]

/* The ops: name, number of integer and memory arguments, the call.  v holds the integer arguments and m the memory
   operands, each in the order they appear in the tuple. */
#define OPS(OP) \
    OP(section, 1, 0, _PyJitAsm_Section(A, v[0])) \
    OP(bind, 1, 0, _PyJitAsm_Bind(A, L(0))) \
    OP(align, 1, 0, _PyJitAsm_Align(A, v[0])) \
    OP(quad_label, 1, 0, _PyJitAsm_QuadLabel(A, L(0))) \
    OP(mov_rr, 2, 0, _PyJitAsm_MovRR(A, v[0], v[1])) \
    OP(mov_imm, 2, 0, _PyJitAsm_MovImm(A, v[0], v[1])) \
    OP(mov_rm, 1, 1, _PyJitAsm_MovRM(A, v[0], M(0))) \
    OP(mov_mr, 1, 1, _PyJitAsm_MovMR(A, M(0), v[0])) \
    OP(mov_mi, 1, 1, _PyJitAsm_MovMI(A, M(0), v[0])) \
    OP(mov_rm32, 1, 1, _PyJitAsm_MovRM32(A, v[0], M(0))) \
    OP(mov_mr32, 1, 1, _PyJitAsm_MovMR32(A, M(0), v[0])) \
    OP(mov_mi32, 1, 1, _PyJitAsm_MovMI32(A, M(0), v[0])) \
    OP(movzx_rm8, 1, 1, _PyJitAsm_MovzxRM8(A, v[0], M(0))) \
    OP(movzx_rm16, 1, 1, _PyJitAsm_MovzxRM16(A, v[0], M(0))) \
    OP(movsx_rm8, 1, 1, _PyJitAsm_MovsxRM8(A, v[0], M(0))) \
    OP(movsx_rm16, 1, 1, _PyJitAsm_MovsxRM16(A, v[0], M(0))) \
    OP(movsx_rm32, 1, 1, _PyJitAsm_MovsxRM32(A, v[0], M(0))) \
    OP(lea, 1, 1, _PyJitAsm_Lea(A, v[0], M(0))) \
    OP(lea_label, 2, 0, _PyJitAsm_LeaLabel(A, v[0], L(1))) \
    OP(alu_rr, 3, 0, _PyJitAsm_AluRR(A, v[0], v[1], v[2])) \
    OP(alu_rm, 2, 1, _PyJitAsm_AluRM(A, v[0], v[1], M(0))) \
    OP(alu_mr, 2, 1, _PyJitAsm_AluMR(A, v[0], M(0), v[1])) \
    OP(alu_ri, 3, 0, _PyJitAsm_AluRI(A, v[0], v[1], v[2])) \
    OP(alu_mi, 2, 1, _PyJitAsm_AluMI(A, v[0], M(0), v[1])) \
    OP(test_rr, 2, 0, _PyJitAsm_TestRR(A, v[0], v[1])) \
    OP(test_mr, 1, 1, _PyJitAsm_TestMR(A, M(0), v[0])) \
    OP(test_ri, 2, 0, _PyJitAsm_TestRI(A, v[0], v[1])) \
    OP(test_mi, 1, 1, _PyJitAsm_TestMI(A, M(0), v[0])) \
    OP(imul_rr, 2, 0, _PyJitAsm_ImulRR(A, v[0], v[1])) \
    OP(imul_rm, 1, 1, _PyJitAsm_ImulRM(A, v[0], M(0))) \
    OP(imul_rri, 3, 0, _PyJitAsm_ImulRRI(A, v[0], v[1], v[2])) \
    OP(unary_r, 2, 0, _PyJitAsm_UnaryR(A, v[0], v[1])) \
    OP(unary_m, 1, 1, _PyJitAsm_UnaryM(A, v[0], M(0))) \
    OP(shift_ri, 3, 0, _PyJitAsm_ShiftRI(A, v[0], v[1], v[2])) \
    OP(shift_rcl, 2, 0, _PyJitAsm_ShiftRCl(A, v[0], v[1])) \
    OP(setcc, 2, 0, _PyJitAsm_Setcc(A, v[0], v[1])) \
    OP(cmov_rr, 3, 0, _PyJitAsm_CmovRR(A, v[0], v[1], v[2])) \
    OP(cmov_rm, 2, 1, _PyJitAsm_CmovRM(A, v[0], v[1], M(0))) \
    OP(jmp, 1, 0, _PyJitAsm_Jmp(A, L(0))) \
    OP(jcc, 2, 0, _PyJitAsm_Jcc(A, v[0], L(1))) \
    OP(jmp_r, 1, 0, _PyJitAsm_JmpR(A, v[0])) \
    OP(call_r, 1, 0, _PyJitAsm_CallR(A, v[0])) \
    OP(jmp_m, 0, 1, _PyJitAsm_JmpM(A, M(0))) \
    OP(call_m, 0, 1, _PyJitAsm_CallM(A, M(0))) \
    OP(call_abs, 1, 0, _PyJitAsm_CallAbs(A, (const void *)(uintptr_t)v[0])) \
    OP(push, 1, 0, _PyJitAsm_Push(A, v[0])) \
    OP(pop, 1, 0, _PyJitAsm_Pop(A, v[0])) \
    OP(ret, 0, 0, _PyJitAsm_Ret(A)) \
    OP(movsd_rr, 2, 0, _PyJitAsm_MovsdRR(A, v[0], v[1])) \
    OP(movsd_rm, 1, 1, _PyJitAsm_MovsdRM(A, v[0], M(0))) \
    OP(movsd_mr, 1, 1, _PyJitAsm_MovsdMR(A, M(0), v[0])) \
    OP(movq_xr, 2, 0, _PyJitAsm_MovqXR(A, v[0], v[1])) \
    OP(movq_rx, 2, 0, _PyJitAsm_MovqRX(A, v[0], v[1])) \
    OP(sse_rr, 3, 0, _PyJitAsm_SseRR(A, v[0], v[1], v[2])) \
    OP(sse_rm, 2, 1, _PyJitAsm_SseRM(A, v[0], v[1], M(0))) \
    OP(cvtsi2sd, 2, 0, _PyJitAsm_Cvtsi2sd(A, v[0], v[1])) \
    OP(cvttsd2si, 2, 0, _PyJitAsm_Cvttsd2si(A, v[0], v[1]))

#define OP(name, ints, mems, call) \
    static void op_##name(Program *p, const int64_t *v, const _PyJitMem *m) \
    { (void)v; (void)m; call; }
OPS(OP)
#undef OP

#define OP(name, ints, mems, call) {#name, op_##name, ints, mems},
static const struct {
    const char *name;
    void (*run)(Program *, const int64_t *, const _PyJitMem *);
    int nints;
    int nmems;
} ops[] = {
    OPS(OP)
};
#undef OP

/* The integer at index i of a tuple. */
static int
int_item(PyObject *tuple, Py_ssize_t i, long long *value)
{
    PyObject *item = PyTuple_GetItemRef(tuple, i);
    if (item == NULL) {
        return -1;
    }
    int overflow;
    *value = PyLong_AsLongLongAndOverflow(item, &overflow);
    if (overflow > 0) {
        // An address above INT64_MAX: keep its bits.
        *value = (long long)PyLong_AsUnsignedLongLong(item);
    }
    Py_DECREF(item);
    return PyErr_Occurred() ? -1 : 0;
}

static int
parse_mem(PyObject *item, _PyJitMem *mem)
{
    Py_ssize_t n = PyTuple_GET_SIZE(item);
    if (n != 2 && n != 4) {
        PyErr_SetString(PyExc_TypeError, "memory operand is (base, disp) or "
                        "(base, index, scale, disp)");
        return -1;
    }
    long long values[4];
    for (Py_ssize_t i = 0; i < n; i++) {
        if (int_item(item, i, &values[i]) < 0) {
            return -1;
        }
    }
    *mem = n == 2 ? _PyJitMem_At(values[0], values[1])
                  : _PyJitMem_Indexed(values[0], values[1], values[2], values[3]);
    return 0;
}

static int
run_op(Program *p, PyObject *op)
{
    PyObject *name_object = PyTuple_Check(op) && PyTuple_GET_SIZE(op) >= 1
        ? PyTuple_GetItemRef(op, 0) : NULL;
    if (name_object == NULL || !PyUnicode_Check(name_object)) {
        Py_XDECREF(name_object);
        PyErr_SetString(PyExc_TypeError, "op must be a tuple starting with a name");
        return -1;
    }
    const char *name = PyUnicode_AsUTF8(name_object);
    Py_DECREF(name_object);    // the name is kept alive by the tuple
    if (name == NULL) {
        return -1;
    }
    for (size_t i = 0; i < Py_ARRAY_LENGTH(ops); i++) {
        if (strcmp(ops[i].name, name) != 0) {
            continue;
        }
        int64_t v[MAX_INTS] = {0};
        _PyJitMem m[MAX_MEMS];
        int nints = 0, nmems = 0;
        for (Py_ssize_t k = 1; k < PyTuple_GET_SIZE(op); k++) {
            PyObject *item = PyTuple_GetItemRef(op, k);
            if (item == NULL) {
                return -1;
            }
            int is_mem = PyTuple_Check(item);
            int ok = -1;
            if (is_mem && nmems < MAX_MEMS) {
                ok = parse_mem(item, &m[nmems++]);
            }
            else if (!is_mem && nints < MAX_INTS) {
                long long x;
                ok = int_item(op, k, &x);
                v[nints++] = x;
            }
            Py_DECREF(item);
            if (ok < 0) {
                if (!PyErr_Occurred()) {
                    PyErr_Format(PyExc_TypeError, "bad argument %zd of %s", k, name);
                }
                return -1;
            }
        }
        if (nints != ops[i].nints || nmems != ops[i].nmems) {
            PyErr_Format(PyExc_TypeError,
                         "%s takes %d integers and %d memory operands",
                         name, ops[i].nints, ops[i].nmems);
            return -1;
        }
        ops[i].run(p, v, m);
        return 0;
    }
    PyErr_Format(PyExc_ValueError, "unknown op %s", name);
    return -1;
}

static int
build(Program *p, PyObject *program)
{
    _PyJitAsm_Init(A);
    memset(p->created, 0, sizeof(p->created));
    PyObject *seq = PySequence_Tuple(program);
    if (seq == NULL) {
        return -1;
    }
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(seq); i++) {
        PyObject *op = PyTuple_GetItemRef(seq, i);
        int failed = op == NULL || run_op(p, op) < 0;
        Py_XDECREF(op);
        if (failed) {
            Py_DECREF(seq);
            return -1;
        }
    }
    Py_DECREF(seq);
    return 0;
}

/* jit_asm_assemble(program, address) -> bytes: the program's code as if it
   were placed at `address` (nothing is placed), or None if it cannot be
   resolved there. */
static PyObject *
jit_asm_assemble(PyObject *module, PyObject *args)
{
    PyObject *program;
    unsigned long long address;
    if (!PyArg_ParseTuple(args, "OK", &program, &address)) {
        return NULL;
    }
    Program p;
    if (build(&p, program) < 0) {
        _PyJitAsm_Fini(&p.a);
        return NULL;
    }
    PyObject *result = PyBytes_FromStringAndSize(NULL, _PyJitAsm_Size(&p.a));
    if (result != NULL
        && _PyJitAsm_Resolve(&p.a, (uint8_t *)PyBytes_AS_STRING(result),
                             (uintptr_t)address) < 0)
    {
        Py_SETREF(result, Py_NewRef(Py_None));
    }
    _PyJitAsm_Fini(&p.a);
    return result;
}

/* jit_asm_place(program) -> (address, size), or None if placement fails.
   The code stays placed until jit_memory_free(). */
static PyObject *
jit_asm_place(PyObject *module, PyObject *program)
{
    Program p;
    if (build(&p, program) < 0) {
        _PyJitAsm_Fini(&p.a);
        return NULL;
    }
    _PyJitCode code;
    int placed = _PyJitAsm_Place(&p.a, &code);
    _PyJitAsm_Fini(&p.a);
    if (placed < 0) {
        Py_RETURN_NONE;
    }
    return Py_BuildValue("(KK)", (unsigned long long)(uintptr_t)code.start,
                         (unsigned long long)code.size);
}

/* jit_asm_call(address, ints, doubles, returns_double) calls placed code as
   a C function of up to four integer and four double arguments. */
static PyObject *
jit_asm_call(PyObject *module, PyObject *args)
{
    unsigned long long address;
    PyObject *ints, *doubles;
    int returns_double;
    if (!PyArg_ParseTuple(args, "KO!O!p", &address, &PyTuple_Type, &ints,
                          &PyTuple_Type, &doubles, &returns_double))
    {
        return NULL;
    }
    int64_t i[MAX_INTS] = {0};
    double d[MAX_INTS] = {0};
    if (PyTuple_GET_SIZE(ints) > MAX_INTS || PyTuple_GET_SIZE(doubles) > MAX_INTS) {
        PyErr_SetString(PyExc_ValueError, "at most four arguments of each kind");
        return NULL;
    }
    for (Py_ssize_t k = 0; k < PyTuple_GET_SIZE(ints); k++) {
        long long x;
        if (int_item(ints, k, &x) < 0) {
            return NULL;
        }
        i[k] = x;
    }
    for (Py_ssize_t k = 0; k < PyTuple_GET_SIZE(doubles); k++) {
        PyObject *item = PyTuple_GetItemRef(doubles, k);
        if (item == NULL) {
            return NULL;
        }
        d[k] = PyFloat_AsDouble(item);
        Py_DECREF(item);
        if (d[k] == -1.0 && PyErr_Occurred()) {
            return NULL;
        }
    }
    if (returns_double) {
        double (*f)(int64_t, int64_t, int64_t, int64_t,
                    double, double, double, double) = (void *)(uintptr_t)address;
        return PyFloat_FromDouble(f(i[0], i[1], i[2], i[3], d[0], d[1], d[2], d[3]));
    }
    int64_t (*f)(int64_t, int64_t, int64_t, int64_t,
                 double, double, double, double) = (void *)(uintptr_t)address;
    return PyLong_FromLongLong(f(i[0], i[1], i[2], i[3], d[0], d[1], d[2], d[3]));
}

/* A C function for generated code to call through a register. */
static int64_t
call_target(int64_t x, int64_t y)
{
    return 3 * x + y;
}

/* jit_asm_address(name) -> int: the address of a C function for the tests.
   "Py_HashPointer" is in the interpreter's text, where call rel32 reaches;
   "call_target" is in this module, which may be anywhere. */
static PyObject *
jit_asm_address(PyObject *module, PyObject *name)
{
    const char *s = PyUnicode_AsUTF8(name);
    if (s == NULL) {
        return NULL;
    }
    uintptr_t address;
    if (strcmp(s, "Py_HashPointer") == 0) {
        address = (uintptr_t)&Py_HashPointer;
    }
    else if (strcmp(s, "call_target") == 0) {
        address = (uintptr_t)&call_target;
    }
    else {
        PyErr_SetString(PyExc_ValueError, "unknown function");
        return NULL;
    }
    return PyLong_FromUnsignedLongLong(address);
}

static PyObject *
jit_memory_info(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    if (_PyJitMemory_Init() < 0) {
        Py_RETURN_NONE;
    }
    _PyJitMemoryStats s;
    _PyJitMemory_GetStats(&s);
    return Py_BuildValue(
        "{s:K,s:K,s:K,s:K,s:K,s:K,s:K}",
        "reservation", (unsigned long long)s.reservation,
        "reservation_size", (unsigned long long)s.reservation_size,
        "text_low", (unsigned long long)s.text_low,
        "text_high", (unsigned long long)s.text_high,
        "pages_in_use", (unsigned long long)s.pages_in_use,
        "bytes_in_use", (unsigned long long)s.bytes_in_use,
        "page_size", (unsigned long long)s.page_size);
}

/* jit_memory_alloc(size) -> address, or None if there is no room.  The
   pages are writable and untouched. */
static PyObject *
jit_memory_alloc(PyObject *module, PyObject *size)
{
    size_t n = PyLong_AsSize_t(size);
    if (n == (size_t)-1 && PyErr_Occurred()) {
        return NULL;
    }
    _PyJitCode code;
    if (_PyJitMemory_Alloc(n, &code) < 0) {
        Py_RETURN_NONE;
    }
    return PyLong_FromUnsignedLongLong((uintptr_t)code.start);
}

static PyObject *
jit_memory_free(PyObject *module, PyObject *args)
{
    unsigned long long address;
    size_t size;
    if (!PyArg_ParseTuple(args, "Kn", &address, &size)) {
        return NULL;
    }
    _PyJitCode code = {(uint8_t *)(uintptr_t)address, size};
    _PyJitMemory_Free(&code);
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"jit_asm_assemble", jit_asm_assemble, METH_VARARGS},
    {"jit_asm_place", jit_asm_place, METH_O},
    {"jit_asm_call", jit_asm_call, METH_VARARGS},
    {"jit_asm_address", jit_asm_address, METH_O},
    {"jit_memory_info", jit_memory_info, METH_NOARGS},
    {"jit_memory_alloc", jit_memory_alloc, METH_O},
    {"jit_memory_free", jit_memory_free, METH_VARARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_JitAsm(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}

#else

int
_PyTestInternalCapi_Init_JitAsm(PyObject *module)
{
    return 0;
}

#endif  // _PY_JIT_SUPPORTED
