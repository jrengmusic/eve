# PLAN: history-batch-blocks

**RFC:** none. The objective comes from ARCHITECT's prompts and rulings this session.
**Date:** 2026-11-16
**Language Constraints:** C++17 / JUCE 8.0.17 / JAM (CODING.md, MANIFESTO.md, NAMES.md).

## Overview

ARCHITECT's objective, verbatim:
- *"seq raw byte test must be able to use FULL THROTTLE cpu just like mac ~100% CPU, without lock at audio thread, and never block nor choke message thread"*
- *"message thread could lock, async, even spawn parallel job. but MUST NEVER BLOCKED, CHOKED, NEVER"*
- *"use CriticalSection or MessageManagerLock NEVER HANDROLL"*
- *"reminder, low cpu usage and slowness only happen on windows. on mac it's the fastest among other terminal"*

ARCHITECT's rulings this session:

| # | Ruling (verbatim answer) | Decides |
|---|---|---|
| H1 | Fix design: *"Both (Recommended)"*: build each history block once, complete, on a parallel job; the message thread only swaps the finished block in | Steps 2-4 |
| H2 | *"Measure first"*: apply the Windows-native map-once fix, rerun scout, then decide the tail model | done (Step 0) |
| H3 | Tail model: *"Batch blocks"*: each frame one new block from only the lines that arrived since the last block; complete at creation, never rebuilt; history keyed by first line | Steps 2-3 |

## Evidence (scout, `benchmark/seq-100k-windows.md`, Release, 3 epochs, 7.86 s epoch)

| Zone (s per epoch) | Before map-once | After map-once |
|---|---|---|
| history index mapping | 0.68 | 0.0008 |
| history segment mapping | 0.73 | 0.012 |
| history lines (`TerminalHistory::addLines`) | 2.41 | 0.86 |
| editor block layout (`TextEditor::setBlock`) | 1.22 | 1.37 |
| editor history blocks | 4.15 | 2.78 |
| editor blocks (message thread total) | 4.64 | 3.34 |

- `addLines` runs about twice per frame: 2,512 calls for 1,370 frames. The tail block is rebuilt because it grows (`isLengthChanged`), and the block before it because its open end moved (`isOpenChanged`) (`jam_TextEditor.cpp:1713-1725`).
- Each rebuild lays out up to `TerminalSwap::linesPerBlock` = 256 lines again (`jam_TerminalSwap.h:28`).

## Facts read this session

- **Layout thread state.**
  - `Typeface::getOrCreateLigature` writes `ligatures` with no lock and asserts the message thread (`jam_Typeface.h:315-326`, `:340`). `GlyphArrangement::getDrawRuns` reaches it (`jam_GlyphArrangement.cpp:476-503`).
  - `Typeface::getOrCreateFallback` is guarded by `fallbacksLock` (`jam_Typeface.cpp:253-286`, `jam_Typeface.h:333`).
  - `TextEditor::setBlock` reads `juce::Desktop` and the LookAndFeel (`jam_TextEditor.cpp:1767-1778`).
- **Line read thread state.**
  - `TerminalHistory::addLine` writes no singleton. It writes the `TerminalHistory` member `links` and the document (`jam_TerminalHistory.cpp:3-31`).
  - `TextEditor::addFileLinks` reads the editor's `state` and `terminal` ValueTrees (`jam_TextEditor.cpp:1468-1480`). It also calls `juce::File::exists` (`jam_AnsiDocument.cpp:327`).
- **Block lookups.** `getBlockStart`, `getBlockTop`, `getBlockNumber` and `getBlockNumberAt` sum the line and pixel counts of the `history` entries with a smaller key (`jam_TextEditor.cpp:1805-1863`). Thus any ordered key works.
- **Fixed-number users.**
  - `TerminalSwap::getBlockNumber` and `getBlockLines` have these users: `addBlocks (TerminalSwap&)` (`:1709-1725`), `removeHeadBlocks` (`:1442`), `getOrCreateListLine` (`:2418-2424`) and `getListLine` (`:2438-2440`).
  - `TerminalHistory` uses `linesPerBlock` only as its read-ahead chunk (`jam_TerminalHistory.cpp:165-261`).

## Constraints

- No platform branch outside the native layer.
- Message thread: no wait, no blocking call.
  - A lock is `juce::CriticalSection` (ARCHITECT: *"use CriticalSection or MessageManagerLock NEVER HANDROLL"*).
  - A job runs on a `juce::ThreadPool`, the existing pattern: `PreviewComponent::FrameJobs : public juce::ThreadPool` (`jam_PreviewComponent.h:114`).
- No comment and no doxygen before the audit (CAROL.md Code Hygiene). Design by Contract per CODING.md CRITICAL RULES.
- Engineer introduces no name outside this plan. The new names:
  - `historyEnd` (a getter `getHistoryEnd`, family `getNumHistoryLines`);
  - `ligaturesLock` (family `fallbacksLock`);
  - `HistoryJobs` / `HistoryJob` (family `FrameJobs` / `FrameJob`).

