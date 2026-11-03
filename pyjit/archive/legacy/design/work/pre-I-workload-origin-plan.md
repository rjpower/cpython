# Upstream baseline: actual-driver origin campaign

Status: all50 direct run-mode origin rows passed and clear independent
actual reviewcbcb847d, 2026-10-29. Closure9e725b7f preserves raw traces,
receipts and unchanged input/native inventories. Production run.py and
driver.py remain unchanged. No time-mode, performance metric or extra
build ran. Earlier source-only/launch requirements below retain their
historical scope.

The frozen plan is primary
`build/checkpoint-i-baseline-binding/workload-origins-v1/source-plan.json`,
SHA256 `4f1c1423289d45fbc154ee1f00ca301096109311b1f5c106c4b32dade73c0164`.
Its50 expanded commands come directly from the existing unmodified
BENCHMARKS registry: all29 application kernels, all8 memory rows and13
existing call/string micro rows. The latter two groups are diagnostic
coverage, not additions to the29-workload performance score. No new
workload, reduced loop count or fabricated variant is introduced.

The44 shared input files have exact resolved paths and SHA256 values in
that plan: run.py's registry (`f8abc0b3`), driver.py (`7e61be5c`), selected
workload files, and both regex corpus helper modules. The source-only
builder imports only the registry under the existing host interpreter;
it never imports or runs workload modules. Own upstream source5806,
native584 products, accepted make/configure and qualification closures
remain the other input authorities. Space check found214GiB free.

## Exact command and environment

Let B be the owned upstream321925 build/default, P the primary worktree,
and Q primary build/checkpoint-i-baseline-binding/workload-origins-v1.
Every expanded row is:

```
B/python -S -v -X pycache_prefix=Q/run/NAME/pycache
  P/boil/bench/internal/driver.py run BENCHPATH LOOPS 1 [VARIANT]
```

cwd is P. Each child receives only PATH=/usr/bin:/bin, HOME=/home/power,
LANG=LC_ALL=C.UTF-8, TZ=UTC, PYTHONHASHSEED=0, LD_DEBUG=libs,files and its
own TMPDIR=Q/run/NAME/tmp. Create fresh cache/tmp directories before that
row. No PYTHONHOME/PYTHONPATH, _PYTHON_SYSCONFIGDATA inputs, LD_PRELOAD,
LD_LIBRARY_PATH or site startup is inherited. LD_DEBUG is the explicitly
declared loader trace, not a performance setting. The future paired timing
policy must declare a common temporary-filesystem choice as well; XML
setup really writes and reopens a temporary file.

The existing driver loads the specified file as _boil_bench_module,
calls setup(variant) if present, then run(loops) once. The final1 is RUNS,
unused by run mode; preserving it matches Context.command. Retain stdout
and the complete Python-verbose/ELF-loader stderr separately for each row.
No perf, GNU-time/RSS, boxes or time-mode child is included.

| Application row | Loops | Variant when present |
|---|---:|---|
| richards | 2 | |
| go | 1 | |
| nbody | 1 | |
| float | 1 | |
| spectral_norm | 1 | |
| raytrace | 1 | |
| deltablue | 30 | |
| chaos | 2 | |
| nqueens | 1 | |
| fannkuch | 1 | |
| generators | 2 | |
| comprehensions | 6000 | |
| coroutines | 4 | |
| unpack_sequence | 4000 | |
| deepcopy | 300 | deepcopy |
| deepcopy_reduce | 20000 | deepcopy_reduce |
| deepcopy_memo | 4000 | deepcopy_memo |
| json_dumps | 10 | |
| json_loads | 200 | |
| pickle | 400 | pickle |
| unpickle | 300 | unpickle |
| regex_compile | 1 | |
| logging_format | 1000 | logging_format |
| logging_silent | 60000 | logging_silent |
| logging_simple | 1000 | logging_simple |
| xml_etree_generate | 1 | xml_etree_generate |
| xml_etree_iterparse | 1 | xml_etree_iterparse |
| xml_etree_parse | 1 | xml_etree_parse |
| xml_etree_process | 1 | xml_etree_process |

All13 selected micro rows use registry loops200, inner1000 and no variant:
method_call, call_len, call_abs, call_math_sin, call_py_1, call_py_3,
call_py_kw, call_closure, call_instance, generator_resume, str_join,
str_concat and str_concat_loop. These cover the existing call forms and
fixed-size string cases; they do not establish string growth scaling.

All8 memory rows use loops1: startup, imports, tuples_10m, instances_1m,
dicts_200k and collector_live16/128/512. The three collector rows select
collector.py with variants16/128/512. Original sizes stay intact:10million
tuples,1million instances,200,000 dictionaries, and the collector's nominal
live heap plus1GiB short-lived allocation per row. These are real resource
loads despite collecting no performance metric.

## Reuse the existing origin rules with direct-driver scope

