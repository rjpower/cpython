# Bounded value-ingress audit before 6e/H1 guard simplification

Source-only review, 2026-10-29. No actual canonical-value leak was found in
the central paths examined. This is a finite ingress/return audit, not a
proof over every native method or container writer, and grants no code,
assertion, test, build or benchmark change. Revalidate against the actual
integrated ROOTS/H3 source before H1 implementation. Current ROOTS source
and native products remain frozen.

## Inspected identities

Primary source HEAD: `07e5143af70a5e9c01d96166fff2ffb0ba6565aa`.
The inspected native source files are unchanged locally; concurrent work
is documentation. References below use that primary source's line numbers.
ROOTS HEAD is `ce2ac8991eebb1bbb10698ff2d07a79bd4cba452`; its source differs
in root/lifetime work, so the primary audit is not a native qualification
of an integrated union. Read-only spot checks of its named canonicalizer,
call-result checker and value-native bridges preserve the distinctions
below (its specialized helper lines are 895–972).

| Primary file | SHA256 |
|---|---|
| `Include/pyvalue.h` | `730f979ee22f21c0e15ee161cfe596fd1bcc7a1b0d256d0f3e2dae746990f18e` |
| `Include/internal/pycore_value.h` | `ab8afeadb169715b3c30bba39c9de5487922c60d9ad357d3515d0ec88e59706f` |
| `Include/internal/pycore_call.h` | `4dffcd9cbb06f7856036c5717f861838237a8e4cd81a11fefe492fc26c90addb` |
| `Include/internal/pycore_interpframe.h` | `05c481518b1be0cf6bea2688d5097882c51f718013baf7f1f6548eda55626f44` |
| `Include/methodobject.h` | `9c34026f1ad832930181a63b0b02805366d329aee4aa83f82fa9c29f943a5749` |
| `Python/pyvalue.c` | `a9a7f73e3cec8bc9af3043cf76e3afc43d3fc63419f6a307306c72887e53f8de` |
| `Python/ceval.c` | `112150c7a927ac15aa5e882a3bb96b44984e016f918f446b59b0fdbacfb572ce` |
| `Python/bytecodes.c` | `3958ee7c3bc3929fab9dd4f53d344a7d6c02640eeeaab05586a04ca18b3969af` |
| `Objects/call.c` | `596e9eda1650f30e478d756fdf62686e443039f58ef89b49fa6aa02161569d16` |
| `Objects/methodobject.c` | `c33d4219252549815f08ebc4f9ad852dc46b72288b4dd3d2453f7c960c89a91a` |
| `Objects/descrobject.c` | `7ec00c4c2695e054bd49a925aa322e4e0a0c6b4370da4f0967c2bc24fa1b6589` |
| `Objects/abstract.c` | `292ca46901e82b7664121ebcafbefa270126291fe4d4b6b5a1506a0dfd46d5f1` |
| `Objects/enumobject.c` | `d807c0407bc5451c924ff218f49e2bcde2655e027f3f1a4a93adb9a8c271882b` |

Native identity is recorded only from ROOTS' frozen affected plan:
source digest `6462a11b9655678b1a58858fb4065eb806858564e7433d300873e54dfa19971e`;
default Python `eafcda7aba3e97eacf1cc3fb249c7e3b9c2f68117174f1f448d7e181dd752b40`,
debug `bee0f693ee62c084088c78674fb4f01f25590d41a2203ddba2d2a6d4cd842b0f`,
ASan `d6a71a955c9235a5ea1ed7883f0e32b568291642601105a752ba581df5e962f1`.
These are separately frozen ROOTS build products whose qualification has its
own campaign, not native evidence for a proposed ingress assertion or
simplified guard. No native code was run here.

## What already canonicalizes

`Include/pyvalue.h:208–232` defines `_PyValue_FromObject` as the canonicalizer
itself; `PyValue_FromObject` is its casting macro. Exact compact ints become
inline immediately. Noncompact exact ints go to `Python/pyvalue.c:120`, which
also inlines any int32 value, including an int32 requiring two PyLong digits.
Exact non-NaN floats become inline; bool, int/float subclasses and boxed NaN
retain object identity. `PyValue_FromResult` at 237 handles NULL then uses the
same implementation. Note14 rule3's suspicion of an unchecked
`_PyValue_FromObject` is historical/stale; it is not an identified current leak.

`Python/pyvalue.c:134–178` routes `_PyValue_BoxInt64`, `_PyValue_BoxUInt64`,
`_PyValue_LshiftInt32` and `_PyValue_BoxDouble` results through the canonicalizer.
Their names do not establish a retained small numeric box. Arithmetic overflow
paths in `bytecodes.c:639–685`, range iteration at 3736–3750 and enum at 158–168
therefore do not identify a leak. Unary float negation's raw bits at 419 derive
from an inline non-NaN float, not a heap-object producer.

