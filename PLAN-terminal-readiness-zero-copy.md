# PLAN: terminal-readiness-zero-copy

**RFC:** none. The objective comes from ARCHITECT's prompt and rulings this session.
**Date:** 2026-11-16
**BLESSED Compliance:** verified against the reads cited below.
**Language Constraints:** C++17 / JUCE 8.0.17 / JAM (CODING.md, MANIFESTO.md, NAMES.md).

## Overview

ARCHITECT's objective, verbatim: *"identify code smells. fix the violations in adherence to @SPEC.md FAITHFULLY, implement the isomorphic abstraction that works on windows implementation from those 2 working example, and how are we doing it on mac. LOCK FREE"*.

ARCHITECT's rulings this session, verbatim:

| # | Ruling | Decides |
|---|---|---|
| R1 | *"One shared JAM type"* (option: AsyncOutputStream becomes one readiness + timer + clock type with mac and Windows halves; both TTYs and the displays move onto it) | Steps 2-4 |
| R2 | *"move AsyncOutputStream to Terminal"* | Step 2 |
| R3 | *"0 rules. non-negotiable. NO COPY, NO ALLOC, NO TEMP CONTAINERS, NO SHADOW STATE, only exception copy is cross thread. NON-NEGOTIABLE. never ask for contract violations"* | all steps |
| R4 | *"Only cross-thread"* (MANIFESTO permitted copies 3 and 4 are not permitted in this sprint) | Steps 1, 5, 7 |
| R5 | *"Direct write, stage overflow"* | Step 6 |
| R6 | *"what is the most BLESSED COMPLIANCE"*, answered: replay capacity from the manifest (MANIFESTO :269, :250-256, :339) | Step 8 |
| R7 | *"TerminalFIFO is our ring buffer ... new pattern reinvention is not allowed"* | Steps 1, 5, 7, 8 |

These earlier rulings also bind: *"nothing should wait"*; *"EVENT DRIVEN"*; *"this is lock free audio architecture discpline, end to end"*; *"threading, locking, etc we only delegate what native tty constraints"*; *"our clock rhythm fully event driven sync to native tty"*; *"rendering is one SSOT always, divergence only at native OS"*; *"no fucking new semantics. no new pattern ... no hack. no workaround. no patch. no fixing symptoms. no handroll anything ... DESIGN BY CONTRACT"*; *"why no TerminalClock? i just wanted to ensure nothing is hardcoded, nothing is manual state management, fully event driven"* (SPEC.md:59-60).

### Code smells and violations found (read this session)

1. **The Windows TTY has no clock.** PLAN-clock-no-wait Step 1 deleted it (`PLAN-clock-no-wait.md:19`). macOS ticks in the PTY rhythm with a one-shot timer (`jam_TerminalTTY_posix.cpp:208-238`). Windows takes one tick per ConPTY write: the `seq 1 1000000` run gave about 7 bytes per tick and 997,637 `device clock` calls (`benchmark/seq-windows-report.md`). That is the macOS signature without the clock: 14.7 bytes per read (`jam/carol/SPRINT-LOG.md:550`).
2. **Readiness is written four times.**
   - `jam_TerminalTTY_posix.cpp:204-243`
   - `jam_TerminalDisplay_posix.cpp:15-57`
   - `jam_AsyncOutputStream_windows.cpp:23-31`
   - `jam_TerminalDisplay_windows.cpp:36-48`
   - The macOS `AsyncOutputStream` half has no user.
3. **Windows read: second copy and shadow state.** `AsyncOutputStream` reads into its own `buffer` (`jam_AsyncOutputStream_windows.cpp:81`, `:114`). `read()` then copies with `memcpy` (`jam_TerminalTTY_windows.cpp:805`) and keeps `unreadBytes` (`jam_TerminalTTY_windows.h:323`).
4. **Windows write: second copy.** It copies into `writeBuffer` (`jam_TerminalTTY_windows.cpp:829`).
5. **A manual pseudo-console close.** `processWait` closes the pseudo console by hand (`jam_TerminalTTY_windows.cpp:695-706`). The reference releases it with `ConptyReleasePseudoConsole` (`terminal/src/cascadia/TerminalConnection/ConptyConnection.cpp:451`, `winconpty.cpp:558-577`).
6. **The cut-token `memmove`.** `jam_TerminalProcessor.cpp:169`. It is permitted copy 3, which R4 removes.
7. **The reply `memmove` and staging.** `jam_TerminalProcessor.cpp:115-119`, with `replyBytes`, `reply` and `pendingReplyBytes` (`jam_TerminalProcessor.h:172-175`).
8. **Input lane: a second copy and a staging container.** `pendingInput.append` (`EVETerminalProcessor.cpp:270`).
9. **The grid-event lane changes the data shape in transit.** int64 becomes a `juce::String`, is written into the FIFO, and is then parsed back (`EVETerminalProcessor.cpp:158-166`, `:227-238`).
10. **The command lane copies.** `applyNames` copies and allocates `juce::String` across the ring wrap (`EVETerminalProcessor.cpp:29-55`).
11. **The image record header is copied.** It goes into a stack block through a `MemoryOutputStream` (`jam_TerminalImageRecord.cpp:62-71`).
12. **The Windows display copies twice.** Console records go into `inputRecords`, then a UTF-16-to-UTF-8 conversion runs (`jam_TerminalDisplay_windows.cpp:76-104`). This is permitted copy 4, which R4 removes.
13. **The replay record.**
    - It copies into `chunk` and then into `fixture` (`TerminalMemoryTTY.cpp:76-83`). Growth reallocates the whole record.
    - It has a `#if JUCE_MAC` branch outside the native layer (`TerminalMemoryTTY.cpp:37-39`).
