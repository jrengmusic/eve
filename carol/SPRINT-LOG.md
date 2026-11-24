# SPRINT-LOG.md

**Project:** EVE — Embeddable Viewport Endpoint
**Repository:** /Users/jreng/Documents/Poems/dev/eve
**Started:** 2026-08-30

**Purpose:** Long-term context memory across sessions. Tracks completed work, debts paid, and debts deferred to the inter-sprint ledger (`DEBT.md`). Written by PRIMARY agents only when ARCHITECT explicitly requests.

---

## ⚠️ CRITICAL RULES

**AGENTS BUILD CODE FOR ARCHITECT TO TEST**
- Agents build/modify code ONLY when ARCHITECT explicitly requests
- ARCHITECT tests and provides feedback
- Agents wait for ARCHITECT approval before proceeding

**AGENTS NEVER RUN GIT COMMANDS**
- Write code changes without running git commands
- Agent runs git ONLY when user explicitly requests
- **When committing:** Always stage ALL changes with `git add -A` before commit

**SPRINT-LOG WRITTEN BY PRIMARY AGENTS ONLY**
- **COUNSELOR** writes to SPRINT-LOG
- Only when user explicitly says: `"log sprint"`
- No intermediate summary files
- Latest sprint at top, keep last 5 entries

**⚠️ NEVER REMOVE THESE RULES**

---

<!-- SPRINT HISTORY STARTS BELOW -->
<!-- Latest sprint at top, oldest at bottom -->
<!-- Keep last 5 sprints, rotate older to git history -->

## SPRINT HISTORY

## Sprint: Rendering and live rows — colour emoji on Windows, cast output in UTF-8, backgrounds from the cells, the live block waits only for the closed line of OSC 133 D ✅

**Date:** 2026-11-24
**Duration:** 03:55 (05:51 to 09:46 +0700)
**Plan:** `PLAN-rendering-and-live-rows.md` (Steps 0 to 8)
**Scope (ARCHITECT):** five defects. **[1]** Emoji render monochrome on Windows. **[2]** A `cast` build prints gibberish on Windows. **[3]** The Claude Code plan dialog is stuck when it is higher than the viewport. **[4]** Block backgrounds move on scroll, with holes and trails. *"either quoted user prompt, or diff (red/green) should not stop at text length. but always full winsize width"*. **[5]** Claude Code `/tui classic` goes blank. Then *"END is in scope"*, *"NO AUDIT, until runtime proven fixed"*, `/audit`, `/log`.

### Agents Participated
- COUNSELOR — the root-cause reads, the plan, the read of each Engineer change, the filter of the audit findings, the check of the doxygen pass and its text corrections, this log.
- Pathfinder — inventories, git evidence, the doxygen pass.
- Engineer — the code of the plan steps, the runtime fixes and the audit fixes.
- Auditor (opus) — one run: 9 findings.

### Files Modified (38 total)

jam (31):
- `jam_core/text/jam_AttributedString.cpp:353` — `getCodepoint` gives a space for a `contentCodepoint` atom with codepoint 0 that is not a spacer tail. U+0000 no longer reaches the line break rules.
- `jam_core/text/jam_AttributedChar.{h,cpp}` (`.cpp:3436`) — new `hasBackground`.
- `jam_graphics/fonts/jam_GlyphArrangement.{h,cpp}` — new `getBackground`, `addBackgrounds` (`.cpp:586-654`), `setEntryAdvances` (`.cpp:131`). `getDrawRuns` builds the background runs before the glyph runs and takes `right`. `addGlyph` and `Run::matches` hold no background colour. `addBackground` is deleted. The glyphs of one multi-glyph cluster share the advances of its cells.
- `jam_graphics/fonts/jam_GlyphRasterizer.{h,cpp}` (`.cpp:383-414`, `:495-517`) — new `getImageLayer`: COLR layers become one ARGB image layer. `rasterizeEdgeTable` sends it through the one emoji path.
- `jam_graphics/jam_graphics.cpp:1`, `:7` — `#define JUCE_GRAPHICS_INCLUDE_RENDERING_HELPERS 1`, which `EdgeTableFillers` needs.
- `jam_graphics/fonts/jam_TerminalGlyph.h:341`, `:353` — the up and left arms of `drawLines` reach the corner pixel.
- `jam_style/jam_StyleCustom.{h,cpp}`, `jam_terminal/style/jam_StyleTerminal.{h,cpp}` — new virtual `setRowSpans` with an empty base. The terminal style makes the rows cover every cell.
- `jam_gui/widgets/jam_TextBlock.{h,cpp}` — `setTextRows` calls `setRowSpans`. `addRuns` takes the right edge and gives it to the rows of a line with `Id::isFilled`.
- `jam_gui/widgets/jam_TextEditor.{h,cpp}` (`.cpp:1626-1627`) — `hasLive` waits only for `Id::closedLine`.
- `jam_terminal/buffers/jam_TerminalBuffer.{h,cpp}` — per-slot `rowWidths` and `continuedRows`, `endClosedRow`, `isFilled`, `isContinued`. `writeRow` and `getNumBytes` carry `isFilled`. `prepare` gives the new width to a kept row that is not full.
- `jam_terminal/document/jam_AnsiDocument.{h,cpp}` — `addSegments` takes the tokens from `ReflowDocument::getCellSegments`, with the first segment at cell 0. New `isFilePath`, used by `addRangeLink`. `addWrappedLine` takes `Id::isFilled` of its last row.
- `jam_terminal/document/jam_TerminalHistory.{h,cpp}` — reads `isFilled`. The live root holds `Id::closedLine`.
- `jam_terminal/document/jam_TerminalSwap.{h,cpp}` (`.cpp:158`, `:168`) — a row that is not continued closes its line. A row is a wrapped row only when the line before it is not closed.
- `jam_markdown_graphics/graphics/jam_MarkdownGraphics.cpp:474`, `jam_mermaid_diagram/diagram/jam_MermaidGraphics.cpp:774` — pass `0.0f` for `right`.
- `cast/terminal.md:70`, `generated/jam_Terminal.{h,cpp}` — the row `isFilled`; the generated files come from `cast`.
- Doxygen of the headers above.

