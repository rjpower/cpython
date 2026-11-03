"""Admit external inputs: source archives and the patches applied to them.

Admission happens before anything runs. An archive whose sha256 differs from
its pin is refused. Patches are applied only after the patch file and every
upstream file it touches match their pins, and `touches` must list exactly
the files the patch modifies. A pin bump that changes a patched file therefore
fails naming that file instead of fuzzing the patch onto new code.
"""

from __future__ import annotations

import json
import platform
import re
import shutil
import tarfile
import urllib.request
from pathlib import Path

from ports._support.common import (
    STATE,
    PortError,
    fresh_dir,
    log,
    run,
    sha256_file,
)
from ports._support.recipe import Patch, Recipe, Source

_HUNK = re.compile(r"@@ -\d+(?:,(\d+))? \+\d+(?:,(\d+))? @@")


def fetch(source: Source) -> Path:
    """Return the verified archive for source, downloading it if absent."""
    path = STATE / "sources" / source.filename
    if path.exists():
        _verify(path, source.sha256)
    else:
        _download(source.url, path, source.sha256)
    return path


def fetch_requirement(pin: str) -> Path:
    """Return the wheel PyPI publishes for a name==version pin.

    This is the universal wheel or, for a tool that ships a binary (ninja),
    the py3-none manylinux wheel for the host's machine. The host fetches
    build requirements, so the target interpreter needs neither ssl nor
    network access. PyPI's lookup is cached for offline rebuilds, and its
    published sha256 is verified like a recipe pin.
    """
    name, _, version = pin.partition("==")
    lookup = STATE / "sources" / "pypi" / f"{name}-{version}.json"
    if not lookup.exists():
        _download(f"https://pypi.org/pypi/{name}/{version}/json", lookup)
    files = json.loads(lookup.read_text())["urls"]
    machine = re.escape(platform.machine())
    tool = re.compile(rf"[^-]+-[^-]+-py3-none-manylinux[^-]*_{machine}[^-]*\.whl")
    wheels = [f for f in files if f["filename"].endswith("-none-any.whl")]
    wheels = wheels or [f for f in files if tool.fullmatch(f["filename"])]
    if not wheels:
        raise PortError(f"{pin}: PyPI publishes no universal or"
                        f" {platform.machine()} py3-none wheel")
    return fetch(Source(wheels[0]["url"], wheels[0]["digests"]["sha256"]))


def _download(url: str, path: Path, sha256: str | None = None) -> None:
    """Download url to path; path never holds a partial or refused file."""
    path.parent.mkdir(parents=True, exist_ok=True)
    partial = path.with_name(path.name + ".part")
    log.info("fetch %s", url)
    try:
        with (
            urllib.request.urlopen(url) as response,
            partial.open("wb") as stream,
        ):
            shutil.copyfileobj(response, stream)
    except OSError as error:
        raise PortError(f"cannot fetch {url}: {error}") from error
    if sha256 is not None:
        try:
            _verify(partial, sha256)
        except PortError:
            partial.unlink()
            raise
    partial.rename(path)


def _verify(path: Path, expected: str) -> None:
    actual = sha256_file(path)
    if actual != expected:
        raise PortError(
            f"{path.name}: sha256 is {actual}, the recipe pins {expected};"
            " refusing it"
        )


def unpack(archive: Path, destination: Path) -> Path:
    """Extract archive into a fresh directory; return its top directory."""
    fresh_dir(destination)
    with tarfile.open(archive) as tar:
        tar.extractall(destination, filter="data")
    entries = list(destination.iterdir())
    if len(entries) != 1 or not entries[0].is_dir():
        raise PortError(f"{archive.name}: expected one top-level directory")
    return entries[0]


def parse_patch(text: str) -> tuple[frozenset[str], int]:
    """Return the upstream paths a -p1 unified diff modifies, and the
    number of lines it adds or removes.

    Hunk headers give line counts, so a removed line that itself starts
    with "-- " is never mistaken for a file header.
    """
    paths: set[str] = set()
    changed = old = new = 0  # old/new: lines left in the current hunk
    for line in text.splitlines():
        if old > 0 or new > 0:
            tag = line[:1]
            if tag in ("-", "+"):
                changed += 1
                old, new = (old - 1, new) if tag == "-" else (old, new - 1)
            elif tag != "\\":  # "\ No newline at end of file" is metadata
                old, new = old - 1, new - 1
        elif line.startswith("--- "):
            name = line[4:].split("\t")[0]
            if name != "/dev/null":  # new files have no upstream contents
                paths.add(_strip_component(name))
        elif match := _HUNK.match(line):
            old, new = (1 if n is None else int(n) for n in match.groups())
    return frozenset(paths), changed


def _strip_component(name: str) -> str:
    _, _, path = name.partition("/")
    if not path:
        raise PortError(f"patch path {name!r} is not in -p1 form")
    return path


def admit_patch(patch: Patch, directory: Path, tree: Path) -> None:
    """Refuse patch unless it and the upstream files it touches match."""
    path = directory / patch.file
    actual = sha256_file(path)
    if actual != patch.sha256:
        raise PortError(f"patch {patch.file}: sha256 is {actual},"
                        f" the recipe pins {patch.sha256}")
    modified, _ = parse_patch(path.read_text())
    if modified != set(patch.touches):
        raise PortError(f"patch {patch.file} modifies {sorted(modified)}"
                        f" but touches lists {sorted(patch.touches)}")
    for name, expected in sorted(patch.touches.items()):
        target = tree / name
        if not target.is_file():
            raise PortError(f"patch {patch.file}: {name} is not in the source")
        actual = sha256_file(target)
        if actual != expected:
            raise PortError(
                f"patch {patch.file}: upstream {name} has sha256 {actual},"
                f" the recipe pins {expected}; refresh the patch"
            )


def apply_patches(recipe: Recipe, tree: Path, output: Path) -> None:
    """Apply recipe's patches in order; `touches` pins upstream contents,
    so every patch is admitted before the first one changes the tree."""
    for patch in recipe.patches:
        admit_patch(patch, recipe.directory, tree)
    for patch in recipe.patches:
        run([
            "patch", "-p1", "--forward", "--batch", "--fuzz=0",
            "--no-backup-if-mismatch", "-d", tree,
            "-i", recipe.directory / patch.file,
        ], output)


def patch_lines(recipe: Recipe) -> int:
    """Count hand-written patch lines: the maintenance cost of a port."""
    return sum(
        parse_patch((recipe.directory / patch.file).read_text())[1]
        for patch in recipe.patches
    )
