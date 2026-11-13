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

## Sprint: Build Against the jam Document Contract ✅

**Date:** 2026-11-14
**Duration:** part of one session. `jam/carol/SPRINT-LOG.md` Sprint 187 is the primary record.

### Files Modified
- `Source/EVEProcessor.h:135` — `jam::ParameterLayout::get (*jam::MarkdownDocument::getOrCreate (…))`.
- `Source/EVEView.cpp` — `*jam::HtmlDocument::getOrCreate (…)` (`:348`); `markdown->setDocument (std::move (*document))` (`:612`).
- `Source/EVETerminalProcessor.cpp` — caller of the pointer-returning parse.
- `project-info.md`, `CMakeLists.txt` — the `jam_data_structures` description drops "JSON conversion".

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered
- [x] MANIFESTO.md principles applied

### Problems Solved
- EVE did not compile against `getOrCreate` returning `const T*` and `parse` returning `std::unique_ptr`. END with EVE builds.

### State for Continuation
- Runtime not tested this session.

### Debts Paid
- None

### Debts Deferred
- None

## Sprint: Windows Terminal — Native Path Form, Shell Setting with Fallback, ConPTY Exit by Pipe EOF, History Reader Shares the Writer's Maps, Audit Sweep ✅

**Date:** 2026-11-12
**Duration:** two sessions, joined by a context compaction. The start and end times are not recorded.
**Scope (ARCHITECT):** *"let's continue with eve"*; *"fix the cause not the symptoms, fix the design, design by contract. unidirectional data flow"*; *"`native` implementation always at the very first INPUT to read and very last OUTPUT"*; *"whatever juce implemented as `native` dont fight it"*; shell row "One row, empty = platform", *"why not let user declare explicitly? `<program> <args>`"*, fallback "PowerShell, then cmd", message "shell not found:", *"still running without fail"*; *"why magic strings? just use Identifier lexicon"*; *"windows is wrong, that's a fact. mac already tested and measured end to end"*; *"no hack, no patch, no shortcut"*; *"no juce patch"*; history seam "Reader uses readWrite"; *"history works. terminal functional."*; `/audit until /log no gate`. Plans: `PLAN-shell-seam.md` (Steps 0-7), `PLAN-history-windows.md` (Steps 0-2). Both deleted at completion.

### Agents Participated
- COUNSELOR — the reads of the path flow, the swap ownership, the JUCE map share modes, the ConPTY exit, the POSIX TTY and the state save path; both plans; the validation of every Engineer return; the filter of 17 audit findings and the Pathfinder findings; this log.
- Engineer (sonnet) — the shell seam steps; the reader map mode; the EVE audit fixes; the JAM diagnostics removal; the Windows TTY RAII, resize and read fixes; two follow-ups; the plan deletion.
- Pathfinder (haiku) — the swap ownership read; whatdbg traces; the doxygen pass and its follow-up.
- Auditor (opus) — one run, 17 findings plus diagnostics and drift.

### Files Modified
- `eve/Source/shell/{eve,zsh_integration.zsh,bash_integration.bash}` — under MSYS2 the shells send `cygpath -m` paths (`_eve_path`, `_eve_cwd`); the first input carries the native form.
- `jam/jam_core/text/jam_Format.{h,cpp}` — `toNativePath` deleted; `jam/jam_gui/widgets/jam_PreviewComponent.cpp:357` builds `juce::File` from the path as given.
- `eve/Source/layout/default-eve.md:217` — `shell` setting; the settings table description column widened to 80. `eve/Source/layout/parameters.md:39-42` — model rows `shell` (the program that runs) and `shellNotFound`. `eve/cast/identifiers.md` — `shellNotFound`, `failShellNotFound`, `shellVariable`, `defaultShell`, `defaultShellWindows`, `powershellProgram`, `commandPromptProgram`.
- `eve/Source/EVETerminalProcessor.{h,cpp}` — `getDefaultShell`, `getShell` from the setting; `openTTY` (`.cpp:428-461`) splits once, starts in `cwd` only when it is an absolute path, tries `powershell.exe` then `cmd.exe` on Windows, stores `shell`, `shellNotFound` (empty when the configured program runs) and `shellExited`; the profiler branch stores `shell` too; `applyOpenCommand` and `getQuotedPaths` quote for the `shell` row. `eve/Source/EVEView.cpp:165-171` — `shellNotFound` event shows `shell not found: <program>`.
- `eve/Source/ShellIntegration.{h,cpp}` — `addShellScripts`, `getShellEnvironment`, `getShellArguments` read the integration setting themselves; `isShellIntegrated` private.
- `jam/jam_terminal/tty/native/jam_TerminalTTY_windows.{h,cpp}` — `processWait` closes the pseudo console on process exit, so the session ends by pipe EOF as on POSIX; `resizeWork` runs the resize on the one-thread pool; the four handles are `std::unique_ptr<void, decltype (&CloseHandle)>`; helpers return values (no out-parameters); the program is quoted on the command line; `read()` returns -1 on any non-pending error and with no session; `childPid` and `safeCloseHandle` deleted; pessimistic `funcs` guards removed.
- `jam/jam_terminal/document/jam_TerminalHistory.cpp:125`, `:289`, `jam_TerminalImageRecord.h:99`, `:104` — reader maps use `readWrite`.
- `jam/jam_terminal/{buffers/jam_TerminalBuffer.cpp,document/jam_TerminalSwap.cpp,processor/jam_TerminalProcessor.cpp}`, `jam/jam_gui/widgets/jam_TextEditor.cpp` — ODE diagnostics removed.
- `eve/SPEC.md` — Feature 1 shell setting; failure table rows (Windows fallback; macOS has no fallback); Feature 4 path form.
- Doxygen synced: `EVETerminalProcessor.h`, `ShellIntegration.h`, `jam_TerminalImageRecord.h`, `jam_TerminalTTY_windows.{h,cpp}`.

