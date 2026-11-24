# PLAN: Restore macOS Driver Rhythm Sync

**RFC:** none — objective from ARCHITECT prompt and Sprint 180 baseline (`comparison.md:55-57`, `jam/carol/SPRINT-LOG.md:554`)
**Date:** 2026-11-24
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.17 / JAM

## Overview

This plan restores macOS PTY rhythm pacing in `jam::AsyncOutputStream` to the `bff685632` mechanism (`juce::WaitableEvent::wait`) while keeping `jam::TerminalClock` unchanged, `EVETerminalProcessor::process()` lock-free with zero wait, and Windows `jam::AsyncOutputStream` (`3.827 s`) untouched. It replaces Step 6's `0.2 ms` `std::this_thread::sleep_for`/GCD split (which dropped `seq` to 50% CPU at `2.812 s` / `32.605 s`), reverts `DISPATCH_TIMER_STRICT` in `jam_AsyncUpdater.cpp`, and aligns `SPEC.md` with the audio-driver seam.

## Language / Framework Constraints

- **One object, one translation unit** (`LANGUAGE.md:34`, `CODING.md:896-937`): edits stay inside `jam_AsyncOutputStream_mac.cpp`, `jam_AsyncUpdater.cpp`, and `SPEC.md`. No new files or headers are added.
- **Audio-driver seam (`MANIFESTO.md:269`, `jam/carol/SPRINT-LOG.md:554`):** `EVETerminalProcessor::process()` (the processor tick) never allocates, locks, waits, or blocks. `jam::AsyncOutputStream` is the OS hardware driver seam and paces the interval between ticks using each OS's native mechanism (`juce::WaitableEvent` on macOS for XNU's 1024-byte `TTYCLSIZE`, `SetThreadpoolTimer` on Windows for Cygwin's 128 KiB pipe).
- **No `ling` AST daemon in this session:** `carol_ling` returned `no daemon for manifest`; every symbol and call site below was verified by full-file reads and doxygen XML.

## Framework Inventory

### 1. API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| Inter-tick microsecond wait with instant shutdown wake | JUCE `juce_core` | `juce::WaitableEvent` (`WaitableEvent (bool manualReset)`, `wait (double)`, `signal()`) | `juce_WaitableEvent.cpp:43-67` | 1 |
| Clock period query and listening window end | `jam_terminal` | `TerminalClock::getPeriodMilliseconds()`, `TerminalClock::listen()` | `jam_TerminalClock.h:56, 66`, `jam_TerminalClock.cpp:43-51` | 1 |
| Readiness re-arm on macOS GCD source | `jam_core` | `AsyncUpdater::Source::arm()` | `jam_AsyncUpdater.h:131`, `jam_AsyncUpdater.cpp:195-199` | 1 |
| GCD timer source creation | `jam_core` | `dispatch_source_create (DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue)` | `jam_AsyncUpdater.cpp:137` | 2 |

### 2. Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Inter-tick `juce::WaitableEvent::wait` in macOS TTY driver | `bff685632:jam_TerminalClock.h:79`, `bff685632:jam_TerminalClock.cpp:43-47`, `bff685632:jam_TerminalTTY_posix.cpp:225` | `jam_AsyncOutputStream_mac.cpp:23-47` | 1 |
| Reverse-declaration member destruction to wake/cancel before source teardown | `jam_AsyncOutputStream_windows.cpp:136-156` (`source` declared last) | `jam_AsyncOutputStream_mac.cpp:50-54` (`~Native()` signals `period` before `~source()`) | 1 |
| Skip wait when `periodMilliseconds <= 0.0` | `jam_AsyncOutputStream_windows.cpp:40` | `jam_AsyncOutputStream_mac.cpp:25-26` | 1 |

### 3. Names

| Family | Members | Intent | Step |
|---|---|---|---|
| `TerminalClock` wait event | `period` (`bff685632:jam_TerminalClock.h:79`) | Manual-reset `juce::WaitableEvent` member in `AsyncOutputStream::Native` (`#if JUCE_MAC`) | 1 |
| `TerminalClock` period scalar | `periodMilliseconds` (`jam_TerminalClock.h:85`, `jam_TerminalClock.cpp:23`) | Local `const auto` holding `clock.getPeriodMilliseconds()` | 1 |

