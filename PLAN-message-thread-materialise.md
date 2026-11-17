# PLAN: message-thread-materialise

**RFC:** none. The objective comes from ARCHITECT's prompt.
**Date:** 2026-11-16
**BLESSED Compliance:** verified against the reads cited below
**Language Constraints:** C++17 / JUCE 8.0.17 (`eve/CMakeLists.txt:139`) / JAM

## Overview

ARCHITECT's objective, verbatim: *"DESIGN BY CONTRACT. NO PARSING ON MESSAGE THREAD. MESSAGE THREAD ONLY MATERIALISE IN PARALLEL"*.

After this plan:
- A parallel job does every parse of the record: line decode, wrap join, grid assembly, links and `Document::Index`.
- The message thread only lays out each finished block (materialisation) and never waits for a job.
- The record publishes the end of its last complete logical line. Each history block is built once, up to that end. It is not built again when lines arrive.

## Rulings

| # | Ruling (verbatim) | Decides |
|---|---|---|
| M1 | *"Message thread (Recommended)"*: the job parses, and the message thread lays out each finished block once and does not wait | Steps 4-5 |
| M2 | *"Writer publishes closed end (Recommended)"*: TerminalSwap publishes the end of its last complete logical line, and each block is built once up to it | Steps 1-3, 5 |
| H1 | *"Both (Recommended)"*: *"build each history block once, complete, on a parallel job; the message thread only swaps the finished block in"* (PLAN-history-batch-blocks.md:19). M1 moves its layout part to the message thread. | Steps 4-5 |
| H3 | *"Batch blocks"*: *"each frame one new block from only the lines that arrived since the last block; complete at creation, never rebuilt; history keyed by first line"* (PLAN-history-batch-blocks.md:21). M2 replaces its rule 4 (open tail). | Step 5 |

This plan replaces `PLAN-history-batch-blocks.md`. That plan was never executed.

## Evidence (read this session)

**What the message thread parses today.**
1. The VBlank calls `addBlocks()` (`jam_TextEditor.cpp:100`). That calls `setLiveBlock (TerminalSwap&)` and `addBlocks (TerminalSwap&)` (`:1499-1535`).
2. `setLiveBlock` decodes the live frame with `terminalHistory.addLiveLines`, builds `Document::Index` and calls `addFileLinks` (`:1561-1577`).
3. `addBlock` runs `terminalHistory.addLines`, `Document::Index`, `addFileLinks` and `setBlock` (`:1737-1754`).
4. `addLines` decodes each row with `AttributedString::read` (`jam_TerminalHistory.cpp:7`). It reads forward past the range for wrapped and grid tails (`:157-186`, `:244-278`) and checks grids into scratch documents (`:210-242`). Then it runs `AnsiDocument::addWrappedLines` and `addGrids` (`:116-117`).
5. `AnsiDocument::addFileLinks` calls `juce::File::exists` for each word of each finished output line (`jam_AnsiDocument.cpp:320-330`, `:377-388`).
6. `getOrCreateListLine` builds a missing block on the message thread (`jam_TextEditor.cpp:2417-2435`). `setViewToList` calls it (`:2462`).

**Why blocks are built again.**
- `addBlocks (TerminalSwap&)` rebuilds a resident block on `isLengthChanged`, `isOpenChanged` or `isLinkChanged` (`jam_TextEditor.cpp:1719-1726`).
- `addLines` sets the root `Id::end` on every block whose range reaches the swap end: `isOpen or lines.getEnd() + numTailLines >= lastLine` (`jam_TerminalHistory.cpp:113`).
- MANIFESTO E, Materialisation: *"Do not build a concrete object in steps. Do not build it again."*

**Correction of an earlier statement.** COUNSELOR said that `seq` output is one open grid. That is wrong.
- `isGridStart` needs two or more cell starts (`jam_AnsiDocument.cpp:390-396`), and a `seq` row has one.
- The `open true` flag in the log comes from the swap-end branch at `jam_TerminalHistory.cpp:113`.

