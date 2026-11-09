# 4fffee kernel dump: parser repair and warmup attribution

The first 29-kernel attempt stopped while parsing its first `richards` warmup dump. Its sealed raw journal is `build/pyjit-link-wave1/checkpoint-closure/kernels-4fffee855c9/raw.jsonl` under the boil worktree. Dump 0 has 1,801 numeric lines and 1,800 distinct keys. The sole duplicate is `opcode[GET_ITER].specializable: 1`, at text lines 4 and 452. `Python/pystats.c` prints this metadata flag explicitly and again in its cached-op loop. The earlier checkpoint measurement already recorded this accepted emitter behavior in `pyjit/bench/results/checkpoint-20261103.md`; the flag is not a count.

`pyjit/bench/run.py` now ignores only an identical duplicate `opcode[...].specializable: 1`. All conflicting flags and repeated counters still fail, including total and per-opcode box counts. The original dump is journaled before parsing. Focused tests cover those cases and pass (8 tests via `uv run --no-project python -m unittest discover -s pyjit/bench/tests -p test_pystats_records.py`). The failed attempt's raw record remains untouched; the coordinator should run a fresh attempt directory and journal.

That warmup dump reports 237 CORE boxes and zero THUNK boxes, so the startup-inclusive gate still fails. The five recorded sites were symbolized against the unchanged stats binary at `build/pyjit-link-wave1/pyvalue-rc-repr-stats/python`; its build ID `80373178cc3461711ed3e7b6f878c381b49a50a2` matches the dump:

| Count | Site | Source continuation |
| ---: | --- | --- |
| 128 | CORE/compiler `0x461d24` | `Python/flowgraph.c:eval_const_unaryop`, value result projected while folding a constant |
| 51 | CORE/compiler `0x43d872` | `Python/compile.c:_PyCompile_ConstCacheMergeOne`, cached value projected into an object-only output |
| 49 | CORE/tuple `0x4cb486` | `Python/marshal.c:w_complex_object`, tuple item projected before recursive write |
| 1 | CORE/field `0x4cde91` | `Python/marshal.c:w_complex_object`, slice field projected before recursive write |
| 8 | CORE/unattributed `0x3eb3f7` | `Python/ceval.c:initialize_locals`, keyword value projected before `PyDict_SetValue` |

The ceval site has a direct value-native continuation because `PyDict_SetValue` already borrows a `PyValue` for its value. Flowgraph, constant-cache and marshal writer paths need bounded ownership/interface proposals before changing their object-only consumers. These counts include the benchmark driver's imports and compilation; a separate `-S -c pass` startup probe does not exercise all of them.
