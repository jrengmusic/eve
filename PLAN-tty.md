# PLAN: EVE terminal on the host-plugin contract — TTY on the audio thread, TerminalGraphics as Video, TERMINAL mode

**RFC:** `eve/RFC-EVE-terminal-grand-scheme.md` (§0, §3.6 "The PTY era", §3.7, §3.9, §8). Not gospel; this session's rulings supersede where they differ.
**Date:** 2026-10-24
**Status:** approved by ARCHITECT (plan mode, 2026-10-24). The last step is the log.
**BLESSED Compliance:** verified against MANIFESTO, CODING, NAMES (read this session)
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM. LANGUAGE.md C++/JUCE L: one object, one TU (`LANGUAGE.md:25-38`).

## Context

ARCHITECT: *"can we start implementing the working terminal with TTY? at least rendered and working first. then the next actual step is … A NON DESTRUCTIBLE HISTORY SCROLLBACK … then after that we must measure, compare … 100M seq test."* This sprint is the first part only: a working, rendered terminal. Reflow and the benchmark are later sprints.

The design rule is the host-plugin contract (*"the only logical path to build END-EVE as terminal … is as HOST-PLUGIN contract … you follow the audio plugin discipline? it WILL WORK"*). Golden reference: jreng-filter-strip (JFS).

## Rulings this session (ARCHITECT, verbatim)

- R1 *"why cant we use AUDIO thread as the READER thread?"* + *"TETRIS principles … are for the lowest denominator PROCESSORS … is juce::AudioProcessor have I/O at audio thread, of course it does. that's the highest hierarchy orchestrator"* → the PTY is read and written in `processBlock`, at the orchestrator; no reader thread.
- R2 *"bytes is bytes. as sample, as terminal strings … we have never need to manage THREAD on hot path … Input: READER, Audio Thread, TTY. Output: Terminal LLGC. also Audio thread … Renderer is Vulkan … message thread"*.
- R3 *"i thought TerminalGraphics is the substitute of Video"* → no new decoder object; TerminalGraphics does Video's job.
- R4 *"Terminal "parameters" could be NON-AUTOMATABLE parameter, still valid APVTS state. so no excuse with lock and concurrency"*; *"BYPASS is automatable. it's always EXIST. same as EVE should have MASTER BYPASS, exactly like JFS"*.
- R5 *"just like Audio plugin, must be FULLY EVENT DRIVEN. no manual handroll state machine update, no manual stray variables"*.
- R6 TERMINAL mode: *"more deterministic just add TERMINAL mode"*; *"` is bind key to enter TERMINAL modal mode. ` + ], TERMINAL modal mode is TextEditor NORMAL"*; NORMAL → TERMINAL: escape (*"just ESC?"* — confirmed by the read).
- R7 *"return to terminal, history should not be editable. read only, copy only"* (insert keys return to TERMINAL).
- R8 Harness: *"drop"*.
- R9 *"mouse this sprint"*.
- R10 *"just add API to TextEditor to set its mode, why overthink"*.
- R11 Single-key editor actions move onto `jam::CommandManager`: **"This sprint"**.
- R12 KANJUT: *"only sync after ARCHITECT test runtime. post endpoint /log"*.
- R13 Build: *"only build after plan completed to validate the execution, and post audit pre /log … always build with cast. max 10m, more than that kill … always use protocol tools to debug, debug::Log to print, whatdbg"*.
- R14 Contract: *"0 alloc, 0 copy, 0 mutations … 0 temp containers, 0 new struct, 0 foreign semantics, 0 new pattern … 0 handroll"*.

## Scope (artifacts)

- **jam_terminal:** `tty/` (TerminalTTY, posix, windows), `processor/jam_TerminalFIFO`, `processor/jam_TerminalProcessor`, `graphics/jam_TerminalGraphicsEngine`, `graphics/jam_TerminalGraphicsContext`, `document/jam_AnsiDocument` (lexers, command tables), `jam_terminal.h/.cpp`.
- **jam_gui:** `widgets/jam_TextEditor`, `widgets/jam_TextEditorStatusBar` (mode label only), `commands/jam_CommandManager` (if the move needs it).
- **jam_plugin_bootstrap:** `parameter/jam_ParameterLayout` (`automatable` column).
- **jam cast:** `cast/gui.md` (EditorMode row, CommandId rows), `cast/terminal.md` (AnsiTokenType row), `generated/` by cast only.
- **eve:** `Source/EVEProcessor.*`, `EVETerminalProcessor.*`, `EVEView*.cpp`, `layout/parameters.md`, `layout/component.md`, `layout/ViewLayout.md`, `cast/files.md`, `cast/identifiers.md`, `markdown/` (deleted), `SPEC.md`, `RFC`, `CLAUDE.md`.
- **Out (ratified by plan approval):** reflow of the live screen and SIGWINCH non-destruction (next sprint); 100M benchmark (sprint after); KANJUT sync (R12); inline images from the PTY (OSC 1337 input), Sixel/Kitty; paste; HTS/TBC tab-stop editing (default stops only); DECRQM/DECRQSS/XTWINOPS replies beyond DA and CPR; progressive keyboard protocol (CSI u); SPEC Feature 1 exit/error message text.

## Dependency & API Inventory (reads this session)

- **Clock / I/O (R1, R2):** Standalone drives `processBlock` from the device callback (`juce_StandaloneFilterWindow.h:556-575`, `:598`); host drives it when hosted. POSIX master is `O_NONBLOCK` (`jam_TerminalTTY_posix.cpp:204-208`); `read` returns 0 on `EAGAIN`, -1 at EOF (`:260-280`). Windows `read` issues overlapped `ReadFile`, zero-timeout check (`jam_TerminalTTY_windows.cpp:1299-1387`); `waitForData` honours its timeout (`:1496-1497`) and sees child exit (`:1499-1506`).
- **Blocking to remove:** POSIX `write` sleeps `usleep (100)` on `EAGAIN` (`posix.cpp:287-303`); Windows `write` takes `writeLock` and waits `GetOverlappedResult (…, TRUE)` (`windows.cpp:1391-1415`, member `:412`). `TerminalTTY : public juce::Thread` (`jam_TerminalTTY.h:59-63`), `run()` (`jam_TerminalTTY.cpp:28-63`). No caller outside `jam_terminal/` (grep this session).
- **Crossing primitive:** `juce::AbstractFifo` — no lock, no allocation (`juce_AbstractFifo.cpp:67-140`). `TerminalFIFO` = `juce::OutputStream` over one ring (`jam_TerminalFIFO.h:14`, `:71-72`); `pull (AnsiDocument&)`, `pull()` (`:60`, `:66`).
- **Parser (ruling 51):** `AnsiDocument` static lexers over `Document::Cursor` (`jam_AnsiDocument.cpp:40-142`) allocate only for the unterminated mark (`token.add<bool> (Id::unterminated …)`, `:81`, `:101`, `:139`). Command tables are `Function::Map`s (`csiConstruction`, `:734-756`; `treeConstruction`, `:148-158`). Vocabulary is cast data: `map::CSI` (`generated/jam_Terminal.h:917-1002`), `map::ESC` (`:1170`), `map::DEC` (`:138-195`), `map::ANSI` (`:1304`), `map::SGR` (`:240`), `map::MouseTracking` (`:112-133`), `map::Screen` (`:87`).
- **Video substitute (R3):** `TerminalGraphicsContext` is a `juce::LowLevelGraphicsContext` over the engine rows (`jam_TerminalGraphicsContext.h:61-71`): `drawCodepoints` (`cpp:656-689`), `fillRect` (`:98-113`), `writeVisibleCell` (`:301-324`), `saveState`/`restoreState` (`:72-85`), `getTargetRow` (`:338-344`). Today it takes a whole frame and blanks it (`:7-16`). `TerminalGraphicsEngine` row ring + `process` (`jam_TerminalGraphicsEngine.h:29-176`, `cpp:538-565`); `write()` takes `heightRows` slots (`cpp:382-387`).
- **Allocation points to size at prepare:** context members (`cpp:13`, `:15`); row colour table `colourIds.addIfNotAlreadyThere` (`jam_AttributedString.cpp:243`, bound `colourIdCapacity` `:247`); clusters `clusters.add` (`:121`); true colours (`jam_TerminalGraphicsEngine.cpp:351-354`).
- **Model / events (R4, R5):** APVTS listener → `parameterChanged` → `Function::Map parameters` (`jam_AudioModel.cpp:487-496`; JFS `FilterStripAudioProcessor.cpp:155-159`). Processor-originated write = `setValueNotifyingHost` (`juce_AudioProcessorParameter.cpp:59-63`); `AudioParameterBool/Int::setValue` notify nobody (`juce_AudioParameterBool.cpp:79`, `:84`). View reacts through `PluginEditor::valueTreePropertyChanged` → `registry->callbacks` keyed `Id::toType (id)` (`jam_PluginEditor.cpp:316-356`, keys `:198-225`). `ParameterLayout` builds bool/choice/float rows (`jam_ParameterLayout.cpp:77-154`); attribute builders set no `withAutomatable` (`:39-65`); JUCE default automatable = true (`juce_AudioProcessorParameterWithID.h:119`).
- **Endless event shape (codebase precedent):** VT state changes fire `events.get (id::…)` and flush once per process pass (`endless Video.cpp:99-116`); handlers write the model (`ProcessorEvents.cpp:378-392`).
- **Bypass (JFS):** `parameters.md:10` row; `getBypassParameter` (`FilterStripProcessor.cpp:223-226`); `processBlockBypassed` (`:162-191`); setter (`FilterStripAudioProcessorRegistration.cpp:30-43`); toggle row (`component.md:34`); editor callback (`jam_PluginEditor.cpp:206-210`, `:284-287`).
- **View → chain:** `chainEvents` (`jam_ViewManager.h:45`), bound by `Registry::bindEvent` from the `event` column (`jam_ViewManager.cpp:243-244`); JFS `registration.events` (`FilterStripViewRegistration.cpp:365-402`). `bindToProcessor` by trait (`jam_Registry.h:153`, `:334-358`).
- **Keys:** `jam::CommandManager` (`jam_CommandManager.h:7-28`, `.cpp:5-18`) + `PluginEditor` target (`jam_PluginEditor.cpp:76-100`, `:229-234`). `KeyPressMappingSet` = one key per invocation, many keys per command (`juce_KeyPressMappingSet.h:229-236`, `.cpp:322-343`). TextEditor today: `map::EditorAction` rows + own match (`jam_TextEditor.cpp:808-967`); 7 `insert` comparisons (`:846`, `:873`, `:874`, `:988`, `:1152`, `:1451`, `:1535`). Encoders: `TerminalKeyboard::map (key, applicationCursor, keyboardFlags)` (`jam_TerminalKeyboard.h:102`), `TerminalMouse::map (…)` (`jam_TerminalMouse.h:179-185`).

## Design

