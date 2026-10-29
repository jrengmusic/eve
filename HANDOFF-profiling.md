## Handoff to COUNSELOR: profiling — fix the TTY re-open crash, then close the `login` grandchild check

**From:** MACHINIST
**Date:** 2026-10-29
**Status:** Blocked (waits for ARCHITECT to rule option A or B on the crash fix)

This entry replaces `~/.carol/HANDOFF-profiling.md` (dated 2026-10-28). That file is stale. Section Open Questions item 8 covers it.

### Read First
- `~/Documents/Poems/dev/scout/PLAN-profiling.md` — the locked plan. Rulings at lines 12-24. Amendment A (Step 12 evidence and rulings) and Amendment B (audit dispositions).
- `~/Documents/Poems/dev/scout/RFC-profiling.md` — R2 (state of every requirement), Open Question 12 (the `login` grandchild).
- `~/Documents/Poems/dev/jam/jam_terminal/tty/native/jam_TerminalTTY_posix.cpp:233-312` — `open`, `close`, `isRunning`. Also `:204-231` (`addReadSource`).
- `~/Documents/Poems/dev/jam/jam_terminal/tty/native/jam_TerminalTTY_windows.cpp:656-797` — `open`, `close`, `isRunning`.
- `~/Documents/Poems/dev/jam/jam_terminal/tty/jam_TerminalTTY.h:89-98` — the `open` and `close` contract.
- `~/Documents/Poems/dev/eve/Source/EVETerminalProcessor.cpp:32-36` (quit on `shellExited`), `:168-194` (`openTTY`, profile branch).
- `~/Library/Logs/DiagnosticReports/EVE-2026-10-29-001237.ips` and `EVE-2026-10-29-001346.ips` — the crash evidence. 17 reports exist: `ls ~/Library/Logs/DiagnosticReports/EVE-2026-10-29-*.ips | wc -l` gave 17.
- `~/.carol/skills/profile/SKILL.md:313-320` — the `## Limits` text that says the grandchild check is not verified.
- `~/Documents/Poems/dev/scout/Source/HELP.md` — the built scout contract.

