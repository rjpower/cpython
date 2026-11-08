# Review 1: the value API's route to R27 and step V2a-0 (proposal)

Reviewed: [value-api-v2a0.md](../proposals/value-api-v2a0.md) (665 lines,
uncommitted, 2026-11-04), with its scripts and data in
`build/pyjit-link-wave1/v2a0/`. The proposal was checked against
[value-api.md](../value-api.md) (R26, R27, §2, §9), [pyvalue.md](../pyvalue.md),
the briefs `V2A0-BRIEF.md` and `V2A0-ADDENDUM.md`, and the A5b and A3a
proposals. Code: `build/worktrees/pyvalue-rc-universal`, read-only, at HEAD
`36aa4a0345a`. The proposal counts at `faddfd1454d`; the one commit between
them touches `configure.ac`, `test_sys.py` and `sysmodule.c` and changes none
of the counts.

Probe (scratch directory since removed): one C file, GCC 15.2 `-O2`, Ryzen 7
3800X. It runs an indirect call through `obj->type->slots[k]` (inline) and
through `obj->type->tbl->slots[k]` (pointer), using 1,384-byte types and
65,536 objects spread over NT types. It is a microbenchmark. It bounds the
cost of the extra load; it does not measure the interpreter.

## Verdict

**Accept with changes.** V2a-0 as designed in §4 is sound and can be
implemented now. D1, D2, D8, D9, D10 and Q5 hold (findings 12–13). The route
in §1.6–§1.7 and the gate in §3 need three changes before they are adopted:

- **The wave order costs about 16 h of wall clock.** W4 waits for W3 although
  W3 produces none of the forms W4 needs, and W2 and W3 each edit the same
  core files (finding 1).
- **The gate's kind and owner rules would count boundary boxes as core debt.**
  With them the suite gate fails for reasons unrelated to R27 (finding 2).
- **W4 leaves out the value homes in `Modules/`.** Without them, zero is out
  of reach (finding 3).