eve (5):
- `cast/cmake.cast:634`, `cygwinPtyBundle.cmake:18` — `ENCODING UTF-8` on the `cast` call. The second file is generated.
- `test/glyphs.sh` — `get_glyph` writes UTF-8 bytes, because zsh on cygwin cuts `\U` to 16 bits. `print_codepoints` counts columns.
- `Source/windows/eve-cygwinPTY v0.0.1 Windows.zip` — build output.
- `PLAN-rendering-and-live-rows.md` — new, the locked plan.

end (2):
- `cast/cmake.cast:564`, `pluginBundle.cmake:19` — `ENCODING UTF-8` on the `cast` call. The second file is generated.

`end/.project` is also modified in the tree (+337 lines). No plan step names it.

### Alignment Check
- [x] BLESSED principles followed — each new row value has one owner, `TerminalBuffer`, and the lifetime of its slot. Text, background, caret and selection read the same row spans. One emoji path.
- [x] NAMES.md adhered — every new name joins a family of the plan's inventory, or a sibling predicate.
- [x] MANIFESTO.md principles applied — no new struct, no new atomic, no allocation on the clock. The frame tells the reader one number.
- [ ] Not built after the audit fixes and the doxygen pass.
- [ ] The doxygen index is not regenerated (`<leader>bd` in jam).

### Problems Solved
- **[1] Monochrome emoji.** `rasterizeEdgeTable` accepted only an image layer. Segoe UI Emoji gives COLR `ColourLayer` items. ARCHITECT's screenshots show colour.
- **[2] Gibberish.** `cmake -P` decoded the child output as the console code page. `ENCODING UTF-8` keeps the bytes. ARCHITECT: *"cast output clean"*.
- **[3] [5] Hidden live block.** The live frame named `numHistoryRows`, but history reads only closed lines and the last stored line stays open. One scrolled row hid the live block until OSC 133 D. The frame now names the closed line of D. Not proven in a real Claude Code session.
- **[4] Backgrounds.** A background existed only for a glyph, and an erased cell gives no glyph. Each run painted its backgrounds just before its own glyphs. The backgrounds now come from the cells and paint first. The rows cover every cell. A background that reached the edge fills the content width. ARCHITECT: *"no issue with bg colour so far"*.
- **Emoji clusters.** Segoe UI Emoji shapes the family ZWJ sequence to 3 glyphs and a flag to 2. Each glyph took its own advance. The glyphs now share the cells of the cluster (`SPEC.md:833-834`).
- **Assert in END inside EVE.** A path with leading separators and no host reached `juce::File` (`juce_File.cpp:162`). `isFilePath` gives no file link for it on Windows. ARCHITECT: *"1. clean"*.
- **Audit (9).** Eight fixed, one fixed in part. The `C:/../x` test of finding 5 is rejected: it removes the links of `D:/a/../b`, and `//../x` still asserts.
- **Mistakes owned.** The doxygen run had too many symbols and ran to the 15-minute watchdog. Four of its texts did not agree with the code; they are corrected.

### State for Continuation
- Build EVE and END first. Verify the diff and the quoted prompt on scroll and resize, a long wrapped line in history, the `printf` emoji line, and `test/glyphs.sh`.
- **[3] [5]** Verify a real Claude Code plan dialog that is higher than the window, in `/tui classic` and in fullscreen. The replay of `raw.bin` does not prove it (ARCHITECT: *"i dont think it relevant to actual session"*).
- **[4]** Verify the history backgrounds after a resize on macOS.
- Known limit, from ruling M2 (*"Writer publishes closed end"*): while a wrapped line or a grid is open at the scroll edge, its rows show in neither history nor live until it closes. Read `jam_TerminalSwap.cpp:158-169`.
- Known limit: the text `C:/../x` still reaches the Debug assert of `juce::File`. Read `isFilePath` in `jam_AnsiDocument.cpp`.
- The doxygen block of `drawLines` (`jam_TerminalGlyph.h:330`) was the last item of the stopped pass. `TextBlock::addGraphics`, `setTextRows` and `addRow` have no block (`jam_TextBlock.h:56`, `:82`, `:109`).
- The END commit of this sprint is `c79470b` (2026-11-24 09:49 +0700). Read `git status` for jam and eve.
- The plan did not put the `DEBT.md` entries in its scope (CAROL.md Documentation). COUNSELOR's first form of this entry listed no debt as paid. ARCHITECT corrected it.

