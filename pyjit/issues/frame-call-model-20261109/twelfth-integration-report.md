# Twelfth integration: canonical frame transfers

Runtime `df12a12b131384976466310b7a6138417b7a36eb` carries proved canonical
values through plain yield and the final list-child result store. Both reuse
the existing canonical sink machinery. The list handler keeps its early
publication before retiring the list; instrumented yield is unchanged.
There is no new frame field, ownership protocol, instruction or metadata row.

## Source proof and validation

A proof matches the complete semantic chain, including guards, bounds,
suspension and ownership transfers. Original token spans are reparsed and
compared structurally with the AST actually emitted. Changed or appended AST
statements and altered conditional-compilation structure refuse the proof.
Eight host controls pass. Regeneration changes exactly two final stores;
the other 285 handlers and all generated metadata are byte-identical.

Release and debug each build 117 modules and pass 582 focused tests across
12 modules. Native tests assert actual local, temporary and discarded result
routes, logical values and ownership, including GC/reentry. Independent source
and post-integration review accepted the packet. Compiled list/yield handlers
shrink from 1225/654 bytes to 985/453; instrumented yield remains 851 bytes.
These are static facts, not component speedup measurements. Full qualification
is still open. See [source/post-review](canonical35-source-checkpoint.md).

## Current elapsed evidence

Unchanged workloads, GCC15.2, JIT disabled, CPU15, hash seed0, seven timed calls
per fresh process. The two balanced-pair 29-kernel screen has geometric mean
**0.954489/golden** and **1.163732/upstream**. Every identity, module and process
gate passes. Ratios below are elapsed current/control; lower is better.

| Workload | / Golden | / Upstream |
| --- | ---: | ---: |
| richards | 0.924290 | 1.148112 |
| go | 0.937129 | 1.234128 |
| nbody | 1.002886 | 0.866260 |
| float | 1.011232 | 0.846691 |
| spectral_norm | 0.851240 | 0.727549 |
| raytrace | 0.938386 | 0.979124 |
| deltablue | 0.940611 | 1.627113 |
| chaos | 0.996261 | 0.968461 |
| nqueens | 0.991733 | 1.206901 |
| fannkuch | 0.986760 | 1.228988 |
| generators | 1.025333 | 1.457779 |
| comprehensions | 0.954582 | 1.352770 |
| coroutines | 0.988132 | 1.312460 |
| unpack_sequence | 0.820556 | 1.141341 |
| deepcopy | 0.969532 | 1.357053 |
| deepcopy_reduce | 0.974846 | 1.346982 |
| deepcopy_memo | 1.003086 | 1.329333 |
| json_dumps | 0.986525 | 1.347642 |
| json_loads | 0.993880 | 1.157808 |
| pickle | 0.999719 | 0.900400 |
| unpickle | 0.981527 | 1.214129 |
| regex_compile | 0.977880 | 1.178473 |
| logging_format | 0.977596 | 1.267453 |
| logging_silent | 0.650581 | 0.899568 |
| logging_simple | 0.961720 | 1.222868 |
| xml_etree_generate | 0.989912 | 1.237552 |
| xml_etree_iterparse | 0.973230 | 1.223575 |
| xml_etree_parse | 0.983912 | 1.161243 |
| xml_etree_process | 0.980844 | 1.327066 |

Logging is **0.899568/upstream** in this screen. This preserves parity in a
basic check; the two independent six-round timing gate belongs to source32 and
must be repeated at the final source. Do not assign cross-campaign changes to
this canonical-transfer packet: no direct isolated before/after attribution
was made.

A four-pair repeat of the three prior losses gives generators **1.025477**,
coroutines **0.979644** and nbody **0.986605** against golden. Every coroutine
and nbody pair is below1.0. Generators remain slower in all four pairs and are
the outstanding confirmed regression among those three. The original large
unpack, float, pickle and deepcopy_memo losses are repaired in the screen;
float still has a smaller 1.1% loss in both pairs and deepcopy_memo is 0.3%
above golden overall. These smaller differences have not been repeated. The broad
upstream gap remains uneven: generator/coroutine/comprehension and object-heavy
workloads still trail upstream despite numeric and logging wins.

Evidence: `E11/canonical35-broad-{golden,upstream}/` and
`E11/canonical35-repeat-golden/`. The independent
[raw-evidence audit](canonical35-performance-audit.md) reproduces all per-pair
ratios from the process rows, verifies alternating order and complete identity
stability. No new35 hardware profile was captured;34 counters are historical
and must not be relabelled as current.

## Next integration

The reviewed five-file NULL-only PRE packet has been applied to the primary
runtime after completing these timing windows. Native release/debug build and
focused qualification are in progress. Its
[source checkpoint](null-pre-source-checkpoint.md) describes the shared
physical query, must-NULL joins and independent VALUE/NULL_ONLY demands. No
native pass or debugger repair is claimed yet. Root owns all native jobs,
commits and composition checks; the isolated source has no separate build.

Next gates are the original three peepholer witnesses plus new negative/reentry
controls, then the frame/observer set and remaining debugger census. Ordinary
same-protocol jumps remain functionality to implement. Unsupported protocol
crossings need explicit reviewed safety tests; they are not blanket skips.
Current full release/debug qualification, stats rebuild, the unresolved GC
negative control, the generator regression and final cold review remain open.
Golden remains immutable and selected. The overall goal remains active.
