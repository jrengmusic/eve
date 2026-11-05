# PLAN: LIST Mode

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-05
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)

**Contract for each step (binds this plan):** ARCHITECT builds and tests: *"you dont build. architect build and test runtime"*. *"no new semantics, no magic strings allowed anywhere, no handroll state management, no manual arithmetic, no new pattern, use framework API to its fullest extent, follow established pattern verbatim"*. *"framework API(s) are rich jam first then juce. you probably only need to use it OOTB. if there's genuienely no abstract as SSOT you can use, extend the existing."* Data in transit has *"0 allocation, 0 copy, 0 mutation, 0 temporary container, 0 new struct, and 0 shadow state"* (`MANIFESTO.md:267`). *"we only use juce::String as Input or final Output materialisation to communicate with juce's API. otherwise we always use std::string/string_view everywhere."*

## Overview

LIST mode shows a file tree at the left and a Preview at the right, with the terminal between them. The
highlighted entry drives both. `Enter` opens it through `openFile`. Exit hides the panels, and the
text unwraps.

ARCHITECT: *"LIST mode UI/UX is identical with using Finder/Explorer"*; *"Both LEFT and RIGHT side panel
is EVE native features, juce::Component pure gui"*; *"although we are not touching any terminal even not
TextEditor at all"*; *"only show when user enter LIST mode, hence TextEditor content automatically
reflow"*; *"EVEView could hold Owner<Component>, 3 members. 2 SidePanel"*; *"Side panel preview
immediately dismissed when user exit modal mode"*; *"Preview is only the content"*.

## Language / Framework Constraints

- Winsize: panels do not touch the terminal. EVEView holds `Id::isResizing` true while LIST runs, so
  the TextEditor reflows and writes no winsize (`jam_TextEditor.cpp:1048`, ruling above).
- Thread: `DirectoryContentsList` needs a started `juce::TimeSliceThread`
  (`juce_DirectoryContentsList.h:68-73`). ARCHITECT: *"Message thread free to spawn parallel job"*.
  SPEC: *"EVE starts no thread"* binds the chain. Step 0 records the difference.
- `jam_gui/filebrowser/` holds path pickers only (`jam_PathBrowser.h:15`, `jam_SelectorBrowser.h:15`).
  The tree is JUCE's.

## Dependency & API Inventory

**JUCE:** `juce::SidePanel` (`juce_SidePanel.h:69`, `:133`, `:203`), `juce::FileTreeComponent`
(`setSelectedFile`, `refresh`), `juce::DirectoryContentsList::setDirectory`,
`juce::FileBrowserListener::selectionChanged`, `juce::TimeSliceThread` (`:104`, `:120`).
`juce::File::getSize`, `getLastModificationTime`, `getCreationTime`, `descriptionOfSizeInBytes`.
`juce::Time::formatted`.

**JAM:** `MarkdownDocument::addProperties` (`jam_MarkdownDocument.cpp:1349-1357`), `MarkdownComponent`,
`Preview`. View trim, verbatim: `326387d:EVEView.cpp:87-104`. `EVEView::setResizing`
(`EVEView.cpp:44-51`).

**New rows:** `EditorMode` `list`, `TextEditorCommandId` `list`, `listDown`, `listUp`, display rows
`tree depth`, `tree width`. **New names:** `moveListEntry`, `setPanelsVisible`, `addPanels`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor
runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 0: Record
**Scope:** `SPEC.md:348-360`, `PLAN-Grand-Scheme.md`
**Action:** COUNSELOR replaces the dock text with the ruled panels.

### Step 1: The mode
**Scope:** `jam/cast/gui.md`, `terminal.md`, `spell.md`, `eve/Source/layout/parameters.md`, END as in
PLAN-5 Step 5
**Action:** Add `list` with key 4, the commands `list` (key `l`, mode `terminal`, layer `prefix`),
`listDown` (`j`, mode `list`), `listUp` (`k`, mode `list`), and `openFile` on `return` in mode `list`
through the EVE keys table. The `terminal` row adds the mode `list`. Sizes become 28.
**Validation:** As PLAN-5 Step 1.

