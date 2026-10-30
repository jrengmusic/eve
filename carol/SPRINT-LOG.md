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

## Sprint: TextEditor nvim Parity — the Harness Is an Editable Buffer; History in the TextEditor, Live in the Processor ✅

**Date:** 2026-10-24
**Duration:** multi-session, 2026-10-23 → 2026-10-24
**Plan:** `PLAN-texteditor-nvim-parity.md` (Steps 0–12, Amendments A and B, the B corrections, the Audit B triage). jam Sprint 171 is the primary record.

### Decisions (ARCHITECT, verbatim)
1. *"vim core features parity first then. so we have interactivity, correct design by contract of TextEditor state machine with juce::ValueTree. and the harness file just become another file to load and edit."*
2. *"TextEditor state machine, (juce::ValueTree) will be attached to EVEProcessor APVTS"*.
3. *"eve terminal "NORMAL" screen is actually always in TextEditor INSERT mode"*.
4. *"scrollback budget always cap from settings"*; *"what scollback budget? not scrollback capacity?"*.
5. *"decouple. Processor is live buffer, TextEditor will hold history at least that's how the terminal designed. so SIGWINCH never ever destroy history content."*

### Agents Participated
- COUNSELOR — plan and amendments, briefs, validation by read, audit triage, SPEC/RFC amendments, log.
- Engineer (sonnet) — code, builds, sync, doxygen. Pathfinder (haiku) — reads, inventory. Auditor (opus) — three audits.

### Files Modified (10 tracked + 1 new)
- `Source/EVETerminalProcessor.h/.cpp`:
  - `load` and its print helpers are deleted.
  - The `Id::textEditor` getter `(AnsiDocument&, const std::function<void (AnsiDocument&, bool, int)>&)` pulls into the view's history document and gives the live document and the capacity.
  - `getHistoryCapacity()` reads the `HISTORY_CAPACITY` setting from the current state tree at use (`jam::Model::getChildWithID (…).getProperty`). It builds no `juce::Value`, and it survives `replaceState`.
  - The constructor takes the model only.
- `Source/EVEProcessor.h` — `terminalProcessor { model }`.
- `Source/EVEView.h/.cpp`:
  - The `harness` member and the constructor `load` tell are deleted.
  - The constructor grabs keyboard focus in `callAsync` (END pattern, `ENDView.cpp:28-42`).
  - `focusGained` forwards focus to `createKeyboardFocusTraverser()->getDefaultComponent (this)`, the TextEditor (endless pattern, `Display.cpp:212`).
- `Source/EVEViewComponents.cpp` — file-static `getHarness()`, and a captureless `registerConfig<jam::TextEditor> (Id::textEditor, Id::toTag (Id::winsize), …)` that calls `load (getHarness())`.
- `Source/layout/ViewLayout.md`:
  - the `## normal`, `## insert` and `## visual` key sheets;
  - the `HISTORY_CAPACITY` description: *"the swap keeps this many lines in memory"*.
- `SPEC.md`:
  - Feature 2.3: the TextEditor owns the history and its swap.
  - State Model: ownership, pull, edits.
  - Feature 6: the scrollback cap is `HISTORY_CAPACITY`.
- `RFC-EVE-terminal-grand-scheme.md` — the §3.9 amendments: the swap, the Codec, the decouple, the capacity through the pull.
- `CLAUDE.md`. `PLAN-texteditor-nvim-parity.md` (new).

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered
- [x] MANIFESTO.md principles applied

### Problems Solved
- The config lambda captured the view inside the shared Registry, where the first holder wins. The config is captureless, and the capacity comes through the pull.
- Runtime crash: `jassert (historyRows > 0)` in `prepare`. A `juce::Value` member bound at construction found no settings node yet. It would also point to a replaced tree after `replaceState`. The capacity is now read at use.

