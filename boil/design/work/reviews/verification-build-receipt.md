# Verification v2: controlled build receipt review

Status at09:33 PDT: the initial105-output qualification below is superseded
as incomplete. The first real launcher attempt exposed 67 extension symlinks
whose link identities were recorded without their target contents. Do not
reuse the original qualified receipt for further controls. Both original
receipt files remain unchanged; a strict target-binding repair and separately
reviewed supplement are pending. No component positive gate is claimed.

Decision, 2026-10-29 09:17 PDT: qualify the retained build receipt for the
identity/result component controls. This is not a profile, full-suite or
ROOTS2b qualification result.

Evidence directory: `build/manifest-20261029/identity-stage2b/`.
The original `receipt-unreviewed.json` stays unchanged, SHA256
`394c9e67efac6c00f723057cf2ca3a702fb656ee1d36c11fbb26d9455688b7ad`.
A separate qualified copy changes only its status and review fields.

Independent checks confirm:

- The 6,188 source inputs match clean queue1 source `052d8342e56` at
  `/tmp/boilgate`. Before/after snapshots are byte-identical, SHA256
  `c7f7ed0c261b37e84dc196660073814a7801e3330c1a236bd9f73c3d5ccf4dc3`.
- All 105 recorded build outputs match current files. Their manifest hash is
  `a97dacd04141c7a96dd7bcdff5b102fbfbca6679c14053481580bee97c7bde2d`.
  Makefile, config.status and pyconfig.h match the approved prior configuration.
- Recorder, frozen tools, compiler executable/version and all command-log
  hashes match. The exact clean, build, startup and nine-module smoke commands
  succeeded under the existing build lock, test slot and bounded outer runner.
  The outer command finished at 16:13:08 UTC; subsequent runner cleanup released
  the reservation before ROOTS resumed its release matrix.
- Smoke ran 1,924 tests with 20 skips. Counters contain 162 interpreter records
  across 59 physical PIDs, 1,314 collections, zero fallbacks and 226 report-only
  unpinned hits. This does not establish zero unpinned roots or strict kill mode.
- The retained runtime probe reports the fresh `052d8342e56` executable,
  GCC 15.2.0, Py_DEBUG=1 and the existing `-Og` configuration. Actual sysconfig
  and test origins are in `/tmp/boilgate/Lib`; build sysconfig data and
  `_testinternalcapi` come from this build's extension directory. Actual mode
  is (0,0), stress zero and Python optimization zero.

The unchanged configuration lacks ten optional extensions, including ssl;
its build log retains those diagnostics. No all-extension coverage, O1 policy
adoption, actual-driver/worker mode contract or native integration beyond
queue1 is inferred. The bootstrap recorder's frozen tools remain separate
from the final component launcher. Strict v2 controls must use their own
frozen tools, complete source/output validation and qualified receipt.

Independent follow-up verifies all67 current target byte hashes, link strings,
inodes, sizes and ctime/mtime against the retained supplement proposal
`e7358bae4f5db5d72505cd67e78d6500ee835eadd973cc300122201a12746214`.
All targets are inside the build; their latest mutation is16:10:00.623679 UTC,
before the original build command completed at16:10:23 UTC. The retained
post-build command inventory records no intervening build/output writes.
This supports a post-build supplement; it does not make the original105-entry
manifest complete. A replacement output manifest must bind both aliases and
actual target contents before the replacement receipt can be qualified.
