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

## Sprint: Bootstrap Metadata SSOT + Settings Canon + Glass Appearance ✅

**Date:** 2026-08-31
**Duration:** multi-session

### Agents Participated
- COUNSELOR: fable-5 — JFS canon conformance, white-window/settings causal chains, delegation + per-step disk validation
- Engineer (parallel waves) — EVEView canon shape, EVEProcessor cleanup, style.css rewrite, EVE CAST chain authoring, settings pipeline files

### Files Modified (~10 total)
- `Source/EVEView.h/.cpp` — FilterStripView canon ctor (isReady gate → initialise → derived size via ViewManager::getUISize); initialiseTheme creates StyleManager + StyleTheme and installs the default LAF; initialiseView builds ViewEditor from the master
- `Source/EVEProcessor.h/.cpp` — getMetadata/getParameterLayout/fake MASTER-Bypass deleted; ctor body empty — model seeds from the settings file via PluginEditorLayout::populateTree; parameterManager default-constructs
- `Source/layout/style.css` — JFS convention rewrite: :root palette, `.appearance.DARK`/`.appearance.LIGHT` records, `.window` glass (backgroundBlur/acrylic10), complete @font-face record, golden preview rules
- `Source/layout/interface.md` — formats table removed (config-time truth moved to the cast metadata table)
- `Source/layout/DefaultSettings.xml` — new: UI_SCALE MEDIUM, APPEARANCE DESKTOP, @product@ placeholder
- `cast/CAST.md`, `cast/cmake.cast` — EVE's CAST chain: Projucer-equivalent metadata table, ONE unidirectional path (table → CAST → generated statics now, CMakeLists writer deferred)
- `Source/generated/` — cast output, double-run fixpoint (Metadata.cpp → ProjectInfo.h redesign in flight at log time)
- `CMakeLists.txt` — generated TU wired into target_sources; layout/*.xml added to the BinaryData glob

### Alignment Check
- [x] BLESSED principles followed — SSOT (metadata declared once in the table; hand-seeding twin deleted), unidirectional (config-time vs runtime bootstraps never conflated)
- [x] NAMES.md adhered — table/extern names ARCHITECT-ratified
- [x] MANIFESTO.md principles applied
- [ ] Auditor sweep not run — sprint logged on ARCHITECT command; ProjectInfo.h + JUCE_DONT_DECLARE_PROJECTINFO wiring in flight at log time

### Problems Solved
- White window: stale ~/Library/Application Support/EVE.settings filterState restored LIGHT over the seed; settings pipeline now the single truth — first run writes /Music/JRENG/Settings/EVE.setting with APPEARANCE DESKTOP, resolved dark/light at the style boundary
- Metadata hardcodes eliminated end to end: Projucer-equivalent table is the sole source; nothing downstream restates a value

### Debts Paid
- None

### Debts Deferred
- None

## Sprint: EVE Blank Glass on Vulkan — jam_plugin_bootstrap Document-Driven Rewrite ✅

**Date:** 2026-08-30
**Duration:** — (multi-session, compacted)
**Plan:** PLAN-blank-glass-vulkan.md (locked; ARCHITECT re-rulings during execution supersede plan text where noted)

### Agents Participated
- COUNSELOR — plan authoring, per-step gate validation, audit triage (45 findings), two blocker verifications, three ARCHITECT decision surfaces raised
- Engineer (six waves) — canon data, jam delete-first, PluginEditorLayout/PluginEditor/ViewManager/ViewEditor rewrite, EVE rewrite, CMake edits, two audit-resolution batches + residue micro-batch
- Auditor — single sprint-end sweep, 45 findings, all resolved / rejected-with-citation / logged residual
- Pathfinder — dependency + pattern inventory pre-plan

### Files Modified

**EVE repo:**
- `Source/layout/interface.md` (new) — canon master: `documents` table (interface.md, style.css), `editor` table (width/height/scale), `formats` table (Standalone VST3 AU — future CAST CMake-generation input)
- `Source/layout/style.css` (new) — `:root` palette, `.DARK`/`.LIGHT` variants (canon UPPERCASE, ARCHITECT-ruled), mandatory `.appearance`
- `Source/EVEProcessor.h/.cpp` — rewritten: owns ParameterManager, AudioModel (bypass param), PluginEditorLayout (`files::interfaceLayout`, `BinaryData::fetcher`); constructor seeds appearance + UIScale state children (kuassa `populateTree` twin shape) from `ParameterManager::getDefaultAppearance()/getDefaultScale()`; metadata SSOT via `JucePlugin_Name`/`JucePlugin_VersionString`
- `Source/EVEView.h/.cpp` — thin `jam::PluginEditor` subclass, ctor calls `initialise()`
- `CmakeLists.txt` — the CAST target oracle (ARCHITECT-ruled: self-contained single file, full-format plugin + standalone, old BuildSetup/PluginBuilder toolchain dead): +jam_markdown, deleted-source prune, binary glob → layout *.md/*.css
- `SPEC.md:28` — build line corrected to self-contained CMakeLists
- Deleted: PanelLayout.html, AboutLayout.html, SettingsLayout.html, metadata.md, DefaultSettings.xml ("if you want defer, delete"), Source/generated/* + EVEViewRegistration/Components/PanelCallbacks

**jam repo:**
- `jam_plugin_bootstrap/layout/jam_PluginEditorLayout.h` — rewritten: manifest-driven `parseMaster` (documents table → `documentParsers` dispatch keyed on `Extensions::md/html/css`), monolith master via fresh `MarkdownDocument::parse` (owned value member, zero const_cast), CSS sibling via `Css::getOrCreate`
- `jam_plugin_bootstrap/view/jam_PluginEditor.h` (new) — base editor: `SharedInstance<VulkanEngine>` (primary-display extent), `SharedInstance<StyleManager>`, appearance `juce::Value` listener; `initialise()` = theme → view; size from Document truth (`Id::editor` width/height)
- `jam_plugin_bootstrap/view/jam_ViewManager.h` / `jam_ViewEditor.h` (new) — stateless `buildView` (editor-table size), root component painting `ResizableWindow::backgroundColourId`
- `jam_vulkan/context/jam_VulkanGraphics.cpp` — `#if JUCE_WINDOWS` guards on composition barrier calls (guard symmetry audit-verified)
- Deleted: both jam_Descriptor.h, jam_Registry.h/.cpp ("delete now, rebuild per-need"), jam_Traits.h (all traits orphaned), old view machinery (ViewManager/Editor/Panel/Settings/Content shards, old PluginEditor.h), jam_ViewSize.h, jam_ScaledContent.h, jam_PageSelector.h, jam_ParameterDescriptor.h, jam_Model.inl (+ dead `Model::attach<ManagerType>` declaration), StyleKnob_V1 instantiation, empty registry/ dirs
- `cast/identifiers.md`, `cast/files.md` — +`documents`/`formats`/`interfaceLayout`; −aboutLayout/defaultSettings/panelLayout/settingsLayout/editorLayout; `jam/generated` regenerated, fixpoint confirmed
- Doxygen/prose scrub (sanctioned post-audit pass): jam_plugin_bootstrap.h, jam_data_structures.h, jam/.claude/CLAUDE.md — Descriptor vocabulary removed

### Alignment Check
- [x] BLESSED — B: Processor → PluginEditorLayout → SharedDocuments chain; S/SSOT: Descriptor/NVS carrier tiers deleted, size/appearance truths single-sourced; D: fresh-parse master, no cached-document mutation
- [x] NAMES.md — ratified set only: interface.md, files::interfaceLayout, documents, description, documentParsers, getMaster, buildView, parseMaster (family-joining)
- [x] CODING.md — audit-verified: zero `!`/`&&`/`||`, zero raw `[]`, zero bail-outs, zero underscores, zero forward declarations in new files
- [ ] Runtime verification pending: ARCHITECT builds — glass window appears, `.DARK` background lands, Vulkan swapchain confirmed

### Problems Solved
- Appearance blocker: EVE seeded no `Id::appearance` state child → empty string → `juce::Identifier{""}` jassert in StyleManager; seeded in EVEProcessor ctor. Vocabulary conflict (map::Appearance "DARK" vs css `.dark`) ruled: **canon is UPPERCASE** — CSS conformed
- Step 3 first delivery rejected (const_cast mutation of shared cached documents — D violation); rebuilt on owned fresh-parse master (cast Model.h precedent)
- CMake toolchain misread corrected: PluginBuilder/AppBuilder/Metadata.cmake are the abandoned toolchain; EVE's inline single-file CMakeLists **is** the CAST target oracle

### Debts Paid
- None (DEBT.md empty)

### Debts Deferred (ARCHITECT-ruled)
- kuassa GlyphArrangement shape()/shapeRow() asymmetry — "we deal with it later, when we reach EVE's TextEditor"
- EVE settings as markdown — deferred-by-deletion ruling
- CAST generates EVE's CMakeLists.txt + CAST release binary into toolchain — phase-1 goal after glass window (ARCHITECT's standing reminder)

### Residuals for ARCHITECT
- `CmakeLists.txt` → `CMakeLists.txt` rename (git mv, ARCHITECT's hands)
- `scale` row (editor table) and `formats` table currently reader-less — kept as declared future inputs (UIScale machinery / CAST CMake generation)
- CMake oracle findings left as oracle-shape decisions: binary glob (fonts ride BinaryData for `BinaryData::fetcher`), module set (jam_dsp/jam_animation/jam_debug reachability unproven), spv/ttf dual embedding (jam + BinaryData namespaces)
- Doxygen regen for deleted jam symbols (zero-warning policy check at next jam doc build)
