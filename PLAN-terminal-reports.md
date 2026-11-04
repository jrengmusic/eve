# PLAN: Terminal Reports

**RFC:** none — objective from ARCHITECT prompt, `/pay` of `DEBT-20261103T233912`, and `PLAN-Grand-Scheme.md`
**Date:** 2026-11-04
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM. No `LANGUAGE.md` adaptation changes a step.

## Overview

EVE reports what its terminal does. The tick decodes each report into metadata and Model rows. The message thread materialises text once. END shows the reports in the tab name and starts each new pane in the cwd of the last focused EVE.

## Rulings

Each decision quotes ARCHITECT.

- Hot path: *"metadata only on hot path always, only materialisation ready to render allocate, on message thread, cached, once as complete AST, 0 rules applied (0 copy, 0 mutation, 0 shadow state, ...)"*.
- Lock free: *"our architecture is audio discipline, where lock-free at processor is non-negotiable"*.
- State: *"all terminal events including OSC 133 and future shell integration is jam::Model parameter"*; *"a new terminal state is a new `model.md` row, never a new channel"* (`PLAN-Grand-Scheme.md` §8, as written there).
- Host: *"parameters.md always for automatable parameter"*. Terminal state is never a host parameter (§8).
- Cwd: *"we must add cwd as EVE parameter, AND END also track last focused EVE (pane). so cwd always synced to focused pane. new instance of tab, split, any pane, always inherit cwd from previous focused pane."*; *"default cwd init still $HOME, or anything defined in config"*; *"cwd is not full path only directory name"*.
- Tab order: *"title, whatever application send"*, then *"application name, if it's not sending anything"*, then *"cwd"*, then *"plugin name"*.
- Ephemeral: *"active prompt, show cwd - ran cast, build running, tab could be rename from cast - cmake - ninja - whatever cli running - build done - back to cwd"*.
- Events: *"terminal report, we capture as parameter no? ... this is terminal protocol, all event driven"*.
- OSC 133: *"we already capture OSC 133 to mark history CMIIW we only need to extend"*.
- Link: *"keep the callback stubbed. what happen if the link clicked, selected with visual, etc etc will need thorough discussion because the design quite complex. so link now only rendering"*.
- Link data: *"An OSC 8 link is data apart from style"* (§3.11).

COUNSELOR reading, not a quote: *"cwd is not full path only directory name"* applies to the tab. The Model row holds the full path, because the shell start needs it (`EVETerminalProcessor.cpp:291`). ARCHITECT did not reject this reading when it was stated.

## Decisions from reads, reported to ARCHITECT

- **Time limit of a hold:** 1000 ms. Ghostty `termio/Thread.zig:38`, iTerm2 `iTermTemporaryDoubleBufferedGridController.m:85`.
- **`FORCE_HYPERLINK` is not set.** Claude Code prints `label (url)` when it sees no link support, and only the label when it does (binary line 51593768). With the click stubbed, the URL would disappear.
- **`OSC 52` read (`?`) gets no reply.** Alacritty denies a read by default (`term/mod.rs:1727-1729`). The debt asks for the write.
- **`OSC 52` handles selection `c` only.** Claude Code sends `c` (binary line 51586492). An empty selection byte maps to `sixel` in `imageFunctions` (`terminal.md:1361`).
- **A child tool shows only if it sends a title.** The app name is the typed command.

## Dependency & API Inventory

