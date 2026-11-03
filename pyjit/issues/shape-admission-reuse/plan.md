# Bounded shape-admission decision

Decision budget: 45 minutes. GC owns collector files and native admission.
The bounded source, build, behavior and emission work below is complete at
`c46f1bef67f`; see [results](results.md). The recommendation is integration
review of the simpler family, with no elapsed-speed claim or extra timing
campaign. Applicable landing checks remain the coordinator's next decision.

1. Use frozen profiles, captured PCs and qualified O3 debug information to
   distinguish required admission from the redundant payload lookup. Retain a
   small offline replay and hashes; do not gather a hit-distribution profile.
2. Submit the unapplied DSL diff and task-owned generated results for source
   review. Do not mutate tracked runtime before that review. The proposal owns
   `Python/bytecodes.c`, its necessary generated files and a focused existing
   generator/control test only if a concrete coverage gap appears.
3. Confirm all handler numbers/formats/effects, compiler/specializer output,
   cache layout and H3 counting/observer semantics. Current task-owned generation
   changes only the two C handler bodies and removes an unused private uop ID;
   if serialized handler/compiler/layout contracts change, bump pyc magic and
   regenerate as policy requires. Do not add a metadata exception.
4. After source clearance, apply and regenerate through the existing tools.
   Run relevant host generator/root checks and meaningful shape controls:
   two/four rows, missing/fifth-row deoptimization, inline receiver miss,
   row-address reuse, type mutation, method/data layout, transition and kind
   generalization. Reuse existing cases before adding a test. Check that a
   relevant omission would be detected; no unsafe broad negative campaign.
5. Reuse one accepted ordinary build for necessary native behavior and static
   emitted-code inspection, coordinated with the GC build. No quiet timing
   while GC owns the native window. Do not repeat 80 values, all 29 benchmarks, several
   builds or a broad ladder merely to resolve a subpercent timing claim.

Integrate only a simpler, behavior-preserving shared-family implementation
with demonstrated removal of repeated emitted comparisons and the applicable
finite validation. If source/generator consequences grow, defer it and record
the exact blocker. The alternative structural question is activation ownership:
how much transient frame initialization/allocation remains before a generator
acquires its final embedded frame? That is a separate proposal, not permission
to revive the rejected memcpy patch or redesign calls from residual CPU alone.
