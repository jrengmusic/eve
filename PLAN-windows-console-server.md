# PLAN: windows-console-server (revision 2: BLESSED rewrite)

**RFC:** none. The objective comes from ARCHITECT's prompt.
**Date:** 2026-11-17
**Supersedes:** revision 1 of `D:\Poems\dev\eve\PLAN-windows-console-server.md`. Steps 1–6 of revision 1 are in the tree. Its Step 7 is replaced.
**Language Constraints:** C++17 / JUCE 8 / JAM. Windows 10 22H2 (build 19045) and later.

## Context

The in-process ConDrv server is in the tree.
- Revision 1, Steps 1–6: the sidecar is deleted, the server handle and launch exist, and the request loop, output, input and API answers run.
- The server works, but it does not obey the contract:
  - it duplicates handles;
  - it keeps copies of Model values;
  - it runs a second readiness and clock composition beside `jam::AsyncOutputStream`;
  - its files break Lean.

ARCHITECT's direction, verbatim:
- *"sidecar works. it's fucking slow. it's unacceptable. but it works. the only way to make it faster, is we implement ourselves, as in-process server."*
- *"INVENTORISE THOROUGHLY, this is not fork-vendor, this is BLESSED rewrite."*
- *"we dont build conhost, we dont build openconsole, we dont build conpty. this is our code, rewrite as BLESSED compliance."*
- *"why create EVERYTHING in parallel? what's wrong with ours?"*
- *"why creates shadow state, what is conhost history? we managed our own history"*
- *"how is mac tty handle this? why dont we have SSOT method for both lane? why this need to be diverged and treat differently?"*
- *"did i said explicitly SEAMLESS INTEGRATION? why keep trying to break contract to create parallel state machine?"*
- *"NO SIDECAR."*
- *"DESIGN BY CONTRACT. NON-NEGOTIABLE."*
- *"READ COMPREHENSIVELY MANIFESTO.md, DONT FUCKING USE NUMBERS AS CONSTRAINTS"*

After this plan:
- The Model holds the console state (MANIFESTO :202, *"State belongs exclusively to the Model"*).
- The server reads and stores that state through owner callbacks (R8).
- The server holds only two things:
  - transit data, which lives and dies within one operation (MANIFESTO :198);
  - the connect registry, which is a resource-identity registry (MANIFESTO :252).
- The server reads the default palette from `map::defaultAnsiColours` (Step 3, Correction).
- One readiness type, `jam::AsyncOutputStream`, ticks the server (SPEC.md:53-57).
- Each handle has one owner.
- Each object has one responsibility. A Lean smell leads to the ladder (MANIFESTO :58, :62-65, :72). A line count is never the target.

## Rulings

| # | Ruling (verbatim) | Decides |
|---|---|---|
| R1 | *"our floor is windows 10 22H2+ no need to support deprecated nor older version"* | Deprecated API rows answer `E_NOTIMPL` (kept) |
| R2 | *"why we need parallel state machine? our state machine SSOT is Model"* | No second state machine |
| R3 | *"how can we make native implementation always at its own domain? so EVE Model (not TextEditor) only need to implement ONE CONVENTION SSOT"* | VT bytes upstream; UTF-8 is the one encoding (Step 3) |
| R4 | *"if you need string manipulation, use std::string/string_view with jam::Format and jam::Strings"* | String handling |
| R5 | *"0 rules. non-negotiable. NO COPY, NO ALLOC, NO TEMP CONTAINERS, NO SHADOW STATE, only exception copy is cross thread."* | Transit |
| R6 | *"hold the doxygen pass then, after audit."* | Step order |
| R7 | *"jam always priority over juce"* | API choice |
| R8 | *"Model row via owner callback (Recommended)"* | Model values reach the server through owner callbacks, as `getCursor` does |
| R9 | **Superseded.** Revision 1 kept aliases and history in the server. ARCHITECT: *"why creates shadow state, what is conhost history? we managed our own history"* | No history store and no alias store (Step 4) |
| R10 | *"TerminalConsole family"* | Object names. `TerminalConsoleHistory` is dropped with R9 |
| R11 | *"i have windows 10 22H2 machine to validate"* | Verification |
| R12 | *"why create EVERYTHING in parallel? what's wrong with ours?"* and SPEC.md:53-57 *"one shared readiness type, `jam::AsyncOutputStream`"* | Step 1 |
| R13 | *"how is mac tty handle this? why dont we have SSOT method for both lane?"* | The Unix TTY is the reference for line discipline, modes and encoding |
| R14 | *"message thread could lock, async, even spawn parallel job. but MUST NEVER BLOCKED, CHOKED, NEVER"* | No wait on the message thread or on the context |
| R15 | *"READ COMPREHENSIVELY MANIFESTO.md, DONT FUCKING USE NUMBERS AS CONSTRAINTS"* | Step 5 decomposes by responsibility. No line count is a criterion |

