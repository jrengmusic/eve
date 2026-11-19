# PLAN: Cygwin Relay — the helper drains the msys2 pty in its own rhythm

**RFC:** none — objective from ARCHITECT prompt (*"synthsize BLESSED approach to eliminate bottleck, hop, roundtrip, lost time, etc then write comprehensive executable plan"*)
**Date:** 2026-11-18
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 / JAM, MSVC build of `eve-cygwinPTY` that calls the msys2 runtime through `Runtime` function pointers (LANGUAGE.md C++/JUCE: one object, one translation unit)

**Plan file at lock:** plan mode permits only this file. The first action after the lock copies this plan, unchanged, to `D:\Poems\dev\eve\PLAN-cygwin-relay.md`.

## Overview

This is the second plan of the sprint of DEBT-20261117T223541 (cygwin output comes in pages). The helper `eve-cygwinPTY` reads the msys2 pty master 256 bytes at a time. It flushes each read to EVE with a blocking `WriteFile`. When the pipe is empty, it waits in the runtime's fixed 10 ms poll. This plan makes the helper wait on the runtime's readiness call (`poll`), then read the master until it is empty, then flush once. The runtime stays the only reader of the master, so echo, held input and end of stream keep their msys2 contract. The history writer debt (DEBT-20261117T223545) is not in this plan.

## Context (read and measured this session)

### ARCHITECT rulings (verbatim)

1. *"we follow what the system constraints so no other seam ever wait."*
2. *"we dont set any hardcoded `value`. the value is what seam beyond our control capped, then we make our clock dynamically follow its `rhythm`."*
3. *"FULL THROTTLE genuinely lossless full drain always, lock free at hot thread, any thread at tty beyond ours becomes seamless integration, we never choke them, we also never let them starve. no yield, no wait"*
4. *"we dont fight window, we dont fight msys2-cygwin, we dont fight juce, we empower our architecture with JAM to its fullest extent. probably the bulding blocks already exists too."*
5. *"READ, RESEARCH if you need to. dont leave any gaps for inference, no room for assumptions. then synthsize BLESSED approach to eliminate bottleck, hop, roundtrip, lost time, etc"*

### The chain today, one hop at a time

1. **seq writes to the pty pipe.**
   - Each write takes the output mutex (`pty.cc:1236`).
   - It writes pieces of at most `OUT_BUFFER_SIZE` = 256 bytes (`pty.cc:3448`, `:3462`; `tty.h:14`).
   - Before each piece it queries the pipe (`pty.cc:3418`).
   - When the pipe is full, it sleeps 1 ms (`pty.cc:3438-3441`).
   - This code runs in seq's process. ARCHITECT measured `0.53s user 3.22s system 73% cpu 5.098 total` for 1M lines.
2. **The pty pipe** `pty%d-to-master` holds 128 KiB (`pty.cc:3205-3207`; `fhandler.h:2404`). Its read end is a synchronous handle: open mode 0, no `FILE_FLAG_OVERLAPPED` (`pipe.cc:847`, `:882-883`).
3. **The helper reads the master** (`Session.cpp:188-199`). Each `runtime.read` goes through `fhandler_pty_master::read` (`pty.cc:2462-2472`) to `process_slave_output` (`pty.cc:617-729`). That call:
   - sends held input (`flush_to_slave`, `pty.cc:398-405`);
   - reads the echo pipe first when it has bytes (`pty.cc:636`, `:691-698`);
   - returns at most 256 bytes (`pty.cc:686-687`);
   - sleeps 10 ms on an empty pipe in blocking mode (`pty.cc:659`);
   - returns `EAGAIN` on an empty pipe in non-blocking mode (`pty.cc:652-656`);
   - returns `EIO` at the end of the slave (`pty.cc:642-646`, `hit_eof` `pty.cc:601-611`).
4. **The helper writes to EVE once per read.**
   - `StdioServer::writeFromDevice` flushes after each call (`jam_StdioServer.h:73-79`).
   - The flush makes one `WriteFile` and waits for it (`jam_TerminalFIFO_windows.cpp:76-87`).
