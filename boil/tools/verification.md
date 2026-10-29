# Executable verification profiles

`verify.manifest.json` defines named selections, conditions, dependencies
and exclusions. `verify.py` implements strict validation and plan display.
The separately reviewed launcher executes fixed regrtest forms and records
actual process state. Profile orchestration and numeric-gate integration
remain unfinished. The coordinator owns integration and promotion of
pending prerequisites.

```
uv run --no-project --python /usr/bin/python3 -- python boil/tools/verify.py validate --inventory build/manifest-20261029/case-inventory.json
uv run --no-project --python /usr/bin/python3 -- python boil/tools/verify.py show stress11-debug --build build/integration-debug
uv run --no-project --python /usr/bin/python3 -- python boil/tools/test_verify.py
```

A valid schema is not a passing gate. `show` always labels its output
plan-only and runtime checks unperformed; it returns 2 for declared pending
prerequisites, a missing executable, or missing required case inventory.
`run` is not implemented and is rejected. The verifier and stress getters
are integrated; their actual state in a selected binary is still untested
by this display. Pending queue, precise-entry-path and full-stress-policy
requirements cannot be turned into green results by leaving out a hook or
narrowing a selection.

The schema has no commands, shell evaluation, profile inheritance or
fallback getter/setter. Unknown fields, versions, modes, profiles,
modules, references, zero selections and duplicate JSON keys fail. Full
profiles have no module/match fields, run once through numeric gate2/3,
and retain suite root-counter checking. Numeric gate3 is still a debug
full suite plus test_cext, regeneration and traversal audit.

The reclamation inventory is 31 concrete test IDs from 23 method-family
rows across 11 physical modules: nine logical groups, with multiprocessing
expanded into fork, forkserver and spawn process modules. The historical
880-run record used 13 selections (including the whole tracing-GC class)
and nine modules, with only forkserver processes. That historical subset
is preserved separately; it does not attest the expanded qualification.
All concrete IDs were discovered with the existing 99435658509 binary,
not invented from names. Discovery argv and exact matches are retained in
`build/manifest-20261029/reclamation-discovery-final.json`. This is the raw
discovery record, not the parser input. The normalized inventory is
`build/manifest-20261029/case-inventory.json`; pass that path to
`validate --inventory` or `show --inventory`.

`validate --inventory PATH` also verifies concrete selector membership,
module source hashes and declared factory dependencies. The inventory
object has exactly `revision`, `binary`, `cases` and `source_hashes`.
Missing evidence and source or selector drift fail; static validation
without inventory explicitly reports that membership was not checked.
Rediscover after test-source changes. Validation never runs the tests.

The accepted QUEUES ASan14/stress11 lists are in the registry, including
ctypes. Stress11 has the four previously approved writer/metadata
selectors and no module-wide test_misc exclusion. The ASan14 overflow
filter names only `AddressSanitizerTests.test_buffer_overflow`, preserving
the five allocator tests that share the method name. It does not erase the separately recorded unfiltered ASan smoke
failure. Source skips are metadata, not additional CLI filters; the
shared-heap callback decorator remains tied to 7a1/10a2b. Every exclusion
has its selector, status, scope, evidence, owner and removal condition.
The once-only full stress100000 profile remains blocked until its build
and selectors receive a separate policy decision.

## Proposed execution hook, for review before implementation

Keep gate.sh in charge of locking, slots, timeout groups, process cleanup,
fresh pyc caches and existing numeric gate2/3 semantics. A future runner
validates a named profile and writes a resolved plan with manifest, source,
binary/configuration and launcher hashes into its own evidence directory.
The proposed hook is `GATE_VERIFY_PLAN=/absolute/plan.json`; ordinary
numeric calls without it retain their current behavior. The hook accepts
data only, not arbitrary command text or eval. Every launcher validates
the profile and hashes against the frozen manifest/plan before execution.
An unavailable profile fails before a test can start.

A small hook should replace only regrtest's direct Python invocation with
the fixed `verify_launch.py --plan PATH --role driver -m test ...`,
retaining the literal `-m test` arguments so
gate_run.sh still recognizes the machine-slot operation. The selected
build's Python runs that launcher. The same launcher asserts actual
verifier mode/extras and actual gc_get_stress in the process that executes
the driver. A separate `python -c` probe is insufficient as its witness.
Gate3's auxiliary test_cext stage uses the same contract and a distinct
stage ID; regeneration and audit commands remain unchanged.

For workers, supply the fixed adapter through regrtest's existing
`--python` pair. A small adapter forwards interpreter flags before the
Python launcher, rejects unsupported argv forms and passes worker JSON
unchanged. Its target is the known test.libregrtest.worker module, never
an arbitrary manifest command. Named full profiles reject user-added test
files, match/exclude options, worker overrides and conflicting ambient
verification/stress settings. The old numeric interface stays available
for explicitly ad-hoc gates.