### Alignment Check
- [x] BLESSED principles followed — one path form owner per shell at the first input; one session-end path (EOF) on both platforms; resize and close serialized on the owner pool; RAII for every handle.
- [x] NAMES.md adhered — `shellNotFound` / `failShellNotFound` join `shellExited` / `failShellExited`; `processWait`, `resizeWork` join `readWait`; `pendingWinsize` joins the `pending*` family.
- [x] MANIFESTO.md principles applied.
- [ ] Not built after the audit fixes. No agent ran `cast` or a build after Step 6 of the shell plan.
- [ ] Doxygen not regenerated: EVE has no `doxygen` target in `Builds/Debug`; JAM regenerates in nvim.

### Problems Solved
- **`cwd` asserted at `juce_File.cpp:162`.** MSYS2 sent `/c/Users/x`. The shell now sends the native form; no reader converts.
- **No history on Windows.** JUCE keeps the map handle on Windows (`juce_Files_windows.cpp:515`); a `readOnly` open does not share write (`:510`), so every history read failed (208 of 208). The reader maps `readWrite`, which shares write. ARCHITECT confirmed: *"history works."*
- **`exit` asserted, then did not exit.** ConPTY keeps the pipe open after the shell exits. `processWait` closes the pseudo console, which flushes the last output and breaks the pipe.
- **Audit, resolved:** F1 (quoting keyed on the whole setting), F2 (macOS fallback unreachable: `login` starts the shell, `jam_TerminalTTY_posix.cpp:101-129`), F3, F4, F5, F7, F8, F10, F11, F12, F13, F14 (non-absolute `cwd`), F15, F16, F17, the diagnostics, the `default-eve.md:217` wording; Pathfinder F1, F3-F7.
- **Audit, rejected with the read:** F6 Q1 — the `isOpen` guard keeps a second open from overwriting the `shell` row; F6 Q3 — MANIFESTO S abstracts logic that appears "more than twice", and the settings root appears twice; F9 — the runtime log shows `processExit`, the last bytes, then `readEOF` with the bundled ConPTY; the TTY "drift" — ARCHITECT approved the `processWait` design with `/go`; "two names for one shell" — `"powershell"` and `"powershell.exe"` are different values.
- **Mistakes owned.** I proposed `toNativePath` in the readers before the cause. I offered a JUCE patch, then a new type, then `readWrite`, each before I read the ownership contract. I put a macOS fallback in the plan that the platform cannot reach.

