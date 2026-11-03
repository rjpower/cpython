# Independent counter-retention tooling review

2026-11-01. **ACCEPT the exact tooling change below for its stated host-side
retention and serialization purpose.** This grants no target execution,
counter probe, build, census, timing or attribution claim. The coordinator owns
the driver/README change; the reviewer left both files unchanged and added only
the authorized meaningful host safety tests.

Reviewed SHA256:

| File | Digest |
|---|---|
| `boil/bench/run.py` | `0a7771918e609486940440a1a339fb4ee644e7b6ef80a9ae134ae58c0aa94468` |
| `boil/bench/README.md` | `d1c6cd7be3c974e7bb3af2861704be329ffcc0a4428e2fa9c44df176b452310a` |
| Original three-method test snapshot | `f863f69f514f40084168851843b98ad0ceffd2eaa71f0d63517038f9bc9f38b3` |
| Final `boil/bench/tests/test_pystats_records.py` | `d413075e02688cce2f15f5ab200dc2d13099c7f367133927d3fa146731674e01` |

No blocking driver or documentation finding remains on these bytes.

## Findings and checks

**Retention occurs before counter parsing and schema acceptance.** The actual
new dump is read as bytes, decoded as UTF-8, hashed and appended to the journal
before parsing duplicate keys or checking handler coverage. Each attributable
row binds its sequence, benchmark, loop count, actual command construction,
original source path, text and original-byte hash. A duplicate-key failure keeps
both journal evidence and its original file. Successful parsing removes only
that newly selected source dump; unrelated files are untouched.

**Exclusive path handling fails before target probes.** Journal paths are
resolved and created with mode `x`. A preexisting evidence file is preserved;
output/journal and perf/pystats alias collisions are rejected, as is requesting
pystats retention without the boxes metric. There is one harmless ordering
detail: when both new journal arguments name the same path, the perf branch
reserves an empty file before the later cross-journal rejection. No target runs,
no existing file is overwritten, and the host test records that behavior.

**Signed union differences are retained.** New handler entries, pairs and box
sites use the union of low/high keys and the original loop/inner scale. Low-only
rows therefore retain negative differences rather than disappearing or being
clipped. All parsed warmup/low/high counters survive in `raw_counters`. Existing
total/class/by-instruction fields, low/high call counts, warmup order and units
remain unchanged. The original `by_instruction` filtering is not newly claimed
as a complete signed attribution table.

**Coverage guards remain effective.** Low/high metadata still requires coverage
version 2, a supported dispatch mode and valid domain/entry bounds; both samples
must agree. Missing metadata is rejected rather than guessed. Warmup data is
retained as raw evidence; it is not a third normalized sample or an independent
coverage endorsement.

**Run metadata binds the successful journal.** The actual final file hash,
resolved path and dump count are added to the metric's run information. The
end-to-end host mock exercises the real CLI path, three actual temporary dump
files, parsing, deletion, JSONL writes and final JSON serialization. It verifies
every journal command/text/hash/path and the report's journal binding.
`compare.py` reads the retained group/unit/value fields; additive nested evidence
does not change that consumer. The schema name remains unchanged, as these
fields are additive.

## Meaningful host verification

The original three methods passed independently. Four added methods exercise:

- CLI metric/collision/exclusive-creation failures before any build/child probe.
- Failed child, zero/two new dumps and undecodable bytes, preserving every
  original/unrelated file and avoiding an attributable row.
- Missing coverage metadata in either normalized sample.
- A successful complete CLI run with actual journal/report hash and count
  binding, original-dump cleanup and unrelated-file preservation.

All seven methods passed in 0.012 seconds using the required shared uv cache:

```text
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project --offline --python /usr/bin/python3 python -B -m unittest discover -s boil/bench/tests -p test_pystats_records.py -v
```

Child launches, build metadata/probes, machine metadata and load observations
were mocked where the CLI is exercised. No target interpreter, compiler,
hardware counter or workload ran. Driver and README digests were rechecked
after the added tests; no parser/runtime guard was weakened to obtain a pass.

## Precise limits

The journal promises original **UTF-8 text**. An invalid UTF-8 dump fails during
decode before a JSONL row can be formed, and its original bytes remain in the
stats directory. Failed children or ambiguous/new-file selection likewise
retain originals without pretending to know which file is attributable.
Duplicate-key and later schema failures are journaled. This is deliberately
narrower than a claim that every malformed byte stream is embedded in JSONL.

The existing one-new-file protocol is unchanged; it is not by itself a new
PID ownership proof. Actual census manifests must still bind the exclusive
execution environment, image/modules and source. Unresolved absolute box PCs,
64-site overflow, startup/setup differences, same-word adaptive redispatch and
cached-int reuse remain attribution limitations. The new fields preserve the
evidence needed to see them; they do not turn conversion counts into allocation
counts, adjacent-word frequencies, time shares or native-boundary dominance.