1. **One clock, no thread (R1, R2).** `EVETerminalProcessor::process()` (audio thread) per block: (a) applies the pending winsize (the `pendingOversamplingFactor` shape, `FilterStripAudioProcessor.cpp:103-128`); (b) writes queued input bytes to the PTY; (c) reads up to one prepared block of PTY bytes; (d) decodes them; (e) `TerminalProcessor::process` writes history and live bytes. Open/close/`setWinsize` (TIOCSWINSZ) stay on the message thread. `TerminalTTY` loses `juce::Thread`, `run()`, the reader events, `waitForData`; `read` is the non-blocking poll on both backends (Windows harvests a pending overlapped read inside `read`); `write` makes one non-blocking attempt and returns the bytes accepted.
2. **Lanes (all `AbstractFifo`, one writer, one reader).** Input to PTY: View (message thread) → `TerminalFIFO` → `processBlock` → `TerminalTTY::write` (bytes not accepted stay in the ring). Device replies (DA, CPR) do not enter that lane, because the lane already has its one writer (the View). `processBlock` writes them straight to `TerminalTTY::write`, after the queued input, on the same thread. History and live: unchanged (`TerminalProcessor`).
3. **Decode (ruling 51, RFC:385-387).** `processBlock` lexes the read block with the `AnsiDocument` static lexers over a `Document::Cursor` on the prepared input block. An incomplete trailing sequence is marked by token type (`map::AnsiTokenType` row), not by an added property, so lexing allocates nothing; its bytes move to the head of the prepared block for the next read. Commands dispatch through `Function::Map` tables keyed by the cast vocabulary (`map::CSI`, `map::ESC`, C0, `map::DEC`, `map::ANSI`, `map::SGR`); numeric parameters parse into a prepared fixed array (no `juce::StringArray`). The message-side `AnsiDocument` build keeps its behaviour.
4. **TerminalGraphics is Video (R3).** VT command → LLGC call on a `TerminalGraphicsContext` that lives across blocks (chain member, cold resource, built at `prepare`):

   | VT | TerminalGraphics |
   |---|---|
   | print | `drawCodepoints` at the cursor cell |
   | SGR | pen (atom template + colour ids) |
   | CUP/CUU/CUD/CUF/CUB/CHA/VPA/CR/BS/HT | cursor cell (context state) |
   | DECSC/DECRC | `saveState` / `restoreState` |
   | DECSTBM | clip of the scroll region |
   | ED / EL / ECH | `fillRect` of the region with the pen background |
   | LF/IND at the region bottom, SU | engine `addRow` (retire top row to history) in a full-screen region; in-region row move otherwise |
   | DECSET 1049 / 47 / 1047 | engine screen switch (`map::Screen`): alternate grid, no retirement |

   The context's frame constructor (TUI / cast) stays; the chain uses the live-area target (no blank, no new frame per block). Every growable table is sized at `prepare` to its bound.
5. **Engine (TerminalGraphics).** The ring is the scroll stream: the newest `liveRows` ready slots are the live screen, edited in place (writer and reader are both the audio thread). `process` writes rows above the live area to history, and the live area to the live lane when a block decoded bytes or the row count changed. Alternate screen: one grid sized at `prepare`, never retired.
6. **State is APVTS (R4, R5).** Terminal modes the View consumes are non-automatable rows in `parameters.md`: application cursor, bracketed paste, mouse tracking (`map::MouseTracking` choice), mouse SGR, cursor visible, alternate screen, cursor line, cursor column. `ParameterLayout` reads a new `automatable` column (empty = JUCE default true, so JFS is unchanged) and calls `withAutomatable`. During a block the context state is the calculation working copy (MANIFESTO S: transient); at block end the chain sets each changed mode through its parameter (`setValueNotifyingHost`), the endless flush shape. `setStateInformation` restores them; `parameterChanged` → `Function::Map parameters` applies them to the working copy (JFS shape). The View reacts through `registry->callbacks` keyed `Id::toType (id)`.
7. **MASTER BYPASS (R4).** JFS lines copied: `parameters.md` `bypass` row, `getBypassParameter`, `processBlockBypassed`, `parameters` entry storing `isBypassed` (no `chainFade`: no audio to fade), `process` skipped while bypassed, toggle row.
8. **TextEditor (R6, R7, R9, R10, R11).**
   - `map::EditorMode` gains `terminal`; `insert` stays the default row. Public `setMode (int mode)`.
   - The 7 `insert` comparisons become per-mode lookups (MANIFESTO L).
   - TERMINAL: no status bar, no edits. `` `] `` (chord) → NORMAL. Every other key goes to the `chainEvents` hook; EVE encodes with `TerminalKeyboard::map` (application cursor from APVTS) into the input lane.
   - NORMAL/VISUAL in EVE: read-only, copy-only. EVE's sheets drop `d c x p P u ctrl+r` rows; escape and `i a I A o O` set TERMINAL.
   - Single-key actions run as `jam::CommandManager` commands (TextEditor becomes the `ApplicationCommandTarget`; one key set per mode via `setKeyPresses` on mode change). Chords, counts and operator-pending stay in the editor. `map::EditorAction` keeps only what CommandManager cannot hold.
   - Mouse: `mouseDown/Drag/Up/Wheel`. The hook returns whether the chain consumed the event (`TerminalMouse::map` when tracking is on). Otherwise a drag sets VISUAL from the anchor cell and `y` yanks.
   - Live caret: from the APVTS cursor line/column in TERMINAL mode.
9. **Harness dropped (R8).** `getHarness`, the `load` config, `files::harness`, `markdown/` are deleted; `TextEditor::load` loses its only caller and is deleted (MANIFESTO L, YAGNI). EVE starts the shell from `$SHELL` (SPEC.md:67).

## New Names (ratified by plan approval — NAMES Rule -1)

- `TextEditor::setMode` (R10). `map::EditorMode` row `terminal` (R6). CommandId row `terminal` (the command that sets TERMINAL).
- `map::AnsiTokenType` row `unterminated` (`Id::unterminated` exists).
- Engine: `addRow()` (verb `add`), `getRow (jam::Cell)` overload for the live area (`getRow` family), `setScreen (int)` (`map::Screen`).
- `TerminalFIFO::pull (TerminalTTY&)` overload (`pull` family); `TerminalTTY::write` returns `int`.
- TextEditor hooks: `onKeyPress`, `onMouse` (`on*` family, `jam_MouseEvents.h:37-67`).
- `parameters.md` column `automatable` (JUCE `withAutomatable`); parameter ids `applicationCursor`, `bracketedPaste`, `mouseTracking`, `mouseSgr`, `cursorVisible`, `alternateScreen` (cast `map::DEC` names), `cursorLine`, `cursorColumn` (TextEditor node family).
- EVE `component.md` event value for the text editor row and its `chainEvents` key: `TERMINAL_INPUT` / `Id::terminalInput`.

## Validation Gate

COUNSELOR validates each step before the next by reading the changed files against MANIFESTO, NAMES, CODING and this PLAN. No build per step (R13). One build with cast after the last code step, watchdogged at 10 minutes; one build after the audit, before `/log`. A build failure is diagnosed with `debug::Log` and whatdbg only. @Auditor runs once, after the last code step; findings filtered against a CONTRACT clause by a read.

## Steps

0. Plan of record: this file.
1. **Delete first (R8, Refactor-Rewrite).** Harness: `EVEViewComponents.cpp:3-6`, `:30-35`; `cast/files.md:29`; `markdown/`; `TextEditor::load`. TTY thread: `juce::Thread` base, `run()`, `jam_TerminalTTY.cpp`, reader events, `waitForData`, `instantKillJoinTimeoutMs`. Windows `writeLock`. The compiler lists what remains.
2. **TTY as a dumb device.** Non-blocking `read` (both backends, Windows harvest + exit inside `read`), single-attempt `write` returning bytes accepted. `jam_terminal.h` include order: tty before processor.
3. **Input lane.** `TerminalFIFO::pull (TerminalTTY&)`: writes ready bytes, consumes only what was accepted.
4. **Lexers allocate nothing.** `map::AnsiTokenType::unterminated` (cast `terminal.md`); lexers set the type; message-side `build` reads the type.
5. **Engine scroll stream.** `addRow`, live-area `getRow`, `setScreen` + alternate grid; `process` live-write rule; table capacities at `prepare`.
6. **Context as Video.** Live-area target, state across blocks, members sized at `prepare`; VT → LLGC table (Design 4).
7. **Command tables (audio thread).** C0, ESC, CSI, DEC/ANSI modes, SGR over fixed parameter arrays; DA and CPR replies to the PTY.
8. **ParameterLayout `automatable` column.**
9. **EVE chain.** `EVETerminalProcessor`: TTY, input lane, prepared input block, context, engine; `process()` per Design 1; mode flush at block end; `parameters` map; bypass. `EVEProcessor`: `getBypassParameter`, `processBlockBypassed`, TTY open at construction / close at destruction (message thread), `setWinsize` → TTY. `parameters.md` rows.
10. **TextEditor modes and keys.** `terminal` row, `setMode`, per-mode lookups, CommandManager move, hooks, read-only sheets in EVE `ViewLayout.md`.
11. **Mouse.** Handlers, forward-or-select via hook, drag → VISUAL, yank.
12. **EVE View wiring.** `registration.events` (`bindEvent` for the text editor), callbacks for the mode parameters (caret, key encoding inputs), start in TERMINAL via `setMode`.
13. **Build (cast, 10-minute watchdog).** EVE `--debug`. Fix from compiler output; runtime asserts via whatdbg.
14. **Audit** (once), resolve filtered findings; doxygen zero warnings; build again; SPEC/RFC/CLAUDE.md amendments.
15. **Log** on ARCHITECT's runtime check (`/log`); commit messages in chat. KANJUT sync after (R12).

## Verification (ARCHITECT, runtime)

- Launch EVE Standalone: `$SHELL` prompt renders; typing echoes; `ls`, `vim`, `htop`, `less` render and redraw; colours (SGR 256 and truecolour).
- `` `] `` → NORMAL (status bar shows), `hjkl` over history, `v`/drag selects, `y` yanks to clipboard; escape / `i` → TERMINAL; no edit key changes history.
- Mouse in `vim` with `:set mouse=a` forwards; outside tracking, drag selects.
- MASTER BYPASS toggle freezes the screen; un-bypass resumes.
- Resize: shell receives SIGWINCH (`tput cols` follows).
- Debug build: no assert in a whatdbg trace launch.

## BLESSED Alignment

- **B:** the chain owns TTY, lanes, context, engine as cold resources sized at `prepare`; the host owns the thread.
- **L:** existing families only (`add*`, `get*`, `pull`, `on*`, Function::Map tables, cast rows).
- **E:** View tells (chainEvents) and pulls (getters); chain never touches the View; lanes are `AbstractFifo`.
- **S (SSOT):** cast vocabulary drives every dispatch; APVTS holds terminal modes; one key system (CommandManager) for single keys.
- **S (Stateless):** context state is a calculation working copy, flushed to APVTS per block.
- **D:** same bytes → same rows → same wire bytes.

