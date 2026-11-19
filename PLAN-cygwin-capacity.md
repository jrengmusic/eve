# PLAN: Cygwin Capacity — the cygwin lane syncs its clock to the msys2 pty

**RFC:** none — objective from ARCHITECT prompt (`/pay` of DEBT-20261117T223541 and DEBT-20261117T223545)
**Date:** 2026-11-18
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 / JAM (CODING.md placement: one object, one translation unit)

**Plan file at lock:** plan mode permits only this file. The first action after the lock copies this plan, unchanged, to `D:\Poems\dev\eve\PLAN-cygwin-capacity.md`.

## Overview

On Windows, the cygwin lane runs the macOS clock law. But it syncs to 65536, a size that EVE sets, and not to the capacity of the seam beyond EVE's control. This plan gives the msys2 pty pipe capacity to every seam that EVE owns after that pty. It also measures the history writer, which takes 5.38 s of a 6.08 s run.

## Context (measured and read this session)

1. **Crash fixed before this plan (ARCHITECT: *"fix that then measure"*).**
   - EVE crashed at quit with `0xC0000374` (STATUS_HEAP_CORRUPTION).
   - whatdbg showed the order `systemRequestedQuit` (`EVEStandaloneApp.cpp:79`), then `shutdown` (`:88`), then `CloseHandle` in `TerminalSwap::setSegment` (`jam_TerminalSwap.cpp:183`) from `EVETerminalProcessor::handleAsyncUpdate` (`EVETerminalProcessor.cpp:334`).
   - `~AsyncUpdater` waits for the callback (`jam_AsyncUpdater.cpp:64-74`), but it runs after the members are destroyed.
   - Fix applied: `cancelPendingUpdate();` after `device.stop();` in `releaseResources` (`EVETerminalProcessor.cpp:389`). Sibling: `VirtualDevice::stop` (`jam_VirtualDevice.cpp:49-52`).
2. **Measurement** (`benchmark/seq-1m-windows-cygwin-report.md`, Release, 3 epochs, seq 1 to 1000000):
   - `tty bytes ready` 310 calls. 6,888,896 bytes / 310 = about 22 KB for each tick, one tick about every 19.6 ms (`launch prompt` 6.08 s).
   - `swap history lines` 37 calls, 5.38 s. `swap live lines` 2395 calls, 2.4 ms.
   - When the shell exited, the history writer was at line 941,243 (crash trace locals).
3. **Clock law** (`jam_TerminalClock.cpp:19-41`):
   - A tick with `numSamples >= bufferSize` gives period 0 (`:29-31`).
   - Any other tick gives `min (bufferSize / 2 / rate, maximumPeriod)` (`:36`). The cap is one vblank (`EVETerminalProcessor.cpp:546`).
4. **Seams today on the cygwin lane:**
   - EVE device buffer 65536 (`EVETerminalProcessor.cpp:362`). It goes through `openTTY` (`:547`) to `TerminalCygwinTTY::open`.
   - That call sizes the three named pipes (`jam_TerminalCygwinTTY_windows.cpp:14`, `:60-62`) and the readiness clock (`:85`).
   - Helper rings: `static constexpr int bufferSize { 65536 };` (`cygwinPTY/Source/Session.h:103`), used at `Session.cpp:121`, `:133-134`.
   - EVE does not pass a capacity to the helper (`jam_TerminalCygwinTTY_windows.cpp:78`, `Main.cpp:35`).
5. **The seam beyond EVE's control:**
   - The slave-to-master pipe of the msys2 pty is made with `fhandler_pty_common::pipesize` (`msys2-runtime/winsup/cygwin/fhandler/pty.cc:3204-3207`).
   - Its value: `static const unsigned pipesize = 128 * 1024;` (`local_includes/fhandler.h:2404`).
