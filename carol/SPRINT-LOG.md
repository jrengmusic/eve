# SPRINT-LOG.md

**Project:** EVE — Ephemeral Nexus Display
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

## Sprint: TextEditor Foundation — Renderer Core, MODE-Node State, Vim Basic ✅

**Date:** 2026-09-20
**Duration:** one session (continued across compaction, 2026-09-19 → 20)

### Agents Participated
- COUNSELOR (fable-5) — orchestration, PLAN-texteditor-foundation.md (Locked Decisions amended on every ARCHITECT ruling), per-step validation by own reads
- Pathfinder ×1 — inventory (one fabricated `shape()` line caught by read, discarded)
- Engineer ×9 waves — video-lane delete, Index rename, GlyphArrangement first pass, TextEditor move + plugin-lane attempt, main-view rewrite, Step 6 insert/remove ×2, keys + modes + motions, editing/undo/clipboard ×2, status bar, docs
- Auditor — none (ARCHITECT: "NO AUDIT")

### Files Modified (jam 16, eve 14 — key entries)
- `jam_terminal/video/**`, `transport/jam_TerminalCellFifo.h`, `jam_graphics/text/jam_Row.h`, `jam_TerminalLine.h`, `model/jam_TerminalModel.*`, `jam_terminal/widgets/jam_TextEditor.h` — DELETED (video lane, TerminalModel, old widget)
- `jam_core/document/jam_Document.h/.cpp` — `Element::previousSibling` (doubly linked, ARCHITECT-ruled); `addChild (parent, after, id)` positional overload (two-arg delegates), `removeChild`; `Index` renamed vocabulary (`maxBytes`, `numBytes`, `isReleased`, `release`, `writeLine`, `readPage`, `num*`), `insertLine`/`removeLine`/`setNumCells`/`getNumLines`/`getCellRange`/`getRowNumber (line, column)`; `setColumns (0)` = unwrapped
- `jam_core/document/jam_DocumentIndex.cpp` — counted B-tree insert/remove: `addLeaf` (create) → `appendLeaf`/`insertLeaf` (`splitNode` cascade) / `removeLeaf` → `removeNode` + `collapseRoot()`; `addRoot` single root-growth site; `getNumLines (Node*)`; `addNumBytes`; no merge/borrow
- `jam_markdown/document/jam_MarkdownBlockParser.cpp:1683` — `replaceLastChild` on `addChild`/`removeChild` (hand walk deleted)
- `jam_markdown/document/jam_ConfigValidator.h/.cpp` — fourth rule `keys` (every row `type == string`)
- `jam_vulkan/font/jam_GlyphArrangement.h/.cpp` — public first pass `clear()`, `shape (cells, start, end, row)`, private `addEntry`
- `jam_style/jam_StyleCustom.h` — `getMonoMetrics()` beside `getMonoFont()` (LAF owns cell metrics); `jam_style.h` jam_vulkan include ahead of StyleCustom
- `jam_gui/widgets/jam_TextEditor.h/.cpp` — NEW HOME: `juce::Viewport` + `ValueTree::Listener`; ctor `(Document&, Index::Codec, ValueTree editorState)`; state = the MODE parameter node; `valueTreePropertyChanged` → `events` map → `modes` map → virtual `normal()/insert()/visual()`; `GlyphArrangement` painter, metrics from the LAF at construction; `setKeys (mode, tree)`, `keyPressed` → `keyDispatch` lanes → `run` → `commands` (`addMotionCommands`/`addModeCommands`/`addEditCommands`); `rangeOperators`/`lineOperators`; `applyMotion (target, operatorEnd)`, `applyOperator`/`applyVisualOperator`; write paths `setLine`/`addLine`/`removeLine` inside `ReplaceLine`/`InsertLine`/`RemoveLine` UndoableActions; `insertText`/`insertLines`/`deleteRange`/`deleteLines`; clipboard register; cursor/selection painting via `getCellBounds`; `parentHierarchyChanged` focus
- `jam_gui/jam_gui.h/.cpp` — TextEditor included; `jam_terminal.h` include removed
- `jam/cast/identifiers.md` (+47 ids: count, pendingOperator, selectionAnchor→anchorLine/anchorColumn, cursorLine/cursorColumn, keys, insert, visual, textEditor, visualMode, 33 action ids), `bimaps.md` + `spell.md` (`EditorMode`, `EditorState`, `VisualMode`); `generated/jam_Identifiers.h`, `jam_Bimaps.h` regenerated
- `eve/Source/EVEProcessor.h/.cpp` — KANJUT one-liner `AudioModel model { parameterManager, *this, ParameterLayout::get (MarkdownDocument::getOrCreate (files::parametersLayout)) }`; editorModel/createAndAddParameter/MessageOverlay deleted
- `eve/Source/EVEView.h/.cpp` — owns `AnsiDocument document` + `std::unique_ptr<jam::TextEditor> editor` (main view, sibling of `view`, MODE node injected); `getConfigDocument` (one parse/validate/fallback), `setEditorKeys` on build and hot reload; `initialiseRegistry` (juce::Label + `registerViewComponent ("MODE")` with file-static `ChoiceText` ValueSource), `initialisePanels` (status-bar ViewPanel); `resized` trims the editor by `getUIPanelHeight()`
- `eve/Source/layout/parameters.md` — NEW: `mode` choice (normal, insert, visual); `eve.md` — `## keys` table (33 rows, per-mode chords); `PanelLayout.html` — NEW status bar (mode/line/column labels); `ViewLayout.md` — title-only (ViewLayout.html deleted)
- `eve/cast/files.md` + `Source/generated/Files.h` — `panelLayout` row; `SPEC.md` (State Model, Cell-bearing element, Keyboard Summary), `RFC-EVE-terminal-grand-scheme.md` (§5.1 executed, §5.3 Gap 2 amended), `CLAUDE.md` refreshed; `PLAN-texteditor-foundation.md` carries every ruling; `PLAN-document-index.md` deleted