The current diagnostic verifier cannot be called unchanged: it requires
the diagnostic's report/completion marker and explicit backend list.
Factor its existing import-trace/file-binding logic into a small task-owned
routine; do not wrap or modify the target driver to imitate that marker.
For direct run-mode commands, require process return0, the source-reviewed
driver's straight-line setup/run return, complete startup/cleanup traces,
and a code-object trace for the exact selected workload file. The main
driver's path/hash is an explicit argv input. SourceFileLoader.exec_module
does not promise a normal import-completion line for _boil_bench_module;
do not reject valid direct loading merely for that absent line.

Expected filesystem origins are:

| Kind | Accepted binding |
|---|---|
| stdlib source and transitive imports | Exact U/Lib paths/content from owned source-inputs.json |
| builtin/frozen modules | Accepted upstream executable and generated frozen-source/product identities |
| stdlib extension and generated sysconfig | Exact B/build library aliases and B/Modules resolved products in the584-product inventory |
| driver/workload code | Named44-file shared manifest, not the whole primary tree |
| regex corpus imports | The two separately hashed kernels/data/bm_regex_*.py files; setup intentionally adds that data directory to sys.path |
| fresh bytecode | That row's new cache; trace source relationship must bind to one of the preceding source inputs |

Source imports inside setup/run matter: pickle imports its stdlib backend
in setup, XML selects the C ElementTree accelerator in setup, and regex
setup imports both corpus modules. A load-only or startup-only witness
does not close these paths. Unknown loaders, external source/extension
origins, unbound bytecode or incomplete traces stop the campaign. Existing
workload semantic checks remain useful but are not a universal result
oracle. The exact pyexpat namespace proof remains prior diagnostic evidence;
direct traces prove imports/files, not every module's runtime attributes.

LD_DEBUG must be parsed as actual loaded objects, not search candidates:
`trying file=` is not evidence a library was loaded. Preserve the raw
combined trace and bind observed link-map/init/fini/open records to resolved
paths/bytes. The first diagnostic's known external files are the host
ld-linux loader, libc and libm, with exact resolved hashes in source-plan.
Own executable/extension aliases must match the accepted product inventory.
Any additional external library requires explicit recorded disposition;
do not silently accept everything under /usr/lib. Kernel/vDSO metadata
and unresolved/deleted mappings retain their distinct limits. Host readelf
DT_NEEDED/RPATH/RUNPATH inspection may corroborate loader evidence; it
does not replace it or prove consumed SDK headers.

Host-only fixtures exercise a direct-driver trace without a witness
marker, its shared workload and regex helpers, an early foreign import,
unbound cache, missing cleanup, a loader search candidate versus actual
load, and an unexpected external library. Reuse
the existing valid wrong-origin controls' evidence where inputs/guards
are unchanged; do not rerun the18-module suite for a trace-parser change.

## Admission, closure and later timing

Use a bounded adaptation of the accepted V2 supervisor: one task/build
lock, one normal machine slot, retained inherited descriptors, bounded
children and final sweep. Propose campaign ceiling3600seconds, per-child
180seconds for application/micro rows and600seconds for memory rows.
Capture exact584 native products before, after and after cleanup, plus
unchanged source/tools/config and44 shared inputs. Fresh reports/caches
are outputs, not changes to the upstream build. Stop on any unexpected
failure; preserve partial evidence and obtain a revised exact grant for
a fresh attempt rather than retrying automatically.

Bind accepted qualification closure058d9bfb, its successful final/inner
receipts, raw diagnostic/correctness results and cache-access receiptdf8d.
Its old test-created0200 cache files are historical outputs, not inputs
to this fresh-cache phase; do not invent an ongoing need to chmod them
for every later launch. Review the minimal receipt carry-forward and
current native/source guard explicitly when preparing the adapter.

driver.py's time branch returns through a different tail: after warmup
and timed calls it imports json and prints durations. Run mode returns
before that import. Its exact source hash and the accepted diagnostic's
json/_json origins provide planning evidence, not per-workload time-mode
coverage. The later timing plan must handle that extra import/context
explicitly and remove -v/LD_DEBUG; no time-mode execution is granted here.

After actual-origin review and GC2b integration, take the first quiet,
properly bound elapsed baseline early. Complete workload origins do not
close SDK consumed-header provenance or upstream/fork eval-policy matching.
Keep source-specific-fno-gcse, any sensitivity/fully matched cohort and
the roughly2x pre-JIT target separate from this origin-only campaign.

## Implemented task-owned source package, not executed workloads

The coordinator granted source implementation of this plan. Q now holds
origin_trace.py, campaign.py and the bounded V2 phase_runner adaptation.
The original plan documentdedcca9e and first implementation/control bytes
are preserved under Q/history-first-draft. The original registry source
plan4f1c1423 remains unchanged and is checked against the final50 rows.

