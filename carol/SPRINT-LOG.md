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

## Sprint: Finder Parity — Preview through the Terminal Protocol, openFile, File List, Markdown Panel, OPEN and LIST Modes, No Count, jam::Array ✅

**Date:** 2026-11-05
**Duration:** one session across a context compaction. The start and end times are not recorded.
**Scope (ARCHITECT):** the six plans `PLAN-1-preview-protocol.md` to `PLAN-6-list-mode.md`. *"i actually think Preview should be the first to implement CORRECTLY, and wire thorugh the whole terminal protocol"*. *"we dont use "count" we use "num""*, *"no count anywhere. must be consistent"*, with the option *"Every count name in EVE and JAM becomes num, and END, TIT, CAKE and WHATDBG follow. External contract strings stay"*. *"dont use vector use jam::Array if possible anywhere"*. *"you can use jam::Array as sub for HeapBlock"*. *"and also implement for windows. runtime test still need to be tested when i get to windows. add to DEBT. implement now"*. *"complete outstanding until /log no gate"*.

### Agents Participated
- COUNSELOR: the six plans; the protocol reads; each delegation; validation of each return; the SPEC and grand scheme amendments; the triage of each audit finding by a read; this log.
- Engineer (sonnet): the code of each plan; the count rename; the `jam::Array` sweep; the audit fixes.
- Pathfinder (haiku): targeted reads; the old panel trim from `326387d`; the consumer check of the changed types; the doxygen pass in two scopes.
- Auditor (opus): one run over PLAN-2 to PLAN-6 and the two sweeps. PLAN-1 had its own run before ARCHITECT's *"complete outstanding until /log no gate"*.

### Files Modified
- PLAN-1, confirmed by ARCHITECT's runs (*"works"*, *"all works"*): `jam/jam_gui/widgets/jam_PreviewComponent.*` replaces `jam_Preview.*` and `jam_PreviewOverlay.*`: one component paints every box in one call, `FrameJobs` decodes in parallel, one vblank clock runs each GIF. `jam/jam_terminal/image/jam_ImageDocument.*` replaces the GIF metadata headers: one complete document per image, with one child per frame. `jam_TextEditor.*`: `addPreview`, text paints in `Content::paintOverChildren`, `setImageBounds`, `setPreviewBounds`, the `images` tree. `jam_TerminalProcessor.*`: the `previewBox` row, the EVE preview arm, the alternate screen and blank cell rules. `jam_terminal/image/native/*`: `loadImageNative`, software images, `ComApartment` on Windows. `jam_core/text/jam_Format.*`: `toNativePath`. `jam_plugin_bootstrap/view/jam_ViewManager.h`: child snapshot in `applyToComponentRows`. `eve/Source/shell/eve` on `PATH`; `eve/Source/ShellIntegration.*`, `EVEView.cpp`, `EVETerminalProcessor.cpp`, `layout/component.md`, `layout/parameters.md`, `cast/files.md`.
- PLAN-2: `jam/cast/core.md`, `gui.md`; `jam_markdown/document/jam_ConfigDocument.cpp`: a table with an `any` row takes any key. `jam_TextEditor.*`: `applyTerminalMouse`, `applyCellMouse` with `selectionThreshold { 2 }`, `applyLink`. `jam_Format.*`: `appendQuoted`. `jam_TerminalProcessor.*`: public `getUriPath`, `applyToDecodedText`. `eve`: the `## open with` table, the `url` row, `openFile`, `getOpenCommand`, `appendPath`, `addInput`, `applyOpenCommand`.
- PLAN-3: `jam_AnsiDocument.*`: `addFileLinks`, `getText`, `isGridLine`, `isBlank`, `getEntryEnd`. `jam_TextEditor.*`: `getLastOutputBlock` reads the `outputBlock` row, `isLinkChanged` in `addBlocks`. `jam_TerminalProcessor.cpp`: the `outputBlock` report. `eve`: the `outputBlock` row, the `directory` row.
- PLAN-4: `eve/Source/layout/component.md` row 2, `EVEView.*`: `openMarkdown`, `setMarkdownVisible`, `applyToMarkdown`, `keyPressed`. `jam/cast/gui.md`: `EditorMode` `panel`. `default-eve.md`: `dismiss`, `.md` takes `markdown`.
- PLAN-5: `jam/cast/gui.md`, `terminal.md`, `spell.md`, `style.md`: `open`, colour rows, sizes 28. `jam_TextEditor.*`: `applyToVisibleLines`, `applyToVisibleLinks`, `applyLinkLabel`, `paintLinkLabels`, `Id::page`. `jam_ENAXClientExtensions.cpp`, `end/Source/config/keys.md`, `end/cast/bimaps.md`, `end/Source/ui/ENDView.cpp`, `EditorView.cpp`. `eve/Source/textEditor.css`, `layout/parameters.md`.
- PLAN-6: new `jam/jam_markdown_graphics/component/jam_FilePreview.*`. `jam_PreviewComponent.*`: `bindToImages`. `jam_TextEditor.*`: `getLink`, `getListEntry`, `moveListEntry`, `setCursorToTerminal`, `paintColumns`, `paintListEntry`. `EVEView.*`: `panels`, `directoryThread`, `directoryList`, `addPanels`, `setPanelsVisible`, `setPanelFile`, the trim in `resized`, `juce::FileBrowserListener`. `default-eve.md`: `tree depth`, `tree width`.
- Count rename: about 240 names over about 150 JAM files, 4 EVE sources, the cast tables and `user-modules-info.md`, then bare `count` in about 40 files. Examples: `Id::count` to `numRepeats`, `Grapheme::Entry::count` to `numCodepoints`, `HashMap::count` to `num_entries` (the fork keeps its snake case, NAMES Rule 5). Consumers changed where they use a JAM name: `end/Source/ui/ENDView.cpp`, `SessionView.cpp`, `lookAndFeel/ENDLookAndFeel.cpp`, `ARCHITECTURE.md`, `SHADERS.md`, `ggwp/Source/Build.cpp`.
- `jam::Array` sweep: 133 declarations over 45 JAM files: `jam_graphics`, `jam_fuzzy`, `jam_animation`, `jam_dsp`, `jam_terminal`, `jam_clap`, `jam_core`, `jam_gui`, `jam_data_structures`, `jam_pdf`, plus `jam_VulkanMesh.h`. `endless/Source/terminal/action/ActionListSelection.cpp:40` takes parentheses.
- `eve/SPEC.md`, `README.md`, `PLAN-Grand-Scheme.md`, `CLAUDE.md`, `DEBT.md`, `PLAN-1` to `PLAN-6`.
- Doxygen: each new declaration and each stale block in the sprint scope.

### Alignment Check
- [x] BLESSED principles followed: a CLI signal is a Model row; widgets react to `juce::ValueTree` events; one component paints every image in one call; one command opens every target.
- [x] NAMES.md adhered: each new name joins a family. `applyUrl` became `setPanelFile`, because `apply` pushes onto a target that the caller supplies.
- [x] MANIFESTO.md principles applied
- [ ] Not validated by a compile. Only PLAN-1 was built and run. PLAN-2 to PLAN-6, both sweeps and the audit fixes have seen no compiler. The doxygen zero-warning check has not run.

