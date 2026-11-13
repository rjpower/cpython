# Validated option 5: independent generated and native review

Source/native GO for the intended bounded transformation. This is not timing
admission. Source is clean commit
`e95328ba1d6f3ea74dd92d0870213367287d64ef` in
`BASE/worktrees/frame54-clears-validated`. Both
`clears-validated-initial-{release,debug}-build.log` finish with 117 checked
modules and zero failed imports. Root owns all execution/qualification;
this reviewer used only source reads, nm, readelf, objdump and non-running
GDB type information. No inferior, generator, compiler or test was run.

| Matching executable | SHA256 | ELF build ID |
| --- | --- | --- |
| BASE/frame54-clears-validated-release/python | ec5afe84413004d1da4471163eb13e19fffa67f8dcae7e10dc9c966ec698a83a | 54e7bcd1acb1cd299b33f28685e51a6b5d77acfb |
| BASE/frame54-clears-validated-debug/python | 321c78e0c28abe3bb5562afc5409258255fef5c3ccada70a20901c7dc5c2d2ba | 05dbb73b5654b50b8f02ee0f1db08f504f6d4514 |

## Generated and actual release route

The generated handler contains only the shared geometry assertion before
IR_VIEW; it has no release fallback to ADD_INT. Both inline guards, fallible
boxing and its NULL/error test precede B adoption and C consumption. The
certified narrow B overwrite remains the final publication. The strict
source-body certificate and ordinary integer body were not weakened.

The actual release variant starts at `0x344ab0`, size296 bytes (`0x128`).
There is no A/B comparison, B/C alias comparison, B-discard comparison or
B-local-boundary test anywhere in the handler, and no geometry fallback
edge to generic ADD_INT. The remaining prefix/local-boundary load serves
C consumption, which is still required.

- C address is computed into r15 at `0x344adc`; its inline guard is the
  comparison/branch at `0x344ae3/0x344ae6`.
- B address is computed into rbx at `0x344af9`; its inline guard remains at
  `0x344aff/0x344b02`. Both misses share the ordinary feedback-driven
  deoptimization dispatch beginning `0x344b68`; operands are still intact.
- Integer addition and overflow branch are `0x344b17/0x344b1a`.
  Overflow calls `_PyValue_BoxInt64` at `0x344bab`; the result NULL test
  at `0x344bb8` occurs before either home changes. NonNULL success rejoins
  at `0x344b2d`, and NULL failure tail-enters error at `0x344bd3`.
- C's local/scratch classification compares the execution-prefix local
  boundary at `0x344b38`, skipping its clear for a local. The sole home
  NULL store is `49 c7 07 00 00 00 00` at `0x344b3e`, to C through r15.
- The success result store is `48 89 3b` at `0x344b45`, symbol offset
  `+0x95`, to physical B through rbx. There is no B-to-NULL store.
  No call, allocation, error edge or observation separates the final C
  clear from that B publication.

Generic ADD_INT remains642 bytes at `0x356110` with its ordinary separate
B/C clears at `0x3561b6/0x3561c9`. The specialized variant is additional
text, not removal of the generic handler. Native inline closes were already
absent and are not a further saving.

## Geometry, stack and moved cost

Both GIL ELFs retain fixed header112 bytes, sizeof120 including the declared
trailing home. No frame field or owned-home capacity changes here.

| Release text / stack reserve | Initial four-check S | Validated S |
| --- | --- | --- |
| Overwrite variant text |380 bytes |296 bytes |
| Overwrite variant rsp local reserve |32 bytes |32 bytes |
| Generic ADD_INT text / rsp reserve |642 /48 bytes |642 /48 bytes |
| CALL_PY_EXACT_ARGS text |973 bytes |973 bytes |
| RETURN_GENERATOR text |1002 bytes |1002 bytes |
| specialize text |447 bytes |495 bytes |
| _PyCode_Validate text |7840 bytes |8081 bytes |

The source selection proof becomes actual native work. In release
`specialize` at `0x450390`, the ADD_INT selection region loads pinned code,
its physical frame size (`0x450490`, subtracting14 header words at
`0x45049e`) and full localsplus boundary (`0x4504c2`). It proves A==B,
C!=B, both sentinels excluded, B>=localsplus and B/C<the physical extent
before selecting handler326 at `0x4504e0`. These include the added bounds
missing from the earlier selector. They occur on selection, not at every
successful variant dispatch. Its prologue retains the same saved rbx and
8-byte rsp local reserve. Common validation adds cold constructed-refinement
checks; its symbol grows241 bytes. These text differences are not executed
instruction or cycle counts.

## Debug contract and limits

The matching debug handler at `0x393f23`, size1379 bytes (`0x563`), retains
the complete shared geometry assertion. A/B equality is tested at
`0x393f8e`; C inequality, both read bounds, local boundary and physical
extent checks at `0x394034`–`0x394073` lead to `__assert_fail` before input
views. Its 64-byte rsp local reserve includes debug-only validation and
ownership checks and is not comparable to release throughput.

The ingress/hidden-constant proof remains in
`option5-validated-post-source.md`: common constructor validation covers
marshal as well as public constructors, original-word restoration preserves
that proof, polymorphic rows remain ordinary ADD_INT, and hidden constant
homes lie below the full localsplus boundary. The original four-check S
identity and measurements remain separate evidence. Root must supply focused
behavior results, reached-success census and balanced unchanged-workload
measurements before deciding admission. No elapsed cause or performance win
is inferred here.