### 4. Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "Aggregate (brace) initialization is ALWAYS preferred" | `juce::WaitableEvent period { true };`, `const auto periodMilliseconds { clock.getPeriodMilliseconds() };` | `jam_AsyncOutputStream_mac.cpp:26` |
| "No bail-out guards — positive nesting" | `if (numSamples > 0) { ... }` | `jam_AsyncOutputStream_mac.cpp:23` |
| "Use C++ alternative tokens: `not`, `and`, `or`" | `not`, `and`, `or` outside preprocessor | `jam_AsyncUpdater.cpp:167` |
| "Use the framework API fully" | `juce::WaitableEvent::wait (double)` instead of `std::this_thread::sleep_for` | `bff685632:jam_TerminalClock.cpp:45` |

### 5. Absence

| Need | Query | Result |
|---|---|---|
| `AsyncOutputStream` macOS callers of `source.onTimer` | Direct read of `jam_AsyncOutputStream_mac.cpp:1-72` and `jam_AsyncUpdater.cpp:126-215` | None outside `jam_AsyncOutputStream_mac.cpp:43-47`. `timerSource` never fires unless `startTimer()` is called. |
| Windows dependency on `jam_AsyncOutputStream_mac.cpp` | Direct read of `jam_AsyncOutputStream_mac.cpp:1` (`#if JUCE_MAC`) and `jam_AsyncOutputStream_windows.cpp:1` (`#if JUCE_WINDOWS`) | None. Windows compiles `jam_AsyncOutputStream_windows.cpp` only. |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step and runtime proof,
covering the whole sprint — never per step. Doxygen updates run in the dedicated post-audit
Pathfinder pass only (`CAROL.md` Code Hygiene).

## Steps

### Step 1: Restore `juce::WaitableEvent` inter-tick pacing in `jam_AsyncOutputStream_mac.cpp`
**Scope:** `jam/jam_terminal/tty/native/jam_AsyncOutputStream_mac.cpp`
**Action:**
1. In `AsyncOutputStream::Native`, replace the `if (numSamples > 0)` block (`:23-40`) with:
   ```cpp
   if (numSamples > 0)
   {
       if (const auto periodMilliseconds { clock.getPeriodMilliseconds() }; periodMilliseconds > 0.0)
           period.wait (periodMilliseconds);

       clock.listen();
       source.arm();
   }
   ```
2. Remove the unused `source.onTimer = [this] { ... };` assignment (`:43-47`).
3. Add a destructor `~Native() { period.signal(); }` and a member `juce::WaitableEvent period { true };` declared immediately before `AsyncUpdater::Source source;` so `~Native()` wakes any sleeping wait before `~source()` cancels and joins the GCD read source.
4. Leave `jam_AsyncOutputStream_windows.cpp` and `jam_TerminalClock.{h,cpp}` untouched.
5. No comments and no doxygen edits in this step.
**Inventory:** API rows 1–3 (`juce::WaitableEvent`, `TerminalClock::getPeriodMilliseconds`, `TerminalClock::listen`, `AsyncUpdater::Source::arm`); Pattern rows 1–3; Names rows 1–2; Synthesis rows 1–4; Absence rows 1–2.
**Validation:** COUNSELOR reads `jam_AsyncOutputStream_mac.cpp:1-72`, verifies member order (`period` before `source`), `~Native()` signals `period`, zero `sleep_for`, zero `startTimer`, zero `onTimer`, zero comments, and formatting matches `CODING.md`.

### Step 2: Revert `DISPATCH_TIMER_STRICT` in `jam_AsyncUpdater.cpp`
**Scope:** `jam/jam_core/concurrency/jam_AsyncUpdater.cpp`
**Action:**
1. At `jam_AsyncUpdater.cpp:137`, change:
   ```cpp
   timerSource.reset (dispatch_source_create (DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, queue));
   ```
   back to:
   ```cpp
   timerSource.reset (dispatch_source_create (DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue));
   ```
2. Leave all other lines in `jam_AsyncUpdater.cpp` untouched.
3. No comments and no doxygen edits in this step.
**Inventory:** API row 4 (`dispatch_source_create`).
**Validation:** COUNSELOR reads `jam_AsyncUpdater.cpp:130-150`, verifies only the mask argument changed back to `0` and no other line changed.

