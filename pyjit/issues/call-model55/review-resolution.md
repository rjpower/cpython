# Call-model55 design review resolution

Status: design contract accepted after independent post-review. This is a
reviewed proposal, not an implemented or measured runtime change. The
[design](../../design/proposals/register-call-model55.md) owns the target;
the [plan](plan.md) owns sequencing and the [peer report](peer-review.md)
retains the initial findings and final disposition.

Astra medium performed one broad architecture and primary-source review.
An independent Sol source audit checked the actual runtime; a separate Sol
reviewer challenged the complete proposal. The coordinator and source auditor
incorporated the corrections before independent post-review.

| Initial must-fix | Resolved contract |
| --- | --- |
| Unspecified phase/root schema and hot cost | PC plus literal uint8 family-assigned phase; bounded site tables; interned range maps; nullable outputs canonicalized; dynamic unpack uses a registered initialized-prefix cursor. No map decode per inline consume. |
| Incomplete roots during callbacks and retirement | Lazy nested transition records describe detached C/result owners, mapped remaining owners, prefix state and retiring reservations. Cursor/bit advances precede decref; external roots are distinct from container-edge subtraction. No ordinary borrowed mortal scratch bank. |
| Execution image identity | Existing independent code pin retains semantic/image authority without a new per-call image owner. Site geometry is stable; patches preserve certified contracts; geometry-changing transforms require a new validated code/image. |
| Operand encoding and public admission | Low16 handler selectors, concrete register/constant domains, CD24 LOAD_POOL and common construction/replace/marshal validation replace spare-bit assumptions and compiler-only trust. |
| Migration mixed unrelated changes | M0–M3 keep NULL authority while schema, roots and all readers migrate. M4 switches coherently; M5 compares equivalent bitmap; M6 adds optional lowering separately; M7 qualifies the composition against selected54 and upstream. |

Additional cost corrections retain useful caches and 112-byte GIL geometry,
type independent code/function pins and price new unbound-local initialization
when it overlaps stale scratch.

Final peer post-review found all five resolved at design level and no new
blocking invariant. Its remaining nonblocking scope note is clarified in M5:
the rival maintains mortal-owner bits for **all variable value homes**, named
locals as well as scratch. Both candidates keep identical initialized-local
boundness and initialized-value policies. Header and transition roots remain
separate. Measurements must include local bit maintenance.

The recommendation is static scratch maps plus initialized locals first,
with a complete physical-bitmap rival before representation selection. Prior
small experiments do not reject a bitmap. No exact CPython destruction order
or frontend stack-pop order is a language requirement. Initial retirement is
bounded by semantic operation completion rather than arbitrary frame exit;
K=8 bounds detached slots, not object bytes or destructor execution time.

Implementation gates remain open: generated/native publication cost, actual
collector accounting, malformed images, partial failures, callback mutation,
escaped/suspended frames, retention and performance. The peer checked seven
worked lifecycle traces; that does not replace implementation tests or prove
a speedup. Current golden54 and its qualification limits remain unchanged.
