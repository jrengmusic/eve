# HANDOFF — EVE

**Date:** 2026-10-27
**Record:** JAM `carol/SPRINT-LOG.md` (Sprint 176 is the last logged sprint).
**Plan in flight:** `~/Documents/Poems/dev/jam/PLAN-terminal-model-lane.md` (approved 2026-10-27).

The handoff of 2026-09-13 (first light of the one-shot viewer) is superseded. EVE launches, runs the shell and renders it.

---

## Read First

1. `SPEC.md` — Core Principles 7–10 (two state lanes, no second copy of a truth, one prepare path, one serial clock context), the State Model and its Terminal state machine section, and the Architecture Constraints.
2. `RFC-EVE-terminal-grand-scheme.md` — the 2026-10-27 amendments in §0, §3.9 and §8. They quote ARCHITECT's rulings verbatim.
3. The plan above — the steps, the new names and the validation of each step.

## Current State

- **Throughput** (`comparison.md`): `seq 1 100000000` in 1:01.59 at 98% CPU; third behind endless (59.609) and WezTerm (1:00.90), ahead of Ghostty, kitty and Alacritty.
- **Sprint 176** fixed the blank prompt at launch, routed resize and history capacity through `prepareToPlay`, ordered the auto-scroll after the content size, and gave the TTY its open-or-resize decision.
- **In flight** (the plan): the terminal state machine moves from nine APVTS host parameters to the `jam::Model` lane (`model.md`, `ModelLayout`); `parameters.md` returns to automatable parameters only; a row's atom count becomes its content end; PTY readiness and the clock share one serial context with zero-sample ticks; KANJUT is synced.

## Build

`cast cast/spell.md --debug` (EVE) after `cast cast/spell.md` (JAM). ARCHITECT builds.
