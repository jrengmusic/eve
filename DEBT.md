# DEBT.md

**Purpose:** Inter-sprint ledger of debts — bugs, nitpicks, friction observed during usage. Drained by sprints via `/pay` (COUNSELOR planning) and `/log` (hygiene drain). **JRENG = paid in full, cash. No triage.**

**Format:** Each entry uses **O / D / E** articulation — Observation, Divergence, Expectation. IDs are UTC timestamps (`DEBT-YYYYMMDDTHHMMSS`). Newest entries at top. Add via `carol debt add`.

**Lifecycle:** Created lazily on first `carol debt add`. Entries appended via interactive prompt. Entries removed by `carol debt clear <id>` (called by `/log` hygiene step after SPRINT-LOG receipt is written). Survives `carol reset` — debts persist across protocol resets.

---

## DEBT-20261025T214955

**Observation:** EVE terminal protocol: a custom OSC code carries a CommandId name.
**Divergence:** No OSC code reaches the commands. User shell scripts cannot invoke them.
**Expectation:** The processor sends the CommandId name up through events, and the editor calls CommandManager::invokeDirectly. User shell scripts reach the commands through it.

---