Before runpy executes driver/worker, and again in finally, read both true
getters. Record run/profile/stage ID, role, PID, actual mode/extras/stress,
flags, executable, source/config/manifest/launcher hashes and the worker
JSON hash plus its exact tests and match filters. Capture this data before
runpy mutates argv. Preserve module errors/SystemExit and nonzero results;
an end-state assertion may make a run fail but must never turn failure
into success. A missing end record after a worker crash is a failed run.
Missing getters fail; no setter repairs a supposedly inherited mode.

Uniform witnesses cover actual regrtest driver/workers. Deliberately
isolated or custom test-created children have their own hazard-specific
mode assertions; there is no blanket claim about all Python descendants.
Start/end checks detect lasting state changes, not every transient value
inside a test. Temporary stress changes must restore the real prior value.

A required run must fail if its selected build lacks a prerequisite,
configuration qualification or witness. Bind witnesses to scheduled
worker test lists and retain stdout/stderr and counts. Keep root_stats'
missing-file, empty/no-collection and fallback failures. Unpinned counts
remain report-only for suites; stale100 has its separate zero oracle.
Implement this runner only after the coordinator reviews the hook and
qualifies launch behavior with ignored-environment, changed-state,
missing-hook, worker-failure and missing-evidence controls.

## Launcher checkpoint (stage 2a, reviewed for landing)

`verify_launch.py` and `verify_worker.sh` implement the fixed driver,
worker and `-m platform` auxiliary entry forms. This checkpoint adds no
`run` command or numeric-gate hook. A resolved launch plan binds the
registry/profile, build and configuration, launcher hashes, selection
index, supplied source provenance, evidence file and stage. The launcher checks
exact driver arguments derived from the registry; a full profile cannot
accept an added test or filter. Build-kind and assertion checks use the
selected interpreter's actual configuration. Every named verification
profile requires Python assertions; -O/-OO is rejected for release builds
too. Native compiler O1 is a separate setting.

Driver and worker witnesses read both true getters before and after
runpy. A lasting mismatch or missing getter fails; no setter repairs
state. Module exceptions and nonzero SystemExit propagate. The known
platform-reporting call has role `auxiliary` and contributes no test
coverage. Unknown forms emit failure evidence. The adapter preserves
explicitly allowed interpreter flags before entering the launcher and
routes unsupported options to a rejection record rather than executing
an arbitrary module or code option.

The driver instruments regrtest's existing `create_worker_process`
binding to retain the scheduled PID, exact test/filter list and serialized
worker-config hash. Worker witnesses carry the corresponding hash and PID.
This will let the orchestrator reject a missing or substituted worker
witness, rather than accepting an incomplete set of start/end records.
Evidence appends use a file lock across driver and worker processes.

The eight launcher host control groups cover state changes, missing/ignored hooks,
module errors, auxiliary classification, unknown forms, full-profile
narrowing, changed plan identity and flag placement/code-option rejection.
These host controls use mocked state where stated. The bounded actual
CPython qualification below is complete. The next checkpoint must add run
orchestration, scheduled/witness reconciliation, counter checking and the
small numeric-gate hook before any named profile can claim execution.


The actual launcher controls are retained under
`build/manifest-20261029/launcher-stage2a/`. `qualification-v3.log` and
`audit-v3.log` record real off/kill driver+worker pairs, the known platform
auxiliary, ignored-environment rejection, unknown-module rejection,
malformed-worker execution failure, an injected lasting stress change,
and an injected missing getter. The negative mutations occur only in
control children; production launchers contain no setters. The positives
run one discovered exact case through a separate control registry and
preserve `-Werror` and `-P` inside the actual worker. Both positive root
records contain 57 collections across two processes, without fallback or
unpinned results. This qualifies the launcher component, not a named full
suite or stress profile.

The selected binary is existing `build/dev-debug/python`, whose actual
native version is 02bcd2f6132. Source identity and the scoped source hashes
are retained in `source-context.json`; H3 stage1 changed tools/tests while
native products stayed identical. `controls.manifest.json`, its normalized
inventory, exact resolved plans, worker JSON/config identities and all
stdout/stderr are retained. Earlier drafts are preserved under v1/v2;
v1's control driver stopped on its own argv-indexing error, corrected
before v3. All final safety controls and Ruff pass. No primary build was
created or reconfigured.


Review clarification: source_revision/source_sha256 are supplied provenance
at this launcher-only checkpoint, explicitly labeled as unverified in each
witness. Binary/Makefile hashes and actual build-kind checks do not attest
that the binary was built from that supplied source. Run orchestration must
validate the complete source snapshot before and after a run, enforce the
ASan environment, reconcile every scheduled worker and check counters
before it can report a gate. Repeated module-order profiles may launch the
whole eleven-module set together, preserving each module in a fresh worker;
individual-case profiles still require an explicit case index.

Review controls: the repeated-module profile resolves to all eleven module
arguments with no case filter, while a missing individual-case index fails.
All fifteen host safety groups pass. The actual release `-O` negative
control fails before module execution with Py_DEBUG=0 and optimize=1; its
plan, witness and diagnostic are `release-O.{plan.json,jsonl,log}` in the
launcher evidence directory. Earlier off/kill and auxiliary controls remain
applicable; this review change does not repeat unrelated runtime tests.