14. **Doxygen and SPEC text.** They state my removed design as fact:
    - `jam_TerminalClock.h:8`, `:14`
    - `jam_TerminalTTY.h:9`, `:77-79`
    - `jam_TerminalDisplay.h:18`, `:32-33`
    - `jam_TerminalTTY_windows.h:126-131`, `:171`
    - `jam_TerminalDisplay_windows.h:36`
    - SPEC.md:46-48, :53-55

## Constraints

- **Native divergence only.** Every platform difference stays in the native halves. EVE code has no `#if JUCE_MAC` branch for readiness ("divergence only at native OS").
- **One serial context.** Readiness, the timer and the clock run on the device context. That context is a thread pool of one thread on Windows (`jam_AsyncUpdater.cpp:26-28`) and a serial queue on macOS. SPEC CP10 (`SPEC.md:129-130`).
- **Deadline context.** Readiness, the timer and `read()` run on it, so none of them may wait, lock or allocate (MANIFESTO :269).
- **Copies:** only the OS read or write into the owner's buffer, and the cross-thread FIFO write (R3, R4).
- **Builds only through `cast`.**
  - Pathfinder runs one Windows build through the scout manifest's `## toolchain` row.
  - macOS halves are not compiled on this machine. ARCHITECT builds macOS.
- **Engineer naming.** Engineer introduces no name outside this plan (CODING.md, "No identifier latitude"). The new names, each from an established family:
  - **Windows `AsyncOutputStream::Native`:** `timer` and `isCancelled`.
  - **Windows TTY:** `readOverlapped`, `pendingWriteAddress`, and `ConPtyFuncs::release` / `ReleaseFunc` / `releasePseudoConsoleSymbol`.
  - **`TerminalFIFO`:** `writeFromTerminalTTY`, from the family `TerminalSwap::writeFromTerminalBuffer` (`jam_TerminalSwap.h:95`), and `struct Native` (the family is `AsyncOutputStream::Native`).
  - **Windows display:** `savedInputCodePage`, from the family `savedInputMode` (`jam_TerminalDisplay_windows.h:59`).
  - **EVE:** `gridEventTypes`, and `Id::recordCapacity` from the family `Id::historyCapacity` (`EVETerminalProcessor.cpp:342`).
- **Implementation rules.** Design by Contract per CODING.md CRITICAL RULES. No comment and no doxygen before the audit (CAROL.md Code Hygiene).

## Dependency & API Inventory

**Windows readiness (Learn, quoted in the Librarian reports this session)**
- `SetThreadpoolWait`: *"You must re-register the event with the wait object before signaling it each time"*.
- `SetThreadpoolTimer`, one-shot: *"If this parameter is zero, the timer is signaled once"*.
- `SetThreadpoolTimer`, due time: *"If negative, it indicates the amount of time to wait relative to the current time"* (100 ns units, per the Learn example `-(1 * 10 * 1000 * 1000)` = 1 s).
- `SetThreadpoolTimer`, zero due time: *"the timer expires immediately"*.
- `CloseThreadpoolTimer`, cancel sequence: `SetThreadpoolTimer (NULL)` → `WaitForThreadpoolTimerCallbacks (TRUE)` → close.
- One worker thread: *"A single worker thread can execute multiple callback functions, one at a time"*.
- OVERLAPPED: *"you should use a manual reset event"*.

**Zero-byte overlapped `ReadFile` as the readiness signal**
- libuv production use: `pipe.c:1633-1645`, and the comment at `pipe.c:2354-2355`: *"The zero-read completed without error, indicating there is data available in the kernel buffer"*.
- ReactOS npfs: it pends on an empty queue and copies nothing (`read.c:132-140`, `readsup.c:56`).
- No Microsoft page states this. Step 10 measures it.

