# PLAN: OPEN Mode

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-05
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)

**Contract for each step (binds this plan):** ARCHITECT builds and tests: *"you dont build. architect build and test runtime"*. *"no new semantics, no magic strings allowed anywhere, no handroll state management, no manual arithmetic, no new pattern, use framework API to its fullest extent, follow established pattern verbatim"*. *"framework API(s) are rich jam first then juce. you probably only need to use it OOTB. if there's genuienely no abstract as SSOT you can use, extend the existing."* Data in transit has *"0 allocation, 0 copy, 0 mutation, 0 temporary container, 0 new struct, and 0 shadow state"* (`MANIFESTO.md:267`). *"we only use juce::String as Input or final Output materialisation to communicate with juce's API. otherwise we always use std::string/string_view everywhere."*

## Overview

OPEN mode shows one key label on each visible link. A label key opens that link.

ARCHITECT: *"user can enter OPEN (jump) mode, this is similar to vim jump flash (?) so each file gets
one character as keyboard shortcut to jump and open immediately"*; *"this is exclusive for "terminal
modal", LIST mode and OPEN mode"*; *"endless ... rewritten from scratch"*.

## Dependency & API Inventory

**JAM:** `map::EditorMode` (`gui.md:244-259`), `textEditorKeys`, `textEditorModes`, `textEditorLayers`,
sizes `@textEditorKey` 24 (`spell.md:134-136`). `getKeyInputs` (`:490-538`), `getCarets` (`:540-571`),
the `Id::mode` handler (`:725-733`), `getActions`, `paintSelection` as the paint shape (`:1409-1433`),
`getCellBounds`, `applyLink` (PLAN-2). ENAX `keys` rule (`jam_ENAXClientExtensions.cpp:39-45`).
END: `EditorView::setMode` (`EditorView.cpp:159-170`), `keys.md:94`, `bimaps.md:91`, `ENDView.cpp:326-331`.
Behaviour only: endless `Input.cpp:298-329`, labels `a` to `z`, space pages.

**New rows:** `EditorMode` `open`; `TextEditorCommandId` `open`; colour ids `linkLabelBackgroundColourId`,
`linkLabelTextColourId`, sibling of `TextEditor` colour rows. **New names:** `getVisibleLinks`,
`paintLinkLabels`, `applyLinkLabel`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor
runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: The mode and its command
**Scope:** `jam/cast/gui.md`, `terminal.md`, `spell.md`, `eve/Source/layout/parameters.md`
**Action:** Add `open` with key 3 to `EditorMode`. Add the `TextEditorCommandId` row `open`
(`0x4800018`), key `o`, mode `terminal`, layer `prefix`. The row `terminal` adds the mode `open`. The
sizes at `spell.md:134-136` become 25. `parameters.md:17` choices become
`normal, visual, terminal, open`.
**Validation:** The tables keep one row order (`terminal.md:1837-1841`). Row counts match the sizes.

### Step 2: The view
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:** `getActions` adds `Id::open`: `editor.setMode (map::EditorMode::open)`. `getCarets` adds a
row that gives an empty rectangle. The `Id::mode` handler shows the status bar for each mode that is
not `terminal`.
**Validation:** The status bar label comes from the mode table (`jam_TextEditorStatusBar.cpp:24-29`).

### Step 3: Labels
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`, `jam/cast/style.md`, `eve/Source/textEditor.css`
**Action:**
1. Private `jam::Array<juce::Point<int>> getVisibleLinks() const;` gives the first cell of each link
   run on the visible lines, in order, history then live.
2. Private `void paintLinkLabels (juce::Graphics& g);` runs at the end of `Content::paint` in `open`
   mode. It fills `getCellBounds` of each cell with the label background, and draws the label with the
   mono font. The label of an index is `juce::String::charToString (labels[index])`, with `labels` a
   named constant of `a` to `z`.
3. A label page is the state property `Id::count`, in the shape of the existing count property: the
   label at index `i` is on page `i / size`. Use `jam` page math through a named constant. `space`
   moves the page.
4. Colour rows and css rows follow `--TextEditor--highlightColourId`.
**Validation:** No member holds a list. Paint reads state and writes nothing.

### Step 4: The key
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:** `getKeyInputs` adds `map::EditorMode::open`: it runs `commandManager->invokeKey` first, for
the exit rows, then `applyLinkLabel (key)`. That finds the cell of the label on the current page,
sets the mode to `terminal`, and calls `applyLink (cell)`.
**Validation:** `applyLink` is the one click path (PLAN-2). Every key is consumed in the mode.

### Step 5: END
**Scope:** `jam/jam_plugin_bootstrap/enax/jam_ENAXClientExtensions.cpp`, `end/Source/config/keys.md`,
`end/cast/bimaps.md`, `end/Source/ui/ENDView.cpp`, `end/Source/ui/EditorView.cpp`
**Action:** The ENAX `keys` rule also removes the `open` row. END adds the row
`open | string | o | prefix` and the command `open` (`0x4700021`), in the shape of `normal`
(`ENDView.cpp:326-331`). `EditorView::setMode` range ends at `map::EditorMode::open`.
**Validation:** ARCHITECT: *"END as host always win"*. The mode travels through the `mode` host parameter.

### Step 6: Docs
**Scope:** `SPEC.md` Keyboard Summary, State Model
**Action:** COUNSELOR adds the mode and the keys.

### Step 7: Hand over, Step 8: Audit, Step 9: Doxygen pass
As PLAN-1 Steps 10 to 12.

## Acceptance (ARCHITECT builds and tests)

1. `` ` `` then `o` after `ls` shows labels. A label opens its file.
2. `space` pages. `escape` leaves.
3. The same under END through END's prefix layer.

## BLESSED Alignment

- **SSOT:** one mode table, one `applyLink`.
- **Stateless:** paint derives the labels from the link runs.

## Risks / Open Questions

None.
