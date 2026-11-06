# PLAN: Finder Part 6 — The Widget

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 30, 31, 33, 34, 40 to 44, 47

## Overview

`jam::FinderComponent` is a generic JAM widget, isomorphic to TextEditor: its own ValueTree, command ids and colour ids. EVEView creates it on demand at the caret cell. Its height is `AUTO` or a row count, with temporary rows when the prompt is at the bottom. This part ends with an empty two-column shell that opens, resizes and dismisses. Parts 7 and 8 fill the columns.

## Language / Framework Constraints

- One object, one translation unit.
- `jam_gui` may use `jam_terminal` (`jam_gui.h:17`), where `StyleTerminal` lives.
- A column is a `juce::TreeView` with `juce::TreeViewItem` rows for the tree and the list, or `PreviewComponent` for the preview. Each column scrolls (ruling 40).

## Dependency & API Inventory

**Isomorphic to TextEditor:**
- `Model::Component`, `ApplicationCommandTarget`, listener (`jam_TextEditor.h:107-112`). `bindToModel` takes its state (`jam_TextEditor.cpp:85-106`).
- Command table in CAST, default keys from tables (`terminal.md:1856-2090`, `jam_TextEditor.cpp:123-142`), `getAllCommands`, `getCommandInfo`, `perform` (`:663-674`, `:735`).
- Colour ids in an enum from `map::ColourId` (`jam_TextEditor.h:116-122`), rows in the sheet (`textEditor.css`).

**Style:** `StyleCustom` line calls take a `Document::Element`, with mono defaults (`jam_StyleCustom.h:84-107`). The TextEditor's `StyleTerminal` lives with the view (`component.md:8`, `jam_ViewEditor.h:160`), so `setLookAndFeel (&editor.getLookAndFeel())` holds. The shape is `EVEView.cpp:672`.

**Rows:** a TextEditor line is an `Id::line` element with an `AttributedString` (`jam_TextEditor.h:25`). `TextBlock::setBlock` builds runs for every line of a block (`jam_TextBlock.h:19`, `:27`), and each run paints with `run.paint` (`jam_TextEditor.cpp:1909-1912`).

**TreeView:** `setRootItem`, `setRootItemVisible`, `setOpenCloseButtonsVisible`, `setIndentSize`, `setDefaultOpenness`, `getViewport`, `scrollToKeepItemVisible` (`juce_TreeView.h:694`, `:714`, `:759`, `:825`, `:729`, `:813`, `:810`). `TreeViewItem`: `paintItem`, `getItemHeight`, `mightContainSubItems`, `itemOpennessChanged`, `itemSelectionChanged`, `itemClicked`, `itemDoubleClicked`, `paintOpenCloseButton` (`:356`, `:311`, `:254`, `:288`, `:416`, `:393`, `:409`, `:366`). Pattern: `DebugModelMonitor::…::Item` (`jam_DebugModelMonitor.h:139-178`, `.cpp:86-113`). Visible rows only build components (`juce_TreeView.cpp:366-395`, `:686-692`). `TreeView` wants focus and has its own keys (`:970`, `:1347-1363`).

**Cells:** live cursor row: `Id::cursor` (`parameters.md:39`, `jam_TextEditor.cpp:632`). Live origin: `getCellBounds (getNumHistoryLines(), 0)` plus `getRowOffset()` (`:1349-1350`). `getCellBounds` is public (`jam_TextEditor.h:487`). The view takes a zoom transform (`EVEView.cpp:39`), so coordinates go through `getLocalArea`.

**Temporary rows:** the content is history plus live (`jam_TextEditor.cpp:1217-1220`). `setViewToEnd` (`:987-990`). `previewBox` is the existing cell box row (`SPEC.md:161-162`, `jam_TextEditor.cpp:1339`). Winsize uses the terminal bounds, not the content (`:1180-1198`), so temporary rows do not reach the terminal.

**Config:** `## display` rows, read with `styleManager->getConfig` (`default-eve.md:12`, `EVEView.cpp:632-633`). Key rows with a mode (`default-eve.md:51`). Mode by registration (Part 3).

**Ownership:** `ephemerals` (Part 1).

