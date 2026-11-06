# PLAN: Finder Part 2 — The Preview Family

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 18, 22 to 29

## Overview

The current image painter takes its true name, `ImagePreviewComponent`. A new `PreviewComponent` hosts one content, picked by direct lookup. Image content scales to fit, and all other content scrolls. Image metadata renders through `MarkdownComponent` front matter. The terminal image overlay becomes ephemeral.

## Language / Framework Constraints

- One object, one translation unit. `PreviewComponent` is one header and one `.cpp`.
- The host lives in `jam_gui/widgets`. MANIFESTO: *"Lower layers never know about higher layers."* `jam_markdown_graphics` depends on `jam_gui` (`jam_markdown_graphics.h:16`), so the host holds a `juce::Component` and names no content type.

## Dependency & API Inventory

**Image painter:**
- `jam::PreviewComponent` paints image boxes in one call (`jam_PreviewComponent.h:12-31`). Its placement has only `xLeft | yTop` (`:497`), so `drawImage` scales by the smaller ratio (`jam_PreviewComponent.cpp:412-416`, `juce_RectanglePlacement.cpp:63-64`). Scale to fit already holds.
- `bindTo` adds it to the TextEditor content (`jam_PreviewComponent.cpp:263-267`, `jam_TextEditor.cpp:113-116`). `bindToImages` takes any images tree (`:288-292`).
- `TextEditor::setImageBox` writes a box (`jam_TextEditor.cpp:1329-1335`). `setPreviewBounds` writes the `eve preview` box (`:1337-1362`).

**References, by LSP:** `jam::PreviewComponent`: 45 in 4 files (`jam_PreviewComponent.h`, `.cpp`, `EVEView.cpp:286`, `jam_FilePreview.h:59`). `jam::FilePreview`: 10 in 3 files (`jam_FilePreview.h`, `.cpp`, `EVEView.cpp:668`, `:724`; Part 1 removes the two EVE lines). Data: `component.md:10`, `eve/cast/identifiers.md:34`. The LSP index reaches `end/Source` (the `Id::cwd` references list `ENDView.cpp:191`), so these counts cover END.

**Metadata:**
- `MarkdownDocument::addProperties (NamedValueSet)` adds the block as the first child of the root (`jam_MarkdownDocument.h:453-462`, `.cpp:1349-1357`).
- `addPropertyLines` writes a bold `Properties` header and one line per property (`.cpp:1487-1510`). `getPropertyType` gives text, number, date, date-time, list or tags (`:1408-1423`). `addPropertyRow` draws an icon, the key and the value (`jam_MarkdownGraphics.cpp:192-211`).
- Metrics come from the sheet (`jam/resources/markdown.css:235-245`).
- `FilePreview` already builds the rows: `getFileProperties` gives size, type, created and modified (`jam_FilePreview.cpp:11-22`).

**Ephemeral overlay:** view-table row 1 builds the painter with the view (`component.md:10`). `addBindings` calls `bindTo` for a row with `primary` (`jam_ViewManager.cpp:262-270`). Part 1 gives EVEView its `ephemerals`.

**Whelmed style:** Part 1, Step 3. `FilePreview` takes the LookAndFeel from the markdown component today (`EVEView.cpp:670-673`, removed in Part 1); the host takes its style from its owner.