## Steps

### Step 1: Batch blocks on the message thread (H3)
**Scope:** `jam_gui/widgets/jam_TextEditor.{h,cpp}`. The rules:
1. **The key.** A `history` key is the first swap line of the block. It is no longer `line / linesPerBlock`.
2. **One new block per pass.** Let `getHistoryEnd()` be the largest key plus the block's `Id::size`, or `swap.getLines().getStart()` when `history` is empty. Each `addBlocks (TerminalSwap&)` pass builds one block from `[getHistoryEnd(), min (getHistoryEnd() + linesPerBlock, swap.getLines().getEnd()))`. It repeats while new lines remain.
3. **Never rebuilt.** A block is never rebuilt for a length change or an open-end change. `isLengthChanged` and `isOpenChanged` are deleted.
4. **Open last logical line.** A block whose last logical line is open (`Id::end` set by `TerminalHistory::addLines`, `jam_TerminalHistory.cpp:107-108`) is not added. Its lines are read again on the next pass, when more lines exist. The live block shows the screen in the meantime.
5. **The link rule stays.** `isLinkChanged` (`:1720-1722`) is kept: when the recording block ends, a block built during recording is replaced once. That is a state update, not a per-frame rebuild.
6. **Head removal.** `removeHeadBlocks` removes a block when its key plus `Id::size` is at most `swap.getLines().getStart()`.
7. **Grid lookup.** `getListLine` and `getOrCreateListLine` find the block whose key is at most the grid start and whose end is above it. This replaces `TerminalSwap::getBlockNumber`.

**Validation:**
- `addLines` runs once per new block, not once per frame per block.
- `editor block layout` calls scale with new lines.
- Scout `seq-100k-windows.md` is run again, and the numbers go to ARCHITECT.

### Step 2: Ligature cache under a CriticalSection (H1)
**Scope:** `jam_graphics/fonts/jam_Typeface.{h,cpp}`.
- Add `juce::CriticalSection ligaturesLock`, the pattern of `fallbacksLock` (`jam_Typeface.h:333`, `jam_Typeface.cpp:260`, `:281-283`).
- `getOrCreateLigature` takes it with `const juce::ScopedLock`. Its `JUCE_ASSERT_MESSAGE_THREAD` is deleted.

**Validation:** layout may run off the message thread. The lock is held only for one map lookup or insert.

### Step 3: History blocks built on a parallel job (H1)
**Scope:** `jam_gui/widgets/jam_TextEditor.{h,cpp}`.
1. **The job.** A `HistoryJobs : public juce::ThreadPool` member with one thread runs a `HistoryJob`. The job reads the lines (`TerminalHistory::addLines`), builds the `Document::Index`, and lays out the block (`TextBlock::setBlock`).
   - Its inputs come from the message thread when the job starts. These are the style reference, the view width, the scale, and the recording block with its directory map for `addFileLinks`.
   - `terminalHistory` moves into the job's ownership.
2. **The hand-over.** The job hands the complete `TextBlock` to the message thread through the editor's `juce::AsyncUpdater`, which already exists (`jam_TextEditor.h:124`). The message thread only inserts it into `history`.
3. **Shutdown and theme change.** On `lookAndFeelChanged`, on `resized` with a new width, and in the destructor:
   1. The editor calls `removeAllJobs (true, timeout)`.
   2. It then lays out again.
   This is the one place where the message thread waits, and only for one running block build.

**Validation:**
- `editor history blocks` on the message thread is near 0 in scout.
- No message-thread wait during `seq`.

### Step 4: Measure and report
- Scout `seq-100k-windows.md`, Release.
- The numbers go to ARCHITECT: epoch, user, editor blocks, layout, history lines.
- ARCHITECT runs `time seq 1 10000` and `seq 1 100000` in EVE on Windows (the Verdict lane).

## Open questions

- **Q1.** Step 3.3 makes the message thread wait for one block build, on a theme change or a width change only. Is that within *"could lock"*? The alternative is a generation number: a stale block is dropped on arrival, and the message thread never waits.
- **Q2.** Step 3 moves the `TerminalHistory` link cache (`links`) off the message thread. `setLiveBlock` also reads lines through `terminalHistory.addLiveLines` (`jam_TextEditor.cpp:1568`). Thus the live block needs its own `TerminalHistory`, or the live build moves to the job too.

## BLESSED Alignment

- **S (Stateless) and SSOT.** The end of the history is derived from `history` (`getHistoryEnd`). It is not a counter.
- **Materialisation.** A block is built once, complete. A replacement happens only for the recording-link state update.
- **B (Bound).** The job pool is owned by the editor. It is emptied before layout state changes and before destruction.
