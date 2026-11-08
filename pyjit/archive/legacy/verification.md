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
by this display. Pending precise-entry-path and full-stress-policy
requirements cannot be turned into green results by leaving out a hook or
narrowing a selection.

Queue owner routing is available after integration052d8342e56 and its
detached checks. A selected build must still be bound to source containing
that landing; availability in this registry does not qualify an older
binary. The existing case inventory predates queue test-source changes
and must be rediscovered before actual case-profile execution.

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

## Verification identity and orchestration checkpoints

This section records the reviewed design. The identity/result prerequisite
is implemented; its bounded component qualification is recorded below.
Numeric-gate orchestration remains a later checkpoint. Registry selections,
exclusions and pending prerequisites remain unchanged. Neither checkpoint
changes native sources or gate_run.sh. Stale probes need a separate child
contract before their named profiles can run.

### Identity and result prerequisite

Version the internal launch plan to v2 and update its active callers without
a v1 fallback. Historical v1 plans and witnesses retain their original tool
hashes and launcher-only interpretation. The plan must distinguish:

- **Build source:** the canonical configured source directory, its revision
  and a recorded execution-input snapshot. Resolve it through the selected
  interpreter's actual configuration and build Makefile, requiring agreement.
  A supplied primary checkout is not a substitute: integration Makefiles
  currently name `/tmp/boilgate`.
- **Runtime source:** the actual uninstalled Lib/test and Lib roots used by
  the driver/workers, with their own input snapshot. Validate the registry
  and case inventory against this explicit root. Do not rebind verify.REPO.
- **Tools:** a run-owned frozen copy of the registry, inventory and executable
  verification tools, with paths and hashes. Freeze gate.sh, unchanged
  gate_run.sh, root_stats.py and the launcher/adapter/validator/orchestrator
  together. Hash every consumed tool; do not mix live and frozen siblings.
- **Build receipt:** an explicit recorded build command and its successful
  outcome, frozen source inputs before/after, compiler/configuration identity,
  and resulting Python, library and all built extension hashes. Configuration includes
  Makefile, pyconfig.h and the loaded sysconfig data. Extensions include the
  actual _testinternalcapi and test-support modules. The receipt must bind
  the build output to the source snapshot, including generated inputs.

A source snapshot, a current git HEAD, sys.version, Makefile equality or
`make -q` alone cannot establish that receipt. The initial implementation
consumes a coordinator-qualified receipt; it does not rebuild or reconfigure
an occupied build to manufacture one. Receipt creation/qualification is an
explicit prerequisite. Every real v2 launch, including component controls,
requires the same receipt/source/output checks; component-only describes
coverage, not weaker identity validation. Existing unqualified v1 binaries
and witnesses remain historical. No launcher component can produce a named
profile PASS or claim to test the current native tree. A prerequisite that
requires a native landing must be supported by the receipt's source identity,
not just by the registry declaring it available.

Version the source-snapshot scope. Start with all tracked files, excluding
only `boil/{README,PROCESS,WORKLOG}.md`, `boil/tools/verification.md` and
Markdown files under `boil/design/`; these concurrent documentation edits
do not invalidate a native run. Include executable files under boil, not the
whole boil directory as a documentation exclusion. Add explicitly admitted
generated/untracked execution inputs, including ignored generated headers;
record paths, symlink targets, file modes and content hashes. Reject unknown
untracked executable inputs in the source roots. Build/evidence outputs are
separate from source inputs. Preserve before/after manifests and differences;
compare the declared build/runtime inputs, not the active primary's status.
Compare execution inputs independently of HEAD; record both the receipt's
revision and the observed revision, so a commit of excluded documentation
can proceed while a changed executable input still fails identity checks.
The existing rule against changing executable source during a gate remains.

The coordinator granted the bounded launcher extension for these v2 fields
and actual origins separately from the future runner. Check actual
resolved origins and hashes for test, test.support, regrtest infrastructure,
sysconfig data and _testinternalcapi in each driver/worker. Check the scheduled
test module's actual object before run_unittest runs it; an import-time skip
records its resolved source and result instead. Do not import the test early
to discover its origin, nor infer the loaded Lib from an installation path.
Keep the worker.main runpy entry. Require the fresh run pycache prefix in the
verified driver/workers, so stale source-tree bytecode cannot bypass source
binding. Custom children remain outside this uniform witness contract.

Port the already exercised scheduler/TestResult.from_json PID/config/seed
binding and unittest addSkip ID/reason capture from ROOTS's build-owned
`build/takeover-20261029/{matrix_v2_assert_state,module_contract}.py`.
Do not import an untracked build helper into production tools. Its retained
module-contract controls reject corrupted real-evidence replays; that is
useful input, not new execution or shared-runner qualification. Adapt its
module-specific expected counts and skip policy to each qualified source/build
selection instead of retaining ROOTS's fixed eleven-module assumptions.

