# 05: Tier 0, a baseline compiler of IR blocks to machine code

Status: implemented behind `--with-tier0` (off by default) on 2026-10-24 in
three commits: call-threaded code (`baa81aa4255`), inline fast paths
(`26f4080cf9d`), and tests with this note. `--with-tier0` requires
`--with-tail-call-interp`, an x86-64 host and the GIL. The measurements are
of release builds made with clang-21 on a Ryzen 7 3800X (Zen 2). They were
taken on `b36c99a7593` with `--with-ir-interp`, before the register IR
became the only interpreter.

## Decisions

1. **The unit of compilation is one IR block.** That is the lowered words of
   one code object, compiled whole into one mapping. A block compiles once
   its heat reaches 1024: each call adds 16 and each backward jump adds 1, so
   64 calls or 1024 back edges suffice. A block compiles at most four times.
2. **The block's code behaves as a tail-call handler.** It has the handler
   signature, so the interpreter enters it as the handler of the current
   word, and it leaves by tail call. Python calls and returns through tier 0
   do not grow the C stack.
3. **Instructions that do not run inline call a second copy of their
   handler.** This copy is compiled in *call mode*. The handler index is read from the word at run
   time, so specialisation and despecialisation need no recompilation.
4. **Common instructions run inline.** They guard their operands and fall
   back to the call when a guard fails.
5. **Instrumentation patches the code.** It overwrites the first five bytes of
   every instruction with a jump to the interpreter, so the fast path never
   tests whether the code is still valid.

## ABI

| Register | At the start of each instruction's code |
|---|---|
| r12 | the frame |
| r13 | the stack pointer, `&localsplus[slot]` |
| r14 | the thread state on entry to a handler; it is also at `[rbp-8]` |
| r15 | the address of the instruction's first word (`next_instr`) |
| edi, esi, edx | fields A, B and C of the word, on entry to a handler |
| rax | the frame that a handler returns |

**Entry.** The interpreter enters through the block's entry point with edi
holding the offset of the word's code. The prologue pushes rbp, the thread
state, the frame and the call table, which then sit at `[rbp-8]`, `[rbp-16]`
and `[rbp-24]`. It aligns rsp to 16 bytes and jumps to the offset.

**Handlers in call mode** are `preserve_none` and clobber every register
except rsp and rbp. Each handler stores `next_instr` and `stack_pointer` in
`_PyThreadStateImpl.tier0` and returns one of:
- the frame to go on with;
- NULL, when an error leaves the entry frame;
- the frame tagged with `_PyTier0_INTERPRET`, when only the interpreter can
  run the next word. These words are the instrumented and transparent
  handlers, `IR_LOWER`, `IR_REMAP` and `INTERPRETER_EXIT`.

Handlers in call mode set `frame->instr_ptr` as the interpreter's handlers
do. Returning to the machine code does not change it, because
`INSTRUMENTED_LINE` reads the previous instruction there. Inline code writes
it only just before a destructor can run.

## Code shape

A call takes about 72 bytes:

    mov r14, [rbp-8]; mov edi, A; mov esi, B; mov edx, C   (used fields only)
    movzx eax, word [r15]; mov rcx, [rbp-24]; call [rcx + rax*8]
    cmp rax, [rbp-16]; jne leave
    mov r12, rax; mov rcx, [rbp-8]; mov r15, [rcx+next]; mov r13, [rcx+sp]
    cmp r15, [rip + &words[w+n]]; jne redispatch

Inline `BINARY_OP_ADD_INT`, with its slow path out of line:

    mov rax, [r12 + slot(B)]; mov rcx, [r12 + slot(C)]
    mov rdx, rax; and rdx, rcx; cmp rdx, [rip + INT_BASE]; jb slow
    add eax, ecx; jo slow; or rax, [rip + INT_BASE]
    mov [r12 + slot(A)], rax; lea r13, [r12 + slot(A) + 8]; lea r15, [r15 + 16]

The two exits from a call work like this:
- `redispatch` handles another word of the same frame. It jumps there
  through the block's jump table.
- `leave` handles a different frame. If the returned frame has code for its
  next word, `leave` jumps into it; this covers calls into compiled functions
  and returns to compiled callers. Otherwise it tail-calls
  `_PyTier0_Continue()`, which goes on in the interpreter.

Inline code computes r13 from the word's fields rather than tracking a stack
depth. That makes it agree with handlers in both stack and register formats.

