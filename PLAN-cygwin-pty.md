# PLAN: Cygwin PTY

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-17
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 + JAM; msys2 / Cygwin runtime loaded at run time (LGPLv3+, `CYGWIN_LICENSE:5-9`, `COPYING.LIB:109-114` §4d1)

## Context

On Windows, an msys2 program (zsh) treats a handle as a terminal only if it is a console or an msys2 pty pipe (`dtable.cc:282-339`, `:997-1023`). On a console, each write costs about three blocking round trips (`console.cc:79-103`, `:2842-2872`). On the msys2 pty, a slave write is one `WriteFile` into a 128 KiB pipe (`pty.cc:3409-3455`, `fhandler.h:2404`). The console path is the bottleneck. The dedicated server-thread work (PLAN-windows-console-throughput.md) did not remove it.

ARCHITECT's direction:
- Undo the dedicated server context (U5-U1). Keep the in-process ConDrv server for native shells (pwsh, cmd).
- Add a TTY backend for msys2 and Cygwin shells over the msys2 pty, through a helper process.
- *"msys-2.0.dll and cygwin1.dll must be explicitly written at eve.md, we shipped default path assuming user install it, and at default path. user can still override. but still internal fallback, when those arent available just like zsh, we run powershell by default"*.
- *"config dll just follow verbatim established pattern. eve.md is crosplatform single dot config. windows lane are harmless for mac."*
- Helper name: *"eve-cygwinPTY.exe"*. Helper home: *"Sub-project in eve/"* (AskUserQuestion answer).

Why a separate process is required (read facts):
- The runtime needs padding at the bottom of the calling thread's stack until unload. It should load before other threads exist (`dcrt0.cc:1084-1088`). EVE runs inside a host that owns `main` and the thread stacks.
- Init starts a signal thread and writes over the first 4K of the calling stack (`cygload.cc:157-161`). SPEC.md:39-40 says EVE starts no thread.
- The runtime keeps one state per process (`dcrt0.cc:1068`, `:1099-1102`). Several EVE instances can share one host process.
- No shipped msys2 tool carries a window size over pipes. util-linux `script` 2.40.2 opens the pty with no size when stdin is a pipe (`pty-session.c:198`). It resizes only under `if (pty->isterm)` (`:579`).

## Overview

1. EVE keeps the in-process ConDrv server for native Windows shells, with the dedicated server thread removed.
2. For a shell on a configured msys2 or Cygwin runtime, EVE starts `eve-cygwinPTY.exe`. The helper loads the runtime DLL, opens a pty, starts the shell on the slave, and relays raw bytes over its stdin and stdout.
3. EVE reads the helper's stdout through the existing pipe readiness row, the same as the Unix master fd.

## Language / Framework Constraints

- The helper is a console app. It owns `main`, so it can reserve the runtime's stack padding (`cygload.cc:453`, `cygwin::padding padding;` in `main`).
- The helper loads the DLL with `juce::DynamicLibrary`. It never links the runtime. The DLL path comes from EVE's settings, so it is the user's installed DLL (LGPL §4d1).
- The helper never calls `fork`. A `fork` from a process that loaded the DLL with LoadLibrary is not proven (`sigproc.cc:1110-1113` against `fork.cc:410`, `:515`). The helper uses `spawnv` (`Runtime.h` member `spawnv`; `Session::spawnShell`).
- EVE's tick does not wait (SPEC.md:970). `TerminalFIFO::writePendingData (Handle)` waits (`jam_TerminalFIFO_windows.cpp:77-89`, `GetOverlappedResult (..., TRUE)`), so EVE does not call it on the tick.
- The 10 ms idle poll on the msys2 master read (`pty.cc:633-667`, `select.cc:797-827`) is inside the runtime. EVE cannot change it.

## Framework Inventory