Add selection/summary capture through the existing regrtest reporter hooks,
forwarding the original reporter unchanged, and a plan-derived `--junit-xml`
path. Record the resolved run selection and each returned TestResult state/stats.
Retain raw XML for concrete successful test IDs and outcomes as well as the
actual skip records; do not parse a green console summary as execution proof.
The hooks must preserve module errors, nonzero exits and the existing reporter.

### Gate orchestration checkpoint

`verify.py run PROFILE --build BUILD --source-root SOURCE --receipt RECEIPT
--evidence DIR [--inventory INVENTORY]` delegates to verify_run.py. The source
argument must match the resolved build source, and evidence must be a new
run-owned directory. There are no extra test/filter/worker-command operands.
Validate all prerequisites, hashes, inventory and build conditions before a
test starts. Resolve the complete profile into hash-bound invocation plans:
one main invocation for module/full selections; one per concrete case for
individual selections; repeat the complete selection exactly repeat times.
The eleven-module order profile uses all eleven modules in one driver per
repetition, with each module in its fresh worker. Do not multiply that profile
into eleven filtered cases or call the historical 880 subset equivalent.

Use one canonical driver-argv constructor. The named gate hook receives its
resolved argv as ordinary process arguments and does not append GATE_IGNORE
or any other filter. This avoids the current mismatch: driver_arguments puts
`--python` after exclusions, whereas ordinary gate1 appends its ignore after
the caller's arguments. The launcher still requires exact profile argv.
Named runs reject conflicting ambient mode/stress/filter/worker overrides;
set requested startup environment only in run children, never through a
getter-repairing setter. ASan runs require exactly the approved effective
options, reject conflicting or duplicate settings, and witness that environment
in actual driver/workers. Python -O/-OO remain rejected; native O1 is separate.

Keep the ordinary numeric interface unchanged when no named plan is present.
With a named plan, check the requested numeric level/build against the plan
before execution. gate_run.sh still sees literal `-m test` in each command,
and retains slot-before-build-lock ordering, timeout groups and cleanup.
Take final preflight and postflight identity checks inside the existing build
lock, around the actual launcher, so a plan formed while waiting cannot attest
an intervening rebuild. Shared identity helpers belong in verify.py: the driver
checks complete input snapshots once before and after execution; workers check
their tool/build identity and actual infrastructure/test-module bindings. Avoid
rehashing the complete source tree in every worker. Do not take the same build
lock in the outer runner.

Named gate2 remains one full suite. Named gate3 remains one debug full suite,
then test_cext, regeneration and traversal audit. Create a distinct hash-bound
cext plan with stage=cext, its own witness/result/counter paths and the same
profile/build/source identity. Never mutate the main plan in place or weaken
the launcher's exact argv check. Run regeneration in the declared build and
compare the declared build-source snapshot before/after; run the audit from
that source tree. Snapshot checking and command outcomes must both pass.

Reconcile each invocation only after its processes have exited. Require one
driver start/end, every scheduled worker's matching PID/config/test/filter
start/end, and a result for every resolved selection. Bind returned results to
the scheduled PID, serialized config and random seed. Reject missing, duplicate,
substituted, wrong-stage and unexpected worker records. A known platform
auxiliary has paired state witnesses but supplies no test coverage. For an
individual case require its exact concrete ID to execute once without skipping;
for a repeated module profile require every required reclamation ID to execute
in its full module on every repetition. Record all other skips by ID/reason;
source skips never become extra CLI filters. Full-suite counts are measured,
not pinned to a historical total, and the complete resolved module list must
be accounted for even when platform/resource skips occur.

Use fresh counter files per invocation/stage. Require parseable main-interpreter
records for the verified driver/workers and retain test-created child records.
Keep root_stats' missing-file, empty/no-collection and fallback failures; keep
ordinary suite unpinned totals report-only. Record counts and skips together
with repetition completion. Stale profiles stay unavailable until a separately
reviewed adapter proves all five actual 100-attempt first-collection oracles
and its stricter zero conditions in the executing children. Do not substitute
the existing 20-attempt unittest or a generic root-counter aggregate.

Preserve stdout/stderr, original command status, witnesses, counters, XML,
plans and snapshot differences on failure. Audit failure also returns nonzero;
it cannot turn an original failure into success. Write the aggregate PASS only
after every required invocation and postflight check succeeds. Pending ROOTS
and full-stress-policy prerequisites remain blocking, including for a partial
run that happened to execute some selections successfully.

### First qualification and negative controls