### Alignment Check
- [x] BLESSED — S/SSOT: one state node (MODE), Document the only content, Index the only wrap owner; E: VTPC dispatch tables, virtuals for derived views, no bail-outs, LAF owns metrics; D: metrics established at construction and trusted, `Value::map` zero-range contract, no defensive asserts; B: three Document write paths, UndoManager owns transactions
- [x] NAMES.md — every new member joins a family (`add*`/`get*`/`set*`/`apply*`, `*Operators`, `*Line`); `previousSibling`, `EditorMode`/`EditorState`/`VisualMode`, `ChoiceText` are the new names, ratified by execution under ARCHITECT's no-gate order
- [x] MANIFESTO.md — Lean splits on every function COUNSELOR read; registration tables (`add*Commands`) remain long by nature
- [ ] Auditor — skipped on ARCHITECT's order ("complete the plan. NO AUDIT. NO GATE"); doxygen pass not run (Code Hygiene: post-audit)

### Problems Solved
- Framework lane for the editor: the HTML `buildContent` lane serves bars/dialogs only (ViewPanel/ViewContent/ViewSettings); the SVG editor lane is ruled out; the main view is owned in code, the status bar rides the canonical bottom `ViewPanel` row through the Registry
- Editor state: six `juce::Value`s replaced by one `ValueTree` (the MODE node) with `valueTreePropertyChanged` as a dispatcher and mode virtuals — APVTS propagates `value` writes to the host parameter (juce_AudioProcessorValueTreeState.cpp:442-446)
- Vim keys matched by text character (JUCE `createFromDescription` keeps only the last character upper-cased, juce_KeyPress.cpp:231); `pendingOperator` stores the armed key name, operators are identified by action, so eve.md rebinding never breaks `dd`/`dw`
- `Element` made doubly linked (ARCHITECT ruling) so `removeChild` is O(1) and the block parser's hand walk died
- Shared `$TMPDIR/juce-patched` was re-patched by an END configure (END's patch set lacks the attributed-text hook) — EVE reconfigure restores its stamp; agents no longer build END from EVE work

### State for Continuation
- Verify before acting: `cmake --build Builds/Debug --target EVE_Standalone` green (last: 8c pass); ARCHITECT runtime test pending — navigate `h j k l w b e 0 $ gg G ctrl+u ctrl+d`, counts, `i a I A o O`, typing/return/backspace, `v V ctrl+v` + `d c y`, `dd dw cw cc x p P u ctrl+r`, eve.md `## keys` hot reload, status bar mode/line/column
- Open (RFC amendment): cursor shape as any codepoint; terminal defaults (insert mode, hidden status bar); `Index::setNumCells` keeps line count, `insertLines`/`deleteRange` re-style joined text with one style id; `prefixes` rebuilt from all modes on every `setKeys`
- Doxygen pass owed on: jam_TextEditor.h, jam_Document.h (Index additions, previousSibling), jam_DocumentIndex.cpp, jam_StyleCustom.h, jam_ConfigValidator.h, jam_GlyphArrangement.h — post-audit per Code Hygiene
- Toolchain: eve `cast cast/spell.md` runs the full Release toolchain; the AAX wraptool step fails on a missing iLok licence (pre-existing)

### Debts Paid
- `DEBT-20260908T000000` — superseded by ARCHITECT's NO SVG ruling: no component.md/ViewLayout.svg join; the editor is built in code, the status bar is HTML through the Registry
- `DEBT-20260908T000001` — already satisfied: `EVEAudioProcessor.h:16,:19` declare `jam::ViewManager::Getters`/`Events`

### Debts Deferred
- None

---

## Sprint: TextEditor Viewport Widget + eve.md Config/Style Chain ✅

**Date:** 2026-09-15
**Duration:** one session (continued across compaction)

### Agents Participated
- COUNSELOR (fable-5) — orchestration, seam design, RFC/doc authorship, per-step validation
- Pathfinder ×3 — style-flow facts, MessageOverlay wiring facts, grid-parser trace (killed on ARCHITECT's type-string correction)
- Engineer ×15 waves — config/style/watcher/overlay seams, TextEditor restructures, audit sweep, doxygen pass, hygiene
- Auditor ×1 — 47 findings, all resolved or dispositioned

### Files Modified (jam 20, eve 12, end 4 — key entries)
- `jam_terminal/widgets/jam_TextEditor.h` — rewritten: juce::Viewport subclass; owns AnsiDocument, Document::Index, static wire codec; nested Content painter; visibleAreaChanged → Index::setViewport; setContentSize with column-change gate; font materialised once at lookAndFeelChanged
- `jam_terminal/jam_terminal.h` — jam_style dependency declared + included; include-map comment corrected
- `jam_core/document/jam_Document.h` + `jam_DocumentIndex.cpp` — public `Index::setViewport (firstRow, lastRow)`; paintRow reads via the readying `getElement`; getPrefixTotal param nouns
- `jam_data_structures/parameter/jam_ParameterManager.h/.cpp` — explicit ctor (settingExtension default), `getOrCreateUserSettings (defaults, document, configFile)` overload, 2-arg delegates, `isSettingsOld` const&
- `jam_plugin_bootstrap/layout/jam_PluginEditorLayout.h` — md lane overloads: `isReady/loadDefaultSettings/isSettingsValid/populateTree (…, configFile)`; ConfigValidator gate + UI-scale parity + xml-parity debug::Log
- `jam_style/style_manager/jam_StyleManager.h/.cpp` — ValueTree ctor (config/darkConfig), public `registerStyle (ValueTree, ValueTree)` reload API, `hasFont`, `registerFont (alias, filename, height, kerning)`, md window-read branches, null-guards; jam_style purged of markdown types (layer law — the compiler exposed jam_style below jam_markdown via jam_mermaid_diagram)
- `jam_style/style_manager/jam_StyleTheme.h/.cpp` — `getMonoFont` override (hasFont-gated, StyleCustom fallback); `jam_StyleCustom.h` getMonoFont → juce::FontOptions
- `jam_gui/widgets/jam_MessageOverlay.h/.cpp` — absorbed from END; fontCallback member, noSplitLine at decision points, axisLineStyle default from map::OverlayAxisLine
- `jam_markdown/document/jam_ConfigDocument.h/.cpp` — `getValueTree (rootType, valueColumn)`; 1-arg delegates with Id::value
- `jam/cast` — identifiers.md rows message/overlay/config/mono; bimaps.md + spell.md OverlayAxisLine
- `eve/Source/EVEProcessor.h/.cpp` — ctor injection into EVEView (getters deleted); OVERLAY row + Id::message ParameterText (messageCapacity) on terminalModel; layout carries files::defaultConfig
- `eve/Source/EVEView.h/.cpp` — configFile from cast strings (`.config/end` + `eve.md`); isReady gate with binary-defaults fallback + overlay validator error; dual-source initialiseTheme; fileChanged hot-reload (validate → registerStyle trees → broadcast); two-line resized via theme->getWindowPadding; ViewEditor::create + editor size both read eve.md
- `eve/Source/layout/eve.md` — NEW: settings (UI_SCALE/APPEARANCE), UI_size (SSOT), colours palette, window chrome, style (light/dark), fonts (mono DisplayMonoBook 12) — pandoc grids, uniform 100-col, type strings int/float/colour/string
- `eve/Source/layout/ViewLayout.md` — title-only (UI_size moved); `style.css`, `DefaultSettings.xml` — DELETED
- `eve/cast/files.md` — configDirectory ".config/end", defaultConfig "eve.md"; `CLAUDE.md` file table refreshed; `RFC-EVE-terminal-grand-scheme.md` — Gap 2 executed amendment + NEXT SPRINT mandate; both PLAN files deleted
- `end/Source` — MessageOverlay files deleted, call sites repointed to jam::MessageOverlay

### Alignment Check
- [x] BLESSED — B: TextEditor owns its document machinery; E: chain extended at seams only, zero parallel channels; S: eve.md single config truth, UI_size single home; D: event-driven end to end (watcher + DarkModeSettingListener)
- [x] NAMES.md — lexicon strings verified at source (int/float/bool, `--`-prefixed colour ids); setContentSize/fontCallback/registerStyle family shapes
- [x] MANIFESTO.md — layer law enforced by compiler evidence; fonts materialised at state updates
- [x] Auditor once (47 findings: fixed in 4 waves; rejections cited to rulings/CODING's own forms; records listed below)

### Problems Solved
- Blank/crashed launches root-caused by evidence, not guesses: silent md gates (now xml-parity logged), zombie editor on gate failure (now binary-defaults + overlay error), grid `type` cells using C++ id names instead of lexicon strings ("int"/"float"), ViewEditor's non-template getUISize consumer of the deleted ViewLayout table
- jam_style ↔ jam_markdown cycle exposed by compiler; style lane redesigned to jam_core ValueTree types
- END-generated map::OverlayAxisLine moved into jam cast for shared MessageOverlay

### State for Continuation
- NEXT SPRINT (ARCHITECT-ruled, verbatim in RFC-EVE-terminal-grand-scheme.md §5.3 Gap 2 amendment): jam::TextEditor as a full-feature vim-based text editor — text editing; modal insert/normal/visual; configurable cursor shape (any codepoint incl. emoji); terminal defaults insert-mode/status-bar-hidden with Visual-mode copy; terminal wires in afterwards like nvim's own terminal (refs: ~/Documents/Poems/dev/endless/ SPEC.md:44-45,:98-152; ~/Documents/Poems/dev/neovim/)
- Verify before acting: build green post-audit-sweep (ARCHITECT's build at log time), doxygen regen zero warnings
- Open ARCHITECT rulings: eve.md window `close` row (title-strip clearance, jam_ViewContent.h:83-86); END's duplicate OverlayAxisLine cast blocks (end/cast/bimaps.md:1-17, spell.md:105-111) collide at END's next regen; jam_style.h:10 deps line still names jam_markdown (build-graph change)
- On-record residuals: defaultFixture absolute path (EVEView.cpp:4, dies with the pty lane); isSettingsOld Xml::parse noise on md files; ButtonTab unguarded static_cast; textEditorId + "TERMINAL" tree id pending cast registration; StyleManager config-lane parity gaps (metrics/appearance/multi-font — unused by eve.md today); window opacity 0.75 on disk (ARCHITECT-tuned)

### Debts Paid
- None

### Debts Deferred
- None commanded this sprint. Ledger carries DEBT-20260908T000000 (editor lane join) and DEBT-20260908T000001 (processor registries) — untouched, outside this sprint's scope, standing for the next scope per JRENG law.

## Sprint: Document::Index — Counted B-tree Lookup + Residency Tier (buffer-bench verdict → clean-room jam) ✅

**Date:** 2026-09-15
**Duration:** one session (spans 2026-09-14 → 15)
**Plan:** PLAN-buffer-bench.md (executed, then deleted per hygiene ruling — evidence lives in tests/buffer_bench/artifacts/results.md) → PLAN-document-index.md (locked, executed)

### Agents Participated
- COUNSELOR (fable-5) — two /goplan cycles, ruling relay, per-step validation, two audit triages (41 + 38 findings, all resolved), RFC/SPEC amendments
- Pathfinder ×2 — harness/Document inventory; Index-sprint discovery (Document internals, EVE config lane)
- Librarian ×1 (domain-mode) — rope/line-index survey, cited: Boehm–Atkinson–Plass 1995, Ropey, xi-rope, Tatham counted B-tree, JumpRope, Vim memline, VS Code piece tree, Zed SumTree
- Engineer ×10 waves — bench harness (Steps 1–5 + audit fixes + spill lane), jam Index core, tier, EVE wiring, audit resolution, L extractions + doxygen authoring
- Auditor ×2 — buffer-bench sweep (NEEDS_WORK, 41 findings) and document-index sweep (NEEDS_WORK, 38 findings); every finding resolved or corrected in-sprint

### Decisions (ARCHITECT-ruled)
1. Document stays the SSOT — no rewrite; the one-shot AST contract holds; editing is line-level state update, never a second content model.
2. Name: `Document::Index` (over LineIndex/Line/Lines) — joins Document's nested vocabulary family (Property/Span/Token/Cells).
3. Full tier inside Index: budget + watermark eviction + wire-byte spill + rehydrate.
4. Budget default 1/16 machine RAM, ceiling not reservation; EVE override row `scrollbackBudgetMb` (camelCase — ARCHITECT corrected my snake_case); the shipped table was REMOVED so the machine-relative default is the live path.
5. Spill format: ANSI wire bytes — the byte-fixpoint-proven lane.
6. Candidate triangle measured (counted B-tree, width skip-list vs bare walk, cached anchor); winner counted B-tree, arity 16; ONE winner ships — no dual mode.
7. Editor-first build order affirmed (the RFC's own §5.3/§6.1 ordering).

### Files Modified (eve)
- `tests/buffer_bench/` (NEW) — sandbox harness: CMakeLists.txt, main.cpp, Anchor.h, CountedTree.h, SkipList.h, ProjectInfo.h; artifacts/results.md + per-run files. Measured verdict: index beats every chain strategy 4–5 orders of magnitude at 10⁶–10⁷ lines; spill 89 B/line, rehydrate ~100 µs/50-line page, byte-exact round trip; backward scroll is structurally O(N) on the chain (no prevSibling, jam_Document.h:194-197)
- `Source/EVEView.h/.cpp` — `documentIndex` (tiered) over ansiDocument; static `getCodec` (jam_terminal encode + AnsiDocument::parse); framework single-call table/row config read; file-scope Identifiers
- `Source/layout/ViewLayout.md` — net unchanged (terminal table added Step 4, removed at audit per decision 4)
- `RFC-EVE-terminal-grand-scheme.md` — [AMENDED 2026-09-15] ×3 (§3.3 Index ruling + constants, §5.3 gap 1 closed, §7.4 deferral withdrawn) + sourced-figure correction (310–850 B/line resident, per-run artifacts)
- `SPEC.md` — Feature 2.3 residency tier ("no line is ever lost to the tier"); Feature 6 override row; State Model Index sentence
- `PLAN-document-index.md` (NEW, locked); `PLAN-buffer-bench.md` deleted

### Files Modified (jam)
- See jam/carol/SPRINT-LOG.md Sprint 122 — jam_Document.h (Index surface + doxygen), jam_DocumentIndex.cpp (NEW), jam_core.cpp registration, jam_BinaryCodec.h/.cpp DCF enforcement

### Alignment Check
- [x] BLESSED — B: Owner<Node> flat ownership, TemporaryFile RAII; S: eviction is whole-Cells state update, residentBytes single owner; E: Codec injected at construction, jam_core gained zero upward includes (layer law audit-verified); L: every function ≤30 lines after addLeaf/addSpan extraction; D: fixed-seed gates, byte-exact spill fixpoint, append/evict interleave gate
- [x] NAMES.md — ratified set in PLAN-document-index.md plus in-sprint ratifications: appendCarry, addCounts, setCold, addLeaf, addSpan, getPath/getLeftmostHotPath/getResidentBytes
- [x] CODING.md — audit-verified after resolution of all 38 findings
- [ ] Doxygen zero-warning UNVERIFIED — regen entry point is nvim `<leader>bd` only (build-doxygen.sh is nvim-state-driven); ARCHITECT runs interactively or accepts author-only

### Problems Solved
- First bench matrix invalidated (-O0 build + use-after-free in the append lane) — caught by audit, fixed, re-measured at -O3
- `cells->clear()` retains malloc'd capacity — eviction freed nothing; fixed by whole-object Cells replacement (the plan's own "complete replaces complete")
- Stale ancestor `isCold` under append/evict interleave (throw path) — setCold recompute + new interleave gate
- Blank-line rehydrate null dereference — empty parse ⇒ empty Cells, never dereference
- Hand-rolled config table lookup → framework getTables/getTableRow single calls
- Unsourced 602 B/line RFC figure → corrected to the sourced 310–850 range

### State for Continuation
- Verify before acting: buffer_bench, ansi_fixpoint, llgc_parity all exit 0 (2026-09-15); EVE Debug Standalone/VST3/AU link clean
- `documentIndex` has NO production reader yet — the viewer paints the document directly; viewport-driven paint through getElement is the buffer/widget sprint; an evicted line paints empty until then (unreachable at shipped defaults)
- Index lifetime contract: raw Element* leaves — rebuild the Index after any re-parse (doxygen states it)
- CAST identifier-lane residual: "terminal"/"scrollbackBudgetMb" are file-scope statics in EVEView.cpp, not generated Id:: entries
- Unexplained one-off: one bench run exited 1 with completely empty output, never reproduced
- First-light goal (HANDOFF.md — EVE renders ls.ansi on screen) still open, untouched by this sprint

### Debts Paid
- None

### Debts Deferred
- DEBT-20260908T000000, DEBT-20260908T000001 — standing from the prior ledger; outside this sprint's ARCHITECT-defined scope

## Sprint: Terminal LLGC Full Feature Parity — One-Shot Viewer + llgc_parity ✅

**Date:** 2026-09-13
**Duration:** one session
**Plan:** PLAN-terminal-llgc-parity.md (locked; jam-side implementation logged in jam/carol/SPRINT-LOG.md Sprint 119, same date)

### Agents Participated
- COUNSELOR: fable-5 — design lock, plan, delegations, per-step validation, 48-finding audit triage
- Librarian ×1, Pathfinder ×4, Engineer ×9 waves, Auditor ×1 — full roster in jam Sprint 119

### Files Modified (eve repo)
- `PLAN-terminal-llgc-parity.md` — NEW: the locked plan (cell law, patch seam, both image lanes, ratified names)
- `project-info.md` — sixth patch row (`juce-attributed-text-hook.patch`) in the `## patch` table; `jam_terminal` row in `## user module`
- `CMakeLists.txt` — cast-regenerated only: six-entry patch list, jam_terminal module + link lines
- `tests/ansi_fixpoint/CMakeLists.txt` — consumes the PATCHED JUCE tree via the mirrored stamp machinery (same /tmp target, idempotent with eve); `addRegistrySeed` deleted from main.cpp (NAMES Rule 6 `seed` ban; Stamp now self-interns its default)
- `tests/llgc_parity/` — NEW: the sprint's formal proof. Four suites: (1) native-component byte-fixpoint + text-content oracle (Label/TextButton/GroupComponent under LookAndFeel_V4, headless); (2) software-renderer pixel oracle over a flat-geometry fixture, zero mismatch budget, AA-boundary + fractional-coverage exemptions only, box-drawing drawLine coverage; (3) image round-trip — OSC 1337 decoded by TerminalITerm2Decoder pixel-exact, half-block downsample + fixpoint, covered-region blank asserted non-vacuously; (4) rounded-rect bg inheritance, curved cell-granular clips, transparency colour-lerp with character survival. Artifacts dump to `artifacts/*.ansi` for `cat` eyeball
- `Source/EVEView.h/.cpp` — one-shot viewer: `ansiDocument` + `terminalView` (jam::TextEditor) members; fixture from the standalone command line (cwd-relative safe); full-bounds temporary placement (ARCHITECT amends visually); `setReadOnly` call removed with the API's death

### Alignment Check
- [x] Both harnesses green from eve root; EVE_Standalone Debug links clean; three consecutive llgc_parity runs byte-identical
- [x] The backend-swap proof stands: the same `juce::Graphics` surface renders to Vulkan (EVE viewer) and to escape bytes (harness) with zero component knowledge

### Problems Solved
- EVE compiles jam_terminal for the first time (module row + cast regen — 3 generated lines)
- The harness exposed two real writer defects and one fix interaction before any consumer shipped (detail in jam Sprint 119)

### How ARCHITECT Runs It
- Viewer: `./Builds/Debug/EVE_artefacts/Debug/Standalone/EVE.app/Contents/MacOS/EVE tests/ansi_fixpoint/fixtures/ls.ansi`
- Eyeball: `cat tests/llgc_parity/artifacts/*.ansi` in any truecolor terminal

### Debts Paid
- None commanded

### Debts Deferred / Ledger Standing
- DEBT-20260908T000000 (component.md editor-lane join) and DEBT-20260908T000001 (processor registry retypes) — standing, target the EVE plugin lane, untouched this sprint
- Builds/Release partial (killed runaway cast build; artifacts only)

## Sprint: AnsiDocument — the Sixth Document Domain (core + fixpoint) ✅

**Date:** 2026-09-13
**Duration:** one session
**Plan:** PLAN-ansidocument.md (locked; two ARCHITECT re-rulings executed in place: role-indexed palette, TextLayout death pulled forward; harness relocated jam→eve mid-sprint)

### Agents Participated
- COUNSELOR: fable-5 — RFC/spec/pipeline discovery, plan authoring, per-step disk validation, two audit triages, five disclosed failures below
- Pathfinder ×5: jam/SKiT survey, Vulkan LLGC chain, endless coverage diff, hygiene sweep, Document-domain pattern inventory
- Engineer (six waves): deletions, Cells row, CAST lanes, AnsiDocument/AnsiValidator, fixpoint harness, two audit-resolution waves, relocation, doxygen pass
- Auditor ×2: sprint-end sweep (3 blockers/11 major/28 minor/10 doc), re-audit (R1-R31) — every finding fixed or rejected with a citation

### Files Modified
**eve repo:**
- `PLAN-ansidocument.md` — NEW: the locked plan, amended in place for Step 4b (role-indexed palette), the TextLayout death override, and the harness relocation
- `tests/ansi_fixpoint/` — NEW home (relocated from jam): self-contained console target; `main.cpp` fixpoint loop (parse → validate → derive → emit → re-parse → byte-compare); `ProjectInfo.h`; fixtures: `ls.ansi`, `wide.ansi`, `grapheme.ansi`, `hyperlink.ansi`, `colon.ansi`, `invalid/unterminated.ansi`, `generate.sh`
- jam-repo surface logged in jam/carol/SPRINT-LOG.md Sprint 116 (same date)

### Alignment Check
- [x] BLESSED — **S**: SGR flag↔wire vocabulary declared once on its owner (`Stamp::sgrFlagCodes`/`underlineStyleCodes`), consumed by decoder, encoder, and harness; **E**: ANSI at two edges (decode in AnsiDocument, encode in GraphicsEngine); **D**: the fixpoint is the formal proof — byte-identical re-emission, automated
- [x] NAMES.md — AnsiDocument/AnsiValidator/Cells/Id::cells/Id::ansi/Id::unterminated/AnsiTokenType ratified at plan approval; every in-flight helper coordinator-ratified against the Verb Contract and Rule 5 families
- [x] CODING.md — audit-verified twice; zero input asserts in the parser (wire bytes are data; AnsiValidator is the failure channel)
- [x] Doxygen pass complete: zero warnings (doxygen 1.18.0) over all touched files; R26 prose ghosts of the deleted DFA trio rewritten
- [x] Harness green from eve root: all fixtures fixpoint-stable, full Stamp vocabulary sweep (14 flag rows, palette 1/9/196 fg/bg, truecolor, colon-form), OSC 8 real round-trip, invalid fixture rejected. eve Debug green throughout

### Problems Solved
- The terminal write lane is now a Document domain: bytes → AnsiDocument AST (line elements carrying `Cells`) → both LLGC materialisations. TerminalParser/DispatchTable/Transition deleted; their knowledge re-authored to the Document contract (byte-class table via CAST, token dispatch via Function::Map)
- Role-indexed palette (ARCHITECT ruling): cells carry the palette index; RGB resolves at materialisation — no palette table at parse or encode; basic-16 maps onto palette 0-15
- Writer completeness enforced by the fixture oracle: flag attributes, palette, underline truecolor, and OSC 8 hyperlinks all emit — silent-drop classes eliminated
- eve never compiled jam_terminal (absent from the module list); the harness is the module's first and only compile gate

### COUNSELOR Failures, Disclosed
- Asserted "deleting TextLayout orphans markdown draw callers" — zero callers existed; grep disproved it after ARCHITECT challenged
- Cited `MarkdownLayout`/`MarkdownDocument::drawRows` as family precedent throughout the session — both are phantoms from stale doxygen; the widget-sprint premise needs re-grounding
- Plan Step 1's validation premise ("jam module compiles via eve build") was impossible as written — no eve target compiles jam_terminal
- Plan Step 2 predicted two visitor edits; the :443 visitor walks Token::Value, not Element::Value — one edit, compiler-proven
- Handed ARCHITECT harness build commands after he had transferred the build duty to COUNSELOR

### Debts Paid
- None drained (`carol debt clear` not run — no entry was in this sprint's scope)

### Debts Deferred / Ledger Standing
- DEBT-20260908T000000 (component.md editor-lane join) and DEBT-20260908T000001 (processor registry retypes) — standing, untouched: both target the EVE plugin build lane this sprint never entered; they enter the next EVE sprint per JRENG law
- TerminalLine death — deferred (CellFifo carries it end-to-end; CellFifo ruled untouched); dies with the live-lane FIFO redesign

### Residuals for ARCHITECT (unfiltered)
- `Document::preprocess` rewrites CR/CRLF/FF→LF and NUL before any domain parses — the fixpoint holds over preprocessed bytes, not raw wire (engine-wide; five domains affected by any change)
- Non-SGR C0 (TAB, BS, …) consumed without trace per the locked plan's content-species scope — live-lane jaw
- `Stamp::code` flag is dead — defined, never produced, never consumed; its doc had claimed phantom consumers
- `applyExtendedColor*Form` write-target pointers dispositioned as the `apply` verb's caller-supplied target — override is yours
- Stamp index-0 blank entry stays session-owner responsibility (VulkanEngine precedent; harness conforms)
- Harness `ProjectInfo.h`/CMakeLists are hand-authored outside the CAST lane; ProjectInfo consumer requirement undocumented for non-plugin consumers
- git-diff capture fixture omitted (agents run no git) — add by your hand if wanted
- eve module list still lacks jam_terminal (SPEC.md:26 expects it) — EVE consumer sprint, via project-info.md
- RFC §3.4 "TerminalParser stands as-is" superseded; RFC :245/:326 still name TextLayout; SPEC carries the nine feature gaps — your amendment pass
- TerminalVideo has no byte-feed driver until the §7.2 streaming unification sprint



**Date:** 2026-09-09
**Duration:** one edit (side effect of jam Sprint 96 — Vulkan campaign port)

### Agents Participated
- COUNSELOR: fable-5 — root cause + fix (ARCHITECT-approved single line)

### Files Modified (1 total)
- `cast/cmake.cast:333` — `HEADER_NAME "JamVulkanShaderData.h"` → `"jam_VulkanShaderData.h"`. jam's umbrella includes the snake name (`jam_vulkan.h:220`, pre-dating the port); the camel name was ECHO-era lexicon (ECHO is dead — the snake name syncs via the ordinary filePrefix token row with no special whole-name step). Generated `CMakeLists.txt` follows on the next cast regen.

### Alignment Check
- [x] BLESSED / NAMES / MANIFESTO — one-line data fix, matches KANJUT's `kuassa_VulkanShaderData.h` convention

### Problems Solved
- Fatal `'jam_VulkanShaderData.h' file not found` at `jam_vulkan.h:220` — eve generated the binary-data header under the dead camel name; mismatch predates the jam port (backup-verified).

### Debts Paid
- None

### Debts Deferred
- None

---

*(older sprints rotated to git history)*