### State for Continuation
- Build with `cast cast/spell.md --debug` in eve; the Windows TTY changes (RAII, out-parameter removal, `resizeWork`) are not compiled yet.
- Runtime checks on Windows: history after commands; `exit` closes the app (ARCHITECT saw a delay of a few seconds after `readEOF`; the cause is not located); a resize while the shell runs; a `shell` setting with a quoted path that holds spaces; a missing program shows `shell not found:` and PowerShell runs; `eve preview` under MSYS2.
- macOS: build and run once; the shell setting and the `shell` row are new there too.
- `cwd` restore: the shell starts in the saved `cwd` of the last session (`EVETerminalProcessor.cpp:437-438`), by design (`EVETerminalProcessor.h` openTTY block). ARCHITECT asked why macOS does not show it; not answered by evidence yet.
- `jam_TerminalTTY_windows.h` `open` `@param environment` says a variable replaces an inherited one; `buildEnvironmentBlock` appends after the parent block. The Windows rule for a duplicate name is not verified.
- `jam_TerminalTTY_windows.cpp:371` pipe timeout literal is 100 s; its old comment said 1 s.
- LSP returned empty results on JAM files; load the JAM project in nvim before the next LSP check. Run `<leader>bd` in jam.

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261105T000000` — Windows runtime test (unchanged; `toNativePath` in its text is deleted).

---

## Sprint: Keycap and Grapheme Clusters — Clusters Join Across Reads, Keycap in Two Cells with the Digit in the Box, a Cluster Never Narrows, Link Label at the Character, Half-Thickness Underline, Cluster Test Script ✅

**Date:** 2026-11-11
**Duration:** two sessions, joined by a context compaction. The start and end times are not recorded.
**Scope (ARCHITECT):** *"we dont need to adhere to other terminal, we need to conform to Unicode SPEC"*; *"why dont we treat this like ligatures? use 2 cell then fit, number inside box"*; *"number inside box. correct."*; option A for the join of an Extend codepoint ("A", then "/go"); *"i dont understand, all options are garbage"*; *"if non-deterministic unexpected behaviour must be `fixed` with contract violations then your understanding is wrong, or the whole design is wrong"*; *"489 passed, 0 failed"*; *"no audit."*; *"update the doxygen when necessary."*; *"/log"*. The underline thickness and the open-mode link label were earlier requests in the first session.

### Agents Participated
- COUNSELOR — the reads of the decoder, the glyph path and the Unicode data; the root cause of the split cluster; the root cause of the width rule; the validation of every Engineer return; the test script fixes; the read-back of the doxygen pass; this log.
- Engineer (sonnet) — `applyExtension` and the processor split; the keycap width, constraint and enclosure; the first version of `test/clusters.sh`.
- Librarian (haiku) — the Unicode 17.0.0 core specification: rule P6, D54, D56, section 23.4.
- Pathfinder (haiku) — the git status read; the doxygen pass on five headers.
- Auditor — not run. ARCHITECT: *"no audit."*

### Files Modified (11 total)
- `jam/jam_core/text/jam_AttributedChar.{h,cpp}` — new `applyExtension` (`.cpp:3339-3370`): replays the cluster through the grapheme segmentation step and appends the following codepoints that continue the cell. `getWidth (cluster)` (`.cpp:3319-3332`): U+20E3 gives two columns first; the U+FE0E branch that made a cluster narrow is deleted, so a codepoint that joins a cluster can only widen it.
- `jam/jam_core/text/jam_AttributedString.h` — doxygen only: the width class comes from the whole cluster (`:66`, `:184`).
- `jam/jam_terminal/processor/jam_TerminalProcessor.{h,cpp}` — `addTextRun` first calls `addExtension`, then writes the remaining clusters (`.cpp:503-511`). New `addCluster` (`:513-538`), `addExtension` (`:540-568`), `getPrecedingColumn` (`:570-579`), `getCluster` (`:581-592`). Each cluster reads its own row from the buffer.
- `jam/jam_graphics/fonts/jam_GlyphConstraintTable.cpp:24-32` — `case 0x20e3`: fit, centre, centre, `maxCellSpan 2`.
- `jam/jam_graphics/fonts/jam_GlyphArrangement.{h,cpp}` — static `isEnclosing`, `isEnclosed`, `applyEnclosure` (`.cpp:100-129`), called from `addEntry` (`:182-183`): the box takes the cell span of the atom and the base sits in the middle. `getDecorations`: `underlineScale { 0.5f }` (`:587`, `:597`). Doxygen of `addEntry` and `getDecorations` synced.
- `jam/jam_gui/widgets/jam_TextEditor.{h,cpp}` — new `getLinkLabelColumn` (`.cpp:1821-1834`); the label callback receives that column (`.h:717`). Doxygen of `getLinkLabelColumn`, `applyLinkLabel` and `paintLinkLabels` synced to the bodies.
- `eve/test/clusters.sh` — new, untracked. It measures the cursor position with the CPR query and compares a whole write with a write split at every byte. Sections: absolute widths, split equals whole (start of line, one and two columns before the right edge, insert mode), cursor moves between the writes, and a rendering view.

### Alignment Check
- [x] BLESSED principles followed — the fix of the width rule removes the cause of the non-deterministic result; no pending state, no timer.
- [x] NAMES.md adhered — each new name joins the `add`, `get`, `apply` families.
- [x] MANIFESTO.md principles applied
- [x] Validated by ARCHITECT's build and run: `test/clusters.sh` gives 489 passed, 0 failed.
- [ ] No audit. ARCHITECT: *"no audit."*
- [ ] Zero doxygen warnings not confirmed. Regeneration is ARCHITECT's (`<leader>bd` in jam).

### Problems Solved
- **A cluster split across two reads.** `AnsiDocument::getTextToken` ends a text token at the bytes that are available (`jam_AnsiDocument.cpp:47-69`). `addTextRun` then restarted the segmentation, which broke UAX #29 rule GB9 (an Extend codepoint joins the cluster before it). The processor now extends the cluster before the cursor first.
- **Keycap.** Unicode rule P6 says an enclosing mark surrounds its base. The cluster `digit FE0F 20E3` takes two cells, the box fits the span, and the digit sits inside the box (ARCHITECT: *"number inside box. correct."*).
- **The same bytes gave a different result when the read split.** The only rule that made a cluster narrower was U+FE0E (`jam_AttributedChar.cpp`, old line 3331). A wide base at the last column wrapped before its selector arrived, and the narrowing could not undo the wrap. The spec has no text that makes U+FE0E shrink a cell: `tr51.txt:1744-1745` says only that emoji have about the same advance width as CJK ideographs. The rule is deleted. A cluster width can now only grow, and growth is a forward wrap.
- **Test script.** Terminal echo of the CPR replies corrupted the measure (`stty -echo`, restored in the `EXIT` trap). The per-byte `printf` calls of `write_hex` made three false failures (one `printf` for all bytes now). zsh arithmetic needs `&&`, not `and`.
- **Mistakes owned.** I offered three patch options for the narrowing before I read the width rule. ARCHITECT's ruling that all options were garbage was correct, and the root cause was in `getWidth`. I also first read the byte 1 to 3 failures as a decoder fault; the cause was the script.

### State for Continuation
- ARCHITECT runs `<leader>bd` in jam and confirms zero warnings; the doxygen pass changed five headers and did not regenerate the index.
- Open, no ruling: when a keycap has a background colour or an underline, the shifted digit spreads half a cell past the atom. `addBackground` (`jam_GlyphArrangement.cpp:576-582`) and the underline rectangles use `entry.originX` and `entry.advance`. A fix needs one new `Entry` field.
- Visible change to check: `😀 FE0E` now takes two cells (`test/clusters.sh:235` expects 2). `❤ FE0E` keeps one cell because U+2764 is narrow without the selector.
- LSP results on framework files can be empty: `findReferences` of `applyExtension` and `addExtension` returned only the declaration. ARCHITECT loads the JAM project in nvim before the next LSP check.
- `jam_GlyphConstraintTable.cpp:1-5` holds a file-level doxygen block. The pass did not touch it.

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261105T000000` — Windows runtime test (unchanged).

---

## Sprint: Published Lines and Live Rows — One Row One Line, Reader Joins at Build, Clear Only at a Prompt, Ctrl+L to the Shell, TerminalHistory and TerminalEncoder ✅

**Date:** 2026-11-10
**Duration:** two sessions, joined by a context compaction. The start and end times are not recorded.
**Plan:** `PLAN-published-lines-and-live-rows.md` (11 steps).
**Scope (ARCHITECT):** the crash `END-2026-11-10-182526.ips` (`jam::AttributedString::setAtom` ← `AnsiDocument::addGrids`, address `0x3fffffff8`); *"Reader joins at build"*; *"stray variables for state machine are not allowed. if it was state machine must be Model's parameter, hence message thread could listen, other thread could use atomic"*; *"Clear only at a prompt"*; *"Remove the clear command"*; *"our architecture is NOT `typical` terminal topology. we preserved history faithfully. our rendering layer is COMPLETELY decoupled. audio architecture discipline always lock free on terminal (audio thread)"*; *"DESIGN BY CONTRACT"*. Audit rulings: *"No cut"*; *"No, drop them"*; fontId *"No change"*; the names *"TerminalHistory"* and *"TerminalEncoder"*; AnsiDocument *"Keep in AnsiDocument"*; help pages: *"my point is cast, scout, ggwp HELP page should be identical"*, then *"ggwp clears too"*.

### Agents Participated
- COUNSELOR — the reads behind the plan; the DCF filter of 19 audit findings; the `isLinkChanged` fix; the doxygen read-back and corrections in `jam_TerminalSwap.h` and `EVETerminalProcessor.h`; `SPEC.md`; this log.
- Engineer (sonnet) — Steps 1 to 7; the audit fixes; the TerminalHistory and TerminalEncoder splits; the ggwp clear.
- Auditor (opus) — one sweep, 19 findings.
- Pathfinder (haiku) — the inventories; the TextEditor inventory; the doxygen pass in five runs (the first run stopped at the watchdog).

