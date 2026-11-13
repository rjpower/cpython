# Option 5 independent implementation review

Source GO for coordinator regeneration and release/debug qualification. This
review is independent of the author and covers the final handwritten diff in
`BASE/worktrees/frame54-clears`, based on golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. No generator, compiler, runtime,
test, profiler or timing was executed by this reviewer. Generated output and
native evidence remain pending root; this is not admission or a measured win.

The proposal/peer contract is implemented as one execution-only ADD_INT variant,
pinned to handler 326. The number fits both the 12-bit handler domain and uint16
metadata. ExecutionVariant inherits the ordinary specialized instruction and
format; canonical_handler/shadow_of map it to ADD_INT, then existing BaseOpcode
and instrumentation recover BINARY_OP. Compiler pristine words do not contain
the variant. Non-PRISTINE metadata is not a constructor admission check:
_PyCode_Validate/check_words accept valid handlers with extent/feedback checks.
The release geometry fallback is therefore required for forged variant words;
a future removal of that fallback requires explicit constructor admission.
Both fb_record and fb_put_row canonicalize the new variant before storing
polymorphic rows. Root must verify those generated tables and restoration routes.

Specialization checks A==B, C!=B, B at or above the actual local boundary and B
not discard. The generated release-mode fallback repeats the geometry before
IR_VIEW or ownership changes and tail-enters ordinary ADD_INT with this_instr.
The 12-bit slot fields and the rejected 4095 sentinel establish the declared
slot-domain bound; ordinary IR construction remains responsible for frame extent.
The existing two inline-integer guards still precede arithmetic and any adoption.

The certificate reuses canonical_inline_int_source rather than trusting an
operation name or adding another body whitelist. That proof checks actual input/
output shapes, guarded body tokens, successful overflow join, NULL/error test,
INPUTS_DEAD and both exact integer retirement suffixes. The additional validator
requires the two borrowed B/C inputs, A destination, no window and the exact
overwrite mode. Changes inserting unknown calls, polling, error/deopt exits or
retirement work fail the certificate in the new mutation tests. Result boxing
and its failure happen before adoption, while PRE homes remain intact.

On the successful certified route B is inline and owns no reference count.
Its C owner adopts bits without clearing the physical cell; C retains its normal
local-or-scratch consumption. C!=B prevents C consumption from vacating B. Both
retirement operations are proven inline-only, so no callback, allocation, poll
or exit can observe the suppressed clear. The result may be boxed; its owning
value is stored unchanged before the next observation. The output asserts the
actual preexisting inline input rather than an omitted intermediate NULL.
No mutable frame state, validity map, general teardown change or new owner mode
was introduced.

Review requested two concrete corrections, now present in the frozen source:
the overwrite emits the exact physical B expression, IR_SLOT(IR_B(ir_word)),
as Astra required for native alias certainty; and focused behavior now includes
two scratch inputs as well as C-local input. The AdoptTemp comment now describes
the new certified caller. Source git diff --check passes.

New behavior tests cover both input geometries, inline overflow to a boxed
result, guard miss, pristine-versus-executable identity, opcode observation,
local output and discard. Generator negatives inspect actual changed producer/
retirement bodies, and generated-text checks assert fallback before views,
error before adoption, preserved C consumption, absent B consumption and B
output. These tests have not run. B==C is explicitly rejected in emitted code;
the handwritten suite does not claim executed hostile-word injection coverage.

Root's remaining gates are regeneration and handler-domain/table checks;
release/debug compilation; focused generator/IR/value tests; generated success,
overflow/error and fallback inspection; instrumentation enable/disable and
canonical restore (including polymorphic feedback); and native confirmation
that B's NULL store disappears while accounting for geometry checks and extra
handler text. Count reached eligible sites outside timing before the common
screen/confirmation campaign. Reject no-hit or losing breadth. This source
review supplies no instruction, cycle or elapsed improvement claim.

## Debug publication correction

Independent source GO for the subsequent three-file correction relative to
the committed initial option5 implementation. Root's debug execution exposed
that final result flushing could use ordinary IR_STORE_TEMP despite the intended
old inline bits, because that path held its canonical proof in lay.canonical_result.
The correction handles the certified overwrite before generic publication and
requires the existing canonical_result, lay.canonical_result or canonical_outputs
proof. It emits IR_OVERWRITE_INLINE_INT_TEMP at physical B. That narrow macro
asserts the old input is inline and the incoming owner is nonNULL/canonical,
then stores and checks the result without closing the old count-free value.
Ordinary temporary NULL assertions remain intact. The new source test requires
this specific macro and rejects either ordinary temporary-publication macro.
This reviewer ran no generator/runtime/compiler/test for the correction; root
must regenerate and repeat the failed debug qualification before admission.

## Corrected release native review

The consumer/validity reviewer inspected clean source
`4fbd16a14621f2b5327a0bfde6c2198190e71428` and the matching
`BASE/frame54-clears-release/python`, build ID
`ccfd4d0857a76d19a4eb1783c27ebe005e9b525b`. Operations were read-only nm,
objdump, readelf and GDB type inspection; no inferior, build, test or timing
was run by that reviewer. Root reports matching builds complete and runs the
focused tests separately. This section does not preempt those results.

GDB confirms fixed header112 bytes and sizeof120 including the declared
trailing home. CALL_PY_EXACT_ARGS remains973 text bytes and RETURN_GENERATOR
1002, matching golden51. Generic ADD_INT remains642 bytes at0x355ed0.
The new overwrite variant is380 bytes at0x356160 with32-byte stack reservation,
against the generic handler's48 bytes. Both handlers remain in the executable;
the variant adds380 bytes of handler text, plus selection/metadata effects,
rather than shrinking generic ADD's text by262 bytes.

The successful variant route computes B's physical address into r15 at
0x3561ed and C's into r14 at0x3561da. Its sole home-NULL store is
`movq $0,(%r14)` at0x356230, conditional on C being scratch. It directly writes
the result to `(%r15)` at0x356237. There is no B-to-NULL store. Golden generic
ADD's separate B clear at0x355f76 and C clear at0x355f89 remain in its handler;
this confirms a native store is actually removed on the admitted variant route.
Inline input closes were already absent natively and are not additional savings.

Four geometry checks remain on each variant entry: masked A/B inequality at
0x356179/7e; masked B/C equality at0x356199/9e; B discard at0x3561ac/b1;
and B/local-boundary at0x3561bf/c2. They add field extraction/xor/mask work and
branches before the ordinary two inline-integer guards. The handler also still
loads the local boundary for C's consume classification. Its lower stack size
and removal of generic destination replacement/discard machinery do not erase
these recurring checks. Constructor-time validation is a separate proposal,
not a change to this measured candidate.

Overflow calls BoxInt64 at0x3562a9. The NULL/error test at0x3562b6 precedes both
home changes; failure tail-enters error at0x3562ca. Success rejoins at0x356224,
preserves C's consume rule, and stores the owning canonical result at B.
No call, allocation or error edge occurs between the final C clear and B result
store. The narrow publication macro adds no release canonicalization call or
old-value close. Native facts establish the intended route, not its frequency
or throughput. Untimed reached-route counts and unchanged workload measurements
remain required before admission.
