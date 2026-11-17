# PLAN: windows-console-throughput

**RFC:** none. The objective comes from ARCHITECT's prompt.
**Date:** 2026-11-17
**BLESSED Compliance:** verified against the reads cited below.
**Language Constraints:** C++17 / JUCE 8 / JAM. Windows 10 22H2 (build 19045) and later. Windows only.

## Overview

The client gets the reply to each write as soon as the payload is in a byte ring that the console owns. That ring does the job the kernel pty buffer does on macOS. The console server runs on its own `jam::AsyncUpdater` serial context. The decoder takes the ring bytes on its tick, as it does on macOS. The cross-platform layers do not change. The one exception is the visibility of `TerminalFIFO::writeFromDevice` (Step 1, Risks 1).

## Context: what the measurement shows

Runs: `benchmark/seq-10k-windows-server-time-report.md` and `benchmark/seq-1k-windows-server-time-report.md`, both at 17-11-2026 17:20, after the `Document` move fix (`jam_Document.cpp:260-279`).

**How a line moves now:**
1. A client write stays pending until the decoder reads its payload.
   - `setPendingWrite` completes no `WriteConsole` (`jam_TerminalConsole_windows.cpp:582-586`, `jam_TerminalConsoleOutput_windows.cpp:368-374`).
   - Only `read()` completes it (`jam_TerminalConsole_windows.cpp:610-615`).
2. After a tick with bytes, readiness waits one clock period (`jam_AsyncOutputStream_windows.cpp:38-41`). The 10k run has 147 `ttyBytesReady` calls (`10k:94`).
3. The decoder ran 13769 times (`terminalProcess`, `10k:78`). Nearly all of these runs come from the editor's pull on the message thread:
   1. The editor's `winsize` getter calls `triggerAsyncUpdate` (`EVETerminalProcessor.cpp:209-214`).
   2. The swap write calls `requestProcess` (`:302-307`).
   3. `requestProcess` gives `clock (0)`, then `process` (`jam_VirtualDevice.cpp:84-92`).
4. Thus the client advances at the editor's frame rate. The run has 6931 `editorBlocks` calls for 10k lines (`10k:82`).
5. `editorBlocks` takes 4.19 s of the 5.77 s `launchPrompt` (`10k:82`, `:117`). It costs 165 µs per call at 1k and 605 µs per call at 10k (`1k:82`).
6. The decoder costs 180 ms for 10k lines (`10k:78`).

**macOS uses the same period law** (`jam_AsyncOutputStream_mac.cpp:23-31`). There, the shell writes into the kernel buffer and does not wait for the decoder. `TerminalUnixTTY::read` is `::read (master, …)` (`jam_TerminalTTY_posix.cpp:275-277`).

**On Windows, msys2 waits on each `WriteConsoleW`.** That call sits between `acquire_attach_mutex` and `ReleaseMutex` (msys2 `console.cc:2842-2872`).

**ARCHITECT's direction, verbatim:**
- *"this is not rendering problem, this is traffic problem."*
- *"TextEditor contract should NOT be changed."*
- *"why we need to touch macOS lane? what's wrong with the idea implemented windows exclusive? the crossplatform layers must holds as is"*
- *"message thread could lock, async, even spawn parallel job. but MUST NEVER BLOCKED, CHOKED, NEVER"*
- *"0 rules. non-negotiable. NO COPY, NO ALLOC, NO TEMP CONTAINERS, NO SHADOW STATE, only exception copy is cross thread."*
- *"jam always priority over juce"*
- *"CONTRACT: - juce::Value ALWAYS message thread only - other thread use atomic for the exact parameter."*
- *"use CriticalSection or MessageManagerLock NEVER HANDROLL"*
- *"NO SIDECAR."*

## Language / Framework Constraints

- Windows-only code stays under `#ifdef JUCE_WINDOWS` in the `*_windows.{h,cpp}` files.
- The JAM module unity build compiles each body one time (CODING.md, One object, one translation unit).

## Copies after this plan (MANIFESTO E, closed list, :273-282)

| Bytes | Copy | Thread | Permitted copy |
|---|---|---|---|
| `WriteConsole` payload | client buffer → `sequence` ring, through `READ_INPUT` (`readInputPayload`, `jam_TerminalConsoleRequest_windows.cpp:52-57`) | server context | 1. OS or device boundary |
| Rendered VT bytes (screen API, echo) | created in place in `sequence` through `jam::Format` on `juce::OutputStream` (`jam_Format.h:416`) | server context | none needed: creation (MANIFESTO :254) |
| `sequence` → decoder `buffer` (`TerminalProcessor::read`, `jam_TerminalProcessor.cpp:113-121`) | one copy-out in `tty.read` | chain context | 2. Ownership handoff between threads |
| Typed and reply bytes | lane buffer → `queue`, in `tty.write` (`writeComplete`, `jam_TerminalConsoleInput_windows.cpp:32-43`) | chain context | 2. Ownership handoff between threads |
| `queue` / `clientQueue` bytes → read payload | `writePayload` from the ring in place (`jam_TerminalConsoleInput_windows.cpp:237-241`), or through the `records`, `units`, `bytes` scratch of one request | server context | 1. OS or device boundary |
| `WriteConsoleInput` records | client buffer → `records`, through `READ_INPUT` (`jam_TerminalConsole_windows.cpp:1013`); UTF-8 encoded into `clientQueue` (`:1017-1028`) | server context | 1. OS or device boundary; the encode is a creation |

