# PLAN: Cross-Platform Terminal Throughput Redesign

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-25
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (`LANGUAGE.md`: one object, one translation unit; 30 lines per function and 3 branches per switch unchanged)

## Overview

Eliminate the four cross-platform architectural bottlenecks proven by `scout` and `jam@bff685632` / `eve@d1e2679` forensics while preserving the SIMD UTF-8 codec, contiguous swap row serialization, and Vulkan capacity fixes already in the working tree. Restore full-speed batch swap draining, O(1) identifier hashing, live-tail `HistoryJob` deferral, and exact `TerminalClock` pacing so macOS and Windows both run at hardware-paced throughput with zero data races.

## Language / Framework Constraints

- **One object, one translation unit** (`CODING.md:896-937`, `LANGUAGE.md:25-39`): Keep templates and `constexpr` in headers; place every non-template body in its object's single `.cpp` file.
- **Zero includes in submodule files** (`CODING.md:805-846`): Add zero `#include` directives in submodule `.h` and `.cpp` files.
- **Deadline thread contract** (`MANIFESTO.md:269`, `CODING.md:1093`, `SPEC.md:45-65`): `EVETerminalProcessor::process()` and every callee perform zero heap allocation, zero locks, zero waits, and zero blocking calls.
- **Data-race-free handoff** (`MANIFESTO.md:271, 438`, `SPEC.md:490-495`): Published swap bytes stay immutable while a reader holds them. Every `firstLine` store and `heldLine` load on the hold boundary stays `std::memory_order_seq_cst`.
- **Pre-audit hygiene** (`CAROL.md` Code Hygiene): Write zero comments and zero doxygen blocks during implementation; run comments and doxygen in the dedicated post-audit Pathfinder pass.

## Framework Inventory

### 1. API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| O(1) avalanching integer hash for interned `juce::Identifier` | `jam_core` | `jam::Hash<juce::Identifier>`, `std::hash<juce::Identifier>`, `jam::Wyhash::hashInt` | `jam_core/utilities/jam_Hash.h:17-28, 50-67`; `jam_core/utils/jam_HashMap.h:309` | 1 |
| Lock-free raw atomic parameter access on deadline thread | `jam_data_structures` | `jam::Model::getParameter<T>`, `jam::Parameter<int64_t>::getRawValue`, `Parameter::store` | `jam_data_structures/model/jam_Model.h:996-1008`; `jam_ParameterBase.h:219, 230-237` | 1 |
| VT decoder event publication | `jam_terminal` | `jam::TerminalProcessor::process`, `setBytes`, `setTokens`, `setEvents`, `screenConstruction` | `jam_terminal/processor/jam_TerminalProcessor.h:35, 119`; `jam_TerminalProcessor.cpp:64-120, 305-340, 853-886` | 1 |
| Batch history row ring reader and lane query | `jam_terminal` | `jam::TerminalBuffer::read (int maxRows)`, `getNumFreeRows`, `getFirstHistoryRow`, `getNumRowBytes` | `jam_terminal/buffers/jam_TerminalBuffer.h:212, 328`; `jam_TerminalBuffer.cpp:108-115, 585-590` | 2 |
| Swap record ring space check and placement | `jam_terminal` | `jam::TerminalRecordRing::getNumFreeRecords`, `setRecordStart`, `addRecord`, `isTaken` | `jam_terminal/document/jam_TerminalRecordRing.h:36, 54, 76, 94`; `jam_TerminalRecordRing.cpp:8-92` | 2 |
| Single-file swap writer and Dekker reader | `jam_terminal` | `jam::TerminalSwap::writeFromTerminalBuffer`, `setLineData`, `writeLine`, `setStoredLine`, `setLines` | `jam_terminal/document/jam_TerminalSwap.h:110, 176-189, 248`; `jam_TerminalSwap.cpp:75-221` | 2 |
| View history job scheduler and live-tail viewport | `jam_gui` | `jam::TextEditor::addBlocks`, `addJobs`, `setJobs`, `hasLive`, `isViewAtEnd`, `getMode`, `visibleAreaChanged`, `triggerAsyncUpdate` | `jam_gui/widgets/jam_TextEditor.h:925-1013`; `jam_TextEditor.cpp:48-51, 1307-1326, 1503-1682, 1946-1958` | 3 |
| Cross-platform PTY traffic clock and OS drivers | `jam_terminal` | `jam::TerminalClock::getPeriodMilliseconds`, `process`, `listen`, `AsyncOutputStream::Native` | `jam_terminal/tty/jam_TerminalClock.h:59-66`; `jam_TerminalClock.cpp:48-51`; `jam_AsyncOutputStream_mac.cpp:23-30`; `jam_AsyncOutputStream_windows.cpp:38-60` | 4 |
| Release profiler zones and kernel sampling | `jam_profiler` / `scout` | `jam::Profiler::Zone`, `scout` manifest (`## run`, `## sample`, `## toolchain`) | `profile/SKILL.md`; `eve/benchmark/seq-10m-mac-now*.md` | 5 |