6. **History writer sample** (whatdbg, 7 samples on the writer thread):
   - 4 samples are in `TerminalSwap::setSegment` (`jam_TerminalSwap.cpp:183`): file create, write of the last byte, map, close (`jam_TerminalImageRecord.cpp:8-23`).
   - 3 samples are in `AttributedString::write` (`jam_TerminalBuffer.cpp:402`).
   - The count of segment files for each run is not measured.

## ARCHITECT rulings (verbatim)

1. *"the idea is we follow what the system constraints so no other seam ever wait. mac value because that was mac system constraints. what should we use for windows? isnt this also related to cygwin?"*
2. *"we dont set any hardcoded `value`. the value is what seam beyond our control capped, then we make our clock dynamically follow its `rhythm`."*
3. *"1024 for mac for example, not arbitrary magic value, it's mac os system tty, that we use for our event driven clock to SYNC. the isomorphic abstraction i believe HOLDS here. bytes are bytes no? we just need to integrate"*
4. Scope of the 128 KiB (AskUserQuestion answer): *"Cygwin lane only"*. ConDrv (pwsh, cmd) keeps 65536.
5. *"fix that then measure"* — the crash fix of Context item 1.

## Language / Framework Constraints

- No comment and no doxygen before the audit (CAROL.md Code Hygiene). Step 8 is the doxygen pass.
- Generated files are CAST output. `jam/generated/jam_Terminal.*` changes only in a cast run (the scout toolchain row `../../jam/cast/spell.md`). No agent edits generated files.
- Deadline thread: no step adds a lock, a wait or an allocation to `EVETerminalProcessor::process` or to the readiness tick.
- Builds run only through cast, inside the scout run (profile skill, Builds).

## Framework Inventory

### API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| Device buffer to the TTY | jam_audio_devices | `VirtualDevice::open (…, int bufferSizeSamples)`, `getCurrentBufferSizeSamples()` | `jam_VirtualDevice.cpp:20-26`, `:65` | 1 |
| Lane decision (cygwin runtime present) | eve | `EVETerminalProcessor::getRuntime (const juce::String& program)` (static) | `EVETerminalProcessor.cpp:62-75`, `.h:328` | 1 |
| Shell program of the session | eve | `EVETerminalProcessor::getShell()` | `EVETerminalProcessor.cpp:503-510` | 1 |
| Program token of the shell | JUCE | `juce::StringArray::fromTokens (…, true)`, `String::unquoted()` | `EVETerminalProcessor.cpp:446-447` | 1 |
| TTY open with buffer size | jam_terminal | `TerminalTTY::open (context, bufferSize, maximumPeriodMilliseconds, …)` | `jam_TerminalTTY.h:95-97`, doc `:76-77` | 1 |
| Pipes and clock of the cygwin lane | jam_terminal | `TerminalCygwinTTY::open`, `createNamedPipe (name, access, bufferSize)`, `reader.open (…, bufferSize, …)` | `jam_TerminalCygwinTTY_windows.cpp:11-17`, `:60-62`, `:85` | 1, 2 |
| Helper command line | jam_terminal | `helperArgs` in `TerminalCygwinTTY::open` | `jam_TerminalCygwinTTY_windows.cpp:78-81` | 2 |
| Helper argv parse | cygwinPTY | `main` | `cygwinPTY/Source/Main.cpp:25-49` | 2 |
| Helper rings | jam_terminal | `TerminalFIFO::prepare (capacity)`, `StdioServer::open (context, bufferSize, …)`, `AsyncOutputStream::open (…, bufferSize, …)` | `Session.cpp:121`, `:133-134`; `jam_StdioServer.cpp:29-42` | 2 |
| Profile zone | jam_core | `jam::Profiler::Zone zone { Id::… };` under `#if JAM_USING_PROFILER` | `jam_TerminalSwap.cpp` `setLiveLines` zone (`Id::swapLiveLines`) | 3 |
| Zone identifier | jam cast | `| @id | <name> | \`<name>\` | |` row | `jam/cast/terminal.md:59`, `:95-96` | 3 |
| Measurement | scout | manifest `## run`, `## profiler`, `## toolchain` | `benchmark/seq-1m-windows-cygwin.md` | 5 |

### Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Device opened with an OS value, function-local | `EVETerminalProcessor.cpp:360-365` | `:372` | 1 |
| The device buffer reaches the TTY unchanged | `EVETerminalProcessor.cpp:547` | `:431`, `:463` | 1 |
| Lane branch on the runtime | `EVETerminalProcessor.cpp:81-87` (`runtime.getFullPathName().isNotEmpty()`) | `createTTY` | 1 |
| Shell token parse | `EVETerminalProcessor.cpp:446-447` | `openTTY` | 1 |
| Value on the helper command line, parsed with `juce::String (argv[n]).getLargeIntValue()` | `jam_TerminalCygwinTTY_windows.cpp:78`; `Main.cpp:35` | packed winsize, size handle | 2 |
| Constructor parameter named `new…` for a member of the same name | `jam_TerminalClock.h:40` `prepare (int newBufferSize, …)` | `TerminalClock::prepare` | 2 |
| Zone at the head of a swap function | `jam_TerminalSwap.cpp` `setLiveLines` (`Id::swapLiveLines`), `writeFromTerminalBuffer` (`Id::swapHistoryLines`) | `TerminalSwap` | 3 |

### Names

| Family | Members | Intent | Step |
|---|---|---|---|
| EVE device spec values | `virtualBlockSize` (`:5`), `virtualSampleRate` (`:360`), `virtualBufferSize` (`:362`, `:364`) | Device spec, set per OS (SPEC.md:135) | 1 |
| Seam size of a device or a pipe: `bufferSize` | `jam_TerminalTTY.h:95`, `jam_AsyncOutputStream.h:51`, `jam_StdioServer.h:38`, `Session.h:103` | Largest number of bytes that one read returns (`jam_TerminalTTY.h:76`) | 1, 2 |
| Cygwin lane names | `TerminalCygwinTTY`, `Id::cygwinRuntime`, `getOrCreateCygwinPty` | The cygwin pty lane | 1 |
| Swap zones | `swapHistoryLines`, `swapLiveLines` (`terminal.md:95-96`); reader `historySegmentMapping` (`:59`) | One zone for each swap stage | 3 |

New names (each passes NAMES.md Rule -1, from the families above):
- `cygwinBufferSize`: function-local `static constexpr int` in `EVETerminalProcessor::prepare()`. Family: `virtualBufferSize` and `bufferSize`. Prefix: the cygwin lane names.
- `newBufferSize`: `Session` constructor parameter. Family: `TerminalClock::prepare (int newBufferSize, …)`.
- `Id::swapSegmentMapping`: zone identifier. Family: `swapHistoryLines`, `swapLiveLines`, `historySegmentMapping`.

### Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "No magic values" | An OS seam value is a named function-local constant | `EVETerminalProcessor.cpp:364` |
| "Owner first, narrowest scope" | The device value lives in `prepare()`, its one user | `EVETerminalProcessor.cpp:360-365` |
| SSOT across the two processes | A value that the helper needs travels on its command line from EVE | `jam_TerminalCygwinTTY_windows.cpp:78`, `Main.cpp:35` |
| "Aggregate (brace) initialization" | `static constexpr int … { … };` | `EVETerminalProcessor.cpp:5` |
| "Deadline threads" | No new work in the tick | `EVETerminalProcessor.cpp:551-564` unchanged |

### Absence

| Need | Query | Result |
|---|---|---|
| A TTY API that returns its capacity | doxygen member lists of `jam::TerminalTTY`, `TerminalUnixTTY`, `TerminalWindowsTTY`, `TerminalCygwinTTY` | No such member |
| A run-time query of the msys2 pipe size | Grep `GetNamedPipeInfo` in `D:\Poems\dev\jam` and `D:\Poems\dev\eve` | No matches found |
| A capacity passed to the helper | `jam_TerminalCygwinTTY_windows.cpp:78` | Absent: runtime, size handle, packed winsize, shell only |
| A zone on the swap segment writer | `jam/cast/terminal.md` identifier rows | Only `historySegmentMapping` (reader, `jam_TerminalHistory.cpp:334`) |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final code step, covering the whole sprint — never per step.

