# PLAN: eve-plugin-contract

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-15
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 / JAM (CODING.md, MANIFESTO.md, NAMES.md)

## Overview

Restore EVE's audio-plugin contract so each EVE instance loads in subseconds, standalone and under END.
The host is the sole author of size. Size reaches the chain through the existing event lane. The chain
follows the JUCE driver contract on `jam::VirtualDevice`, its audio driver. The saved state is the whole
model. Then measure with scout.

ARCHITECT rulings carried:
- *"END as whole application including ALL its plugin instance … session preserverance is the CONTRACT"*
- *"then just reset at sigwinch? winsize changed, prepare whatever"*
- *"VirtualDevice is our audio drivers"*
- *"sole author of winsize is WINDOW … at END as the host, sole autor is END"*; under END: *"END"*
- *"winsize can saved to settings too"*
- *"process always lock free"*; *"EVENT DRIVEN. no manual callback, no manual state management"*
- *"new pattern is not allowed"*

## Language / Framework Constraints

- MANIFESTO E, deadline threads: the clock tick (`process`) allocates nothing, locks nothing, waits on nothing.
- CODING: no bail-out guards, no manual RAII cleanup, framework API first (JAM, then JUCE).
- SPEC.md Architecture Constraints (`SPEC.md:949-968`): no clock start outside the host's
  `prepareToPlay` / `releaseResources` pair; no lock on the clock.

## Dependency & API Inventory

- JUCE driver contract (proven pattern, already used by the chain):
  - Device setup change order: `stop` → `open` → `start` (`juce_AudioDeviceManager.cpp:821`, `:889`, `:898`).
  - `audioDeviceAboutToStart (device)` prepares before callbacks begin (`juce_AudioIODevice.h:119-133`;
    `AudioProcessorPlayer.cpp:344-373`). `audioDeviceStopped` releases (`AudioProcessorPlayer.cpp:375-384`).
- `jam::VirtualDevice` (`jam_VirtualDevice.h`): `open`, `close`, `start (callback)` → `audioDeviceAboutToStart`
  (`jam_VirtualDevice.cpp:37-47`), `stop` → `audioDeviceStopped` (`:49-60`), `requestProcess`, `clock`.
- EVE event lane: `EVEView::terminalEvents` on the `terminal` tree (`EVEView.cpp:138-144`, `:146-239`).
- Decoder state publication: `TerminalProcessor::applyEvents` fires every runtime row from the decoder
  (`jam_TerminalProcessor.cpp:310-346`); `EVETerminalProcessor::registerEvents` stores them (`:139-164`).
- State: `jam::AudioModel` is the APVTS (`jam_AudioModel.h:25`); `copyState` / `replaceState` from JUCE
  (`juce_AudioProcessorValueTreeState.h:391`, `:404`); `AudioModel::setState` (`jam_AudioModel.cpp:181-194`).
- Display config read: `styleManager->getConfig (jam::StyleManager::getDisplaySheet())` (`EVEView.cpp:379`).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs
ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 0: Write the plan to the project root
**Scope:** `PLAN-eve-plugin-contract.md`
**Action:** COUNSELOR copies this plan to the EVE project root.
**Validation:** file content equals the approved plan.

### Step 1: The chain follows the driver contract
**Scope:** `EVETerminalProcessor.h/.cpp`
**Action:**
- Delete `restart` (both overloads) and `winsizeAttachment` (`EVETerminalProcessor.h:59-80`, `:347-348`;
  `.cpp:66-70`, `:402-405`).
