# Hot85: the remaining slow cases, and golden84's hot code size

Approved 2026-11-21. After promoting [parity84](parity84.md)'s IP2 as
golden84, the user asked for two things:
- a breakdown, by opcode and code structure, of what still makes deepcopy and
  the other slow cases slower than upstream;
- a cleanup of the hot code size, with rare branches in hot handlers moved
  out to functions.

Base: golden84, `4be2eb5d31f`. Evidence `E59/hot85/` (`coordinator.md` is
the log; `common.md` and `brief-*.md` are the briefs). Gates are correctness
and the allocator limit (50M; golden84 is at 44.84M). Moving a rare path out
of line or declaring a rare helper cold is a structural change; padding,
alignment and layout searches are not.

## Phase A: analysis

| Unit | Content |
| --- | --- |
| LB | deepcopy, deepcopy_reduce, deepcopy_memo, logging_format, logging_simple, json_dumps, json_loads, pickle, unpickle: excess per loop by opcode family, by native symbol group and by Python function and line; exact ledgers of the five largest operations |
| XK | xml_etree_{iterparse,parse,process,generate}, nqueens, comprehensions, coroutines, regex_compile, deltablue: the same method, and the excess by source construct |
| HC | cycles, instructions and front-end counters on CPU 15 for all 29 (golden84, golden82, upstream); spectral_norm's op-cache mechanism; the hot footprint by function and handler; an inventory of rare paths in the 40 hottest handlers and 20 hottest helpers, with a proposed form per family; one prototype family in `worktrees/hot85-hc` |

## Phase B: hot-code size

Implementation units from HC's plan, each in a worktree from golden84
(`E59/hot85/mkbuild.sh`), integrated on `pyjit/hot85`, with full suites,
instructions for all 29 and the timed all-29 against golden84 and upstream.