### Step 3: Align `SPEC.md` with the audio-driver seam
**Scope:** `eve/SPEC.md`
**Action:**
1. In `eve/SPEC.md:45-63`, update the Core Principle 2 bullet so it states the exact seam:
   - The processor tick (`EVETerminalProcessor::process()`) never waits, locks, or allocates.
   - `jam::AsyncOutputStream` is the OS driver seam and paces the inter-tick period of `jam::TerminalClock` with each OS's native mechanism: `juce::WaitableEvent` on macOS (for XNU's 1024-byte `TTYCLSIZE`), and a one-shot threadpool timer (`SetThreadpoolTimer`) on Windows (for Cygwin's 128 KiB pipe), re-arming directly when the period is `0`.
   - Keep all verbatim ARCHITECT quotes intact.
**Inventory:** `SPEC.md:45-63`, `jam/carol/SPRINT-LOG.md:554`, `MANIFESTO.md:269`.
**Validation:** COUNSELOR reads `eve/SPEC.md:40-70`, confirms ASD-STE100 prose, zero contradiction with `MANIFESTO.md:269` or code, and intact ARCHITECT quotes.

### Step 4: Measure, audit, doxygen, and log
**Scope:** `eve/benchmark/seq-10m-mac-after.md`, full sprint file set (`jam_TextEditor.cpp`, `jam_AsyncOutputStream_mac.cpp`, `jam_AsyncOutputStream_windows.cpp`, `jam_AsyncUpdater.cpp`, `EVETerminalProcessor.cpp`, `SPEC.md`)
**Action:**
1. Delegate `scout` run (`eve/benchmark/seq-10m-mac-after.md`) to `@Pathfinder` and report `epoch`, `user`, `system`, and `tty bytes ready` against `seq-10m-mac-report.md` (`22.51 s`) and `2026-10-29`.
2. ARCHITECT builds Release and runs interactive `time seq 1 1000000` and `time seq 1 10000000` against the `0.543 s` / `5.474 s` baseline (`comparison.md:56-57`).
3. Once ARCHITECT confirms runtime proof, invoke `@Auditor` ONCE over the entire sprint against CONTRACT + this plan, filter every finding through COUNSELOR's Design by Contract Filter, and resolve every validated finding.
4. Delegate the dedicated post-audit doxygen pass to `@Pathfinder` (`jam_TextEditor.h`, `jam_AsyncOutputStream.h`, `jam_TerminalClock.h`, `EVETerminalProcessor.h`) and validate every block against code.
5. Wait for ARCHITECT's `/log` before logging the sprint and writing commit messages.
**Inventory:** `profile` skill (`scout --mcp`), `doxygen-writer` skill.
**Validation:** `scout` shows `dropped == 0`; ARCHITECT confirms Release `time seq` verdict; zero unresolved Auditor findings; every doxygen block verified against code.

## BLESSED Alignment

- **B (Bound):** `AsyncOutputStream::Native` owns `period` (`juce::WaitableEvent`) and `source` (`AsyncUpdater::Source`). `~Native()` signals `period` before `~source()` cancels and joins, so teardown is deterministic with zero dangling wake.
- **L (Lean):** Removes the `timerThresholdMilliseconds` branch and unused `source.onTimer` lambda from `jam_AsyncOutputStream_mac.cpp`; net line reduction.
- **E (Explicit):** Positive nesting `if (const auto periodMilliseconds { clock.getPeriodMilliseconds() }; periodMilliseconds > 0.0)` makes the zero-period fast path explicit.
- **S (SSOT):** `jam::TerminalClock` remains the single rate/period law shared by macOS and Windows.
- **S (Stateless):** Zero new state flags; `period` is a synchronization primitive owned by the macOS driver only.
- **E (Encapsulation):** Each OS driver encapsulates its native pacing inside its `#if JUCE_MAC` / `#if JUCE_WINDOWS` translation unit; `EVETerminalProcessor` and `TerminalClock` stay ignorant of OS timing primitives.
- **D (Deterministic):** Restores macOS XNU PTY cadence (`0.543 s` / `5.474 s` / `53.668 s` at 98–99% CPU) without touching Windows Cygwin (`3.827 s`).

## Risks / Open Questions

- None. Closed before approval.
