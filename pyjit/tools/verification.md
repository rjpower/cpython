# Verification tools

Use the [validation contract](../design/validation.md) for current gates and
the [verification issue](../issues/verification-orchestration/status.md) for
unfinished shared orchestration.

- `verify.manifest.json` declares named selections, dependencies and exclusions.
- `verify.py validate` checks the registry and an explicit case inventory;
  `show` displays a plan. Shared profile execution is not implemented there.
- `verify_launch.py` and `verify_worker.sh` record actual executing worker
  identity/modes/results. Concrete qualified campaigns use their bound helpers.
- `gate.sh` and `gate_run.sh` supply numeric gates and admission/cleanup.
  Use actual build locks, fresh target caches and the existing global slots.

Schema validation is not native coverage. A plan, an expected mode or a passing
predecessor cannot stand in for actual terminal workers and current bindings.
Keep exact exclusions and failures visible. See the
[tool guide](README.md) for the maintained helper entry points.
