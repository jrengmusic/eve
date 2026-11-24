# PLAN: rendering and live rows

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-24
**BLESSED Compliance:** verified against the reads cited below
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM

## Overview

This plan fixes the five defects that ARCHITECT reported:

- **[1]** Emoji render monochrome on Windows.
- **[2]** A `cast` build prints gibberish on Windows.
- **[3]** The Claude Code plan dialog is stuck.
- **[4]** Block backgrounds move, break, and stop at the text length.
- **[5]** The screen goes blank in `/tui classic`.

[3] and [5] have one cause. ARCHITECT, on scope: *"END is in scope"*. ARCHITECT, on the order: *"NO AUDIT, until runtime proven fixed"*.

## Evidence

- **[1]** `rasterizeEdgeTable` accepts only an image layer (`jam_GlyphRasterizer.cpp:466`). `juce::Typeface::getLayersForGlyph` gives `ColourLayer` items for `seguiemj.ttf` (COLR v0 records).
- **[2]** A plain `cmake -P` turns `e29481` into `ce93c3b6c3bc`. With policy CMP0176 NEW the bytes stay `e29481`. Two call sites run `cast`: `eve/cast/cmake.cast:634` and `end/cast/cmake.cast:564`.
- **[3], Claude Code side:** in a private cygwin pty, Claude Code 2.1.331 fullscreen shows the options and scrolls (`scratchpad/ccrun/full1`, `full3`). In classic, 153 plan rows go to history and the options are live rows (`ccrun/classic1/snap_02_dialog.txt`). Session `abb6310b`: `/tui classic` 19:35:05Z, plan of 202 lines rejected 19:44:35Z, `/tui fullscreen` 19:45:07Z, the same plan approved after 1.6 s.
- **[3] and [5], EVE side:** the live frame writes `numHistoryRows` (`jam_TerminalBuffer.cpp:430`). `hasLive()` waits until history reaches it (`jam_TextEditor.cpp:1623-1628`). History reads only closed lines, and the last stored line stays open (`jam_TerminalSwap.cpp:162-169`). Thus one scrolled row hides the live block until OSC 133 D.
- **[4], paint:** an erased cell has codepoint 0 (`jam_TerminalProcessor.cpp:676-690`). Codepoint 0 gives no glyph (`jam_GlyphArrangement.cpp:348`). A background exists only for a glyph (`:576-582`, `:650`). Each run fills its backgrounds just before its own glyphs (`:682-701`), so a later run paints over earlier glyphs.
- **[4], reflow:** U+0000 goes into the line break rules as a codepoint (`jam_ReflowDocument.cpp:580`). A segment has no trailing blanks (`:498-513`). Thus the rows of a wrapped line do not cover every cell. The pad segment (`jam_AnsiDocument.cpp:214-215`) can make an extra row.
- **[4], width:** ARCHITECT: *"either quoted user prompt, or diff (red/green) should not stop at text length. but always full winsize width"*. No record field says that a background reached the edge.

## Rulings that this plan keeps

| Ruling (verbatim) | Source |
|---|---|
| *"Writer publishes closed end (Recommended)"*: TerminalSwap publishes the end of its last complete logical line, and each block is built once up to it | M2, `PLAN-message-thread-materialise.md:22` |
| *"REFLOW, wrapping or unwrapping are NEVER ever affected by SIGWINCH nor winsize. it's always GUI exclusive getLocalBounds() only"* | SPEC principle 13 |
| *"all ... one lane, unidirectional SSOT"*; *"same content bound into same cell"* | ARCHITECT prompt, [4] |
| Orchestrator tells. The worker holds no atomic and no decision getter. | SPEC `[AMENDED 2026-11-19]` Told workers |

## Framework Inventory

