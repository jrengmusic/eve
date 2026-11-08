# PLAN: One style sheet per product, isomorphic with JFS

It supersedes Steps 11 and 13 of `PLAN-text-editor-absorption.md`. Step 13 calls for *"one table of the `ENDLookAndFeel.cpp:523-537` form"*, which this plan removes.

## Context

ARCHITECT's runtime test passed for OPEN, list navigation, open by click, Whelmed images and the cast/scout/ggwp rebuild. READER fails: VISUAL (drag or `v`) asserts in `juce::LookAndFeel::findColour` (`juce_LookAndFeel.cpp:94`), called from `TextEditorStatusBar::paint` (`jam_TextEditorStatusBar.cpp:47`). The markdown style never gets the status bar colours, because they exist only in EVE's `TEXT_EDITOR` sheet (`eve/Source/textEditor.css:9-15`).

The cause is that jam diverges from the plugin_bootstrap contract that JFS (KANJUT) follows:

| Contract (JFS) | jam today |
|---|---|
| One `style.css` per product: `:root`, `.appearance`, `.appearance.DARK`, `.metrics`, `.window`, `@font-face`, one class block per registered style (`jreng-filter-strip/Source/layout/style.css`) | EVE has 8 CSS files. jam ships `markdown.css`, `mermaid.css`, `textEditor.css` and the fonts (`eve/project-info.md:20-22`, `:425-429`, `:451`) |
| `registerStyle (type)` gives the style `setSheet (toType (type))` (`kuassa_Registry.h:168-179`). `ViewManager::applyAppearance` applies `STYLE`, then the style's own sheet (`kuassa_ViewManager.cpp:322-328`) | Same code. `StyleTerminal`, `StyleMermaid` and `StyleMarkdown` override `setAppearance` to apply extra sheets or to set colours themselves (`jam_StyleTerminal.cpp:11-17`, `jam_StyleMermaid.cpp:11-14`, `jam_StyleMarkdown.cpp:25-35`) |
| `StyleManager::setAppearance` maps every key through the generated `map::ColourId` (`kuassa_StyleManager.cpp:176-208`) | Adds `setThemeColours`, a C++ pairing table of 8 rows (`jam_StyleManager.cpp:218-249`), and the literal flex key `Id::resizerBar` (`:207`, `jam_PluginEditorLayout.cpp:98`, `:128`) |

A colour id that no sheet names has no default in either framework. `StyleCustom` has no constructor (`kuassa_StyleCustom.h:17`). Only JUCE ids get defaults, from `LookAndFeel_V4` (`juce_LookAndFeel_V4.cpp:70-72`, `:1341`). JFS never asserts only because its sheet names every custom id it paints (`style.css:169-173`).

`theme.md` is the style tree written to disk (`jam_PluginEditorLayout.cpp:672-679`) and parsed back (`jam_ConfigDocument.cpp:233-246`). Today it:
- mixes 4 types in one table per sheet (`## markdown`: 53 colour, 108 string, 8 float, 45 em);
- spells palette references 2 ways (`CODE_HIGHLIGHT` at `theme.md:282`, `glass narwhal grey` at `:71`);
- repeats "colour id" and "font id" in 533 keys.

## ARCHITECT rulings (verbatim)

1. *"There is only ONE SSOT STYLE … this is StyleManager responsibility to set, this is plugin_bootstrap contract … THERE IS NO MANUAL HANDROLL set of LookAndFeel"*
2. *"JFS has theme with its style.css, we also have theme.md, only different on storing on disk for user access, but the parser all available"*
3. *"non-negotible, must be using StyleManager, StyleManager is ANY Style-agnostic … NO HARDCOING AT ALL. KANJUT and JAM will be synced … didnt change the pattern, did not change the CONTRACT"*
4. *"fonts are project resources, Display Mono carried by eve not jam"*
5. *"all css spread on jam, should be eve's style.css to build Style(s) that eve using"*
6. *"those Style must have internal `default` colours fallback … because our custom widgets has unique colourId that have no juce counterpart"*
7. *"wrong/missing colours should not CRASH with assert … but showing the `error` VISUALLY"*
8. *"VISIBLE COLOUR is not silent fail. VIVID VISUAL COLOUR is LOUD. louder than assert, it also satisfy no garbage defensive programming"*
9. *"the theme.md pattern is garbage … table for metrics, table for fonts, table for colours … why they key need "colour id", "font id" … parser responsible to match with the map, whatever key being used as the ssot from css"*
10. *"if we could customize resizer bar, the width, the svg, from theme same contract should be applied with scrollbar, user configurable"*
11. *"using assert everywhere is FORBIDDEN. it's pessimistic programming"*. MANIFESTO **D** (`MANIFESTO.md:385`).
12. *"there's no pattern to reivent … JFS is the pattern. it's battle tested, QA tested, it's commercial product"*
13. *"suffix dropped at USER CONFIG THEME. THIS IS THE PATTERN with END canon … EVE still responsible to generate and write its own, when not hosted at END. this already decided"*
14. *"there is no magic strings hardcoded anywhere, the lexicon is table driven manifest by cast"* and *"NO HAND WRITTEN"*
15. *"the pattern is DETERMNISTIC, jam::Format already can do any round trip key to whatever css, markdown need"* and *"SSOT, UNIDIRECTIONAL, DESIGN BY CONTRACT"*
16. *"no audit. only audit when i explicit told you so"*
17. On the two merge collisions (`--carbon`, `@font-face mono`): *"merge. one definition. one colour, one size"*. EVE's definition stays, because jam's sheets fold into EVE's `style.css` (ruling 5): `--carbon: #333435` (`eve/Source/layout/colours.css:8`), `mono` = `DisplayMonoMedium.ttf`, height 12 (`eve/Source/layout/style.css:1-6`).