**Bounded read**
- *"A read operation is completed successfully when all available bytes in the pipe are read or when the specified number of bytes is read"* (Learn, Named Pipe Type, Read, and Wait Modes).
- `read()` asks for at most the bytes `PeekNamedPipe` reports.

**`ReleasePseudoConsole`**
- Exported by the sideloaded `conpty.dll` as `ReleasePseudoConsole = ConptyReleasePseudoConsole` (`terminal/src/winconpty/dll/winconpty.def`).
- Exported by kernel32 from build 26100 (Learn). It is present on this host (10.0.26300).
- `winconpty.cpp:558-561`: *"allowing conhost to shut down automatically once the last client has disconnected ... a ReadFile() on the output pipe will return ERROR_BROKEN_PIPE"*.
- Learn: *"You must still call ClosePseudoConsole"*.
- EVE always extracts `conpty.dll` (`EVETerminalProcessor.cpp:111-113`).

**Mirrored storage**
- Learn VirtualAlloc2 "Scenario 1": *"Create a circular buffer by mapping two adjacent views of the same shared memory section"*. The size is a multiple of `dwAllocationGranularity`.
- SDK guard: `#if (NTDDI_VERSION >= NTDDI_WIN10_RS4)` (`Windows Kits/10/Include/10.0.26100.0/um/memoryapi.h:986-1062`, with `VirtualAlloc2` at `:996` and `MapViewOfFile3` at `:1010`). ConPTY already needs 1809 (`jam_TerminalTTY_windows.h:5-6`).
- macOS: TPCircularBuffer, `vm_allocate (2 × length)` → `vm_deallocate` of the second half → `vm_remap` of the first half onto it, with `round_page` lengths.
- One physical copy of the bytes, mapped at two adjacent addresses.

**Console VT input**
- Learn SetConsoleMode, `ENABLE_VIRTUAL_TERMINAL_INPUT`: input becomes VT sequences *"that can be retrieved ... through ReadFile or ReadConsole"*.
- Learn SetConsoleMode, `ENABLE_LINE_INPUT` off: *"the functions return when one or more characters are available"*.
- Learn GetNumberOfConsoleInputEvents: *"When the input buffer is not empty, the state of a console input buffer handle is signaled"*.
- Terminal source:
  - VT key input is stored as one `KEY_EVENT` record per UTF-16 unit of VT text (`host/inputBuffer.cpp:706-714`, `:814`).
  - A queue that holds only records that are not keys makes `ReadFile` wait (`host/stream.cpp:138-241`).

**JUCE**
- No JUCE type gives a non-blocking readiness for a pipe, or a one-shot timer on a `PTP_POOL` (Pathfinder survey, Section 10: no `CreateThreadpool*` call in JUCE modules).
- `juce::Identifier (String::CharPointerType nameStart, String::CharPointerType nameEnd)` exists (`juce_Identifier.h:72`).

**JAM**
- `TerminalFIFO` is the ring (`jam_TerminalFIFO.h:9`). Its users of `applyPendingData` are `EVETerminalProcessor.cpp:33` only.
- `writePendingData (TerminalTTY&)` consumes only the accepted bytes (`jam_TerminalFIFO.cpp:69-87`).
- Profiler table rows are `key | type | value` (`benchmark/replay.md:17-25`). The `int` type exists (`:6`). Identifier rows are in `cast/identifiers.md:20` (`| @id | args | \`args\` | |`).

## Validation Gate

COUNSELOR validates each step before the next one. The checks are:
- MANIFESTO.md (BLESSED)
- NAMES.md
- ~/.carol/CODING.md
- this PLAN, with no deviation and no scope drift.

@Auditor runs once, after the final code step, and covers the whole sprint. It never runs per step.

## Steps

### Step 0: Write the plan to the EVE root
**Scope:** `D:\Poems\dev\eve\PLAN-terminal-readiness-zero-copy.md`
**Action:** COUNSELOR copies this approved plan there.
**Validation:** the file content equals the approved plan.

### Step 1: TerminalFIFO mirrored storage and the TTY read member (R4, R7)
**Scope:** JAM
- `jam_terminal/processor/jam_TerminalFIFO.{h,cpp}`
- new `jam_terminal/processor/native/jam_TerminalFIFO_windows.cpp` and `jam_TerminalFIFO_mac.cpp`
- the include list of `jam_terminal.cpp` (next to `:26`)

