# Parity84: the clean changes profile83 ranked

Approved 2026-11-21. After [profile83](profile83.md) ranked the clean changes
that remain against upstream, the user approved its proposed wave. Gates are
correctness and the allocator limit (50M); measurements are recorded and
attributed, not gated.

Base: golden82, `5194e957c5d`. Integration branch `pyjit/parity84`; evidence
`E59/parity84/` (`coordinator.md` is the log; `common.md` and `brief-*.md`
are the unit briefs).

## Units

| Unit | Content | Profile83 items | Phase | State |
| --- | --- | --- | --- | --- |
| NB | Native-call publication as one counted loop; one native→Python entry routine replacing the eight-layer chain; value callees called without object-API thunks; exceptions built from the argument window; `isinstance` with an exact-type exit and one value entry | A1–A5 | 1 | running |
| GL | Test the generators back-end stall (`IR_SKIP_RESUME`'s form) on CPU 15; specialize `GET_ITER` over a Python `__iter__`; create generators at the call; one "plain generator" bit for finishing; store the resume point at suspension | B1–B4 | 1 | running |
| IT | Specialized membership tests and compare exits; inline `FOR_ITER` for `tp_iternext` iterators; one truth test for None or plain objects; the unpack element loop | D1–D3, E6, E9 | 1 | running |
| RM | Allocation entry, GC visits and unicode-key traversal, dict first insert and unicode lookup, `makeuniversal`, `json.dumps` integers, the borrow stack's base chunk, big-int bitwise ops, `MAKE_FUNCTION` | E1–E5, E7, E8, E10 | 1 | running |
| BD | Per-site bind plans for defaults and keywords; `CALL_FUNCTION_EX` with a `**kwargs` dict binds in place | C1, C2 | 2, on NB's entry routine | waiting |

Hot-code size (profile83 group F) is not in this wave.

- **IP1:** merge NB, GL, IT and RM; full suites; instructions for all 29;
  timed all-29 against golden82 and upstream.
- **IP2:** merge BD; full suites; timed all-29 against golden82 and
  upstream; results here.