### 2. Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| O(1) interned pointer hash for `juce::Identifier` | `jam_core/utilities/jam_Hash.h:17-28` (`std::hash<juce::Identifier>`) | `jam::HashMap<juce::Identifier, ...>`, `jam::Function::Map<juce::Identifier, ...>`, `jam::Model::getParameter`, `TerminalProcessor::Events` | 1 |
| Cached raw `std::atomic<Type>` parameter binding for deadline thread | `eve/Source/EVETerminalProcessor.h:364-365`, `EVETerminalProcessor.cpp:21` (`bypass`) | `EVETerminalProcessor::process()` (`:581`) | 1 |
| Transient calculation variable living within one `process()` call (`MANIFESTO.md:198, 288`) | `jam_terminal/processor/jam_TerminalProcessor.cpp:73` (`wasSyncOutput`) | `TerminalProcessor::process()` (`:64-104`) | 1 |
| Batch `ScopedRead` over `TerminalBuffer` history rows | `jam_terminal/buffers/jam_TerminalBuffer.cpp:108-115` (`read (int maxRows)`); `jam@bff685632:jam_TerminalSwap.cpp:100-108` | `TerminalSwap::writeFromTerminalBuffer()` (`jam_TerminalSwap.cpp:75-93`) | 2 |
| Dekker `seq_cst` store-load fence between `firstLine` and `heldLine` | `jam_terminal/document/jam_TerminalSwap.h:177-188`, `jam_TerminalSwap.cpp:133-149` | `TerminalSwap::writeFromTerminalBuffer()`, `TextEditor::HistoryJob` | 2, 3 |
| VBlank-driven UI job scheduling with async catch-up | `jam_gui/widgets/jam_TextEditor.cpp:44-47, 100-101, 1644-1677` | `TextEditor::addBlocks()`, `visibleAreaChanged()`, `valueTreePropertyChanged()` | 3 |
| Shared `TerminalClock` law across macOS `WaitableEvent` and Windows `SetThreadpoolTimer` | `jam@bff685632:jam_terminal/tty/jam_TerminalClock.cpp:19-47` | `jam_AsyncOutputStream_mac.cpp:25-26`, `jam_AsyncOutputStream_windows.cpp:40-48` | 4 |

### 3. Names

