# M0 directive correction source review

ACCEPT exact clean `f0a49b6d30a38db705329bcfd5e5f2d6d2413710` (parent `49cc47360ccd79c7a89acc8bfa3d546f94afc46c`) for a separately reviewed fresh debug batch. The three-file delta adds a physical line boundary before the specializer-end directive, regenerates helper whitespace, and adds the directive-line regression test. Non-whitespace helper text and all VM/control/ownership source outside this correction remain unchanged.

Independent replay passes all 31 host generator methods and reproduces the helper header. The actual new regression method rejects the old header with 27 failed subtests. Retained authorized `gcc -fsyntax-only` checks of ceval.c and jit_runtime.c both returned zero in 0.637 seconds under the failed debug configuration; the existing signedness warning remains visible. No object, link or runtime credit follows from syntax-only success.

The failed 49cc batch, source and all 21 historical source-capsule artifacts remain preserved. The author disclosed and restored the historical host receipt after a reused producer wrote there; its exact original hash is independently verified. Eight runtime methods remain pending. No M1/native JIT, elapsed or qualification acceptance is implied.
