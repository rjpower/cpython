"""Parse and validate package recipes, ports/<ref>/recipe.json.

A recipe pins everything a build reads from outside the repository: the
source archive by sha256, each patch by sha256, and each upstream file a patch
touches by sha256. Unknown fields are errors, so a typo cannot silently drop
a pin. Parsing reads only the recipe directory; it never downloads.
"""

from __future__ import annotations

import hashlib
import json
import re
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any

from ports._support.common import PORTS, PortError

_REF = re.compile(r"[a-z0-9][a-z0-9._-]*(/[a-z0-9][a-z0-9._-]*)+")
_SHA256 = re.compile(r"[0-9a-f]{64}")
_PIN = re.compile(r"[A-Za-z0-9._-]+==[A-Za-z0-9.!+_-]+")
_REQUIRED = {"name", "version", "kind", "source"}
_OPTIONAL = {
    "requires", "build_requires", "build_options", "patches", "tests", "bench",
}
_PATCH_FIELDS = {"file", "sha256", "touches"}
_SUFFIX = {
    "pure-wheel": "-none-any.whl",
    "sdist": ".tar.gz",
    "configure-make": ".tar.gz",
}
NATIVE = "configure-make"  # the only kind that builds a library, not a wheel


@dataclass(frozen=True)
class Source:
    url: str
    sha256: str

    @property
    def filename(self) -> str:
        return self.url.rsplit("/", 1)[1]


@dataclass(frozen=True)
class Patch:
    """A patch file and the upstream files it modifies, all by sha256."""

    file: str
    sha256: str
    touches: dict[str, str]


@dataclass(frozen=True)
class Recipe:
    ref: str
    directory: Path
    digest: str  # sha256 of the recipe.json bytes
    name: str
    version: str
    kind: str
    source: Source
    requires: tuple[str, ...]
    build_requires: tuple[str, ...]
    build_options: tuple[str, ...]  # extra `pip wheel` arguments (sdist only)
    patches: tuple[Patch, ...]
    tests: tuple[str, ...]
    bench: tuple[str, ...]


def is_native(recipe: Recipe) -> bool:
    return recipe.kind == NATIVE


def check_ref(ref: str) -> str:
    if not _REF.fullmatch(ref):
        raise PortError(f"{ref!r} is not a port reference like python/six")
    return ref


def all_refs(root: Path = PORTS) -> list[str]:
    """Every package port, the default selection for each verb."""
    return [
        path.parent.relative_to(root).as_posix()
        for path in sorted(root.glob("python/*/recipe.json"))
    ]


def load_recipe(ref: str, root: Path = PORTS) -> Recipe:
    path = root / check_ref(ref) / "recipe.json"
    if not path.is_file():
        raise PortError(f"{ref}: no recipe at {path}")
    data = path.read_bytes()
    document = json.loads(data)
    if not isinstance(document, dict):
        raise PortError(f"{ref}: recipe.json must hold an object")
    digest = hashlib.sha256(data).hexdigest()
    return _parse(ref, path.parent, digest, document)