### Files Modified
- `jam/cast/terminal.md` — `@id isWrapped`.
- `jam/jam_terminal/buffers/jam_TerminalBuffer.{h,cpp}` — the binary `writeRow` writes `isWrapped` last; `getNumBytes` counts it; new `getNumBytes() const` sizes the widest row of both grids; `prepare` keeps every row whole, releases the images of dropped rows on both grids, and no longer clears the frame; `clearAlternate` deleted; the VT encoder moved out.
- `jam/jam_terminal/buffers/jam_TerminalEncoder.{h,cpp}` — new `struct TerminalEncoder`: `writeRow` (VT), `writeEraseScreen`, and the private `writeStyle`, `writeColour`, `writeLink`, `writeLinks`, `writeCodepoint`.
- `jam/jam_terminal/document/jam_TerminalSwap.{h,cpp}` — the writer stores one row as one line and never seeks back; a wrapped row with no placement gets no cell starts; `getNumReadRows` and `numReadRows` deleted; `prepare (int newSegmentBytes)`; `applyToLiveData`; the readers moved out.
- `jam/jam_terminal/document/jam_TerminalHistory.{h,cpp}` — new reader: `addLines`, `addLiveLines`, the wrapped head and tail, the grid head and tail, `isGridOpen`, `addImageLines`, the link cache.
- `jam/jam_terminal/document/jam_AnsiDocument.{h,cpp}` — `addWrappedLines`, `addWrappedLine`.
- `jam/jam_terminal/processor/jam_TerminalProcessor.{h,cpp}` — `reset` blanks a changed grid only at an active prompt and keeps the cursor otherwise; `applyWinsize`.
- `jam/jam_terminal/jam_terminal.{h,cpp}` — includes of the two new objects.
- `jam/jam_core/concurrency/jam_AsyncUpdater.{h,cpp}` — `handleUpdateNow`.
- `jam/jam_gui/widgets/jam_TextEditor.{h,cpp}` — `TerminalHistory terminalHistory`; `isOpenChanged` compares with the swap end line; `addFileLinks` writes the recording block to the root only when the last line belongs to it; `isLinkChanged` has two terms.
- `eve/cast/identifiers.md`, `eve/Source/layout/default-eve.md` — the `clear` command and its key row deleted.
- `eve/Source/layout/parameters.md` — the `previousGridStart` row was added in Step 1 and removed by audit finding 2; net no change.
- `eve/Source/EVEView.{h,cpp}`, `eve/Source/EVETerminalProcessor.{h,cpp}` — the `clear` command removed; `prepare` drains the history, runs `applyWinsize`, drains again, then sizes the swap with `terminalBuffer.getNumBytes()`; four trigger pairs became `handleUpdateNow`.
- `eve/CLAUDE.md` — `clear,` removed from the callback list.
- `eve/SPEC.md` — `[AMENDED 2026-11-10]` items: no `clear` command, `ctrl+l` to the program, one row one line with the join at build, clear only at a prompt, no cut, alternate rows dropped; stale swap citations corrected.
- `scout/Source/Main.cpp:272`, `cast/Source/main.cpp:1633` — `jam::TerminalEncoder::writeEraseScreen`.
- `ggwp/Source/Main.cpp` — the scout clear block and `isTerminalOutput`, so the three help pages start the same.

### Alignment Check
- [x] BLESSED principles followed — no stored line changes after its publish; the reader reads only the atomics `firstLine` and `endLine`; no code on the clock tick.
- [x] NAMES.md adhered — every new name joins a family named in the plan, or ARCHITECT named it.
- [x] MANIFESTO.md principles applied.
- [ ] Not validated by a compile. No agent ran `cast` or a build.
- [ ] Zero doxygen warnings not confirmed. clangd timed out for most calls this session.

### Problems Solved
- **The crash path.** The writer edited a published line in place for a wrapped row, and the reader read `previousGridStart` and `numReadRows` as plain members. Both read paths could give a line whose cells and `gridStart` disagree; `getGutter` then stayed at `INT_MAX` (`jam_AnsiDocument.cpp:447-455`). Each row is now one immutable line; the reader joins wrapped lines at build.
- **The frame had three authors.** The `clear` command and a grid change erased the frame. Now only a grid change at an active prompt blanks it; Ctrl+L reaches the shell.
- **Audit (19).** Resolved: 1 (chunked wrapped tail), 2, 3, 5, 18 (Model row removed, `isGridOpen`), 4, 12 (`numReadRows` deleted), 6 and 9 (rulings), 7 (released images), 8 (`getNumBytes() const`), 11 (`reset` simplified), 13 (`TerminalHistory`), 14 (`TerminalEncoder`; `isLinkChanged` at `jam_TextEditor.cpp:1169`), 15 (`handleUpdateNow`), 19 (doxygen and SPEC). Rejected with the read: 10 (the decoder never writes `fontId`), 16 (nothing reads `Id::isWrapped` after the join), 17 (no clause covers the parameter order).
- **Mistakes owned.** I proposed a four-object TextEditor split from a file-size count. The only line-level violation was `:1168`, and the split needed `friend` or public data members. I sent one doxygen run with about 40 blocks; it hit the watchdog. Engineer edited a doxygen block before the audit; the pass rewrote it.

### State for Continuation
- Run `cast cast/spell.md --debug` in `jam` and `eve` (`Id::isWrapped` in; `Id::previousGridStart` and `CommandId::clear` out). Then build `jam`, `eve`, `end`, `scout`, `cast`, `ggwp`.
- Verification list: `PLAN-published-lines-and-live-rows.md` § Verification, items 2 to 8.
- Run `<leader>bd` (jam) and `ninja doxygen` (eve, end); confirm zero warnings.
- Open, no ruling: the prompt wraps by one or two cells on a new pane. A whatdbg trace of `jam_TextEditor.cpp:1375` and `EVETerminalProcessor.cpp:458` gives the two winsize values.
- Fact noted: `addReleasedImage` asserts capacity and drops a record when full (`jam_TerminalBuffer.cpp:338-344`); the `prepare` release loop runs before `prepareRows` sizes it again.

### Debts Paid
- `DEBT-20261108T145906` — Ctrl+L destroys the Claude Code active prompt on the normal screen. Ctrl+L now goes to the program, and no command erases the frame.

