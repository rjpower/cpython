# Existing evaluator samples: source/assembly join

Offline inspection only. Joined the existing four accepted `stacks.txt`
files and root's source-line reports to the exact ce2ac release ELF
`boil-10a/build/default/python`, SHA256 eafcda7aba3e97eacf1cc3fb249c7e3b9c2f68117174f1f448d7e181dd752b40.
The binary still matches the original attribution pre/post binding. No
target, perf recording, build or benchmark was run. `objdump -d -l` read the
existing ELF; only bounded joined sites are retained in evaluator-sites.json.

The join uses the sampled leaf IP's physical evaluator owner, including
inlined leaves only where the stack records that **same IP** in EvalFrame.
It does not charge callers' inclusive samples to the evaluator. The earlier
direct-leaf-symbol-only extraction is retained separately and is incomplete
for inlined helpers; conclusions below use the complete same-IP join.

| Row | Samples | Evaluator leaf weight/all weight | Leading evaluator sites (share of all weight) |
| --- | ---: | ---: | --- |
| generators | 1,154 | 51.13% | SEND_GEN trailing dispatch16905 5.46%; generator receiver type check16870 3.99%; landing pad attributed to16826 2.95%; feedback-entry arithmetic2.77%; YIELD instr_ptr store18892 2.60% |
| coroutines | 2,337 | 27.30% | frame return-slot copy1.50%; compact-int guard1.41%; object-to-value float check0.94%; evaluator dispatch16517 0.81%; SEND_GEN exception-link store0.77% |
| comprehensions | 2,438 | 21.12% | list receiver guard9186 1.35%; object-to-value float check0.90%; type load0.86%; dispatch/landing9159 0.66% |
| richards | 604 | 79.47% | feedback-entry arithmetic8.44%; polymorphic shape-row match6.46%; conditional bool guard2.98%; truth fast guard2.48% |

These are period-weighted shares of the original cpu-clock:u samples, not
fractions of wall time, operation costs, branch misses or predicted savings.
They complement rather than replace the accepted GC/root attribution.

Concrete generated sites use ce2ac's source, not shifted e9 line numbers:

- generated_ir_cases.c.h:16905 is SEND_GEN's final DISPATCH. Hot actual
  instructions include the target-table `lea` at ELF0x458c3b and indirect
  `jmp *%rax` at0x458c49. :16870 is the PyGen/PyCoro exact-type admission
  comparison (0x458bb1). The 2.95% source bucket at:16826 actually samples
  `endbr64` at0x44b67a, the adjacent handler landing pad; treating that
  compiler source attribution as the cost of a distinct source operation
  would be wrong. No inference that endbr64 itself consumes that fraction.
- YIELD_VALUE:18892 updates the suspended frame's instr_ptr; the hot store
  is0x44fead. :18920 LOAD_IP(1) reloads the resumed parent IP, followed by
  :18924 dispatch. These are required frame transitions in the non-tail
  evaluator, not the tail-only handler-entry `tstate->current_frame` reload.
  :12247 is the backward-no-interrupt jump handler's dispatch.
- pycore_ir.h:281 computes `entries + i*FB_STRIDE`. Richards' hottest
  instructions are `lea`/`shl` (0x44ad3c/0x44ad43), not a load of current_frame.
  pycore_interpframe.h:63 is `_PyFb_ShapeRow` comparing shape/version rows,
  also not `_PyFrame_GetCode` or a frame-pointer reload. Coroutines':249
  is frame-copy `dest->return_slot = src->return_slot`.

The same-IP evaluator join had no unmapped evaluator IP in these files.
That is not complete process symbolization: libc's prominent0x19842d bucket
still has no source interpretation here, and non-evaluator paths were not
reclassified. DWARF can assign several instructions/inline call sites to one
line; static line attribution cannot identify an executing opcode frequency.
The time-based interrupt samples can skid, and an indirect-branch landing
point can reflect preceding front-end work. There are no IPC/branch-miss/
cache counters in this event. Richards has only604 samples; small buckets
and differences between these four rows do not support fine cost rankings.

One shared question for a later bounded experiment: **does dispatch topology
materially explain the non-GC evaluator gap?** A same-Clang computed-goto
baseline versus genuine preserve_none tailcall dispatch, with identical IR,
feedback/value rules and frame/root publication contracts, could address
that question. Establish compiler effects separately; GCC non-tail versus
Clang tail is not a matched dispatch comparison. H1's tail-only frame reload
work cannot by itself explain these measured GCC non-tail sites. This is a
question, not an experiment/implementation grant or a predicted gain; no
benchmark-specific opcode fusion follows.

Inspected ce2ac generated source SHA256:
9d7120660e93095063f9caa516e910267e79d2b787b78571fb6efa86f1dc0c6e;
pycore_ir.h ed40912bb116d725c524ef7c9cfdff5395eb6079a1d3b7444a7c8c09c1b72319;
ceval_macros.h e1ff21c108e1e8081926711b9baa9a4978b53cd1522192f3a86a54b29138741f.
