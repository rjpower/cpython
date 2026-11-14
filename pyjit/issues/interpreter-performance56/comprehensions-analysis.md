# Why comprehensions trails upstream

Current runtime `883524c180e4486b47df77ee2276c9276c0ec32c` takes
1.337402 times upstream's elapsed time in the selected56 all-29 campaign.
New matched three-pair counters give **1.464449× native instructions** and
**1.333309× user cycles** relative to upstream per workload loop. Both profiles and all six counter
invocations are valid with stable exact source/native identities.

This is a workload of nested lists/dicts, attribute reads and small calls, not
only a comprehension-dispatch test. Each tray makes 108 `dict.get` calls,
162 list appends, 54 dictionary subscriptions, 42 Python predicate calls and
18 generator objects. Setup is excluded. The operation counts come from the
unchanged 24-widget input and agree between freshly warmed upstream and candidate
execution; instrumentation for attribute counts ran separately from timing.

## Specific expensive paths

| Operation | Candidate flat samples | Concrete difference from upstream |
| --- | ---: | --- |
| `CALL_METHOD_DESCRIPTOR_FAST` | 5.82% | Re-enters general value-vectorcall after specialized callable/flags/self guards; repeats object/vectorcall and descriptor self/arity checks. |
| `FOR_ITER_LIST` | 5.79% | Out-of-line list element getter plus spills; generic publication still classifies a known canonical element and manages the direct register recipient. Getter samples 1.73% across all callers. |
| `LIST_APPEND` | 1.05% | Always calls an out-of-line append helper, which separately samples 2.78%; upstream inlines spare-capacity store/size increment and calls only the resize slow path. |
| `LOAD_ATTR_SHAPE_LOCAL_TEMP` | 3.87% | A frequent guarded read/acquire path; no isolated share of the upstream gap is established. |
| `CALL_PY_EXACT_ARGS` | 3.40% | Real predicate/generator calls retain admission, frame and ownership work. This is a secondary target here, rather than a measured explanation for the whole gap. |

These percentages describe candidate physical-symbol self samples. They cannot
be subtracted from upstream's monolithic evaluator or added into predicted
savings. The first three rows have concrete implementation/native differences;
only counterfactual experiments can quantify each one's contribution to 33.7%.

### Native method call and dictionary lookup

The inner source expression is:

```python
[id_to_widget.get(dwid) for dwid in w.derived_widget_ids]
```

It executes 108 times per tray. Warmed disassembly identifies
`LOAD_ATTR_METHOD_NO_DICT → LOAD_FAST_BORROW → CALL_METHOD_DESCRIPTOR_FAST`.
The candidate's specialized call checks descriptor flags and self, then calls
`_PyValue_VectorcallTstate`. That dispatcher again checks object kind,
vectorcall support/offset/target. `method_value_vectorcall_descr` then repeats
argument/self checks and the keyword check before invoking the method.
Upstream's specialized opcode loads the checked method target and uses the
narrow `_PyCallMethodDescriptorFast_StackRef` helper. The candidate descriptor
wrapper's existing recursion protection must be preserved when removing redispatch.

Inside the method, candidate `dict_get_impl` calls `PyDict_GetValue`, which
checks dictionary/key kind and cached-string hashing before the integer-key
path pins the dictionary and query and hashes the value. It then calls
`_PyDict_GetValue_KnownHash`, which checks the dictionary again and pins the
query again before lookup and result acquisition. Inline integer query pins
have no mortal refcount updates, but still introduce value classification and
control; the dictionary pin is a real increment/decrement. Upstream's
`dict_get_impl` hashes then goes directly to its lookup/result helper.

This value-native method path does not require boxing its inline integer key.
The extra work is dispatch, checks and ownership handling.

The candidate profile independently places 2.30% in `PyDict_GetValue`, 2.06%
in `_PyDict_GetValue_KnownHash`, and 1.17% in `dict_get`. This establishes that
those layers execute, not that every instruction in them is redundant. Key
hash/equality can call Python in the general case; an optimization must carry
valid existing owner/effect facts or use a guarded callback-free subcase.

### List iteration and append