### Debts Paid
- `DEBT-20261117T223545` — the prompt no longer prints before `seq` finishes. ARCHITECT: *"no issue with active prompt race with seq anymore, that have been paid."* (END Sprint 110). This sprint rewrote that gate. The live block waits for the row that D closed (`jam_TerminalBuffer.cpp:364-365`, `jam_TextEditor.cpp:1625-1627`), and D scrolls every row above the cursor into history (`jam_TerminalProcessor.cpp:1224-1227`).
- `DEBT-20261115T221459` — END load, close, focus and keyboard on Windows. ARCHITECT: *"focused issues fixed"* (END Sprint 110) and *"the only debt untested is image"*. No scout run after 2026-11-13 measured load or close.

### Debts Deferred
- `DEBT-20261105T000000` — image preview, untested on Windows. ARCHITECT: *"the only debt untested is image, that's also untested on latest build on mac"*.

---

## Sprint: macOS launch abort — a line with no Id::heights uses the cell height ✅

**Date:** 2026-11-19
**Duration:** One session. The start and end times are not recorded.
**Scope (ARCHITECT):** The macOS crash stack (`std::terminate` from `juce::MessageQueue::runLoopSourceCallback`), *"regression with windows eve"*. Then *"probably related to Document changes at jam?"*. Then `/go`.

### Agents Participated
- COUNSELOR — the trace spec, the root cause from the jam diffs, the fix specification, the read of the Engineer change, this log.
- Pathfinder — the crash report, log and git evidence; the whatdbg crash trace of the Release Standalone.
- Engineer — the change in `jam_TextBlock.cpp`.

### Files Modified (1 total)
- `jam/jam_gui/widgets/jam_TextBlock.cpp:243-251` — `TextBlock::addRowHeights` reads `Id::heights` only when `line.contains (Id::heights)`. Otherwise each row takes the cell height through `juce::jmax (cellHeight, height)`. This is the contract at `jam_TextBlock.h:114`: *"A line with no `Id::heights` uses the cell height for each row."*

### Alignment Check
- [x] BLESSED principles followed — positive nesting, no bail-out guard. The fix adds no per-cell copy of the cell height to the terminal lines, because that copy would be shadow state.
- [x] NAMES.md adhered — no new name. `contains` and `get` are the owner API of `Document::Element`.
- [x] MANIFESTO.md principles applied — the data is read where it lives; nothing is copied.

### Problems Solved
- **EVE aborts at launch on macOS.** The whatdbg stderr shows `libc++abi: terminating due to uncaught exception of type std::out_of_range: Element::get: absent key heights`. The throw is at `jam_Document.h:297`.
- **Why the stack ends at `runLoopSourceCallback`.** That function is `noexcept` (`JUCE/modules/juce_events/native/juce_MessageQueue_mac.h:110`). Any throw from a message callback calls `std::terminate` there, and the frames that threw are lost.
- **Root cause.** The staged jam `Document` change deleted `Token::find` and `Element::find`, which returned `nullptr` for an absent key. `addRowHeights` moved to the throwing `get` and lost its `heights != nullptr` branch. The terminal lines have no `Id::heights` (`jam_AnsiDocument.cpp:220-222`, `:526-534`). `StyleCustom::setLineAdvances` fills only `Id::advances` (`jam_StyleCustom.cpp:126-137`). The eve Windows commits did not cause the abort.
- **The other migrated `find` sites.** The staged diff has eight more. Each one keeps its absent-key behaviour: a `get` with a default, `contains` then `get`, or a `gridStart` dereference that had no null test before either.

### State for Continuation
- Not built. Not tested by ARCHITECT. Rebuild EVE (`cast cast/spell.md`), then launch the Standalone on macOS.
- The jam working tree has a large staged and unstaged change set from other work. The fix is one hunk in `jam_TextBlock.cpp`. Read `jam_TextBlock.cpp:234-268` before acting.
- The uncommitted `JsonDocument::get` now throws on an absent key (`jam-uncommitted.diff:147`). Its callers are not checked. That throw is not the cause of this abort, because the abort message names `Element::get`.
- This log keeps more than 5 entries. The rotation did not run.

### Debts Paid
- None

### Debts Deferred
- None

---

## Sprint: Windows cygwin seq without choke — one swap file, closed swap risks, terminal workers told by the Model ✅

**Date:** 2026-11-18 to 2026-11-19
**Duration:** several sessions. The start and end times are not recorded.
**Scope (ARCHITECT):** `DEBT-20261117T223541` (seq on the cygwin pty starts fast, then chokes into pages). Then *"fix this sprint. no open risk should be left before we wrap."* Then *"if Terminal<Processor> is analogous to DSP processor … it should not hold any atomic at all. it should not have getter. it should be TOLD by Model (parameter)"* and *"orchestrator TELL never ask, dumb workers downstream always being TOLD"*. Then *"can we eliminate the cost"* and *"proceed until /log no gate"*.