**New names (ARCHITECT's):** `ImagePreviewComponent`, `FilePreviewComponent`, `PreviewComponent`. `setContent` follows `juce::SidePanel::setContent` (`PLAN-Grand-Scheme.md:825`).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Rename the image painter
**Scope:** `jam/jam_gui/widgets/jam_PreviewComponent.h`, `.cpp` (rename to `jam_ImagePreviewComponent.*`), `jam/jam_gui/jam_gui.h`, `.cpp`, `jam/jam_markdown_graphics/component/jam_FilePreview.h`, `eve/Source/EVEView.cpp:286`, `eve/Source/layout/component.md:10`, `eve/cast/identifiers.md:34`, `jam_TextEditor.h:190` prose
**Action:** `jam::PreviewComponent` becomes `jam::ImagePreviewComponent`. The row type and `Id::previewComponent` become `imagePreviewComponent`. Behaviour does not change.
**Validation:** Ruling 22. LSP `findReferences` on the old name gives nothing. `carol apply` runs any scripted edit.

### Step 2: Rename the file preview
**Scope:** `jam/jam_markdown_graphics/component/jam_FilePreview.h`, `.cpp` (rename to `jam_FilePreviewComponent.*`), `jam/jam_markdown_graphics/jam_markdown_graphics.h`, `.cpp`
**Action:** `jam::FilePreview` becomes `jam::FilePreviewComponent`.
**Validation:** Ruling 23.

### Step 3: The host
**Scope:** `jam/jam_gui/widgets/jam_PreviewComponent.h`, `.cpp` (new), `jam/jam_gui/jam_gui.h`, `.cpp`
**Action:** `jam::PreviewComponent : public juce::Component` holds `std::unique_ptr<juce::Component> content`. `void setContent (std::unique_ptr<juce::Component> newContent)` replaces the whole content, adds it and lays it out. `resized` gives the content the local bounds.
**Validation:** Ruling 23. Creation replaces a complete content, so nothing mutates (MANIFESTO **E**). It names no content type.

### Step 4: The contents
**Scope:** `jam/jam_markdown_graphics/component/jam_FilePreviewComponent.h`, `.cpp`
**Action:** `FilePreviewComponent` keeps its image box and its `MarkdownComponent` (`jam_FilePreview.h:56-62`, `.cpp:80-93`). A `.md` file no longer reaches it, because the lookup of Step 6 gives `MarkdownComponent` directly. It shows an image with metadata, or the metadata and the fenced text of a text file (`.cpp:54-78`), or the metadata of binary content (`SPEC.md:431`).
**Validation:** ARCHITECT selected *"Painter plus markdown"*. `ImagePreviewComponent` scales to fit and plays a GIF (`SPEC.md:434-435`, ruling 26).

### Step 5: The metadata rows
**Scope:** `jam/jam_markdown_graphics/component/jam_FilePreviewComponent.cpp`
**Action:** `getFileProperties` gives size, type, created and modified (`jam_FilePreview.cpp:11-22`). An image adds a dimensions row, from the `juce::ImageFileFormat` that `setImage` already finds (`:36`). No tags (ruling 25).
`addProperties` takes a second argument, `header`, and stores it on the block. `addPropertyLines` reads it, with `Id::properties` as the default, in the form of `block->get<int> (Id::type, default)` (`jam_MarkdownDocument.cpp:1375`, `:1493`). `FilePreviewComponent` passes `Information`.
**Validation:** Rulings 24, 25, and ARCHITECT's choice *"Caller gives it"*. Parsed frontmatter and Whelmed keep the default. The style is the GUI markdown style (ruling 29). The rows are free data: any key, any value (`jam_MarkdownDocument.cpp:1306-1322`, `:1327-1357`).

### Step 5a: `IconId`, the slots of the property icons
**Scope:** `jam/cast/style.md`, `jam/jam_markdown_graphics/graphics/jam_MarkdownGraphics.cpp`, `jam/jam_markdown_graphics/style/jam_StyleMarkdown.*`, `jam/resources/markdown.css`
**Action:**
1. `## IconId` in `style.md`, in the form of `## FontId` (`style.md:846-871`): one row per `map::PropertyType` (`jam/cast/markdown.md:152-161`).
2. The sheet names the value of each slot: `--Markdown--textIconId`, sibling of `--Markdown--textFontId`. The value is an SVG or a codepoint.
3. `addPropertyIcon` reads the slot through the style, as `getFont (int fontId)` reads a font (`jam_StyleMermaid.h:47-52`), and the static table goes (`jam_MarkdownGraphics.cpp:179-184`). The current six files become the default values in `jam/resources/markdown.css`, so nothing changes until a sheet changes.

**Validation:** ARCHITECT: *"colourId better, the data is free. closed table mean the slots fixed but still configurable for any svg / codepoints just like colour and font"*. A slot is selected by the property type, as today (`jam/cast/markdown.md:147-149`). COUNSELOR reads how the sheet value form separates a codepoint from an SVG name before delegation; `@font-face` `src` is the nearest form (`style.css:44-46`).

### Step 6: The lookup
**Scope:** `eve/Source/EVEView.h`, `.cpp`
**Action:** A `jam::Function::Map` named for what it yields (NAMES Rule 7, Rule 8) maps a file to its content: a `.md` file to `MarkdownComponent`, any other file to `FilePreviewComponent`. The call site in Part 8 uses it.
**Validation:** MANIFESTO **L**: *"Replace with direct lookup."* The test for an image stays inside `FilePreviewComponent` (`jam_FilePreview.cpp:36`).

### Step 7: The overlay becomes ephemeral
**Scope:** `eve/Source/layout/component.md`, `eve/Source/EVEView.h`, `.cpp`
**Action:** Row 1 leaves the table. A listener on the `images` tree (`jam_TextEditor.cpp:93`) creates the `ImagePreviewComponent` with `registry->make` on the first box, calls `bindTo` and `bindToImages`, and destroys it when the tree has no child.
**Validation:** Ruling 18: *"only being created, layout on demand"*. The carried checks of PLAN-1 run again (tracker). `eve preview` and the third-party lanes write the same tree (`jam_TextEditor.cpp:1337-1362`).

### Step 8: Hand over, audit, doxygen
**Action:** As Part 1, Step 8.

## Acceptance (ARCHITECT builds and tests)

1. The PLAN-1 carried checks (tracker) pass with the renamed and ephemeral painter.
2. With no image on screen, no painter exists: a read of the component tree in `DebugModelMonitor` shows no child.
3. Part 8 exercises the host and the lookup.

## BLESSED Alignment

- **Bound:** the host owns its content. EVEView owns the host and the overlay.
- **Lean:** the image path leaves `FilePreviewComponent`.
- **Explicit:** each type name states its one job.
- **SSOT:** one lookup picks the content. One test finds an image.
- **Stateless:** the boxes are Model rows, as today.
- **Encapsulation:** the host knows no content type.
- **Deterministic:** the same file gives the same content.

## Risks / Open Questions

None open. Step 5a holds one read for COUNSELOR before delegation.
