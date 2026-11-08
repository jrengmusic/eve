# PLAN: TextEditor Absorption

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)

## Overview

`jam::TextEditor` absorbs `jam::MarkdownComponent` and becomes the one text widget, with a read-only state. Images and mermaid go to `PreviewComponent`, and the editor holds only the layout. Every CLI help page moves to it, as the proof that the module is a general widget. The plan also folds in the bugs that ARCHITECT reported during the runtime test, the scrollbar and theme work, and the OPEN-only redesign.

This plan replaces `PLAN-scrollbar-theme.md`. Its rulings and steps are in Part B.

**Status:** Locked 2026-11-07. The section *Amendment (locked)* at the end of this file holds rulings 37 to 40. It overrides Step 3, adds to Step 4, replaces Part C (Steps 17 to 25) and closes the open questions.

## ARCHITECT's rulings, verbatim

**Absorption**

1. *"so the design is probably ABSORBTION."*
2. *"TextEditor enriched with what markdown as rich text need"*
3. *"images, including mermaid now delegated to ImagePreviewComponent, TextEditor only holds the layout"*
4. *"MarkdownComponent could be deleted"*
5. *"TextEditor now serves as the SSOT of Text rendering widget. setReadOnly (true), becomes preview widget."*
6. *"CLI migration also fold into this plan, since that's the validation of the module as general widget works for any shape, gui, tui, proportional, mono, it does not matter"*
7. *"whelmed (markdown component) - should also be able to navigate with vim jk, gg, G - text selection, etc - cmd+c copy, y yank"*
8. *"IIUC TextEditor was designed to be advanced width agnostic, isnt our cell already carry the width?"*

**Bugs**

9. *"bug: creating new tab did not load EVE instance"*; *"at this instance now even creating new split failed to load EVE."*
10. *"this is must be SSOT, one path, one command, default new pane (including created by new tab), always load EVE"*
11. *"i deleted ~/.config/end/ content"*; *"it only writes display.md, keys.md, themes/"*; *"this violates contract. because plugins IS also the exact same contract. getOrCreate() memoized, check on disk, write when missing"*
12. *"there are `standard` common keyboard convention for terminal"*; *"need to be wired as default"*. ARCHITECT pasted the list that Step 3, Step 4 and Open Question 4 cover.
13. *"so with alternate, it should be just CLI responsibility"*
14. *"while at normal, we should NOT destroy anything. probably best, reflow, layout, repaint, (same path with when resized() ?)"*
15. *"so only redraw active prompt for normal. there your answer. same path, same commandId, one SSOT with clear, WITHOUT erasing any history"*
16. *"clear is erasing history yes"*
17. *"[1] markdown rendered works. but i got those messages on my console"* (`dynamic_cast error 2`).

**Scrollbar and theme** (from `PLAN-scrollbar-theme.md`; ARCHITECT: *"fold into plan, including Viewport with flex and scrollbar for alternate"*)

18. *"auto scroll to bottom should be able overriden when user scroll up, so any new incoming row when scrolled up should not dragging to bottom. user scroll up always win."*
19. A typed key returns the view to the tail: *"yes"*.
20. *"our scrollbar rendering should still SSOT, at least overriden method. one as is, according to history content, another calculation form whatever content from alternate screen"*
21. The alternate bar: *"draggable"*.
22. *"scrollbar should only visible when the pane focused"*
23. *"scrollbar could be customised with flex. if flex available from them draw the flex svg, otherwise the default fallback is current method of drawing, so always optional. this is theme-style-driven. for gfx can use the same flex svg with pane separator"*
24. *"eve should be able to read theme.md"*. EVE adopts *"All shared tables"*.
25. *"probably better if eve.md is like display.md, only `app` specific. while `style` specific merged into theme.md"*
26. The default theme: *"END owns, EVE reads only"*.
27. *"this could be branched with PluginHostType(). isEND()? read only, END guaranteed have the table. other format, end must write its own table for user to override."*
28. The tree:
```
~/.config/end/
|- eve
  |- display.md (eve's display)
  |- theme.md (only getOrCreate when not hosted in END, standalone)
```
29. The table names: ARCHITECT selected *"EVE keeps window in display"*. `## window` stays app-specific in `eve/display.md`, and EVE's `## style` goes. Its three colours come from END's `## window` background and `## scrollbar`.

**OPEN only**

30. *"the design is wrong. we dont need LIST mode at all. it's always only OPEN mode."*
31. *"show each openable file from its filename unique chars a-z, A-Z, so 52 possibility `per page`, tab to cycle the page (look how endless did this)"*
32. *"at OPEN mode ctrl+shift+j previous column list, ctrl+shift+k next column list"*
33. *"active selected column list is not what exactly above active prompt, but always first FOUND above active prompt in history"*
34. *"links always active for whole history content including url, i dont think current url links works"*
35. *"active selected column only for OPEN mode"*
36. *"[2] OPEN mode, the label is unreadable."*

## Language / Framework Constraints

- One object, one translation unit (`LANGUAGE.md:34-35`, CODING.md). `TextBlock` and `TextEditor` stay separate objects.
- Module layers. `jam_gui` includes `jam_style`, `jam_document` and `jam_terminal` (`jam_gui.h:79-83`). It does not include `jam_markdown` or `jam_markdown_graphics`. `jam_markdown_graphics` includes `jam_gui`, `jam_markdown` and `jam_mermaid_diagram` (`jam_markdown_graphics.h:41-47`). So `TextEditor` never names `MarkdownDocument`, `MarkdownGraphics` or `MermaidGraphics`. It reaches them through `Document` and through `StyleCustom` virtuals.
- Under END, the key hook gives END every key first (`juce_ComponentPeer.cpp:201-203` in the patched cache).
- *"agents never change a version."* (`SPEC.md:839`).
- ARCHITECT builds and tests. Agents do not build. No doxygen until ARCHITECT's runtime test.

## Dependency & API Inventory

**Pane load**

- `Nexus::Nexus` calls `offloadPlugins` once (`Nexus.cpp:51`). `offloadPlugins` tests only the stamp (`:177`).
- `TabView::loadPlugin` writes the disk path with no read of the disk (`TabView.cpp:27-30`).
- `Session::loadPlugin` has silent exits when the format or the type is absent (`Session.cpp:78-97`).
- Three call sites pair an add with a load: `newTab` (`ENDView.cpp:190-192`), split (`:348-351`), drag split (`TabView.cpp:150-152`). The drop writes its own file (`EditorView.cpp:97`).
- Sibling of getOrCreate: `if (… and not file.existsAsFile()) file.replaceWithText (…)` (`ConfigModel.cpp:142-143`).

