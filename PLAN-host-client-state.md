# PLAN: Host Client State Seam

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-06
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM. One object, one translation unit.

## Overview

All EVE state becomes Model rows, except `bypass`. END reaches EVE through one seam that works in every format. `commandManager` is the one registry of operations in each binary. `exit` closes the pane in END and quits the application in standalone.

ARCHITECT's rulings for this plan:

- *"there's no \"report\" it's all terminal state. a Model parameter. its state changes, terminal command is the sole author. Listener react, always UNIDIRECTIONAL, always SSOT, no copy, no shadow state, there's no such thing as `report`"*
- *"possibly except BYPASS, all parameters now could be just Model parameter, we dont fight audio plugin architecture, instead we integrate seamlessly, the legit path. no manual pointer casting, no poking ABI, no manual async"*
- *"commandManager is the SSOT of `operation`. terminal state machine driven by command, IT IS `operation`, it's not report. it's a state that the ORCHESTRATOR layer could listen and react to TELL what should be happen as callback"*
- *"must be cross format. hence this is END's contract as host to have ONE EXACT SEAM to handle all format, the only exceptions when there's a format that legitimately could not adhere to design, then ENAX is the path"*
- *"`exit` should close END's pane (exit eve)"*; *"standalone eve should exit, close the app. why cant this be SSOT?"*
- *"END is the host. it holds its plugin instance stateInfo."*
- WHELMED: *"Exclude WHELMED"*. WHELMED stops building until a later sprint moves its `zoom` row.
- *"no audit"*. This plan has no Auditor step.
- *"this is smell. no fucking applyGesture at resized(). never call atomic on message thread. why branching on message thread. this is EVE's CONTRACT. not PluginEditor."*

Out of scope, unchanged: the ENAX `engine` rule with its peer, key and mouse hooks (`jam_ENAXClientExtensions.cpp:15-37`), the `keys` rule (`:39-45`), the profile rules (`:84-106`). ARCHITECT has not ruled on them.

## Language / Framework Constraints

- Generated files are CAST output. Engineer edits `cast/*.md`, `parameters.md`, never `generated/*`.
- No comment and no doxygen before Step 13 (CAROL.md Code Hygiene).
- No build by an agent. ARCHITECT runs `cast` and tests.
- Delete first (CAROL.md Refactor-Rewrite). The tree does not compile between steps. The compiler lists what remains.

## Dependency & API Inventory

**JUCE**
- `updateHostDisplay` calls each listener's `audioProcessorChanged` (`juce_AudioProcessor.cpp:431-436`).
- `juce::AudioProcessorListener::ChangeDetails` names no row (`juce_AudioProcessorListener.h:76-82`).
- VST2 client needs `parameterInfoChanged` (`juce_audio_plugin_client_VST2.cpp:1217-1218`). AU client needs `parameterInfoChanged` (`juce_audio_plugin_client_AU_1.mm:2039-2041`). VST3 client needs `nonParameterStateChanged` (`juce_audio_plugin_client_VST3.cpp:1560-1561`, `:1623-1624`).
- Raw state: VST2 `getVSTClient()->getChunkData` and `setChunkData` (`juce_AudioPluginExtensions.h:120-123`). VST3 `getXmlFromBinary`, child `IComponent`, base64 (`juce_VST3PluginFormatImpl.h:2814-2819`, `:3021-3022`). AU plist key `JUCE_STATE_DICTIONARY_KEY` (`juce_audio_plugin_client_AU_1.mm:856-861`). CLAP raw (`jam_ClapPluginInstance.cpp:1094-1108`).
- `ValueTree::readFromData` reads one tree and interns names in the reader's binary (`juce_ValueTree.cpp:1047-1095`).
- `ValueTree::copyPropertiesFrom` sends one change per changed row (`juce_ValueTree.cpp:144-145`, `:201-208`), and it removes absent rows (`:203-205`). Listeners of a parent tree receive child changes (`:107`).
- `ApplicationCommandTarget::invokeDirectly (id, true)` posts the command and holds the target by `WeakReference` (`juce_ApplicationCommandTarget.cpp:38-50`, `:70`). After `perform` it touches no member (`:74-75`).
- `StandardApplicationCommandIDs::quit` runs `systemRequestedQuit` (`juce_Application.cpp:85-87`). With no application object, the manager finds no target (`juce_ApplicationCommandManager.cpp:231-235`).
- `AudioProcessorGraph` prepares a node in its rebuild (`juce_AudioProcessorGraph.cpp:483`), so END writes state before `graph.addNode` (`Session.cpp:112`).