| Frozen source/launch | SHA256 |
|---|---|
| phase_runner.py | `bff785d1a898d28a435cbcac2b82cfc379873b7d7294360069c37a40b5a00117` |
| origin_trace.py | `c398218c5ef1ca10081e991c2864cc6f2e319fc1d0d725c8384e2748286c0e99` |
| campaign.py | `389a541739e25ca836088a595432c843ca4dd0525207253829af8e75fd340e26` |
| plan.json | `b5b38e63937f971e90648e5874048d06533cb28a6eff12ab34c74eb4b30af0f6` |
| origin-launch.json | `23bf93c55c6dea16ddac6431cf95ccc3031170b30691bd546fbe4b11cb428099` |

Seven existing supervisor/lock/inventory/make-guard function bodies are
AST-identical to V2. New guards bind the50 exact rows,44 shared files,
15 current host tools (the14 build tools plus readelf),38 selected readable
qualification artifacts and the exact accepted qualification/cache
receipts. Old0200 test caches remain untouched. Serial campaign children
retain a command receipt even on timeout. Native584/source5806/config14
guards run before, after and after final cleanup under the ordinary protocol.

Static host readelf inspected the existing executable and67 extension
ELFs, without executing them. `elf-dependencies.json` SHA256
`faf853da941917bd50e881e6be530dec7bdb5907dcc18d292b5682035eed53fe`
binds all68 raw DT_NEEDED/RPATH/RUNPATH outputs and readelf's exact bytes.
The additional admissible external files are libffi, liblzma, libncursesw,
libpanelw, libtinfo and libz: each exact resolved hash was already in
pre-makeb960 **and** is named by an actual configured product's DT_NEEDED.
This is predeclared admissibility, not a claim those six were loaded.
The parser still requires actual selected link-map/init/fini/open records
and matching bytes; no arbitrary /usr/lib allowance is present.

Twenty-eight host-only controls pass at these final source hashes;
`host-controls-v4/results.json` SHA256
`d786c5cfc0e39a3827dbf0fd53ccf34cbb000ba862c22ab8e760d856b1352025`.
They include complete direct/shared-corpus traces, early foreign imports,
unknown loaders, orphan/wrong caches, absent workload/cleanup, foreign
resolved aliases, search-only foreign candidates versus selected foreign
loads, unresolvable sonames, wrong programs/extra processes, missing
extension ELF events, row/qualification/shared-file drift and missing
inherited locks. A harmless /usr/bin/true host process supplied an actual
glibc LD_DEBUG format fixture. Synthetic ELF fixture files are never run.
These are parser/guard evidence, not target workload or timing evidence.

Independent exact package review and a coordinator execution grant are
still required. The accepted18-module correctness run is not repeated.

Before launch, root and independent review found that the first parser
omitted generated Python/data products from its accepted file table,
despite the intended sysconfig allowance. The complete119-artifact frozen
package is preserved at primary build/checkpoint-i-baseline-binding/
workload-origins-reviewed-v1, with predecessor provenancef0aa73cc and an
explicit preservation receipt. The focused successor adds **all584
manifest-bound build products** to Python file binding, while keeping
actual ELF filtering and observed-load requirements separate. Added host
controls accept generated sysconfig code and reject changed bytes and an
unlisted generated file. Runner, campaign,50 exact commands, shared inputs,
qualifying18-module receipt and native products remain unchanged. No
target workload was executed to discover or repair this coverage gap.

## Actual run-mode origin closure

Independent launch review96308cd7 and the exact coordinator grant preceded
the once-only23bf93c5 launch. All50 rows returned0 and passed origin checks:
29 kernels,8 memory workloads and13 call/string micro rows, serially under
the normal slot/build lock. Final/gate/inner returned0 and final cleanup
verified identical584 native products. Source5806,15 tools,44 shared files
and38 critical qualification artifacts remained unchanged. Historical0200
test caches were neither opened nor chmodded. No time mode, perf/RSS/box
metric, build or18-module correctness rerun was included.

Joint `workload-origin-closure-v1.json`, SHA256
`9e725b7ff8aa5b06ede10e82c101015ef6b11d497727e782f0eed1bdce56f0ac`,
under primary build/checkpoint-i-baseline-binding binds1169 raw artifacts.
The union contains117 trace-observed Python files and118 import names.
Actual external ELF loads were only the host loader, libc, libm, liblzma
and libz, each matching its predeclared resolved bytes. The unused ffi/
curses/panel/tinfo declarations are not reported as observed loads.

Independent actual reviewcbcb847d accepts that run-mode scope, without
timing/parity credit. The [first quiet timing plan](pre-I-first-quiet-timing-plan.md)
is source-only and requires separate source/build/quiet-window decisions
and an exact grant. Extra time-mode JSON-tail imports and SDK/header/eval
policy limits are not silently closed by these50 run-mode traces.