**Keys and clear**

- `mapCtrl` handles letters, `@`, space and `[ \ ] ^ _` (`jam_TerminalKeyboard.cpp:75-92`, `:224-231`). `mapAlt` encodes cursor, function and editing keys (`:119-130`).
- `mapPlain` sends a plain tab on `shift+tab` (`:170-184`, `:220`). fzf decodes CSI `Z` as `ShiftTab` (`fzf/src/tui/light.go:738-739`).
- `clear` row: empty value (`default-eve.md:47`). Callback: `EVEView.cpp:337-341`, `:513-516`. Body: `EVETerminalProcessor.cpp:402-410`, then `jam_TerminalProcessor.cpp:42-48`.
- `tty->setWinsize` runs `TIOCSWINSZ` and `kill (childProcess, SIGWINCH)` (`jam_TerminalTTY_posix.cpp:365-369`). `prepare` calls it last (`EVETerminalProcessor.cpp:387`). `reset` homes the cursor (`jam_TerminalProcessor.cpp:38`), so a full prepare is not "only redraw".
- Measured: `ctrl+l` in zsh writes `ESC[H ESC[2J`, then the prompt. `/usr/bin/clear` writes `ESC[3J ESC[H ESC[2J`. zsh binds `^L` to `clear-screen` (`man zshzle:1990-1991`).
- ED 3 stays as `:982`. ED 2 is `:981`.

**TextEditor**

- Source of lines: `terminalSource` returns null by default (`jam_TextEditor.cpp:21`), and `addBlocks` skips on null (`:956`). The clock starts only in `bindToProcessor` (`:60`).
- `TextBlock::setBlock` takes one font set and one metric set for a block (`jam_TextBlock.cpp:57-76`). `addRowHeights` uses one cell height (`:192-196`).
- Per-line virtuals exist: `getLineFont`, `getLineBoldFont`, `getLineMetrics`, `getLineIndent`, `getLineFonts` (`jam_StyleCustom.h:84-107`). `StyleMarkdown` overrides them (`jam_StyleMarkdown.h:120-193`).
- `getCellBounds` and `getCell` already read per-line advances and row tokens (`jam_TextEditor.cpp:1273-1289`, `:1391-1412`).
- Terminal reads of the `terminal` tree are listed at `:75`, `:101`, `:484-485`, `:566-568`, `:632-634`, `:655`, `:845`, `:932-933`, `:941`, `:946`, `:1339`, `:1357`, `:1467-1468`. `setMode` writes it (`:109`).
- Navigation, selection, `y` and `cmd+c` exist by table (`jam_Gui.h:497-515`, `jam_TextEditor.cpp:233-245`, `:317-327`, `:387-410`, `:474-492`). Text comes from cells (`:402`).

**What MarkdownComponent adds, and where it goes**

- `setTextRows`, `addRowHeights` with padding and tallest cell (`jam_MarkdownComponent.cpp:206-230`, `:256-280`).
- `setTableRows`, `addRow`, the gap row (`:232-254`, `:155-158`).
- `getLineWidth`, `getLineLeft`, a capped and centred column (`:335-345`).
- `MarkdownGraphics` per line: pills, code backgrounds, rules, table borders, bullets, property icons, link areas, quote bars (`jam_MarkdownGraphics.h:189-390`). It derives from `AttributedGraphics` (`:29`), which paints shapes, images, runs and links (`jam_AttributedGraphics.cpp:258-272`).
- Mermaid: parse, graphics and image (`jam_MarkdownComponent.cpp:287-333`, `jam_MermaidGraphics.cpp:10-19`).
- Paging: `getPageRows`, `paintPage`, `getRowNumber`, `getNumRows` (`:28-59`, `:386-392`).

**Consumers of MarkdownComponent**

- Help pages: `cast/Source/Help.cpp`, `scout/Source/Help.cpp`, `ggwp/Source/Help.cpp`. Each is one function that lays out and paints the viewed component into a `TerminalGraphicsContext` (`:16-34`).
- `stamp`: PDF by row range (`stamp/Source/Main.cpp:11-24`, `:34-63`, `:80-99`, `:128-157`; `Manual.h:52`, `:79`, `:99`; `ManualPages.cpp:23-61`, `:120-141`; `ManualContents.cpp:80-96`).
- `whelmed`: `WHELMEDViewComponents.cpp:11`, `:27-32`, `:67`.
- `eve`: `EVEView.cpp:283`, `:315`, `:584-611`.
- `jam`: `FilePreview` (`jam_FilePreview.h:62`, `.cpp:28`, `:76`, `:92`).
- `tit`, `whatdbg` and `end` do not link the module (`CMakeLists.txt` read). `cake` is not in the dev root.

**Images**

- A box is an `Id::image` child of the images tree, with `Id::x`, `Id::y`, `Id::width`, `Id::height` (`jam_TextEditor.cpp:1329-1335`). A box with `Id::path` is a file job, and any other box is a record job (`jam_PreviewComponent.cpp:330-333`).
- No path takes a ready `juce::Image` or `juce::Drawable` (`jam_PreviewComponent.h:37-101`, `:444`, `:475`).
- `juce::ImageCache` is the framework API that holds an image by a hash. The Model row holds only the number.

**Scrollbar and theme** — the inventory of `PLAN-scrollbar-theme.md`:

- Follow: `jam_TextEditor.cpp:976-977`, `:828-829`, `:574-575`, `:890-891`.
- Bar: `:843-848`, `:1177`, `juce_Viewport.cpp:431-434`, `jam_TextEditor.cpp:1830`, `:792-795`, `juce_LookAndFeel_V4.cpp:643-656`, `jam_StyleCustom.h:17`.
- Alternate: `jam_TerminalBuffer.h:504`, `parameters.md`, `jam_TerminalProcessor.cpp:1062-1073`, `:1114-1118`, `:1215-1235`, `EVETerminalProcessor.cpp:98-99`, `:184-188`.
- Flex: `jam_Svg.h:541`, `:591`, `jam_style.h:10`, `ENDLookAndFeel.cpp:475-486`, `:562-578`, `jam_SvgFlex.cpp:51-93`, `ConfigModel.cpp:135-144`, `end/cast/bimaps.md:35`.
- Config: `Files.cpp:35-36`, `EVEProcessor.h:119-124`, `jam_PluginEditorLayout.cpp:59-77`, `:599-604`, `jam_ConfigDocument.cpp:470-471`, `:505-519`, `EVETerminalProcessor.cpp:332`, `Files.cpp:44`, `jam_ENAXClientExtensions.cpp:7`, `end/Source/config/theme/gfx/theme.md`, `display.md:16`, `ENDLookAndFeel.cpp:523-537`, `:549-560`.

