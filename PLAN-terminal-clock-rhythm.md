# PLAN: TerminalClock — the device clocks at the PTY's traffic rate

**RFC:** none — objective from ARCHITECT's prompt (session 2026-10-29)
**Date:** 2026-10-29
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM (LANGUAGE.md "C++ / JUCE": one object, one translation unit)

## Context

ARCHITECT, verbatim:
- *"measure and verify, to determine HOW TO GET the "dynamic" rhythm"*
- *"how to design by contract, in adherence to our architecture to sync into tty rhythm (must be crossplatform, although OS specific implmentation might or might not be branched), still SSOT logic. ISOMORPHIC ABSTRACTION exactly like an analog audio device/drivers"*
- *"CONTRACT/SPEC/ARCHITECT's rulings principles STANDS as is"*
- *"the full throttle only when it RUNS … audio thread STILL LOCK FREE, processBlock never blocked as PROCESSOR"*
- *"not TerminalRhythm … why no TerminalClock? i just wanted to ensure nothing is hardcoded, nothing is manual state management, fully event driven. design by contract"*
- *"why not using AudioProcessor spec as the SSOT? the number could be setup differently according to OS"*; *"spec has buffer size, block size, sample rate"*
- Sample rate: *"Traffic rate"* — sampleRate is the measured delivery rate in bytes per second, updated from each tick's numSamples; the spec value only sizes the ring.
- *"Windows just IMPLEMENT from static analysis you can read. runtime test could be done later on actual windows machine"*

