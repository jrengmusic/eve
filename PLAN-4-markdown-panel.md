# PLAN: Markdown Panel

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-05
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)

**Contract for each step (binds this plan):** ARCHITECT builds and tests: *"you dont build. architect build and test runtime"*. *"no new semantics, no magic strings allowed anywhere, no handroll state management, no manual arithmetic, no new pattern, use framework API to its fullest extent, follow established pattern verbatim"*. *"framework API(s) are rich jam first then juce. you probably only need to use it OOTB. if there's genuienely no abstract as SSOT you can use, extend the existing."* Data in transit has *"0 allocation, 0 copy, 0 mutation, 0 temporary container, 0 new struct, and 0 shadow state"* (`MANIFESTO.md:267`). *"we only use juce::String as Input or final Output materialisation to communicate with juce's API. otherwise we always use std::string/string_view everywhere."*

## Overview

A `.md` target opens read-only in `jam::MarkdownComponent`, a full-view sibling of the TextEditor.

ARCHITECT: *"its MarkdownComponent serves as ephemeral EVE childern ... as full View sized "panel" as
sibling of TextEditor"*; *"it's always read-only anyway"*; *"MarkdownComponent basically done, tested,
so we only need to wire the event driven mechanics"*.

## Dependency & API Inventory

**JAM:** `MarkdownComponent::setDocument` (`jam_MarkdownComponent.h:52`), `MarkdownDocument::parse`
(`jam_MarkdownDocument.h:72`), `StyleMarkdown`, `Registry::registerComponent`, `registerStyle`.
Whelmed's wiring, verbatim: `WHELMEDViewComponents.cpp:11`, `:67`; `component.md:8`. END's key entry:
`ENDView.cpp:57-60`. Visibility by a component property: `EVEView.cpp:234-238`, `:311-319`.
**JUCE:** `juce::Viewport::keyPressed`.

**New rows:** `map::CommandId::markdown`, `dismiss`; `map::EditorMode::panel`.
**New name:** `setMarkdownVisible`, sibling of `setPreviewVisible`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor
runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: The sibling
**Scope:** `eve/Source/layout/component.md`, `eve/Source/EVEView.cpp`
**Action:** Add the row `| 2 | markdown | | | | markdown | | |`. Register
`r.registerComponent<jam::MarkdownComponent> (Id::markdown);`, `r.registerStyle<jam::StyleMarkdown> (Id::markdown);`
and a `registerConfig<jam::MarkdownComponent>` that sets `setVisible (false)` and `setWantsKeyboardFocus (true)`.
**Validation:** `applyComponentBounds` gives it the view bounds. `markdown.css` is in the layout glob
(`project-info.md:431`). The row is the owner: the view table owns it through `children`.

### Step 2: The open-with row
**Scope:** `eve/Source/layout/default-eve.md`, `eve/cast/identifiers.md`
**Action:** The `.md` row takes the value `markdown`. Add the `## CommandId` rows `markdown` and `dismiss`.

### Step 3: The command
**Scope:** `eve/Source/EVEView.h`, `.cpp`
**Action:** Registry callback `map::CommandId::markdown`: it finds the child by the view table
(`view->getChildComponent` with the row's `z`, through a named constant), reads the `url` row, maps the
file with `juce::MemoryMappedFile`, calls `setDocument (jam::MarkdownDocument::parse (view, origin))`
with a `std::string_view` over the mapping (`jam_MarkdownDocument.h:83`), then `setMarkdownVisible (true)`. `setMarkdownVisible` writes the `Id::markdown` component property. The
callback `jam::Format::toType (Id::markdown)` applies it: `setVisible`, and `grabKeyboardFocus` when
shown. Callback `map::CommandId::dismiss` writes false. On hide, the TextEditor takes the focus.
**Validation:** The shape of `Id::preview` (`EVEView.cpp:234-238`). `MarkdownComponent` has no edit API.

### Step 4: Keys
**Scope:** `jam/cast/gui.md`, `eve/Source/EVEView.h`, `.cpp`, `eve/Source/layout/default-eve.md`
**Action:** Add the `EditorMode` row `panel` with key 5. The panel component state holds
`Id::mode` = `map::EditorMode::panel`. Override `bool keyPressed (const juce::KeyPress& key) override;`
in EVEView, in the shape of `ENDView.cpp:57-60`:
`return commandManager->invokeKey (key, model.getOrCreateComponentState (jam::Format::toTag (Id::markdown)));`.
Add the key row `dismiss | string | escape q | panel | Close the panel.`.
**Validation:** `juce::Viewport` handles the arrows and passes other keys up. A row with a mode matches
only that mode (`jam_CommandManager.cpp:65-70`), so `escape` stays free in TERMINAL.

### Step 5: Docs
**Scope:** `SPEC.md:341-344`, `:327`
**Action:** COUNSELOR states the panel.

### Step 6: Hand over, Step 7: Audit, Step 8: Doxygen pass
As PLAN-1 Steps 10 to 12.

## Acceptance (ARCHITECT builds and tests)

1. A click on `README.md` shows it rendered, full view, mermaid included.
2. `escape` returns to the terminal with focus. The terminal did not change.
3. Hot reload of the theme reaches the panel.

## BLESSED Alignment

- **Bound:** the view table owns the panel.
- **SSOT:** one document parse, one `openFile` path.
- **Encapsulation:** the terminal path does not change.

## Risks / Open Questions

None. `.md` in a shell uses `nvim README.md` as before.