## Risks / Open Questions

None open: the names above and the Out list are ratified with the plan.

## Execution State

- Step 0: done.
- Step 1 (harness): done. `EVEViewComponents.cpp` holds no harness (`:1-40`); `cast/files.md` harness row, `markdown/`, `TextEditor::load` deleted. `Source/generated/Files.*` regenerate at the build step.
- Steps 1–3 (TTY): done. `TerminalTTY` has no `juce::Thread` base (`jam_TerminalTTY.h:59-62`); `write` returns `int` (`:136`). POSIX `write` = one non-blocking attempt (`posix.cpp:277-292`). Windows `read` harvests with a zero timeout and detects child exit (`windows.cpp:1306-1339`); Windows `write` has no lock and never waits, `writePending` beside `readPending` (`:1388-1429`). `TerminalFIFO::pull (TerminalTTY&)` (`jam_TerminalFIFO.cpp:86-104`). Include order: tty before processor.
  - Correction (CODING, one object one TU): `jam_TerminalTTY.cpp` stays and holds `rememberDimensions`; only `run()` left it.
- Step 4: done. `map::AnsiTokenType::unterminated = 4` (`cast/terminal.md:61-69`, `generated/jam_Terminal.h:64`, `:73`; cast fixpoint confirmed). Lexers return the type (`jam_AnsiDocument.cpp:78`, `:93`, `:126`); `build` tests it (`:145`).
- Corrections from reads during Steps 5–7 (execution under the lock):
  - Scroll verbs take the cast vocabulary names `scrollUp` / `scrollDown` (`map::CSI`, `generated/jam_Terminal.h:975-976`; NAMES Rule 5) in place of the placeholder `addRow`.
  - DECSC/DECRC do not map to `saveState`/`restoreState`: the context `State` holds the clip list (`jam_TerminalGraphicsContext.h:369-375`), so a restore would also restore the scroll region; the endless saved cursor holds cursor, pen, wrap, origin and charset only (`endless Video.h:491-501`).
  - `applyToVisibleCells` built a `juce::RectangleList` per draw (`jam_TerminalGraphicsContext.h:481`): replaced by per-clip-rectangle intersection.
  - The context publishes its frame in the constructor and writes through the engine frame rows; `cast` `printHelp` output is unchanged (`cast/Source/Help.cpp:23-31`).
  - Per-row tables are sized at `prepare` with the new `AttributedString::allocate` (`jam::Array::ensureStorageAllocated`, `jam_Array.h:184-190`; `clear` keeps capacity, `:569-580`). Name: the exact action (NAMES Rule 6 bans `ensure`).
  - The true-colour id is a pure function (`jam_TerminalTrueColours.cpp:11`, `:21-28`): new static `TerminalTrueColours::getColourId`; SGR rows take `std::string_view` parameter fields and touch no store; the message-side document registers true colours in `addTextRun`.
  - Dead branches removed: the `unterminated` checks in `applyCsiSequence`/`applyOscSequence` (the token is removed before dispatch, `jam_AnsiDocument.cpp:145-149`) and `addUnterminatedMark`. `AnsiValidator`'s root rule now reads a mark that nothing sets (it was unreachable before this sprint too); recorded for the audit.
