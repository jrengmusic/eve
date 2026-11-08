# PLAN: Finder Part 7 — BROWSE

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 33, 34, 36 to 39, 46

## Overview

BROWSE shows the tree on the left and the cwd's file list on the right. `Tab` moves between columns. `Enter` on a folder changes Finder's cwd, and both columns update. `Enter` on a file uses Open With. `Esc` closes Finder and sends the terminal to Finder's cwd.

## Language / Framework Constraints

- A directory read off the deadline thread is permitted. The existing thread ruling: *"Message thread free to spawn parallel job"* (`SPEC.md:399-401`).
- The Model holds the rows as value data. A `TreeViewItem` is a materialisation of one node.

## Dependency & API Inventory

**Scan:** `menu::addFiles` calls `findChildFiles` and `sort`, and writes a `PopupMenu` during the scan (`jam_Menu.cpp:39-96`). `addDirectories` does the same for folders (`:98-163`). Callers: `jam_Menu.cpp:70`, `:127`, `:171`, `:174`, `jam_ComboBox.h:27`, `:50`, `:118` (LSP). Both skip hidden files (`:52-53`, `:110`).

**Selector contract:** an item list, a `juce::Value` bound to the Model, a reaction (`jam_Selector.h:360-362`, `:373`, `jam_PresetSelector.h:95`).

**Tree and list rows:** Part 6 inventory. `DebugModelMonitor::…::Item` mirrors one ValueTree node and builds children on open (`jam_DebugModelMonitor.cpp:86-113`). Its `getUniqueName` depends on child order (`:244-250`). `paintOpenCloseButton` (`juce_TreeView.h:366`) and `StyleDebug` show the style override (`jam_StyleDebug.h:53`).

**Glyphs:** `TextBlock::setBlock` (`jam_TextBlock.h:27`). Faces: `mono`, `symbols`, `--notoSymbols`, `--notoEmoji` (`style.css:23-60`).