### Problems Solved
- **Terminal images on a Component.** ARCHITECT rejected the placement model: *"this is wrong mental model"*. A CLI signal is a registered row, and the widget only listens. The native lane sends a file as metadata, so no bytes cross the PTY: *"why not just the file as metadata? suffice?"*. The third-party lanes stay for compatibility. fzf treats any `ESC]1337;` as a full-window image and exits with `CSI J`, so `setBlankCells` clears the box.
- **Isomorphism.** `ImageDocument` is the AST like `AnsiDocument`, the decoded frames are the runs, the `images` tree is the Model, and paint is one call. ARCHITECT: *"one component per image is costly"*.
- **A parent is not an owner.** ARCHITECT: *"juce::Component hierarchy is decoupled from ownership"*. The view table owns the component, and `bindTo` reparents it into the content. `applyToComponentRows` reads a snapshot, because the reparent shifts each later z index. The panel lookup uses the `type` property for the same reason.
- **Decode delay.** `FrameJob` decodes each GIF frame on its own pool, so the first frame shows at once and a same-pool wait cannot lock.
- **File list.** OSC 133 D scrolls the output into history (`SPEC.md`, Resize), so the links build at creation in `addBlocks`. `history.erase` from PLAN-3 Step 3 is not used, because `addBlock` treats an absent block as new and shifts the cursor. `isLinkChanged` compares the block's stored number with the row.
- **A command with no output brought the old list back (audit H3).** The fold over resident lines also ran on each vblank (H2). The processor's `outputBlock` became one Model row, so one read replaces the walk, and `true` or `cd` clears the list.
- **Null read (audit H1).** A file that opens with an error keeps its range with a null address (`juce_SharedCode_posix.h:585`). `FilePreview::setFile` and `openMarkdown` test `getData()`, the shape of `jam_PreviewComponent.cpp:242`.
- **Winsize in LIST (audit M1).** `resizeEnd` and each animator change cleared the gate. `resizeEnd` now keeps it while a panel shows, and `setPanelsVisible` sets the bounds, then the gate.
- **No temporary container (audit M4, L6).** `getVisibleLinks` from PLAN-5 built and sorted an array on each paint. `applyToVisibleLines` and `applyToVisibleLinks` walk in place, and `applyToVisibleImages` uses the same walk.
- **Audit findings also resolved.** M3, M6, M7, L1 to L7, L10, L11, L12 (bounds), L13, L14, L17, L20 (`applyUrl`), L21 to L24, and the link discovery part of M9: `AnsiDocument::addFileLinks` now holds the filesystem test, so the view holds none.
- **Audit findings rejected, each with its read.** M2: `getVisibleLinks` holds only rows in view, so `j` past the view needs the line walk. M5: `isPanelShowing` is framework API, and the panel can differ from the mode. M8: PLAN-2 Step 5 and PLAN-4 Step 3, and the view holds no state. M9, the rest: MANIFESTO L, *"decomposing it would only relocate the same lines across more files"*, and the earlier ruling in this log. M10: `EVEView.cpp:12` writes before the listener exists (`jam_PluginEditor.cpp:210`). L8: `getMotions` and `getActions` are the lookup tables that MANIFESTO L asks for. L9: `TerminalSwap::linesPerBlock` bounds one build. L12 `isNew`: the three sites hold different payloads, and one helper would be a carrier. L15: `panels` is empty by design until the first LIST. L16: both sites call the one pair `getUriPath` and `applyToDecodedText`. L18: the z index shifts. L19: the row exists and is used. L20, other names: each was verbatim in a delegation prompt.
- **Sweep rows kept.** `jam_HashMap.h` (allocator parameter), `Tokens` and `tokenGroups` (copied into a job), `ResourceFetchers` (a const lvalue is copied), `BrickWall::getCoefficients`, `FrequencyGrid::getBandRanges`, the `FrequencyGrid` tick tables, `SimdStackBlur` history (`jam_Array.h:797`), `juce::Typeface::getLayersForGlyph`, two `vk::ResultValue` returns, `juce::ColourGradient::createLookupTable`.
- **Rename rulings from the read.** The unit string `"count"`, the message `count differs from the default: `, `FrameCount`, `frame_count_mod` and CSS `counter` stay as external strings. The `counter` family and `isCounterClockwise` stay: they are not `count`. English prose stays. Consumer-own names stay, by the option text: *"END, TIT, CAKE and WHATDBG follow"*.
- **Mistakes owned.** The doxygen protocol skipped for `Model::Component`, with a false absence claim. The bytes recommendation. Text out of the terminal model. *"you delegate"*, after I pushed work back. One component per image. A watchdog armed late. A delegation prompt that said `carol apply --expect 0` writes nothing: on a file with no match it writes the same bytes, so Pathfinder rewrote 279 consumer files with identical content, and only their modification times changed.
- **Watchdogs.** The array inventory and the `jam_graphics` sweep reached 15 minutes and were stopped. The cause was a read of every site in one prompt, and `carol apply` retries. Each resumed with a narrow task. ARCHITECT stopped the run once at a watchdog call and resumed with *"continue"*.