Cause (measured): `seq` is the critical path (ARCHITECT's current Release: 17.39 s user + 45.88 s system = 1:03.80). Every PTY read and write takes `clock_lock` (`devfs_vnops.c:811`, `:850`, `:866`; `clock.c:635-636`) and `tty_lock` (`tty_dev.c:415`, `:633`). EVE reads once per `seq` write (14.7 bytes per read). The PTY raises a read event on every write (`tty_dev.c:450`, `:476-479`) and has no rhythm.

Harness (`scratchpad/ptyfloor/ptyfloor.c`, zero terminal work): a reader that waits between ticks reaches `seq` 52.5 s and `cat` 4.9 s; EVE's shape today is 66.4 s and 5.5 s. A fixed period starves `cat` (35.2 s). The law with tuned constants worked; this plan replaces the constants with spec values and event payloads.

## The spec mapping (SSOT, per OS)

| Spec quantity | JUCE | EVE value | Owner |
|---|---|---|---|
| Buffer size | `AudioIODevice::getCurrentBufferSizeSamples` (`juce_AudioIODevice.h:282`) | the PTY's capacity: macOS 1024 = kernel output queue (`tty.h:105`, `tty.c:3274`); Windows 65,536 = one overlapped read (`jam_TerminalTTY_windows.cpp:665`) | `VirtualDevice::open`, per OS |
| Block size | `ProcessSpec::maximumBlockSize` (`juce_ProcessContext.h:49-50`) | 65,536, unchanged: the read buffer and the ring (`EVETerminalProcessor.cpp:156`, `:181-186`, `:229-231`) | `EVETerminalProcessor::prepare` |
| numSamples per tick | `audioDeviceIOCallbackWithContext (…, numSamples, …)` may vary per callback (`juce_AudioIODevice.h:101-108`) | the bytes the readiness event reports: macOS `data = t_outq.c_cc` (`tty_ptmx.c:864-866`) via `dispatch_source_get_data`; Windows the bytes of the completed read | the TTY backend (driver) |
| Sample rate | `AudioIODevice::getCurrentSampleRate` (`juce_AudioIODevice.h:288`) | spec value sizes the ring only (unchanged, `:155`); the traffic rate is measured from numSamples per tick | `jam::TerminalClock` |

The ring stays at today's size. It reserves 48 bytes per column per row (`jam_AttributedString.cpp:96-102`, `jam_AttributedChar.h:440`, `jam_Grapheme.h:35-42`); a ring sized by a byte rate cannot be allocated.

## The event-driven clock

Each tick is one readiness event (the driver's interrupt):
1. The event arrives with its payload, numSamples.
2. The driver suspends the event source and clocks the device with numSamples. The tick reads until empty, lock-free (unchanged).
3. `jam::TerminalClock` takes numSamples and the time since the previous tick, sets the traffic rate, and gives the period.
4. The driver waits the period (`juce::WaitableEvent::wait`, `juce_WaitableEvent.h:76`), then re-arms the source. The next event carries the bytes that arrived during the period.
5. When the PTY stays empty, no event comes and nothing runs.

No counter, no tuned constant: the period comes only from the buffer size, numSamples and the elapsed time. Step 1 measures the law.

## Language / Framework Constraints

- CODING.md CRITICAL RULES; Code Hygiene: no comments before the audit; doxygen last.
- One object, one translation unit; the module headers include the submodule files (`jam_terminal.h:45`, `:102-104`; `jam_terminal.cpp:28-32`).
- END safety on every run: kill only `$!`; END pid 38202 unchanged; `window.state` = `a8fa7d46`, `END.cache` = `325b6ec8`; one measurement at a time.

## Validation Gate
Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

## Steps

### Step 0: Write the plan to the project root
**Scope:** `eve/PLAN-terminal-clock-rhythm.md`.
**Action:** COUNSELOR writes this plan to the EVE root.
**Validation:** the file matches this plan.

### Step 1: Measure the constant-free law in the harness
**Scope:** `scratchpad/ptyfloor/` only.
**Action:** COUNSELOR adds the event-driven mode to the harness: suspend, payload numSamples (`dispatch_source_get_data`), read until EAGAIN, period, wait, resume. Two laws, each from the buffer size B, numSamples n and the elapsed time t only:
- **L1:** rate = n ÷ t; period = (B − n) ÷ rate (the time the free space takes to fill; a full buffer gives 0).
- **L2:** as L1, with rate smoothed by the weight n ÷ B (a full tick replaces the rate; a small tick nudges it).
Each runs `seq` and `cat` 2 times, B = 1024.
**Validation:** a law passes when `seq` ≤ 53.0 s and `cat` ≤ 5.5 s in both runs. The law with the lower `seq` + `cat` sum is the law. When no law passes, COUNSELOR stops and reports the numbers to ARCHITECT.

### Step 2: `jam::TerminalClock`
**Scope:** `jam/jam_terminal/tty/jam_TerminalClock.h`, `.cpp` (new); `jam_terminal.h`, `jam_terminal.cpp` (includes).
**Action:** @Engineer writes the object for the Step 1 law: `prepare (int bufferSize)`; `process (int numSamples)` — sets the rate from numSamples and the time since the previous tick (`juce::Time::getMillisecondCounterHiRes()`, `juce_Time.h:348`); `getPeriodMilliseconds() const`. No other public name.
**Validation:** COUNSELOR reads both files: the names above only; no constant beyond what the law needs; no counter; positive nesting; `not`/`and`/`or`; no comment.

### Step 3: macOS driver
**Scope:** `jam_TerminalTTY_posix.h`, `.cpp`.
**Action:** @Engineer: member `jam::TerminalClock clock`; `open()` calls `clock.prepare (bufferSize)` with the device buffer size it receives; the read-source handler: suspend, `const auto numSamples { static_cast<int> (dispatch_source_get_data (source)) }`, `onBytesReady (numSamples)`, `clock.process (numSamples)`, `period.wait (clock.getPeriodMilliseconds())`, resume. `read()` no longer resumes the source.
**Validation:** COUNSELOR reads the lines; one resume site; the cancel path keeps its semaphore.

### Step 4: Windows driver (static analysis)
**Scope:** `jam_TerminalTTY_windows.h`, `.cpp`.
**Action:** @Engineer: the same calls in the thread-pool wait callback (a captureless lambda in `open()`, the posix sibling); numSamples = the bytes of the completed overlapped read.
**Validation:** COUNSELOR reads the lines against Step 3. ARCHITECT tests on Windows later.

### Step 5: The interface and EVE
**Scope:** `jam_TerminalTTY.h` (`onBytesReady` takes `int numSamples`), `EVETerminalProcessor.cpp` (`device.clock (numSamples)`; the device buffer size per OS at `device.open`), `TerminalMemoryTTY.cpp` (its inner `onBytesReady` takes the argument).
**Action:** @Engineer applies the signature change through every caller.
**Validation:** the compiler shows every caller; COUNSELOR reads each.

### Step 6: SPEC amendments
**Scope:** `eve/SPEC.md` lines 37-42, 103-106 (*"Each clock tick is a zero-sample block"* becomes the event's numSamples), 168-172, 395.
**Action:** COUNSELOR edits, each new line with ARCHITECT's quotes above.
**Validation:** no other text changes.

### Step 7: Build, control, measure
**Action:** @Engineer builds `Builds/Profile` with the profiler, COUNSELOR runs `scout benchmark/control.md`, @Engineer rebuilds without the profiler. COUNSELOR runs `seq` 3 times and `cat` 2 times through EVE, before (baseline) and after, and one `fs_usage` window.
**Validation:** ninja 0 warnings; control exits 0; the `seq` median is below 1:03.84 and below END's 59.6 s; `cat` is not slower than its baseline.

### Step 8: Audit, doxygen
**Action:** @Auditor sweeps once; COUNSELOR filters; doxygen pass, 0 warnings. Log and commit on ARCHITECT's word only.

## Amendment A — the measured law and the audit (2026-10-29)

ARCHITECT, verbatim: *"Measure L3"*; *"Measure L4 (gain 2)"*; *"Trace L4's blocks"*; *"why always ask to measure and stop? what stop gives you? to give up and accept we will never make eve to reach its max potential?"*; *"One vblank"*; *"why failed? what failed? why leave it failed?"*.

**Step 1 outcome (harness, `scratchpad/ptyfloor/`):**

| Law | `seq` | `cat` | Cause of failure (trace) |
|---|---|---|---|
| L1, L2 | stalled | 5.51-6.32 s | a full tick lowered the rate; periods grew 97 µs → 10.7 ms |
| L3 (full tick never lowers) | stalled | 5.44 s | a saturated reader observes at most B ÷ t: rate locked near 4 bytes/µs |
| L4 (gain 2 on a full tick) | 53.40, 55.57 s | 5.10, 5.61 s | (B − n) ÷ rate assumes n bytes stay queued; every tick empties the queue |
| L5 (period = B ÷ periods ÷ rate) | 53.26, 54.65 s | 4.86, 5.18 s | 103 s period after 1 s idle: t counted idle time |
| **L6** (L5, rate over the listening window) | **52.93, 52.37 s** | **5.01, 5.21 s** | passes; idle writer 2.43 s |

**The law (L6), implemented in `jam::TerminalClock`:**
- `prepare (bufferSize, maximumPeriodMilliseconds)`; `process (numSamples)` at the start of each readiness tick; `wait()` after the tick. `wait()` replaces the public `getPeriodMilliseconds()` and the driver's own event of Steps 2-3: the law needs the re-arm instant, which only the clock's own wait observes.
- The measured rate = numSamples ÷ the previous listening window (the previous tick start to the previous re-arm). A full read (numSamples ≥ bufferSize) raises the rate only (`max`) and gives a 0 period. Other reads smooth the rate with the weight numSamples ÷ bufferSize.
- Period = min (bufferSize ÷ `periods` ÷ rate, one vblank). `periods` = 2, the JUCE ALSA family (`juce_ALSA_linux.cpp:271`). The vblank bound is ARCHITECT's *"One vblank"*: the period grew without limit in an interactive session (audit #3).
- The driver calls `clock.process` before `onBytesReady`, so the window includes the tick.

**EVE verdict (Step 7, `Builds/Profile` without the profiler):** `seq` 55.74, 56.33, 56.52 s (median 56.33 s; before 1:04.18; END 59.6 s); `seq` system 36.59-36.78 s (before 46.21 s); `cat` 33.04, 33.34 s (baseline 1:38.78, 46.19 s). Bytes per data read: about 174, below the 300 criterion: 48% of data reads were ≤ 16 bytes, the trailing read that picks up the bytes written during the decode.

**Corrections after Step 7 and the audit:**
- The tick ends on a short read. A master read drains the queue until it is empty or the buffer is full (`tty_dev.c:699-705`); a read that returns less than requested has emptied the PTY (`SPEC.md:178`). `TerminalProcessor::process` returns whether the read filled its space; the drain events fire on the drained tick.
- The macOS driver re-arms only when numSamples > 0. At EOF the read event carries the bytes still queued (`tty_ptmx.c:864-866`, `:889-897`), so a drained EOF reports 0 and does not spin (audit #1).
- `TerminalTTY::open` takes `bufferSize` and `maximumPeriodMilliseconds`; the buffer size travelled under the name `maximumBlockSize` (audit #9).
- Windows numSamples = the completed read + `PeekNamedPipe`'s queued bytes; `GetOverlappedResult` is asserted (audit #5, #17).
- Names: `measuredRate` (audit #14); the member `bufferSize` (audit #15). `<poll.h>` removed when unused (audit #18).

**Audit findings rejected, with the refuting read:** #2 (ARCHITECT's ruling, and MANIFESTO E's scope *"This applies to creation and to transit"*); #6 (the history writer's `requestProcess`, `EVETerminalProcessor.cpp:137-141`, ticks and re-arms); #12 (`cancelPendingUpdate` is a `dispatch_sync_f` barrier on the queue the read source targets, `jam_AsyncUpdater.cpp:73-79`; the Windows twin, `PLAN-terminal-clock.md:61`); #13 (ARCHITECT's *"Traffic rate"* ruling); #16 (the clock's own calculation state); #19, #20 (ARCHITECT: *"300-30-3 GUIDE STANDS … but never as reason to decompose"*); #22 (`SPEC.md:178`, read until empty); #21's quote part (ARCHITECT's words this session). #11 (a full ring with a full queue loops at a 0 period until the history writer frees rows) is not measured: it needs a profiler run that counts readiness ticks against reads under `cat`.

**EVE verdict after the corrections** (`Builds/Profile` without the profiler; control passes, 35 crash reports before and after): `seq` 53.95, 54.97, 55.24 s (median 54.97 s; END 59.6 s; before the sprint 1:04.18); `seq` system 35.52-36.11 s; `cat` 33.82, 35.11 s. Bytes per data read during `seq`: 863.5 (24,595 reads per second; 1,168,236 before the sprint). 92% of data reads are near the 1024-byte queue: the decode of a full queue is now the limit, not the read rhythm.

## BLESSED Alignment
- **B:** the clock is owned by the driver that owns the event source; no thread.
- **L:** one object, one job: rate → period.
- **E:** each tick is an event with its payload; no hidden counter.
- **S (SSOT):** the buffer size is the device spec per OS; the law is one object for both platforms.
- **S (Stateless):** the rate is the clock's calculation state, replaced at each tick.
- **E (Encapsulation):** the tick never waits; the driver waits between ticks.
- **D:** Step 1 and Step 7 measure.

## Risks / Open Questions
- **Windows wait precision** is not measured; ARCHITECT tests later.
- **Step 1 can fail.** Then COUNSELOR stops with the numbers.
