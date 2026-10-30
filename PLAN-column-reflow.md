# PLAN — Column Reflow of Terminal History Grids

**Status:** Locked by ARCHITECT `/go` (2026-10-30).

## Objective

A column grid in terminal history reflows to the TextEditor content width on every resize. The producer does not matter.

ARCHITECT, verbatim:
- *"i just need ls, eza, whatever column on terminai history always layout symetrical, beautifully, deterministically, without ever losing any performance."*
- *"no hardcode about ls nor eza."*
- *"this is about history file lists columns, i'm 100% sure the pattern is deterministic"*

## Rulings

1. **Width.** Each column has the width of its own widest entry. Every column is followed by the same gutter. The remaining width goes to the end of the line.
   ARCHITECT, superseding *"Widest entry, uniform"*: *"the column dividers always divide equally which looks lazy. the calculation should also consider the gutter? so make it equal space between column? then the reminder could be added to EOL as whitespace if it not zero"*.
   The row count is the smallest count whose layout fits the content width.
2. **Order.** The original grid gives the order of the entries. One layout rule places them at every width. The original form is not a separate case.
   ARCHITECT: *"why dont we 'capture' the original state as reference? so we always know what is the actual order?"*;
   *"resize to > 60, more column, less row - resize to < 60, less column, more row"*;
   *"no need original form. if the layout is most deterministic with ONE SSOT layout reflow rule"*.
3. **Entry tag.** A gutter cell is an `AttributedChar::flexGap` atom. Its `codepoint` field holds the order index of the entry.
   ARCHITECT: *"pack (bitshift) item order in its flexGap metadata"*.
   Facts: the 21-bit `codepoint` of a flex atom has no other meaning (`jam_AttributedChar.h:20`, `:155`).
   A gap of n cells is n flex atoms, so the gap width needs no field.
   One value needs no `jam::Union` (`jam_Union.h:41`).
4. **Layout engine.** `juce::FlexBox` with component-free `FlexItem` (`juce_FlexBox.cpp:765-790`, `:806`).
5. **Scope.** Every grid, of any height.
   ARCHITECT answer: *"Every grid, any height"*.
6. **Block mark.** Each row carries the number of its command output block in the row record.
   ARCHITECT answer: *"Row record (Recommended)"*.
7. **Grid line.** One grid block is one document line. A lane reads to the end of its last block.
   ARCHITECT answer: *"One block, one line (Recommended)"*.
8. **Recognition rule.** Inside one block, every row splits at gutters of 2 or more blank cells, and the cell starts sit at the same columns in every row.
   ARCHITECT answer: *"Yes, this rule"*.

## Steps

### Step 1 — Block mark in the row record (jam_terminal)

1. `jam::TerminalProcessor` counts the output blocks. OSC 133 C starts a new block (`jam_TerminalProcessor.cpp:845`).
2. While `isRecording`, each row that the processor writes text into gets the number of the current block.
3. `jam::TerminalBuffer` keeps the block number of each ring slot. A reused or cleared slot has no block.
4. `TerminalBuffer::writeRow` stores the block number after the image placements. `getNumBytes` counts it.
5. The live frame writes the block number of each frame row with the same record.
6. `TerminalSwap::addLine` reads the block number into the line as `Id::outputBlock`.
7. Identifier `outputBlock` joins `jam/cast/terminal.md` (family: OSC 133 `outputBlockStart`, `jam_Terminal.h:1385`).

### Step 2 — Recognition at the writer, merge at lane creation (jam_terminal)

The recognition moved from the lane reader to the swap writer.
A reader-side look-back cost O(L²) per lane. ARCHITECT: *"without ever losing any performance"*.
ARCHITECT, on lanes and index files: *"cant the logic implement in chunks? so after 256 started new chunk then merge?"*.

1. `TerminalSwap::writeFromTerminalBuffer` computes the cell starts of each row (`AnsiDocument::getCellStarts`).
2. It compares them with the previous row (`AnsiDocument::isGridStart`, `isGridRow`). It writes the first line number of the grid run as `gridStart` in the row record. A row outside a grid writes `noGridStart`.
3. `TerminalSwap::addLine` reads `Id::outputBlock` and `Id::gridStart` into the line.
4. `TerminalSwap::addLines` reads the lane range over every index file it crosses (`addRangeLines`).
5. `removeGridHead` removes the head of a grid that starts before the lane. A grid that is not whole loses its `gridStart`.
6. `addGridLines` reads the tail of the last grid chunk by chunk, one time. The grid is whole when the next line has another block. A grid that is not whole loses its `gridStart`.
7. When the look-ahead reaches the swap end with the chain whole, the lane is open. The document root gets `Id::end` = the swap end.
8. The document root `Id::size` holds the number of stored rows. The lane length check compares it.
9. `AnsiDocument::addGrids` merges each run of one `gridStart` into one line of entries.
10. Each gutter becomes `flexGap` atoms. The `codepoint` of a gutter atom holds the order index of its entry, read down each original column. An overflow of the 21-bit field asserts.
11. `TextEditor::addLanes` builds a lane again when its length changed, or when the lane is open and the swap end moved.

### Step 3 — FlexBox layout (jam_gui)

1. Each entry has the width of its own text. The gutter is the right margin of each entry (ruling 1).
2. On each layout, `juce::FlexBox` places the entries in columns with wrap, sorted by order index. The layout height has a tolerance of half a row.
3. `getNumRows` finds the smallest row count that fits the content width by binary search.
4. One rule at every width: wider gives more columns and fewer rows, narrower gives fewer columns and more rows.
5. The caret, the selection and the hit-test of a grid line read the entry positions (`Id::left`, `Id::top`).

### Step 4 — Verification by ARCHITECT

1. Build and run.
2. Run `ls` (or any grid producer). Resize narrower and wider. The grid keeps uniform columns and its entry order at every width.
3. Right-aligned tables (`ls -l`, `ps`, `df`) stay as printed.