| Family | Members | Intent | Step |
|---|---|---|---|
| `jam::Hash` specialisation family | `Hash<juce::Identifier>`, `Hash<juce::String>` (`jam_Hash.h:50-80`) | Provide `is_avalanching` 64-bit `operator()` using `Wyhash::hashInt` on interned pointer | 1 |
| `EVETerminalProcessor` atomic parameter bindings | `bypass` (`EVETerminalProcessor.h:364`) | Join with `background` (`std::optional<std::reference_wrapper<const std::atomic<int64_t>>>`, zero raw pointer member per `CODING.md:1078`) | 1 |
| `TerminalBuffer` lane query family (`SPEC.md:540`) | `getNumFreeRows()` (`jam_TerminalBuffer.h:212`) | Join with `getNumReadyRows()` — returns ready history row count (`fifo.getNumReady() - heightRows`) | 2 |
| `TerminalSwap` writer family | `writeFromTerminalBuffer`, `writeLine`, `setStoredLine`, `setLines` (`jam_TerminalSwap.h:110, 235, 248, 318`) | Replace per-row `writeNextLine` with `writeLines` (batch writer) and `getNumWritableLines` (safe batch count) | 2 |
| `TextEditor` query and scheduler family | `isViewAtEnd`, `hasLive`, `getMode`, `addJobs`, `addBlocks` (`jam_TextEditor.cpp:48, 1564, 1622, 1644`) | Keep existing public/private names; add zero new member flags | 3 |
| `TerminalClock` timing family | `prepare`, `process`, `listen`, `getPeriodMilliseconds` (`jam_TerminalClock.h:40-66`) | Keep exact public API and `jam_TerminalClock.h:59-64` contract | 4 |

### 4. Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "No `==` / `!=` on strings — an Identifier compare is a pointer compare" (`CODING.md:1087`) | Hash `juce::Identifier` by its interned `StringPool` pointer (`id.getCharPointer().getAddress()`) via `Wyhash::hashInt`, matching `std::hash<juce::Identifier>` | `jam_core/utilities/jam_Hash.h:26`; `jam_core/utils/jam_HashMap.h:309` |
| "No non-owning naked pointers as members" (`CODING.md:1078`) | Hold `std::optional<std::reference_wrapper<const std::atomic<int64_t>>>` when initialized after `ModelLayout::addParameters`, never a raw pointer | `eve/Source/EVETerminalProcessor.h:364-368` |
| "Deadline threads: zero allocation, lock, wait, or blocking call" (`CODING.md:1093`) | Atomic `memory_order_relaxed` load for `background`, zero `HashMap` lookups in `EVETerminalProcessor::process()` | `eve/Source/EVETerminalProcessor.cpp:581` |
| "No bail-out guards — positive nesting and assert" (`CODING.md:1068-1069`) | Assert preconditions with `jassert`; use nested positive `if` blocks; use result returns only when the answer is complete | `jam_terminal/buffers/jam_TerminalBuffer.cpp:108-115` |
| "Use C++ alternative tokens" (`CODING.md:1072`) | `not`, `and`, `or` everywhere in C++ code | `jam_terminal/document/jam_TerminalSwap.cpp:151-171` |
| "No shadow state — Model is the one truth" (`MANIFESTO.md:170, 244`) | Derive streaming state from existing `getMode()`, `isViewAtEnd()`, and `liveJob != nullptr`; add zero boolean member flags | `jam_gui/widgets/jam_TextEditor.cpp:1669` |
| "Lean 300/30/3" (`MANIFESTO.md:54-79`, `LANGUAGE.md:36`) | Keep every new or modified function at or under 30 lines and 3 branches | `jam_terminal/document/jam_TerminalSwap.cpp:208-221` |

### 5. Absence

| Need | Query | Result |
|---|---|---|
| Ready history row count getter on `TerminalBuffer` | Direct read of `jam_TerminalBuffer.h:1-600` and `jam/docs/xml/index.xml` | Absent; only `getNumFreeRows()` (`jam_TerminalBuffer.h:212`) and `read (int maxRows)` (`:328`) exist |
| Batch Dekker swap writer on `TerminalSwap` | Direct read of `jam_TerminalSwap.h:1-339` and `jam_TerminalSwap.cpp:1-255` | Absent; `writeNextLine` calls `terminalBuffer.read (1)` one row at a time |
| Cached `background` atomic on `EVETerminalProcessor` | Direct read of `EVETerminalProcessor.h:350-394` and `EVETerminalProcessor.cpp:579-589` | Absent; `process()` calls `state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::background)` every tick |
| Live-tail `HistoryJob` deferral in `TextEditor::addJobs` | Direct read of `jam_TextEditor.cpp:1564-1601` | Absent; `addJobs()` always spawns `HistoryJob` at `firstLine` whenever `closed.getEnd() > getHistoryEnd()` |
| Scroll / mode-switch async wake in `TextEditor` | Direct read of `jam_TextEditor.cpp:1307-1326, 1946-1958` | Absent; neither `visibleAreaChanged()` nor `valueTreePropertyChanged(Id::mode)` calls `triggerAsyncUpdate()` |