**Action:**
1. Delete the `jam::Array<char> buffer` member (`.h:117`) and its `allocate` (`.cpp:11`).
2. Add the storage.
   - A private `struct Native` with a `std::unique_ptr<Native> native` member holds one memory section mapped twice, back to back.
   - Windows: `CreateFileMapping` + `VirtualAlloc2 (MEM_RESERVE | MEM_RESERVE_PLACEHOLDER)` + `VirtualFree (MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)` + two `MapViewOfFile3 (MEM_REPLACE_PLACEHOLDER)`, as Learn Scenario 1 does.
   - macOS: `vm_allocate` + `vm_deallocate` + `vm_remap`, as TPCircularBuffer does.
3. `prepare` rounds `capacity + reservedSlot` up to the allocation granularity (Windows) or the page size (macOS). It gives that total to `fifo.setTotalSize`.
4. Change the copy paths.
   - `write` makes one `memcpy` into the mapping at the write index plus the staged bytes. That is the permitted cross-thread copy.
   - `writePendingData (TerminalTTY&)` makes one `tty.write` with all ready bytes.
   - `writePendingData (juce::OutputStream&, int)` makes one `output.write`.
5. `applyPendingData` makes one callback call with a `char*` to all ready bytes. It consumes the count that the callback returns.
6. Add `int writeFromTerminalTTY (TerminalTTY& tty)`.
   - It calls `tty.read` into the free space at the write index.
   - It publishes the bytes read with `finishedWrite`.
   - It returns the result of `tty.read`, so `-1` passes through.

**Validation:**
- One mapping, and no `memmove`.
- A view of all ready bytes is contiguous.
- The public members of `.h:18-111` keep their signatures. The changes: the `applyPendingData` callback returns `int` and takes `char*`, and `writeFromTerminalTTY` is added.

### Step 2: AsyncOutputStream moves to jam_terminal as the readiness type (R1, R2)
**Scope:** JAM
- Delete `jam_core/native/jam_AsyncOutputStream.h`, `jam_AsyncOutputStream_windows.cpp` and `jam_AsyncOutputStream_mac.cpp`, and their include lines `jam_core.h:124` and `jam_core.cpp:10-11`.
- New `jam_terminal/tty/jam_AsyncOutputStream.h`, `jam_terminal/tty/native/jam_AsyncOutputStream_windows.cpp` and `jam_AsyncOutputStream_mac.cpp`.
- Their include lines go in `jam_terminal.h` after `tty/jam_TerminalClock.h` (`:108`) and in `jam_terminal.cpp` after `tty/jam_TerminalClock.cpp` (`:31`). The guard stays `#if JUCE_WINDOWS or JUCE_MAC` (`jam_AsyncOutputStream.h:12`).

**Action:**
1. **Interface.**
   - It keeps `Handle` and `onBytesReady` (`jam_AsyncOutputStream.h:27-35`, `:83`).
   - New open signature: `bool open (Handle stream, jam::AsyncUpdater::Context context, int bufferSize, double maximumPeriodMilliseconds)`.
   - It keeps `void close()`.
   - Deleted: `read()`, the internal buffer, and handle ownership. The macOS `::close (stream)` (`_mac.cpp:43`) and the Windows `CloseHandle` deleter (`_windows.cpp:9`) go.
2. **macOS `Native`.** It holds the members moved from `TerminalUnixTTY`: `readSource`, `timerSource`, `isReadSourceSuspended`, `cancelled` and `clock` (`jam_TerminalTTY_posix.h:189-202`).
   - The read handler, the timer handler and the cancel handler are moved verbatim from `jam_TerminalTTY_posix.cpp:208-242`, with `tty.` renamed to `native.`. The profiler zone `Id::ttyBytesReady` (`:218-220`) moves with them.
   - `open` calls `clock.prepare (bufferSize, maximumPeriodMilliseconds)`.
   - `close` is moved from `:298-309`.
