# RC successor source closure

Source-only grant ends 2026-11-01 05:07 UTC. Starting tree is clean
`1440f8c6fd1`, the explicit coherent core restoration from `b5541cbbbcd9` onto
the maintained documentation/evidence history. Primary and tracing branches
remain untouched. No compiler, target, benchmark or probe is authorized here.

The first closure increment will select the existing reference-counted branch
of the historical core rather than invent new ownership rules. Resolve owning
`Py_GC` preprocessor alternatives to false, remove `Python/gc_live.c` and its
Unix/Windows build entries, remove `--with-gc` and its generated definition,
and prevent externally supplied owning-GC macros from silently changing policy.
Keep the separate historical diagnostic verifier: it observes a completed RC
cycle collection and neither owns nor frees objects. Its optional diagnostic
mode is not a collector selector.

Retain the GIL RC policy, owned/borrowed NaN-boxed stack references, optimizing
register compiler, cyclic collector and normal debug/stackref checks. Preserve
real decref reentrancy and existing RC clear-before-callback behavior. Do not
copy current tracing-only call-effect metadata into this restored generator.
The public pointer-based object/container/native ABI remains the restored
version; later public PyValue/container/native-slot work stays deferred.

Give this successor a distinct extension ABI and bytecode cache namespace,
including a different magic token, so neither present tracing artifacts nor
unrelated upstream modules/caches are accepted accidentally. The first native
manifest must still select historical boxed shapes and bind its exact source,
configuration and fresh products.

Host checks will audit the conditional projection, unresolved owning symbols,
build/header references, generated configure consistency, Python syntax and
existing generator/ownership controls. Projection resolves only the owning
collector macro; other configuration branches keep their original bodies.
Native gates remain separate: immediate destruction/weakrefs, cycles and
resurrection, borrowed stackrefs and reentrant decrefs, generators/frames,
native ownership and extension ABI, embedding/reinitialization/subinterpreters,
then compiler/interpreter and port qualification. Historical gate results are
evidence of a coherent ancestor, not a new qualification result.

The fixed GC decision deadline is 15:14:44 UTC. This isolated fallback can be
prepared without promoting it or disturbing the live tracing attempt.