## State Inventory: each value and its one holder

Source: members at `jam_TerminalConsole_windows.h:886-913`, `jam_TerminalConsoleInput_windows.h:122-125` and `jam_TerminalTTY_windows.h:229-236`.

| Value | Today | Holder after this plan | Why |
|---|---|---|---|
| Server handle | TTY `server` (`TTY_windows.cpp:285`) and a console duplicate (`Console_windows.cpp:158-160`) | `TerminalConsole::server` only | B: one owner |
| Input-available event | TTY `inputEvent` (`TTY_windows.cpp:286`) and a duplicate in `TerminalConsoleInput` (`ConsoleInput_windows.cpp:17-19`) | `TerminalConsoleInput::inputEvent` only. Its `open` creates the event | B |
| `readEvent`, `readOverlapped`, `source`, `clock`, `tick`, `onTimer` | Console (`Console_windows.cpp:161-208`, `:484-492`) | `jam::AsyncOutputStream` (`reader`). The console keeps only `readEvent` and `readOverlapped`, the event and the `OVERLAPPED` of its `READ_IO` | R12; SPEC.md:53-57 |
| `winsize` (`std::atomic<int64_t>`) | Console (`.h:904`, `.cpp:883-894`) | Model row `winsize` (`parameters.md:57`), read through an owner callback `getWinsize` | R8; the row reads lock-free on any thread (`jam_ParameterBase.h:219`) |
| Cursor | Model row `cursor`, through `getCursor` (`EVETerminalProcessor.cpp:72`) | unchanged | R8 |
| Current attributes (`defaultAttributes` constant, `.cpp:34`, `:704`) | constant | Model row `pen` (`parameters.md:103`), read through an owner callback `getPen`. The legacy attributes come from `colourId`/`bgColourId` (`jam_AttributedChar.h:23-24`), through the inverse of the decoder's SGR colour map | SPEC Principle 6; S |
| Colour table (`defaultColorTable`, `.cpp:37-43`) | constant Campbell palette | `map::defaultAnsiColours` (`jam_Terminal.h:2543`), the JAM default palette. The theme palette is in the LookAndFeel, on the message thread (Step 3, Correction) | S: one default palette; B: threads bound to their context |
| `outputMode` | Console (`.h:901`). Written at `.cpp:170` and `:665`. Read only at `:655`, and no output path reads it | deleted. `GetConsoleMode` on the output answers the fixed constant of the decoder's behaviour. `SetConsoleMode` on the output succeeds and stores nothing | S: the stored value changed no behaviour |
| `inputCodePage`, `outputCodePage` | Console (`.h:902-903`) | deleted. UTF-8 is the one encoding. `GetConsoleCP` answers `CP_UTF8`. `SetConsoleCP` succeeds for `CP_UTF8` and answers `statusNotSupported` for any other value. `convertToWide` and `getLangId` use `CP_UTF8` | R3, R13: the Unix lane has one encoding |
| Input mode (`TerminalConsoleInput::mode`, `.h:124`, set to `defaultInputMode` at `ConsoleInput_windows.cpp:22`) | ConsoleInput | a new Model row `inputMode` (int, no default). `TerminalConsole::open` stores the initial mode, `defaultInputMode` (Step 3). The server reads it through an owner callback `getInputMode`, and stores it through an owner callback `setInputMode`. Delete `mode`, `getMode` and `setMode` | MANIFESTO :202, :425; R2; R8 |
| Resize record pending (`isResizePending`) | ConsoleInput | unchanged: an input record in transit. It lives from `setWinsize` to the read that removes it | MANIFESTO :198 |
| Process registry (`numProcesses`, `processes`, `processGroups`) | Console | unchanged: the identity that the driver gives at connect and takes back at disconnect, read in place | MANIFESTO :252 |
| `requests`, `firstPendingRead`, `numPendingReads`, `reply`, `connection`, `writeOffset`, `writeSize` | Console | unchanged. Each one lives from a `READ_IO` to its `COMPLETE_IO` | MANIFESTO S: transient |
| `records`, `units`, `cells`, `bytes` | Console, sized in `open` (`.cpp:175-178`) | unchanged: work buffers for the OS boundary, sized off the deadline path | MANIFESTO E, permitted copy 1 |
| Command history and aliases | none (revision 1, Step 7, was not built) | none. The shell keeps its own history: zsh and PSReadLine. EVE keeps its own history | R9 |

