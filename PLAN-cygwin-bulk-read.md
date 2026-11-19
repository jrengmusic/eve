# PLAN: Cygwin Bulk Read — the helper drains the msys2 pty pipe in one read per wake

**RFC:** none — objective from ARCHITECT prompt (*"profile, pinpoint where the time goes, which seam still waits, why our clock still not synced to the rhythm, then synthesize the solution"*; *"measure. facts only. never leave any gaps for inference and assumptions"*; `/goplan`)
**Date:** 2026-11-18
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 / JAM, MSVC build of `eve-cygwinPTY` that calls the msys2 runtime through `Runtime` function pointers (CODING.md: one object, one translation unit)

**Plan file at lock:** plan mode permits only this file. The first action after the lock copies this plan, unchanged, to `D:\Poems\dev\eve\PLAN-cygwin-bulk-read.md`.

## Overview

This is the third plan of the sprint of DEBT-20261117T223541 (cygwin output comes in pages). The helper's reader thread is saturated in the runtime master read, which moves at most 256 bytes per call. seq sleeps on the full pipe. This plan keeps one runtime read per drain pass, for echo, held input and end of stream. It then reads the rest of the pty pipe in one `ReadFile`, through the runtime's own handle (`_get_osfhandle`) and the JAM pipe read (`jam::TerminalFIFO::read`).

## Context (measured and read this session)

### ARCHITECT rulings (verbatim)

1. *"we follow what the system constraints so no other seam ever wait."*
2. *"FULL THROTTLE genuinely lossless full drain always, lock free at hot thread, any thread at tty beyond ours becomes seamless integration, we never choke them, we also never let them starve. no yield, no wait"*
3. *"we dont fight window, we dont fight msys2-cygwin, we dont fight juce, we empower our architecture with JAM to its fullest extent. probably the bulding blocks already exists too."*
4. *"profile, pinpoint where the time goes, which seam still waits, why our clock still not synced to the rhythm, then synthesize the solution"*
5. *"measure. facts only. never leave any gaps for inference and assumptions"*

### Measurements

1. **ARCHITECT's runs, Release, after the relay change.** `seq 1 1000000` five times: 69 %, 71 %, 76 %, 74 % and 73 % CPU, 4.770 s to 5.208 s total.
2. **The helper that runs is the new build.**
   - `sha256sum` of `~/.config/end/shell/eve-cygwinPTY.exe` and of the Release artefact: both `d2e4af5f85bb87b051b690a28d2048254c73071c9fdb30baeddb1dc749c1adeb`.
   - The stamp `.cygwin-pty-stamp` holds `fe7534e9…adea`. That value is `CAST_CYGWIN_PTY_HASH` at `Builds/ProfileSymbols/build.ninja:112`.
3. **Helper reader thread**, whatdbg sample, 33 samples, 50M run:
   - `KERNELBASE!PeekNamedPipe` 48.5 %. `ntdll!NtCreateEvent` (15.2 %) is a frame inside it (`out-helper.txt:550-551`).
   - `ntdll!NtReadFile` 39.4 %.
   - `ZwWaitForMultipleObjects` as the leaf frame: 3.0 %. The thread almost never waits.
   - The runtime `pipesel` thread: `NtWaitForSingleObject` 97 %.
4. **seq main thread**, 36 samples:
   - `ZwWaitForMultipleObjects` 33.3 %;
   - `NtQueryInformationFile` 25.0 %;
   - `NtWriteFile` 16.7 %;
   - `NtReleaseMutant` 11.1 %;
   - `NtWaitForSingleObject` 11.1 %.
5. **EVE, per 1M epoch** (`seq-1m-windows-cygwin-relay-report.md`, median of 3):
   - `tty bytes ready`: 272 calls (`:85`).
   - `terminal read`: 621 calls, 13.7 ms in total (`:77`).
   - seq writes 6,888,896 bytes for 1M lines, without the terminal's `\r` per line. That is at least 25,327 bytes per tick.
   - EVE's ring holds 131,072 bytes: the helper's command-line argument `131072`.

### The code paths (read this session)

1. **The runtime master read gives at most 256 bytes per call** (`tty.h:14`; `pty.cc:620`, `:686-687`). Each call does this work:
   - `bg_check` (`pty.cc:2464`);
   - `flush_to_slave`, which locks and unlocks the input mutex (`pty.cc:625`, `:401-404`);
   - a peek of the echo pipe (`pty.cc:636`);
   - a peek of the pty pipe (`pty.cc:638`, `:365`);
   - one `ReadFile` (`pty.cc:699`).