### API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| Advance of one atom | jam_graphics | `GlyphArrangement::getAdvance (text, index, fonts, metrics)` | `jam_GlyphArrangement.cpp:263` | 2 |
| Colour of a colour id | jam_graphics | `jam::Colours::getColour (laf, id, colour)` | `jam_GlyphArrangement.cpp:621-625` | 2 |
| Rectangles of one colour | JUCE | `juce::RectangleList<float>`, `Graphics::fillRectList` | `jam_GlyphArrangement.cpp:684-688` | 2 |
| Segments with one token at least | jam_document | `ReflowDocument::getCellSegments` | `jam_ReflowDocument.cpp:600-608` | 3 |
| Packed rows | jam_document | `ReflowDocument::setRows (line, width, cutWidth, measure)`, `Document::Token::setSpan` | `jam_ReflowDocument.cpp:778`, `:650` | 3 |
| Style lane of a line | jam_style | virtuals of `StyleCustom` | `jam_StyleCustom.h:98-125` | 3 |
| Per-slot row mark | jam_terminal | `wrappedRows`, `setWrapped`, `isWrapped` and siblings | `jam_TerminalBuffer.cpp:24-29`, `:56-61`, `:137-157`, `:293-304`, `:357-381` | 4, 5 |
| Row record | jam_terminal | `TerminalBuffer::writeRow`, `getNumBytes` | `jam_TerminalBuffer.cpp:396-474` | 4 |
| Document attribute | jam_core | `Element::add<bool>`, `get<bool> (id, default)` | `jam_TerminalHistory.cpp:33`, `jam_StyleCustom.cpp:88` | 4 |
| Colour layers | JUCE | `juce::ColourLayer::clip`, `::colour`, `juce::ImageLayer` | `juce_Typeface.h:40-66` | 6 |
| Fill an edge table | JUCE | `EdgeTable::translate`, `EdgeTable::iterate`, `RenderingHelpers::EdgeTableFillers::SolidColour<PixelARGB>` | `juce_EdgeTable.h:82`, `:112`; `juce_RenderingHelpers.h:510-565` | 6 |
| Child output encoding | CMake | `execute_process (... ENCODING UTF-8)`; minimum version 4.2.0 | `eve/project-info.md:58` | 7 |

### Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| A call builds all runs, paint only draws | `getDrawRuns` `jam_GlyphArrangement.cpp:660-670` | `jam_TextBlock.cpp:282`, `:293`; `jam_MarkdownGraphics.cpp:215`, `:474`; `jam_MermaidGraphics.cpp:634`, `:767` | 2, 4 |
| `getCodepoint` gives a space for a cell with no own text | flexGap, `jam_AttributedString.cpp:353-356` | 13 sites (ling findReferences) | 1 |
| A style virtual with an empty base, one override | `StyleCustom` draw virtuals, `jam_StyleCustom.h:28-48` | `StyleTerminal`, `StyleMarkdown` | 3 |
| A per-slot array rotates, resizes, clears and swaps with its row | `wrappedRows`, `closedRows` | `jam_TerminalBuffer.cpp:28-29`, `:60-61`, `:301-302`, `:366-367`, `:376-377` | 4, 5 |
| The frame header tells the reader one number | `jam_TerminalBuffer.cpp:430`, `jam_TerminalHistory.cpp:292` | `jam_TextEditor.cpp:1626-1627` | 5 |
| A colour glyph goes through one image layer | `jam_GlyphRasterizer.cpp:466-480` | `getEmojiGlyphBounds`, `fitEmojiToCellBox`, `getEmojiGlyphImage` | 6 |

### Names

| Family | Members | Intent | Step |
|---|---|---|---|
| Atom predicates | `AttributedChar::isSameStyle` (`jam_AttributedChar.h:367`) → new `hasBackground() const noexcept` | `has` answers | 2, 4 |
| `GlyphArrangement` helpers | `getFace`, `getTypefaceIndex`, `addGlyph` → new `getBackground`, `addBackgrounds` (replaces `addBackground`) | `get` returns, `add` inserts | 2 |
| Geometry parameter | `left`, `top`, `origin` → new parameter `right` | right edge, logical pixels | 4 |
| `StyleCustom` row virtuals | `setLineAdvances` → new `setRowSpans` | `set` writes on the line | 3 |
| Per-slot row data of `TerminalBuffer` | `outputBlocks`, `wrappedRows`, `closedRows` → new `rowWidths`, `continuedRows` | one value per slot | 4, 5 |
| Row predicates of `TerminalBuffer` | `isWrapped (int)`, `isClosed (int)`, `getOutputBlock (int / Cell)` → new `isFilled (int)`, `isFilled (jam::Cell)`, `isContinued (int)` | `is` answers | 4, 5 |
| History counters | `numHistoryRows`, `endClosedLine` → new `endClosedRow` | number after the last row that D closed | 5 |
| Identifier vocabulary `cast/terminal.md` | `isRecording` :70, `isWrapped` :72 → new `isFilled` | line attribute | 4 |
| `GlyphRasterizer` getters | `getEmojiGlyphBounds`, `getMonoGlyphImage` → new static `getImageLayer` | returns the layer | 6 |

### Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| Use the framework API fully | Tokens, rows and advances come from `ReflowDocument` and `GlyphArrangement` | `jam_TextBlock.cpp:159-170` |
| No fake-carrier structs | `Run` is the materialisation. No new struct. | `jam_GlyphArrangement.h:135` |
| One object, one translation unit | Bodies in the object's `.cpp` | `jam_StyleTerminal.cpp` |
| No out-parameters | `std::optional` and values | `StyleCustom::getFixedRowHeight` |
| Deadline threads | Per-slot arrays are sized in `prepare`. `scrollUp` allocates nothing. | `jam_TerminalBuffer.cpp:43-72` |

### Absence

| Need | Query | Result |
|---|---|---|
| JAM fill of an `EdgeTable` into an ARGB image | `workspaceSymbol fillEdgeTable` | JUCE only (`juce_RenderingHelpers.h:2265`) |
| A row fact "background reached the edge" | reads of `writeRow`, `TerminalHistory::addLine` | no field |
| Writers of `bgColourId` outside the terminal lane | `bgColourId` sites | terminal and ANSI only. Markdown and Mermaid atoms have no background, so they never get a background run. |

## Validation Gate

COUNSELOR validates each step against MANIFESTO.md, NAMES.md, CODING.md and this plan, before the next step.

ARCHITECT's order: **no audit before the runtime proof** (step 8). After it, @Auditor runs once. The doxygen pass follows the audit. No comment and no doxygen before that. Each Engineer prompt pastes its inventory rows. Engineer builds nothing. ARCHITECT builds.

## Steps

### Step 0: Write the plan to the EVE root
**Scope:** `D:\Poems\dev\eve\PLAN-rendering-and-live-rows.md`.
**Action:** COUNSELOR copies this approved plan there.
**Validation:** the two files are equal.

### Step 1: An erased cell reads as a blank [4]
**Scope:** `jam_core/text/jam_AttributedString.cpp`, `jam_terminal/document/jam_AnsiDocument.cpp`, `jam_graphics/fonts/jam_GlyphArrangement.cpp`.
**Action:**
1. `AttributedString::getCodepoint` returns `Chars::space` for a `contentCodepoint` atom with codepoint 0 that is not a `spacerTail`. A spacer tail keeps 0.
2. Remove the tests that can no longer be true: `codepoint == 0 or` in `AnsiDocument::isBlank` (`:227`) and `followingCodepoint == 0 or` in `getCellSpan` (`:93`).
3. `jam_TextEditor.cpp:445` and `applyToCodepoints` stay. They see spacer tails.

**Inventory:** API "segments", pattern "`getCodepoint` gives a space".
**Validation:** each of the 13 sites is read. No U+0000 reaches the line break rules, except on a spacer tail, which must join its wide cell.

### Step 2: Backgrounds come from the cells and paint first [4]
**Scope:** `jam_core/text/jam_AttributedChar.{h,cpp}`, `jam_graphics/fonts/jam_GlyphArrangement.{h,cpp}`.
**Action:**
1. `bool AttributedChar::hasBackground() const noexcept`: `bgColourId` is not `colourIdNone`, or `inverse` is 1.
2. Delete `addBackground`. Delete its call (`:650`).
3. `static std::optional<juce::Colour> getBackground (AttributedChar, const juce::LookAndFeel&, juce::Colour)`: the fill colour of an atom, or no value. An inverse atom gives its foreground, as `addGlyph` does today (`:621-628`).
4. `addBackgrounds (runs, text, startIndex, endIndex, origin, fonts, metrics, advances, laf, colour) const`: joins neighbour atoms with an equal fill into one rectangle. The rectangle uses the told advances, or `getAdvance (text, index, fonts, metrics)` when none are told. It adds the rectangle to the background run of that colour. A background run has empty glyph arrays, and the typeface index and the font size of its first atom.
5. `getDrawRuns` calls `addBackgrounds` before the glyph loop, so every background run comes before every glyph run.
6. `addGlyph` sets no background colour. `Run::matches` has no background colour. `Run::paint` calls `drawGlyphs` only when the run has glyphs.

