"""Report where the declared records differ from the inferred ones.

For each uop of Python/bytecodes.c that declares a contract, the record the
generator builds from the declarations (ir_contract.py) beside the record
that today's inference (ir_records.py) gives the same body: effects, guard
facts and result facts.  Review reads it; the generator never does.  It
lives until G0d deletes the inference, and this script with it.

    python Tools/cases_generator/ir_contract_diff.py [Python/bytecodes.c]
"""

import argparse
from dataclasses import replace

import analyzer
from generators_common import ROOT
import ir_contract
import ir_generator
from ir_records import Subjects, guard_facts, infer_effects, result_facts, site_tokens
from ir_records import GUARD_TOKENS


def inferred(uop: analyzer.Uop) -> analyzer.Uop:
    "The uop without its clauses, which the inference reads"
    return replace(uop, clauses={})


def differences(records, parts: dict[tuple[str, str], analyzer.Uop]) -> list[str]:
    "The differences, one per line, given the uops by handler and name"
    lines = []
    seen = set()
    compared: list[str] = []
    for handler, record in sorted(records.items()):
        for urec in record.uops:
            part = parts.get((handler, urec.name))
            if part is None or ir_contract.contract_of(part) is None:
                continue
            key = (urec.name, tuple(str(i) for i in urec.inputs))
            if key in seen:
                continue
            seen.add(key)
            compared.append(urec.name)
            bare = inferred(part)
            found = []
            effects = infer_effects(bare)
            if effects != urec.effects:
                found.append(f"effects: declared {sorted(urec.effects)}, "
                             f"inferred {sorted(effects)}")
            subjects = Subjects(bare, urec.inputs)
            sites = [t for t in site_tokens(bare) if t.text in GUARD_TOKENS]
            for site, guard in zip(sites, urec.guards[-len(sites):] if sites else ()):
                facts, reason = guard_facts(bare, site, subjects)
                if tuple(map(str, facts)) != tuple(map(str, guard.facts)):
                    found.append(f"guard at line {site.line}: declared "
                                 f"{[str(f) for f in guard.facts]}, inferred "
                                 f"{[str(f) for f in facts]}")
            results = result_facts(bare, urec.outputs)
            if tuple(map(str, results)) != tuple(map(str, urec.results)):
                found.append(f"results: declared {[str(f) for f in urec.results]}, "
                             f"inferred {[str(f) for f in results]}")
            for line in found:
                lines.append(f"{handler}/{urec.name}: {line}")
    lines.append(f"{len(compared)} declared uops compared")
    return lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("input", nargs="?", default=str(ROOT / "Python" / "bytecodes.c"))
    args = parser.parse_args()
    analysis = analyzer.analyze_files([args.input])
    records = ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))
    parts = {(name, part.name): part
             for name, inst in ir_generator.handler_instructions(analysis).items()
             for part in inst.parts if isinstance(part, analyzer.Uop)}
    lines = differences(records, parts)
    print("\n".join(lines))


if __name__ == "__main__":
    main()