### State for Continuation
- `cast cast/spell.md --debug`: 0 errors, 0 warnings.
- Runtime:
  - A whatdbg trace launch (25 s) has no stop.
  - Keys sent through the OS reach `TextEditor::keyPressed`: `j`, Escape, `j`, `x`, `u`, `i`, Return, Backspace. A temporary `debug::Log` proved it, and the log was then removed.
  - An AppleScript quit gives `exit=0`, with no assertion, no leak and no crash report.
  - ARCHITECT confirmed the keyboard works. The status row is now a separate row below the text.
  - A trackpad-scroll assert in `jam::TextEditor::paintRows` (a visible line with no runs) is fixed. The runs were rebuilt from a stale view area. jam Sprint 171 has the detail.
- No interactive check of the editor has run yet. The plan's Verification list covers it. Delete `~/.config/end/eve.md` first, so the key sheets seed.
- The chain (`prepare`, `setWinsize`, FIFO sizing) stays for the TTY sprint (Design 6). A window size change does not resize the FIFOs.

### Debts Paid
- None

### Debts Deferred
- None

---

## Sprint: Reflow at the Window Width — TextEditor Is the Only Renderer; the Harness Prints from the AST ✅

**Date:** 2026-10-21
**Duration:** one session, 2026-10-21 (context compacted once)
**Plan:** `PLAN-reflow.md` (Steps 0–10 and the execution state; also `~/.claude/plans/giggly-dancing-pike.md`)

### Decisions (ARCHITECT, verbatim)
1. *"css width is necessary to set baseline, a default "config". but without it reflow at Component side should just always adhere to whatever available space from juce::Component::getLocalBounds().getWidth()"*; `maxLineWidth`: **"Unset means bounds"**.
2. *"mermaid diagram, images, tables, follow exactly the same logic … images could simply use AffineTransform or juce::Rectangle API to fit"*.
3. *"in EVE reflow should always the FINAL materialiser, the renderer, the COMPONENT. the TextEditor"*; *"ideally TextEditor just call ReflowDocument API to reflow ANYTHING, SSOT"*.
4. *"Table should wrap each column"*; *"each table column can be treated exactly like whole component "column""*; **"only real table"**; *""one line" is the data. but rendered reflow adding/removing lines"*.
5. Table mark **"OSC 133 marker"**; mark writer **"Print from the AST"**; decorations **"Layout properties"**.
6. *"all "terminal" concern always at Processor (audio thread) hot path? lock free"*.
7. Names: `lineStart, E`; `columnStart, F`; `fill, G`. Quote bar: *"just make consistent. what's the established pattern?"* — a quote bar is a fill (`jam_MarkdownGraphics.cpp` `addQuoteBars`), so no bar key.

### Agents Participated
- COUNSELOR — plan, rulings record, every delegation brief, validation of each Engineer return by own reads, audit triage (30 resolved, 14 rejected with reads), visual validation of the captures, doxygen read-through and three corrections, log.
- Pathfinder (haiku) — code reads, whatdbg traces and samples, Release timing runs, capture sweeps, read-only git.
- Engineer (sonnet) — Steps 1–8, audit fixes in three batches, builds, ephemeral timing lines and their removal, doxygen pass.
- Auditor (opus) — one sprint audit (40 findings).

### Files Modified (28 tracked + 1 new)
- EVE:
  - `Source/EVETerminalProcessor.h/.cpp` — `load` parses, calls `addLines`, and prints the AST into engine rows inside `suspendProcessing` (`:50-89`). No `MarkdownComponent`, no `GraphicsContext`. Per line: `getNumRows`, `addRows`, `addRow`, `setAtom`, `addCells`, `addImage`, `getDiagramImage`, `getNumObjects`, `addMarkers`. `prepare` sizes history from `engine.getNumBytes()`.
  - `Source/layout/style.css` — `maxLineWidth` deleted (R1: unset means bounds).
  - `PLAN-reflow.md` — new.