5. **EVE reads its pipe in the event-driven way.**
   - The readiness source uses a zero-byte overlapped read and a thread-pool wait, with a clock at 128 KiB (`jam_AsyncOutputStream_windows.cpp:105-114`; `jam_AsyncUpdater.cpp:225`, `:264`; `jam_TerminalCygwinTTY_windows.cpp:42-46`).
   - Each tick held about 22 KB, below 128 KiB (`benchmark/seq-1m-windows-cygwin-capacity-report.md`).

### Why the runtime stays the only reader

1. **Echo** in ECHO mode goes to a second pipe, `echo_r` (`termios.cc:625-626`; `pty.cc:475-482`). Only `process_slave_output` reads it. No exported call returns its handle (`fhandler.h:2578`, `:2589`; `pty_master_thread` duplicates six other handles, `pty.cc:2665-2810`).
2. **Held input:** `accept_input` keeps the rest of the input when the input pipe is full (`pty.cc:585-589`). The master read sends it later (`pty.cc:398-405`).
3. **End of stream has no event.**
   - A slave close signals nothing to the master (`pty.cc:1018-1043`).
   - The master holds its own write end `to_master` until its close (`fhandler.h:2580`; `pty.cc:2179-2180`), so the pipe never breaks while the master is open.
   - Only the `inuse` event that the slave holds tells the end (`pty.cc:817`; `tty.cc:189-196`). The runtime tests it in its read and in its select peek.
4. **A second thread on the read handle can block the runtime.** The handle is synchronous (`pty.cc:3207`). Microsoft documents that PeekNamedPipe *"can block thread execution the same way any I/O function can when called on a synchronous handle in a multi-threaded application"* (learn.microsoft.com, PeekNamedPipe, Remarks). msys2 makes its other pty pipes overlapped *"in order to prevent PeekNamedPipe() from blocking in transfer_input()"* (`pty.cc:3216-3221`). Microsoft does not document a zero-byte synchronous read (Librarian report, Q1).
5. **JUCE `NamedPipe` does not fit.** Its buffers are fixed at 4096 bytes (`juce_Files_windows.cpp:1096-1098`), and its `read` blocks with a timeout (`:1179-1210`). It cannot open the pty pipe, which allows one instance (`pipe.cc:847`, `:882-883`).

### The runtime's own readiness call

1. `poll` is exported (`cygwin.din:1045`). It runs `cygwin_select` (`poll.cc:83-84`). A negative timeout waits with no limit (`poll.cc:83-84`; `select.cc:351`).
2. For a pty master, the select record installs `peek_pipe` and the `pipesel` thread (`select.cc:1337-1352`, `:829-852`).
   - `peek_pipe` sends held input (`select.cc:719-722`).
   - It reports the pty pipe and the echo pipe (`select.cc:740-743`).
   - It reports the end of the slave (`select.cc:769-776`).
3. The thread polls with a delay that starts at 0 ms and rises to 10 ms (`select.cc:797-827`: `cygwait (pi->bye, sleep_time >> 3)`, `sleep_time` from 0 to 80). The blocking read polls at a fixed 10 ms (`pty.cc:659`).
4. Record and flag: `struct pollfd { int fd; short events; short revents; }`, `typedef unsigned int nfds_t`, `POLLIN 1` (`sys/poll.h:18`, `:33-39`).

### Non-blocking mode and errno

1. `ioctl (master, FIONBIO, &value)` reaches `fhandler_base::ioctl`, which calls `set_nonblocking` (`pty.cc:2512-2516`, `:2555-2556`; `fhandler_termios::ioctl` returns 1 for any request except `TIOCSCTTY`, `termios.cc:743-746`; `base.cc:1352-1355`). `FIONBIO` is `0x8004667e` (`asm/socket.h:27`; `sys/termios.h:72`). `Runtime` already binds `ioctl` (`Runtime.h:48`).
2. The flag changes only the empty-pipe branch of the master read (`pty.cc:652-656`). The master write passes it only to the echo write, where `is_echo` skips the branch that reads it (`pty.cc:475-482`, `:3412-3444`). The helper sets it once, before the relays open, so no thread changes it while another thread reads it.
3. `__errno` is exported (`cygwin.din:60`). It returns `int*` (`newlib/libc/errno/errno.c:14-18`). `EAGAIN` is 11 and `EINTR` is 4 (`newlib/libc/include/sys/errno.h:47`, `:40`). The MSVC CRT defines `errno` as a macro (`ucrt/errno.h:24`), so the binding takes another name.