**New names (ARCHITECT's):** `FinderComponent`, `finder`, `fuzzy`, `browse`. `FinderCommandId` follows `TextEditorCommandId`. `finderRows`, a `## finder` table row. `reservedRows`, a TextEditor state row; SPEC's word is *"reserve"* (`SPEC.md:442-443`).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: The tables
**Scope:** `jam/cast/` tables beside `TextEditorCommandId`, `jam/cast/spell.md`, `eve/Source/textEditor.css` or a new sheet row
**Action:** `FinderCommandId` with `dismiss`, `nextColumn`, `nextMode`, `open`. Default keys: `escape`, `tab`, `tab` in the `prefix` layer, `return`. Colour ids for the background and the selected row.
**Validation:** Rulings 37, 38, 47. CAST form as `terminal.md:1866-1897`. COUNSELOR pastes those rows.

### Step 2: The class
**Scope:** `jam/jam_gui/widgets/jam_FinderComponent.h`, `.cpp` (new), `jam/jam_gui/jam_gui.h`, `.cpp`
**Action:** `class FinderComponent : public juce::Component, public jam::Model::Component<FinderComponent>, public juce::ApplicationCommandTarget, private juce::ValueTree::Listener`. It holds two column slots, `std::unique_ptr<juce::Component>`, as `PreviewComponent` holds its content. `keyPressed` calls `commandManager->invokeKey (key, state)`, as `EVEView.cpp:86-91`. `resized` splits the bounds into the two slots by the `Id::proportion` row, and the optional filter slot of Part 8 takes one row at the top.
**Validation:** Ruling 47. No data member shadows a row.

### Step 3: Registered modes
**Scope:** `eve/Source/EVEView.cpp`
**Action:** `registerMode` for `fuzzy` and `browse`: key input forwards to Finder, caret is empty, as OPEN's (`jam_TextEditor.cpp:640-643`). The status bar shows the label.
**Validation:** Ruling 14, ruling 34.

### Step 4: Open on the command
**Scope:** `eve/Source/EVEView.h`, `.cpp`, `eve/Source/layout/default-eve.md`
**Action:** Command `finder`, key row `ctrl+F`, no mode (`jam_CommandManager.cpp:67`). It writes the `Id::value` row of the `finder` component state to the source name `file`. The registry callback reads the row: non-empty creates the component in `ephemerals` with the TextEditor's LookAndFeel, `addAndMakeVisible`, `toFront (true)`. Empty destroys it, writes `terminal` to the mode row and grabs the focus.
**Validation:** Rulings 18, 32. Part 1's shape. A row, not a boolean. The key always opens: *"ctrl+f always open Finder"*.

### Step 5: The anchor
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`, `eve/Source/EVEView.cpp`
**Action:** `TextEditor` gets a public `getCaretBounds()`, built from the existing cell map: the row of `Id::cursor`, column 0. EVEView sets Finder's bounds from it through `getLocalArea`, with the width of the terminal bounds, on the Finder callback and on `resized`.
**Validation:** Ruling 31: the first row is the prompt row. Ruling 49: the caret is the anchor at a prompt and in a program. `getCarets` holds the same read (`jam_TextEditor.cpp:632-636`), and they share one function.

### Step 6: Height
**Scope:** `eve/Source/layout/default-eve.md`, `jam/jam_gui/widgets/jam_FinderComponent.*`
**Action:** The `## finder` table row `rows`: `auto` or a number. `FinderComponent::getRows` returns the row count: a number as given, or for `auto` the larger item count of the two columns, capped by the rows from the caret to the viewport bottom, plus temporary rows. An item count row in the state drives it.
**Validation:** Rulings 41, 42, 44. Cell height comes from `getMonoMetrics` (`jam_StyleCustom.h:72`). A value in a table, not a constant.

### Step 7: Temporary rows
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:** State row `reservedRows`. `setContentSize` adds that row count in cell heights to the content (`jam_TextEditor.cpp:1217-1220`). The state listener calls `setContentSize` and `setViewToEnd`. Finder writes the row when the rows it needs exceed the rows below the caret, and writes 0 on dismiss.
**Validation:** Ruling 43. `setWinsize` does not read the content (`:1180-1198`), so no SIGWINCH. The view returns to the end on 0, so the prompt returns to the bottom.

### Step 8: Hand over, audit, doxygen
**Action:** As Part 1, Step 8.

## Acceptance (ARCHITECT builds and tests)

1. `ctrl+F` at a prompt mid-screen: an empty two-column widget starts on the prompt row, at the configured row count.
2. At the bottom of the viewport: rows appear, the view scrolls up, and `escape` restores the view with no SIGWINCH.
3. Blank terminal with `auto`: the widget uses the empty rows, and stops at the viewport bottom.
4. `ctrl+F` inside nvim: the widget opens at nvim's caret. `escape` returns the focus to nvim.
5. A theme hot reload reaches the widget through the shared style.
6. Zoom in, then `ctrl+F`: the widget sits on the prompt row.
7. Status bar shows the mode label.

## BLESSED Alignment

- **Bound:** EVEView owns the widget. The view owns the style it borrows and outlives it.
- **Lean:** a widget with slots. Content arrives in Parts 7 and 8.
- **Explicit:** rows name the height, the reservation and the source.
- **SSOT:** the cell map is `getCellBounds`. The caret is `Id::cursor`.
- **Stateless:** the widget derives its bounds from rows.
- **Encapsulation:** JAM holds the widget and EVE wires it.
- **Deterministic:** the same rows give the same box.

## Risks / Open Questions

- Rows printed by a program while Finder is open move the caret. The bounds listener follows `Id::cursor`. ARCHITECT's run shows if the widget must hold still.
- `~/.config/zsh/zshrc:107` binds `^F` to `fzf-file-widget`. EVE takes the key first, by intent. ARCHITECT: *"native finder with ctrl+f should override/replace fzf, that's the goal"*.
