# PLAN: Terminal Bugs

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-05
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM. LANGUAGE.md C++/JUCE adapts Lean only: one object, one translation unit.

## Overview

This plan fixes the nested screen cursor, the cursor shape, the file drop, and the winsize coalesce in EVE and JAM. It amends SPEC.md. The zshrc and nvim edits go to MACHINIST.

ARCHITECT's rulings for this plan:

- [1]: *"Both (Recommended)"*: zshrc arms the trap at the first prompt, and EVE coalesces outside frame changes.
- `winsize`: *"Amend SPEC (Recommended)"*: the code stays.
- [4]: *"nvim config too"*: MACHINIST changes the nvim highlight or query. EVE stays unchanged for [4].
- [3]: *"EVEView should be FileDragAndDropTarget, dragging file should pasting the pathname to active prompt"*.
- [5]: *"block char only at normal mode, entering insert mode it changed to | pipe cursor by default following nvim"*.
- Constraint: *"live buffer at terminal processor and history rendered above active prompt at TextEditor is COMPELETELY DECOUPLED"*. No step changes history.
- Constraint: *"always lock free at terminal (audio) thread hot path"*.

## Language / Framework Constraints

- Generated files are CAST output. Engineer edits `jam/cast/terminal.md`, and never `jam/generated/*`.
- A non-template body goes in the object's one `.cpp`. A template stays in the header (CODING.md, TRANSLATION UNIT PLACEMENT).
- No comment and no doxygen before the audit (CAROL.md Code Hygiene).
- No build by an agent. ARCHITECT runs `cast cast/spell.md` and tests (CAROL.md Build).

## Dependency & API Inventory

**JUCE**
- `juce::FileDragAndDropTarget`: `isInterestedInFileDrag`, `filesDropped` (`juce_FileDragAndDropTarget.h:61`, `:112`). JUCE walks up from the component under the mouse (`juce_ComponentPeer.cpp:470-472`) and uses `dynamic_cast` (`:458`), so the base is public.
- `juce::Timer`: `startTimer`, `stopTimer`, `isTimerRunning`, `timerCallback`.
- `juce::Rectangle<float>::removeFromBottom`, `removeFromLeft`.

**JAM, used as is**
- `jam::TerminalBuffer::getScreen` (`jam_TerminalBuffer.h:267`).
- `map::Screen` with `normal = 0`, `alternate = 1` (`generated/jam_Terminal.h:128-131`).
- `map::CursorShape`, keys 0 to 6 (`generated/jam_Terminal.h:1141-1163`).
- `map::CSI::setCursorStyle` (`:1027`) and `map::CsiIntermediate::space` (`cast/terminal.md:813-820`).
- `AnsiDocument::getParameterValue`: `std::from_chars` stops at the intermediate and keeps the default on an empty field (`jam_AnsiDocument.cpp:783-789`).
- `TerminalKeyboard::bracketedPasteStart`, `bracketedPasteEnd` (`jam_TerminalKeyboard.h:109`, `:116`).
- `TextEditor::setMode`, public (`jam_TextEditor.h:174`).
- `jam::PluginEditor::coalesceWindowTimeMs`, protected, 300 (`jam_PluginEditor.h:180-182`).

**EVE, used as is**
- `ShellIntegration::appendPath` (`ShellIntegration.cpp:101-122`).
- `EVETerminalProcessor::getShell` (`EVETerminalProcessor.h:268-278`).

**Established patterns**
- A table sized by the last enum key: `map::EraseInDisplay::eraseSavedLines + 1` (`jam_TerminalProcessor.cpp:885`).
- An array read in bounds by a positive range check: `jam_TerminalProcessor.cpp:761`.
- The intermediate lane of a CSI: the DECRQM branch (`jam_TerminalProcessor.cpp:328-329`).
- A reported terminal row: `mouseTracking`: the field and `applyEvents` (`:297`), the cast row (`cast/terminal.md:60`), the EVE registration (`EVETerminalProcessor.cpp:96-97`), the Model row (`parameters.md:33`).
- A repaint on a Model row: `Id::cursor` (`jam_TextEditor.cpp:788`).
- A typed view child walk: `applyToMarkdown` (`EVEView.h:284-292`).

**Consumers.** `EVETerminalProcessor.cpp:364` is the only caller of `TerminalProcessor::process` in the LSP result. `events.get` throws on an unregistered key (`jam_HashMap.h:1645-1649`), so Step 3 registers the new key there.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final code step, covering the whole sprint — never per step.

## Steps

### Step 1: One saved cursor for each screen