*(Note: `carol_ling` MCP returned `no daemon for manifest: /Users/jreng/.gemini/config/plugins/carol`; all absence proofs use Doxygen XML index scans and full header/source reads.)*

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against `MANIFESTO.md` (BLESSED), `NAMES.md`, `~/.carol/CODING.md`, and the locked PLAN decisions (no deviation, no scope drift). `@Auditor` runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: O(1) `Identifier` Hashing, Cached `background`, and Once-Per-Tick `setEvents`
**Scope:** `jam/jam_core/utilities/jam_Hash.h`, `eve/Source/EVETerminalProcessor.h`, `eve/Source/EVETerminalProcessor.cpp`, `jam/jam_terminal/processor/jam_TerminalProcessor.h`, `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`
**Action:**
1. In `jam_Hash.h:62-66` (`jam::Hash<juce::Identifier>::operator()`):
   - Replace `Wyhash::hashBytes (name.toRawUTF8(), name.getNumBytesAsUTF8())` with `return Wyhash::hashInt (reinterpret_cast<std::uintptr_t> (identifier.getCharPointer().getAddress()));`.
   - Matches `std::hash<juce::Identifier>` (`jam_Hash.h:26`), `CODING.md:342` (`uintptr_t` pointer round-trip), and `CODING.md:1087` (pointer identity for `juce::Identifier`).
2. In `EVETerminalProcessor.h` & `EVETerminalProcessor.cpp`:
   - Add member `std::optional<std::reference_wrapper<const std::atomic<int64_t>>> background;` (zero raw pointer member, `CODING.md:1078`).
   - Initialize `background` once in `EVETerminalProcessor` constructor right after `jam::ModelLayout::addParameters(...)` (`EVETerminalProcessor.cpp:25`) using `state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::background)->getRawValue()` with `jassert`.
   - In `EVETerminalProcessor::process()` (`EVETerminalProcessor.cpp:585`): read `background->get().load (std::memory_order_relaxed)` instead of `state.getParameter(...)`.
3. In `jam_TerminalProcessor.h` & `jam_TerminalProcessor.cpp`:
   - Remove the 3 per-token `processor.setEvents (events)` calls inside `screenConstruction` (`jam_TerminalProcessor.cpp:859-879`) that re-fired all 30 event keys on every CSI, OSC, and multi-byte control token.
   - Instead, return a `bool` from `setBytes` / `setTokens` (transient per-call result, `MANIFESTO.md:198, 288` — zero persistent member flag!) indicating whether any CSI, OSC, or multi-byte control token ran during the tick.
   - In `TerminalProcessor::process()` (`:73-93`): when `isDrained and not syncOutput`, call `setEvents (events)` once if `wasSyncOutput or hasModeEvent`; otherwise emit only `Id::cursor` and `Id::isWrapPending`.
   - Keep `TerminalProcessor::process()` at or under 30 lines by decomposing tick event publication into a focused private helper if needed.
**Inventory:** API rows 1–3; Pattern usage rows 1–3; Names rows 1–2; Synthesis rows 1–5, 7; Absence row 3.
**Validation:** COUNSELOR reads every edited file at all modified lines; verifies zero `Wyhash::hashBytes` in `Hash<juce::Identifier>`, zero `state.getParameter` in `EVETerminalProcessor::process()`, zero persistent flags added to `TerminalProcessor`, zero bail-out guards, `not`/`and`/`or` tokens, and zero new comments.

