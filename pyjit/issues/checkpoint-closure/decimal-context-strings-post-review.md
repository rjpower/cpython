# Decimal Context string methods: source post-review

Reviewed frozen [`decimal-context-strings.diff`](decimal-context-strings.diff). **Approved; no must-fix finding.** The three decorated methods have matching generated `METH_VALUE` entries and borrow the polymorphic argument. Each method closes its converted Decimal after the mpdecimal operation. The two formatting methods preserve the `mpd_to_*_size` → `unicode_fromascii` → `mpd_free` order, including free on Unicode allocation failure. All three check fallible Unicode construction before transferring the result to an owned value.

Terra reports extension compile/link, 69 focused Context tests and `_pydecimal` comparisons for capitals 0/1, small/large integers, bool and int subclasses, and invalid float/string inputs. Merged integration remains the coordinator's gate.