This is the macOS count. On macOS, the shell's `write` goes into the kernel, and `::read` takes it into the decoder `buffer`.

The `std::string sequence` and its `offset` are deleted. No staging buffer remains.

## Thread ownership after this plan

| State | Owner thread | Cite |
|---|---|---|
| `requests`, `reply`, `connection`, `firstPendingRead`, `numPendingReads`, the process registry, `records`, `units`, the `TerminalConsoleOutput` scratch (`cells`, `units`, `bytes`), `writeOffset`, `writeSize`, `isResizePending` | server context (`TerminalConsole` as `jam::AsyncUpdater`) | `jam_TerminalConsole_windows.h:179-205`; `jam_TerminalConsoleOutput_windows.h:236-249`; `jam_TerminalConsoleInput_windows.h:202-213` |
| `sequence` (TerminalFIFO) | writer: server context. Reader: chain context | `jam_TerminalFIFO.h:16-18` |
| `queue` (TerminalFIFO) | writer: chain context (`tty.write`). Reader: server context | `jam_TerminalFIFO.h:16-18` |
| `clientQueue` (TerminalFIFO), `echoOffset` | server context only (`writeConsoleInput` → `addClientInput`; reads through `applyBytes`, `applyEcho`) | `jam_TerminalConsoleInput_windows.h:127-148`; `jam_TerminalConsole_windows.cpp:1028` |
| `numInterrupts` (`std::atomic<int>`) | writer: chain context and server context (`writeInput`, `jam_TerminalConsole_windows.cpp:933`). Reader: server context (`handleAsyncUpdate`, `:376`) | `jam_TerminalConsole_windows.cpp:376-377`, `:933` |
| `isServerOpen` (`std::atomic<bool>`) | writer: server context (`closeServer`) and `open`. Reader: chain context (`read`, `write`, `onEvent`) | `jam_TerminalConsole_windows.cpp:370`, `:674` |
| `reader` (`AsyncOutputStream`, event row) | chain context (`device.getContext()`) | `EVETerminalProcessor.cpp:397`, `:427` |
| Model values (`getWinsize`, `getCursor`, `getPen`, `getInputMode`, `getOutputMode`, `setInputMode`, `setOutputMode`) | any thread | `jam_ParameterBase.h:164-165`, `:218-219`; `jam_Model.h:1002-1019` |
| `outputEvent`, `resizeEvent`, `readEvent` (kernel events) | any thread | `SetEvent` |

## Framework Inventory

### 1. API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| Own serial context with one thread | jam_core | `jam::AsyncUpdater` (private base): `triggerAsyncUpdate`, `handleAsyncUpdate`, `getContext`, `cancelPendingUpdate` | `jam_AsyncUpdater.h:19-72`; Windows pool min = max = 1 at `jam_AsyncUpdater.cpp:49-61` | 2 |
| Readiness on a handle, on a chosen context | jam_core | `AsyncUpdater::Source::open (context, handle)`, `arm` | `jam_AsyncUpdater.h:119-131`; `.cpp:294-309` | 2 |
| Byte ring, one writer and one reader, lock-free | jam_terminal | `TerminalFIFO`: `prepare`, `getNumReady`, `getFreeSpace`, `write`, `flush`, `applyPendingData` | `jam_TerminalFIFO.h:20-164` | 3, 4, 5 |
| Write into the free space of the ring from a device call | jam_terminal | `TerminalFIFO::writeFromDevice` (public after Step 1) | `jam_TerminalFIFO.h:166-194` | 1, 3 |
| Formatted bytes straight into a stream | jam_core | `Format::appendBytes (juce::OutputStream&, …)`, `appendDecimal`, `appendUtf8` (templates over the buffer) | `jam_Format.h:416`, `:427-435`, `:567-574` | 3 |
| The ring is a stream | jam_terminal | `class TerminalFIFO : public juce::OutputStream` | `jam_TerminalFIFO.h:20` | 3 |
| Payload read from the driver | jam_terminal | `TerminalConsoleRequest::readInputPayload` | `jam_TerminalConsoleRequest_windows.cpp:52-57` | 3 |
| Readiness type of the chain | jam_terminal | `AsyncOutputStream` event row, `onEvent` | `jam_AsyncOutputStream_windows.cpp:10-16`; `jam_TerminalConsole_windows.cpp:259-263` | 4 |
| Model reads and stores on any thread | jam_data_structures | `Parameter<int>::getValue`, `Parameter<int64_t>::getValue`, `Model::storeValue` | `jam_ParameterBase.h:164-165`, `:218-219`; `jam_Model.h:1002-1019` | 2-5 |