**Execution notes (Step Gate corrections):**
- The `.markdown` class block needs dark values: `--Markdown--backgroundColourId` and `--Markdown--textColourId` differ between `.appearance` and `.appearance.DARK` (`markdown.css:58`, `:76`, `:85`, `:103`). The parser gives a dark child only to `.appearance.DARK` (`jam_Css.cpp:672-675`). Step 8 parses `.<class>.DARK` into a `DARK` child of that class sheet, the same form as `.appearance.DARK`. `StyleManager::setAppearance` already applies a sheet's `DARK` child (`jam_StyleManager.cpp:183-185`).
- The menu `opacity` metric had one reader, the deleted `setThemeColours`. `PopupMenu::backgroundColourId` takes a palette colour instead. That colour is `--glassBunker: #090D12A6`, the window background `bunker` at alpha 0.65, in the `glass*` family (`glassNarwhalGrey`, `glassCorbeau`, `glassLittleMermaid`).
- The Display fonts are copied into `eve/Source/fonts`.

18. *"there should not be any jam::fonts anymore no? all will be project resources"*. Step 6b below carries it out.

#### Step 6b: No `jam::fonts`
**Scope:**
- the `jam_graphics_FontData` block in each cast template: `eve/cast/cmake.cast:323-332`, `whelmed/cast/cmake.cast:316-324`, `end/cast/cmake.cast:286-294`, `archetype/{plugin,plugin-only,app}/cast/cmake.cast`;
- the `fonts` layout-glob rows that point into `jam/resources/fonts`: `whelmed/project-info.md:408` and the archetype `init.cast` rows;
- `stamp/project-info.md:15-18`;
- `end/Source/lookAndFeel/ENDLookAndFeel.{h,cpp}` (`.h:8`, `.cpp:513-518`);
- `endless/Source/MainComponent.{h,cpp}` (`.h:37`, `.cpp:105`, `:108`);
- `archetype/ARCHETYPE.md:397`, `:410`;
- delete `jam/resources/fonts/*.ttf`.
**Action:** Each project embeds its own Display fonts from its own `Source/fonts/`, through its own `BinaryData`. Code that names `jam::fonts::X` names `BinaryData::X`. jam ships no font.
**Validation:** Rulings 4 and 18.

## Contract

- **One direction:** `style.css` → `Css::getValueTree` → the config tree → `StyleManager` → each registered style. `theme.md` is that tree on disk. Its parser gives back the same tree.
- **Keys:**
  - The CSS key is the source. `jam::Format` converts it in both directions (`toWords`, `toTag`, `toType`, `toCamelCase`, `jam_Format.cpp:231-244`, `:668-717`).
  - The generated `map::ColourId`, `map::FontId` and the cast metric tables are the only lexicon.
  - No pairing table is written in C++.
- **Colours:** the style is the owner. At construction it holds a colour for every id in `map::ColourId`. A missing or invalid colour paints vivid magenta. No assert guards a colour read.

## Steps

### Part A: The colour contract

#### Step 1: Every style holds every colour
**Scope:** `jam/jam_style/jam_StyleCustom.h`, `.cpp`
**Action:** The `StyleCustom` constructor iterates the generated `map::ColourId`. It sets `juce::Colours::magenta` on each id that `LookAndFeel_V4` did not set (`isColourSpecified`). The JUCE ids keep their V4 defaults.
**Validation:** Rulings 6, 7, 8, 11. The invariant is set once, at the owner (MANIFESTO **D**). No caller of `findColour` can reach `juce_LookAndFeel.cpp:94`. This step alone ends the READER crash.