### Step 2: Replace Per-Row `TerminalSwap::writeNextLine` Loop with Batch Dekker Drain
**Scope:** `jam/jam_terminal/buffers/jam_TerminalBuffer.h`, `jam/jam_terminal/buffers/jam_TerminalBuffer.cpp`, `jam/jam_terminal/document/jam_TerminalSwap.h`, `jam/jam_terminal/document/jam_TerminalSwap.cpp`
**Action:**
1. In `jam_TerminalBuffer.h` & `jam_TerminalBuffer.cpp`:
   - Add `int getNumReadyRows() const noexcept;` beside `getNumFreeRows()` (`jam_TerminalBuffer.h:212`, `SPEC.md:540`).
   - Implementation: `const auto numReady { fifo.getNumReady() }; jassert (numReady >= heightRows); return numReady - heightRows;`.
2. In `jam_TerminalSwap.h` & `jam_TerminalSwap.cpp`:
   - Delete `writeNextLine()` (`CAROL.md` Refactor-Rewrite Discipline — delete first, replace with batch drain).
   - Add private helpers (each `<= 30` lines, `<= 3` branches):
     - `int getNumWritableLines (int numReady, juce::int64 held) const`: returns `numReady` when `capacity <= 0`; otherwise computes `juce::jmin (numReady, capacity)` when `held < 0`, or clamps by `held + getNumStoredLines() - endStoredLine` and `held < endStoredLine ? lines.getNumFreeRecords (file, getLineExtent (indexExtent, held), getLineOffset (indexExtent, held), rowBytes) : juce::int64 { capacity }` when `held >= 0`, floored at `0`.
     - `void setStoredLine (juce::int64 targetEndLine, int numLines, const Events& events)`: advances `storedLine` while `storedLine < endStoredLine` and (`targetEndLine - storedLine > getNumStoredLines()` or `lines.getNumFreeRecords (file, getLineExtent (indexExtent, storedLine), getLineOffset (indexExtent, storedLine), rowBytes) < numLines`); raises `firstReadableLine = juce::jmax (firstReadableLine, storedLine)` and emits `Id::firstLine` (`seq_cst`) once if changed.
     - `void writeLines (TerminalBuffer& terminalBuffer, int numLines, const Events& events)`: calls `auto reader { terminalBuffer.read (numLines) };` ONCE and iterates `reader.forEach(...)` writing each row via `writeLine(...)`, advancing `endStoredLine` and `endClosedLine`, and skipping `AnsiDocument::getCellStarts`, `getGridStart`, `setGridStart`, and `previousStarts` work when `outputBlock == TerminalBuffer::noOutputBlock and previousBlock == TerminalBuffer::noOutputBlock`.
   - In `TerminalSwap::writeFromTerminalBuffer()`:
     - Loop while `const auto numReady { terminalBuffer.getNumReadyRows() }; numReady > 0`:
       1. `const auto numWritable { getNumWritableLines (numReady, heldLine.load (std::memory_order_seq_cst)) };`
       2. If `numWritable == 0`, stop (`heldLine` blocks further writes).
       3. Call `setStoredLine (endStoredLine + numWritable, numWritable, events);` (publishes `Id::firstLine` `seq_cst` before hold check).
       4. Execute one Dekker confirmation load: `const auto confirmedHeld { heldLine.load (std::memory_order_seq_cst) };`.
       5. If `confirmedHeld < 0 or confirmedHeld >= storedLine`, call `writeLines (terminalBuffer, numWritable, events);` (otherwise loop once to re-bound `numWritable` against `confirmedHeld` without consuming from `terminalBuffer`).
     - After the loop, call `setLines (endStoredLine, terminalBuffer.getFirstHistoryRow(), events);`, `events.get (Id::endLine, int64_t { endStoredLine });`, and `events.get (Id::closedLine, int64_t { endClosedLine });` once.
**Inventory:** API rows 4–6; Pattern usage rows 4–5; Names rows 3–4; Synthesis rows 4–7; Absence rows 1–2.
**Validation:** COUNSELOR reads `jam_TerminalBuffer.h/.cpp` and `jam_TerminalSwap.h/.cpp`; verifies formal Dekker `seq_cst` store-before-load ordering (`SPEC.md:490-495`, `MANIFESTO.md:271, 438`), zero per-row `read (1)` / `heldLine.load` / `isTaken`, `<= 30` lines per function, zero bail-out guards, and zero comments.

