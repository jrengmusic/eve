# PLAN: Terminal Clock — EVETerminalProcessor Owns Its Clock in Every Host

**RFC:** none — objective from ARCHITECT's prompt (session 2026-10-29)
**Date:** 2026-10-29
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM (LANGUAGE.md "C++ / JUCE": one object, one translation unit)

## Overview

`EVETerminalProcessor` owns its clock, `jam::VirtualDevice`, and is the clock's `juce::AudioIODeviceCallback`. Above the seam everything stays JUCE's contract: the host tells `EVEProcessor::prepareToPlay` / `releaseResources`, and `processBlock` carries audio only. The standalone device swap, the reach into `StandalonePluginHolder`, the two `std::function` hops, the player lock on the tick and the binary branch of the history writer all go. The wiring is the same in every host (standalone, VST3, AU, END).

ARCHITECT, verbatim:
- *"EVETerminalProcessor should own the VirtualDevice, hence prepareToPlay upstream still juce contract, we only take over the control at seam we need to drive ourselves"*
- *"dont conflate VirtualDevice == actual analog drivers. it's not. VirtualDevice exists as our clock, since the architecture must use a device. we need our own clock and drivers, so the question is how can we take over control without fighting juce, but seamlessly integrate our design into it"*
- *"it SHOULD NOT MATTER. what binary we're using as host-client. VirtualDevice as full throttle clock is EVE SPEC contract"*
- *"why vblank must be branched? vblank at all build contract are exactly the same"*
- History: *"fixed names + git. but date could be included as metadata on the report no? if scout not have the feature yet, write RFC to scout repo, i will work on it in parallel"*

## Language / Framework Constraints

- CODING.md CRITICAL RULES: no bail-out guards; positive nesting; `not/and/or`; brace init; no out-parameters; no non-owning naked pointer members; no `friend`; deadline threads do not allocate, lock or wait.
- CODING.md "Scope and ownership of values": a value used by one function lives in that function.
- CODING.md Code Hygiene: no comments or doxygen before the audit; doxygen pass is last.
- LANGUAGE.md C++/JUCE: bodies stay in each object's one `.cpp`.

## Dependency & API Inventory

JUCE (patched cache `~/.cache/cast/juce/b9a5c30…/modules/`):
- `juce::AudioIODeviceCallback` (`juce_AudioIODevice.h:66-143`): `audioDeviceIOCallbackWithContext`, `audioDeviceAboutToStart` (*"called … after the device has been restarted … find out the sample rate and block size"*, :120-133), `audioDeviceStopped`.
- `juce::AudioIODevice::stop` contract: *"Any pending calls to the callback class will be flushed before this method returns"* (:260-265).
- JUCE's own self-callback owner: `class StandalonePluginHolder : private AudioIODeviceCallback` (`juce_StandaloneFilterWindow.h:54`). No JAM/END/EVE class derives from it today (doxygen scan; JUCE hits only).
- Standalone shutdown (EVE runs `EVEStandaloneApp : jam::AudioStandaloneApp`, `EVEStandaloneApp.h:7`): `AudioStandaloneApp::shutdown` → `mainWindow = nullptr` (`jam_AudioStandaloneApp.cpp:142-146`) → `MainWindow::pluginHolder` destroyed (`jam_AudioStandaloneApp.h:111`) → `~StandalonePluginHolder` → `handleDeletePlugin` → `stopPlaying` (`juce_StandaloneFilterWindow.h:129-137`, `:456-460`) → `player.setProcessor (nullptr)` (:277-280) → `oldOne->releaseResources()` (`juce_AudioProcessorPlayer.cpp:183-191`); then the holder deletes the processor (:459).
- Player lock removed from the tick: `AudioProcessorPlayer::audioDeviceIOCallbackWithContext` starts with `const ScopedLock sl (lock);` (`juce_AudioProcessorPlayer.cpp:235`).