**Where layout reads unguarded state (M1).**
- The layout reads LookAndFeel colours (`jam_GlyphArrangement.cpp:621-625`, `jam_Colours.cpp:156`). No lock guards them (`juce_LookAndFeel.cpp:86-107`).
- A theme change writes them in place (`jam_StyleManager.cpp:197-199`, `:211`).
- The ligature cache asserts the message thread (`jam_Typeface.h:317`).
- The layout reads `juce::Desktop` (`jam_TextEditor.cpp:1778`).

**Where a logical line closes (M2).**
- The wrapped mark is on the continuation row (`jam_TerminalProcessor.cpp:505-510`). The writer stores it as `isWrapped and placements.isEmpty()` (`jam_TerminalSwap.cpp:94`).
- A grid run continues while the next row keeps its `gridStart` (`jam_TerminalSwap.cpp:142-150`). A run is a grid only when the row after it has another output block (`jam_AnsiDocument.cpp:426-427`, `jam_TerminalHistory.cpp:268`).
- OSC 133 D moves the rows above the cursor into history with `scrollUp`, on the normal screen only (`jam_TerminalProcessor.cpp:1243-1254`, `jam_TerminalBuffer.cpp:311-332`).
- The writer cannot read the live rows. The clock (`VirtualDevice` context) and the writer (`EVETerminalProcessor`'s own `jam::AsyncUpdater` context) are two contexts (`jam_VirtualDevice.h:19-21`, `EVETerminalProcessor.h:30-32`). The writer reads only the slots that `read()` publishes (`jam_TerminalBuffer.cpp:97-104`).
- A per-slot mark moves with its row, as `wrappedRows` does (`jam_TerminalBuffer.cpp:27`, `:58`, `:137`, `:142`, `:287`, `:356`).

## Language / Framework Constraints

- **One object, one translation unit** (CODING.md). The job types are nested in `TextEditor`, so their bodies go in `jam_TextEditor.cpp`.
- **The 30-line and 3-branch limits apply** (LANGUAGE.md:36).
- **Shared code.** This plan changes shared JAM code: `jam_TerminalBuffer`, `jam_TerminalProcessor`, `jam_TerminalSwap`, `jam_TerminalHistory`, `jam_AnsiDocument`, `jam_TextEditor`. The read path of the history has no platform half. ARCHITECT's *"these changes should be on windows only"* was said about the frame-composition plan.
- **Message thread.** No wait, and no blocking call. The one wait is the JUCE `removeJob` of a job that has already set its finish flag, the established pattern (`jam_PreviewComponent.cpp:362-366`, `:413-416`).
- **Hygiene.** No comment and no doxygen before the audit. Design by Contract per CODING.md CRITICAL RULES. Engineer runs no git. Builds run only through cast or the scout `## toolchain` rows.
- **Names.** Engineer adds no name that this plan does not list.

## Dependency & API Inventory

**JAM (priority)**

| Need | Pattern used | Cite |
|---|---|---|
| Per-slot row mark | `wrappedRows`, `setWrapped`, `isWrapped` | `jam_TerminalBuffer.h:371-373`, `:486`; `.cpp:27`, `:58`, `:134-143`, `:287`, `:356`, `:365` |
| Published line counter | `std::atomic<juce::int64> firstLine`, `endLine`, release store, acquire load | `jam_TerminalSwap.h:261-262`, `.cpp:187-188`, `:211-214` |
| Job on a pool, hand-back to the message thread | `DecodeJob` / `RecordJob` / `FileJob`, `isFinished` release store, `owner.triggerAsyncUpdate()` | `jam_PreviewComponent.h:166-319`, `.cpp:208-212`, `:368-380`, `:389-421` |
| Job slot | `std::unique_ptr<DecodeJob> job` per preview | `jam_PreviewComponent.cpp:368-380` |
| Wait constant | `static constexpr int waitForJob { -1 }` | `jam_PreviewComponent.h:106` |
| Swap held by a job | `const TerminalSwap& swap` | `jam_PreviewComponent.h:283` |
| Map | `jam::HashMap` `addOrReplace`, `contains`, `at` | `jam_TerminalHistory.cpp:11`, `:18` |
| One-line peek into a scratch document | `isGridOpen` | `jam_TerminalHistory.cpp:231-242` |
| Message-thread hand-back | `TextEditor` is a private `juce::AsyncUpdater`, and `handleAsyncUpdate` calls `addBlocks()` | `jam_TextEditor.h:124`, `.cpp:44-48` |

**JUCE**
- `juce::ThreadPool (const Options&)`, `ThreadPoolOptions::withNumberOfThreads` (`juce_ThreadPool.h:166-198`, `:223`).
- `addJob (ThreadPoolJob*, bool deleteJobWhenFinished)` (`:290-291`).
- `removeJob (job, interruptIfRunning, timeOutMilliseconds)` (`:320-322`, `.cpp:250-277`).
- `~ThreadPool` calls `removeAllJobs (true, 5000)` and then `stopThreads()` (`juce_ThreadPool.cpp:130-134`).
- `ThreadPoolJob (const String& name)`, `runJob`, `jobHasFinished`.

**New names (each from a cited family, NAMES.md Rule 5)**

| Name | Family |
|---|---|
| `TerminalBuffer::closedRows`, `setClosed (jam::Cell row, bool isRowClosed)`, `isClosed (int slot)` | `wrappedRows`, `setWrapped (jam::Cell row, bool isRowWrapped)`, `isWrapped (int slot)` |
| `TerminalSwap::closedLine`, `getClosedLines()` | `firstLine`, `endLine`, `getLines()`; "closed end" from M2 |
| `TerminalHistory::getOutputBlock (const TerminalSwap&, juce::int64 line)` | `TerminalBuffer::getOutputBlock (int slot)` |
| `AnsiDocument::addGrids (AnsiDocument&, juce::int64 nextOutputBlock)`, `hasGridLine (juce::int64 gridStart)` | `addGrids`, `getGridLine` |
| `TextEditor::BlockJob`, `LiveJob`, `HistoryJob` | `DecodeJob`, `RecordJob`, `FileJob` |
| `BlockJob` members `owner`, `swap`, `recordingBlock`, `directories`, `cwd`, `block`, `isFinished`, `setFinished`, `addFileLinks`; ctor params `ownerToUse`, `swapToUse`; `HistoryJob::lines`, `linesToUse` | `DecodeJob::owner`, `RecordJob::swap`, `setFinished`, `isFinished`; `TextEditor::addFileLinks`, `getRecordingBlock`; `Id::cwd` |
| `TextEditor::liveJob`, `historyJob`, `blockJobs`, `applyJobs`, `addJobs`, `getHistoryEnd`, `hasListLine`, local `waitForJob` | `preview.job`, `decodeJobs`, `applyFrames`, `addJob`; `getNumHistoryLines`; `getListLine`; `waitForJob` |

## Validation Gate

COUNSELOR validates each step before the next one. The checks are:
- MANIFESTO.md (BLESSED)
- NAMES.md
- ~/.carol/CODING.md
- this PLAN, with no deviation and no scope drift.

@Auditor runs once, after the final code step. It never runs per step.

## Steps

### Step 0: Write the plan to the EVE root
**Scope:** `D:\Poems\dev\eve\PLAN-message-thread-materialise.md` (new).
**Action:** COUNSELOR copies this approved plan there.
**Validation:** the file content equals the approved plan.

### Step 1: The OSC 133 D row closes the history (M2)
**Scope:** JAM `jam_terminal/buffers/jam_TerminalBuffer.{h,cpp}`, `jam_terminal/processor/jam_TerminalProcessor.cpp`.

**Action, `TerminalBuffer`.** Add `closedRows` beside each `wrappedRows` line, in the same form:
1. Member `jam::Array<bool> closedRows;` after `wrappedRows` (`.h:486`).
2. Declarations `void setClosed (jam::Cell row, bool isRowClosed);` and `bool isClosed (int slot) const;` after `isWrapped` (`.h:371-373`).
3. In `.cpp`:
   - the rotate (`:27`);
   - the resize (`:58`);
   - the bodies of `setClosed` and `isClosed`, in the form of `:134-143`;
   - `closedRows.set (slot, false);` in `clear (const ScopedWrite&)` (`:287`);
   - the swap (`:356`);
   - `setClosed (row, false);` in `clear (jam::Cell)` (`:365`).

**Action, D callback** (`jam_TerminalProcessor.cpp:1243-1254`). Inside the `map::Screen::normal` branch, before `scrollUp`, add:
```cpp
if (processor.cursor.y > 0)
    terminalBuffer.setClosed (jam::Cell { processor.cursor.y - 1 }, true);
```

**Validation:**
- Each `wrappedRows` site has its `closedRows` sibling.
- The mark sits on the last row that D moves into history.
- A reused slot has no mark.

### Step 2: TerminalSwap publishes the closed end (M2)
**Scope:** JAM `jam_terminal/document/jam_TerminalSwap.{h,cpp}`.

**Action:**
1. Add the member `std::atomic<juce::int64> closedLine { 0 };` after `endLine` (`.h:262`).
2. Add `juce::Range<juce::int64> getClosedLines() const noexcept;` after `getLines()` (`.h:142`).
   - Body: load `firstLine` once with acquire.
   - Return `{ first, juce::jmax (first, closedLine.load (std::memory_order_acquire)) }`.
3. In `writeFromTerminalBuffer`, in the `forEach` (`.cpp:89-108`), after `gridStart` (`:96`):
   1. When `not isWrapped and (gridStart == TerminalBuffer::noGridStart or gridStart != previousGridStart)`, store `newEndLine` to `closedLine` with release. Every line before this row is complete.
   2. After `writeLine` and `setLines`, when `terminalBuffer.isClosed (slot)`, store `newEndLine + 1` to `closedLine` with release.

**Validation:**
- `closedLine` never passes `endLine`.
- A wrapped run or a grid run that continues does not move it.
- The row that D marks closes its own line.

### Step 3: TerminalHistory reads only closed lines (M2)
**Scope:** JAM `jam_terminal/document/jam_TerminalHistory.{h,cpp}`, `jam_terminal/document/jam_AnsiDocument.{h,cpp}`.

**Action, bounds.** Replace `swap.getLines().getEnd()` with `swap.getClosedLines().getEnd()` at `jam_TerminalHistory.cpp:165`, `:167`, `:221`, `:226`, `:259` and `:261`. The head checks `:145` and `:196` keep `getLines().getStart()`.

**Action, no open block.** Delete:
- `isGridOpen` (`.cpp:231-242`) and its declaration;
- `lastLine` and `isOpen` (`:100`, `:103`);
- the `Id::end` write (`:113-114`).

**Action, peek.** Add the private `juce::int64 getOutputBlock (const TerminalSwap& swap, juce::int64 line)`.
- When `line` is below `swap.getLines().getEnd()`, it reads that one line into an `AnsiDocument scratch`, in the form of `:233-241`, and returns its `Id::outputBlock`.
- Otherwise it returns `TerminalBuffer::noOutputBlock`. After D, the row after the output has no output block.

**Action, coverage.**
1. `addGridLines` counts the run lines that it keeps. It returns that count when `isGrid` is true, and 0 otherwise.
2. `addGridTail` returns that count, or 0.
3. At `:268`, `isGrid` becomes: `next != nullptr ? *next->get<juce::int64> (Id::outputBlock) != *line->get<juce::int64> (Id::outputBlock) : getOutputBlock (swap, swap.getClosedLines().getEnd()) != *line->get<juce::int64> (Id::outputBlock)`.
4. `addLines`:
   1. It computes `coveredEnd` as `lines.getStart() + numLines + numTailLines + numGridTailLines`.
   2. It sets the root `Id::size` to `coveredEnd - lines.getStart()`.
   3. It calls `AnsiDocument::addGrids (document, getOutputBlock (swap, coveredEnd))`.
   4. It returns the covered count.

**Action, `AnsiDocument`:**
1. `addGrids (AnsiDocument& document, juce::int64 nextOutputBlock)`. At `:426-427`, the last run in the document compares `nextOutputBlock` in place of `next == nullptr`.
2. Add `bool hasGridLine (juce::int64 gridStart) const` → `gridLines.contains (gridStart)`, next to `getGridLine` (`:440-443`).

**Validation:**
- No read passes the closed end.
- A grid run that ends in its own output block is not a grid, and a run that D ends is a grid.
- `Id::size` is the number of swap lines that the block covers.
- `addGrids` has one caller (`jam_TerminalHistory.cpp:117`).

### Step 4: The job types and the live block on the job (M1, H1)
**Scope:** JAM `jam_gui/widgets/jam_TextEditor.{h,cpp}`.

**Action, types** (private, after the form of `jam_PreviewComponent.h:166-319`):
1. `struct BlockJob : public juce::ThreadPoolJob`:
   - Constructor `BlockJob (TextEditor& ownerToUse, TerminalSwap& swapToUse)`. It runs on the message thread and passes `Id::line.toString()` to `juce::ThreadPoolJob` (the form of `jam_PreviewComponent.cpp:124`).
     - It sets `recordingBlock` from `owner.getRecordingBlock()`.
     - It sets `cwd` from `juce::URL { owner.terminal.getProperty (Id::cwd).toString() }.getLocalFile()`.
     - It fills `directories` with each child of `owner.state` that has `Id::outputBlock`, as `juce::URL { child.getProperty (Id::value).toString() }.getLocalFile()`.
     - This is the one copy across threads (MANIFESTO E, permitted copy 2).
   - Members:
     - `TextEditor& owner;`
     - `TerminalSwap& swap;`
     - `const juce::int64 recordingBlock;`
     - `jam::HashMap<juce::int64, juce::File> directories;`
     - `const juce::File cwd;`
     - `TextBlock block;`
     - `std::atomic<bool> isFinished { false };`
   - `void addFileLinks (AnsiDocument& document) const` is the body of `TextEditor::addFileLinks` (`jam_TextEditor.cpp:1469-1485`). It uses `recordingBlock`, and its directory lambda is `[this] (juce::int64 outputBlock) { return directories.contains (outputBlock) ? directories.at (outputBlock) : cwd; }`.
   - `void setFinished()` takes the form of `jam_PreviewComponent.cpp:208-212`, with `owner.triggerAsyncUpdate()`.
2. `struct LiveJob : public BlockJob`. Its `runJob` takes the body of `setLiveBlock (TerminalSwap&)` (`:1563-1573`) on `block`:
   1. the `Id::editorLiveBlock` zone;
   2. `owner.terminalHistory.addLiveLines (swap, *document)`;
   3. `block.index`;
   4. `addFileLinks`;
   5. `block.document`;
   6. `setFinished()`, then return `jobHasFinished`.

**Action, members.** Add `std::unique_ptr<LiveJob> liveJob;` and `std::unique_ptr<HistoryJob> historyJob;`. Then add `juce::ThreadPool blockJobs { juce::ThreadPool::Options{}.withNumberOfThreads (1) };` as the last member, so that the pool is destroyed first (`juce_ThreadPool.cpp:130-134`).

**Action, functions:**
1. `bool applyJobs()`. When `liveJob` has `isFinished` (acquire):
   1. `blockJobs.removeJob (liveJob.get(), true, waitForJob)`, with a local `static constexpr int waitForJob { -1 }`;
   2. `setBlock (liveJob->block, false)`;
   3. `undoManager.clearUndoHistory()`;
   4. `live = std::move (liveJob->block)`;
   5. `liveJob.reset()`;
   6. return `true`.
2. `void addJobs (TerminalSwap& swap)`. When `liveJob == nullptr and swap.hasLiveLines()`:
   1. `liveJob = std::make_unique<LiveJob> (*this, swap)`;
   2. `blockJobs.addJob (liveJob.get(), false)`.
3. `addBlocks()` (`:1499-1535`):
   - `isLiveUpdated` becomes `const auto isLiveUpdated { applyJobs() };`, and the `setLiveBlock (*swap)` call is deleted.
   - `addJobs (*swap)` runs after `addBlocks (*swap)`.
   - The rest stays.
4. Delete `setLiveBlock (TerminalSwap&)` (`:1561-1577`) and its declaration.

**Validation:**
- No `addLiveLines` call remains on the message thread.
- The message thread calls `setBlock` once for each finished live job.
- One live job at a time.

### Step 5: History blocks on the job, keyed by first line, built once (M1, M2, H1, H3)
**Scope:** JAM `jam_gui/widgets/jam_TextEditor.{h,cpp}`, `jam_terminal/document/jam_TerminalSwap.h`.

**Action, `HistoryJob`.**
- `struct HistoryJob : public BlockJob` with `HistoryJob (TextEditor& ownerToUse, TerminalSwap& swapToUse, juce::Range<juce::int64> linesToUse)` and `const juce::Range<juce::int64> lines;`.
- Its `runJob` runs:
  1. the `Id::editorHistoryBlocks` zone;
  2. `owner.terminalHistory.addLines (swap, *document, lines)`;
  3. `block.index`;
  4. `addFileLinks`;
  5. `block.document`;
  6. `setFinished()`.

**Action, scheduling (in `addJobs`).** When `historyJob == nullptr`:
1. **New lines.** Let `start` be `getHistoryEnd (swap)` and `closed` be `swap.getClosedLines()`. When `closed.getEnd() > start`, add a `HistoryJob` for `{ start, juce::jmin (start + TerminalSwap::linesPerBlock, closed.getEnd()) }`.
2. **Link replacement (H3 rule 5).** Otherwise, find the first `history` entry whose root `Id::outputBlock` is neither `TerminalBuffer::noOutputBlock` nor `getRecordingBlock()`. Add a `HistoryJob` for `{ key, key + *root->get<int> (Id::size) }`. This is the one state update that replaces a block.

**Action, apply (in `applyJobs`).** When `historyJob` has `isFinished`:
1. `removeJob`;
2. when the root `Id::size` is not 0, `setBlock (historyJob->block, true)` and then `addBlock (historyJob->lines.getStart(), std::move (historyJob->block))` (`:1408-1429`);
3. `historyJob.reset()`.

**Action, `getHistoryEnd`.** `juce::int64 getHistoryEnd (const TerminalSwap& swap) const`:
- the largest `key + Id::size` over `history`, at least `swap.getClosedLines().getStart()`;
- `swap.getClosedLines().getStart()` when `history` is empty.

**Action, head removal.** `removeHeadBlocks` (`:1439-1452`) removes an entry when `key + Id::size` is at most `swap.getLines().getStart()`.

**Action, list lines.**
1. `getListLine` (`:2437-2442`) finds the entry with `key <= gridStart < key + Id::size`.
2. Add `bool hasListLine (juce::int64 gridStart) const`. It is true when such an entry exists and its `AnsiDocument::hasGridLine (gridStart)` holds.
3. Delete `getOrCreateListLine` (`:2417-2435`).
4. `setViewToList` (`:2462`), `applyToLinkLabels` (`jam_TextEditor.h:920`) and `paintActiveList` (`:2788`) call `getListLine` only when `hasListLine` holds.

**Action, delete first:**
- `addBlocks (TerminalSwap&)` (`:1703-1735`) and `addBlock (TerminalSwap&, juce::int64, juce::Range<juce::int64>)` (`:1737-1754`);
- `TextEditor::addFileLinks` (`:1469-1485`);
- their declarations;
- `TerminalSwap::getBlockNumber` and `getBlockLines` (`jam_TerminalSwap.h:144-156`, `.cpp:216-219`), once no user remains.

`addBlocks()` then sets `const auto isUpdated { applyJobs() };`, and `isLiveUpdated` and `isHistoryUpdated` are deleted.

**Validation:**
- `addLines` runs only on the job thread.
- Each block is built once, except the H3 rule 5 replacement.
- The message thread runs no `addLines`, `addLiveLines`, `Document::Index` or `addFileLinks`.
- `getHistoryEnd` derives from `history`, with no counter.

### Step 6: Remove the ODE lines
**Scope:**
- `jam_TextEditor.cpp:46` and `:100`. The VBlank lambda goes back to `[this] (double) { addBlocks(); }`.
- `jam_TextEditor.cpp:1515`.
- `jam_VulkanGraphicsEngine.cpp:457` with its added blank line `:458`, and `:461`.
- `EVETerminalProcessor.cpp:46`.

The `:1726-1730` lines go with Step 5.

**Validation:** no `ode:` string remains in the JAM or EVE sources.

### Step 7: Build and measure (Pathfinder)
**Scope:** `benchmark/seq-100k-windows.md`. Its `## toolchain` rows build JAM and EVE through cast.
**Action:**
1. Pathfinder runs `scout` once, with a 15-minute watchdog.
2. A compile error goes back through Engineer to the step that owns the file.
3. COUNSELOR compares these numbers with `benchmark/seq-100k-windows-composition-report.md`:
   - `editor blocks` (message thread);
   - `editor block layout`;
   - `history lines`;
   - the epoch.

**Validation:** the report exists, and the numbers go to ARCHITECT.

### Step 8: Audit
**Action:** @Auditor sweeps once against CONTRACT and this PLAN, across every changed file. Each finding goes through the Design by Contract Filter and is resolved this sprint.

### Step 9: Doxygen pass (Pathfinder)
**Scope:**
- every changed declaration;
- the stale blocks: `jam_TextEditor.h:734`, `:660-673`, `:640-648`, `:1508`, `:1512`, `:1530`;
- `jam_TerminalHistory.h:10-63`;
- `jam_TerminalSwap.h:10-24`, `:141-156`.

**Action:** COUNSELOR gives Pathfinder the symbols, the LSP positions, the index paths and the regeneration command. COUNSELOR then reads each block against the code.

### Step 10: ARCHITECT builds and tests
**Action:** on Windows, ARCHITECT tests:
- `time seq 1 10000` at an interactive prompt (line by line, and no freeze at OSC 133 D);
- `ls` grids;
- links after a command ends;
- the OPEN list mode;
- resize.

## BLESSED Alignment
- **B.**
  - `TextEditor` owns the pool and the two job slots. The pool is destroyed before the members that its jobs touch.
  - The swap outlives the view (SPEC State Model).
- **L.**
  - Four functions and one read-forward test are deleted (`isGridOpen`, `getOrCreateListLine`, `setLiveBlock (TerminalSwap&)`, `addBlock (TerminalSwap&, …)`).
  - Each new function has three branches or fewer.
- **E.**
  - Each name comes from a cited family.
  - The jobs tell the owner through `triggerAsyncUpdate`. The owner never asks a running job.
- **S (SSOT).**
  - The closed end has one source, `TerminalSwap::closedLine`.
  - The history end derives from `history`.
  - The directories cross threads once per job.
- **S (Stateless).** The job slots are the established `preview.job` form.
- **E (Encapsulation), Complete at Creation.**
  - A block is created once, complete, on the job.
  - The message thread materialises it once (M1).
  - The only replacement is the link state update.
- **D.**
  - Each block is built from closed lines only, so the same record gives the same blocks.
  - The pool has one thread, so the jobs finish in order.

## Risks / Open Questions
- None. M1, M2, H1 and H3 are closed.