### Step 3: Defer Live-Tail `HistoryJob` Churn in `TextEditor` Without Shadow State
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.cpp`
**Action:**
1. In `TextEditor::hasLive()` (`jam_TextEditor.cpp:1622-1627`):
   - Return `true` when `live.document == nullptr`, or when `getMode() == map::EditorMode::terminal and isViewAtEnd()`, or when `not live.document->getRoot()->contains (Id::closedLine)`, or when `getHistoryEnd() >= *live.document->getRoot()->get<juce::int64> (Id::closedLine)`.
   - Effect: Live terminal output paints immediately at the tail without waiting for `HistoryJob` to load 10,000 lines from `firstLine`.
2. In `TextEditor::addJobs()` (`jam_TextEditor.cpp:1572-1600`):
   - Compute `const auto isStreamingTail { getMode() == map::EditorMode::terminal and isViewAtEnd() and liveJob != nullptr };` from existing Model/View state (zero shadow state, `MANIFESTO.md:170, 244`).
   - Run the `HistoryJob` scheduling block only when `historyJob == nullptr and not isStreamingTail`.
   - Effect: During continuous streaming (`seq`), `liveJob` runs on vblank while `HistoryJob` stays idle (`heldLine == -1`, zero swap stalls, zero wasted history block layout/eviction). As soon as output pauses (`liveJob == nullptr`) or the user scrolls up (`not isViewAtEnd()`) or leaves `terminal` mode (`getMode() != terminal`), `addJobs()` immediately builds history blocks with full scrollback intact.
3. In `TextEditor::visibleAreaChanged()` (`jam_TextEditor.cpp:1946-1958`) and `TextEditor` `Id::mode` listener (`jam_TextEditor.cpp:1307-1326`):
   - Call `triggerAsyncUpdate()` when `not isReadOnly()` so scrolling up or switching to `normal` / `visual` / `reader` triggers `addBlocks()` immediately instead of waiting for the next vblank.
**Inventory:** API row 7; Pattern usage row 6; Names row 5; Synthesis rows 4–7; Absence rows 4–5.
**Validation:** COUNSELOR reads `jam_TextEditor.cpp` at all modified lines; verifies zero shadow state, zero new boolean members, exact `hasLive()` / `addJobs()` / `visibleAreaChanged()` / `Id::mode` behavior, and zero comments.

### Step 4: Restore Exact `TerminalClock::getPeriodMilliseconds()` Law
**Scope:** `jam/jam_terminal/tty/jam_TerminalClock.cpp`
**Action:**
1. In `jam_TerminalClock.cpp:48-51`:
   - Restore `return periodMilliseconds;` (matching `jam_TerminalClock.h:59-64`, `jam@bff685632`, and `SPEC.md:45-57`).
   - Preserves Windows `128 KiB` pipe timer (`jam_AsyncOutputStream_windows.cpp:40-48`) and restores macOS `1024 B` PTY batching (`jam_AsyncOutputStream_mac.cpp:25-29`).
**Inventory:** API row 8; Pattern usage row 7; Names row 6; Synthesis row 4.
**Validation:** COUNSELOR reads `jam_TerminalClock.cpp:45-54` and verifies exact match with `jam_TerminalClock.h:59-64`.

### Step 5: Sequential `scout` Verification and ARCHITECT Release `time seq` Verdict
**Scope:** `eve/benchmark/seq-10m-mac-redesign*.md`
**Action:**
1. Run all three `scout` lanes sequentially in Release (`dropped == 0`, zero overlap, `profile/SKILL.md`):
   - Live `How long` (`5` epochs, `seq 1 10000000`, baseline = `seq-10m-mac-now-report.md`).
   - Live `Where` + `cswitch` (`3` epochs, `intervalMs = 1`, `stackDepth = 64`).
   - Memory Replay `How long` (`5` epochs, `seq 1 1000000`, baseline = `seq-1m-mac-now-replay-report.md`).
2. Compare every zone (`epoch`, `user`, `system`, `tty bytes ready`, `terminal process`, `swap history lines`, `editor history blocks`, `vulkan flush recording`, `vulkan end frame`, `Wyhash::hashBytes`, `WaitableEvent::wait`) against `seq-10m-mac-now-report.md` and `seq-1m-mac-now-replay-report.md`.
3. Present exact `scout` numbers to ARCHITECT for Release `time seq 1 1000000` and `time seq 1 10000000` (plus Windows Cygwin) Verdict. Hold Step 6 until ARCHITECT approves the measured result.
**Inventory:** API row 9.
**Validation:** COUNSELOR verifies `build type == Release`, `dropped == 0`, zero regressions across live and replay lanes, and exact `## sample` / `## cswitch` elimination of all four bottlenecks.