def _parse(
    ref: str, directory: Path, digest: str, document: dict[str, Any]
) -> Recipe:
    missing = _REQUIRED - set(document)
    unknown = set(document) - _REQUIRED - _OPTIONAL
    if missing or unknown:
        raise PortError(
            f"{ref}: missing fields {sorted(missing)},"
            f" unknown fields {sorted(unknown)}"
        )
    name, version, kind = (
        _string(ref, document[key], key) for key in ("name", "version", "kind")
    )
    if name != ref.rsplit("/", 1)[1]:
        raise PortError(f"{ref}: name {name!r} must match the directory")
    if kind not in _SUFFIX:
        raise PortError(f"{ref}: kind must be one of {sorted(_SUFFIX)}")
    requires = tuple(
        check_ref(_string(ref, item, "requires"))
        for item in _items(ref, document, "requires")
    )
    build_requires = tuple(
        _pin(ref, item) for item in _items(ref, document, "build_requires")
    )
    patches = tuple(
        _patch(ref, directory, item)
        for item in _items(ref, document, "patches")
    )
    build_options = tuple(
        _string(ref, item, "build_options")
        for item in _items(ref, document, "build_options")
    )
    if kind == "pure-wheel" and (patches or build_requires or build_options):
        raise PortError(
            f"{ref}: a pure-wheel port installs the published wheel as is;"
            " it takes no patches, build_requires or build_options"
        )
    tests = _scripts(ref, directory, document, "tests")
    if kind == NATIVE:
        if (patches or build_requires or build_options or requires or tests
                or "bench" in document):
            raise PortError(f"{ref}: a {kind} library takes only name,"
                            " version, kind and source")
    elif not tests:
        raise PortError(f"{ref}: declare at least one test script")
    return Recipe(
        ref, directory, digest, name, version, kind,
        _source(ref, document["source"], kind), requires, build_requires,
        build_options, patches, tests, _scripts(ref, directory, document, "bench"),
    )


def _source(ref: str, value: object, kind: str) -> Source:
    if not isinstance(value, dict) or set(value) - {"url", "sha256"}:
        raise PortError(f"{ref}: source takes only url and sha256")
    url = _string(ref, value.get("url"), "source.url")
    if not url.startswith("https://") or not url.endswith(_SUFFIX[kind]):
        raise PortError(f"{ref}: a {kind} source must be an https:// URL"
                        f" ending in {_SUFFIX[kind]}")
    return Source(url, _sha256(ref, value.get("sha256"), "source.sha256"))


def _patch(ref: str, directory: Path, value: object) -> Patch:
    if not isinstance(value, dict) or set(value) != _PATCH_FIELDS:
        raise PortError(f"{ref}: a patch needs exactly file, sha256, touches")
    file = _relpath(ref, value["file"])
    if not (directory / file).is_file():
        raise PortError(f"{ref}: patch {file} does not exist")
    touches = value["touches"]
    if not isinstance(touches, dict):
        raise PortError(f"{ref}: {file} touches must map paths to sha256")
    return Patch(file, _sha256(ref, value["sha256"], file), {
        _relpath(ref, path): _sha256(ref, digest, f"{file} touches {path}")
        for path, digest in touches.items()
    })


def _scripts(
    ref: str, directory: Path, document: dict[str, Any], key: str
) -> tuple[str, ...]:
    scripts = []
    for item in _items(ref, document, key):
        if not isinstance(item, dict) or set(item) != {"script"}:
            raise PortError(f"{ref}: {key} entries are {{\"script\": path}}")
        script = _relpath(ref, item["script"])
        if not (directory / script).is_file():
            raise PortError(f"{ref}: {key} script {script} does not exist")
        scripts.append(script)
    return tuple(scripts)


def _items(ref: str, document: dict[str, Any], key: str) -> list[Any]:
    value = document.get(key, [])
    if not isinstance(value, list):
        raise PortError(f"{ref}: {key} must be a list")
    return value


def _pin(ref: str, value: object) -> str:
    if not isinstance(value, str) or not _PIN.fullmatch(value):
        raise PortError(f"{ref}: build_requires {value!r} must be name==X")
    return value


def _string(ref: str, value: object, what: str) -> str:
    if not isinstance(value, str) or not value:
        raise PortError(f"{ref}: {what} must be a non-empty string")
    return value


def _sha256(ref: str, value: object, what: str) -> str:
    if not isinstance(value, str) or not _SHA256.fullmatch(value):
        raise PortError(f"{ref}: {what} needs a sha256 (64 hex digits)")
    return value


def _relpath(ref: str, value: object) -> str:
    """Admit a path that stays inside the directory it is relative to."""
    path = PurePosixPath(_string(ref, value, "path"))
    if path.is_absolute() or ".." in path.parts:
        raise PortError(f"{ref}: path {value!r} must be relative and inward")
    return path.as_posix()