- **Dispatch tables:** `oscFunctions` (`terminal.md:1300`), `privateModes` (`:1273`), `shellIntegrationFunctions` (`:1317`), `imageFunctions` (`:1335`), `oscConstruction` (`jam_TerminalProcessor.cpp:819`), `shellIntegrationConstruction` (`:846`), `privateModeConstruction` (`:755`).
- **Reply stream:** preallocated in `prepare` (`jam_TerminalProcessor.cpp:14-15`), written in `process` (`:59-63`). Sibling: `statusReply` (`.h:105`, `.cpp:741-742`).
- **Payload lane:** `imageHeaderConstruction` (`:946`), `imageSequenceConstruction` (`:993`), `imageConstruction` (`:1038`), `payloadDecoding` (`:1080`), `addImageData` (`:202`). Records: `TerminalImageRecord::writeImages`, `getImageData`.
- **Model text:** `ParameterText::store` (`jam_ParameterText.h:52`), `Model::storeValue` (`jam_Model.cpp:396`). Sibling event: `Id::preview` (`EVETerminalProcessor.cpp:112`, `jam_TerminalProcessor.cpp:936-942`).
- **Percent decode:** `applyPercentDecoding` (`jam_TerminalProcessor.cpp:885`), `maxDecodedLength` (`:863`).
- **ENAX:** `Nexus::registerExtensions` (`Nexus.cpp:58-69`), `ENAXClientExtensions::set` (`.h:36-43`), rules (`.cpp`), ids (`jam/cast/core.md:74-80`). Siblings: `manifestFileGetter` (`Nexus.cpp:31`), `promptPaintedCallback` (`:30`).
- **Focus:** `OwnerComponent::valueTreePropertyChanged` gives `Id::focus` to one child chain (`jam_OwnerComponent.cpp:104-113`), so one EVE holds `focus`.
- **Tab name:** `SessionView::getTabName` (`SessionView.cpp:93-107`).
- **Underline paint:** `AttributedChar.underline`, drawn by `GlyphArrangement` (§3.11).
- **Time source:** `juce::Time::getMillisecondCounterHiRes` (`jam_TerminalClock.cpp:21`).
- **Names:** `cwd`, `title`, `foregroundProcess`, `syncOutput` come from endless (`Identifier.h:160`, `Model.cpp:173`, `:183`, `VideoMode.cpp:186`). `hyperlink` is `map::OSC::hyperlink`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step, and only on ARCHITECT's word (*"do not audit until i explicitly say so"*).

Every step: no allocation, lock or wait on the tick. No comment and no doxygen. No hand edit of a generated file: cast tables are the source.

## Steps

### Step 1: Identifiers and Model rows
**Scope:** `jam/cast/terminal.md`, `jam/cast/core.md`, `eve/Source/layout/parameters.md`.
**Action:**
1. Add `@id` rows: `cwd`, `title`, `foregroundProcess`, `syncOutput`, `clipboard`, `background`, `hyperlink`, `terminalReport`.
2. Add `@string` rows `enaxStartDirectory` = `enax.start-directory` and `enaxTerminalReport` = `enax.terminal-report`.
3. Add `## model` rows: `cwd` string 1024, `title` string 1024, `foregroundProcess` string 1024, `syncOutput` bool 0, `clipboard` int64 -1, `background` int64 0.
4. Register events in `EVETerminalProcessor::registerEvents`: `title`, `foregroundProcess` and `cwd` copy the `Id::preview` line. `syncOutput` joins the bool loop at `:93`. `clipboard` is Step 7. `hyperlink` is an event with an empty body: the stub.
**Validation:** rows join their tables. The stub has no body. No host parameter is added.

### Step 2: OSC table rows
**Scope:** `jam/cast/terminal.md`, `jam_TerminalProcessor.h`, `.cpp`.
**Action:**
1. Add `OscFunction` ordinals `cwd`, `title`, `background`, `hyperlink`, and `oscFunctions` rows for `map::OSC::setCwd`, `setWindowTitle`, `setTitleOnly`, `setDefaultBackground`, `hyperlink`.
2. Raise `oscConstruction` to the last ordinal, and add `juce::MemoryOutputStream& reply` to its signature. Pass `reply` through `screenConstruction` (`:553-556`) and `applyOscSequence` (`:307`).
3. Title lambda: fire `Id::title` with the view after the first separator.
4. Cwd lambda: take the path after the host in `file://host/path`, decode it with `applyPercentDecoding`, fire `Id::cwd`.
**Validation:** each new key has one row. Each lambda holds views and one stack array. No `std::string`.

