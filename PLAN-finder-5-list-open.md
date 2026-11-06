# PLAN: Finder Part 5 — LIST Traverses Lists, OPEN Jumps on the Active List

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 4 to 6, 9, 10, 13

## Overview

LIST mode moves the active column list with `k` and `j`, and the view scrolls to it. Leaving LIST keeps the selection. OPEN labels the links on screen, and links now exist on the active list. Both modes start from TERMINAL.

## Language / Framework Constraints

Part 3 made both registered modes in EVE, with bodies moved unchanged. Part 4 made the state. This part changes behaviour only.

## Dependency & API Inventory

- Registered slots (Part 3). `setMode` (`jam_TextEditor.cpp:108-111`).
- `columnLists`, `activeList` (Part 4). A change of `activeList` rebuilds links (Part 4, Step 4).
- Cursor keeps the view: a change of the cursor keeps the cell in view (`jam_TextEditor.h:57`, `jam_TextEditor.cpp:803-811`). `setCursor` (`jam_TextEditor.h:931`).
- Residency: `addBlocks` builds from the first visible block minus one (`jam_TextEditor.cpp:1011-1016`), and `addBlock` keeps the view when a block joins above (`:884-892`). A target line must be resident before `getCellBounds` (`:1371`).
- Keys: `listDown` `j`, `listUp` `k` in mode `list`; `open` `o` and `list` `l` in mode `terminal`, layer `prefix` (`jam/cast/terminal.md:1961-1967`, `:2067-2073`, `:2112-2113`, moved by Part 3). END's prefix takes `f` for LIST (`SPEC.md:727-728`).
- Labels: `applyLinkLabel`, `paintLinkLabels` walk visible links (`jam_TextEditor.cpp:1574-1634`).

**New names:** none. `listDown` and `listUp` keep their names.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Delete the entry highlight
**Scope:** EVE's registered LIST bodies (Part 3, Step 5), `eve/Source/layout/default-eve.md`
**Action:** Delete `getListEntry`, `moveListEntry`, `paintListEntry`, the LIST entry reaction `setCursorToTerminal` and `moveListEntry` (`jam_TextEditor.cpp:820-824`), and the `open file` key row in mode `list` (`default-eve.md:57`).
**Validation:** Ruling 4: *"the indicator is the hyperlinks (underline)"*. `paintColumns` stays, because `paintSelection` calls it (`jam_TextEditor.cpp:1889`). `setCursorToTerminal` stays for NORMAL (`:431`).

### Step 2: `k` and `j` move the active list
**Scope:** EVE's `listDown`, `listUp` callbacks, `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:** A callback writes `activeList` to the previous or the next `columnList` child, by child order. At an end, the row does not change.
**Validation:** Ruling 5. An unchanged value sends no event. The Model is the one selection.

### Step 3: The view follows the row
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:** The state listener handles `Id::activeList`: it builds the blocks from the list's `Id::line` block to the first resident block through `addBlock`, finds the first line of the list among them, and calls `setCursor`.
**Validation:** Ruling 6. `setCursor` scrolls through the existing path (`:803-811`). `addBlock` keeps the view correct (`:884-892`).

### Step 4: Both start from TERMINAL
**Scope:** `eve/Source/layout/default-eve.md`
**Action:** The key rows `open` and `list` keep mode `terminal` only. `escape` in each returns to TERMINAL.
**Validation:** Ruling 9.

### Step 5: Leaving LIST keeps the selection
**Scope:** EVE's LIST entry reaction
**Action:** No code resets `activeList` on leaving. On return to TERMINAL, the mode handler calls `setViewToEnd` as today (`jam_TextEditor.cpp:828-829`).
**Validation:** Ruling 10. Part 4, Step 3 moves `activeList` when a new list appears.

### Step 6: Hand over, audit, doxygen
**Action:** As Part 1, Step 8.

## Acceptance (ARCHITECT builds and tests)

1. `cd end`, `ls`, `cd eve`, `ls`. `` ` `` `l`, then `k`: the `end` list shows the underlines and the view scrolls to it. The `eve` list loses them. `j` moves back.
2. `escape`: TERMINAL, view at the end. A scroll up shows the `end` list still underlined.
3. `` ` `` `o`: labels on the active list if on screen. A label opens through `openFile`.
4. A click on a link of the active list opens it.
5. A new `ls`: that list becomes active.
6. Under END: `` ` `` `f`, `` ` `` `o` behave the same.

## BLESSED Alignment

- **Bound:** the Model owns the selection.
- **Lean:** the highlight path goes.
- **Explicit:** the row states the selection.
- **SSOT:** `activeList` drives links and scroll.
- **Stateless:** callbacks write one row and hold nothing.
- **Encapsulation:** EVE tells. TextEditor reacts.
- **Deterministic:** the same keys give the same list.

## Risks / Open Questions

- OSC 8 links in other blocks stay links (`SPEC.md:372`), so OPEN labels them too when they are on screen (`jam_TextEditor.h:632-644`). The rulings name file links only. ARCHITECT's run shows if this matters.