**OPEN**

- `linkLabels` is positional (`jam_TextEditor.cpp:5`). `applyLinkLabel`, `paintLinkLabels` (`:1574-1634`).
- `addFileLinks` links only the last output block (`:936-942`, `jam_AnsiDocument.cpp:350-360`), and it uses the current `cwd` (`:941`). Only OSC 8 gives web links.
- Endless contract: the label is a char from the name, the first preferred, later chars tried (`LinkSpan.h:73-81`). Paging: `LinkManager.cpp:54-98`.
- Label colour: `var(--blank)` (`eve/Source/textEditor.css:8`).
- LIST rows: `jam/cast/gui.md:50-52`, `:265`, `jam/cast/terminal.md:1895-1897`, `default-eve.md:51`. Paint: `jam_TextEditor.cpp:1814-1815`. Keys: `:590-593`.

**New names**

- `TextEditor::setDocument`, `TextEditor::setReadOnly`, the row `Id::isReadOnly`, `TextEditor::getViewedComponent`.
- `TextEditor::getPageRows`, `paintPage`, `getRowNumber`, `getNumRows`: moved, same names.
- `StyleCustom::getLineGraphics`, `getLinePadding`, `getLineWidth`, `getLineLeft`, `getImageBox`. The last four follow `StyleMarkdown`'s family (`jam_StyleMarkdown.h:182`, `:252`).
- `Nexus::getOrCreatePlugin`.
- `Id::imageHash`, sibling of `Id::path` on a box. `ImageJob`, sibling of `FileJob` and `RecordJob`.
- From Part B: the operation `scrollbar`, the rows `programLines`, `programTop`, `programRows`, the member `programBar`, `StyleCustom::flex`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Part A: Bugs

### Step 1: The plugin follows getOrCreate
**Scope:** `end/Source/plugins/Nexus.h`, `Nexus.cpp`, `end/Source/ui/TabView.cpp`
**Action:** Add `Nexus::getOrCreatePlugin`, which returns the `files::evePlugin` file. It checks the disk, and it runs the archive extraction when the file is absent or the stamp differs. `TabView::loadPlugin` (`:29`), `registerEVE` (`:81`) and the constructor (`:51`) use it.
**Validation:** Ruling 11. Sibling `ConfigModel.cpp:142-143`. No bail-out. One test of the disk.

### Step 2: One path for a new pane
**Scope:** `end/Source/ui/TabView.*`, `end/Source/ui/ENDView.cpp`
**Action:** `TabView::addPane` is the one path: it adds or splits, writes the `cwd`, then loads EVE. Delete the pairs at `ENDView.cpp:191-192` and `:350-351`. The drag split at `TabView.cpp:150-152` calls `addPane`. The `loadPlugin` command stays, for a pane whose plugin closed. The drop keeps its own file (`EditorView.cpp:97`), because it names a file.
**Validation:** Ruling 10. A write of the `cwd` comes before the load, because `addPlugin` sends the pane rows to the plugin (`Session.cpp:114`).

### Step 3: `ctrl+l` runs `clear`
**Scope:** `eve/Source/layout/default-eve.md`, `eve/Source/EVEView.cpp`, `eve/Source/EVETerminalProcessor.cpp`, `jam/jam_terminal/processor/jam_TerminalProcessor.*`, `eve/SPEC.md`
**Action:**
1. The `clear` row takes the value `ctrl+l` and the mode `terminal`, and its description becomes *"Redraw the active prompt."*.
2. On the normal screen, `EVETerminalProcessor::clear` calls `tty->setWinsize (getWinsize())` and nothing else. The shell redraws its prompt on `SIGWINCH`.
3. On the alternate screen, the `clear` callback sends the byte 0x0C through `addInput`.
4. Delete `TerminalProcessor::clear` (`:42-48`), which then has no caller.
5. Amend the `clear` paragraph of `SPEC.md`. The typed `clear` still erases history through its ED 3 bytes (`:982`).
**Validation:** Rulings 13, 14, 15, 16. No row, no frame and no history line changes. The cursor stays, because `reset` does not run.

### Step 4: `ctrl` plus a non-letter key
**Scope:** `jam/jam_terminal/keyboard/jam_TerminalKeyboard.cpp`
**Action:** `mapCtrl` gains the cursor, function and editing branches, with the helpers that `mapAlt` calls (`:119-130`).
**Validation:** Ruling 12: *"Alt + B or Ctrl + Left"*, *"Alt + F or Ctrl + Right"*. No new encoder: `getCursorKey` already encodes modifiers (`:369-374`).

### Step 5: `shift+tab`
**Scope:** `jam/jam_terminal/keyboard/jam_TerminalKeyboard.*`, its generated constants
**Action:** `mapPlain` sends `CSI Z` when shift is down on the tab key, with `Chars::csiIntroducer`.
**Validation:** `fzf/src/tui/light.go:738-739`. A named constant holds the final byte.

### Step 6: The console messages
**Scope:** `jam/jam_vulkan/` at `jam_VulkanImagePixelData.h:409`, `jam_VulkanGraphics.cpp:1510`, `:1529`, `:1546`, `jam_VulkanBindlessRegistry.cpp:24`, `:102`
**Action:** The cast on a plugin's `SoftwarePixelData` stops. The owner tests the image type before the cast. @Engineer reads `juce::ImagePixelData::createType` and `vulkanImageTypeID` first. `createType` allocates on each call, so the test uses one constant of the owner.
**Validation:** Ruling 17. No console line under END with a markdown image. No allocation per paint.

### Part B: Scrollbar and theme

### Step 7: Scroll up wins
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`, `eve/SPEC.md`
**Action:** In `addBlocks`, read whether the view sits at the tail before `setContentSize`, from the view position, the content height and the view height. Call `setViewToEnd` only when it did. Keep `:574-575` and `:828-829`. Amend `SPEC.md`: *"In TERMINAL mode, new output moves the view to the tail"*.
**Validation:** Ruling 18. No flag: the view position is the one truth. Reaching the bottom resumes the follow.

### Step 8: One predicate for the bar
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`
**Action:** One function sets the bar from focus and from the screen: `hasKeyboardFocus (true)`, and not the alternate screen. `focusOfChildComponentChanged` and the `Id::alternateScreen` row call it. It replaces the `Id::isRecording` line (`:845`), because a running command on the normal screen keeps its history bar.
**Validation:** Ruling 22. The thickness stays trimmed (`:1177`), so no reflow and no winsize write.