## Steps

### Step 0: Crash fix (done before the lock)
**Scope:** `eve/Source/EVETerminalProcessor.cpp:386-392`
**Action:** none. `cancelPendingUpdate();` is at `:389`, after `device.stop();`. The scout run of 16:36 ran 3 epochs with exit 0.
**Validation:** COUNSELOR reads `:386-392` once more. The Auditor covers it.

### Step 1: The cygwin lane opens the device with the msys2 pty capacity
**Scope:** `eve/Source/EVETerminalProcessor.cpp` `prepare()` (`:358-378`) only.
**Action:** replace the Windows branch at `:361-362` with:
```cpp
#if JUCE_WINDOWS
    static constexpr int cygwinBufferSize { 128 * 1024 };
    const auto program { juce::StringArray::fromTokens (getShell(), true)[0].unquoted() };
    const auto virtualBufferSize { getRuntime (program).getFullPathName().isNotEmpty() ? cygwinBufferSize : virtualBlockSize };
#else
```
The macOS branch (`:364`) and `device.open` (`:372`) stay unchanged. `openTTY` (`:547`) already passes `device.getCurrentBufferSizeSamples()` to `TerminalCygwinTTY::open`. That call sizes the pipes and the clock with it (`jam_TerminalCygwinTTY_windows.cpp:60-62`, `:85`). No jam change.
**Inventory:** API rows 1-5; patterns 1-4; name `cygwinBufferSize`; synthesis 1, 2, 4.
**Validation:**
- `128 * 1024` is the form of `fhandler.h:2404`.
- The lane test is the expression of `createTTY` (`:84`).
- The token parse is the form of `:446-447`.
- No member, no new function, no other file changes.
- ConDrv still gets `virtualBlockSize` (ruling 4).

### Step 2: The helper rings take EVE's buffer size from the command line
**Scope:** `jam/jam_terminal/tty/native/jam_TerminalCygwinTTY_windows.cpp:78`; `eve/cygwinPTY/Source/Main.cpp:25-49`; `eve/cygwinPTY/Source/Session.h` (constructor `:22`, member `:103`); `eve/cygwinPTY/Source/Session.cpp:12-17`.
**Action:**
1. `jam_TerminalCygwinTTY_windows.cpp:78`: after the packed winsize, append `+ " " + juce::String (bufferSize)`, before `" " + shell.quoted()`. The command line becomes: runtime path, size pipe handle, packed start size, buffer size, shell, shell arguments.
2. `Main.cpp`: `jassert (argc > 5);`. The `Session` gets `static_cast<int> (juce::String (argv[4]).getIntValue())` after the packed winsize, and the shell arguments from `juce::StringArray (argv + 5)`.
3. `Session.h`: the constructor takes `int newBufferSize` after `std::int64_t packedWinsize`. `static constexpr int bufferSize { 65536 };` becomes `const int bufferSize;`, declared at the same place.
4. `Session.cpp:12-17`: the initializer list sets `bufferSize { newBufferSize }`, in declaration order. `Session.cpp:121`, `:133-134` stay unchanged.
**Inventory:** API rows 6-9; patterns 5, 6; name `newBufferSize`; synthesis 3.
**Validation:**
- One source of the buffer size: EVE's device spec. No literal 65536 is left in the helper.
- Member initialization order matches the declaration order.
- `bufferSize` keeps its name and its uses.
- `Main.cpp` argv indices match the new command line.