First qualify the v2 resolver/auditor and additive launcher changes with host
fixtures and an explicit component-control registry. Use an existing quiescent
binary, accurately labeled, for bounded real off/kill driver-worker runs, a
two-module order and two-repeat case run, and the distinct cext-stage contract.
Do not report these controls as a full gate or the reclamation20 qualification.
ASan environment rejection can qualify on host fixtures first; a matching ASan
binary is required for an actual ASan positive. Named full-suite promotion
waits for a qualified receipt and complete gate2/3 execution.

Controls must fail for wrong build/runtime roots, a substituted actual module,
an older or mismatched build receipt/extension, source or tool drift, a changed
plan, ambient ASan/mode conflict, exclusion-order mismatch, a reused main plan
for cext, and gate-level mismatch. Reconciliation controls remove/substitute
one scheduled worker or result, mark a required case skipped/zero, omit one
repetition, remove an end witness, inject a failure and delete/empty/corrupt the
counter file or add fallback. Confirm ordinary gates retain their argv, named
gates reject narrowing, and timeout/worker-failure evidence survives. Real
runtime controls retain the existing ignored-environment, missing-getter and
lasting-state failures. No native mutation is needed for these controls.

### Identity/result component implementation

The launcher accepts only v2 plans. Shared helpers bind source/runtime roots,
complete execution-input snapshots, build outputs, frozen tools and a
coordinator-qualified receipt. Actual infrastructure and scheduled test-module
origins are checked before execution, with fresh plan-bound bytecode caches.
Build output inventories include both extension symlink identities and the
resolved in-build target bytes; external targets are rejected.

The targeted result checker binds scheduled and returned PID/configuration/
seed records, module origins, exact qualified counts, concrete XML outcomes,
skip IDs/reasons and process counters. Protected tests also reject skipped
parameterized or message-bearing subtests. This checker has no full-suite
selection/count/skip policy and cannot produce a named profile PASS.

The cext component stage uses `-q -u cpu -j1 test_cext` with the frozen worker
command. This adds one isolated module worker to the ordinary sequential
cext invocation so scheduling and result witnesses apply. Its short scheduled
name `test_cext` is bound explicitly to canonical origin `test.test_cext`.
It has no case filter or remote-exec exclusion. Ordinary numeric gates are
unchanged; no run command, gate hook or prerequisite promotion is implemented.

Host qualification covers 36 registry/launcher/identity/result controls.
Retained before-fix controls expose acceptance of changed extension bytes
behind an unchanged symlink, external build targets, protected subtest skips,
and the cext renderer's missing multiprocessing option.

Component evidence is retained under
`build/manifest-20261029/identity-stage2b/`. The configured debug build uses
native revision `052d8342e56` and `/tmp/boilgate` as its source, with its existing
`-Og` configuration and ten unavailable optional modules. The
[receipt review](../design/work/reviews/verification-build-receipt.md)
records its clean rebuild, smoke and separate target-byte supplement. Original
105-entry receipts remain unchanged and superseded: they recorded extension
links without their target bytes. The replacement receipt binds 172 outputs.
Initial harness/incomplete-origin failures and all superseded plans/tools
are retained alongside the final proposal; they supply no positive coverage.

The frozen `qualification-plan-final.json` batch completed with outer status
zero. Off, kill and each of two separate repetitions executed the exact
value-span case in an actual driver/worker pair. The full context/weakref
module invocation reported 203 tests and five approved source skips; all 198
remaining IDs succeeded. The cext invocation reported 16 tests and seven
approved source skips; all nine remaining IDs succeeded. The platform auxiliary
also passed its kill-mode state checks. Each driver cohort reconciled actual
PID/configuration/seed/origin/XML/count/skip records and collection counters,
with zero fallback. Final complete source/output identity checks passed.

Seven isolated native negatives rejected ignored environment, a missing getter,
a lasting stress change, malformed configuration inside actual worker.main,
a substituted test package before its initialization, a missing receipt and
a wrong runtime root. Eleven corruptions of the successful actual evidence
rejected missing worker/result/end/origin records, narrowed positive counts,
wrong configuration/seed/stage, missing counters, fallback and an allowlisted
protected subtest skip despite successful base XML. Commands, raw logs,
witnesses, counters, XML, audits and exact source skip policy remain in
`actual-v2-final/` and the surrounding evidence directory.

This qualifies only the identity/result component on the recorded build.
ASan environment controls use host fixtures; there is no actual ASan positive.
The cext fixture's custom children are outside the uniform regrtest worker
witness contract. Full suites, twenty repetitions, stale-child adapters,
production selection/count/skip policy, orchestration and gate integration
remain later work. No named profile is promoted by this batch.