### What this plan removes, and what stays

| Hop | Today | After |
|---|---|---|
| Helper to EVE | one `WriteFile` per read of 256 bytes or less | one `WriteFile` per drain, up to the ring capacity of 128 KiB |
| Helper wait on an empty pipe | fixed 10 ms (`pty.cc:659`) | the select thread: 0 ms at first, rising to 10 ms (`select.cc:820-822`) |
| End-of-stream detection | the blocking read's poll | the select peek (`select.cc:769-776`) |
| seq's writes, 256 bytes each | unchanged: in seq's process | unchanged |
| The runtime read unit, 256 bytes | unchanged: `pty.cc:686-687` | unchanged |

## Language / Framework Constraints

- No comment and no doxygen before the audit (CAROL.md Code Hygiene). Step 6 is the doxygen pass.
- The helper calls the runtime only through `Runtime` pointers bound by name (`Runtime.cpp:16-31`). A runtime record crosses the boundary as a mirror struct in `Runtime.h`, as `Winsize` does (`Runtime.h:9-15`).
- No change to jam and no change to EVE source. EVE embeds the helper zip, so the measure step rebuilds EVE after the helper (`benchmark/seq-1m-windows-cygwin-capacity.md:27-37`).
- Builds run only through cast, inside the scout run (profile skill, Builds).

## Framework Inventory

### API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| Readiness of the master: pty bytes, echo, end | msys2 runtime | `poll (struct pollfd*, nfds_t, int)` | `cygwin.din:1045`; `poll.cc:24-139`; `select.cc:697-793`, `:797-827`, `:1337-1352` | 1, 2 |
| Non-blocking master read | msys2 runtime | `ioctl (fd, FIONBIO, int*)` | `pty.cc:2512-2516`, `:2555-2556`; `base.cc:1352-1355`; `asm/socket.h:27` | 2 |
| Empty pipe tells `EAGAIN` | msys2 runtime | `read` in non-blocking mode | `pty.cc:652-656` | 2 |
| End of stream tells `EIO` | msys2 runtime | `read` | `pty.cc:642-646` | 2 |
| Error number of the calling thread | msys2 runtime | `__errno ()` returns `int*` | `cygwin.din:60`; `errno.c:14-18` | 1, 2 |
| Bind a runtime function by name | JUCE | `juce::DynamicLibrary::getFunction<Signature> (name)` | `Runtime.cpp:16-31` | 1 |
| Read a device into the ring, publish, flush once | jam_terminal | `StdioServer::writeFromDevice (Callback)` | `jam_StdioServer.h:73-79`; `jam_TerminalFIFO.h` `writeFromDevice` | 2 |
| One flush to EVE | jam_terminal | `TerminalFIFO::writePendingData (Handle)` | `jam_TerminalFIFO_windows.cpp:76-87` | 2 |
| Stop the helper's message loop | JUCE | `juce::MessageManager::getInstance()->stopDispatchLoop()` | `Session.cpp:202` | 2 |
| Measurement | scout | manifest `## run`, `## profiler`, `## toolchain` | `benchmark/seq-1m-windows-cygwin-capacity.md` | 3 |

### Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Runtime record as a mirror struct in `Runtime.h` | `struct Winsize` (`Runtime.h:9-15`) | `Session.cpp:47`, `:159-160`, `:174` | 1 |
| Runtime function as a member pointer, bound by name, checked in `isLoaded` | `ioctl` (`Runtime.h:48`; `Runtime.cpp:17`, `:36`) | every `Runtime` member (`Runtime.h:47-62`) | 1 |
| Runtime request code as a `static constexpr` local of its one using function in `Session.cpp` | `setControllingTerminal` in `openShell`, `noWait` in `spawnShell`, `hangupSignal` in `applyHangup`, `setWindowSize` in `applySizes` | `Session.cpp:37-40`, `:73-74`, `:145`, `:166`, `:186-191` | 2 |