2. **seq's write path has two waits** (`pty.cc:1224-1255`, `:3400-3509`):
   - `WaitForSingleObject` on the output mutex (`pty.cc:431`);
   - `cygwait (1)` when `WriteQuotaAvailable == 0` (`pty.cc:3438-3440`). It reaches `WaitForMultipleObjects` (`cygwait.cc:79`).

   seq is in the foreground group, so `bg_check` returns early (`termios.cc:217`).
3. **The seq wait frame.**
   - The runtime DLL has no symbols on disk: Glob found only `C:\msys64\usr\bin\msys-2.0.dll`.
   - The 33.3 % wait frame sits one runtime frame below the frames of the pipe query and of the mutex wait. The frames above it are the same (`out-seq.txt:35-49`, `:66-80`, `:96-112`). That extra frame matches the out-of-line `cygwait` (`cygwait.cc:27`).
   - No other `WaitForMultipleObjects` is in the write path of item 2.
4. **The runtime gives its pipe handle.**
   - `_get_osfhandle` is exported (`cygwin.din:144`; `include/io.h:19`, `extern long _get_osfhandle(int)`).
   - It returns `cfd->get_handle ()` (`syscalls.cc:3256-3258`).
   - For the master, that handle is the pipe that the runtime peeks and reads (`pty.cc:383`, `:699`).
   - The runtime's width environment is LP64 (`sysconf.cc:679`, `"POSIX_V6_LP64_OFF64"`), so `long` holds a 64-bit handle.
5. **Who touches that handle in the helper:**
   - the runtime read and peek on the reader thread (`pty.cc:383`, `:699`);
   - the `pipesel` thread, only while `poll` runs. `poll` stops that thread before it returns (`select.cc:862-864`).
   - The forward thread reads `from_slave_nat`, a different handle (`pty.cc:2862`).
   - Echo writes go to `echo_w` (`pty.cc:479`).
   - `FIONREAD` peeks the handle (`pty.cc:2544-2547`). The helper does not call `FIONREAD`.
6. **JAM reads a pipe with what is available.**
   - `jam::TerminalFIFO::read (AsyncOutputStream::Handle, char*, int)` peeks, then reads `min (available, maxBytes)` in one `ReadFile` (`jam_TerminalFIFO_windows.cpp:50-72`).
   - It gives the bytes read, 0 when none are ready, or -1 when the peek fails (`jam_TerminalFIFO.h:139-151`).
   - `AsyncOutputStream::Handle` is `HANDLE` on Windows (`jam_AsyncOutputStream.h:29`; `jam_AsyncUpdater.h:79-81`).
7. **Discard mode.**
   - With `FLUSHO` set, the runtime read discards the bytes it reads (`pty.cc:707-708`), and the slave writes nothing (`pty.cc:3407-3408`).
   - `FLUSHO` toggles on `VDISCARD` (`termios.cc:507`).
   - In this plan, the runtime read comes first in each pass. Under `FLUSHO`, it discards until the pipe is empty and gives `EAGAIN` (`pty.cc:652-656`). The bulk read then has no bytes to read.
   - One case differs from the runtime: `VDISCARD` arrives between the runtime read and the bulk read of one pass. That bulk read gives its bytes; the runtime would discard them.
8. **Echo backlog.** [CORRECTED 2026-11-18, Auditor finding 1. Item 7 first said "One case differs", and BLESSED Alignment D said "echo comes before pty output in each pass, as in the runtime". Both missed this case.]
   - The runtime checks the echo pipe first on every call, and gives at most 256 bytes per call (`pty.cc:635-637`, `:686-687`, `:691-693`). Thus the runtime alone gives a whole echo backlog before any pty output.
   - The pass reads the pty pipe in bulk after every positive runtime read (`Session.cpp:212-213`). With more than 256 bytes of echo waiting, the pass gives 256 bytes of echo, then the pty pipe, then the next echo bytes.
   - The helper has no handle to `echo_r` (`fhandler.h:2578`, `:2589`). No code change keeps echo first.

### What this plan removes, and what stays

| Hop | Today | After |
|---|---|---|
| Drain of the pty pipe | one runtime read per 256 bytes, 5 or more kernel calls each | one runtime read, then one peek and one `ReadFile` for the rest, up to the free ring space |
| seq's full-pipe sleep (`pty.cc:3438-3440`) | 33.3 % of seq samples | the measure step counts it again |
| Echo, held input, end of stream | the runtime read | unchanged: the runtime read, first in each pass |
| seq's own write: query, write, mutex per 256 bytes (`pty.cc:3418`, `:3448-3450`, `:1236`) | in seq's process | unchanged, beyond EVE's control |

