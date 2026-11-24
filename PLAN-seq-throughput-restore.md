# PLAN: Restore seq Throughput Without Regressing Universal Architecture

**RFC:** none — objective from ARCHITECT prompt and `scout` profile (`benchmark/seq-10m-mac-report.md`)
**Date:** 2026-11-24
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.17 / JAM

## Overview

This plan removes the two measured bottlenecks that regressed macOS `seq` throughput from `0.543 s` / `5.474 s` (`comparison.md:56-57`) to `1.020 s` / `10.455 s` while keeping the universal architecture (one mapped `TerminalSwapFile`, told workers via Model atomics, `hasLive()` on `Id::closedLine`, and non-blocking `AsyncOutputStream` + `TerminalClock`). It stops the `BlockJob::setFinished()` → `triggerAsyncUpdate()` → `addBlocks()` ~1,600 Hz message-thread spin loop, releases `Id::heldLine` immediately after `terminalHistory.addLines()` returns on the worker thread, and bypasses `source.startTimer()` when `TerminalClock::getPeriodMilliseconds() <= 0.0` on both macOS and Windows.

## Language / Framework Constraints

- **One object, one translation unit** (`LANGUAGE.md:34`, `CODING.md:896-937`): all edits stay in the existing `.cpp` files of `TextEditor` and `AsyncOutputStream`. No new files or headers are added.
- **Deadline threads** (`MANIFESTO.md:269`, `SPEC.md:1205-1207`): the clock context never blocks, waits, locks, or allocates. Bypassing `startTimer()` when `period <= 0.0` calls only `clock.listen()` and `arm()`, both non-blocking.
- **No `ling` AST daemon in this session:** `carol_ling` returned `no daemon for manifest`; every symbol and call site in the inventory below was verified from JAM doxygen XML and direct full-file reads.

## Framework Inventory

### 1. API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| VBlank-paced editor update | JUCE / jam_gui | `juce::VBlankAttachment` (`vBlankAttachment`), `TextEditor::addBlocks()` | `jam_TextEditor.h:1686`, `jam_TextEditor.cpp:101` | 1 |
| Job completion flag | jam_gui | `TextEditor::BlockJob::isFinished`, `BlockJob::setFinished()` | `jam_TextEditor.cpp:1475-1479` | 1 |
| Lock-free `seq_cst` atomic parameter write | jam_data_structures | `jam::Model::storeValue<int64_t> (tag, id, value)` → `Parameter<int64_t>::store (v)` | `jam_Model.h:1013-1019`, `jam_ParameterBase.h:230-234` | 2 |
| Held-line identifier and sentinel | jam_terminal | `Id::heldLine`, `TerminalSwap::noHeldLine` | `jam_Terminal.h:55`, `jam_TerminalSwap.h:35` | 2 |
| Clock period query and listening window end | jam_terminal | `TerminalClock::getPeriodMilliseconds()`, `TerminalClock::listen()` | `jam_TerminalClock.h:56, 66`, `jam_TerminalClock.cpp:43-51` | 3 |
| Readiness re-arm and one-shot timer | jam_core | `AsyncUpdater::Source::arm()`, `AsyncUpdater::Source::startTimer (double)` | `jam_AsyncUpdater.h:131, 141`, `jam_AsyncUpdater.cpp:195-206, 271-284` | 3 |

### 2. Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| VBlank drives view updates (`addBlocks()`) | `jam_TextEditor.cpp:101` | `jam_TextEditor.cpp:100-101, 1140, 2840, 2846, 2852, 2858` | 1 |
| Non-message-thread atomic store of a terminal hold parameter via `Model::storeValue` | `EVETerminalProcessor.cpp:344, 351` (`state.storeValue (terminalTag, Id::heldImage, ...)`) | `EVETerminalProcessor.cpp:344, 351`; `jam_Model.h:1014` | 2 |
| Immediate re-arm on zero deferral in `AsyncOutputStream` | `jam_AsyncOutputStream_windows.cpp:42-45` (`arm (*this)`) and `jam_AsyncOutputStream_mac.cpp:27-31` (`clock.listen(); source.arm();`) | `jam_AsyncOutputStream_mac.cpp:23-31`, `jam_AsyncOutputStream_windows.cpp:38-52` | 3 |

### 3. Names