**Scope:** `jam/jam_terminal/processor/jam_TerminalProcessor.h`, `.cpp`.
**Cause:** A nested `?1049h` overwrites the one slot (`jam_TerminalProcessor.cpp:691`), so `?1049l` restores the wrong row (`:697`). Claude Code then moves relative to the cursor it assumes (`Iv(this.frontFrame.cursor,this.displayCursor)`, Claude Code 2.1.318 at byte 274738300 and after). wezterm keeps one saved cursor per screen (`term/src/terminalstate/mod.rs:232-238`). This is a statement of behaviour only. It is not a pattern source.
**Action:**
1. Delete `savedPen` (`:170`) and `savedCursor` (`:173`) from the header.
2. Add `std::array<AnsiDocument::Pen, map::Screen::alternate + 1> savedPens;` and `std::array<jam::Cell::Point, map::Screen::alternate + 1> savedCursors;`.
3. The save lambda (`:689-693`) names its `TerminalBuffer&`. It writes `savedCursors.at (screen)` and `savedPens.at (screen).attributes`, where `screen` is `static_cast<size_t> (terminalBuffer.getScreen())`.
4. The restore lambda (`:694-698`) reads the same slots.
5. The `1049` lambda (`:863-877`) keeps its order: it saves before the switch, and it restores after the switch. Thus a save on the alternate screen never writes the normal slot.
**Validation:** Read each use. LSP does not list `.cpp` uses here, so COUNSELOR reads the whole `controlConstruction` and `privateModeConstruction` tables, and the compile confirms that no old name remains. No history path changes (`TerminalBuffer::scrollUp` is untouched). No allocation on the terminal thread: a scalar and an attribute assignment.

### Step 2: Decode the cursor shape in JAM

**Scope:** `jam/cast/terminal.md`, `jam/jam_terminal/processor/jam_TerminalProcessor.h`, `.cpp`.
**Cause:** `jam_TerminalProcessor.cpp:333` skips any CSI with an intermediate byte, so `CSI Ps SP q` stops there.
**Action:**
1. `cast/terminal.md`, `## identifiers`: add `| @id | cursorShape | `cursorShape` | |` in the row form of `:59-61`, after the `cursor` row.
2. Header: add `int cursorShape { map::CursorShape::defaultShape };` beside `cursorVisible`.
3. `applyCsiSequence`, the branch at `:331-335`, after the `not hasIntermediate` call:
   - When `intermediate == map::CsiIntermediate::space` and `text.back() == map::CSI::setCursorStyle`, read `AnsiDocument::getParameterValue (fields.at (0), map::CursorShape::defaultShape)`.
   - Store it in `cursorShape` only when `value >= map::CursorShape::defaultShape and value <= map::CursorShape::steadyBar`. This is the shape of `:761`.
4. `applyEvents`: add `events.get (Id::cursorShape, int { cursorShape });` beside `mouseTracking` (`:297`).
5. RIS entry (`:677-688`): set `processor.cursorShape = map::CursorShape::defaultShape`.
**Validation:** No new `CsiFunction` row, because `csiFunctions` has no intermediate column (`cast/terminal.md:1252`). The only new name is `cursorShape`, which is the family of `map::CursorShape` and of endless's row (`endless/Source/terminal/Identifier.h:218`). The value is a scalar, so the hot path stays lock free.

### Step 3: Report the cursor shape to the Model

**Scope:** `eve/Source/layout/parameters.md`, `eve/Source/EVETerminalProcessor.cpp`.
**Action:**
1. `parameters.md`, `## model`: add the row `| cursorShape | choice | | 0 | Cursor shape that the terminal processor reports: a key of map::CursorShape |`, after `mouseTracking`. Keep the table widths.
2. `EVETerminalProcessor.cpp:96`: add `Id::cursorShape` to the `int` event list.
**Validation:** All identifiers of `applyEvents` are registered with matching argument types. The row matches `mouseTracking` (`parameters.md:33`).

### Step 4: Paint the caret shape