**Inventory:** API rows "Advance", "Colour", "Rectangles"; pattern row 1; names `hasBackground`, `getBackground`, `addBackgrounds`.
**Validation:** a blank cell has a background. A glyph that spans blanks does not change the background of those blanks. One rectangle per neighbour span, so no seam. The order is backgrounds, then glyphs. No background run reaches `Svg::Format::text` (`jam_SvgFormat.cpp:165`), per the Absence row.

### Step 3: Terminal rows cover every cell [4]
**Scope:** `jam_style/jam_StyleCustom.{h,cpp}`, `jam_terminal/style/jam_StyleTerminal.{h,cpp}`, `jam_gui/widgets/jam_TextBlock.cpp`, `jam_terminal/document/jam_AnsiDocument.cpp`.
**Action:**
1. `AnsiDocument::addSegments`: the tokens are `ReflowDocument::getCellSegments (cells)`. The pad segment goes.
2. `virtual void StyleCustom::setRowSpans (Document::Element& line) const`: empty base.
3. `StyleTerminal::setRowSpans` override: the first row starts at cell 0. Each row ends where the next row starts. The last row ends at the last cell.
4. `TextBlock::setTextRows` calls `style.setRowSpans (line)` after `ReflowDocument::setRows` and before `addRowHeights`.

**Inventory:** API rows "Segments", "Packed rows", "Style lane"; pattern row 3; name `setRowSpans`.
**Validation:** blanks at a row end hang past the edge and make no row. Text and background use the same row spans. `StyleMarkdown` has no override, so markdown and Mermaid rows do not change. `isGridLine`, `getCell`, `getCellBounds` and `paintColumns` are read against the new spans.

### Step 4: A background that reached the edge fills the width [4]
**Scope:** `jam/cast/terminal.md`, `jam_terminal/buffers/jam_TerminalBuffer.{h,cpp}`, `jam_terminal/document/jam_TerminalSwap.{h,cpp}`, `jam_TerminalHistory.cpp`, `jam_AnsiDocument.cpp`, `jam_gui/widgets/jam_TextBlock.{h,cpp}`, `jam_graphics/fonts/jam_GlyphArrangement.{h,cpp}`, and the callers `jam_MarkdownGraphics.cpp:474`, `jam_MermaidGraphics.cpp:767`.
**Action:**
1. Add the row `isFilled` to `jam/cast/terminal.md`. The generated files come from `cast`, never by hand.
2. `jam::Array<int> rowWidths` in `TerminalBuffer`, beside `closedRows`: rotate, resize, swap, and both `clear` functions set `widthCols`.
3. `isFilled (int slot) const` and `isFilled (jam::Cell row) const`: the row holds at least its width in atoms, and its last atom has a background. The ring uses `rowWidths`. The alternate grid uses `widthCols`.
4. `writeRow` takes `bool isFilled` after `isWrapped` and writes it last. `getNumBytes` counts the byte. The live lambda and `TerminalSwap::writeLine` pass the value.
5. `TerminalHistory::addLine` reads it into `Id::isFilled`. `addWrappedLine` takes the value of its last row.
6. The ten-argument `getDrawRuns` takes `float right` after `advances`. `addBackgrounds` extends the last rectangle to `right` when the last atom has a background. The eight-argument overload, the grid lane, markdown and Mermaid pass `0.0f`.
7. `TextBlock::addRuns` takes the right edge (`indent + contentWidth`). It passes the edge for each row of a line with `isFilled`.

**Inventory:** API rows "Per-slot row mark", "Row record", "Document attribute"; pattern rows 1 and 4; names `rowWidths`, `isFilled`, `right`.
**Validation:** each `closedRows` site has its `rowWidths` sibling. The record reader and writer agree byte for byte. A badge that ends before the edge does not fill. The fill is correct at each view width, live and history.