### 2. Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| A class takes `jam::AsyncUpdater` as a private base and opens its sources on `getContext()` | `TerminalMemoryTTY` (`TerminalMemoryTTY.h:13-14`, `.cpp:17-19`) | `VirtualDevice` (`jam_VirtualDevice.h:19-21`), `EVETerminalProcessor` (`EVETerminalProcessor.h:30-31`) | 2 |
| A reader takes ring bytes in place and reports what it consumed | `TerminalFIFO::writePendingData (TerminalTTY&)` (`jam_TerminalFIFO.cpp:61-64`) | `TerminalConsoleInput::readUnicode` (`jam_TerminalConsoleInput_windows.cpp:82-109`) | 4, 5 |
| A device call writes into the free space of the ring | `writeFromTerminalTTY (TerminalTTY&)` → `writeFromDevice` (`jam_TerminalFIFO.cpp:83-86`) | `writeFromTerminalTTY (Handle)` (`jam_TerminalFIFO_windows.cpp:50-75`) | 3 |
| A device call writes into a caller destination | `renderBytes`, `renderUnicode (…, char* destination, int maxBytes, …)` (`jam_TerminalConsoleOutput_windows.cpp:622-668`) | `TerminalConsoleOutput::read` (`:83-101`) | 3 |
| An owner callback brings a Model value to the server | `console.getWinsize = [this] { return getWinsize(); };` (`jam_TerminalTTY_windows.cpp:37-38`) | `getCursor`, `getPen`, `getInputMode`, `setInputMode`, `getOutputMode`, `setOutputMode` (`:34-53`) | 2-5 |

### 3. Names

| Family | Members | Intent | Step |
|---|---|---|---|
| `TerminalFIFO` writer set | `write`, `flush`, `writeFromTerminalTTY`, `writeFromDevice` (`jam_TerminalFIFO.h:80-194`) | Fill the ring and publish | 1, 3 |
| `apply*` on the input | `applyRecords`, `applyBytes`, `applyEvent` (`jam_TerminalConsoleInput_windows.cpp:74`, `:229`, `:294`) | Push a result onto a target that the caller gives | 5: `applyEcho` |
| Transit position | `offset`, `writeOffset` (`jam_TerminalConsoleOutput_windows.h:239-241`) | Progress in a buffer in transit | 5: `echoOffset` |
| Count members `num*` | `numPendingReads`, `numProcesses` (`jam_TerminalConsole_windows.h:186-191`) | A count | 5: `numInterrupts` |
| Input ring `queue` with the domain word *client* | `queue`; `clientInput`, `clientOutput` (`jam_TerminalConsole_windows.cpp:328-329`) | The ring of the records that the client writes back | 5: `clientQueue` |
| `add*` on the input | `addResize` (`jam_TerminalConsoleInput_windows.h:60`) | Insert into the input | 5: `addClientInput` |
| Booleans `is*` | `isResizePending` (`jam_TerminalConsoleInput_windows.h:207`) | A condition | 4: `isServerOpen` |

No other name is new. `sequence` keeps its name and changes its type to `TerminalFIFO`.

### 4. Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "Use the framework API fully" | A serial context comes from `jam::AsyncUpdater`, never from a hand-written thread | `jam_VirtualDevice.h:19-21` |
| "Deadline threads … does not allocate, lock, wait" | The owner sizes the rings in `open`, off the tick | `jam_TerminalConsole_windows.cpp:253-255`; `jam_TerminalConsoleInput_windows.cpp:15-18` |
| "No bail-out guards" | Positive nesting; a result return when the answer is known | `jam_TerminalConsole_windows.cpp:626-644` |
| "Uniform signature in the type" / lookup over branches | `LookupTable` dispatch | `jam_TerminalConsoleOutput_windows.cpp:475-488` |
| "No out-parameters" | Return the value | `jam_TerminalConsoleInput_windows.cpp:31-43` |

### 5. Absence

| Need | Query | Result |
|---|---|---|
| A second SPSC byte ring type in JAM | LSP `workspaceSymbol` `SpscRing`, `ByteRing`, `TerminalRing` | No ring type found. `TerminalRing` matches only `map::MermaidNodeShape::terminalRing` (`jam_Mermaid.h:344`) |
| A public `TerminalFIFO` writer that hands the free space to a callback (before Step 1) | `jam_TerminalFIFO.h:20-164` (public section, read in full) | None. `writeFromDevice` was private (`:178-195`) |

## Validation Gate

COUNSELOR validates each step before the next one, against:
- MANIFESTO.md (BLESSED);
- NAMES.md;
- ~/.carol/CODING.md;
- the Inventory rows of the step;
- this PLAN, with no deviation and no scope drift.

@Auditor runs once, after the final code step.

## Steps