3. **Windows `Native`.** It holds `readEvent` (manual reset), `readOverlapped`, `readWait` (the existing names, `_windows.cpp:10-117`), `timer` (`PTP_TIMER`), `isCancelled` (`std::atomic<bool>`) and `clock`.
   - `open`:
     - It creates `readWait` and `timer` on `context` through a `TP_CALLBACK_ENVIRON` with `SetThreadpoolCallbackPool`, as in `jam_TerminalTTY_windows.cpp:692-694`.
     - It calls `clock.prepare`.
     - It arms readiness.
   - **Two handle kinds.** `GetFileType (stream)` selects one of two entries.
     - **Pipe** (`FILE_TYPE_PIPE`). Arm: `ResetEvent (readEvent)`, then a zero-byte `ReadFile (stream, &zeroRead, 0, nullptr, &readOverlapped)`, then `SetThreadpoolWait (readWait, readEvent)`. On an immediate failure, `SetEvent (readEvent)` runs first, so that the end of the stream gives a tick. Count: the `lpTotalBytesAvail` of `PeekNamedPipe`, or 0 when it fails.
     - **Console** (`FILE_TYPE_CHAR`). Arm: `SetThreadpoolWait (readWait, stream)`. Count: `GetNumberOfConsoleInputEvents`.
   - **Readiness callback.** It runs `GetOverlappedResult (FALSE)` (pipe only), then gets the count. Then it calls `clock.process (numSamples)` and `onBytesReady (numSamples)`. When `numSamples > 0` and not `isCancelled`, it calls `SetThreadpoolTimer (timer, relative due = period in 100 ns units, 0, 0)`. This mirrors `jam_TerminalTTY_posix.cpp:214-226`.
   - **Timer callback.** `clock.listen()`, then arm. This mirrors `:232-237`.
   - **`close`**, in order:
     1. `isCancelled.store (true)`.
     2. `WaitForThreadpoolWaitCallbacks (readWait, FALSE)`.
     3. `SetThreadpoolTimer (timer, nullptr, 0, 0)` and `WaitForThreadpoolTimerCallbacks (timer, TRUE)`.
     4. `CancelIoEx (stream, &readOverlapped)` (pipe), then `SetThreadpoolWait (readWait, nullptr, nullptr)` and `WaitForThreadpoolWaitCallbacks (readWait, TRUE)`.
     5. `GetOverlappedResult (…, TRUE)` (pipe), the existing pattern of `_windows.cpp:35-49`.
   - **Why the close order holds.** Callbacks run one at a time on the single pool thread. After step 2, every readiness callback that read `isCancelled` as false has returned. After step 3, no timer callback is queued or running. Step 4 cancels the one read that a timer callback may have issued. Its readiness callback reads `isCancelled` as true and arms no timer.

**Validation:**
- No buffer and no `read()`.
- Handlers make no wait, lock or allocation.
- Each half mirrors the other's structure: a readiness source, a one-shot timer, and the clock.

### Step 3: Both TTYs on the readiness type, and the zero-copy Windows TTY (R1, R3, R4)
**Scope:** JAM `jam_terminal/tty/native/jam_TerminalTTY_posix.{h,cpp}`, `jam_TerminalTTY_windows.{h,cpp}`, and `jam/cast/terminal.md` (the `ttyClockWait` and `ttyExitPseudoConsole` rows, `:97-102`).

**Action, macOS TTY:**
- Delete `addReadSource` and its members (`posix.cpp:203-244`, `posix.h:174-203`).
- Add `jam::AsyncOutputStream reader` under `#if JUCE_MAC`. The name is from `jam_TerminalTTY_windows.h:315`.
- `open` sets `reader.onBytesReady` with the existing lambda form (`jam_TerminalTTY_windows.cpp:665-668`) and calls `reader.open (master, context, bufferSize, maximumPeriodMilliseconds)` in place of `:272-273`.
- `close` calls `reader.close()` in place of `:297-310`, before `::close (master)`.

**Action, Windows TTY.**
1. **Delete:**
   - the duplicate handle (`:661-662`);
   - `unreadBytes` (`.h:317-323`) and `writeBuffer` (`.h:303-308`, `:649`);
   - `processWait`: its member, callback and close (`.h:283-290`, `:695-706`, `:748-753`);
   - the `ttyExitPseudoConsole` and `ttyClockWait` identifier rows. `ttyClockWait` has no use, and `ttyExitPseudoConsole`'s only use is `:701` (Pathfinder inventory);
   - the bodies of `read` (`:796-812`) and `write` (`:816-838`).
2. **ConPtyFuncs:**
   - Add `ReleaseFunc = HRESULT (WINAPI*) (HPCON)`, a `release` member, and `releasePseudoConsoleSymbol { "ReleasePseudoConsole" }` after `:81`.
   - `isValid` (`:110`) also checks `release`.
   - Both load branches resolve it (`:165-170`, `:188-193`).
3. **`open`:**
   - It names the `double` parameter `maximumPeriodMilliseconds` (`:641`).
   - It calls `reader.open (pipe.get(), context, bufferSize, maximumPeriodMilliseconds)`.
   - After `spawnProcess` it calls `funcs.release (console)` (`ConptyConnection.cpp:451`).
4. **`read`:**
   - `PeekNamedPipe` gives `available`. A failure returns `-1`, so the end of the stream reaches `Id::shellExited` (`jam_TerminalProcessor.cpp:89`). Zero returns `0`.
   - Otherwise `ReadFile (pipe, buf, min (available, maxBytes), nullptr, &readOverlapped)` reads straight into the caller's buffer.
   - It asserts synchronous completion (Learn, bounded read). `GetOverlappedResult (FALSE)` gives the count it returns.