**JAM, used as is**
- `jam::Model`: `storeValue` (`jam_Model.h:1005`), `flush` (`:929`), `getParameter` (`:980`), `addListener` (`:823`), `ParameterAttachment` (`:246`).
- `jam::ModelLayout::addParameters` type table (`jam_ModelLayout.cpp:13-27`). It has no float row.
- `jam::MatrixComponent::childRemoved` handles an unfocused pane (`jam_MatrixComponent.cpp:431`).

**Established patterns**
- Model row events: `terminalEvents` in `EVEView` (`EVEView.cpp:135-138`).
- Command dispatch by name: `PluginEditor::perform` (`jam_PluginEditor.cpp:101-110`), `invokeDirectly` by name (`jam_TextEditor.cpp:1539-1540`).
- A dispatch table is a plain noun (NAMES Rule 8).

## Validation Gate

Each step is validated by COUNSELOR before the next step, against MANIFESTO.md, NAMES.md, CODING.md and this plan. No Auditor runs. ARCHITECT: *"no audit"*.

## Steps

### Step 0: Copy the plan
**Scope:** `eve/PLAN-host-client-state.md`.
**Action:** COUNSELOR copies this file to the project root.
**Validation:** Content equals this plan.

### Step 1: Delete the report path
**Scope:** `eve/Source/EVEView.*`, `eve/Source/EVETerminalProcessor.cpp`, `eve/Source/EVEProcessor.h`, `jam/jam_plugin_bootstrap/enax/jam_ENAXClientExtensions.*`, `jam/jam_terminal/processor/jam_TerminalProcessor.*`, `end/Source/plugins/Nexus.*`, `end/Source/plugins/Session.*`, `end/Source/ui/ENDView.*`, `end/Source/ui/EditorView.*`, `end/Source/ui/SessionView.cpp`, `end/cast/identifiers.md`.
**Action:**
1. EVE: delete `applyTerminalReport`, its `getReports` loop, and the `set (Id::startDirectory, ...)` call.
2. JAM: delete the `terminalReport`, `startDirectory` and `size` rules, the `enax` report, `paneUuid`, the `enaxPaneUuid` read, and `TerminalProcessor::getReports`.
3. END: delete `terminalReportCallback`, `startDirectoryGetter`, `paneUuidGetter`, `displayBoundsGetter`, their `extensions` rows, `getLoadingPane`, `loadingPanes`, `registerTerminalActions`, `getPaneRow`, `getParameter`, `setParameter`, `setSizeParameters`, `isENAX`, the `AsyncUpdater` base and `handleAsyncUpdate` of `EditorView`, the `removePane` action and identifier row, and the `Id::enax` property.
**Validation:** No use of a deleted name remains in the read files. `engine` and `keys` rules are unchanged.

### Step 2: Float row in the Model table
**Scope:** `jam/jam_plugin_bootstrap/parameter/jam_ModelLayout.cpp`.
**Action:** Add a `Id::floatingPoint` entry beside `Id::integer` that builds `jam::Parameter<float>`, the shape of the `Id::int64` entry (`:25-27`).
**Validation:** One table row. No branch.

