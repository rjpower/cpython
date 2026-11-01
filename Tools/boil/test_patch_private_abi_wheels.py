"""Actual pinned vendor/installer code under fixed host runtime facts."""

import ast
import base64
import contextlib
import csv
import hashlib
import importlib
import importlib.util
import io
import json
import sys
import sysconfig
import tempfile
import types
import typing
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch
import argparse
import patch_private_abi_wheels as tool

REPOSITORY = Path(__file__).resolve().parents[2]
EXPECTED_PATCHED = {
    "pip": "a0f564ec764b528ee5473887dec474c27ad8fa8fe9430fc5843c96e99e5edbdd",
    "setuptools": "93db1553711fa6ae2723e19c467906b306a5e1d1e20287f8f934da5f19fb110a",
}
MEMBERS = [
    ("pip", "pip/_vendor/packaging/tags.py"),
    ("setuptools", "setuptools/_vendor/packaging/tags.py"),
    ("setuptools", "setuptools/_vendor/wheel/vendored/packaging/tags.py"),
]
RESULTS = []


def load_tags(path, name):
    package = types.ModuleType(name)
    package.__path__ = [str(path.parent)]
    sys.modules[name] = package
    return importlib.import_module(name + ".tags")


def facts(case):
    private = case not in ["ordinary_cp", "ordinary_pypy"]
    debug = case == "private_debug"
    soabi = (
        ("boilrc1-316" + ("d" if debug else "") + "-x86_64-linux-gnu")
        if private
        else (
            "cpython-316-x86_64-linux-gnu"
            if case == "ordinary_cp"
            else "pypy316_pp73-x86_64-linux-gnu"
        )
    )
    ext = "." + soabi + ".so"
    suffixes = [ext]
    if debug:
        suffixes.append(".boilrc1-316-x86_64-linux-gnu.so")
    if case == "bad_ext":
        ext = ".inconsistent.so"
    if case == "missing_primary":
        suffixes = []
    config = dict(
        SOABI=soabi,
        EXT_SUFFIX=ext,
        SHLIB_SUFFIX=".so",
        Py_DEBUG=int(debug),
        Py_GIL_DISABLED=0,
        py_version_nodot="316",
    )
    proxy = types.SimpleNamespace(
        **{k: v for k, v in vars(sys).items() if k != "implementation"}
    )
    proxy.version_info = (3, 16, 0, "final", 0)
    proxy.implementation = types.SimpleNamespace(
        name="pypy" if case == "ordinary_pypy" else "cpython"
    )
    return config, suffixes, proxy


@contextlib.contextmanager
def runtime(case, *modules):
    import importlib.machinery

    config, suffixes, proxy = facts(case)
    with contextlib.ExitStack() as stack:
        stack.enter_context(
            patch.object(
                sysconfig,
                "get_config_var",
                side_effect=lambda name: config.get(name),
            )
        )
        stack.enter_context(
            patch.object(importlib.machinery, "EXTENSION_SUFFIXES", suffixes)
        )
        for module in modules:
            stack.enter_context(patch.object(module, "sys", proxy))
            stack.enter_context(
                patch.object(
                    module,
                    "platform_tags",
                    side_effect=lambda: iter(["linux_x86_64"]),
                )
            )
        yield config, suffixes, proxy


def check_private(tags, case):
    with runtime(case, tags):
        result = list(tags.sys_tags())
    allowed = {"boilrc1_316_x86_64_linux_gnu", "none"}
    if case == "private_debug":
        allowed.add("boilrc1_316d_x86_64_linux_gnu")
    assert {tag.abi for tag in result} == allowed
    assert any(
        tag.interpreter == "cp316"
        and tag.abi
        == (
            "boilrc1_316d_x86_64_linux_gnu"
            if case == "private_debug"
            else "boilrc1_316_x86_64_linux_gnu"
        )
        for tag in result
    )
    assert any(str(tag) == "py3-none-any" for tag in result)
    return result


