# Census fixture contract review

Read-only source and retained-marker review on 2026-11-06, against frozen
runtime `6a0e4c12c17`. No tests, builds, or benchmarks ran. This receipt
proposes a bounded accounting packet; it does not clear any census failure.

## Proven source behavior

`Lib/test/test_pyvalue_box_sites.py:_stats` and
`Lib/test/test_value_slots.py:_box_kind_counters` both:

1. Warm their target, then clear the process counters and enable collection.
2. Run the target, disable collection, and call `sys._stats_dump()`.
3. Select a new `.txt` dump by the current PID, read it, and unlink only that
   `.txt` file (lines 63 and 2408 respectively).

`Python/pystats.c` publishes the completed dump by renaming `<stem>.tmp` to
`<stem>.txt`, then renames the corresponding `<stem>.run` to `<stem>.done`.
The fixture therefore consumes one member of a paired artifact and leaves
the other. `Tools/build/check_r27.py:Folded.read_markers` correctly treats a
`.done` without a folded dump of the same stem as an identity error.

The retained census inventory reports 247 orphan `.done` markers tagged
`test_pyvalue_box_sites` and four tagged `test_value_slots`. A sampled orphan
also identifies the matching regrtest worker in its argv. The source proves
that successful fixture calls can create this exact mismatch, and the tags
match all 251 reported orphans. It does **not** provide a per-call execution
trace proving which deletion produced each orphan or the exact 247-call
count. Preserve that distinction.

## Counter state and directory contract

Deleting both files would remove the visible orphan while losing evidence.
The helpers also clear pre-existing counters and leave collection disabled.
`_Py_StatsClear` and `_Py_StatsOff` record resets; `sys._stats_dump` dumps and
clears counters, then creates a new marker only if collection is enabled.
The next packet must preserve counters and enabled state around fixture
measurements, including exceptions. Pair cleanup alone cannot establish
whole-suite coverage.

The producer resolves directories in this order: `-X pystats_dir`, then
`PYTHON_PYSTATS_DIR`, then the default. Both fixture readers consult only the
environment and default. Changing only the environment to isolate fixtures
can therefore leave the producer writing to the suite's explicit `-X`
directory while the fixture reads elsewhere. A private directory or prefix
must have one explicit producer/reader contract and retained accounting.
An ignored fixture prefix would not meet that requirement.

## Two missing dump identities

The summary reports missing `.done` records for these folded dump stems.
Their retained `.run` records narrow the process family:

| Stem | PID | Tag |
| --- | ---: | --- |
| `18e57091c288d6d60035dc42000000018084bb30` | 3529794 | `test.test_multiprocessing_fork.test_misc` |
| `18e5709952ee752e0036193400000001d7b491e0` | 3545396 | `test.test_multiprocessing_forkserver.test_misc` |

Both argv invoke `multiprocessing.forkserver.main(...)`. This establishes
their launch context, not the reason the completion marker is missing.
The census reader's `fold_dir(delete=True)` consumes raw `.txt` files after
folding, and those two raw dumps are no longer present. Do not infer a
successful or intentional exit from the marker tags.

## Minimum next packet

First specify how fixture samples retain their raw dump/identity pair and
how the outer census retains uninterrupted coverage. Prefer a small shared
fixture helper with non-destructive snapshots if the existing producer can
support them. If destructive probes require child isolation, explicitly
account for the child, its private output, and the surrounding parent
segments. Keep unknown or missing evidence fatal; do not add tag allowances.

Focused validation should cover producer/reader directory precedence,
multiple sample pairs, unrelated concurrent dumps, complete raw-pair
retention, and counter/enabled-state continuity on success and exception.
Negative cases must still reject missing or contradictory pair members.
Then run the two affected modules under the census collector in a fresh
directory and reconcile every sample and process identity.

Separately capture the two forkserver families with raw dumps retained at
the publish/completion transition. Pair dump PID and stem with start identity
and exit evidence before deciding whether the fault is publication ordering,
process lifetime, or another cause. Only after focused accounting passes
should the coordinator run a new full census. The current 251 orphans and
two missing identities remain unresolved admission failures.
