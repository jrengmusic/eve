# PLAN: Published Lines and Live Rows

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-10
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE / JAM (`~/.carol/LANGUAGE.md` C++ / JUCE section)

**Plan file at lock:** plan mode permits only this file. The first action after the lock copies this
plan, unchanged, to `/Users/jreng/Documents/Poems/dev/eve/PLAN-published-lines-and-live-rows.md`.

## Overview

END crashed on 2026-11-10 at 18:25:26 inside the EVE plugin (`END-2026-11-10-182526.ips:44-49`):
`EXC_BAD_ACCESS` at `0x3fffffff8` in `jam::AttributedString::setAtom`, called from
`jam::AnsiDocument::addGrids` on the vblank callback of the main thread. This plan removes the
contract violations on that path, and it applies three rulings that ARCHITECT gave in the same
session: a wrapped row joins its line in the reader, a grid change clears the live rows only at a
prompt, and Ctrl+L goes to the shell. The work is in `jam` and `eve`. END gets no change.

## Context: findings that this plan corrects

1. **A published history line is edited.** For a wrapped row, the writer seeks back and writes the
   joined line over the last stored line (`jam_TerminalSwap.cpp:95-112`, `:123-127`, `:134`). That
   line is below `endLine` (`:119`), so the reader's range holds it (`:249-252`). The writer runs
   on the serial queue of `jam::AsyncUpdater` (`EVETerminalProcessor.cpp:314-330`,
   `jam_AsyncUpdater.h:16-22`). The reader runs on the main thread (`jam_TextEditor.cpp:98`).
   Clause: *"No thread changes the slot after the publish. A state update replaces the slot. It
   does not edit it."* (`MANIFESTO.md` E, Published data is immutable).
2. **The reader reads plain writer members.** `previousGridStart` (`jam_TerminalSwap.h:361`) is
   written at `jam_TerminalSwap.cpp:139` and read at `:335-337`. `numReadRows` (`.h:362`) is written
   at `.cpp:135` and read through `getNumReadRows` (`:239-242`, caller `jam_TextEditor.cpp:1165`).
   Clause: *"A plain (non-atomic) read that races a write"* (`MANIFESTO.md` anti-patterns, D).
3. **The crash arithmetic.** `getGutter` starts at `INT_MAX` and lowers it only for a row with two
   or more cell starts (`jam_AnsiDocument.cpp:447-455`). `addEntries` appends one atom per gutter
   step (`:494-495`). `sizeof (AttributedChar) == 8` (`jam_AttributedChar.h:516`), and
   `0x3fffffff8 == 0x7fffffff * 8`. The writer marks a grid run only when its first row has two or
   more cell starts (`jam_AnsiDocument.cpp:390-396`, `jam_TerminalSwap.cpp:177`). The reader
   therefore needs a line whose cells and `gridStart` do not agree. Findings 1 and 2 are the read
   paths that can give one. No read proves which one ran on 2026-11-10.
4. **The frame has three authors.** The TTY bytes author the frame (`jam_TerminalProcessor.cpp:58`).
   The `clear` command also erases it (`EVETerminalProcessor.cpp:489-510`). A grid change also
   erases it and homes the cursor (`jam_TerminalBuffer.cpp:30-34`, `jam_TerminalProcessor.cpp:36-37`).
   The erase happens before the child learns the new size (`EVETerminalProcessor.cpp:465`, `:474`).

## ARCHITECT rulings (verbatim)

1. *"Reader joins at build"* — the wrap join.
2. *"stray variables for state machine are not allowed. if it was state machine must be Model's
   parameter, hence message thread could listen, other thread could use atomic"* — state that the
   reader needs.
3. *"Clear only at a prompt"* — a grid change. Context: *"ctrl+l should not erase active prompt,
   nor resize (because creating new pane) not clearing whatever on live buffer because there is no
   active prompt yet"*.
