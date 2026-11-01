# Canonical Clinic correction and existing-image reuse

Host-only proposal after the first packaging-v3 row stopped on 2026-11-01 at
10:05:50 UTC. The selected HEAD remains
`8e132becdad485f9d281d63c5ec1c9c60ef3e56f`; its generated header is dirty.
No source restoration, compilation, target invocation, or commit was performed
for this audit.

The actual `make -s regen-cases clinic` returned zero but changed exactly
`Python/clinic/sysmodule.c.h`. All four later packaging rows remain unexecuted.
Keep this failed batch and its raw outputs. The row did not establish a clean
regeneration gate or packaging compatibility.

## Hypothesis and smallest correction

The RC projection removed the historical `!defined(Py_GC)` condition around
`sys.getrefcount` but left Clinic's conditional-method fallback behind.
Current `Python/sysmodule.c:1990` declares the method unconditionally.
Its generated header defines `SYS_GETREFCOUNT_METHODDEF` unconditionally at
line1081, before its former `#ifndef` fallback. `Python/sysmodule.c` includes
that header at line79 and consumes the method macro at line2873; neither file
undefines it. Thus the fallback's empty definition is never selected, for
release, debug, ASan, or global StackRef builds. None of the four configuration
differences guards the real definition. This argument does not depend on
assuming that `Py_DEBUG` implies a particular reference-debug mode.

The canonical diff removes two blank lines and the four-line dead fallback,
then updates the generated checksum. Header SHA256 changes from
`0c76786ccee95d7a148aa00dc5f625649d94ff392b40b1d4f5899807b9932288`
to `4fc00af65b95c870e03a0406e9bef9790f73174361bdaeee8756bd20fd1bfdca`.
Removing exactly the dead fallback and Clinic checksum comment gives identical
whitespace-delimited source tokens. There is no changed wrapper, converter,
method entry, function body, declaration, or runtime policy.

After independent review, accept this actual generator output as a separate
one-file correction commit. Do not hand-edit the generator, restore stale
output, amend the packaging commit, or broaden the delta. A later canonical
regeneration must leave tracked bytes unchanged. No claim of that future result
is made here.

## Actual build effects and reuse conditions

Make first reran configure/config.status. `configure.ac:152` and generated
`configure:3799` explicitly remove `pybuilddir.txt`; `Makefile.pre.in:993–1004`
documents its bootstrap/import-path role. This is a real image-support file,
not an ignorable temporary file.

The read-only audit hashes every current regular product against the accepted
four-profile bridge, including the original584 files per image:

| Profile | Existing products | Changes versus accepted bridge |
| --- | ---: | --- |
| debug |589, formerly590 | `config.log` changed; `pybuilddir.txt` missing |
| release |590 | none |
| ASan |584 | none |
| global StackRef diagnostic |590 | none |

All four Python binaries, `_testembed` binaries, extensions, Makefiles and
`pyconfig.h` files remain byte-identical. No products were added. Debug
`config.log` changed from `5ff859a563b2377082ebf8f9b03e0fc344e180455e5ed821db2bb1ece6e05c16`
to `0335caeba7878b32b08ba6cd797280ccbf1086bf3ab5470e85478aeb3df5e0d8`.
Do not describe all584 debug products as unchanged or restore the old log.

Under a separate explicit mutation grant, restore only the missing debug
`pybuilddir.txt` using an existing same-worktree profile's identical27-byte
marker, `build/lib.linux-x86_64-3.16` without a newline. Release, ASan and
StackRef copies all retain the exact old debug hash
`f8ad55466962a6061189f1a7f096fb982eeec56a74513f9053a99796453ee087`.
Verify its mode and bytes, own existing debug extension directory, and all
other products; retain the new config.log as an explicitly enumerated metadata
delta. Do not run `make`, an interpreter, or sysconfig merely to recreate it.
This restoration is proposed, not executed.

Source reasoning establishes that the removed fallback is inactive. It does
not establish byte-identical newly compiled objects: source line numbers,
debug information, and macro-expanded diagnostic locations may differ.
Existing-image reuse therefore needs independent acceptance of this narrow
semantic bridge, rather than a claim of a fresh build. If that is insufficient,
the smallest further compiler grant is preprocessing `Python/sysmodule.c`
before/after under each of the four exact existing configurations into separate
evidence files, without replacing selected source or products. Compare complete
outputs and disposition any diagnostic line-location differences explicitly;
unexpected executable tokens reject reuse. No preprocessing was run here.

## Next finite work and remaining landing blockers

1. Independent review of the exact generated diff, source proof, and four-image
   inventories; root alone grants the one-file commit and marker restoration.
2. Bind the actual new HEAD, exact source delta and declared debug metadata
   changes to a successor reuse bridge. Preserve all old bridge and batch files.
3. Reuse the five-row packaging plan under a fresh reviewed activation: first
   prove regeneration is clean, then run the two packaging modules in each
   ordinary profile, four literal foreign-wheel CLI calls, and unchanged CPU
   extension tests. Stop on the first unexpected result. No new cohort or
   acceptance claim is added by this proposal.
4. Root's adopted global StackRef scope remains unsupported/unvalidated, with
   its failures unresolved; it is not a passing profile. Ordinary full suites,
   the adjudicated ASan run, and prior source/generation receipts retain their
   stated historical scopes.
5. Landing still requires the reviewed primary merge/tree projection, exact
   protection of the five user files, and fresh main-tree release/debug builds
   plus the existing42-method smoke per image. No merge or JIT grant follows
   from this audit.

Evidence: `build/rc-clinic-correction-audit-20261101/{summary,debug,release,rc-asan,rc-stackref-debug}.json`;
prior bridge `build/rc-private-abi-packaging-final-bridge-20261101-v3/bridge-final.json`;
failed row `build/worktrees/rc-successor/build/rc-packaging-feedback-v3/affected/debug-regen.{log,result.json}`.
