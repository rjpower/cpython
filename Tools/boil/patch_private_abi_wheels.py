"""Reproduce the pinned downstream boilrc1 build/installer tag policy.

This source candidate writes new wheels only. It neither installs packages nor
changes runtime identity, native loaders or upstream stable-ABI declarations.
"""

from __future__ import annotations
import argparse
import ast
import base64
import csv
import hashlib
import io
import json
import zipfile
from pathlib import Path

POLICY = "boilrc1-private-tags-v1"
PROFILES = {
    "fd7f4dfa8190e014013b9f08f3e973f4e4c26ffd179a204895533a7254813a70": {
        "pip/_vendor/packaging/tags.py": "00d607671610569f4194e41838753902b0cd548a53165e0a95ba1c6150569c5b",
        "pip/_internal/utils/compatibility_tags.py": "0e23522eaa6eaee5d46a6190c0e2765990720c62d319a5e2f4ef977fc7ce8b7e",
    },
    "e147c0549f27767ba362f9da434eab9c5dc0045d5304feb602a0af001089fc51": {
        "setuptools/_vendor/packaging/tags.py": "085aab2730337365cd19ec5eac7fff4fe639230abb59bb185ec88b1112d6c58d",
        "setuptools/_vendor/wheel/vendored/packaging/tags.py": "7de7475e2387901c4d6535e8b57bfcb973e630553d69ef93281ba38181e281c0",
    },
}
PRIVATE_HELPER = """def _private_cpython_abis():
    # The loader's own suffixes are the authority; never infer upstream ABI.
    import importlib.machinery
    import sysconfig
    soabi = sysconfig.get_config_var("SOABI")
    if not isinstance(soabi, str) or not soabi.startswith("boilrc1-"):
        return None
    suffix = sysconfig.get_config_var("SHLIB_SUFFIX")
    extension = sysconfig.get_config_var("EXT_SUFFIX")
    if (interpreter_name() != "cp" or not isinstance(suffix, str)
            or not suffix or extension != "." + soabi + suffix):
        raise ValueError("inconsistent boilrc1 private ABI metadata")
    private = [item for item in importlib.machinery.EXTENSION_SUFFIXES
               if item.startswith(".boilrc1-") and item.endswith(suffix)]
    if not private or private[0] != extension:
        raise ValueError("boilrc1 primary extension suffix is not loader-authorized")
    return [_normalize_string(item[1:-len(suffix)]) for item in private]


"""


def sha(data):
    return hashlib.sha256(data).hexdigest()


def patch_tags(source):
    tree = ast.parse(source)
    function = next(
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef) and node.name == "sys_tags"
    )
    if "_private_cpython_abis" in source:
        raise ValueError("private tag adapter already present")
    lines = source.splitlines(keepends=True)
    lines.insert(function.lineno - 1, PRIVATE_HELPER)
    source = "".join(lines)
    old = '    interp_name = interpreter_name()\n    if interp_name == "cp":\n'
    new = '    private_abis = _private_cpython_abis()\n    interp_name = interpreter_name()\n    if private_abis is not None:\n        yield from generic_tags(interpreter="cp" + interpreter_version(warn=warn), abis=private_abis)\n    elif interp_name == "cp":\n'
    if source.count(old) != 1:
        raise ValueError("unsupported sys_tags source layout")
    result = source.replace(old, new)
    ast.parse(result)
    return result


def patch_installer(source):
    anchor = "from pip._vendor.packaging.tags import (\n"
    if source.count(anchor) != 1:
        raise ValueError("unsupported pip tags import layout")
    source = source.replace(
        anchor, anchor + "    _private_cpython_abis,\n    sys_tags,\n"
    )
    tree = ast.parse(source)
    function = next(
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef) and node.name == "get_supported"
    )
    if not (
        isinstance(function.body[0], ast.Expr)
        and isinstance(function.body[0].value, ast.Constant)
    ):
        raise ValueError("unsupported get_supported source layout")
    lines = source.splitlines(keepends=True)
    lines.insert(
        function.body[0].end_lineno,
        """    if _private_cpython_abis() is not None:
        if any(value is not None for value in (version, platforms, impl, abis)):
            raise ValueError("explicit cross-target options are unsupported on boilrc1")
        return list(sys_tags())
""",
    )
    result = "".join(lines)
    ast.parse(result)
    return result


def record_bytes(data):
    return (
        "sha256="
        + base64.urlsafe_b64encode(hashlib.sha256(data).digest())
        .rstrip(b"=")
        .decode()
    )