| Family | Members | Intent | Step |
|---|---|---|---|
| `TextEditor::BlockJob` methods | `setFinished`, `getRow`, `addFileLinks` (`jam_TextEditor.cpp:1460-1484`) | No new name; existing `setFinished` drops the `triggerAsyncUpdate()` call | 1 |
| `jam::Model` atomic writers | `storeValue` (`jam_Model.h:1014`) | Writes `seq_cst` atomic parameter from any thread | 2 |
| `TerminalClock` / `AsyncUpdater::Source` | `getPeriodMilliseconds`, `listen`, `arm`, `startTimer` (`jam_TerminalClock.h:56,66`, `jam_AsyncUpdater.h:131,141`) | Local constant `period` matches `TerminalClock::getPeriodMilliseconds()` | 3 |

### 4. Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "Aggregate (brace) initialization is ALWAYS preferred" | `const auto period { clock.getPeriodMilliseconds() };` | `jam_AsyncOutputStream_windows.cpp:28-29` |
| "No bail-out guards — positive nesting" | `if (const auto period { ... }; period > 0.0) { ... } else { ... }` | `jam_AsyncOutputStream_windows.cpp:38-45` |
| "Use C++ alternative tokens: `not`, `and`, `or`" | `not`, `and`, `or` everywhere outside preprocessor | `jam_TextEditor.cpp:1538` |
| "Use the framework API fully" | `terminalModel.storeValue (terminalTag, Id::heldLine, ...)` instead of a new atomic or lock | `EVETerminalProcessor.cpp:344, 351` |
| "Deadline threads: no allocate, lock, wait, or blocking call" | Direct `clock.listen(); source.arm();` when `period <= 0.0`; non-blocking timer when `period > 0.0` | `jam_AsyncOutputStream_mac.cpp:27-31` |

### 5. Absence

| Need | Query | Result |
|---|---|---|
| Zero-delay shortcut inside `AsyncUpdater::Source::startTimer` | Direct read of `jam_AsyncUpdater.cpp:201-206` (macOS), `:271-284` (Windows), `:329-333` (wrapper) | None — `startTimer` always schedules the OS timer (`dispatch_source_set_timer` / `SetThreadpoolTimer`), and `AsyncUpdater::Source` only has two `startTimer` callers (`jam_AsyncOutputStream_mac.cpp:24`, `jam_AsyncOutputStream_windows.cpp:40`). |
| Other `TextEditor::triggerAsyncUpdate()` callers | Full read of `jam_TextEditor.cpp:1-2956` | Only two callers exist: `Id::isRecording` property change (`:1369`, kept) and `BlockJob::setFinished()` (`:1478`, removed in Step 1). |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step and runtime proof,
covering the whole sprint — never per step. Doxygen updates run in the dedicated post-audit
Pathfinder pass only (`CAROL.md` Code Hygiene).

## Steps

### Step 1: Pace `TextEditor::addBlocks()` to vblank only for completed block jobs
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:**
1. In `TextEditor::BlockJob::setFinished()` (`jam_TextEditor.cpp:1475-1479`), remove `owner.triggerAsyncUpdate();`. Keep `isFinished.store (true, std::memory_order_release);`.
2. Leave `TextEditor::handleAsyncUpdate()` (`:44-47`), `juce::AsyncUpdater` inheritance (`jam_TextEditor.h:128`), and `Id::isRecording` (`jam_TextEditor.cpp:1369`) untouched so prompt-boundary updates still wake immediately while `BlockJob` harvests run once per vblank via `vBlankAttachment` (`:101`).
3. No comments and no doxygen edits in this step.
**Inventory:** API row 1–2 (`vBlankAttachment`, `BlockJob::setFinished`); Pattern row 1; Names row 1.
**Validation:** COUNSELOR reads `jam_TextEditor.cpp:1470-1485`, verifies only `owner.triggerAsyncUpdate();` was removed, no other symbol or comment changed, and `Id::isRecording` (`:1369`) remains intact.

### Step 2: Release `Id::heldLine` immediately after `terminalHistory.addLines()` returns
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:**
1. In `TextEditor::HistoryJob::runJob()` (`jam_TextEditor.cpp:1516-1531`), immediately after `owner.terminalHistory.addLines (...)` (`:1524`) and before `block.index = ...` (`:1525`), add:
   ```cpp
   terminalModel.storeValue (terminalTag, Id::heldLine, int64_t { TerminalSwap::noHeldLine });
   ```
