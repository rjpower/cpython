# Independent private-ABI packaging prototype review

2026-11-01. **ACCEPT the frozen host source prototype for separately authorized application and native feedback.** Capsule SHA `5d22355f84cc21478a39249a781c696df75a3896887d7bedb0a863f04cc947a2`; patch tool SHA `bdb69d5ca79f3a4fdf90bb8daa087b06a07024fb81744d1986b657949d744d63`; host test SHA `873e41117f6929e8c11c5b1cbe3d11037f87e8e248f774227b618e730f3f0b60`. This is source/artifact acceptance, not installer or extension qualification.

The implementation closes all four sites from accepted v2 design: three vendored `packaging.tags.sys_tags` producers and pip's real `compatibility_tags.get_supported` installer route. Private ABI tags come from loader-authorized private suffixes after exact SOABI/EXT_SUFFIX/native-suffix checks. Debug's release alternative is inherited from that loader table. Ordinary CPython/PyPy ordered tags and explicit public `cpython_tags` behavior remain unchanged. The default private TargetPython list/set cache uses corrected tags; any explicit target override fails before ordinary CPython inference. Pure-Python `none` tags remain available. No runtime identity, loader suffix or stable-ABI contract is broadened.

I independently replayed all 33 host tests in a copied reviewer-owned directory: 28 actual vendor/installer policy rows and five archive controls. The tests exercise actual pinned tag functions, compatibility_tags and unchanged TargetPython AST, not a reconstructed installer oracle. Twelve additional actual-function controls reject empty string, empty list and False for each of the four explicit target parameters. The three modified tag modules restore byte-exact original source after removing the shared generated helper/branch. The installer adapter's exact None predicate was independently checked structurally.

Both complete output archives were independently reproduced byte-identically by the retained tests. Unknown inputs/member hashes, signatures, aliases and occupied outputs fail closed. Whole-wheel input pins restrict the supported layouts. Independent archive comparison found no deleted or unintended changed members: pip changes its two policy files, outer RECORD and marker; setuptools changes its two policy files, two nested vendor RECORDs, outer RECORD and marker. Unrelated member bytes and nested RECORD rows remain intact. Marker hashes match changed members and original inputs; the marker avoids a self-referential output hash. External manifests bind the final archives. ZIP order/timestamps/compression policy and outer RECORD membership/hash/size rules agree.

Accepted output SHA256 values are:

- pip: `a0f564ec764b528ee5473887dec474c27ad8fa8fe9430fc5843c96e99e5edbdd`.
- setuptools: `93db1553711fa6ae2723e19c467906b306a5e1d1e20287f8f934da5f19fb110a`.

The author's first RECORD selector failure and stale nested-RECORD draft remain preserved. My first artifact-audit attempt accidentally shadowed the host inspect module through its filename; a second assertion depended on AST formatting. Both reviewer-only logs are retained, and the corrected structural audit passed. Neither changed the prototype, wheel bytes or selected source.

All 31 capsule artifact/input/output/reference bindings were independently rehashed. Selected RC dce654018d5 remains clean and its bundled wheels retain their original input hashes. The prototype writes new files only; it performs no installation, target call, compilation or network fetch. Temporary extracted reviewer fixtures were removed after replay; retained wheels/scripts can regenerate them.

Root application authority, final public file layout, a new exact compiled/frozen/config/product reuse bridge and fresh dynamic wheel hashes remain required. Actual ensurepip/venv, unchanged 16-case C/C++ extension feedback and regeneration are still pending. The original 15 wheel-build failures stay failed. Native foreign-wheel rejection needs its separately frozen gate before public installer isolation is claimed. Cross-target pip options, other wheel/tool versions and historical ecosystem ports remain outside this integration.

Evidence: `build/rc-private-abi-packaging-implementation-review-20261101-v1/`; author prototype and every earlier draft remain untouched under `build/rc-private-abi-packaging-prototype-20261101/`.