### Step 0: Write the plan to the EVE root — done
This file.

### Step 1: A public writer over the free space of the ring — done
**Scope:** `jam_TerminalFIFO.h`.

**Action:** Move the template `writeFromDevice` from `private:` to `public:`. Do not change its body. Option (a) of Risks 1, as approved with the plan.

**Inventory:** API row "Write into the free space…"; Names row `TerminalFIFO` writer set.

**Validation:**
- No body change.
- The macOS build is unchanged in behaviour.

### Step 2: The server on its own context
**Scope:** `jam_TerminalConsole_windows.{h,cpp}`.

**Action:**
1. `class TerminalConsole : private AsyncUpdater`, in the form of `TerminalMemoryTTY.h:13-14`.
2. `requestSource.open (getContext(), readEvent.get())` and `resizeSource.open (getContext(), resizeEvent.get())`. These replace `context` at `jam_TerminalConsole_windows.cpp:312`.
3. `reader.open (outputEvent.get(), context, …)` stays on the `context` argument, which is the device's context.
4. `close()` does these in order:
   1. `reader.close()`;
   2. `requestSource.close()`;
   3. `resizeSource.close()`;
   4. `cancelPendingUpdate()`;
   5. the cancel of the pending `READ_IO`;
   6. the handle resets, as now (`:345-363`).

**Inventory:** API rows "Own serial context", "Readiness on a handle"; Pattern row `jam::AsyncUpdater` private base.

**Validation:**
- No `std::thread`, no `juce::Thread`, no lock.
- `cancelPendingUpdate` is not called from the server's own context (`jam_AsyncUpdater.h:61`).

### Step 3: Writes complete into the `sequence` ring
**Scope:** `jam_TerminalConsoleOutput_windows.{h,cpp}`, `jam_TerminalConsole_windows.cpp`.

**Action:**
1. `std::string sequence` and `size_t offset` become `TerminalFIFO sequence`. `prepare (bufferSize)` sizes it, in place of the reserve at `jam_TerminalConsole_windows.cpp:255`.
2. The writers write into `sequence` through its `juce::OutputStream` base, and each render step ends with `sequence.flush()`:
   - the file helpers `appendByte`, `appendEscape`, `appendPrivateMode` and `appendCharacters` (`:19-69`) take `juce::OutputStream&`;
   - `Format::appendBytes`, `appendDecimal` and `appendUtf8` already accept that stream (`jam_Format.h:416`).
3. `isFitting` and `getNumFitting` read `sequence.getFreeSpace()` in place of `capacity() - size()` (`:113-122`).
4. A write payload goes in through `sequence.writeFromDevice`. Its callback is `renderUnicode` or `renderBytes` with `(destination, maxBytes)` (`:622-668`). The callback chooses by `request.isUnicodeWrite()`, as `read` does at `:94-97`.
5. `TerminalConsole::setPendingWrite` and the pending-request path do these:
   1. Copy the payload until `writeOffset == writeSize`, or until the ring has no free space.
   2. When the payload is done: `setWritten`, then the next `READ_IO`.
   3. When the ring is full: the request stays pending as a descriptor only, and the request loop stops for this request (`:267-269`).
6. Each publish calls `SetEvent (outputEvent.get())`. `outputEvent` becomes an auto-reset event (`CreateEventW (nullptr, FALSE, …)` at `:245`). The `ResetEvent` branch of `applyOutputEvent` (`:953-954`) is deleted.

**Inventory:** API rows ring, writer, Format, stream, payload read; Pattern rows "device call writes into the free space", "device call writes into a caller destination"; Copies table.

**Validation:**
- One copy for each payload byte (OS boundary). Rendered bytes are created in place.
- No `std::string` staging, no second copy on the server thread (MANIFESTO :282).
- No `get` verb that changes state.

### Step 4: The chain takes the bytes
**Scope:** `jam_TerminalConsoleOutput_windows.{h,cpp}`, `jam_TerminalConsole_windows.{h,cpp}`.

**Action:**
1. `TerminalConsoleOutput::read (char* destination, int maxBytes)` copies up to `maxBytes` published bytes of `sequence` into `destination`, through `applyPendingData`. It renders nothing.
2. `TerminalConsole::read` does these:
   1. It calls `output.read`.
   2. It calls `triggerAsyncUpdate()`, so the server resumes a write that waits for space.
   3. It returns the count, or -1 when `isServerOpen` is false and the ring is empty.
3. `reader.onEvent` returns `output.getNumReady()`, which is `sequence.getNumReady()`, or -1 under the same rule. It no longer reads `getRequest()` (`:261`).
4. Add `std::atomic<bool> isServerOpen`. The server context stores `false` where it resets `server` (`:287`). The chain context reads it. This follows *"other thread use atomic for the exact parameter."*

**Inventory:** API rows ring, readiness; Pattern row "reader takes ring bytes in place"; Names row `is*`.

**Validation:**
- The chain context reads no server-owned member (Thread ownership table).
- One copy, cross-thread.

