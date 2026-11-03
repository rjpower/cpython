"""Build Cython as pure Python: it is a build tool, and the compiled
modules would only make code generation faster."""

from dataclasses import replace

from ports._support.build import BuildContext, pip_wheel


def build(ctx: BuildContext) -> None:
    pip_wheel(replace(ctx, env={**ctx.env, "NO_CYTHON_COMPILE": "true"}))
