# SPRINT-LOG.md

**Project:** END — Ephemeral Nexus Display
**Repository:** /Users/jreng/Documents/Poems/dev/end
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

## Sprint: Bootstrap Metadata SSOT + Settings Canon + Glass Appearance ✅

**Date:** 2026-08-31
**Duration:** multi-session

### Agents Participated
- COUNSELOR: fable-5 — JFS canon conformance, white-window/settings causal chains, delegation + per-step disk validation
- Engineer (parallel waves) — ENDView canon shape, ENDProcessor cleanup, style.css rewrite, END CAST chain authoring, settings pipeline files

### Files Modified (~10 total)
- `Source/ENDView.h/.cpp` — FilterStripView canon ctor (isReady gate → initialise → derived size via ViewManager::getUISize); initialiseTheme creates StyleManager + StyleTheme and installs the default LAF; initialiseView builds ViewEditor from the master
- `Source/ENDProcessor.h/.cpp` — getMetadata/getParameterLayout/fake MASTER-Bypass deleted; ctor body empty — model seeds from the settings file via PluginEditorLayout::populateTree; parameterManager default-constructs
- `Source/layout/style.css` — JFS convention rewrite: :root palette, `.appearance.DARK`/`.appearance.LIGHT` records, `.window` glass (backgroundBlur/acrylic10), complete @font-face record, golden preview rules
- `Source/layout/interface.md` — formats table removed (config-time truth moved to the cast metadata table)
- `Source/layout/DefaultSettings.xml` — new: UI_SCALE MEDIUM, APPEARANCE DESKTOP, @product@ placeholder
- `cast/CAST.md`, `cast/metadata.md`, `cast/template.cast` — END's CAST chain: Projucer-equivalent metadata table, ONE unidirectional path (table → CAST → generated statics now, CMakeLists writer deferred)
- `Source/generated/` — cast output, double-run fixpoint (Metadata.cpp → ProjectInfo.h redesign in flight at log time)
- `CMakeLists.txt` — generated TU wired into target_sources; layout/*.xml added to the BinaryData glob

### Alignment Check
- [x] BLESSED principles followed — SSOT (metadata declared once in the table; hand-seeding twin deleted), unidirectional (config-time vs runtime bootstraps never conflated)
- [x] NAMES.md adhered — table/extern names ARCHITECT-ratified
- [x] MANIFESTO.md principles applied
- [ ] Auditor sweep not run — sprint logged on ARCHITECT command; ProjectInfo.h + JUCE_DONT_DECLARE_PROJECTINFO wiring in flight at log time

### Problems Solved
- White window: stale ~/Library/Application Support/END.settings filterState restored LIGHT over the seed; settings pipeline now the single truth — first run writes /Music/JRENG/Settings/END.setting with APPEARANCE DESKTOP, resolved dark/light at the style boundary
- Metadata hardcodes eliminated end to end: Projucer-equivalent table is the sole source; nothing downstream restates a value

### Debts Paid
- None

### Debts Deferred
- None

## Sprint: END Blank Glass on Vulkan — jam_plugin_bootstrap Document-Driven Rewrite ✅

**Date:** 2026-08-30
**Duration:** — (multi-session, compacted)
**Plan:** PLAN-blank-glass-vulkan.md (locked; ARCHITECT re-rulings during execution supersede plan text where noted)

### Agents Participated
- COUNSELOR — plan authoring, per-step gate validation, audit triage (45 findings), two blocker verifications, three ARCHITECT decision surfaces raised
- Engineer (six waves) — canon data, jam delete-first, PluginEditorLayout/PluginEditor/ViewManager/ViewEditor rewrite, END rewrite, CMake edits, two audit-resolution batches + residue micro-batch
- Auditor — single sprint-end sweep, 45 findings, all resolved / rejected-with-citation / logged residual
- Pathfinder — dependency + pattern inventory pre-plan

### Files Modified

**END repo:**
- `Source/layout/interface.md` (new) — canon master: `documents` table (interface.md, style.css), `editor` table (width/height/scale), `formats` table (Standalone VST3 AU — future CAST CMake-generation input)
- `Source/layout/style.css` (new) — `:root` palette, `.DARK`/`.LIGHT` variants (canon UPPERCASE, ARCHITECT-ruled), mandatory `.appearance`
- `Source/ENDProcessor.h/.cpp` — rewritten: owns ParameterManager, AudioModel (bypass param), PluginEditorLayout (`files::interfaceLayout`, `BinaryData::fetcher`); constructor seeds appearance + UIScale state children (kuassa `populateTree` twin shape) from `ParameterManager::getDefaultAppearance()/getDefaultScale()`; metadata SSOT via `JucePlugin_Name`/`JucePlugin_VersionString`
- `Source/ENDView.h/.cpp` — thin `jam::PluginEditor` subclass, ctor calls `initialise()`
- `CmakeLists.txt` — the CAST target oracle (ARCHITECT-ruled: self-contained single file, full-format plugin + standalone, old BuildSetup/PluginBuilder toolchain dead): +jam_markdown, deleted-source prune, binary glob → layout *.md/*.css
- `SPEC.md:28` — build line corrected to self-contained CMakeLists
- Deleted: PanelLayout.html, AboutLayout.html, SettingsLayout.html, metadata.md, DefaultSettings.xml ("if you want defer, delete"), Source/generated/* + ENDViewRegistration/Components/PanelCallbacks

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
- Appearance blocker: END seeded no `Id::appearance` state child → empty string → `juce::Identifier{""}` jassert in StyleManager; seeded in ENDProcessor ctor. Vocabulary conflict (map::Appearance "DARK" vs css `.dark`) ruled: **canon is UPPERCASE** — CSS conformed
- Step 3 first delivery rejected (const_cast mutation of shared cached documents — D violation); rebuilt on owned fresh-parse master (cast Model.h precedent)
- CMake toolchain misread corrected: PluginBuilder/AppBuilder/Metadata.cmake are the abandoned toolchain; END's inline single-file CMakeLists **is** the CAST target oracle

### Debts Paid
- None (DEBT.md empty)

### Debts Deferred (ARCHITECT-ruled)
- kuassa GlyphArrangement shape()/shapeRow() asymmetry — "we deal with it later, when we reach END's TextEditor"
- END settings as markdown — deferred-by-deletion ruling
- CAST generates END's CMakeLists.txt + CAST release binary into toolchain — phase-1 goal after glass window (ARCHITECT's standing reminder)

### Residuals for ARCHITECT
- `CmakeLists.txt` → `CMakeLists.txt` rename (git mv, ARCHITECT's hands)
- `scale` row (editor table) and `formats` table currently reader-less — kept as declared future inputs (UIScale machinery / CAST CMake generation)
- CMake oracle findings left as oracle-shape decisions: binary glob (fonts ride BinaryData for `BinaryData::fetcher`), module set (jam_dsp/jam_animation/jam_debug reachability unproven), spv/ttf dual embedding (jam + BinaryData namespaces)
- Doxygen regen for deleted jam symbols (zero-warning policy check at next jam doc build)
