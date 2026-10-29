# PLAN: Seq Profiling — Pinpoint the 100M Bottleneck Against endless, Same Conditions

**RFC:** none — objective from ARCHITECT's prompt (session 2026-10-29)
**Date:** 2026-10-29
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM (LANGUAGE.md "C++ / JUCE"); zsh 5.9 for the measurement scripts

## Context

EVE runs `seq 1 100000000` in 1:03.98 (17.62 s user, 45.95 s system), endless in 59.609 s (19.31 s user, 39.38 s system) (`comparison.md`). The wall time is `seq`'s own CPU at 99%; the gap is `seq`'s system time. Scout's zones sit on EVE's reader only and cannot see `seq`'s kernel cost; three zones per ~10-byte read distort the run (`live-report.md:62`, 5.92M dropped). The parity number and the EVE↔endless difference have never been measured under the same conditions, with the same instrument, without zones.

ARCHITECT, verbatim:
- *"how are you going to profile accurately so you can pinpoint precisely and find the most deterministic solution"*
- *"why endless can stream 100M with the same chunk size sub 1m??"* (both read 64 KiB: EVE `EVETerminalProcessor.cpp:165`, endless `TTY.h:285`)
- *"full end-to-end audit fold into plan"*
- END binary: *"Applications/END.app be careful. this is the terminal i have been using now for this session"*
- History: *"fixed names + git"*

## Language / Framework Constraints

- CODING.md CRITICAL RULES and Code Hygiene hold for any fix in Step 5 (no comments before the audit, doxygen last).
- CAROL Profiling: scout for zone timings; CAROL Debugging / trace skill: whatdbg `sample.lua` for the Where lane. A sample run never overlaps a timing run (`~/.carol/skills/trace/SKILL.md:128-129`).
- CAROL Destructive-Edit Discipline for any backup/restore of END state files.

## Dependency & API Inventory

- **zsh `time`** (zsh 5.9): `TIMEFMT` escapes `%U %S %E %P %J` and `%w` voluntary / `%c` involuntary context switches (`man zshparam`, TIMEFMT). Default line matches `comparison.md` raw lines.
- **EVE shell path (Release, no profiler):** `$SHELL` (`EVETerminalProcessor.cpp:219`) through `/usr/bin/login` (`jam_TerminalTTY_posix.cpp:102-127`); zsh integration sets `EVE_ORIG_ZDOTDIR` from the parent `ZDOTDIR` (`ShellIntegration.cpp:46-51`) and restores it (`Source/shell/zsh_zshenv.zsh:6-8`).
- **END shell path:** `shell.program = "zsh"`, `integration = "true"`, `daemon = "false"` (`~/.config/end/end.lua:59`, `:349`, `:361`) — single-process mode, the window process owns its PTYs (`endless/Source/Main.cpp:188-198`). More than one instance is allowed (`endless/Source/Main.h:58`). `END_ORIG_ZDOTDIR` restore (`~/.config/end/zsh/.zshenv:6-8`). END state files: `~/.config/end/window.state`, `~/.config/end/cache/END.cache`.
- **Where lane:** `nvim --headless -l ~/.claude/skills/trace/sample.lua <spec>`; launch mode takes `program` and `cwd` only (`SKILL.md:213`); output `INCLUSIVE`/`LEAF` shares per thread (`:184-196`). EVE's clock queue has no label (`jam_AsyncUpdater.cpp:14`), so the reader is found by its frames (`EVETerminalProcessor::process`, `jam::TerminalProcessor::process`).
- **Scout** (`benchmark/*.md`) for EVE zone timings after fixes; reports keep fixed names, history in git.

## Validation Gate
Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

## Steps

