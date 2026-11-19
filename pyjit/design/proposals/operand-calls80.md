# Calls80: operand-encoded calls

Status: proposal, 2026-11-19. User decisions (2026-11-19): the child frame
starts at the arena top (design O, one storage model); the snapshot rule
stays (no writer-pays scheme); no borrowed arguments. Open: whether opcode
tracing keeps IR-word read semantics (recommended) or must match upstream
exactly. The call and return handlers are written in the shape that the
handler design (`E59/handlers81-design/`, in progress) chooses. It
changes no source. Evidence is in `E59/calls80-design/` (paths below are
relative to it):

- `raw/census-*.txt`: dynamic census (line-weighted) of call inputs, binary
  and compare inputs, return inputs and load words, for eight kernels;
- `raw/census-static-*.txt`: the same census with static weighting;
- `raw/reconcile.md`: one table comparing the two;
- `raw/estimate.txt`: kernel estimates;
- `scripts/call_census.py`, `scripts/reconcile.py`, `scripts/estimate.py`;
- `survey-notes.md`: source facts from two read-only surveys of checkpoint78.

Source references are to checkpoint78 (`99a04733128`). Instruction counts
marked "measured" come from the
[execution79](../../issues/register-semantics59/execution79-instruction-gap.md)
ledgers. Everything else is an estimate, and is marked as one. The
coordinator re-ran both behaviour checks in section 2 on checkpoint78 and
upstream (`raw/trace-write-check.txt`).

## Recommendation in brief

1. **Encoding.** A call is a head word followed by `ceil(n/4)` operand words.
   Here `n` is the number of operands: the self input, if any, then the
   arguments. An operand word is an ordinary IR word whose handler is a new id,
   `IR_OPERANDS`. Its fields A–D name four slots. Every member of a call
   family computes its length from the head's C field, so specialized,
   generic and instrumented forms always agree. No call is stacked in the IR.
2. **Frame.** The child frame starts at the thread arena top for every caller,
   thread or heap. The window and the overlap go. So do `saved_top`, the
   overlap clobber proofs, and the separate copy path for heap callers.
3. **Evaluation order.** The existing copy-propagation rule stays. A local
   becomes an operand only where its load could fold today. Loads blocked by
   an intervening op that may run Python stay as snapshot copies. A
   "writer pays" alternative is specified in section 2. It is not
   recommended: its gain is at most about 1% of instructions, against a
   second code image and a pc map.
4. **Constant operands.** These need no new handlers. Inline-scalar and
   immortal constants get constant slots wherever a register operand reads
   them, not only inside loops. `return None` becomes `RETURN_VALUE_EMPTY`
   naming the None slot.
5. **Borrowed arguments.** Not now. The measured ceiling is 7 instructions per
   heap argument, at most 0.27% of a kernel's instructions. It needs a
   per-frame ownership mask that GC traversal, frame copies and the
   `f_locals` proxy must all honour.

Estimated effect after handlers80 lands (estimates, ±50% on the operand
items):

| Kernel | Operand calls (design O) | Constant slots | Heap-caller path |
| --- | ---: | ---: | ---: |
| DeltaBlue | −0.8% | −0.7% | — |
| raytrace | −0.5% | −0.5% | — |
| richards | −0.35% | −0.3% | — |
| deepcopy_memo | −5% | 0 | — |
| coroutines | (included) | −1.8% | −5.4% |

The percentages are shares of checkpoint78's instruction count.
Handlers80's own effect is execution79's 7–9% of upstream's total; it is
separate and not repeated here.

---

## 0. What the data says about call inputs

The two censuses disagree only in what they count. Execution79's static move
census asks where each `LOAD_FAST_BORROW` word goes: 66% fill a call window.
This census asks where each call input comes from, and weights each word by
the run-time count of its source line. The table puts both on the same eight
kernels. Each kernel appears twice, with each weighting. Columns from Callable
onward are shares of all call inputs, counting callable and self slots.

| Kernel | Weighting | LFB words filling a window | Call inputs | Callable | Self | Local arg, foldable | Local arg, blocked by order or line | Constant arg | Temporary arg |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| deltablue | static | 64% | 474 | 38% | 24% | 14% | 2% | 9% | 13% |
| deltablue | dynamic | 83% | 139,318 | 44% | 41% | 9% | 1% | 1% | 5% |
| raytrace | static | 61% | 439 | 33% | 15% | 7% | 2% | 17% | 27% |
| raytrace | dynamic | 40% | 7,884,346 | 37% | 26% | 7% | 2% | 1% | 28% |
| richards | static | 89% | 299 | 33% | 13% | 11% | 5% | 9% | 28% |
| richards | dynamic | 83% | 1,436,406 | 38% | 34% | 6% | 9% | 0% | 13% |
| deepcopy_memo | static | 53% | 37 | 43% | 5% | 24% | 0% | 16% | 11% |
| deepcopy_memo | dynamic | 84% | 1,735 | 44% | 1% | 44% | 0% | 0% | 12% |
| coroutines | static | 100% | 13 | 46% | 8% | 15% | 0% | 15% | 15% |
| coroutines | dynamic | 100% | 485,584 | 50% | 0% | 0% | 0% | 0% | 50% |
| chaos | static | 50% | 281 | 35% | 7% | 9% | 2% | 9% | 37% |
| chaos | dynamic | 27% | 1,910,703 | 33% | 9% | 10% | 0% | 2% | 45% |
| go | static | 52% | 279 | 41% | 23% | 14% | 1% | 5% | 17% |
| go | dynamic | 44% | 3,305,081 | 36% | 30% | 18% | 2% | 3% | 11% |
| nbody | static | 15% | 28 | 46% | 7% | 14% | 4% | 0% | 29% |
| nbody | dynamic | 1% | 40,024 | 50% | 0% | 50% | 0% | 0% | 0% |
| all eight | static | 59% | 1,850 | 36% | 16% | 11% | 2% | 11% | 23% |
| all eight | dynamic | | 15,203,197 | 37% | 24% | 10% | 2% | 1% | 25% |

How to read it:

- Both claims hold. Most `LOAD_FAST_BORROW` words feed a window: 59% static
  here, against execution79's 66%. The census scripts classify consumers
  differently; this one traces each window input back to its producer. Yet
  locals are a minority of call inputs. Callable and self are 61% of inputs,
  dynamically. Temporaries are another 25%.
- **Computed temporaries already land in the window.** Their producer's dst is
  the window slot: a `BINARY_OP`, an inner `CALL` or a `LOAD_ATTR` writes
  there directly. With the overlap they then cost nothing at the call. The
  method self slot works the same way: `LOAD_ATTR_METHOD_*` writes callable
  and self to `A` and `A+1`. Only locals and constants pay a copy today:
  36 instructions per `LOAD_FAST_BORROW`, 37 per `LOAD_COMMON_CONSTANT`, and
  52 per `LOAD_CONST` of the kwnames tuple (measured).
