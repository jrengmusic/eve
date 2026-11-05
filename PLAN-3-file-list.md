# PLAN: The Last Output Block Is the File List

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-05
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)

**Contract for each step (binds this plan):** ARCHITECT builds and tests: *"you dont build. architect build and test runtime"*. *"no new semantics, no magic strings allowed anywhere, no handroll state management, no manual arithmetic, no new pattern, use framework API to its fullest extent, follow established pattern verbatim"*. *"framework API(s) are rich jam first then juce. you probably only need to use it OOTB. if there's genuienely no abstract as SSOT you can use, extend the existing."* Data in transit has *"0 allocation, 0 copy, 0 mutation, 0 temporary container, 0 new struct, and 0 shadow state"* (`MANIFESTO.md:267`). *"we only use juce::String as Input or final Output materialisation to communicate with juce's API. otherwise we always use std::string/string_view everywhere."*

## Overview

After OSC 133 D, each entry of the last output block that names a real file gets a link and an
underline. A click opens it through `openFile`. No rule names `ls`.

ARCHITECT: *"when user run `ls` on active prompt, the last block of the list will serves as the file
list"*; *"when ls "active", OSC 133 D after last called ls, mark "openable" file with underscore,
hyperlink"*; *"must be SSOT, UNIVERSAL, GENERAL, not hardcoded to specific pattern"*; *"no hardcode
about ls nor eza"*.

## Language / Framework Constraints

SPEC principle 5, *"Terminal output text is never parsed"*, changes: the filesystem decides, and the
text is only the candidate. Step 0 records this with ARCHITECT's words.

## Dependency & API Inventory

**JUCE:** `juce::File::getChildFile`, `exists`, `juce::URL (juce::File)` (`juce_URL.cpp:157-189`).
**JAM:** `Id::outputBlock` on each line, and on the merged grid line (`jam_TerminalSwap.cpp:220`,
`jam_AnsiDocument.cpp:381`). `TerminalBuffer::noOutputBlock` (`jam_TerminalBuffer.h:61`).
`AttributedString::addLink`, `applyLinkUnderline`. `TextBlock::isGridLine`. Grid entries are `Id::tokens`
(`jam_AnsiDocument.cpp:341-363`). `getText` (`jam_TextEditor.cpp:5-16`). `Id::cwd` row (`parameters.md:57`).
The `Id::isRecording` property dispatch (`jam_TextEditor.cpp:743-748`).

**New names:** `getLastOutputBlock`, `addFileLinks`, `addRangeLink`. **New row:** `directory`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor
runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 0: Record
**Scope:** `SPEC.md:76-77`, `PLAN-Grand-Scheme.md`
**Action:** COUNSELOR amends principle 5 with the quotes above.

### Step 1: The last block
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:** Add a private `juce::int64 getLastOutputBlock() const;`. It walks `history`, and each root
child in reverse from the end of the block with the highest number, and returns the first
`Id::outputBlock` that is not `TerminalBuffer::noOutputBlock`, or `noOutputBlock`.
**Validation:** Only output rows hold a block (`jam_TerminalProcessor.cpp:449-450`). A result return.

### Step 2: Links at creation
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:**
1. Add a private `void addFileLinks (TextBlock& block);`. It runs in `addBlocks (TerminalSwap&)` before
   `setBlock (block, true)`, when `Id::isRecording` is off. For each root line whose `Id::outputBlock`
   equals `getLastOutputBlock()`:
   - a grid line: each `Id::tokens` entry is one candidate range;
   - any other line: the whole line, then each word between blanks, are the candidate ranges.
2. A private `void addRangeLink (AttributedString& cells, juce::Range<int> range);` reads `getText` as
   `std::string`, and makes a `juce::String` only at `juce::File::getChildFile`. When `exists()`, it
   calls `cells.addLink (range, { juce::URL { file }.toString (false), {} })`.
3. After the walk, the line calls `applyLinkUnderline()`.
4. A range with a link run is not a candidate, so OSC 8 stays.
**Validation:** This is creation (MANIFESTO): the block is not complete until `setBlock`. The block
stays on the message thread. One block bounds the stat calls. `ls`, `eza`, `fd`, `find` and `rg -l`
take one path.

### Step 3: The list ends at the next command
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:** In the `Id::isRecording` property handler, when the row turns on, erase the resident block
of the last output block with `history.erase`. `addBlocks` builds it again with no link, because the
row is on.
**Validation:** ARCHITECT: *"Add and remove operations are state updates"* (MANIFESTO). The swap holds
the bytes, so nothing is lost.

### Step 4: Directories
**Scope:** `eve/Source/layout/default-eve.md`, `eve/Source/EVEView.cpp`, `jam/cast/core.md`
**Action:** Add the row `directory | string | cd | The shell command of a directory`. `getOpenCommand`
takes the key `Format::toType (Id::directory)` when `file.isDirectory()`.
**Validation:** SPEC: *"Enter = cd the terminal"*. Add the `@id` row when none exists.

### Step 5: Hand over, Step 6: Audit, Step 7: Doxygen pass
As PLAN-1 Steps 10 to 12.

## Acceptance (ARCHITECT builds and tests)

1. `ls`, `ls -l`, `eza`, `fd .`, `find . -type f`: entries show an underline, and a click opens.
2. The next command removes the underline.
3. A click on a directory runs `cd`.
4. A resize reflows the grid and keeps the links.

## BLESSED Alignment

- **SSOT:** one link model: OSC 8 and the file list share `getLinkRuns`.
- **Deterministic:** the filesystem decides. No rule names a program.

## Risks / Open Questions

- `ls sub/` lists names relative to `sub`, so they do not resolve against `cwd`. The row holds no
  argument of the command.
