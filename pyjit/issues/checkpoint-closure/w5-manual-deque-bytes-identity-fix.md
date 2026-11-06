# Additive value-identity cleanup

Apply after the frozen deque iterator patch, the original deque/bytes follow-up, and its separate review-fixes patch. Replace raw word comparisons with `PyValue_Is` for descriptor identity and the `_count_elements` default-zero test. This expresses Python value identity while masking the pointer ownership tag; inline zero is already represented by the canonical inline word.

`_collectionsmodule.c` compiled cleanly in the S build. Root review confirmed `_PyType_LookupValue` reports only status 0/1, so no failure status is omitted from the fast-path test.