- Steps 5–6: done, validated. Engine `getRow (jam::Cell)` / private `getSlot (jam::Cell)` (`jam_TerminalGraphicsEngine.cpp:397-421`), `setScreen` + `alternate` grid (`:453-462`), `scrollUp`/`scrollDown` (`:464-498`) over private `swap (jam::Cell, jam::Cell)` (JUCE `juce::Array::swap (int, int)` family, `juce_Array.h:1004`) and `reset (jam::Cell)` (`:500-520`; DRY correction of a first draft that repeated the swap-and-clear block). `process` writes the live area whenever `newLiveRows > 0`; the `liveRows` member is gone (`:636-674`). `AttributedString::allocate` (`jam_AttributedString.cpp:90-95`). Context publishes its frame in the constructor and keeps no write handle (`jam_TerminalGraphicsContext.cpp:7-19`); `applyToVisibleCells` intersects each clip rectangle (`.h:495-504`). Row `std::swap` moves pointers only (`AttributedString` defaulted move, `jam_AttributedString.h:41-42`; `jam::Array` move, `jam_Array.h:75-101`).
- Step 7a: done, validated. SGR rows take `juce::Span<const std::string_view>`; one split template `getFields<int>` serves CSI parameters and colon sub-parameters (`jam_AnsiDocument.h:173-188`); `TerminalTrueColours::getColourId` is pure (`jam_TerminalTrueColours.cpp:8-12`). Field semantics equal the former `juce::StringArray::addTokens` (`juce_StringArray.cpp:346-368`: empty fields kept, empty text gives none).
- Correction (read, Step 7b): the VT path writes the engine rows through the `AttributedString` API, not through LLGC calls. Reason: the LLGC contract is a compositing painter — `fillCells` never replaces a character (`jam_TerminalGraphicsContext.h:571`), `drawCodepoints` inherits the destination background (`:550-555`) and registers only the foreground id (`.cpp:672`); a VT print replaces the cell and a VT erase blanks it. `AttributedString::set (int, juce::Span<const char32_t>, AttributedChar)` (`jam_AttributedString.h:150`) is the print primitive. TerminalGraphics (the engine) does Video's grid job (R3). Design 4's LLGC table is superseded by this line.
- Step 7b-1: done, validated. Lexer: ESC intermediates (`jam_AnsiDocument.cpp:72-91`; terminfo `xterm-256color`, the `TERM` of `jam_TerminalTTY_posix.cpp:95`: `sgr0=\E(B\E[m`, `smacs=\E(0`); public static `getToken (Cursor)` (`:40-49`). `TerminalFIFO::getNumReady` / `getFreeSpace` (`jam_TerminalFIFO.cpp:19-27`, `juce::AbstractFifo` family). Cast: mode Ids in jam_terminal; `cursorLine`/`cursorColumn` moved from `cast/gui.md` to `cast/terminal.md` (jam_terminal sits below jam_gui; narrowest scope that holds every use); `map::EraseInDisplay` rows `eraseBelow`/`eraseAbove`/`eraseSavedLines`; new `map::EraseInLine` (registered in `cast/spell.md:1748-1759`, the jam cast manifest). Cast fixpoint confirmed.
- Step 7b design facts (reads): terminfo `u8=\E[?1;2c` (DA reply), `u6=\E[%i%d;%dR` (CPR), `it#8`, `smir`/`rmir` (IRM), no `rep` (REP out, YAGNI). Backpressure: the decoder reads the PTY only when the live lane is empty, and stops a block when the history lane lacks one screen of headroom; the PTY holds the rest. A trailing incomplete UTF-8 sequence is carried with `juce::CharPointer_UTF8::operator--` + `isValidString` over a zeroed `reservedSlot` byte before the block (`juce_CharPointer_UTF8.h:100-108`, `:438-486`). Decoder tables are members built in the constructor (message thread), so the audio thread never allocates them.
- Step 8: done, validated. `ParameterLayout::isAutomatable` (`jam_ParameterLayout.cpp:39-43`) feeds `withAutomatable` in the four attribute builders (`:45-75`); `Id::automatable` (`cast/plugin_bootstrap.md:35`). Violation recorded: the Engineer edited with a four-pass sed and no dry-run or backup (CAROL Destructive-Edit Discipline); corrected by a full read of the file (168 lines = 158 + 10 intended lines, no residue).
- Step 7b-2: done, validated. `jam::TerminalProcessor` decodes on the audio thread: `process (TerminalTTY&, TerminalGraphicsEngine&, const jam::Function::Map<juce::Identifier, void>& events)` (`jam_TerminalProcessor.cpp:30-52`); tables are const members built in the constructor (`:5-14`); the flush fires eight events after each decoded block (`applyEvents`). Correction in review: the history headroom scales with the token length (`:93`; a text token of `n` bytes wraps at most `n / width` times), and `prepare` asserts the capacity that a full read block needs (`:24-26`). `applyTokens` is 31 lines (Lean smell; for the audit).
- Correction (MANIFESTO D) to Design 6: `parameterChanged` applies `bypass` only. The terminal processor is the sole author of the terminal modes. A mode restored from a saved session into a new shell's decoder (for example `alternateScreen`) would change the screen that the same PTY bytes produce.
- Correction (CODING "configure only the difference") to Design 7: no `processBlockBypassed` override. JFS overrides it only to report latency (`FilterStripProcessor.cpp:162-165`); EVE has no latency.
- Step 9 design: the chain owns the TTY (`createTTY`, platform `#if`, endless precedent `Processor.cpp:383-387`), the input lane (`chainEvents[Id::terminalInput]`, View writes, audio thread pulls), the decoder events map (`events`, handlers call `setValueNotifyingHost` on the mode parameters through a tag String built at registration), and opens the PTY in `prepare` at the first valid winsize (`$SHELL`, fallback per `SPEC.md:67`). Resize: `EVEProcessor::setWinsize` runs `suspendProcessing` + `prepare`.
- Step 9: done, validated. `EVETerminalProcessor.cpp:10-131`, `EVETerminalProcessor.h:17-83`, `EVEProcessor.cpp:169-180`, `parameters.md:6-28` (`automatable` column, `bypass` + eight terminal rows), `Id::terminalInput` (EVE cast, fixpoint).
- Step 10 split: 10b (CommandManager move) before 10a (TERMINAL mode). Facts behind 10b: JUCE rejects command id 0 (`juce_ApplicationCommandManager.cpp:61`), and `map::EditorAction` keys start at 0, so every editor action becomes a `map::CommandId` row (`0x4600005`…, plus `terminal` `0x4600026`) and `map::EditorAction` is deleted ("EditorAction shrinks to what JUCE cannot hold"). `KeyPress::createFromDescription` drops case and text (`juce_KeyPress.cpp:231-235`, `operator==` `:52-63`), and a KeyListener runs before `keyPressed`. So a description of at most `CommandManager::chordLength` (2) characters stays on the editor's text path and dispatches through `invokeDirectly`; longer descriptions are key presses that the editor's own `jam::CommandManager` holds, set per mode.
- Step 10b: done, validated. `map::CommandId` holds every editor action (`cast/gui.md`, `0x4600005`–`0x4600026`); `map::EditorAction` deleted. `jam::CommandManager::chordLength` (2) is the single limit: `setKeyPresses` holds descriptions longer than it; the editor's text path matches the rest and dispatches with `invokeDirectly` (`jam_TextEditor.cpp` `keyPressed`, `perform`, `applyOperand`). `PluginEditor::perform` runs only `settings` or a zoom key. Include order fixed in the manifest (`user-modules-info.md:1013-1014`: CommandManager before TextEditor). Violation recorded: the Engineer replaced `map::EditorAction::` with a sed (backups kept, no dry-run); corrected by my read of every changed function.
- Correction (MANIFESTO E, complete at creation): the editor's `state` is a component node with the default mode from the constructor (`jam_TextEditor.cpp:50-51`), because `lookAndFeelChanged` runs before `bindToModel` and `map::EditorMode::get` throws on an empty mode (`jam_Bimap.h:160`).
- Step 10a + 11: done, validated. `map::EditorMode::terminal` (3), `Id::terminal` moved from EVE's cast to jam's. `setMode`, hooks `onKeyPress` / `onMouse`, TERMINAL forwarding in `keyPressed` (a failed chord prefix is forwarded first), counts only in normal and visual, status bar in normal and visual only, history caret hidden in TERMINAL. Live caret: `juce::Value`s that refer to the `cursorLine` / `cursorColumn` / `cursorVisible` parameter nodes (`bindToModel`), with `juce::Value::Listener` → repaint; no shadow copy. Mouse: `Content` overrides → `applyMouse`; the live lane goes to `onMouse` in TERMINAL; the history lane sets the anchor on press and VISUAL on drag; the wheel scrolls the viewport only when `onMouse` declines.
- Step 12: done, validated. `component.md` event `terminalInput`; `EVEViewComponents.cpp:26-73` binds `onKeyPress` (`TerminalKeyboard::map` with the `applicationCursor` value; returns whether bytes went out, so unencoded keys reach the PluginEditor commands), `onMouse` (`TerminalMouse::map` with `mouseTracking` / `mouseSgr`), and starts in TERMINAL. `ViewLayout.md` version `0.0.3` (an older user copy is replaced, `jam_PluginEditorLayout.cpp:27-33`), read-only NORMAL/VISUAL sheets, `terminal` rows for escape and `i a I A o O`, TERMINAL sheet `leaveMode` `` `] ``. Violation recorded: the Engineer rewrote `ViewLayout.md` with a Python script and no backup (CAROL Destructive-Edit Discipline); corrected by a full read (no residue outside the three sheets and the version).
- Step 13: done. First build `EXIT=0`, 0 errors. whatdbg trace launch found `JUCE Assertion failure in jam_TextEditor.cpp:1597` (`setCursor` needs one history line; reached from `bindToModel` → `valueTreeRedirected`). Cause: the harness load used to supply the line (R8 dropped it). Fix at the owner (MANIFESTO D): the editor's history document starts with one empty line, indexed at the first style application (`jam_TextEditor.cpp:43-45`). Same read: nothing kept the live lane in view as history grew; in TERMINAL mode `setContentSize` pins the view to the bottom (`:1503-1509`). whatdbg does not stop on the real-time audio thread (the `processBlock` breakpoint never stopped while a probe printed), so `debug::Log` probes traced the audio path: prepare → TTY open → `processBlock` → chain → decode (149 + 403 bytes of prompt). Probes found a second prepare from the View's `setWinsize` with the same grid (pixel part only), which wiped the prompt with no SIGWINCH; fix: the chain re-prepares only on a cell-grid change, holding the processor for `suspendProcessing` (JFS `FilterStripAudioProcessor.h:317`). Re-run: no assertion, one prepare per grid, prompt decoded. Probes removed.
- Step 14 audit: NEEDS_WORK, 66 findings. COUNSELOR read each cited line (Design by Contract Filter).
  - Resolved in six file-disjoint Engineer batches, each validated by a read:
    - Windows TTY: `write` returns `int` (#1). `write` copies into the owned `writeBuffer` and returns this call's count; 0 while a write is pending (#2). `consumeReadBuffer` is a member that advances the offset (#3). `read` is one issue/harvest/consume path (#4, 63 lines). Constructor body in the .cpp; `conptyDir` held by value, because the caller passes a temporary (#5).
    - TTY base/POSIX: bodies moved to `jam_TerminalTTY.cpp` (#6). The grid-change guard is in the EVE chain only; `rememberDimensions`/`lastResize*` deleted (#7). `getEnvValue` serves both platforms (#9).
    - Decoder: `prepare (historyRows, engine)` computes both lane capacities at the owner (#56). `hasHeadroom` also bounds the reply stream with `maxReplyLength`, so no reply drops (#19). `getCompleteLength` extracted; `applyTokens` is 24 lines (#15). Structured bindings `[fields, numFields]` (#11). `intermediateOffset`, `sizeof (marker)` (#18). `getNextTabStop`, `getScreenRow`, ED rows reuse EL rows, single `value` (#16). ED 3 has no row (#17, xterm ED 3 leaves the screen). `alternateScreen` and `setScreen` deleted; the engine owns the screen (`getScreen`) (#34). `TerminalProcessor::reset`/`TerminalFIFO::reset` deleted (#60). `AnsiDocument::Parameters` alias (#21). `ansiVocabulary` built at static initialisation (#30). Dead `Id::unterminated` rule, cast rows `@id unterminated` and `failUnterminated` deleted; cast fixpoint (#29).
    - AttributedString/engine/context: `allocate` → `prepare` (#28, NAMES Rule 5, `TerminalFIFO::prepare`). The span `set` writes one cluster, no second segmentation (#26, #27). `removeCluster` keeps the cluster table dense (#24). `setScreen` idempotent, allocation-free assert (#37). `appendRowEnd` (#36). Clip regions merge with `juce::RectangleList::add` (#38).
    - TextEditor/commands: `getMode`/`setMode` everywhere (#41). `getTypingModes` per-mode lookup replaces the insert comparisons (#40, Design 8). The view follows the tail only when it is at the bottom; `setContentSize` no longer scrolls (#42). Redundant condition (#44) and duplicate assert (#45) removed. Live-caret Values are bound by EVE, not by the widget (#46). Binding names (#50). `CommandManager::applyCommandInfo`; PluginEditor `getActions`/`applyZoom` register and perform only its own commands (#52, #53).
    - EVE: input lane writes only within free space (#33). No per-block `isRunning` (#55). `getWinsize` reads the model; `pendingWinsize` deleted; `prepare` re-prepares only on a grid change against `engine.getBounds()`, so `prepareToPlay` no longer wipes the screen (#57, #62). `open` result asserted (#61). `reset` deleted (#60). Parameter names `component`, `chainEvents` (#66).
  - Rejected with the read that refutes them:
    - #8, #10, #48: no CONTRACT clause covers them. #10: the caller clamps the value. #48: `find_if` is a search, not a branch chain.
    - #12: PLAN Design 6, the working copy is flushed to APVTS.
    - #13: R14 "0 new struct". A whole-Pen copy would copy its `AttributedString` and arrays (`jam_AnsiDocument.h:117-123`).
    - #14: R3, "no new decoder object".
    - #20: an empty remainder completes no token.
    - #22: `jam::Array::add` reallocates before it constructs from a reference (`jam_Array.h:386-391`), so the copy is required.
    - #23: established `add*Rows` family.
    - #35: values computed in place, not data in transit.
    - #39: the context runs in cast (`cast/Source/Help.cpp:23-31`).
    - #43: JUCE zeroes the text character when Cmd is held (`juce_NSViewComponentPeer_mac.mm:906-908`).
    - #45 (data part): the editor buffer's one line is created at construction.
    - #49: PLAN:171, selection over history.
    - #51: no sheet binds F1–F9.
    - #58, #64: JFS verbatim (`FilterStripAudioProcessor.cpp:41`).
    - #59: the change test reads the Model.
    - #60 (`ProcessSpec` part): JFS chain shape.
    - #63: the first decoded block flushes all eight modes.
  - ED 2 retirement to history (#17, SPEC:85) belongs to SPEC Feature 2 (Non-Destruction). ARCHITECT sequenced that feature as the next sprint ("then the next actual step is … A NON DESTRUCTIBLE HISTORY SCROLLBACK").
  - ARCHITECT rejected the ruling questions and pointed to MANIFESTO, CODING and NAMES. ARCHITECT's words: *"300-30-3 is GUIDE. THINK HARD, is the IMPLEMENTATION ACTUALLY ADHERE, DESIGNED BY CONTRACT? dont just lazily asking for decompostion or exceptions."* Corrections:
    - Row tables (#24, #25): MANIFESTO E, *"identity is assigned and released through the registry's own API … never re-indexes"*. The swap-and-pop `removeCluster` was a re-index, so it is deleted.
      - Clusters use `assignClusterIndex`/`releaseClusterIndex` over `freeClusterIndices`.
      - Colour ids are released at zero `colourIdUses` onto `freeColourIds`, and slots never move.
      - The pattern is `VulkanBindlessRegistry::assignBindlessIndex`/`releaseBindlessIndex` (`jam_VulkanBindlessRegistry.h:64-70`, `:174`), in `jam_AttributedString.cpp:122-147`, `:194-229`, `:283-311`.
    - POSIX `getEnvVar` (68 lines): dead code. The jam TTY introspection surface (`getEnvVar`, `getCwd`, `getProcessName`, `getForegroundPid`, `getShellPid`, `addShellEnv`, `clearShellEnv`, `shellIntegrationEnv`) had no caller in dev outside `jam_terminal/tty/` (endless keeps its own copies). It is deleted with its helpers (MANIFESTO L YAGNI; E "Dead getters are dead code"). `jam_TerminalTTY.cpp` held nothing after that; its include is gone from `jam_terminal.cpp` and the file is deleted.
    - Windows `read`/`write` (63 and 28 lines) adhere. Win32 requires an overlapped buffer to stay valid until completion (ReadFile and WriteFile `lpBuffer`: *"This buffer must remain valid for the duration of the read operation"*). A read *"may return before the read operation is complete … ERROR_IO_PENDING"*. So the TTY owns the in-flight buffer (MANIFESTO B). This is not a transit stage. `read` is one job: polling one overlapped read.
    - `keyPressed` (82), `perform` (34) and `Content::paint` (64) are mode decision tables and truth tables in imperative code (MANIFESTO L, *"Replace with direct lookup"*; Design 8). `getTypingModes` only renamed `mode == insert`, so it is a proxy, not a lookup.
    - TextEditor (1812 lines) breaks MVP role separation (MANIFESTO E, *"The View composes. The Processor holds the business logic."*). The View holds the interpreter (`keyPressed`, `perform`, `applyOperand`, `cancelOperator`, motions, actions, `shapes`) and the undoable edits (`InsertLine`, `RemoveLine`, `setLine`, `addLine`, `removeLine`, `addText`). The contract fix moves them to `TextEditorProcessor` (families `TextEditor*`, `jam_TextEditorStatusBar`, and `*Processor`, `jam::TerminalProcessor`). The per-mode key table and the simpler `perform` are written there once. New files `jam_gui/widgets/jam_TextEditorProcessor.h/.cpp` are a scope change (CAROL Scope), so the sprint stops here for ARCHITECT.
  - Bypass toggle row (#65) closed by ARCHITECT: *"no need UI, just add automatable parameter. it is not YAGNI, but it might be useful for HOST to "TELL" instance to BYPASS when doing something."* The automatable `bypass` row (`parameters.md:11`) and `getBypassParameter` (`EVEProcessor.cpp:174-177`) already serve the host. Design 7's toggle row and the Verification line "MASTER BYPASS toggle freezes the screen" are dropped.
  - Key resolution (ARCHITECT, *"why not it's CommandManager job?"*): `jam::CommandManager::invokeKey` resolves text chords and prefixes from the stored sheet. TextEditor's `keyPressed` runs the per-mode `getKeyInputs` entry (count, chord, text, forward). `getCarets` and `paintSelection` replace the paint mode tests. `getTypingModes` is deleted. No `TextEditorProcessor`.
  - Final state: build EXIT=0, 0 errors, 0 warnings in jam/ and eve/Source/ (`build-step14c.log`); doxygen 0 warnings (jam lib, eve project); SPEC, RFC §3.6 and EVE CLAUDE.md amended.
  - Open for ARCHITECT (superseded by the corrections above): Lean splits that need new names (#4 Windows `read` 63 lines, #9 POSIX `getEnvVar` 68, #47 TextEditor 1812 lines, `keyPressed` 82, `Content::paint` 64). The colour-table bound (#25): compaction invalidates a colour id that a writer holds between two `addColourId` calls (`TerminalProcessor.cpp:195-196`, `jam_AttributedString.cpp:139-140`). The bypass toggle row (#65): Design 7 "toggle row" against EVE's lane, where each component takes the full view bounds (`layout/component.md:3`).
- Known limits recorded (not in this plan's VT table): DEC special graphics charset (`smacs`/`acsc`) prints the ASCII letters; DCS/APC/PM/SOS strings are not lexed as strings (no cast vocabulary); DL at row 0 of a full-screen region retires the deleted rows to history (engine retirement rule); an unterminated sequence longer than one read block (OSC 1337 input, Out) is dropped.

---

# Round 2 — history on the JUCE audio pipeline, verbatim within CONTRACT

**Date:** 2026-10-25. Approved by ARCHITECT (ExitPlanMode). Scope ruling: *"Both, this plan"*.

## Overview

EVE's history follows JUCE's disk-streaming audio pipeline, with the JUCE API names. The contract changes three things: the type is the terminal wire bytes, the drain is the message thread, and every dependency is passed at the call site. The processor-side `SwapDocument` owns the history record, so the record outlives the widget. The TextEditor is the receiver: it builds the view on demand. The runtime fixes (no synthetic line, no live soft-wrap, font) fold in.

## Rulings (ARCHITECT, verbatim)

- *"follow JUCE pattern, API verbatim? within the adherence into our CONTRACT?"*
- *"why cant we just use audio pipeline?"*, after *"processBlock() the name already implies, process in chunk … why do we need to reinvent new machinary?"*
- *"TextEditor INDEED hold the history but already materialised on demand. that's exactly the design. but the document SSOT outlives the widget."*
- *"GUI is ephemeral … all state machine, swap, history etc could always outlives GUI. gui only materialisation, on demand"*
- *"the "COPY" is the transfer … still the contract HOLDS."*
- *"SwapDocument? as append-only document?"*, then *"SwapDocument, so the API something like toFile() is direct, no circus, no handroll"*
- *"evict and rehydrate are smells. FOREIGN SEMANTICS"*
- *"there should not be any synthetic history, no fake empty lines, dont hack. live buffer == viewport, FIFO to history, render … Viewport must be created naturally."*
- *"when content lines not exceeds the viewport available rows, SHOULD NOT SCROLL"*
- *"why not use the whole area minus padding, then repaint (relayout) according scrollbar visibility?"*
- *"history is append-only, clear with "clear" command."*
- R7 (SPEC.md:115): *"return to terminal, history should not be editable. read only, copy only"*
- R14: *"0 alloc, 0 copy, 0 mutations, unidirectional, SSOT, 0 temp containers, 0 new struct, 0 foreign semantics, 0 new pattern, 0 handroll"*

## Defects and causes (reads)

1. **Blank rows and a scrollbar.** The constructor parses a synthetic `"\n"` into the history (`jam_TextEditor.cpp:43-44`), because `setCursor` asserts one history line (`:1652`).
2. **Each live row wraps.** The grid columns come from the whole area (`EVEView.cpp:56-58`). Layout reflows at `getMaximumVisibleWidth()` (`jam_TextEditor.cpp:1280`), which excludes a visible scrollbar (`juce_Viewport.h:175-180`). `setLineRows` reflows every lane at the content width (`:1384`).
3. **Small font.** `style.css:877` `--height: 11`. END uses 12 (`end/Source/config/theme/gfx/theme.md:74`).
4. **Ownership.** The TextEditor owns `historyDocument` and `Document::Swap` (`jam_TextEditor.h:670`, `:675`). Both die with the view.
5. **`Document::Swap` bounds nothing.** `AnsiDocument::addLines` calls `addSource` (`jam_AnsiDocument.cpp:5-9`). `addSource` appends every byte to `source` (`jam_Document.cpp:261-274`), and nothing trims it. `evict` clears the cells of complete lines (`jam_DocumentSwap.cpp:57-60`).

## JUCE pattern, mapped verbatim

### Writer side — `AudioFormatWriter::ThreadedWriter::Buffer` (`juce_AudioFormatWriter.cpp:223-345`)

| JUCE | EVE | Contract delta |
|---|---|---|
| `AbstractFifo fifo` + `write()` (:244-266) | `TerminalFIFO history` + `write`/`flush` (`jam_TerminalFIFO.h:35`, `:60`, `:88`) | exists |
| `writePendingData()`: `prepareToRead`, `writer->…(start1,size1)`, then `(start2,size2)`, `finishedRead` (:273-316) | `TerminalFIFO::pull (SwapDocument& destination)`, in place of `pull (AnsiDocument&)` (`jam_TerminalFIFO.cpp:66-75`). It gives each contiguous block to `destination.write (data, size)`. | The writer comes at the call, not as a stored pointer (CODING: no non-owning pointer members). No bail-out guards (:246, :280). |
| `std::unique_ptr<AudioFormatWriter> writer` → file (:337) | `SwapDocument` owns `juce::TemporaryFile file` and its `juce::FileOutputStream`. `write (data, size)` appends the block to the file. | The bytes are the encoding, so there is no codec. |
| `receiver->addBlock (samplesWritten, buffer, start, size)` (:288, :297) | The TextEditor's `addBlock (block)` runs at each pull for each new block. It records the block's line count and pixel height. | The receiver comes at the call (getter visitor), not through `setDataReceiver` (:318) and `thumbnailLock` (:338). Drain and view share the message thread, so no lock (R2, `PLAN-tty.md:18`). |
| `TimeSliceThread` (:268-271) | none: `pull()` drains on the message thread | R1/R2 (`PLAN-tty.md:17-18`) |
| `setFlushInterval` / `writer->flush()` (:304-313) | `SwapDocument::write` flushes its stream after each block | The default of 0 never flushes before destruction. A per-block flush keeps the record on disk while the plugin lives. |

### Reader side — `BufferingAudioReader` (`juce_BufferingAudioFormatReader.h:47-104`)

| JUCE | EVE | Contract delta |
|---|---|---|
| `source` → the file (:94) | the same `file` of `SwapDocument` | — |
| `blocks`, `numBlocks` (:97, :101) | `SwapDocument` holds a fixed number of resident blocks, `numBlocks`. Each block is one `AnsiDocument`. It parses one block's byte range with `addLines`, so it is complete when it is created (MANIFESTO E). | `BufferedBlock` (:79-86) is not copied. R14 forbids a new struct, and `AnsiDocument` already is the capability. |
| block `range` (:83) | `SwapDocument` holds the file byte range of every block: `jam::Array<juce::Range<juce::int64>> ranges`. This is value data, appended once per `write`. | — |
| `getBlockContaining (pos)` (:89) + `readNextBufferChunk()` (:90) | `getOrCreateBlock (blockNumber)` returns the resident block. When the block is absent, it reads that range from the file and parses a new block. It then drops the blocks outside the window of `numBlocks` around it. | NAMES Rule 3: the read is a side effect, so the name uses `getOrCreate` (Rule 1 verb table). No thread, no `CriticalSection` (:100), no `timeoutMs`. |
| `samplesPerBlock` (:92) | One block is one drain of `pull()`. The engine writes whole rows to the history FIFO (`appendFrameRow`, `appendRowEnd`), so a drained block always ends at a line boundary. | A block is a parse unit, so it cannot cut a line. |

### Receiver — `AudioThumbnail` (`juce_AudioThumbnail.h:59-149`)

| JUCE | EVE TextEditor |
|---|---|
| `addBlock` reduces each block to a summary for the whole source (`.cpp:774-807`) | `addBlock` records the block's line count and pixel height at the current width. This is the scroll geometry for the whole history. Resident blocks keep their `Document::Index` (unchanged in `jam_core`; MarkdownComponent also uses it). |
| `reset` → `clear` + new channels (`.cpp:643-653`) | `reset()`: on a width change, the geometry is derived again from every block, read from the file one block at a time. SPEC Feature 2.1 requires layout derived again from the logical lines, byte-identical. |
| `clear()` (`.cpp:625-630`) | `SwapDocument::clear()` (the `clear` command). The trigger stays out of scope (SPEC Feature 2.2). |

## Design consequences

1. **`Document::Swap` leaves `jam_core`.** Delete `jam_Document.h:776-870`, `jam_DocumentSwap.cpp`, `jam_core.cpp:14` and `user-modules-info.md:259`. Delete the TextEditor's `swap`, `getCodec` (`.h:656-664`, `.cpp:112-155`) and every `swap.*` call.
2. **`SwapDocument`** (`jam_terminal/document/jam_SwapDocument.h/.cpp`). It is a `Document::Writer`-family record: `toFile (const juce::File&) const` writes the whole record (`jam_Document.h:982`). It owns `file`, the output stream, `ranges`, `blocks` and `numBlocks`, and has `write`, `getOrCreateBlock`, `clear` and `toFile`. It is not a subclass of `AnsiDocument`, because `addLines` is not virtual (`jam_AnsiDocument.h:74`) and the record holds blocks, not one parse.
3. **Owner: `jam::TerminalProcessor`.** `SwapDocument historyDocument;` sits next to `liveDocument` (`jam_TerminalProcessor.h:90`). `pull()` loses its parameter (`:63`). `getHistoryDocument()` joins `getLiveDocument()` (`:73`). The class doc (`:13-14`) states the new ownership.
4. **EVE getter.** The visitor gets both documents and whether the live document was replaced. The `historyDocument` parameter and `getHistoryCapacity` at the call go (`EVETerminalProcessor.cpp:46-51`). `HISTORY_CAPACITY` keeps sizing the history FIFO (`jam_TerminalProcessor.cpp:25`, through `prepare`).
5. **TextEditor = receiver.** It owns no document. The history lane draws the resident blocks that the visible pixel range needs, found through the geometry, and builds an `Index` per resident block. `getOrCreateCells` becomes `getCells` (NAMES Rule 3: it no longer creates).
6. **History is read-only (R7).** Delete the edit machinery that writes to the history: `InsertLine`, `RemoveLine`, `addLine`, `removeLine`, `setLine`, `addText`, `applyText`, `undoManager`, and every action that calls them. The compiler lists the call sites (Refactor-Rewrite Discipline).
7. **No synthetic line; one cursor line space.** Delete `jam_TextEditor.cpp:43-44`. A line number `n` below the history line count reads history. Above it, `n` reads the live lane at `n − historyLines` (the offset already used at `:928`). `setCursor` limits the line over the total and loses the assert at `:1652`. `leaveMode` from TERMINAL sets the cursor to `historyLines + liveCursorLine`, `liveCursorColumn` (Neovim `terminal.c:1001-1005`). `valueTreeRedirected` (`:1144-1155`) stops its `setCursor` calls: a restored node is complete state.
8. **The live lane does not soft-wrap.** `setLineRows` (`:1384`) takes the wrap width per lane: the live lane passes the line's own advance width, so each line gives one row. History reflows at the content width.
9. **Grid width.** It stays the whole area minus padding (`EVEView.cpp:56-58`). `visibleAreaChanged` (`:1278-1287`) already lays out again on a scrollbar change. With no synthetic line, nothing scrolls while the content fits.
10. **Font.** `style.css:877` `--height: 12`.

## New names (ratified by plan approval)

- From JUCE, verbatim: `write`, `flush`, `addBlock`, `reset`, `clear`, `blocks`, `numBlocks`.
- Rule 3 adjustment: `getOrCreateBlock`.
- Family siblings: `historyDocument` (`liveDocument`), `getHistoryDocument` (`getLiveDocument`), `toFile` (Writer), `ranges` (`BufferedBlock::range`), `getCells` (`getCell`, `getCellBounds`).
- ARCHITECT: `SwapDocument`.

## Validation Gate

COUNSELOR validates each step before the next one, against MANIFESTO, NAMES, CODING and this plan: no stored non-owning pointer, no bail-out guard, no new struct, no foreign verb, no comment, names only from this plan. @Auditor runs once, after the last step.

## Steps

0. Plan of record: this section.
1. **Delete first.** `Document::Swap` (consequence 1); the TextEditor's `historyDocument`, `swap`, `getCodec`, the synthetic line and `swap.setCapacity`; the edit machinery (consequence 6); the `valueTreeRedirected` clamping. Validation: the grep for `Document::Swap|getCodec|evict|rehydrate|swap\.` gives no hit outside `carol/` and `docs/`.
2. **`SwapDocument`.** New files. Include them after `AnsiDocument` (`jam_terminal.h:80`, `jam_terminal.cpp:18`). Add the `user-modules-info.md` row. Validation: the writer side and the reader side match the tables above.
3. **`TerminalFIFO::pull (SwapDocument&)`** replaces `pull (AnsiDocument&)`. The live path keeps `pull()` (`:77-88`) with the parse inline. Validation: the call shape matches `writePendingData`, with no guards.
4. **Processor owns the history** (consequence 3).
5. **EVE getter** (consequence 4).
6. **TextEditor receiver:** `addBlock`, `reset`, the geometry, the block lane, `getCells` (consequence 5).
7. **Cursor line space** (consequence 7).
8. **Live lane without soft-wrap** (consequence 8).
9. **Font** (consequence 10).
10. **Documents.** RFC:518-525 ownership. SPEC Feature 2.3 (`SPEC.md:94-102`) and State Model (`:174-183`): `SwapDocument`, blocks, receiver. `ViewLayout.md:195`: `HISTORY_CAPACITY` becomes *"history FIFO size in rows"*.
11. **Build.** cast `--debug`, 10-minute cap, build only. If the build fails, use whatdbg (R13).
12. **Audit** (once). Resolve the findings, run the doxygen pass, then build again.
13. **ARCHITECT runtime check.** Then `/log`, commit messages, and the KANJUT sync (R12).

## Verification (ARCHITECT, runtime)

- Launch: no blank row above the prompt, no scrollbar, and no scroll while the content fits.
- `ls`: each row shows once.
- Output past one screen: the scrollbar shows, history scrolls, and live rows do not wrap.
- `` `] ``: the cursor lands on the shell cursor. `hjkl` moves across history and live lines.
- Close and reopen the plugin editor: the history is still there.
- Resize narrow, then back: the history layout is the same.
- Font size matches END.