These instructions run inline:
- `LOAD_FAST`, `LOAD_FAST_BORROW`, `STORE_FAST` and their pairs, and
  `POP_TOP`;
- the constant loads;
- the jumps; `JUMP_BACKWARD` also tests the eval breaker;
- `POP_JUMP_IF_FALSE`, `POP_JUMP_IF_TRUE` and `TO_BOOL_BOOL` on exact bools,
  recording the branch history;
- add, subtract, multiply and compare on two int32s or two doubles;
- `FOR_ITER_RANGE`.

Reference counts change inline. When a count reaches zero, the code calls
`tier0_release()`. Builds with `Py_REF_DEBUG` call helpers in both
directions, so that the total reference count stays right.

## Deoptimisation

- **Guards.** A failed guard calls whatever handler the word names at that
  moment, and that handler may despecialise the word. The inline guard
  stays, so from then on the site costs a guard plus a call.
- **Instrumentation.** `_PyIR_SyncInstrumentation()` calls
  `_PyTier0_Invalidate()` before it rewrites a handler. Every instruction's
  code is padded to at least five bytes, and no label or return address
  falls inside those bytes. So a frame that runs the code, or returns into
  it, meets a `jmp` at its next instruction. The jump goes to the exit,
  which hands the word to the interpreter. The code stays mapped until the
  code object dies. After the instrumentation is removed the block can
  compile again, up to four compiles in all.
- **Lowering again.** INSTRUCTION events lower a fused block a second
  time, one word per instruction. The old block's words then run
  `IR_REMAP`, and its code is invalidated. Machine code is entered only at
  words of the code object's current block: both the frame switch and
  `_PyTier0_Continue()` check the bounds. A frame in the old block therefore
  moves over in the interpreter. `_PyIR_Clear()` frees the code of both
  blocks.
- **Eval breaker.** `JUMP_BACKWARD` tests it inline. Handlers test it for
  calls.
- **Exceptions.** Handlers in call mode jump to the exception labels in call
  mode. An exception caught in the same frame comes back at the except block
  through `redispatch`. An exception that leaves the entry frame comes back
  as NULL.

## Implementation decisions and limitations

1. **Calls go through a table.** The handler index is read from the word, so
   every call is `call [table + index*8]`. Each word has its own call site,
   and these predict well: tier 0 has fewer branch misses than the
   interpreter.
2. **State goes through the thread state.** Every call costs two stores and
   two loads, for `next_instr` and `stack_pointer`. One of the two stores is
   volatile, so the two stay separate. Without that, clang copies both from
   a new frame with a single 16-byte store, which the 8-byte loads cannot
   forward from, and each Python call between compiled frames meets two
   store-to-load interlocks.
3. **Every handler exists twice.** The interpreter and call mode are compiled
   from the same generated cases. See "Where the cycles go".
4. **Some blocks are never compiled.** These are blocks with an `IR_EXT`
   prefix, blocks of more than 65536 words, the static `_Py_InitCleanup`
   block and the lowering stub. An instrumented block waits until the
   instrumentation is removed.
5. **Each block has its own mapping, freed with the code object.** Code that
   a recompile replaced stays mapped until then, because frames may still be
   running it.
6. **No `Py_STATS` and no `Py_STACKREF_DEBUG`.** Both stop the build with an
   `#error`. Tier 0 does not pass `lastopcode`. With `Py_STACKREF_DEBUG` a
   stackref that is not an inline value holds a table ID where it would hold
   an address. The machine code uses such stackrefs as addresses: it
   changes reference counts, loads the iterator of `FOR_ITER_RANGE` and the
   frame's code object, and embeds constants' addresses. `IR_DROP_BORROW`
   changes how handlers drop borrows, not this layout, so the `#error`
   stays.
7. **Several instruction families never run inline.** `LOAD_ATTR_*`,
   `LOAD_GLOBAL_*`, the calls and the subscripts are always calls, and so
   will be any fused `COMPARE_OP` + `POP_JUMP_IF` handler that the IR adds.
   So is any instruction with an input marked `IR_BORROW`.
8. **Boxed operands take the slow path.** nbody keeps its floats in lists,
   so they are boxed, and each operation pays a guard and a call.
9. **The threshold lets startup code compile.** Each benchmark compiles
   about 49 blocks from imports and startup, against 1 to 30 blocks of the
   benchmark itself. A threshold of 256 calls, or of 4096 back edges,
   compiles the same benchmark blocks and cuts the startup blocks to about
   25.