### Step 9: Flex in the style
**Scope:** `jam/jam_style/jam_StyleCustom.*`
**Action:** `StyleCustom` holds `Svg::Flex::Segments flex`, set by its owner from the svg text, with `colourScheme` (`jam_StyleCustom.h:232`). Override `drawScrollbar`: authored segments paint the thumb through `Svg::Flex::paint`, rotated as `drawPaneEdge` rotates a vertical bar. Otherwise call `juce::LookAndFeel_V4::drawScrollbar`.
**Validation:** Ruling 23. One painter (ruling 20). The art keeps its own colours.

### Step 10: The app config moves
**Scope:** the row that generates `Files.cpp:35-36`, `eve/Source/layout/default-eve.md`, `eve/Source/EVETerminalProcessor.cpp`, `eve/README.md`
**Action:** The settings file becomes `<config>/eve/display.md`. `default-eve.md` keeps `display`, `keys`, `open with`, `settings` and `window`. The shell folder stays at `<config>/shell`, so `:332` takes the config root.
**Validation:** Rulings 25, 28, 29. `Id::padding` stays reachable (`jam_ViewManager.h:232`).

### Step 11: Style tables leave the app file
**Superseded** by `PLAN-style-isomorphism.md` (`:3`).
**Scope:** `jam/jam_plugin_bootstrap/layout/jam_PluginEditorLayout.*`
**Action:** `getDefaultSettings` writes only `default-eve.md`. A second path reads the theme. `isEND()`: read `<config>/themes/<name>/theme.md` and `flex/`, where `<name>` is the `theme` row of `<config>/display.md`, and write nothing. Any other host: `ConfigDocument::getOrCreateDefaultFile` on `<config>/eve/theme.md` with EVE's binary tables. EVE's binary tables gain `scrollbar`, `overlay`, `menu` and the `window` background row, with END's key names. Both paths go through `addTables`.
**Validation:** Rulings 26, 27, 28. ARCHITECT: *"INVALID should show error message and continue to render with default value"*. The theme's `window` keys and the app `window` keys share no key.

### Step 12: END's theme gains the tables
**Scope:** `end/Source/config/theme/gfx/theme.md`
**Action:** Add EVE's style tables, so END's `addTables` accepts them: `terminal`, `text editor`, `markdown`, `mermaid`, `fonts`, and the `colours` rows these tables name.
**Validation:** Ruling 26. `addTables` needs each table in END's default (`jam_ConfigDocument.cpp:505-519`).

### Step 13: Shared tables reach the style
**Superseded** by `PLAN-style-isomorphism.md` (`:3`).
**Scope:** `jam/jam_style/`, `eve/Source/EVEView.cpp`, `eve/Source/layout/style.css`
**Action:** Delete the `.appearance` block (`style.css:3-7`). EVE maps the theme's `window` background and `scrollbar`, `overlay` and `menu` rows to colour ids, in one table of the `ENDLookAndFeel.cpp:523-537` form. It loads `resizerBar.svg` into `StyleCustom::flex` for each style. A change of the theme file runs the same load through `fileChanged`.
**Validation:** Rulings 24, 29. A table, not a branch. The watcher covers the config folder (`jam_PluginEditor.cpp:76`).

### Step 14: The operation
**Discarded** with Steps 15 and 16. ARCHITECT: *"discard all alternate scrollbar. remove all instrumentation."*
**Scope:** `jam/cast/terminal.md`, `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`, `eve/Source/EVETerminalProcessor.cpp`, `eve/Source/layout/parameters.md`, `eve/Source/EVEProcessor.cpp`
**Action:** `ESC ] 1337 ; EVE ; scrollbar ; lines=N;top=N;rows=N BEL` stores three integer rows. Leaving the alternate screen clears them.
**Validation:** *"One envelope, one path"* (`SPEC.md:133-138`). No program named. The tick stores integers.

### Step 15: The program bar
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`
**Action:** A `juce::ScrollBar` child sits in the bar's gutter. The row events set its range. Step 8's predicate shows it on the alternate screen when `programLines` is above 0. It paints through the same `drawScrollbar`.
**Validation:** Ruling 20. `updateVisibleArea` owns the viewport's bar (`juce_Viewport.cpp:431-434`). A program that reports nothing has no bar.

### Step 16: Drag replies to the program
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`
**Action:** `juce::ScrollBar::Listener::scrollBarMoved` sends `ESC ] 1337 ; EVE ; scrollbar ; top=N BEL` through `terminalInput`.
**Validation:** Ruling 21. *"The GUI writes to the terminal through the input lane only"* (`SPEC.md:186`).

### Part C: Absorption

Each step moves a body out of `MarkdownComponent`. The body never exists in two places (CAROL.md Refactor-Rewrite Discipline). The build stays broken from Step 17 until Step 25. The compiler lists what remains.

### Step 17: The style answers per line
**Scope:** `jam/jam_style/jam_StyleCustom.*`, `jam/jam_markdown_graphics/style/jam_StyleMarkdown.*`
**Action:** `StyleCustom` gains virtuals with mono defaults: `getLinePadding (line)`, an empty border; `getLineWidth (viewWidth)`, the view width; `getLineLeft (viewWidth)`, 0; `getLineGraphics (line, left, width)`, an empty `AttributedGraphics`; `getImageBox (image, width)`, the box of Step 21. `StyleMarkdown` overrides them with the bodies of `jam_MarkdownComponent.cpp:335-345`, `jam_StyleMarkdown.h:252` and `MarkdownGraphics`. `StyleMarkdown` owns the drawable cache.
**Validation:** Rulings 2, 8. `jam_style` already includes `jam_graphics`, which owns `AttributedGraphics` (`jam_style.h:37`). No layer inverts.

### Step 18: TextBlock lays out per line
**Scope:** `jam/jam_gui/widgets/jam_TextBlock.*`, `jam/jam_markdown_graphics/component/jam_MarkdownComponent.cpp`
**Action:** `setBlock` takes the style and asks it for each line's fonts and metrics. Move into `TextBlock`: `setTextRows`, the padded and tallest `addRowHeights`, `setTableRows`, `addRow`, and the gap row. The object cell advance moves through `getImageBox`. `isGridLine` keeps its branch.
**Validation:** Ruling 8. A mono style returns the same values, so a terminal block lays out byte-identical (`SPEC.md:282-283`). No second layout path.

