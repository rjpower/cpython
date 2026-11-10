# B proposal review: canonical direct class values

**Approved for the proposed bounded B implementation. No correctness blocker found.** This is a source/proposal review, not approval of an implementation diff or native validation. A remains a separate later increment. Read the [final Astra architecture review](astra-architecture-review.md) and [implementation proposal](lookup-implementation-proposal.md); inspected actual frozen runtime46 `b8e7e1bee6611eeda320ce163cd3ce89323da877`.

The scope is to generalize existing class-load handlers 189/190 to canonical guarded object/scalar payloads, retaining class/metaclass/descriptor admission. Keep ATTR48/POLY120, IDs, layout and strict API checker unchanged. No frame/call changes or new descriptor kinds. The producer/reader/snapshot changes must land together.

## Writer, guard and lifetime audit

`Python/specialize.c:specialize_class_load_attr` already performs the shared metaclass getattro/provider check, metaclass descriptor classification, class/MRO lookup, nonzero class-version admission and mutable-metaclass version selection. Adding `INLINE_VALUE` beside admitted METHOD/NON_DESCRIPTOR cases is sufficient; do not create a second lookup path or weaken the existing MUTABLE enum-like conditions. The guarded assignment must use the canonical `descriptor`, not nullable `descr`.

For admitted objects, preserve the existing free-threaded deferred-refcount helper; call it only for `PyValue_IsObject(descriptor)`. Inline values must never pass a NULL object into it. This representation change is not authority to widen object admission or reinterpret a mutable descriptor as a non-descriptor.

Handler189 checks receiver is a class and its current version equals `class_version`. Handler190 first validates receiver-type/metaclass version, then class version. The existing producer rejects metaclass data-descriptor/overridden-provider cases. Mutable metaclass mutation is therefore guarded independently; the class version alone is not the proof. The immutable-metaclass route retains its existing coverage. Test both routes rather than restricting all formerly admitted objects to builtin metaclasses.

The class or an MRO base dictionary owns the cached object value. `_PyValueGuarded` stores nonowning metadata and creates a transient borrow only after guards. Type mutation invalidates affected type versions before displaced dictionary values retire: `Objects/typeobject.c:type_setattro_value` calls `PyType_Modified` before `type_update_dict`, and closes `old_value` afterward. The acquired result must independently survive receiver/class destruction or reentrant mutation. The proposed canonical acquisition before `IR_RETIRE_PUBLISHED_RESULT` does this. No ownership is transferred from the cache.

Ordinary class method-form loads still return the admitted raw function/value plus a NULL self suffix. They must not bind a plain function to the accessed class. Non-method attribute reads return just the value. New inline admission must preserve data-descriptor precedence, inherited class mutation, deletion, custom metaclass behavior and normal error fallback.

## Reset, history and initialization

| Consumer/producer | Required B action | Source finding |
| --- | --- | --- |
| `specialize_class_load_attr` | Write `class_value`, versions, then existing handler | Sole direct-class payload producer located |
| `_LOAD_ATTR_CLASS` | Guarded read/acquire/publication/retirement; preserve NULL suffix | Both handlers share this uop |
| `fb_row_of(FB_ATTR)` | Move189/190 from `payload.descr` to `payload.class_value` | Current code still places them in native-pointer branch |
| `fb_put_row`/`fb_save_mono`/`fb_record` | Preserve scalar guard metadata and publish handler last | Whole independent payload copied; no borrowed-result conversion required |
| `fb_begin_ex(FB_ATTR)` | No storage change | Zeros every payload byte after the header of the48-byte entry before replacement |
| `make_exec` in `Python/ir.c` | No storage change | Zeroes fresh feedback and initializes only headers/static args |
| Inspection/history | Interpret the payload by active handler | `_testinternalcapi.get_feedback` exposes versions/handler rows, not a live cached value |

No historical poly execution/replay path was found that reconstructs a live value for these handlers. Historical words remain nonowning observations and must never be loaded through a stale class proof. Keep an explicit object→scalar→other-handler respecialization test at one site, including deletion/mutation and invalidated observations. A zero payload cannot become active: all producer assignments precede `specialize`.

## Selector and serialization decision

`pycore_opcode_metadata.h` currently maps legacy selector6 of189/190 to `offsetof(_PyFb_LOAD_ATTR, descr)`. Regeneration must change both to `class_value`; the selector number/width and canonical IR operands stay unchanged. The generator's `write_uop` already recognizes `class_value/4` as `_PyValueGuarded` and enforces attribute feedback. Keep independent guarded/native storage; no casts or pointer-view compatibility path.

**No magic bump is required for B as scoped.** `Python/marshal.c` serializes `_PyCode_GetCode`'s immutable original words, fbinit, constslot table and ordinary code data, not specialized execution handlers, warmed feedback payloads or generated C offsets. `_PyCode_GetCode` reads `co_words`, whereas `_co_code_adaptive` is a separate inspection interface. Fresh exec feedback is initialized by the current runtime. Generic IDs, immutable field roles, fbinit units and ATTR48/POLY120 geometry do not change. This conclusion does not authorize serializing specialized words or reusing a retired opcode in A.

## Independent host generation probe

Using host `uv run --no-project python -B`, replaced only the proposed `_LOAD_ATTR_CLASS` uop text in an in-memory copy of actual `Python/bytecodes.c`, then ran the existing analyzer and IR generator through `TestGuardedAttributeCodegen.generate(runtime=True)`. The temporary input was cleaned up; no runtime source or generated product was written.

Both `LOAD_ATTR_CLASS` and `LOAD_ATTR_CLASS_WITH_METACLASS_CHECK` passed the **existing strict** publication and effect-free suffix validation. Assertions verified typed guarded metadata, class guard before guarded load, load→canonical acquisition→canonical temporary publication→receiver retirement, reloadable `this_instr` and post-retirement refresh, retained `_PUSH_NULL_CONDITIONAL`, and eventual destination store. No generator change, handler-name exemption or suffix relaxation is needed for the proposed body.

The first host harness attempt prepended runtime Lib and collided with host asyncio; the successful probe used the host standard library with runtime Lib appended. This was a host import setup issue, not a generator rejection. No native interpreter, build or regeneration command ran.

## Implementation/post-review gates

- Add real-source host codegen checks for both handlers, both guard orders, typed selector destinations, result-phase metadata, reloadable instruction and the real NULL suffix. Test that an effectful/incorrect suffix is still rejected where the existing contract tests cover that rule.
- Focused native tests must cover inline int/float/bool/None and mortal object identity; inherited class/MRO replacement/deletion; metaclass data-descriptor insertion and provider override; mutable-metaclass guard misses; scalar→descriptor/object transitions; ordinary function method-form NULL self; and result survival through last receiver retirement/reentry. Tests should preserve Python behavior and coherent ownership, not incidental destructor transcripts.
- Coordinator must regenerate all affected products together, inspect source-to-generated selector/result contracts, compile release/debug and run focused ownership/cache/call/API checks. Retain size assertions and serialized feedback compatibility checks. A source-only approved proposal does not establish those results.

Decision sent to coordinator before implementation authorization. Author may implement exactly B after the frozen measurement window ends. Ready for a separate actual-diff/generated/native-evidence post-review; no runtime edits, native jobs or commits were performed in this review.