## BLESSED Alignment

- **B:** the chain owns `SwapDocument`, its file and its blocks. The widget owns only its geometry and its glyph runs.
- **L:** no thread, no lock, no codec, no per-line residency sets.
- **E:** the writer and the receiver come at the call. Each block is complete at creation and is replaced whole. No complete line changes. Data moves one way: audio → FIFO → file → block → view.
- **S:** one record, the file, written once at creation. `ranges` indexes it. The FIFO is transit.
- **D:** the same bytes give the same blocks, and the same layout at a given width.

## Out (approved with the plan)

- ED 2 retirement, and ED 3 / the `clear` trigger (SPEC Feature 2.2).
- Non-destructive live reflow on SIGWINCH.

## Execution State (Round 2)

- Step 0: done.
- Step 1 (jam_core): done, validated. `Document::Swap` is gone from `jam_Document.h` (`:774-777` now run `Index` → `Vocabulary`); `jam_DocumentSwap.cpp`, its `jam_core.cpp` include and its `user-modules-info.md` row are deleted.
- Correction (read, block size): a drain can be one row, so a block of one drain breaks a fixed resident window. A block closes at `bytesPerBlock` (JUCE `samplesPerBlock` = 32768, `juce_BufferingAudioFormatReader.h:92`) and always ends at a row end: each history row ends with SGR reset + newline (`jam_TerminalGraphicsEngine.cpp:196-200`) and the FIFO publishes after whole rows (`:670`). The resident window follows JUCE `nextReadPosition` (`:96`): `setNextReadPosition (blockNumber)` drops the blocks outside `[blockNumber, blockNumber + numBlocks)`. `getNumRanges()` gives the block count (`getNum*` family).
- Correction (MANIFESTO L, YAGNI): `SwapDocument::clear()` is not written. Its trigger is Out (SPEC Feature 2.2), so it would have no caller. `toFile` stays (ARCHITECT: *"the API something like toFile() is direct"*).
- Step 1 (TextEditor): done, validated by a full read. `historyDocument`, `swap`, `getCodec`, `undoManager`, `InsertLine`/`RemoveLine`, `addLine`/`removeLine`/`setLine`/`addText`/`applyText`, the 15 edit actions and their helpers, the synthetic line and the `valueTreeRedirected` clamping are deleted. `isChangingWord` (`CommandId::change`) and `getStartColumn`'s `isChanging` parameter went with the `change` action.
- Steps 2–5: done, validated. `jam_SwapDocument.h:7-28`, `.cpp:5-69`; `jam_terminal.h:81`, `jam_terminal.cpp:19`; `TerminalFIFO::pull (SwapDocument&)` (`jam_TerminalFIFO.cpp:66-75`) and the live `pull()` with the parse inline (`:77-95`); `TerminalProcessor::pull()`, `getHistoryDocument()`, member `historyDocument`; EVE getter `visit (SwapDocument&, AnsiDocument&, bool)` (`EVETerminalProcessor.cpp:46-52`). The open block is always resident: `write` creates it, and `setNextReadPosition` keeps it. Correction: `juce::pointer_sized_int` in place of an unqualified `ssize_t` (`jam_SwapDocument.cpp:42`; Windows declares `ssize_t` in namespace `juce`). `user-modules-info.md` lists no `jam_terminal` document row, so no row was added.
- Steps 6–8: done, validated by a full read.
  - Lanes: `jam::HashMap<int, Lane> history` keyed by block number; `blockLines`/`blockPixels` are the per-block summary.
  - Block functions: `addBlock`, `reset`, `addVisibleBlocks`, and the global geometry `getNumLines`, `getNumHistoryLines`, `getNumHistoryPixels`, `getBlockNumber`, `getBlockNumberAt`, `getBlockStart`, `getBlockTop`, `getCellBounds (int, int)`, `getCell (SwapDocument&, Point)`.
  - Every history access runs inside `terminalSource (shouldSetLane, apply)`, in `perform`, the mouse overrides, the timer and `lookAndFeelChanged`. The editor stores no document (CODING: no non-owning pointer members). `perform` sets the cursor first, so the cursor's block is resident for every command.
  - `leaveMode` from TERMINAL lands on `getNumHistoryLines() + liveCursorLine`. The live lane wraps at `max (contentWidth, line advance)`.
  - Correction (NAMES Rule 3): `getOrCreateCells` keeps its name, because it now lays out the line's block on demand.
  - Recorded: the Engineer edited the `history` member doc (`jam_TextEditor.h:597`) before the audit; the doxygen pass owns it.
  - Known limit: an image that covers lines past a block end paints only in its own block; each block parses with a fresh pen.