### Step 19: One line painter
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`, `jam_TextBlock.*`
**Action:** `TextBlock` holds `AttributedGraphics` per line, from `getLineGraphics`, beside `runs`. `Content::paint` paints the shapes under the text. `paintOverChildren` paints the runs and the link areas. The column of `getLineWidth` and `getLineLeft` places the lines. `paintRows` body leaves `MarkdownComponent`.
**Validation:** Ruling 2. MANIFESTO **E**, Materialisation: the block owns its graphics, built once with the block. `MarkdownGraphics` still derives from `AttributedGraphics` (`jam_MarkdownGraphics.h:29`).

### Step 20: The read-only state
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`, the row in `jam/cast/`
**Action:**
1. `setDocument (std::unique_ptr<Document>)` replaces the one block, adds its lines, sets its rows and shows it from the top.
2. `setReadOnly (bool)` writes `Id::isReadOnly` to `state`, and the `terminal` command does not enter terminal mode while it is true.
3. The constructor gives the editor its own `terminal` tree with mode normal. `bindToModel` replaces it (`:91`).
4. No clock runs, because `bindToProcessor` does not run (`:60`).
5. `getViewedComponent` returns `content`, for the paint targets of Steps 22 and 23.
**Validation:** Rulings 5, 7. `j`, `k`, `gg`, `G`, visual, `y` and `cmd+c` come from the existing tables with no new row (`jam_Gui.h:497-515`). `MarkdownDocument` needs `addLines` before the call, because `jam_gui` does not know it.

### Step 21: Images go to the preview
**Scope:** `jam/jam_gui/widgets/jam_PreviewComponent.*`, `jam_TextEditor.cpp`, `jam_StyleMarkdown.*`, `jam/jam_markdown_graphics/graphics/jam_MarkdownGraphics.*`
**Action:**
1. `setImageBounds` asks `getImageBox`, which returns the box row of an image with its size. The default gives the record image in cells, as `:1291-1298` does today.
2. `StyleMarkdown` returns a box with `Id::path` for a raster file, and a box with `Id::imageHash` for a mermaid block or a vector image. It paints that image through `MermaidGraphics::getImage` or the drawable, and adds it to `juce::ImageCache` under that hash.
3. `PreviewComponent::addJob` starts `ImageJob` for a box with `Id::imageHash`. It reads the cache and publishes one frame.
4. Delete `addImage`, `addObjectImage` from `MarkdownGraphics`, and the diagram members and functions of `MarkdownComponent` (`:287-333`).
**Validation:** Ruling 3. MANIFESTO **E**: the Model row holds a number, never an image. `jam_gui` names no mermaid type.

### Step 22: Paging moves
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`
**Action:** Move `getPageRows`, `paintPage`, `getRowNumber` and `getNumRows` (`jam_MarkdownComponent.cpp:28-59`, `:386-392`) to `TextEditor`, over its one block.
**Validation:** Their callers are `stamp` only (`stamp/Source/Main.cpp:36`, `:58`, `:149`, `ManualPages.cpp:27`, `:32`, `:44`, `:61`, `:126`, `ManualContents.cpp:96`).

### Step 23: CLI help pages
**Scope:** `cast/Source/Help.cpp`, `scout/Source/Help.cpp`, `ggwp/Source/Help.cpp`
**Action:** Each `printHelp` builds a `jam::TextEditor` with `setReadOnly (true)` and `setDocument`, and paints `getViewedComponent` into its `TerminalGraphicsContext`, as `:16-34` does today. The `MarkdownDocument::addLines` call joins the parse.
**Validation:** Ruling 6. `cast --help`, `scout --help` and `ggwp --help` print the same bytes as before. The scrollbar thickness does not narrow the column.

### Step 24: The other consumers
**Scope:** `stamp/Source/Main.cpp`, `Manual.h`, `ManualPages.cpp`, `ManualContents.cpp`, `whelmed/Source/WHELMEDViewComponents.cpp`, `jam/jam_markdown_graphics/component/jam_FilePreview.*`, `eve/Source/EVEView.*`
**Action:** Each replaces `jam::MarkdownComponent` with `jam::TextEditor`, read-only. EVE registers the ephemeral as a `TextEditor` with `Id::markdown` style (`EVEView.cpp:283`, `:315`, `:593-606`). Whelmed keeps `Id::markdown` as its registration key.
**Validation:** Ruling 4. `EVEView::keyPressed` keeps `dismiss` in the `panel` mode (`default-eve.md:49`), and the editor passes keys it does not use up to its parent.

### Step 25: Delete MarkdownComponent
**Scope:** `jam/jam_markdown_graphics/component/jam_MarkdownComponent.*`, `jam_markdown_graphics.h`, its `.cpp`
**Action:** @Engineer deletes the two files and their include lines.
**Validation:** Ruling 4. LSP `findReferences` from each of the six projects returns nothing.

### Part D: OPEN only

### Step 26: LIST mode goes
**Scope:** `jam/cast/gui.md:50-52`, `:265`, `jam/cast/terminal.md:1895-1897`, `jam/jam_gui/widgets/jam_TextEditor.*`, `eve/Source/layout/default-eve.md:51`, `end/Source/config/keys.md:92`, `end/Source/ui/`
**Action:** Delete the `list`, `listDown` and `listUp` rows, the `list` mode, `paintListEntry`, `getListEntry`, `moveListEntry`, the `:590-593` key input, `:1814-1815`, the `open file` row and END's `list` row.
**Validation:** Ruling 30. The compiler lists what remains.

### Step 27: Labels from the name
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`
**Action:** `applyLinkLabel` and `paintLinkLabels` take labels from the file name of each link, `a-z` then `A-Z`, first char preferred, later chars tried, 52 for each page. `tab` in OPEN cycles the page. The page number is a row on `state`.
**Validation:** Ruling 31. Pattern `LinkSpan.h:73-81`, `LinkManager.cpp:54-98`. `linkLabels` (`:5`) goes. No row keeps a label: the label derives from the name.