5. **`write`, with no copy.**
   - **Harvest.** When `writeEvent` is signaled and `buf` equals `pendingWriteAddress`, it runs `GetOverlappedResult (FALSE)` and `ResetEvent`, then returns the count.
   - **Idle.** When `HasOverlappedIoCompleted` holds, it runs `WriteFile (pipe, buf, len, nullptr, &writeOverlapped)` from the caller's ring bytes and stores `pendingWriteAddress`. A synchronous success is harvested at once. A pending write returns `0`. An error returns `-1`.
   - **Pending.** Otherwise it returns `0`.
   - **Why the ring bytes stay valid.** The FIFO keeps unaccepted bytes in place (`jam_TerminalFIFO.cpp:69-87`).
   - **Why `pendingWriteAddress`.** Two FIFOs, input and reply, call `write`. The address ties the result to the FIFO that issued the write.
6. **`close`**, in order:
   1. Wait for `resizeWork` (`:755-759`).
   2. Terminate the child (`:761-777`).
   3. `reader.close()`.
   4. Close the pseudo console (`:779-785`).
   5. `CancelIoEx (pipe, &writeOverlapped)` and `GetOverlappedResult (TRUE)`.
   6. Reset `pipe` and `writeEvent`.

**Validation:**
- No `memcpy` in the TTY.
- No member that stages bytes.
- No `processWait`.
- The read contract `jam_TerminalTTY.h:116-125` and the write contract `:127-137` are unchanged.

### Step 4: Both displays on the readiness type (R1, R4)
**Scope:** JAM `jam_terminal/display/native/jam_TerminalDisplay_posix.{h,cpp}`, `jam_TerminalDisplay_windows.{h,cpp}`.

**Action, macOS display:**
- Delete `readSource`, `timerSource`, `isReadSourceSuspended`, `clock`, the read and timer handlers, and their close (`posix.cpp:19`, `:29-53`, `:55`, `:88-98`, `posix.h:64-65`, `:67`, `:69`).
- `resizeSource` and `cancelled` stay for the resize tick only (`:20-27`, `:54`, `:56`, `:100-102`).
- Add `jam::AsyncOutputStream reader`. `open` opens it on `STDIN_FILENO` with `bufferSize` and `maximumPeriodMilliseconds` (`:76-79`). `close` closes it first.

**Action, Windows display:**
1. **Delete:** `readWait` (`.h:58`, `.cpp:36-48`, `:56-61`, `:101`), `inputRecords` (`.h:61`, `.cpp:34`), `getUnicodeChar` (`.cpp:5-11`) and the `read` body (`:76-104`).
2. **Code page.** Add `std::optional<UINT> savedInputCodePage`. `open` saves `GetConsoleCP()` and calls `SetConsoleCP (CP_UTF8)`. `close` restores it, as it restores the modes (`:63-73`).
3. **Readiness.** `open` names the `double` parameter `maximumPeriodMilliseconds` (`:18`) and calls `reader.open (input, context, bufferSize, maximumPeriodMilliseconds)`.
4. **`read`:**
   - `PeekConsoleInputW (input, &head, 1, &numPeeked)` reports the head record.
   - When the head is a `KEY_EVENT` with a nonzero `UnicodeChar`, `ReadFile (input, buf, maxBytes, &numRead, nullptr)` returns the VT bytes straight into `buf`.
   - Any other head record is removed with `ReadConsoleInputW (input, &head, 1, &numRead)`, and the call returns `0`.
   - So `ReadFile` never meets a queue that holds only records that are not keys (`stream.cpp:138-241`).

**Validation:**
- Only the readiness type arms readiness.
- `read()` no longer re-arms.
- No record array and no conversion.
- `TerminalWindowsDisplay` and `TerminalUnixDisplay` have no instantiation in JAM, END or EVE (Pathfinder inventory, Grep), so this step is checked by compile only.

### Step 5: The decoder reads through TerminalFIFO, and replies go through TerminalFIFO (R4, R7)
**Scope:** JAM `jam_terminal/processor/jam_TerminalProcessor.{h,cpp}`.

**Action:**
1. **Delete:**
   - `juce::MemoryBlock buffer`, `pendingBytes` and `reservedSlot` (`.h:155`, `:174-175`). The spare byte has no reader: its only uses are `:14`, `:73`, `:130`, `:138` and `:164-165`.
   - `replyBytes`, `std::unique_ptr<juce::MemoryOutputStream> reply` and `pendingReplyBytes` (`.h:172-173`, `:176`).
   - The `memmove` lines `:119` and `:169`.
