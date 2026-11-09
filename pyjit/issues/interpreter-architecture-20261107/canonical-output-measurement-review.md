# R5 paired shape-read measurement review

2026-11-07. **Accept the bounded comparison with the controls below.** This
refines the [measurement plan](measurement-plan.md); it is not an executed
measurement or a claim about R5 source that is still under implementation.

Compare the frozen R4 release executable with R5 in the same existing build
environment. A shape-read expression whose result uses a temporary targets
the changed publication site. A folded-local assignment is a useful control
because its final publication is deliberately retained. Treat the latter as
a specificity check, not a subtraction estimator: the forms have different
store and retirement traffic. The full kernel, subset and counter campaigns
remain necessary for broad effects.

The proposed hardlink under `build/python-architecture-r4-20261107` must keep
the R4 ELF frozen across the R5 rebuild. Verify its build ID and inode after
linking and rebuilding; do not assume the linker always replaces the output
inode. A distinct copy in that same build directory also preserves executable
path discovery without coupling the retained file to the rebuild's writes.
The retained `r4-gcc` ELF provides an independent identity reference.

Probe both executable roles after the R5 rebuild. Record native Git/build ID,
JIT-off state, build/stdlib/import paths and loaded DSOs; confirm the common
DSOs load under R4 and require no new R5-only symbol. Reusing an ABI label is
not sufficient evidence that the mixed executable/DSO combination works.

Use fresh child processes, fixed hash seed, identical warmup and fixed loop
counts within each pair, and balanced old/new then new/old order over multiple
rounds. Keep the load guard and full load tuples. After warmup and again after
timing, assert actual `LOAD_ATTR_SHAPE` selection and the instruction's
temporary/local destination class. Keep those inspections, imports and value
construction outside the timed region. Retain raw vectors, paired ratios,
spreads, process order, failed attempts and identity snapshots. Do not adjust
the paired result using sequential upstream-control drift.

Keep inline integers, ordinary floats, mortal objects, large integers and NaN
as separate result rows. Hold receiver and value owners through timing so a
destructor does not become per-iteration work. Ensure each measured reader
actually retains its intended specialization and result destination. If the
elapsed effect remains below noise, say so; the expected machine-code deletion
alone does not establish a speedup.

Collector source post-review: `measure-canonical-home.py` implements balanced
order correctly using the stable cell index plus round parity, independent of
the rotated cell traversal. It retains all per-process timing vectors and
paired ratios, inspects specialization/destination before and after timing,
keeps owner/value lifetimes outside the loop's final destruction, and checks
clean source, executable identities and common imported-file manifests.
These mechanisms are suitable for this bounded elapsed comparison.

I initially requested the historical method-JIT disable probe. That API and
backend are absent from the pinned current runtime, so requiring the old API
would be wrong. For these exact sources, explicitly assert and record that
`jit_method_natural` is absent from `_testinternalcapi` before and after timing,
along with the existing upstream-JIT-off assertion. Reintroduction of the API
must fail this collector until a new explicit recipe is reviewed. With that
current-source gate, no further methodological blocker was found. This review
does not turn a smoke run into a completed timing campaign.

Final collector inspection confirms both JIT-off assertions and the explicit
method-API-absence assertions before and after timing, with both absence
values retained in each child's JSON. The collector is accepted for the
bounded paired run. Results and any performance conclusion require a separate
evidence review.
