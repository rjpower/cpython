# Extension rebuild authorization

At 2026-11-01 22:47 UTC the user clarified the intended compatibility boundary:

> what's this about, what compability with what? the whole idea of this project is to rebuild extensions etc IIRC, otherwise let's continue

The selected `boilvalrc1` runtime may change the private C API and ABI. Extensions
must be rebuilt against that runtime. Preserving old borrowed object container
getters or loading extensions built against the old ABI is not a requirement.
Python-level behavior and ownership correctness remain required.

This directly authorizes the dictionary getter/iteration migration that automatic
approval review previously rejected for insufficient user authorization. The
two refusal records remain unchanged at
`build/pyvalue-register-interpreter-checkpoint-20261101-v1/dict-port/approval-rejections.json`.
The dictionary owner was instructed to retry the same command with this user
authorization, rather than execute an equivalent through another route.

Source review, producer-aware regeneration, rebuilding affected extensions and
runtime qualification still apply. This is authorization to implement the
migration, not acceptance of the unfinished source or its performance.

The third attempt was also rejected: automatic approval review said the user's
extension-rebuild clarification did not name the exact API-removal action. The
script remains unexecuted. The coordinator reported this to the user and sent
an approval question naming `PyDict_GetItem`, `PyDict_GetItemWithError`,
`PyDict_GetItemString`, `PyDict_Next`, their internal equivalents and their owning
PyValue replacements. That exact action remains held pending the answer; other
source work and review continue.

The user then requested an explicit opaque extension API and a slower rebuilt
source migration bridge, with gradual adoption and no hard compatibility
requirement. That instruction supersedes immediate execution of this script and
its exact-action approval question. The script remains unexecuted; see
[current extension direction](extension-api-direction-20261101.md).