## Dependency & API Inventory

**JAM (first, R7)**

| Need | API | Cite |
|---|---|---|
| One readiness type | `jam::AsyncOutputStream`: `open (stream, context, bufferSize, maximumPeriodMilliseconds)`, `close`, `onBytesReady` | `jam_AsyncOutputStream.h:21-68` |
| Windows half | `Native`: `getNumReady` and `arm` rows selected by `GetFileType`, `source.onReady` → `clock.process` → `onBytesReady`, and a timer → `listen` → `arm` | `jam_AsyncOutputStream_windows.cpp:16-187` |
| Other Windows users that keep the pipe and console rows | `StdioServer::reader`, `TerminalWindowsDisplay::reader` | `docs/xml/classjam_1_1StdioServer.xml:59-60`, `classjam_1_1TerminalWindowsDisplay.xml:53-54` |
| Period law: a tick of at least `bufferSize` bytes gives a period of 0 | `TerminalClock` | `jam_TerminalClock.h:16-22` |
| Tick runs inline, only while the device plays | `VirtualDevice::clock` | `jam_VirtualDevice.cpp:76-82` |
| Model reads on any thread | `Parameter<int>::getValue`, `Parameter<int64_t>::getValue` | `jam_ParameterBase.h:165`, `:219` |
| Owner callback precedent | `TerminalTTY::onBytesReady`, `TerminalWindowsTTY::getCursor` | `jam_TerminalTTY.h:60`, `jam_TerminalTTY_windows.h:227` |
| Palette | `map::defaultAnsiColours` | `jam_Terminal.h:2543` |
| Colour id to ANSI index | `AnsiDocument::getIndexedColour (int colourId)` | `jam_AnsiDocument.cpp:892-904` |
| Pen layout | `colourId` bits 25-34, `bgColourId` bits 35-44 | `jam_AttributedChar.h:23-24` |
| Input lane | `TerminalFIFO`, `TerminalKeyboard::applyKeyPresses`, `getKeyEvent` | `jam_TerminalConsoleInput_windows.h:110-122` |
| VT writer | `TerminalConsoleOutput` | `jam_TerminalConsoleOutput_windows.cpp:58-251` |

**EVE owner reads**
- `getWinsize()`: `EVETerminalProcessor.cpp:319-322`.
- `createTTY`: `:68-83`.
- `prepare` → `openTTY` → `tty->setWinsize`: `:470-507`. It runs from `audioDeviceAboutToStart` (`:376-377`).

**Windows API (no change):** the ConDrv IOCTLs (`Console_windows.cpp:6-13`), `CreateProcessW` with `STARTUPINFOEXW` (`TTY_windows.cpp:193-233`), overlapped `DeviceIoControl`.

