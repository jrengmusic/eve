# PLAN: Finder Part 8 — FUZZY

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 26, 33 to 35, 39, 45

## Overview

FUZZY has a filter input on top, the file list on the left, and the Preview on the right, with fzf's matching. `jam_fuzzy` enters EVE's build, and its gaps close. This is no port: *"we never translate byte-to-byte"*.

## Language / Framework Constraints

- *"Message thread free to spawn parallel job"* (`SPEC.md:399-401`).
- Measurement uses `scout` (CAROL.md Profiling). COUNSELOR writes the manifest.

## Dependency & API Inventory

**Engine, `jam_fuzzy` (2975 lines):**
- `Fuzzy::matchV1`, `matchV2`, `exactMatch`, `exactBoundaryMatch`, `prefixMatch`, `suffixMatch`, `equalMatch` (`jam_BonusMatrix.h:257-352`). `Pattern::parse` with AND and OR groups (`:225-248`). `Scheme::path` (`:87-92`).
- `Fuzzy::Finder`: thread pool, chunks of 1024, `jam::Mailbox` (`jam_BonusMatrix.h:379-418`, `.cpp:28`, `:58-148`). Its module is in no EVE build: `eve/project-info.md:270-295` has no row, and `SPRINT-LOG.md:181` says so.

**Gaps, each read:**
| Gap | Evidence |
|---|---|
| Items replace as a whole | `setItems` (`jam_BonusMatrix.cpp:162-173`) |
| No match positions | `Result` holds start, end, score (`jam_BonusMatrix.h:129-136`). `Pattern::match` returns start 0 and the text length (`jam_FuzzyPatternMatch.cpp:128-133`). `dispatchMatch` passes no `positions` (`:30-31`). `matchV2` can give them (`jam_BonusMatrix.h:269`) |
| Score-only sort | `jam_BonusMatrix.cpp:105-107` |
| Rescan on each key | `setPattern` (`:175-181`, `:114-148`) |
| Raw `new` and `delete` | `jam_BonusMatrix.cpp:109-110`, `:145-146`. `Mailbox` takes a raw pointer and the caller deletes (`jam_Mailbox.h:16-18`, `:41`) |

**fzf parity, by the manual:** walker default `file,follow,hidden` (`fzf.1:1347`), skip `.git,node_modules` (`:1366`), root is the working directory (`:1361`). Adaptive height follows the input size (`:396-397`). Query syntax comes from `Pattern::parse`. `--layout` default is `default` (`:465-466`); ruling 35 puts the input on top.

**Filter input:** `juce::TextEditor` (ruling: *"editable juce::TextEditor/juce::Label"*), with the terminal mono font from `getMonoFont` (`jam_StyleCustom.h:60`).

**Rows and highlight:** `FinderItem` (Part 7). `AttributedString` styles per cell (`jam_MarkdownDocument.cpp:1428-1438` shows styled appends).

**Preview:** the host and the lookup (Part 2). Image scales to fit (ruling 26). Markdown uses its own style (rulings 27, 28).

**New names:** `getResultPositions` or the `positions` member follows `matchV2`'s parameter name.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: The module joins the build
**Scope:** `eve/project-info.md`, `jam/jam_fuzzy/`
**Action:** Add the `@user-module` row `jam_fuzzy`, in the form of `eve/project-info.md:271`. Fix the compile errors that the first build shows; `SPRINT-LOG.md:381` names `jam_BonusMatrix.*` as unbuilt after the array sweep.
**Validation:** ARCHITECT builds. The compiler is the truth.

### Step 2: Close the raw ownership
**Scope:** `jam/jam_fuzzy/finder/jam_BonusMatrix.cpp`
**Action:** `publishResults` and the empty path hold each `ResultSet` in a `std::unique_ptr`, `release` into `Mailbox::write`, and take the returned pointer into a `std::unique_ptr`.
**Validation:** CODING, *"NEVER use raw `new` or `delete`"*. `Mailbox`'s contract stays (`jam_Mailbox.h:37-41`).