### API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| Relay stdin and stdout of the helper | jam_terminal | `jam::StdioServer` (`open`, `applyPendingData`, `write`, `flush`) | `jam_StdioServer.h:23`, `:38`, `:102-108`; `jam_StdioServer.cpp:36-41`, `:49-53` | 6 |
| Read readiness of a pipe | jam_terminal | `jam::AsyncOutputStream`, `FILE_TYPE_PIPE` row (`getNumPipeBytes`, `armPipe`) | `jam_AsyncOutputStream_windows.cpp:66-78`, `:95-104`, `:111-123` | 4, 6 |
| Read a pipe into the ring | jam_terminal | `TerminalFIFO::writeFromTerminalTTY (Handle)` | `jam_TerminalFIFO.h:137`; `jam_TerminalFIFO_windows.cpp:50-75` | 4 |
| Read a pipe into a caller buffer (the device read that `TerminalTTY::read` returns) | jam_terminal | `TerminalFIFO::read (Handle, char*, int)`, public static, used by `TerminalCygwinTTY::read` | `jam_TerminalFIFO.h:139`; `jam_TerminalCygwinTTY_windows.cpp:122` | 4 |
| Start a process with chosen std handles and one inherited handle | jam_terminal | `TerminalConsole::createProcess` (static), shared by `spawnProcess` and `TerminalCygwinTTY::open` | `jam_TerminalConsole_windows.h:55`; `jam_TerminalConsole_windows.cpp:184-229`; `jam_TerminalCygwinTTY_windows.cpp:82` | 4 |
| Quote a path on a command line | JUCE | `juce::String::quoted` | `juce_String.h:903` | 4 |
| Pack and unpack the pty size | jam_terminal | `jam::TerminalWinsize::pack`, `unpack` | `jam_TerminalWinsize.h:42`, `:51` | 4, 6 |
| Wait on a process handle | jam_core | `jam::AsyncUpdater::Source` (`onReady`, `open`, `arm`) | `jam_AsyncUpdater.h:84-164`; `Session.h` member `shellExit` | 6 |
| Run the helper's message loop | JUCE | `juce::AsyncUpdater`, `juce::MessageManager::runDispatchLoop`, `stopDispatchLoop` | `Session.h`; `Main.cpp`; `Session.cpp` `run` | 6 |
| Unpack the helper archive | JUCE | `juce::ZipFile`, `juce::MemoryInputStream` | `ShellIntegration.cpp:76-91` | 8 |
| Runtime functions that the helper calls | msys2 runtime | `dup`, `dup2`, `read`, `write`, `close`, `kill`, `signal`, `setpgid`, `tcsetpgrp`, `setsid`, `cygwin_internal`, `pthread_create`, `pthread_join`, loaded by name | `Runtime.h`; `Runtime.cpp` | 5, 6 |
| Write the ring to a pipe | jam_terminal | `TerminalFIFO::writePendingData (Handle)` (waits) | `jam_TerminalFIFO.h:96`; `jam_TerminalFIFO_windows.cpp:77-89` | 3 |
| TTY interface | jam_terminal | `jam::TerminalTTY` (`open`, `close`, `isOpen`, `read`, `write`, `setWinsize`, `onBytesReady`) | `jam_TerminalTTY.h:53` | 4 |
| Load a DLL at run time | JUCE | `juce::DynamicLibrary` | `juce_DynamicLibrary.h` | 5 |
| Find the plugin or app binary | JUCE | `juce::File::currentExecutableFile` ("If running as a plug-in or DLL, this will (where possible) be the DLL") | `juce_File.h:939-950` | not used: Step 8 unpacks the helper into the shell integration folder, and Step 7 gets it from `ShellIntegration::getOrCreateCygwinPty` |
| Read a setting with a default | jam_core | `ParameterManager::getDefaultValueForSettings (jam::Format::toTag (Id::...), default)` | `ShellIntegration.cpp:19` | 2, 7 |
| Ship a binary payload | EVE cast | helper packed by `cast --pack` into `Source/windows`; embedded as BinaryData; hashed into `CAST_CYGWIN_PTY_HASH`; unpacked with a stamp file (replaces the first form `BinaryData::Raw` + `replaceWithData`, see Step 8) | END `cast/cmake.cast:370-397`; END `Nexus.cpp:200-241`; `ShellIntegration.cpp:51-91`; `files.md` rows `cygwin pty stamp`, `archive extension` | 8 |
| Start a pty | msys2 runtime | `openpty` | `bsdlib.cc:97-133` | 6 |
| Take the pty as controlling tty | msys2 runtime | `ioctl (slave, TIOCSCTTY, 0)` | `termios.cc:742-771` | 6 |
| Start the shell | msys2 runtime | `spawnv` (`_P_NOWAIT`), constant `noWait` in `Session.cpp` | `spawn.cc:976-1011`; `Runtime.h` | 6 |
| Set the pty size | msys2 runtime | `ioctl (master, TIOCSWINSZ)` | `pty.cc:2511-2559` | 6 |
| Init the runtime | msys2 runtime | `msys_dll_init` / `cygwin_dll_init` | `dcrt0.cc:1084-1104`; `cygwin.din:408` | 5 |

### Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Console app built by cast | whatdbg `project-info.md:45` `juce_add_console_app`; `cast/cmake.cast:111` | whatdbg, scout (`project-info.md:48`) | 5 |
| StdioServer in a console app | whatdbg `Processor.h:307`; `Processor.cpp:22`, `:49` (`stdio.open (nullptr, ...)`), `:57`, `:138` | whatdbg | 6 |
| Windows TTY wired in EVE | `EVETerminalProcessor.cpp:67-88` (`createTTY`, callbacks `getCursor` … `setOutputMode`) | EVE | 7 |
| Shell fallback list | `EVETerminalProcessor.cpp:432` `fallbackShells { Id::powershellProgram, Id::commandPromptProgram }` | EVE | 7 |
| Setting with a default path | `ShellIntegration::getStartDirectory` (`ShellIntegration.cpp:16-22`) | EVE | 2, 7 |
| Payload written at run time under the settings directory | `ShellIntegration::addShellScripts` (`ShellIntegration.cpp:24-48`) for the scripts; directory from `EVETerminalProcessor.cpp:425`. The helper archive uses the END form instead: `ShellIntegration::getOrCreateCygwinPty` (`ShellIntegration.cpp:51-91`), the same as `Nexus::getOrCreatePlugin` (END `Nexus.cpp:200-241`) | EVE | 8 |
| Runtime loader in a native app | `cygload.cc:67-75` (padding), `:152-164` (LoadLibrary, `msys_dll_init`) | msys2 testsuite | 5 |

### Names

