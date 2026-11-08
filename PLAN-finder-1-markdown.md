# PLAN: Finder Part 1 — Markdown Opens as an Ephemeral Component

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 7, 16 to 21, 27, 28

## Overview

[AMENDED 2026-11-07] `jam::MarkdownComponent` below names the read-only `jam::TextEditor`. ARCHITECT: *"MarkdownComponent could be deleted"*.

A click on a `.md` file creates a `jam::MarkdownComponent` that EVEView owns. It renders as Whelmed renders, in front of the TextEditor, with the terminal live behind it. `cmd+w` closes it and destroys it, and that key wins while it is open. The side panels and the current markdown path go first.

## Language / Framework Constraints

- One object, one translation unit (`LANGUAGE.md:34-35`). This part adds no object.
- Delete first (CAROL.md Refactor-Rewrite Discipline): Steps 1 and 2 delete, and Steps 3 to 6 build.
- ARCHITECT builds and tests. Agents do not build.

## Dependency & API Inventory

**On-demand creation, established:**
- `Registry::make`, keyed by type: *"Component factories"* (`jam_Registry.h:55`). `Registry::style`, keyed by type (`:56`).
- `ViewManager::addComponent` with `styleName` creates one component, sets its style, applies appearance and fonts, and moves both into their `Owner`s (`jam_ViewManager.cpp:148-184`).
- `ViewManager::setAppearance (view, lafs, lightOrDark)` applies an appearance to an `Owner<StyleCustom>` (`jam_ViewManager.cpp:330-333`). `ViewEditor` uses it (`jam_ViewEditor.cpp:9-12`).
- `PluginEditor::setAppearance` is virtual (`jam_PluginEditor.h:96`). It ends with `sendLookAndFeelChange` (`jam_PluginEditor.cpp:147-154`), which reaches every child (`juce_Component.cpp:2213-2233`). `fileChanged` calls it on a config change (`jam_PluginEditor.cpp:198-214`).

**State to callback, established:** a change of `Id::value` on a tree with `Id::id` queues the id. `handleAsyncUpdate` runs the registry callback of that key (`jam_PluginEditor.cpp:188-196`, `:216-229`). EVE registers such a callback for `Id::message` (`EVEView.cpp:412-420`).

**Keys, established:** `EVEView::keyPressed` tests a key against the markdown state (`EVEView.cpp:86-91`). A row with a mode matches only when the state holds that mode (`jam_CommandManager.cpp:65-70`). JUCE walks from the focused component up through its parents, and the first use ends the walk (`juce_ComponentPeer.cpp:207-228`). END binds `cmd+w` in every mode (`end/Source/config/keys.md:24`), on a key listener of `ENDView` (`ENDView.cpp:55-58`).

**Render, established:**
- `MarkdownComponent::setDocument (MarkdownDocument&&)` (`jam_MarkdownComponent.h:52`). `MarkdownDocument::parse (std::string_view, origin)` (`jam_MarkdownDocument.h:83`). `juce::MemoryMappedFile` (`EVEView.cpp:593`).
- Whelmed's sheet: `@markdown-style` is `../jam/resources/markdown.css` (`whelmed/project-info.md:20`), embedded as `markdownStyleSheet` (`:382`). Whelmed's `mermaidStyleSheet` is beside it (`:384`).
- EVE's sheet differs: 95 lines, `lineHeight: 1em`, `margin: 0em`, no `fontSize` (`eve/Source/layout/markdown.css:43`, `:79`). Whelmed's has `fontSize: 16`, `lineHeight: 1.5em`, `maxLineWidth: 43.75em`, `margin: 2em` (`jam/resources/markdown.css:196-197`, `:233-234`).

**New names:** none outside the families above. `EVEView` members `ephemeralStyles` and `ephemerals` follow `ViewEditor::lookAndFeels` and `children` (`jam_ViewEditor.h:160-162`); ARCHITECT ruled `Owner<Component>` (`PLAN-Grand-Scheme.md:812-814`).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Delete the side panels
**Scope:** `eve/Source/EVEView.h`, `.cpp`, `eve/Source/layout/default-eve.md`
**Action:** Delete:
1. `juce::FileBrowserListener` (`EVEView.h:25`), `selectionChanged`, `fileClicked`, `fileDoubleClicked`, `browserRootChanged` (`EVEView.cpp:611-623`).
2. `filePreview`, `leftPanel`, `rightPanel`, `directoryThread`, `directoryList`, `panels` (`EVEView.h:470-490`).
3. `getPanel`, `getTreeRoot`, `addPanels`, `setPanelsVisible`, `setPanelFile` (`EVEView.cpp:625-640`, `:658-727`).
4. The trim in `resized` (`EVEView.cpp:42-52`), `isListMode` (`:130-133`), the constructor lines (`:24-25`).
5. The `terminalEvents` entries `Id::url` and `Id::mode` (`:157`, `:197-203`).
6. The rows `preview width`, `tree depth`, `tree width` (`default-eve.md:21`, `:33`, `:35`). Parts 7 and 8 add their own rows.