**[CORRECTED 2026-11-18]** The row above first read "file-scope `static constexpr` in `Session.cpp`" (`Session.cpp:3-6`). That form broke CODING.md: *"A value or type used by one function lives inside that function"*. Auditor finding 4 named it. Each constant has one using function, so each constant is now a local of that function.
| The reader thread is the only reader of the master | `Session::run` (`Session.cpp:181-203`), thread from `pthreadCreate` (`Session.cpp:19-24`) | `~Session` joins it (`Session.cpp:29`) | 2 |
| A device read fills the ring through a callback | `readMaster` (`Session.cpp:188-194`) | `stdio.writeFromDevice` | 2 |
| Read the device until it is empty, then end the tick | SPEC Feature 2: *"Each clock tick reads the PTY until it is empty"* (`SPEC.md:368-371`) | macOS readiness source on the master descriptor (`jam_AsyncUpdater.cpp` `Source::Native`, `DISPATCH_SOURCE_TYPE_READ`) | 2 |

### Names

| Family | Members | Intent | Step |
|---|---|---|---|
| Runtime function pointers, named for the bound C function | `openpty`, `ioctl`, `read`, `write`, `close`, `cygwinInternal` (`cygwin_internal`), `pthreadCreate` (`pthread_create`) (`Runtime.h:47-62`) | camelCase of the runtime's name | 1 |
| Runtime mirror records | `Winsize` (runtime `struct winsize`), fields `rows`, `cols`, `pixelWidth`, `pixelHeight` (`Runtime.h:9-15`) | PascalCase of the runtime's record; semantic field names | 1 |
| Session request constants | `setControllingTerminal`, `setWindowSize`, `noWait`, `hangupSignal`, `ignoreAction`, `getWindowsProcessId` (`Session.cpp:3-9`) | the meaning of the runtime value | 2 |
| Session reader lambda | `readMaster` (`Session.cpp:188`) | reads the master into the ring | 2 |

New names (each passes NAMES.md Rule -1):
- `poll`: `Runtime` member that binds `poll`. Family: runtime function pointers.
- `errorNumber`: `Runtime` member that binds `__errno`. The family form `errno` is a macro in the MSVC CRT (`ucrt/errno.h:24`). POSIX calls the value the error number.
- `Pollfd`, with the fields `descriptor`, `requestedEvents`, `returnedEvents`: the mirror of `struct pollfd` (`sys/poll.h:33-37`). Family: `Winsize`.
- `setNonBlocking { static_cast<int> (0x8004667eu) }`: `FIONBIO`, *"set/clear non-blocking i/o"* (`asm/socket.h:27`). Family: `setWindowSize`.
- `dataToRead { 1 }`: `POLLIN`, *"Set if data to read."* (`sys/poll.h:18`).
- `noTimeout { -1 }`: a negative `poll` timeout waits with no limit (`poll.cc:83-84`). Family: `noWait`.
- `wouldBlock { 11 }`: `EAGAIN` / `EWOULDBLOCK`, *"Operation would block"* (`errno.h:47`, `:191`).
- `interruptedCall { 4 }`: `EINTR`, *"Interrupted system call"* (`errno.h:40`).
- `numPolled { 1u }`: the count of `Pollfd` records in the call, type `unsigned int` (`sys/poll.h:39`).

### Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "Constants over magic numbers" | A runtime value is a named `static constexpr` local of its one using function (CODING.md "Narrowest scope that holds every use"). [CORRECTED 2026-11-18: first read "file-scope", `Session.cpp:3-10`] | `Session.cpp:186-191` |
| "No bail-out guards" | Positive nesting in `run` | `Session.cpp:183-200` |
| "Use the framework API fully" | Runtime calls go through `Runtime`; the ring and the flush go through `StdioServer` | `Session.cpp:190-193` |
| "Deadline threads" | The helper's reader thread is not a clock thread. Its only wait is the runtime's `poll` | `Session.cpp:19-24` |
| "Aggregate (brace) initialization" | `Pollfd readiness { master, dataToRead, 0 };` | `Session.cpp:3-10` |