- Step 11: build EXIT=0, 0 errors, 0 warnings (`scratchpad/build-r2.log`). jam cast regenerated `generated/jam_Gui.h` with no deleted row left.
- Step 12 audit: NEEDS_WORK, 11 bugs + 32 findings. COUNSELOR read each cited line (Design by Contract Filter).
  - Resolved:
    - **B1, B11:** `SwapDocument::write` closes a block at the first line end after `bytesPerBlock`, inside each call. The ring wrap can no longer cut a line.
    - **B2, F3, F4:** `setNextReadPosition` is deleted. `removeBlock` is driven by the TextEditor's lane erase in the same branch, so a lane and its block leave together. `numBlocks` is gone.
    - **B3, B8:** the first pull of an editor runs with `shouldSetLane`, so it builds its live index, and `reset` keeps only the open block.
    - **B4, F9:**
      - `setCursor` loses its assert (plan consequence 7).
      - `perform` dispatches only when a line exists. The named threat is no line before the first live frame.
      - The per-command cursor re-check is gone.
      - `moveHalfPage` reads lines through `getOrCreateCell`.
    - **B5:** `setLanes` limits a stale cursor line at the owner of content change, with no listener.
    - **B6:** `FileOutputStream` buffer size 0 and no `flush` (`juce_FileOutputStream.cpp:95-114`; `flush` runs fsync).
    - **B7, F10:** `visibleAreaChanged` triggers a `juce::AsyncUpdater` pull, the established form of `jam_PluginEditor.h:31`.
    - **B10:** `prepare` drains the history FIFO before its reset.
    - **F2, F20:** cumulative `blockLineEnds` and `blockPixelEnds` with `std::upper_bound`, the `Document::lineOffsets` form.
    - **F5:** `getOrCreateCell`.
    - **F7:** `addBlock` returns `void`.
    - **F8:** `caretWidth` deleted.
    - **F11:** `setLanes`, `addVisibleContent` and `setColours`.
    - **F14:** `historyDocument` is a `unique_ptr` with a const getter, the sibling form of `liveDocument`.
    - **F15:** `bytesPerBlock` is private.
    - **F16:** `last()` and the `try_emplace` entry.
    - **F17:** the stream assert moves to the constructor.
    - **F21:** `getNumBlocks`.
    - **F22:** `setLineRows (…, wrapWidth)`, decided by the caller.
    - **F23:** live-lane selection is painted.
    - **F25:** a closed block is complete; the open block grows.
    - **F26:** `TerminalProcessor::Visit`.
  - Rejected, with the read that refutes each:
    - **B9:** with the editor closed, the PTY holds the backpressure (`:161`), and the record outlives the view.
    - **F1:** the per-block summary is the receiver record of the plan, with one writer, `addBlock`.
    - **F6:** `addBlock` inserts a lane into `history`, the `add` contract.
    - **F12, F13:** ARCHITECT ruled on the TextEditor size in Step 14 (`:209-211`, no `TextEditorProcessor`).
    - **F18:** creation can use a work buffer (MANIFESTO E).
    - **F19:** ARCHITECT, *"the API something like toFile() is direct"*.
    - **F24, F27:** the ViewManager getter binding contract.
    - **F28, F29:** plan rulings (consequence 4, SPEC Feature 2.1).
    - **F30:** nested components are members of their owner (MANIFESTO B holds).
    - **F31:** the residency check is the caret contract.
    - **F32:** *"the COPY is the transfer"*.
  - New names, from families: `removeBlock`, `getNumBlocks`, `addBlock` (SwapDocument), `bytesPerBlock` (JUCE `samplesPerBlock`), `blockLineEnds`, `blockPixelEnds`, `setLanes`, `addVisibleContent`, `setColours`, `getOrCreateCell`, `Visit` (the `visit` variable, nested-alias form of `ReflowDocument::Measure`).