- jam:
  - `cast/terminal.md`, `cast/core.md`, `cast/markdown.md`; generated `jam_Terminal.h`, `jam_Core.*`, `jam_Markdown.*` — `ShellIntegration` rows `lineStart E`, `columnStart F`, `fill G`; `indent` moved to core.
  - `jam_terminal/graphics/jam_TerminalEncoder.h/.cpp` — `appendOscCommand`, `appendShellIntegration`, `appendLineStart`, `appendColumnStart`, `appendFill`; `appendImageArgument` is one template plus the Colour overload; `appendOscIntroducer` folded into `appendOscCommand`.
  - `jam_terminal/document/jam_AnsiDocument.h/.cpp` — OSC 133 `E`/`F`/`G` decode into `Id::indent`, column child lines (with their images) and `Id::fill` children. Arguments are checked at the entry; `getShellIntegrationProperties` table; `addSegments` builds the cell-line segments once.
  - `jam_terminal/graphics/jam_TerminalGraphicsEngine.h/.cpp` — `MarkerPlacement` / `addMarkerPlacement`; `appendRowSpan` writes markers in the row text; `getNumBytes()` of the prepared rows; `getNumBase64Bytes`.
  - `jam_gui/widgets/jam_TextEditor.h/.cpp` — reflow at the visible width minus the indent; per-column reflow (`setTableRows`); images fitted `onlyReduceInSize` (`setImageRows`), also in columns; fills painted; runs built at layout for visible lines only (`addVisibleRuns`); `setLane`, `getContentWidth`.
  - `jam_style/jam_StyleCustom.h/.cpp` — `getLineIndent` reads `Id::indent` cells × cell width.
  - `jam_markdown_graphics/style/jam_StyleMarkdown.h/.cpp` — `getLineWidth (availableWidth)` caps at `maxLineWidth` only when the sheet has it; `getBlock` / `getType` public.
  - `jam_markdown_graphics/graphics/jam_MarkdownGraphics.h/.cpp` — `getPills`, `getPill`, `getFills`, `hasFill` public; the only author of line fills.
  - `jam_markdown_graphics/component/jam_MarkdownComponent.cpp` — `getLineWidth` through `StyleMarkdown`; diagram raster through `MermaidGraphics::getImage`.
  - `jam_mermaid_diagram/diagram/jam_MermaidGraphics.h/.cpp` — `getImage`, the one mermaid raster.

### Alignment Check
- [x] BLESSED principles followed — one reflow (`ReflowDocument::setRows`), one fill author (`MarkdownGraphics::getFills`), one mermaid raster; no allocation on the audio-thread path (SPEC.md:255); paint builds nothing (ruling 6).
- [x] NAMES.md adhered — new names approved or from established families (`getFills` beside `getPills`, `setAtom` in the `AttributedString::set` family, `appendShellIntegration` in the `append*` family).
- [x] MANIFESTO.md principles applied — creation at load (the print); marks complete at decode; materialisation at layout.

