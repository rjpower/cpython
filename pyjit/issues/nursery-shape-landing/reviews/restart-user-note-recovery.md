# Exact recovery of the four remaining user notes

2026-10-31. Follow-up to [the initial recovery audit](restart-recovery-audit.md).
The original audit is preserved. Its four unresolved legacy-file losses are
now resolved by exact byte recovery; this changes no runtime source or
qualification result.

A filename-only search identified historical Codex session logs containing
the four missing paths. The four relevant session headers all name this exact
CPython workspace. Only matching target-file Add/Update patch records were
replayed; no commands from those logs were executed. The reconstructed files
were first written under `build/restart-recovery/candidates/`. All four matched
their SHA-256 fingerprints in the pre-loss
`build/documentation-migration-20261031/protected-user-files.json` manifest.
There were no patch replay failures. Each missing destination was then created
exclusively and its final bytes rehashed.

All paths below are under `boil/archive/legacy/design/work/reviews/`:

| Recovered file | Bytes | Verified SHA-256 |
| --- | ---: | --- |
| `10a-native-cleanup-protocol-review.md` | 9,998 | `b309e294eca141ad3b581e705e6171e568cdc44ef06b50b0c22caa40c6a3a23f` |
| `10a-native-cleanup-protocol.md` | 17,868 | `eaf8383a29503931f4ebec24df72479ab7f66e3cd360b048b3606823d37874ca` |
| `7a-completion-graph-independent-review.md` | 7,306 | `45cc28bb82af93fddc536792c644aaa7cd26100edf661fed6d3215031336d4b4` |
| `7a-retirement-completion-policy.md` | 13,476 | `e6fbd7c626e404a512d834c11a71ca8f36aba6de6ea44e31ebae07e29b9e85ac` |

Evidence remains in `build/restart-recovery/extract_session_notes.py`,
`candidates/session-patch-replay.json` and `session-notes-restored.json`.
The replay receipt identifies each source session, record line, timestamp and
patch fingerprint. It does not copy unrelated conversation content.

Together with the earlier exact Git-blob recovery of the modified
`7a-owner-metadata-source-review.md`, all five legacy files protected by the
migration manifest have their original recorded bytes again. The four files
listed above remain untracked and the source review remains modified, as
before the loss. They are unrelated user work and must remain outside project
integration commits. The separately reconstructed landing-results document
retains its own reconstruction provenance.