JAM:
- `jam::VirtualDevice` (`jam_audio_devices/audio_io/jam_VirtualDevice.h/.cpp`): `open`, `start (AudioIODeviceCallback*)` calls `audioDeviceAboutToStart` (`.cpp` `start`), `stop` = `devicePlaying = false; cancelPendingUpdate(); callback->audioDeviceStopped(); callback = nullptr`, `clock (int)` checks the atomic `devicePlaying` (`.cpp:76-82`), `requestProcess`, `getContext`.
- `jam::AsyncUpdater` (`jam_core/concurrency/jam_AsyncUpdater.h/.cpp`): `cancelPendingUpdate` = *"Drops the queued call and waits until a callback in progress finishes"* (h:46). macOS: `dispatch_sync_f (queue, nullptr, noop)` barrier (cpp:73-79) — it also drains the TTY read source, which targets the same queue (`jam_TerminalTTY_posix.cpp:209`). Windows: `WaitForThreadpoolWorkCallbacks (work, FALSE)` only (cpp:81-83); the TTY readiness is a thread-pool wait on the same one-thread pool (`jam_TerminalTTY_windows.cpp:133-136`, `:695-700`; pool max 1 thread, `jam_AsyncUpdater.cpp:22-28`).
- Release family for a chain object that owns an execution context: `kuassa::SpectrumProcessor::releaseResources()` stops its thread (`kuassa_SpectrumProcessor.h:111-114`); JUCE `AudioProcessor::releaseResources`.

EVE (current tree):
- `EVEProcessor.h:111-112` `device`; `:116` `spec`; `EVEProcessor.cpp:4-5` `virtualSampleRate`, `virtualBlockSize`; `:29-57` standalone wiring; `:67` `device.stop()` in the profile destructor; `prepareToPlay` (suspend → `terminalProcessor.prepare (spec, device.getContext())` → resume); `releaseResources` empty; `process<T>` calls `terminalProcessor.process()`; `setStateInformation` suspend → `setState` → `model.setState` → resume → `prepare()`.
- `EVETerminalProcessor.h:57` public `prepare (spec, context)`; `:60` public `process()`; `:71-75` `onProcessRequest`, `onBytesReady`; `:139` private `prepare (spec, winsize, historyCapacity, context)`; `:153` `openTTY (blockSize, winsize, context)`; `:165` `tty`.
- `EVETerminalProcessor.cpp:23` readiness forward; `:96-99` writer branch on `wrapperType`; `:110-111`, `:131-132`, `:143-144`, `:232-233` `onProcessRequest` sites; `:157-212` prepare bodies; `:214-222` `process`; `:224-234` `clear` (uses `processor.suspendProcessing`).
- Identifier `deviceClock` (`cast/identifiers.md:26`).

## Validation Gate
Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

## Steps

### Step 1: Plan file and scout RFC
**Scope:** `eve/PLAN-terminal-clock.md` (new, this plan); `scout/RFC-report-date.md` (new).
**Action:** COUNSELOR writes this plan to the EVE root. COUNSELOR loads the `rfc-writer` skill and writes the scout RFC: the report's `## RUN` table gains the run date, so a report kept under a fixed name in git carries when it was measured. Evidence: the `## RUN` table holds `buildType` only (`~/.carol/skills/profile/SKILL.md:213`; `eve/benchmark/live-report.md:1657-1663`). Quote ARCHITECT's history ruling.
**Validation:** RFC follows the `rfc-writer` format; every claim cites file:line; no scout source edited.

### Step 2: VirtualDevice stop flushes on Windows (JAM)
**Scope:** `jam/jam_core/concurrency/jam_AsyncUpdater.cpp` (Windows branch of `cancelPendingUpdate` only).
**Action:** After `shouldDeliver.store (0)`, submit the work item, then wait for it: `SubmitThreadpoolWork (work.get()); WaitForThreadpoolWorkCallbacks (work.get(), FALSE);`. `deliver` reads 0 and calls nothing. On the one-thread pool the work runs only after the callback in progress (incl. the TTY readiness wait) finishes — the Windows twin of the macOS barrier at `:78`. This makes `VirtualDevice::stop` meet JUCE's flush contract (`juce_AudioIODevice.h:260-265`) on both platforms.
**Validation:** macOS branch untouched; `cancelPendingUpdate` doc (h:46) now true on both platforms; no new name. Windows compile is not available on this machine — recorded for a Windows build.