**New names (family, NAMES Rule 5):**
- `TerminalWindowsTTY::getWinsize` and `TerminalConsole::getWinsize`, owner callbacks. Family: `getCursor`. EVE's own `getWinsize()` is at `:319`.
- `TerminalWindowsTTY::getPen` and `TerminalConsole::getPen`, owner callbacks. Family: `getCursor`, and the Model row `pen`.
- `AsyncOutputStream::onEvent`, an owner callback of the event stream row. Family: `onBytesReady` and `Source::onReady`.
- `TerminalConsole::reader`, the `AsyncOutputStream` member. Family: `StdioServer::reader`, `TerminalWindowsDisplay::reader`.
- `getInputMode` and `setInputMode`, owner callbacks on the TTY and the console. Family: `getCursor`, and `TerminalConsoleInput::getMode`/`setMode` (`.h:34-36`), which they replace.
- `jam_TerminalConsoleMessages_windows.h` (Step 5). Family: R10.
- `map::consoleColours`, a CAST table in jam `cast/terminal.md` (Step 3). Family: `consoleSgrColours`, named for its output domain (NAMES Rule 7).

**New domain term (NAMES Rule -1, ratified with the approval of this plan):**
- The Model row `inputMode` in `Source/layout/parameters.md`. It holds the console input mode that a client sets with `SetConsoleMode`. No row covers this concept. The nearest family is the `*Mode` rows: `insertMode` (`:91`), `originMode` (`:89`), `mode` (`:81`).

**Deleted names:**
- TTY: `server`, `inputEvent`.
- Console: `source`, `clock`, `tick`, `outputMode`, `inputCodePage`, `outputCodePage`, `winsize`, `defaultAttributes`, `defaultColorTable`. `resizeEvent` and `resizeSource` stay (Step 3).
- `TerminalConsoleInput::open`'s `newInputEvent` parameter.

## Validation Gate

COUNSELOR validates each step before the next one, against:
- MANIFESTO.md;
- NAMES.md;
- CODING.md;
- this PLAN, with no deviation and no scope drift.

@Auditor runs once, after Step 5.

## Steps

### Step 0: Write the plan to the EVE root
`D:\Poems\dev\eve\PLAN-windows-console-server.md` gets this revision. It replaces revision 1.

### Step 1: One readiness type
**Scope:** `jam_AsyncOutputStream.h`, `jam_AsyncOutputStream_windows.cpp`, `jam_TerminalConsole_windows.{h,cpp}`.

**Action:**
1. The Windows half selects its row by `GetFileType` through a `LookupTable`, with three rows:
   - pipe: unchanged.
   - console (`FILE_TYPE_CHAR`): unchanged.
   - event (`FILE_TYPE_UNKNOWN`): an event that the owner's overlapped I/O signals.
2. For the event row:
   - `getNumReady` returns `onEvent()`.
   - `arm` is `source.arm()`, the same as the console row.
   - `onEvent` is a Windows-only owner callback, `std::function<int()>`. The owner sets it before `open`, and `open` asserts it.
3. `TerminalConsole` owns `reader` and opens it on its request event.
4. `onEvent` does this:
   1. It answers each completed request that is not a write: connect, disconnect, object, API, read, flush.
   2. It issues the next `READ_IO` with the reply.
   3. It repeats while `READ_IO` completes at once.
   4. It stops at a pending `READ_IO` or at a write.
   5. It returns the pending output bytes (`writeSize - writeOffset + output.getNumReady()`), or -1 when the server closed.
5. Requests are answered on the readiness path, so they do not depend on the tick. The tick runs only while the device plays (`jam_VirtualDevice.cpp:78`).
6. `read()` keeps the drain:
   1. When the payload of a write request ends, it completes the write.
   2. It issues the next `READ_IO`, as it does now (`.cpp:513-514`).
7. Output that is not tied to a request completes no request.
8. Delete from the console: `source`, `source.onTimer`, `clock`, `tick`, and the `isWritePending` branch of `onReady`.

**Validation:**
- One readiness type (SPEC.md:53-57).
- No timer and no clock in the console.
- No wait on the context (R14).
- No `get` verb that changes state.

### Step 2: One owner for each handle
**Scope:** `jam_TerminalTTY_windows.{h,cpp}`, `jam_TerminalConsole_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}`.