### Step 3: OSC 133 reports
**Scope:** `jam_TerminalProcessor.h`, `.cpp`, `eve/Source/shell/zsh_integration.zsh`, `bash_integration.bash`, `powershell_integration.ps1`.
**Action:**
1. Add `std::string_view` and `const Events&` to `shellIntegrationConstruction` (`.h:155`). Pass them at `.cpp:830`.
2. `outputStart`: keep its body. Read `cmdline_url=` from the arguments, decode it, and fire `Id::foregroundProcess` with its first word.
3. `outputEnd`: keep its body. Fire `Id::foregroundProcess` and `Id::title` empty.
4. zsh: `_eve_preexec` prints `\033]133;C;cmdline_url=%s\007` with `_eve_encode "$1"`. `_eve_precmd` encodes `$PWD` with `_eve_encode`.
5. bash: `PS0` runs one `_eve_preexec` function, which reads `history 1` as kitty does (`kitty.bash:218-230`), and it encodes `$PWD`.
6. PowerShell: @Librarian reads the PSReadLine API for the current buffer first. Then the Enter handler sends the same field. A read that gives no API leaves `OSC 133;C` as it is, and COUNSELOR reports it.
**Validation:** `outputBlock` and `isRecording` still change as before (`:849-852`). No new table or row for C and D.

### Step 4: XTVERSION and DECRQM
**Scope:** `jam_TerminalProcessor.h`, `.cpp`.
**Action:**
1. `maxReplyLength` becomes 25, the `OSC 11` reply.
2. Add `versionReply { "\x1bP>|END\x1b\\" }`, the same name as `TERM_PROGRAM` (`jam_TerminalTTY_posix.cpp:50`).
3. In `applyCsiSequence`: marker `>` with final `q` writes `versionReply`. Every other `>` sequence stays dropped.
4. Marker `?`, intermediate `$`, final `p`: reply `CSI ? Ps ; Pm $ y`. For 2026, `Pm` is 1 during a hold and 2 otherwise (kitty `parser.py:1066-1075`). Any other mode gets 0 (`ctlseqs.txt:1503-1518`).
**Validation:** the `intermediate` filter at `:288` still drops every other sequence. Claude Code asks `DECRQM 2026` only after an XTVERSION reply (binary line 51593982).

### Step 5: Sync hold
**Scope:** `jam/cast/terminal.md`, `jam_TerminalProcessor.h`, `.cpp`, `EVETerminalProcessor.cpp`.
**Action:**
1. Add `PrivateMode::syncOutput`, `DEC` key 2026, and the `privateModes` row. The lambda sets `syncOutput` and `syncStart`.
2. `process()`: while `syncOutput`, and less than `syncTimeoutMilliseconds` (1000) after `syncStart`, skip `applyEvents` (`:73`) and `setLiveLines` (`:77`). The time read runs only while `syncOutput` is on. Past the limit, `syncOutput` goes off.
3. `process()` fires `Id::syncOutput` each call, outside the drained test.
4. The getter at `EVETerminalProcessor.cpp:158` calls `triggerAsyncUpdate` only when the `syncOutput` row is off.
5. Reset turns it off: the `reset` control (`:591-601`) and `TerminalProcessor::reset` (`:18`).
**Validation:** `Id::shellExited` still fires during a hold. A view pull runs `process()` during a hold (`:169`), and `:77` runs with no bytes, so no timer is added. The text path gains no test per byte.

### Step 6: OSC 11
**Scope:** `jam_TerminalProcessor.cpp`, `EVETerminalProcessor.cpp`, `jam_TextEditor.cpp`.
**Action:**
1. `TextEditor::lookAndFeelChanged` writes `findColour (juce::ResizableWindow::backgroundColourId)` to the `background` row, like `Id::winsize` (`:1063`).
2. Add `events.add<juce::uint32&> (Id::background, ...)`. It reads the Model parameter, like `Id::previewImage` (`EVETerminalProcessor.cpp:116`).
3. The lambda answers only `?`. It writes `ESC ] 11 ; rgb:rrrr/gggg/bbbb`, each channel times 257 (`stream_handler.zig:1730-1736`), then the terminator of the query (`osc.zig:259-262`).
**Validation:** the tick reads one atomic. The reply fits `maxReplyLength`.

