# Independent peer of the remaining debugger scope audit

The audit is useful triage, not admission to port thirteen tests to refusal.
The documented frame/Pdb APIs allow bounded jumps and do not prescribe the
entire CPython matrix. They also do not justify arbitrary per-method bans.
Use a general proof-based rule: retain compatible protocol, handled-context and
binding owners; an actual transition needs an explicit proved action.

The eleven ordinary positives in the draft retain their behavior assertions.
Its requested actual-PC/phase probes correctly avoid treating an executing
coroutine as suspended or a source line as evidence of manager entry. Refusal
controls must preserve target/protocol owners and normal subsequent execution;
they need not promise that unrelated allocation-triggered finalizers never run.

Two except-arm entries cannot yet be classified as intentional refusal. Arms
may share the same PUSH_EXC_INFO generation and saved-previous owners. If their
context and required owners/bindings are compatible, the jump may be an ordinary
implementation gap. A named binding absent at the target is a separate concrete
requirement, not evidence that every arm crossing creates a context. Probe both
actual source/target states before deciding. Root explicitly rejects inventing
a lexical-arm policy merely to fit the proposed thirteen-case count.

With-manager substitution, actual caught exits, pending-finally abandonment and
yield cancellation describe distinct capabilities/actions; conservative refusal
may be an intentional support boundary after evidence and explicit admission.
Normal protocol behavior must remain correct. No runtime/test changes, skips,
fixture ports or native executions were made by A for this peer.