- Calls have few operands. Dynamically, 99.5% of call sites have three
  arguments or fewer (`call sites by argc` in each census). Zero-argument
  method calls are the largest class: 54% of richards' calls and 75% of
  DeltaBlue's.

The estimates in section 8 use the dynamic counts. Line weighting
miscounts lines that execute partially, and loops within one line. Against
the IBS handler shares it agrees to within about 20%. Examples: DeltaBlue has
15.3K `LOAD_FAST_BORROW` executions per loop by census, against 18.3K from
IBS. Raytrace has 1.79M against 1.51M.

---

## 1. Encoding

### Constraints

The word is 64 bits: a 9-bit handler, 7 flag bits and four 12-bit fields
(`Include/internal/pycore_ir.h`). Four flag bits are the slot-class flags
`IR_F_LOCAL_A..D`. Two are `IR_F_NO_SELF` and `IR_F_NO_KW`. Bit 15 is
reserved. A call needs at least five things: a dst, the callable, a
self-or-none input, the feedback offset, and the arguments. One word cannot
hold them. Something has to give: either the window convention, which names
every input by one base field, or the single word.

Fixed width is not a principle here; the user has said so. The choice below
is made on cost and on the number of consumers that must change.

### Options

| | A. Fixed-length call types | B. Trailing operand words (recommended) | C. Per-code operand table | D. Operands in the feedback record |
| --- | --- | --- | --- | --- |
| Shape | `CALL_OPS` family: head plus one operand word, at most 4 operands. Larger calls keep today's stacked `CALL` | Head plus `ceil(n/4)` operand words, every arity | Head field indexes `co_operands` | `_PyFb_CALL` grows an operand list |
| Loads per call for the operands | 1 | 1 per 4 operands | 2 (table base, then entry) | 1 (`fb` is already loaded for the version guard) |
| Words for `f(x, y)` | 3 (`LOAD_GLOBAL`, head, operands) against 4 today | 3 | 2 | 2 |
| Length known from | handler format | head's C field: `1 + ((C + 3) >> 2)` | always 1 | always 1 |
| `next_instr`, `return_pc` | `+2` | `+ length` | `+1` | `+1` |
| `f_lasti`, traceback, line table, exception table | point at the head; operand word shares head's location and handler range | same | unchanged | unchanged |
| Instrumentation | twin replaces head; walkers skip 1 word | twin replaces head; walkers skip `length − 1` | unchanged | unchanged |
| Specialization | same length within the family | same length within the family, since every member reads one C field | unchanged | unchanged, but the record mixes immutable operands with mutable feedback |
| Two IR forms for calls | yes: operand and stacked | no | no | no |
| Serialized in `co_code` / `.pyc` | yes | yes | needs a new code field | needs the feedback table serialized as code data |
| Tools that must learn it | all word walkers | all word walkers | every reader of call inputs: verifier, regalloc, dis, `get_ir` | same as C, plus the feedback vector's 32 KB limit (`IR_FB_UNITS_MAX`) becomes a correctness limit |

Option A meets the user's "explicitly stack the complex cases" literally.
But it keeps two IR forms for one Python construct, and the compiler must
pick one. Arity is known statically, so that part works. Polymorphism is not
known, and option A does not need it, but the stacked form needs its own
generic handler and verifier rules. Option B has one form for all arities.
It costs one shift and add per call for the length, about 2 instructions
(estimate).

Options C and D keep the word stride. They move a call's inputs out of the
instruction, though. Every consumer of call inputs would then read a second
structure. That means the verifier (`_PyRegCfg_Verify`, `check_words`), the
dataflow passes in `Python/regcfg_passes.c`, the disassembler and the
observers. D also turns a performance limit (feedback size) into a
correctness limit. The survey found about a dozen stride-1 walkers, listed
below. Each needs one call to a length helper. That is mechanical work. A
second home for inputs is a permanent design cost. The existing relaxed
`FOR_ITER`/`SEND` adapter in `Python/instrumentation.c` (`instr_length`,
lines 100–113) already makes some instructions three words long. Option B
generalizes it.

### Recommended encoding (option B)

Head word, for every member of the CALL, CALL_KW and CALL_FUNCTION_EX
families, specialized or not:

| Field | Meaning |
| --- | --- |
| handler | the family member, or its instrumented twin |
| `IR_F_NO_SELF` | there is no self operand (as today, a compile-time fact from `_PyRegPass_CallRoles`) |
| `IR_F_NO_KW` | EX only: no kwargs operand |
| `IR_F_LOCAL_A`, `IR_F_LOCAL_B` | dst or callable names a local (derived in the executing copy, as today) |
| A | dst, a slot or `IR_NO_DST` |
| B | callable slot |
| C | `n`, the number of operands: self (if `!NO_SELF`), then the positional arguments, then the keyword values |
| D | feedback offset |

Operand words follow, `ceil(n/4)` of them:

| Field | Meaning |
| --- | --- |
| handler | `IR_OPERANDS`, a new id that is never dispatched. The handler table maps it to a fatal error |
| `IR_F_LOCAL_A..D` | operand k names a slot below `co_nlocalsplus`, derived by `_PyIR_ExecLocalFlags()` like any word |
| A–D | operands `4i` to `4i + 3`; an unused field holds `IR_FIELD_MAX` |

Making the operand word an ordinary word is deliberate. The executing copy's
flag derivation applies to it unchanged. `check_words` can range-check its
fields like any other word. `get_ir` and `dis` print it as a word. A
debugger that only computes `instr_ptr − ir_words` never lands on it.

**Source kinds.** The field value and its derived flag decide the kind, with
no new kind bits:

| Slot | Kind | At a Python call | At a native call |
| --- | --- | --- | --- |
| below `co_nlocalsplus`, a named local | borrowed | child takes `PyValue_DUP` | borrowed into the view |
| below `co_nlocalsplus`, a constant slot (`CO_FAST_CONST`, section 6) | borrowed; inline or immortal, so the DUP is one tag test | DUP (tag test) | borrowed |
| at or above `co_nlocalsplus` | temporary, owned by the caller | moved into the child; the caller's home is set NULL | borrowed into the view; released after the call |

Cells and free variables never appear as operands. Only `LOAD_FAST_BORROW`
folds (`_PyRegPass_CopyProp`), and a cell is read by `LOAD_DEREF` into a
temporary. `check_words` rejects an operand that names a cell or free slot.

**Callable and self.** The callable is field B and may be a local, for
example `fn(x)` with `fn` a parameter. The method form keeps today's
protocol. `LOAD_ATTR_METHOD_*` and `LOAD_SUPER_ATTR_METHOD` write the callable
or attribute to `A` and self-or-NULL to `A+1`. The call names `A` as B and
`A+1` as operand 0. A self that is NULL at run time is still tested at run
time: round 3 found that test is semantic. `LOAD_GLOBAL` no longer writes a
NULL slot. Its oparg's low bit becomes meaningless for call sites, and the
conversion in `Python/regcfg.c` (`convert_instr`) drops it. This saves the
measured 14-instruction `_NULL_SLOT_CONDITIONAL`, less the bit test that
remains (estimate: 11 saved).

