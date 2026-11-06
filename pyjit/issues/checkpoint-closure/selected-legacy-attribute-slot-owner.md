# Legacy attribute-slot box attribution

The 7baf full stats census has one invalid owner in `test_xxlimited`: an inline value assigned through `xxlimited_35`'s legacy `tp_setattr` slot. `call_setattro` passed the absent `tp_setattro` slot as the adapter box owner, so stats wrote owner `-` even though the selected legacy C slot was known. The same selection issue existed for `tp_getattr` and for an inline receiver.

`selected-legacy-attribute-slot-owner.patch` supplies the selected non-NULL slot pointer at each adapter box call. It changes attribution only; the dispatch, box count, and error/ownership paths are unchanged. The patch applies cleanly to the current runtime and the patched C unit passed syntax-only compilation with release flags. The existing `test_xxlimited` attribute assignment is the behavioral check. At stats integration, verify the legacy slot-adapter row has a real `xxlimited_35` owner and invalid-owner count is zero; keep the strict accounting gate unchanged.