### Step 5 (revision 3): one ring for each writer
**Why revised:** the audit found Bug 1 in revision 2.
- Two chain lanes call `tty.write` in the same tick: the input lane (`EVETerminalProcessor.cpp:520`) and the decoder reply lane (`jam_TerminalProcessor.cpp:109-110`).
- The revision 2 offer slot returns the count of an earlier offer, whichever lane made it. That breaks `TerminalTTY::write`, which returns *"Number of bytes the PTY accepted"* for its own `buf` (`jam_TerminalTTY.h:127-137`).
- Thus the chain accepts bytes at once. Each writer has its own ring, as the established lanes `reply` and `input` do.
- These alternatives are excluded:
  - A lock: the chain is a clock thread (MANIFESTO :269).
  - A change to the reply lane: ARCHITECT, *"the crossplatform layers must holds as is"*.

**Action:**
1. `queue`: the writer is the chain (`TerminalConsole::write` → `writeInput` → `TerminalConsoleInput::write`). The reader is the server. `TerminalConsoleInput::write` does no reader work: no `applyEvent`.
2. The chain's `TerminalConsole::write` does these, then returns the count:
   - `writeInput`, which splits at each unescaped ETX. Each ETX adds 1 to `std::atomic<int> numInterrupts`.
   - `triggerAsyncUpdate()`.
3. `clientQueue` (`TerminalFIFO`, in `TerminalConsoleInput`, sized in `open` with `queue`): the server is both the writer and the reader. `writeConsoleInput` writes the encoded records into it through `TerminalConsoleInput::addClientInput`, through the same `writeInput` ETX split. Then it calls `triggerAsyncUpdate()`.
4. Each consuming read takes exactly one ring: `clientQueue` when it has ready bytes, otherwise `queue` (`applyBytes`, `jam_TerminalConsoleInput_windows.h:127-148`). This is the order that msys2 rebuilds (msys2 `console.cc:637-649`). A read of both rings in one call overwrote the first result in `readUnicode`, `readLine` and `writeText`. Every view-only query (`getNumRecords`, `isEmpty`, `hasLine`) covers both rings. `getNumCharacters` is deleted: its one caller was the revision 2 `handleAsyncUpdate`.
5. Echo covers `queue` only, through `echoOffset` and `applyEcho` (the revision 1 form). A consuming read of `queue` lowers `echoOffset`.
   - Audit Bug 3: no echo while `output.isRequestPending()`. The echo then waits, and the next update echoes it.
6. `handleAsyncUpdate` does these in order:
   1. `endTasks` for each count of `numInterrupts.exchange (0)`;
   2. the echo;
   3. `completeReads()`;
   4. `input.applyEvent()`;
   5. the render resume;
   6. `applyOutputEvent()`.
7. Delete `pendingInput`, `numPendingInput` and `numAcceptedInput`.

**Names:**
- `clientQueue`: family `queue`, with the domain word *client* (`clientInput`, `clientOutput`, `jam_TerminalConsole_windows.cpp:328-329`).
- `addClientInput`: family `add*` on the input (`addResize`).
- `numInterrupts`, `echoOffset`, `applyEcho`: as in revision 1.

**Thread ownership:**
- `queue`: writer is the chain, reader is the server.
- `clientQueue`: the server only.

**Validation:**
- Each ring has one writer and one reader (`jam_TerminalFIFO.h:16-18`).
- `tty.write` returns the count of its own bytes.
- The chain makes no lock and no wait.

### Step 5 (revision 2, superseded by revision 3): Input across the two contexts. The server is the one thread of `queue`
**Why revised:** revision 1, below, gave `queue` two writers. `writeConsoleInput` writes `queue` on the server context (`jam_TerminalConsole_windows.cpp:990-1018`, `:1011`). msys2 writes its input records back through `WriteConsoleInputW` and checks them at once with `PeekConsoleInputW` (msys2 `console.cc:579-623`). Thus the write-back must land at once and in order, on the server. Deferral through the chain is not deterministic, and `statusNotImplemented` loses typed input.

**Scope:** `jam_TerminalConsole_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}`.

**Action:**
1. `TerminalConsole::write` (chain context) offers the bytes as a view. It copies nothing. A view passed by value is not a copy (MANIFESTO :280). The input lane keeps the bytes published at the same address until `write` reports them accepted (`jam_TerminalFIFO.h`, `writePendingData (TerminalTTY&)`: *"A TTY that writes asynchronously can rely on that address."*).
   - It reads `numPendingInput` (acquire).
   - When it is 0, the offer slot is empty. The call takes `numAcceptedInput.exchange (0)` as its result.
   - When `numBytes` is larger than the result, it stores `pendingInput = data + result` and `numPendingInput = numBytes - result` (release), then calls `triggerAsyncUpdate()`.
   - It returns the result, or -1 when `isServerOpen` is false.