### Step 3: EVETerminalProcessor owns the clock (EVE)
**Scope:** `eve/Source/EVETerminalProcessor.h/.cpp`.
**Action:**
1. Bases add `private juce::AudioIODeviceCallback` (sibling: `juce_StandaloneFilterWindow.h:54`).
2. Member `jam::VirtualDevice device { jam::VirtualDeviceType::virtualDeviceTypeName, jam::VirtualDeviceType::virtualDeviceTypeName };` declared before `tty` (the read source targets the device's queue; `tty` is destroyed first).
3. Public `void prepare();` replaces `prepare (spec, context)`: `device.stop(); device.open ({}, {}, virtualSampleRate, virtualBlockSize); device.start (this); device.requestProcess();`. `virtualSampleRate { 48000.0 }` and `virtualBlockSize { 65536 }` move from `EVEProcessor.cpp:4-5` into this function as `static constexpr` (narrowest scope).
4. Public `void releaseResources();` = `device.stop();`.
5. `audioDeviceAboutToStart (juce::AudioIODevice*)` (parameter unnamed: it is the member `device`): builds `juce::dsp::ProcessSpec spec { device.getCurrentSampleRate(), static_cast<juce::uint32> (device.getCurrentBufferSizeSamples()), static_cast<juce::uint32> (device.getActiveOutputChannels().countNumberOfSetBits()) }` and runs the body of today's public `prepare (spec, context)` (`cpp:157-166`). `audioDeviceIOCallbackWithContext` calls `process()`. `audioDeviceStopped` has an empty body (pure virtual).
6. The private overloads drop the `context` parameter; `openTTY` and the TTY `open` take `device.getContext()`.
7. Constructor: `tty->onBytesReady = [this] { (JAM_USING_PROFILER) const jam::Profiler::Zone zone { Id::deviceClock }; device.clock (0); };` replaces `cpp:23`.
8. Delete `onProcessRequest` and `onBytesReady` (h:71-75). Each former call site (`cpp:110-111`, `:131-132`, `:143-144`, `:232-233`) calls `device.requestProcess()` with no null test.
9. `process()` moves to `private:` (its only caller is the tick).
10. Getter (`cpp:96-99`): delete the `wrapperType` branch; the getter always calls `triggerAsyncUpdate()` (SPEC Core Principle 2: *"The clock and the history writer run on the serial event contexts of `jam::AsyncUpdater`"*).
11. `clear()`: `processor.suspendProcessing (true/false)` becomes `device.stop()` … `device.start (this); device.requestProcess();`.
12. Destructor: `jassert (not device.isPlaying());` (JUCE calls `releaseResources` before destruction: `juce_AudioProcessorPlayer.cpp:183-191`).
**Validation:** names used: `device`, `prepare`, `releaseResources`, `virtualSampleRate`, `virtualBlockSize`, JUCE overrides — all from existing families; no new name. No bail-out; no lock on the tick; `processor` member still used only by `openTTY` (profile) and the profile listener.

### Step 4: EVEProcessor is pure JUCE contract (EVE)
**Scope:** `eve/Source/EVEProcessor.h/.cpp`.
**Action:**
1. Delete `device` (h:111-112), `spec` (h:116), the constants (cpp:4-5) and the standalone block (cpp:29-57).
2. `prepareToPlay (sampleRate, samplesPerBlock)`: `juce::ignoreUnused (sampleRate, samplesPerBlock); terminalProcessor.prepare();`. The suspend bracket goes: the host `processBlock` no longer touches the terminal, so the guard has no threat (MANIFESTO B, Guard Rule).
3. `releaseResources()`: `terminalProcessor.releaseResources();`.
4. `process<T>`: delete `terminalProcessor.process();`; the body keeps the channel clear.
5. Profile destructor: delete `device.stop();` (cpp:67). The drain and `writeSamples` run after JUCE's `releaseResources` (standalone shutdown order above).
6. `setStateInformation`: call `terminalProcessor.releaseResources();` before `terminalProcessor.setState (state);` so the clock is stopped while the Model takes the restored values (Grand-Scheme §8: *"Each parameter has one writer thread"*). The existing `prepare();` restarts the clock. `suspendProcessing` stays around `model.setState` (APVTS host side).
**Validation:** no reference to `StandalonePluginHolder`, `wrapperType_Standalone` or `JucePlugin_Build_Standalone` remains in `EVEProcessor.cpp` outside the profile destructor; `EVEProcessor` owns no device; one prepare path (SPEC Core Principle 9).

### Step 5: Build and smoke run
**Scope:** `eve/Builds/Profile`, `eve/Builds/Replay`; no source.
**Action:** COUNSELOR tells ARCHITECT before each run. Pathfinder runs `scout <abs>/eve/benchmark/control.md` (3 epochs; builds through the manifest `## toolchain`; build grant). Then `scout <abs>/eve/benchmark/replay.md`. Report exit codes, new crash reports (`~/Library/Logs/DiagnosticReports/EVE-*.ips` count before/after), the `## EVE` tables, and zombie check.
**Validation:** build exit 0 with 0 warnings; control exit 0; 0 new crash reports; `dropped` 0 in replay.

### Step 6: Live 100M measurement
**Scope:** `eve/benchmark/live.md` run; reports overwrite under fixed names (git keeps history; ARCHITECT commits).
**Action:** COUNSELOR tells ARCHITECT before the run. Pathfinder runs `scout <abs>/eve/benchmark/live.md`. COUNSELOR reports `epoch`, `user`, `system`, `deviceClock`, `ttyBytesReady`, `terminalRead`, `terminalProcess`, `dropped` medians against the committed `live-report.md` (`git diff`).
**Validation:** run exit 0; numbers quoted from the new report with file:line.

### Step 7: SPEC and project docs
**Scope:** `eve/SPEC.md`, `eve/CLAUDE.md`, `eve/PLAN-Grand-Scheme.md` (§8 amendment block only).
**Action:**
- SPEC Core Principle 2: the clock's tick (`audioDeviceIOCallbackWithContext` of `EVETerminalProcessor`) reads and writes the PTY; `EVETerminalProcessor` owns the clock; ARCHITECT quotes above.
- SPEC Core Principle 10: the TTY chunk size is the clock's block size, set at `device.open`.
- SPEC Feature 2 writer bullet (`:146-149`): the writer runs on the processor's serial context in every host; quote *"vblank at all build contract are exactly the same"*.
- SPEC Architecture Constraints: add ✅ *EVETerminalProcessor owns its clock in every host; above the seam, JUCE's contract*.
- Grand-Scheme §8: amendment block `[AMENDED 2026-10-29 — the clock is the chain's own]`, quoting ARCHITECT; *"writing bytes is processBlock"* now reads as the clock's tick (`:1012-1013` already defines it as *"the clock that drains the bytes"*).
- CLAUDE.md `Source/EVEProcessor.h/.cpp` and `Source/EVETerminalProcessor.h/.cpp` rows.
**Validation:** every new decision line carries an ARCHITECT quote from this session; no other text changed.

### Step 8: Audit, doxygen, log
**Scope:** files of Steps 2–4.
**Action:** @Auditor sweeps once. COUNSELOR filters each finding (read + clause). Then the doxygen pass on the changed headers (`EVETerminalProcessor.h`, `EVEProcessor.h`, `jam_AsyncUpdater.h` doc unchanged unless a mismatch), 0 warnings. Sprint log on ARCHITECT's word; commit messages in chat.
**Validation:** every accepted finding resolved; doxygen 0 warnings.

## Amendment A — Clock lifecycle (audit, ARCHITECT `/go` 2026-10-29)

Audit findings 1, 2, 4, 5: the View started EVE's clock outside the host's `prepareToPlay` / `releaseResources` pair — `EVEProcessor::prepare` (`EVEProcessor.cpp:176-180`, gate `getBlockSize() > 0` stays true after a release), `clear()` (`EVETerminalProcessor.cpp:259-268`, block size 0 before any host prepare → division at `cpp:238`), and `setStateInformation` → `prepare()`. Rulings in conflict: *"make it consistent then"* (SPEC 9) and *"prepareToPlay upstream still juce contract"* (host pairs: VST3 `terminate` → `releaseResources`, `juce_audio_plugin_client_VST3.cpp:2628-2632`; AU dispose → `Cleanup` → `releaseResources`, `AUBase.cpp:127-133`, `juce_audio_plugin_client_AU_1.mm:239-247`). ARCHITECT approved the proposal with `/go`.

**Step A1 (EVE):** `eve/Source/EVETerminalProcessor.h/.cpp`, `EVEProcessor.h/.cpp`, `EVEView.cpp`.
1. Only the host pair starts and stops the clock: `prepare()` (from `prepareToPlay`) and `releaseResources()`.
2. `EVETerminalProcessor` gets a public template `restart (Callback&& callback)` in the header: `const auto isPlaying { device.isPlaying() }; device.stop(); callback(); if (isPlaying) { device.start (this); device.requestProcess(); }`. A no-argument `restart()` in the `.cpp` calls it with an empty lambda. `restart` comes from the JUCE/CLAP family (`AudioDeviceManager::restartLastAudioDevice`; CLAP `request_restart`, `jam_ClapPluginInstance::hostRequestRestart`: *"deactivate and then reactivate"*).
3. `clear()` = `restart ([this] { triggerAsyncUpdate(); handleUpdateNowIfNeeded(); terminalProcessor.clear (terminalBuffer); });`.
4. `EVEProcessor::prepare()` becomes `EVEProcessor::restart()` = `terminalProcessor.restart();` (no `getBlockSize` gate). `EVEView.cpp:56`, `:72` call `processor.restart()`.
5. `setStateInformation`: `suspendProcessing (true); terminalProcessor.restart ([this, &state] { terminalProcessor.setState (state); model.setState (state); }); suspendProcessing (false);` — the separate `releaseResources()` and trailing `prepare()` calls go.
6. Audit finding 7: `audioDeviceAboutToStart` reads the block size from `device` (no `juce::dsp::ProcessSpec` carrier); the private `prepare (spec)` is deleted; the private `prepare (winsize, historyCapacity)` reads `device.getCurrentSampleRate()` and `device.getCurrentBufferSizeSamples()`.
**Validation:** no path starts the clock except `prepare()`; `restart` restarts only a running clock; the build passes; a `control` scout run exits 0.

**SPEC 9 / Architecture Constraints:** init through `prepareToPlay`; resize, history capacity, clear and state restore restart the clock (stop → change → start) only while it runs.

**Audit dispositions:** accepted and resolved — 1, 2, 4, 5 (Step A1), 7 (Step A1.6), 15 and the stale-doc list (doxygen pass), 16 (constructor `prepare()` deleted), 19 (`SPEC.md:35`, `:113`, `:122`). Rejected with the refuting read — 3 (release before delete holds: VST3, AU, standalone citations above), 9 and 20 (PLAN decisions), 11 (the reformat and `//====` comment predate the Engineer dispatch: the pre-dispatch read showed them), 13 (`rEvent`/`wEvent` stay local until both are valid, `windows.cpp:667-675`, so `close()` is not equivalent), 17 (JFS sibling `FilterStripProcessor.h` has the same shape), 18 (`jam::ViewManager::Getters` signature), 19 at `SPEC.md:374` (`suspendProcessing` still guards `setStateInformation`), 21 (JUCE `releaseResources` is not `noexcept`). Open for ARCHITECT — 6 (standalone with no audio device never prepares), 8 (`processor` reference read only in profile builds), 10 and 12 (splits need a new name, NAMES Rule -1), 14 (Windows readiness callback named `process`).

## BLESSED Alignment
- **B:** one owner per object — the host owns `EVEProcessor`; `EVEProcessor` owns `EVETerminalProcessor`; `EVETerminalProcessor` owns its clock, TTY and chain. Destruction order follows declaration (device outlives TTY). `releaseResources` stops the clock before destruction (asserted at the owner).
- **L:** deletions dominate: the standalone block, two callback members, four null tests, the writer branch, the device and spec members in `EVEProcessor`.
- **E (Explicit):** no bail-out; the tick and readiness call `device.clock (0)` directly; constants live in the one function that uses them.
- **S (SSOT):** one event channel to the clock (`device.requestProcess()` / `device.clock (0)`) replaces two `std::function` channels; one writer path in every host.
- **S (Stateless):** no new flag; the device's own atomic `devicePlaying` gates the tick.
- **E (Encapsulation):** no plugin code knows its host — the `StandalonePluginHolder` reach-in is deleted; JUCE's self-callback pattern (`StandalonePluginHolder : private AudioIODeviceCallback`) is used one level down.
- **D:** the same wiring in every binary; the clock's stop meets JUCE's flush contract on both platforms (Step 2).

## Risks / Open Questions
- **Windows:** Step 2 and the deleted Windows `isRunning` (earlier this session) are not compiled on this machine. A Windows build verifies them.
- **Standalone audio device:** with the swap gone, JUCE's standalone keeps its real audio device open (holder `init`, `juce_StandaloneFilterWindow.h:119-127`). `EVEProcessor::processBlock` then runs per hardware block for audio only. The "after" live numbers include that load.
- **Profile skill pointer:** `~/.carol/skills/profile/SKILL.md:260` cites `EVEProcessor.cpp:37` for `deviceClock`; after Step 3 the zone is in `EVETerminalProcessor.cpp`. The skill is outside this plan's scope (MACHINIST).
- **`clear()` restart:** `device.start (this)` runs `audioDeviceAboutToStart`, so a clear now also prepares the chain (JUCE: *"called … after the device has been restarted"*, `juce_AudioIODevice.h:120-125`). On an unchanged grid the prepare keeps the frame (jam Sprint 176, `SPRINT-LOG.md:490`) and sends a same-size `SIGWINCH`, which the shell does not redraw on (`SPRINT-LOG.md:515`).
- **History precondition:** the current `benchmark/*-report.md` and `*-samples.md` are the "before" state; ARCHITECT commits them before Step 5 so git holds them.