**Action:**
1. `TerminalConsole::open (context, bufferSize, maximumPeriodMilliseconds)` does these in order:
   1. It opens `\Device\ConDrv\Server` into `server`.
   2. `input.open (server, capacity)` creates the input event and sends `SET_SERVER_INFORMATION` with it. The input owns the event and registers it itself, so no getter exposes the event (MANIFESTO :234).
   4. It opens `reader`.
2. Move `openDevice`, `buildEnvironmentBlock`, `buildCommandLine`, `createProcess` and `spawnProcess` from `TTY_windows.cpp:103-256` to the console `.cpp`.
3. `spawnProcess` becomes the member `TerminalConsole::spawnProcess (shell, args, workingDirectory, environment)`. It returns the process handle.
4. `TerminalWindowsTTY::open` calls `console.open`, then `process.reset (console.spawnProcess (…))`.
5. The TTY keeps `process` and `console` only. Delete the TTY `server` and `inputEvent`, and the `DuplicateHandle` calls at `Console_windows.cpp:158-160` and `ConsoleInput_windows.cpp:17-19`.

**Validation:**
- Each handle has one RAII owner.
- `close` cancels `reader`, then closes the handles, then the child.

### Step 3: Values from their owners
**Scope:** `jam_TerminalTTY_windows.{h,cpp}`, `jam_TerminalConsole_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}`, `jam_TerminalConsoleOutput_windows.{h,cpp}` (`getAttributes` and `getColorTable`; this object already owns the legacy attribute layout, `ConsoleOutput_windows.cpp:8-12`, `:131`), `EVETerminalProcessor.cpp` (`createTTY`), `Source/layout/parameters.md` (the `inputMode` row), the `@id` row `inputMode` in jam `cast/terminal.md`, jam `cast/terminal.md` and `cast/spell.md` (the `consoleColours` table), and `generated/jam_Terminal.h` (regenerated by `cast ../jam/cast/spell.md`).

**Action:**
1. Delete `winsize` and `getWinsize() const` from the console.
2. Add the owner callbacks `getWinsize` (`std::function<TerminalWinsize()>`) and `getPen` (`std::function<int64_t()>`) on the TTY and the console. Forward them as `getCursor` is forwarded (`TTY_windows.cpp:300-301`).
3. EVE `createTTY` sets them, beside `getCursor` (`:72`):
   - `getWinsize` → `getWinsize()` (`:319-322`);
   - `getPen` → the Model row `pen`.
4. Delete `console.setWinsize (TerminalWinsize { dims… })` at `TTY_windows.cpp:305`.
5. `setWinsize (ws)` keeps only the resize notification. `resizeEvent` and `resizeSource` stay, because `prepare` runs on the thread that starts the device (`EVETerminalProcessor.cpp:376-377`, `:506`). The parameter is unused, because the size comes from the Model.
6. `getScreenBufferInfo` does these:
   - `Attributes` come from `getPen()`. The steps:
     1. `colourId` and `bgColourId` (`jam_AttributedChar.h:161-165`) go through the existing inverse, `AnsiDocument::getIndexedColour` (`jam_AnsiDocument.cpp:892-904`). The result is the ANSI index.
     2. A new CAST table, `consoleColours` (ANSI index 0-15 to console colour index, the inverse of `consoleSgrColours`, `jam_Terminal.h:2850-2869`), gives the legacy attribute.
     3. `colourIdNone`, or an index above 15, gives the legacy default for that field.
   - `ColorTable` comes from `map::defaultAnsiColours` (`jam_Terminal.h:2543`), the existing JAM default that `StyleTerminal` seeds from (`jam_StyleTerminal.cpp:7-8`).
   - **Correction (2026-11-17).** The first text read `ColorTable` from `jam::Colours::getColour`. The ANSI ids 1-16 (`jam_Style.h:1421-1436`) are below the store's first id 604 (`jam_Style.h:1677`), and `getColour` asserts `contains` (`jam_Colours.cpp:34`). Those ids resolve through the LookAndFeel (`jam_Colours.cpp:156`), which is message-thread state (MANIFESTO :44).
