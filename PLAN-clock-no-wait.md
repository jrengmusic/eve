# PLAN: clock-no-wait

**RFC:** none — objective from ARCHITECT's prompt
**Date:** 2026-11-16
**Approval:** ARCHITECT `/go` on the proposal in chat (2026-11-16).

## Context

- ARCHITECT: *"nothing should wait"*; *"why wait? is wait contpty constraints?"*; *"on mac, previously the issue is rhythm, our clock full throttle is faster than the mac tty, hence we design a clock that dynamically follow the rhythm of tty. so no time ever lost"*.
- MANIFESTO, deadline threads: *"A thread with a hard deadline, such as the audio thread or a clock thread, does not allocate, lock, wait, or make a blocking call."*
- `TerminalClock::wait()` blocks the device context on a `juce::WaitableEvent` (`jam_TerminalClock.cpp:43-47`). Both readers call it (`jam_TerminalTTY_windows.cpp:671`, `jam_TerminalTTY_posix.cpp:229`).
- Windows runtime evidence: the first msys piece of a key is read alone, and `clock.wait()` holds the rest 18.9 ms (`ready 1 53`, `wait 54` → `woke 54`), so a partial frame crosses a vblank. msys2 splits one write into one `WriteConsoleW` per escape sequence (`console.cc:3956-3961`); OpenConsole writes once per call (`VtIo.cpp:458-469`).
- macOS record: without the clock's batching, EVE read 14.7 bytes per read and `seq` spent 46 s in the kernel; with it, 863.5 bytes per read (`jam/carol/SPRINT-LOG.md:550`).

## Steps

### Step 1: Windows reader has no deferral
**Scope:** JAM `jam_terminal/tty/native/jam_TerminalTTY_windows.{h,cpp}`
**Action:** the reader lambda calls `onBytesReady (numSamples)` only. Delete the `clock.prepare` call in `open`, the `clock.process` and `clock.wait` calls in the lambda, and the `clock` member. The pending overlapped read is the re-arm (SPEC.md:49-50), so each ConPTY `WriteFile` completion ticks at once.
**Validation:** no `TerminalClock` use in the Windows TTY; no wait on the device pool; the read contract (`jam_TerminalTTY.h:116-125`) unchanged.

### Step 2: macOS reader follows the rhythm without blocking
**Scope:** JAM `jam_terminal/tty/native/jam_TerminalTTY_posix.{h,cpp}`, `jam_terminal/tty/jam_TerminalClock.{h,cpp}`
**Action:** after the tick, the handler re-arms the read source after the clock period through a dispatch timer on the same serial context, in place of the blocking `clock.wait()`. `close()` cancels the timer with the read source. `TerminalClock` loses `wait()` and its `WaitableEvent`; the backend reads the period and ends the listening window when the timer fires.
**Validation:** no blocking call on the serial context; the rate law unchanged (window = tick start to period end); `close()` leaves no timer that can fire after the TTY is gone.
**Open:** the clock method that ends the listening window is a new name (NAMES Rule -1); ARCHITECT names it.

### Step 3: SPEC and doxygen
**Scope:** `SPEC.md` (Core Principle 2, lines 45-56), the headers above.
**Action:** the text states the event-driven rhythm with ARCHITECT's quotes only.

### Step 4: Measure
**Action:** ARCHITECT builds; typing on Windows (no twitch); scout `benchmark/typing-windows.md`; macOS `seq 1 100000000` against 53.668 s.
