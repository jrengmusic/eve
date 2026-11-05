# DEBT.md

**Purpose:** Inter-sprint ledger of debts — bugs, nitpicks, friction observed during usage. Drained by sprints via `/pay` (COUNSELOR planning) and `/log` (hygiene drain). **JRENG = paid in full, cash. No triage.**

**Format:** Each entry uses **O / D / E** articulation — Observation, Divergence, Expectation. IDs are UTC timestamps (`DEBT-YYYYMMDDTHHMMSS`). Newest entries at top. Add via `carol debt add`.

**Lifecycle:** Created lazily on first `carol debt add`. Entries appended via interactive prompt. Entries removed by `carol debt clear <id>` (called by `/log` hygiene step after SPRINT-LOG receipt is written). Survives `carol reset` — debts persist across protocol resets.

---

## DEBT-20261105T000000

**Observation:** PLAN-1 implements Windows with no Windows run: WIC decoders with ComApartment on a pool thread (`jam_TerminalImageSequence_windows.cpp`), `Format::toNativePath` for an MSYS2 path such as `/c/Users/x`, the `eve` script on `PATH` under MSYS2 zsh, and `powershell_integration.ps1`.
**Divergence:** ARCHITECT: "and also implement for windows. runtime test still need to be tested when i get to windows. add to DEBT." No Windows build or run has tested this code.
**Expectation:** On Windows, build, then run `fzf --preview 'eve preview {}'` with PNG, JPEG and GIF, from MSYS2 zsh and from PowerShell, and confirm the image shows at the preview box and a text file shows none.

---