### Step 7: OSC 52 write
**Scope:** `jam/cast/terminal.md`, `jam_TerminalProcessor.h`, `.cpp`, `EVETerminalProcessor.h`, `.cpp`, `EVEView.cpp`.
**Action:**
1. Add `ImageFunction::clipboard` and the `imageFunctions` row `Chars::lowerC`. Raise each `map::ImageFunction::sixel + 1` array to `clipboard + 1`.
2. `imageHeaderConstruction`: for `map::OSC::setClipboard`, the header is `c;`.
3. `imageSequenceConstruction`: as sixel (`:1031-1032`), with a nonzero pixel size, so `addHeaderByte` stays out (`jam_TerminalITerm2Decoder.cpp:125`). `++imageCount`.
4. `payloadDecoding`: the `file` row. `imageConstruction`: `addImageData` end record, then `events.get (Id::clipboard, juce::int64 { imageCount - 1 })`.
5. The event stores the number in an atomic slot. `handleAsyncUpdate` writes the Model row after `writePendingData`, so the record exists when the view reads it.
6. `EVEView::valueTreePropertyChanged` calls a new `EVETerminalProcessor::copyToClipboard`, which reads `swap.getImageData` and calls `juce::SystemClipboard::copyTextToClipboard`.
**Validation:** a sequence over 64 KiB passes the lane in parts and prints no text (`:124-127` no longer applies to it). The lane back-pressure stays (`:186`).

### Step 8: OSC 8 metadata and render
**Scope:** `jam_AttributedString.h`, `.cpp`, `jam_TerminalProcessor.h`, `.cpp`, `jam_TerminalSwap.h`, `.cpp`.
**Action:**
1. `AttributedString::prepare` reserves `linkRuns` for `numAtoms`. Add `addLinkRun (juce::Range<int> range, int linkNumber)`: it adds a run and touches no string.
2. The hyperlink lambda, on open: when `images` has room for the URI, `addImageData` writes the URI bytes as one record under `imageCount++`, and `activeLink` holds the number. On close or no room, `activeLink` is empty. Fire `Id::hyperlink` with the number.
3. `addTextCell` adds the cell to the run of `activeLink`.
4. `activeLink` stays out of `savedPen`, as in endless (`Video.h:580-581`). Reset clears it.
5. `TerminalSwap::addLine`, on the message thread: each run number gives one `Link`, read once from `getImageData` into `jam::HashMap<juce::int64, AttributedString::Link> links` on the swap. Remove `const` from `addLines`, `addRangeLines`, `addSegmentLines`. An empty record stays out of the map, so the next read takes it.
6. `addLine` then applies underline to the atoms of each run, through a new `AttributedString::applyLinkUnderline`. The tick atoms keep their style.
7. No code goes into `applyMouse`.
**Validation:** the tick holds a number and no string. `getNumBytes` already counts the runs (`jam_TerminalBuffer.cpp:576`). `append` (`jam_AttributedString.cpp:54-58`) and `writeLinks` work, because `links` is complete after `addLine`.

### Step 9: The pane owns its parameters
**Scope:** `end/Source/ui/EditorView.h`, `.cpp`, `end/cast/`.
**Ruling:** *"END is not routing report to its pane, it TELLS its pane, a parameter value, such as cwd, all event driven, drives tab name, set new instance cwd etc. no handroll anything, no manual state machine management, respect the architecture SSOT, possibly registered command"*.
**Action:**
1. The `EditorView` constructor adds three `jam::ParameterText` to its row, the shape of `TabView.cpp:8`: `Id::title`, `Id::foregroundProcess`, `Id::cwd`. None of them is `Id::name`.
2. The default of `Id::cwd` is the `cwd` of the pane that holds `Id::focus` at that time. The new row exists before the split moves focus (`jam_MatrixComponent.cpp:342`). No second store holds it.
**Validation:** the row is the only store of each value. No member copy. No route.

