"""Recipe admission: parsing pins, patch pins and dependency order."""

import hashlib
import json
from pathlib import Path

import pytest

from ports._support.common import PortError
from ports._support.graph import resolve
from ports._support.recipe import Patch, load_recipe
from ports._support.sources import admit_patch, parse_patch

ZEROS = "0" * 64
PATCH = """\
--- a/lib.c
+++ b/lib.c
@@ -1,2 +1,2 @@
 int x;
-int y;
+long y;
"""


def write_recipe(root: Path, name: str, **fields: object) -> None:
    directory = root / "python" / name
    (directory / "tests").mkdir(parents=True)
    (directory / "tests" / "smoke.py").write_text("")
    url = f"https://example.invalid/{name}-1.0-py3-none-any.whl"
    recipe = {
        "name": name,
        "version": "1.0",
        "kind": "pure-wheel",
        "source": {"url": url, "sha256": ZEROS},
        "tests": [{"script": "tests/smoke.py"}],
        **fields,
    }
    (directory / "recipe.json").write_text(json.dumps(recipe))


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


@pytest.fixture
def patch_inputs(tmp_path: Path) -> tuple[Path, Path]:
    """A recipe directory holding PATCH and the upstream tree it fits."""
    recipe_dir, tree = tmp_path / "recipe", tmp_path / "src"
    recipe_dir.mkdir()
    tree.mkdir()
    (recipe_dir / "fix.patch").write_text(PATCH)
    (tree / "lib.c").write_text("int x;\nint y;\n")
    return recipe_dir, tree


def test_recipe_without_source_sha256_is_rejected(tmp_path: Path) -> None:
    url = "https://example.invalid/pkg-1.0-py3-none-any.whl"
    write_recipe(tmp_path, "pkg", source={"url": url})
    with pytest.raises(PortError, match="source.sha256 needs a sha256"):
        load_recipe("python/pkg", tmp_path)


@pytest.mark.parametrize(("touches", "message"), [
    ({"lib.c": ZEROS}, "upstream lib.c has sha256 .*refresh the patch"),
    ({}, r"modifies \['lib.c'\] but touches lists \[\]"),
])
def test_patch_admission_rejects_touches_mismatch(
    patch_inputs: tuple[Path, Path], touches: dict[str, str], message: str
) -> None:
    recipe_dir, tree = patch_inputs
    patch = Patch("fix.patch", sha256(recipe_dir / "fix.patch"), touches)
    with pytest.raises(PortError, match=message):
        admit_patch(patch, recipe_dir, tree)


def test_patch_admission_accepts_matching_pins(
    patch_inputs: tuple[Path, Path],
) -> None:
    recipe_dir, tree = patch_inputs
    touches = {"lib.c": sha256(tree / "lib.c")}
    admit_patch(Patch("fix.patch", sha256(recipe_dir / "fix.patch"), touches),
                recipe_dir, tree)


def test_patch_lines_that_look_like_headers_are_hunk_lines() -> None:
    # Removing "-- note" and adding "++ note" produce header-like lines.
    text = "--- a/q.sql\n+++ b/q.sql\n@@ -1 +1 @@\n--- note\n+++ note\n"
    assert parse_patch(text) == (frozenset({"q.sql"}), 2)
    assert parse_patch(PATCH) == (frozenset({"lib.c"}), 2)


def test_graph_orders_dependencies_first(tmp_path: Path) -> None:
    write_recipe(tmp_path, "app", requires=["python/lib", "python/base"])
    write_recipe(tmp_path, "lib", requires=["python/base"])
    write_recipe(tmp_path, "base")
    order = [node.ref for node in resolve(["python/app"], tmp_path)]
    assert order == ["python/base", "python/lib", "python/app"]


def test_graph_rejects_cycles(tmp_path: Path) -> None:
    write_recipe(tmp_path, "a", requires=["python/b"])
    write_recipe(tmp_path, "b", requires=["python/a"])
    with pytest.raises(PortError, match="python/a -> python/b -> python/a"):
        resolve(["python/a"], tmp_path)


@pytest.mark.parametrize(("fields", "message"), [
    ({"kind": "sdist", "build_options": [3]}, "build_options must be a non-"),
    ({"build_options": ["-Cx=1"]}, "takes no patches, build_requires or"),
])
def test_build_options_are_validated(
    tmp_path: Path, fields: dict[str, object], message: str
) -> None:
    write_recipe(tmp_path, "pkg", **fields)
    with pytest.raises(PortError, match=message):
        load_recipe("python/pkg", tmp_path)