### Debts Deferred
- `DEBT-20261105T000000` — Windows runtime test (unchanged).

---

## Sprint: Invalid Config Never Stops the Plugin — Validator Covers the Style Path, Failed Loads Reach the Overlay, Pane Is the Sole Size Author, MoltenVK False Trigger Removed ✅

**Date:** 2026-11-10
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"i told you invalid config must never failed, but showing error. no fucking garbage assert. this is unacceptable"*; *"fix it first"* (the `[recovery]` messages); *"[1] and [2] are same defects. you fix the cause, the design, the race, you'll fix both. DESIGN BY CONTRACT."*; *"jam_Resizer.h is DEAD. should be deleted."*; *"Function::Map defer, wrap this sprint first"*.

### Agents Participated
- COUNSELOR — the reads of the config, style, VST2 and size paths; the filter of 15 audit findings; direct edits of `Session.cpp`, `Session.h`, `jam_VulkanDevice.cpp`, the `description` skip, `EditorView.cpp`, `EVEView.{h,cpp}`, `SPEC.md`; this log.
- Engineer (sonnet) — the Validator rules and two correction rounds; the deletion of `jam_Resizer.h`.
- Auditor (opus) — one run on the invalid-config fix, 15 findings.
- Pathfinder (haiku) — the `Function::Map` inventory; the doxygen pass over `jam_ConfigValidator.h`, `jam_Model.h`, `Session.h`.

### Files Modified
- `jam/jam_markdown/document/jam_ConfigValidator.{h,cpp}` — `metrics`, `fonts`, `window` rules (`getMetricsFailures`, `getFontFailures`, `getStyleTextFailures`, the last skips `description`); `getValueFailures` requires a value for number and boolean types; `getKeyFailures` rejects a key with no row key; `getSetText` for the repeated set text.
- `jam/jam_plugin_bootstrap/layout/jam_PluginEditorLayout.cpp` — `getSourceFailures` (embedded file with a font extension) is in the settings and the theme rules; `getSettingsTree` adds the children a table gained on disk.
- `jam/jam_data_structures/model/jam_Model.{h,cpp}` — `Model::addChildrenFrom`, because `setValuesFrom` skips a child with no match.
- `jam/jam_vulkan/device/jam_VulkanDevice.cpp` — only a MoltenVK message that holds `MTLCommandBuffer` requests recovery. A later edit by another agent removed the `[recovery]` log line from this block.
- `jam/jam_core/buffer/jam_Resizer.h` — deleted; include and module row removed.
- `end/Source/plugins/Session.{h,cpp}` — the two asserts on external load results are removed; a failed scan or a null instance writes the overlay message.
- `end/Source/config/ConfigTheme.cpp` — `addChildrenFrom` after `setValuesFrom`.
- `end/Source/ui/EditorView.cpp:240` — `resized()` before `createProcessorEditor()`, so the rows hold the pane size when the plugin editor is built.
- `eve/Source/EVEView.{h,cpp}` — `resizeEnd` does not write `width` and `height` under END.
- `eve/SPEC.md` — Resize item 6.

### Alignment Check
- [x] BLESSED principles followed — validation stays in the Validator; size flows one way from the pane.
- [x] NAMES.md adhered — new names join the `get…Failures` family; `addChildrenFrom` joins `setValuesFrom`.
- [x] MANIFESTO.md principles applied.
- [x] No change to `jam_StyleManager`, KANJUT or JFS; no version bump. `jam_vulkan` was touched only on ARCHITECT's *"fix it first"*.
- [ ] Zero doxygen warnings unconfirmed; no index was regenerated by an agent.

### Problems Solved
- **A stale config killed the load.** The `EVEProcessor` constructor reads the disk theme. A throw reached `catch (...)` in JUCE (`juce_VSTPluginFormatImpl.h:2167`), and END received a null instance. The exact row is unrecoverable, because the file was deleted. The rules now reject every input the Auditor traced to a throw or a null read.
- **New faces on disk were dropped.** An alias to a new face would throw in `StyleManager::getFont`. `addChildrenFrom` carries the face.
- **Every MoltenVK message requested recovery**, and each burst marked the glyph atlas and the texture cache dirty. The `primitive restart` warning no longer does.
- **EVE drew at 800×600 in a larger pane, and OMP wrapped.** EVE was built at the default rows and corrected later, and it wrote its own size back to the rows it follows. ARCHITECT reports no issue so far on pressure run, size and wrap.
- **Audit:** 1–8, 10, 12, 15 resolved. Rejected with the read: 9 (EVE's default differs from END's theme, `jam_PluginEditorLayout.cpp:116`), 11 (one `Session` creation, no erase found in `end/Source`), 13 (MANIFESTO S: more than twice), 14 (no clause).
- **Mistakes owned.** I handed ARCHITECT "delete once" for a stale file instead of making the loader safe. My first table cited `jam_Css.cpp:2068`; the throw is at `:301`. My `window` rule scanned `description` and rejected a valid default file. I edited a `Session.h` comment before the audit.

### State for Continuation
- Corrupt `theme.md` run not reported: check defaults render and the overlay shows the error.
- `<leader>bd` in jam, `ninja doxygen` in eve and end; confirm zero warnings.
- `jam/PLAN-gpu-memory-failure-recovery.md:139-142` still says any MoltenVK id triggers recovery; the code at `jam_VulkanDevice.cpp:408-413` narrows it. Confirm a real failure carries `MTLCommandBuffer`; the text comes from that plan's `:41`, and no MoltenVK source was read.
- `eve/Source/layout/parameters.md:73`, `:75` do not state the END exception.
- Real `get`/`add` mismatches, not fixed: `ggwp/Source/Artwork.cpp:68`; endless `TTY.cpp:64`, `Processor.cpp:432`, `VideoDCS.cpp:94`, `:117`, `VideoOSCExt.cpp:111`, `VideoOSC.cpp:218`.

### Debts Paid
- None

### Debts Deferred
- `Function::Map` replacement (no ledger id; ARCHITECT: *"Function::Map defer"*). Agreed shape, not implemented: variadic signatures with overload selection, an explicit size per map over `juce::FixedSizeFunction`, `Function::Array` deleted, `ViewManager::Events` a variadic map keyed by `juce::Identifier` (ARCHITECT: *"prefer the latter"*).
- `DEBT-20261108T145906` — Ctrl+L destroys the Claude Code prompt on the normal screen (unchanged).
- `DEBT-20261105T000000` — Windows runtime test (unchanged).

