# PLAN: Finder Part 9 — The Action List

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 32, 48

## Overview

The same `FinderComponent` lists the registered commands, filters them, and runs the selected one. A different key opens it.

## Language / Framework Constraints

Part 8 supplies the filter and the list. This part adds one source and one reaction.

## Dependency & API Inventory

- `getNumCommands`, `getCommandForIndex`, `getNameOfCommand`, `getDescriptionOfCommand`, `invokeDirectly` (`juce_ApplicationCommandManager.h:157`, `:162`, `:177`, `:186`, `:219`).
- `TextEditor::getCommandInfo` uses the name as the description (`jam_TextEditor.cpp:672-673`). `CommandManager::setInfo` takes a name (`jam_CommandManager.h:84`). Readable text is in the `description` column of the key rows (`default-eve.md:51`).
- `EVEView::invokeCommand` runs a name through the TextEditor's chain (`EVEView.cpp:729-738`).
- Source row: `finder` state `Id::value` (Part 6, Step 4). Reference only: endless's list (`endless/Source/terminal/action/ActionList.h`).

**New names:** the source name `action`. Command `finderActions`, sibling of `finder`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Sources are a lookup
**Scope:** `eve/Source/EVEView.h`, `.cpp`
**Action:** A `jam::Function::Map` keyed by source name fills the `finder` state and gives the selection reaction. Move `file` (Parts 7 and 8) into it, and add `action`.
**Validation:** MANIFESTO **L**: a table, not a branch. Rule 8: a noun for what it holds.

### Step 2: The action source
**Scope:** `eve/Source/EVEView.cpp`
**Action:** One child per registered command: `Id::name`, and `Id::description` from the key row of that name. A selection calls `invokeCommand`.
**Validation:** Ruling 48. `getCommandForIndex` is the source, so a new command needs no table here.

### Step 3: The key
**Scope:** `eve/Source/layout/default-eve.md`, `eve/cast/`
**Action:** Command `finderActions` writes `action` to the source row. Its key row: `?`, layer `prefix`, no mode.
**Validation:** Ruling 32: *"for action list it could be triggered by different shortcut"*. ARCHITECT: *"prefix + `?`"*. Under END, this row goes to END with the other prefix rows (Part 3, Step 6).

### Step 4: Layout
**Scope:** `jam/jam_gui/widgets/jam_FinderItem.*`, `eve/Source/EVEView.cpp`
**Action:** One list with the filter on top. Each row has two labels: the keyboard shortcut and the description. Table lines give the two cells: a line can hold child lines as columns (`jam_TextBlock.h:80-90`, `jam_TextEditor.h:82-84`).
**Validation:** ARCHITECT: *"2 column label: Keyboard shortcut | description"*; *"this is endless parity"*. Endless's row has a name label and a shortcut label (`endless/Source/terminal/action/ActionRow.cpp:27-39`). Order: ARCHITECT selected *"Shortcut first"*: the keyboard shortcut at the left, the description at the right.

### Step 5: Hand over, audit, doxygen
**Action:** As Part 1, Step 8.

## Acceptance (ARCHITECT builds and tests)

1. The key opens the list at the caret. Typing filters. `Enter` runs the command.
2. A command from `eve/cast` appears with no change here.
3. `prefix + tab` does nothing in this source.

## BLESSED Alignment

- **Bound:** the source fills Model rows.
- **Lean:** no new widget.
- **Explicit:** source names are rows.
- **SSOT:** the command manager is the one list.
- **Stateless:** the list holds no copy beyond its rows.
- **Encapsulation:** Finder knows no command.
- **Deterministic:** the same registry gives the same rows.

## Risks / Open Questions

None open.
