# Dictionary source implementation authorization

This is the current implementation authorization, following the earlier
design-only decision and the successful independent list/structseq source
review. It authorizes dictionary source edits by coroutine_design_review in
`build/worktrees/pyvalue-rc-universal`, based on `95be1d28`.

The user's current instruction is: “use your parallel agents, starting new ones
if needed, to restore the pyvalue across the board, and then let's take a
moment to check our register interpreter performacne with the unified pyvalue
and basic optimizations.” The user also requires one representation throughout
interpreter operations and containers. The coordinator applies that instruction
through the [accepted restoration design](design.md), including its explicit
selection of a private value-native ABI and removal of generic borrowed object
getters. The target ABI is `boilvalrc1`; exact legacy binary compatibility is
outside that selected contract.

The required API changes are part of restoring the requested representation.
An inline scalar field has no existing PyObject to borrow. A fresh scalar box
needs an owner and can fail to allocate. Keeping a generic borrowed getter would
require invalid ownership, a permanent compatibility box cache, or a second
stored representation. The reviewed design rejects those approaches.

The coordinator therefore authorizes removal of the generic borrowed dictionary
object getters and iteration API, including PyDict_GetItem, GetItemWithError,
GetItemString, borrowed SetDefault/known-hash variants and PyDict_Next, with
their corresponding internal declarations. Replace them with owning PyValue
lookups/iteration and explicit owned, fallible object bridges such as
PyDict_GetItemRef. Update affected usages through the dependent consumer ports;
do not introduce borrowed scalar-box aliases, ABI casts or fallback modes.
Existing Python dictionary behavior, RC/cyclic collection and ownership/error
contracts remain required.

The independently accepted exact cut is
`build/pyvalue-register-interpreter-checkpoint-20261101-v1/dict-port-cut.md`,
SHA256 `678846005cde2dad968ccad588a1ddc30e6cb9c3f9c009c95824e68295dad36e`.
The interface handoff is its sibling `dict-value-api.md`, originally
`359d8c867b7e892315aef60b5bf0d30f742f25faedd09ad5684b12c6f2f67592`.
The latter's initial “postcommit” wording is superseded: preserve the existing
pre-mutation watcher notification order and unraisable-error behavior.

The cut owns Objects/dictobject.c, the public/storage/internal dictionary
headers, the three managed-value declarations in pycore_object.h, and the
listed dictionary/CAPI tests. Narrow additional handoffs cover only
builtins_dict_watcher in Python/pylifecycle.c, module_dict_watcher in
Objects/moduleobject.c, and the three dictionary test callbacks in
Modules/_testcapi/watchers.c. The existing owner explicitly handed off those
functions/declarations. The watcher key/value ABI is value-native with owned
snapshots backing callback-duration borrows. Interpreter/generator and other
raw-layout consumers retain their named owners.

Source implementation began at 2026-11-01T22:30:46.570335Z, with checkpoints
every20 minutes and the first90-minute ceiling at 2026-11-02T00:00:46.570335Z.
The prepared `dict-port/port_getters.py` changes only the handed-off dictionary
source/header paths and implements these explicitly authorized removals and
owning replacements. The coordinator reviewed that script. Reversible source
edits and host checks are authorized; source review and coherent native
qualification still precede adoption. No candidate runtime result is claimed.
