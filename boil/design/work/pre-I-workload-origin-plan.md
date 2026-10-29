# Upstream baseline: actual-driver origin campaign

Status: source-only plan, 2026-10-29. Diagnostic/correctness closure058d9bfb
clears independent actual review9bac61da. This next campaign has **not run**;
its bounded adapter and trace-parser successor still need implementation,
independent review and an exact coordinator grant. Production run.py and
driver.py remain unchanged. No workload timing is proposed here.

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

Before launch review, host-only fixtures should exercise a direct-driver
trace without a witness marker, its shared workload and regex helpers,
an early foreign import, unbound cache, missing cleanup, a loader search
candidate versus actual load, and an unexpected external library. Reuse
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
