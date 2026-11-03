# Project hierarchy and coordinator handoff

Parent: [documentation ownership](../../DOCUMENTATION.md). Date: November 3, 2026.
Owner: coordinator; independent review: Sol 6.1.

Problem: the former entrypoint mixed experiment chronology with project status,
and current packet designs were ahead of several detailed status pages. A new
coordinator could mistake prepared artifacts or completed workers for active work.

Selected design: one `pyjit/` root; project overview/status → stage outcomes →
workstreams → exact experiment evidence. STATUS owns aggregate state, HANDOFF
owns restart identities and pending gates, and the stage/workstream pages own
local responsibilities. Keep implementation in CPython's source directories.
Historical branch, ABI and build identities and frozen receipts retain their
original names. There is no compatibility alias for the old documentation root.

Implementation moves 1,649 tracked files, updates active documentation/tool
references, adds five stage pages and four runtime workstreams, and records the
actual checkpoint56/v22 handoff. It preserves existing user changes unstaged.
Runtime source and the held projection remain unchanged.

Evidence is retained under `build/pyjit-docs-handoff-20261103-v1`: original
file/index inventory, rename receipt, reference-edit lists, validation and link
reports, independent review, tool test logs and guarded commit receipt.
Validation checks original modes and frozen bytes, protected user file/index
identities, runtime HEAD/index/held hashes, local links and script syntax.
Verification, benchmark-parser and gate-runner host controls exercise the moved
tools. No runtime build or performance result is claimed by this increment.

Independent review rejected the initial generic path rewrite inside dated
experiment receipts. Twenty-three issue documents and one mixed historical
design were restored exactly. The initial validation receipt is retained as
`validation.initial.json`; its directory-only frozen-file classification was
too broad a claim. The final validator also protects source-bound experiment
documents outside named archive/review directories. Review prompts that still
required tracing GC were corrected to the selected RC design.

The wide link audit retains eleven pre-existing missing targets in older
experiment records, including references to removed source and detached review
attachments. New navigation has no missing local targets. Those original
records remain historical evidence; they are not the project status authority.

Decision: adopt the hierarchy after independent review and passing focused
checks. The next coordinator starts from [HANDOFF](../../HANDOFF.md) and
reconciles its snapshot before assigning runtime work.