@contextlib.contextmanager
def installer(kind):
    tags = TAGS[kind, 0]
    root = STAGE / kind / "pip"
    saved = {
        k: v
        for k, v in sys.modules.items()
        if k == "pip" or k.startswith("pip.")
    }
    for key in saved:
        del sys.modules[key]
    try:
        for name, directory in [
            ("pip", root / "pip"),
            ("pip._vendor", root / "pip/_vendor"),
            ("pip._vendor.packaging", root / "pip/_vendor/packaging"),
            ("pip._internal", root / "pip/_internal"),
            ("pip._internal.utils", root / "pip/_internal/utils"),
        ]:
            module = types.ModuleType(name)
            module.__path__ = [str(directory)]
            sys.modules[name] = module
        sys.modules["pip._vendor.packaging.tags"] = tags
        compatibility = importlib.import_module(
            "pip._internal.utils.compatibility_tags"
        )
        source = (root / "pip/_internal/models/target_python.py").read_text()
        tree = ast.parse(source)
        node = next(
            n
            for n in tree.body
            if isinstance(n, ast.ClassDef) and n.name == "TargetPython"
        )
        misc = ast.parse((root / "pip/_internal/utils/misc.py").read_text())
        normalize = next(
            n
            for n in misc.body
            if isinstance(n, ast.FunctionDef)
            and n.name == "normalize_version_info"
        )
        namespace = dict(
            __name__="pip._internal.models.target_python",
            sys=sys,
            Tag=tags.Tag,
            get_supported=compatibility.get_supported,
            version_info_to_nodot=compatibility.version_info_to_nodot,
            cast=typing.cast,
        )
        exec(
            compile(
                ast.Module(body=[normalize, node], type_ignores=[]),
                "actual-pinned-TargetPython-and-normalize",
                "exec",
            ),
            namespace,
        )
        yield tags, compatibility, namespace
    finally:
        for key in list(sys.modules):
            if key == "pip" or key.startswith("pip."):
                del sys.modules[key]
        sys.modules.update(saved)


class PolicyRows(unittest.TestCase):
    pass


def vendor_row(index, case):
    def test(self):
        modified = TAGS["patched", index]
        original = TAGS["original", index]
        if case in ["private_release", "private_debug"]:
            result = check_private(modified, case)
            with runtime(case, original):
                old = list(original.sys_tags())
            self.assertFalse(
                any(tag.abi.startswith("boilrc1_") for tag in old),
                "unpatched negative must miss private ABI",
            )
            with runtime(case, modified, original):
                self.assertEqual(
                    list(map(str, modified.cpython_tags())),
                    list(map(str, original.cpython_tags())),
                )
        elif case in ["bad_ext", "missing_primary"]:
            with runtime(case, modified), self.assertRaises(ValueError):
                list(modified.sys_tags())
        else:
            with runtime(case, modified, original):
                self.assertEqual(
                    list(map(str, modified.sys_tags())),
                    list(map(str, original.sys_tags())),
                )
        RESULTS.append(
            dict(
                group="vendor",
                vendor=MEMBERS[index][1],
                case=case,
                passed=True,
            )
        )

    return test


for index in range(3):
    for case in [
        "private_release",
        "private_debug",
        "bad_ext",
        "missing_primary",
        "ordinary_cp",
        "ordinary_pypy",
    ]:
        setattr(
            PolicyRows, f"test_vendor_{index}_{case}", vendor_row(index, case)
        )


def installer_row(case):
    def test(self):
        if case in ["ordinary_cp", "ordinary_pypy"]:
            outputs = []
            for kind in ["original", "patched"]:
                with (
                    installer(kind) as (tags, module, namespace),
                    runtime(case, tags),
                ):
                    outputs.append(list(map(str, module.get_supported())))
            self.assertEqual(*outputs)
        else:
            profile = (
                "private_debug"
                if case.endswith("debug")
                else "private_release"
            )
            with (
                installer("patched") as (tags, module, namespace),
                runtime(profile, tags) as (_, __, proxy),
            ):
                if case.startswith("override_"):
                    key = case.removeprefix("override_")
                    kwargs = {
                        key: {
                            "version": "316",
                            "platforms": ["linux_x86_64"],
                            "impl": "cp",
                            "abis": ["boilrc1_316_x86_64_linux_gnu"],
                        }[key]
                    }
                    with self.assertRaisesRegex(ValueError, "cross-target"):
                        module.get_supported(**kwargs)
                elif case.startswith("target_"):
                    namespace["sys"] = proxy
                    target = namespace["TargetPython"]()
                    result = target.get_sorted_tags()
                    self.assertIs(result, target.get_sorted_tags())
                    self.assertEqual(set(result), target.get_unsorted_tags())
                    self.assertEqual(
                        list(map(str, result)), list(map(str, tags.sys_tags()))
                    )
                else:
                    self.assertEqual(
                        list(map(str, module.get_supported())),
                        list(map(str, tags.sys_tags())),
                    )
        RESULTS.append(dict(group="installer", case=case, passed=True))

    return test