**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`.
**Action:**
1. In the terminal entry of `getCarets` (`:589-596`), add a local `static const std::array<juce::Rectangle<float> (*) (juce::Rectangle<float>), map::CursorShape::steadyBar + 1> caretShapes` in key order: `defaultShape`, `blinkingBlock`, `steadyBlock` return the cell. `blinkingUnderline`, `steadyUnderline` return `cell.removeFromBottom (caretThickness)`. `blinkingBar`, `steadyBar` return `cell.removeFromLeft (caretThickness)`.
2. Add a local `static constexpr float caretThickness { 2.0f };`, the JUCE caret width (`juce_CaretComponent.cpp:64`).
3. The visible branch returns `caretShapes.at (shape) (editor.getCellBounds (...))`, where `shape` is the `Id::cursorShape` property of `editor.terminal`.
4. Property table: add `properties.add<TextEditor&> (Id::cursorShape, repaintContent);` beside `:788`.
**Validation:** A lookup and no branch chain (MANIFESTO L). Table name is a plain noun (NAMES Rule 8). Scope is the narrowest that holds the use (CODING SCOPE). The blink keys paint like the steady keys: the terminal caret has no blink at `:1657-1658`, and this plan adds none. Only the terminal mode entry changes.

### Step 5: One paste path in TextEditor

**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`.
**Action:**
1. Move the bracket and write lines of the `Id::paste` action (`:473-479`) to a new public member `void applyPaste (std::string_view text);`, in the `.cpp`.
2. The `Id::paste` action becomes `editor.applyPaste (jam::Strings::getView (text));`, where `text` is the clipboard text.
**Validation:** Behaviour of `Id::paste` is unchanged. One source for the bracket markers (MANIFESTO S). `apply` is the family of `applyLink` (`jam_TextEditor.h:798`).

### Step 6: EVEView takes the file drop

**Scope:** `eve/Source/EVETerminalProcessor.h`, `.cpp`, `eve/Source/EVEView.h`, `.cpp`.
**Action:**
1. `EVETerminalProcessor`: add public `std::string getQuotedPaths (const juce::StringArray& paths) const`. For each path, it appends `Chars::space` when the text is not empty (`jam::Format::appendUtf8`), then `ShellIntegration::appendPath (text, getShell(), path.toRawUTF8())`. It returns the text as UTF-8.
2. `EVEView` adds the public base `juce::FileDragAndDropTarget`, with `isInterestedInFileDrag`, which refuses a path with a control byte (`ShellIntegration::hasControlByte`), and `filesDropped`.
3. Add the template `applyToTextEditors (Callback&& callback)` in the header, in the shape of `applyToMarkdown` (`EVEView.h:284-292`), filtered on `Id::type`, `Id::textEditor`. `setResizing` (`EVEView.cpp:68-75`) uses it.
4. `filesDropped`: read `getQuotedPaths (files)` once. For each text editor, call `editor.setMode (map::EditorMode::terminal)` and `editor.applyPaste (text)`.
**Join rule source:** ARCHITECT's endless default: *"\"space\" joins paths with spaces"* and *"paths with special characters are quoted for the active shell"* (`endless/Source/resources/config/default_nexus.lua:90-98`). No config row: nothing passes another value (MANIFESTO YAGNI).
**Input size:** No bound. ARCHITECT: *"why hardcode the limit of pasting?"*. Input waits in `pendingInput`, and each vblank moves the next part into the input lane.
**Validation:** Tell only. The panel's `juce::FileTreeComponent` is a `TreeView`, thus a nearer target in the JUCE walk, so a drop over the panel does not reach `EVEView` when the tree is interested. No `juce::File` is opened in `isInterestedInFileDrag` (`juce_FileDragAndDropTarget.h:53-55`).

### Step 7: Coalesce an outside frame change

**Scope:** `eve/Source/EVEView.h`, `.cpp`.
**Cause:** `resizeStart` runs only from a JUCE border or corner drag and from `windowWillStartLiveResize` (`juce_ResizableBorderComponent.cpp:141`, `juce_ResizableCornerComponent.cpp:74`, `juce_NSViewComponentPeer_mac.mm:2844`). `redirectMovedOrResized` (`mm:1236-1240`) does not call it. Ruling: *"why dont we make resize always coalesce, FINAL STOP only"* (`SPEC.md:507`); *"EVEView orchestrate"* (`SPEC.md:509`).
**Action:**
1. `EVEView` adds the private base `juce::Timer` and `timerCallback() override`.
2. Add private `bool isResizing()`, which reads `Id::isResizing` of the text editors through `model.getOrCreateComponentProperty`, with `applyToTextEditors`.
3. The first lines of `EVEView::resized`, before `jam::PluginEditor::resized()`: when `isTimerRunning() or not isResizing()`, call `setResizing (true)` and `startTimer (coalesceWindowTimeMs)`.
4. `resizeStart`: `stopTimer()`, then `setResizing (true)`.
5. `resizeEnd`: `stopTimer()`, then the existing call.
6. `timerCallback`: `resizeEnd()`.
**Truth table:** A live resize and an open panel hold the gate with no timer, so `resized` adds nothing. An outside frame change finds the gate open or the timer running, so each step extends the window. The final stop writes the winsize through `Id::isResizing` (`jam_TextEditor.cpp:787`).
**Validation:** The gate has one truth, the Model row. The timer is `juce::Timer` state, and no manual boolean exists. `view` is not null at the first `resized`, because `initialiseView` runs before `setScaling` (`jam_PluginEditor.cpp:207`, `EVEView.cpp:16-19`). The constant is reused, with no restated value. In a fresh state, the view builds the TextEditor with its gate open (`jam_ViewEditor.h:127-130`), and that first write forks the shell. Audit finding 2 corrected this line. A restored winsize forks at its own size (`EVETerminalProcessor.cpp:300-301`).