**Keyword names.** In the CALL_KW family the last operand is a `co_consts`
index, not a slot. The format declares that operand's role as IMM, and the
handler reads `co_consts[k]` borrowed, because the frame pins the code. This
removes the `LOAD_CONST` of the names tuple (52 instructions, measured in
`c_kw`). The slot-class flag derived for that field is ignored.

**CALL_FUNCTION_EX.** It gets the same head. Its operands are the positional
iterable or tuple and, unless `IR_F_NO_KW`, the kwargs mapping. Today its C
field holds the feedback offset. That moves to D.

**Limits.** C is 12 bits, so a call has at most 4,095 operands. Frames already
have at most 4,094 slots (`IR_MAX_SLOTS`), so the encoding adds no new limit.
Handler fast paths are generated for one operand word, `n ≤ 4`. That covers
99.5% of dynamic sites in the census, counting self as an operand. Larger
calls take a continuation that loops over the operand words. The form and the
handler are the same; only the path differs.

**Jumps and tables.** No jump may target an operand word. The assembler emits
operand words with the head's location entry and inside the head's
exception-table range. `check_words` (`Objects/codeobject.c`) enforces three
rules:

- no branch target lands on an operand word;
- no exception range starts or ends between a head and its operands;
- an operand word appears only after a call head and is never reached by
  stepping.

**Word walkers that change.** These are the walkers the survey found. Each
gains one helper, `_PyIR_WordLength(word)`, which returns 1 for every
non-call word:

- `Objects/codeobject.c`: `check_words`, `semantic_step_core`,
  `validate_semantic_and_position_maps`, `validate_normal_transfers`;
- `Python/assemble.c`: emission, `assemble_location_info`,
  `assemble_exception_table`, `place_generator_boundaries`;
- `Python/instrumentation.c`: `instr_length` and its callers (lines 615,
  763, 2099–2203, 2361, 2567, 2622, 2647, 3943);
- `Lib/dis.py`: `_unpack_opargs`, which also reports the operand list as
  `srcs`;
- `Modules/_testinternalcapi.c`: `get_ir`, which keeps one tuple per word and
  names operand words `IR_OPERANDS`;
- `Python/ir.c` home-view and plan code keyed by word index (operand words
  get no phase or plan entries);
- `Objects/frameobject.c` `mark_stacks` and `_PyCode_InstrDepthBefore`: the
  call's reads become its operand list, not a stack extent.

Remote debugging (`Modules/_remote_debugging/*.c`) and
`Tools/gdb/libpython.py` compute only word indices of heads. They need no
change.

---

## 2. Evaluation order

### The rule and what it costs

Python evaluates `f(x, g())` left to right. If `g()` rebinds `x` through
`frame.f_locals`, the call must see the old `x`. The write goes through
`framelocalsproxy_setitem` (`Objects/frameobject.c` 311–382), the only
external writer of a fast local; deletion raises there, and cells go through
`_PyCell_SetValue`. The overwritten value stays alive in
`f_overwritten_fast_locals`, but the slot holds the new value.

The rule is unchanged from copy propagation (`_PyRegPass_CopyProp`,
`Python/regcfg_passes.c` 155–320; verifier `check_folded_load`,
`Python/regcfg.c` 1261). A local may become a call operand when four things
hold:

- its load is on the call's line;
- the load is in the call's exception region;
- nothing between the load and the call writes the local;
- nothing between may run Python (`_PyIR_MayRunPython`).

The call reads its local operands at entry, before any code runs. That is
equivalent to upstream, whose arguments are already on the stack when `CALL`
starts. A blocked load stays a `LOAD_FAST_BORROW` into a temporary, and the
operand names the temporary.

The census measures how often the rule forces a copy (dynamic counts per
loop):

| Kernel | Call args, local and foldable | Call args, local but blocked (order, line or write) | Binary/compare inputs, local but blocked | Blocked share of binary inputs |
| --- | ---: | ---: | ---: | ---: |
| DeltaBlue | 12,022 | 706 | 0 | 0% |
| raytrace | 590,237 | 120,170 | 741,424 | 5.9% |
| richards | 91,403 | 131,595 | 20,310 | 4.3% |
| chaos | 195,037 | 5,003 | 515,278 | 7.7% |
| go | 585,047 | 69,421 | 305,363 | 6.0% |
| nbody | 20,005 | 1 | 200,055 | 1.4% |

Richards is the outlier for calls. Its common shape is
`self.fn(pkt, self.task_rec)` (`Task.runTask`, line 219). The generic
`LOAD_ATTR` of `self.task_rec` may run Python, so `pkt` must be snapshotted
even though the specialized load never runs code. Line changes block
almost nothing: go's 62,865 blocked returns are the only visible case.

**Ceiling of a fix.** Suppose every blocked load folded. Each removes a
dispatch: 36 instructions today, 25 after handlers80. Under design O it
also removes the 7-instruction temporary move (estimates):

| Kernel | Blocked loads per loop | Saving today | Saving after handlers80 |
| --- | ---: | ---: | ---: |
| raytrace | 861,594 | 31M (0.9%) | 22M (0.6%) |
| richards | 151,905 | 5.5M (0.9%) | 3.8M (0.65%) |
| DeltaBlue | 706 | 0.03M (0.05%) | 0.02M |

### A "writer pays" alternative

The rare writer could pay instead. One sound scheme:

1. **Two images from one compilation.** The compiler still emits today's
   snapshot copies in `co_code`; this is the *safe image*. The executing copy
   is built from it as the *fast image*. Each snapshot whose reader crosses an
   op that may run Python is deleted, and its reader's operand is renamed to
   the local.
2. **Pending-read table.** For every word that may run Python, a per-code
   sorted table lists the locals whose folded reads are pending across it,
   with their snapshot slot.
3. **The writer.** `framelocalsproxy_setitem` looks up the target frame's
   current word in the table. If the local it writes is pending there, it:
   (a) switches the frame to the safe image, translating `instr_ptr`,
   `return_pc`, `ir_words` and `ir_feedback` through the pc map;
   (b) fills each pending snapshot slot with a DUP of the local's current
   value, which is still the value at load time because any earlier write
   would already have switched; and
   (c) performs the write. The frame then runs the safe image until it
   returns.

What it must guarantee:

- the proxy is the only external writer of fast locals, which holds today;
- both images agree on every slot at every word that may run Python, except
  the snapshot slots;
- monitoring event offsets, `f_lasti`, the line table, the exception table
  and every word-indexed side table report safe-image indices or translate
  them.

