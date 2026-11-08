# Measurement preflight review and actual-build check

The local harness now requires an explicit diagnostic/qualified label and
records source, executable revision, configuration, imported module origins,
ELF build IDs and immutable module-check evidence before and after measurement.
Qualified preflight rejects stale executables and mismatched Python/C imports.
Preflight-only output cannot claim an eligible measured result. The unchanged
timing plan has three rotated rounds over 29 kernels, a load limit of four and
a 45-minute budget beginning after preflight. RSS is a separate later stage.

Sol's post-review required executable/source revision agreement, native-module
type/origin checks, and explicit ineligibility before actual timing. Those
findings are corrected. Luna reran the focused identity/negative fixtures
through `uv` with the configured shared cache; they pass. No timing was run.

The coordinator ran the actual three-build preflight on frozen runtime
`af82c03c4e5`. Controls, imports, configuration, ELF IDs and the 116/116/117
module-check rosters pass. The candidate correctly fails one check: its
embedded executable revision remains `812cba08f4b`, while source HEAD is
`af82c03c4e5`. Ordinary incremental builds did not refresh getbuildinfo after
the commit. After the final source commit, refresh that build-info object in
each profile and relink with the normal complete build; then repeat preflight.
Do not relabel the stale version as a matching executable.

Evidence: external build root
`pyjit-link-wave1/checkpoint-closure/preflight-af82c03c4e5/`, including the
full snapshot and failure status. Timing was not started. Current full R27
and functional qualification are still failing; this harness check does not
admit the runtime.