2. `TerminalConsole::handleAsyncUpdate` (server context) takes the offer.
   - When `numPendingInput` (acquire) is above 0, it runs the original write body on the view: the echo with `input.getNumCharacters()`, then `writeInput`, then `output.writeEcho`.
   - It stores the accepted count in `numAcceptedInput` (release). Then it stores 0 in `numPendingInput` (release).
   - The copy into `queue` is the one cross-thread copy (MANIFESTO :276).
   - Then it runs `completeReads()`, the render resume of Step 3, and `applyOutputEvent()`.
3. `writeInput` calls `endTasks` again, on the server context. `numInterrupts` is deleted.
4. `TerminalConsoleInput` returns to its form before revision 1: `applyEvent` is private and `write` calls it; there is no `applyEcho`, no `echoOffset`, no rerouting of the reads; `getNumCharacters` is back.

**Names:** `pendingInput` (`std::atomic<const char*>`), `numPendingInput` (`std::atomic<int>`), `numAcceptedInput` (`std::atomic<int>`). Families: `pending*` (`numPendingReads`, `pendingBytes`, `isRequestPending`), `num*`, and the `numAccepted` local (`jam_TerminalConsoleInput_windows.cpp:33`).

**Thread ownership change:** `queue` has one thread, the server context. It is both the writer and the reader.

**Validation:**
- `queue` is touched only on the server context.
- `writeConsoleInput` and typed input share one writer, so their order is the order of arrival.
- The chain context makes no lock, no wait and no copy (MANIFESTO :269).
- The server reads the offered bytes only between the acquire of `numPendingInput` and its release store of 0. The lane does not consume them until `write` returns them as accepted.

### Step 5 (revision 1, superseded by revision 2)
**Scope:** `jam_TerminalConsole_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}`.

**Action:**
1. `TerminalConsole::write` (chain context) does only writer-side work:
   1. Under `ENABLE_PROCESSED_INPUT`, it splits at each unescaped `ETX`, as `writeInput` does now (`:874-904`).
   2. It writes each segment with `input.write`.
   3. It adds 1 to `std::atomic<int> numInterrupts` for each `ETX`, in place of `endTasks`.
   4. It calls `triggerAsyncUpdate()`.
2. `TerminalConsoleInput::write` drops `applyEvent()` (`:41`), because `applyEvent` reads the queue, which is reader-side work.
3. `TerminalConsole::handleAsyncUpdate` (server context) does these in order:
   1. `endTasks (CTRL_C_EVENT, consoleCtrlCFlag, 0)` once for each count of `numInterrupts.exchange (0)`.
   2. `input.applyEcho`, which writes the echo into `output` while the line and echo modes are on.
   3. `completeReads()`.
   4. `input.applyEvent()`.
   5. It resumes the pending write of Step 3 when the ring has free space.
   6. `applyOutputEvent()`.
4. `TerminalConsoleInput` gains `size_t echoOffset`, the count of front queue bytes already echoed. Its lifetime is the same as `isResizePending`: from arrival to the read that consumes the bytes.
   - `applyEcho` views the queue in place (`applyPendingData`, consumes 0).
   - It passes the bytes after `echoOffset` to `output.writeEcho`, with `TerminalConsoleLine::getNumCharacters` of the bytes before them (`jam_TerminalConsoleInput_windows.cpp:155-166`).
   - It advances `echoOffset` by the bytes whose echo fits.
   - Each read that consumes `n` bytes lowers `echoOffset` by `n`, to a floor of 0.
5. Delete the echo reserve at `:633-637`.

**Inventory:** Names rows `apply*`, transit position, `num*`; Pattern row "reader takes ring bytes in place".

**Validation:**
- `queue` has one writer (chain) and one reader (server) (`jam_TerminalFIFO.h:16-18`).
- The registry is read only on the server context.
- Echo and program output share one writer, so their order is the order of arrival.

### Step 6: Build and measure
1. Pathfinder runs `scout D:/Poems/dev/eve/benchmark/seq-10k-windows-server-time.md`, then `scout D:/Poems/dev/eve/benchmark/seq-1k-windows-server-time.md`. The toolchain rows build JAM and EVE.
2. COUNSELOR reads these values in both reports:
   - `epoch`;
   - `ttyBytesReady` calls;
   - `terminalProcess` calls;
   - `editorBlocks` calls and value;
   - `launchPrompt`.
3. The expected shift: `terminalProcess` calls fall toward `ttyBytesReady` calls, and `epoch` per line falls.

### Step 7: Audit
@Auditor, once, across every changed file.

### Step 8: Doxygen pass
Pathfinder, after the audit, on every changed declaration (CAROL.md Code Hygiene).

### Step 9: ARCHITECT tests
Run these on 26300 and 19045, in EVE Standalone:
- zsh: `time seq 1 1000000`;
- typing, Ctrl+C on a running `seq`, and resize while `vim` runs;
- cmd: cooked-read typing, backspace and Enter, with echo;
- pwsh: PSReadLine editing.

