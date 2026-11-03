# Consolidate project documentation

- State: closed
- Outcome: reported
- Decision: integrate the consolidated layout
- Owner: roots_audit; coordinator reviews and integrates
- Updated: 2026-10-31
- Budget: 60 minutes; first inventory checkpoint within 10 minutes
- Depends on: none

## Hypothesis

One consistent live layout, concise workflow and bounded experiment records make
next actions unambiguous without requiring readers to reconstruct old timelines.

## Experiment

Move current contracts to descriptive design paths and work records to issues.
Archive inactive notes/reviews and remove their historical navigation from live
docs. Replace the long workflow with a short action summary and relevant links.
Preserve user changes and raw evidence. Check current links; archival links need
not be repaired. Keep source code, builds and raw benchmark outputs unchanged.

## Decision rule

Integrate when live navigation, status ownership and experiment scoping are
consistent, old top-level reviews/numbered work layouts are removed, and unrelated
user bytes survive. Reject/revise any migration that loses content or preserves
two competing live layouts.

## Now / next

Migration and coordinator review are complete. Current contracts and issue
records replace the mixed live layout; workflow and entry points are concise.
Original documents and user-owned review bytes were independently checked.
The migration is pushed at abd5c0926df, with concise workflow and experiment
records at 71ee990b802. The benchmark guide now follows the same current layout;
its obsolete narrative/report is preserved separately. Future work follows the
bounded template.