---

## Sprint: The Theme File Is the Style Sheet in Markdown — One Mirror per CSS Block, CssWriter Feeds the CSS Parse, Fixed-Signature Function::Map ✅

**Date:** 2026-11-09 to 2026-11-10
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"theme.md user should not care about hierarchy. fonts is fonts, colours is colours, metrics is metrics."*; *"there should be only ONE metrics table"*; *"DESIGN BY CONTRACT UNIVERSAL and GENERIC. no harcoding of assumptions, that only works for this pattern. must be ABSTRACTION"*; *"what is tree with key? what the fuck is this new bullshit foreign"*; *"dont fix the symptoms. wrong name means wrong mental model ... fix the cause, fix the design"*; *"if ConfigDocument need to be rewritten from scratch as SettingsDocument to be BLESSED compliant, FIX IT NOW. no more hack and patch, no hardcoding, no workaround"*. The plan was approved through plan mode. During the run: *"if all args are always juce::ValueTree&, why not define it as jam::Function::Map<juce::Identifier, juce::ValueTree&> events; ? then add could be auto deduce?"* (option "Events: fixed-signature map" chosen); *"can we add to CODING, template with erased type such as <..., void> only allowed for different return args?"*.

### Agents Participated
- COUNSELOR — the reads of ConfigDocument, the Validator, CssDocument, StyleManager, the END and EVE theme lanes, `jam::Function` and `juce::FixedSizeFunction`; the plan copy; SPEC, ARCHITECTURE and CODING edits; the `getThemeTree` signature fix; the `user-modules-info.md` rows; the filter of 28 audit findings and the doxygen findings; this log.
- Pathfinder (haiku) — the rewrite inventory; the doxygen pass over eight headers; two `Function::Map` census runs (END with JAM, JFS with KANJUT; still running at this log).
- Engineer (sonnet) — Steps 1–4, Step 5, Step 6, the lossless-mirror batch, audit fixes A (stopped by the watchdog at 15 minutes and resumed) and B, the final two fixes, the `Function::Map` specialization and its callers, the DRY and header-body fixes.
- Auditor (opus) — one run, 28 findings, 5 blocking.

### Files Modified
- `jam/jam_markdown/document/jam_ConfigDocument.{h,cpp}` — the sheet emit and the sheet parse are deleted (`getSheetType`, `getPropertyKey`, `getComponentWords`, `addSubTable`, `###` tables, COMPONENT, lexicon guessing, `getColourValue`, the `em` UNIT child, the colour type guess). `toDocument (const CssDocument&)` writes one table per section: `colours` (palette + colour-id rows), `fonts` (face rows + font-id rows, attributes from the fonts section on the first face), `metrics`, `window`, one per class; record columns (`dark`, …) come from the section's record children. `getAliasKey` is the one lexicon rule for the writer and the Validator. `getRowKey` is public; `isRowType` is new. `toDocument (const juce::ValueTree&)` stays for `jam_Profiler.cpp:172`.
- `jam/jam_markdown/document/jam_CssWriter.{h,cpp}` (new) — `CssWriter : Document::Writer` writes the css text of a theme document. `CssDocument::parse` of that text builds the sections, so the css document is the only builder of its sections.
- `jam/jam_markdown/document/jam_ConfigValidator.{h,cpp}` — the `alias` rule (key from `getAliasKey`; each value or record cell names a colour row or a face row); `row` split into `row`, `row.key`, `row.numbers`, `row.value`, `row.src`, `row.type` (type change against the default row); palette, face and alias rows need no default row; a `colour` cell is hex in every table; named boolean terms.
- `jam/jam_markdown/jam_markdown.{h,cpp}`, `jam/user-modules-info.md` — the CssWriter submodule rows and includes.
- `jam/jam_web/css/jam_Css.{h,cpp}` — the COMPONENT section is deleted; face records are typed `Format::toType` of the family (the `addWidgets` form); `getSheetType (styleSheet)` is new.
- `jam/jam_core/function_map/jam_Function.h` — partial specialization `Map<KeyType, ReturnType (Args...)>` over `jam::HashMap<KeyType, std::function<ReturnType (Args...)>>`: `add (key, fn)` with no template argument, `get` with no `dynamic_cast`, `contains`.
- `jam/jam_plugin_bootstrap/layout/jam_PluginEditorLayout.{h,cpp}` — `getStyleTree`, `getThemeTree (defaultDocument)`, `getSettingsTree (…, getTree)`; `getThemeSettings` writes `toDocument` of the style sheet that `StyleManager::getStyleSheet()` names (`std::find_if`, no sentinel); under END only `getThemeRules()` apply; the theme default parses with origin `files::theme`.
- `end/Source/config/ConfigTheme.cpp`, `ConfigDirectory.{h,cpp}`, `ConfigModel.{h,cpp}` — `addFileTables` and `addDefaultTables` take `getDefaultDocument` and `getTree`; `getStyleTree` is `CssDocument::parse (CssWriter {}.getText (document)).getValueTree (THEMES, sheet type)`; `getDefaultText` (no memo); `getConfigTree`, `getDefaultDocument`; `saveToPath` body in the `.cpp`.
- `end/Source/config/display.md` — `## tab` and `## pane` moved here from `theme.md`. `end/Source/config/theme/gfx/theme.md` — front matter only.
- `end/Source/Main.{h,cpp}`, `lookAndFeel/ENDLookAndFeel.{h,cpp}`, `ui/ENDView.{h,cpp}`, `ui/EditorView.{h,cpp}` — events maps are `jam::Function::Map<juce::Identifier, void (juce::ValueTree&)>`; `add` has no template argument.
- `end/SPEC.md` (Theme style, Theme on disk, Theme load, Pane padding), `end/ARCHITECTURE.md` (Config Chain), `end/PLAN-style-driven-theme.md` (the locked plan with Execution facts).
- `~/.carol/CODING.md` — CRITICAL RULES and checklist: "Uniform signature in the type".
- Doxygen pass over the eight headers above.

