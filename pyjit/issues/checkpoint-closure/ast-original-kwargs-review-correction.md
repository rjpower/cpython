# Original-keyword review correction

The v2 review receipt, `ast-original-kwargs-peer-review-final.md`, describes
the earlier guard and is not the final v3 rule. The v3 guard checks the actual
per-class object and value vectorcall fields at the metatype offsets. It admits
the original tuple/dictionary type-call path only when both fields are
inactive, while the selected type-call pair matches. Ordinary Python classes
and `object` have both fields null. Active built-ins, including `type`, retain
normal vectorcall validation. The exact v3 rule and source review are recorded
in [the v3 receipt](ast-original-kwargs-peer-review-v3.md).

Merged v3 validation passed the release and debug targeted suites, the 412-case
integer matrix, 17 kwargs/descriptor/recursion cases, and two direct-C
`object()` cases against upstream. The broader full-suite, strict R27, and
performance gates remain separate and outstanding.