### Absence

| Need | Query | Result |
|---|---|---|
| A supported handle to the echo pipe | `fhandler.h:2578`, `:2589`; `pty_master_thread` (`pty.cc:2665-2810`) | Private member. The control thread does not duplicate it |
| A master-side event at the slave's end | `fhandler_pty_slave::close` (`pty.cc:1018-1043`); `tty::slave_alive` (`tty.cc:189-196`) | None. The end is a test of the `inuse` event |
| A JAM or JUCE readiness source for a Cygwin descriptor | `AsyncOutputStream` takes a Windows `HANDLE` (`jam_AsyncOutputStream.h:28`, `:51`); JUCE `NamedPipe` (`juce_Files_windows.cpp:1085-1285`) | None. The runtime's `poll` is the readiness call |
| `pthread_getw32threadhandle_np` export | `cygwin.din` grep | NOT FOUND |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final code step, covering the whole sprint — never per step.

## Steps

### Step 1: Runtime binds `poll` and `__errno`
**Scope:** `eve/cygwinPTY/Source/Runtime.h`, `eve/cygwinPTY/Source/Runtime.cpp`.
**Action:**
1. `Runtime.h`: after `struct Winsize`, add:
```cpp
struct Pollfd
{
    int descriptor;
    short requestedEvents;
    short returnedEvents;
};
```
2. `Runtime.h`: after `pthreadJoin`, add:
```cpp
    int (*poll) (Pollfd*, unsigned int, int) { nullptr };
    int* (*errorNumber) () { nullptr };
```
3. `Runtime.cpp`: after the `pthreadJoin` binding, add:
```cpp
    poll = library.getFunction<int (Pollfd*, unsigned int, int)> ("poll");
    errorNumber = library.getFunction<int* ()> ("__errno");
```
4. `Runtime::isLoaded`: append `and poll != nullptr and errorNumber != nullptr`.
**Inventory:** API rows 1, 5, 6; pattern rows 1, 2; names `Pollfd`, `poll`, `errorNumber`.
**Validation:**
- The field order and types of `Pollfd` match `sys/poll.h:33-37`.
- Each binding name is the exported name (`cygwin.din:1045`, `:60`).
- No other member changes.
- No comment and no doxygen.

### Step 2: The reader thread waits on `poll`, drains the master, flushes once
**Scope:** `eve/cygwinPTY/Source/Session.cpp` only: file-scope constants (`:3-10`) and `Session::run` (`:181-203`).
**Action:**
1. Delete first: the `readMaster` lambda (`:188-194`) and the loop (`:196-199`).
2. Add file-scope constants after `standardHandleIds` (`:10`): `setNonBlocking`, `dataToRead`, `noTimeout`, `wouldBlock`, `interruptedCall`, `numPolled`, with the values in the Names table.
3. In `run`, inside `if (openShell())`, before `openRelays()`: set the master non-blocking once.
```cpp
        int isNonBlocking { 1 };
        runtime.ioctl (master, setNonBlocking, &isNonBlocking);
```
4. In `run`, after the relays open, the reader:
   - The callback `readMaster (char* data, int maxBytes)` calls `runtime.read (master, data + numRead, maxBytes - numRead)` until a read returns 0 or less, or the ring is full.
   - It returns the bytes read when that count is above 0.
   - With no bytes read, it returns 0 when the last read gave -1 with `*runtime.errorNumber() == wouldBlock`. It returns -1 in all other cases: a read of 0, or -1 with any other error.
   - The loop: `Pollfd readiness { master, dataToRead, 0 };`. While the last drain result is 0 or more:
     - `poll (&readiness, numPolled, noTimeout)` above 0 → `numRead = stdio.writeFromDevice (readMaster);`
     - `poll` of -1 with `*runtime.errorNumber() == interruptedCall` → poll again.
     - Any other `poll` result ends the loop.
