# Three bounded experiments

Each stage receives a concrete source review, finite runtime question and
integrate/reject decision. None is an automatic execution grant for the next.

1. **Minimal functioning young collection (this issue).** Existing allocator,
   exact cell/tuple allocation age, explicit minor, conservative all-old roots,
   ordinary lifecycle/sweep and promotion in place. Validate useful data,
   young reclamation, old retention and eventual major cleanup. Integrate only
   a small opt-in functioning consumer; reject/re-scope if isolation or honest
   accounting cannot remain simple. No performance success is inferred.
2. **Broaden common coverage and reduce death work.** Inspect actual first
   results; admit generator/coroutine allocation plus embedded frames where
   lifecycle contracts close. Assess compact same-space/page metadata and
   cheap reclaim for completed storage. Preserve required native cleanup and
   side effects. Integrate a measured useful consumer or reject the chosen
   backend; cells/tuples alone do not establish broad coverage.
3. **Reduce old scanning and settle policy.** Replace the conservative old
   census with complete core barriers and scanned native-owner boundaries,
   using a full-trace oracle and missed-edge negatives. Measure old/native
   scan work, RSS/slack, pauses and major debt. Copying is an optional storage
   strategy with its own writable-alias/pinning proof, not a mandatory stage.

Reorder stages2/3 if the first executable consumer or captured cost proves
that another order is simpler. Keep one collector and one source authority.

## First feedback packet

Before native launch: freeze actual source/diff and supported-mode boundary,
obtain independent review, and send coordinator owned paths plus literal build
and test selection. Reuse existing O1/Py_DEBUG GCC build and aux-aware worker
recipes in owned project directories, normal slots/locks and shared caches.
Expected build roughly2–4min from prior cohorts; focused feedback should be
seconds, whole GC roughly1–2min. These are scheduling estimates, not a grant
or deadline promise. Stop unexpected failure and retain its first result.

No current serving/baseline/upstream images are rebuilt. No commit/push before
coordinator review. Later qualification scope follows the actual changed
contracts; an inactive metadata patch does not justify a full ladder.
