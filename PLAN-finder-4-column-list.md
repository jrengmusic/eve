# PLAN: Finder Part 4 — Column Lists Are TextEditor State

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 2 to 4, 6, 8, 10

## Overview

TextEditor records each output block that names real files as a column list, with its own directory. One list is active, and only it shows links. A new column list becomes active. The next command no longer removes the links.

## Language / Framework Constraints

- The tick never touches this state. The record is a message-thread write, in the shape of the `images` tree.
- The filesystem decides what is a file (`SPEC.md:76-81`).

## Dependency & API Inventory

**Output blocks:**
- OSC 133 C increments `outputBlock` and starts recording (`jam_TerminalProcessor.cpp:1181-1184`). OSC 133 D ends it and scrolls the live rows into history (`:1199-1207`).
- Each stored row carries its output block (`jam_TerminalBuffer.cpp:343-359`, `jam_TerminalSwap.cpp:117`), and each read line has `Id::outputBlock` (`jam_TerminalSwap.h:116`).
- The Model has `outputBlock`, `isRecording`, `cwd` rows (`parameters.md:45`, `:35`, `:53`). OSC 7 runs in the shell's precmd, before the next command (`zsh_integration.zsh:7-9`, `bash_integration.bash:38`, `powershell_integration.ps1:15`).
- The tick stores a row with `storeValue`, and the next `flush()` writes it to the tree (`jam_Model.h:994-1011`).

**Links:**
- `AnsiDocument::addFileLinks (document, outputBlock, directory)` tests each candidate against the filesystem (`jam_AnsiDocument.cpp:308-360`). A grid line links each entry, and any other line links its text and words (`:332-345`).
- `TextEditor::addFileLinks` calls it with the last block and the `cwd` row, and stores the block on the root (`jam_TextEditor.cpp:936-942`).
- `isLinkChanged` rebuilds a resident block when its stored block differs (`:1023-1025`).

**Residency:** blocks of 256 lines (`jam_TerminalSwap.h:28`). `addBlocks` builds from one block above the first visible block to the tail (`jam_TextEditor.cpp:1011-1016`). `getCellBounds` asserts a resident line (`:1371`). `addBlock` shifts the cursor and the view when a block joins above (`:884-892`). `removeHeadBlocks` drops a block the swap lost (`:895-928`).

**Model tree shape, established:** the `images` component state: one child per box, cleared at bind, read by a listener (`jam_TextEditor.cpp:93-94`, `jam_PreviewComponent.cpp:288-298`).

**Scroll, established:** a cursor change keeps the cell in view (`jam_TextEditor.cpp:803-811`).

**New names (ARCHITECT's term):** `columnLists`, `columnList`. `activeList`, a row of the same tree, follows `Id::cursorLine`-style noun rows. `getActiveOutputBlock` replaces `getLastOutputBlock` (`jam_TextEditor.h:902`).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: The state
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`, `jam/cast/core.md` or the `@id` table that holds the sibling rows
**Action:** `bindToModel` takes `columnLists = model.getOrCreateComponentState (Format::toTag (Id::columnLists))` and clears its children, as `images` does (`jam_TextEditor.cpp:93-94`). Each child is one `Id::columnList` with `Id::outputBlock`, `Id::cwd` and `Id::line`, the first block number of the list. The tree holds the row `Id::activeList`, an output block number.
**Validation:** Ruling 3. Value data only. No fake carrier: this is Model state.

### Step 2: The directory of a block
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:** The listener of the `terminal` tree (`jam_TextEditor.cpp:797`) handles `Id::outputBlock`: it keeps the new block number and the `cwd` row for Step 3, in the `Id::outputBlock` row of `columnLists`, as the candidate.
**Validation:** OSC 7 precedes C in each shell script. See Risks for the flush order.

### Step 3: A candidate becomes a column list
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`, `jam/jam_terminal/document/jam_AnsiDocument.*`
**Action:** In `addBlocks (TerminalSwap&)`, when `isRecording` is off and the block holds lines of the candidate: test the candidate with the directory of Step 2, through the `addRangeLink` test (`jam_AnsiDocument.cpp:318-325`). A candidate with a link becomes one `columnList` child, and `activeList` takes its number.
**Validation:** Ruling 8, and ruling 4: *"latest column list"*. A block with no real file (`git status`) adds nothing and takes nothing. A test that adds links to a scratch document is creation (MANIFESTO **E**). The test reads the tail, which is resident (`jam_TextEditor.cpp:1013`).

### Step 4: Links follow the active list
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:**
1. `getLastOutputBlock` becomes `getActiveOutputBlock`, which reads `activeList`. The `isRecording` branch goes (`jam_TextEditor.cpp:932-933`).
2. `addFileLinks` takes the directory from the active `columnList` child, not from the `cwd` row (`:941`).
3. `isLinkChanged` compares with `getActiveOutputBlock` (`:1023`), so a change of `activeList` rebuilds each affected resident block with the existing path.

**Validation:** Ruling 2: not exclusive to the list above the prompt. `SPEC.md:373` goes (tracker, Step 2). `ls sub/` still resolves against the list's `cwd` (carried risk, `PLAN-3`).

### Step 5: Eviction
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:** `removeHeadBlocks` removes each `columnList` child whose `Id::line` block left the swap. When the active child leaves, `activeList` takes the latest remaining child, or `noOutputBlock`. `clear` empties the tree.
**Validation:** No list points at history that does not exist (`jam_TextEditor.cpp:906`).

### Step 6: Hand over, audit, doxygen
**Action:** As Part 1, Step 8.

## Acceptance (ARCHITECT builds and tests)

1. `cd end`, `ls`, `cd eve`, `ls`: only the second list shows underlines from EVE, and a click opens a file of `eve`.
2. `ls`, then `git status`: the `ls` list stays underlined and clickable.
3. `ls -l`, `fd .`, `find . -type f`: each is a column list.
4. A command with no real file output adds no list: `echo hello`.
5. A resize reflows and keeps the links.

## BLESSED Alignment

- **Bound:** TextEditor's Model state owns the list. The swap owns the bytes.
- **Lean:** the existing rebuild path serves the change of active list.
- **Explicit:** the directory is a named row of each list.
- **SSOT:** `activeList` is the one selection.
- **Stateless:** blocks hold links as creation output.
- **Encapsulation:** the test stays in `AnsiDocument`.
- **Deterministic:** the same history and filesystem give the same lists.

## Risks / Open Questions

- **Flush order.** `Model::flush` walks the adapters in map order, on a timer (`jam_Model.cpp:278-297`), so rows do not arrive in store order. A command line such as `ls; cd x` can deliver its `outputBlock` and the later `cwd` in one flush, so that list takes `x`. No name then resolves, and the list is not recorded. A typed command is safe, because OSC 7 runs long before the C mark. If ARCHITECT's run shows the limit matters, the fix belongs at the tick, as one more event at the C mark.
