# Restart qualification review

Reviewed 2026-10-31: frozen union
`e9e10d5b503eaf219797a5db44766eeab6fad4dc`, its retained evidence in
`../boil-nursery-shape-qualification/build/nursery-shape-*`, and the
[remaining qualification plan](../resume-plan.md). This is an independent
evidence and execution-design review, not landing acceptance. No native suite,
rebuild or retry was launched by the reviewer.

## Decision

The completed checks can stand through an explicit restart bridge. The new
60-minute, remaining-only batch is clear to proceed after the current release
cleanup observation and normal admission. The original 90-minute attempt
remains incomplete and expired; the restart does not extend its budget.

The release full result is terminal and valid as test execution, but its
historical cleanup receipt is missing. A fresh cleanup observation must identify
the current boot/time and remain distinguishable from historical completion.
Neither this review nor the retained PASS label supplies that missing evidence.
Debug full OFF, debug full KILL, both stress selections, ASan16, closure and
independent landing review remain required.

## Evidence checked

Independent SHA256 comparison found no mismatches in:

- All 39 artifacts of early completion `94ab2b24`, all 60 artifacts of common
  completion `692fac53`, and all 32 common component references.
- All 6,945 frozen source/helper inputs in broad plan `78ed8429`.
- All 588 recorded product entries in each release, debug, ASan and Clang-tail
  image, including resolved paths and recorded aliases.
- The twelve recorded tool files for each GCC build and ten Clang-tail tool
  files, including their resolved paths and hashes.
- All three raw artifacts bound by release-full result `b131fe53`.
- All 29 references in nursery component follow-up `59c53194`.

The unchanged release validator was replayed against retained state/root
records. It returned the exact stored summary: 564 modules, 51,261 tests,
3,795 skips, 565 driver/worker processes, 58,701 collections and zero unwind
fallbacks. Module states were 462 PASSED, 94 SKIPPED and eight RESOURCE_DENIED;
there were 67 inner skip events and one report-only unpinned snapshot visit.
The validator retained all 333 protected selectors and the single exact
`test_remote_exec_deleted_static_executable` exclusion. The report-only visit
does not become a zero gate.

All ten GC/test paths in the composition proof are byte-identical to nursery
component `3a473612157`. The retained negative program uses the same old-boundary
body, with the nursery disabled immediately before `collect(0)`. Its raw log
shows the nominated `minor reclaimed an old owner` assertion and completion of
the `finally` block before exit 1. It remains a component negative, with its
uncontrolled hash seed under `-I`; it is not relabeled as union execution.

The early five nursery and 34 shape controls, three stale six-by-100 cohorts,
ASan/tail OFF29 and tail KILL ownership result capsules remain narrowly scoped.
Their passing outcomes do not replace the unfinished broad/closure gates or
establish a speedup, nursery coverage expansion or default-policy change.

## Restart limits

Coordinator preflight `build/restart-recovery/qualification-preflight.json`
(`ba25ad80`) binds the unchanged union, source/products/tools/headers and current
boot `7bbe1ecf-490c-40f3-8db6-d46a8f6b6ba3`; it records no live candidate
executables. The script acquires the serialization lock and invokes the original
broad, ASan and closure loaders. It does not itself acquire global admission
slots or all build locks; those remain launcher obligations. The preflight has
no UTC field, so the separately timestamped cleanup/launch envelope must supply
the current observation time.

Old process handles and lock files cannot establish running work after reboot.
Completed hash-bound results survive; unfinished jobs receive no completion
credit. Retain each original result and stop at the first unexpected failure or
the new deadline, as the plan requires.