**Validation:** Ruling 7. `getUrlFile` stays, because `openFile` uses it (`EVEView.cpp:557`). `jam::FilePreview` stays as a class until Part 2. LIST mode keeps its keys and its highlight until Part 5.

### Step 2: Delete the markdown path
**Scope:** `eve/Source/layout/component.md`, `eve/Source/EVEView.h`, `.cpp`, `eve/Source/layout/markdown.css`, `eve/project-info.md`
**Action:** Delete:
1. Row 2 of the component table (`component.md:12-13`).
2. The `registerConfig<jam::MarkdownComponent>` (`EVEView.cpp:303-308`).
3. `applyToMarkdown` (`EVEView.h:381-385`), `openMarkdown`, `setMarkdownVisible` (`EVEView.cpp:590-609`).
4. The constructor lines 10 to 13 (`EVEView.cpp:10-13`).
5. The callbacks at `EVEView.cpp:360-370` and `:394-410`.
6. The file `Source/layout/markdown.css`.

**Validation:** Ruling 21. `registerComponent<jam::MarkdownComponent>` and `registerStyle<jam::StyleMarkdown>` stay (`EVEView.cpp:287`, `:326`), because Step 4 uses them. The `.md` row stays (`default-eve.md:152`).

### Step 3: Whelmed's sheet
**Scope:** `eve/project-info.md`
**Action:** Add the rows that Whelmed has, verbatim: `@markdown-style` and `markdownStyleSheet` (`whelmed/project-info.md:20`, `:382`). EVE's `## binary` table holds `terminalStyleSheet` as the sibling (`eve/project-info.md:421`).
**Validation:** `LayoutDocuments` parses every binary-data resource with a known extension, css included (`jam_PluginEditorLayout.h:270-302`), so a `## binary` row and a `Source/layout/*.css` glob file (`project-info.md:431`) reach the parse the same way. `CLAUDE.md` says each css file gives one table of `eve.md`; COUNSELOR amends that line for `markdown.css`. EVE keeps `Source/layout/mermaid.css`, because Whelmed's pair is a separate row (`whelmed/project-info.md:384`) and no ruling names it.

### Step 4: One state row, one callback
**Scope:** `eve/Source/EVEView.h`, `.cpp`
**Action:**
1. Command `markdown` (`EVEView.cpp:360`): it writes the path of `getUrlFile()` to the `Id::value` row of the `markdown` component state, through `model.getOrCreateComponentProperty`, the call at `:608`.
2. The registry callback `jam::Format::toType (Id::markdown)` reads that row. A non-empty value calls `applyMarkdown` (Step 5). An empty value clears `ephemerals` and `ephemeralStyles`, removes `Id::mode` from the markdown state, and calls `grabKeyboardFocus`.
3. Command `dismiss`: it writes an empty `Id::value`.
4. The constructor writes an empty `Id::value`.

**Validation:** No boolean row. The path is the one truth: empty is closed. MANIFESTO: *"State belongs exclusively to the Model."* `focusGained` gives the focus to the TextEditor (`EVEView.cpp:77-84`).

### Step 5: Create on demand
**Scope:** `eve/Source/EVEView.h`, `.cpp`
**Action:** `applyMarkdown`:
1. Clear `ephemerals` and `ephemeralStyles`, then call `jam::ViewManager::addComponent` with type `Id::markdown` and style `Id::markdown` (`jam_ViewManager.cpp:148-184`). It gives the component, owned by `ephemerals`, with its style owned by `ephemeralStyles`.
2. Parse the file with `juce::MemoryMappedFile` and `MarkdownDocument::parse`, then call `setDocument`.
3. `addAndMakeVisible`, `setWantsKeyboardFocus (true)`, `toFront (true)`, then `messageOverlay.toFront (false)`, because `messageOverlay` must stay over every child (`EVEView.cpp:18`).
4. Write `Id::mode` = `map::EditorMode::panel` to the markdown state. `EVEView.cpp:12` holds the current form.
5. Member order: `ephemeralStyles` before `ephemerals`, so the component dies before its style.