4. *"Remove the clear command"* — Ctrl+L. Context: *"we should adhere to shell SPEC"*.
5. *"our architecture is NOT `typical` terminal topology. we preserved history faithfully. our
   rendering layer is COMPLETELY decoupled. audio architecture discipline always lock free on
   terminal (audio thread)"*.

## Language / Framework Constraints

- One object, one translation unit (`LANGUAGE.md` L, `CODING.md` placement). Every new body goes to
  the `.cpp` of its object. No new file.
- No comment and no doxygen before the audit (`CAROL.md` Code Hygiene). Step 9 is the doxygen pass.
- Generated files are CAST output (`eve/CLAUDE.md:42`, `:77-78`). The plan edits the cast tables.
  ARCHITECT runs `cast`. The generated `jam/generated/jam_Terminal.*` and
  `eve/Source/generated/Identifiers.*` change only in that run. No agent edits them or builds.
- No lock, wait, allocation or blocking call enters the clock thread. No step adds code to
  `TerminalProcessor::process` or to `EVETerminalProcessor::process`.

## Dependency & API Inventory

| Need | Existing API, used as it is |
|---|---|
| Cross-thread state | Event to Model row: `events.get (Id::numPrompts, int { numPrompts })` (`jam_TerminalProcessor.cpp:304`), stored by `state.storeValue` (`EVETerminalProcessor.cpp:177-178`), row in `eve/Source/layout/parameters.md:45`. The store is a lock-free atomic (`jam_ParameterBase.h:222-226`). The view reads the terminal tree (`jam_TextEditor.cpp:1019-1023`). |
| Published line range | `firstLine`, `endLine` atomics, `TerminalSwap::getLines` (`jam_TerminalSwap.cpp:249-252`). |
| Wrapped row fact | `TerminalBuffer::isWrapped (int slot)` (`jam_TerminalBuffer.cpp:141-144`), set by the decoder (`jam_TerminalProcessor.cpp:498`). |
| Run merge at build | `AnsiDocument::addGrids` / `addGrid` / `removeLines` (`jam_AnsiDocument.cpp:408-438`, `:499-528`, `:461-473`). |
| Run that crosses a range edge | `TerminalSwap::removeGridHead`, `addGridTail`, `addGridLines` (`jam_TerminalSwap.cpp:369-389`, `:412-446`). |
| Join of cells with links | `AttributedString::append (const AttributedString&, juce::Range<int>)` (`jam_AttributedString.cpp:37-68`). The source must have run `setLinks()` (`jam_AttributedString.h:90` doc); `addLine` runs it (`jam_TerminalSwap.cpp:265-278`). |
| Line layout properties | `AnsiDocument::addSegments` (`jam_AnsiDocument.cpp:219-232`). |
| Element properties | `Element::add` appends and never replaces; readers take the first match (`jam_Document.h:452-458`, `:358-366`). In-place change of a payload: `*line->find<juce::int64> (Id::gridStart) = …` (`jam_TerminalSwap.cpp:384`). `bool` is a `Document::Value` alternative (`structjam_1_1Document.xml:84-85`). |
| Rows to history | `TerminalBuffer::scrollUp` over the whole grid on the normal screen (`jam_TerminalBuffer.cpp:292-313`); sibling use at OSC 133 D (`jam_TerminalProcessor.cpp:1202-1206`). |
| Blank frame slots | `clear (write (n))` (`jam_TerminalBuffer.cpp:302`, `:87-96`, `:261-270`). |
| Blank screen and home | `eraseInDisplayConstruction.at (map::EraseInDisplay::eraseEntireScreen)` and `setCursor` (`jam_TerminalProcessor.cpp:41-46`, `:980`, `:632-639`). |
| Prompt state | `isRecording`, `numPrompts` (`jam_TerminalProcessor.h:201`, `:203`; writes at `.cpp:1183`, `:1200`, `:1211`). zsh writes D then A in `precmd`, C in `preexec` (`eve/Source/shell/zsh_integration.zsh:8-9`, `:13`). |
| Key to shell | `TerminalKeyboard::map` after `commandManager->invokeKey` (`jam_TextEditor.cpp:634-645`, `jam_TerminalKeyboard.cpp:34-53`). |