## BLESSED Alignment
- **B.** Each thread owns its state (Thread ownership table). The rings give the one cross-thread path in each direction.
- **L.** No new object. `sequence` changes type, and `std::string` plus `offset` go.
- **E.** Every new name joins a cited family (Names table).
- **S (SSOT).** Model values stay behind owner callbacks. The ring holds the bytes in transit only.
- **S (Stateless).** `echoOffset` and `numInterrupts` are transit counts, in the form of `isResizePending` and `writeOffset`.
- **E (Encapsulation).** Data moves one way: client → `sequence` → decoder `buffer`, lane → `queue` → client, and client records → `clientQueue` → client.
- **D.** One writer for each ring: `sequence` and `clientQueue` on the server, `queue` on the chain. The server's context is serial (`jam_AsyncUpdater.cpp:49-61`).

## Risks / Open Questions
1. **Closed with the approval of this plan.** The ruling *"the crossplatform layers must holds as is"* and the need for a public writer over the free space of `TerminalFIFO` did not both hold. Option (a) was in the plan as approved: `writeFromDevice` became public with no body change (Step 1).

## Undo: remove the dedicated server context

**Why:** ARCHITECT, after the Step 9 test: *"it didnt give significant differences in user experience: - still slow - active prompt still prints before seq"*. ARCHITECT ordered this section: *"documents comprehensively how to undo the whole circus of dedicated thread"*.

### Baseline: no git copy exists
- `git status` in `D:\Poems\dev\jam` shows the six console files as untracked (`??`): `jam_TerminalConsole_windows.{h,cpp}`, `jam_TerminalConsoleOutput_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}`.
- Thus git holds no copy of their state before this sprint. This section is the only record of that state.
- The pre-sprint line numbers below (`old :N`) are the cites of the Context section and of Steps 2-5. They point into the file before this sprint. They do not match the current file.
- `jam_TerminalTTY_windows.cpp` (`MM`, 570 lines changed) holds the earlier ConPTY → ConDrv sprint. This sprint did not change it. Do not undo it.
- The `Document` move fix (`jam_Document.cpp:260-279`, `jam_Document.h:919-939`) came before Step 0 (Context: *"after the `Document` move fix"*). It is a defect fix, not part of the server context. Keep it.

### What the undo keeps
- The in-process ConDrv server itself (the earlier sprint).
- Each ring has one writer. After the undo, `sequence` and `queue` are both on the device context again, so the one-writer rule holds with no cross-thread ring.

### Order
Undo in reverse step order: 5, 4, 3, 2, 1. Build after each step with the scout toolchain rows (Step 6). The compiler lists what remains.

### U5. Undo Step 5 (revision 3): input on one context
Files: `jam_TerminalConsoleInput_windows.{h,cpp}`, `jam_TerminalConsole_windows.{h,cpp}`.
1. Delete `clientQueue` (`jam_TerminalConsoleInput_windows.h:255`) and its `prepare` (`.cpp:16`).
2. Delete `addClientInput` (`.h:53`, `.cpp:50-53`).
3. `writeConsoleInput` (`jam_TerminalConsole_windows.cpp:1028`) passes `&TerminalConsoleInput::write` to `writeInput` again, in place of `&TerminalConsoleInput::addClientInput`. Delete its `triggerAsyncUpdate()` (`:1032`).
4. Delete the `writeComplete` file helper (`jam_TerminalConsoleInput_windows.cpp:32-43`). Its body returns to `TerminalConsoleInput::write` (`:45-47`), on `queue`.
5. `TerminalConsoleInput::write` calls `applyEvent()` again at its end (old `:41`). `applyEvent` (`.cpp:302`) returns to `private:`.
6. `applyBytes` (`.h:157-178`) reads `queue` only, with no `echoOffset` update. Remove the `clientQueue` branch.
7. `getNumRecords`, `isEmpty`, `hasLine` and `flush` cover `queue` only. Delete the `clientQueue.applyPendingData` lines (`.cpp:136`, `:152`, `:168`, `:197`).
8. Delete `echoOffset` (`.h:261`), its reset in `flush` (`.cpp:199`), and `applyEcho` (`.h:84-94`).
9. Restore `TerminalConsoleInput::getNumCharacters`. The pre-sprint echo called it (Step 5 revision 2, point 4: *"`getNumCharacters` is back"*). Its body counts the characters of the queue bytes through `TerminalConsoleLine::getNumCharacters` (old `:155-166`).
10. `writeInput` (`jam_TerminalConsole_windows.cpp:911-941`) calls `endTasks (CTRL_C_EVENT, consoleCtrlCFlag, 0)` again for each unescaped ETX, in place of `++numInterrupts` (`:933`). This was the pre-sprint form (old `:874-904`).
11. Delete `numInterrupts` (`jam_TerminalConsole_windows.h:228-229`) and the `exchange` loop in `handleAsyncUpdate` (`.cpp:376-377`).
12. `TerminalConsole::write` (`.cpp:670-681`) returns to the pre-sprint body: the echo with `input.getNumCharacters()`, then `writeInput`, then `output.writeEcho` (Step 5 revision 2, point 2, *"the original write body"*). Delete its `triggerAsyncUpdate()` (`:677`).
13. Restore the echo reserve that Step 5 revision 1 deleted (old `:633-637`).

