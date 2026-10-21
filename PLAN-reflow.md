# PLAN: EVE reflow — TextEditor is the only renderer; the harness prints from the AST

**RFC:** none. The objective comes from ARCHITECT's prompt and this session's rulings. `RFC-EVE-terminal-grand-scheme.md` is binding (SPEC.md:2).
**Date:** 2026-10-21
**Status:** approved by ARCHITECT (plan mode, 2026-10-21)
**BLESSED Compliance:** verified against MANIFESTO, CODING, NAMES (re-read this session)
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md C++/JUCE: one object, one TU)

## Context

ARCHITECT: *"this sprint focus to make text reflow as robust and as fast as possible. reflow works but why it capped at certain column? not at window width?"* and *"it at least need 2-3s from blank window to get the text rendered, and it's blocking the message thread."*

Cause of the cap (reads this session):
- `load` lays the harness out in a `MarkdownComponent` at `StyleMarkdown::getColumnWidth()` (`EVETerminalProcessor.cpp:63`, `:67`), which is `maxLineWidth 100em` (`style.css:842`). It sizes the engine grid to the same width (`:75`).
- The engine ends every row with `\n` (`jam_TerminalGraphicsEngine.cpp:203`). AnsiDocument makes one line for each `\n` (`jam_AnsiDocument.h:59`).
- TextEditor reflows each line by itself (`jam_TextEditor.cpp:146-152`). It can split lines but cannot join them.
- This breaks rulings 43 and 44 and PLAN-launch V5/V6 (`PLAN-launch.md:239`, `:244`). Audit 2 resolution A08 (`:321`) accepted the break by misreading ruling 56.

Rulings this session (ARCHITECT, verbatim):
- R1 *"css width is necessary to set baseline, a default "config". but without it reflow at Component side should just always adhere to whatever available space from juce::Component::getLocalBounds().getWidth()"*; `maxLineWidth`: **"Unset means bounds"**.
- R2 *"mermaid diagram, images, tables, follow exactly the same logic … images could simply use AffineTransform or juce::Rectangle API to fit"*.
- R3 *"in EVE reflow should always the FINAL materialiser, the renderer, the COMPONENT. the TextEditor"*.
- R4 *"Table should wrap each column"*; *"each table column can be treated exactly like whole component "column""*; *"only real table"*.
- R5 *"ideally TextEditor just call ReflowDocument API to reflow ANYTHING, SSOT"*.
- R6 *""one line" is the data. but rendered reflow adding/removing lines"* (confirmed: rows grow and shrink, lines stay).
- R7 Table mark: **"OSC 133 marker"**. Mark writer: **"Print from the AST"**. Decorations: **"Layout properties"** (SPEC.md:185-188 cell-bearing element).
- R8 *"all "terminal" concern always at Processor (audio thread) hot path? lock free"*.

Binding constraints:
- SPEC.md:255: *"❌ Reader thread: ValueTree, Document, mutex, allocation, block, yield, sleep"*. RFC:820 binds this rule to `processBlock`.
- Therefore the print from the AST runs in the cold path (the tell, inside `suspendProcessing`; ruling 48). It writes the engine row ring. `process()` only serialises prepared rows, as it does today (`jam_TerminalGraphicsEngine.cpp:206-233`).

## Language / Framework Constraints

- One object, one `.cpp`. Templates and `constexpr` in headers (CODING TU section).
- jam_gui must not depend on jam_markdown or jam_markdown_graphics (ruling 7; `PLAN-launch.md:326`). TextEditor draws decorations from generic line properties only.
- The vocabulary comes from cast tables (`cast/terminal.md`). Generated headers are never edited by hand.

## Dependency & API Inventory (reads this session)

