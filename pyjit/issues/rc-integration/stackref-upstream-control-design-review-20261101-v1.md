# Independent review: one upstream stackref control

2026-11-01 08:53 UTC. **ACCEPT design only**, binding the source audit SHA `16f4702de1a44ce6f4baa0602f459d3c0bf2a0013c55359a037e1fd6cfa9fe73` and its receipt `8ccd1b8c5c1e0ac30afbfbe4f37f373ce453d11a44b595ea8c9103d1a7b43d33`.

I independently rehashed all 29 source references, compared the three actual method bodies by AST, and compared the whole embedding C entry file. The method bodies agree between RC dce654018d5 and upstream 321925c92c9; the embedding decorator difference remains explicit. Source inspection supports distinct shutdown, asynchronous reader and remote representation hypotheses. It establishes no upstream native result and no general exemption for the 12 failing RC modules.

One fresh GCC O1/Py_DEBUG/Py_STACKREF_DEBUG upstream image and the three fixed standard methods are a useful finite causal comparison. The 1,200-second build plus 180-second test allowance and two 210-second reserves fit the proposed 1,800-second inclusive window. Retain all three standard outcomes without failfast, including the threading method's two existing subtests. Actual raw signatures, source/tool/profile origins, compiled checker witnesses and dual-executable cleanup remain required. A different runtime necessarily has different IR, frame and value implementation; matching the three test bodies does not erase those differences.

Identical upstream signatures would support a narrowly reviewed checker applicability amendment. Different signatures or RC-only failures remain unresolved. The failed RC full run stays failed partial evidence. This review grants no native activation, runtime fix, exclusion or qualification credit. Exact concrete helpers still require review.

Host evidence: `build/rc-stackref-partial-outcome-review-20261101-v1/design-observations.json`. No target or compiler ran.