2. Keep `HistoryJob::HistoryJob` (`:1507-1508`) and `HistoryJob::~HistoryJob` (`:1513`) unchanged so the message-thread `ValueTree` property stays in sync when `setJobs()` destroys the finished or cancelled job.
3. No comments and no doxygen edits in this step.
**Inventory:** API row 3–4 (`Model::storeValue`, `Id::heldLine`, `TerminalSwap::noHeldLine`); Pattern row 2 (`EVETerminalProcessor.cpp:344, 351`); Synthesis row 4.
**Validation:** COUNSELOR reads `jam_TextEditor.cpp:1505-1535`, verifies `terminalModel.storeValue` runs right after `addLines` (before `Document::Index` and `addFileLinks`), uses `int64_t { TerminalSwap::noHeldLine }` (matching `jam::Parameter<int64_t>`), and touches no `ValueTree` from the worker thread.

### Step 3: Bypass `source.startTimer()` when `TerminalClock::getPeriodMilliseconds() <= 0.0`
**Scope:** `jam/jam_terminal/tty/native/jam_AsyncOutputStream_mac.cpp`, `jam/jam_terminal/tty/native/jam_AsyncOutputStream_windows.cpp`
**Action:**
1. In `jam_AsyncOutputStream_mac.cpp:23-24`, replace:
   ```cpp
   if (numSamples > 0)
       source.startTimer (clock.getPeriodMilliseconds());
   ```
   with:
   ```cpp
   if (numSamples > 0)
   {
       if (const auto period { clock.getPeriodMilliseconds() }; period > 0.0)
       {
           source.startTimer (period);
       }
       else
       {
           clock.listen();
           source.arm();
       }
   }
   ```
2. In `jam_AsyncOutputStream_windows.cpp:38-45`, apply the identical check inside `if (numReady > 0)`:
   ```cpp
   if (numReady > 0)
   {
       if (const auto period { clock.getPeriodMilliseconds() }; period > 0.0)
       {
           source.startTimer (period);
       }
       else
       {
           clock.listen();
           arm (*this);
       }
   }
   else if (numReady == 0)
   {
       arm (*this);
   }
   ```
3. No comments and no doxygen edits in this step.
**Inventory:** API row 5–6 (`TerminalClock::getPeriodMilliseconds`, `TerminalClock::listen`, `AsyncUpdater::Source::arm`, `startTimer`); Pattern row 3; Names row 3; Synthesis rows 1, 2, 5; Absence row 1.
**Validation:** COUNSELOR reads both files at the edited lines, confirms both platform halves are isomorphic, neither calls moved-from `source.onTimer`, formatting matches `CODING.md` (Allman braces, brace init, space before `(`), and zero comments were added.

### Step 4: Measure with `scout` and verify against baseline
**Scope:** `eve/benchmark/seq-10m-mac-after.md` (manifest, `baseline: seq-10m-mac-report.md`)
**Action:**
1. COUNSELOR writes `eve/benchmark/seq-10m-mac-after.md` (`epochs: 3`, `seq 1 10000000`, `baseline: seq-10m-mac-report.md`) and delegates the `scout` run to `@Pathfinder`.
2. Verify `editorBlocks`, `editorBlockLayout`, `swapHistoryLines`, and `vulkan end frame` call counts drop from ~20,000+ back to vblank-paced counts (~600), and compare `epoch` against `seq-10m-mac-report.md` (`22.51 s`) and `2026-10-29` (`5.58 s` equiv.).
3. ARCHITECT builds Release and runs `time seq 1 1000000` and `time seq 1 10000000` against the `0.543 s` / `5.474 s` baseline (`comparison.md:56-57`).
4. If the verdict still trails `0.543 s` / `5.474 s`, run a `## sample` stack profile on macOS to isolate the remaining delta before audit.
5. After runtime proof, `@Auditor` runs once, followed by the Pathfinder doxygen pass (`jam_TextEditor.h:38, 1575, 1645, 1652`, `jam_AsyncOutputStream.h:17-18`), then `/log`.
**Inventory:** `profile` skill (`scout --mcp`, `--standalone-profile`).
**Validation:** Report shows `dropped == 0` and statistically significant `epoch` reduction; ARCHITECT confirms interactive `time seq` meets or beats baseline.