### Alignment Check
- [x] BLESSED principles followed — one Model (`CssDocument`), two serializations; the owner builds its sections; no tier is derived in ConfigDocument.
- [x] NAMES.md adhered — `CssWriter` joins `MarkdownWriter`; `getAliasKey`, `getSheetType`, `getStyleTree`, `getThemeTree`, `isRowType` join their families; the `Map` specialization adds no name.
- [x] MANIFESTO.md principles applied.
- [x] No file under `jam_vulkan`; no change to `jam_StyleManager`, KANJUT or JFS; no version bump.
- [ ] Not built. No agent ran `cast` or a build.
- [ ] Zero doxygen warnings unconfirmed; no index was regenerated.

### Problems Solved
- **The tiers were guessed back.** The emit started from the collapsed `getValueTree` tree and rebuilt the css tiers with predicates, COMPONENT and `###` tables. Now the markdown is written from the sections and read back through the css parse.
- **Two metrics tables, sheet hierarchy on disk.** `theme.md` has `## colours`, `## fonts`, one `## metrics`, `## window` and one table per class, with plain words.
- **The reader mutated a finished `CssDocument` from outside its owner** (Auditor 5). `CssWriter` + `CssDocument::parse` replaced it.
- **EVE inside END rejected END's theme** (Auditor 1): the reference rule compared END's palette with EVE's. Under END only the theme rules apply.
- **Audit, resolved:** 1–12, 15 (record columns), 16, 17, 19–23, 25–27. **Rejected with the read:** 13 (a pair of handles, MANIFESTO "A scalar, a handle ... does not copy"), 14 (`style.md:114` has no `colour id` suffix), 15 rest (YAGNI; `toType` tree identity, `jam_Css.cpp:2183`; plan excludes page/element), 18 and 28 (work buffers in creation), 24 (ConfigDocument is one object after the reader moved; CODING "Never make a small .cpp to meet 300/30/3").
- **Doxygen pass findings:** DRY `isRowType` and the header body of `saveToPath` resolved; the `atRuleTypes` instance, the `add` container parameters (NAMES Rule 1 `addToken` example), a bare line count and a two-copy list rejected.
- **Mistakes owned.** I briefed `getThemeTree()` with no parameter, so it parsed the defaults twice; I fixed the signature. I called the type-erased `Function::Map` a breach of fail-fast; ARCHITECT corrected it: the `dynamic_cast` assert is deterministic. I copied the `add<…>` family shape into my own briefs without testing it against BLESSED.

### State for Continuation
- ARCHITECT runs `cast` in jam, eve and end, builds all three, and deletes `~/.config/dev/themes/gfx/theme.md` once (same version, never overwritten).
- Check first at compile: `jam::HashMap::insert ({ key, std::function })` in the `Map` specialization (`jam_Function.h:347-373`); the `getThemeTree (defaultDocument)` declaration (`jam_PluginEditorLayout.h`).
- Runtime: a fresh `theme.md` (tables above); a palette colour, a colour id, a face height and a metric edited live; a corrupt row renders defaults with the error; hosted EVE under END with END's colours; Markdown and Mermaid colours and fonts; dark appearance.
- Run `<leader>bd` in jam and `ninja doxygen` in eve and end; confirm zero warnings.
- The `Function::Map` census (END with JAM, JFS with KANJUT) is running. Its result decides whether uniform maps store `juce::FixedSizeFunction<N, R (Args...)>` (`juce_FixedSizeFunction.h:96-101`, no allocation) and the buffer size `N`.

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261108T145906` — Ctrl+L destroys the Claude Code prompt on the normal screen (unchanged).
- `DEBT-20261105T000000` — Windows runtime test (unchanged).

---

## Sprint: END Theme on the Style Contract — style.css Is the One Source of Fonts, Colours and Metrics, the Markdown Emit Is the Inverse of the Parse, Style Reads the Disk Tree ✅

**Date:** 2026-11-09
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"previous sprint counselor tried to fix theme.md config contract because it was messy"*; *"it's either we fix this mess, or we could start from HEAD"*; the config contract: *"ConfigDocument should not doing any garbage defensive programming with bullshit jassert"*, *"error should never be failure. carried binary always provide the default fallback. validation is always Document::Validator job"*, *"binary write to virgin state to disk, disk always win."*, *"if disk was invalid, rendering never fails ... BUT Validator must show error."*; the css contract: *"css contract: ALL RELATED STYLE SSOT - fonts - colour - metrics"*, *"StyleManager build the map - StyleCustom consume downstream, style related `config` always through Style, never by poking config directly."*, *"config - file watcher listener events - style changed - component repaint"*; the format contract: *"md must be using plain words without any redudancy of `colour id`, `font id`"*, *"the contract is at CALLEE. starts from SSOT cast table manifest ... the problem is INCONSISTENCY"*; *"why both options are garbage? why there are shadow state?"*; *"there are other agents working on Vulkan at jam ... be VERY CAREFUL"*. The plan was approved through plan mode.

### Agents Participated
- COUNSELOR — the reads of StyleManager (jam and KANJUT), the theme lane, ConfigDocument and Css; the plan; Step 1 and Step 8 edits; every read-back; the filter of 49 audit findings; this log.
- Pathfinder (haiku) — tree state and diffs of eight repos, StyleManager history, plans and logs, HEAD `theme.md`, two inventories, the removed emit code, the doxygen pass.
- Engineer (sonnet) — the emit inverse and the `getOrCreate` overload; the style data, ConfigTheme and consumers; the audit fixes; the plan copy.
- Auditor (opus) — one run stopped by the watchdog at 15 minutes (one run held six traces), then two partitions, 21 and 28 findings.

### Files Modified
- `jam/jam_markdown/document/jam_ConfigDocument.{h,cpp}` — no `jassert`. The emit is the parse's inverse again: `getKeyWords` drops the component words and the id suffix, `getCellText` writes an alias in words, `addSheetTables` writes `### colours`, one table per component, `### fonts`, `### appearance` and `### metrics` under one `##` sheet; a table with child rows writes its properties as `### metrics`. `hasIdEntry` knows an id key by lexicon membership, because `text button text colour off id` (`cast/style.md:114`) does not end with `colour id`. An empty dark cell parses as the light value. `getOrCreate (name, text)` memoizes a document parsed from given text. Doxygen synced.
- `jam/jam_markdown/document/jam_ConfigValidator.cpp:231-237` — a font `src` that names no default file is a validation failure (`failUnknownReference`), so a bad disk file never reaches `StyleManager::getOrCreateTypeface`.
- `jam/jam_style/jam_StyleCustom.{h,cpp}` — `getDefaultScrollbarWidth` has no `isEND()` branch; the `scrollBarThickness` metric of the style sheet is the one path.
- `jam/jam_web/css/jam_Css.{h,cpp}` — the comma-list attribute and `getStyleAttributeType` are deleted.
- `end/Source/config/theme/gfx/style.css` (was `gfx.css`) — the three faces whose file END does not ship are deleted (`symbols`, `--notoSymbols`, `--notoEmoji`: the crash at `jam_StyleManager.cpp:105-107`); `.window` (`blur`, `mac`, `win`); `--ResizableWindow--backgroundColourId`; tab and pane metrics; `tab` and `overlay` faces. The file name makes the sheet `STYLE`, the sheet hosted EVE reads.
- `end/Source/config/theme/gfx/theme.md` — only rows that are not a font, colour or metric: `## tab` (`position`, `always visible`, `uppercase`), `## pane` (`split line`).
- `end/Source/config/ConfigTheme.{h,cpp}` — `getOrCreateDefaultText`: carried `theme.md`, a newline, then the markdown built from `style.css`, once per file name (the canon form `kuassa_PluginEditorLayout.cpp:601-603`); the virgin write uses it.
- `end/Source/lookAndFeel/ENDLookAndFeel.{h,cpp}` — StyleManager is created over the THEMES tree; the `theme` event runs `registerStyle`, `applyColours`, `applyFontStyles`, `loadGraphics`; fonts, metrics, window and colours read through StyleManager; the per-table events and the menu opacity read are deleted. Doxygen synced.
- `end/Source/Main.{h,cpp}` — `initialiseStyle` is the one init call. Doxygen synced.
- `end/cast/files.md`, `end/cast/identifiers.md` — `style.css`; eight metric ids added, seven ids with no reader deleted.
- `end/SPEC.md:164-`, `end/ARCHITECTURE.md` (engine lifecycle, thread table, config chain), `end/PLAN-style-driven-theme.md` (replaced by the locked plan).

