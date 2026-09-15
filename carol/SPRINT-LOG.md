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

## Sprint: XML Prologue Parse + Settings Gate Diagnostics + Audit Sweep ✅

**Date:** 2026-09-02
**Duration:** multi-session (compacted twice)

### Agents Participated
- COUNSELOR: fable-5 — causal-chain diagnosis (two chained root causes), KANJUT comparison, audit triage, per-step disk validation
- Engineer (five waves) — XML token types + tree construction, ParameterManager dead-code + diagnostics, PluginEditorLayout contract, Document extractions, Lean splits
- Auditor — single sprint-end sweep; findings resolved, or withdrawn with a MANIFESTO citation
- Pathfinder — call-site inventory for the dead-getter deletions

### Files Modified (~18 total)

**EVE repo:**
- `Source/EVEView.cpp:8` — JFS gate `if (layout.isReady (model))` replaces `jassert (layout.isReady (model))`; the FilterStripView canon shape, verbatim
- `Source/EVEView.h:10` — out-of-line `~EVEView()` deleted; it duplicated `jam::PluginEditor::~PluginEditor`'s `setDefaultLookAndFeel (nullptr)`
- `project-info.md` — `Reserarch` typo fixed; `Source/generated/ProjectInfo.h` regenerated, fixpoint confirmed
- `SPEC.md`, `CLAUDE.md`, `carol/SPRINT-LOG.md` — END residue renamed to EVE per ARCHITECT ruling ("EVE is END, renamed"), under dry-run/backup/verify protocol
- Deleted: `PLAN-blank-glass-vulkan.md` — objective complete

**jam repo:**
- `cast/bimaps.md` — `XmlTokenType` gains `processingInstruction`, `declaration`, `comment`; `generated/jam_Bimaps.h` regenerated, fixpoint confirmed by cksum
- `jam_core/xml/jam_XML.h` — `getPrologue`/`getDeclaration` emit their own token types; new `skipToken` consumes them during tree construction; comments tokenise in `getToken` with a named `commentCloseLength`; `build()` adopts the parsed element as `root`
- `jam_core/xml/jam_XmlValidator.h` — sentinel test `root->firstChild != nullptr` becomes `not root->id.isNull()`
- `jam_core/document/jam_Document.cpp/.h` — local `node` renamed `element` (NAMES Rule 6); three raw `std::string[]` to `.at()`; extracted `isCommentOpen`, `getQuote`, `getDepth`, and an `openToken` lambda
- `jam_core/debug/jam_Log.h` — `path()` to `getPath()`; `~Scope()` clears logger and logFile; template parameters named `Argument`/`DecayedArgument`
- `jam_data_structures/parameter/jam_ParameterManager.h/.cpp` — missing `#pragma once` added; four dead getters deleted (`getVersionString`, `getProductWebsite`, `getUserManual`, `getDefaultOrientation`, zero call sites across jam/eve/end/tit); three dead `!= nullptr` conjuncts removed; extracted `isScaleValid` and `getScaleElement`; six stale doxygen blocks naming a vanished `Metadata` constructor corrected to `ProjectInfo`
- `jam_plugin_bootstrap/layout/jam_PluginEditorLayout.h` — `jassert (ParameterManager::getInstance() != nullptr)` at the single ownership boundary; three downstream doubts removed; extracted `isDocumentsValid`, `getDocumentValidators`, `getFileExtension`, `addParameters`; `populateTree` returns `parameter.isValid()` directly
- `jam_plugin_bootstrap/view/jam_PluginEditor.h` — `initialiseListeners()` deleted (zero overrides); `darkModeSettingChanged()` no longer re-checks the instance
- `jam_graphics/.../jam_SVG.cpp`, `jam_AttributedGraphics.cpp`, `jam_style/jam_StyleTogglePush.cpp` — `root->firstChild` reverted to `root`

### Alignment Check
- [x] BLESSED — **D**: the sentinel-root doubt removed at the owner, not re-checked downstream; **E**: every runtime failure branch names itself through `debug::Log`; **S/SSOT**: `document.root` is the one root
- [x] NAMES.md — `skipToken`, `getDepth`, `getQuote`, `isCommentOpen`, `getScaleElement`, `getDocumentValidators` join existing families (Rule 5); `operatorHandler` renamed `operatorFunction` (Rule 6)
- [x] CODING.md — `.at()` over raw `[]`, `not (` spacing, no bail-outs, no underscores
- [ ] `Document::getTokens` stays at 83 lines. It is one tokeniser loop with one responsibility; the two real responsibility boundaries are already extracted. MANIFESTO **L**: a helper carved out only to move lines below the limit is relocation, not decomposition

### Problems Solved
- Blank/crashing window had two chained root causes. (1) jam's XML tokeniser emitted the `<?xml ?>` prologue as a full-span `text` token; `addText` rejects non-whitespace text before the root, so no document with a prologue could parse. jam merged kuassa's separate `Token::text`/`length` fields into one `Span`, which destroyed the "consume bytes, carry no text" property. (2) After that fix, `build()` left a sentinel root, so `getChildByID("SETTINGS")` on `document.root` found nothing. Both are porting defects — jam changed the data shape without migrating the consumers
- ARCHITECT ruling applied: `jam::XmlDocument` is a drop-in replacement for `juce::XmlElement`. `build()` adopts the parsed element as `root`; the four consumers I had shifted to `root->firstChild` were reverted
- The failure was invisible because `isSettingsValid()` and `populateTree()` returned false with no diagnostic while every other gate logged. Design by Contract fix applied

### Debts Paid
- None (no DEBT.md in the repository)

### Debts Deferred
- None

### Residuals for ARCHITECT
- `debug::Log::write` is a no-op without a live `Log::Scope`. `PluginEditorLayout` parses during **Processor** construction, before any View exists, so the `Log::Scope` at `EVEView.h:18` cannot capture parse-time diagnostics
- Eleven audit findings were withdrawn as training priors, each with a MANIFESTO citation: F1, F5, F9, F23, F37, F39, F47, F50, F52, F66, F67

*(older sprints rotated to git history)*