7. Delete `defaultAttributes`, `defaultColorTable`, `outputMode`, `inputCodePage` and `outputCodePage`. Do the mode and code-page answers in the State Inventory.
8. Add the Model row `inputMode` to `parameters.md`, in the form of the `int` rows (`:27`, `:43`). Leave its default blank, as `winsize` (`:57`) is blank.
9. `defaultInputMode` (`ConsoleInput_windows.cpp:8`) is the one source of the initial mode. `TerminalConsole::open` calls `setInputMode (defaultInputMode)`, in place of `mode = defaultInputMode` (`:22`). Each new session then starts in the initial mode, and a restored state does not carry the mode of an earlier client.
10. Delete `TerminalConsoleInput::mode`, `getMode` and `setMode`.
11. Every read of the mode calls `getInputMode()`:
    - `applyRecords` (`ConsoleInput_windows.h:112`);
    - `writeInput` (`Console_windows.cpp:853`);
    - `resizeSource.onReady` (`:212`);
    - `getConsoleMode` (`:653`).
12. `setConsoleMode` on the input calls `setInputMode (mode)`.
13. EVE `createTTY` sets `getInputMode` and `setInputMode` on the `inputMode` row: `getParameter<jam::Parameter<int>>` and `getValue`, and `storeValue`. Both are lock-free on any thread (`jam_Model.h:13-16`, `:1002-1019`).

**Validation:**
- No console member holds a Model value or a palette value.
- Owner callbacks are asserted once, at `open`.

### Step 4: Cooked read, Unix line discipline (`TerminalConsoleLine`)
**Scope:** new `jam_TerminalConsoleLine_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}`, `jam_TerminalConsoleOutput_windows.{h,cpp}` (the echo), `jam_TerminalConsole_windows.{h,cpp}` (the API table, `write`, `read`, `isReadReady`, `writeText`), and the includes in `jam_terminal.h` and `jam_terminal.cpp`.

**Action:**
1. Under `ENABLE_LINE_INPUT`, `ReadConsole` completes only when the input queue holds a line end. The bytes stay in the queue, `TerminalFIFO`, until then. No line buffer exists.
2. At completion:
   - erase and kill apply over the line bytes as they go into the client's buffer;
   - under `ENABLE_PROCESSED_INPUT`, CR becomes CR LF.
3. Under `ENABLE_ECHO_INPUT`, `write()` echoes the arriving bytes into `output`. A backspace echoes `BS SP BS`.
4. Line editing is the Unix canonical set: erase, kill and the line end. No history and no alias exist.
5. The alias and history API rows go to `setNotImplemented`, the R1 row:
   - `AddConsoleAlias`, `GetConsoleAlias`, `GetConsoleAliasesLength`, `GetConsoleAliasExesLength`, `GetConsoleAliases`, `GetConsoleAliasExes`;
   - `ExpungeConsoleCommandHistory`, `SetConsoleNumberOfCommands`, `GetConsoleCommandHistoryLength`, `GetConsoleCommandHistory`;
   - `SetConsoleHistory`, `GetConsoleHistory`.

**Validation:**
- No store beside the input queue (R5, R9).
- One responsibility (R10).
- String work through `std::string_view` and `jam::Format` (R4).

### Step 5: Lean decomposition (one object, one translation unit)
**Scope:** `jam_TerminalConsole_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}`, `jam_TerminalConsoleOutput_windows.{h,cpp}`, new `jam_TerminalConsoleMessages_windows.h`, and `jam_terminal.h`.

**Action:**
1. Move the ABI transcription (`Console_windows.h:1-766`) to `jam_TerminalConsoleMessages_windows.h`. `Request` becomes `TerminalConsoleRequest` in `jam_TerminalConsoleRequest_windows.{h,cpp}`, because its member functions need their own translation unit (one object, one TU).
2. `TerminalConsole` keeps:
   - the request loop;
   - the IO and API tables;
   - connect, disconnect and object;
   - the process registry and the ctrl events;
   - the info answers;
   - the launch.