## Language / Framework Constraints

- No comment and no doxygen before the audit (CAROL.md Code Hygiene). Step 7 is the doxygen pass.
- The helper calls the runtime only through `Runtime` pointers bound by name (`Runtime.cpp:16-33`).
- No change to jam and no change to EVE source. EVE embeds the helper zip, so the measure step rebuilds EVE after the helper (manifest `## toolchain`).
- Builds run only through cast, inside the scout run (profile skill, Builds).

## Framework Inventory

### API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| The Windows handle of the master's pty pipe | msys2 runtime | `long _get_osfhandle (int)` | `cygwin.din:144`; `include/io.h:19`; `syscalls.cc:3252-3264` | 1, 2 |
| Bind a runtime function by name | JUCE | `juce::DynamicLibrary::getFunction<Signature> (name)` | `Runtime.cpp:16-33` | 1 |
| Read what a pipe has ready, in one call, with no wait | jam_terminal | `static int jam::TerminalFIFO::read (AsyncOutputStream::Handle, char*, int)` | `classjam_1_1TerminalFIFO.xml:559`; `jam_TerminalFIFO.h:139-151`; `jam_TerminalFIFO_windows.cpp:50-72` | 2 |
| Echo first, held input, end of stream, empty | msys2 runtime | `read` in non-blocking mode | `pty.cc:625`, `:636`, `:642-646`, `:652-656` | 2 |
| Read the device into the ring, flush once | jam_terminal | `StdioServer::writeFromDevice (Callback)` | `jam_StdioServer.h:74-80` | 2 |
| Measurement | scout | manifest `## run`, `## profiler`, `## toolchain` | `benchmark/seq-1m-windows-cygwin-relay.md` | 3 |
| Where lane | whatdbg | `sample.lua`, attach by pid | trace skill, Spec — sample | 3 |

### Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Runtime function as a member pointer, bound by name, checked in `isLoaded` | `cygwinInternal` (`Runtime.h:72`; `Runtime.cpp:29`, `:39`) | every `Runtime` member (`Runtime.h:59-76`) | 1 |
| A runtime handle value as `uintptr_t` | `uintptr_t (*cygwinInternal) (int, ...)` (`Runtime.h:72`) | `Session.cpp:60` | 1 |
| An integer to `HANDLE` through `uintptr_t` | `reinterpret_cast<HANDLE> (static_cast<uintptr_t> (…))` (`Main.cpp:36`) | `jam_TerminalCygwinTTY_windows.cpp:85` (the reverse) | 2 |
| Read a pipe handle with `TerminalFIFO::read` | `TerminalCygwinTTY::read` → `TerminalFIFO::read (output.get(), buf, maxBytes)` (`jam_TerminalCygwinTTY_windows.cpp:128-131`) | `jam_TerminalFIFO.cpp:91` | 2 |
| A device read fills the ring through a callback | `readMaster` (`Session.cpp:201-215`) | `stdio.writeFromDevice` (`Session.cpp:223`) | 2 |
| A runtime value is a `static constexpr` local of its one using function | `Session.cpp:186-191` | `Session.cpp:37-40`, `:73-74`, `:145`, `:166` | 2 |

### Names

| Family | Members | Intent | Step |
|---|---|---|---|
| Runtime function pointers, camelCase of the bound C name | `openpty`, `ioctl`, `read`, `write`, `cygwinInternal` (`cygwin_internal`), `pthreadCreate` (`pthread_create`), `poll` (`Runtime.h:59-75`) | the runtime function it binds | 1 |
| Session pipe handles | `sizePipe` (`Session.h:121`) | a Windows pipe that the session reads | 2 |
| Session reader lambda | `readMaster` (`Session.cpp:201`) | reads the master into the ring | 2 |

New names (each passes NAMES.md Rule -1):
- `getOsfhandle`: the `Runtime` member that binds `_get_osfhandle`. Family: runtime function pointers. `get` returns and stores nothing (NAMES.md Rule 1, Verb Contract).
- `masterPipe`: the local `HANDLE` in `Session::run`, the pipe that the master reads (`pty.cc:383`, `:699`). Family: `sizePipe`.

### Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "Use the framework API fully" | A pipe read uses `jam::TerminalFIFO::read`; the ring and the flush use `StdioServer` | `jam_TerminalCygwinTTY_windows.cpp:130`; `Session.cpp:223` |
| "Owner first, narrowest scope" | A value of one function is a local of that function | `Session.cpp:186-191` |
| "No bail-out guards" | Positive nesting inside `readMaster` | `Session.cpp:206-214` |
| "Aggregate (brace) initialization" | `const auto name { … };` | `Session.cpp:213` |
| `reinterpret_cast` "uintptr_t ↔ pointer round-trips" (CODING.md) | `reinterpret_cast<HANDLE> (uintptr_t)` | `Main.cpp:36` |

### Absence

| Need | Query | Result |
|---|---|---|
| A runtime call that reads more than 256 bytes of the master | `tty.h:14`; `process_slave_output` (`pty.cc:617-729`) | None. `rlen` caps at `sizeof outbuf` = 256 (`pty.cc:686-687`) |
| Symbols of `msys-2.0.dll` | Glob `**/msys-2.0.{dbg,pdb,dll}` under `C:\msys64\usr`; Glob `**/*.{dbg,map}` under `msys2-runtime` | `msys-2.0.dll` only; no `.dbg` |
| An integer-to-`HANDLE` cast in jam | Pathfinder grep `reinterpret_cast<(HANDLE\|void\*)>\s*\(` over `D:/Poems/dev/jam` | One hit: a function pointer to `void*` (`jam_ENAXClientExtensions.cpp:72`). The precedent is `Main.cpp:36` |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final code step, covering the whole sprint — never per step.

## Steps

### Step 1: Runtime binds `_get_osfhandle`
**Scope:** `eve/cygwinPTY/Source/Runtime.h`, `eve/cygwinPTY/Source/Runtime.cpp`.
**Action:**
1. `Runtime.h`: after `errorNumber` (`:76`), add `uintptr_t (*getOsfhandle) (int) { nullptr };`.
2. `Runtime.cpp`: after the `errorNumber` binding (`:33`), add `getOsfhandle = library.getFunction<uintptr_t (int)> ("_get_osfhandle");`.
3. `Runtime::isLoaded`: append `and getOsfhandle != nullptr`.
**Inventory:** API rows 1, 2; pattern rows 1, 2; name `getOsfhandle`.
**Validation:**
- The bound name is the exported name (`cygwin.din:144`).
- `uintptr_t` holds the 64-bit `long` (`sysconf.cc:679`).
- No other member changes. No comment and no doxygen.

### Step 2: `readMaster` reads the rest of the pipe in one read
**Scope:** `eve/cygwinPTY/Source/Session.cpp`, `Session::run` only (`:184-228`).
**Action:**
1. After `runtime.ioctl (master, setNonBlocking, &isNonBlocking);` (`:196`), add:
```cpp
        const auto masterPipe { reinterpret_cast<HANDLE> (runtime.getOsfhandle (master)) };
```
2. Change the capture of `readMaster` to `[this, masterPipe]`.
3. In the `do` body, after `numRead += static_cast<int> (juce::jmax<juce::int64> (0, result));`, add:
```cpp

                if (result > 0 and numRead < maxBytes)
                    numRead += juce::jmax (0, jam::TerminalFIFO::read (masterPipe, data + numRead, maxBytes - numRead));
```
4. Nothing else in `run` changes. The `while (result > 0 and numRead < maxBytes);` condition, the `isDrained` result and the `poll` loop stay.
**Inventory:** API rows 1, 3, 4, 5; pattern rows 3, 4, 5; names `masterPipe`, `readMaster`; synthesis rows 1-5.
**Validation:**
- Read `Session.cpp` at the changed lines.
- Each pass makes one runtime read first. So echo first, held input and end of stream stay in the runtime (Context, code path 1).
- The bulk read asks for no more than the free ring space (`maxBytes - numRead`). It reads only the bytes that the peek reports, so it does not wait (`jam_TerminalFIFO_windows.cpp:55-63`).
- A failed peek (-1) adds 0 bytes. The next runtime read reports the error.
- No lock, no sleep, no new wait. Branches: 3 or fewer per lambda.
- No change outside the scope. No comment and no doxygen.

### Step 3: Measure
**Scope:** new `eve/benchmark/seq-1m-windows-cygwin-bulk.md` (COUNSELOR writes it); whatdbg specs in the scratchpad.
**Action:**
1. The manifest copies `benchmark/seq-1m-windows-cygwin-relay.md`, with these changes:
   - `samples` = `seq-1m-windows-cygwin-bulk-samples.md`;
   - `output` = `seq-1m-windows-cygwin-bulk-report.md`;
   - `baseline` = `seq-1m-windows-cygwin-relay-report.md`.
   - The `## toolchain` rows stay: jam, then `../cygwinPTY/cast/spell.md`, then EVE with `--standalone-profile-symbols`.
