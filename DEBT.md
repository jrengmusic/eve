# DEBT.md

**Purpose:** Inter-sprint ledger of debts — bugs, nitpicks, friction observed during usage. Drained by sprints via `/pay` (COUNSELOR planning) and `/log` (hygiene drain). **JRENG = paid in full, cash. No triage.**

**Format:** Each entry uses **O / D / E** articulation — Observation, Divergence, Expectation. IDs are UTC timestamps (`DEBT-YYYYMMDDTHHMMSS`). Newest entries at top. Add via `carol debt add`.

**Lifecycle:** Created lazily on first `carol debt add`. Entries appended via interactive prompt. Entries removed by `carol debt clear <id>` (called by `/log` hygiene step after SPRINT-LOG receipt is written). Survives `carol reset` — debts persist across protocol resets.

---

## DEBT-20261016T214137

**Observation:** EVE Standalone needs about 5–7 s from launch until the window shows; jfs Standalone, built with the same jam, opens almost at once.
**Divergence:** The EVEView constructor runs the whole terminal round trip synchronously on the message thread before the window exists (EVEViewComponents.cpp registerConfig<jam::TextEditor> lambda → getTerminalFrame): MarkdownDocument::parse of markdown/markdown.md, MarkdownComponent layout, one software image per mermaid diagram at display scale, one OSC 1337 PNG+Base64 encode per image into a std::tmpfile, AnsiDocument::parse, one PNG decode per image in the TextEditor.
**Expectation:** EVE opens in subseconds, grand total, like jfs; find and remove every bottleneck on the launch path.

---