for case in [
    "default_release",
    "default_debug",
    "target_release",
    "target_debug",
    "override_version",
    "override_platforms",
    "override_impl",
    "override_abis",
    "ordinary_cp",
    "ordinary_pypy",
]:
    setattr(PolicyRows, "test_installer_" + case, installer_row(case))


class ArchiveControls(unittest.TestCase):
    def test_two_reproducible_archives_and_records(self):
        for name, source in ORIGINALS.items():
            with (
                self.subTest(wheel=name),
                tempfile.TemporaryDirectory(
                    dir=ROOT, prefix="host-repack-"
                ) as tmp,
            ):
                output = Path(tmp) / source.name
                manifest = Path(tmp) / "manifest.json"
                report = tool.repack(source, output, manifest)
                kept = PATCHED[name]
                self.assertEqual(output.read_bytes(), kept.read_bytes())
                with (
                    zipfile.ZipFile(source) as before,
                    zipfile.ZipFile(output) as after,
                ):
                    allowed = set(report["changed_members"]) | {
                        report["record"],
                        report["marker"],
                    }
                    for member in before.namelist():
                        if member not in allowed:
                            self.assertEqual(
                                before.read(member), after.read(member), member
                            )
                    marker = json.loads(after.read(report["marker"]))
                    self.assertNotIn("output_wheel_sha256", marker)
                    seen = set()
                    for member, digest, size in csv.reader(
                        io.StringIO(after.read(report["record"]).decode())
                    ):
                        seen.add(member)
                        if member == report["record"]:
                            self.assertEqual((digest, size), ("", ""))
                        else:
                            data = after.read(member)
                            expected = (
                                "sha256="
                                + base64.urlsafe_b64encode(
                                    hashlib.sha256(data).digest()
                                )
                                .rstrip(b"=")
                                .decode()
                            )
                            self.assertEqual(digest, expected)
                            self.assertEqual(size, str(len(data)))
                    self.assertEqual(seen, set(after.namelist()))
                    for record in report["changed_members"]:
                        if not record.endswith(".dist-info/RECORD"):
                            continue
                        base = record.rsplit("/", 2)[0] + "/"
                        old_rows = list(
                            csv.reader(
                                io.StringIO(before.read(record).decode())
                            )
                        )
                        new_rows = list(
                            csv.reader(
                                io.StringIO(after.read(record).decode())
                            )
                        )
                        self.assertEqual(
                            [row[0] for row in old_rows],
                            [row[0] for row in new_rows],
                        )
                        for old_row, new_row in zip(
                            old_rows, new_rows, strict=True
                        ):
                            if (
                                base + old_row[0]
                                not in report["changed_members"]
                            ):
                                self.assertEqual(old_row, new_row)
                        for relative, digest, size in csv.reader(
                            io.StringIO(after.read(record).decode())
                        ):
                            member = base + relative
                            if (
                                member != record
                                and member in report["changed_members"]
                            ):
                                data = after.read(member)
                                self.assertEqual(
                                    digest, tool.record_bytes(data)
                                )
                                self.assertEqual(size, str(len(data)))
                    self.assertEqual(
                        sum(
                            name.endswith(".dist-info/RECORD")
                            for name in report["changed_members"]
                        ),
                        2 if name == "setuptools" else 0,
                    )

    def test_unknown_and_altered_vendor_fail_before_output(self):
        source = ORIGINALS["pip"]
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        with tempfile.TemporaryDirectory(
            dir=ROOT, prefix="host-negative-"
        ) as tmp:
            root = Path(tmp)
            bad = root / "unknown.whl"
            bad.write_bytes(b"not a wheel")
            with self.assertRaisesRegex(ValueError, "SHA256"):
                tool.repack(bad, root / "out.whl", root / "m.json")
            self.assertFalse((root / "out.whl").exists())
            wrong = dict(tool.PROFILES[digest])
            wrong["pip/_vendor/packaging/tags.py"] = "0" * 64
            with (
                patch.dict(tool.PROFILES, {digest: wrong}),
                self.assertRaisesRegex(ValueError, "vendor source"),
            ):
                tool.repack(source, root / "out.whl", root / "m.json")
            self.assertFalse((root / "out.whl").exists())

    def test_exclusive_output(self):
        with tempfile.TemporaryDirectory(
            dir=ROOT, prefix="host-existing-"
        ) as tmp:
            root = Path(tmp)
            out = root / "wheel"
            out.write_bytes(b"keep")
            with self.assertRaises(FileExistsError):
                tool.repack(ORIGINALS["pip"], out, root / "m.json")
            self.assertEqual(out.read_bytes(), b"keep")

    def test_signed_archive_rejected_before_output(self):
        with tempfile.TemporaryDirectory(
            dir=ROOT, prefix="host-signature-"
        ) as tmp:
            root = Path(tmp)
            signed = root / "signed.whl"
            with (
                zipfile.ZipFile(ORIGINALS["pip"]) as before,
                zipfile.ZipFile(signed, "w") as after,
            ):
                for member in before.namelist():
                    after.writestr(member, before.read(member))
                after.writestr("pip-26.2.2.dist-info/RECORD.jws", b"signature")
            digest = tool.sha(signed.read_bytes())
            profile = tool.PROFILES[tool.sha(ORIGINALS["pip"].read_bytes())]
            with (
                patch.dict(tool.PROFILES, {digest: profile}),
                self.assertRaisesRegex(ValueError, "signed wheels"),
            ):
                tool.repack(signed, root / "out.whl", root / "m.json")
            self.assertFalse((root / "out.whl").exists())

    def test_manifest_and_wheel_cannot_alias(self):
        with tempfile.TemporaryDirectory(
            dir=ROOT, prefix="host-output-alias-"
        ) as tmp:
            out = Path(tmp) / "same"
            with self.assertRaisesRegex(ValueError, "distinct outputs"):
                tool.repack(ORIGINALS["pip"], out, out)
            self.assertFalse(out.exists())