### Step 3: EVE parameters become Model rows
**Scope:** `eve/Source/layout/parameters.md`, `eve/Source/layout/default-eve.md`, `eve/Source/EVEProcessor.h`.
**Action:**
1. Move `zoom` (float, default 1), `width`, `height`, `mode` (default 2) and `focus` from `## parameter` to `## model`. `bypass` stays.
2. Remove the `ranges` argument at `EVEProcessor.h:106-112`.
3. Delete the `scaling` config row (`default-eve.md:23`). EVE has one scaling contract, so the branch goes.
**Validation:** `## parameter` holds `bypass` only. Table widths are kept.

### Step 4: EVE's contract leaves PluginEditor
**Scope:** `jam/jam_plugin_bootstrap/view/jam_PluginEditor.*`, `jam/cast/plugin_bootstrap.md`, `jam/jam_data_structures/model/jam_AudioModel.cpp`, `eve/Source/EVEView.*`, `eve/cast/identifiers.md`.
**Cause:** ARCHITECT: *"CommandManager is EVE's, NOT PLUGIN EDITOR"*; *"PluginEditor is fucking bloated because EVE."*
**Action:**
1. JAM, delete from `PluginEditor`: the `commandManager` member (`jam_PluginEditor.h:250`), `addCommands`, `getDefaultKeys`, `getViewDefaultKeys`, `getKeysSheet`, the `juce::ApplicationCommandTarget` overrides (`:108-134`), `pluginEditorCommandId`, `isZoomScaling`, `getZoomSize`, `setZoom`, the zoom branch of `resized`, and the `zoom`, `width`, `height`, `focus`, `zoomIn`, `zoomOut`, `zoomReset` callbacks. `resized` keeps the view bounds line.
2. JAM cast: delete `map::PluginEditorCommandId`, `pluginEditorKeys`, and the `zoom`, `zoomIn`, `zoomOut`, `zoomReset`, `zoomStep` identifiers (`plugin_bootstrap.md:74-78`, `:199-209`). `settings` stays a callback.
3. JAM: make `setScaling` virtual. Replace `getRawParameterValue (parameterID)->load()` at `jam_AudioModel.cpp:440` with the form at `:426-427`.
4. EVE: `EVEView` owns `jam::SharedInstance<jam::CommandManager> commandManager`, the key sheet, and the command target overrides. `addCommands` moves here, runs after `initialise`, and runs again in an override of `fileChanged`.
5. EVE cast: add `zoomIn`, `zoomOut`, `zoomReset`, `settings` to `map::CommandId`, in the row form of `cast/identifiers.md:69-75`, and the identifiers `zoom`, `zoomStep`.
6. `EVEView::setScaling`: `setResizable (true, false)`, then `setSize` from the `width` and `height` rows.
7. `EVEView::resized` does not call the base. It reads `zoom` with `getTerminalState().getProperty`, sets `view` bounds with `ViewEditor::getViewBounds` and the scale transform. It writes no row. Its coalesce and panel lines stay.
8. `EVEView::resizeEnd` writes the `width` and `height` rows. This is the final stop (`EVEView.cpp:95-99`), so `resized` has no gesture.
9. `zoomIn`, `zoomOut`, `zoomReset` are `registry->callbacks` rows in the shape of `clear` (`EVEView.cpp:343-347`). Each writes the `zoom` row with `setProperty`, the shape of `Id::background` (`:54`), limited by `static constexpr float` constants in `EVEView.cpp`.
10. `terminalEvents` gets `Id::zoom`, which calls `resized`, and `Id::focus`, which grabs or gives away keyboard focus.
**Validation:** No atomic read on the message thread in `PluginEditor` or `EVEView`. No `setValueNotifyingHost` in `resized`. No scaling branch. `PluginEditor` holds no command, key, zoom or focus name.