**Names.** Each new name joins an established family (NAMES Rule 5). The lock of this plan fixes
them. No other name enters any delegation.

| Name | Where | Family |
|---|---|---|
| `Id::isWrapped` | `jam/cast/terminal.md` `## identifiers` | `TerminalBuffer::isWrapped`, `Id::isWrapPending`, `Id::isRecording` |
| `Id::previousGridStart` | `jam/cast/terminal.md`; row in `eve/Source/layout/parameters.md` | member `previousGridStart`; `Id::gridStart`, `Id::activeGridStart`, `Id::removedGridStart` |
| `bool isWrapped` (parameter) | `TerminalBuffer::writeRow`, `TerminalSwap::writeLine` | same |
| `AnsiDocument::addWrappedLines`, `AnsiDocument::addWrappedLine` | `jam_AnsiDocument.h/.cpp` | `addGrids`, `addGrid` |
| `TerminalSwap::removeWrappedHead`, `TerminalSwap::addWrappedTail` | `jam_TerminalSwap.h/.cpp` | `removeGridHead`, `addGridTail` |
| `TerminalSwap::addImageLines` | `jam_TerminalSwap.h/.cpp` | `addImages`, `addLines` |
| `TerminalProcessor::applyWinsize` | `jam_TerminalProcessor.h/.cpp` | `applyEvents`, `applyCarriageReturn`; `TerminalTTY::setWinsize` |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift).
@Auditor runs ONCE, after the final code step, covering the whole sprint — never per step.
COUNSELOR reads the owner API again before each delegation and pastes each sibling verbatim.

## Steps

### Step 1: Tables — identifiers, Model row, `clear` removal

**Scope:** `jam/cast/terminal.md`, `eve/Source/layout/parameters.md`, `eve/cast/identifiers.md`,
`eve/Source/layout/default-eve.md`
**Action:**
1. `jam/cast/terminal.md` `## identifiers`: add two `@id` rows, `isWrapped` and `previousGridStart`,
   in the form of the row at `:55` (`| @id | gridStart | \`gridStart\` | |`).
2. `eve/Source/layout/parameters.md` `## model`: add the row `previousGridStart`, type `int64`,
   default `-1`, in the form of the `clipboard` row (`:67`). Description: the `gridStart` of the
   last line that the record writer wrote; `-1` when that line is in no grid.
3. `eve/cast/identifiers.md`: delete the `clear` row of `## CommandId` (`:73`).
4. `eve/Source/layout/default-eve.md`: delete the `clear` key row (`:47-49`). The text at `:42`
   stays: *"In terminal mode every other key goes to the shell."*
**Validation:** rulings 2 and 4. Rows match their table siblings character for character in form.
No `version:` field changes. No generated file changes.

### Step 2: Row record carries the wrapped mark

**Scope:** `jam/jam_terminal/buffers/jam_TerminalBuffer.h`, `jam_TerminalBuffer.cpp`
**Action:**
1. `TerminalBuffer::writeRow` (binary lane, `.cpp:361-377`, `.h:78`) takes a last parameter
   `bool isWrapped` and writes it after `gridStart` with `output.writeBool`.
2. `TerminalBuffer::getNumBytes` (`.cpp:551-572`) adds the bytes of the mark to `rowBytes`, as a
   named constant beside `gridStartBytes` (`:564`).
3. The live write passes `false` (`.cpp:396-399`).
**Validation:** the record order is cells, placements, `outputBlock`, `gridStart`, `isWrapped`.
`getNumBytes` equals the bytes that `writeRow` writes for a full row. No magic number.

### Step 3: Writer — one row, one line; state through the Model

