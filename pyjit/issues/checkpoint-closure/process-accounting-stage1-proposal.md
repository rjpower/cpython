# Process accounting capture hardening: stage 1 proposal

This is a proposal only. It does not change `worker.py`, `check_r27.py`, or the
runtime. The source review is [`process-accounting-mechanism-review.md`](process-accounting-mechanism-review.md).
The exact accounting-rule manifest remains empty. Stage 1 improves the
trustworthiness of launch-event records; it does not prove that a child
started, finished, or left a stats dump.

## Launch record contract

Replace `_launch`'s `(words, env)` result with an explicit shape that keeps the
audit event's actual arguments, the executable/path field, and how much can be
inferred. Product identity must come from the executable/path field supplied
by the audit event, not a search through `argv` and not `argv[0]`:

- `subprocess.Popen` provides `executable` separately from `argv`. When it is
  set, use that actual executable for product identity even if `argv[0]` names
  something else. When it is unset, use the API's actual executable rule for
  the first argument, without treating later arguments as candidates.
- `os.posix_spawn`, `os.exec`, and `os.spawn` provide the path separately from
  `argv`; use that path as the executable identity.
- If the executable is a known build product, retain the original `argv`
  unchanged for flag prediction. `_predict` must parse interpreter flags from
  the actual argv positions after its program-name element; it must not search
  the vector for another executable.
- If the executable is `env`, recognize only a fully parsed, explicitly
  supported prefix (`-i`, `--ignore-environment`, and `NAME=value` entries)
  followed by the target executable. If the prefix is malformed, uses an
  unsupported option, or does not identify a target, record the event as
  opaque and fail closed. Do not scan arbitrary later arguments for a product.

- For structured `argv` (the list/tuple supplied to `Popen`, `posix_spawn`,
  `exec`, or `spawn`), retain one decoded string per original element using
  `os.fsdecode`. Do not split an element containing spaces. In particular,
  `['python', '-c', 'print("a b")']` remains three arguments, with the payload
  unchanged.
- For a top-level command string supplied by an audit event such as
  `os.system`, preserve the decoded command as `command` and set
  `shape: "opaque-shell"`. Do not invent an `argv` by applying `shlex`. On
  POSIX, the `subprocess.Popen` audit event receives the normalized argv and
  separate executable; when that executable is a shell, retain that actual
  shell argv but mark the command it may execute as opaque. Its `-c` payload
  is not a Python exec vector.
- Convert path-like items with `os.fsdecode` at their actual item boundary.
  Any conversion or prediction error becomes an audit-capture failure; it must
  not escape the audit hook.

Keep a distinction between a directly recognized build executable and an
opaque shell event. Only predict `counts: true` or `false` for a structured
direct launch whose executable is identified from the path field or the
complete `env` grammar. Opaque shell records use `counts: null` and a reason
that says the child command is unknown. Do not treat `-c` payload tokens as
executables. Opaque events remain diagnostic and fail qualification until a
separately reviewed rule can justify their treatment.

## Fail-closed hook behavior

The audit callback remains non-throwing so an instrumentation failure cannot
change the Python test's control flow. Replace the blanket silent `except`
with a bounded in-memory failure list and a best-effort `<tag>.audit-error`
ledger. A record contains only stable diagnostic fields such as parent pid,
tag, audit event, stage (`decode`, `predict`, `spawn-write`, or `error-write`),
and exception type. Do not include exception messages or arbitrary audit
arguments, which may contain caller-controlled data.
Keep a small cap and record that additional errors were omitted.

If the primary `.spawn` capture fails, retain the failure in memory and try to
append it to `.audit-error`. If that write also fails, retain both failure
stages in memory. The callback still returns normally. After
`run_single_test()` returns, `worker_process()` must surface any in-memory
capture failure through the ordinary `TestResult` serialization path: mark
the test result failed and append a named error entry describing the capture
failure. Preserve any test errors already present. Do not raise from the hook
or convert the audit failure into an exception inside the test. Keep worker
exit zero after writing the result so `run_workers` reads the JSON result;
its nonzero-exit branch instead discards the result and reports only
`WORKER_FAILED`. If result serialization itself fails, the existing worker
failure path remains authoritative.

This makes a failed error-ledger write visible whenever the worker reaches
result serialization. If the worker is killed before that point, the existing
unfinished `.run` marker remains a separate gate failure; stage 1 does not
claim durable reporting across process death.

## Reader behavior and count preservation

`Folded.read_markers()` should retain predicted-true events in a separate
`counted_spawn` diagnostic list instead of discarding them. Keep predicted-
false direct launches in the existing uncounted list, and retain opaque events
in a distinct diagnostic list or as explicitly opaque records. Every opaque
launch is an accounting error until a reviewed disposition says otherwise.
None of these lists is a child-process count or a reconciliation result.

Read every `.audit-error` file. The existence of any `.audit-error` file is an
accounting error, including an empty file (which can indicate an interrupted
ledger write). Malformed records, unreadable files, and truncated final
records also add `accounting_errors`; the normal suite gate already rejects
that field. A malformed `.spawn`
JSON line must also become an accounting error instead of being skipped. Do
not treat a `counts: true` record as a dump, or treat a `counts: false` event
as proof that a child ran. Keep `suite_violations()`' exact rule manifest
empty.

## Focused fixtures

Add focused tests for the worker's launch-shape helper and the checker reader:

1. A structured `-c` vector with spaces and quotes preserves exactly the same
   element boundaries after `os.fsdecode`; no payload element is split or
   searched as an executable. `Popen(argv=['fake-argv0'],
   executable=build_python)` identifies the build executable from the separate
   executable field; `Popen(argv=[other, '-c', build_python_path])` does not
   misidentify the payload as the executable.
2. `os.system("python -c 'print(1 2)'")` is recorded as an opaque command
   with no asserted argv or predicted boolean. On POSIX, `Popen` normalizes a
   string argument to a one-element argv before its audit event; test that
   behavior at the launch-shape helper if needed, not by expecting a raw string
   from a live Popen event. A `Popen(shell=True)` or structured `sh -c` launch
   preserves the actual shell argv but does not classify its payload as the
   shell's child argv.
3. A direct structured launch for the build interpreter keeps the existing
   environment and `-E`/`-I` prediction behavior, while a `.spawn` record with
   `counts: true` is retained separately from uncounted events.
4. Inject a `.spawn` write failure and verify the hook returns normally, an
   `.audit-error` record is attempted, and the in-memory failure is available
   to the worker result path. Then inject both the primary write failure and
   `.audit-error` write failure; verify the callback still returns normally
   and the worker result still fails with the in-memory diagnostic.
5. Checker fixtures for a valid error record, an empty error file, malformed
   JSON, truncated ledger, and unreadable `.audit-error` file each produce a
   nonempty `accounting_errors` list and a suite violation. A valid counted
   `.spawn` line remains visible in `counted_spawn` and does not by itself
   satisfy any child/dump accounting.

Run only these focused worker/checker tests after review. Do not run a suite or
claim that child starts, successful exits, `.run` markers, or finished dumps
have been reconciled.

## Separate follow-up

Finished-process identity needs a later producer change in `Python/pystats.c`
and an independently reviewed reader update. A checker-only patch cannot
recover pid/ppid/argv after the `.run` marker is removed. The post-success
receipt design is documented separately in
[`process-identity-stage2-proposal.md`](process-identity-stage2-proposal.md).