### Step 5: Editor mode is a Model row
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`.
**Action:** Delete `modeAttachment`. The `terminal` property table gets `Id::mode`, which calls `setMode` with the row value. `setMode` from a key writes the row, so the row is the one truth.
**Validation:** One writer path. `Model::ValueAttachment` has no use left in `jam_TextEditor`.

### Step 6: EVE tells the host, and takes rows
**Scope:** `eve/Source/EVEProcessor.*`, `eve/Source/EVETerminalProcessor.*`.
**Action:**
1. `EVEProcessor` listens to the Model tree. For `title`, `foregroundProcess`, `cwd`, `zoom` and `shellExited`, it calls `updateHostDisplay (ChangeDetails{}.withParameterInfoChanged (true).withNonParameterStateChanged (true))`. The five ids are a local `static const std::array` named `hostRows`.
2. `setStateInformation`: a tree of type `Format::toType (Id::terminal)` sets each of its properties on the Model and does not restart. Any other tree keeps the restore path (`EVEProcessor.cpp:160-166`).
3. `openTTY` starts in the `cwd` row. An empty row keeps `ShellIntegration::getStartDirectory()`.
**Validation:** A live tell has no clock restart. Each row keeps one author: END sends only `zoom`, `mode`, `focus` and the first `cwd`. No lock on the clock.

### Step 7: Exit in EVE
**Scope:** `eve/Source/EVEView.cpp`.
**Action:** The `shellExited` event shows the overlay, then calls `commandManager->invokeDirectly (juce::StandardApplicationCommandIDs::quit, true)`.
**Validation:** No branch on the host. In a plugin the manager finds no target (`juce_ApplicationCommandManager.cpp:231-235`).

### Step 8: END seam
**Scope:** `end/Source/plugins/PluginState.h`, `.cpp` (new), `end/Source/plugins/Session.*`.
**Action:**
1. New class `PluginState`, private `juce::AudioProcessorListener`. It has one responsibility: the state of one hosted instance. The constructor takes the instance and the pane row, adds the listener, and selects one entry of a `static const` table `stateFormats`, keyed by `getPluginDescription().pluginFormatName`. Each entry holds a read and a write function for raw state: VST2, VST3, AU, CLAP.
2. `audioProcessorChanged`: read the raw state, `ValueTree::readFromData`, then `copyPropertiesFrom` from its `terminal` child to the `terminal` child of the pane row.
3. Public `void setRows (const juce::ValueTree& rows)` writes a `terminal` tree through the write function.
4. `Session` owns `jam::HashMap<jam::UUID, std::unique_ptr<PluginState>>`. `addPlugin` builds the entry and writes `cwd` before `graph.addNode`. `removePlugin` erases the entry before `graph.removeNode`.
5. The seam applies only to a plugin whose `manufacturerName` matches `ProjectInfo::companyName`, compared with `juce::String::compare`.
**Validation:** One branch, at discovery. No format name outside `stateFormats`. RAII removes the listener.

### Step 9: END panes read the held state
**Scope:** `end/Source/ui/EditorView.*`, `end/Source/ui/SessionView.cpp`, `end/Source/ui/ENDView.cpp`.
**Action:**
1. `SessionView` and `ENDView` read `title`, `foregroundProcess` and `cwd` from the `terminal` child of the pane row.
2. `EditorView::setZoom`, `setMode` and the `focus` write call `PluginState::setRows`.
**Validation:** No terminal row sits directly on the pane row. `width` and `height` have no END writer, because `setEditorBounds` already sizes the editor (`EditorView.cpp:89-90`) and `EVEView::resizeEnd` writes the rows (Step 4).

### Step 10: commandManager is END's one registry
**Scope:** `end/Source/action/ENDActions.*` (delete), `end/Source/ui/ENDView.*`, `TabView.cpp`, `SessionView.cpp`, `EditorView.cpp`, `end/Source/plugins/Nexus.cpp`, `end/Source/Main.*`, `end/cast/identifiers.md`.
**Action:**
1. Delete `ENDActions`. `ENDView::perform` dispatches through a `static const` table `commands`, keyed by `juce::CommandID`.
2. `TabView.cpp:157`, `:279`, `SessionView.cpp:53`, and `ENDView.cpp` run calls become `commandManager->invokeDirectly`.
3. Operations with an argument (`newPane`, `loadPlugin`) become public members of `ENDView` and `TabView`. `newSession` and `closeTab` become members.
4. Delete the `cast/identifiers.md` rows that `map::CommandId` already names.
**Validation:** Each command name has one declaration. `grep ENDActions` in `end/Source` is empty. `end/SPEC.md:315-323` is rewritten in Step 12.

### Step 11: Exit in END
**Scope:** `end/Source/ui/EditorView.*`.
**Action:**
1. `EditorView` becomes a `juce::ApplicationCommandTarget` with one command, `map::CommandId::closePane`.
2. Its row listener takes `Id::shellExited` from the `terminal` child and calls `invokeDirectly (map::CommandId::closePane, true)` on itself.
3. `perform` closes its own pane: with more than one pane, `getSession()`, `TabView::remove`, `removePlugin`. With one pane, `SessionView::remove`.
4. `ENDView::perform` for `closePane` calls `perform` on the focused pane.
**Validation:** No manual async. The pane closes by identity, focused or not. No member is read after the removal.

### Step 11a: Every stray is a row
**Scope:** `jam/cast/terminal.md`, `jam/cast/gui.md`, `jam_TerminalProcessor.*`, `jam_TextEditor.cpp`, `EVETerminalProcessor.*`, `EVEView.cpp`, `parameters.md`.
**Cause:** ARCHITECT: *"everything should be Parameter, no strays ana manual state management allowed"*.
**Action:** The decoder state reports through `applyEvents` as Model rows. `pendingClipboard` and `numYanks` are rows. The `hyperlink` event is deleted.
**Validation:** Each row has one author. No allocation on the terminal thread.

### Step 12: Docs
**Scope:** `eve/SPEC.md`, `end/SPEC.md`, `eve/CLAUDE.md`.
**Action:** COUNSELOR rewrites the rows and lines that this plan changes, with ARCHITECT's quotes. `eve/SPEC.md:236-238`, `:260`, `:266`, `:593-603`, `:727-731`, `:784`. `end/SPEC.md:255-300`, `:315-335`.
**Validation:** Each new line maps to a ruling above or a file:line of changed code.

### Step 13: Doxygen pass
**Scope:** The members Steps 1 to 11 add or change.
**Action:** @Pathfinder writes the pass from exact targets. COUNSELOR reads each block against the code.
**Validation:** CODING.md COMMENTS, `doxygen-writer` skill.

### Step 14: Report
**Action:** COUNSELOR reports changed files and commit messages in chat. Logging starts with ARCHITECT.
**Validation:** File content against each step.

## BLESSED Alignment

- **B:** `Session` owns `PluginState`. RAII removes the listener.
- **L:** `stateFormats` and `commands` are lookups. `ENDActions` is removed.
- **E:** One seam. END holds no ABI read for state.
- **S (SSOT):** One command registry per binary. One row table in EVE. END holds the state of its plugin, as ruled, and replaces it from the plugin.
- **S (Stateless):** `EditorView` and `SessionView` read the row.
- **E (Encapsulation):** Data moves one way per row. Each row has one author.
- **D:** The same row change gives the same event in each format.

## Verification (ARCHITECT)

1. Run `cast` in `jam`, `eve`, `end`, then build EVE and END. Build `EVE.vst` first.
2. `exit` in a focused pane, then in an unfocused pane: that pane closes. Last pane: the tab closes. Last tab: END quits.
3. `exit` in standalone EVE: the application quits with its state saved.
4. Tab name follows `title`, then `foregroundProcess`, then `cwd`. A split starts in the inherited `cwd`.
5. Zoom, mode and focus under END still work. Resize a pane: no shell restart, no lost history.
6. Load EVE as VST3, then as AU, and repeat 2 and 4.

## Risks / Open Questions

None open. A VST3 or AU live tell rebuilds its wrapper on each `zoom`, `mode` or `focus` change. Those rows change on user action only, not per frame.