### Step 2: The highlighted entry
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:** `getVisibleLinks` (PLAN-5) serves here. Action `listDown` and `listUp` call
`moveListEntry (direction)`. It moves the cursor to the next link cell, and writes the link `uri` to
the `url` row with no `openFile`. `paintSelection` also paints the run under the cursor in `list` mode.
**Validation:** The cursor is the highlight, so no second state exists.

### Step 3: Preview content
**Scope:** `jam/jam_gui/widgets/jam_Preview.h`, `.cpp`
**Action:** Add `void setFile (const juce::File& file);`. An image file goes to `setImageData`. Preview
gains a child `MarkdownComponent` under the image: ARCHITECT: *"we could just use MarkdownComponent with
frontmatter metadata"*. Metadata uses `addProperties` with a `juce::NamedValueSet` of size, type,
created and modified, the JUCE input of that API. Text content is a fenced block parsed from a
`std::string_view` over a `juce::MemoryMappedFile`, cut at the SPEC bound of 1 MiB. Binary content
gives metadata only.
**Validation:** `jam_gui` needs `jam_markdown_graphics` here. If the module order forbids it, the
content moves to a `jam_markdown_graphics` class `jam::FilePreview` that holds a `jam::Preview`. COUNSELOR
reads the module order before delegating. `paint` calls only materialised objects.

### Step 4: The panels
**Scope:** `eve/Source/EVEView.h`, `.cpp`, `eve/Source/layout/default-eve.md`
**Action:**
1. Members: `jam::Owner<juce::Component> panels;`, per ARCHITECT, and `juce::TimeSliceThread directoryThread`,
   `juce::DirectoryContentsList directoryList`, in `EVEView`. `addPanels` runs at first use.
2. Left `juce::SidePanel` holds `juce::FileTreeComponent`. Right holds the Preview.
3. `EVEView::valueTreePropertyChanged`: a change of `Id::mode` on the TextEditor node calls
   `setPanelsVisible (mode == map::EditorMode::list)`, which calls `showOrHide`, `setResizing`.
4. `onPanelShowHide` calls `resized()`.
5. `resized` trims the view by the shown panels, verbatim from `326387d:EVEView.cpp:102`, left too.
6. Root: `cwd` up by the `tree depth` row, through `getParentDirectory`. `setSelectedFile` follows `url`.

**Validation:** `SidePanel` follows parent resizes (`juce_SidePanel.cpp:272-275`). The view bounds narrow,
so the history reflows and `isResizing` holds the winsize.

### Step 5: One selection
**Scope:** `eve/Source/EVEView.cpp`
**Action:** `terminalEvents.add (Id::url, ...)` tells the Preview `setFile`, and tells the tree
`setSelectedFile`. `FileBrowserListener::selectionChanged` writes the `url` row with `juce::URL { file }`.
**Validation:** An unchanged value sends no event, so the two directions end. Consume the event payload.

### Step 6: Hand over, Step 7: Audit, Step 8: Doxygen pass
As PLAN-1 Steps 10 to 12.

## Acceptance (ARCHITECT builds and tests)

1. `ls`, then `` ` `` `l`: panels open, the text reflows, the prompt is unchanged, no SIGWINCH.
2. `j`, `k` move. The image, the GIF and the text content follow. Metadata shows under the image.
3. `return` opens through `openFile`.
4. `escape` hides the panels, and the text unwraps.
5. A click in the tree moves the highlight.

## BLESSED Alignment

- **Bound:** `EVEView` owns the panels.
- **SSOT:** the `url` row is the one selection.
- **Encapsulation:** the panels are siblings, and the tick does not change.

## Risks / Open Questions

- `juce::SidePanel` draws a title bar and an animated slide (`juce_SidePanel.cpp:115-116`). Its look
  follows the LookAndFeel methods (`juce_SidePanel.h:172-179`).
