# Decimal and PyValue

`_decimal` uses the unified value representation at polymorphic Python entry points. Decimal instances and Context instances remain heap objects; their concrete pointers may be borrowed while their owning value or receiver stays live. Python integers and floats must stay as values until a real object API boundary. Return an owned `PyValue`, use `PyValue_NULL` only for an error, and keep every owned operand alive across callbacks. Object slot and C entry points use paired thunks; private core calls use the value entry directly. This is a design target, not evidence that every `_decimal` entry has been ported.

## Conversion and operators

Convert Decimal, integer and float operands in the order required by the existing operation. Decimal operands receive an owned reference; integer conversion uses the exact-base value path and preserves bool and integer-subclass semantics. Float conversion preserves the active Context, FloatOperation flags and traps, signed zero, infinities and NaN behavior. Unsupported operands return `NotImplemented` where the number protocol requires it, while Context methods retain their documented TypeError paths. Numeric slots must pair the value slot with the object-form thunk and release converted operands on all failures.

Rich comparisons need special care for float and complex inputs, Rational numerator and denominator callbacks, exceptions, NaNs and reflected operations. Attribute values are owned across calls that can execute Python. The [comparison proposal](../issues/checkpoint-closure/decimal-richcompare-proposal.md) records the reviewed contract; a worker-tested implementation is still queued for integration and post-review. Its Rational callback order follows the pre-port W5 C source; verify the C extension is loaded when comparing against upstream.

Exact integer work should use value-aware helpers, including the [long GCD design](../issues/checkpoint-closure/long-value-gcd-proposal.md). Its word and mixed-size paths avoid temporary boxed integers, and the both-large path retains the existing Lehmer algorithm. The object GCD entry remains a compatibility boundary. Provider and caller marks must be enabled together after all core callers migrate.

## Clinic and construction

A selective Clinic value method keeps concrete receivers in pointer space, lends polymorphic arguments as values, and returns an owned value. The `@value_function` producer is restricted to core-only output with at most one generated file destination. A selective `@value_constructor` pairs the object `tp_new` entry with a value vectorcall parser; registration applies to the exact immutable base so mutable subclass overrides continue through ordinary type dispatch. Omitted constructor arguments, explicit None, keywords, duplicate arguments and failure cleanup remain distinct. See the [selective method](../issues/checkpoint-closure/clinic-per-method-value-proposal.md) and [constructor](../issues/checkpoint-closure/decimal-selective-constructor-proposal.md) proposals and their post-reviews.

A selective initializer is a separate facility: its object `tp_init` wrapper and value entry share parsing and an integer error result. Its exact-base value call must retain tp_new/tp_init order, discard a newly created instance on init failure, assert the expected allocator and initializer, and never bypass mutable subclass overrides. This producer and Context adoption are in progress, not a qualified runtime capability.

## Context

Context numeric and rounding descriptors need value getters and setters, with owned getter results and borrowed setter inputs. Preserve deletion errors, integer range and subclass behavior, rounding diagnostics and flag/trap dictionary lifetimes. Custom attribute dispatch must have corresponding value slots; descriptors alone do not make ordinary Context attribute access value-native.

The current-context lookup returns a borrowed pointer. Any operation that keeps
that Context across warning handlers, operand conversion, formatting or other
Python callbacks must take a reference before the first callback. A callback
may replace the context and trigger collection. `localcontext` similarly owns
its saved outer context before constructing or modifying the inner copy, then
transfers that reference into the manager or releases it on failure.

`Context.__init__` treats None as an omitted field, while `__replace__` and `localcontext` preserve the difference between omitted and explicitly supplied None. Apply changes in the existing observable order: precision, rounding, Emin, Emax, capitals, clamp, traps, then flags. Hold list, dict and SignalDict inputs through reentrant callbacks and stop at the first error. Own the current Context across any warning, operand conversion or other callback that can replace the thread current Context; a borrowed cache pointer alone cannot survive that replacement. The [Context proposal](../issues/checkpoint-closure/decimal-context-init-proposal.md) contains the initializer and ownership details.

## Qualification and remaining gates

Worker-built patches, focused tests and review receipts are recorded under [checkpoint closure](../issues/checkpoint-closure/). They are queued until the coordinator integrates them and performs merged build, behavioral and boxing validation. Shared runtime `6aec17eefeb` includes the selective Decimal constructor, numeric slots, conversions and major method families. The Context/initializer, rich comparison, formatting and lifetime follow-ups remain queued; check the current integration record rather than inferring landing from a W5 diff. Remaining work includes the complete Context value path, remaining polymorphic methods and object boundaries, the `_decimal` core-mark/compiler census, and fail-closed R27 static, startup, kernel and suite gates. Performance comparisons require the coordinator's qualified merged build and upstream measurements.