### Step 5: Drain `terminalBuffer` independently of vblank and disable macOS GCD timer coalescing
**Scope:** `eve/Source/EVETerminalProcessor.cpp`, `jam/jam_core/concurrency/jam_AsyncUpdater.cpp`
**Action:**
1. In `EVETerminalProcessor::process()` (`eve/Source/EVETerminalProcessor.cpp:574-584`), after `while (terminalProcessor.process (...)) {}`, add a function-local constant `static constexpr int freeRowThreshold { virtualBlockSize / 2 };` and call `triggerAsyncUpdate();` when `terminalBuffer.getNumFreeRows() < freeRowThreshold`.
2. In `AsyncUpdater::Source::Native::Native()` (`jam/jam_core/concurrency/jam_AsyncUpdater.cpp:138`), pass `DISPATCH_TIMER_STRICT` instead of `0` as the mask argument to `dispatch_source_create (DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, queue)`.
3. No comments and no doxygen edits in this step.
**Inventory:** API: `TerminalBuffer::getNumFreeRows()` (`jam_TerminalBuffer.cpp:92`), `AsyncUpdater::triggerAsyncUpdate()` (`jam_AsyncUpdater.cpp:76-88`, lock-free coalesced), `DISPATCH_TIMER_STRICT` (`<dispatch/dispatch.h>`, external SDK constant per `NAMES.md` Rule 2). Synthesis: narrowest scope (`CODING.md:270-274`), no magic value (`MANIFESTO.md:89`).
**Validation:** COUNSELOR reads both files at the edited lines, confirms zero comments, exact scope, and re-runs `scout` (`benchmark/seq-10m-mac-after.md`) before ARCHITECT runs Release `time seq`.

### Step 6: Hybrid native driver pacing in `AsyncOutputStream` (macOS only — ARCHITECT ruling "A")
**Scope:** `jam/jam_terminal/tty/native/jam_AsyncOutputStream_mac.cpp`
**Action:**
1. In `jam_AsyncOutputStream_mac.cpp:23-35`, add `static constexpr double timerThresholdMilliseconds { 0.2 };` and read `const auto period { clock.getPeriodMilliseconds() };`.
2. When `period >= timerThresholdMilliseconds`, call `source.startTimer (period)` (non-blocking GCD timer for interactive/slow traffic).
3. Otherwise (`period < timerThresholdMilliseconds`), when `period > 0.0`, call `std::this_thread::sleep_for (std::chrono::duration<double, std::milli> { period });`, then call `clock.listen();` and `source.arm();`.
4. Leave `jam_AsyncOutputStream_windows.cpp` untouched. No comments and no doxygen edits in this step.
**Inventory:** API: `TerminalClock::getPeriodMilliseconds()`, `TerminalClock::listen()`, `AsyncUpdater::Source::startTimer()`, `AsyncUpdater::Source::arm()`, `std::this_thread::sleep_for` + `std::chrono::duration<double, std::milli>` (transitive via `juce_core`, `CODING.md:779`). Synthesis: narrowest scope (`CODING.md:270-274`), no magic values (`MANIFESTO.md:89`).
**Validation:** COUNSELOR reads `jam_AsyncOutputStream_mac.cpp:20-45`, verifies zero comments, zero extra includes, exact scope, and runs `scout` (`benchmark/seq-10m-mac-after.md`) to verify `epoch` and `tty bytes ready` against the `2026-10-29` baseline.

## BLESSED Alignment

- **B (Bound):** Ownership is unchanged. `HistoryJob` still releases `Id::heldLine` in `~HistoryJob()` on cancel/destruction, and releases the atomic hold early once its read finishes.
- **L (Lean):** Zero new files, zero new classes, zero new members, net line reduction in `BlockJob::setFinished()`.
- **E (Explicit):** `if (const auto period { clock.getPeriodMilliseconds() }; period > 0.0)` states the two paths explicitly with positive nesting.
- **S (SSOT):** `vBlankAttachment` is the single pace for block harvests; `jam::Model` `Id::heldLine` remains the single atomic hold.
- **S (Stateless):** No new flag or state added to `TextEditor`, `TerminalClock`, or `AsyncOutputStream`.
- **E (Encapsulation):** Platform timer bypass stays inside the native `AsyncOutputStream` halves; `TextEditor` uses `Model::storeValue`.
- **D (Deterministic):** Removing the ~1,600 Hz worker→message feedback loop eliminates message-thread starvation and `heldLine` stalls during floods; non-blocking re-arm keeps the clock context responsive.

## Risks / Open Questions

- None blocking approval. Step 4's `scout` and interactive `time seq` runs will confirm whether `dispatch_source_set_timer` on sub-full chunks (`period > 0.0`) leaves any remaining delta against `0.543 s` / `5.474 s`.