The candidate getter `PyList_GET_VALUE` is a 13-byte out-of-line array read.
The retained native FOR_ITER_LIST body calls it with three stack spills/reloads and a
32-byte stack reservation. Upstream reads the element inline. Both acquire
one owning item; that required acquisition is not an extra candidate refcount.
After acquisition the candidate still executes generic publication, including
object/PyFloat/PyLong classification visible in native code, despite the list
storage already being canonical. It also installs/replaces the named local
recipient. Upstream directly publishes onto its stack; its later STORE_FAST
has its own ownership cost, so that work must be compared together.

Append is separately inefficient: `_PyList_AppendTakeValue` contains both the
capacity test and fast store but remains out of line. Upstream's internal list
header inlines those operations. Both consume the input owner without a new
DUP; there is no extra append refcount pair to remove.

Inlining a list getter was previously rejected because the combined runtime
regressed coroutines. That result remains valid. A new experiment must measure
the whole composition; this diagnosis is not permission to restore that patch.

## What this rules out and what to try

The warm inner list body executes six opcode words per derived ID versus
upstream's eight: FOR writes its local destination directly and the dictionary
receiver load disappears into the attribute operand.
The slowdown therefore cannot be inferred from Python bytecode count alone.
The common handlers and native boundaries execute too much machine code.

Generator creation/control copying samples 1.54%, FOR completion 1.37%, yielded
value 0.65% and generator iteration 0.56%. They remain targets, but the profile
does not support making generator teardown the leading explanation here.
Likewise, the common hash lookup is real useful work on both runtimes.

Prioritize (1) a checked value-native method-call entry that consumes the
specialized admission facts, (2) canonical list-iteration result transport,
and (3) a measured list append fast path. Review dictionary ownership scopes
as part of the first item. These are general contracts usable across workloads,
not comprehension-specific shortcuts. Establish per-change elapsed/counter
results and regression sentinels before attributing a fraction of the gap.

## Sources and evidence

Candidate source, pinned to 883:
[descriptor-call opcode](https://github.com/rjpower/cpython/blob/883524c180e4486b47df77ee2276c9276c0ec32c/Python/bytecodes.c#L4989),
[value dispatcher](https://github.com/rjpower/cpython/blob/883524c180e4486b47df77ee2276c9276c0ec32c/Include/internal/pycore_value_call.h#L108),
[descriptor wrapper](https://github.com/rjpower/cpython/blob/883524c180e4486b47df77ee2276c9276c0ec32c/Objects/descrobject.c#L568),
[dictionary entry](https://github.com/rjpower/cpython/blob/883524c180e4486b47df77ee2276c9276c0ec32c/Objects/dictobject.c#L2042),
[known-hash lookup](https://github.com/rjpower/cpython/blob/883524c180e4486b47df77ee2276c9276c0ec32c/Objects/dictobject.c#L1364),
[list iteration](https://github.com/rjpower/cpython/blob/883524c180e4486b47df77ee2276c9276c0ec32c/Python/bytecodes.c#L3844),
[list primitives](https://github.com/rjpower/cpython/blob/883524c180e4486b47df77ee2276c9276c0ec32c/Objects/listobject.c#L254).
Upstream:
[checked method call](https://github.com/rjpower/cpython/blob/3a440b65695615e9fd9912342b79cdd54fced0d2/Python/bytecodes.c#L5290),
[dictionary get](https://github.com/rjpower/cpython/blob/3a440b65695615e9fd9912342b79cdd54fced0d2/Objects/dictobject.c#L4795),
[inline append](https://github.com/rjpower/cpython/blob/3a440b65695615e9fd9912342b79cdd54fced0d2/Include/internal/pycore_list.h#L36).

[Handoff](../../HANDOFF.md) defines E56. New evidence is
E56/comprehensions57-{record,stat} and comprehensions57-inspection. Source/native audit excerpts are retained
under comprehensions57-source-audit; the getter flat bucket includes all callers.
The existing checked capture-checkpoint-perf wrapper admits the complete selected56
all-29/upstream campaign before profiling only this case. Record has two
invocations; stat has three alternating pairs. Affinity 15/hash 0/JIT disabled and
build/native identities match the selected campaign. Warm adaptive disassembly
and separate count probes are inspection only, not timed workloads. No runtime
source was changed, built or patched. Sol source/native reviewers independently
checked the method/list differences; root owns captures and this synthesis.