### Context
The profiling sprint (scout runs the manifest's `## toolchain`, then the epochs) is built and committed. This session verified the built state against the plan. The check of RFC Open Question 12 (does `RUSAGE_CHILDREN` count a descendant of EVE, through `/usr/bin/login`) could not finish. The profile build of EVE crashes at random in `jam::TerminalUnixTTY::open`. The crash blocks every scout run that uses a short-lived shell command.

### Completed
- **Scout is updated and committed.** `git log` shows `2330495` "Make scout identical to cast and run one command per profile" (2026-10-28 21:35). `scout --version` printed `scout 0.1.0 (2330495)`. The 2026-10-28 handoff said "Blocked, waits for scout update". That state is gone.
- **Plan steps 0-11, 13 and 14 are in the tree.** Evidence: `Source/Toolchain.h/.cpp`, `Validator.h/.cpp`, `Processor.h/.cpp` exist; `skills/trace/epoch.lua` exists; nvim commit `ffc9fde` ("Give clangd the benchmark compile database and rename kind to type"); `.carol` commit `491b987`; Amendments A and B are in `PLAN-profiling.md`.
- **Step 7 verified** (Pathfinder, 2026-10-29 00:04-00:05, scratch folder `.../scratchpad/step78`). Each case matched the Step 4 table and `HELP.md`:
  - `scout --help`: guide, exit 0.
  - `scout`: guide, exit 0.
  - `scout missing.md`: guide, stderr `scout: not found: missing.md`, exit 1.
  - `scout empty.md` (0 bytes): no output, exit 0. The 2026-10-28 handoff expected exit 1. Plan ruling 2 (copy cast) gives exit 0.
  - `scout --version`: `scout 0.1.0 (2330495)`, exit 0.
  - `scout empty.md --bogus`: stderr `scout: --bogus: unknown flag`, exit 1.
  - A manifest whose `output` folder does not exist: exit 0, `does-not-exist/report.md` created (237274 bytes).
- **Step 8 verified** (Pathfinder, same time). `scout /Users/jreng/Documents/Poems/dev/eve/benchmark/replay.md`, run from the EVE root: exit 0, stderr empty, `buildType` Release, report 10 s old. Medians: `epoch` 1.849479583333333e9 ns (errorPercent 1.690142114301969), `user` 1.024419e9 (1.659184376705235), `system` 1.082017e9 (0.6708767052643351), `dropped` 0.0. The toolchain output streamed (cast, cmake, ninja). ninja printed "no work to do". The run did not recompile.
- **Step 10 record verified.** `skills/profile/SKILL.md:313-320` quotes `epoch` 6.7365e10, `user` 9.4932e10, `system` 8.9613e10. `eve/benchmark/live-report.md` rows hold `6.736519520833334e10`, `9.4931742e10`, `8.9613302e10`. The numbers match. `buildType` is Release (`live-report.md:1662`).
- **Step 11 verified.** `grep -c JAM_USING_PROFILER eve/compile_commands.json` gave 89.
- **Step 15 verified** (documentation pass for scout).
  - The scout build has no doxygen target: `ninja doxygen` printed `ninja: error: unknown target 'doxygen'`.
  - I ran doxygen with the template `~/.config/nvim/doxygen/Doxyfile.project`, `INPUT` = `scout/Source`, output and `WARN_LOGFILE` in the scratchpad. Result: 44 XML files, `warn.log` empty (0 lines).
  - I read all seven `.cpp` files (`Benchmark`, `Processor`, `Toolchain`, `Validator`, `Help`, `ProfileReport`, `Statistics`). None holds a doxygen block. All seven headers hold doxygen blocks. `Main.cpp` documents its file-static functions (PLAN Amendment B, rejected finding 15).
- **Stale EVE instance killed on ARCHITECT's word.** PID 12374 (`eve/Builds/Release/.../EVE`, age 1 day 6 hours, parent PID 1) and its children 12376 (`login`) and 12377 (`zsh`). `ps` afterwards showed no EVE process.

### Crash evidence (verified)
- **Symptom.** The profile build of EVE (`eve/Builds/Profile/EVE_artefacts/Release/Standalone/EVE.app/Contents/MacOS/EVE <manifest>`) exits 133 (SIGTRAP). macOS shows "EVE quit unexpectedly" for each crash. ARCHITECT's desktop got one dialog per crash.
- **Crash report** (both reports have the same frames). `exception`: `EXC_BREAKPOINT`, `SIGTRAP`. `asi`: `BUG IN CLIENT OF LIBDISPATCH: Release of a suspended object`. Faulting thread: `JUCE v8.0.14: Message Thread`. Frames, top first:
  1. `libdispatch _dispatch_queue_xref_dispose.cold.1`
  2. `_dispatch_queue_xref_dispose`
  3. `_dispatch_xref_dispose`
  4. `dispatch_release`
  5. `jam::TerminalUnixTTY::open(...)`
  6. `EVETerminalProcessor::openTTY(int, jam::TerminalWinsize, dispatch_queue_s*)`
  7. `EVETerminalProcessor::prepare(juce::dsp::ProcessSpec const&, dispatch_queue_s*)`
  8. `EVEProcessor::prepareToPlay(double, int)`
  9. `juce::AudioProcessorPlayer::setProcessor`
  10. `juce::AudioProcessorPlayer::audioDeviceAboutToStart`
  11. `juce::MessageManager::callAsync<EVEProcessor::EVEProcessor()::$_2>` (the message callback)
- **Code reading** (deduction from the source, not a run):
  - `open` returns early only when `isRunning()` is true (`jam_TerminalTTY_posix.cpp:237`).
  - `isRunning()` runs `waitpid (childProcess, nullptr, WNOHANG) == 0` (`:308`). A shell that already exited makes it false, and the call reaps the child.
  - A second `open` then forks again (`:247`). It overwrites `master` and `childProcess` with no teardown. `addReadSource` replaces `readSource` (`:209`).
  - The read handler suspends the old source (`:215-216`). `read` resumes it only after a drain (`:334-335`). The old source is still suspended when `readSource.reset` releases it. libdispatch traps.
  - A first `open` cannot trigger this, because `readSource` is null then.
  - The documented contract is "When the shell already runs, the call changes nothing and returns true" (doxygen XML, `classjam_1_1TerminalUnixTTY.xml`, member `open`).
  - Windows has the same shape: `bool result { isRunning() };` (`jam_TerminalTTY_windows.cpp:660`). The re-open does not call `close()` first. `close()` tests `process != INVALID_HANDLE_VALUE` (`:739`).
  - `isRunning` has no `referencedby` entry outside `open` in the jam doxygen XML. The compiler has not confirmed this.
- **Observed rate** (not characterised):
  - Two direct runs exited 133 at 00:12-00:13: `control.md` in the scratch folder, and the same manifest copied into `eve/benchmark/`.
  - Then 11 direct runs exited 0 (`seq.md` once, `control.md` once, then six `control.md` and three `seq.md` in one loop).
  - Through scout, `control` (shell `-c "true"`) exited 1 with `scout: .../EVE: command failed` three times. `burn` hung twice.
- **Second symptom, cause not established.** In the first `burn` run EVE (PID 45146) ran 2 minutes. Its shell child 45147 was `<defunct>` (a zombie). EVE did not quit. `sample 45146` showed the main thread in the AppKit event loop. The sample header reported `Physical footprint: 1.6G`, peak `1.7G`. I stopped it with `TaskStop`. The second `burn` run hit my 120-second watchdog (exit 143).

### Remaining
1. **ARCHITECT:** rule option A or B (Open Questions item 1). Owner: ARCHITECT.
2. **COUNSELOR:** apply the ruled fix. This is jam source. If B is ruled, the edit set is:
   - `jam_TerminalTTY_posix.cpp:237`: replace `if (isRunning()) return true;` with a session-exists test (`childProcess > 0`, the test `close()` uses at `:268`).
   - Delete `isRunning` in `jam_TerminalTTY_posix.cpp:302-312` and `jam_TerminalTTY_posix.h:164` if the compiler shows no other caller.
   - `jam_TerminalTTY_windows.cpp:660`: replace `isRunning()` with the session-exists test (`process != INVALID_HANDLE_VALUE`). Delete `isRunning` at `:786-797` and `jam_TerminalTTY_windows.h:249` under the same condition.
   - Update the `open` contract text in `jam_TerminalTTY.h`. Doxygen is written last (CODING, Doxygen Discipline).
   - Windows is not built on this machine. Verify the Windows edit against the compiler on a Windows machine.
3. **COUNSELOR or MACHINIST (build grant):** rebuild the profile build. Rows from `eve/benchmark/live.md:32-36`, run from `~/Documents/Poems/dev/eve`: `cast ../jam/cast/spell.md`, then `cmake -S . -B Builds/Profile -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF -DCAST_PROFILE=ON`, then `ninja -C Builds/Profile`.
4. **Check the crash is gone.** Run one EVE at a time and tell ARCHITECT before each run. Each crash shows a macOS dialog. Command: `scout <abs>/control.md`, where the manifest has epochs 3, the `command` cell is the absolute path of the profile EVE, the `profiler` table has shell `/bin/zsh`, and the `args` cell is `-c "true"`. Repeat enough times to show a stable result. The 133 rate is not known (see Crash evidence).
5. **Close the `login` grandchild check** (RFC Open Question 12). Fixtures from this session, in `/private/tmp/claude-501/-Users-jreng--carol/574cb4c0-b41b-48a7-a88d-e3f140ab42ea/scratchpad/gc/`: `control.md`, `burn.md`, and the file `big` (1572864000 bytes of zeros). The folder is session scratch and can vanish. Rebuild it like this:
   - `burn.md` is `control.md` with the `args` cell `-c "shasum -a 256 <abs path of big>"`.
   - Standalone baseline: `time shasum -a 256 big > /dev/null` gave 2.61 s user, 0.17 s system, 2.833 s total.
   - Run `scout <abs>/control.md`, then `scout <abs>/burn.md`. Read the `user` row of each report.
   - If `RUSAGE_CHILDREN` counts the descendant, the `user` median of `burn` exceeds the `control` median by about 2.6e9 ns. If the difference is near 0, the descendant is not counted.
   - The zombie observation (Crash evidence, second symptom) predicts "not counted" when EVE does not reap its shell. This is an inference, not a measurement.
6. **Record the result** in `~/.carol/skills/profile/SKILL.md` `## Limits`. Replace the sentence "Whether `RUSAGE_CHILDREN` includes the `seq` grandchild is still not verified." (lines 319-320).
7. **ARCHITECT:** decide the commit. Items under Files Modified list the uncommitted changes.

### Key Decisions
ARCHITECT's rulings in this session, verbatim:
- *"kill"* — kill the stale EVE instance 12374.
- *"1"* — for the crash dialogs: ARCHITECT clicks Ignore; no machine setting changes.
- *"write handoff at eve repo"* — destination of this file.
- *"READ FIRST THOROUGHLY"* and *"why always half ass, dont leeave any open not verify"* — every claim is verified or listed as open.

Rulings that still bind (from `PLAN-profiling.md` and `RFC-profiling.md`):
- *"profiling with scout is granted with build permission"* (RFC item 4).
- *"i dont need to build profile with nvim. with profiling protocol, agent granted permission to build. profiling is always agents building"* (RFC item 9).
- *"correct mental model: scout is IDENTICAL to cast on everything with --help, building, manifest, etc. the only divergence are cast generate code from table-template. while scout build specifically for profiling no other options and output report in markdown with mermaid chart"* (PLAN ruling 2).
- *"Keep 100M"* — live epoch size (PLAN ruling 5).
- *"Mirror cast"*, *"Split: epoch.lua"*, *"rename kind→type"*, *"fix Subprocess cwd"* (PLAN Amendment B).

### Files Modified
- `eve/HANDOFF-profiling.md` — this file (new).
- `~/.carol/agents/pathfinder.md` (Profile protocol) — adds the build exemption: the `scout` run builds the target from the manifest's `## toolchain`, under the build grant.
- `~/.carol/agents/counselor.md:251-255` — the profiled target builds inside the `scout` run; Pathfinder builds nothing except those rows.
- `~/.carol/skills/profile/SKILL.md` — the `EXIT_SUCCESS` row names the empty manifest (verified in Step 7); `## Limits` adds the EVE trap with its evidence pointer. All three `~/.carol` edits are uncommitted.
- `eve/benchmark/replay-report.md` and `eve/benchmark/replay-samples.md` — rewritten by the Step 8 scout run. `git status` shows both as modified. They are scout output.
- `scout/Builds/Release/*` — 17 tracked build outputs differ from HEAD (`git status --short` in `scout`). Examples: `scout_artefacts/Release/scout`, `scout.pkg`, `build.ninja`, `compile_commands.json`. The mtimes: `build.ninja` and `CMakeCache.txt` 00:01:46, `scout` binary 00:02:54, `scout.pkg` 00:02:55. I ran no build then, so I do not attribute the rebuild to myself. `.ninja_log` has mtime 00:07:36. That time matches my `ninja doxygen` call, which failed with "unknown target". Attribution is not verified.
- `~/Library/Logs/DiagnosticReports/EVE-2026-10-29-*.ips` — 17 crash reports from my EVE runs. Outside any repo.
- Scratch only: `.../scratchpad/step78/`, `.../scratchpad/gc/`, `.../scratchpad/doxy/`. I created `eve/benchmark/zz-control-test.md` for one run and deleted it with `rm -f`; I also ran `rm -f` on `eve/benchmark/control-samples.md`. `git status` for `eve` lists only the two replay files, so neither file remains.
- No source file in `jam`, `eve` or `scout` was edited this session. `git status` in `jam` shows 0 changes.

### Open Questions
1. **Crash fix, A or B.** Decider: ARCHITECT.
   - A: call `close()` before the fork in `open`. It tears the old session down. After a shell exit, a second `prepareToPlay` starts a second shell with the same `args`. A profile run would then run the workload twice in one epoch.
   - B: return early when a session exists (`childProcess > 0`; Windows `process != INVALID_HANDLE_VALUE`). Shell exit stays visible through `read`. `open` still works after `close()`. `isRunning` loses its only caller.
   - MACHINIST recommended B, for MANIFESTO D (determinism). ARCHITECT has not ruled.
2. **Why EVE did not quit after its shell exited** (the zombie, Crash evidence second symptom). `EVETerminalProcessor.cpp:32-35` quits on `shellExited`. Which code sets `shellExited`, and whether it needs a read at EOF, is not read. Decider: COUNSELOR reads it. This affects item 5, because an unreaped child cannot add to `RUSAGE_CHILDREN`.
3. **Why `prepareToPlay` runs twice.** The stack shows `audioDeviceAboutToStart` → `setProcessor` → `prepareToPlay` from a `callAsync` in the `EVEProcessor` constructor. The first call is not in the report. Whether a device restart or a second prepare call causes it is not read. Decider: COUNSELOR reads it.
4. **Crash rate.** 2 traps in the first 3 direct runs, then 11 clean direct runs, then repeated failures through scout. No factor is established (not the stale instance: the first traps came after it was killed; not the folder: the trap repeated in `eve/benchmark/`). Decider: the check in Remaining item 4.
5. **Memory.** The `sample` header of the hung EVE reported a 1.6G physical footprint (peak 1.7G). The test file `big` is 1.5G. Whether the two relate is not read. Decider: COUNSELOR, only if the footprint repeats.
6. **Windows `isRunning` callers.** Doxygen shows none outside `open`. The Windows compiler has not confirmed it. Decider: the Windows build.
7. **Replay and live numbers after the fix.** The Step 8 and Step 10 numbers came from builds with the crash present. The crash shows only on a second `open`; the recorded runs finished with exit 0. Whether the fix changes them is not measured. Decider: a rerun after item 3.
8. **Stale file `~/.carol/HANDOFF-profiling.md`.** It carries the same subject. Its status "Blocked (waits for ARCHITECT to update scout)" is wrong. Deleting it is a destructive step. Decider: ARCHITECT.

### Next Steps
- Trigger: ARCHITECT rules A or B on Open Question 1.
- Then do Remaining items 2 to 6 in order. One EVE run at a time. Tell ARCHITECT before each run.
- COUNSELOR owns item 2 (source). MACHINIST can run items 3 to 6 after the fix.