### Step 6: Post-Verdict `@Auditor` Sweep and `@Pathfinder` Doxygen Pass
**Scope:** All modified and new files across `jam` and `eve` from this sprint
**Action:**
1. Only after ARCHITECT's positive Release `time seq` Verdict: run `@Auditor` ONCE over the full sprint file set against `MANIFESTO.md`, `CODING.md`, `NAMES.md`, `LANGUAGE.md`, `SPEC.md`, and this locked PLAN.
2. Filter every Auditor finding through COUNSELOR's Design by Contract Filter; resolve every validated finding in this sprint via `@Engineer` and re-verify.
3. Delegate the dedicated post-audit doxygen pass (`doxygen-writer` skill) to `@Pathfinder` with exact headers, symbols, implementation files, and doxygen regeneration command; validate every block against code.
4. Hold `/log` and cleanup of temporary `eve/benchmark/seq-*` files for ARCHITECT's explicit instruction.
**Inventory:** Full Framework Inventory (Sections 1–5 above).
**Validation:** Zero unresolved validated Auditor findings; every doxygen block verified against code; clean Release build.

## BLESSED Alignment

- **B (Bound):** `EVETerminalProcessor` owns `state` and binds `background` to `state`'s parameter lifetime with `std::optional<std::reference_wrapper<const std::atomic<int64_t>>>` (zero raw pointer). `TerminalSwap` owns its single-file ring and enforces Dekker hold boundaries per batch.
- **L (Lean):** Every new or rewritten helper stays at or under 30 lines and 3 branches. Per-row loop bloat (`writeNextLine`) is deleted, not wrapped.
- **E (Explicit):** Zero magic numbers, positive nesting, `jassert` preconditions, and explicit `std::memory_order_seq_cst` / `memory_order_relaxed` atomics.
- **S (Single Source of Truth):** `jam::Hash<juce::Identifier>` shares pointer identity with `std::hash<juce::Identifier>` and `juce::Identifier::operator==`. `TextEditor` derives live-tail streaming directly from `getMode()`, `isViewAtEnd()`, and `liveJob != nullptr` with zero shadow flags. `TerminalClock::getPeriodMilliseconds()` matches its header contract verbatim.
- **S (Stateless):** `TerminalProcessor` carries zero new persistent state (`hasModeEvent` is a transient per-tick return value). `TextEditor` adds zero state variables.
- **E (Encapsulation):** Unidirectional layer flow (`jam_core` → `jam_terminal` → `jam_gui` → `eve`); zero allocation/copy/hash in transit on deadline threads; `TerminalBuffer::getNumReadyRows()` stays lane API (`SPEC.md:540`).
- **D (Deterministic):** Same cross-platform batch Dekker protocol, O(1) pointer hash, live-tail scheduling, and `TerminalClock` law run identically on macOS ARM64 and Windows x86_64 with zero data races.

## Risks / Open Questions

None. Every design choice traces directly to `MANIFESTO.md`, `CODING.md`, `NAMES.md`, `LANGUAGE.md`, `SPEC.md`, `jam@bff685632` / `eve@d1e2679`, and measured `scout` evidence.
