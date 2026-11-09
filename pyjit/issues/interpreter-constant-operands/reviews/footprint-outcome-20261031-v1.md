# Matched native footprint and current disposition, 2026-10-31

**Current disposition: performance decision OPEN following user steering.** Matched text growth is diagnostic. It does not prove an elapsed regression or reject this source increment. No timing scores have been collected. Native consumer/work-removal evidence remains incomplete, and revised performance-plan review and activation are still required before timing.

The earlier held performance plan, SHA256 `103c37231e718fd5d6bc013ea4edebe3696460769ef6b640bf8edf4f1a59401a`, imposed a 5% evaluator-text cap. Applying it produced historical receipt `build/constant-operands-review/footprint-outcome-20261031-v1.json`, SHA256 `3ae0d8c622311c481d59db04055143e3fa68466c0fc2ded8628ee4ad079cea1a`, with a rejection disposition. That receipt remains unchanged. The coordinator then reported the user's challenge to a static-size hard gate and reopened the decision before any timing. Superseding receipt `build/constant-operands-review/footprint-user-steering-20261031-v2.json`, SHA256 `32519280c6aac4348f26a85d88866ea845f9b92f12a2319f5022657cc2aada1d`, supersedes only that disposition and preserves the measurement. The original plan is also preserved byte-for-byte at `build/constant-operands-review/performance-plan-before-user-steering-20261031.md`.

## Exact measurement

The evaluator policy is identical for both ELF images: include every executable `STT_FUNC` range named `_PyEval_EvalFrameDefault` or that name followed by a dot, including any cold/cloned shards; form the union of address intervals so overlapping symbol-table aliases count once. The images each have two identical dynamic/static symbol aliases and one unique function range. Neither has a named cold or cloned shard in this family.

| Image | Unique link-time range | Native bytes |
| --- | --- | ---: |
| Parent | `[0x44f270, 0x471ef7)` | 142,471 |
| Candidate | `[0x4525f0, 0x47b901)` | 168,721 |

The delta is 26,250 bytes, **18.424802%**. This measures evaluator native text; it does not measure hot working-set size, cache misses or elapsed performance. The independent structured ELF symbol inspection agrees with `/usr/bin/readelf -sW`; the readelf binary SHA256 is `c857339616bbbfa5eba32733e22365048903fbaf6ed2126b897dd138bcb741fc`.

Parent image: `../boil-nursery-shape-qualification/build/release/python`, SHA256 `289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f`, compiled source `e9e10d5b503eaf219797a5db44766eeab6fad4dc`. Candidate: `build/worktrees/constant-operands/build/release/python`, SHA256 `bc3039eb4d48325740b726b14a2dd59a094163ad3408409f173f743c2db5a3f7`, compiled source `a84965eaa90267ac1dbf1b4c97ab032aaf26cedd`.

The parent-to-arithmetic-base `32f14069fd2a0b34b67f167621d2de4671369a49` bridge changes none of Python/Include/Objects/Modules/Parser/cases-generator runtime sources, configure or Makefile template. Both sealed builds used the same configure arguments, make command and exact compiler/cc1/collect2/assembler/linker/tool identities. Their `pyconfig.h` files are byte-identical; the complete Makefiles become identical after substituting only the worktree root. Both images are ELF64 little-endian x86-64 PIE (`ET_DYN`). The measurement is therefore matched configuration/source-parent evidence.

## Preserved native feedback and limits

Successful debug selections remain separately attributed: new cases 11, IR 74 and generator tests 83 at test checkpoint `a475`; disassembly 147, code 47 with one skip, and tracing 442 with one skip at `c76`; monitoring 108 at `a849`. Generator tests comprise 82 ordinary passes and one existing expected failure. The code skip is `test_interned_constants`/`does not intern all constants`; tracing skips `test_jump_extended_args_for_iter` because CPU resources were not enabled. These are separate selections, not a count of unique cases. Their runtime image remains compiled `7f0`, with reviewed Python-test-only bridges. Fresh `a849` O3 and its eleven release cases passed. Positive result/cleanup/raw-artifact hashes are retained in the independent receipt; these selections recorded zero fallback.

Earlier test-oracle failures, failed inventory/parser evidence, the original 82-second O3 timeout, its archived partial products, and the corrected-source passes remain distinct. The new O3 completion batch stopped at its emission row: the host reader required `ET_EXEC` and rejected the actual PIE image. No emission result or consumer-word child was produced. This is a host observation failure, not a semantic-test failure or completed native work-removal proof.

Actual coroutine `n-1`/`n-2` and spectral `+1` words, complete shared/cold native paths, and dynamic-left/error-path work-removal attribution remain to be observed with a reviewed PIE/relocation reader. No timing, qualification or landing claim follows from this report. All review commands were host-only static/file inspection; no candidate interpreter, workload or emission retry ran. The original completion deadline remains `22:45:24.688115 UTC`.