### Step 3: Match positions
**Scope:** `jam/jam_fuzzy/finder/jam_BonusMatrix.h`, `.cpp`, `jam_FuzzyPatternMatch.cpp`
**Action:** A `Finder::getPositions (itemIndex)` runs `matchV2` with `positions` for one visible row, on the message thread, with its own `Slab`.
**Validation:** Positions are for visible rows only, so the list scan stays free of them. `matchV2` gives them as is (`jam_BonusMatrix.h:269`).

### Step 4: Order
**Scope:** `jam/jam_fuzzy/finder/jam_BonusMatrix.cpp`
**Action:** `std::sort` becomes `std::stable_sort` by score, so items keep their source order on a tie (`:105-107`).
**Validation:** MANIFESTO **D**: same input, same order.

### Step 5: The walker
**Scope:** `eve/Source/EVEView.*`, `eve/Source/layout/default-eve.md`, `jam/jam_gui/menu/jam_Menu.*`
**Action:** A `juce::ThreadPoolJob` on the existing pool shape walks the cwd with `juce::RangedDirectoryIterator`, with rows `walker skip` (`.git node_modules`) in `## finder`. It posts each chunk of paths to the message thread, which adds to the state and calls `setItems`.
**Validation:** Parity by the manual. `juce::RangedDirectoryIterator` takes the follow-symlink policy (`juce_RangedDirectoryIterator.h:134-138`), which gives fzf's `follow`. The pool pattern: `jam::SharedInstance<juce::ThreadPool>` (`jam_ImagePreviewComponent.h:518`). Chunk replacement is a state update. A deadline thread is not involved.

### Step 6: The columns
**Scope:** `jam/jam_gui/widgets/jam_FinderComponent.*`
**Action:** Mode `fuzzy`: the filter row on top. The left slot is the flat `FinderItem` list of the ranked results from `readResults`. The right slot is a `PreviewComponent`.
**Validation:** Rulings 34, 35. `readResults` is lock-free (`jam_BonusMatrix.cpp:183-186`). A vblank or an async update pulls it, the shape of `jam_TextEditor.h:36`.

### Step 7: Selection drives Preview
**Scope:** `eve/Source/EVEView.cpp`, `jam/jam_gui/widgets/jam_FinderComponent.*`
**Action:** `Id::selected` change: the Part 2 lookup builds the content and calls `setContent`. `Enter` opens through `openFile`.
**Validation:** Ruling 39. Event payload: the row delivers the path.

### Step 8: Height follows matches
**Scope:** `jam/jam_gui/widgets/jam_FinderComponent.*`
**Action:** The item count row takes the match count in FUZZY. `getRows` (Part 6) reads it, and `reservedRows` follows live.
**Validation:** Ruling 45.

### Step 9: Measure
**Scope:** a `scout` manifest in the scratchpad
**Action:** COUNSELOR writes a manifest: a tree of 100000 files, FUZZY open, one keystroke, then a scroll. Epochs compare one `FinderItem` per row with one `TextBlock` per row and with a block per column.
**Validation:** `jam_TextBlock.h:19`, `juce_TreeView.cpp:674`. The report's numbers go to ARCHITECT, who rules any change.

### Step 10: Hand over, audit, doxygen
**Action:** As Part 1, Step 8.

## Acceptance (ARCHITECT builds and tests)

1. `ctrl+F`: typing filters with fzf's query syntax: `foo 'exact ^start end$ !no`.
2. The list highlights the matched cells.
3. Moving the selection updates Preview: text scrolls, markdown renders with its style and scrolls, an image scales to fit with metadata under it, a GIF plays.
4. `AUTO`: the height follows the match count.
5. A large tree does not stall the terminal.

## BLESSED Alignment

- **Bound:** the job, the matcher and the widget each have one owner.
- **Lean:** the engine exists.
- **Explicit:** skip dirs are table rows.
- **SSOT:** ranked results come from one mailbox.
- **Stateless:** rows read results and hold no copy.
- **Encapsulation:** the matcher knows no UI.
- **Deterministic:** the same query and tree give the same order.

## Risks / Open Questions

- Step 9 can change the row shape. That is ARCHITECT's call on the numbers.