The total of 367 agent-hours is roughly right. Some work can be removed, but
other work is missing (finding 1's table). The plan's real cost is wall clock.

## Findings

| # | Severity | Finding | Evidence | Fix |
|---|---|---|---|---|
| 1 | must-fix | **Dependency order.** §1.7 chains W2 → W3 → W4: 9 + 7.5 + 12.5 h of the 48 h critical path. W4 needs the forms of M3 (Clinic value emission, `METH_VALUE`), V2a-1a (`PyValue_FromInt64` …), W1-0, M1b and M2. It needs none of W3's core caller edits. Modules are leaf units that opt in with `Py_VALUE_CORE`. W2 and W3 both sweep `Objects/` and `Python/`, so each core file is edited twice, which contradicts P5 and R21 ("edit each file once"). | §1.6 "W4 … Waits for W3"; "W3 … Uses W1, W2 forms". W3's caller families (call 283, construction 414, `IsTrue` 43, `_PyNumber_Index` 25) need M3 and V2a-1a forms only | Merge W2 and W3 into one core wave that starts when M3 lands. Start W4 at the same point, in parallel, once the core Clinic regeneration has run, since it is the first use of the generator. Start M3 when V2a-0b (layout and pass) and M4 are in, not after V2a-0d. Run I3, I4 and I5 in parallel after I1: they convert disjoint handler families. Resulting schedule below |
| 2 | must-fix | **The gate misattributes boundary boxes.** (a) "Unclaimed means core" counts compatibility APIs that box for an extension caller as CORE when they do not use the three primitives. (b) An owner given by type or `ht_module` cannot be resolved for module-less heap types, and "a Python class is core" misattributes a Python subclass that inherits an extension's object slot. (c) `__builtin_return_address(0)` names the wrong frame after a sibling call or inlining. (d) Unresolved addresses count as core, so libffi callers (`ctypes.pythonapi`) fail. (e) `_PyValue_SwapBoxKind`, if it swaps thread state around the slot call, labels the extension's nested boxes with the adapter's kind | (a) `getargs.c:388`, `:600`, `:1780`, `:2438` box with `_PyBorrow_TupleItemsAt(…, BOX_SITE_PARSER)`; `_PyBorrow_Keep` at `:1867`, `:1990`, `:2471`. (b) `_testcapi/heaptype.c` makes 29 heap types with `PyType_FromSpec*` and no module | Make the owner the code address the boundary crosses into: the object-slot function the adapter calls, `ml_meth` or the vectorcall pointer for the call adapter, and the return address for a thunk. In stats builds, mark thunks `Py_NO_INLINE` and build with `-fno-optimize-sibling-calls` and no LTO; every extension in the suite is in-tree, so that is enough. A box inside a `Py_VALUE_COMPAT` function body uses a THUNK-kind primitive, and `check_value_api.py` checks this. Pass the kind as an argument to the box primitive, like the site, not as thread state. An address outside libpython and the in-tree `.so` files is external; only an unresolved address inside them counts as core |
| 3 | must-fix | **W4 omits value homes in `Modules/`.** A module that stores an inline value in a `PyObject *` home boxes: core debt under R27. value-api.md §7.4 lists the candidates for V6, but no wave or step owns them. In the most important case, `deque` is a container | `_collectionsmodule.c:131` `PyObject *data[BLOCKLEN]`; §7.4: `itertools` (`count`, `repeat`, `accumulate`, `groupby`, `tee` data, `zip_longest` fill value), `operator.itemgetter`, the `_pickle` memo. Also the `_pickle` unpickler's `Pdata` stack (`PyObject **`), `lru_cache` link nodes, and the `_asyncio` future result | Add W4-0: one lead agent converts the module homes with M4's field support, giving `deque` value forms as W1-0 does for list. Count the homes first, with the §7.4 member regex run over `Modules/`. Reserve 15–25 agent-hours until the count exists |
| 4 | should-fix | **W1b runs into V2a-0 and M4.** W1b (T4–T10) edits files that V2a-0 (T0–T7) and M4 have in flight. The collision table in §2 lists neither. W1b's batches also include `Python/bytecodes.c`, which belongs to the deferred `ceval.c` unit and is A3a's | `wave-core.tsv` container calls: `typeobject.c` 102, `abstract.c` 8, `object.c` 5, `descrobject.c` 2, `sliceobject.c` 2; `bytecodes.c` is in W1b's batch 0 | Put the V2a-0 and M4 files in W1b's last batch, after those steps land. Remove `bytecodes.c` and every file in the `ceval.c` unit from the wave batch generator |
| 5 | should-fix | **V2a-1a-iii needs a value call that does not exist yet.** The value `slot_*` functions call `__add__` and the other dunders. HEAD has no `_PyValue_Vectorcall`: it arrives in M3. Yet the graph shows V2a-1a needing only M1a | `rg _PyValue_Vectorcall` finds nothing at HEAD; value-api.md §2.4: the `slot_*` functions "call it with the value vectorcall (§3)" | Move V2a-1a-iii after M3, or state the interim path: boxing through the call adapter, counted and recorded. W2 does not need V2a-1a-iii |
| 6 | should-fix | **Q2: drop the typed unary wrappers.** For `repr`, `str` and `hash` of a type whose receiver is never inline, the adapter already passes the pointer and boxes nothing, so `_Py_VALUE_TYPED_*` adds 99 dual forms that achieve nothing. R1's rule, "concrete means a typed pointer", already makes the object slot the principled form there. The same applies to the unary slots of module types in W4 | §1.6 W2: repr 67, str 11, hash 21; §3.3: "The adapter boxes nothing for an object receiver" | Keep value sides only where an operand or a result can be inline: binary and ternary number slots, compare, `subscript`, `item`, `contains`, `iternext`, `send`, attributes, and int and float generally. Replace the end-of-W2 assertion "the adapter never runs for a static builtin type" with "the adapter never boxes for one" |
| 7 | should-fix | **The worker rules miss the semantic traps.** The cheap-worker contract covers retyping and the `IsObject` test, but not these: (a) a borrow read from a heap home and passed to code that can run Python must keep an owner (upstream's `Py_INCREF` around a compare); (b) pointer identity becomes `PyValue_Is`; (c) forwarding slots must not become NotImplemented wrappers; (d) the NotImplemented-for-inline choice is left to each worker's judgement (R1) | (a) the `_PyValue_Hash` comment, `value_ops.c:22–23` ("an owner in case `__hash__` removes the value"); `list_contains`'s `Py_INCREF(item)` upstream. (c) `weakref.proxy`'s `WRAP_BINARY` forwards `proxy + 5` | Add (a)–(c) to the rules header, each with a before and after example. Have the lead or a script choose the NotImplemented wrapper from a fixed list of first-statement patterns (`CHECK_BINOP`, `if (!PyX_Check(v) \|\| !PyX_Check(w)) Py_RETURN_NOTIMPLEMENTED`). Every other binary slot goes to a worker as a value-semantics site |
| 8 | should-fix | **The R27 zero is out of reach on two known paths.** (a) Sequence concatenation has no value side, so the dispatcher's `sq_concat` fallback boxes an inline operand on error paths the suite tests (`[] + 1`, `'a' + 1`, `b'' + 1`, `bytearray += 1`). (b) `_ctypes`' `py_object` conversions (`py_object(1000)`, `pythonapi` calls) exit to foreign C by design, yet R27 counts `_ctypes` as core | §2.5 "Sequence slots are object slots, called with a boxed operand only if the other operand is inline"; `test_ctypes` uses `pythonapi` | (a) Add `sq_concat` and `sq_inplace_concat` value sides, `(PyObject *, PyValueBorrow)`, for the four or five core sequences (2 fields). (b) Define ctypes' object exits as CALL_ADAPTER boxes whose owner is foreign. List any further paths that only the suite run finds as W5 remnant, and size W5 for that tail |
| 9 | should-fix | **Q6: drop the `PyType_Modified` recomputation.** It is speculative. Nothing in the tree writes an object slot after `PyType_Ready`, and Cython, which does, is out of reach until ports resume after 10b. It also needs a per-type "own fields" mask that §4.4 does not place, and an atomic write that free threading, a non-goal, would demand | `rg` for `->tp_*`/`nb_* =` outside `typeobject.c` finds only `structseq.c:606`, which runs before ready; pyvalue.md D6 makes free threading a non-goal | Keep the debug pair assertion in the dispatchers. Record the recomputation as a ports-step item. For the same reason, remove "free-threaded `queue_slot_update`" from §4.5 |
| 10 | should-fix | **Q4 contradicts R27 and adds an allowlist.** Treating the limited-API in-tree modules as extensions breaks "every in-tree module is core" and creates the exemption that C6 rejects. They are tiny | `errno`, `_stat`, `termios`, `_statistics`, `_scproxy`, `_multiprocessing/posixshmem`; `_statistics._normal_dist_inv_cdf` takes floats | Port the six in W4b: drop `Py_LIMITED_API` in the fork and use Clinic value emission (under 2 agent-hours). The three limited-API test modules stay object-form |
| 11 | note | **Schedule arithmetic.** (a) A3a starts at T4 and its own estimate is 12 h of wall clock, so it lands near T16, not T12. (b) I2 cannot land "in the gap before A3a branches", because M4 (T0–T6) is ready after A3a branches at T4. (c) The P6 ranking rests on one kernel | A3a proposal §6; §1.7's windows. The slice family is 3.46M of 3.49M from fannkuch; fannkuch is 5.95M (41%) of the 14.39M | Re-date the interpreter steps from T16. Land I2 after A3a, or have A3a rebase over it (two handlers). Say "slices: 24%, nearly all fannkuch" |
| 12 | note | **D1: keep the inline fields.** The probe shows inline is never slower. A per-type pointer table costs up to 0.9 ns per dispatch while the types are cached, and 3–4 ns when they are not. A shared table, which saves the memory, needs copy-on-write inheritance to stay shared. Per-suite tables (value fields appended to `PyNumberMethods` and the others) cost a load and a NULL test, as upstream does, and save nothing for heap types, which embed their suites. ABI is equal: every option moves the `ht_*` fields and needs the SOABI bump | Probe, ns per call (inline / pointer). NT=8, predicted: 1.50 / 1.49; mispredicted: 4.74 / 5.18. NT=1,000: 1.49 / 1.74; mispredicted: 5.37 / 6.27. NT=20,000: 2.95 / 7.02; mispredicted: 6.70 / 9.70. Shared table at NT=20,000: 3.18 / 3.11. A minimal class costs 1,744 B (`sys.getsizeof`), so +424 B is +24% | Keep D1. About 12–15 of the 51 fields are on hot generic paths: the common binary ops, `richcompare`, `hash`, `nb_bool`, `nb_index`, `getattro`, `subscript`, `ass_subscript`, `contains`, `iternext`, `call`. Order them first so that they share two cache lines. Optionally, declare the public field as opaque function-pointer storage, with typed access in `pycore_value_slots.h`, so that no `PyValue` signature enters a public header (R10's intent). That also makes the R26 edit unnecessary |
| 13 | note | **The value-slot contract holds.** D8 (deduplicate on object slots) is right and §2.5 is wrong: under the invariant, equal object slots imply equal value slots or zero. Comparing the chosen functions runs a copied object slot twice. Reflected operations, a subclass going first (`PyType_IsSubtype(PyValue_TYPE(w), PyValue_TYPE(v))`) and NotImplemented through the adapter (`FromObjectSteal` of the immortal, then `PyValue_Is`) port exactly. So does `__index__` returning any int. Inline int hashes equal float hashes (`value_ops.c:12–20`, `_Py_HashDouble`). Errors return `PyValue_NULL` | `value_ops.c:10–28`, `:30–53`; §4.6 | State finding 7(a) as a contract line in §4.6: "a `PyValueBorrow` argument stays valid for the whole call, including Python code run by a slot; the caller holds the owner". Add tests: an extension type with only an object `nb_index`, and an `__eq__` that deletes its dict key during lookup, through the adapter |
| 14 | note | **Compatibility and layout.** D15: the fork already has its own tag (`SOABI=boilvalrc1-316-…`, `configure.ac:7032`), so the change is a bump to `rc2`. Make the bump a standing rule for any public-layout change, not a V2a-0 decision. Q3: agree with moving `PySliceObject` to internal; a sixth frozen field contradicts R26. M4's file list misses the struct's in-tree users. Limited API: pyvalue.md D6 already leaves abi3 binaries unsupported | `rg PySliceObject`: `rangeobject.c`, `codeobject.c`, `compile.c`, `marshal.c`, `_ctypes/_ctypes.c` | Add those five files to M4. In D15, say "bump" |
| 15 | note | **Collisions (item 6).** "V2a-0 can start now" holds for implementation. Landing has three dependencies the proposal does not list: (a) A5b-3's `public-value` rule must accept function-pointer fields, so V2a-0 edits `check_value_api.py`, which is missing from §4.9; (b) `configure.ac`, and the regenerated `configure`, is shared with A3.2 (`musttail` check) and A3.3 (computed-goto options), so "Shared with A3a: none" is false; (c) A5b-3 and V2a-0 both rewrite `_PyValue_Hash`, as stated | A5b §8.1; A3a §6; `value_ops.c:24` | List them in §4.9 and §2. Land the SOABI bump on its own, or regenerate `configure` at the rebase |
| 16 | note | **More of W4 can be scripted.** Number construction is 1,037 of W4's 3,952 calls (26%; 142 of `posixmodule.c`'s 271). Most of these return the number or pass it straight to a setter | `wave-modules.tsv`, `construct` column | Add script classes: `return PyLong_From*/PyFloat_FromDouble/PyBool_FromLong(…)` in a value-returning function, and Clinic return converters (`-> int`, `-> double`), which emit the value form. Re-measure the script share on W4's pilot batch |
| 17 | note | **Spec slot ids (D13).** The source is `Python/slots.toml` (generator inputs), and `Python/slots_generated.c` is also generated. §4.9 names neither. Internal ids need a reserved range so that upstream merges cannot collide with them | `Tools/build/generate_slots.py:20–24` | Name both files; reserve a range of internal ids |

## Recounts (item 7)

| Claim | Recount at `36aa4a0345a` | Verdict |
|---|---|---|
| 102 generated box calls over 65 targets: 82 `PyValue_Box`, 19 `_PyValueTemps_Object`, 1 `PyValue_AsObject` | `gen_sites.py`: identical | holds |
| 128 hand-written core rows, 18 of them in `bytecodes.c`; 29 in `Modules/` | `hand_sites.py . Objects Python`: 128, `bytecodes.c` 18. `Include/` adds 16 rows, all primitive definitions | holds |
| 2,166 core object-form calls (+53 in Clinic headers); 2,711 in `Modules/` | `valueapi-objcalls.py`: 2,166 and 53; `Modules`: 2,711 | holds |
| 14.39M boxes per pass; the family table | `family_rank.py < ranked.txt`: 14,392,802, with every family equal to §1.2. `ranked.txt` drops sites under 500, a 314 difference. The `BUILD_SLICE` lines (`:4259`, `:4267`) are `TARGET(BUILD_SLICE)` at `dbce7b1260d` | holds; see 11(c) |
| W4: 135 files, 3,965 calls | With the stated exclusions: 3,952 calls in 134 files. These include `_testinternalcapi*` (318 calls), which the gate treats as test code; `_xxtestfuzz` (27); and the Windows- or macOS-only `_winapi.c`, `overlapped.c` and `_scproxy.c` (66), which a Linux build cannot list | about 10% too high for a Linux, compiler-driven wave. State how platform-only files are swept |

## Size and wall clock (item 1)

| Part | Proposal (agent-h) | Review estimate | Why |
|---|---:|---:|---|
| Mechanism (M1a, V2a-1a, M1b, M2, M4, M3, G) | 51.5 | 50 | Q6 recomputation and the FT queue out (9); `sq_concat` sides in (8) |
| Interpreter I1–I5 | 24 | 24 | unchanged; run in parallel after I1 |
| W1a + W1b | 23 | 23 | unchanged; reordered (4) |
| W2 + W3, merged | 121 | 105–115 | 99 typed unary wrappers out (6); the NotImplemented class done by script (7) |
| W4 | 136 | 120–135 | platform and test files out, no unary module slots, return and construct scripts (6, 16); module homes in, +15–25 (3) |
| W5 | 12 | 15–20 | the suite-only tail (8) |
| **Total** | **367** | **about 340–370** | |

Schedule with finding 1. It keeps the proposal's assumptions: A5b-3 lands
at T3, and A3a starts at T4 and lands at T16, per its own estimate.

| Window | Running |
|---|---|
| T0–T7 | V2a-0 (0a and 0b by about T4); M4; A5b-3, then W1a; A3a from T4 |
| T4–T10 | W1b, with the V2a-0 and M4 files last |
| T6–T16 | M3 (after V2a-0b and M4) |
| T7–T13 | V2a-1a (i and ii), M1b, M2, G |
| T16–T26 | merged core wave (W2 + W3), about 16 workers; V2a-1a-iii |
| T16–T28 | W4-0 (module homes), then W4a and W4b, 16 workers |
| T16–T28 | I1, then I3, I4 and I5 in parallel |
| T28–T32 | W5 and the gate |

The R27 gate is reachable at about T32, against T48. Wall clock is set by
the interpreter chain and W4, not by the core waves. One builder can serve
both waves: a `make -k` round covers `Objects/`, `Python/` and `Modules/` at
once.

## What holds

- **The two kinds of step.** The wave protocol follows the user's direction:
  marking makes the compiler list every site, one builder runs the builds,
  workers edit disjoint file batches, and a pilot batch opens each wave.
  P3's visible exits and P4's counted defer line keep the ratchet honest.
- **The inheritance pass and invariant** (§4.4, D4) implement R14, and
  `slotdef_values[]` (D5) keeps `struct wrapperbase` intact.
- **Q5** is right: a cached small int crosses the representation through an
  object form, so it counts as a box. Without it, a suite exercised only by
  small ints would hide the site. `box_cached` keeps the cost visible.
- **"Unclaimed means core"** is the right default once finding 2 makes every
  compatibility API claim its kind: errors then fall on the conservative
  side.
- **The kernel check** (zero of every kind except TEST) is achievable after
  W4, because the kernels run only in-tree code.
