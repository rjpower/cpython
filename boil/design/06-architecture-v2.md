# 06: Architecture v2: a fixed-width register IR from the compiler, one value type, a tracing GC

Status: approved 2026-10-25 with two review amendments (sections 3.2 and 3.4, step 10). Supersedes the encoding in note 01,
the ownership bits of note 02, and note 03 (deferred reference counting),
which is dropped. Notes 04 (shapes) and 05 (tier 0) stand, with the changes in
sections 3.5 and 5.

## 0. What Phase 1 got wrong, and the correction

Phase 1 was built under one constraint: keep the stack bytecode, the
`PyObject*` ABI and reference counting, and bolt the new pieces on beside
them. Each of the three complaints about the result traces back to that
constraint.

**The variable-size encoding.** The IR is lowered from stack bytecode at the
first call, so it inherited the bytecode's shape: an `oparg` that can be
anything, `EXTENDED_ARG`, a feedback entry at a distance. A second word and an
`IR_EXT` prefix were the cheapest way to carry those through. They are not
needed once the compiler emits the IR itself. Section 2 fixes the word at 64
bits with four 12-bit fields and no prefix.

**16-bit slot fields.** In a register VM the operand field names a frame slot in
memory. The register file is the frame; there is no second storage tier to
spill to, and spilling is a machine-register concept that belongs to tier 1,
which allocates machine registers over these slots with liveness it computes
itself. The field width only bounds frame size. Lua 5 and LuaJIT use 8 bits
and reject functions that need more; Dalvik uses 4, 8 or 16 with variable-width
instructions; Ignition uses 8 with `Wide` and `ExtraWide` prefixes, which is
what Phase 1 copied. The right question is how wide a fixed field has to be.
Measured over every code object in `Lib/` (99,269 of them): the largest frame
needs 803 slots, the largest constant table 4,048 entries, the most feedback
sites 1,180, the longest jump 5,458 words. Twelve bits covers all of it with
five times headroom and makes the word fixed.

**Two value representations.** `_PyStackRef` exists because frames had to
carry something `PyObject*` cannot: a bit saying whether this slot owns a
reference count. The inline ints and floats were added to that word because it
was the only word we controlled. So we had NaN boxing in frames, raw pointers
in the heap, boxing at every container store, and ownership ceremony
(`IR_BORROW`, flush before escaping calls, `IR_DROP_BORROW`, dead-temporary
rules) threaded through the generator. All of it is reference counting showing
through. Remove reference counting and every frame slot, container cell and
attribute slot holds the same 64-bit `PyValue`; a store is a store and a read is
a read. Section 3 does that, with a non-moving tracing collector that scans
frames precisely and C stacks conservatively, so that existing C code keeps
working without a handle API.

The result is a design that follows Lua, V8 and JavaScriptCore where they
agree, and departs from CPython where CPython's choices were forced by its ABI:

| | Phase 1 | v2 |
|---|---|---|
| IR produced by | lowering at first call | the compiler, stored in the pyc |
| Instruction word | 64 or 128 bits, `IR_EXT` prefix | 64 bits, fixed |
| Fields | 3 × 16 bits, borrow bit in bit 0 | 4 × 12 bits, no flags |
| `LOAD_FAST`, `STORE_FAST`, `COPY`, `SWAP`, `POP_TOP`, `TO_BOOL` | dispatched, 40 to 45 percent of all dispatches | not instructions; operands name locals directly |
| Bytecode kept beside the IR | yes, with a pcmap | no |
| Value type | `_PyStackRef` in frames, `PyObject*` in the heap | `PyValue` everywhere |
| Ownership bit | yes | none |
| Memory management | reference counts plus cycle GC | non-moving tracing GC, mimalloc as allocator |
| Stack pointer in frames | yes | no; frame size is static |
| Argument passing | copy into the callee frame | callee frame overlaps the caller's argument window |
| C API | `PyObject*`, handle API planned | `PyObject*` stays a heap pointer; `PyValue` for values; no handle API |

## 1. The pipeline

Today's pipeline (Phase 1, PR #1), in one line each: the compiler emits stack
bytecode; at the first call the bytecode is lowered to a register IR with a
pcmap back to bytecode offsets; the IR interpreter specialises in place and
writes typed feedback; instrumentation rewrites the bytecode and is mirrored
into the IR; an optional tier 0 compiles IR blocks to call-threaded machine
code. Two forms of every function exist, and three subsystems (pcmap,
absorption, `IR_REMAP`) exist only to keep them in step.

The v2 pipeline has one form:

```mermaid
flowchart LR
    src["source"] --> ast["AST"] --> cfg["control-flow graph<br/>(flowgraph.c, unchanged)"]
    cfg --> ra["slot assignment and folding<br/>(new: Python/regalloc.c)"]
    ra --> ir["code object: IR words,<br/>line table, exception table"]
    ir --> |first call: memcpy| exec["executing copy + feedback vector"]
    exec --> interp["interpreter<br/>handlers generated from bytecodes.c"]
    interp <--> |specialise, record| exec
    exec --> |hot| t0["tier 0: template JIT"]
    t0 --> |hotter| t1["tier 1: SSA, speculation, register allocation"]
    t1 -.-> |deopt: slots are the frame state| interp
    mon["sys.monitoring"] --> |handler column| exec
```

- The pyc holds the IR. `dis` shows the IR. There is no stack bytecode.
- Specialisation and instrumentation write the executing copy. The pristine
  words in the code object are the deoptimisation and de-instrumentation
  source. (The free-threaded build's thread-local bytecode is this same
  mechanism, one copy per thread.)
- The feedback vector is allocated with the executing copy, so code that never
  runs costs nothing beyond its words.
- Every tier reads the same feedback vector. Tier 1 deoptimises by writing its
  live machine registers to their frame slots and jumping into the interpreter
  at a word index. Because the IR's registers are the frame slots, the frame is
  the deoptimisation state and no side table is needed.

## 2. The IR

### 2.1 Word

```
 63      52 51      40 39      28 27      16 15       0
+----------+----------+----------+----------+----------+
|    D     |    C     |    B     |    A     |  handler |
+----------+----------+----------+----------+----------+
```

