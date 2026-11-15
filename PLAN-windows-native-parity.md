# PLAN: windows-native-parity

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-16
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 / JAM (CODING.md, MANIFESTO.md, NAMES.md)

## Context

ARCHITECT's objectives, in order:
1. *"fix eve first, fix the rendering"*
2. *"eve standalone close must also instant and CLEAN"*
3. *"then fix end, load eve. each instance load and close at END is FUCKING UNBEARABLY SLOW"*
4. *"end focus and keyboard management is fucking garbage, cant type … ON MAC THIS IS WORKING"*

ARCHITECT rulings that bind this plan:
- *"rendering is one SSOT always, divergence only at native OS"*
- *"dont fucking hack windows then it might break mac"*
- *"no fucking new semantics. no new pattern. no fucking plugin state poking layer. no manual state management. no hack. no workaround. no patch. no fixing symptoms. no handroll anything"*
- The reference for the Windows ConPTY layer is `~/Documents/Poems/dev/terminal/` (Microsoft Terminal). The JAM file names that code as its model (`jam_TerminalTTY_windows.cpp:7`, `:60`).

Evidence found before this plan:
- **Shell output bypasses ConPTY.** In a whatdbg trace of the Debug standalone, zsh wrote its stdout and stderr to EVE's own stdout. The `ls` listing, every `zsh: command not found`, and the shell-integration marks `OSC 133` and `OSC 7` went there, not to the decoder. Only zle's prompt and echo reached the screen. ARCHITECT saw the same: *"type ls enter. NOTHING shown"*.
- **Cause.** `spawnProcess` sets no `dwFlags` on `STARTUPINFOEXW` (`jam_TerminalTTY_windows.cpp:561-562`). The reference sets `siEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES;` and leaves the three `hStd*` members null (`terminal/src/cascadia/TerminalConnection/ConptyConnection.cpp:34-36`). Microsoft: *"If the existing value of the standard handle is NULL … the handle is replaced with a console handle"* (`getstdhandle.md:100`). So the child gets the pseudoconsole as its stdio. Without the flag, the child keeps the std handles of EVE. macOS cannot diverge here: `forkpty` makes the pty the child's stdio.
- **Focus under END.** END hosts EVE on Windows as VST2: the DLL exports only `VSTPluginMain`. The JUCE VST2 host window forwards native focus only on macOS. The jam patch `juce-plugin-window-focus-hook.patch` adds `setWantsKeyboardFocus (true)` and a `focusGained` that calls `makeFirstResponder` inside `#if JUCE_MAC` (`juce_VSTPluginFormat.cpp:75-91`, `:630-643`). The Windows branch has no `SetFocus` (`juce_VSTPluginFormat.cpp:424-443`). END's `EditorView` makes no focus call (`EditorView.cpp`, full read). The pane `Id::focus` row is written by its owner (`jam_OwnerComponent.cpp:112-121`). Today's change removed the only path that turned the pane focus into keyboard focus (the HEAD `Session` `Id::focus` event into `PluginState`). ARCHITECT ruled that path out.

## Language / Framework Constraints

- Native divergence only, in the JAM Windows TTY. Shared rendering, decoder, EVE and END code stay unchanged.
- Builds run only through `cast`. ARCHITECT: *"build permission granted, but not to be abused for code validation. build only to ensure runtime running as expected. Pathfinder can build and send keyboard input EVE, capture the visual whatever. use build sparingly, EFFICIENTLY."*

## Dependency & API Inventory

- Windows API: `STARTUPINFOEXW`, `STARTF_USESTDHANDLES`, `PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE`, `CreateProcessW` — as in the reference (`ConptyConnection.cpp:34-36`, `:166-177`).
- Runtime check: PowerShell `Start-Process -PassThru`, `WScript.Shell.AppActivate` / `SendKeys` (proven in this session: `activated=True`), `System.Drawing` screen copy, `Process.CloseMainWindow()`.
- The END focus facts in Context are carried to the next plan (ARCHITECT: *"then we continue to end"*).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

## Steps

### Step 0: Write the plan to the EVE project root
**Scope:** `PLAN-windows-native-parity.md`
**Action:** COUNSELOR copies this plan to the EVE project root.
**Validation:** file content equals the approved plan.

### Step 1: The shell's stdio is the pseudoconsole
**Scope:** JAM `jam_terminal/tty/native/jam_TerminalTTY_windows.cpp` (`spawnProcess`, `:561-562`)
**Action:** Set `si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;` after `si.StartupInfo.cb`. Leave `hStdInput`, `hStdOutput`, `hStdError` null and `bInheritHandles` `FALSE`, as the reference does (`ConptyConnection.cpp:34-36`, `:171`). No other change.
**Validation:** the line matches the reference; no new name; no other line changes; CODING brace init and spacing.

### Step 2: One Debug build of EVE
**Scope:** EVE `Builds/Debug`
**Action:** Pathfinder runs `cast cast/spell.md --debug` in the EVE root, once. ARCHITECT: *"use build sparingly, EFFICIENTLY"*; the build exists only to check the runtime.
**Validation:** exit 0. A compile error goes back to Step 1 through Engineer; no second build without a code change.

### Step 3: Runtime check by Pathfinder, with a screen capture
**Scope:** `Builds/Debug/EVE_artefacts/Debug/Standalone/EVE.exe`; scratchpad
**Action:** Pathfinder runs one PowerShell script in the foreground that COUNSELOR writes to the scratchpad. The script starts EVE with `Start-Process -PassThru` (its handle stops it), waits for the prompt, activates the window, sends `abcdefgh`, captures the window, sends `{BACKSPACE}` × 8, `ls{ENTER}`, captures the window again, then closes the window with `CloseMainWindow()` and measures the time to process exit. A process that is still alive after the wait is stopped through its handle and reported. Pathfinder returns the two PNG paths, the close time, and `tasklist` for `EVE.exe`, `OpenConsole.exe`, `zsh.exe`.
**Validation:** COUNSELOR reads both PNGs. Pass: the `ls` listing is on screen; the typed text has no underline. The close time and the leftover process list go to ARCHITECT.

### Step 4: ARCHITECT builds and tests
**Scope:** EVE standalone
**Action:** ARCHITECT builds and tests the runtime. ARCHITECT: *"ARCHITECT will build and test runtime when you have fix it, then we continue to end, and speed optimization"*.
**Validation:** ARCHITECT's report. END (objectives 3 and 4) and the speed work follow in the next plan, from the facts in Context and from ARCHITECT's report.

## BLESSED Alignment
- B: the child's stdio has one owner, the pseudoconsole. The check script stops only the process it started, through its own handle.
- L: one flag line.
- E: the change is in the Windows-native file only.
- S (SSOT): rendering and the decoder stay shared; the reference is the single source for the ConPTY launch.
- S (Stateless): no new member, no flag, no tracked state.
- E (Encapsulation): no EVE, END or shared-layer code changes.
- D: shell output reaches the decoder on every launch path, as on macOS.

## Risks / Open Questions
- None.