### Step 10: Registered commands tell the pane
**Scope:** `end/Source/ui/ENDView.cpp`, `end/Source/plugins/Nexus.h`, `Nexus.cpp`, `jam_ENAXClientExtensions.cpp`, `EVEView.cpp`, `ShellIntegration.cpp`.
**Action:**
1. `ENDView` registers `actions.add<const char*>` for `Id::title`, `Id::foregroundProcess`, `Id::cwd`, on the `ENDActions` map (`ENDActions.h:30`). Each tells the parameter of the focused pane with `setValue`. One EVE holds `focus` (`jam_OwnerComponent.cpp:104-113`).
2. `Nexus` exports `enaxTerminalReport`: it calls `ENDActions::get` with the action name and the value (`ENDActions.h:40`), the shape of `promptPaintedCallback` (`Nexus.cpp:30`, `:66`).
3. `Nexus` exports `enaxStartDirectory`, the shape of `manifestFileGetter` (`:31`, `:67`). It reads the `cwd` parameter of the focused pane.
4. Client rules `Id::terminalReport` and `Id::startDirectory` join `ENAXClientExtensions`.
5. `ShellIntegration::getStartDirectory` asks the rule. The fallback is the config value (`ShellIntegration.cpp:16`). An empty host value also takes the config value.
6. `EVEView::valueTreePropertyChanged`: on `title`, `foregroundProcess` or `cwd`, tell the rule when the `focus` parameter is on. On `focus` turning on, tell all three.
**Validation:** outside END no rule exists, and EVE behaves as today. The action does nothing but tell.

### Step 10a: Tab name
**Scope:** `end/Source/ui/SessionView.h`, `.cpp`.
**Action:**
1. `SessionView::valueTreePropertyChanged` (`:81`) adds the three properties. The parameter flush fires it.
2. `getTabName` keeps its `rename` result. Then it reads the source pane: `title`, `foregroundProcess`, the last folder of `cwd` with `juce::File::getFileName`, then `Id::name`.
**Validation:** the tab name is one read of the row, driven by the event. No poll and no second store.

### Step 11: Measure and log
**Scope:** `scout` manifest, `carol/SPRINT-LOG.md`.
**Action:** ARCHITECT builds and tests. COUNSELOR runs the raw byte benchmark with `scout` against the baseline. `DEBT-20261103T235332` and `DEBT-20261103T233912` drain at `/log`.
**Validation:** the raw byte median does not fall.

## Amendments

Audit of the sprint, 2026-11-04.

- **Pane identity.** ARCHITECT: *"add field to ENAX, pane UUID == plugin UUID"*. This replaces Step 10.1, 10.3 and 10.6. The extension `enax.pane-uuid` gives each EVE instance its pane UUID in `ENAXClientExtensions::ENAXClientExtensions`. `enax.terminal-report` and `enax.start-directory` carry it. END writes the row of that pane (`ENDView::getPaneRow`). EVE reports from any pane on each change, with no `focus` gate and no report in `focusGained`.
- **Step 6.1.** `EVEView::lookAndFeelChanged` writes the row, because the `terminal` row is not in `TextEditor`'s tree (`juce_ValueTree.cpp:777`).
- **Step 7.5.** The clipboard record ends as released, so no release follows it. `handleAsyncUpdate` runs the exchange before `writePendingData`.
- **Step 8.2.** A link record holds `params;uri`, ends as released, and takes the owner line of each row that holds its number (`TerminalSwap::writeLine`).
- **Step 10.5.** `EVETerminalProcessor::openTTY` asks the rule, because `ShellIntegration` holds no processor.

## BLESSED Alignment

- **B:** the tick owns its metadata. The swap owns the materialised links. Each END pane owns its cwd, title and app name on its row.
- **L:** new work is table rows and lambdas. No new branch chain.
- **E:** every report is an event with a name. The stub is one empty event.
- **S (SSOT):** terminal state is a Model row. The URI lives once in the record.
- **S (Stateless):** view and END hold no copy of a value the Model holds. A pane row holds only what END shows.
- **E (Encapsulation):** data moves one way: tick to Model to view to END. END sets the cwd through a rule that EVE asks.
- **D:** same bytes, same rows.

## Risks / Open Questions

None. Each point above is closed by a ruling or a read.