**Validation:** Ruling 19. The `addComponent` overload at `jam_ViewManager.h:367` is below `private:` (`:237`), so JAM moves its declaration to the public section, and no second factory appears. A `MemoryMappedFile` is the permitted OS boundary read, and the document is a creation (MANIFESTO **E**, Complete at Creation).

### Step 6: Bounds and appearance
**Scope:** `eve/Source/EVEView.h`, `.cpp`
**Action:**
1. `resized` sets each child of `ephemerals` to `getLocalBounds()`.
2. Override `setAppearance`: `jam::ViewManager::setAppearance (this, ephemeralStyles, model.getAppearance())`, then `jam::PluginEditor::setAppearance()`.

**Validation:** *"full View sized "panel""* (`PLAN-Grand-Scheme.md:781-782`). The virtual is at `jam_PluginEditor.h:96`. The base sends the LookAndFeel change that reaches the child (`jam_PluginEditor.cpp:153`).

### Step 7: The key
**Scope:** `eve/Source/layout/default-eve.md`
**Action:** The `dismiss` row takes the value `cmd+w`, with mode `panel`.
**Validation:** Ruling 20 and ruling 21. With no markdown open, the state holds no `Id::mode`, so the row cannot match (`jam_CommandManager.cpp:67-69`), and `cmd+w` reaches END. With markdown open, EVEView uses the key first (`juce_ComponentPeer.cpp:207-228`).

### Step 8: Hand over, audit, doxygen
**Action:** ARCHITECT builds and runs the acceptance list. @Auditor runs once. Pathfinder writes the doxygen pass after the audit (CAROL.md Code Hygiene).

## Acceptance (ARCHITECT builds and tests)

1. `ls`, then a click on `README.md`: it renders as in Whelmed, mermaid included, full view.
2. The shell still runs behind it: a background job prints, and after close the history holds the output.
3. `cmd+w` closes it, and the TextEditor has the focus. `exit` still closes the pane when no markdown is open.
4. Under END: `cmd+w` closes the markdown and not the pane. A second `cmd+w` closes the pane.
5. A theme hot reload reaches an open markdown.
6. A message overlay shows over the markdown.

## BLESSED Alignment

- **Bound:** EVEView owns the component and its style. The style outlives the component.
- **Lean:** the panels and three functions go. One callback remains.
- **Explicit:** the mode row exists only while markdown is open, so no guard asks.
- **SSOT:** the path row is the one truth of open and closed.
- **Stateless:** the component holds the parsed document only. It is a materialisation of the row.
- **Encapsulation:** `MarkdownComponent` does not know its context (ruling 27).
- **Deterministic:** the same path gives the same view.

## Corrections at execution

Each correction follows from a read and a quoted clause. None changes a ruling.

1. **`resizeEnd`.** Step 1 deletes `isListMode`, so `setResizing (isListMode())` (`EVEView.cpp:102`) becomes `setResizing (false)`. Ruling 7.
2. **Step 4, constructor.** The write runs before `model.state.addListener` (`jam_PluginEditor.cpp:72`), so no callback runs, and a restored state can hold both rows. The constructor also removes `Id::mode`. This plan, BLESSED Alignment: *"the mode row exists only while markdown is open"*.
3. **Step 6, `setAppearance`.** The first argument is `view.get()`, not `this`. The contract is *"Root of the previously-built component tree"* (`jam_ViewManager.h:89`), and the rows resolve against the children of that argument (`:416-420`).
4. **Focus.** `focusGained` takes the first focusable child (`EVEView.cpp:57-64`), which is the TextEditor while `view` is child 0. A pane switch under END would give the keys to the TextEditor. Ruling 19: *"this must always WIN when there's Markdown file opened"*. The markdown sets `setExplicitFocusOrder (1)` (`juce_FocusTraverser.cpp:148-158`). `invokeCommand` then cannot use the traverser default, so it runs through `applyToTextEditors`. This keeps *"terminal actually still active in its backround"*.
5. **Config version.** A version bump was applied and reverted. `SPEC.md:839`: *"agents never change a version."* A disk row with an unknown key rejects its whole table (`jam_ConfigDocument.cpp:470-471`, `:512-513`), and an older disk copy is replaced (`:426-438`). ARCHITECT's disk copy `~/.config/end/eve.md` keeps the old rows until ARCHITECT rewrites it.
6. **Identifiers.** `previewWidth`, `treeDepth` and `treeWidth` leave `eve/cast/identifiers.md`, because Step 1 deleted their last use. Part 7 adds `treeDepth` again.