- **AST lines:** `MarkdownDocument::addLines()` (`jam_MarkdownDocument.h:396-409`), `addLine` (`.cpp:1006-1036`, keys `Id::from`, `Id::tokens`, `Id::cells`, `Id::images`, `Id::row`, `Id::indent`, `Id::padding`, `Id::height`), table rows with child cell lines that carry `Id::left` / `Id::width` fractions (`.cpp:1112-1137`), object lines for rule, mermaid and image (`.cpp:1067-1079`), gap lines (`.cpp:992-993`).
- **AST colours:** cells hold `map::ColourId` LookAndFeel ids (`.cpp:1139-1146`, `:1168-1197`). The paint resolves them through `laf.findColour` (`jam_GlyphArrangement.cpp:520`, `:592`; `jam_ColoursUtilities.cpp:104-108`). The terminal maps colours with `GraphicsEngine::getOrCreateColourId` (`jam_TerminalGraphicsEngine.cpp:83-108`), the same call `GraphicsContext::drawGlyphs` uses (`jam_TerminalGraphicsContext.cpp:637`).
- **Row writes:** `AttributedString::resize`, `set (index, atom, source)`, `addColourId` (`jam_AttributedString.h:95`, `:132`, `:207`). The sibling is `drawCodepoints` (`jam_TerminalGraphicsContext.cpp:664-683`).
- **Style metrics (single author):** `StyleMarkdown::getLineIndent`, `getBlockIndent`, `getLinePadding`, `getGapHeight`, `getLineMetrics` (`jam_StyleMarkdown.cpp:256-416`). Optional-key read: `StyleManager::getConfig (sheet).getProperty (key, default)` (`jam_StyleMarkdown.cpp:155-160`).
- **Decoration geometry (single author):** `MarkdownGraphics` constructions (`jam_MarkdownGraphics.cpp:53-78`, `:127-146`, `:191-231`, `:270-278`). Pills: `getPills()` (`:43-51`, private today).
- **Images:** `MarkdownDocument::getImage` (`jam_MarkdownDocument.h:410-477`), `ImageLoader::getPng` returns the file bytes when the loader holds them (`jam_ImageLoader.h:76-85`), mermaid raster `MarkdownComponent::getOrCreateDiagramImage` (`jam_MarkdownComponent.cpp:310-333`), cell size `StyleMarkdown::getCellBounds` (`jam_StyleMarkdown.cpp:58-76`).
- **Wire:** `appendRowText`, `appendInlineImage`, `appendImageArgument`, `appendOscIntroducer` (`jam_TerminalEncoder.h:46`, `:149`, `:183`, `:197`). OSC dispatch is `oscConstruction` (`jam_AnsiDocument.cpp:280-301`). Argument parse is `TerminalITerm2Decoder::forEachArgument` (`jam_TerminalITerm2Decoder.h:91`). OSC 133 vocabulary: `map::OSC::shellIntegration` (`generated/jam_Terminal.h:229`), sub-command table `## ShellIntegration` (`cast/terminal.md:1057-1068`, letters A–D).
- **Engine:** per-slot `ImagePlacement` (`jam_TerminalGraphicsEngine.h:229-249`), `appendFrameRow` (`.cpp:177-204`), `getNumBytes` (`.cpp:235-250`).
- **AnsiDocument line build:** `addLine`, `addImages`, `addCoveredImages` (`jam_AnsiDocument.cpp:172-228`).
- **TextEditor reflow:** `setLineRows` → `ReflowDocument::setRows (line, width, width, measure)` (`jam_TextEditor.cpp:140-155`). Per-column sibling: `MarkdownComponent::setTableRows` (`jam_MarkdownComponent.cpp:227-238`). Image row height sibling: `setImageRows` (`:140-149`). Fit: `RectanglePlacement (… onlyReduceInSize)` (`jam_MarkdownGraphics.cpp:299-300`).

## Validation Gate

COUNSELOR validates each step before the next. The checks are MANIFESTO (BLESSED), NAMES, CODING and this PLAN, read in the changed files. There is no ARCHITECT round trip and no Auditor round trip per step. COUNSELOR builds with cast (ruling 55): `cast cast/spell.md --debug` in eve, and cast for jam where jam changes. @Auditor runs ONCE, after the last code step.

## Steps

### Step 0: Measure the launch today (baseline)
**Scope:** no source change. A whatdbg `sample` spec in the scratchpad, run by @Pathfinder on EVE Release.
**Action:** Sample the message thread from exec to the first painted history row. Record the time of each stage: `load` (parse, layout, paint, PNG), `process`, TextEditor `timerCallback` (pull, `addLines`, `setAdvances`, `setLineRows`, `addRuns`, image decode).
**Validation:** Each stage has a number from the sample, not from code reading.