2. @Pathfinder runs `scout D:/Poems/dev/eve/benchmark/seq-1m-windows-cygwin-bulk.md`, with a 15-minute watchdog.
3. COUNSELOR reads these values against the baseline: `epoch`, `launch prompt`, `tty bytes ready` (calls and bytes per tick), `terminal read`, `swap history lines`.
4. Where lane, the same method as the measurement in Context:
   - @Pathfinder starts EVE on `benchmark/seq-50m-windows-cygwin-where.md` and reports the pids.
   - COUNSELOR writes the seq and helper sample specs with those pids.
   - @Pathfinder runs `sample.lua` on seq, then on the helper.
   - COUNSELOR compares with the earlier samples: the seq `ZwWaitForMultipleObjects` share (33.3 % before), and the share of reader-thread samples that wait.
5. `sha256sum` the installed helper against the Release artefact.
**Validation:**
- scout exit 0, Release build.
- The installed helper hash equals the artefact hash.

### Step 4: SPEC amendment
**Scope:** `eve/SPEC.md` principle 10, after the `[AMENDED 2026-11-18]` block at `:173-187`.
**Action:** COUNSELOR adds a second `[AMENDED 2026-11-18]` block. It supersedes the sentence *"The cygwin helper reads the pty master only through the runtime, so the runtime keeps the echo, the held input and the end of the stream."* Text:
- Each drain pass makes one runtime read first. The runtime keeps the echo, the held input and the end of the stream.
- The pass then reads the rest of the pty pipe in one read, through the runtime's `_get_osfhandle` and `jam::TerminalFIFO::read`.
- The seam beyond EVE's control is seq's own write: 256 bytes per piece, each with a pipe query, a write and the output mutex (`pty.cc:3418`, `:3448-3450`, `:1236`).
- The discard-mode case of Context, code path 7, stated as behaviour.
- ARCHITECT rulings 3, 4 and 5 beside it, verbatim, and `/go` on this plan.
**Validation:** each line carries a citation or a verbatim quote. No line states behaviour that Step 2 does not have.

### Step 5: Auditor
**Scope:** every file of Steps 1-2.
**Action:** @Auditor validates against MANIFESTO.md, NAMES.md, CODING.md and this plan. COUNSELOR reads each cited file:line, names the clause, and resolves each confirmed finding in this sprint.

### Step 6: Measure again after the audit fixes
**Action:** run Step 3, actions 2, 3 and 5, again when a fix changes code. With no code change, skip this step.

### Step 7: Doxygen pass
**Scope:** `Runtime.h` (`getOsfhandle`); `Session.h` (`run()` at `:36-52`: the bulk read after each runtime read).
**Action:** @Pathfinder writes the pass on COUNSELOR's exact targets (doxygen-writer skill). COUNSELOR reads each block against the code.

### Step 8: Report
**Action:** COUNSELOR reports the Step 3 values against the baseline in chat. ARCHITECT runs the interactive `seq` test.

## BLESSED Alignment

- **B:** the reader thread stays the only thread that reads the pipe outside `poll` (Context, code path 5). The handle stays the runtime's; the helper does not close it.
- **L:** one binding, one local, two lines in the lambda.
- **E:** the bulk read is the JAM call that EVE's own cygwin TTY already uses (`jam_TerminalCygwinTTY_windows.cpp:130`). The handle comes from the runtime's exported API.
- **S (SSOT):** echo, held input and end of stream stay in the runtime read, first in each pass.
- **S (Stateless):** no new member.
- **E (Encapsulation):** the helper calls exported runtime functions only.
- **D:** the same input gives the same output. Each pipe keeps its own byte order. Echo and pty output interleave per 256 bytes of echo backlog (Context, code path 8). [CORRECTED 2026-11-18: first read "echo comes before pty output in each pass, as in the runtime"]
- **Copies:** one `ReadFile` into the ring (MANIFESTO permitted copy 1, OS boundary). The runtime path copies twice: `ReadFile` into `outbuf`, then `memcpy` (`pty.cc:699`, `:710`).

## Risks / Open Questions

None open. The one behaviour difference (Context, code path 7) is in Step 4's SPEC text.