**Tree root:** `cwd` up by `tree depth` (`SPEC.md:398`, the code removed in Part 1 is `EVEView.cpp:630-640` in today's tree, pasted here):
```cpp
const auto depth { static_cast<int> (display.getChildWithName (jam::Format::toType (Id::treeDepth)).getProperty (Id::value)) };
juce::File root { getTerminalState().getProperty (Id::cwd).toString() };

for (int level { 0 }; level < depth; ++level)
    root = root.getParentDirectory();
```

**Open With:** `openFile (path, terminal)` (`EVEView.cpp:567-588`). `isBusy` reads `isRecording` and `alternateScreen` (`:572`). The `directory` row gives `cd` (`:544-545`).

**cd:** `applyOpenCommand` writes the command, the quoted path and a carriage return to the input (`EVETerminalProcessor.cpp:204-212`). `ShellIntegration::appendPath` takes the shell (`:209`). `getShell` gives the shell.

**New names:** the `@id` row `treeDepth` in `eve/cast/identifiers.md` returns with the `tree depth` row (Part 1 deleted both). `getFiles`, returning the sorted list with no menu; `addFiles` calls it, the verb contract (`NAMES.md` Rule 1). `FinderItem`, one `TreeViewItem` over one Model row. `Id::selected`, the selected path row. `syncDirectory`, a command in `map::CommandId`. `clearLine`, a `@string` in `eve/cast/`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Split the scan
**Scope:** `jam/jam_gui/menu/jam_Menu.h`, `.cpp`
**Action:** `juce::Array<juce::File> getFiles (const juce::File& directory, int findType, const juce::String& pattern)` runs `findChildFiles` with a sort. `addFiles` and `addDirectories` call it. No behaviour changes for the seven callers.
**Validation:** NAMES Rule 1: *"`get` returns, the caller `add`s"*. MANIFESTO **S**: one scan. `findType` passes `findFiles`, `findDirectories`, or both, with `ignoreHiddenFiles`.

### Step 2: The row
**Scope:** `jam/jam_gui/widgets/jam_FinderItem.h`, `.cpp` (new)
**Action:** `FinderItem : juce::TreeViewItem, juce::ValueTree::Listener`, holding one Model node, verbatim shape of `Item` (`jam_DebugModelMonitor.h:140-178`). `paintItem` paints the runs of one `TextBlock` line, with the terminal `StyleCustom`. `getItemHeight` is `getMonoMetrics().cellHeight`. `mightContainSubItems` reads `Id::isDirectory`. `itemSelectionChanged` writes `Id::selected`.
**Validation:** Ruling 33. Rows by the `StyleCustom` line calls (`jam_StyleCustom.h:84-107`). `getUniqueName` uses the path, not the child index (`jam_DebugModelMonitor.cpp:249`), because a directory change reorders rows.

### Step 3: The Model rows
**Scope:** `eve/Source/EVEView.cpp` or the `finder` source table, `jam/jam_gui/widgets/jam_FinderComponent.*`
**Action:** A `file` source fills the `finder` state: `Id::cwd`, `Id::selected`, and one child per item with `Id::path`, `Id::isDirectory`. A change of `Id::cwd` replaces the children with `getFiles`. The first open writes the terminal's `cwd`.
**Validation:** Ruling 32: *"at cwd"*. A complete state replaces a complete state (MANIFESTO **E**). Event payload: the listener reacts to the changed child (CODING, consume the event payload).

### Step 4: Two columns
**Scope:** `jam/jam_gui/widgets/jam_FinderComponent.*`, `eve/Source/layout/default-eve.md`
**Action:** The left slot is a `juce::TreeView` of `FinderItem`s from the tree root: `cwd` up by the `tree depth` row (pasted above; the row returns to `## finder`). The right slot is a `juce::TreeView` with a hidden root and flat `FinderItem`s of the cwd. Both have `setWantsKeyboardFocus (false)`, so Finder holds the focus and its command table owns the keys (`juce_ComponentPeer.cpp:207-228`).
**Validation:** Rulings 36, 40. Keys: the arrow and `j`/`k` rows of `FinderCommandId` call `TreeView::getItemOnRow`, and `setSelected`, and `scrollToKeepItemVisible`.

### Step 5: Keys
**Scope:** `FinderComponent::perform`
**Action:** `nextColumn` swaps the active column. `open`: on a folder, write `Id::cwd`; on a file, write `Id::url` and invoke `openFile`, the path at `jam_TextEditor.cpp:1536-1539`. `nextMode` writes the mode row. `dismiss` is Step 6.
**Validation:** Rulings 37, 38, 39. Open With is the same path as a click.

### Step 6: Esc syncs the terminal
**Scope:** `eve/Source/EVEView.cpp`, `eve/cast/`, `eve/Source/shell/zsh_integration.zsh`, `bash_integration.bash`, `powershell_integration.ps1`
**Action:**
1. The `dismiss` command of `FinderCommandId` writes the empty source row of the `finder` state, and nothing else (Part 6, Step 4).
2. The registered callback of that row reacts: when Finder's `Id::cwd` row differs from the terminal's `cwd` row, and `isRecording` and `alternateScreen` are off (`EVEView.cpp:572`), it invokes the command `syncDirectory`.
3. `syncDirectory`, a registered command in `map::CommandId`, writes the `clear line` sequence, then calls `applyOpenCommand` with the `directory` row value.
4. Each integration script binds that one private sequence to its whole-line kill:
   - zsh: `kill-whole-line`, with `bindkey -M` in `emacs`, `viins` and `vicmd`. The binding reaches a keymap only through `bindkey` in that keymap (`man zshzle`).
   - bash: `kill-whole-line`, with `bind -m` in `emacs`, `vi-insert` and `vi-command`. It is unbound by default (GNU Bash manual, *Commands For Killing*).
   - PowerShell: `Set-PSReadLineKeyHandler -Chord … -ScriptBlock` calling `RevertLine`, beside the existing `Enter` handler (`powershell_integration.ps1:27`). `RevertLine`: *"Reverts all input to the current input."* A `Set-PSReadLineOption -EditMode` resets handlers (Microsoft docs), so the script sets the handler after the shell's own profile.

**Validation:** ARCHITECT: *"use Model as parameter, event driven registrable callback, with command manager, no handrolling anything"*, and *"Script binds one sequence"*. Rulings 46, 38. One sequence means one source: the `clearLine` `@string` in `eve/cast/`. `addShellScripts` writes the scripts as raw binary data (`ShellIntegration.cpp:22-37`), so they cannot hold a CAST value. `getShellEnvironment` gives each shell its variables (`ShellIntegration.h:39-49`, `.cpp:63`), so it sets `EVE_CLEAR_LINE`, and each script binds that variable. The literal exists once. With shell integration off (`ShellIntegration.h:19`), the shell has no binding, and the command sends only `cd`. COUNSELOR tests each binding in a pty after the lock; the research ran no live zsh widget (Librarian report, Needs 4).

### Step 7: Hand over, audit, doxygen
**Action:** As Part 1, Step 8.

## Acceptance (ARCHITECT builds and tests)

1. `ctrl+F`, then `prefix + tab`: BROWSE. Left is the tree, right is the cwd's list.
2. `Tab`, `j`, `k`, `Enter` on a folder: both columns update. The terminal does not change yet.
3. `Esc`: the terminal runs `cd` and the prompt shows the new directory.
4. Type `abc` at the prompt, then the same: `abc` is cleared and `cd` runs.
5. `Enter` on `.cpp`: `nvim`. On `.md`: the markdown panel. On a file with no row: the system default app.
6. Inside nvim: `Esc` closes Finder and types nothing.
7. Icons from the Nerd Font render in the terminal font and cell.

## BLESSED Alignment

- **Bound:** Finder owns its rows. The tree view owns its items.
- **Lean:** one row type serves the tree and the list.
- **Explicit:** the selected path and the cwd are rows.
- **SSOT:** one scan. One Open With path.
- **Stateless:** an item reads its node and holds no copy.
- **Encapsulation:** EVE supplies the source.
- **Deterministic:** the same directory gives the same rows.

## Risks / Open Questions

- Measure: one `FinderItem` per file costs memory, and each `TreeView` update collects every item (`juce_TreeView.cpp:674`). Part 8 measures this with `scout`.
- The research cites bash 5.x docs and probed bash 3.2 only. PowerShell defaults come from the 7.6 docs, with no `pwsh` run.