### Step 28: Column list selection
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.*`, `jam/cast/`
**Action:** The rows `ctrl+shift+j` (previous) and `ctrl+shift+k` (next), mode `open`, move the active column list. The active list starts at the first list found above the prompt. The view scrolls to it. The selection shows only in OPEN.
**Validation:** Rulings 32, 33, 35. The row holds the output block of the list.

### Step 29: Links across the whole history
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`, `jam/jam_terminal/document/jam_AnsiDocument.*`, the record of `cwd`
**Action:** `addFileLinks` runs on each block with its own `cwd`. A URL detector marks web links in cells, with the existing `AttributedString::Link`. Links stay active in every mode that shows them.
**Validation:** Ruling 34. @Engineer reads where OSC 7 lands before this step, and the block row that carries `cwd` is named in the prompt.

### Step 30: Label colour
**Scope:** `eve/Source/textEditor.css:8`
**Action:** `linkLabelTextColourId` takes a visible colour from the sheet.
**Validation:** Ruling 36.

### Part E: Close

### Step 31: Documents
**Scope:** `eve/SPEC.md`, `eve/PLAN-finder-0-tracker.md`, `eve/PLAN-finder-1-markdown.md`, `eve/CLAUDE.md`, `PLAN-scrollbar-theme.md`
**Action:**
1. Amend `SPEC.md`: Feature 4 (LIST text), Feature 5, the Preview family, the `clear` paragraph.
2. The tracker marks rulings 4, 5, 6, 10, 13 and Parts 3, 4, 5 as superseded, with rulings 30 to 36 here. It marks `MarkdownComponent` in rulings 17, 19, 20, 23, 24, 27 as the read-only `TextEditor`.
3. @Engineer deletes `PLAN-scrollbar-theme.md`.
**Validation:** Each decision line quotes ARCHITECT.

### Step 32: Hand over, audit, doxygen
**Action:** ARCHITECT runs `cast` in `jam`, `eve`, `end`, `cast`, `scout`, `ggwp`, `stamp` and `whelmed`, builds, and runs the acceptance list. @Auditor runs once. The doxygen pass runs after ARCHITECT's runtime test.

## Acceptance (ARCHITECT builds and tests)

1. Delete `~/.config/end/plugins` while END runs. A new tab and a new split load EVE, and the folder returns.
2. Every new pane by tab, split and drag loads EVE. A dropped binary still loads that binary.
3. `ctrl+l` at a zsh prompt: the prompt redraws, and no line is lost. In nvim: nvim redraws. A typed `clear` erases the history.
4. `ctrl+left`, `ctrl+right` and `shift+tab` reach the shell. All `ctrl+letter` keys of ruling 12 work.
5. No `dynamic_cast` line on the console with a markdown image.
6. `seq 1 100000`: scroll up. The view stays. Scroll to the bottom: it follows again. A typed key returns to the tail.
7. Two panes: only the focused pane shows the bar. Text does not reflow.
8. Remove the flex file: the default thumb paints. Restore it: the flex thumb paints.
9. Standalone: `eve/display.md` and `eve/theme.md` appear. A bad row shows in the overlay and the default renders.
10. Under END: no `eve/theme.md`. A colour change in `themes/gfx/theme.md` reaches EVE.
11. A test program on the alternate screen sends the operation: the bar shows its position, and a drag sends `top=N`. nvim with no report: no bar.
12. Open a `.md` file with a table, code, an image, an SVG and a mermaid block. It renders as Whelmed does. `j`, `k`, `gg`, `G`, `v`, `y` and `cmd+c` work. `cmd+w` closes it.
13. `cast --help`, `scout --help`, `ggwp --help`: the same output as before.
14. `stamp` writes the same PDF as before, images included. Whelmed renders its harness.
15. A terminal pane renders as before: same rows, same wrap, same images.
16. OPEN: each file has a label from its name. `tab` pages. `ctrl+shift+j` and `ctrl+shift+k` move the list. A URL and an older list's file open.

## BLESSED Alignment

- **Bound:** the block owns its graphics. The preview owns its frames. The style owns its drawable cache. Nexus owns the plugin on disk.
- **Lean:** one text widget replaces two. One path adds a pane. One painter draws the bar.
- **Explicit:** each style virtual names what it yields. The rows name the program's report.
- **SSOT:** the document is the text. The disk is the plugin's truth. The view position is the follow state. END's theme is the default under END.
- **Stateless:** no follow flag, no bar state, no label table. `isReadOnly` is a Model row.
- **Encapsulation:** `jam_gui` names no markdown or mermaid type. EVE never writes END's files. Nothing in EVE names a program.
- **Deterministic:** the same rows give the same view. A mono style gives the terminal layout unchanged.

## Risks / Open Questions

Each question closes before approval.

1. **Scope.** ARCHITECT named *"couple of CLI"* help pages. Deleting `MarkdownComponent` (ruling 4) also needs `stamp` and `whelmed` (`stamp/Source/Main.cpp:11`, `WHELMEDViewComponents.cpp:11`). Step 24 includes them. ARCHITECT confirms.
2. **`stamp` images.** `stamp` paints one page once, into a PDF context (`stamp/Source/Main.cpp:143-149`). `PreviewComponent` decodes on a pool and applies frames on an async update (`jam_PreviewComponent.cpp:358-363`). So an image in a PDF page has no frame at paint time. ARCHITECT rules how the PDF path gets its images.
3. **ED 2 from a program.** `SPEC.md:284-285` says cleared lines retire to history. `jam_TerminalProcessor.cpp:981` blanks them. Step 3 stops `ctrl+l` from sending ED 2 at a prompt, but any program can still send it. ARCHITECT rules which one holds.
4. **`alt+b`, `alt+f`, `alt+.`** Ruling 12 lists them. They send ESC plus the event's text character (`jam_TerminalKeyboard.cpp:147-151`, `juce_NSViewComponentPeer_mac.mm:874`). Sending the key's letter makes them work, but Option can then no longer compose a character. ARCHITECT rules on the default.
5. **`ctrl+v`.** It is bound to `visualBlock` in terminal mode (`jam_Gui.h:512`, `:580`), so the shell never gets it. It is not in ruling 12's list. ARCHITECT rules whether it stays.
6. **Part D here.** ARCHITECT said *"fold into plan"* about the new-tab bug and the scrollbar. The OPEN-only rulings came earlier and were not named. Part D is here because they change the same widget. ARCHITECT confirms or moves it to its own plan.
7. **The name.** ARCHITECT says `ImagePreviewComponent` (ruling 3, tracker ruling 22). This plan keeps `PreviewComponent`, because `PLAN-finder-2-preview.md` holds the rename. ARCHITECT confirms.
8. **Layout cost.** Step 18 asks the style for each line. `getLineFonts` returns by value (`jam_StyleCustom.h:107`), and a live block rebuilds on each vblank with new lines (`jam_TextEditor.cpp:961-962`). This runs on the message thread, so no deadline thread breaks. No measurement exists yet. A `scout` run on the terminal path gives one if ARCHITECT asks.
9. END's disk `theme.md` is version 0.0.1. It gains the new tables only when ARCHITECT rewrites it (`SPEC.md:839`). Until then, EVE under END reads no `## markdown` table from it and renders the binary default.

