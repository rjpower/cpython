# Foundation implementation

The user authorized universal PyValue restoration followed by a register
interpreter performance checkpoint. The accepted design remains binding:
reference counting and cyclic collection, one value representation, and all
three dependent implementation cuts. Foundation completion alone is not adoption.

Implement in `build/worktrees/pyvalue-rc-universal`, branch
`boil/pyvalue-rc-universal`, from `95be1d28a12ebbd9de102f70e1b5e881d06c7384`.
This retains the existing JIT source and its ownership contracts. Interpreter
measurement must explicitly prevent JIT admission and verify that no machine
code ran; JIT performance work follows the interpreter checkpoint.

The source scope is the 93 existing paths, three rename destinations and two
new files in `build/pyvalue-rc-foundation-manifest-20261101-v1/manifest.json`
(SHA256 `46d190fe57cad6de42962da48392814ae4b4d650c202bd4b349e42d0acdedea2`).
The semantic requirements are in its `source-cut.txt`
(`712710f0e8c88d92ae15ad52d984a9aca1e07e05c64c262e71dde557010083e9`).
Independent scope review accepted the closure:
`build/pyvalue-rc-native-consumer-map-20261101-v1/foundation-scope-review.md`
(`7fe2d835fb5d648979dea23e4eb6d39b81cc0b3d498d0464aaf2a09ddc38d8d8`),
receipt `9767f4920df83005162d7a6ae1bf79a74716fdad7713e8f8ab730db0e0f3e26d`.

Owner: restart_audit. Independent source reviewer: value_api_consumer_map.
The coordinator granted source edits, listed generators and host checks in the
implementation message. Checkpoints are every 20 minutes, with a 120-minute
foundation budget measured from that message. Report incomplete ownership
closure as incomplete. Do not silently expand the scope or deadline.

Prioritize actual prepare/publish/release behavior, fallible owned object
bridges and real frame/call consumers. A type rename is insufficient. Preserve
existing opcode IDs and JIT continuations. Review the actual diff before a fresh
focused native build. Broad qualification and performance run after the
dependent container, attribute and native-interface work is coherent.

The wider native consumer census is a dependency map, not permission to replace
historical files wholesale. Bundled modules, Clinic products, wheel/cache/import
identity and RC ownership remain required work. Keep failed evidence immutable.