#### Step 2: Colour lookups fail visibly, not by assert
**Scope:** `jam/jam_style/style_manager/jam_StyleManager.cpp`, `jam/jam_web/css/jam_Css.cpp`
**Action:**
1. `StyleManager::getColour` returns `juce::Colours::magenta` for an alias that no sheet holds. Delete `jassertfalse` (`:163`).
2. `CssDocument::toColourValue` returns magenta for a value that is not valid hex. The `transparent` keyword stays transparent.
**Validation:** Rulings 7, 8, 11. Today both paths give a transparent colour, which is a silent fail (MANIFESTO **E**).

#### Step 3: Delete the C++ pairing table
**Scope:** `jam/jam_style/style_manager/jam_StyleManager.h`, `.cpp`
**Action:** Delete `setThemeColours` and its call (`:206`, `:218-249`). The ScrollBar, Label, ResizableWindow and PopupMenu ids come from `.appearance` through `map::ColourId`, as in JFS (`style.css:96-97`, `:124-128`, `:134`).
**Validation:** Rulings 3, 14. KANJUT has no such table (`kuassa_StyleManager.cpp:176-208`).

### Part B: Styles hold no sheet logic

#### Step 4: One style sheet read by every style
**Scope:** `jam/jam_style/style_manager/jam_StyleManager.*`, `jam/jam_terminal/style/jam_StyleTerminal.*`, `jam/jam_mermaid_diagram/jam_StyleMermaid.*`, `jam/jam_markdown_graphics/style/jam_StyleMarkdown.*`, `jam/jam_plugin_bootstrap/view/jam_ViewManager.cpp`, `jam/jam_gui/style/jam_StyleTheme.cpp`
**Action:**
1. `StyleManager::getStyleSheet()` joins `getDisplaySheet`, `getWindowSheet` and `getFontsSheet`. It replaces the two local copies of `Format::toType (Id::style)` (`jam_ViewManager.cpp:324`, `jam_StyleTheme.cpp:5-10`).
2. Delete the `StyleTerminal::setAppearance` override.
3. `StyleMermaid` keeps no static sheet. Its metrics and fonts read `getStyleSheet()`.
4. `StyleMarkdown::setAppearance` keeps only its own derivations (`appearanceNumber`, `listMarkerWidth`). Delete `:30` and `:32-33`. Its metrics and fonts read `getStyleSheet()`.
**Validation:** Rulings 1, 3, 12. A style's own sheet is its class block, and `StyleCustom::setAppearance` applies it (`jam_StyleCustom.cpp:240-245`), as `StylePopupTextBox` does in KANJUT.

#### Step 5: One metric name space
**Scope:** `jam/cast/mermaid.md:1508`, the code that reads it
**Action:** The mermaid `lineHeight` row becomes `mermaidLineHeight`, because one `.metrics` block holds both modules. Markdown already owns `lineHeight` (`markdown.css:197`, `mermaid.css:761`). The new name joins the `codeLineHeight`, `tableLineHeight`, `metadataLineHeight` family (NAMES Rule 5).
**Validation:** Ruling 14. It is a cast row, so the generated `map::MermaidStyleSheet` follows.

### Part C: Each product owns its sheet and its fonts

#### Step 6: EVE's `style.css`
**Scope:** `eve/Source/layout/style.css`, delete `eve/Source/layout/{colours,menu,overlay,scrollbar,terminal,window}.css` and `eve/Source/textEditor.css`, `eve/project-info.md`, `eve/Source/fonts/`
**Action:**
1. Build one `style.css` in the JFS shape, from EVE's 8 files plus `jam/resources/markdown.css` and `mermaid.css`:
   - `:root`: one palette;
   - `.appearance` and `.appearance.DARK`: the base colour ids and font ids, JFS spelling (`--TextEditorStatusBar--backgroundColourId`);
   - `.metrics`;
   - `.window`;
   - `@font-face`;
   - `.textEditor` and `.markdown` class blocks, which hold only overrides. `.markdown` carries the `TextEditor` background and text that `jam_StyleMarkdown.cpp:32-33` sets in C++ today.