### U4. Undo Step 4: the chain reads through the request
Files: `jam_TerminalConsoleOutput_windows.{h,cpp}`, `jam_TerminalConsole_windows.{h,cpp}`.
1. `TerminalConsole::read` (`.cpp:657-663`) renders the pending request again and completes it, as before: *"Only `read()` completes it"* (old `:610-615`). Delete its `triggerAsyncUpdate()` (`:661`).
2. `TerminalConsoleOutput::read` renders into the destination again, through `renderUnicode` or `renderBytes`, chosen by `request.isUnicodeWrite()` (old `TerminalConsoleOutput.cpp:83-101`, `:94-97`).
3. `reader.onEvent` (`jam_TerminalConsole_windows.cpp:259-263`) reads `getRequest()` again, as at old `:261`, in place of `isServerOpen` and `output.getNumReady()`.
4. Delete `isServerOpen` (`.h:226`) and its stores (`.cpp:317`, `:370`). `closeServer` (`:368-372`) resets `server` only.

### U3. Undo Step 3: writes complete at the decoder read
Files: `jam_TerminalConsoleOutput_windows.{h,cpp}`, `jam_TerminalConsole_windows.cpp`.
1. `TerminalFIFO sequence` (`jam_TerminalConsoleOutput_windows.h:257`) becomes `std::string sequence` and `size_t offset` again. `prepare` (`.cpp:73-74`) becomes the reserve at old `jam_TerminalConsole_windows.cpp:255`.
2. The file helpers `appendByte`, `appendEscape`, `appendPrivateMode` and `appendCharacters` (old `:19-69`) take the `std::string` again. Delete every `sequence.flush()` (`.cpp:294`, `:355`).
3. `isFitting` (`.cpp:95`) and `getNumFitting` (`.cpp:98-100`) read `capacity() - size()` again (old `:113-122`).
4. Delete the `sequence.writeFromDevice` payload path (`.cpp:360`). The payload goes into the destination of `read` again (U4.2).
5. `TerminalConsole::setPendingWrite` (`jam_TerminalConsole_windows.cpp:624-627`) completes no `WriteConsole` again. Delete `render() ? setWritten() : nullptr`. The pre-sprint form is at old `:582-586` and old `TerminalConsoleOutput.cpp:368-374`.
6. Delete the render resume in `handleAsyncUpdate` (`.cpp:402`).
7. `outputEvent` returns to a manual-reset event: `CreateEventW (nullptr, TRUE, FALSE, nullptr)` at `:245`. `applyOutputEvent` (`:986-989`) gets back its `ResetEvent` branch (old `:953-954`).
8. `getNumReady` (`.cpp:90`) and `maxEchoBytes` use: check each against the compiler after point 1. Delete what has no caller.

### U2. Undo Step 2: the server on the device context
Files: `jam_TerminalConsole_windows.{h,cpp}`.
1. Delete the private base: `class TerminalConsole : private AsyncUpdater` (`.h:18`) becomes `class TerminalConsole`.
2. Delete `handleAsyncUpdate` (`.h:220`, `.cpp:374-405`). Its remaining calls (`completeReads`, `applyOutputEvent`) return to their pre-sprint call sites in the request loop.
3. `requestSource.open` and `resizeSource.open` (`.cpp:313`) take `context` again, in place of `getContext()` (old `:312`).
4. Delete `cancelPendingUpdate()` from `close()` (`.cpp:352`). The close order returns to old `:345-363`.
5. Delete every `triggerAsyncUpdate()` call that remains. The compiler lists them after point 1.
6. Restore the class doxygen (`.h:10-18`). It must not name the serial context.

### U1. Undo Step 1: `writeFromDevice` is private again
File: `jam_terminal/processor/jam_TerminalFIFO.h`.
1. Move `private:` back above the `writeFromDevice` block. This is the reverse of the git diff: delete the `private:` at the new line after `return numRead; }`, and insert `private:` before the `writeFromDevice` doxygen block (diff hunks `@@ -163,7 +163,6 @@` and `@@ -194,6 +193,7 @@`).
2. Undo U1 only after U3.4. Before that, `jam_TerminalConsoleOutput_windows.cpp:360` still calls it.

### After the undo
1. Regenerate the JAM doxygen. Fix each block that names a deleted member.
2. Run `scout D:/Poems/dev/eve/benchmark/seq-10k-windows-server-time.md`. Compare with the 17:20 run: 10k epoch 6.62 s.
3. Delete the rows of this PLAN that the undo removes: the Copies table, the Thread ownership table, and the Names rows `clientQueue`, `addClientInput`, `echoOffset`, `numInterrupts` and `isServerOpen`.
