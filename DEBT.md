# DEBT.md

**Purpose:** Inter-sprint ledger of debts — bugs, nitpicks, friction observed during usage. Drained by sprints via `/pay` (COUNSELOR planning) and `/log` (hygiene drain). **JRENG = paid in full, cash. No triage.**

**Format:** Each entry uses **O / D / E** articulation — Observation, Divergence, Expectation. IDs are UTC timestamps (`DEBT-YYYYMMDDTHHMMSS`). Newest entries at top. Add via `carol debt add`.

**Lifecycle:** Created lazily on first `carol debt add`. Entries appended via interactive prompt. Entries removed by `carol debt clear <id>` (called by `/log` hygiene step after SPRINT-LOG receipt is written). Survives `carol reset` — debts persist across protocol resets.

---

## DEBT-20261103T235332

**Observation:** Old endless and new END share one product name and one bundle id: `PRODUCT_NAME END` and `com.JRENG.END` (`endless/CMakeLists.txt:83`, `:87`; `end/CMakeLists.txt:232`, `:239`). `/Applications/END.app` holds the old binary. ARCHITECT: *"i could just drop old end entirely, probably just kept the binary at endless repo"*.
**Divergence:** A Release `END.app` copied to `/Applications` replaces the old binary, and the fallback is lost unless the old app has moved first. Old endless also uses `~/.config/end` (`endless/Source/AppModel.h:39`).
**Expectation:** The old `END.app` lives in the endless repo before the first Release install of new END. ARCHITECT runs the Release END from `/Applications` as the daily driver.

---

## DEBT-20261103T235331

**Observation:** Every END build and every EVE build use one config root, `~/.config/end`: `ConfigDirectory.h:75` for END and `eve/cast/files.md:31` for EVE. Every END path derives from that root (`Nexus.cpp:84`, `:163`, `Main.cpp:254`). ARCHITECT: *"at least i could build release, running it from Applications/"*; *"then we route dev-end-eve to ~/.config/dev/ ?"*.
**Divergence:** A dev END offloads its plugins over the files of the daily END (`Nexus.cpp:161-188`). Dev and daily EVE read one `eve.md`, which hot-reloads (`SPEC.md:533`), so a table change in a dev build shows errors in the daily EVE.
**Expectation:** A dev build of END and EVE uses `~/.config/dev/`, and a Release build uses `~/.config/end/`, with one switch at both sites. The daily driver stays apart from the build under development.

---

## DEBT-20261103T233912

**Observation:** Claude Code 2.1.317 contains these sequences in its binary (`~/.local/share/claude/versions/2.1.317`), and the live decoder has no handler for them: `CSI ? 2026 h` and `l` (synchronized output), `OSC 52 ; c` (clipboard), `OSC 11 ; ?` (background query), `OSC 0` (title), `OSC 8` (hyperlink), and the `XTVERSION` query. A string in the binary shows that Claude Code can send the sequence. It does not show that each run sends it.
**Divergence:** `map::privateModes` has no `syncOutput` row (`jam/cast/terminal.md:1286-1297`), and `map::oscFunctions` holds only `shellIntegration` and `iterm2Image` (`:1313-1314`), so each of these OSC codes reaches the no-op handler (`jam_TerminalProcessor.cpp:312`, `cast/terminal.md:1012`). `TerminalProcessor::applyCsiSequence` drops a `CSI >` sequence (`jam_TerminalProcessor.cpp:299`), so `XTVERSION` gets no reply. `AnsiDocument` handles `OSC 8` (`jam_AnsiDocument.cpp:530`), but the live table does not. No error shows: frames are not batched, a clipboard write does nothing, the background query gets no reply, the title does not change, and a link is not live. EVE `SPEC.md` Success Criteria: *"Run vim, fzf, lazygit, tmux inside EVE without visual defects"*.
**Expectation:** The live decoder handles every sequence in the Observation through its dispatch tables (`cast/terminal.md` `## privateModes`, `## oscFunctions`), per the terminal spec. Claude Code runs in EVE with its frames batched, its clipboard write delivered, its background query answered, its title shown and its links live. No sequence reaches a no-op handler.

---