Every instruction is one word. `handler` is a 16-bit index into the dispatch
table (246 handlers today, including 21 instrumented ones; folding `BINARY_OP`'s
operation into the handler adds about 30). A, B, C and D are 12-bit fields
whose meaning is fixed per handler class:

| Class | A | B | C | D | Examples |
|---|---|---|---|---|---|
| 3-slot, feedback | dst | src1 | src2 | fb | `ADD`, `SUBSCR`, `COMPARE_LT`, `STORE_SUBSCR` (A is the container) |
| 2-slot, feedback | dst | src | fb | 0 | `LOAD_ATTR`, `STORE_ATTR` (A is the object), `UNPACK_SEQUENCE` (C is count, D fb), `GET_ITER` |
| 1-slot, feedback, immediate | dst | fb | imm24 | | `LOAD_GLOBAL` |
| immediate | dst | imm24 | | | `LOAD_CONST`, `MAKE_FUNCTION`, `LOAD_DEREF` (B is the cell slot) |
| jump | cond slot | target24 | | | `JUMP`, `JUMP_IF_FALSE`, `JUMP_IF_NONE`, `JUMP_BACKWARD` |
| loop | dst | iter | fb | target12 | `FOR_ITER`, `SEND` |
| call | dst | callee | argc | fb | `CALL`, `CALL_KW` (kwnames in slot callee+argc+2) |
| window | dst | base | count | | `BUILD_LIST`, `BUILD_TUPLE`, `CALL_FUNCTION_EX` |

Rules:

- A slot field names a frame slot, 0 to 4094. Locals, cells and free variables
  come first, temporaries after. A code object that needs more than 4095 slots
  is a compile error, as a function with more than 255 registers is in Lua.
  The stdlib's largest need is 803 and the compiler already bounds literal and
  call stack use at 30 elements.
- A feedback field names an entry in the code object's feedback vector; index
  0 is a shared entry that never specialises, used by sites past the 4,095th.
- Static operands of feedback instructions, such as the attribute name of
  `LOAD_ATTR` or the global's name, live in the feedback entry, which the
  specialised handler reads anyway for its guard. The word carries only
  dynamic operands and the entry index.
- `imm24` is C and D together. A jump target is an absolute word index, 24
  bits. `FOR_ITER`'s 12-bit target is a forward distance; the assembler
  relaxes a loop whose exit is further than 2,047 words by jumping to a
  trampoline `JUMP`, as any assembler does. No stdlib loop needs it.
- A handler writes its dst last, after the operation cannot fail, so `z = f(z)`
  can target `z` directly.
- There is no `EXTENDED_ARG`, no second word, no prefix and no flag bit.

Decoding is one 64-bit load, a `movzx` for the handler, and a shift and mask
per field used. Phase 1's handlers read their 16-bit fields from memory
because a single-pass lowering could not keep them in registers; here the
word is in a register and field extraction is two ALU operations each.

### 2.2 What the compiler folds away

With slots as operands, the stack machine's data-movement instructions have
no counterpart:

| Stack bytecode | v2 |
|---|---|
| `LOAD_FAST x` | the consumer's operand is `x`'s slot |
| `STORE_FAST z` | the producer's dst is `z`'s slot |
| `COPY n`, `SWAP n` | the operands name the slots directly |
| `POP_TOP` | nothing (the temporary is simply dead) |
| `TO_BOOL` + `POP_JUMP_IF_*` | `JUMP_IF_*` with its own feedback; the bool fast path is in the jump |
| `UNARY_NOT` + `POP_JUMP_IF_*` | the inverted jump |
| `RESUME` | the eval-breaker and instrumentation-version check moves into the call and resume paths; `JUMP_BACKWARD` keeps its check |
| `LOAD_FAST_BORROW`, the pair forms, `IR_BORROW`, absorbed loads | gone; there is nothing to borrow |

Measured with the Py_STATS build on the executing Phase 1 interpreter, these
instructions are 45 percent of richards' dispatches (28.6M of 63.3M) and 42
percent of go's (75.3M of 179.2M), before counting `RESUME_CHECK` (4.6 and
3.1 percent). The handler bodies that remain are the ones that do work:
attribute loads, calls, compares, jumps.

Two examples. First the one from the PR description:

```python
def g(o):
    return o.x + o.y
```

```
slot 0 = o, temporaries from 1
0  LOAD_ATTR  r1, r0, fb0        ; r1 = o.x, name in fb0
1  LOAD_ATTR  r2, r0, fb1        ; r2 = o.y
2  ADD        r1, r1, r2, fb2
3  RETURN     r1
```

Four words, 32 bytes, four dispatches. The Phase 1 IR took six words and five
dispatches plus `RESUME_CHECK`; the stack bytecode eight instructions.

Second, a numeric loop body:

```python
for i in range(n):
    total += a[i] * b[i]
```

```
4  FOR_ITER   r_i, r_it, fb, +6
5  SUBSCR     r5, r_a, r_i, fb
6  SUBSCR     r6, r_b, r_i, fb
7  MUL        r5, r5, r6, fb
8  ADD        r_total, r_total, r5, fb
9  JUMP_BACKWARD 4
```

Six dispatches per iteration against thirteen in the stack bytecode.

### 2.3 Slot assignment

`Python/flowgraph.c` already knows the stack depth at every instruction and
already runs peephole passes over basic blocks. A new pass at assembly time
maps each stack position to a temporary slot (depth d becomes slot
`nlocalsplus + d`, exactly as the Phase 1 lowering did), then folds:

1. A `LOAD_FAST x` whose value is consumed by the next instruction in the
   block becomes that instruction's operand.
2. A producer followed by `STORE_FAST z`, where the temporary has no other
   reader, writes `z` directly.
3. `COPY` and `SWAP` are resolved by renaming.
4. `TO_BOOL` and `UNARY_NOT` immediately before a conditional jump fold into
   the jump.

Lua's compiler assigns registers with the same stack discipline (`freereg`),
so this is the standard approach for a register VM, not a shortcut. Temporaries
beyond a block boundary keep their stack-position slot, which makes jump
targets agree on slot contents without a dataflow pass. Tier 1 computes real
liveness over the finished IR.