### Step 1: Full end-to-end data-path audit (read-only)
**Scope:** the chain PTY → paint. JAM: `jam_terminal/tty/*`, `processor/*`, `buffers/*`, `document/*` (`AnsiDocument`, `TerminalSwap`), `jam_core` text (`AttributedString`, `AttributedChar`), `jam_gui/widgets/jam_TextEditor.*`, `jam_graphics/fonts/jam_GlyphArrangement.*`; EVE: `EVETerminalProcessor.*`, `EVEProcessor.*`, `EVEViewComponents.cpp`.
**Action:** COUNSELOR writes the plan file to the EVE root as `PLAN-seq-profiling.md`. @Auditor sweeps the chain, every hop from `read` to `paint`, against MANIFESTO E *Complete at Creation* (the four transit rules, the closed list of permitted copies, "Do not build it again"), S (shadow state, second dirty flag), B (thread binding, deadline-thread rule: no allocate/lock/wait on the tick), and CODING CRITICAL RULES. Known candidates to confirm or refute: lane rebuilds per vblank (`jam_TextEditor.cpp:822-862`), `pendingInput` staging (`EVETerminalProcessor.cpp:123-139`), `publishedFrameId` beside `frameId` (`jam_TerminalBuffer.h:320`, `:328`). Each finding: hop, file:line, clause, and whether it runs per read pass, per tick, per vblank or per line.
**Validation:** COUNSELOR filters each finding (read the line, name the clause); accepted findings become Step 5 items; rejected ones carry the refuting read.

### Step 2: Release build without the profiler
**Scope:** `eve/Builds/Profile`.
**Action:** @Engineer runs `cmake -S . -B Builds/Profile -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF -DCAST_PROFILE=OFF` and `ninja -C Builds/Profile`. No other build or scout run touches `Builds/Profile` until Step 4 ends.
**Validation:** exit 0, 0 warnings; `JAM_USING_PROFILER` absent from `Builds/Profile/compile_commands.json` (`CMakeLists.txt:341`).