### Problems Solved
- Column cap: the old lane laid out the harness at `maxLineWidth 100em` and ended each row with `\n`, so TextEditor could split lines but never join them. Now one row is one logical line of any length, and TextEditor reflows at its bounds (1600 wide: a 147-character line on one row).
- `std::bad_variant_access` at the first pull: `addCoveredImages` read `Id::height` of `Id::fill` children; fixed with `isTag (Id::image)`.
- Audit: 40 findings; 30 resolved, 14 rejected with reads (PLAN-reflow.md, Step 10). One acceptance (#4) was wrong at first: `getType` / `getBlock` were private; the build failed and they were made public.

### State for Continuation
- Builds through cast, 0 errors, 0 warnings: EVE `--debug` and `--no-sign`. Doxygen 0 warnings, jam and eve (scratch Doxyfiles from the nvim templates; EVE's CMake has no `doxygen` target). Debug trace: `END outcome=timeout stops=0`.
- Launch is not lower than Step 0 (processor → first painted 749–807 ms). Measured costs (Release): the image round trip ~290 ms (picture ~97, PNG encode ~90, base64 + PNG decode ~101), advances ~55 ms (PLAN-reflow.md Step 9).
- Open, not shown to be caused by this sprint: Han ideographs blank at `markdown/markdown.md:481`, `:483`; full-width line `:489` blank; grid cell `:471-472` joins two bullets.
- Per-column wrap with long cell text is not exercised by the harness.
- Tooling: the whatdbg sampler fails with `timeout must be >= 0` (`~/.carol/skills/whatdbg-trace/sample.lua:51-52`, clock read twice; `trace.lua:190` clamps).
- Process: three Engineer edits used Python writers without backup (TextEditor, AnsiDocument, AnsiDocument.h doxygen); every changed region was read in full, no damage. The Auditor left background work; it was stopped at log time.

### Debts Paid
- None

### Debts Deferred
- None

---

## Sprint: EVE Launch in Subseconds — Terminal Round Trip on the JUCE Plugin Contract; Style Is the Sole Raster Author ✅

**Date:** 2026-10-21
**Duration:** multi-session, 2026-10-19 → 2026-10-21 (jam Sprint 170 is the framework record; KANJUT Sprint 148)
**Plan:** `PLAN-launch.md` (Part A, Part B v3 V0–V9, Part C C0–C3, Audit 2 resolution, rulings 1–67, Step R)

### Agents Participated
- COUNSELOR — plan, rulings record, every delegation brief, validation of each Engineer return by own reads, visual validation of the captures, PLAN/RFC amendments, log.
- Pathfinder (haiku) — code and manifest reads, git history of `lookAndFeelChanged`, kernel submodule set difference, capture sweeps, timing runs.
- Engineer (sonnet) — implementation, cast builds, `cast --sync`, KANJUT manifest mirror, doxygen pass.
- Auditor (opus) — Audit 2, once; findings resolved or rejected with reads (PLAN "Audit 2 — resolution").

### Decisions (ARCHITECT, verbatim, this segment)
1. *"the sole author should be Style since the SSOT is css-settings. so css carried by binary as default, write to disk as settings (exactly like other settings contract), settings always win, when a project did not "open" the parameter as settings, Style always return the default carried css. eve should have settings for rasterisation, embolden, etc it's the contract carried from old end (endless)"*; *"EVE using its own eve.md"*.
2. *"both StyleTheme are identical. it's kernel module. both framework should be synced with cast --sync. be careful with the design. don't break JFS. IIUC JFS is the example of rasterisation/embolden default value carried by css, but never write to Settings. the contract Holds"*.
3. *"StyleManager should be GlyphAtlas agnostic whenever Vulkan not included as deps"*; *"LookAndFeel communicate to GlyphAtlas. Window is responsible for rendering engine, PluginEditor should not ever know about Vulkan"*; Vulkan contract unchanged (*"why the fuck do you think you need to change all Vulkan contract?"*).
4. *"history capacity same Settings contract: ViewLayout.md to eve.md, 10k row"*.
5. *"if it changes toInt signature just create overload. toInt without flag always nearest"*.

### Files Modified (EVE)
- `Source/EVETerminalProcessor.h/.cpp` — new: the JFS chain shape (ruling 37); `load` maps the harness `readOnly`, parses, paints once into `terminal::GraphicsContext`; `prepare` sizes the live lane by the winsize and the history lane by the winsize plus `HISTORY_CAPACITY` rows (`:20-31`).
- `Source/EVEAudioProcessor.h` — deleted (renamed to `EVETerminalProcessor`).
- `Source/EVEProcessor.h/.cpp` — chain member `terminalProcessor`, `prepareToPlay` / `process` / `setStateInformation` drive it.
- `Source/EVEView.h/.cpp`, `EVEViewComponents.cpp`, `EVEViewPanelCallbacks.cpp` — the View tells the harness `File` and the winsize; it never touches the engine.
- `Source/layout/ViewLayout.md` — `## settings` rows `RASTERIZER freetype`, `GAMMA 2.2`, `CONTRAST 0.0`, `EMBOLDEN false`, `HISTORY_CAPACITY 10000` (version unchanged, 0.0.2).
- `cast/identifiers.md` — `historyCapacity` row; `Source/generated/Identifiers.*` regenerated by cast.
- `Source/layout/style.css`, `component.md`, `ViewLayout.md` — harness and terminal rows.
- `CLAUDE.md`, `project-info.md` — stale panel/about text removed.
- `RFC-EVE-terminal-grand-scheme.md` — amended (§0–§3.10; §3.10 raster configuration and history capacity).
- `PLAN-launch.md` — new; rulings 1–67, Step R and its state.

### Alignment Check
- [x] BLESSED principles followed — one author per value (Style), settings win over the CSS default, no shadow defaults in the theme, no atlas call in StyleManager, Vulkan contract unchanged.
- [x] NAMES.md adhered — `historyCapacity` from ARCHITECT's words; `liveCapacity`, `getFontsStyleValue`, `applyCoverage` join existing families; settings keys are `Id::toTag` of existing ids.
- [x] MANIFESTO.md principles applied — creation clause for the parse (A04); lock-free FIFOs; materialisation at load, paint reads only.

### Problems Solved
- Launch: Release, warm, from exec — editor +257 ms, first window paint +706 ms, history pulled +893 ms (PNG write `Z_BEST_SPEED`, `PNG_FILTER_NONE`: encode 220 → 62 ms for 51 images).
- Widen reflow: `TextEditor::resized()` (JUCE `updateVisibleArea` fires `visibleAreaChanged` only on a changed visible area).
- Raster configuration had three authors (VulkanEngine constructor, StyleTheme hard defaults, PluginEditor pokes): Style is now the sole author through `StyleManager::getFontsStyleValue`; the LookAndFeel tells the atlas.
- A05: one capacity sized both lanes; now history and live are sized separately.

### State for Continuation
- Builds through cast, 0 errors, 0 warnings: EVE `--debug` and `--no-sign`; jfs `--debug` and `--no-sign`; kuassa `cast cast/spell.md`. Doxygen 0 warnings: jam, KANJUT, EVE (nvim `Doxyfile.project` template; EVE's CMake has no `doxygen` target).
- `~/.config/end/eve.md` and `~/Library/Application Support/EVE.settings` were deleted and re-seeded by the Debug run (ruling 55); the seed carries the new `## settings` rows (`eve.md:63-71`). A host session saved before this sprint restores a state without `HISTORY_CAPACITY` (`AudioModel::setState` replaces the whole state, `jam_AudioModel.cpp:177-181`); `prepare` asserts `historyRows > 0`.
- Visual validation (Debug, fresh seed): f0, f03, f45, f6, f99 render text, code, lists, rules, links, logos and mermaid correctly.
- The Release launch was not measured again after Step R; the timing scripts need the ephemeral `launch` lines (`timing.sh:13`).

### Debts Paid
- `DEBT-20261016T214137` — EVE opens in subseconds (first window paint +706 ms Release, warm); the terminal round trip runs on the JUCE plugin contract.

### Debts Deferred
- None

---

## Handoff to COUNSELOR: EVE launch in subseconds (DEBT-20261016T214137) — terminal I/O on the JFS/TETRIS topology [CONSUMED — superseded by the sprint above]

**From:** COUNSELOR
**Date:** 2026-10-20
**Status:** Consumed

### Context
Objective: *"EVE opens in subseconds, grand total, like jfs; find and remove every bottleneck on the launch path."* Run mode: *"no gate until /log"*. Part A (launch path) is mostly done in the working tree. The first-load still blocks the message thread: Debug stages were source `setRows` 1160 ms, `endFrame` 1652 ms, `AnsiDocument::parse` 1427 ms, destination `setRows` 1491 ms, `setContentSize`/repaint 544 ms. ARCHITECT then directed a redesign of the terminal I/O on the audio-plugin topology (JFS = `jreng-filter-strip`, TETRIS contract). The design is complete and recorded in `PLAN-launch.md` Part B (rulings 16–29 verbatim, steps B1–B8, all points resolved).

### Completed
- Part A in the working tree (not built after the last changes): ImageLoader content memo, file/memory sources, encode chain mirroring decode, SIMD unpremultiply; `applyTo` deleted → `Css::getValueTree`; settings-from-disk contract restored (D3 struck); TextEditor `load`, materialise at layout; `GraphicsEngine (juce::OutputStream&)`; `Typeface` removed from the terminal engine; `style.css` `--TextEditor--backgroundColourId`; jam_terminal keeps `jam_style`.
- Auditor item 13 fixed: every libpng `setjmp` is in a `noexcept` frame with trivial locals (`readPngHeader`, `readPngRows`, `writePngRows`; `jam_ImageLoader.cpp:444-456`, `:475-507`, `:654-675`), JUCE pattern `juce_PNGLoader.cpp:389-404`.
- KANJUT DSP contract restored: `kuassa/user_modules/codebase-for-dummies/docs/TETRIS.md` (947 lines, sha256 `12281fe0…6208`, from `9571d363`); ARCHITECT also placed a copy at `kuassa/user_modules/TETRIS.md`.
- `PLAN-launch.md` rewritten: Part A status + Part B design.

### Remaining
- Part B steps B1–B8 (`PLAN-launch.md`): `TerminalFIFO` (juce::OutputStream over juce::AbstractFifo), `AnsiBuffer` + GraphicsEngine on TETRIS verbs `prepare`/`process`/`reset` (delete `beginFrame`/`endFrame`, all draw scratch allocated in `prepare`), `TerminalProcessor` (chain member), `EVEAudioProcessor` as the chain, `EVEProcessor::prepareToPlay`/`process` drive it, AnsiDocument append path `addLines`, ViewEditor owns AnsiDocument and pulls (Analyzer timer shape), TextEditor reflow over stored advances (resize currently does not reflow: `visibleAreaChanged` only calls `setContentSize`, `jam_TextEditor.cpp:59-63`).
- Part A leftovers: Steps 2, 5, 6, 7, 9, 10 status must be checked against the working tree.
- Open Auditor items: 3, 6, 8, 9, 15, 20, 29, 32.
- Remove the ephemeral `launch` timestamp lines (`jam_AudioStandaloneApp.cpp`, `EVEViewComponents.cpp`, `jam_TextEditor.cpp`, `jam_TerminalGraphicsEngine.cpp`).
- Build/measure by ARCHITECT; @Auditor once; doxygen pass (stale blocks above `createPngReadState` and `readPngRowsToImage` in `jam_ImageLoader.cpp`); log; `carol debt clear DEBT-20261016T214137`.

### Key Decisions
- Topology identical to JFS (ARCHITECT: *"ISOMORPHIC, ANALOGOUS, IDENTICAL topology on how Audio Plugin designed. JFS is the example"*). `processBlock` is the clock and drains the bytes. TETRIS layers: plugin processor → chain → members; allocation only in `prepare` (*"Processor IS ALLOCATE"*).
- AnsiDocument is the append-only scrollback SSOT on the message thread, owned by ViewEditor, cleared only by `clear`, never by SIGWINCH/resize (*"why reparsing? FUCK NO…"*). AnsiDocument = parse, reflow, metadata; TextEditor = materialise; Processor = transport.
- TerminalGraphics sends bytes, is thread agnostic: Component-driven on the message thread for a TUI and for this sprint's harness; a chain processor for the real terminal.
- Names ruled: `TerminalProcessor`, `TerminalFIFO`, `AnsiBuffer`, TETRIS verbs only. `TerminalProcessor` is NOT YAGNI (*"EVE is actual terminal emulator… you bypass the foundation, you will writing WRONG MENTAL MODEL"*).
- One viewport screen (winsize) per send; ring sized by the winsize; ring capacity ≠ scrollback cap.

### Files Modified
- `jam/jam_graphics/image_formats/jam_ImageLoader.h/.cpp` — content memo, encode chain, setjmp frames.
- `jam/jam_graphics/fonts/jam_SimdBlend.h` — SIMD unpremultiply.
- `jam/jam_gui/widgets/jam_TextEditor.h/.cpp` — load, materialise at layout, image registration.
- `jam/jam_terminal/graphics/jam_TerminalGraphicsEngine.h/.cpp` — OutputStream sink, no Typeface.
- `jam/jam_terminal/jam_terminal.h` — dependencies.
- `jam/jam_web/css/jam_Css.h/.cpp` — `applyTo` deleted, `getValueTree` overloads.
- `jam/jam_markdown/document/jam_MarkdownDocument.cpp` — `isImage` without decode.
- `jam/jam_plugin_bootstrap/*` — settings tree names.
- `eve/Source/EVEViewComponents.cpp` — harness chain (to be replaced by Part B).
- `eve/Source/layout/style.css` — TextEditor background colour.
- `eve/PLAN-launch.md` — Part B.
- `cast/Source/Help.cpp`, `stamp/Source/Main.cpp` — Css API callers.
- `kuassa/user_modules/codebase-for-dummies/docs/TETRIS.md` — restored.

### Open Questions
- None in the plan. ARCHITECT's standing instruction: never ask what the discussion or the code answers; execute.

### Next Steps
- Read `PLAN-launch.md` Part B and `kuassa/user_modules/TETRIS.md`. Start B1 (`TerminalFIFO`): owner APIs read — `juce::AbstractFifo` (`juce_AbstractFifo.h:92-112`, `:154-204`, scoped `read`/`write` `:292`, `:314`), `juce::OutputStream` pure virtuals (`juce_OutputStream.h:66`, `:75`, `:81`, `:95`), non-seekable sibling `GZIPCompressorOutputStream::setPosition` → `jassertfalse; return false;` (`juce_GZIPCompressorOutputStream.cpp:163-167`). Delegate B1–B8 to Engineer with no build; ARCHITECT builds.

---

## Sprint: No Panel, Settings on cmd+, — EVE on the Optional-Panel Bootstrap and the Settings Vocabulary ✅

**Date:** 2026-10-19
**Duration:** one long session (jam Sprint 169 is the primary record: decisions, design, audit triage, violations)
**Plan:** `PLAN-optional-panel-settings.md` (approved; last step the log)

### Decisions (ARCHITECT)
1. *"without PanelLayout.html, determinisitically Panel should not be created at all"*; **"Delete About"**; Settings *"created with keyboard shortcut"*, **"cmd+,"**.
2. *"stick to EVERYTHING using 'Settings', only diverge with markdown vs xml"*; lane type **"SettingsDocument<FormatDocumentType>"**.

### Files Modified
- Deleted: `Source/layout/PanelLayout.html`, `AboutLayout.html`, `Source/images/JRENG_logo.png`, `Source/buttons/settings_{normal,over,down}.svg`, `cast/lookuptables.md`, `Source/generated/LookupTables.h`.
- `cast/files.md`: `panel layout`, `about layout`, `settings normal/over/down` rows out; `config directory` → `markdown settings directory`, `default config` → `markdown settings file name`.
- `cast/identifiers.md`: `animationStrip`, `buttonDialog`, `developer`, `jreng`, `jrengLogo` rows and `## tags` out. `cast/spell.md`: lookup-table and tags blocks out. `project-info.md`: `images` glob row out, `html` description.
- `Source/EVEView.h/.cpp`: `initialisePanels`, `attachAboutDialog` out; `getOrCreateSettings`; `initialiseTheme (settings)`.
- `Source/EVEViewPanelCallbacks.cpp`: only the `UIScale` callback. `EVEViewRegistration.cpp`: settings ButtonSVG, ButtonLogo, ButtonDialog out. `EVEViewComponents.cpp`: AnimationStrip, ScrambledText, `aboutBox` content out.
- `Source/EVEProcessor.h/.cpp`: `jam::SettingsDocument<jam::ConfigDocument>`, `files::markdownSettingsDirectory`, `files::markdownSettingsFileName`.
- `Source/layout/ViewLayout.md`: `## keys` row `settings | string | cmd+,`; version `0.0.2`.
- `PLAN-optional-panel-settings.md`: new.

### State for Continuation
- Build `cast cast/spell.md --debug`: `EXIT=0`, 0 errors, 0 warnings. ARCHITECT runs EVE Standalone: no panel row; `cmd+,` opens Settings; APPEARANCE / UI_SCALE write to `~/.config/end/eve.md` (re-seeded by version 0.0.2).
- `CLAUDE.md` Source/layout row still lists `PanelLayout.html` and `AboutLayout.html`.
- Next (ARCHITECT): the TextEditor state machine with ValueTree.

### Debts Paid
- None

### Debts Deferred
- None

## Sprint: One Atom, One Style Word, Both Lanes on One SSOT — CMYK Mermaid, Display-Scale Diagrams ✅

**Date:** 2026-10-17
**Duration:** one long session (jam Sprint 168 is the primary record: decisions, design, audit triage, violations)
**Plan:** `~/.claude/plans/playful-greeting-locket.md` (approved; ARCHITECT *"proceed until /log"*)

### Decisions (ARCHITECT)
1. *"BOTH lane must be IDENTICAL, ANALOGOUS, ISOMORPHIC, even implemented with the EXACT SSOT method"*; **"One style word per run"**; **"Font alias sets size"**; **"Each font use is a role"**.
2. *"mermaid.css colour scheme need to converge with terminal.css … cast --help css is the CMYK scheme"*; *"i dont like the white bg for railroad, should be inverted"*; version **"0.0.1"**.

### Files Modified
- `Source/EVEViewComponents.cpp`:
  - `StyleMarkdown { Id::toType (Id::style), appearance }` is built once with its told sheet.
  - The engine is told `setColorMode`; the pixel scale is the TextEditor scale × `Displays::getDisplayForRect (…)->scale` (precondition asserted).
  - Wire true colours are registered on the TextEditor's LookAndFeel before `setDocument`; `setIndexedColours` is gone; `getOrCreateMonoMetrics`.
- `Source/layout/style.css`:
  - 646 `--MermaidDiagram--…ColourId` rows (323 per appearance block, both blocks identical, EVE's pattern) map jam's mermaid scheme to CMYK through `--terminalIndexed11/13/14/15/233`. Fill brightness follows jam's own label pairing; each label takes its contrast from its fill.
  - 71 `--MermaidDiagram--…FontId` slots point at `var(mono)`; `var(--terminalIndexedN)` replaces `--terminalPaletteN`.
- `Source/layout/ViewLayout.md:2`: `version: 0.0.1`.
- `markdown/markdown.md`: HELP.md appended (harness).

### State for Continuation
- Run `cast` for jam and eve before the build (new jam `.cpp` manifest rows). ARCHITECT builds and checks the mermaid diagrams, headings, decorations and images in the TextEditor.
- Open rulings are listed in jam Sprint 168 (eve-side: the round-trip harness duplication with cast, `std::tmpfile` on Windows, `__FILE__` harness path, stale `CLAUDE.md` / `SPEC.md` lines).

### Debts Paid
- None

### Debts Deferred
- None

## Sprint: Terminal LLGC Round Trip — TextEditor as the Harness ✅

**Date:** 2026-10-16
**Duration:** one session (jam Sprint 167 is the primary record: decisions, design, audit triage, violations)
**Plan:** `~/.claude/plans/playful-greeting-locket.md` (approved; last step the log)

### Decisions (ARCHITECT)
1. **"One chain in EVE"**; **"TextEditor only"**; *"delete llgc parity. This sprint IS the llgc parity with TextEditor as the harness"*.
2. **"Images and mermaid"**; **"Diagram is an Image"**; *"terminal.css, but config contract is IDENTICAL"*.

### Files Modified
- `Source/EVEViewComponents.cpp`:
  - `getCapturedBytes` (`std::tmpfile`, positive nesting) and `getTerminalFrame` (the `cast --help` chain on `markdown/markdown.md`, inline images on).
  - The TextEditor registration, its config (`AnsiDocument::parse (data, size)` into `TextEditor::setDocument`) and `registerStyle<jam::StyleCustom> (Id::terminal)`.
  - The `MarkdownComponent` and `StyleMarkdown` registrations are deleted, and the URL is now `Id::companyWebsite`.
- `Source/layout/component.md`: one row, `textEditor` with style `terminal`. `Source/layout/ViewLayout.md:2`: version 0.0.2.
- `project-info.md`: `jam_terminal` user module; `@terminal-style` alias; `## binary` row `terminalStyleSheet`.
- `cast/identifiers.md`: `terminal`, `textEditor`. `cast/files.md`: `harness` (`markdown.md`).
- Generated by cast: `CMakeLists.txt`, `Source/generated/Identifiers.*`, `Files.*`.
- `tests/llgc_parity/`: deleted.

### State for Continuation
- Builds: `cast cast/spell.md --debug` 0 errors, 0 warnings (after the doxygen pass and the KANJUT sync).
- ARCHITECT runs EVE Standalone. The window shows the TextEditor with `markdown.md` through the terminal round trip. The first launch replaces `~/.config/end/eve.md` with the default (version 0.0.2).
- `tests/ansi_fixpoint` calls APIs that jam no longer declares.
- `CLAUDE.md` still lists `tests/llgc_parity` and the TextEditor-era layout.

### Debts Paid
- None

### Debts Deferred
- None