Dead temporaries keep stale values until overwritten, which can retain
garbage, as Ignition's register file does. The compiler emits a `CLEAR` for a
temporary that held the only reference to a large value only where it can tell
(end of a `with` body, loop exit of a `FOR_ITER` over a materialised
sequence); otherwise we accept the retention.

### 2.4 Code object

| Field | Contents |
|---|---|
| `co_ir` | the words, immutable, from the pyc |
| `co_exec` | executing copy, allocated at first call; handler column rewritten by specialisation and instrumentation |
| `co_feedback` | the feedback vector, allocated with `co_exec`; entry kinds as in note 01 |
| `co_nslots`, `co_nlocalsplus` | frame size, static |
| `co_linetable` | as today, keyed by word index |
| `co_exceptiontable` | ranges over word indices; handler word, slot for the exception, slot for `lasti` or none; no depth |
| `co_consts`, `co_names`, `co_localsplusnames` | unchanged |

`co_code` returns the words as bytes. `dis`, `co_positions`, tracebacks and
`f_lasti` work in word indices. The pyc magic number changes once.

Instrumentation sets the handler of a word in `co_exec` to the instrumented
handler and restores it from `co_ir`'s handler plus the specialisation state
when removed. `INSTRUCTION` events fire per word; there are no absorbed loads
to re-lower. `LINE` events use the line table.

### 2.5 Feedback

The feedback vector is the one from note 01 (kinds COUNTER, BRANCH, UNARY,
BINARY, ATTR, GLOBAL, CALL; 16 to 40 bytes; up to four shapes before
megamorphic), with two additions: an entry carries its static operand
(section 2.1), and CALL entries record the callee's function version so that
tier 1 can inline. The interpreter, tier 0 and tier 1 read the same entries.
`specialize.c` remains the writer.

### 2.6 What the JIT gets

- Fixed-width words: a template per handler, one `mov`/`lea` per field.
- Registers are frame slots, so deoptimisation metadata is "which machine
  register holds which slot", and the interpreter resumes at any word.
- Basic blocks are implicit in the jump targets; SSA construction over 4,095
  virtual registers with explicit defs is textbook.
- Compare and branch fuse in the JIT by peeking at the next word, so the
  interpreter needs no superinstructions.
- No reference counting: a move is a move, a guard failure needs no
  cleanup, and values can stay in machine registers across instructions.

## 3. The object model

### 3.1 One value type

`PyValue` is a 64-bit word with the Phase 1 encoding minus the ownership bit:

```
Top 16 bits     Low 48 bits          Meaning
0x0000          0                    NULL: absent, unbound, error with exception set
0x0000          pointer              heap object; low 4 bits zero
0x0001          int48                internal index, never visible to Python
0x0002-0x7FF2   double + 2^49        float, +0.0 to +inf
0x8002-0xFFF2   double + 2^49        float, -0.0 to -inf
0xFFFE          0 then int32         int
0xFFFF                               reserved
```

`None`, `True` and `False` stay pointers to static objects, so that "is it a
heap object" has one answer and `Py_TYPE` has three cases. Inline ints stay
int32 so that the overflow check is the processor's flag; a 48-bit inline int
is possible in this encoding and is noted as a measured option, not taken.
NaN results are boxed.

`PyValue` is the type of a frame slot, a container cell, an instance slot, a
dict key or value, a function argument and a C API value. `_PyStackRef`,
`Py_TAG_REFCNT`, `PyStackRef_CLOSE`, `PyStackRef_DUP`, `PyStackRef_Borrow`
and the generator's `inline_ok` and borrow annotations are deleted.

`PyObject*` remains a pointer to a heap object. It is what C code uses after it
has established that a value is not inline. Converting a `PyValue` to a
`PyObject*` boxes an inline int or float into a fresh heap object; since there
are no reference counts, boxing has no ownership consequence.

### 3.2 Heap object header

```c
typedef struct {
    uintptr_t ob_header;   /* type pointer in bits 0-47, GC and flag bits in 48-63 */
} PyObject;
```

Eight bytes instead of sixteen. `Py_TYPE(o)` is one `and`. The sixteen flag
bits hold: mark, old (sticky mark for a later generational mode), finalizer
queued, has weak references, static (not heap allocated), owns external
resources, and spare. Only the collector writes them, with the world stopped.

The hidden-class layout is already the instance layout of section 3.3:
header, shape, slots. The open choice is whether the shape should move into
the header word in place of the type, as V8's map does, with the type reached
through the shape. That saves two loads on every `LOAD_ATTR` and `STORE_ATTR`
and costs one on every `Py_TYPE` in the runtime's C code. The right time to
decide is step 6c, when shapes are on by default and we can count both: if
attribute guards outnumber `Py_TYPE` loads in the C runtime on the benchmark
set, the header becomes the shape. Until then the type stays in the header,
and every type has a root shape so that the two layouts are
interchangeable.

### 3.3 Instances

An instance is `header, shape, slots...`. The shape (note 04) is owned by the
type and describes slot order and kinds. Attribute access guards the type
version and the shape pointer, then loads the slot. Slot kinds BOXED, I32 and
F64 exist from the first version; a F64 slot holds a raw double and
`LOAD_ATTR_SHAPE` produces an inline float from it without a tag test. The
shape's property table records the kind; a store of a non-float into an F64
slot transitions the shape. Dictionary mode (a materialised `__dict__`) is a
shape whose single slot points at the dict. `__slots__` classes are shapes
that cannot transition. Weak references live in a side table keyed by
address, with the header flag saying whether to consult it, so the
pre-header goes away.

### 3.4 Memory management

Reference counting is removed. `Py_INCREF`, `Py_DECREF`, `Py_CLEAR`,
`Py_SETREF` and friends compile to nothing; `Py_REFCNT` returns a large
constant so that code which tests `Py_REFCNT(x) == 1` to mutate in place takes
its safe path.

The collector:

- **Allocator:** mimalloc, already in the tree and already used by the
  free-threaded collector to enumerate objects. Size-class pages give a sweep
  by heap visit and an "is this address an object" test for conservative
  scanning.
- **Algorithm:** non-moving mark-sweep, stop-the-world, triggered by bytes
  allocated since the last collection. Generational collection by sticky mark
  bits with a write barrier is a measured follow-up (section 6, step 7), not
  part of the first version, because a missing barrier is a silent heap
  corruption and the barrier audit needs a working collector to verify
  against.
- **Precise roots:** interpreter frames (every slot, plus the frame header's
  references), thread and interpreter state, module and type objects
  (static types are registered at `PyType_Ready`), the finaliser queue.
- **Conservative roots:** each thread's C stack and spilled registers, and
  the writable data segments of every loaded shared object (via
  `dl_iterate_phdr`, as Boehm does). A word is a root if mimalloc says it
  lies inside an allocated block; interior pointers mark their block. This is
  what lets `Objects/`, `Modules/` and every port keep holding raw `PyObject*`
  in C locals and static variables without a handle API.

  Conservative scanning is the first pass, not the end state. A precise
  moving collector (nursery bump allocation, compaction) is the target once
  the ports are green on the tracing collector, and three things in this
  design keep that door open: the collector's root set is an abstraction
  (`gc_roots.c`) that the conservative scanner implements and a precise one
  will replace; new C API functions take and return `PyValue` and are
  written so that the handle discipline can be added at the signature
  without changing callers' logic; and the ports tree records, per port,
  which C code holds heap references across allocations, which is exactly
  the inventory the handle migration will need. The moving collector and
  its handle API are step 10, after tier 1.
- **Traversal:** `tp_traverse` for every type that holds references, not only
  GC-tracked containers. Step 4 of the plan builds a verifier that finds the
  gaps while reference counting still protects us.
- **Finalisation:** `__del__` and weak reference callbacks run from a queue at
  safepoints; an object with a finaliser survives the collection that finds it
  unreachable and dies in the next. `tp_dealloc` is called during sweep only
  for types flagged as owning external resources (files, sockets, buffers,
  locks, mmap, numpy data), and may not touch other heap objects. For every
  other type the body of `tp_dealloc` was releasing references and freeing
  memory, which the collector now does.
