# Independent public packaging layout review

2026-11-01. **ACCEPT exactly the five staged files** in capsule `3cfa50dd2246910fa10091f707f590588da919deecdc9e119feaa708d9a81fce`, with manifest `29ffa3359426864db3c84d089fd22ca00cd4a0e866984d9e995aef1ca25a4554`. This is a public-layout/source review; root retains application and native authority.

The public patch tool and both wheels are byte-identical to the accepted prototype. Their exact hashes are carried by the five-file manifest. Only the standalone test path adapter and policy note are new. Public tests take explicit original pip/setuptools paths and a new owned work directory, resolve patched wheels from ordinary repository locations, and validate original/output hashes before fixture extraction. Existing directories fail rather than being reused. No prototype or worktree source path is embedded in these public files.

Independent AST comparison preserves every original function and class definition, except the one deterministic-output comparison now reading `PATCHED[name]`. The only added function prepares explicit fixtures; the main entry parses arguments before running the same tests. I replayed the actual public entry against the immutable staged layout in a new reviewer-owned directory: all 33 tests passed, retaining all 28 policy rows and five archive controls. Nothing was installed or run under a target interpreter.

I rehashed every staged capsule reference, verified exact five-file membership, bytes, sizes and modes, and checked that selected RC dce654018d5 remained clean with original bundled wheel hashes. The policy note explains private tags, unsupported cross-target overrides, original archive recovery, deterministic patching/RECORD provenance, explicit downstream output hashes and pending native boundaries. No C, loader, configuration or original test_cext change is part of this layout.

The prior prototype review remains authoritative for packaging semantics. Native installation, extension ownership, ensurepip/venv, regeneration, foreign-wheel rejection and a new compiled-input/product reuse bridge remain pending. The previous CPU-extension failures are not reclassified. No previous rejected/unexecuted application draft was run or amended here. Root must independently establish current idle state before copying these five files; this review neither copies nor commits them.

Evidence: `build/rc-private-abi-packaging-public-layout-review-20261101-v1/`. Extracted reviewer fixtures were removed after replay; original/staged source and author evidence remain unchanged.