2. **New members.** `jam::TerminalFIFO buffer` and `jam::TerminalFIFO reply`.
   - `prepare` (`:9-18`) calls `buffer.prepare (maximumBlockSize)` and `reply.prepare (maximumBlockSize / minRequestLength * maxReplyLength)`.
   - `reset` sizes `images` with `buffer.getFreeSpace() + buffer.getNumReady()` in place of `buffer.getSize()` (`:31`).
3. **Read.** `read` (`:123-134`) becomes `buffer.writeFromTerminalTTY (tty)`. `numReadable` is `buffer.getFreeSpace()` (`:73`).
4. **Tokens.** `applyTokens` and `setPendingBytes` (`:136-170`) run inside `buffer.applyPendingData`. The callback returns the consumed offset.
   - The cut token stays published in the ring and has no copy.
   - The `isBlockFull` rule (`:165-168`) returns the whole size.
   - In-place payload decoding takes the `char*` from the callback (`:234`, `:1452-1455`).
5. **Replies.** Every reply writer takes `juce::OutputStream&` in place of `juce::MemoryOutputStream&`. These include the construction tables (`.h:244`) and the `applyTokens` / `applyToken` / `applyCsiSequence` parameters. `TerminalFIFO` is a `juce::OutputStream` (`jam_TerminalFIFO.h:14`).
   - `applyBytes` (`:103-121`) ends with `reply.flush()` and then `reply.writePendingData (tty)`.
   - `setPosition` and `getPosition` (`:105`, `:110`) go.

**Validation:**
- The byte lane holds one OS read into the ring and no other copy.
- No `memmove` and no `MemoryOutputStream`.
- `process()` keeps its return contract (`:67-97`).

### Step 6: The EVE input, command and grid lanes (R3, R4, R5)
**Scope:** EVE `Source/EVETerminalProcessor.{h,cpp}`.

**Action:**
1. **Input (R5).**
   - When `pendingInput` is empty, `addInput` (`:268-272`) writes `min (input.getFreeSpace(), size)` bytes straight into `input`, flushes, and appends only the rest to `pendingInput`.
   - When `pendingInput` is not empty, `addInput` appends all the bytes, so the order is kept.
   - `applyPendingInput` is unchanged (`:274-286`).
2. **Commands.**
   - Delete `applyNames` (`:29-55`).
   - `applyCommands` (`:181-188`) reads `commands` with `applyPendingData`. It builds each name with `juce::Identifier { juce::String::CharPointerType (data), juce::String::CharPointerType (terminator) }` (`juce_Identifier.h:72`) and returns the bytes consumed.
3. **Grid events.**
   - Add `static const std::array<juce::Identifier, 3> gridEventTypes { Id::gridStart, Id::removedGridStart, Id::firstLine }`. It is the one source of the list at `:155`.
   - The writer (`:153-171`) writes `writeInt (index)` and `writeInt64 (line)` per event, the record form of `jam_TerminalProcessor.cpp:252-254`.
   - `applyGridEvents` (`:190-239`) reads the records in place with `juce::ByteOrder::littleEndianInt` / `littleEndianInt64`, the pattern of `jam_TerminalImageRecord.cpp:68-70`, and dispatches `listUpdates` with `gridEventTypes.at (index)`.

**Validation:**
- No `juce::String` on the command and grid lanes.
- An input that fits makes one cross-thread copy.
- Only overflow is staged.

### Step 7: The image record header is read in place (R4)
**Scope:** JAM `jam_terminal/document/jam_TerminalImageRecord.cpp`.

**Action:**
- Delete `headerBlock` and `headerStream` (`:62-63`, `:67`).
- In each loop pass, `images.applyPendingData` reads the number, the size and the encoding at the view with `juce::ByteOrder` (`:68-70`) and returns `imageHeaderBytes`.
- The payload still goes to the file with `images.writePendingData (imagesFileStream, size)` (`:76`). That is the OS write.

**Validation:** no header copy; the record grammar is unchanged (`jam_TerminalImageRecord.h:14-17`).

### Step 8: Replay record (R6, R7)
**Scope:**
- EVE `Source/TerminalMemoryTTY.{h,cpp}` and `Source/EVETerminalProcessor.cpp` (`createTTY`, `:108-124`)
- `cast/identifiers.md` (a new `recordCapacity` row after `:20`)
- `benchmark/replay.md`, `benchmark/replay-where.md` (a `recordCapacity | int | <bytes>` row in `## profiler`)

**Action:**
1. **Delete:** `chunk` (`.h:82-83`, `.cpp:16`), the `juce::MemoryBlock fixture`, the `MemoryInputStream stream`, and the `#if JUCE_MAC` dispatch (`.cpp:37-39`).
2. **The record.**
   - `fixture` becomes a `jam::TerminalFIFO`.
   - The constructor takes the capacity, and `createTTY` passes `getProfilerTable()[Id::recordCapacity]` (`:471-484`).
   - `open` calls `fixture.prepare (capacity)` once.
