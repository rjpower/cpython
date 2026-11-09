# Sixth performance source review

This is a read-only location review of the retained `c1424be11db4` candidate.
The `sixth-logging-perf-*` and `sixth-nbody-perf-*` evidence is under
`build/pyjit-link-wave1/frame-call-model-20261109` in the runtime checkout.
The PMU summaries have 18 logging and 12 n-body valid invocations at 100%
event running. The separate cycle records have zero lost samples and stable
recorded binary identity; their roughly 2,000 samples per role locate work,
not a precise per-handler speed ratio.

| Workload and comparison | Cycles | Instructions | Branches | Branch misses |
| --- | ---: | ---: | ---: | ---: |
| Silent logging, candidate / upstream | 0.9182 | 1.0450 | 1.1032 | 0.9970 |
| N-body, candidate / golden | 1.0409 | 1.0333 | 1.0586 | 1.0402 |

Logging's extra branches coincide with *fewer* cycles and essentially the
same misses; branch count alone does not establish a regression. N-body's
3.33% extra instructions and 4.09% extra cycles justify source work, but the
record does not isolate a cause in the recent cold owner/observer changes.
The earlier flat elapsed screen for `4a1f70f` does not negate this PMU result.

1. **Carry canonical-home knowledge through hot duplication/publication.**
   N-body's candidate samples put 12.66% in `LOAD_FAST_BORROW`, including
   2.86% attributed to inlined `_PyValue_Publish`; the retained candidate and
   golden disassemblies both show exact `PyLong_Type`/`PyFloat_Type` tests after
   duplicating a local. `Python/bytecodes.c:LOAD_FAST_BORROW` duplicates an
   already canonical frame home, while `IR_STORE_DST` publishes it again.
   Silent logging also attributes 1.41% of the 13.94% method-with-values
   handler to publication of a guarded method descriptor. A small, proved
   producer fact at these existing boundaries could select a direct canonical
   store and remove the repeated type test. The proof must cover exact boxed
   numerics, local replacement and error ownership; the descriptor guard does
   not by itself prove every arbitrary producer canonical. Inspect emitted
   GCC code and measure each route before extending this fact beyond audited
   cases. Both n-body binaries have the local-load check, so this is an
   improvement opportunity, not an explanation of the candidate/golden gap.

2. **Reduce the work in the existing float-specialized path.** N-body's
   candidate cycle record places 14.31%, 9.45% and 8.30% in multiply, add and
   subtract float handlers. Inlined `PyValue_FloatOperandValues`,
   `PyValue_FromDouble` and float-tag construction appear within multiply;
   `Python/bytecodes.c` invokes the same operand conversion and NaN-capable
   result helper in all three. A narrower guarded float/float path could avoid
   repeated mixed-int classification while preserving the mixed-inline-int
   and boxed-NaN exits. This is a hypothesis to test against generated code
   and workload operand mix, not a measured gain or permission to weaken
   exceptional ownership. The retained golden profile has nearly the same
   hot-handler ordering, so flat sample shares cannot assign the 3.33%
   instruction gap to one arithmetic helper.

3. **Keep call-binding work visible as a separate logging target.** The
   candidate record places 11.54% in `CALL_PY_EXACT_ARGS` and 10.64% in
   `CALL_PY_GENERAL`; 4.43% of the latter is attributed to inlined
   `call_bind_known_python`. `Python/bytecodes.c` already has a guarded exact
   overlapping-frame route and a known-general binding route. The next
   bounded question is whether the frequent `logger.debug(m)` shape still
   performs binding work that a versioned, fixed positional/default layout
   can settle once. Retain existing frame initialization, overlap and error
   phases until an actual call-shape census and assembly comparison show a
   removable operation. Frame clear and method lookup also have substantial
   samples, so a binder-only win cannot be inferred from the 10.64% handler
   share.

These are ranked candidates for measured engineering, not patches or
qualification claims. The two workload comparisons use different controls
(upstream for logging, golden for n-body); their percentages must not be
subtracted or pooled. The cycle records are diagnostic location evidence,
whereas the PMU ratios above come from the valid repeated stat invocations.