### Step 3: Zone on the segment writer
**Scope:** `jam/cast/terminal.md` (identifier rows); `jam/jam_terminal/document/jam_TerminalSwap.cpp` `setSegment` (`:179-185`).
**Action:**
1. `terminal.md`: add `| @id     | swapSegmentMapping        | \`swapSegmentMapping\`        |                                              |` after the `swapLiveLines` row (`:96`), in the column form of `:95-96`.
2. `setSegment`: first statement
```cpp
#if JAM_USING_PROFILER
    const jam::Profiler::Zone zone { Id::swapSegmentMapping };
#endif
```
in the form of the zone in `setLiveLines`.
**Inventory:** API rows 10-11; pattern 7; name `Id::swapSegmentMapping`.
**Validation:**
- The zone compiles only under `JAM_USING_PROFILER`.
- No generated file is edited by hand.
- No other code changes.

### Step 4: SPEC amendment
**Scope:** `eve/SPEC.md:135-137` (principle 10).
**Action:** COUNSELOR amends the device buffer clause with the mark `[AMENDED 2026-11-18]`. Text: the device's buffer size is the capacity of the PTY that the shell writes to. For macOS that is 1024, the kernel output queue. For Windows ConDrv it is one overlapped read (65536). For the Windows cygwin lane it is the msys2 pty pipe, 128 KiB (`fhandler.h:2404`). The helper gets the same value on its command line. Beside it, rulings 1, 2, 3 and 4 verbatim.
**Validation:** each changed line carries a verbatim quote. No line states behaviour that Steps 1-3 do not have.

### Step 5: Measure
**Scope:** new `eve/benchmark/seq-1m-windows-cygwin-capacity.md` (COUNSELOR writes it).
**Action:**
1. The manifest copies `seq-1m-windows-cygwin.md`, with these changes:
   - `samples` = `seq-1m-windows-cygwin-capacity-samples.md`;
   - `output` = `seq-1m-windows-cygwin-capacity-report.md`;
   - `baseline` = `seq-1m-windows-cygwin-report.md`;
   - `## toolchain` rows, in order: `cast | ../../jam/cast/spell.md`, then `cast | ../cygwinPTY/cast/spell.md` (it packs the helper zip into `Source/windows`), then `cast | ../cast/spell.md --standalone-profile-symbols`.
2. @Pathfinder runs `scout D:/Poems/dev/eve/benchmark/seq-1m-windows-cygwin-capacity.md` (profile skill), with a 15-minute watchdog.
3. COUNSELOR reads these values:
   - `tty bytes ready` calls, and the bytes for each tick (6,888,896 / calls);
   - `swap history lines` value and calls;
   - `swap segment mapping` value and calls;
   - `launch prompt`, `epoch`, and the change from the baseline.
**Validation:**
- Exit 0 and Release.
- The `Source/windows` zip has the time of this run.
- `EVE.exe` and `eve-cygwinPTY.exe` are gone after the run.

### Step 6: Auditor
**Scope:** every file of Steps 0-3.
**Action:** @Auditor validates against MANIFESTO.md, NAMES.md, CODING.md and this plan. COUNSELOR reads each cited file:line, names the clause, and resolves each confirmed finding in this sprint.

### Step 7: Doxygen pass
**Scope:** `Session.h` (constructor, `bufferSize`); `jam_TerminalCygwinTTY_windows.h:50-64` (`open`: the helper receives the buffer size); `EVETerminalProcessor.h` (`prepare()`: the cygwin lane value; `releaseResources`: the pending update is cancelled before the members go).
**Action:** @Pathfinder writes the pass on COUNSELOR's exact targets (doxygen-writer skill). COUNSELOR reads each block against the code.

### Step 8: Report
**Action:** COUNSELOR reports the Step 5 values in chat against the baseline:
- the tick count and bytes per tick (DEBT-20261117T223541);
- the writer time, the segment count and the cost per segment (DEBT-20261117T223545).
The history writer fix comes from these values in a second plan in this sprint. ARCHITECT runs the interactive `seq` test.