3. **Recording.** `handleAsyncUpdate` (`:74-87`) loops `fixture.writeFromTerminalTTY (*tty)`, and the end signals `recorded`. A full record asserts.
4. **Serving.** `read` serves the record with `fixture.applyPendingData`. That is one copy into the decoder ring, standing in for the OS read. It returns `-1` when the record is empty. `isOpen` is true when `recorded` has been signaled.
5. **The first replay tick.** It comes from `device.requestProcess()` in `applyPendingInput` (`EVETerminalProcessor.cpp:285`), which the text editor getter runs (`:257`).

**Validation:**
- One allocation, at open.
- No growth copy.
- No platform branch in EVE.

### Step 9: SPEC
**Scope:** `SPEC.md` Core Principle 2 (`:45-60`).

**Action:**
- Replace the Windows payload sentence (`:46-48`) and the Windows re-arm sentence (`:53-55`). The new text: Windows ticks in the PTY rhythm the same way as macOS, through the shared readiness type. The payload is the bytes `PeekNamedPipe` reports. A one-shot thread-pool timer on the clock's context ends the listening window and re-arms readiness after the clock's period. The re-arm is a zero-byte overlapped read.
- ARCHITECT's R1 and R2 quotes go beside the text.
- `:132-133` (the device buffer size per OS) is unchanged, because the value is unchanged.

**Validation:** each new SPEC line carries ARCHITECT's words.

### Step 10: Build and measure (Pathfinder)
**Scope:** `benchmark/seq-windows.md` (existing manifest; its `## toolchain` row builds Release with profile symbols).
**Action:**
- Pathfinder runs `scout` with that manifest once, with a 15-minute watchdog. A compile error goes back to the step that owns the file, through Engineer.
- COUNSELOR reads the report and compares it with `benchmark/seq-windows-report.md`. The checks:
  - `device clock` calls (was 997,637) and bytes per tick (was about 7);
  - the epoch time (was 66.6 s);
  - `terminal read` calls (was 1,007,833).
- The zero-byte read is proved here: a pipe readiness tick has `numSamples > 0`, and the bounded `ReadFile` asserts no pending read in Debug.

**Validation:** the report exists, and the measurements are given to ARCHITECT as numbers.

### Step 11: Audit
**Action:** @Auditor sweeps once against CONTRACT and this PLAN, across all changed files. Each finding goes through the Design by Contract Filter and is resolved this sprint.

### Step 12: Doxygen pass (Pathfinder)
**Scope:** every changed header and implementation in Steps 1-8, plus the stale lines in finding 14.
**Action:** COUNSELOR gives Pathfinder the symbols, the LSP positions, the doxygen index paths and the regeneration command. Pathfinder writes per the `doxygen-writer` skill. COUNSELOR reads each block against the code.

### Step 13: ARCHITECT builds and tests
**Action:** ARCHITECT builds macOS and Windows and tests the runtime:
- typing;
- `seq`;
- the replay (`benchmark/replay.md`);
- close.

## BLESSED Alignment
- **B, Bound.**
  - Each readiness lane has one owner: the readiness type inside its TTY or display.
  - The handle stays with the TTY or display.
  - `close` ends every callback before it returns.
- **L, Lean.** Four readiness copies become one type with two halves. `processWait`, `applyNames`, `chunk`, `writeBuffer` and `unreadBytes` are deleted.
- **E, Explicit.** No bail-out guards. The synchronous bounded read and a full replay record assert.
- **S, Single Source of Truth.**
  - One readiness implementation and one ring type (`TerminalFIFO`).
  - The grid-event list is declared once (`gridEventTypes`).
- **S, Stateless.**
  - The cut token and unaccepted replies stay in the ring as published bytes. No staging member.
  - `pendingWriteAddress` is the identity of the one outstanding OS write.
- **E, Encapsulation.**
  - Platform code stays in the native halves.
  - EVE has no readiness branch.
  - Data keeps its shape: views over the ring, int64 records.
- **D, Deterministic.** The close order is proved on the one-thread pool. The rhythm follows the measured PTY rate on both platforms.

## Risks / Open Questions
- **Linux.** `TerminalFIFO`'s `Native` has Windows and macOS halves only. JAM's Linux branches (`jam_terminal.cpp:34`, `jam_TerminalTTY_posix.cpp:15`) would no longer build `TerminalFIFO`. EVE's platforms are macOS and Windows (EVE `CLAUDE.md`, Project Identity).