### Plans executed (all at the project root)
1. `PLAN-cygwin-relay.md` — the helper `eve-cygwinPTY` waits on the runtime `poll`, drains the msys2 pty master, and flushes once per drain.
2. `PLAN-cygwin-bulk-read.md` — after one runtime read, the helper reads the rest of the pty pipe in one `ReadFile` through `_get_osfhandle`.
3. `PLAN-cygwin-capacity.md` — each TTY backend states its system capacity. `prepare()` opens the TTY, then the device with that capacity. The helper rings take EVE's buffer size from the command line.
4. `PLAN-swap-one-file.md` — the choke was a new swap segment file every ~2,350 lines; Defender held each first reopen for 4.0 ms on average. Now one memory-mapped file per `TerminalSwap` (`TerminalSwapFile`, `TerminalRecordRing`). The history drops the oldest line first. `heldLine` is a terminal Model row.
5. `PLAN-swap-risks.md` — seq_cst parameter store; line and image hold order (D1); images on the record ring; bounded extents; the thread of `prepare`.
6. `PLAN-terminal-workers-told.md` — Steps 1-15, with Step 5b and Step 9b added during execution.

### Agents Participated
- COUNSELOR — plans, scout manifests, the read of each Engineer change against its step, validation of the audit findings, the doxygen instructions and their check against the code, this log.
- Engineer — the code of every plan step and every accepted audit fix.
- Pathfinder — inventories, scout runs, whatdbg traces and probes, the doxygen passes.
- Auditor — one sweep per plan after its last step (Opus for the told-workers plan, findings F1-F14).

### Files Modified (told-workers plan; the earlier plans list theirs in their Scope lines)
- `eve/Source/layout/parameters.md:77-90`, `jam/cast/terminal.md` — seven int64 Model rows: `firstLine`, `endLine`, `closedLine`, `lineIndex`, `firstImage`, `endImage`, `imageIndex`.
- `jam/jam_terminal/document/jam_TerminalSwap.h/.cpp` — no atomic and no decision getter. Plain writer values. Boundaries are reported through `events`. Step 9b: `endLine` and `closedLine` are reported once per `writeFromTerminalBuffer` call (`.cpp:89-90`); `firstLine` only when it changes (`.cpp:36`, `:136`, `:219`).
- `jam/jam_terminal/document/jam_TerminalImageRecord.h/.cpp` — the same for the image boundaries; D1 report order in `addEntries`; F2 hold pass-through.
- `jam/jam_terminal/document/jam_TerminalRecordRing.h/.cpp` — `getNumRecords` and `getNumFreeRecords` return `juce::int64`.
- `jam/jam_terminal/document/jam_TerminalHistory.h/.cpp` — the job readers take the row atomics as arguments.
- `jam/jam_terminal/processor/jam_TerminalProcessor.h/.cpp` — `images` and `liveSlots` lanes come in as arguments; the decoder holds `screen` and tells the buffer through `setScreen`; `setTransitImage`; it tells `tty.setCursor` and `tty.setPen` at the end of `process`.
- `jam/jam_terminal/buffers/jam_TerminalBuffer.h/.cpp` — `getScreen` removed.
- `jam/jam_terminal/tty/jam_TerminalTTY.h` — `setCursor`, `setPen` added; `isOpen` removed from the base.
- `jam/jam_terminal/tty/native/jam_TerminalTTY_windows.h/.cpp`, `jam_TerminalConsole_windows.h/.cpp` — the console is told cursor, winsize and pen; it holds them as calculation inputs; no `get…` callbacks.
- `jam/jam_gui/widgets/jam_TextEditor.h/.cpp`, `jam_PreviewComponent.h/.cpp` — readers read the Model rows; `setImageBounds` stores the hold first (F1); `getRow` is `noexcept`.
- `eve/Source/EVETerminalProcessor.h/.cpp` — owns the `images` and `liveSlots` lanes; `tty` exists only while its session is open; `ttyResize` tells the device context; `registerImageEvents` resolves each row once; `copyToClipboard` holds the image; `writePendingData` reads `getRawParameterValue<int64_t>`.
- `eve/Source/EVEView.cpp` — `firstLine` row drives `applyGridEvents`.
- `eve/Source/TerminalMemoryTTY.h/.cpp` — forwards `setCursor` and `setPen`; the destructor closes the session.
- `eve/SPEC.md` — `[AMENDED 2026-11-19]` told-workers item.
- `PLAN-swap-risks.md` — Names table (F5).
- `~/.carol/NAMES.md:122`, `:124` — `set` covers data; `apply` runs an operation whose body is `set` operations. ARCHITECT: *"set is not only value, it's also data. NAMES.md is wrong."* and *"apply is operational … the body of `apply` are `set` operation"*.
- `comparison.md:92-161` — Windows timings.

### Alignment Check
- [x] BLESSED principles followed — the workers hold no cross-thread atomic and no decision getter; each boundary is a Model row; each lane belongs to EVE.
- [x] NAMES.md adhered — `setTransitImage` replaced `setImageStart` on the decoder, because `TerminalImageRecord::setImageStart` already holds that name (Rule 5).
- [x] MANIFESTO.md principles applied.
- [ ] The doxygen XML regeneration did not run.