### State for Continuation
- Run `cast` in `jam`, then `eve`, `end`, `ggwp` and the other consumers: the `Id::` renames, `numRepeats`, `outputBlock`, `url`, `openFile`, `markdown`, `dismiss`, `open`, `list`, `listDown`, `listUp`, `created`, `modified`, `treeDepth`, `treeWidth`, `directory`, `any`, and the colour rows need it. Until then the generated headers hold the old names.
- Then build `jam`, `eve`, `end`, `endless`. Expect compile errors. No compiler has seen PLAN-2 to PLAN-6, the rename or the sweep. Most risk: `jam_TextEditor.h` templates (`applyToVisibleLines`), `jam_Rotary.*`, `jam_Filter.h` loops, `jam_BonusMatrix.*`, `jam_AnsiDocument.cpp` header order for `juce::URL`, the `EVEView.cpp:12` use of `map::EditorMode::panel`.
- 279 consumer files have new modification times. A build rebuilds them.
- ARCHITECT tests, in plan order: the Acceptance list of PLAN-2 to PLAN-6. First: a click after `ls`, `` ` `` `o` labels, `` ` `` `l` panels with no SIGWINCH, `.md` panel with `escape`, hot reload of the theme in the panel.
- The first LIST show animates from an empty rectangle: `juce::SidePanel` has no bounds until it is shown (`juce_SidePanel.cpp:109-121`). Read the result on a run.
- `ls sub/` lists names that do not resolve against `cwd` (PLAN-3 Risks).
- Run `<leader>bd` and `ninja doxygen`, and confirm zero warnings. LSP on JAM `.cpp` files gave wrong targets in this session: check `.clangd`.
- `PLAN-1` to `PLAN-6` stay until ARCHITECT's run closes them.
- `jam_TextEditor.cpp` `perform` keeps `isUsed`: a statement runs after it, so no result return fits. `Noise::Brown` and the fuzzy finder have no caller in EVE.
- Windows: nothing in this sprint has built or run there.

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261105T000000` — Windows runtime test of the preview path, by ARCHITECT: *"runtime test still need to be tested when i get to windows. add to DEBT."*

---

## Sprint: Terminal Reports — Tab Name and Cwd from the Pane, OSC and DEC Reports on the Tick, ENAX Pane UUID, Audit of Four Sprints ✅

**Date:** 2026-11-04
**Duration:** one session, after the cell width log. The start and end times are not recorded.
**Scope (ARCHITECT):** `/pay` of `DEBT-20261103T233912`. *"we must add cwd as EVE parameter, AND END also track last focused EVE (pane). so cwd always synced to focused pane. new instance of tab, split, any pane, always inherit cwd from previous focused pane."* *"TAB name priority in order: - title, whatever application send - application name, if it's not sending anything. running application means it's ephemeral, always updated whether the app running / stopped (with OSC 133?) - cwd - plugin name"*. *"ok link fold. but keep the callback stubbed"*. *"END is not routing report to its pane, it TELLS its pane, a parameter value, such as cwd, all event driven"*. `/audit 4 (from the whole session)`. *"add field to ENAX, pane UUID == plugin UUID"*. *"ONLY ASK QUESTION NO READ CANT ANSWER"*. *"no new pattern, no new semantics, follow established pattern verbatim"*. *"why there is hardcoded registerEVE? ENAX plugin is not only EVE"*.

### Agents Participated
- COUNSELOR: the plan; the triage of each Auditor finding by a read; validation of each return; the doc edits for `end/SPEC.md`, `end/ARCHITECTURE.md`, `PLAN-terminal-reports.md`; this log.
- Pathfinder (haiku): the Claude Code binary probe; the doxygen pass in three scopes.
- Librarian: the prior art in Ghostty, kitty, iTerm2, Alacritty; the PSReadLine API.
- Engineer (sonnet): the plan steps; audit groups T, F, N, E, D, M, R, G, N2. Groups S1, S2 and S3 were stopped and reverted.
- Auditor (opus): three runs, one per code domain, over the three logged sprints of this session and this sprint.

### Files Modified
- `jam/cast/terminal.md`, `core.md`, `spell.md`: `OscFunction`, `oscFunctions`, `PrivateMode`, `privateModes`, `ImageFunction`, `imageFunctions` rows; ids `background`, `clipboard`, `cwd`, `foregroundProcess`, `hyperlink`, `syncOutput`, `enax`; strings `enaxPaneUuid`, `enaxStartDirectory`, `enaxTerminalReport`.
- `jam/jam_terminal/processor/jam_TerminalProcessor.h`, `.cpp`: OSC 0, 2, 7, 8, 11, 52, OSC 133 C and D reports, XTVERSION, DECRQM, DEC 2026 hold. `transitImages` keeps the number of each image in transit. `process` takes the background colour as a calculation input, so no out-parameter remains. `getReports` is the one list of the three report ids.
- `jam/jam_terminal/document/jam_TerminalSwap.*`, `jam_TerminalImageRecord.cpp`, `jam_AnsiDocument.*`, `jam/jam_core/text/jam_AttributedString.*`: link records take the owner line of each row that holds them. `Link` holds `juce::String`, so a copy is one reference count. `setLinks` and `applyLinkUnderline` are two calls.
- `jam/jam_data_structures/model/jam_ParameterText.*`: `maxTextLength`; `store` cuts at a codepoint boundary, and it no longer asserts on terminal input.
- `jam/jam_terminal/tty/*`, `eve/Source/TerminalMemoryTTY.*`: `programName`, `isOpen`.
- `jam/jam_plugin_bootstrap/enax/jam_ENAXClientExtensions.*`: `paneUuid`, read once at construction. The terminal report and start directory rules carry it. The `Id::enax` report marks the pane. A start directory applies only when it is a directory.
- `jam/jam_gui/layout/jam_MatrixComponent.*`: `getPanePosition`, `getOppositePane`, `isCrossRow`, `isRotatable`, `applySeamProportions`. `jam/jam_gui/commands/jam_CommandManager.*`: `setInfo`, `addCommands`.
- `jam/jam_clap/format/jam_ClapPluginFormat.cpp`: flat owned CF handles, no raw `new`, the right factory message.
- `jam/jam_graphics/fonts/jam_GlyphArrangement.*`, `jam_Typeface.*`, `jam_GlyphRasterizer.*`, `jam/jam_graphics/engine/jam_CpuGraphicsEngine.h`, `jam/jam_terminal/buffers/jam_TerminalBuffer.*`, `jam/jam_style/*`, `jam/jam_gui/widgets/jam_TextEditor.*`: the cell source and the `Lookup` and `Measure` abstractions are deleted, because each had one implementation. The ligature cache moved to `jam::Typeface`. A wide atom with no glyph advances two cells. `lookAndFeelChanged` runs `resized`, so `--kerning` hot reload reaches the winsize. A view at the content end has offset 0. Rasterizer names follow the verb set.
- `end/Source/plugins/Nexus.*`, `Session.*`, `end/Source/action/ENDActions.h`, `end/Source/ui/ENDView.*`, `EditorView.*`, `TabView.*`, `SessionView.*`: the pane row holds `title`, `foregroundProcess`, `cwd`. `loadingPanes`, `getLoadingPane`, `getPaneRow`, `getFocusedPane`, `getInheritedDirectory`, `isENAX`, `addPluginModule`, `addBundle`, `getDisplayBounds`, `isHorizontal`, `areaOptions`, `splitEdges`.
- `end/Source/config/ConfigDirectory.h`, new `ConfigDirectory.cpp`: bodies and dynamic initialisation left the header.
- `eve/Source/EVETerminalProcessor.*`, `EVEView.*`, `ShellIntegration.cpp`, `Source/layout/parameters.md`, `Source/shell/*`, `cast/identifiers.md`: Model rows, the clipboard handoff, `terminalEvents`, `getShell`, `registerImageEvents`, `cmdline_url` in OSC 133 C, the home folder default, `$([char]27)` for `powershell.exe`.
- `eve/SPEC.md`, `README.md`, `CLAUDE.md`, `end/SPEC.md`, `ARCHITECTURE.md`, `CLAUDE.md`: synced to the code. Deleted: `eve/PLAN-eve-md-cleanup.md`.
- Doxygen: each new declaration and each stale block in the audited scope.

### Alignment Check
- [x] BLESSED principles followed: the tick holds numbers, no string; a report is one Model row and one pane row; the owner holds each module.
- [x] NAMES.md adhered: each new name joins a family. `Id::enax` joins the `enax*` ids.
- [x] MANIFESTO.md principles applied
- [ ] Not validated by a compile. Nothing in this sprint after the first runtime test was built.
- [ ] Doxygen zero-warning check not run: `<leader>bd` for jam, `ninja doxygen` for EVE.
- [ ] Step 11 not run: the raw byte benchmark needs a build.

### Problems Solved
- **Claude Code in EVE.** The decoder had no handler for six sequence families. Each now runs through a table row or a small branch set. A hold ends after 1000 ms (Ghostty `termio/Thread.zig:38`), and a repeated `?2026h` does not extend it.
- **Tab name and cwd.** ARCHITECT confirmed by run: *"so far tab name works, cwd for new pane works"*.
- **Pane identity.** `enaxTerminalReport` is one global function, so END wrote to the focused pane, and EVE sent nothing while unfocused. A background tab kept `cast` in its name. A read of JUCE, the plugin SDKs, the child process and the peer id found no identity both sides see, except CLAP's per-instance `clap_host_t`, and EVE has no CLAP target. ARCHITECT ruled the ENAX field. `Session::loadPlugin` adds the pane to `loadingPanes` before `createPluginInstanceAsync`, and the callback removes it. JUCE posts one message per load and gives one callback per load, so the head names the instance under construction.
- **ENAX is not EVE.** Three gates tested the EVE file. An ENAX plugin now reports `Id::enax` in its constructor through the existing channel, and `EditorView::isENAX` reads the pane row. `registerEVE` stays: it lists the default plugin.
- **Typeface outlived its module.** END's context interns a typeface that a plugin built (`jam_VulkanLowLevelGraphicsContext.cpp:1706`), and JUCE unloads a bundle with retain count 1 (`juce_VSTPluginFormatImpl.h:785-786`). `Nexus::addPluginModule` holds each ENAX module.
- **A `juce::File` crossed ENAX.** The crash `pointer being freed was not allocated` came from END's static empty holder (`juce_String.cpp:172-177`). ENAX carries C types only.
- **Audit findings, resolved.** RIS allocation on the tick; link records with no owner; the wrong record for a Kitty chunk after OSC 8; OSC 52 race and leak; a -1 row reading before the mapping; a long title asserting; the `~` default on Windows; shell scripts rewritten on each resize; `--kerning` not reaching the winsize; a dead or remote cwd; `/a:b` cwd; a bare OSC 7 path; PowerShell 5 escapes; the repeated command boilerplate; raw `new`; 1024 in several owners.
- **Audit findings, rejected, each with its read.** Whole `Pen` on the tick (`jam_AnsiDocument.h:199`). `pendingClipboard` (MANIFESTO permitted copy 2). `:195` reserve test (capacity is the buffer plus the reserve). Fake 1×1 size (named constants, Step 7.3). `:995` narrowing (out of reach). Hold stall (bounded). `links` not pruned, repeated URI, `Format::toType` repeats, `FOO=1 make`, ICH and DCH links, persisted rows (no clause). bash `PS0` (no read). B6 (JUCE's own shape, by ruling). B7, B8 (no path today). B9 (`jam_Model.h:1026`). B12 (no proof). R1 empty holder (`juce_Font.cpp:87`). V7 (twice only). V20, D2, D4, DC2, DC3, T1, T2. L1 (ruling: *"it's MatrixComponent commandId"*). F10 (`jam_MermaidGraphics.cpp:612`). F11 (a state update). F15, F20, F21 (`jam_MarkdownGraphics.cpp:204`). `setInfo` (fills a JUCE struct in place). `jassert` with a positive `if` (`MANIFESTO.md:116-118`).
- **God object findings rejected.** `TextEditor`, `GlyphArrangement`, `TerminalProcessor`, `Nexus`. CODING: *"NEVER POLLUTE codebase with scattered cpp with small implementation to statisfy 300-30-3 GUIDE"*. MANIFESTO L: *"decomposing it would only relocate the same lines across more files"*. The tables are lookups already, and the bodies share one state. `Nexus`: offload and registration are one job, and `extensions` is a table.
- **Mistakes owned.** A repeated process query in place of the terminal report. A forced underline in the atom. A hand-rolled report route. A file drop and a drag split missing from the first cwd path. A question to ARCHITECT for new names, and *"you may overrule"* on findings the contract answers. Nested objects with their own `.cpp`, a new pattern: three Engineers ran on it, were stopped, and the tree was repaired from its own evidence. EVE file gates in the audit fixes.
- **Watchdogs.** One doc sync and one doxygen scope reached 15 minutes and were stopped. Their edits were confirmed by read.

### State for Continuation
- Run `cast` first: `Id::enax`, `Id::enaxPaneUuid` and `ConfigDirectory.cpp` need it. Then build `jam`, `end`, `eve`. Expect compile errors in the code that no build has seen. The `Link` read sites and `ParameterText` visibility from `jam_terminal` carry the most risk.
- ARCHITECT tests: frame batching in Claude Code; OSC 52 copy over 64 KiB; OSC 11; OSC 8 underline; a background tab name after a build ends; a new pane in a third-party plugin pane; `--kerning` hot reload.
- Run `<leader>bd` and `ninja doxygen`, and confirm zero warnings. `doxygen` crashed earlier (`~/Library/Logs/DiagnosticReports/doxygen-2026-11-04-064421.ips:46`).
- Step 11: run the raw byte benchmark with `scout` against the baseline. `PLAN-terminal-reports.md` stays until then.
- Held by ruling: link click and selection are stubs; `FORCE_HYPERLINK` stays off; `ls` link marking at `OSC 133 D` (§3.11); the kerning value; `~/.config/dev/` is temporary.
- `end/Source/config/ConfigDirectory.h:46`, `:52`, `Main.cpp:252`, `Main.h:28`, `jam_ClapPluginFormat.h:109` stay stale, on *"dont change the comment"*.

### Debts Paid
- `DEBT-20261103T233912` — the live decoder handles every listed sequence through its tables. A link is render only, by ruling.
- `DEBT-20261103T235332` — closed by ARCHITECT: *"not issue with bundle id. old binary is dead."*

### Debts Deferred
- None

---

## Sprint: Cell Width from the Unhinted Advance, Whole-Row Viewport, 1024-Unit Fonts, Ligatures from Config ✅

**Date:** 2026-11-04
**Duration:** one session, after the keyboard focus log. The start and end times are not recorded.
**Scope (ARCHITECT):** *"[1] font kerning looks too tight. what option do we have to adjust the spacing for each characters, slightly spaced out? is our calculation wrong? ... ideally we have baseline 0 kerning, this baseline could then overriden via config"*; *"[2] ... look at top most row. there's remants of previous history sometimes left on top ... we should just always snap to cell height, so no cutoff. and no remnants of normal screen at alternate"*; *"config should be wired, AFTER we have correct calculation"*; *"we actually have new fonts to be built /Users/jreng/Documents/Poems/dev/fonts/ all of it now is 1024 base, not 1003 anymore ... build with ggwp"*; *"issue now: no ligatures renderd"*; *"IIUC the spec also ligatures is config driven, so it must be hot reload"*.

### Agents Participated
- COUNSELOR: reads of the metric path, the row path, the FreeType load path, the config overlay and the old ligature code; design; validation of each Engineer return; this log.
- Engineer (sonnet): the cell width, kerning and row snap edits; the ggwp build and install; the ligature code and config row.
- Pathfinder (haiku): the font tables of the old `DisplayMonoBook.ttf`; the git trace of the deleted ligature code.
- Auditor: not run. ARCHITECT: *"do not audit until i explicitly say so"*.

### Files Modified (15 total, and 12 built fonts)
- `jam/jam_graphics/fonts/jam_GlyphRasterizer.cpp:505` — the cell measure loads with `FT_LOAD_NO_HINTING`, so `ceil26Dot6ToPx` (`:537`) rounds the real advance up.
- `jam/jam_style/jam_StyleCustom.cpp:35-48` — `setMonoFont` adds the kerning once to `monoMetrics.cellWidth`. The mono `Fonts` table carries factor 0, so `getFontMeasure` (`jam_GlyphArrangement.cpp:221`) does not add it again. `monoFonts.add` stays before `getOrCreateMetrics`, because the add registers the typeface (`jam_Typeface.cpp:465-467`).
- `jam/jam_gui/widgets/jam_TextEditor.h:432-434`, `jam_TextEditor.cpp:720`, `:1028-1034`, `:1203-1218`, `:1229`, `:1333` — the viewport holds whole rows through `Cell::Rectangle::fromPixel` and `toPixel`. `getRowOffset` gives the distance from the view top to its row top. `Content::paint`, `applyMouse` and `keepCursorVisible` use it.
- `jam/jam_graphics/fonts/jam_GlyphArrangement.h:204`, `:218`, `:489-493`, `:661-665`, `:681` — `ligatureFeature`, `Fonts::hasLigatures`, the three ligature declarations, the three constants, the `ligatures` cache.
- `jam/jam_graphics/fonts/jam_GlyphArrangement.cpp:57-59`, `:275-298`, `:413-472` — `Fonts::add` reads the feature; `addCellEntries` tries a ligature first; `isLigatureCandidate`, `getOrCreateLigature`, `findLigatureGlyph`.
- `jam/jam_style/style_manager/jam_StyleManager.cpp:206-214` — `setAppearance` reads the `ligatures` row and tells `setMonoFont` through `FontOptions::withFeatureEnabled`.
- `jam/cast/style.md:39` — `Id::ligatures`.
- `jam/resources/fonts/` — the six Display and Display Mono fonts, rebuilt with ggwp. All report 1024 units per em. Display goes from 102 to 111 glyphs. Display Mono goes from 1443 to 118 glyphs.
- `eve/Source/layout/default-eve.md:19-20` — `ligatures` row of `## display`, type `bool`, value `true`.
- Outside the repos: `fonts/display/DisplayBook.ttf` and its two siblings, `fonts/display-mono/`, `fonts/display-serif/`, `fonts/display-serif-mono/`, three weights each. All 12 builds exited 0. The Serif fonts have no consumer, so they stay beside their manifests.

### Alignment Check
- [x] BLESSED principles followed: one cell width owner; no shadow scroll state, because `getRowOffset` derives on demand; `getClusterGlyph` stays the one shape.
- [x] NAMES.md adhered: `getOrCreateLigature` joins `getOrCreateFallback`; `findLigatureGlyph` joins the `find*Glyph` family; `getRowOffset` joins `getRowTop`.
- [x] MANIFESTO.md principles applied
- [ ] New declarations hold no doxygen yet, per CAROL.md Code Hygiene. The pass waits for the audit.
- [ ] No Auditor run, on ARCHITECT's word.

### Problems Solved
- **Cell too narrow.** `measureMaxCellWidth26Dot6` loaded glyphs hinted. The font has no bytecode, so FreeType autohints it (`ftobjs.c:969`) and rounds the advance (`afloader.c:567`). Display Mono gave 7 for a design advance of 7.36 at size 12. The cell is now 8. The new font has the same ratio, 628/1024.
- **`--kerning` was half wired.** It widened each cell (`jam_GlyphArrangement.cpp:221`), but `setWinsize` used the plain `cellWidth` (`jam_TextEditor.cpp:1053`), and a wide character got one share. It now lives once in `monoMetrics.cellWidth`.
- **Top row remnant and cut-off.** The row count floors the view height. Terminal mode anchors the content to the bottom (`jam_TextEditor.cpp:861`), so a strip smaller than one row sat at the top. `Content::paint` draws history above the live rows, so the strip showed the last history row on both screens. The viewport now holds whole rows, and the leftover sits empty below the grid.
- **Scroll snap by paint offset.** A view position snap in `visibleAreaChanged` would stick on a small trackpad delta: the wheel starts from the view position (`jam_Viewport.cpp:49`, `:54`). So the view position stays pixel exact, and `Content::paint` draws from the row top.
- **Ligatures were deleted.** Commit `e78e5776c` (Sprint 163) removed `tryLigature` and `findLigatureGlyph` as *"ligature and composite lanes deleted"*. The restored lane keeps the old rules: 3 cells then 2, narrow ASCII, one style, one glyph from the shape. The new parts are the `Fonts` gate, the JUCE `liga` feature as the carrier, and a cache that also holds misses. ARCHITECT confirmed nvim renders them.
- **Config row.** `ConfigDocument::addTables` rejects a disk table whose headers differ from the default (`jam_ConfigDocument.cpp:490`), so a `## fonts` column would fail every existing disk copy, and agents never change the version (`SPEC.md:547`). The row went to `## display`. Hot reload needs no new code: `fileChanged` → `registerStyle` → `setAppearance` → `sendLookAndFeelChange` → `TextEditor::lookAndFeelChanged` → `setBlocks`.
- **Prompt ligatures.** A typed `->` stays two glyphs, because `zshrc:131` and `:142` give `-` and `>` two colours, and the rule needs one style (`jam_GlyphArrangement.cpp:424`). ARCHITECT: *"correct. echo or quoted works."*
- **Mistakes owned.** The first scroll snap proposal, corrected before code. The `@font-face --ligatures` recommendation, withdrawn on `jam_ConfigDocument.cpp:490`. The first font read stepped `hmtx` by 2 bytes, so its side bearings looked like advances. Only the 615 value stood.

### State for Continuation
- **Cell width is held open.** ARCHITECT: *"now visually the kerning too spaced out"*, then *"hold that thought"* for the fonts. Options on record: `--kerning: -0.0417` at `eve/Source/layout/style.css:27` gives 7.5 (15 device px at 2x), or a scale-aware baseline, which changes the contract at `jam_GlyphRasterizer.h:255`. No decision yet.
- **Prompt ligature rule is unchanged.** The colour check at `jam_GlyphArrangement.cpp:424` stays, because ARCHITECT did not rule to change it.
- Hot reload of `ligatures` and `--kerning` is not reported as tested. Add the `ligatures` row to `~/.config/dev/eve.md` and flip it.
- Check the Display Mono glyph drop from 1443 to 118 against what the fonts must hold. ARCHITECT said the fonts add glyphs. A missing character now takes a fallback font.
- Doxygen after the audit: `jam_GlyphArrangement.h:13`, `:48` still say that `hb_shape` serves grapheme clusters only; the new declarations need blocks. `SPEC.md` has no `ligatures` line, and writing it is gated.
- The old fonts are backed up only in the session scratchpad, and git holds them too.

### Debts Paid
- None

### Debts Deferred
- None

---

## Sprint: Keyboard Focus Follows Pane Focus; MatrixComponent Owns the Pane Focus Commands ✅

**Date:** 2026-11-04
**Duration:** one session, after the Shift+Enter log. The start and end times are not recorded.
**Scope (ARCHITECT):** *"keyboard focus is not following pane focus. this is contract violations. i need to click the pane although it already focused (the outline shows) after creating new split. all must be event driven, not manually manage the state maching"*; *"SSOT, navigating with prefix + hjkl, clicking, moving focus after split, etc must be exact ONE commandId, feasible?"*; *"then the design is wrong. if focus management is MatrixComponent responsibility, then it's MatrixComponent commandId, END only tell"*.

### Agents Participated
- COUNSELOR: reads of the focus path, the plugin load order, the JUCE command target chain, the cast module table; validation of each changed line; the module table row move; this log.
- Engineer (sonnet): `EditorView` change, the move of the commands to `MatrixComponent`.
- Auditor: not run. ARCHITECT: *"do not audit until i explicitly say so"*.

### Files Modified (12 total)
- `end/Source/ui/EditorView.cpp:163-183` — the plugin-loaded event writes the row's `Id::focus` into the plugin's `focus` parameter, through the one write site that `Id::focus` uses. No new state and no helper.
- `jam/cast/gui.md:55-58`, `:226` — `Id` rows `paneDown`, `paneLeft`, `paneRight`, `paneUp`; new `MatrixCommandId` table, ids from `0x4900001`.
- `jam/cast/spell.md:950-961` — `map::MatrixCommandId` in `jam_Gui.h`, a copy of the `TextEditorCommandId` row.
- `jam/user-modules-info.md:988` — `commands/jam_CommandManager.h` moves above `layout/jam_OwnedComponent.h`. This table gives the include order of the generated `jam_gui.h`.
- `jam/jam_gui/layout/jam_MatrixComponent.h:18-19`, `:100-106`, `:141`, `:220-221` — `juce::ApplicationCommandTarget` base, its four overrides, `focusPane`, `matrixCommandId`, `commandManager`.
- `jam/jam_gui/layout/jam_MatrixComponent.cpp:10`, `:16`, `:341-379` — both constructors register the commands; `perform` looks up the direction and calls `focusPane`, which ends in `setFocusedChild`.
- `end/cast/bimaps.md`, `end/cast/identifiers.md` — the four pane focus rows are deleted.
- `end/Source/ui/ENDView.h`, `end/Source/ui/ENDView.cpp:31`, `:70-73` — `registerFocusActions` is deleted; `getNextCommandTarget` returns `getActiveTabView()`; `setKeyPresses` runs after `newSession`.
- `end/Source/ui/TabView.h`, `end/Source/ui/TabView.cpp` — `focusPane` is deleted.

### Alignment Check
- [x] BLESSED principles followed: `setFocusedChild` stays the one focus write; the owner of the panes owns the commands; END tells through the target chain.
- [x] NAMES.md adhered: `MatrixCommandId` joins `TextEditorCommandId`; `focusDirections` keeps its existing name as a dispatch noun (Rule 8).
- [x] MANIFESTO.md principles applied
- [ ] Not validated by a clean compile. The last build gave one error, fixed after that build, and no build ran after the fix.
- [ ] No Auditor run, on ARCHITECT's word.

### Problems Solved
- **A new split had the outline but no keyboard focus.** `split` runs `childAdded`, then `setFocusedChild` (`jam_MatrixComponent.cpp:342`), which writes the row's `Id::focus` (`jam_OwnerComponent.cpp:111`). At that time `editor` is null, so the old write site dropped the event. The plugin loads asynchronously (`Session.cpp:91-93`), and `Id::name` then builds the editor (`Session.cpp:115`). No focus write followed, so EVE's `focus` parameter stayed 0. A second `setFocusedChild` would do nothing, because `ValueTree` sends no event for an unchanged value (`juce_ValueTree.cpp:144-145`).
- **Command ownership.** One `commandId` for every focus path is not possible: a command carries no argument (`juce_ApplicationCommandManager.h:219`, `ENDView.cpp` `perform`), and each key row maps to one command (`jam_CommandManager.cpp:28`). ARCHITECT's design holds: `MatrixComponent` owns four commands, and click and split were already inside it (`jam_OwnerComponent.cpp:94`, `jam_MatrixComponent.cpp:342`).
- **`CommandManager` unknown at `jam_MatrixComponent.h:221`.** The include order of `jam_gui.h` comes from `jam/user-modules-info.md`, so `cast` overwrote the hand edit.
- **Mistakes owned.** A code edit before ARCHITECT's *"discuss first"*. A layer argument against a JAM command id, which `TextEditorCommandId` refuted. A plan step that edited generated `jam_gui.h` when the source is `user-modules-info.md`.

### State for Continuation
- Build `jam` through `cast` first. Check that `jam_gui.h` gives `commands/jam_CommandManager.h` above `layout/jam_OwnedComponent.h`, and that `map::MatrixCommandId` has its four enum names.
- ARCHITECT tests these: a new split takes keys with no click; the first pane of a new tab takes keys; prefix + hjkl moves focus, and keys follow; the same after a tab switch.
- `setKeyPresses` now runs inside the `callAsync` of the `ENDView` constructor (`ENDView.cpp:31`). A key sheet row of a pane command is skipped if it runs before a `TabView` exists (`jam_CommandManager.cpp:14`). `ENDView.cpp:445` sets it again on a config event.
- The command chain has a loop: `ENDView` goes to the active `TabView`, and `TabView` goes to its parent target, `ENDView`. `getTargetForCommand` asserts on the loop only for a command that no target owns (`juce_ApplicationCommandTarget.cpp:111`). Every registered command has an owner, so this path does not run.
- The new `MatrixComponent` declarations have no doxygen. They wait for the comment pass.
- The state of the three previous checks in the Shift+Enter log is partly known: `cast` ran, and the build reached `jam_MatrixComponent.h`. Shift+Enter and the dev config root are not reported as tested.

### Debts Paid
- None

### Debts Deferred
- None

## Sprint: Shift+Enter Is a Terminal Key Row; Debug END and EVE Use the Dev Config Root ✅

**Date:** 2026-11-04
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"why i cant shift+enter to create new line in this claude code?"*; *"we need to shift the config to ~/.config/dev/ so this running instance wont conflict with we're working on check @DEBT.md"*; *"this should not be hardcoded to TextEditor, as plain TextEditor newline should be just simply enter. shift+enter newline only for TERMINAL mode"*; *"why harcodes? isnt SSOT? fix it"*; *"dont change the comment. config/dev/ only always temporary"*; *"after plan and execution, do not audit until i explicitly say so"*.

### Agents Participated
- COUNSELOR: reads of the key encoders, the key sheet, the host key hook, the Kitty and Ghostty sources, the nvim doc, the Claude Code binary, the cast read path; validation of each changed line; this log.
- Pathfinder (haiku): plan and config root read, Claude Code binary commands.
- Engineer (sonnet): the two implementations. The first file set had eight files, the second had three.
- Auditor: not run. ARCHITECT: *"do not audit until i explicitly say so"*.

### Files Modified (11 total)
- `jam/cast/terminal.md:1851`, `:1914`, `:1970`, `:2008`, `:2046` — one `newline` row in `TextEditorCommandId`, `textEditorKeys mac`, `textEditorKeys windows`, `textEditorModes`, `textEditorLayers`: key `shift+return`, mode `terminal`. The tables are positional, so each one takes a row.
- `jam/cast/gui.md:51` — `Id::newline`.
- `jam/cast/spell.md:134-136` — the three key lookup tables grow from 23 to 24.
- `jam/jam_gui/widgets/jam_TextEditor.cpp:464-468` — `Id::newline` action beside `paste`. It writes `Chars::newline` through `terminalInput`.
- `end/project-info.md:369`, `eve/project-info.md:379` — `configDirectory` DEFINES row. `CAST_CONFIG_DIRECTORY` is `.config/dev` for Debug and `.config/end` otherwise.
- `end/Source/config/ConfigDirectory.h:75` — `configDirectoryName` reads the define.
- `eve/cast/files.md:31` — `markdown settings directory` reads the define. The cell has no format, so the macro stays raw (`cast/Source/Model.cpp:975-985`). The value column of the table is wider.
- `jam/jam_clap/format/jam_ClapPluginFormat.h:37`, `:135`, `jam/jam_clap/format/jam_ClapPluginFormat.cpp:161`, `:167`, `:257-266` — the `inHousePath` literal is gone. The host passes `hostPluginsDirectory` through the constructor.
- `end/Source/plugins/Nexus.cpp:80` — END passes `ConfigDirectory::Config::getPath (Id::plugins).getFullPathName()`.

### Alignment Check
- [x] BLESSED principles followed: one root switch in the `DEFINES` lane, each root path read from one owner, a key as a data row.
- [x] NAMES.md adhered: `hostPluginsDirectory` joins the `host*` family; `CAST_CONFIG_DIRECTORY` joins `CAST_PLUGINS_HASH`.
- [x] MANIFESTO.md principles applied
- [ ] Not validated by a compile. No cast and no build ran.
- [ ] No Auditor run, on ARCHITECT's word.

### Problems Solved
- **Shift+Enter submitted.** With no key row, terminal mode called `TerminalKeyboard::map` with `keyboardFlags` at 0 (`jam_TextEditor.cpp:511-516`), which returns `carriageReturn` for Return (`jam_TerminalKeyboard.cpp:172-183`, `:218`). Claude Code reads `0xd` as `return` and `0xa` as `enter`, a line break (binary lines 51593976-51593978, 51599945). Old endless wrote `\n` from a `newline = "shift+return"` binding (`default_keys.lua:102`, `MainComponentActions.cpp:65`). The new row has the same shape, and only terminal mode matches it (`jam_CommandManager.cpp:65-70`).
- **Convention.** No read shows a standard for this key. Kitty legacy mode sends `0xd` for Shift+Enter (`keyboard-protocol.rst:530`). Ghostty sends `\x1b[27;2;13~` (`function_keys.zig:199`). nvim negotiates `CSI > 3 u` or `CSI > 4 ; 2 m` (`tui.txt:185-195`). Claude Code's own setup binds ESC CR in VS Code and Zed (binary lines 51600164-51600174).
- **Ctrl+J gave nothing, Ctrl+Shift+J gave a new line.** The peer offers each key to END first (`jam/patch/juce-key-pressed-hook.patch:21-23`, `Nexus.cpp:7-13`, `ENDView.cpp:57-59`). `~/.config/end/keys.md:75` binds `ctrl+j` to `join down`. `ctrl+shift+j` has no row, so EVE's `mapCtrl` sends `0xa` (`jam_TerminalKeyboard.cpp:75-78`).
- **Mistakes owned.** A first Pathfinder prompt read far past the question. A claim that Ctrl+J gives a new line came from `mapCtrl` alone. A claim that Shift+Enter cannot be a key row missed `jam_PluginEditor.cpp:328`. Each one was corrected in the chat.

### State for Continuation
- Nothing is cast or compiled. The generated files and `CMakeLists.txt` of `jam`, `end` and `eve` are stale until the next `cast`. Check these three things first:
  1. `eve/Source/generated/Files.cpp` gives `juce::String::fromUTF8 (JUCE_STRINGIFY (CAST_CONFIG_DIRECTORY))`.
  2. `CMakeLists.txt` of `end` and `eve` holds the `configDirectory` row with its `$<IF:...>` quoted.
  3. `JUCE_STRINGIFY` gives `.config/dev` in Debug and `.config/end` in Release.
- ARCHITECT tests Shift+Enter in Claude Code in hosted EVE, and in a plain TextEditor mode. The second must not change.
- A Debug END makes `~/.config/dev/`. ARCHITECT: *"config/dev/ only always temporary"*.
- Comments stay stale on ARCHITECT's word *"dont change the comment"*: `ConfigDirectory.h:46`, `:52`, `Main.cpp:252`, `Main.h:28`, `jam_ClapPluginFormat.h:108`. `jam_ClapPluginFormat.h:28-30` has no `@param` for the new parameter.
- The negotiated layer is not built. `TerminalKeyboard` encodes `CSI 13;2u` (`:172-179`), but `jam_TextEditor.cpp:516` passes no flags and `applyCsiSequence` drops `CSI >`. If it is built, the key row wins over it, because `invokeKey` runs first (`:511`). This belongs to `DEBT-20261103T233912`.
- `DEBT-20261103T235332` stays in the ledger. This sprint did not touch it. ARCHITECT: *"i have already backup old binary to endless repo"*.
- `eve/CLAUDE.md:59` lists `HANDOFF.md` as present, and `:42-44` list three `Source/EVEView*.cpp` files. None of these four files exists.

### Debts Paid
- `DEBT-20261103T235331` — a Debug END and EVE use `~/.config/dev/`, a Release build uses `~/.config/end/`, with one switch in the `DEFINES` lane of each project.

### Debts Deferred
- None

## Handoff to COUNSELOR: EVE terminal protocol — one envelope, one path; measure and decode; Preview foundation

**From:** COUNSELOR
**Date:** 2026-10-30
**Status:** In Progress. Every PLAN step through Step 17 is done except Step 14, which is ARCHITECT's runtime test. The sprint is not logged.

### Read First
1. `PLAN-eve-terminal-protocol.md`. This is the locked plan, approved with `/go`. It holds the Preview fold (Steps 12a–12g, `:226-275`) and the Step 17 audit resolution (`:309-343`).
2. `PLAN-Grand-Scheme.md` §3.11 "File browser and Preview". This is the vision recorded in Step 12a.
3. `SPEC.md`. It holds the protocol, the lanes, the dock and the mental model (Step 15).
4. `DEBT.md:11-15` — `DEBT-20261025T214955`. The sprint pays it. It stays in the ledger until the log (PLAN `:298`).

### Context
- ARCHITECT's prompt: *"next sprint is 2-5"* (`jam/carol/SPRINT-LOG.md:62-66`).
- Items 2, 3 and 4 measure and decode. Item 5 pays `DEBT-20261025T214955` as the foundation of EVE's own terminal protocol, with scope *"+ preview + image"* (PLAN `:11-15`).
- On 2026-10-30, ARCHITECT folded the Preview foundation into the sprint (PLAN `:5`, `:28-36`).
- Since then, COUNSELOR has resolved the Step 17 audit batches, run the final validation build and run the doxygen pass.

### Completed
- **Steps 0–13, 15–17.** The PLAN steps are done. The Step 4 criterion is missed (see Open Questions).
- **Step 17 audit.** The Auditor reported 104 findings. Each one is resolved or rejected per DCF. The batches A1, B2a–B9 are listed at PLAN `:315-329`. The rejected findings, each with its read, are at PLAN `:331-343`.
- **Dispositions written after PLAN `:343`:**
  - #82 is resolved by B2a. `TerminalProcessor` owns the `images` lane and sizes it in `reset` (`jam_TerminalProcessor.cpp:24`). `audioDeviceAboutToStart` reaches `reset` through `prepare` (`EVETerminalProcessor.cpp:221-229`, `:261-270`).
  - #63 is rejected. `imageBounds` is replaced whole on each vblank. MANIFESTO E: *"A complete state replaces a complete state."* The vblank tell also retries an image whose bytes are not yet in the record (PLAN `:75`).
  - #86 is rejected. The dock visibility has one truth, the `preview` component property (`EVEView.cpp:112-120`).
  - #87 is rejected. The base class also dispatches with `property ==` (`jam_PluginEditor.cpp:343`).
  - #88 is rejected. A read of a Model row asks no object for its state (the same read as #79).
  - #80, part, is rejected. `juce::Identifier { name }` is the Design's own call (PLAN `:81`).
  - #84, part, is rejected. The base class owns the view bounds (`jam_PluginEditor.cpp:43-69`). Only the hand-written inverse transform changed, to `getLocalArea` (`EVEView.cpp:102`).
- **Corrections in this session, now in the tree:**
  - `writePendingData` and `handleAsyncUpdate` are back to their original form (`EVETerminalProcessor.cpp:167-179`). `setOverlays` calls no drain (`EVEView.cpp:122-145`).
  - The raw RGB path uses the new `jam::simd::convertRgbPixel`, `convertRgb4` and `convertRgb`. They follow the RGBA sibling, with SSE2, NEON and scalar arms (`jam_SimdBlend.cpp:860-923`).
  - `Lane::images` is renamed to `Lane::imageBounds` (NAMES Rule 3 and Rule 5).
  - `setImageBounds()` runs at the end of `addLanes()` again.
- **Final validation build.** Both builds exit 0. The logs have 0 `warning:`, 0 `error:` and 0 `FAILED` lines. Both reach `[65/66]` (`<scratchpad>/build-release.log:73`, `<scratchpad>/build-profile.log:73`; scratchpad `/private/tmp/claude-501/-Users-jreng-Documents-Poems-dev-eve/750cd3a0-9445-4670-9579-0d6cb7b98a57/scratchpad`).
  - Release: `cast cast/spell.md --no-sign` (`project-info.md:138-142`).
  - Profile: `cmake -S . -B Builds/Profile -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF -DCAST_PROFILE=ON`, then `ninja -C Builds/Profile` (`benchmark/burn.md:34-36`).
- **Doxygen pass.** The pass documents all 23 changed headers.
  - jam: 0 warnings (`~/.config/nvim/doxygen/Doxyfile.lib`, run with `~/.config/nvim/scripts/build-doxygen.sh`).
  - EVE: 0 warnings (`~/.config/nvim/doxygen/Doxyfile.project`).
  - Both builds pass again after the pass.

### Remaining
1. **Step 14 — ARCHITECT.** Run EVE. Senders, with PNG, GIF and JPEG:
   - `imgcat` (cells, `px`, `auto`);
   - `kitten icat`, with and without `--unicode-placeholder`;
   - `chafa`;
   - fzf with `bin/fzf-preview.sh`;
   - a full-screen CLI on the alternate screen.

   The criteria are at PLAN `:284-288`. No run of the current build has exercised the overlays or the dock.
2. **Step 11 and Step 12g checks — ARCHITECT, in the same run:**
   - `printf '\e]1337;EVE;command;name=clear\a\e]1337;EVE;command;name=clear\a'` clears twice (PLAN `:224`).
   - `eve preview <png>` and `eve preview <gif>` show in the dock. An empty path hides it. `ctrl+shift+P` toggles it (PLAN `:275`).
3. **Log — COUNSELOR, on ARCHITECT's word only** (PLAN `:307`).
   - Write the sprint block per `/log`.
   - Then run `carol debt clear DEBT-20261025T214955`, with the receipt first.
4. **Optional — COUNSELOR.** Check the doxygen prose for ASD-STE100 sentence length. The doxygen Engineer flagged this.

### Key Decisions
- *"be careful with the design we have been meticulously optimize to get fastest performance, don't fucking break working implementation. no fucking hack, DESIGN BY CONTRACT?"*
- *"if you genuinely needed and doesn't exist, write one, follow established pattern verbatim, CROSSPLATFROM"* — about SIMD RGB.
- *"why do we need specific RGB only? why not carry the alpha? RGB can use Alpha always opaque"*
- *"that's the plan. why the fuck flip flop and fucking churn tokens? do you fucking need to ask what i have approved?"* — the overlay of PLAN `:75` and `:269` stands: the vblank tell, plus a retry when the bytes are not yet in the record.
- *"TextEditor no image"*
- *"so no images, hold no image data, and name it image? why not using RectangleList?"*
- *"NAMES.md OBEY THE FUCKING PROTOCOL DESIGN BY CONTRACT CONTRACT NON NEGOTIABLE"*
- *"final validation only. not fucking every turn"* — builds.
- *"Percent-encode values"* — audit #89 (PLAN `:313`).
- *"Siblings via owner"*; *"EVEView, like overlays"* (PLAN `:360-361`).
- *"TextEditor only "reserve the space""* (PLAN `:32`).
- *"keep for now, but NOT for native Preview (Component)."* — the EVE `image` operation (PLAN `:36`).
- *"no fucking resume"* — never resume an agent. A correction goes to a fresh Engineer.
- The earlier rulings of PLAN `:17-36` stay closed.

### Files Modified
eve (`git -C eve status --short`, 2026-10-30):
- `Source/EVETerminalProcessor.h/.cpp` — lanes `commands` and `images`, `addCommand`, `getCommands`, `getSwap`, `registerEvents` additions, `prepare` drain via `triggerAsyncUpdate` + `handleUpdateNowIfNeeded`.
- `Source/EVEView.h/.cpp` — `applyCommands`, `applyImage`, `setOverlays`, the dock (`resized`, `setPreview`, `setPreviewVisible`, `isPreviewVisible`).
- `Source/EVEViewComponents.cpp` — the `onImages` binding.
- `Source/EVEViewPanelCallbacks.cpp` — the `preview` callback.
- `Source/layout/ViewLayout.md`, `Source/layout/parameters.md`, `cast/identifiers.md`, `Source/generated/Identifiers.*` — the model rows and the keys row; `commandCount` moved to EVE's cast.
- `Source/shell/zsh_integration.zsh`, `bash_integration.bash`, `powershell_integration.ps1` — the `eve` function with percent-encoding.
- `SPEC.md`, `PLAN-Grand-Scheme.md`, `comparison.md`, `carol/SPRINT-LOG.md:3`; new `README.md`, `LICENSE`, `PLAN-eve-terminal-protocol.md`.
- `benchmark/` — the control, live and replay reports; new `cat.md`, `live-sample.md`, `live-where.md`, `replay-where.md` and their reports.

jam (`git -C jam status --short`, 2026-10-30):
- `jam_terminal/processor/jam_TerminalProcessor.h/.cpp` — payload mode, the OSC 1337, DCS and APC routes, the `images` lane, `eveOperations`, percent decoding, Kitty placements.
- `jam_terminal/processor/jam_TerminalFIFO.h/.cpp` — the `maxBytes` overload and `applyPendingData`.
- `jam_terminal/image/` — `jam_TerminalITerm2Decoder`, `jam_TerminalKittyDecoder` (restored, metadata only), `jam_TerminalSixelDecoder`, `jam_TerminalImageSequence.h`; new `jam_TerminalImageEncoding.h`. Deleted: `jam_TerminalSkit.h/.cpp`, `jam_TerminalDecodedImage.h`, `jam_TerminalPendingImage.h`.
- `jam_terminal/document/` — `jam_TerminalSwap`, `jam_AnsiDocument` (`stringTerminator`, `getIndexedColour`, `dcs`/`apc`); new `jam_TerminalImageRecord.h/.cpp`.
- `jam_terminal/buffers/jam_TerminalBuffer.h/.cpp` — `ImagePlacement {destination, image}`, placements on both grids, `removeImagePlacements`.
- `jam_terminal/graphics/jam_TerminalGraphicsContext.h/.cpp` — the inline image branch removed.
- `jam_gui/widgets/jam_TextEditor.h/.cpp` — reserved space only, `imageBounds`, `onImages`, `setImageBounds`.
- New `jam_gui/widgets/jam_Preview.h/.cpp`; `jam_gui.h/.cpp`.
- `jam_graphics/fonts/jam_SimdBlend.h/.cpp` — `convertRgbPixel`, `convertRgb4`, `convertRgb`.
- `jam_data_structures/model/jam_Model.h/.cpp`, `jam_ParameterText.h`, new `jam_ParameterText.cpp`; `jam_plugin_bootstrap/parameter/jam_ModelLayout.cpp` — the `text` row and `store`.
- `jam_core/text/jam_AttributedString.h/.cpp` — `setAtom`.
- `cast/terminal.md`, `cast/gui.md`, `cast/plugin_bootstrap.md`, `cast/spell.md`, `generated/*`, `user-modules-info.md`.

### Open Questions
- **Step 4 criterion — ARCHITECT.** Decode = `terminalProcess` − `terminalRead`. It went from 15.38 s (Step 3, this session) to 15.49 s (`benchmark/live-report.md:19`, `:27`: 16.340 s − 0.853 s). That is +0.7%. The criterion was "no worse" (PLAN `:304`).
- **Sixel caps — ARCHITECT ratifies.** 4096 × 4096 pixels, and 65535 for a parameter.
- **`ninja doxygen` — ARCHITECT.** No EVE build directory defines the target. `CLAUDE.md` lists it. The pass ran `doxygen` direct on `Doxyfile.project`.
- **scout kernel sampler — ARCHITECT.** There is a known bug at `scout/Source/Sampler.cpp:154`.

### Next Steps
- Start at Remaining item 1, when ARCHITECT reports the Step 14 run. Fix each runtime failure from a read, per CAROL.md Failure Protocol.
- Log when ARCHITECT says "log sprint".