## Amendment A (2026-11-18): the device follows the TTY

**Cause.** Audit B1: `prepare()` fixed the device size from the configured shell before any TTY opened (`EVETerminalProcessor.cpp:362-364`). A fallback pwsh or cmd then opened with 128 KiB (`:461-474`, `jam_TerminalTTY_windows.cpp:55`).

**ARCHITECT (verbatim):**
- *"understand the contract, windows native os 64kb, always make it aligned. cygwin is 128kb? always following the exact system constraints"*
- *"device follow tty, should not poke layers."*
- `/go` on the design: each TTY backend states its system capacity, and `prepare()` opens the TTY before the device, then opens the device with that capacity.

**Replaces Step 1.** The lane test in `prepare()` (`cygwinBufferSize`, `program`, `getRuntime` at `:362-364`) is deleted. EVE does not work out a capacity.

### Step A1: Each TTY backend states its system capacity (jam, EVE replay TTY)
**Scope:** `jam_TerminalTTY.h`; `jam_TerminalTTY_posix.h/.cpp`; `jam_TerminalTTY_windows.h/.cpp`; `jam_TerminalCygwinTTY_windows.h/.cpp`; `eve/Source/TerminalMemoryTTY.h/.cpp`.
**Action:**
1. `TerminalTTY`: `virtual int getDefaultBufferSize() const noexcept = 0;` in the platform interface group.
2. `TerminalUnixTTY`: returns the macOS kernel output queue, `static constexpr int outputQueueSize { 1024 };`, function-local.
3. `TerminalWindowsTTY`: returns the Windows native read, `static constexpr int overlappedReadSize { 65536 };`, function-local.
4. `TerminalCygwinTTY`: returns the msys2 pty pipe, `static constexpr int ptyPipeSize { 128 * 1024 };` (`fhandler.h:2404`), function-local.
5. `TerminalMemoryTTY`: returns `tty->getDefaultBufferSize()`.
**Names:** `getDefaultBufferSize` joins `juce::AudioIODevice::getDefaultBufferSize` (`juce_AudioIODevice.h:219`, "Returns the default buffer-size to use") and `jam::VirtualDevice::getDefaultBufferSize` (`jam_VirtualDevice.h:40`). The device spec is set from it.
**Validation:** each backend has one `override`. Each value names its system seam. Every subclass is covered: `classjam_1_1TerminalTTY.xml:5-7` and `TerminalMemoryTTY.h:13`.

### Step A2: `prepare()` opens the TTY, then the device with its capacity
**Scope:** `eve/Source/EVETerminalProcessor.h/.cpp`: `prepare()`, `prepare (winsize, historyCapacity)`, `openTTY`.
**Action:**
1. `prepare()`: delete the `#if JUCE_WINDOWS … #endif` block of `virtualBufferSize`. After `device.stop();`, when `getWinsize()` is valid, compute `vblankMilliseconds` (the expression now at `:548`) and call `openTTY (vblankMilliseconds, winsize)`. Then `device.open ({}, {}, virtualSampleRate, tty->getDefaultBufferSize());`.
2. `openTTY`: the `int bufferSize` parameter goes. Both `tty->open` calls (`:433`, `:465`) pass `tty->getDefaultBufferSize()`, so each TTY opens with its own capacity, fallback included. The lambda capture loses `bufferSize`.
3. `prepare (winsize, historyCapacity)`: delete the `vblankMilliseconds` line and the `openTTY` call (`:548-549`). `tty->setWinsize (winsize);` and `flush();` stay.
**Validation:**
- Each start aligns the device to the open TTY (ARCHITECT: *"device follow tty"*).
- EVE does not compute a capacity.
- No new member. The deadline tick is unchanged.

### Step A3: SPEC principles 9 and 10
**Scope:** `eve/SPEC.md:125-131` and the 2026-11-18 block of principle 10.
**Action:** principle 9: the TTY opens in `prepare()` before the device opens, because the device follows the TTY. Principle 10: each TTY backend states its system capacity, and the device opens with the capacity of the TTY that is open. Windows native is 64 KiB. Both carry the ARCHITECT quotes of this amendment.