### Problems Solved
- **Choke on Windows cygwin seq.** Root cause: a new segment file per ~2,350 lines and the Defender hold on each first reopen. One mapped file per swap removed it. ARCHITECT: *"stream prints on TextEditor is continuous without choking."*
- **Measured result (scout, `benchmark/seq-1m-windows-cygwin-one-file-report.md`, 2026-11-19 08:10, 3 epochs).** Epoch median 5.4388647e9 ns (min 5.0568468e9, max 6.9327582e9), baseline 6.876026e9, change −20.9 % (`:10`). Dropped 0 (`:115`). Swap history lines 3.924577e8 ns against the baseline 5.3580712e9, change −92.7 % (`:73`). The run before Step 9b gave 6.801246e8 ns.
- **Interactive runs (ARCHITECT, Release, ROG Ally Z1 Extreme).** `seq 1 1000000` 3.827 s total; Windows Terminal 1:07.03, Rio 1:15.55, Alacritty 1:24.81 (`comparison.md:104-115`).
- **Terminal workers told.** The swap and the image record report boundaries to the Model. Readers load the rows seq_cst after their hold store (D1).
- **Per-line report cost.** The swap reported 3-5 `storeValue` calls per line. Step 9b reports `endLine` and `closedLine` once per write call, and `firstLine` only on change.
- **Mistakes owned.** I applied Step 2 myself (four lines in three files) against the 1-2 line limit, then delegated all later code. I first wrote "the orchestrator is told"; ARCHITECT corrected it.

