# math.dist value conversion receipt

Patch `r27-math-dist-values.diff` is based on runtime `b9888ea3b40`. Its two tuple-item owners now reach the existing value-to-double conversion without object projection. A W5 release shadow build passed `test.test_math.MathTests.testDist` and `r27-math-dist-probe.py`, which covers p-before-q callback ordering, conversion exceptions, inline and large integers, and dimension errors. The W5 source and release build were restored. This is focused worker evidence; merged qualification and source post-review are separate.