| Family | Members | Intent | Step |
|---|---|---|---|
| TerminalTTY backends | `TerminalUnixTTY`, `TerminalWindowsTTY` (`jam_TerminalTTY_windows.h:49`), `TerminalMemoryTTY` (EVE `Source/TerminalMemoryTTY.h`) | `Terminal<Platform>TTY` implements `TerminalTTY` | 4 |
| New member | `TerminalCygwinTTY` | The TTY over the Cygwin-family pty | 4 |
| Settings keys | `shell integration` (`default-eve.md:213`), `shell` (`:217`) | lower-case words, `key \| type \| value \| description` | 2 |
| Identifier vocabulary | `defaultShell`, `defaultShellWindows`, `powershellProgram`, `commandPromptProgram` (`identifiers.md:104-112`) | `default<Noun>` for a shipped default | 2 |
| New identifiers | `msysRuntime`, `cygwinRuntime` (keys); `defaultMsysRuntime` = `C:/msys64/usr/bin/msys-2.0.dll`, `defaultCygwinRuntime` = `C:/cygwin64/bin/cygwin1.dll` | key and shipped default, same shape as `shell` and `defaultShellWindows` | 2 |
| Helper binary | `eve-cygwinPTY.exe` (ARCHITECT) | the helper product name | 5 |
| Helper objects | `Runtime` (the loaded runtime and its functions), `Session` (the pty, the shell and the relays), `RuntimePadding`, `cygwinPTYCRTStartup` (the entry point), `Winsize` (the runtime's `struct winsize`) | `Runtime.h`, `Session.h`, `Main.cpp` | 5, 6 |
| Backend selection and unpack | `EVETerminalProcessor::getRuntime`, `ShellIntegration::getOrCreateCygwinPty`, `Files` `cygwinPtyStamp`, `archiveExtension` | `getOrCreate` for the unpack, as in END `Nexus::getOrCreatePlugin` | 7, 8 |
| `TerminalCygwinTTY` file-local names | `pipePrefix`, `maxPipeInstances`, `createNamedPipe`, `createPipeClient`; members `numWriting`, `packedSize`, `sizeOverlapped`, `writeOverlapped` | the pipe plumbing of the backend | 4 |

### Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "No bail-out guards" | positive nesting, result return | `jam_TerminalTTY_windows.cpp:55-59` |
| "Use the framework API fully" | `StdioServer` for stdio, `AsyncOutputStream` for readiness, `juce::DynamicLibrary` for load | `whatdbg Processor.cpp:49`; `jam_StdioServer.cpp:36-41` |
| "No `==` / `!=` on strings" | Identifier dispatch | `ShellIntegration.cpp:13` |
| "Deadline threads" | the tick never waits | SPEC.md:970 |
| "One object, one translation unit" | one header, one `.cpp`, platform file in `native/` | `jam_TerminalTTY_windows.{h,cpp}` |
| "Owner first … configure only the difference" | defaults live in `identifiers.md`, read through `getDefaultValueForSettings` | `ShellIntegration.cpp:19` |

### Absence

| Need | Query | Result |
|---|---|---|
| Out-of-process pty with a size channel over pipes, shipped with msys2 | util-linux `script` 2.40.2 source (`C:\msys64\var\lib\pacman\local\util-linux-2.40.2-2`) | no size when stdin is a pipe (`pty-session.c:198`, `:579`); no size option (`script.c:770-788`) |
| A second target inside EVE's cast | `cast/cmake.cast:151`, `project-info.md:62` | one `juce_add_plugin` target; no `juce_add_console_app` or `add_executable` in EVE `cast/` |
| A session flag for `posix_spawn` | grep `POSIX_SPAWN_SETSID` in msys2-runtime | none (`spawn.h:42-47`) |
| Non-waiting pipe write in TerminalFIFO | `jam_TerminalFIFO_windows.cpp:77-89` | only the waiting form exists |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

## Steps

### Step 1: Undo the dedicated server context
**Scope:** `jam_TerminalConsole_windows.{h,cpp}`, `jam_TerminalConsoleOutput_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}`, `jam_TerminalFIFO.h`.
**Action:** Do U5, U4, U3, U2, U1 of `PLAN-windows-console-throughput.md:360-430`, in that order. Run a scout build after each one. U1 makes `writeFromDevice` private again, only after U3.4. Remove the `isServerOpen` shadow state. Keep the `Document` move fix (`jam_Document.cpp:260-279`).
**Inventory:** TTY interface; Windows TTY pattern.
**Validation:** Each U-step matches its text in the undo section. The build is clean. No member that the undo deletes remains. No comment or doxygen is added.

### Step 2: Runtime settings
**Scope:** `cast/identifiers.md`, `Source/layout/default-eve.md`, generated `Source/generated/Identifiers.*` (through cast).
**Action:**
1. Add `@id msysRuntime`, `@id cygwinRuntime`, `@string defaultMsysRuntime` (`C:/msys64/usr/bin/msys-2.0.dll`), `@string defaultCygwinRuntime` (`C:/cygwin64/bin/cygwin1.dll`) next to `shell` and `defaultShellWindows`, in their exact row shape.
2. Add two rows to the settings table after `shell` (`default-eve.md:217`): `msys runtime` and `cygwin runtime`. Copy the column shape of the `shell` row. The value column holds the shipped default path (`default-eve.md:219`, `:221`), as ARCHITECT ruled: *"msys-2.0.dll and cygwin1.dll must be explicitly written at eve.md"*. The description names `eve-cygwinPTY` as the process that loads the runtime.
3. Regenerate with `cast cast/spell.md --debug`.
**Inventory:** settings-key family; identifier family; `getDefaultValueForSettings` pattern.
**Validation:** The rows match the `shell` row shape. The generated identifiers exist. macOS reads the rows and does nothing with them.

### Step 3: No change (correction)
**Correction:** The input lane gives its bytes to `TerminalTTY::write` and keeps them at the same address until the TTY accepts them: *"A TTY that writes asynchronously can rely on that address"* (`jam_TerminalFIFO.h:79-80`, `writePendingData (TerminalTTY&)`). Step 4 writes from that address with an overlapped `WriteFile`. EVE never calls `writePendingData (Handle)`, so this step changes nothing. A second ring in the TTY would copy the same bytes again, and MANIFESTO forbids that (*"A copy per hop"*).

### Step 4: `jam::TerminalCygwinTTY`
**Scope:** new `jam_terminal/tty/native/jam_TerminalCygwinTTY_windows.{h,cpp}`; the include in `jam_terminal.h`. Two shared members that the backend needs were added outside the first scope list: `TerminalFIFO::read (Handle, char*, int)` (`jam_TerminalFIFO.h:139`) and `TerminalConsole::createProcess` (`jam_TerminalConsole_windows.h:55`). Each one avoids a second copy of code that exists (CODING.md DRY).
**Action:**
1. Implement `TerminalTTY`. Constructor arguments: the helper file and the runtime file.
2. `open` creates two pipes with `FILE_FLAG_OVERLAPPED` on EVE's ends: one for the helper's stdin, one for its stdout. It creates a third pipe for the window size. It starts the helper with the runtime path, the size pipe handle, the packed start size (`TerminalWinsize::pack`, built from `dims` with pixel size 0, as `jam_TerminalTTY_posix.cpp:211` does), the shell, its arguments, the working directory and the environment. The helper command line is: runtime path, size pipe handle, packed start size, shell, shell arguments.
3. `read` is the device read that `TerminalTTY::read` returns. It calls `TerminalFIFO::read (Handle, char*, int)` on the stdout pipe. `writeFromTerminalTTY (TerminalTTY&)` calls it with the free space of the ring. Readiness uses `AsyncOutputStream` (pipe row), and `onBytesReady` fires from it.
4. `write` writes from the caller's address with an overlapped `WriteFile` on the stdin pipe. When a write is pending, the call checks it with `GetOverlappedResult (..., FALSE)`: complete gives its count, incomplete gives 0. When no write is pending, the call starts one; immediate completion gives its count, a pending start gives 0. It never waits (`jam_TerminalFIFO.h:79-80`).
5. `setWinsize` writes the `jam::TerminalWinsize` bytes into the size pipe.
6. `close` closes the pipes. The helper ends on EOF of its stdin.
**Inventory:** TTY backend family; `AsyncOutputStream` pipe row; `TerminalFIFO`; Windows TTY pattern (`jam_TerminalTTY_windows.cpp:55-59`).
**Validation:** No wait on the tick path. No thread is started. One object, one TU. Every name is in this plan.

### Step 5: Helper project and runtime loader
**Scope:** new `eve/cygwinPTY/`: `project-info.md`, `cast/spell.md`, `cast/cmake.cast`, `Source/Main.cpp`.
**Action:**
1. Copy the whatdbg console-app project files (`project-info.md:45`, `cast/cmake.cast:111`) with these substitutions: product `eve-cygwinPTY`, module list `jam_core`, `jam_terminal`.
2. The runtime padding is reserved by a custom entry point, `cygwinPTYCRTStartup` (`Main.cpp:19-23`, linker `/ENTRY:cygwinPTYCRTStartup` at `cygwinPTY/cast/cmake.cast:188-190`), the form of `cygload.cc:101-104` ("Even subverting the entry point"). `main` keeps the `Runtime` until the message thread is joined. `Runtime` loads the runtime path from the first argument with `juce::DynamicLibrary`, then calls `msys_dll_init` or `cygwin_dll_init`. It chooses by the exported symbol that `getFunction` finds.
**Inventory:** console-app pattern; runtime loader pattern; `juce::DynamicLibrary`.
**Validation:** `cast cygwinPTY/cast/spell.md` builds `eve-cygwinPTY.exe`. EVE's own cast is unchanged.

### Step 6: Helper pty, shell and relay
**Scope:** `eve/cygwinPTY/Source/`.
**Action:**
0. The helper runs its message loop on a runtime pthread (`Main.cpp`). `Session` runs the master read on a second runtime pthread (`Session.cpp` `run`). The master is a runtime fd, and the read is the runtime's `read` (`Runtime.h`).
1. `openpty` with the start size (`Session::getWinsize` of the packed start size).
2. `ioctl (slave, TIOCSCTTY, 0)`. The helper starts with `sid == pgid == pid` and no ctty (`pinfo.cc:98-100`), so the call succeeds (`termios.cc:758-759`), and the pty session becomes the helper's (`pinfo.cc:582-583`).
3. Save fd 0-2 with `dup`. Put the slave on fd 0-2 with `dup2`. Call `spawnv (_P_NOWAIT, shell, …)`. Put the saved fds back with `dup2`. The child copies sid, pgid and ctty (`sigproc.cc:260-262`). Putting the fds back restores the std handles that `StdioServer` reads (`jam_StdioServer.cpp:11-18`).
3a. Job control, found at run time with a trace and fixed from the runtime source. After the spawn, `Session::openShell` ignores `SIGTTIN` and `SIGTTOU` (`signal`), because the master read and write call `bg_check` (`pty.cc:2464`, `:2238`) and an orphaned group gives `EIO` (`termios.cc:243-257`). It then calls `setpgid (shell, shell)` and `tcsetpgrp (slave, shell)` before the relays open, so the first `TIOCSWINSZ` does not signal the helper's own group, which waits `WSSC` = 60000 ms (`pty.cc:2526-2537`, `sigproc.cc:29`, `:844-847`).
3b. The shell process handle is a `jam::AsyncUpdater::Source` (`Session.h` `shellExit`). When the shell exits, `Session::releaseTerminal` calls `setpgid (0, shell)` and `setsid`, which release the controlling terminal of the pty (`syscalls.cc:1215-1232`, `cygheap.cc:130-137`), so the master read ends and the helper exits.
4. `StdioServer::open (nullptr, …)` relays stdin bytes to the master. Master bytes go to stdout through `StdioServer::write`.
5. An `AsyncOutputStream` on the size pipe applies `ioctl (master, TIOCSWINSZ)`.
6. The helper ends when the shell exits or stdin reaches EOF.
**Inventory:** runtime API rows; StdioServer pattern.
**Validation:** No `fork`. No hand-rolled relay loop. ARCHITECT confirms at run time: ^C, job control and resize.

### Step 7: EVE selects the backend
**Scope:** `Source/EVETerminalProcessor.cpp` (`createTTY` `:67-88`, `openTTY` `:388-448`).
**Action:**
1. Read `msys runtime` and `cygwin runtime` with `getDefaultValueForSettings` and the Step 2 defaults.
2. The shell runs on a runtime when that runtime file exists in the shell's directory. Then `createTTY` makes `jam::TerminalCygwinTTY` with the helper file and that runtime. Otherwise it makes `TerminalWindowsTTY`.
3. When the configured runtime does not exist, use the fallback list `powershell.exe`, then `cmd.exe` (`:432`), the same as a missing zsh.
4. The helper file is the one written in Step 8.
**Inventory:** Windows TTY wiring; fallback pattern; settings pattern.
**Validation:** The macOS path is unchanged. No `==` on strings. The replay wrapper (`:83-87`) still wraps the backend.

### Step 8: Ship the helper with every format
**Scope:** `cygwinPTY/project-info.md`, `cygwinPTY/cast/{spell.md,cmake.cast}`, `project-info.md`, `cast/{spell.md,cmake.cast,files.md}`, `Source/ShellIntegration.{h,cpp}`, `Source/EVETerminalProcessor.cpp`.
**Ruling (2026-11-18):** ARCHITECT: *"make it identical then. we have working pattern, why deviates"*. The pattern is the EVE to END sync. The first form (a glob of `cygwinPTY/Builds/${CMAKE_BUILD_TYPE}/…`) embedded no helper in a Release build, because nothing built the Release helper.
**Action:** Mirror the EVE to END sync.
1. The helper's cast packs `eve-cygwinPTY.exe` with `cast --pack` into `Source/windows` of EVE after each build. This is the same as EVE's pack into `../end/Source/plugins` (`cast/cmake.cast:511-541`, `project-info.md:217-227`).
2. EVE's cast globs `Source/windows/*.zip`, filters by the pack platform, and requires one archive on Windows. It hashes the archive into `CAST_CYGWIN_PTY_HASH`. This is the same as END (`end/cast/cmake.cast:370-397`).
3. EVE unpacks the archive at run time into the shell folder when the stamp differs or the helper is absent. This is the same as `Nexus::getOrCreatePlugin` (`end/Source/plugins/Nexus.cpp:200-241`).
**Inventory:** EVE to END pack and offload pattern.
**Validation:** Standalone, VST3 and a hosted instance each find the helper. No copy step per format.

### Step 9: Audit and doxygen
**Scope:** all files above.
**Action:** @Auditor runs once. COUNSELOR validates each finding. Then Pathfinder does the doxygen pass, including the JAM blocks that name members that Step 1 deleted. Regenerate the JAM doxygen.
**Validation:** Every finding is resolved or rejected with a read. ARCHITECT runs `seq` in zsh, pwsh and cmd.

## BLESSED Alignment

- **B:** The helper owns the runtime, the pty and the shell. Each EVE instance owns one helper. Pipes are RAII handles.
- **L:** One backend object, one helper. No new thread in EVE.
- **E:** Runtime paths are settings with shipped defaults. Nothing is hardcoded twice.
- **S (SSOT):** The pty size lives in the runtime. EVE sends it on change. No shadow copy.
- **S (Stateless):** `isServerOpen` is removed in Step 1.
- **E (Encapsulation):** EVE sees only `TerminalTTY`. Runtime details stay in the helper.
- **D:** The tick never waits (Step 4.4). Bytes cross each boundary once: master to the helper's stdout, and the pipe into EVE's ring (MANIFESTO permitted copy 1).

## Risks / Open Questions

- Run-time proof, not decisions: Step 6.2-6.3 rest on read code paths (`termios.cc:758-759`, `sigproc.cc:260-262`). ARCHITECT's test confirms them. A failure goes to a trace through whatdbg (trace skill), not to a new design.