10. **A shared call stub was tried and dropped.** The stub replaced each call
    sequence with one out-of-line stub per instruction length, reached by
    `call stub` with the fields in registers. Call words shrank from 72 to
    18 bytes, and blocks from 53 to 28 bytes per IR word. But each call ran
    five more instructions (call, ret, push, pop, lea), cycles rose by 9 to
    21%, and L1i misses did not change. Placing all code in one contiguous
    arena did not change the misses either.
11. **Tests.** `Lib/test/test_tier0.py` drives tier 0 through
    `_testinternalcapi.tier0_compile()` and `tier0_has_code()`. Under
    `-R 3:3` it reports 2 to 5 memory blocks per run, and `-R 6:6` is
    clean. Tier 0 does not cause this. Running the same test bodies in one
    process and counting `sys.getallocatedblocks()` per run gives
    169, 15, 3, 4, 2, 6, 0, 1, 0, ... on a debug build without tier 0, with
    the tier 0 hooks stubbed out. With tier 0 it gives 169, 14, 3, 4, 2, 4,
    0, 1, 0, .... Compiling, invalidating and recompiling one code object
    in a loop, or compiling and dropping fresh code objects, allocates
    nothing after the first round. The growth is the interpreter's own
    warm-up on these test bodies; its source in the interpreter has not
    been traced.

## Measurements

Instructions per iteration, from `boil/tools/micro_loops.py`:

| Loop | base | call-threaded | inline |
|---|---|---|---|
| empty loop skeleton | 87.1 | 123.4 | 36.3 |
| int add | 103.2 | 125.0 | 28.9 |
| float add | 130.1 | 152.9 | 35.8 |
| int compare | 110.4 | 131.1 | 28.9 |
| float compare | 114.2 | 136.1 | 33.9 |
| list-of-floats subscript | 118.3 | 140.1 | 85.8 |
| two `STORE_FAST` | 69.1 | 93.0 | 23.9 |
| attribute load | 75.2 | 90.9 | 67.8 |
| attribute store | 158.1 | 172.0 | 140.7 |
| new instance, four attributes | 2018.4 | 2253.0 | 2038.9 |
| new instance, empty `__init__` | 1220.1 | 1398.5 | 1383.2 |

For the pyperformance benchmarks, each benchmark body ran directly with
`PYTHONHASHSEED=0`, pinned to one CPU, after one warm run. Each figure is
one run's share of the difference between processes of 1 and 7 runs. The
counts come from the 7-run process with the fewest cycles out of five, and
the times are the best mean of those five. In the table, "commit 2" is
the inline fast paths as they were before the rebase (`f86efd25c98`), and
"commit 3" adds the separate stores of implementation decision 2:

| Benchmark | instructions, base | instructions, commit 2 | instructions, commit 3 | cycles, base | cycles, commit 2 | cycles, commit 3 | ms, base | ms, commit 2 | ms, commit 3 |
|---|---|---|---|---|---|---|---|---|---|
| nbody | 1489.8M | 1268.8M | 1268.9M | 466.6M | 497.5M | 482.3M | 109.0 | 115.7 | 114.1 |
| richards | 645.8M | 635.0M | 637.8M | 247.6M | 263.9M | 263.6M | 57.9 | 61.6 | 61.0 |
| go | 1432.0M | 1405.0M | 1406.5M | 532.0M | 602.1M | 600.2M | 123.6 | 140.1 | 139.1 |

Instruction counts for three more benchmarks, measured at commit 2:

| Benchmark | base | commit 2 |
|---|---|---|
| spectral_norm | 1085.3M | 918.4M |
| nqueens | 1067.8M | 1038.2M |
| float | 943.8M | 932.5M |

Cycle counts of one build vary by up to 10% between sessions on this
machine: nbody's base build ranged from 466.6M to 516.0M. Compare columns
within one table only.

Code size, over all blocks compiled during the warm run and one further run:

| Benchmark | blocks | bytes per IR word | bytes per call | bytes per inline instruction |
|---|---|---|---|---|
| richards | 68 | 52.9 | 71.8 | 42.8 |
| nbody | 50 | 52.7 | 71.8 | 43.7 |
| go | 81 | 53.1 | 71.9 | 43.5 |

## Where the cycles go

Per-run counters from `perf stat`, by the same method as above but with 1
and 6 runs. The cycles of these runs are noisier, so they are left out:

| Counter, per run | richards, base | richards, tier 0 | go, base | go, tier 0 |
|---|---|---|---|---|
| instructions | 646.4M | 639.5M | 1432.1M | 1409.6M |
| branch misses | 1.09M | 0.51M | 1.86M | 1.04M |
| indirect branch mispredicts | 0.62M | 0.07M | 0.93M | 0.38M |
| return mispredicts | 0 | 0 | 0 | 0 |
| L1i misses (`ic_fw32_miss`) | 0.32M | 0.89M | 1.23M | 3.23M |
| L1i fetches (`ic_fw32`, op cache misses) | 93.3M | 115.2M | 185.0M | 248.7M |
| iTLB misses | 0 | 0.02M | 0 | 0 |
| store-to-load interlocks | 0.14M | 0.08M | 0 | 0 |

Share of the cycles, from `perf record -e cycles:u` over 6 runs:

| Where | richards, base | richards, tier 0 | go, base | go, tier 0 |
|---|---|---|---|---|
| handlers, interpreter | 55.4% | 0.5% | 70.9% | 0.5% |
| handlers, call mode | | 34.3% | | 47.1% |
| machine code | | 18.4% | | 22.1% |
| `_PyTier0_Continue()` | | 0.0% | | 0.0% |
| other C | 46.1% | 41.1% | 28.4% | 29.1% |

This is my reading of the three candidate causes:

- **The second copy of every handler.** It costs nothing during the runs.
  The interpreter's handlers take 0.5% of the cycles, so only one copy is
  hot at a time. The binary is larger, and a program that mixes compiled
  and interpreted code would keep both copies hot. These benchmarks do not.
- **Indirect calls and returns instead of computed gotos.** These are not
  the cause either. Indirect mispredicts fall by 60 to 90%, because each
  word has its own call site, and returns never mispredict. Total branch
  misses fall by 0.6 to 0.8M per run, worth about 10 to 15M cycles.
- **Frame switches.** In commit 2 each Python call between compiled frames
  cost two store-to-load interlocks: a call loop ran at 128.9 cycles per
  iteration against the interpreter's 105.7. Separating the stores
  (implementation decision 2) brings it to 117.5. Switching frames inside
  the machine code keeps `_PyTier0_Continue()` at 0%. The remaining 12
  cycles per call are not yet attributed. Two candidates are the stash
  round trip and the indirect jump from one site per block that enters the
  target's code.

What remains is the front end. L1i misses rise 2.6 to 2.8 times and op-cache
misses by a quarter to a third. On go that is 2M extra misses per run,
worth some 25 to 40M cycles, against a measured gap of about 68M. The
handlers that tier 0 still calls are as large as before. They now share the
cache with machine code that is specific to each function: 53 bytes per IR
word, so a 30-instruction loop body spans about 25 cache lines. The
interpreter shares one copy of its handlers among all functions. Halving the
machine code with a shared stub did not reduce the misses, though. So the
misses do not follow the byte count simply, and the rest of the gap is
unexplained. One candidate is the latency of the stash round trip on every
call. A counter cannot show it, and testing it means passing state in
registers.

## What tier 1 needs

1. **Register-cached stack top and locals.** Inline code now stores every
   result to its frame slot and the next instruction loads it back. Tier 1
   should keep values in registers across instructions and write them to the
   frame only at calls and exits.
2. **Inline fast paths for the calls that remain.**
   `LOAD_ATTR_INSTANCE_VALUE`, which is 12 to 15% of go's cycles in both
   builds, the shape guards, `STORE_ATTR_INSTANCE_VALUE`, `LOAD_GLOBAL_*`,
   and frame push and pop for `CALL_PY_EXACT_ARGS` and `RETURN_VALUE`. This
   removes both the calls and the handler code they keep hot.
3. **One copy of the handlers.** The interpreter and the machine code should
   share handler bodies. That halves the handler code in the binary and keeps
   a program that mixes tiers from running two hot copies.
4. **State in registers across calls.** Handlers should return `next_instr`
   and the stack pointer in registers (rax:rdx, or a convention of our own)
   rather than through the thread state.
5. **Less code, laid out hot.** Compile hot regions rather than whole
   blocks, keep startup code out, and put hot code together with the cold
   paths elsewhere.
6. **Direct calls.** Call the handler named at compile time behind a guard
   on the word's handler index, instead of calling through the table.
7. **Unboxed floats in containers.** nbody's floats are boxed in lists, so
   its arithmetic leaves the inline path. This needs object-model work
   (note 02).
