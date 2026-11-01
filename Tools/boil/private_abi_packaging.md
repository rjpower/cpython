# Packaging tools for the private RC ABI

The RC runtime keeps CPython's interpreter identity and uses private
`boilrc1-` extension suffixes. Its compiled extensions are not upstream CPython
or stable-ABI binaries. The bundled downstream pip and setuptools wheels teach
their actual tag producers and pip installer route about that boundary.

Supported ABI tags come only from the runtime's own loader suffixes, after
checking SOABI and EXT_SUFFIX consistency. Debug can accept its loader's
declared release alternative. Pure Python `none` tags remain supported.
Ordinary CPython/PyPy ordered tag behavior and the explicit public
`cpython_tags()` API are unchanged. On this private runtime, pip's explicit
version/platform/implementation/ABI target overrides are unsupported and raise
an error. Other SDK/build-tool versions need separately reviewed profiles;
these two wheels do not qualify historical port binaries or upstream abi3.

## Reproduce the downstream wheels

`patch_private_abi_wheels.py` accepts only exact original whole-wheel and vendor
source hashes. It writes new archives, never installs packages, fetches files,
changes an input wheel or invokes the target runtime. Preserve upstream bytes
outside the tracked tree; the last unmodified selected source supplies them:

```sh
mkdir -p build/private-abi-originals
git show dce654018d5ae16ffe5e772976d9ccc2c9d1e01a:Lib/ensurepip/_bundled/pip-26.2.2-py3-none-any.whl > build/private-abi-originals/pip-26.2.2-py3-none-any.whl
git show dce654018d5ae16ffe5e772976d9ccc2c9d1e01a:Lib/test/wheeldata/setuptools-79.0.1-py3-none-any.whl > build/private-abi-originals/setuptools-79.0.1-py3-none-any.whl
```

The original SHA256 values are
`fd7f4dfa8190e014013b9f08f3e973f4e4c26ffd179a204895533a7254813a70`
and `e147c0549f27767ba362f9da434eab9c5dc0045d5304feb602a0af001089fc51`.
Do not commit duplicate original archives or extracted vendor trees.

For each original wheel, use a new output and manifest path:

```sh
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project --offline --python /usr/bin/python3.14 python -B Tools/boil/patch_private_abi_wheels.py \
  build/private-abi-originals/pip-26.2.2-py3-none-any.whl \
  build/pip-26.2.2-py3-none-any.whl --manifest build/pip-private-abi.json
```

Apply the same command to the setuptools original. Unknown input/layouts,
signatures, inconsistent metadata, malformed outer RECORDs and occupied or
aliased outputs fail closed. Fixed ZIP order/timestamps/compression produce
byte-identical repeated archives. The writer updates the two affected nested
setuptools vendor RECORDs before the outer wheel RECORD. Unrelated bytes,
licenses and vendor RECORD rows remain intact. An internal marker binds policy,
original archive and changed-member hashes; the external manifest binds the
final wheel, avoiding self-referential hashes.

Accepted downstream archive SHA256 values are
`a0f564ec764b528ee5473887dec474c27ad8fa8fe9430fc5843c96e99e5edbdd`
(pip) and `93db1553711fa6ae2723e19c467906b306a5e1d1e20287f8f934da5f19fb110a`
(setuptools). Their retained upstream filenames and versions do not imply
upstream content identity. Build recipes and SDK caches must bind these content
hashes and their actual RC ABI identity.

## Host checks and native boundary

The standalone host tests take explicit original wheel paths and a new
task-owned workspace. Patched wheels are read from the repository's ordinary
bundled pip and test wheeldata locations, including when this layout is staged:

```sh
UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project --offline --python /usr/bin/python3.14 python -B Tools/boil/test_patch_private_abi_wheels.py \
  --original-pip build/private-abi-originals/pip-26.2.2-py3-none-any.whl \
  --original-setuptools build/private-abi-originals/setuptools-79.0.1-py3-none-any.whl \
  --work-dir build/private-abi-host-tests
```

The 33 tests retain all 28 vendor/actual-installer policy rows and five archive
controls. They use pinned source functions under fixed host facts and do not
install or execute the selected runtime. A host pass does not qualify native
wheel construction, installation or extension ownership. Actual ensurepip/venv,
the unchanged CPU test_cext suite, regeneration and a separately bounded
foreign-wheel rejection gate remain required before claiming that coverage.
No loader, runtime identity, C header or original extension test is changed by
this tooling policy.