### State for Continuation
- Not built or tested by ARCHITECT after the told-workers plan. The scout run compiled the Release ProfileSymbols build with exit 0.
- Auditor F11 (a `RecordJob` decodes on exit) and F13 (name list against delegation prompts) are not resolved. Read `jam_PreviewComponent.cpp` `RecordJob` and the told-workers audit before acting.
- Open rulings for ARCHITECT: the `set` or `apply` name of the three in-place reads (`applyPending` keeps `apply` by ARCHITECT's concurrency test); the seven-atomic argument list of `TerminalHistory::addLines`; the decoder `reply` and `buffer` FIFOs and the swap `liveOutputs` and `lines` as lanes; EVE `getValue()` sites against `getRawParameterValue`; the `device` and `tty` member order.
- `jam_ParameterBase.h` store doc blocks still say "relaxed"; the store is seq_cst (`PLAN-swap-risks.md` Step 1).
- `Source/windows/eve-cygwinPTY v0.0.1 Windows.zip` is rebuilt by each scout run.
- Nothing from this sprint is committed.

### Debts Paid
- `DEBT-20261117T223541` — one swap file per instance removed the choke; ARCHITECT confirmed continuous output, and scout shows 0 dropped.

### Debts Deferred
- `DEBT-20261117T223545` — the prompt printed before seq finished. Not re-observed; the entry stays.
- `DEBT-20261115T221459` — END load, close, focus and keyboard on Windows (unchanged).
- `DEBT-20261105T000000` — Windows runtime test of the image preview (unchanged).

---

## Sprint: Cygwin PTY close-out — one copy per boundary, the child environment replaces inherited variables, the tick never reads a TTY that is not open ✅

**Date:** 2026-11-18
**Duration:** part of one session. The start and end times are not recorded.
**Scope (ARCHITECT):** the open items of the sprint "Cygwin PTY" below: *"resolve now"*; the crash stack with *"pre-audit no crash."*; the run results *"debug no crash. seq, ^C, resize works"*, *"cmd, pwsh works"* and *"works."* (job control); and *"write me shell script @test\"*.

### Agents Participated
- COUNSELOR — the reads of the owner APIs; the three fix specs with their exact code; the read of each Engineer change against the brief; the transcript read that located the audit-fix brief; this log.
- Engineer — the three fixes, the two cast builds, and `test/jobcontrol.sh`.
- Pathfinder — git evidence; the doxygen follow-up; a transcript search that I stopped at the 15-minute watchdog because its scope was too wide.

### Files Modified
- `jam/jam_terminal/processor/jam_TerminalFIFO.h:180-208` — `writeFromDevice` is public again, text and body unchanged. A device read lands in the free space of the ring.
- `jam/jam_terminal/display/jam_StdioServer.h:61-79` — new public `writeFromDevice`: it gives the free space of the output ring to a device callback, then flushes to the standard output.
- `eve/cygwinPTY/Source/Session.{h,cpp}` — the `buffer` member is gone. `run()` reads the master through `stdio.writeFromDevice` (`Session.cpp:187-198`). The master byte no longer crosses a staging buffer.
- `jam/jam_terminal/tty/native/jam_TerminalConsole_windows.cpp:130-171` — `buildEnvironmentBlock` builds the overrides (`TERM`, `TERM_PROGRAM`, the caller's variables) in a `juce::StringPairArray`, which ignores the case of keys, and drops each inherited variable with the same name. This is the contract at `jam_TerminalTTY.h:88-89`.
- `eve/Source/EVETerminalProcessor.cpp:551-564` — `process()` runs the chain only while `tty->isOpen()`.
- `eve/test/jobcontrol.sh` (new, 117 lines) — checks the process group of a background job, `SIGSTOP` and `SIGCONT`, the `fg` handoff, and the suspend of a background read. It prints the `^Z`, `bg`, `fg`, `^C` steps for the keyboard.
- Doxygen: `jam_StdioServer.h`, `jam_TerminalFIFO.h`, `jam_TerminalConsole_windows.h`, `jam_TerminalCygwinTTY_windows.h`, `Session.h`, `EVETerminalProcessor.h`.

### Alignment Check
- [x] BLESSED principles followed — MANIFESTO E, "A copy per hop" is forbidden: the master read is one OS-boundary copy into the ring, and the ring write to the pipe is the second boundary.
- [x] NAMES.md adhered — `writeFromDevice` joins the family of `TerminalFIFO::writeFromDevice` and `writeFromTerminalTTY`.
- [x] MANIFESTO.md principles applied.
- [ ] The project doxygen regeneration did not run. `ninja doxygen` in `Builds/Debug` fails with `unknown target 'doxygen'`. JAM `<leader>bd` did not run. Scratch runs with the project Doxyfile template report 0 warnings for the edited headers.

### Problems Solved
- **Extra copy in the helper.** `Session::run` read the master into `buffer`, then `StdioServer::write` copied it into the ring. The read goes straight into the ring now.
- **Inherited variable won over the integration variable.** The Windows block appended the new variable after the parent block, so the parent value stayed first. The new variable replaces it.
- **`close` against the owner contract.** `jam_TerminalTTY.h:102` says `close` terminates the shell. `TerminalCygwinTTY::close` closes the input pipe. The helper reads end of stream, sends SIGHUP to the shell (`Session.cpp:113-116`, `:143-146`), and exits when the shell output ends. The block states this chain. No code changed.
- **Debug launch crash.** `process` ran before `openTTY` and called `TerminalConsole::read` on a console that `open` never wired. The guard makes the tick safe whenever the TTY is not open. ARCHITECT: *"debug no crash."*
- **Run results.** ARCHITECT: *"seq, ^C, resize works"*, *"cmd, pwsh works"*, and *"works."* for the job-control script.
- **Mistakes owned.** My first Engineer call carried a model that the definition does not name; the hook denied it, and I sent the call again with `haiku`. My first transcript search for the audit-phase change had too wide a scope and ran past the watchdog; I stopped it and read the audit-fix brief directly.

### State for Continuation
- Not proven: why the crash showed only after the audit. The audit-fix brief (transcript line 7906) touched `jam_TerminalCygwinTTY_windows.cpp`, `Main.cpp`, `Runtime.{h,cpp}`, `Session.{h,cpp}`, `EVETerminalProcessor.cpp` line 84, and `default-eve.md` rows 219 and 221. None of these is on the crash path (`jam_TerminalTTY_windows.cpp:100`, `jam_TerminalConsole_windows.cpp:628`). `prepare` opens the TTY only when `winsize.isValid()` (`EVETerminalProcessor.cpp:410`).
- Behaviour of the copy loop: `Session::run` ends when a read returns 0 or less. A full output ring also returns 0 (`jam_TerminalFIFO.h:203-205`). That happens only after a failed write to the standard output. `writeFromDevice` does not clamp the callback result to `maxBytes` (`jam_TerminalFIFO.h:205`); `runtime.read` returns at most `maxBytes`.
- Not tested: the fallback to powershell when the runtime file is absent (`PLAN-cygwin-pty.md` Step 7.3).
- The EVE build packs `end/Source/plugins/EVE v0.0.1 Windows.zip`, a path outside the EVE tree. This is the EVE to END pipeline.
- The observations of the earlier entry stay open and unchanged: `writeLine` loses a line on a failed payload write, `writeScrollLines` writes `CSI 0 L` for a zero distance, `writeClearScreen` has no call site found.
- `CLAUDE.md` of EVE names `ninja doxygen`. The target does not exist.
- Nothing from this sprint is committed.

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261117T223541` — seq on the cygwin pty paged and choked after a fast start. ARCHITECT reported *"seq works"*; the paging was not re-observed or measured after the copy fix, so the entry stays.
- `DEBT-20261117T223545` — the active prompt printed before seq finished. Not re-observed; the entry stays.
- `DEBT-20261115T221459` — END: load and close of each EVE instance, and focus and keyboard on Windows (unchanged).
- `DEBT-20261105T000000` — Windows runtime test of the image preview (unchanged).

---

## Sprint: Cygwin PTY — zsh on Windows runs over an msys2 pty through eve-cygwinPTY, ConDrv stays for pwsh and cmd ✅

**Date:** 2026-11-17 to 2026-11-18
**Duration:** one session, across a context compaction. The start and end times are not recorded.
**Scope (ARCHITECT):** `PLAN-cygwin-pty.md`. Rulings: *"msys-2.0.dll and cygwin1.dll must be explicitly written at eve.md, we shipped default path assuming user install it, and at default path. user can still override. but still internal fallback, when those arent available just like zsh, we run powershell by default"*; *"config dll just follow verbatim established pattern. eve.md is crosplatform single dot config. windows lane are harmless for mac."*; helper name *"eve-cygwinPTY.exe"*; helper home *"Sub-project in eve/"*; *"make it identical then. we have working pattern, why deviates"* (the EVE to END pack and offload pattern); *"dont change this machine toolchain. encapsulate"*; *"UNIDIRECTIONAL data flow, fully event driven"*; *"/audit, doxygen pass, then /log"*. ARCHITECT's run result: *"works, from empirical observations only this is faster than conpty."*

### Agents Participated
- COUNSELOR — the reads of the msys2 runtime source and of JAM; the plan; the validation of every Engineer return; the filter of the audit findings; the whatdbg trace specs; the read of each doxygen block against the code; this log.
- Engineer — the plan steps 1 to 8; the audit fixes (10 findings applied, builds exit 0, no warnings).
- Pathfinder — the whatdbg traces; the git evidence; the doxygen pass in four runs (new headers, the Console, Output and Input headers, and the TTY, helper and EVE headers).
- Auditor — one run over the sprint.

### Files Modified
- `jam_terminal/tty/native/jam_TerminalCygwinTTY_windows.{h,cpp}` (new) — `TerminalCygwinTTY` implements `TerminalTTY` over three named pipes (input, output, size) with overlapped I/O. `open` starts the helper with `"<runtime>" <size pipe handle> <packed start size> "<shell>" <args>` (`.cpp:78`). `read` is `TerminalFIFO::read (output.get(), buf, maxBytes)` (`.cpp:121-124`). `write` starts one overlapped `WriteFile` and checks it with `GetOverlappedResult (..., FALSE)`, so the tick never waits (`.cpp:126-158`). `setWinsize` writes the packed 64-bit size (`.cpp:160-168`).
- `jam_terminal/processor/jam_TerminalFIFO.h`, `processor/native/jam_TerminalFIFO_windows.cpp:50-72` — public static `read (Handle, char*, int)`: peek the pipe, then read only the available bytes.
- `jam_terminal/tty/native/jam_TerminalConsole_windows.{h,cpp}` — static `createProcess` (`.cpp:184-229`), shared by `spawnProcess` (`.cpp:346`) and `TerminalCygwinTTY::open`. The dedicated server context is undone (U5 to U1 of `PLAN-windows-console-throughput.md`), and `isServerOpen` is gone.
- `jam_terminal/tty/native/jam_TerminalConsoleOutput_windows.{h,cpp}`, `jam_TerminalConsoleInput_windows.{h,cpp}` — the same undo: `std::string sequence`, one `queue`, no `clientQueue`, no `numInterrupts`.
- `jam_terminal/jam_terminal.h` — the include of the new backend.
- `eve/cygwinPTY/` (new sub-project): `project-info.md`, `cast/{spell.md,cmake.cast}` (`/ENTRY:cygwinPTYCRTStartup`, `cast/cmake.cast:188-190`), `Source/Main.cpp`, `Source/Runtime.{h,cpp}`, `Source/Session.{h,cpp}`. `Runtime` loads the runtime DLL with `juce::DynamicLibrary`, calls `msys_dll_init` or `cygwin_dll_init`, and binds 16 functions by name. `Session` opens the pty with the start size, makes the slave the controlling terminal, starts the shell with `spawnv` between `dup` and `dup2` of fd 0 to 2, ignores `SIGTTIN` and `SIGTTOU`, puts the shell in its own process group and makes it the foreground group, relays stdio and the size pipe, and on shell exit calls `setpgid (0, shell)` and `setsid` (`Session.cpp:43-72`, `:148-152`).
- `eve/Source/EVETerminalProcessor.{h,cpp}` — `createTTY (program, directory)` and `getRuntime (program)`: a program with a configured runtime DLL in its folder gets `TerminalCygwinTTY`, any other program gets `TerminalWindowsTTY` (`.cpp:61-122`). `openTTY` builds the TTY of the program it opens (`.cpp:428`, `:460`). The conpty sidecar code is deleted.
- `eve/Source/ShellIntegration.{h,cpp}` — `getOrCreateCygwinPty`: finds the zip resource, unpacks it with `juce::ZipFile`, keeps a stamp of `CAST_CYGWIN_PTY_HASH` (`.cpp:51-91`).
- `eve/cast/identifiers.md`, `eve/Source/layout/default-eve.md:219,:221`, `eve/Source/generated/Identifiers.*` — `msysRuntime`, `cygwinRuntime`, `defaultMsysRuntime`, `defaultCygwinRuntime`, and the two settings rows.
- `eve/cast/files.md`, `eve/cast/cmake.cast:346-372`, `eve/Source/generated/Files.*`, `eve/cast/spell.md`, `eve/project-info.md` — the pack and offload pattern of END: the helper's cast packs the zip into `Source/windows`, EVE's cast globs it, filters by platform, and hashes it.
- `eve/PLAN-cygwin-pty.md` — the locked plan, with each correction found at run time.
- `eve/DEBT.md` — two entries (see Debts Deferred).
- Doxygen pass: `jam_TerminalCygwinTTY_windows.h`, `jam_TerminalFIFO.h`, `jam_TerminalConsole_windows.h`, `jam_TerminalConsoleOutput_windows.h`, `jam_TerminalConsoleInput_windows.h`, `jam_TerminalTTY_windows.h`, `EVETerminalProcessor.h`, `ShellIntegration.h`, `cygwinPTY/Source/Runtime.h`, `cygwinPTY/Source/Session.h`. The working tree also holds uncommitted work of earlier sprints; `git status` lists both trees.

### Alignment Check
- [x] BLESSED principles followed — one backend object per platform behind `TerminalTTY`; the tick never waits; the helper owns the runtime, the pty and the shell.
- [x] NAMES.md adhered — every name comes from the plan and its families.
- [x] MANIFESTO.md principles applied.
- [ ] Not built after the doxygen pass. The pass changed comments only.
- [ ] The project doxygen regeneration did not run. `ninja doxygen` in `Builds/Debug` fails: `unknown target 'doxygen'`. JAM `<leader>bd` did not run. The zero-warning result comes from scratch runs with the project Doxyfile template, in the session scratchpad.
- [ ] Open rulings: see State for Continuation items 1 to 3.

### Problems Solved
- **zsh runs over the msys2 pty.** The runtime loads inside the helper, where `main` is the helper's. EVE and its host never load it. ARCHITECT: *"works, from empirical observations only this is faster than conpty."*
- **Resize without a stall.** `TIOCSWINSZ` on a master whose group is not the foreground group waits 60000 ms for a self-signal (`pty.cc:2526-2537`, `sigproc.cc:29`, `:844-847`). `Session::openShell` sets the shell group and the foreground group before the relays open.
- **Master read and write failing with `EIO`.** `bg_check` (`pty.cc:2464`, `:2238`) with an orphaned group. `SIGTTIN` and `SIGTTOU` are ignored after the spawn.
- **Shutdown chain.** The shell exit calls `releaseTerminal`, the master read ends, the helper exits. EOF of the helper stdin hangs up the shell.
- **Helper delivery.** The first form embedded no helper in a Release build. ARCHITECT ruled *"make it identical then. we have working pattern, why deviates"*. The form now mirrors END (`end/cast/cmake.cast:370-397`, `Nexus::getOrCreatePlugin`).
- **Audit.** Ten findings applied; each was read against the code before the fix. F1 and the copy note were not applied (see State for Continuation item 1).
- **Doxygen pass.** New blocks on every symbol above. Stale blocks corrected from the code: `numInterrupts`, `clientQueue`, `addClientInput`, `isServerOpen`, the `handleAsyncUpdate` of the Console, and the render and read split of the Console output. Scratch validation with the project Doxyfile template: 0 warnings for the ten headers.
- **Mistakes owned.** I stopped the first doxygen run at the 15-minute watchdog before it reported, and re-asked for the report. I read the `Winsize` brief against code and found a layout claim that no header supports; the block was corrected to the code. The sentence limit of 20 words was missed once; it was corrected.

### State for Continuation
- Open ruling 1, for ARCHITECT: `Session.cpp:187-193` reads the master into `buffer`, then `stdio.write` copies it into the ring. MANIFESTO E lists *"A copy per hop"* as forbidden. `PLAN-cygwin-pty.md` BLESSED D says bytes cross each boundary once. Both citations hold; neither is asserted.
- Open ruling 2, for ARCHITECT: `jam_TerminalTTY.h:102` says `close` terminates the child process if it is still alive. `TerminalCygwinTTY::close` (`jam_TerminalCygwinTTY_windows.cpp:96-119`) closes the handles and does not terminate the helper. `PLAN-cygwin-pty.md` Step 4.6 says the helper ends on EOF of its stdin.
- Open ruling 3, for ARCHITECT: `jam_TerminalTTY.h:88-89` says the environment replaces the parent block. `buildEnvironmentBlock` (`jam_TerminalConsole_windows.cpp:155-161`) appends the variables after the parent block.
- Verify first: the cast pack from the helper into `Source/windows` runs after each helper build. An earlier read found that a Release build also installs the VST and VST3 files. This was not read again in this span.
- Verify at runtime (ARCHITECT): `^C`, job control, resize, pwsh and cmd, and the fallback to powershell when the runtime file is absent (`PLAN-cygwin-pty.md` Step 9).
- Verify first, a Debug launch under whatdbg crashes before the helper path runs. Trace `trace-eve-order.out.txt`: `process` (`EVETerminalProcessor.cpp:555`) runs on a pool thread before `openTTY` (`:546`, `:460`). The call reaches `TerminalConsole::read` (`jam_TerminalConsole_windows.cpp:628`) on the TTY of the member initializer (`EVETerminalProcessor.h:357`), whose `getOutputMode` is empty because `open` never ran (`jam_TerminalTTY_windows.cpp:50`). `prepare` calls `openTTY` only when `winsize.isValid()` (`EVETerminalProcessor.cpp:410`). Not read: why ARCHITECT's own launches run `openTTY` first. `git` has no history for `jam_TerminalConsole_windows.cpp` (untracked).
- Observations from the doxygen pass, not validated against a clause: `TerminalConsoleInput::writeLine` consumes the line before the payload write, so a failed write loses the line (`jam_TerminalConsoleInput_windows.cpp:246-260`); `writeScrollLines` writes `CSI 0 L` for a zero distance, which no current caller reaches (`jam_TerminalConsoleOutput_windows.cpp:191-202`); `writeClearScreen` has no call site found, and the LSP gave no references.
- LSP on JAM files returns no symbols. Load the project in nvim, so `.clangd` points at the `JUCE_WINDOWS` project, before the next `findReferences`.
- `CLAUDE.md` of EVE names `ninja doxygen`. The target does not exist in `Builds/Debug`.
- Nothing from this sprint is committed.

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261117T223541` — seq on the cygwin pty starts line by line, then pages and chokes (ARCHITECT: *"add to DEBT as issues need to be resolved next sprint"*; the Windows lane needs the clock of macOS, driven by the pty speed).
- `DEBT-20261117T223545` — the active prompt prints before seq finishes.
- `DEBT-20261115T221459` — END: load and close of each EVE instance, and focus and keyboard on Windows (unchanged).
- `DEBT-20261105T000000` — Windows runtime test of the image preview (unchanged).

---

