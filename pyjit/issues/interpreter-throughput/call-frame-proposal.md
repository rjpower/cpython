# Python call frame proposal

Status: proposed for independent review; no implementation yet. Scope: one
bounded general-call fast path, with the existing binder as fallback.

## Existing ownership and the proposed boundary

A thread stack chunk holds two stacks: frame headers grow down and value slots
grow up. A frame records its slot base and the value-stack top to restore on
pop. Exact-argument CALLs already push an overlapping frame: its base is the
first argument in the caller's window, its function field takes the callable,
and its argument locals take the existing window owners. The compiler verifies
that other live caller temporaries lie below a CALL window. A generator-owned
caller has slots outside the thread stack, so it cannot use this overlap.

The general transaction keeps its caller window rooted while a consuming binder
works on duplicated arguments. That matters: allocation, keyword comparison,
default lookup, error formatting, release and instrumentation can execute
Python, mutate the function or inspect the caller. It must not expose the same
reference as two owners or leave consumed bits in a caller visible to reentry.
Blindly passing the caller's array to the existing binder is therefore unsafe.

Add a fast path for a prepared positional CALL from a thread-owned frame with
no keyword-only parameters, with at most the declared positional count and
enough positional defaults to fill the rest. Support *args only when it is
empty: bind the immortal empty tuple. Support **kwargs by allocating a fresh
empty dict before the commit while the parent window still owns its inputs. Require an optimized Python function,
standard vectorcall, no PEP 523 hook, an already prepared executable, sufficient
existing stack space, and the CALL opcode's verified overlap invariant. Decline
all other cases to the existing general binder. Check that instrumented calls
satisfy the same verifier guarantee before enabling them; otherwise decline.

All fallible or reentrant work, including executable preparation, empty kwargs
dict allocation and their parent refreshes, precedes the final guard. The local
dict is an owned root, not yet installed in either frame. If allocation or
refresh fails, release it under the existing error/revalidation rules; if the
final guards fail, release it before falling back and refresh again if needed. Pin the code while preparing; after refresh,
recheck the callable, vectorcall, code identity, defaults and other guards.
Then, without an escaping call, transfer callable and argument ownership to an
overlapping child, initialize missing positional defaults with owned duplicates,
write empty *args and transfer the prepared kwargs dict if present, clear the caller's non-transferred window slots,
set return slot and publish child as current. Any releases after publishing must
follow the existing transaction's order and refresh/error protocol. The commit
phase must not permit a failing operation between detaching ownership and making
the child safely clearable. In particular, the current `_PyFrame_PushOverlapping`
selects `func_code` itself: either prove it still equals the pinned code at this
non-reentrant point or leave this fast path out. Return and error behavior must
match the normal call and exact overlap path, including observer/monitor events.

This scope handles empty *args and **kwargs, a candidate for logging.debug;
confirm dynamic coverage and benchmark it before accepting the complexity.
Nonempty *args needs tuple allocation and a carefully reviewed precommit or
staged ownership protocol. Keywords and general fallible binding remain with
the transaction. The fast path must not change error wording or release order.
If any constraint cannot be proved, reduce it to the plain no-keyword defaults
case or abandon overlap for that case.

## Files and validation

Initial implementation: `Python/ceval.c` only; call behavior tests in the
appropriate existing `Lib/test` module. The coordinator owns frame layout,
initialization/cleanup, compiler and generators. No changes to those interfaces
are assumed. Review the actual generated CALL/instrumentation mapping and
compaction proof before coding; changing the IR verifier is a separate reviewed
packet.

Tests: positional-only and regular defaults, empty *args, bound and unbound
method receivers, inline and mortal arguments, nested and recursive calls,
generator callers (fallback), enough arguments to trigger stack growth (fallback),
missing/extra/keyword errors, mutable defaults and code mutation during
monitoring or executable setup, frame introspection, and destructor/reentry
ordering on success and failure. Run relevant targeted tests in both release and
debug through coordinator builds; no full suite for this increment alone.

At integration, compare matched GCC JIT-off runs against the same pre-change
baseline, especially logging_silent and deltablue, plus a basic positional
call/defaults/empty-varargs microbenchmark. Record exact source/build identity,
commands, host load, repetitions and spread; inspect regressions rather than
claiming a win from noisy single runs. Coordinator schedules shared builds and
performance runs and decides whether the path earns its complexity.

## Reviewed commit map

The fast path is one preflight, final guard and non-escaping commit. Re-read
code identity, vectorcall, defaults and shape after the final allocation and
refresh. Initially decline instrumented calls; proving verifier provenance and
monitor-once behavior is separate. Never fall back after detaching an owner.

| Owner before commit | Owner after commit |
| --- | --- |
| caller window callable | child `f_funcobj`; caller callable cell becomes NULL |
| caller receiver, when present | child positional local 0 in the same cell |
| caller positional arguments | child positional locals in those same cells |
| caller NULL receiver, when absent | remains NULL outside the child base |
| function default tuple entries | owned duplicates in missing child positional locals |
| immortal empty tuple, with *args | child *args local |
| preflight's fresh empty dict, with **kwargs | child **kwargs local; preflight local relinquishes ownership |

Only non-transferred caller cells are cleared. The overlapping argument cells
are child homes and must never be blanket-cleared. No callback, allocation,
release or failure may occur until every transferred owner is in a clearable,
published child. Tests also check distinct kwargs dictionaries on successive
calls, retained/returned kwargs, caller inspection and mutation of code/defaults
during preparation, and fallback after disposal of a prepared dictionary.
