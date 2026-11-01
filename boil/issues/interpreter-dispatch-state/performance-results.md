# Slot-base elapsed result: rejected

2026-11-01. Reject source `3ddc437679ebbb9f8629c1670655c58776075dfc`.
The fixed comparison failed the predeclared coroutine primary in both rounds.

| Workload | Round 0 elapsed change | Round 1 elapsed change |
| --- | ---: | ---: |
| Coroutines | +11.98% | +11.00% |
| Generators | +26.13% | +26.71% |
| Unpack sequence | +47.75% | +52.77% |
| Spectral norm | +11.04% | +8.80% |

Positive means slower. These are four selected workloads, not a broad-suite
result. The intended outgoing slot-cache stores disappeared, while separate
native inspection observed additional common frame/IP traffic. This result
does not identify the causal compiler pass, dynamic instruction contribution
or cache effect. Code size was diagnostic, not a rejection threshold.

Root activated the independently reviewed plan at 00:39:23.851891 UTC with a
45-minute deadline. Eight unscored origin processes preceded sixteen scored
processes containing eighty timed calls. All completed with idle cleanup by
00:40:41.102 UTC. Root observed session 30844 terminal exit 0 and independently
confirmed no remaining target/compiler processes. Independent review replayed
191 artifacts, all raw durations and forty-eight quiet checks. The fixed
primary and descriptive adequacy checks fail; all three controls also regress
more than 2% in both rounds. No further samples were added.

Evidence master: `build/interpreter-slot-base-quiet-v1`.
Plan: `7682690de1669e4f2ce99afeb729eb844304d2ef80fdeba0c901bff9b7c3799e`.
Activation: `99f4be9e34916d07dbe64c09983245cbacde177affa2564148b346a730231c2c`.
Score: `f3c06723bff76bcd31ed975ae096aadb76ee2be526ffab57fe8c65a47ae08eb1`.
The [independent outcome](reviews/quiet-outcome-20261101-v1.md), SHA-256
`eaedeaace8e1edf2bdde39c35ae11cddd104f7d2609ee1af22984e1a72eade60`,
owns the detailed replay and its limits. The earlier source/native correctness
receipts remain valid for their bounded scopes; they supply no landing credit.

The next question is the [explicit state ABI](../../design/vm-design-candidate.md),
under a new reviewed experiment. This rejection does not revive the rejected
constant-operand implementation or silently extend this campaign.