That last guarantee is the expensive one. Today the executing copy and
`co_code` have the same length. Instrumentation tables, semantic maps,
observation maps and feedback ordinals are all indexed by that shared word
index (survey). A fast image shorter than `co_code` breaks all of them.
Keeping the length by replacing the snapshot with a no-op word still
dispatches, at about 15 instructions (estimate), so most of the gain is lost.

**Recommendation.** Keep the snapshot rule. The gain is at most 0.6–0.9% in
the kernels most affected. The scheme needs a second image, a pc map, and a
word-index translation in every observer. Record it as a user decision, open
question 2. Two cheaper facts help in the meantime:

- Handlers80 cuts each snapshot from 36 to about 25 instructions.
- `INSTRUCTION`-event and opcode-trace callbacks that write `f_locals` mid-line
  already see this difference for folded register operands today. Checked on
  checkpoint78: in `def h(x, y): return x * y + 1`, an opcode-trace callback
  sets `x = 1000` at the multiply. The call `h(10, 2)` then returns 2001;
  upstream's order gives 21. Call operands extend this existing behaviour to
  call arguments; they do not create it. Step 2b's tests pin it (section 9).

---

## 3. Frame changes

### Constraint

Overlap gives a zero-cost transfer of temporaries, but it costs this
machinery:

- `saved_top` in the header, because a child base inside the caller's slots
  cannot restore the old top;
- the shared-tail NULL proofs: `_PyFrame_ReserveForCall` debug asserts, the
  `semantic_step_core` "unproved child clobber home" proof
  (`Objects/codeobject.c` ~2955), and `overlapping_call` with the verifier
  rule "a temporary live across a call above its window" (`Python/regcfg.c`
  1247, 1504);
- a separate copy path for heap callers (`call_push_moved_exact_args`,
  `call_transfer_exact_args`). Measured in coroutines it costs 128
  instructions per call, 8.1% of the kernel;
- keyword plans that stage values through C locals because the window is
  also the child (`call_bind_known_python`, `Python/ceval.c` 4348–4394);
- the `f_lineno` setter rule "the child owns the argument window"
  (`Objects/frameobject.c` ~3560);
- double visits of overlapped cells in the free-threaded GC
  (`Python/gc_free_threading.c` 436, 826).

With operands there is no window to overlap, so all of this can go.

### The child at the arena top (design O, recommended)

- **Placement.** For every exact Python call, from a thread or a heap caller,
  `base = datastack_top`. For a thread caller this is the caller's slot end,
  because no child overlaps any more. `_PyFrame_ReserveForCall` becomes:
  check space, `end = base + nslots`, `_PyStackChunk_EnsureFrameStorage`,
  `datastack_top = end`, `datastack_limit = header`. Its `physical_end`
  maximum and the `saved_top` computation go.
- **Initialized frontier.** No change in kind. `chunk->initialized` still
  says that inactive cells below it are NULL. The child's cells lie wholly
  above the old top, so they are NULL once `EnsureFrameStorage` returns. The
  call writes the arguments into `base[0..n)`, and the rest stays NULL. The
  caller never shares cells with the child, so the shared-tail assert loop
  goes.
- **Deleted header field.** `saved_top` goes, and the header shrinks from 112
  to 104 bytes. Popping restores `datastack_top = frame->base`, as upstream
  does. A frame that begins a new chunk still needs the previous chunk's
  value top. `_PyStackChunk` gains `value_top`, written by `push_chunk`
  (`Python/pystate.c` 3160–3174) and read by `_PyThreadState_PopChunk`
  (3207); one word per chunk. The static asserts on `phase_state` and
  `owned_slots` (`pycore_interpframe_structs.h` 72–77) move. Debug offsets
  use `offsetof` and follow automatically.
- **Heap callers.** They take the ordinary path, because the child's place
  never depended on the caller. `call_push_moved_exact_args` and
  `call_transfer_exact_args` are deleted.
- **Init shim.** `CALL_ALLOC_AND_ENTER_INIT` already places its shim and
  child at the top and copies. It now fills from operands and drops
  `call_transfer_exact_args`.
- **Compiler.** The window allocation in `compact_op` (`Python/regcfg.c` 962:
  "a window that is not consecutive", "a local in a window") goes for calls.
  So does `overlapping_call`. A temporary may live across a call at any slot.

**Cost.** Each temporary operand now moves: decode, load, store into the
child, and NULL the caller's home. That is about 7 instructions (estimate).
The NULL store stays because NULL is the empty-home authority
(`call-and-frame-lifetimes.md`). Self is a temporary in every method call.
So `o.m()` gets about 10 instructions slower, and it is the commonest call in
DeltaBlue and richards (estimate).

### The middle option: keep the window (design W)

The compiler keeps windows. The operand list names, for each window
position, either the window slot itself (already in place) or a local or
constant source that `CALL` fills into the window. Then the child overlaps
as today.

| | O: child at top | W: window kept |
| --- | --- | --- |
| Local and constant arguments | filled by CALL, about 9 each | same |
| Temporary arguments and self | moved, about 7 each | in place, 0 |
| Heap callers | ordinary path | still separate; can be made inline (same saving) |
| `saved_top`, clobber proofs, verifier window rules, KW staging, FT double visit | deleted | kept |
| Register allocation | free | windows constrain it |
| Native calls | gather a borrowed view, about 3 per argument; `PY_VECTORCALL_ARGUMENTS_OFFSET` available | window is the array; offset slot available only when self is virtual |
| Kernel effect after handlers80 (estimate, `raw/estimate.txt`) | DeltaBlue −0.8%, raytrace −0.5%, richards −0.35%, coroutines −5.4% | DeltaBlue −1.1%, raytrace −1.0%, richards −0.8%, coroutines −5.6% |

On instruction count W leads by 0.3 to 0.5 points in three of five kernels,
almost all of it from moving self. O deletes one storage model. Thread and
heap callers then share one path. The native path no longer depends on
window layout, and the verifier and code validator lose their hardest call
rule.

O is recommended. It is the design without a dual form, and its cost is
bounded and measured: about 7 instructions per temporary operand. The choice
is open question 1; the encoding is the same either way. One refinement was
considered and rejected. The call could read the method receiver from its
local instead of the `A+1` temporary, but the generic `LOAD_ATTR` may run
Python, and the receiver read would then be order-blocked in the same cases.

---

## 4. Call paths

The rules below hold on every path.

- **Read once, first.** A call reads all its operands at entry, before
  anything that may run Python. That includes specialization, which may
  allocate and so trigger GC finalizers, method expansion, monitoring, tuple
  preparation, kwargs dict creation and the `__init__` allocation. Later
  steps use the values read, never the homes. A local read this way stays
  valid even if a callback rebinds the local, because the proxy keeps the
  old value in `f_overwritten_fast_locals`. A temporary cannot be written by
  Python.