**Scope:** `jam/jam_terminal/document/jam_TerminalSwap.h`, `jam_TerminalSwap.cpp`,
`eve/Source/EVETerminalProcessor.cpp`
**Action:**
1. `TerminalSwap::writeFromTerminalBuffer` (`.cpp:78-148`): delete the join. The locals
   `joinedCells` and `joinPosition`, the block at `:95-112`, and the two branches at `:123-127`
   go. Each row of the reader handle is one line at `newEndLine`. The `else if` at `:128-131`
   becomes the only index switch.
2. In the same function: `const auto isWrapped { terminalBuffer.isWrapped (slot) };`. The cell
   starts are computed only when the row has an output block and `isWrapped` is false; otherwise
   `starts` is empty (form of `:120`). `getGridStart` then gives `noGridStart` for a wrapped row
   (`jam_AnsiDocument.cpp:390-406`), and `applyGridStart` fires `Id::removedGridStart` for an open
   grid (`jam_TerminalSwap.cpp:185-195`).
3. `TerminalSwap::writeLine` (`.cpp:150-173`, `.h:303`) takes a last parameter `bool isWrapped`
   and passes it to `TerminalBuffer::writeRow`, also in its recursion at `:171`.
4. After the row loop, beside the `Id::firstLine` event (`.cpp:145`), the writer fires
   `events.get (Id::previousGridStart, int64_t { previousGridStart });`.
5. `EVETerminalProcessor::registerImageEvents` (`.cpp:177-178`): add `Id::previousGridStart` to the
   `int64_t` list that calls `state.storeValue`.
6. Delete `TerminalSwap::getNumReadRows` (`.cpp:239-242`, `.h:98`). After item 1 each row is one
   line, so `numReadRows` equals the end line that `setLines` publishes (`.cpp:134-140`, `:220`).
   The reader takes that fact from `getLines().getEnd()` in Step 5. A Model row for it would hold
   the published end line a second time (`MANIFESTO.md` S, shadow state).
**Validation:** rulings 1, 2, 5. The writer performs no `setPosition` on `segmentOutput` or
`indexOutput`. No stored line is written twice. The member `numReadRows` stays private writer
state with no reader. No lock, no new member.

### Step 4: Reader — the wrapped mark on each line

**Scope:** `jam/jam_terminal/document/jam_TerminalSwap.h`, `jam_TerminalSwap.cpp`
**Action:**
1. `TerminalSwap::addLine` (`.cpp:259-286`): after the `Id::gridStart` property (`:283`), add
   `line->add<bool> (Id::isWrapped, input.readBool());`.
2. `TerminalSwap::addImages` (`.cpp:288-315`): delete the line `image->add<int> (Id::line,
   previousLines.size());` (`:308`). `getCoveredImage` does not read `Id::line` (`:317-329`).
3. New `void TerminalSwap::addImageLines (AnsiDocument& document)`: for each `Id::line` child of
   the root, in order, with its number in the document counted from 0, add `Id::line` with that
   number to each `Id::image` child of the line. `addLiveLines` (`.cpp:479-495`) calls it after its
   row loop. Step 5 calls it in `addLines`. `getImageBox` reads this number as the line number of
   the block (`jam_TextEditor.cpp:1487`), and the document can hold fewer lines than stored rows
   after a merge (`jam_AnsiDocument.cpp:431-435`).
**Validation:** every line element has `Id::isWrapped`. Every `Id::image` element has exactly one
`Id::line` property, and it equals the index of its line in the finished document. Range-for and
structured bindings per CODING.md. No out-parameter.

### Step 5: Reader — join wrapped lines at build