5. `juce::MessageManager::getInstance()->stopDispatchLoop();` stays the last statement of `run` (`:202`).
**Inventory:** API rows 1-5, 7-9; pattern rows 3-6; names of Step 2; synthesis rows 1-5.
**Validation:**
- Read `Session.cpp` at the changed lines.
- The master is set non-blocking before `openRelays`, so no thread changes the flag while another thread reads it.
- The loop ends only at the end of the master (a read of 0, or -1 with an error other than `EAGAIN`) or on a `poll` failure other than `EINTR`. A full ring does not end the loop.
- One `writeFromDevice` per `poll` wake, so one flush per drain (`jam_StdioServer.h:73-79`).
- No lock, no sleep, no wait other than `poll`.
- Branches: 3 or fewer per function. Lines: 30 or fewer per function.
- No change outside the scope. No comment and no doxygen.

### Step 3: Measure
**Scope:** new `eve/benchmark/seq-1m-windows-cygwin-relay.md` (COUNSELOR writes it).
**Action:**
1. The manifest copies `benchmark/seq-1m-windows-cygwin-capacity.md`, with these changes:
   - `samples` = `seq-1m-windows-cygwin-relay-samples.md`;
   - `output` = `seq-1m-windows-cygwin-relay-report.md`;
   - `baseline` = `seq-1m-windows-cygwin-capacity-report.md`.
   - The `## toolchain` rows stay: jam, then `../cygwinPTY/cast/spell.md`, then EVE with `--standalone-profile-symbols`.
2. @Pathfinder runs `scout D:/Poems/dev/eve/benchmark/seq-1m-windows-cygwin-relay.md`, with a 15-minute watchdog.
3. COUNSELOR reads these values against the baseline:
   - `tty bytes ready` calls, and the bytes per tick;
   - `swap history lines`;
   - `launch prompt`;
   - `epoch`.
**Validation:**
- Exit 0 and a Release build.
- The `Source/windows` helper zip has the time of this run.

### Step 4: SPEC amendment
**Scope:** `eve/SPEC.md` principle 10 (`:138-172`).
**Action:** COUNSELOR adds an `[AMENDED 2026-11-18]` block in the amendment form of `:149`. Text:
- The cygwin helper reads the pty master only through the runtime.
- It waits on the runtime's `poll` and reads the master until it is empty.
- It then writes the drain to EVE in one write.
- The runtime's read unit (256 bytes) and its select delay are the seam beyond EVE's control.
- ARCHITECT rulings 3, 4 and 5 stand beside it, verbatim.
**Validation:** each line carries a verbatim quote. No line states behaviour that Step 2 does not have.

### Step 5: Auditor
**Scope:** every file of Steps 1-2.
**Action:** @Auditor validates against MANIFESTO.md, NAMES.md, CODING.md and this plan. COUNSELOR reads each cited file:line, names the clause, and resolves each confirmed finding in this sprint.

### Step 6: Doxygen pass
**Scope:** `Runtime.h` (`Pollfd` and its fields, `poll`, `errorNumber`); `Session.h` (`run()` at `:36-44`: the wait on `poll`, the drain until `EAGAIN`, one flush per drain, the end conditions).
**Action:** @Pathfinder writes the pass on COUNSELOR's exact targets (doxygen-writer skill). COUNSELOR reads each block against the code.

### Step 7: Report
**Action:** COUNSELOR reports the Step 3 values against the baseline in chat. ARCHITECT runs the interactive `seq` test.

## BLESSED Alignment

- **B:** the reader thread stays the only reader of the master, and `~Session` still joins it (`Session.cpp:29`). The runtime keeps its own handles. EVE owns no new thread.
- **L:** one struct, two bindings and six constants. One callback and one loop in `run`.
- **E:** each runtime value is named, and its source header line is cited. The loop ends on the runtime's own end signals.
- **S (SSOT):** echo, held input and the end of stream stay in the runtime (`pty.cc:617-729`). The helper keeps no second copy of their state.
- **S (Stateless):** no new member. The non-blocking flag is the runtime's own state, set once.
- **E (Encapsulation):** the helper calls only exported runtime functions. No handle of the runtime is touched.
- **D:** the same bytes reach EVE in the same order. Only the count of writes and the empty-pipe wait change.
- **Deadline thread:** no change to EVE's tick or clock.

## Risks / Open Questions

None open.