def update_vendor_records(data, changes):
    # Vendored distributions retain their own RECORDs, relative to _vendor.
    # Update their changed-source rows before the outer wheel hashes metadata.
    for record in sorted(data):
        if not record.endswith(".dist-info/RECORD") or record.count("/") == 1:
            continue
        base = record.rsplit("/", 2)[0] + "/"
        rows = list(csv.reader(io.StringIO(data[record].decode())))
        changed = False
        for row in rows:
            if len(row) != 3:
                raise ValueError("invalid vendor RECORD row: " + record)
            member = base + row[0]
            if member not in changes:
                continue
            before = changes[member]["before"]
            checksum = (
                "sha256="
                + base64.urlsafe_b64encode(bytes.fromhex(before))
                .rstrip(b"=")
                .decode()
            )
            if row[1] != checksum:
                raise ValueError("vendor RECORD mismatch: " + member)
            row[1:] = [record_bytes(data[member]), str(len(data[member]))]
            changed = True
        if changed:
            original = data[record]
            result = io.StringIO()
            csv.writer(result, lineterminator="\n").writerows(rows)
            data[record] = result.getvalue().encode()
            changes[record] = dict(
                before=sha(original),
                after=sha(data[record]),
                role="vendor RECORD integrity",
            )


def read_wheel(path):
    raw = Path(path).read_bytes()
    digest = sha(raw)
    if digest not in PROFILES:
        raise ValueError("unsupported original wheel SHA256: " + digest)
    with zipfile.ZipFile(io.BytesIO(raw)) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError("duplicate wheel members")
        if any(
            name.startswith("/") or "\\" in name or ".." in Path(name).parts
            for name in names
        ):
            raise ValueError("unsafe wheel member")
        if any(
            name.endswith(("/RECORD.jws", "/RECORD.p7s")) for name in names
        ):
            raise ValueError("signed wheels require separate handling")
        data = {name: archive.read(name) for name in names}
        attrs = {
            info.filename: info.external_attr for info in archive.infolist()
        }
    records = [
        name
        for name in names
        if name.count("/") == 1 and name.endswith(".dist-info/RECORD")
    ]
    if len(records) != 1:
        raise ValueError("expected one RECORD")
    (record,) = records
    seen = set()
    for name, checksum, size in csv.reader(io.StringIO(data[record].decode())):
        if name in seen or name not in data:
            raise ValueError("invalid RECORD membership")
        seen.add(name)
        if name == record:
            if checksum or size:
                raise ValueError("RECORD must not hash itself")
        elif checksum != record_bytes(data[name]) or size != str(
            len(data[name])
        ):
            raise ValueError("RECORD mismatch: " + name)
    if seen != set(names):
        raise ValueError("unrecorded wheel member")
    return digest, data, attrs, record


def repack(input_path, output_path, manifest_path):
    input_path, output_path, manifest_path = map(
        Path, [input_path, output_path, manifest_path]
    )
    if output_path.exists() or manifest_path.exists():
        raise FileExistsError("exclusive outputs required")
    if input_path.resolve() == output_path.resolve():
        raise ValueError("input wheel is immutable")
    if output_path.resolve() == manifest_path.resolve():
        raise ValueError("wheel and manifest require distinct outputs")
    digest, data, attrs, record = read_wheel(input_path)
    changes = {}
    for name, expected in PROFILES[digest].items():
        original = data[name]
        if expected is None or sha(original) != expected:
            raise ValueError("unexpected vendor source: " + name)
        transform = (
            patch_installer
            if name.endswith("/compatibility_tags.py")
            else patch_tags
        )
        data[name] = transform(original.decode()).encode()
        changes[name] = dict(before=sha(original), after=sha(data[name]))
    update_vendor_records(data, changes)
    marker = record.rsplit("/", 1)[0] + "/boilrc1-private-tags.json"
    if marker in data:
        raise ValueError("already patched wheel")
    data[marker] = (
        json.dumps(
            dict(
                policy=POLICY,
                input_wheel_sha256=digest,
                changed_members=changes,
            ),
            sort_keys=True,
            indent=2,
        )
        + "\n"
    ).encode()
    attrs[marker] = 0o100644 << 16
    result = io.StringIO()
    writer = csv.writer(result, lineterminator="\n")
    for name in sorted(data):
        writer.writerow(
            [name, "", ""]
            if name == record
            else [name, record_bytes(data[name]), str(len(data[name]))]
        )
    data[record] = result.getvalue().encode()
    with (
        output_path.open("xb") as stream,
        zipfile.ZipFile(
            stream, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
        ) as archive,
    ):
        for name in sorted(data):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = attrs[name]
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(
                info,
                data[name],
                compress_type=zipfile.ZIP_DEFLATED,
                compresslevel=9,
            )
    report = dict(
        policy=POLICY,
        input=dict(path=str(input_path), sha256=digest),
        output=dict(
            path=str(output_path), sha256=sha(output_path.read_bytes())
        ),
        changed_members=changes,
        record=record,
        marker=marker,
    )
    with manifest_path.open("x") as stream:
        json.dump(report, stream, indent=2, sort_keys=True)
        stream.write("\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    print(
        json.dumps(
            repack(args.input, args.output, args.manifest), sort_keys=True
        )
    )


if __name__ == "__main__":
    main()