### Step 8: Amend SPEC.md

**Scope:** `eve/SPEC.md`.
**Action:** COUNSELOR writes, with ARCHITECT's quotes:
1. `SPEC.md:517-519`: the `winsize` row has no default. `audioDeviceAboutToStart` prepares only with a valid winsize. *"Amend SPEC (Recommended)"*.
2. `:507-516`: the final stop covers an outside frame change: `EVEView` is the timer.
3. The terminal contract: one saved cursor per screen; the cursor shape row.
4. `:522`: the trap is armed at the first prompt.
5. The view contract: the file drop.
**Validation:** Each new line maps to a ruling in this plan or to a file:line of the changed code. Nothing else changes.

### Step 9: Audit

**Scope:** Steps 1 to 7.
**Action:** @Auditor runs one sweep against SPEC.md, MANIFESTO.md, NAMES.md, CODING.md, and this plan. COUNSELOR validates each finding by a read, and resolves each confirmed finding through @Engineer.
**Validation:** Each finding is resolved (file:line) or rejected (the read that refutes it).

### Step 10: Doxygen pass

**Scope:** The members that Steps 1 to 7 added or changed. Includes the `coalesceWindowTimeMs` doc, which now names the view coalesce too.
**Action:** @Pathfinder writes the pass with exact targets from COUNSELOR. COUNSELOR reads each block against the code.
**Validation:** CODING.md COMMENTS, `doxygen-writer` skill.

### Step 11: Report

**Action:** COUNSELOR reports the changed files, the audit findings verbatim, the MACHINIST items below, and the commit messages in chat. COUNSELOR does not log the sprint, because logging starts with ARCHITECT.
**Validation:** File content against each step.

## MACHINIST Items

COUNSELOR does not edit `~/.config`. COUNSELOR tests item 1 in the scratchpad after the lock, with `zsh -ic` and a pty, and gives the result with the item.

1. `~/.config/zsh/zshrc:181`: replace `TRAPWINCH() { omp_repaint_prompt }` with:
   ```
   _omp_arm_repaint() { TRAPWINCH() { omp_repaint_prompt }; add-zsh-hook -d precmd _omp_arm_repaint }
   add-zsh-hook precmd _omp_arm_repaint
   ```
   The hook is added after `_omp_precmd` (`init…zsh:479`), so `_omp_status` has a value when the trap can fire.
2. `~/.config/nvim`: change the highlight or the query for `shortcut_link` (`markdown_inline/highlights.scm:81`) and `pipe_table_header` (`markdown/highlights.scm:30-31`), so a pandoc grid table does not show them. `after/queries` exists, with `cpp` only. MACHINIST selects the form with ARCHITECT.

## BLESSED Alignment

- **B:** The processor owns the saved cursors. The view owns the timer. Nothing crosses the terminal and message threads directly.
- **L:** `caretShapes` is a lookup. No new file. No config row with no second value.
- **E:** All values are named, and every parameter is visible. No bail-out guard.
- **S (SSOT):** `applyPaste` is the one bracket path. `appendPath` is the one quote path. `Id::isResizing` is the one gate.
- **S (Stateless):** The shape and the saved cursors are decoder working state (`jam_TerminalProcessor.h`). The shape has a Model row, and the view reads that row.
- **E (Encapsulation):** The view tells the editor. The terminal thread writes only atomics through `state.storeValue`.
- **D:** The same bytes give the same cursor. Fix at the owner, with no extra guard.

## Verification (ARCHITECT)

1. Run `cast cast/spell.md`, then build.
2. Ctrl+G from a long Claude Code prompt, then quit nvim. Expect no overlaid rows. Scroll up. History is intact.
3. nvim: `i` gives a bar. Esc gives a block. Quit, and the shell has a steady block.
4. Drag one file, then several with spaces in their names, to the prompt. Expect quoted paths, joined by a space, with no Enter.
5. Open EVE under PaperWM many times after the zshrc edit. Expect no OMP error, and one prompt redraw after the tile settles.
6. Drag the window corner, and pause during the drag. Expect no winsize change until release.
7. Enter and leave LIST mode. Expect no SIGWINCH, as in `SPEC.md`.

## Risks / Open Questions

None open. Each is closed above: [1], `winsize`, [4] by ARCHITECT's answers. The input bound in Step 6 equals the existing `Id::paste` bound. The cause in Step 1 comes from reads, and Verification 2 proves it at runtime.