### Step 5: The live block waits only for the closed line of OSC 133 D [3] [5]
**Scope:** `jam_terminal/buffers/jam_TerminalBuffer.{h,cpp}`, `jam_terminal/document/jam_TerminalSwap.cpp`, `jam_TerminalHistory.cpp`, `jam_gui/widgets/jam_TextEditor.cpp`.
**Action:**
1. `juce::int64 endClosedRow { 0 }` in `TerminalBuffer`. In `scrollUp`, when a row goes to history and row 0 is closed, `endClosedRow` is `numHistoryRows + 1`, before the increment.
2. The frame header writes `endClosedRow` in place of `numHistoryRows`.
3. The live root attribute is `Id::closedLine` in place of `Id::firstLine` (`jam_TerminalHistory.cpp:292`, `jam_TextEditor.cpp:1626-1627`). `hasLive()` waits only for that line.
4. `jam::Array<bool> continuedRows` and `isContinued (int slot) const`, beside `closedRows`. `scrollUp` sets it for the row that goes to history: the wrapped mark of the next row. In a grid of one row the mark is true, because the wrap mark comes after the scroll.
5. `TerminalSwap::writeNextLine`: after the store, the closed line is `line + 1` when the row is closed, or when it has no grid start and is not continued. The rule at `:162-163` stays for grids.

**Inventory:** API row "Per-slot row mark"; pattern rows 4 and 5; names `endClosedRow`, `continuedRows`, `isContinued`.
**Validation:** after a scroll with no D, `hasLive()` is true and no history line is missing above the live block. After D, the live block still waits for history (no prompt tear). A grid run still closes at its end. Single writer, no new atomic, no allocation on the clock.

### Step 6: COLR layers make one colour image [1]
**Scope:** `jam_graphics/fonts/jam_GlyphRasterizer.{h,cpp}`.
**Action:**
1. `static juce::GlyphLayer getImageLayer (std::vector<juce::GlyphLayer> layers)`: the union of the `clip` bounds gives an ARGB image. Each clip moves to the image origin and fills with its colour through `EdgeTableFillers::SolidColour<juce::PixelARGB>`. A layer with no colour fills white, as `getMonoGlyphImage` does (`:429`). The result is an `ImageLayer` with a translation to the bounds position.
2. `rasterizeEdgeTable`: an image layer is used as it is. Layers where a non-empty `ColourLayer` has a colour go through `getImageLayer`. Both then use the one emoji path. Every other glyph goes to `getMonoGlyphImage`, as today.

**Inventory:** API rows "Colour layers", "Fill an edge table"; pattern row 6; name `getImageLayer`.
**Validation:** the Apple bitmap path does not change. A text-style glyph with no colour layer stays mono. The atlas gets an ARGB image (`jam_GlyphAtlas.cpp:84`).

### Step 7: CMake decodes `cast` output as UTF-8 [2]
**Scope:** `eve/cast/cmake.cast:634`, `end/cast/cmake.cast:564`.
**Action:**
1. COUNSELOR first runs the saved experiment with `ENCODING UTF-8` in place of the policy line. The bytes must stay `e29481`. If they do not, the edit uses the policy line that is already proven.
2. Add `ENCODING UTF-8` to the two `execute_process` calls. The generated `.cmake` files come from the next `cast` run, never by hand.

**Inventory:** API row "Child output encoding".
**Validation:** byte check in the scratchpad. No other `execute_process` changes.

### Step 8: Runtime proof
**Scope:** none. ARCHITECT builds and runs.
**Action:** COUNSELOR gives ARCHITECT one list:
- **[1]** `printf` of colour emoji.
- **[2]** A `cast` build of EVE and of END.
- **[3] [5]** Claude Code `/tui classic` with a plan higher than the window; then `/tui fullscreen`.
- **[4]** `cat` of the captured byte files in `scratchpad/ccrun`; scroll; resize; a red and green diff; a quoted prompt; history after a resize on mac.

A failed item goes back to its step. @Auditor, the resolution of its findings, and the doxygen pass run only after ARCHITECT confirms.

## BLESSED Alignment

- **B:** each new row value has one owner, `TerminalBuffer`, and the lifetime of its slot.
- **L:** no new struct, no new file. `addBackgrounds` is one responsibility.
- **E (Explicit):** `right`, `isFilled`, `endClosedRow` state their meaning. No magic value.
- **S (SSOT):** text, background, caret and selection read the same row spans. One emoji path.
- **S (Stateless):** the reader holds no new state. The frame tells its closed line.
- **E (Encapsulation):** the style selects the lane. The decoder tells, the swap writes, the view reads.
- **D:** the live block depends on one written number, not on a race between two lanes.

## Risks / Open Questions

None.