2. Palette names that collide get no silent merge. For example, `--carbon` is `#333435` at `eve/Source/layout/colours.css:8` and `#33302E` at `jam/resources/mermaid.css:3`. Pathfinder lists every colliding name and both values. Each collision goes to ARCHITECT before the merge (NAMES Rule -1).
3. `project-info.md`:
   - delete the `@markdown-style` and `@mermaid-style` rows and `terminalStyleSheet` (`:20-22`, `:425-429`);
   - the fonts glob reads `Source/fonts/*.ttf` (`:451`);
   - the Display and DisplayMono files move into `eve/Source/fonts/`.
**Validation:** Rulings 4, 5, 12.

#### Step 7: The other consumers of the jam sheets
**Scope:** `whelmed`, `stamp`, `cast/scout/ggwp Source/Help.cpp:11`, delete `jam/resources/{markdown,mermaid,textEditor}.css`
**Action:** Each project gets one `style.css` in the same shape, and its `StyleManager` loads that one sheet. jam keeps no CSS.
**Validation:** Rulings 5, 12. Step 4 makes `StyleMarkdown` read the `STYLE` sheet, so these projects stop working without it (`Help.cpp:11` loads `MARKDOWN` and `MERMAID`). ARCHITECT approved this scope with the plan.

### Part D: `theme.md` is `style.css` on disk

#### Step 8: The CSS tree keeps the component
**Scope:** `jam/jam_web/css/jam_Css.cpp`, `jam/jam_style/style_manager/jam_StyleManager.cpp`
**Action:**
1. `addAppearance` splits each key at its `--` into a component and a property (`Chars::doubleDash`, `jam_Css.cpp:756`). It stores one record per component under the sheet.
2. `StyleManager::setAppearance` builds each map key with `Format` from the record and the property, then looks it up in `map::ColourId` or `map::FontId`.
**Validation:** Ruling 15. `Format::toType` of the whole key loses the boundary today (`jam_Css.cpp:401-402`).

**Step 8 as executed (Step Gate correction):**
- The component is kept in a `COMPONENT` child of the sheet, keyed like `UNIT` (`jam_Css.cpp:406-422`, `:2059-2067`). It is not kept as one record per component.
- The flat alias keys stay as they are, so `StyleManager::setAppearance` and `getFont` are unchanged (`jam_StyleManager.cpp:136-146`, `:175-203`).
- `.<class>.DARK` becomes a `DARK` child of the class sheet.
- Reason: the existing pattern for per-key metadata in the sheet tree is `UNIT` (MANIFESTO **E**: *"A new pattern where one exists is always wrong"*).

**Step 9 table names (NAMES Rule -1 correction: no ratification asked; the names derive from the tree):** each root child of the tree (a sheet: `STYLE`, `MARKDOWN`, `FONTS`) is a `## <Format::toWords (sheet type)>` heading. Each component of a sheet's `COMPONENT` child is a `### <Format::toWords (component)>` table under that heading. The `Markdown` component of `STYLE` (`### markdown` under `## style`) and the `.markdown` class sheet (`## markdown`) are then distinct, with no new name.

#### Step 9: Writer and parser, one table per tier
**Scope:** `jam/jam_markdown/document/jam_ConfigDocument.*`
**Action:**
1. `toDocument` writes `## colours` (`:root`), one table per `.appearance` component, `## metrics`, `## window`, `## fonts`, and one table per class block.
2. A component table is named with the `Format::toWords` of its component. Its keys are the property words without the id suffix that `map::ColourId` and `map::FontId` share (END canon, `theme.md:108-118`).
3. Palette references are written as palette words, not tags.
4. `getValueTree` rebuilds the same tree through `Format`. The id comes from the same maps.
**Validation:** Rulings 9, 13, 15. Round trip: `style.css` → tree → `theme.md` → tree, with the same tree each time.

#### Step 10: END's theme in the same shape
**Scope:** `end/Source/config/theme/gfx/theme.md`, `end/Source/lookAndFeel/ENDLookAndFeel.cpp:521-541`, `end/cast/bimaps.md:35-54`
**Action:**
1. END's theme takes EVE's tables in the Step 9 shape.
2. `ENDLookAndFeel::setColours` stops using its pairing table. It reads the component tables through `map::ColourId`.
**Validation:** Rulings 13, 14. END hosts EVE on one shared `theme.md` (rulings 24-28 of the absorption plan). ARCHITECT approved this scope with the plan.

### Part E: Scrollbar, same contract as the resizer bar