def prepare_fixtures(original_pip, original_setuptools, work_dir):
    global ROOT, ORIGINALS, PATCHED, TAGS, STAGE
    ROOT = work_dir.resolve()
    ROOT.mkdir(parents=True, exist_ok=False)
    ORIGINALS = {
        "pip": original_pip.resolve(),
        "setuptools": original_setuptools.resolve(),
    }
    PATCHED = {
        "pip": REPOSITORY
        / "Lib/ensurepip/_bundled/pip-26.2.2-py3-none-any.whl",
        "setuptools": REPOSITORY
        / "Lib/test/wheeldata/setuptools-79.0.1-py3-none-any.whl",
    }
    STAGE = ROOT / "source-fixtures"
    for name, original in ORIGINALS.items():
        tool.read_wheel(original)
        if tool.sha(PATCHED[name].read_bytes()) != EXPECTED_PATCHED[name]:
            raise ValueError("unexpected patched wheel: " + name)
        for kind, source in [
            ("original", original),
            ("patched", PATCHED[name]),
        ]:
            destination = STAGE / kind / name
            destination.mkdir(parents=True)
            with zipfile.ZipFile(source) as archive:
                archive.extractall(destination)
    TAGS = {
        (kind, index): load_tags(
            STAGE / kind / name / member, f"_boil_tag_fixture_{kind}_{index}"
        )
        for kind in ["original", "patched"]
        for index, (name, member) in enumerate(MEMBERS)
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original-pip", type=Path, required=True)
    parser.add_argument("--original-setuptools", type=Path, required=True)
    parser.add_argument(
        "--work-dir",
        type=Path,
        required=True,
        help="new task-owned directory for extracted host fixtures",
    )
    args = parser.parse_args()
    prepare_fixtures(
        args.original_pip, args.original_setuptools, args.work_dir
    )
    result = unittest.TextTestRunner(verbosity=2).run(
        unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__])
    )
    (ROOT / "policy-rows.json").write_text(
        json.dumps(
            dict(
                rows=RESULTS,
                expected=28,
                actual=len(RESULTS),
                all_passed=result.wasSuccessful(),
            ),
            indent=2,
            sort_keys=True,
        )
        + "\n"
    )
    raise SystemExit(not result.wasSuccessful())
