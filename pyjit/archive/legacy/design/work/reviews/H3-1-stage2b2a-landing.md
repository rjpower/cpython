# H3-1 stage2b.2a landing review

Independent read-only ROOTS review,2026-10-29, of frozen clean
`dc02f76c0ccf69cd7195431dc0335a349bdec48f`. Verdict: LAND_2A.
No reviewer tests, builds or edits. Native integration stays behind ROOTS2b.

The source removes only SafeAddr2Line's mutable monitoring-cache dependency,
keeping bounds-first immutable lookup, plus the approved private export and
exact additive CAPI helper/entry. Ordinary Addr2Line is unchanged. Removing
the hook restores the entire native-parent CAPI file. All seven generated
products and existing compiler/root-checker fixtures match their parent.

The reviewer checked56 source hashes,8 build/config/CAPI products and4 control
source hashes. Copied current/old helper bodies, patches and matching extension
products agree with the frozen evidence. Real instruction heads agree; old
cache lookup fails the intended valid synthetic-offset assertion for both
relaxed FOR_ITER and SEND on both builds. Invalid offsets never enter the old
helper. The new empty/bounds/collecting-monitor controls remain intact.

Final release/Og smoke passes1,924 each; affected suites pass1,519 per build
and actual-kill1,519, with11 workers reporting(2,0). All156 root records are
distinct physical PIDs:1,498collections, zero fallback/unpinned. Regeneration
is unchanged;19 optional syntax configurations pass, with four missing-header
families explicitly untested. Initial pre-relink smoke is kept separate from
the final candidate evidence. No committed-head rerun is inferred.

The detached frame->code->linetable root chain is sound for this change.
Emergency dead-code heuristics remain best effort, and monitored immutable
lookup's O(n) cost is unmeasured. No storage2b.2b, stable ABI, full emergency
reclamation or final H3/performance clearance is implied.
