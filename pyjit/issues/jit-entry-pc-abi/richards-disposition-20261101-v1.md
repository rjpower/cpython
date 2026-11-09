# Richards regression: disposition before broader qualification

Keep source599897 held. The completed fixed four-workload campaign passes its
primary, GM4 and deletion-sensitivity rules, while Richards is slower in both
rounds (about 5.15% and 3.49%). That is a real unresolved control loss under the
declared >2% rule, not a result to discard or relabel as noise. The original
rejected source67f5d22 and its scores remain separate.

Richards has a substantially different call structure from unpack_sequence.
The registered run(2) invokes Richards.run(1) twice. Each inner run constructs
six tasks and calls schedule(), whose loop repeatedly calls
isTaskHoldingOrWaiting(), runTask(), isWaitingWithPacket() and a dynamically
selected task fn(). Task methods then call one another: qpkt() calls findtcb()
and addPacket(); release() calls findtcb(); packet queues call append_to(). The
source's successful-inner-iteration check expects 23,246 qpkt calls and 9,297
hold calls. Those four distinct method paths alone account for 79,035 Python
method entries per intended successful iteration (158,070 for registered
run(2)), before the scheduler, fn, release, queue and setup calls.

These are conditional source counts, not observed dynamic counts. In particular,
run() ignores Richards.run's returned boolean, so a successful elapsed command
does not itself establish those counters. No new counter or target invocation
was made. Exact source hash, ten relevant method locations/call expressions and
retained score rows are in source-facts.json. The source authorities are
richards.py:198–245, 364–424 and the unchanged registry's loops=2.

The universal TryEntry hook therefore has many plausible receiving entries here,
including prefixes dominated by attributes, state changes and calls rather than
three admitted integer arithmetic operations. The bounded decoder stops at
unsupported words; Probe caches structural rejection before the full observer
guards. Source appearances do not prove the compiled IR or per-code admission
state. Neither native frequency nor the proportion of entry cost is known.
Call interception/rejection, remaining register clobbers and evaluator layout
are competing explanations; recovered IP registers on selected ordinary paths
do not eliminate them. Static call structure does not establish the measured
slowdown's cause.

There are two honest immediate dispositions: reject general-purpose adoption
now, or retain the candidate only for one predeclared all-29 elapsed decision
gate before paying for publication and full stdlib qualification. Recommend the
latter as the next evidence step, with adoption still held. It tests whether the
four-workload gain generalizes; it does not explain away Richards. A passing
all-29 mean still requires an explicit recorded decision about any repeated
Richards or other workload loss. A known regression may be accepted as a named
tradeoff, or may cause rejection; it must not silently become unconditional
acceptance. No shorter region, runtime selector, extra samples or source repair
is part of that disposition.

For that separate gate, fix all 29 registered kernel names/variants/loop counts
before activation, compare only accepted RC parent versus source599897 under
matched GCC/O3, and reuse the existing controller/unchanged driver. Preserve raw
values, per-round equal-weight geometric means and exact single-value deletion
sensitivity. Specify all finite process/sample counts, order rotation, origin
warmups, RSS/whole-process residual and deadlines in advance. Keep per-workload
ratios visible, including the four prior controls; retain the old four-workload
campaign as its own decision record. No upstream arm, GC parity claim,
instruction-counter project, hardware prerequisite or broad test cohort is
needed to answer this limited question.