- **Commit.** For a Python callee, the commit is
  `tstate->current_frame = child` (`_PUSH_FRAME`). Before it the caller's
  homes own every input, and nothing has changed. After it the child owns
  its bindings and function, and the caller's temporary operand homes are
  NULL. Between the reservation and the commit nothing allocates, calls out
  or can call back.
- **Errors before commit** are guard failures. They deopt to the family's
  generic handler (`JUMP_TO_PREDICTED`) with no effects. Without effects the
  `CALL_EFFECTS` fallback label is not needed on exact paths.
- **Errors after a native call** leave the temporary operands in their homes.
  The unwinder clears them with the rest of the scratch, as it clears the
  window today (`error_scope_clear`).
- **Handlers80 compliance.** Fast paths hold the guards, the reservation, the
  operand fill and the header writes, all inline. Allocation, vectorcall,
  specialization and constant-slot fills of mortal constants are tail-called
  continuations.

| Path | Before commit | Commit and after | Unwinding and callbacks |
| --- | --- | --- | --- |
| `CALL_PY_EXACT_ARGS` | Guards as today: PEP 523, version, `co_argcount == argc + self`, stack space, recursion. Reserve at the top | Fill: local DUP, temp move, constant slot tag test. Callable temp moves into `f_funcobj` (a local callable is DUP'd). Save `return_slot` and `return_pc` after the operand words. Push | No callback before commit. Return and retirement are unchanged |
| Method call (`o.m(x)`) | Operand 0 is the `A+1` temporary. Test it for NULL at run time | Self moves into child slot 0 | As above |
| `CALL_BOUND_METHOD_EXACT_ARGS` | Callable temporary is a `PyMethod`. Guard `im_func`'s version | Child slot 0 takes DUP(`im_self`) and `f_funcobj` takes DUP(`im_func`). The method temporary is closed before commit. That close is callback-free: both referents keep other owners, and the type is exact `PyMethod`. Today's `IR_PUBLISH_TEMP` of window[0..1] and its `CALL_EFFECTS` go | As above |
| `CALL_PY_GENERAL` (defaults) | `_PyFbCall` plan guards as today | Operands fill the positional prefix. Defaults are DUP'd from the current tuple (unchanged rule, including the defaults-offset check). Empty varargs. `**kwargs` dict or pending index | Building a kwargs dict allocates: it runs before the reservation, after the operand read, and a refresh follows it, as today |
| `CALL_KW_PY` | Plan guards as today. The names tuple is read from `co_consts` (borrowed) | Positional operands go to the prefix. Keyword operands go straight to their cached destinations; the C-local staging and the names-owner close go | The guards ran before any effect, so a miss enters the generic keyword path |
| `CALL_KW` generic, bound, non-Py | As the generic path below, with `kwnames` from `co_consts` | | |
| `CALL_ALLOC_AND_ENTER_INIT` | Guards. Read operands. Allocate self (continuation; may run GC) | Shim at the top, then init frame above it. Shim slot 0 takes DUP(self). Child slot 0 takes self; operands go to child slots 1.. The type temporary is closed after the commit, because it can be the class's last reference | Allocation failure: nothing has moved; raise. `EXIT_INIT_CHECK` is unchanged |
| `CALL_FUNCTION_EX` | Operands: callable, args, kwargs. `call_transaction_prepare_tuple` converts a non-tuple, which may run `__iter__` | Python callee: `eval_frame_push_and_init_ex_code` at the top (already non-overlapping). Native: `_PyValue_Call(callable, tuple, kwargs)` | The converted tuple replaces the args temporary, as today. A local args operand is not replaced: the tuple goes into a C local and is closed after the call |
| Generic `CALL` and `CALL_NON_PY_GENERAL` (native, vectorcall) | Read operands into a borrowed view `PyValue v[1 + n]` on the C stack (n ≤ 7), else heap. `v[0]` is a scratch slot | `_PyValue_VectorcallTstate(callable, v + 1, n | PY_VECTORCALL_ARGUMENTS_OFFSET, kwnames)`. No pin of the callable: a temporary cannot change, and a local's old value survives a proxy write | Result to a temporary dst at once. Then detach the temporary operands (bits to C, homes NULL), close them in reverse order, refresh once, run the periodic check, and store a local dst last. This is today's order in `call_transaction_native`, without the window detach and retire arrays |
| Generic `CALL` reaching a Python function | Same view. If the function is plain (`_PyFunction_Vectorcall`, no PEP 523, not instrumented, code shape fits), take the exact push continuation | Fill from the view: DUP for locals, move for temporaries | `call_transaction_push_python_fallback` and the overlap path `call_transaction_overlap_python` merge into this one continuation |
| Builtins (`CALL_LEN`, `CALL_ISINSTANCE`, `CALL_TYPE_1`, `CALL_STR_1`, `CALL_TUPLE_1`, `CALL_LIST_APPEND`, `CALL_BUILTIN_*`, `CALL_METHOD_DESCRIPTOR_*`) | Guards on callable identity or type, as today, reading operand slots directly | The result goes straight to dst; nothing is published to window[0] first. The temporary operands and callable are released after. A local `x` in `len(x)` is read in place and not released | Errors leave operands in their homes for the unwinder |
| Super method (`super().m(x)`) | `LOAD_SUPER_ATTR_METHOD` keeps its 3-slot window: it is not a call. It writes attribute and self-or-NULL to `A` and `A+1` | The following call is an ordinary method call | Unchanged |
| Polymorphic sites | One form. The site re-specializes to its newest observation, as today. A megamorphic site stays generic, and the generic path's Python continuation is the exact push without the version guard | | |

**Where stacking is still needed.** Only at run time, never in the IR:

- the borrowed view for vectorcall, native callees and PEP 523;
- the tuple of `CALL_FUNCTION_EX`;
- a heap view above seven native arguments.

The compiler never has to guess which sites are polymorphic.

**What goes:**

- `call_transaction_begin`'s window recheck loop;
- `call_transaction_detach` and `call_transaction_retire`;
- `call_clear_owned_slots`;
- window publication in `call_transaction_expand_method` (bound-method
  expansion now writes the view);
- `_PyFrame_PushOverlapping`;
- `call_push_moved_exact_args` and `call_transfer_exact_args`;
- `_PyIR_CallInputHoles` and `_PyIR_CallInputRole`, since `IR_F_NO_SELF` now
  means "no operand", not "virtual cell";
- the `WINDOW` output kind for call formats in
  `Tools/cases_generator/ir_generator.py` and `pycore_ir_metadata.h`.

---

## 5. Observation

- **`sys.monitoring` CALL.** `call_transaction_monitor` takes the callable and
  arg0 (self if present, else the first argument, else `MISSING`) from the
  view, after the operand read and before the call. Upstream does the same
  from its stack. C_RETURN and C_RAISE use the same two values, held through
  the call as today. Instrumented twins have the same length as their
  family.
- **Instrumentation tables.** Operand words carry no events. `instr_length`
  skips them, so LINE and INSTRUCTION instrumentation never displaces an
  operand word's handler.
- **`sys.settrace`.** The `call` event sees the child's locals bound, as
  today. `c_call`, `c_return` and `c_exception` come from monitoring through
  `Python/legacy_tracing.c`, which reads the instrumentation vector, not the
  window. No change.
- **Frame objects and `f_locals`.** The caller's named locals are unchanged by
  a call. Temporaries are never visible. During a native call the caller's
  temporary operands stay in their homes, so GC sees them. During a Python
  call they are in the child. The callee's `f_locals` shows its parameters,
  as today. Hidden constant slots stay hidden (`CO_FAST_CONST`).
- **Tracebacks and line numbers.** `tb_lasti` and `f_lasti` are the head's
  index. Operand words share the head's location, so `PyCode_Addr2Line` is
  unaffected.
- **`f_lineno` jumps.** The setter's requirement of a discarded result
  (`return_slot == IR_NO_DST`) stays. Its window bookkeeping
  (`Objects/frameobject.c` ~3560–3580 and 3792–3836) shrinks, because the
  child owns no caller cells.
- **PEP 523.** Specialized handlers deopt on `_CHECK_PEP_523` as today. The
  generic path vectorcalls the function with the view. `_PyEval_VectorValue`
  DUPs from it and pushes at the top, which it already does.
- **GC.** `_PyGC_VisitFrameStack` and the home views no longer see
  overlapping frames.

---

## 6. Constant operands

**Constraint.** A register operand can only name a slot. A second operand
kind, such as "constant pool index", would need a kind bit for every field
of every handler, and there is one spare flag bit. Or it would need a branch
in every operand read. Constant slots (`_PyRegPass_ConstSlots`,
`Python/regcfg_passes.c` 806–1060) already give constants a slot: a hidden
local filled at frame push. They are limited to loop bodies, to non-window
readers, and to inline-scalar or immortal values.

**Rule.** Give a constant slot to every inline-scalar or immortal constant
that a register operand reads: binary and compare inputs, call operands,
`RETURN_VALUE` and `RETURN_VALUE_EMPTY` inputs, `STORE_ATTR` values. Cap it at
eight slots per code object, filled in order of static use. Every reader
then names a slot, and no handler changes: the readers already accept locals
through their borrow fields (`RETURN_VALUE_EMPTY` has `borrow_mask` 1;
`BINARY_OP_ADD_INT` has 3).

**Fill.** Today `_PyFrame_FillConstSlots` loops over a table, reads
`co_consts`, XDUPs and stores. That is about 9 instructions per constant
(estimate) and outlined. The constant slots are already contiguous: the pass
places them between the cells and the free variables. So the executing copy
keeps a ready image of their `PyValue` bits, `_PyCodeExec.const_image`, and
`_PyFrame_Initialize` copies it inline. That costs about 1 instruction per
slot plus 3 (estimate). Mortal constants, as after
`code.replace(co_consts=...)`, keep their loads.

**Cases.**

- `return None`: `LOAD_COMMON_CONSTANT` (37, measured) plus
  `RETURN_VALUE_EMPTY` becomes `RETURN_VALUE_EMPTY A=<None slot>`. Returning a
  local-class slot duplicates it first, and for None that is a tag test. Net
  about −28 per return (estimate), or −16 after handlers80. The
  certification of empty returns (`_PyIR_PLAN_ALL_RETURNS`,
  `validate_normal_transfers`) must accept a constant-slot source. No new
  handler id, no instrumented twin. Round 3 said a `RETURN_NONE` handler
  would need both.
- Small ints into binary and compare ops: `LOAD_SMALL_INT_INLINE` disappears
  where its reader has a borrow field. In coroutines, 364K such loads per
  loop sit outside any loop and are not converted today.
- Constants as call arguments: the operand names the slot.

**Estimate** (`raw/estimate.txt`): DeltaBlue −0.7%, raytrace −0.5%,
richards −0.3%, coroutines −1.8% after handlers80. The fill is charged
conservatively, at 4 per call for frames with binary constants. In
deepcopy_memo the fill can cost slightly more than it saves (+0.6%). The cap
and the order of static use bound that cost. The pass could also skip
functions whose only constant use is on a cold branch; the census cannot see
that.

**Extension.** `SEND`'s None input and the `BUILD_*` windows are window
inputs today. Giving them operand lists is the same work as for calls. It is
left out of this proposal.

---

## 7. Borrowed arguments

**Measured gain.** `a_obj` (two heap arguments) costs 450 instructions,
against 430 for `a` (two inline ints). Upstream's costs are 349 and 343. The
excess is (450 − 430) − (349 − 343) = 14 for two arguments, or 7 per heap
argument. The census gives a ceiling, assuming every local argument is a
heap object: 0.18% for DeltaBlue, 0.15% for raytrace, 0.27% for richards and
about 0 for coroutines.

Upstream borrows. Its tagged stackrefs let a callee bind a borrowed
reference. `_PyFrame_Copy` and the generator and return paths make them
heap-safe (`PyStackRef_MakeHeapSafe`), and GC skips tagged references.

**Hazards in this runtime:**

1. **One encoding.** Bit 0 of a pointer-space word means immortal, and each
   value has one encoding (`pycore_value.h`, around line 236). A borrowed tag
   in the word would break that: identity tests on bits, and the canonical
   home checks.
2. **Rebinding the parameter.** `STORE_FAST` or `DELETE_FAST` on a borrowed
   binding must not release it.
3. **`f_locals` writes into the callee.** The proxy must make the binding
   owned (DUP), and keep the old value alive as today.
4. **`f_locals` writes into the caller.** These are safe:
   `f_overwritten_fast_locals` keeps the old value until the caller dies,
   and the caller outlives the callee.
5. **Frames copied into generators.** `_PyFrame_CopyCreatedGenerator` and
   `_PyFrame_Copy` must DUP every borrowed binding. The copy is on the
   coroutines hot path.
6. **Frame objects that outlive the call.** `take_ownership`
   (`Python/frame.c` 299–370) copies the frame and must DUP.
7. **Cells.** `MAKE_CELL` moves a parameter into a cell, so a cell parameter
   must be owned.
8. **GC.** Traversal must not count a borrowed binding as an edge. Otherwise
   the referent's internal-reference count goes below its refcount.
   `_PyGC_VisitFrameStack` and every home-view reader must honour the
   borrowed set.

**Options:**

- **B1: tag the word.** Rejected by hazard 1.
- **B2: per-frame borrowed mask.** The compiler marks "read-only parameters":
  never stored, deleted or celled by the callee (a `CO_` bitmask). At the
  call, `mask = local-operand bits ∧ co_readonly_params`, stored in the
  header. Retirement, generator copy, frame-object takeover, the proxy and
  GC traversal consult it.
  - Cost: mask arithmetic and a store, about 3 per call (estimate); one bit
    test per parameter at retirement; and a header field, which needs 2
    bytes the 112-byte header does not have unless `saved_top`'s removal
    frees them.
  - Net: about 4 per borrowed heap argument (estimate).
- **B3: borrow only when the callee cannot expose its frame.** That cannot be
  known statically: `sys._getframe` and tracing can expose any frame.
  Rejected.

**Recommendation.** Do not borrow now. Net of the mask, the ceiling is under
0.2% of instructions. B2 adds ownership state that GC traversal, frame
copies and the proxy must each consult, and those are exactly the places
where the ownership review of values77 found bugs. If later measurements
show heap-argument counts matter, B2 is the design. It belongs with
values76's rule 1, and it is open question 3.

---

## 8. Estimates

### Per call shape

All counts are instructions per call, net of the loop. "Today" is measured
(`per-call-perfstat.txt`). "Handlers80 only" uses the clang-21 attribution
build where it exists (measured, but clang changes other code generation
too). Elsewhere it subtracts 11 per `LOAD_FAST_BORROW` removed by the
convention (36 against 25). The operand columns are estimates built from
these unit costs:

| Unit | Today | After handlers80 |
| --- | ---: | ---: |
| `LOAD_FAST_BORROW` removed | −36 | −25 |
| `LOAD_COMMON_CONSTANT` removed | −37 | −25 |
| kwnames `LOAD_CONST` removed | −52 | −40 |
| `LOAD_GLOBAL` NULL slot removed | −11 | −11 |
| CALL entry without `call_transaction_begin` | −8 | −8 |
| reservation without `saved_top` | −5 | −5 |
| operand word load and advance | +3 | +3 |
| local or constant operand fill | +9 | +9 |
| temporary operand move (self included) | +7 | +7 |

| Case | Today | Handlers80 only | Arithmetic of the operand change (today's units) | Operands, today | Operands + handlers80 | Upstream |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| `f(x, y)` | 430 | 389 (clang) | −2×36 −11 −8 −5 +3 +2×9 = −75 | 355 | 336 | 343 |
| `o.m(x)` | 396 | 360 (clang) | −36 −8 −5 +3 +9 +7 = −30 | 366 | 341 | 334 |
| `o.store(x)`, returns None | 495 | 461 (clang) | −30, then None from a slot −37 +3 +6 = −28 | 437 | 426 | 403 |
| Bound method `bm(x)` | 441 | ~419 | −2×36 −8 −5 +3 +2×9 = −64 | 377 | ~377 | 342 |
| Default argument | 501 | ~490 | −36 −11 −8 +3 +9 = −43 | 458 | ~458 | 479 |
| Keyword argument | 733 | ~711 | −2×36 −52 −11 −8 +3 +2×9, staging about −15 = −137 | 596 | ~608 | 558 |
| `__init__` | 1,589 | ~1,578 | −36 −11, `call_transfer_exact_args` −34 +3 +9 = −69 | 1,520 | ~1,520 | 1,202 |
| Callee local dies | 963 | ~952 | as `o.m(x)` = −30 | 933 | ~933 | 702 |
| `len(x)`, native | 211 | ~200 | −37 −11, arg release about −20, +3 = −65 | 146 | ~146 | 148 |
| Native method on a list subclass (per call) | 765 | | −36, window detach/retire and callable pin about −50, +6 = −80 | ~685 | | 463 |
| Polymorphic site | 794 | | −36 −11 +12; the generic Python continuation replaces `call_transaction_push_python` and `run_vector` (about −150, enabled by O but separable) | ~610 | | 541 |

Remaining gaps, by item, after this proposal:

- `__init__`: the shim and allocation.
- Callee whose local dies: slow retirement, 268 against 87 (execution79).
- Native calls: the refreshes (execution79 item 3).
- Polymorphic site: the attribute side.

Exact calls reach upstream's count or come within 10% of it.

### Kernels

Instructions per loop, as a share of checkpoint78's
(`scripts/estimate.py`, `raw/estimate.txt`). Each column is separate. The
handlers80 column repeats execution79's measured estimate, as a share of
*upstream's* total.

| Kernel | Handlers80 (execution79) | Operand calls, design O, today's units | O after handlers80 | W after handlers80 | Constant slots after handlers80 | Borrowing ceiling |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| DeltaBlue | 9.4% of up. | −1.1% | −0.8% | −1.1% | −0.7% | −0.18% |
| raytrace | 7.3% of up. | −0.7% | −0.5% | −1.0% | −0.5% | −0.15% |
| richards | 7.3% of up. | −0.5% | −0.35% | −0.8% | −0.3% | −0.27% |
| deepcopy_memo | 8.1% of up. | −7.7% | −5.3% | −5.1% | +0.6% | −1.8% |
| coroutines | 7.6% of up. | −5.4% | −5.4% | −5.6% | −1.8% | 0 |

The coroutines operand figure is almost all heap-caller work. It assumes 72
saved per exact call from a coroutine frame: 128 measured in
`call_push_moved_exact_args` and `call_transfer_exact_args`, less about 45
for the inline reservation and initialization, and 11 for operand work. The
deepcopy_memo figure comes from its `LOAD_FAST_BORROW` share: 11.8% of
instructions, and 84% of those words fill windows.

The two effects add roughly, but not exactly. Handlers80 makes every removed
dispatch worth less, and the operand handlers must themselves be call-free.
The constant-slot fill estimate is the least certain item; step 3 should
record it per kernel.

---

## 9. Migration plan

The steps are in dependency order. Each step deletes the form it replaces.
Gates are on correctness; instruction counts are recorded, not gated.
Agent-hours assume Sonnet workers on mechanical units and one careful
implementer per step for the core.

| Step | Content | Deletes | Test ladder | Agent-hours (estimate) |
| --- | --- | --- | --- | ---: |
| 0 | Handlers80 lands (prerequisite, its own workstream). New call handlers follow its continuation rule from the start | | | |
| 1 | **Child at the top.** Every Python push reserves at `datastack_top` and moves or DUPs from the window. One inline path for thread and heap callers. `saved_top` becomes `_PyStackChunk.value_top`. Files: `pycore_interpframe.h`, `pycore_interpframe_structs.h`, `pystate.c`, `ceval.c` (`call_bind_known_python`, `call_transaction_overlap_python`, `_PyEval_CreateInitFrame`), `bytecodes.c` `_INIT_CALL_PY_EXACT_ARGS`, `codeobject.c` clobber proof, `regcfg.c` `overlapping_call` and verifier rule, `frameobject.c` jump rule, `gc_free_threading.c` | `_PyFrame_PushOverlapping`, `call_push_moved_exact_args`, `call_transfer_exact_args`, `saved_top`, the overlap proofs | (1) `f(x, y)` loop; a generator calling a function; recursion deep enough to force `push_chunk`; a keyword call; `__init__`. (2) `test_call`, `test_extcall`, `test_generators`, `test_coroutines`, `test_frame`, `test_sys_settrace`, `test_gc`, `test_ir_*`. (3) Full suite at the end | 4–6: one implementer, plus 1 for test fan-out |
| 2a | **Encoding core.** `IR_OPERANDS` id, `_PyIR_WordLength`, executing-copy flags. Assembler emission. `check_words` rules. Walkers: `semantic_step_core`, validators, `instr_length` and its loops, `dis`, `get_ir`, `mark_stacks`, `_PyCode_InstrDepthBefore`, `ir.c` plans. Magic number bump | | (1) `dis.dis` and `get_ir` of one call of each family. (2) `test_dis`, `test_code`, `test_monitoring` (instrumentation walkers), `test_ir_*` | 4–6: about 12 walker units of 20 minutes, plus a cleanup pass |
| 2b | **Compiler.** `convert_instr` makes call inputs an operand list. `compact_op` drops windows for calls. `_PyRegPass_CopyProp` gains operand borrow fields. `check_folded_load` covers operands. `_PyRegPass_CallRoles` becomes "has a self operand". `_PyRegPass_ElideNull` drops `LOAD_GLOBAL`'s NULL. CALL_KW names become an IMM operand | Call windows in the compiler and verifier | (1) IR of `f(x, y)`, `o.m(x)`, `f(x, g())` (must keep the snapshot), `f(*a, **k)`, `f(a, b=1)`. (2) An eval-order test: `g` rebinds `x` through `sys._getframe(1).f_locals`; the call must see the old `x`. Also opcode-trace and INSTRUCTION-event writes, to pin the existing behaviour. (3) `test_compile`, `test_ir_*`, `test_generated_cases` | 3–4 |
| 2c | **Handlers.** `ir_generator.py`: the call family format is an operand list; a fast path for one operand word; a continuation loop beyond. About 30 handlers and their twins, one unit each. `ceval.c` call transaction becomes view-based (one careful unit) | `call_transaction_begin` recheck, `detach`, `retire`, `call_clear_owned_slots`, window publication, `_PyIR_CallInputHoles` | (1) The cases of execution79's `calls.py`, under `-X dev` on a debug build. (2) `test_call`, `test_extcall`, `test_class`, `test_descr`, `test_super`, `test_monitoring`, `test_sys_settrace`, `test_capi`. (3) Full suite on release and debug | 8–12 in all: about 30 handler units of 15–20 minutes in parallel, about 3 for the transaction, about 2 for cleanup |
| 3 | **Constant slots everywhere.** `_PyRegPass_ConstSlots` drops the loop-only and window exclusions and gains the cap. `_PyCodeExec.const_image` with an inline fill. Empty-return certification accepts constant slots | The loop-only rule; `LOAD_COMMON_CONSTANT` before returns | (1) `return None` in a loop body and outside one; `n - 1` outside a loop; `f(1, None)`; `code.replace(co_consts=...)` with a mortal constant. (2) `test_code`, `test_dis`, `test_frame` (hidden slots stay out of `f_locals`), `test_sys_settrace`, `test_ir_*`. (3) Full suite | 3–5 |
| 4 | Writer pays (only if the user chooses it; section 2) | | | 15+ |
| 5 | Borrowed arguments (only if the user chooses it; section 7) | | | 8–12 |

Recommended total, steps 1 to 3: about 23–34 agent-hours, or about 2 to 3
working days of wall clock with fan-out. Record instruction counts per
kernel after steps 1, 2c and 3.

Judder: step 1 alone makes exact thread calls about 7 instructions slower per
temporary argument (estimate), while coroutines improve. Step 2c recovers the
local and constant copies. The opposite order, operands first, would build
design W and then tear it down. That is a stopgap, so it is not proposed.

---

## 10. Risks and open questions

1. **O or W (user decision).** O deletes the overlap machinery and has one
   storage model. W keeps the overlap and is 0.3–0.5 points cheaper in
   instructions on three of five kernels (estimate), almost all of it from
   moving self. Recommendation: O.
2. **Writer pays (user decision).** At most 0.6–0.9% in raytrace and
   richards after handlers80. It needs a second image and a word-index
   translation in every observer. Recommendation: no.
3. **Borrowed arguments (user decision; changes the frame ABI).** The ceiling
   is under 0.3%, with hazards in GC traversal and frame copies.
   Recommendation: no, for now.
4. **Mid-line `f_locals` writes from INSTRUCTION events or opcode tracing.**
   These already see current values through folded register operands.
   Checked: an opcode-trace write makes `x * y + 1` return 2001 where
   upstream gives 21 (section 2). The opcode-event stream differs too: a
   folded load emits no event, so `h` reports four opcode events against
   upstream's five (`raw/trace-write-check.txt`). Call
   operands extend that behaviour to call arguments. Is it accepted, or must
   such tracing disable folding? Step 2b pins the current behaviour with a
   test either way.
5. **Header layout.** Removing `saved_top` takes the header from 112 to 104
   bytes and moves `phase_state`. External debuggers that read the offsets
   table adapt; any that hard-code offsets break.
6. **Estimate quality.**
   - The census is line-weighted (±20% against IBS).
   - Unit costs inside handlers are estimates from the ledgers, not
     measurements of new code.
   - The clang column is an attribution build that also changes other code
     generation.
   - The constant-slot fill cost is the least certain item.
7. **Magic number and `.pyc`.** The encoding changes `co_code`. Per the
   project rules this is not an admission gate.
8. **Free-threaded build.** Overlap is already disabled there
   (`ceval.c` 4018, 4326). O makes both builds take the same path. Its GC no
   longer double-visits cells.

---

## Summary

- **Recommendation.** Calls carry their inputs as trailing operand words, four
  slots per word, with the length from the head's count. The child frame
  always starts at the arena top. Constants become constant slots wherever a
  register operand reads them. The evaluation-order rule and the caller-owns
  frame ABI stay as they are.
- **Per call** (estimates, after handlers80):
  - `f(x, y)` 336 against upstream's 343, from 430 today;
  - `o.m(x)` 341 against 334, from 396;
  - `o.store(x)` returning None 426 against 403, from 495;
  - `len(x)` about 146 against 148, from 211.
- **Kernels** (after handlers80, on top of it): operand calls save 0.35–0.8%
  in DeltaBlue, raytrace and richards, 5% in deepcopy_memo and 5.4% in
  coroutines. Constant slots save another 0.3–1.8%.
- **User decisions.** Design O or W; writer pays (recommended no); borrowed
  arguments (recommended no); whether mid-line tracing writes must defeat
  folding.
- **Evidence.** `E59/calls80-design/`, in `raw/` and `scripts/`.