## Amendment (locked)

### New rulings, verbatim

37. *"all project using MarkdownComponent migrated. runtime behaviour must be identical. we only swap the Component"*
38. *"stamp need to conform. only need to merge the the paint/graphics into single blit no? or Preview could have accessor to get its Images. so TextEditor OPTIONALLY could render the image in a single paint blit."*
39. *"TextEditor rendering images is not new, this actually the pattern in the past. Decoupling into PreviewComponent does not change the principles, only concern separation"*
40. *"why ctrl+l not just same path with SIGWINCH? clear live buffer redraw active prompt with whatever winsize parameter?"*

41. *"we should never scan full history. every incoming block always known, the AST always complete, the state of blocks could be immediately stored as parameters, a STATE: how many block, its absolute position (reflow agnostic), etc"*

### Step 28 (adds, ruling 41)

42. *"BLESSED compliance design, unidirectional, always SSOT. there is no excuses we must perform heavy operation on anything. each terminal command is known, we have always known the incoming data, we always know the position, Document already using b-rope for direct fast lookup even in huge document, it does not matter whether the swap file gone, it the history, hence the document state must always synced to whatever Model state need to operate any registered callback. there is no manual management of state, no manual scan, everything is event driven"*

The Model holds the list state: the `gridStarts` child of the terminal tree has one `gridStart` row for each column list, with its absolute line number, and its `activeGridStart` property holds the active list. The record write fires the events `gridStart`, `removedGridStart` and `firstLine`. EVE writes the rows from these events. A new list becomes the active list. The list keys write `activeGridStart`. The editor reacts to the row, and it loads only the block that holds the list. `AnsiDocument::gridLines` gives the line of a list in its block, built with the AST. A block outside the view and outside the tail stays out.

43. *"Whelmed (markdown), should always hide cursor. cursor should only visible for selection: entering visual mode by dragging - entering V-BLOCK, V-LINE - showing status bar - so the contract of whelmed actually default of TextEditor, EXCEPT its NORMAL mode hide both status bar and cursor - we could make this `mode` as parameter on its own, let say `READER`, a registrable `mode`"*
44. *"LINK(s) should be enabled always for the entire history"*; *"OPEN mode `active` block always starts from latest (bottom), found column list above active prompt, so marking `active` column block always n-1 of num column list block found in history. -1 means no column list"*; *"active column list, updated and back to n-1, when there's new column list being added from terminal. so it always from latest to oldest, bottom to top."*; *"`active` column list to OPEN and cycle ALWAYS 1, the selected only. tab cycle only on `active`"*

### READER (ruling 43)

45. *"in READER mode, jk always scroll, not operating cursor - only mouse drag, visual mode (block, line, etc) move cursor and selection"*; *"READER mode could just be TextEditor canon mode. not registrable, like TERMINAL. hence the contract and implementation follow established pattern"*; *"command for scrolling, moving cursor, etc should be registered TextEditor commandId, this way there is no branching and manual checking. each mode have shared keyboard to command, also each mode have their own `scheme`"*

46. *"TextEditor is NORMAL by default, always TextEditor in vim core parity feature"*; *"External caller can set explicitly to TERMINAL or READER, downstream all flow, keymapping to command, return command (from what state), etc dictated by those state"*; *"`default` mode is NORMAL, TERMINAL, READER. so returning to `default` with ESC from modal state always return to NORMAL, TERMINAL, READER"*

The editor starts in NORMAL. The row `defaultMode` holds its default mode. `setDocument` tells READER. EVE's Model row tells TERMINAL. The command `defaultMode` holds the `escape` key row and enters that mode. A yank and the second press of a visual key enter it too. `setMode` holds no check.

`READER` is a canon row of `map::EditorMode`, with its own key input as TERMINAL has. It hides the status bar and the cursor. Its key rows are the commands `scrollDown`, `scrollUp`, `scrollDocumentStart`, `scrollDocumentEnd`, `scrollHalfPageUp`, `scrollHalfPageDown` and the three visual entries. It has no `escape` row, so `escape` passes up. Visual mode shows the cursor and the status bar. A keyboard entry to visual mode starts at the top line of the view.

### OPEN labels (ruling 44)

Labels and `tab` act on the active column list only. `activeGridStart` holds the absolute line of the last row, or -1 when no row exists.

### Open questions: all closed

1. `stamp` and `whelmed` migrate: ruling 37.
2. `stamp` images: accessor plus single blit, ruling 38 (Step 22 below).
3. ED 2 from a program: no code change. ED 2 blanks the frame (`jam_TerminalProcessor.cpp:981`). Ruling 40 uses this blank: *"clear live buffer"*. Ruling 16 needs it for typed `clear`. Step 3 amends `SPEC.md:284-285` and `:868`.
4. `alt+b`, `alt+f`, `alt+.`: ruling 12. Step 4 sends the key's letter. Option no longer composes a character in a pane.
5. `ctrl+v`: no ruling changes it. The row stays.
6. Part D stays in this plan.
7. The name stays `PreviewComponent`. The rename stays in `PLAN-finder-2-preview.md`.

Items 8 and 9 stay as notes.

### Step 3 (overrides)

**Scope:** `eve/Source/layout/default-eve.md`, `eve/Source/EVEView.cpp`, `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`, `eve/SPEC.md`
**Action:**
1. The `clear` row takes the value `ctrl+l` and the mode `terminal`. Its description becomes *"Clear the screen and redraw the active prompt."*.
2. Delete the ED 3 line from `TerminalProcessor::clear` (`jam_TerminalProcessor.cpp:45`). `EVETerminalProcessor::clear` (`EVETerminalProcessor.cpp:402-410`) does the rest: the restart writes pending history, ED 2 blanks the frame, the cursor goes home, and the clock start runs `prepare` (`:308-309`), which calls `tty->setWinsize` (`:387`).
3. On the alternate screen, the `clear` path sends the byte 0x0C through `addInput`.
4. Amend `SPEC.md`: the `clear` paragraph, `:284-285` and `:868`.
**Validation:** Rulings 13, 15, 16, 40. No history line changes. The typed `clear` alone erases history, through its ED 3 bytes (`:982`).

