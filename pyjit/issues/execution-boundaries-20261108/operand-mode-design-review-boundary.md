# Independent operand-mode design review

Reviewed 2026-11-08 from the proposed design and frozen runtime `0765e9be47f`.
Design is suitable for implementation with the following explicit conditions.
No runtime validation is claimed by this source review.

The chosen local receiver / temporary destination shape has a sound ownership
budget. The receiver's frame edge survives; the result duplicates the canonical
heap home and publishes to a distinct temporary. The type/shape guards precede
consumption and result creation. Consequently the variant can remove receiver
consumption and displaced-local retirement without moving an observable
release. It must retain the canonical duplication and optional NULL output.

1. Normalize physical IDs at every feedback storage/interpretation boundary.
   In particular `fb_begin_ex` saves the old physical IR_HANDLER, while
   `fb_saw_other_shape` compares rows to LOAD_ATTR_SHAPE. Leaving 319 in those
   rows can change the existing multiple-shape policy. Normalize both old
   observations and new success rows, or make every row consumer understand
   the canonical mapping. Do not merely fix stats array indexing.
2. The descriptor must inherit the specialized family and original format in
   all consumers, including generator guard-deopt and conservative contracts.
   Verify IR-only flags and HandlerBase rather than relying on shadow alone.
   Ensure get_ir exposes the variant and ordinary disassembly decodes its
   original four fields without indexing a 256-entry table with 319.
3. Test aliasing where the receiver's attribute points back to the receiver,
   alongside a returned last-reference mortal object and shape mutation. The
   local and temporary index domains cannot alias, but their objects can.
   Verify exceptional generic fallback and respecialization keep old shape
   history. Instrumentation enable/disable must restore generic family rules.
4. Treat the claimed two slot-class decisions as a source budget until compiled
   code confirms what GCC already combines or removes. The selection branch
   belongs in specialization, never the generated hot handler. One inherited
   variant is proportionate; do not broaden it across the family without new
   evidence.

The design already includes these areas in its gate; conditions above make
specific source consumers and assertions explicit. Native release/debug compile,
focused runtime results, generated diff review and coordinator integration remain
required after implementation.