#### Step 11: Width and art from the theme
**Scope:** `jam/jam_style/jam_StyleCustom.*`, `jam/jam_plugin_bootstrap/layout/jam_PluginEditorLayout.cpp:97-98`, `:126-131`, `jam/jam_style/style_manager/jam_StyleManager.cpp:207`, `end/cast/bimaps.md`, `end/Source/config/svg/`
**Action:**
1. `.metrics` gains `--scrollbar` (JFS `--comboBox` form, `style.css:184`). `StyleCustom` overrides `getDefaultScrollbarWidth` to read it. `Viewport` reads the width again on `lookAndFeelChanged` (`juce_Viewport.cpp:345-352`).
2. The flex file is named from `Id::scrollbar` through `Format::toFileName`, replacing the literal `"resizerBar.svg"` (`:128`) and the key `Id::resizerBar` (`:98`, `:207`).
3. END's `FileFlex` gains a `scrollbar` row, and gfx ships `scrollbar.svg`, a copy of `resizerBar.svg`.
**Validation:** Rulings 10, 14, and ruling 23 of the absorption plan.

### Part F: Documents
#### Step 12
**Scope:** `eve/SPEC.md`, `eve/CLAUDE.md` (Layout row), `eve/PLAN-text-editor-absorption.md` (Steps 11 and 13 marked superseded)
**Action:** Record the rulings and the new file layout.

### Part G: History reflow

19. ARCHITECT, on reflow: *"currently reflow only affect visible viewport area, THIS IS GARBAGE. TextEditor reflow after resize should reflow THE WHOLE HISTORY scrollback"*. On the design question, ARCHITECT pointed to `@carol/MANIFESTO.md`.
    - **Cause:** an auto-wrapped line reaches the record as separate rows. The processor wraps with `addLine` (`jam_TerminalProcessor.cpp:498-503`), and the record writes one line per row (`jam_TerminalSwap.cpp:111-120`).
    - **Answer from MANIFESTO E, Complete at Creation (`MANIFESTO.md:250-252`, `:278`, `:286`):** the record is where history is created, so it stores whole logical lines. A row that continues the previous row joins the previous record line when the record writes it. The processor marks the row when it wraps, and nothing downstream joins rows.

#### Step 13: The record stores logical lines
**Scope:** `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`, `jam/jam_terminal/buffers/jam_TerminalBuffer.{h,cpp}`, `jam/jam_terminal/document/jam_TerminalSwap.{h,cpp}`. Also `jam_AttributedString.{h,cpp}`, only if no append of one string to another exists.
**Design (from discovery):**
1. The wrap fact is per-slot data in `TerminalBuffer`, in the `outputBlocks` form (`jam_TerminalBuffer.h:510`, `.cpp:127-142`). It is set for the row that a soft wrap starts (`jam_TerminalProcessor.cpp:498-503`). It moves with `swap`, and it is reset by both `clear` overloads and by `setScreen` (`:248-256`, `:258-271`, `:314-328`).
2. `TerminalSwap::writeFromTerminalBuffer` (`:78-109`) does not start a new line for a wrapped row. It appends the row's cells to the last written line, writes the joined line again, and points that line's index entry at the new bytes. This is creation-time coalescing (`MANIFESTO.md:278`). `endLine` counts logical lines only.
3. `firstHistoryRow` (`jam_TerminalBuffer.cpp:152-160`) counts rows, but `setLines` (`jam_TerminalSwap.cpp:178-182`) compares it with lines. The swap maps rows it has read to lines, so an erase of saved lines sets `firstLine` to the line of that row.
4. The grid classifier (`getGridStart`, `:140-148`) reads the joined line.
5. Live rows stay rows (`jam_TextEditor.cpp:1167`). The shell redraws the screen on `SIGWINCH`.
**Action:**
1. The processor marks the row that a soft wrap starts.
2. The record writer joins a marked row onto the previous record line.
3. Every record line number then means a logical line.
**Validation:** Ruling 19 and MANIFESTO **E**. Resize lays out every resident block again (`jam_TextEditor.cpp:1252-1255`), so the whole history reflows.

## Verification (ARCHITECT builds and runs)

1. READER, then VISUAL by drag, `v`, `V` and `ctrl+v`: the status bar paints in theme colours, and no assert fires.
2. Delete one `.appearance` row from `style.css`: that element paints magenta, and nothing asserts.
3. Delete EVE's user `theme.md`: EVE writes it again, with tables `colours`, one per component, `metrics`, `window`, `fonts`, and class blocks. No key contains "colour id" or "font id". Palette references read as words.
4. Edit a theme row (a colour, `scrollbar` width): EVE applies it live. Replace `flex/scrollbar.svg`: the thumb art changes.
5. Run EVE inside END: it reads END's `theme.md`, and END's own colours come from the same tables.
6. `cast --help`, Whelmed and stamp render as before.
7. No audit unless ARCHITECT asks for one (ruling 16). The doxygen pass follows ARCHITECT's runtime test.