### Step 4 (adds)

`mapAlt` sends ESC plus the key's letter for a text key, from `code` and shift, not from the event's text character (`jam_TerminalKeyboard.cpp:131-158`).

### Part C (replaces Steps 17 to 25 above)

Each step moves a body out of `MarkdownComponent`. The body never exists in two places. The build stays broken from Step 17 until Step 25.

**The swap contract (ruling 37).** A consumer changes only the type name. These calls stay as they are today:
- `setDocument (jam::MarkdownDocument::parse (...))`: `cast/scout/ggwp Help.cpp:19`, `stamp/Source/Main.cpp:17`, `WHELMEDViewComponents.cpp:32`, `jam_FilePreview.cpp:76`, `EVEView.cpp:606`.
- `getViewedComponent` (`Help.cpp:21`, `:28`), `setScrollBarsShown` (`stamp/Source/Main.cpp:15`), `getPageRows`, `paintPage`, `getRowNumber`, `getNumRows`.

**Step 17: The style answers per line.** `StyleCustom` gains virtuals with mono defaults: `getLinePadding (line)`, `getLineWidth (viewWidth)`, `getLineLeft (viewWidth)`, `getLineGraphics (line, left, width)`, `getImageBox (image, width)`. `StyleMarkdown` overrides them with the bodies of `jam_MarkdownComponent.cpp:335-345`, `jam_StyleMarkdown.h:252` and `MarkdownGraphics`. It owns the drawable cache. It sets `juce::TextEditor::backgroundColourId` from the markdown background (`jam_TextEditor.cpp:1801`, `jam_MarkdownComponent.cpp:63`, `:359`).

**Step 18: TextBlock holds any document and lays out per line.**
1. `TextBlock::document` becomes `std::unique_ptr<Document>` (`jam_TextBlock.h:14`, `jam_Document.h:904`). `addFileLinks` keeps its `AnsiDocument` reference from the new block.
2. `setBlock` takes the style and asks it for each line's fonts and metrics.
3. Move into `TextBlock`: `setTextRows`, the padded and tallest `addRowHeights`, `setTableRows`, `addRow`, the gap row (`jam_MarkdownComponent.cpp:155-158`, `:206-280`).

**Step 19: One line painter.** `TextBlock` holds `AttributedGraphics` per line, from `getLineGraphics`, beside `runs`. `Content::paint` paints the shapes. `paintOverChildren` paints the runs and the link areas. `getLineWidth` and `getLineLeft` place the column.

**Step 20: The read-only state.**
1. `setDocument` is a template in the header. It moves the document into a `std::unique_ptr`, calls its `addLines`, replaces the one block, and calls `setReadOnly (true)`. `jam_gui` names no markdown type.
2. `setReadOnly (bool)` writes `Id::isReadOnly` to `state`. The `terminal` command does not enter terminal mode while it is true.
3. The constructor gives the editor its own `terminal` tree with mode normal. `bindToModel` replaces it (`:91`).
4. `getViewedComponent` and `setScrollBarsShown` forward to `viewport`.
5. While read-only, behaviour matches `MarkdownComponent`: content width is `viewport.getMaximumVisibleWidth()` (`jam_MarkdownComponent.cpp:284`, `:337`); `getRowOffset` is 0 (`jam_TextEditor.cpp:1419-1441`); a width change and `lookAndFeelChanged` lay out again (`jam_MarkdownComponent.cpp:66-81`); a key that no row takes goes to `viewport.keyPressed` (`juce_Viewport.cpp:610-631`); no winsize write.

**Step 21: Images go to the preview.**
1. `TextEditor` owns a `std::unique_ptr<PreviewComponent>`, made by `setDocument` at the first image box and bound to `images` (`jam_gui.h:136`, `:158`).
2. `setImageBounds` asks `getImageBox`. The default gives the record image in cells, as `jam_TextEditor.cpp:1291-1298` does today.
3. `StyleMarkdown` returns a box with `Id::path` for a raster file. For a mermaid block it returns a box with `Id::imageHash`: it paints through `MermaidGraphics::getImage` and adds the image to `juce::ImageCache`.
4. `PreviewComponent::addJob` reads a hash box from the cache and applies its one frame at once, with no job.
5. A vector image stays a drawable in `MarkdownGraphics` (`jam_AttributedGraphics.h:350-354`), so a PDF keeps its vectors.
6. Delete `addImage` from `MarkdownGraphics`, and the diagram members and functions of `MarkdownComponent` (`:287-333`). The box of a diagram is `getDiagramTarget` (`:307-313`).

**Step 22: Paging and the single blit.**
1. Move `getPageRows`, `getRowNumber` and `getNumRows` to `TextEditor` (`jam_MarkdownComponent.cpp:28-59`).
2. `PreviewComponent::getImage (box)` waits for the job of the box with `decodeJobs->waitForJobToFinish`, applies the frames, and returns the current frame.
3. `TextEditor::paintPage (g, rows)` paints in one pass, in `Content`'s order: the fills and shapes, then each image of `rows` through `getImage` at its box, then the runs. It writes the box of each image in `rows`.
4. The live view does not change: the preview stays a child.

**Step 23: CLI help pages.** Replace `jam::MarkdownComponent` with `jam::TextEditor` at `Help.cpp:16` in `cast`, `scout` and `ggwp`. `--help` prints the same bytes.

**Step 24: The other consumers.** Replace the type name in `stamp`, `whelmed`, `FilePreview` and `eve`. EVE and Whelmed keep `Id::markdown` as the registration key.

**Step 25: Delete MarkdownComponent.** @Engineer deletes the two files and their include lines. LSP `findReferences` from each of the six projects returns nothing.

### Acceptance changes

- Item 3 becomes: `ctrl+l` at a zsh prompt: the screen clears, the prompt redraws at the top, and history above it stays reachable. In nvim: nvim redraws. A typed `clear` erases the history.
- Item 4 adds: `alt+b`, `alt+f` and `alt+.` reach the shell.
- Item 12 adds: a markdown file scrolls in pixels, as in Whelmed, and arrow and page keys still work.
- Item 14 becomes: `stamp` writes the same PDF as before, with raster, vector and mermaid images. Whelmed behaves as before, plus the vim keys.
