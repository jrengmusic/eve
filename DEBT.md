# DEBT.md

**Purpose:** Inter-sprint ledger of debts — bugs, nitpicks, friction observed during usage. Drained by sprints via `/pay` (COUNSELOR planning) and `/log` (hygiene drain). **JRENG = paid in full, cash. No triage.**

**Format:** Each entry uses **O / D / E** articulation — Observation, Divergence, Expectation. IDs are UTC timestamps (`DEBT-YYYYMMDDTHHMMSS`). Newest entries at top. Add via `carol debt add`.

**Lifecycle:** Created lazily on first `carol debt add`. Entries appended via interactive prompt. Entries removed by `carol debt clear <id>` (called by `/log` hygiene step after SPRINT-LOG receipt is written). Survives `carol reset` — debts persist across protocol resets.

---

## DEBT-20261108T145906

**Observation:** In Claude Code on the normal screen, Ctrl+L clears the display but the active prompt row is not rendered afterwards. The history stays, and typing still works.
**Divergence:** ARCHITECT: "there's another bug with Claude Code with normal screen which probably related, if i pressed ctrl+l (clear?), the whole history perserved but active prompt gone, although i can still type"; "ctrl+l, still destroys active prompt in CC with normal screen"; "add to DEBT.md ctrl+l destroys CC active prompt rendering with normal screen".
**Expectation:** After Ctrl+L in Claude Code on the normal screen, the prompt renders again at the active row. The same check passes in fullscreen (alternate screen).

---

## DEBT-20261105T000000

**Observation:** PLAN-1 implements Windows with no Windows run: WIC decoders with ComApartment on a pool thread (`jam_TerminalImageSequence_windows.cpp`), `Format::toNativePath` for an MSYS2 path such as `/c/Users/x`, the `eve` script on `PATH` under MSYS2 zsh, and `powershell_integration.ps1`.
**Divergence:** ARCHITECT: "and also implement for windows. runtime test still need to be tested when i get to windows. add to DEBT." No Windows build or run has tested this code.
**Expectation:** On Windows, build, then run `fzf --preview 'eve preview {}'` with PNG, JPEG and GIF, from MSYS2 zsh and from PowerShell, and confirm the image shows at the preview box and a text file shows none.

---




