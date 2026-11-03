# Claude Fable 5.1 peer-review request

Requested by the user on 2026-11-01 after observing that the ranked hypotheses
were predominantly GC work despite the document's substantial IR/interpreter
assessment. New implementation and native work remain held for reconciliation.

## Corrected live invocation

Completed: direct tool session 78126 returned zero at about 02:00 UTC,
with 20,049 response bytes and empty stderr. The response SHA256 is
`d9c2a4d3fa93e829014c5ee9ed11715fac3b527187f98b886b5b9e1b5a1c4eec`;
its verbatim repository copy is `claude-fable-5.1-review-20261101.md`.
The direct local transcript's assistant events independently identify
`claude-fable-5-1`; its actual medium-effort CLI argument is in the tool record.
The following launch description is preserved as history, not a live handle.

The user corrected the initial permission mode and requested medium effort,
direct CLI execution and local file pointers. The original plan-mode/high-effort
attempt was explicitly terminated; its tool session 14436 returned terminal
143 and its raw output is preserved, uncredited as a completed peer review.

The replacement runs directly in the repository with
`claude --model claude-fable-5-1 --effort medium --print`, using a read-only
review prompt with pointers to README, the integrated design, board, research
reports and new IR-region/stable-storage designs. It uses no Python launcher
and no plan-mode argument. Its current tool session is 78126. Output is
`build/claude-fable-peer-review-20261101/review-medium-v1.md`, with a separate
`review-medium-v1.stderr.log`. Poll this live invocation rather than relaunching
because output has not yet appeared. The initial packet below records the
superseded attempt, not the replacement's submission method.

## Preserved initial attempt

The installed Claude Code 2.1.316 CLI explicitly contains the full model ID
`claude-fable-5-1`. One external, source-only request was launched at about
01:49 UTC with that exact model, high effort, no tools, empty MCP configuration,
safe mode, no session persistence and no automatic fallback. The actual stream's
initialization event confirms `model: claude-fable-5-1`. This is a model peer
review, not source execution or an independent benchmark.

## Frozen input

- Repository head: `ca24abdb3490f23200ecbd38a73b1a0961a0df47`
- Main design SHA256: `576b27633ebb9e1918604f3e7af2b0ac8784fc7d97f675362b0208d8afe61b97`
- Request SHA256: `54d87644edeeecbe1324fcb7ada8f4fc528dcd4cf7be66b40119865f6ee4bbb5`
- Request bytes: 125,766
- Evidence directory: `build/claude-fable-peer-review-20261101`

`inputs.json` binds the complete design, README, board, issue state, prior
independent design review, three research reports, measured slot-base rejection,
incomplete cadence result and ten implementation excerpts. `request.txt`
contains the exact submission. `launch.json` binds arguments, process identity,
request and the fifteen-minute ceiling. `response.jsonl` and `stderr.log`
retain raw model output; `result.json` will record terminal status and hashes.
The existing live tool handle is session 14436, Claude PID 1283036; it must be
polled rather than restarted because an observation wait expires.

## Requested advice

Critique the integrated target and its evidence, especially the conflation of
correctness contracts and performance assumptions. Propose a better mechanism-
based breakdown across compiler/IR, VM/backend, values/native boundaries and
allocator/collector integration. Rank falsifiable hypotheses with useful
artifacts, causal controls, semantic obligations, whole-workload decisions,
dependencies, parallel assignments and explicit stop/defer choices.

The prompt explicitly rejects arbitrary equal allocation of effort, automatic
SSA/assembly endorsements, transferred external speedups and invented hotness
or death-eligibility histograms. The review must supply its own first steps and
a dependency diagram, not merely approve the prior team's recommendation.
The old adoption deadline is preserved without making its integration owed.

Preserve the returned review verbatim. Record coordinator dispositions and a
revised work plan separately; do not silently rewrite the peer's conclusions.