- Step 12 doxygen: docs for `SwapDocument`, `TerminalProcessor`, `TerminalFIFO`, `TextEditor`, `EVETerminalProcessor`. JAM 0 warnings, EVE 0 warnings (`scratchpad/doxygen-jam-r2.log`, `doxygen-eve-r2.log`). EVE has no `ninja doxygen` target; the project step of `doxygen.lua` regenerated it.
- Step 12 build: EXIT=0, 0 errors (`scratchpad/build-r2b.log`). Ninja found no work: ARCHITECT's concurrent `cast` build had compiled the final tree, so its warning count is not in this log.
- Step 13: `/log` written (jam `carol/SPRINT-LOG.md`, Sprint 172). ARCHITECT's runtime check and the KANJUT sync (R12) come next.
- Step 1 (edit machinery, R7): the deleted actions take insert mode with them. `insert` was reachable only through `enterInsert`. So `map::EditorMode::insert`, the `insert` key input, caret and key-sheet rows, EVE's `## insert` sheet and the 15 `map::CommandId` rows of the deleted actions are deleted. `ViewLayout.md` version moves to `0.0.4`, so user copies are replaced (`jam_PluginEditorLayout.cpp:27-33`).

---

# Round 3 — full throttle, O(visible) view, fonts out of the box

**Date:** 2026-10-25. Approved by ARCHITECT (ExitPlanMode). It follows ARCHITECT's runtime test of Round 2 (Sprint 172).

## Problems (ARCHITECT's runtime test) and causes (reads)

| Problem | Cause |
|---|---|
| `seq 100000` takes 14.7 s at 0% CPU. endless finishes it at 97% CPU. | The decoder reads the PTY only when the live FIFO is empty (`jam_TerminalProcessor.cpp:38`). It reads once per block (`:40`). Only the view's 60 Hz timer empties the live FIFO (`jam_TextEditor.cpp:86`, `jam_ComponentUtils.h:17`). So there is at most one PTY read per GUI frame. endless drains to EAGAIN (`endless/…/TTY.cpp:58-66`). |
| Closing after `seq 100000` hangs for seconds. | `PluginEditor::~PluginEditor` calls `setLookAndFeel (nullptr)` (`jam_PluginEditor.cpp:40`). That calls `lookAndFeelChanged` on the TextEditor, then `terminalSource (true, …)`, then `reset`, which parses and lays out every history block (`jam_TextEditor.cpp:769-785`). A width change pays the same cost. |
| Text and box-drawing lines are thin. | `EMBOLDEN` defaults to `false` (`ViewLayout.md:176`). END and endless default to `true`. Embolden doubles the procedural line thickness (`jam_TerminalGlyph.h:317-327`). |
| Nerd icons are missing or scaled wrong. | The fallback asks only the OS (`jam_Typeface.cpp:409-438`). EVE bundles no symbol face. endless bundles `SymbolsNerdFont-Regular.ttf` (`endless/Source/MainComponent.cpp:107-111`). The cell-fit table is Nerd Fonts v3.4.0 (`jam_GlyphConstraintTable.cpp:1-8`). |
| 12 pt looks smaller than endless. | Not found by reading. `setPointHeight` sets "the size of the font's em-square in logical pixels" (`juce_Font.h:276-282`). Every Display Mono file has the same advance of 0.6132 em (fontTools). A runtime measurement is needed. |

## Rulings (ARCHITECT, verbatim)

- *"eve should bundle nerd font, just like endless with display mono. it should work OOTB"*
- Pull clock: **`juce::VBlankAttachment`**.
- *"can we make it always O(1)?"*; *"how can we make the traffic without gate? always FULL THROTTLE?"*
- *"the events chain should not be manually managed, lock free, wait free, yield free, always full throttle"*
- *"why producer cant just always send everything? Message thread could queue if genuinely needed, but full throttle means the plumbing should leave no possiblity of data loss … audio never drop anything, not even a single sample"*; *"why now we need BufferSPSC again? TerminalFIFO is not the buffer?"*
- Amplified excess (one LF retires a whole row, `jam_TerminalGraphicsEngine.cpp:479-486`): **"PTY holds the input"**.
- Wheel in the line model: **"Eased, by rows"**.
- *"we need the font to be embolden, user configurable default settings should be true"*
- *"ask the right questions no read can't answer. obey the protocol"*

## Design

### A. Full throttle: event-driven, no gate, no loss

1. **Producer clock = `processBlock`.** Each block reads the PTY until `read` returns 0 or the read buffer is full. The gate is deleted, so the decoder never waits for the view.
2. **Lanes are sized from the clocks at `prepare`.**
   - The per-block output bound is screenBytes + rowBytes × `readChunkSize` / width (the `hasHeadroom` expression, `:118-125`).
   - `blocksPerVblank` = ⌈sampleRate / maximumBlockSize / refreshRateHz⌉. The refresh rate comes from the primary display's `verticalFrequencyHz`, read as `VulkanEngine::getFrameBudget` reads it (`jam_VulkanEngine.cpp:218-221`). The fallback is `VulkanEngine::indeterminateRefreshRateHz`, which becomes public.
   - History capacity = the per-block bound × `blocksPerVblank`.
   - `HISTORY_CAPACITY` and `getHistoryCapacity` are deleted.
3. **The producer sends everything.** History rows retire every block. The only limit is the ruled one: a token whose output does not fit stays in the read buffer, and the PTY holds what follows.
4. **Live lane = the newest frame.**
   - The engine grid is the live-screen source of truth.
   - A block publishes a frame when the live lane is empty. Otherwise the grid keeps the state for the next block.
   - The engine's `process` splits into `process (history, newLiveRows)` and `appendFrame (live)`.
5. **Consumer clock = vblank.**
   - The TextEditor pulls on `juce::VBlankAttachment` (`juce_VBlankAttachment.h:60`).
   - `juce::Timer` and `juce::AsyncUpdater` are deleted.
   - Each vblank drains every lane into the record, the message-thread queue.

### B. O(visible) view: scroll by lines, eased by rows

1. **Scroll state** lives on the TextEditor node:
   - `scrollOffset` is lines from the tail; 0 is the tail (the documented binding of `jam::Scrollbar`, `jam_Scrollbar.h:58`).
   - `scrollRow` is the rows of the bottom visible line that lie below the view.
   - New output with `scrollOffset > 0` adds the appended line count, so the view stays still.
2. **No pixel content.**
   - `jam::Viewport`, the full-height `Content`, `setContentSize`, `blockPixelEnds`, `getBlockTop`, `getBlockNumberAt`, `getNumHistoryPixels` and `reset` are deleted.
   - `Content` covers the view and paints bottom-up from line `getNumLines() - 1 - scrollOffset`.
   - The block summary is `blockLineEnds` only.
3. **Scroll bar.** The existing, unused `jam::Scrollbar` (`jam_Scrollbar.h:27-60`), with `setRange (lines, visibleLines)` and its value `referTo` the node's `scrollOffset`.
4. **Wheel eased by rows.**
   - `juce::Animator` over `juce::VBlankAnimatorUpdater` (the `jam::Viewport` form, `jam_Viewport.h:86-89`, `.cpp:109-124`), with the same display settings.
   - `jam::Viewport::getDurationMs` and `getEasing` become public static, so there is one copy.
   - Trackpad deltas move rows directly.
5. **O(visible) layout.**
   - Each vblank makes resident only the blocks of the visible lines.
   - A width or LookAndFeel change lays out the resident lanes again.
   - Any other block is laid out at the current width when it becomes resident.
6. **Geometry.**
   - `getVisibleLines()` returns the visible line range, and `getLineTop (int)` returns a line's top.
   - These feed `getCellBounds`, `getOrCreateCell`, `keepCursorVisible` and `moveHalfPage`.

### C. Fonts out of the box

1. `EMBOLDEN` defaults to `true`, and the layout version goes to `0.0.5`.
2. **Bundle the symbols face.**
   - `SymbolsNerdFont-Regular.ttf` (Nerd Fonts 3.4.0) goes in `eve/Source/fonts/`.
   - A `symbols` row in the `project-info.md` layout table.
   - A `style.css` `@font-face` `symbols`, which `StyleManager` registers even when no rule uses it (`jam_StyleManager.cpp:48-54`, `:77`, `:99-101`).
3. **Bundled first.** `Typeface::getOrCreateFallback` searches the bundled entries (not `isSystemTypeface`) in registration order, and asks the OS only after that.
4. **Font size.**
   - Measure with a whatdbg `trace` in `GlyphRasterizer::calcMetrics` (`fontHeightJuce`, `pixelsPerEm`, the cell).
   - The target is a 12 logical px em, the endless value (`778c32168:jam_typeface.mm:225-228`).
   - On a mismatch, fix the conversion at its owner, never in CSS.

## New names (families)