Central object-return conversions also canonicalize: legacy eval arguments
at `ceval.c:2056/2065`, object-array conversion at `pycore_value.h:317`, generator
send results at `bytecodes.c:1577`, method NOARGS at 4842 and native NOARGS at
`methodobject.c:506`. Tuple/list/set/dict/cell/type/generator constructors at
their explicit `PyValue_FromObject` uses are known non-int result types; they
are not dynamic small-int ingresses to pursue merely because a cast appears.

## Dynamic boundaries that trust the value contract

| Boundary | Current evidence and proposed debug check |
|---|---|
| Generic vectorcall, `pycore_call.h:150–169`; `call.c:99` | Native vectorcall returns a `PyValue`; `_Py_CheckFunctionResultValue` checks NULL versus exception, then returns it unchanged. An allocation-free canonical-result assertion here would cover these generic calls, without converting their result. |
| Value-native METH_O/FASTCALL/METHOD, `methodobject.c:426–535`, `descrobject.c:389–508` | The ABI explicitly returns PyValue (`methodobject.h:23–33`); methods return their word directly. A buggy value-native producer could return a raw exact numeric pointer. This is a contract boundary, not evidence that any inspected method does so. |
| Specialized builtins/classes, `ceval.c:829–906` | Fast, keyword, descriptor and builtin-class function pointers return PyValue with only NULL/exception checks. These bypass the generic result checker and need the same proposed assertion at their return boundary. |
| Direct specialized METH_O, `bytecodes.c:4492/4722` | Direct builtin/descriptor calls similarly bypass that checker. Check before publishing the result; do not silently canonicalize away an ABI violation. |
| Frame arguments/returns, `ceval.c:1643`, `pycore_interpframe.h:197` | Argument copies and `_PyFrame_SetReturnValue` transfer existing PyValues. Debug checks here can detect a missed ingress before a semantic frame slot is used. Return/yield early stores must still anchor the value before unlinking/collection. |

Existing `_PyValue_CheckSlotResult` (`pycore_value.h:107`) already rejects a
noncanonical object result at tp_call/getter/descriptor dispatch sites; it
does not cover every vectorcall or specialized native path. Reuse its
allocation-free representation test concept, with useful boundary identity,
rather than introduce an object conversion. Review checker call effects and
root ownership for any actual new assertion implementation separately.

The int guards at `bytecodes.c:624/629` use `_PyValue_IsCompactInt` and
`_PyValue_CompactInt` (`pycore_value.h:185–211`), accepting a second exact-heap
compact representation. Before removing that arm, a debug operand assertion
should make a forbidden heap form fail visibly rather than silently deopt.
Two-digit-but-int32 values require the ingress/result checks too; a compact-
only diagnostic cannot prove canonicality for all int32 values. Legitimate
large ints still fail specialization normally; bool/subclass semantics stay.
This is a proof prerequisite, not authorization to simplify those guards now.

## Raw roots are a different domain

`_PyValue_FromBits` is the unchecked word builder. Its object-pointer use in
`_PyValueSpan_AppendObject` (`pycore_value.h:379`) is intentional: it anchors
the actual box that C code may point into. Root `.object` assignments do the
same. `_PyValue_ObjectRooted` explicitly says that a root boxed in place must
not subsequently be used as a semantic PyValue. **Canonicalizing these raw
anchors is forbidden:** replacing a box pointer with an immediate could let
the actual box die while C still uses it. Do not add a blanket canonicality
assertion to root/span append/pop or all initialized-span elements. NULL,
internal iterator/lasti indices and wrapped/error words also need their
existing domain rules rather than a universal semantic-value assertion.

## Proposed falsifying controls, not executed

Use copied value-native METH_O, FASTCALL and vectorcall producers returning
`_PyValue_FromBits((uintptr_t)PyLong_FromLong(7))`: the proposed debug boundary
must reject before a semantic slot receives it. A canonical `PyValue_FromResult`
control with the same result must pass. Include a two-digit int32, both int32
edges and overflow to distinguish legitimate large boxes. Exercise generic
and specialized calls so the latter cannot hide behind generic coverage.
Include +/-zero, +/-infinity, NaN, bool and numeric subclasses with their intended
identities. A separate raw-root liveness positive must retain an actual small
numeric box across collection while the semantic-slot negative still fails.
Pair a frame-return/argument injection with its normal canonical equivalent.
No fixed repetition count or speed prediction follows from these controls.