### Step 3: Same-condition baseline — Verdict lane
**Scope:** scratch folders `scratchpad/seq-eve/`, `scratchpad/seq-end/` (each a `.zshrc`); no source.
**Action:**
1. COUNSELOR writes each `.zshrc`: `TIMEFMT='%J  %U user %S system %P cpu %*E total %w waits %c switches'`, `{ time seq 1 100000000 } 2>> <folder>/result.txt`, `print done >> <folder>/result.txt`.
2. END safety (before the first END run): record the PIDs of every running `/Applications/END.app` process (ARCHITECT's session); @Engineer copies `~/.config/end/window.state` and `~/.config/end/cache/END.cache` to the scratchpad.
3. @Pathfinder runs 5 rounds, each: END then EVE. Each run launches the executable directly with `ZDOTDIR=<folder>` (`/Applications/END.app/Contents/MacOS/END`, `eve/Builds/Profile/.../EVE`), waits for the next `done` line (timeout 300 s), then kills only the launched PID. Nothing else runs on the machine meanwhile.
4. After the rounds: ARCHITECT's END PIDs are alive and unchanged; @Engineer compares the two END state files with the backups and restores them on a difference (Destructive-Edit Discipline: dry-run diff first, restore, verify, delete backup).
**Validation:** 10 result lines (5 per terminal); medians and min/max of total, user, system, waits, switches per terminal; ARCHITECT's END untouched.

### Step 4: Where lane — reader profile, both terminals
**Scope:** two sample specs in the scratchpad; no source.
**Action:** COUNSELOR writes one `sample.lua` launch spec per terminal (`threads all`, `delayMs 3000`, `intervalMs 100`, `maxSamples 300`, `stackDepth 24`, `topRows 20`, `pauseTimeoutMs 8000`, `timeoutMs 120000`). @Pathfinder runs each once with `ZDOTDIR=<folder>` in its environment, so `seq` runs inside the sampled terminal; the result file proves it. Same END safety as Step 3. Not concurrent with any timing run.
**Validation:** `END outcome=limit|exited`; the reader thread is identified by its frames; COUNSELOR reports the reader's shares: `read`, decode, per-pass publish (`swap.setLiveLines`, `terminalBuffer.process`), and wait, for EVE; the matching frames for END (`TTY::run`, `UnixTTY::read`, `Parser::process`, `Processor::process`).

### Step 5: Resolve the accepted audit findings
**Scope:** the files named by the accepted Step 1 findings.
**Action:** @Engineer implements each accepted finding with its clause and exact change, one dispatch per independent group; any new name goes to ARCHITECT (NAMES Rule -1). Then build the profile build through scout `control.md` (exit 0, no new crash report).
**Validation:** COUNSELOR reads every change against the finding, the clause and CODING; `control` exits 0.

### Step 6: Re-measure after the fixes
**Scope:** as Steps 2–4.
**Action:** rebuild Release without the profiler (Step 2 commands); repeat Step 3 with 5 EVE runs and 2 END runs (drift control); run `scout benchmark/live.md` and `replay.md` for EVE's zone timings (profile build, rebuilt by the manifests).
**Validation:** each change counts only when the EVE median moves outside the Step 3 min/max spread and END stays inside its own.

### Step 7: Record and pinpoint
**Scope:** `eve/comparison.md`, `eve/PLAN-seq-profiling.md` (findings section).
**Action:** `comparison.md` gets a "Same-condition runs 2026-10-29" section: method, both terminals, all five metrics, raw lines. The plan gets the findings: the measured difference between the two readers (Step 4 shares), the `seq` cost per terminal (Step 3), the audit fixes and their measured effect (Step 6), and the next mechanism to change, each with its citation. A change to how EVE reads the PTY is a new plan for ARCHITECT.
**Validation:** every number traces to a result line, a sample line or a report line.

### Step 8: Audit, doxygen, endpoint
**Scope:** files changed in Step 5.
**Action:** @Auditor sweeps once; COUNSELOR filters; doxygen pass on changed headers (0 warnings). The sprint log follows only on ARCHITECT's word.
**Validation:** every accepted finding resolved.

## Step 1 Findings — COUNSELOR Filter (2026-10-29)

The Auditor returned 31 findings (F1–F31). COUNSELOR read each cited line for every accepted finding and named the clause. A rejected finding carries the read or the missing clause.

### Accepted → Step 5

- **F1 — ring overflow and counter drift.**
  - Read: the gate checks `getNumFreeRows() >= height` once per token (`jam_TerminalProcessor.cpp:86`). Each wrap of a text token scrolls (`:186-190` → `addLine` `:208-211` → `TerminalBuffer::scrollUp` `jam_TerminalBuffer.cpp:227-230` → `write (1)`). `write` adds the requested count, not the written count (`jam_TerminalBuffer.cpp:100-102`).
  - Clauses: S SSOT, *"int currentIndex; // duplicates what the container owns"*. MANIFESTO D, *"establishing the invariant once, at the owner"*: the precondition at `jam_TerminalBuffer.cpp:98` does not hold for a text token that wraps past the free rows.
  - Fix:
    1. `write` adds the written count, `writer.blockSize1 + writer.blockSize2`.
    2. The gate at `:86` demands `height + ceil (token.length() / width)` free rows. A text token of L bytes takes at most L columns, so it adds at most `ceil (L / width)` rows.
    3. The owner sizes the ring for that demand: `EVETerminalProcessor.cpp:240` passes `blocksPerVblank * blockSize + winsize.rows` transit rows. So a drained ring always admits the largest token.
- **F3 — `pendingInput` staging.**
  - Read: the event appends to `pendingInput` (`EVETerminalProcessor.cpp:123`), then copies into the lane (`:132`), then memmoves the rest (`:138`).
  - Clause: MANIFESTO E, *"No temporary container — do not stage the data"*.
  - The only senders are key and mouse encodings (`EVEViewComponents.cpp:44`, `:66`). The lane holds 65536 bytes (`EVETerminalProcessor.cpp:194`).
  - Fix: delete `pendingInput` and `writePendingInput`. The event writes into `input` with the precondition asserted, in the MANIFESTO pattern `jassert (x); if (x) { … }`. The getter loses its `writePendingInput()` call.
- **F5 (live part) and F20 — a second mapping of the live file, and pessimistic checks.**
  - Read: `addLiveLines` maps the live file again (`jam_TerminalSwap.cpp:247`), although the swap owns `live` (`jam_TerminalSwap.h:140`). The constructor asserts the mapping (`jam_TerminalSwap.cpp:23`).
  - Read: `prepare` resets `liveFifo` (`:47`), and `reset` advances the frame id (`jam_TerminalProcessor.cpp:20`). So a ready slot always holds a written frame.
  - Clauses: MANIFESTO E, *"No allocation — the data exists, so build nothing"*. MANIFESTO D, pessimistic programming.
  - Fix: `addLiveLines` reads the slot from `live`, and the `getData()` and `frame != 0` checks go. `addLines` mapping the history files per lane build is lane creation. It is rejected (*"Creation can use … memory allocation"*).
- **F18 — `mutable liveFifo`.**
  - Read: `jam_TerminalSwap.h:141`. A `const` reader consumes the slot (`:95`).
  - Clause: MANIFESTO E, Encapsulation.
  - Fix: delete `mutable`. `addLiveLines` becomes non-const. The getter (`EVETerminalProcessor.cpp:100`) and the TextEditor binding (`jam_TextEditor.cpp:64`, `:798`, `:815`, and the matching header lines) take `TerminalSwap*` / `TerminalSwap&`.
- **F29 — structured bindings with `=`.**
  - Read: `jam_TextEditor.cpp:726, 738, 762, 919, 927, 1165, 1190`.
  - Clause: CODING, *"Aggregate (brace) initialization is ALWAYS preferred"*.
  - Fix: brace initialization.

### Accepted → Step 5, names from the established family (NAMES Rule -1, Rule 5)

Each name comes from the nearest family by a read: the class's own member set first, then the module, then the framework. ARCHITECT approves no name here. A piece that no family name fits means the split is wrong. The split is corrected, not the name ratified (`NAMES.md:21-29`).

- F22 `TextEditor`, F23 `TerminalBuffer`, F25 `AnsiDocument.cpp`: split by responsibility (Lean ladder Q5). No MANIFESTO §L, LANGUAGE.md or CODING.md exception quote exists for them.
- F24 `writeFromTerminalBuffer` (63 lines, `jam_TerminalSwap.cpp:92-155`) has three jobs: append the rows, roll the files over, delete the old files.
- F28: `TerminalBuffer::process()` only does `++frameId` (`jam_TerminalBuffer.cpp:275-278`), and `addVisibleContent` / `addLanes` break the Verb Contract.

### Pending reads, resolved

- **F11 — accepted.**
  - Read: `TerminalSwap::addLine` stores a decoded `ImagePixelData` in the lane document (`jam_TerminalSwap.cpp:188`). The only reader, `TextEditor::getImage`, wraps it in a `juce::Image` (`jam_TextEditor.cpp:1172-1177`).
  - Clause: MANIFESTO E, *"The Model must not contain a concrete object"*. Intent, per the editor's doc: *"Layout decodes the payload once for each image"* (`jam_TextEditor.h:88`).
  - Fix: the swap stores the PNG bytes as value data (`juce::var` holding a `juce::MemoryBlock`). `getImage` materialises the image from them.
- **F17 — rejected.** The only caller chain is `TerminalGraphicsContext::drawImage` → `addImagePlacement` (`jam_TerminalGraphicsContext.cpp:488-499`, `:523-526`). No source constructs a `TerminalGraphicsContext`, and `TerminalProcessor::process` does not reach the chain. No deadline thread runs it.
- **F24 — split by job; names from the family.**
  - `writeLine` joins the `write` family (`writeFromTerminalBuffer`, `TerminalBuffer::writeRow`) on the swap's unit, the line (`addLine`, `getLines`).
  - `setLines` is the `set` of the range that `getLines` returns.
  - `removeHeadFiles` follows `TextEditor::removeHeadLanes` (`jam_TextEditor.h:316`). That function erases the lanes whose block holds no line; this one erases the files whose group holds no readable line.
- **F28:**
  - `addVisibleContent` adds nothing, which breaks the Verb Contract for `add`. Correction: delete it. Its two callers (`jam_TextEditor.cpp:808`, `:878`) call `setContentSize()` and `content.repaint()` themselves. No new name.
  - `TerminalBuffer::process()` is rejected. It is in the chain's `process` family (`EVETerminalProcessor::process`, `TerminalProcessor::process` → `terminalBuffer.process()` at `jam_TerminalProcessor.cpp:49`), and Rule 5 nearest-sibling precedence applies.
  - `addLanes()` is rejected. It matches its overload `addLanes (TerminalSwap&)`, which inserts into `history` (Rule 5).
- **F25 — rejected.** Lexing plus tree building is the framework's Document contract. The base declares the lexer, `virtual int getToken (Cursor& cursor, int segmentType) = 0;` (`jam_Document.h:1203`), and `Document::parse()` drives it (`jam_Document.cpp:661`). Every Document type keeps its lexer: MANIFESTO, *"A new pattern where one already exists is always wrong"*.
- **F22, F23 — accepted. The split crosses the Step 5 scope, so ARCHITECT must rule.** LANGUAGE.md gives no size exception: *"If the object has more than one responsibility, decompose it into objects. Each new object is again one header and one `.cpp`."* (`LANGUAGE.md:35`).
  - F23: the VT encoder and the cell lookup of `TerminalBuffer` have callers outside the sprint artifacts: `cast/Source/Help.cpp:10`, `:32`, `cast/Source/main.cpp:1469`, `scout/Source/Help.cpp:10`, `:32`, `scout/Source/Main.cpp:272`. The family name for the encoder is the Writer family (`MarkdownWriter`, `MermaidWriter`, `OpenTypeWriter`, `PdfWriter`).
  - F22: no other project uses `jam::TextEditor` (the source search of the other projects finds only `juce::TextEditor`). The split adds new object files and edits the `jam_gui` module header and unity source, which are outside "the files named by the accepted Step 1 findings".

### Rejected

- F2: no read shows a lower layer that knows a higher one. The view calls the getter (`EVETerminalProcessor.cpp:100-116`). SPEC 2.3 and ARCHITECT's ruling, *"vblank at all build contract are exactly the same"*, set the writer post.
- F4: a full lane replacement is the state update that MANIFESTO prescribes, *"the owner replaces the full concrete object"*. Building in steps is the forbidden form.
- F6: building the lane document is creation (*"Creation can use … memory allocation"*).
- F7: no read or measurement shows a blocking call. The finding is low confidence.
- F8, F12, F26, F30, F31: no CONTRACT clause is named.
- F9: `publishedFrameId` is private and gates the buffer's own write only (`jam_TerminalBuffer.h:320-328`, `:454`). No caller tracks it.
- F10: the processor writes the ring to the swap before each buffer prepare (`EVETerminalProcessor.cpp:235-236` before `:240`). F1 removes the other drift source.
- F13: the out-parameter is the framework's Getters type (`jam_ViewManager.h:42`, `Function::Map<juce::Identifier, void>`). MANIFESTO, *"A new pattern where one already exists is always wrong"*.
- F14: the TTY owner keeps one child (`jam_TerminalTTY_posix.cpp:237-239`). The shell strings are transient creation inputs (MANIFESTO S).
- F15: `pendingInput` is F3. The quit listener exists only in profile standalone builds (`EVETerminalProcessor.cpp:205-215`).
- F16: the decoder passes `pen` to the document's SGR parser (`jam_TerminalProcessor.cpp:513`, `AnsiDocument::applySgrParameters`). That is the framework API.
- F19: the editor asks before it allocates its own lane document (`jam_TextEditor.cpp:822-826`). It does no work on behalf of the swap.
- F21: the CODING clause names naked pointer members. A captured reference in a `std::function` is not named.
- F27: the store is a visible parameter of `TerminalProcessor::process` (`EVETerminalProcessor.cpp:255`), through the framework singleton.

## Step 7 Findings — Measured (2026-10-29)

Raw lines: `comparison.md`, section "Same-condition runs 2026-10-29".

### `seq` cost per terminal (Steps 3 and 6, medians)

- END: 1:07.56 total, 19.90 s user, 40.54 s system, 90% cpu, 267,398 waits.
- EVE before the fixes: 1:04.08 total, 17.48 s user, 46.01 s system, 99% cpu, 17 waits.
- EVE after the fixes: 1:04.05 total, 17.48 s user, 46.13 s system, 99% cpu, 11 waits.
- EVE is 3.5 s faster than the installed END. `seq` spends 2.4 s less user time in EVE, and 5.5 s more system time.
- In EVE, `seq` almost never blocks: 11 waits. In END, `seq` blocks about 260,000 times.

### The audit fixes and their measured effect (Step 6)

- The EVE Step 6 median (1:04.05) is inside the Step 3 spread (1:03.98–1:04.89). Under the Step 6 rule, no change counts.
- The END drift runs (1:07.26, 1:07.05) are inside the END Step 3 spread (1:06.64–1:08.73). The machine did not drift.
- Thus the Step 5 fixes (F1, F3, F5, F11, F18, F20, F24, F28, F29) correct CONTRACT violations. They do not move the `seq` time.

### Reader shares (Step 4)

- EVE (`scratchpad/seq-eve/sample.txt`, 101 samples):
  - The reader GCD threads are in `__workq_kernreturn` (idle) in 74–92% of samples.
  - `read` is 2–12% of samples.
  - The main thread is in `~VulkanLowLevelGraphicsContext` (`__psynch_cvwait`) in 36.6% of samples.
- END: not run. The attach failed: `DRIVER ERROR driver.lua:130: attach failed`. This is the risk named in "Risks / Open Questions".

### EVE zone timings (Step 6, scout, profile build)

`benchmark/live-report.md` (29-10-2026 06:40, 11 epochs, medians per epoch):

- `epoch` 65.75 s. `user` 93.14 s. `system` 85.70 s. These rows are the whole profile run, `seq` and EVE together.
- `terminalRead` 34.34 s over 87,248,547 calls.
- `terminalProcess` 56.17 s over 87,248,451 calls.
- `swapLiveLines` 0.61 s over 87,248,513 calls.
- `deviceClock` 57.52 s over 8,537,114 calls. `ttyBytesReady` 57.69 s over 8,537,113 calls.
- `swapHistoryLines` 22.72 s over 8,127 calls. `editorLanes` 32.68 s over 8,402 calls.
- `dropped` 3,374,568 zone samples. The zones distort this run. The Verdict lane (zsh `time`) is the number.

`benchmark/replay-report.md` (29-10-2026 06:42, 11 epochs, medians per epoch):

- `epoch` 1.84 s. `user` 1.03 s. `system` 1.15 s.
- `terminalProcess` 146.4 ms over 175 calls. `swapHistoryLines` 230.8 ms over 33 calls. `editorLanes` 64.2 ms over 39 calls.
- `terminalRead` 0.25 ms over 129 calls. `swapLiveLines` 0.21 ms over 175 calls. `dropped` 0.

### The difference between the two readers, and the next mechanism

1. `seq` writes one number per `write` call. Its stdout is a tty, so stdio line-buffers it (`setbuf.3:116-119`).
2. The output of `seq 1 100000000` is 888,888,898 bytes, or 988,888,898 bytes when the tty adds a CR to each line.
3. The live run gives 87,248,547 `terminalRead` calls. That is 10.2–11.3 bytes per read: one `seq` line per `read`.
4. The live run gives 8,537,114 clocks. That is about 10 reads for each wake of the reader (`EVETerminalProcessor.cpp:246`, the loop `while (terminalProcessor.process (...))`).
5. So the EVE reader keeps step with `seq`: each line goes through one `read`, and the PTY never fills (11 waits).
6. END lets the PTY fill: `seq` blocks about 260,000 times. The END reader shares are not measured (Step 4, END not run).
7. The next mechanism to change is the EVE read cadence against the PTY: how many bytes one `read` takes. Per Step 7, a change to how EVE reads the PTY is a new plan for ARCHITECT.

## BLESSED Alignment
- **D:** one instrument per question, the same conditions for both terminals, five runs per side, a change counted only outside the measured spread.
- **E (Encapsulation):** the measurement goes through each program's own shell path (`ZDOTDIR`), with no change to either program.
- **B:** the launched END is a separate process; ARCHITECT's END PIDs and state files are checked before and after.
- Step 5 fixes follow the CONTRACT clauses of their findings.

## Risks / Open Questions
- `/Applications/END.app` (built 26 Apr) may differ from the endless source read here (`Main.h:58`, `Session.cpp:93-98`). The result file proves the `ZDOTDIR` path per run; if no `done` line arrives in 300 s, the run stops and COUNSELOR reports.
- whatdbg launch takes no environment (`SKILL.md:213`). Whether the debuggee inherits the parent environment is checked by the result file in Step 4; if it does not, the Where lane for that terminal is reported as not run.
- A Release END may lack symbols; then its sample frames carry addresses only, and the END Where lane is reported as such.
- Each sample pauses the target, so Where-lane shares are shares of samples, never timings (`SKILL.md:128`).