3. `TerminalConsoleOutput` takes the screen-API rendering: `render*`, the `get*Work` counts, `setPendingOutput`, `setOutputWritten`, `readBytes` and `readUnicode`.
4. `TerminalConsoleInput` takes `writeRecords`, `writeText`, `writeLine` and `isReadReady`.
   - **Correction (2026-11-17).** `writeInput` (the Ctrl+C scan) and `writeConsoleInput` call `endTasks`, which signals the process registry that the console owns. They stay in the console, with `getKeyUnits` beside its only user (MANIFESTO :227, *"Objects are ignorant by design"*).
5. The request and the server handle go to these calls as parameters. No member stores them (CODING: no non-owning pointer members).
6. Each Lean smell goes through the ladder, in order:
   1. Is the branch needed?
   2. Is it a lookup?
   3. Is there a simpler form?
   4. Is it a contract violation?
   5. Is it a responsibility split?
7. The known smells:
   - `setOutputWritten` (`.cpp:977-982`) encodes a decision table keyed by API number in an `if` chain. It becomes a lookup.
   - `getCellSize` (`.cpp:899-900`) asserts and clamps. The owner establishes both invariants: `TextEditor::setWinsize` builds the pixels as cells × cell metrics × scale (`jam_TextEditor.cpp:1955-1962`), and EVE prepares only a valid winsize (`EVETerminalProcessor.cpp:376`). The assert and the clamp go (MANIFESTO :385).
   - `open` (`.cpp:153-230`) loses the readiness wiring in Step 1 and the handle duplicates in Step 2. What remains is one responsibility.
8. A function or a file with one responsibility stays whole. Never split only to move lines (MANIFESTO :62-65, :72).

**Validation:**
- Each object has one responsibility (MANIFESTO :231).
- Each extracted piece is independently meaningful (MANIFESTO :62-65).
- No decision table is left in imperative branches (MANIFESTO :66).

### Step 6: Build
Pathfinder builds through cast (`cast cast/spell.md --debug`).

### Step 7: Audit
@Auditor, once, across every changed file.

### Step 8: Doxygen pass (Pathfinder, after the audit, R6)
It covers:
- every changed declaration;
- the stale TTY blocks: `TTY_windows.cpp:1-57` (pipe, ConPTY) and `TTY_windows.h`;
- the `AsyncOutputStream` Windows half;
- the blocks from the stopped pass: `jam_TextEditor.h:661`, `:673`, `:683`, `:1208`, `:1540`; `jam_TerminalBuffer.h:385`, `:392`; `jam_TerminalSwap.h:150`.

### Step 9: ARCHITECT tests

## Verification
On 26300 and on 19045, in EVE Standalone and in EVE hosted by END:
- zsh (msys2):
  - `time seq 1 1000000`;
  - typing, arrows, and Ctrl+C on a running `seq`;
  - resize while `vim` runs.
- pwsh: PSReadLine editing, arrows and history (PSReadLine's own).
- cmd: cooked-read typing, backspace and Enter.
- `color 1f` in cmd, then `GetConsoleScreenBufferInfo`: the `Attributes` follow the Model pen.
- `scout benchmark/seq-100k-windows.md`, compared with `benchmark/seq-100k-windows-report.md`.

## BLESSED Alignment
- **B.** One owner for each handle (Step 2). `TerminalWindowsTTY` owns `console` and `process`, and `console` owns `server`, `reader` and the input event.
- **L.** One responsibility for each object (Step 5). Lookup tables replace the decision tables. A smell leads to the ladder, never to a line target.
- **E.** Every new name comes from a cited family, except the row `inputMode`, which is ratified with the approval of this plan.
- **S (SSOT).** Each value has one holder (State Inventory). The Model gives the cursor, the size, the pen and the input mode. `map::defaultAnsiColours` gives the default palette.
- **S (Stateless).** The server keeps only two things: transit data (requests, replies and pending records, MANIFESTO :198) and the connect registry (MANIFESTO :252).
- **E (Encapsulation).** Data moves one way. Output goes from the client to the decoder ring. Input goes from the lane to the server to the client.
- **D.** One serial context runs readiness, requests and the tick, through one readiness type.

## Risks / Open Questions
- None.