Steps 2, 3, 5, 6, 7 and 8 stand. Step 5 runs again after A1-A3. The Auditor of Step 6 runs again over Steps A1-A3.

### Audit resolutions (Step 6, two passes)
- **Pass 1, F1:** `firstShellArgument { 5 }` in `main` (`cygwinPTY/Source/Main.cpp:27`). Clause: MANIFESTO S, a hardcoded value that appears more than once.
- **Pass 1, F2:** SPEC wording of the ConDrv size. 65536 is EVE's own value.
- **Pass 1, B1 and B2:** resolved by Amendment A. Pass 2 confirms both are gone.
- **Pass 2, F1 (corrects Step A2 action 2):** `TerminalTTY::open` loses its `int bufferSize` parameter. Each backend sizes its seams with its own `getDefaultBufferSize()`. EVE keeps one call, `device.open (…, tty->getDefaultBufferSize())`. Clause: MANIFESTO S, *"A second copy of any truth is a bug waiting to happen"*. ARCHITECT: *"device follow tty, should not poke layers."* No caller outside EVE (`whatdbg` uses only `TerminalFIFO::writeFromTerminalTTY`).
- **Pass 2, F2:** `openTTY` computes the shell integration `directory` once, at its head. Clause: CODING.md *"Never duplicate code."*
- **Pass 2, F3:** SPEC principle 10 states the placeholder TTY before a valid window size.
- **Pass 2, F4:** the inventory below is current for Amendment A.

### Inventory after Amendment A (replaces the rows of Step 1)
| Need | API | Source |
|---|---|---|
| System capacity of a TTY | `TerminalTTY::getDefaultBufferSize() const noexcept` (pure virtual) | `jam_TerminalTTY.h:115`; overrides `jam_TerminalTTY_posix.cpp:252` (1024), `jam_TerminalTTY_windows.cpp:20` (65536), `jam_TerminalCygwinTTY_windows.cpp:43` (128 KiB), `TerminalMemoryTTY.cpp:44` (forwards) |
| Device follows the TTY | `VirtualDevice::open (…, tty->getDefaultBufferSize())` | `EVETerminalProcessor.cpp:374` |
| TTY open without a size | `TerminalTTY::open (context, maximumPeriodMilliseconds, dims, shell, …)` | `jam_TerminalTTY.h:95` |
| Name family | `juce::AudioIODevice::getDefaultBufferSize`, `jam::VirtualDevice::getDefaultBufferSize` | `juce_AudioIODevice.h:215-219`, `jam_VirtualDevice.h:40` |

Superseded: API rows 1-4, Pattern rows 2-3, the name `cygwinBufferSize`, and Absence row 1 of the Framework Inventory. The BLESSED line "E (Encapsulation): no jam interface change" is superseded: `TerminalTTY` gains `getDefaultBufferSize` and `open` loses `bufferSize`.

## BLESSED Alignment

- **B:** the crash fix stops EVE's own update before its members go. Each seam has one owner of its size: EVE's device spec.
- **L:** one constant and one expression in `prepare()`; one constructor parameter; one zone.
- **E:** the cygwin value is named, and it states the system seam (`fhandler.h:2404`).
- **S (SSOT):** the helper takes EVE's value from its command line. The second literal (`Session.h:103`) goes.
- **S (Stateless):** no new member state in EVE. The helper member is a constructor input.
- **E (Encapsulation):** no jam interface change. `TerminalTTY::open` already receives `bufferSize`.
- **D:** the clock law is unchanged. Only its input becomes the system capacity.
- **Deadline thread:** no change to the tick.

## Risks / Open Questions

None open. The writer fix is not in this plan. Step 8 gives the values that it needs. No read of this session measured the segment count.