**Scope:** `jam/jam_terminal/document/jam_AnsiDocument.h`, `jam_AnsiDocument.cpp`,
`jam_TerminalSwap.h`, `jam_TerminalSwap.cpp`, `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:**
1. New `static void AnsiDocument::addWrappedLines (AnsiDocument& document)`, sibling of `addGrids`
   (`jam_AnsiDocument.cpp:408-438`). A run is a line and the lines after it that carry
   `Id::isWrapped` true. A run of two or more lines whose every `Id::images` list is empty is
   replaced by one line, built by `addWrappedLine`. Any other line stays as it is.
2. New `static void AnsiDocument::addWrappedLine (AnsiDocument& document, Document::Element& first,
   Document::Element& last)`, sibling of `addGrid` (`:499-528`). The new line takes the place of
   `first` (`document.addChild (*root, first.previousSibling, Id::line)`). Its `Id::cells` holds the
   cells of each run line, appended in order with `AttributedString::append (cells, { 0,
   cells.size() })`. It then gets `addSegments`, an empty `Id::images` list, the `Id::outputBlock`
   of `first`, `Id::gridStart` with `TerminalBuffer::noGridStart`, and `Id::isWrapped` false. The
   function then calls the existing `removeLines` (`:461-473`).
3. New `void TerminalSwap::removeWrappedHead (AnsiDocument& document, juce::int64 start)`, sibling
   of `removeGridHead` (`jam_TerminalSwap.cpp:369-389`). When `start` is above
   `getLines().getStart()`, it removes the leading lines of the document that carry `Id::isWrapped`
   true. The block before the range holds them through its tail.
4. New `int TerminalSwap::addWrappedTail (AnsiDocument& document, juce::int64 end)`, sibling of
   `addGridLines` (`:421-446`). It reads the lines after `end` chunk by chunk with `addRangeLines`
   while the appended lines carry `Id::isWrapped` true. It unlinks the first line that does not,
   and every line after it. It returns the number of lines that it kept.
5. `TerminalSwap::addLines` (`.cpp:331-349`, `.h:144`) takes a last parameter
   `juce::int64 previousGridStart`. The order becomes: `addRangeLines`; `removeWrappedHead`;
   `addWrappedTail`; `removeGridHead`; `addGridTail`; root `Id::size`; root `Id::end`;
   `AnsiDocument::addWrappedLines`; `AnsiDocument::addGrids`; `addImageLines`.
   `isOpen` (`:335-337`) compares with the parameter, not with the member. The root `Id::end` is
   set when `isOpen` holds, or when `lines.getEnd()` plus the count from `addWrappedTail` equals
   `getLines().getEnd()`.
6. `TextEditor::addBlock (TerminalSwap&, juce::int64, juce::Range<juce::int64>)`
   (`jam_TextEditor.cpp:1178-1195`): pass
   `static_cast<juce::int64> (terminal.getProperty (Id::previousGridStart))` to `addLines`.
7. `TextEditor::addBlocks (TerminalSwap&)` (`jam_TextEditor.cpp:1165`): `isOpenChanged` compares
   `*end` with `lines.getEnd()` (the local of `:1154`), not with `swap.getNumReadRows()`.
**Consequences, stated for the lock:**
- A wrapped row gets `noGridStart` (Step 3), and a joined line gets `noGridStart`. A grid run that
  holds a wrapped row is no longer whole (`jam_AnsiDocument.cpp:426-427`), so its lines stay plain
  lines. Today such a row can stay in the grid when the joined cells keep the cell starts
  (`jam_TerminalSwap.cpp:119-121`, `jam_AnsiDocument.cpp:398-406`).
- The join has no length limit. Today the writer joins only while the joined cells fit
  `bounds.width * bounds.height` (`jam_TerminalSwap.cpp:106`). That limit is the size of one
  segment (`jam_TerminalBuffer.cpp:551-572`, `jam_TerminalSwap.cpp:27`). Each stored row now fits a
  segment by itself, so the store needs no limit. A logical line of any length is one document
  line, as `eve/SPEC.md:280-281` states: *"the widget re-derives the wrapped layout from logical
  lines"*.
- `addGrids` gets no guard and no assert. The writer owns the grid invariant, and the reader now
  reads only lines that no thread edits (`MANIFESTO.md` D).
**Validation:** rulings 1, 2, 5. `addWrappedLines` and `addWrappedLine` follow `addGrids` and
`addGrid` in shape and in verbs. No property is added twice to one element. No function body is
above 30 lines and no chain is above 3 branches; where a body crosses, COUNSELOR applies the Lean
ladder before any split. The reader reads no member that the writer writes, except the atomics
`firstLine` and `endLine`.

### Step 6: Grid change — keep the rows, clear only at a prompt

**Scope:** `jam/jam_terminal/buffers/jam_TerminalBuffer.h`, `jam_TerminalBuffer.cpp`,
`jam/jam_terminal/processor/jam_TerminalProcessor.h`, `jam_TerminalProcessor.cpp`,
`eve/Source/EVETerminalProcessor.cpp`
**Action:**
1. `TerminalBuffer::prepare` (`jam_TerminalBuffer.cpp:7-35`): the frame rows stay on every call.
   The rotate block (`:16-24`) runs whenever `heightRows > 0`. The count of kept rows is the
   smaller of the old and the new height. After `prepareRows`, the frame is `write (keptRows)`
   followed by `clear (write (newRows - keptRows))` when the new height is larger. The calls
   `clear (frame)` and `clearAlternate()` (`:30-34`) go.
2. In the same function each kept row of the ring frame and of the alternate grid is cut to
   `newCols` atoms with `AttributedString::resize` (`jam_AttributedString.cpp:144-155`). One slot
   of the live file holds at most `cols` atoms for each row (`jam_TerminalBuffer.cpp:555-561`,
   `jam_TerminalSwap.cpp:27-36`), and a longer row makes the live write fail
   (`jam_TerminalSwap.cpp:69`).
3. Delete `TerminalBuffer::clearAlternate` (`.h:502-503`, `.cpp:63`). Its only caller is
   `prepare` (`.cpp:33`).
4. New public `void TerminalProcessor::applyWinsize (jam::TerminalWinsize newWinsize,
   TerminalBuffer& terminalBuffer)`. On the normal screen, when the cursor row is at or below
   `newWinsize.rows`, the call scrolls the grid up by `cursor.y - (newWinsize.rows - 1)` rows with
   `terminalBuffer.scrollUp (jam::Cell { 0 }, jam::Cell { terminalBuffer.getBounds().height - 1 },
   numRows)` and moves the cursor up by the same count with `setCursor`. This is the form of the
   OSC 133 D entry (`jam_TerminalProcessor.cpp:1202-1206`). The rows that leave the grid are
   history rows (`jam_TerminalBuffer.cpp:296-304`).
5. `TerminalProcessor::reset` (`jam_TerminalProcessor.cpp:20-39`): before `winsize = newWinsize`
   (`:25`), two locals: the grid changed (the `cols` or the `rows` of `newWinsize` differ from
   `winsize`), and a prompt is active (`not isRecording and numPrompts > 0`). The line
   `setCursor ({}, terminalBuffer);` (`:37`) becomes:
   - prompt active: when the grid changed, blank the screen with
     `eraseInDisplayConstruction.at (static_cast<size_t> (map::EraseInDisplay::eraseEntireScreen))
     (*this, terminalBuffer)`; then `setCursor ({}, terminalBuffer)`.
   - no prompt active: `setCursor (cursor, terminalBuffer)`, which limits the cursor to the new
     grid (`:632-639`).
6. `EVETerminalProcessor::prepare` (`EVETerminalProcessor.cpp:458-475`): after the first
   `triggerAsyncUpdate(); handleUpdateNowIfNeeded();` pair (`:460-461`), call
   `terminalProcessor.applyWinsize (winsize, terminalBuffer);` and run the pair again, before
   `terminalBuffer.prepare` (`:465`). `prepareRows` resets the FIFO (`jam_TerminalBuffer.cpp:50`),
   so the history rows must reach the swap first.
**Behaviour after the step:**
- Running CLI, or a new pane before its first prompt: the rows stay, and the cursor row stays in
  the grid. Rows that a lower grid cannot hold go to history.
- Active prompt and a changed grid: blank screen, cursor at home, then `SIGWINCH`
  (`EVETerminalProcessor.cpp:474`). This is the behaviour of today.
- Active prompt and the same grid: rows kept, cursor at home. This is the behaviour of today.
- No prompt and the same grid: rows kept, cursor kept. Today the cursor goes home.
**Validation:** ruling 3 and ruling 5. No call runs on the clock thread: `prepare` runs inside
`restart`, with the clock stopped (`EVETerminalProcessor.h:67-77`). `prepare` of the buffer uses
only `write`, `clear` and `resize`. No bail-out guard: `applyWinsize` uses positive nesting. No new
member in the decoder or in the buffer.

### Step 7: Ctrl+L goes to the shell

**Scope:** `eve/Source/EVEView.h`, `EVEView.cpp`, `eve/Source/EVETerminalProcessor.h`,
`EVETerminalProcessor.cpp`, `eve/CLAUDE.md`
**Action:**
1. Delete the `clear` callback registration (`EVEView.cpp:340-344`), `EVEView::clear`
   (`EVEView.cpp:516-519`, `EVEView.h:244-245`), and `EVETerminalProcessor::clear`
   (`EVETerminalProcessor.cpp:489-500`, `EVETerminalProcessor.h:82-87`).
2. `EVEView::clearHistory`, `EVETerminalProcessor::clearHistory` and
   `jam::TerminalProcessor::clear` stay. The history capacity callback uses them
   (`EVEView.cpp:398-405`, `:521-524`, `EVETerminalProcessor.cpp:502-510`).
3. `eve/CLAUDE.md:41`: remove the word `clear,` from the list of callbacks.
**Result:** with no key row for it (Step 1), Ctrl+L falls to `TerminalKeyboard::map`
(`jam_TextEditor.cpp:634-645`). It sends `0x0C`, or CSI u when the program set the kitty keyboard
flags (`jam_TerminalKeyboard.cpp:40-43`). zsh then clears and redraws its prompt
(`/usr/share/man/man1/zshzle.1:2281-2282`). ED 2 from the shell blanks the frame
(`jam_TerminalProcessor.cpp:980`), and ED 3 from the `clear` program hides the history
(`:981`), both as today.
**Validation:** ruling 4. After `cast`, no reference to `map::CommandId::clear` is left in eve
(LSP `findReferences` list of 2026-11-10: `EVEView.cpp:340` only). END holds no `clear` key row
(`end/cast/*.md`, `end/Source/config/*.md`: no match).

### Step 8: Auditor

**Scope:** every file that Steps 1 to 7 changed.
**Action:** @Auditor validates the sprint against MANIFESTO.md, NAMES.md, CODING.md and this plan.
COUNSELOR reads each cited file:line, names the clause, and resolves each confirmed finding in
this sprint.
**Validation:** each finding is resolved with file:line, or rejected with the read that refutes it.

### Step 9: Doxygen pass

**Scope:** the headers and the bodies that Steps 2 to 7 changed.
**Action:** @Pathfinder writes the pass on COUNSELOR's exact targets (`doxygen-writer` skill).
It covers the new functions, the changed parameters, and the blocks that this plan makes wrong:
`TerminalBuffer::prepare` (`jam_TerminalBuffer.h:136-153`), `TerminalProcessor::reset` (`.h:34-48`),
`TerminalSwap::writeFromTerminalBuffer` (`.h:65-88`), `writeLine` (`.h:292-303`), `addLines`
(`.h:125-144`), `addLine` (`.h:161-172`), `addImages` (`.h:174-184`), the member comment of
`previousGridStart` (`.h:361`), and the class doc of `EVETerminalProcessor` where it names `clear`.
**Validation:** COUNSELOR reads each block against the code. A block that states a guarantee the
code does not give is rejected.

### Step 10: SPEC amendment

**Scope:** `eve/SPEC.md`
**Action:** COUNSELOR amends, each with ARCHITECT's words beside it and the mark
`[AMENDED 2026-11-10]`:
1. `:68-70` and `:284-293`: no `clear` command. Ctrl+L is a key that goes to the program.
   ARCHITECT: *"ctrl+l should not erase active prompt"*; *"we should adhere to shell SPEC"*;
   *"Remove the clear command"*.
2. `:118`: the word `clear` leaves the list of restart causes.
3. `:644-648` and `:679-684`: a grid change clears the live screen only while a prompt is active.
   ARCHITECT: *"Clear only at a prompt"*; *"nor resize (because creating new pane) not clearing
   whatever on live buffer because there is no active prompt yet"*.
4. `:912`: the acceptance line for `clear` becomes the Ctrl+L behaviour of item 1.
5. The swap section: a wrapped row is its own stored line, and the reader joins it at build.
   ARCHITECT: *"Reader joins at build"*.
**Validation:** every changed line carries a verbatim quote. No line states behaviour that the
code of Steps 1 to 7 does not have. No `version:` field changes.

### Step 11: Log

**Scope:** `eve/carol/SPRINT-LOG.md`, `eve/DEBT.md`
**Action:** log the sprint per `/log`. Drain `DEBT-20261108T145906` (Ctrl+L destroys the active
prompt, `eve/DEBT.md:11-15`) after its receipt. Write the commit messages for `jam` and for `eve`
in chat. Run no git command.
**Validation:** `/log` format. The receipt is written before the drain.

## Verification (ARCHITECT builds and tests)

1. `cast cast/spell.md --debug` in `eve` (`eve/CLAUDE.md:78`), then the END build.
2. Crash path: in a pane, print output that wraps and output from `ls` in one command, then scroll
   the history while more output arrives. No crash report appears in
   `~/Library/Logs/DiagnosticReports`.
3. Wrapped history: `cat` a file with lines longer than the pane. Resize the pane narrower and
   wider. Each long line reflows as one line.
4. Running CLI: start Claude Code, then split a new pane. The rows of the first pane stay.
5. New pane: split a pane. The text before the first prompt stays.
6. Prompt: at a zsh prompt, resize the pane. The screen clears and the prompt redraws at the top.
7. Ctrl+L at a zsh prompt: the screen clears and the prompt redraws. Ctrl+L in Claude Code on the
   normal screen: the prompt stays (`eve/DEBT.md:15`).
8. A typed `clear` still hides the history.

## BLESSED Alignment

- **B** — each thread keeps its own data. The writer owns the segment files up to the release of
  `endLine`. The reader owns its document. The clock thread owns the frame.
- **L** — the join leaves the writer (`writeFromTerminalBuffer` loses about 30 lines). New
  functions follow their siblings in size. No new file.
- **E (Explicit)** — `previousGridStart` reaches `addLines` as a parameter. No hidden read.
- **S (SSOT)** — the end line is published once, as `endLine`; `numReadRows` has no second reader.
  The frame has one author, the TTY bytes; the decoder clears it only on the ruled prompt case.
- **S (Stateless)** — no new member. State that the view needs is a Model row (ruling 2).
- **E (Encapsulation)** — a stored line is complete when `endLine` releases it, and no thread
  edits it. The join is creation, in the reader's build. Data moves one way: clock, ring, writer,
  file, reader.
- **D** — the grid invariant is set once, by the writer. The reader trusts it with no guard.
- **Deadline thread** — no step adds code to the clock tick.

## Risks / Open Questions

None open. Outside this plan, by ARCHITECT's word nothing is deferred: the wrap of the prompt by 1
to 2 cells has no step, because no read gives the two winsize values at run time. The reads show
the mechanism: the grid changes at `EVETerminalProcessor.cpp:465` before the child learns it at
`:474`, and a new pane can write two winsize values in a short time (`jam_TextEditor.cpp:128`,
`:1350`, `EVEView.cpp:21-27`, `:89-92`). Step 6 removes the blank frame and the homed cursor from
that sequence for a pane with no prompt. A whatdbg trace of `jam_TextEditor.cpp:1375` and
`EVETerminalProcessor.cpp:458` gives the values when ARCHITECT asks for it.