- **Observable changes:** `sys.getrefcount` is removed; `__del__` timing is
  not deterministic (PyPy's semantics); `gc.collect` works; `id` is stable
  because nothing moves. Correct code uses `with` and `close`, as thread #15
  settled. The stdlib audit (`tempfile`, `subprocess`, tests that assert
  refcount behaviour) is part of step 5.
- **Free threading:** the first collector is single-threaded. The
  free-threaded build is dropped at step 5, as the plan already stated for
  the encoding flip. A parallel marker is the natural later addition; the
  design keeps mark state in the header so that it can be done with atomic
  bit sets.

Expected effect: the refcount traffic in handlers and in every C function
disappears (the literature puts it at 10 to 30 percent of cycles; in richards the
Py_STATS build counts 25M increments and decrements of mortal objects over
63M instructions, and 12M more on immortal objects that still test the flag), the
per-object header halves, and resident size rises by the collector's
headroom (expect 1.3 to 2 times live data). We measure RSS with every
benchmark from step 5 on.

### 3.5 C API consequences

The handle-scope API of plan idea 7 and thread #16 was designed for a moving
collector. With a non-moving collector and conservative stacks it buys
nothing and would cost a rewrite of every port. It is dropped. What a port
sees instead, by step:

- **After the collector (step 5):** `Py_INCREF` and `Py_DECREF` are no-ops;
  code that reads `ob_refcnt` directly or relies on `tp_dealloc` ordering
  breaks; a `PyObject*` hidden in `malloc`ed memory without `tp_traverse` is
  found by the verifier and fixed by adding a traverse or allocating from
  the GC heap. numpy and Cython should rebuild with small patches.
- **After `PyValue` (step 6):** `PyList_GET_ITEM`, `PyTuple_GET_ITEM` and the
  like become functions that box an inline value on the way out; direct
  access to `ob_item` and to `PyDictValues` breaks and is patched in the
  Cython port, from which pandas, scikit-learn and the rest are regenerated.
  Functions that take `PyObject*` keep taking it. New API takes and returns
  `PyValue`.

The ports tree stays the compatibility test. The number of hand-written patch
lines per port after the Cython port is the metric for the API, as before.

## 4. The interpreter

- **Dispatch:** the tail-call interpreter (clang) with the computed-goto
  build as fallback. The handler signature passes the frame's slot base, the
  word pointer and the thread state in registers. Per instruction: one word
  load, one `movzx`, one table load and jump, two ALU operations per field
  used.
- **No stack pointer.** Frame size is `co_nslots`, fixed. The `stackpointer`
  field, its maintenance in every handler, `_PyFrame_StackPush` and the
  stack-depth accounting in the unwinder are deleted. The collector scans
  `co_nslots` words.
- **Frames.** The frame header (`code, function, globals, builtins, previous,
  pc, flags, owner`) lives on a frame stack; slots live on a separate
  contiguous value stack per thread, `frame->base` pointing into it. The
  generator's frame keeps its slots in the generator object, as today.
- **Calls.** The caller evaluates the callee into slot `c` and the arguments
  into `c+2 .. c+1+argc`. For a Python callee taking exactly those positional
  arguments, the new frame's base is the caller's `base + c + 2` and the
  arguments are already in place: no copy, which is Lua's convention and the
  largest remaining cost in `CALL_PY_EXACT_ARGS`. Keyword and default
  arguments take the copying path. If the value stack chunk lacks room for
  the callee's slots, the arguments are copied to a new chunk.
- **Return** writes the result into the caller's dst slot and pops both
  stacks. Exceptions unwind by table lookup on the word index; nothing is
  decremented.
- **Generators** store the resuming word index in the frame; `YIELD_VALUE`
  writes into the caller's dst slot. The `gi_yield_offset` byte goes.
- **Specialisation** rewrites the handler field with a 16-bit store. The
  family head for deoptimisation is a table lookup on the handler index.
- **Inline values in registers.** With no stack pointer and no counts, a
  handler holds its operands in registers from load to store; the tail-call
  build keeps the slot base, word pointer and thread state in callee-saved
  registers across handlers.

Phase 1's measurements said the interpreter was never where the gains were.
That stays true. The interpreter changes here remove about 45 percent of
dispatches and all refcount work from the rest; an estimate of 15 to 25
percent on the object-heavy benchmarks is reasonable, and we will know at
step 2 and step 5. The point remains that the IR, the frames and the value
type are what tier 0 and tier 1 need.

## 5. Tiers 0 and 1

Note 05's tier 0 was slower in wall time because every value round-tripped
through memory, every handler was called, and reference counts had to be
right at every exit. Over the v2 IR, tier 0 becomes what Sparkplug is: one
template per handler with the fields baked in as immediates, inline fast
paths for `LOAD_ATTR_SHAPE`, `STORE_ATTR_SHAPE`, `LOAD_GLOBAL_*`, int and
float arithmetic and compares with the branch fused, and calls into the
shared handler bodies for the rest. The slot base lives in a register.
Deoptimisation is a jump into the interpreter at the word index, because the
slots already hold the state.

Tier 1 is a method JIT over SSA built from the IR, driven by the feedback
vector: type and shape speculation with guards, inlining of monomorphic
callees using their own feedback, unboxed int and float values in machine
registers across instructions, linear-scan allocation, and a deoptimiser
that writes registers back to slots. The target is V8's Maglev in size and
ambition. Both tiers use the same small x86-64 assembler from note 05, then
arm64.

## 6. Plan

Each step is owned by one Opus 5.5 agent with a brief that carries a clock
deadline, a test ladder (one tiny end-to-end case, then the targeted test
files, then the full suite, then ports, then measurement), a report every 30
minutes or per rung, a "message me if blocked more than 15 minutes" rule and
an explicit scope cut. I validate each gate myself: full suite in debug and
release, `perf stat -e instructions:u` on the four reference benchmarks
(richards, go, nbody, float), the ports build, and RSS where memory changes.
Each step lands as commits on `weaver/python-boil-the-ocean` with a worklog
entry; nothing lives on a prototype branch.

| # | Step | Deletes | Gate | Budget |
|---|---|---|---|---|
| 1 | Fixed-width encoding and handlers. `pycore_ir.h` gets the 64-bit word with four 12-bit fields; `ir_generator.py` emits handlers that extract fields from the register; `BINARY_OP` becomes per-operation handlers; static operands move into feedback entries; jumps take 24-bit absolute targets. The Phase 1 lowering is adapted to emit the new words, so that the full suite validates the encoding before the compiler changes. | word 1, `IR_EXT`, `IR_SUPER_COMPARE_*`, the `imm`/`fb` location logic in the metadata generator, `_PyIRFormat.nwords` | full suite debug and release; instructions per loop on the four benchmarks within 1 percent of Phase 1 | 2 days |
| 2a | The compiler emits the IR. `assemble.c` writes words, line table and exception table keyed by word index; `regalloc.c` does slot assignment (section 2.3, folding off); code object fields of section 2.4; `co_exec` copy at first call; marshal and pyc magic; `dis` rewritten for the IR; instrumentation on the handler column. | `ir_lower.c`, pcmap, `IR_LOWER`, `IR_REMAP`, `_PyIR_FUSED`, `co_code_adaptive`, `_PyIR_SyncInstrumentation`, `EXTENDED_ARG`, the stack-depth metadata the lowering used | full suite (test_dis, test_compile, test_peepholer, test_code, test_sys_settrace, test_monitoring updated to the IR); ports; lowering cost gone from startup (richards `-l 1` instructions) | 5 days |
| 2b | Folding (section 2.3): `LOAD_FAST`, `STORE_FAST`, `COPY`, `SWAP`, `POP_TOP`, `TO_BOOL` and `UNARY_NOT` into jumps, `RESUME` into call and resume paths. | the opcodes above and their specialisations, `RESUME_CHECK` | Py_STATS dispatch count on richards and go down at least 35 percent; instructions per loop down on all four; suite; ports | 3 days |
| 3 | Frames and calls (section 4): no stack pointer, frame header on its own stack, value stack with overlapping argument windows, `RETURN`/`YIELD` writing the caller's dst, state in registers in the tail-call build. | `stackpointer`, `_PyFrame_StackPush/Pop`, `return_offset`, `gi_yield_offset`, the argument copy in `CALL_PY_EXACT_ARGS` | call micro-benchmark cycles per call below Phase 1's 105.7; suite; ports | 3 days |
| 4 | Collector, verifier mode. `Python/gc_trace.c`: mark from precise roots (frames, thread and interpreter state, registered static types) and conservative roots (C stacks, data segments) via mimalloc's block lookup; `tp_traverse` audit tool that reports every type with pointer members and no traverse; at every `gc.collect` in a debug build, report any object with a nonzero count that the mark did not reach. Reference counting still owns memory. | nothing yet | zero unreached-but-counted objects across the full suite and the seven ports with the verifier on | 4 days |
| 5 | Collector, live. `--with-gc` configure flag first, default later in the step. Eight-byte header; `Py_INCREF`/`Py_DECREF` to no-ops; sweep by heap visit; finaliser queue; weak reference side table; `tp_dealloc` audit and `OWNS_RESOURCES` flag; `sys.getrefcount` removed; stdlib audit for refcount timing; free-threaded build removed. Sub-gates: 5a flag build runs the smoke ladder; 5b full suite; 5c ports rebuild with patch counts recorded; 5d default on, refcount paths deleted. | `ob_refcnt`, `Py_TAG_REFCNT`, `_PyStackRef` ownership, `IR_BORROW` remnants, the cycle collector (`gc.c`), free lists, the free-threaded build and `gc_free_threading.c`, `Py_STACKREF_DEBUG` | suite; ports; instructions per loop down on richards and go; pyperformance geomean and RSS recorded and compared with Phase 1 | 10 days, in four sub-steps |
| 6a | `PyValue` in the core: list, tuple, dict, set, instance slots, cells, globals hold `PyValue`; `_PyStackRef` renamed to `PyValue` and the duplicate helpers deleted; boxing only at the `PyObject*` conversion. | `_PyStackRef`, `PyStackRef_*`, `Py_STACKREF_INLINE_VALUES`, the boxing at container stores | suite; nbody, float, spectral_norm instructions per loop; RSS | 5 days |
| 6b | `PyValue` at the C API boundary and in the ports: item accessors become boxing functions; Cython port patched and the Cython-generated ports regenerated; numpy; patch counts recorded. | `PyList_GET_ITEM` and family as macros | all seven ports green; hand-written patch lines per port recorded | 5 days |
| 6c | Shapes default on, with I32 and F64 slot kinds live, `LOAD_ATTR_SHAPE_POLY`, method loads through shapes. | `--with-shapes`, shared keys for instances, the `PyDictValues` header | suite; ports; richards, raytrace, deltablue, nbody per loop | 4 days |
| 7 | Generational collection: sticky mark bits, remembered set, write barrier in the store macros, barrier verifier (full mark after each minor collection in debug builds). Kept only if pause and throughput both improve on the benchmark set. | nothing | verifier clean; pyperformance and RSS compared with step 6 | 5 days |
| 8 | Tier 0 over the v2 IR (section 5), x86-64. | the Phase 1 `tier0.c` and `ceval_tier0.c.h` | geomean below the deleted upstream JIT's 0.834 against the Phase 0 baseline; suite with tier 0 on | 8 days |
| 9 | Tier 1. | | 2x geomean over Phase 0 on pyperformance, 3x on the numeric subset; ports green | open |
| 10 | Precise moving collector: nursery with bump allocation and evacuation, compaction of the old space, handle discipline in the C API for references held across allocations, ports migrated with the inventory from step 5. | the conservative scanner | ports green; allocation-heavy benchmarks and RSS compared with step 7 | open |

Dependencies: 1 before 2a before 2b. Step 3 and step 4 are independent of
each other and of 2b, and run in parallel once 2a has landed. Step 5 needs 4
and benefits from 3. Step 6a needs 5 (no ownership means `PyValue` is a
rename plus container changes); 6b needs 6a; 6c needs 6a. Steps 7, 8 and 9
follow 6. A Sonnet agent keeps the ports tree rebuilding after every step
that changes the ABI (3, 5, 6a, 6b, 6c).

Why this order. The IR work comes first because it is independent of the
object model, is where the current design is weakest, and because every
later step writes handlers in the generator, which should be in its final
form. Step 1 is kept separate from 2a so that the encoding is validated with
the existing full suite before the compiler, `dis`, marshal and their tests
change in one move. Step 4 exists so that the first time the collector decides
an object is dead, we have already shown over the whole suite and every port
that it finds everything reference counting kept alive. Step 5 comes before
`PyValue` because removing ownership is what makes one value type a rename
rather than a redesign. Note 03's deferred reference counting is skipped: its
only lasting part, root enumeration, is step 4.

## 7. Decisions needed

1. **Non-moving collector with conservative C stacks first, precise moving
   collector later (step 10).** Decided in review: conservative scanning is
   acceptable as a first pass; the design keeps the root set abstract and the
   new API `PyValue`-typed so that the moving collector and its handle
   discipline arrive as a step, not a rewrite.
2. **Hard limits: 4,095 slots and 4,095 feedback sites per code object,
   16M words, 24-bit constant index.** Compile error beyond the slot limit;
   no specialisation beyond the feedback limit. Measured headroom is five
   times over the stdlib.
3. **Inline ints stay int32.** int48 is available in the encoding; take it
   only if a benchmark shows it.
4. **The free-threaded build ends at step 5.** A parallel marker is the
   later path to multi-core, not a concurrent reference-counting scheme.
5. **Skip deferred reference counting (note 03) entirely.** Its root scan is
   step 4; the rest would be deleted at step 5.
6. **Generational collection is step 7, not part of step 5**, so that the
   first collector has no write barrier to get wrong.

## 8. Sources

Lua 5 and LuaJIT (fixed 32-bit words, stack-discipline register allocation,
overlapping call frames, NaN boxing); V8 (Ignition's register file and
feedback vectors, Sparkplug, Maglev, maps, sticky-mark-bit discussions in the
Orinoco notes); JavaScriptCore (NaN-boxing layout, Baseline and DFG tiers);
Boehm-Demers-Weiser (conservative stack and data-segment scanning, interior
pointers); Demers et al. 1990 (sticky mark bits for non-moving generational
collection); Ruby's non-moving GC and PyPy's finalisation semantics as
precedents for the observable changes.

## 9. Step 1 measurements

User-mode instructions per loop (`perf stat -e instructions:u`) of the
pyperformance worker for each benchmark, run at 2 and at 10 loops, with the
difference divided by 8. That excludes startup, imports and lowering. Repeat
runs agree to within 0.1 percent. Phase 1 is f655ef62814. Both computed-goto
builds use gcc 15.2 with `-O3`, and both tail-call builds use clang-21 with
`--with-tail-call-interp`. Ryzen 7 3800X.

gcc, computed goto (Phase 1 is `build/final-snapshot`):

| benchmark | Phase 1 | v0 | v1 | v1b | v1b / Phase 1 |
|---|---|---|---|---|---|
| richards | 562.3M | 618.3M | 583.0M | 575.9M | 1.024 |
| go | 1190.0M | 1356.5M | 1271.4M | 1244.9M | 1.046 |
| nbody | 1013.1M | 1127.8M | 1038.1M | 1030.3M | 1.017 |
| float | 763.6M | 825.0M | 789.8M | 779.7M | 1.021 |

The variants:

- v0 decodes each field from `ir_word` with a shift and a mask, as section
  2.1 planned.
- v1 reads each field from the word in memory, at `this_instr`, with a
  16-bit load and a shift (B, D) or a mask (A, C). CD is a 32-bit load and a
  shift.
- v1b is v1 with the fields as `uintptr_t`, which removes GCC's 16-bit
  arithmetic and zero extensions.
- Making the feedback stride 32 bytes rather than 40 was worse by 0.3
  percent on go and is not kept.
- With the compare-and-branch superinstructions disabled, Phase 1 runs go
  at 1199.5M (+0.8 percent) and the other three within 0.2 percent. Their
  deletion is therefore not the gap.

clang-21, tail call (Phase 1 built the same way):

| benchmark | Phase 1 | v1b | ratio |
|---|---|---|---|
| richards | 643.2M | 631.6M | 0.982 |
| go | 1418.5M | 1382.6M | 0.975 |
| nbody | 1489.8M | 1336.8M | 0.897 |
| float | 941.9M | 906.5M | 0.962 |

The tail-call build passes the 1 percent gate. The computed-goto build stays
within the 3 percent allowance for the fallback dispatch, except go at 4.6
percent.

**Why v0 lost.** Keeping `ir_word` live through the handler body changed
GCC's register allocation in `_PyEval_EvalFrameDefault`. In v0, STORE_FAST
and LOAD_ATTR_INSTANCE_VALUE keep `next_instr` in a stack slot: every
handler loads it, stores it back after `next_instr += 1`, and dispatch loads
it again. The base of `opcode_targets` is also spilled. Only `frame`,
`stack_pointer` and `ir_word` stay in registers, and each field costs a
copy, a shift and a mask. Phase 1 kept `next_instr` in a register, because
every field read used it as a base. In v1 the only value spilled is the
`opcode_targets` base, as in Phase 1. `next_instr` and `this_instr` live in
registers, and dispatch loads only the 16-bit handler. Section 2.1's claim
that a field costs two ALU operations from a register holds per field, but
not for the handler as a whole.

**What remains in v1b is the layout.** LOAD_ATTR_INSTANCE_VALUE runs about 8
more instructions than in Phase 1. The attribute-load micro loop of
`boil/tools/micro_loops.py` goes from 67 to 74, while the loop skeleton is 2
cheaper. The extra instructions:

- A and C need `& 0xfff`, because their 16-bit loads overlap the next field
  (+2).
- The borrow test compares the field with its canonical slot
  (`cmp`/`setne`/`movzbl`) instead of testing a flag bit (+2).
- The feedback entry is an index times 40 from `frame->ir_feedback`, where
  Phase 1 addressed it relative to `next_instr` (+1 to 2).
- The unscaled dst field leads GCC to build `&localsplus[A]` in a chain of
  `lea`s (+2).

Steps 2b (folding the data-movement handlers that dominate go's dispatch
count, and with them most borrow tests) and 3 address these.

**Tail call against computed goto.** In both trees the tail-call build runs
many more instructions than the computed-goto build. For v1b the ratio is
1.30 on nbody, 1.16 on float, 1.11 on go and 1.10 on richards; for Phase 1
it is 1.47, 1.23, 1.19 and 1.14. This is a step 3 item, the interpreter
state kept in registers across handlers, and not something to chase in
step 1.

| micro loop, instructions per iteration | Phase 1 | v0 | v1b |
|---|---|---|---|
| empty loop, i >= 1025 | 68.0 | 75.1 | 66.0 |
| int add, result not cached | 65.8 | 79.0 | 68.8 |
| float add | 75.1 | 87.9 | 77.9 |
| two STORE_FAST | 57.0 | 69.0 | 61.1 |
| attribute load | 67.0 | 85.1 | 74.0 |
| attribute store, existing | 116.9 | 121.1 | 119.2 |

## 10. Step 2a decisions

Step 2a moved the IR into the compiler. The stack bytecode, the lowering
(`Python/ir_lower.c`), its stub and pcmap, `IR_LOWER`, `IR_REMAP`,
`_PyIR_FUSED`, `co_code_adaptive` and the instrumentation sync are gone.
These are the choices made on the way, where they differ from or add to
sections 2.3 and 2.4.

**Pipeline.** `Python/regalloc.c` maps depth d to slot `co_nlocalsplus + d`,
in a function of its own so that step 2b can fold there.
`Python/assemble.c` chooses each handler, encodes the words, numbers the
feedback entries, relaxes FOR_ITER and SEND, and writes the line and
exception tables by word index. Nothing is folded, so `LOAD_FAST_BORROW`
keeps a word and every input field names its canonical slot. Handlers still
keep `stack_pointer`, and the exception table keeps its depth column. The
`IR_BORROWED` machinery for absorbed loads stays in the generator for step
2b.

**Code object.** The trailing array, `co_words`, holds the words followed by
the feedback table: one 32-bit entry per feedback site, `kind << 24 | arg`,
where arg is the static operand. Entry 0 is the shared entry. `co_code`
returns the words, and `co_fbinit` returns the table. `code()` takes the
table as a new last positional argument, `fbinit=b''`, and `replace()`
takes `co_fbinit`, defaulting to the code object's own table. Marshal
writes the table after `co_code`. The pyc magic is 3707.

**Checking.** The Python and C-API constructors check every word: the
handler exists, and the slot fields are below `co_nlocalsplus +
co_stacksize`. A feedback field must be below the number of entries, and
must not be 0 for a handler that reads its operand from the entry. A word with
an inline constant handler whose constant is no longer a compact int or a
float, as after `replace(co_consts=...)`, runs `LOAD_CONST` instead. Code
whose length is a whole number of 2-byte units but not of words is padded
with zero words (CACHE): Cython builds each function's code object from
zeroed code of that kind, which never runs and only spans its line table,
and the numpy and pyyaml ports fail to import without it. Marshal does not
check, just as it trusts bytecode today. Sites past the 4095th share
entry 0 and run a NOFB handler, which carries the operand in the word
(`LOAD_ATTR_NOFB`, `COMPARE_OP_NOFB` and so on).

**Executing copy.** `_PyCode_GetExec()` (Python/ir.c) allocates the words
and the feedback vector together on the first call and initialises the
entries from the table. It is called when a frame is pushed, by
`PyFrame_New`, by the specialiser when it inspects a callee, and when
instrumentation is forced. Frames point into the copy (`ir_words`,
`ir_feedback`). The assembler always emits the inline constant handlers
(`LOAD_SMALL_INT_INLINE`, `LOAD_CONST_INLINE_INT` and `_FLOAT`). The copy
runs the boxing forms instead in a build without inline values or under
`PYTHON_TAGGED_VALUES=0`, so the words do not depend on the build.

**Instrumentation.** Instrumentation rewrites handlers in the executing copy
and restores them from the code object's words. The pristine words replace
the `real[]` array of note 01: the handler to restore is that word's, or for
a NOFB site the NOFB form of the instrumented instruction. A restored word is
generic and warms up again. INSTRUCTION events fire once per word; the
`IR_JUMP` words described below have none.

**Offsets.** Word indices are used everywhere:

- `f_lasti`, `tb_lasti` and `co_lines()`, `co_positions()`, the line and
  exception tables;
- monitoring offsets, including the legacy branch events;
- `dis` offsets and lazy-import locations.

`co_lines()` ends at `len(co_code) // 8`. This is the visible API change of
the step.

**Relaxation.** FOR_ITER and SEND reach `IR_MAX_FORWARD` (2047) words. The
assembler relaxes a loop whose exit is further away into trampolines. A
relaxed FOR_ITER at w exits by distance 1 and is followed by three
`IR_JUMP` words: to the body at w + 4, to END_FOR (where a generator
returns), and past END_FOR. A relaxed SEND has two: to w + 3 and to
END_SEND. `IR_JUMP` does not save the instruction pointer, so the events
after it see the loop as the instruction before. `dis` shows these words as
`JUMP_FORWARD`, with `handler` `IR_JUMP`.

**Handler numbers.** The IR-only handlers now follow the pseudo-instructions
of the compiler. `IR_JUMP` is 267, not 256. As a result
`opcode.opname[n]` names one opcode, pseudo-instruction or handler for every
n.

**dis.**

- `Instruction` gains four fields:
  - `handler`, the name of the word's handler;
  - `dst`, the slot written;
  - `srcs`, the slots read, or the base of a window;
  - `fb`, the feedback index.
- `cache_info` lists the fields of the feedback entry. These come from the
  table, or with `adaptive=True` from the executing copy (state, counter,
  arg, first, second). `show_caches` prints them as a `FEEDBACK` line.
- Each line ends with a register column, for example `r1 <- r1, r2`.
- With `adaptive=True`, an IR-only generic handler such as
  `BINARY_OP_SUBTRACT` or `LOAD_ATTR_NOFB` shows as the instruction it
  runs. Specialisations show as themselves.
- `opcode` takes its tables from the generator (`_ir_formats` via
  `_opcode.get_ir_formats()`, and `_ir_opmap`). `_cache_format` and
  `_inline_cache_entries` are deleted.
- Limit: disassembling bare `co_code` bytes cannot show an operand that
  lives in the feedback table, such as COMPARE_OP's operator or a global's
  name, because those bytes do not include the table.

**Readers.** `_Py_DebugOffsets` drops the IR block and renames
`co_code_adaptive` to `co_words`, and it adds `interpreter_frame.ir_words`.
`_remote_debugging` computes the word index as `(instr_ptr - ir_words) / 8`
and reads the handler as the low 16 bits of the word. `Tools/gdb/libpython.py`
computes `f_lasti` the same way.

**Byte order.** A pyc stores the words in the order they have in memory,
which assumes a little-endian host as every supported 64-bit target is. A
big-endian port would swap them in marshal.

**Smaller changes.**

- Whether `GET_ITER` of a `yield from` must reject a coroutine depends on
  `CO_ITERABLE_COROUTINE`. `types.coroutine()` sets that flag on a copy of
  the code object, so the compiler cannot know it. The lowering used to
  resolve it; now `_PyEval_GetIter` reads the running code's flags.
- The compiler builds a function's annotations dict 15 pairs at a time
  (`BUILD_MAP` and `DICT_UPDATE`), so that a function with thousands of
  annotated parameters stays within the slot limit.
- `EXTENDED_ARG` is never emitted. The opcode stays defined because
  `opcode.EXTENDED_ARG` and `dis` document it.

**Measurements.** gcc with computed goto (dev-default), against step 1 and
against step 1 with absorption disabled (step1-noabsorb), which emits the
same words as 2a. Instructions per loop are the worker at 2 and 10 loops,
differenced, the median of four or five runs; each build has its own pyc
cache.

| per loop (M instructions) | step 1 | step1-noabsorb | 2a | 2a vs noabsorb | 2a vs step 1 |
|---|---|---|---|---|---|
| richards | 577.3 | 594.5 | 597.0 | +0.4% | +3.4% |
| go | 1244.1 | 1305.7 | 1312.7 | +0.5% | +5.5% |
| nbody | 1030.3 | 1030.3 | 1028.6 | -0.2% | -0.2% |
| float | 780.0 | 800.4 | 803.2 | +0.4% | +3.0% |

The gap to step 1 is the absorption that 2a does not do; step 2b's folding
is to win it back.

| one-time (M instructions) | step 1 | step1-noabsorb | 2a |
|---|---|---|---|
| `python -c pass` | 54.1 | 53.5 | 46.7 |
| richards `-l 1` | 995.0 | 1012.4 | 974.2 |
| richards `-l 1` less one loop | 417.7 | 417.9 | 377.2 |

Deleting the lowering saves about 40M instructions on a richards worker and
6.8M at startup.

Peak RSS is the median of five runs, measured two or three times.

| max RSS (KiB) | step 1 | step1-noabsorb | 2a | 2a vs noabsorb |
|---|---|---|---|---|
| `python -c pass` | 10448 | 11248 | 11384 | +1.2% |
| twelve stdlib imports | 19280 | 20204 | 21178 | +4.8% |
| richards `-l 10` | 22356 | 23464 | 24136 | +2.9% |

Imports and richards exceed the 2 percent allowance. The code objects carry
the cost: after the imports, their 3,758 code objects hold 842 KB of stack
bytecode in step 1 and 1,334 KB of words plus 257 KB of feedback tables in
2a. A loaded code object now holds 8 bytes per instruction where it held 2
bytes per code unit. A quarter of the words load a local or a constant;
2b's folding removes many of them.