### Step 1: `maxLineWidth` unset means the bounds (R1)
**Scope:** `jam_markdown_graphics/component/jam_MarkdownComponent.cpp` (`getLineWidth`, `:335-342`); `eve/Source/layout/style.css:842` (delete the row).
**Action:** `getLineWidth` returns `jmin (visible width − margins, maxLineWidth)` when the sheet holds `maxLineWidth`, and `visible width − margins` when it does not. The presence test uses `StyleManager::getConfig (sheet)`, the `getBaseFontSize` shape (`jam_StyleMarkdown.cpp:155-160`). `getColumnWidth` keeps its one caller, cast `Help.cpp:14`, whose sheet sets the key (`jam/resources/markdown.css:233`).
**Validation:** No `0` sentinel. The meaning of 0 in ReflowDocument stays as it is: *"every span is its own row"* (`jam_ReflowDocument.h:78`). Positive nesting, no bail-out.

### Step 2: Vocabulary — OSC 133 line marks (cast)
**Scope:** `jam/cast/terminal.md` `## ShellIntegration`; `jam/generated/jam_Terminal.h` (by cast only).
**Action:** Add three rows to `map::ShellIntegration`, each a letter after `133;`. The names need ARCHITECT approval (see New Names).
- line layout: the properties of the line that follows. Arguments: `indent` (cells) and `bar` (one argument per quote bar, cell offset).
- table column: opens a column of the current table row. Arguments: `left` and `width` (fractions of the table width).
- fill: a decoration of the line. Arguments: `left`, `width` (fractions of the line's content width), `colour` (terminal colour id), and the fill type: background, rule or border.
**Validation:** cast regenerates `generated/jam_Terminal.h`. A second `cast --sync` writes 0.

### Step 3: Encoder — write the marks
**Scope:** `jam_terminal/graphics/jam_TerminalEncoder.h/.cpp`.
**Action:** Add one append function per mark, in the `appendInlineImage` family: OSC introducer, `133;`, the letter, then `key=value` arguments through `appendImageArgument`. Add an `appendImageArgument` overload for a `double` fraction (ruling 64: *"just create overload"*).
**Validation:** Each argument key is the cast constant. No literal strings. One function per mark.

### Step 4: AnsiDocument — decode the marks into the cell-bearing element (SPEC.md:185-188)
**Scope:** `jam_terminal/document/jam_AnsiDocument.h/.cpp`.
**Action:**
- `oscConstruction` gets the `map::OSC::shellIntegration` arm. It dispatches on the letter through a `Function::Map`, the `csiConstruction` shape (`:584-606`).
- Line layout sets the pen's line properties. `addLine` writes them as `Id::indent` plus the bar offsets.
- Table column starts a child line of the line under construction. The child carries `Id::left` and `Id::width`, the `MarkdownDocument::addTableLines` shape (`jam_MarkdownDocument.cpp:1124-1133`). The cells that follow go to that child until the next column mark or the newline.
- Fill adds a child element of the line, the `addImages` shape (`:197-212`).
- A line without marks is unchanged, so shell output stays plain lines.
**Validation:** Marks are complete at creation (MANIFESTO E). No re-scan of `│` cells. Positive nesting. The Function::Map is named as a noun (NAMES Rule 8).

### Step 5: Engine — rows of any length; marks per slot
**Scope:** `jam_terminal/graphics/jam_TerminalGraphicsEngine.h/.cpp`.
**Action:**
- `prepare (rows)` sizes the ring by row count only. A row is one logical line of any length (ruling 43; RFC:370-372). The print sets each row's atoms, so `appendFrameRow` writes no padding blanks.
- Per-slot line marks sit beside `imagePlacements`, the same shape. `appendFrameRow` writes the marks of a slot before its text.
- `getNumBytes` sizes the history from the atom count and the mark bytes of the prepared rows, given by the print.
**Validation:** No allocation in `process` (TETRIS: *"The audio thread must never: … Allocate memory"*, `TETRIS.md:69-75`). Every buffer is sized in `prepare`.

### Step 6: The print — `load` prints the AST (R7, R8)
**Scope:** `eve/Source/EVETerminalProcessor.h/.cpp`. `jam_MarkdownGraphics.h/.cpp`: the width-relative decoration functions, and `getPills()` made public (proven external caller).
**Action (delete first):** Remove the `MarkdownComponent`, `getColumnWidth`, `paintEntireComponent` and `GraphicsContext` use from `load`. Then, inside `suspendProcessing`:
1. Map the file `readOnly`, `MarkdownDocument::parse`, `addLines()`.
2. For each display line, write one engine row:
   - The cells are rebuilt through `set (index, atom, source)`. Each colour id becomes `engine.getOrCreateColourId (styleMarkdown.findColour (id))`. A code or tag run takes its pill background colour as the cell background.
   - The line-layout mark comes from `getLineIndent` and the quote-bar offsets of `getBlockIndent (ancestor)`.
   - Table rows write one table-column mark per cell line, with that line's `Id::left` / `Id::width`.
   - Fills come from the `MarkdownGraphics` decoration functions, expressed against the content width: code background, rule, table header background, borders and separators. One author: those functions also serve `MarkdownComponent` at its own width.
   - A gap line becomes `getGapHeight` empty rows.
   - An image line writes an inline image: file images as their file bytes through `getPng`, mermaid rasterised at its natural bounds. The cell size comes from `getCellBounds`.
3. `prepare (spec)` sizes the FIFOs from the print.

`process()` is unchanged in role. It serialises rows and marks.
**Validation:**
- No Document access in `process` (SPEC.md:255).
- The single author of decorations is MarkdownGraphics (MANIFESTO S).
- The colour map is the GraphicsContext call (`:637`).
- No winsize read in `load` (ruling 56).

### Step 7: TextEditor — render the element: indent, columns, fills, images (R2–R6)
**Scope:** `jam_gui/widgets/jam_TextEditor.h/.cpp`.
**Action:**
- `setLineRows` uses `width = getMaximumVisibleWidth() − Id::indent`.
- A line with column children reflows each child with `ReflowDocument::setRows` at `width × Id::width`. The row height is the tallest column, the `setTableRows` shape. Separators and borders draw for the full row height (R4).
- An image line gets one row whose height is the fitted image height. The fit is `RectanglePlacement (xLeft | yTop | onlyReduceInSize)` to the content width (`jam_MarkdownGraphics.cpp:299-300`), the `setImageRows` shape (R2).
- Fill children paint per line at the current width, the `paintImages` shape (`:261-273`).
- Quote bars paint at their cell offsets for every row of the line.
**Validation:** One reflow lane, `ReflowDocument::setRows` (R5). No markdown type in jam_gui. Paint reads only (ruling 6).

### Step 8: TextEditor — reflow fast
**Scope:** `jam_gui/widgets/jam_TextEditor.h/.cpp`.
**Action:**
- A width change runs `ReflowDocument::setRows` and the row heights for every line, then rebuilds the index.
- Glyph runs are built at paint, only for lines that show, and are kept until the next width change. This is the shape that `MarkdownComponent` documents: *"Paint builds the MarkdownGraphics of a line when the line first shows"* (doxygen `classjam_1_1MarkdownComponent.xml`).
- Advances stay measured once per line (B9: no re-measure on resize).
**Validation:** No run building in `visibleAreaChanged` (today `setRows` rebuilds every line's runs, `jam_TextEditor.cpp:112-121`). Results match the Step 0 measurement shape.

### Step 9: Build, measure, compare
**Scope:** no source change.
**Action:** Build EVE Debug and Release with cast. Run the Step 0 sample again. Do the visual pass that PLAN-launch Step R used (f0, f03, f45, f6, f99): text, lists, quotes, code, rules, tables, links, logos, mermaid. Resize narrow, then wide, past 100 columns.
**Validation:**
- Text follows the window width, both narrower and wider.
- A table wraps per column.
- An image shrinks to fit and never grows.
- Each stage time is lower than in Step 0, or the step names the next cost with a sample.

### Step 10: Audit, doxygen, log
@Auditor once. Resolve the validated findings. Doxygen with zero warnings, written last. Log the sprint on ARCHITECT's "log sprint". Commit messages in chat.

## New Names (ARCHITECT approval — NAMES Rule -1)

- Three `map::ShellIntegration` rows (Step 2): names and letters. The family is `promptStart`, `commandStart`, `outputStart`, `outputEnd`.
- The fill child element id and its type keys (Step 4). The sibling is `Id::image`.
- The three encoder functions (Step 3), in the `append*` family.
- The engine per-slot mark storage (Step 5). The sibling is `ImagePlacement` / `imagePlacements`.
- The width-relative decoration functions of `MarkdownGraphics` (Step 6).

## BLESSED Alignment

- **B:** The chain owns the AST, the ring and the marks as cold resources. TextEditor owns its documents.
- **L:** Every addition joins an existing family: the OSC arm map, `append*`, per-slot placements, line children.
- **E:** The layers point down. jam_gui stays markdown-free. The wire carries the element (SPEC.md:185-188).
- **S (SSOT):** MarkdownDocument creates the structure once. MarkdownGraphics is the one author of decoration geometry. ReflowDocument is the one reflow.
- **S (Stateless):** Paint reads only. `process` serialises only.
- **D:** The same harness gives the same bytes. The same width gives the same rows.

## Risks / Open Questions

- None open except the New Names approval above, which the plan approval covers.

## Execution state

- Step 0: done. A whatdbg sample (Release, 90 samples) gives the stage order but not the times: each capture costs 0.2–1.1 s under the debugger. Order: `AudioStandaloneApp::initialise` → audio device init → `createEditorAndMakeActive` → `EVEView` → `load` → `jam::Window` → `setVisible`. Times from ephemeral `"launch"` `debug::Log` lines (Release, warm, ~10 ms resolution; Log scope made unconditional in `EVEProcessor.h:56-57` for the run, restored at removal):
  - processor → editorStart 180 ms;
  - load layout 110 ms, load paint 70 ms;
  - window visible +40 ms;
  - first pull 60 ms;
  - TextEditor layout 220 ms;
  - first paint 20 ms;
  - total processor → painted 770 ms.

  The `"launch"` lines stay until Step 9 and are then removed (`grep -rn '"launch"'`): `EVEProcessor.cpp:16`, `EVEView.cpp:9,12,14`, `EVETerminalProcessor.cpp` load, `jam_TextEditor.cpp:70,72,105,240`, `jam_AudioStandaloneApp.cpp:55`.
- Names (ARCHITECT, verbatim answers): `lineStart, E`; `columnStart, F`; `fill, G`. The quote-bar key question was answered *"just make consistent. what's the established pattern?"*. Pattern: a quote bar is an `addFill` (`jam_MarkdownGraphics.cpp:270-278`), so there is no bar key. Fill keys are the approved `left`, `width`, `height`, `colour`. EVE maps quote bar, rule and table-line colours to `colourIdNone` (`style.css:23`, `:25`, `:26`; `jam_TerminalGraphicsEngine.cpp:106`), so no thin-line geometry travels this sprint (MANIFESTO L, YAGNI).
- Step 1: done. `StyleMarkdown::getLineWidth (float availableWidth)` (`jam_StyleMarkdown.cpp:274-283`); `MarkdownComponent::getLineWidth` calls it (`jam_MarkdownComponent.cpp:335-338`). The `style.css:842` deletion moves to Step 6, because today's `load` calls `getColumnWidth`, which asserts and throws on a missing key (`jam_StyleManager.h:164-166`).
- Step 2: done. `cast/terminal.md:1068-1070`; `Id::indent` moved from `cast/markdown.md` to `cast/core.md:92`; generated `jam_Terminal.h:1330-1343`, `jam_Core.h:87`.
- Step 3: done. `appendOscCommand`, the `appendImageArgument` double and Colour overloads, `appendLineStart`, `appendColumnStart`, `appendFill` (`jam_TerminalEncoder.cpp:41-46`, `:243-300`). `appendHyperlink` and `appendInlineImage` use `appendOscCommand` (DRY). The local `text` was removed (no identifier latitude).
- Step 4: done. The pen holds `indent`, `columns` and `fills` (`jam_AnsiDocument.h`). `oscConstruction` arms take `AnsiDocument&`. `applyShellIntegration` dispatches through `shellIntegrationConstruction` to `applyLineStart`, `addColumn` and `addFill`. `addLine` writes `Id::indent`. A line with columns gets empty cells, then `addColumns` and `addFills`. Corrections: `///<` comments removed; `lineStart` is one class constant; `getIdentifier` is the helper. An Engineer edit deleted cpp lines 5-171 (prefix match `addLine` / `addLines`). The Engineer restored them, and a read-only git diff confirmed hunks start at :172.
- Step 5: done. `MarkerPlacement { column, markers }` and `markerPlacements` sit beside `imagePlacements`. `addMarkerPlacement (writer, row, column, MemoryBlock)`. `appendRowSpan (target, slot, start, end)` writes markers at their columns inside the text spans. `getNumBytes()` bounds the prepared rows with `linkReserveBytes` and `newlineBytes`. Corrections: duplicate assert removed; literal `+1` became `newlineBytes`.
- Step 6: done. `EVETerminalProcessor::load` maps, parses, calls `addLines`, then prints inside `suspendProcessing`: `getNumRows`, `addRows`, `addRow`, `getAtom`, `addCells`, `addImage`, `getDiagramImage`, `getNumObjects`, `addMarkers`. `prepare` sizes history as `liveCapacity + engine.getNumBytes() + getNumBytes ({ cols, historyRows })`. `style.css` `maxLineWidth` deleted. `MarkdownGraphics::getPills()` made public (one external caller, `getAtom`). `StyleCustom::getLineIndent` reads `Id::indent × cellWidth`. Corrections: colour id 0 passed to `findColour` became `jam::colours::getColour`; the column running sum moved into `addRow`; functions over 30 lines split by responsibility.
- Step 7: done. `setLineRows` calls `setTableRows` (per-column reflow at `contentWidth × Id::width`, row height = tallest) or reflow + `addRowHeights` + `setImageRows` (fit `onlyReduceInSize`, bounds in `lane.images`). `setAdvances` recurses into column children. `paintRows` draws fills, line and child runs, then images. Fix: `std::bad_variant_access` at first pull, traced by whatdbg to `addCoveredImages` iterating `Id::fill` children; now `isTag (Id::image)` (`jam_AnsiDocument.cpp:284`).
- Step 8: done. Runs build at layout, for visible lines only: `addVisibleRuns()` from `visibleAreaChanged`, `timerCallback` and `lookAndFeelChanged` (ruling 6: no materialisation at paint).
- Step 9: done. Release, three runs, integer ms:
  - processor → editorStart ~190 ms;
  - load parse + `addLines` ~10 ms;
  - print ~206 ms;
  - first pull ~57 ms;
  - TextEditor layout ~167 ms;
  - processor → first painted 749–807 ms.

  The total is not lower than Step 0. The next costs are the print (~206 ms) and the TextEditor layout (~167 ms). They are not yet named with a sample. The whatdbg sampler failed two times with `timeout must be >= 0` (`dap.lua:86`). The cause is a clock read two times at `sample.lua:51-52`; `trace.lua:190` clamps the same value. The partial run (85 samples) put 2 samples in `load`, both in glyph raster under `addCells` (mermaid, `EVETerminalProcessor.cpp:184` → `:207` → `:225`).

  The next costs are named with ephemeral `"launch"` `debug::Log` timings (Release, after the audit fixes, three runs, whole ms):
  - parse + `addLines`: 10, 9, 9.
  - print total: 216, 212, 210. Of that, the picture (mermaid raster or `MarkdownDocument::getImage`) takes 100, 97, 96. `ImageLoader::getPng` takes 90, 90, 88.
  - TextEditor first layout: image decode (base64 + PNG, `addImages`) 102, 101, 100; advances 55, 55, 54; rows 2, 2, 2; visible runs 0, 0, 0.

  The image round trip (picture → PNG encode → base64 → base64 decode → PNG decode) is ~290 ms of the launch. Advances are ~55 ms. The `"launch"` lines are removed and the `#if JUCE_DEBUG` guard is restored (`EVEProcessor.h:56-58`).

  Visual pass after the audit fixes (Release, 895 and 420 wide):
  - Correct: metadata keys, code and HTML block backgrounds, inline code and tag pills, lists, quotes, links, logos fitted at 420, mermaid diagrams, pipe and grid tables with column fractions and header background, prose reflow.
  - Open, not shown to be caused by this sprint:
    - The Han ideographs of `markdown.md:481` and `:483` are blank; kana and hangul render.
    - The full-width line `markdown.md:489` is blank.
    - The grid table cell `markdown.md:471-472` shows "bullet in cellsecond bullet" (two list items joined).
- Doxygen: written for every changed symbol; jam and eve runs give zero warnings. Debug assert trace after the fixes: `END outcome=timeout stops=0`. Debug assert trace: `END outcome=timeout stops=0`. Visual pass (Release): metadata keys, headings, code backgrounds and padding, lists, links, pipe tables at column fractions with header background, mermaid fitted, prose reflow at 680 and 420, no cap at 1600 (a 147-character line on one row). The `"launch"` lines are removed and the `#if JUCE_DEBUG` guard restored (`EVEProcessor.h`).
- Step 10: audit done (NEEDS_WORK, 40 findings). COUNSELOR read each cited line.
  - Resolved in three Engineer batches:
    - jam_terminal: #7 (`Id::indent` only on a marked line, `std::optional`), #11 and #12 (argument check at entry, the `addImage` shape), #13 (`getShellIntegrationProperties` table replaces `getIdentifier`), #14 (column children carry their images), #18 (`cellsStart`), #21 (`addSegments`), #25 (template `appendImageArgument`, `appendShellIntegration`), #26 (one base64 size), #27 (`appendOscIntroducer` folded if single caller), #29 (engine marker span).
    - TextEditor: #22 (`setLane`), #23 (`Index::getHeight`), #24 (`getContentWidth`), #28 (table test by any column child), #14 (column images), #29 (target row), #30 (whole `AttributedImage` replacement).
    - markdown, mermaid and EVE: #1 and #20 (`MarkdownGraphics::getFills` / `hasFill`, one fill author), #2 (`MermaidGraphics::getImage`, one raster), #3 (public `getPill`), #4 (`StyleMarkdown::getType` / `getBlock`), #5 (one per-line `getNumRows`), #6 (named `gridColumns`), #8 and #32 (fill colour is the colour of the terminal colour id, `engine.getColour`), #17 and #33 (`setAtom`, no font id on the wire).
  - Rejected, with the read that refutes:
    - #6 (signature part): `prepare (cols, rows)` stays. cast `Help.cpp:24` passes a grid width, and `GraphicsContext` reads `engine.getBounds()` (`jam_TerminalGraphicsContext.cpp:15`).
    - #9: `addImagePlacement` spans `destination.height` slots and writes `appendCursorForward` on continuation rows (`jam_TerminalGraphicsEngine.cpp:155-160`, `:195-198`). An image line is one row (Step 7).
    - #10: `AttributedString::set (index, atom, source)` maps colours through the source table (`jam_AttributedString.cpp:129-130`). The print needs engine colour ids (Step 6).
    - #15: a table element takes its layout from the column marks (R7). The print writes only blank padding before the first column.
    - #16: no reader reads `Id::indent` across documents. `StyleCustom::getLineIndent` reads AnsiDocument lines only; `MarkdownGraphics::getIndent` reads markdown lines only.
    - #19: PLAN Step 3 names `appendImageArgument`.
    - #31: the runs resolve colour ids through `findColour` (`jam_TextEditor.cpp:361`). The LookAndFeel table is the framework lookup that paint reads.
    - #34: the print is creation (MANIFESTO E, "Creation can be complex"). No ruling makes padding a line property; R7 covers decorations.
    - #35: the declared height is the natural size that the fit reads (`jam_TextEditor.cpp:214`). Covered-line entries paint clipped to their own line (`:394`).
    - #36: the scoped `GlyphArrangement` sets the cell measure that StyleMarkdown reads (`jam_StyleMarkdown.cpp:9`, `:92-93`).
    - #37: the auditor finds that `MarkerPlacement` passes the capability test.
    - #38: after #7, a line without a mark has no `Id::indent`, so the default is needed.
    - #39: the image list matches the object cells by creation (MANIFESTO D).
    - #40: `getMemoryBlock` is the creation of the placement, in the cold path.
