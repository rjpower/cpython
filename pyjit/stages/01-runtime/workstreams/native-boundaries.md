# Native parsers and extension boundaries

Parent: [runtime integration](../README.md). Owner role: design and review agents,
worker agents for mechanical packets, coordinator coherent composition and gates.

> **Superseded 2026-11-03.** [native-boundary.md](../../../design/native-boundary.md)
> replaces the explicit owner-scope approach below with an implicit per-thread
> borrow stack. The four frozen FORMAT templates (B1, B2, C, public wrappers)
> are withdrawn. Work now follows packets P1 to P4 in that design. The text
> below is history until this page is rewritten.

The argument-scope export provider is committed as checkpoint56. It retains
actual buffer exports through End. Its expanded C fixture compiles; fourteen
candidate tests remain unexecuted. The parser file still has twelve errors.

Full positional (B1), converter/resource (B2), keyword (C), and public-wrapper
templates are frozen. They have not passed independent coherent review or been
implemented. Public scopes, sixteen raw wrapper migrations, owned unpacking,
caller/Clinic producer closure and the replacement of the Stage1-only whitelist
witness must compose. Do not land changed backend signatures while callers
still depend on the old ownership contract.

Six numeric caller exemplars exist. Broader caller and Clinic producer rules
are unfinished. The lexical census counts references, not manual ports; use
producers and repeated caller patterns for bulk updates, with explicit designs
for escaping outputs and callback-heavy exceptions.

Hard upstream compatibility is not a gate. The opaque API should make rebuilt
source ports practical and incremental; it must not preserve unsafe borrowed
results or add a permanent parser fallback. See [extension stage](../../05-extensions/README.md).

Exact packet seals and outstanding seams are in [HANDOFF](../../../HANDOFF.md).
The [native parser record](../../../issues/pyvalue-rc-restoration/native-parsers/status.md)
retains component gates and historical designs. Exit requires coherent backend,
public APIs and selected callers, then real parser/resource tests on the linked runtime.
