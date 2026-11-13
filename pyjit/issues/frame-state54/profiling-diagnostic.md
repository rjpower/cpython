# Profiling qualification triage

Final classification: an **unreproduced full-run finding with unresolved
cause**. Root's bounded targeted, module and complete-package comparisons all
pass on both debug binaries. No control failure was reproduced, so this is
not a proven preexisting failure, a demonstrated fix, or load/timing noise.
Retain it as an open qualification limit separately from established baseline
debugger, LLTrace and GC-verifier diagnostics.

Read-only source diagnosis of the cold candidate's debug full-suite failure.
No implementation, expected-output change, quarantine or native execution by
this worker. Source remains frozen at cold candidate
`61015a9452d9069646fb5fede9ff963896833b89`; root owns targeted comparison with
selected51 `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`.

Evidence is
`BASE/pyjit-link-wave1/frame-state54/cold-construction-debug-full-debug-full.log`
around lines 997 and 1047. BASE is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.

## What failed and why it is discovered here

The failing assertion says `Stats.print_stats output for Profile`, and its
implementation is `Lib/test/test_profile.py:ProfileTest.test_cprofile`.
Despite the method name, this class uses pure Python `profile.Profile`, not
the C tracing profiler. CProfileTest inherits the same method name, so the
name alone does not identify the profiler implementation.

`test_profiling/__init__.py` calls `load_package_tests`, which discovers the
package's test modules. `test_tracing_profiler.py` imports `ProfileTest` into
its module namespace and defines `CProfileTest(ProfileTest)`. The unittest
loader scans every TestCase class attribute; it does not filter imported
classes by `__module__`. Therefore the imported base ProfileTest is also
executed as part of test_profiling. This is real discovery, not a mistaken
traceback from an unrelated module.

Both classes share `test.profilee` function/code objects. Their inherited
run/runctx tests and deterministic output tests call the same testfunc,
factorial and mul functions, and enable/disable profiling. Module class-name
discovery places CProfileTest before ProfileTest absent later suite reordering.
Shared code thus can have an earlier execution/instrumentation history before
the imported base test runs. The exact full-suite ordering and causal role of
that history are not yet demonstrated.

## What the numbers establish

The workload uses a fake integer TICKS timer. ProfileTest requires a delta of
1000 before comparing outputs; the failure occurred in the later output
comparison. Thus the changed counts/times are not sampling noise, load or
wall-clock jitter.

The source calls factorial(3) twice and factorial(14) once. Correct observed
counts are 23 total/3 primitive factorial calls and 20 mul calls. The log has
20 total/3 primitive factorial calls and 6 mul calls, with recursive caller
counts reduced from 20 to 17. Six mul calls equal those from the two small
factorial(3) evaluations; fourteen is the size of the final factorial(14)
unwind. This numerical alignment suggests incomplete profiling event
accounting during the longer recursion, but does not prove exactly which
events were omitted or why. It is not evidence that fourteen Python calls
failed to execute: TICKS still advanced by the expected total.

The pure profiler enables its dispatcher with sys.setprofile, executes the
workload and clears profiling in finally. It reconstructs counts from event
dispatch and recursive caller accounting. Shared warm code and monitoring
enable/disable sequencing are plausible areas for a targeted comparison;
they are hypotheses, not a confirmed mechanism.

## Scope and next evidence

The package, test_profile.py, profilee.py, profile.py, unittest loader,
instrumentation, legacy tracing, ceval and all hot handler sources are
unchanged against selected51. The three cold public-construction changes do
not provide a source explanation for this ordinary recursive profile workload.
This alone does not establish a preexisting failure: await root's matching
debug control reproduction before classifying it.

A bounded comparison should distinguish the isolated imported ProfileTest
method from the full tracing-profiler module and package, preserving their
test ordering and initial code history. Record actual command paths, source
identities and output counts for both debug binaries. Do not regenerate the
expected statistics or change profiling code in this cold correctness fix.

## Completed bounded comparison

Root ran the following fresh-process comparisons, with command arrays, working
directories, per-run logs and exit codes retained under
`BASE/pyjit-link-wave1/frame-state54`:

| Journal | Scope | selected51 debug | cold debug |
| --- | --- | --- | --- |
| `profiling-repeat-commands.json` | Three alternating runs of `-B -m test -j1 -v --match '*test_cprofile' test_profiling.test_tracing_profiler` | 3 passes | 3 passes |
| `profiling-module-commands.json` | `-B -m test -j1 -v test_profiling.test_tracing_profiler` | pass | pass |
| `profiling-package-commands.json` | `-B -m test -j1 -v test_profiling` | pass | pass |

All ten subprocess exits are zero. Executables are
`BASE/round51-golden-debug/python` and
`BASE/frame54-cold-construction-debug/python`, each invoked from its matching
build directory. Source identities are those listed above. The targeted logs
are `profiling-{golden51,cold}-repeat{1..3}.log`; broader logs are
`profiling-module-{golden51,cold}.log` and
`profiling-package-{golden51,cold}.log`.

The original candidate full run still has one real failure. Passing these
comparisons does not erase it or prove which full-run condition caused it;
the proposed shared-code/instrumentation-history mechanism remains unproven.
No additional jobs, production changes or expectation updates are requested
by this diagnostic. This worker only read root's evidence and updated this
document.