### Alignment Check
- [x] BLESSED principles followed — one source per row, one runtime tree, one flow for init and reload.
- [x] NAMES.md adhered — `hasIdEntry` joins `isIdEntry`, `getOrCreateDefaultText` joins `getOrCreate`, the metric ids join `scrollBarThickness`.
- [x] MANIFESTO.md principles applied.
- [x] No file under `jam_vulkan` and no change to `jam_StyleManager`.
- [ ] Not built. No agent ran `cast` or a build.
- [ ] Zero doxygen warnings unconfirmed; the jam XML still names `getStyleAttributeType`.

### Problems Solved
- **Two truths for the theme.** Embolden and scrollbar thickness lived in `theme.md` and in `gfx.css`; StyleManager held a css-only tree beside the config tree. Now `style.css` holds every font, colour and metric, the disk file is built from it, and StyleManager registers the one THEMES tree on every change.
- **SCREAMING values and `colour id` keys on disk.** The css parse stores a reference as a tag (`jam_Css.cpp:411-412`) and the emit wrote it raw; the previous sprint had deleted the emit's suffix strip. The emit and the parse are inverses again, at the callee.
- **No colours a user can override.** The previous sprint's hand-written `theme.md` had no colour table. The built file has the palette and every component table.
- **Audit, resolved:** A-F1, A-F2 (lexicon membership), A-F10 (dark column), B-1 (font file validation), B-3 (empty dark cell), B-5 (default text per name), B-11 (one init flow).
- **Audit, rejected with the read:** B-6 — `Main.cpp:61-65` cascades on `Id::theme`, after the LookAndFeel listener (`juce_ListenerList.h:403-413`). B-12 — `setAppearance` changes the laf colours, not `colourScheme`, which `loadGraphics` reads (`ENDLookAndFeel.cpp:500`). A-F8 — every StyleManager lookup applies `Format::toType` (`jam_StyleManager.cpp:83`, `jam_StyleManager.h:195`), so the key case of the css tree and the disk tree meet.
- **Mistakes owned.** I first reported a StyleManager contract change, then corrected it: the KANJUT differences came from the locked isomorphism plan. I offered a HEAD-or-fix disposition and a row-source question that the contract answered. My first plan kept hand-authored style rows in `theme.md`.

### State for Continuation
- ARCHITECT runs `cast` in jam, eve and end, builds all three, and deletes `~/.config/dev/themes/gfx/theme.md` once (same `version: 0.0.1`, never overwritten).
- Runtime checks: a fresh `theme.md` with `## style` and its `###` tables; a colour, metric and font height edited live; a corrupt file renders defaults with the error; hosted EVE with END's colours; Markdown and Mermaid fonts and colours; EVE standalone writes its own file in the same shape; dark appearance.
- Run `<leader>bd` in jam and `ninja doxygen` in eve and end; confirm zero warnings.
- Audit findings not resolved, each read and real, larger than this plan: A-F3, F4 (component and dark loss for font-only or palette rows), A-F5 (component tables also hold metrics, and an `### appearance` table exists, against Plan Step 2.4 wording), A-F6, F7 (font-lexicon guess outside `### fonts`), A-F9 (font numbers as strings), A-F11 (units other than `em`), A-F12 to F28 (Lean, DRY and form in `jam_ConfigDocument.cpp` 719 lines and `jam_ConfigValidator.cpp` 382 lines), B-2, B-4 (hosted EVE overlay drops nested children), B-7, B-8 (popup `SafePointer` and `callAsync`), B-9, B-10 (rasterizer assert), B-13 to B-21 (repeats, sentinel, forms, duplicate `theme`/`themes` ids).

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261108T145906` — Ctrl+L destroys the Claude Code prompt on the normal screen (unchanged).
- `DEBT-20261105T000000` — Windows runtime test (unchanged).

---




