# Ports

Pinned PyPI packages, rebuilt from source against this interpreter and
smoke-tested after every interpreter change. Run from the repository root:

```sh
uv run --no-project --python python3 python -m ports build [ref ...] [--force]
uv run --no-project --python python3 python -m ports check [ref ...]
uv run --no-project --python python3 python -m ports bench [ref ...]
```

- `build` resolves `requires`, fetches and verifies sources into
  `build/ports/sources/`, unpacks each into a fresh tree, applies patches,
  builds a wheel with the target interpreter and writes a result record to
  `build/ports/results/<abi_id>/<name>-<version>.json`. A port whose record
  matches its recipe hash (recipe, `build.py`, SDK, driver code) is skipped.
- `check` installs the port and its requirements into a fresh venv with
  `pip --no-index --no-deps`, runs each test script with `-I`, and prints
  PASS or FAIL with the port's count of patch lines.
- `bench` runs each bench script 5 times and appends the median wall time to
  `build/ports/bench.tsv`.

With no refs, a verb covers every port under `python/`. `cpython:<variant>`
(`default`, `ft`; flags in `cpython/recipe.json`) builds this checkout
in `build/ports/cpython-<variant>/`. Logs go to stderr; stdout is one JSON
summary.

`sdk.json` names the interpreter (`--interpreter` overrides it), the `abi_id`
(`--abi-id`), the compiler and CFLAGS. Everything built is stored under the
abi_id, and nothing is installed under another. Bump the abi_id when the C
ABI changes; until then, wheels survive interpreter rebuilds.

## Recipes: `python/<name>/recipe.json`

| field | meaning |
|---|---|
| `name`, `version` | the directory name and the pinned release |
| `kind` | `pure-wheel` (the published py3-none-any wheel), `sdist`, or for `native/` recipes `configure-make` |
| `source` | `{url, sha256}`, from `https://pypi.org/pypi/<name>/<version>/json` |
| `requires` | exact port references, such as `python/pycparser` or `native/libyaml` |
| `build_requires` | `name==version` for every build requirement, transitively; each needs a universal wheel, which the host fetches, so the target interpreter needs no network or ssl. A tool that ships only binaries (ninja) may use the `py3-none-manylinux*` wheel for the host's machine; the host has no system ninja |
| `build_ports` | port references, such as `python/cython`, whose wheels the build venv installs with `build_requires`; each is built first, is not installed with the port, and rebuilding it rebuilds the port (sdist only) |
| `build_options` | extra arguments appended to `pip wheel`, such as `-Csetup-args=-Dfoo=true` (sdist only) |
| `patches` | `[{file, sha256, touches: {path: sha256}}]` |
| `tests`, `bench` | `[{script}]`, relative to the recipe directory |

A `build.py` defining `build(ctx: BuildContext)` (see `_support/build.py`)
replaces the default `pip wheel` step. Use one only when the build cannot be
declared.

## Native libraries: `native/<name>/recipe.json`

A `configure-make` recipe takes only `name`, `version`, `kind` and `source`.
`build` runs `configure --prefix=<prefix> --disable-shared --enable-static
CFLAGS="<sdk cflags> -fPIC"`, `make` and `make install`, into
`build/ports/native/<abi_id>/<name>/`, and records it under
`build/ports/results/<abi_id>/`. A port that lists it in `requires` builds
with `-I<prefix>/include` in CFLAGS and `-L<prefix>/lib` in LDFLAGS, so the
library links statically into its extensions. The dependent's recipe hash
includes the library's, so changing the library rebuilds the port. Native
libraries are built and linked, never installed into a venv.

## Patches

Write `-p1` unified diffs against the unpacked sdist. `touches` lists every
upstream file the patch modifies, with the sha256 of its unpatched contents.
Before `patch -p1 --fuzz=0` runs, the driver checks the patch's sha256, each
touched file's sha256, and that `touches` names exactly the patched files. A
mismatch is an error naming the file, so a pin bump fails loudly instead of
fuzzing; refresh the patch and its hashes. Explain each patch in one sentence
in the port's `README.md`.

## Porting to the tracing collector

Under `--with-gc` reference counts no longer free anything; a collection
traces from roots and sweeps what it did not reach. Look for these in a
port's C code:

- A `PyObject*` in `malloc`ed or `PyMem` memory, or in a `Py_buffer`'s
  `obj`, that no `tp_traverse` visits. Visit it, or register it with
  `PyGC_AddRoot()` while it is held.
- A traverse that leaves out members that cannot form cycles, such as bytes
  and str. The cycle collector allows that; the tracing collector frees
  them.
- A dealloc that can run code or allocate: calling Python, a destructor
  callback, `PyBuffer_Release()` on an arbitrary exporter. The sweep forbids
  both; move the call into `tp_finalize`, which runs before the sweep.
- A dealloc that reads a referent's C state rather than only releasing it,
  as an expat subparser reads its parent's parser. Reference counting
  frees the referrer first; the sweep has no order, so do that work in
  `tp_finalize`.
- Anything that reads `ob_refcnt` or `Py_REFCNT()` to decide behaviour.

## Adding a port

1. Pin the latest release's url and sha256 from PyPI's JSON API.
2. Write `tests/smoke.py`, 20 to 50 lines, that asserts the version and
   exercises real functionality, including any C extension.
3. Run `build` and `check` for the port. Patch only what the interpreter
   breaks.

`uvx pytest ports/_support/tests` tests the driver.
