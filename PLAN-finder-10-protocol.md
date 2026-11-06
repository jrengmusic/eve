# PLAN: Finder Part 10 — Finder over EVE's Protocol

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 48, 49

## Overview

Any program can open Finder at the caret with its own list, and receive the selection back. nvim uses this with no knowledge in EVE of nvim. This part adds the program source. The opening already exists through `eve command <name>`.

## Language / Framework Constraints

- *"One envelope, one path"* (`SPEC.md:133-138`).
- *"The GUI writes to the terminal through the input lane only"* (`SPEC.md:186`).
- nvim configuration is outside this repo. MACHINIST owns `~/.config/nvim`. Replacing snacks there is a later request to MACHINIST.

## Dependency & API Inventory

- Envelope: `ESC ] 1337 ; EVE ; <operation> ; <key=value;…> [ : <payload> ] BEL|ST` (`SPEC.md:136`).
- Dispatch: `eveOperations` lookup (`jam_TerminalProcessor.cpp:1114-1118`, `:1141`), `eveConstruction` (`:1215-1235`), the `map::EveOperation` table (`jam/generated/jam_Terminal.h:1822-1828`). `getArgumentValue` (`:1062-1073`).
- CLI: `Source/shell/eve` (`:17-23`), `powershell_integration.ps1:45-51`. Shell integration puts it on `PATH` (`SPEC.md:144-145`).
- Commands: `commands` lane, `applyCommands`, `invokeCommand` (`SPEC.md:151-153`, `EVETerminalProcessor.cpp:128-155`, `EVEView.cpp:729-738`).
- Preview path is a Model row: `events.get (Id::preview, …)` (`jam_TerminalProcessor.cpp:1225-1233`, `EVETerminalProcessor.cpp:114-115`).
- Reply: `addInput` (`EVETerminalProcessor.cpp:184-187`). nvim gets `TermResponse` with `ev.data.sequence`, and sends with `vim.api.nvim_ui_send` (`neovim/runtime/doc/autocmd.txt:1327-1354`).
- File as metadata: *"why not just the file as metadata?"* (`SPEC.md:156-157`).

**New names:** the operation `finder`, sibling of `command` and `preview`. The row `finderList`, a path, sibling of `preview`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: The operation
**Scope:** `jam/cast/terminal.md`, `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`, `eve/Source/EVETerminalProcessor.cpp`, `eve/Source/layout/parameters.md`
**Action:** Add `finder` to `map::EveOperation` and to `eveConstruction`. It reads `path=` and stores it in the Model row `finderList` through `events`, the form of `preview`.
**Validation:** One envelope. The tick stores a path and no list.

### Step 2: The program source
**Scope:** `eve/Source/EVEView.cpp`
**Action:** The event `Id::finderList` writes `program` to the Finder source row. The source of Part 9, Step 1 reads the file with `juce::MemoryMappedFile` and `MarkdownDocument::parse`, as `EVEView.cpp:593-599` does. One table gives one row per item, with free columns such as `path`, `line`, `label`, read with `getTableRows` and `getTableValueView` (`jam_ViewManager.h:262`, `jam_ViewManager.cpp:262-264`).
**Validation:** Ruling 48, and ARCHITECT's choice *"Markdown table"*. A list file holds the rows, so no list bytes cross the terminal (`SPEC.md:156-160`).

### Step 3: The reply
**Scope:** `eve/Source/EVEView.cpp`, `eve/Source/EVETerminalProcessor.*`
**Action:**
1. `Enter` runs the `open` command of `FinderCommandId`, and it writes the selected row to the `Id::chosen` row of the `finder` state.
2. The source table of Part 9, Step 1 holds one registered reaction per source. For `program`, the reaction writes `ESC ] 1337 ; EVE ; finder ; <column=value;…> BEL` to the input lane with `addInput`, with the selected row's columns, encoded as `_eve_encode` encodes (`Source/shell/eve:4-15`).
3. A dismiss with a `program` source runs the same reaction with no columns, so a waiting program ends.

**Validation:** ARCHITECT: *"use Model as parameter, event driven registrable callback, with command manager, no handrolling anything"*. `SPEC.md:184-186`: *"still under UNIDIRECTIONAL contract"*. The program reads a reply as it reads any terminal report. ARCHITECT selected *"Both"*.

### Step 4: The CLI
**Scope:** `eve/Source/shell/eve`, `powershell_integration.ps1`, `README.md`
**Action:** `eve finder <list file>` emits the operation, in the form of `preview` (`eve:20`, `powershell_integration.ps1:51`). Then it waits for the reply on the terminal and prints its columns to stdout, so `f=$(eve finder list.md)` works. nvim does not use the script: it sends with `nvim_ui_send` and listens on `TermResponse`.
**Validation:** Each shell has the same word. The blocking read form per shell comes from the @Librarian report (Risks).

### Step 5: Hand over, audit, doxygen
**Action:** As Part 1, Step 8.

## Acceptance (ARCHITECT builds and tests)

1. `eve finder list.txt` at a prompt: Finder lists the rows, and `Enter` returns the selection.
2. From nvim, with a test mapping: Finder opens at nvim's caret, the selection arrives in `TermResponse`, and `:edit` opens it.
3. Under END: the same, in the focused pane.

## BLESSED Alignment

- **Bound:** the source fills Model rows. The program owns its list file.
- **Lean:** one operation, one source.
- **Explicit:** the operation names its one argument.
- **SSOT:** one envelope.
- **Stateless:** EVE holds no program state.
- **Encapsulation:** nothing in EVE names nvim.
- **Deterministic:** the same list gives the same rows.

## Risks / Open Questions

- **Blocking read.** The docs give the parts: bash and zsh `read -d`, `-t`, `-u`, `-s`, and `stty -g`, `-icanon`, `-echo`, `min`, `time` (GNU Bash manual, zsh *Shell Builtin Commands*, `man stty`). zsh states that `read -t` *"will not read anything until an entire line has been typed"* in canonical mode. No doc gives the combined recipe. COUNSELOR proves the form with a pty run before delegating Step 4.
- **PowerShell.** The docs do not say if `RawUI.ReadKey` reports the bytes of an OSC reply, or how `[Console]::In` buffers on Windows. Only a Windows run settles it, so the PowerShell half joins the Windows debt (`DEBT-20261105T000000`) when ARCHITECT adds it.