- The existing `prepare()` applies a device setup change in the JUCE order: `device.stop()`,
  `device.open (...)`, `device.start (this)`. The start runs `audioDeviceAboutToStart`, which prepares
  at the new size. No `isPlaying()` question. No new function (MANIFESTO S: *"Check if functionality
  already exists before creating new implementations"*).
- `audioDeviceAboutToStart` prepares from the winsize row (`.cpp:407-421`), unchanged in shape.
- `releaseResources` (`.cpp:391-395`) closes what prepare opened, including the TTY. A setup change
  keeps the shell: the TTY gets the new size (`tty->setWinsize`, `SIGWINCH`). ARCHITECT: *"resizing can
  call SIGWINCH, clear live buffer, realloc, then redraw active prompt, whatever it needs to be done as
  tty, history should be UNAFFECTED"* (SPEC.md:661-663).
- `clearHistory` (`.cpp:564-571`) runs its body through the same setup change.
**Validation:** no device setup call (`stop`, `open`, `start`, `close`) outside `prepare`,
`clearHistory`, `releaseResources`; no `isPlaying()` read; tick path (`process`) unchanged and
lock-free; CODING no bail-out.

### Step 2: Size reaches the chain through the processor's own listener
**Scope:** `EVEProcessor.cpp`, `EVEView.cpp`
**Action:** `EVEProcessor::valueTreePropertyChanged` tells `terminalProcessor.prepare()` on `Id::winsize`.
That listener is on `model.state` from the processor's construction (`EVEProcessor.cpp:27`). JUCE
calls the listeners of the changed tree first, then each parent (`juce_ValueTree.cpp:103-107`), so the
chain Model on the `terminal` tree (`jam_Model.cpp:82`) syncs the `winsize` parameter first. EVEView has
no `winsize` entry. ARCHITECT: *"must be UNIDIRECTIONAL, one flow, one path"*; *"why dont you learn
anything from filter-strip?"* (`FilterStripAudioProcessor.cpp:41`, `:155-159`).
**Validation:** one path from the tree to the chain; no attachment; no View-to-Processor call.

### Step 3: The host is the sole author of width and height
**Scope:** `EVEView.cpp/.h`, `EVEStandaloneApp.cpp/.h`, `Source/layout/parameters.md`
**Action:**
- `EVEView::resizeEnd` writes no rows; delete the `isEND()` branch (`EVEView.cpp:107-113`).
- `EVEStandaloneApp::MainWindow::resizeEnd` forwards to the constrainer only. The window sizes its
  content through the JUCE contract; JUCE saves the state. ARCHITECT: *"standalone does NOT need to be
  save anything manually, width/height is in the Model, it saves by juce to settings"*.
- The standalone window's first size comes from the display `size` row (`default-eve.md:29`) when the
  model has no saved `width` and `height`; the `width` and `height` defaults leave `parameters.md:77`, `:79`.
**Validation:** one writer per host; no host-type branch; one default for the initial size (MANIFESTO S).

### Step 4: The saved state is the whole model
**Scope:** `EVEProcessor.cpp/.h`, `EVETerminalProcessor.cpp/.h`
**Action:**
- `getStateInformation` writes `model.copyState()` whole. Delete the `pluginState` copy
  (`EVEProcessor.cpp:166-174`), including the `cwd` / `zoom` loop.
- `setStateInformation`: `suspendProcessing (true)`, `terminalProcessor.setState (state)`,
  `model.setState (state)`, `suspendProcessing (false)`. `EVETerminalProcessor::setState` applies the
  values that the restored `terminal` child holds (`jam::Model::setValuesFrom`) and moves the Model's
  tree into the incoming state.
- `getStateInformation` writes `model.copyState()` without the rows that `parameters.md` marks
  `runtime` and without the `gridStarts` child. ARCHITECT chose *"Filter at save"*, a *"Column in ##
  model"*, named *"runtime"*. The restored `winsize` reaches the chain through Step 2. Delete the
  `restart` wrapper (`:184-196`) and the `pluginState` rows branch. ARCHITECT: *"why we need copy? why
  we need pluginState?"*.
- The processor gives the `width` and `height` default at construction, from the display `size` row.
**Validation:** no property copy; no second tree; no `pluginState`; `setStateInformation` makes no
device call itself.

### Step 5: Prepare resets the runtime rows from the decoder
**Scope:** JAM `jam_terminal/processor/jam_TerminalProcessor.h/.cpp`, `EVETerminalProcessor.cpp`
**Action:** `TerminalProcessor::reset` takes the `events` map and ends with `applyEvents`, the existing
publication (`jam_TerminalProcessor.cpp:20-46`, `:310-346`). `EVETerminalProcessor::prepare` passes
`events` (`EVETerminalProcessor.cpp:541`). A restored runtime row is replaced by the decoder's state at prepare.
**Validation:** no new event key; reuse of `applyEvents` only; no allocation added.

### Step 6: END holds each plugin's whole state
**Scope:** END `plugins/PluginState.cpp/.h`, `ui/EditorView.cpp`, `ui/ENDView.cpp`, `ui/SessionView.cpp`,
`ui/TabView.cpp`, `plugins/Session.cpp`
**Action:** delete `PluginState`, `Session::setRow`, `Session::hasPluginState`, the company check, and
every END read or write of a plugin's rows (`terminal`). END hosts by the JUCE contract only: graph node,
editor, `setEditorBounds`. ARCHITECT: *"PluginState is A FUCKING HACK"*; *"no hardcoding eve into END's
engine"*; *"POKING getState/setState MANUALLY IS NOT ALLOWED. it's ARCHITECTURAL CONTRACT GUARANGEED by
juce."*
**Validation:** END names no plugin and no plugin tree; END makes no manual `getStateInformation` or
`setStateInformation` call; no EVE-only branch in END.

### Step 7: SPEC amendments
**Scope:** `SPEC.md`
**Action:** amend Resize (`SPEC.md:650-701`) and Tree (`:818-825`) to the contract above, quoting the
rulings in the Overview.
**Validation:** SPEC matches the code read after Step 6.

### Step 8: Measure
**Scope:** `benchmark/launch-windows.md`, END `benchmark/pane-windows.md`, `benchmark/vst-profile-build.md`
**Action:** scout EVE baseline (11 epochs); scout `vst-profile-build.md`; scout END pane baseline.
**Validation:** all epochs end; reports written; no EVE or END process left.

## BLESSED Alignment
- B: the device's lifecycle has one owner and one setup-change path; TTY closes in release.
- L: deletion of `restart`, `winsizeAttachment`, the `pluginState` copy and the host-type branch.
- E: size is told through the event lane; no hidden attachment callback.
- S (SSOT): one saved state (the model); one default size (the display row); one writer of width/height per host.
- S (Stateless): no `isPlaying()` question; runtime rows follow the decoder.
- E (Encapsulation): the processor never drives its own restart from a parameter callback.
- D: a relaunch yields the decoder's rows, not stale restored rows.

## Risks / Open Questions
- None.