- `blocksPerVblank`
- `appendFrame` (`append*`)
- `scrollOffset` (`jam::Scrollbar`)
- `scrollRow` (`scroll*` Ids)
- `getVisibleLines`, `getLineTop`
- `vBlankAttachment`
- `symbols` (font alias, project-info row)

## Steps

0. Plan of record (this section).
1. **Delete first:**
   - the gate and the single read
   - `HISTORY_CAPACITY` and `getHistoryCapacity`
   - the TextEditor pixel model
   - `juce::Timer` and `juce::AsyncUpdater`
2. **Lanes from the clocks:**
   - `TerminalProcessor::prepare (int blocksPerVblank, TerminalGraphicsEngine&)`
   - EVE computes `blocksPerVblank`
   - `indeterminateRefreshRateHz` becomes public
3. **Full-throttle block:** read until 0 or the buffer is full, retire history every block, `appendFrame` when the live lane is empty, split the engine `process`.
4. **Vblank pull.**
5. **Line scroll:**
   - the Ids and node properties
   - `jam::Scrollbar`
   - bottom-up paint
   - `getVisibleLines`/`getLineTop`
   - residency
   - cursor, half page, mouse, carets, selection
6. **Eased rows.**
7. **O(visible) reflow.**
8. **Embolden default and layout version.**
9. **Symbols bundle.**
10. **Bundled-first fallback.**
11. **Build** (cast, 10 min).
12. **Font size trace,** then fix or report, then build.
13. **Documents:** SPEC, RFC, `ViewLayout.md`.
14. **Audit,** then doxygen, then build.
15. **`/log`** and commit messages. ARCHITECT's runtime test and the KANJUT sync (R12) follow.

## Verification (ARCHITECT, runtime)

- `seq 100000` is well under a second with the CPU busy.
- Closing is instant.
- A resize or font change is instant at any history length.
- Nerd icons render and fit.
- Text is bold by default.
- The wheel is eased.
- The scroll bar drags.
- A scrolled view stays still on new output.
- The view follows the tail at offset 0.
- Modes, motions and selection work over history and live.

## Execution State (Round 3)

- Step 0: done.
- Steps 1–3, plus the Step 8 data (batch P): done, validated by reads.
  - The engine `process (history, newLiveRows)` retires rows and flushes history. `appendFrame (live)` writes the newest frame (`jam_TerminalGraphicsEngine.cpp:636-673`).
  - `TerminalProcessor::prepare (int blocksPerVblank, …)` sizes history at `blocksPerVblank × (liveCapacity + rowCapacity × readChunkSize / width)` (`jam_TerminalProcessor.cpp:16-34`).
  - `process` drains the PTY until 0 or the buffer is full, keeps the ruled fit rule (`hasHeadroom`), and publishes when the live lane is empty, with no gate.
  - EVE computes `blocksPerVblank` from `processor.getSampleRate()`/`getBlockSize()` and the display refresh rate (`EVETerminalProcessor.cpp:98-102`). `VulkanEngine::indeterminateRefreshRateHz` is public.
  - `HISTORY_CAPACITY`, `getHistoryCapacity` and the EVE `historyCapacity` Id are deleted. `EMBOLDEN` is `true`, and the layout version is `0.0.5`.
  - Known limit: the lanes are sized at a grid change from the processor's rate and block size. A rate change without a grid change keeps the old size, and the fit rule keeps it lossless. Superseded by F5, F9: `prepare` sizes the lanes from each `ProcessSpec` (`EVETerminalProcessor.cpp:121-125`).
  - Superseded by F27: the engine `process()` only advances the frame, and `appendFrame` publishes it (`jam_TerminalProcessor.cpp` `process`).
  - Live publish: every block whose live lane is empty republishes the grid. When idle, that is one frame per vblank pull.
- Steps 9–10 (batch F): done, validated.
  - `eve/Source/fonts/SymbolsNerdFont-Regular.ttf` (sha1 `df7b1b7e…`).
  - The `project-info.md` `symbols` row.
  - The `style.css` `@font-face symbols`.
  - `findBundledFallback` before the OS search in `getOrCreateFallback` (`jam_Typeface.cpp:409-457`). COUNSELOR corrected it to brace initialisation.
- Steps 1 and 4–7 (batch V): done, validated by reads.
  - Line scroll on `scrollOffset`/`scrollRow` (cast Ids, node properties), with `jam::Scrollbar` bound by `referTo`.
  - `getVisibleLines`/`getLineTop`/`getLineHeight`/`getScrollPosition`/`setScroll`, all O(visible).
  - Residency for the visible lines only (`addVisibleBlocks`). A width or LookAndFeel change lays out the resident lanes only (`setLanes`).
  - `juce::VBlankAttachment` pull. `juce::Timer` and `juce::AsyncUpdater` are deleted.
  - The eased row animation reuses the `jam::Viewport` public statics (`notchUnit`, `smoothUnit`, `getDurationMs`, `getEasing`).
  - Step Gate corrections, from the contract with no ARCHITECT round trip:
    - wheel sign (`jam_Viewport.cpp:88`)
    - `getLineHeight` (DRY)
    - `setScroll` limited to the scroll bar range
    - a notch during an animation first finishes the running one (`scrollTarget`, the `jam::Viewport` form)
    - a smooth event resets the animator
  - COUNSELOR rounds a smooth delta away from zero, so a slow trackpad gesture still scrolls (`jam_TextEditor.cpp` `Content::mouseWheelMove`).
  - Added name from a family: `getLineHeight` (`get*`). Scroll property listeners add the visible runs before a repaint.
- Step 11: build EXIT=0, 0 errors, 0 warnings (`scratchpad/build-r3.log`). `CMakeLists.txt:517` globs `Source/fonts/*.ttf`.
- Step 12: whatdbg `trace` (spec `scratchpad/trace-fontsize.json`).
  - `calcMetrics` for the mono face: `fontHeightJuce = 12`, `pixelsPerEm = 12`, `x_ppem = 12`, `metrics.height = 960` (15 px).
  - The rasterizer key: `fontHeight = 12`, `composedScale = 2`, `key.fontSize = 24`.
  - The em is 12 logical px, the same as endless (`778c32168:jam_typeface.mm:225-228`). Per the plan the size stays. The thin look came from embolden being off, which is now on by default.
- Step 13: SPEC (Feature 2.3, Feature 6, State Model Record/Pull/Geometry/Scroll) and the RFC amendment of 2026-10-25 (full throttle, O(visible)) are written.
- Step 14 audit: NEEDS_WORK, 47 findings. COUNSELOR read each one (Design by Contract Filter).
  - Resolved, processor side:
    - **F1, F2:** 64-bit capacity, sized as one screen plus one row per block.
    - **F5, F9:** `prepare` sizes the lanes from the `ProcessSpec` (the processor rate is the fallback). `reset` holds the terminal state. This is the juce::dsp form.
    - **F6:** history is staged with `write` and published by `flush` only together with `appendFrame`. The view reads history, then live, so it never sees history ahead of its frame.
    - **F8:** `appendFrame` writes only when `frameId` changed (`publishedFrameId`).
    - **F27:** the dead retire read goes. `process()` advances the frame.
    - **F34:** `VulkanEngine::getRefreshRateHz`.
    - **F36:** one read expression, which never reads 0 bytes.
    - **F4:** input that does not fit the lane queues on the message thread (`pendingInput`, `writePendingInput`) and drains on each pull (*"Message thread could queue if genuinely needed"*).
    - **F12:** `SwapDocument::getNumLines` counts the newlines it writes, so the summary needs no layout.
  - Resolved, view side:
    - **F16:** history wraps at the content width after a relayout.
    - **F32, F45:** `isResident`, `getLane`, `getLaneLine`.
    - **F17, F18, F24:** `pendingRows`. The eased steps are relative, a new notch adds to the remaining rows, and a smooth delta accumulates.
    - **F29:** `setScrollPosition`.
    - **F19, F20:** the position is normalized.
    - **F21, F37:** the scroll is reset at bind.
    - **F26:** the scroll bar strip is reserved only while the scroll bar shows.
    - **F41:** relayout only on a width change.
    - **F13:** one walk for a cell.
    - **F44:** `addLaneRuns` is a member.
    - **F38:** the timestamp form of `VBlankAttachment`.
    - **F33:** the modifier-key filter of `jam::Viewport`.
    - **F40:** the guard becomes an assert (COUNSELOR).
    - **F43:** `setLanes` loses its layout loop.
    - Engineer issues, ruled by COUNSELOR from the reads:
      - `isResident` is per line. A history line is resident when its block lane exists and has laid out that line (`jam_TextEditor.cpp:872-879`). The visit runs `setLanes`, then `apply`, then `addVisibleContent` (`:68-70`). So the open block can have lines that are counted but not laid out between the pull and the residency step (F14, F43).
      - `getOrCreateCell` walks once (F13). A line that is not resident uses the one-row estimate until the next residency step. This is the same transient.
      - A scroll bar visibility change calls `resized`, and a width change pulls inside the running visit. This is the path that `perform` and the mode handler already use.
    - Processor side validated by reads: `EVETerminalProcessor.cpp:44-126`, `jam_TerminalGraphicsEngine` `publishedFrameId` is `uint64_t` (COUNSELOR).
  - Rejected, with the read that refutes each:
    - **F3:** Out (`:214`).
    - **F7, F11:** "PTY holds the input". With no view, nothing pulls, so the record does not grow.
    - **F10:** one view per processor.
    - **F14:** residency on vblank (design B.5).
    - **F15, F25:** no clause.
    - **F30, F31:** family names and the one-copy rule.
    - **F35:** the two formulas differ.
    - **F39:** the `jam::Viewport::scrollTarget` precedent.
    - **F42:** the record is read only inside the visit (Round 2).
    - **F46:** ruled.
    - **F47:** recorded.
  - Known limits:
    - **F22:** `keepCursorVisible` and the half page count lines, so they are approximate on wrapped history lines.
    - **F23:** the half-page line and row units match only for one-row lines.
    - **F26 (status row part):** the status row covers the bottom row in normal and visual mode, by design.
- Step 14 doxygen: the Round 3 headers in jam (`jam_TextEditor.h`, `jam_TerminalProcessor.h`, `jam_TerminalGraphicsEngine.h`, `jam_SwapDocument.h`, `jam_VulkanEngine.h`, `jam_Viewport.h`, the `jam_Typeface.h` `getOrCreateFallback` doc) and `EVETerminalProcessor.h`. Regenerated with the `build-doxygen.sh` lib and project steps: JAM 0 warnings, EVE 0 warnings (`scratchpad/doxygen-jam.log`, `doxygen-eve.log`).
- Step 14 build: jam cast EXIT=0; EVE cast `--debug` EXIT=0, 0 errors, 0 warnings (`scratchpad/cast-jam-r3final.log`, `build-r3final.log`).
- Step 15: `/log` as Sprint 173 in `jam/carol/SPRINT-LOG.md`. Next: ARCHITECT's runtime test (Verification above), then the KANJUT sync (R12).