Audit corrections, each from a finding validated by a read:

7. **Mermaid sheet.** Step 3 kept `Source/layout/mermaid.css` and said *"no ruling names it"*. That sentence was wrong. The sheet has no `.metrics` block, and `StyleMermaid::getMetrics` reads that block (`jam_StyleMermaid.h:72`, `jam_StyleManager.h:150`). Ruling 28: *"a gui Style for actual Markdown preview, this is how whelmed rendered"*. EVE now takes Whelmed's `@mermaid-style` and `mermaidStyleSheet` rows, and the file is deleted.
8. **Restore.** `model.setState` replaces the tree (`EVEProcessor.cpp:181`), so the view runs the `markdown` callback in `onSetState` and at the end of the constructor. The callback is the one writer of `Id::mode`. This replaces Correction 2.
9. **Keys while open.** `EVEView::keyPressed` uses every key while `Id::mode` exists, because an unused Tab moves the focus (`juce_ComponentPeer.cpp:230-234`). Ruling 19.
10. **Focus.** The component takes the focus only when the view had it, and the close path does the same. ARCHITECT: *"Plugin doesnt grab its own focus, only being told"* (`PLAN-Grand-Scheme.md:1146-1148`).
11. **File that does not map.** `addMarkdown` writes the empty row and shows the `failNoOpenFile` message. MANIFESTO **E**: *"Prefer assert over silent fail"*.
12. **Names.** `applyMarkdown` is `addMarkdown` (NAMES Rule 1). `getMarkdownState` is the sibling of `getTerminalState` (MANIFESTO **S**).
13. **Faces.** `style.css` drops the seven faces that `jam/resources/markdown.css` gives with the same values. CODING: *"Project code configures only the difference"*.
13a. **Palette collision.** ARCHITECT's run stopped at `jam_PluginEditorLayout.cpp:64`, which asserts the binary default. `eve/Source/layout/colours.css` and `jam/resources/mermaid.css` both define `--carbon`, and a row takes its type from the light value only (`jam_ConfigDocument.cpp:376`). So `getColourFailures` failed on the dark value `concrete`. `getTypeName` now treats an alias that its own table defines as a string. `StyleManager::getColour` resolves it (`jam_StyleManager.cpp:155-160`).
13b. **`cmd+w` under END.** ARCHITECT: *"cmd+w should close whelmed first."*; *"EVE could have parameter if whelmed is opened, END always know its chilren state"*. The key hook gives END every key first (`juce_ComponentPeer.cpp:201-203`), so the path row moves to the terminal Model row `markdownFile` (`parameters.md`). This replaces Step 4's `Id::value` row and Correction 8. EVE reacts through `terminalEvents`, and `hostRows` sends the change to END. `EditorView::close` clears the row when it holds a path, and closes the pane only when it is empty. A shell exit still closes the pane directly (`EditorView.cpp:211-215`). The `Id::mode` row of the markdown state stays, because `CommandManager::isInMode` reads it (`jam_CommandManager.cpp:65-70`).
13c. **Message overlay.** ARCHITECT: *"INVALID should show error message and continue to render with default value"*. `MessageOverlay::showMessage` faded out at once, from alpha 0, so no message was ever visible (`juce_ComponentAnimator.cpp:61`, `:165`, `:287`). `jam_MessageOverlay.h` takes back the hold of its sibling (`endless/.../MessageOverlay.h:93-101`, `:404-408`): `private juce::Timer`, `messageDelayMs` of 5000 ms, and the fade-out in `timerCallback`.
13d. **Runtime fixes.** ARCHITECT's run showed tiny text and a see-through area. `getBaseFontSize` and `getLineWidth` read raw keys, and the tree keys are `Format::toType` (`jam_Css.cpp:413`), so both missed. They now wrap the key, as `jam_StyleManager.h:150` does. `MarkdownComponent::paint` fills the area under a short document.
14. **Order.** `ephemerals.clear()` runs in the callback, and bounds come before `setDocument`, so layout runs once at the real width (`jam_MarkdownComponent.h:20`). These are implementation details of Steps 4 and 5.

## Risks / Open Questions

None open.
