# PLAN: END Blank Glass Window on Vulkan — jam_plugin_bootstrap Rewrite

**RFC:** RFC-END-terminal-grand-scheme.md (consumed; this plan implements the pre-terminal milestone: standalone shell + Vulkan glass)
**Date:** 2026-08-30
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM (LANGUAGE.md reference implementation — header-only preferred ≤~300 LOC; 30/3 unchanged)

## Overview

Rewrite jam_plugin_bootstrap from scratch as the canonical Document-direct plugin UI machinery (Descriptor tier dead), and stand up END's blank glass window on Vulkan from canon markdown + CSS data. Locked ARCHITECT rulings govern: scratch rewrite; Registry = dispatch tables (no Document inheritance); Processor-side AST ownership; monolith master Document + sibling CssDocument; manifest table + extension dispatch replaces hardcoded `load()`; Document-driven lockstep walk; ID-only components; Vulkan from day one.

## Dependency & API Inventory (Pathfinder-verified)

- **jam::Document engine** — complete-at-creation AST, stable `Element*`, `Owner<Element>` arena (jam_Document.h:25–34); `SharedDocuments : SegmentedHashMap<Identifier, unique_ptr<Document>>` (:690–694) as the owned registry; parsers `MarkdownDocument/Html/Css/Xml::getOrCreate()` all converge on it (jam_XML.h:27).
- **jam::Registry** — `Function::Map` members Make/Style/SetImage/Configure/BindTo/BindToView/BindToModel/BindToProcessor/Attach/AttachEvent/Events/MakeContent (jam_data_structures/registry/jam_Registry.h:26). Survives untouched.
- **jam::AudioModel** (jam_data_structures/model/jam_AudioModel.h:17, extends APVTS), **ParameterManager** (parameter/jam_ParameterManager.h:12), **StyleManager/StyleTheme** (jam_style/style_manager/jam_StyleManager.h — binds CssDocument, materialises colours/fonts). All survive untouched.
- **Descriptor tier (to delete)** — `Descriptor<int>` (jam_graphics/registry/jam_Descriptor.h:20) + `Descriptors` singleton (jam_data_structures/registry/jam_Descriptor.h:19). Grep-verified: referenced **exclusively** from jam_plugin_bootstrap — deletion is fully contained.
- **jam::VulkanEngine** — `Instance<VulkanEngine>`, registers `ComponentPeer::externalContextFactory` at construction (jam_VulkanEngine.h:74); per-window `VulkanGraphics` via `createContext(peer)` (:555–615); consumer precedent nexus Main.cpp:96–125 (maxImageExtent from primary display physical bounds, frame budget from refresh rate, pipeline cache file, gpuEnabled). Rewritten PluginEditor keeps its established `SharedInstance<VulkanEngine>` member pattern (jam_PluginEditor.h).
- **Build** — JAM `BuildSetup.cmake` auto-discovers all jam_* modules; `configure_app`/`configure_plugin` declarative entry (jam/cmake; cake CMakeLists.txt:12–64 is the sibling precedent). END's current inline 400+-line CMakeLists bypasses all of it.
- **Keep-list inside plugin_bootstrap** (Descriptor-free, verified): `jam_ViewSize.h`, `jam_ScaledContent.h`, `jam_AudioStandaloneApp.h/.cpp`. `jam_PluginEditorLayout.h` survives rewritten (Step 3).
- **Established containers/idioms:** `jam::HashMap`, `jam::Function::Map`, `jam::LookupTable`, `SharedInstance`, `Owner`, `files::`/`Id::`/`map::` generated lexicon.

## Names Ratified (NAMES.md Rule -1 — ratified by plan approval)

| Name | Kind | Rationale |
|---|---|---|
| `interface.md` / `files::interfaceLayout` | END master layout resource | joins `files::panelLayout/aboutLayout/settingsLayout` family (Rule 5) |
| `documentParsers` | dispatch table (extension → `getOrCreate`) | parsing is a real named operation (Rule 6 agent-noun test: `serializers`/`fetchers` class); table-noun per Rule 8 |
| `getMaster()` | PluginEditorLayout accessor to the monolith master Document | verb contract `get` — returns, no mutation |
| `buildView()` | ViewManager entry — walks one view subtree | verb-noun, replaces `build()`'s Descriptor iteration |

## Validation Gate

Each step is validated by COUNSELOR before the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint. ARCHITECT runs all builds and tests.

## Steps

### Step 1: Canon data — END repo
**Scope:** `end/Source/layout/` — author `interface.md`, rewrite `style.css`; delete `PanelLayout.html`, `AboutLayout.html`, `SettingsLayout.html`, `metadata.md` (its formats table folds into interface.md). `DefaultSettings.xml` stays (descriptor-free XML settings path unchanged).
**Action:** @Engineer authors `interface.md` per CAST conventions: (a) manifest table — `document | file | notes` rows declaring the document set (interface.md itself, style.css, DefaultSettings.xml); (b) `editor` view section — one table carrying window width/height/scale and background style role (role name, never a colour). `style.css` minimal: palette + appearance custom properties (light/dark), window background — mirroring cast's style.css shape.
**Validation:** data declares roles not appearances (RFC §8); every identifier appears once (SSOT); tables parse via `MarkdownDocument::parse` conventions; no layout semantics invented beyond what Step 5 consumes.

### Step 2: Delete-first — jam repo
**Scope:** delete `jam_graphics/registry/jam_Descriptor.h`, `jam_data_structures/registry/jam_Descriptor.h` (Descriptors), and plugin_bootstrap view machinery: `jam_ViewManager.h/.cpp`, `jam_ViewManagerEditor.cpp`, `jam_ViewManagerPanel.cpp`, `jam_ViewManagerContent.cpp`, `jam_ViewEditor.h/.cpp`, `jam_ViewPanel.h`, `jam_ViewSettings.h`, `jam_ViewContent.h`, `jam_PluginEditor.h`; update `jam_plugin_bootstrap.h/.cpp` includes + template instantiations; scrub the two registry includes from the owning module headers.
**Action:** @Engineer deletes the listed files (`rm` delegated), updates module headers so the module compiles as the reduced set (ViewSize, ScaledContent, PluginEditorLayout, AudioStandaloneApp). Old code never coexists with new (Refactor-Rewrite Discipline); remaining compiler errors are the ground-truth worklist for Steps 3–6.
**Validation:** zero references to Descriptor/Descriptors anywhere in jam (grep); module header include list matches surviving files; no compatibility shims.

### Step 3: PluginEditorLayout rewrite — manifest-driven, Processor-owned
**Scope:** `jam_plugin_bootstrap/layout/jam_PluginEditorLayout.h` (rewrite in place).
**Action:** @Engineer rewrites: (1) constructor takes the master resource name; parses it via `MarkdownDocument::getOrCreate` first; (2) reads the manifest table rows; per row dispatches by file extension through `documentParsers` (`jam::LookupTable`/`HashMap`: md/html/xml → domain `getOrCreate`, css → `Css::getOrCreate`) — hardcoded `load()` (:65–74) dies; (3) markdown-family documents splice into the monolith master (CAST Model precedent — `appendChildren` adoption, jam_Document.h:538–581); CssDocument stays sibling; (4) `getMaster()` returns the master Document; `SharedInstance<SharedDocuments>` ownership unchanged inside the struct — the struct itself is now owned by the product's **Processor** (locked ruling), passed to the editor at construction. `loadDefaultSettings()`/`populateTree()`/`isReady()` survive with signatures adjusted to the manifest.
**Validation:** no hardcoded resource names in load path (all from manifest rows); extension dispatch is a lookup, not a branch chain (L-3); splice preserves provenance; ownership chain B-clean: Processor → PluginEditorLayout → SharedDocuments → unique_ptr<Document>.

### Step 4: PluginEditor rewrite — base editor, no Descriptors
**Scope:** new `jam_plugin_bootstrap/view/jam_PluginEditor.h`.
**Action:** @Engineer writes the base editor: constructor takes `AudioModel&` and `PluginEditorLayout&` (references to Processor-owned objects — established KANJUT-lineage pattern); owns `SharedInstance<VulkanEngine>`, `SharedInstance<StyleManager>`, `SharedInstance<Registry>` (no `SharedInstance<Descriptors>`); lifecycle `initialise()`: `initialiseTheme()` → `initialiseRegistry()` (product hook) → `initialiseView()`; model listeners for appearance/bypass as before. Nothing else from the old 346-line editor returns unless a Step-5/6 compiler error demands it.
**Validation:** no singleton reach-through where a constructor reference serves; every member has one owner; listeners consume the event payload; VulkanEngine construction parameters follow nexus precedent (display extent, frame budget, cache file, gpuEnabled) via the SharedInstance in-place constructor.

### Step 5: ViewManager + ViewEditor minimal — Document-walk build
**Scope:** new `jam_plugin_bootstrap/view/jam_ViewManager.h` (+ .cpp only if a real TU boundary emerges), new `jam_ViewEditor.h`.
**Action:** @Engineer writes `ViewManager::buildView (const Document::Element& viewRoot, juce::Component& view, Registry& registry, AudioModel& model, StyleManager& style)` — static, stateless: walks the view subtree in lockstep with the (to-be-created) component children; for blank glass the editor subtree carries only the root: apply `ViewSize` from the element's width/height/scale values, resolve the background style role through StyleManager, set on the view's LAF. `ViewEditor` = root component: paints background colour from LAF, `resized()` no-op beyond scaling. Registry tables may be empty this sprint — the dispatch call sites exist, populated by products later.
**Validation:** walk source is the Document (locked ruling); components carry componentID only; zero data copies out of the AST; ViewManager holds no state; style roles resolve at materialisation, never stored resolved (RFC §8).

### Step 6: END rewrite — Processor, View, standalone shell
**Scope:** END repo: delete `Source/generated/ENDDescriptors.h/.cpp`, `ENDViewRegistration.cpp`, `ENDViewComponents.cpp`, `ENDViewPanelCallbacks.cpp`; rewrite `ENDProcessor.h/.cpp`, `ENDView.h/.cpp`.
**Action:** @Engineer: ENDProcessor owns `ParameterManager` setup, `AudioModel` (near-zero parameter layout: bypass + UIScale + appearance base state), and **`PluginEditorLayout`** (Processor-side ruling — constructed with `files::interfaceLayout`); `createEditor()` passes model + layout to ENDView. ENDView extends the new PluginEditor; `initialiseRegistry()` registers nothing yet (blank glass); standalone entry via existing `jam_AudioStandaloneApp`.
**Validation:** END source references only rewritten jam surface; no descriptor vocabulary anywhere; layout constructed once, editor-open path re-parses nothing; formats remain Standalone/VST3/AU.

### Step 7: END build — declarative CMake
**Scope:** END repo `CmakeLists.txt` (replace).
**Action:** @Engineer replaces the 400+-line inline file with the cake-shaped declarative form: `JAM_ROOT` + `include(BuildSetup)` + `configure_plugin` (PluginBuilder) — formats Standalone/VST3/AU, JAM_MODULES per SPEC stack (jam_core, jam_style, jam_markdown, jam_web, jam_graphics, jam_vulkan, jam_freetype, jam_plugin_bootstrap + JUCE audio modules), BINARY_FILES = Source/layout/interface.md, style.css, DefaultSettings.xml. Inline JUCE-patching, module enumeration, Vulkan/shader boilerplate all die — BuildSetup owns them.
**Validation:** CMakeLists is declarative only (no imperative boilerplate); binary set matches the manifest's document set exactly; ARCHITECT builds and runs — glass window appears, Vulkan swapchain confirmed (engine log / `isGpuAvailable`).

### Step 8: Auditor sweep
@Auditor once, whole sprint, against CONTRACT + this locked plan. Findings resolved before sprint log.

## BLESSED Alignment

- **B** — Processor → PluginEditorLayout → SharedDocuments → Documents: one ownership chain, RAII throughout; VulkanEngine per-process via SharedInstance, contexts retired through the engine.
- **L** — extension dispatch and style resolution are lookups; minimal ViewManager is one walk, no phase god-object this sprint.
- **E** — Registry injection at call sites; no singleton reach-through added; roles not colours in data.
- **S/S** — one AST, zero copies (Descriptor/NVS/properties-bag tiers deleted); AudioModel remains the only runtime state machine.
- **E (encapsulation)** — components dumb, ID-only; Tell-don't-ask walk applies style/size top-down.
- **D** — same data → same window; parse memoized, walk deterministic.

## Risks / Open Questions

- **Recorded follow-on (ARCHITECT's reminder, out of this sprint's scope):** CAST generating END's CMakeLists.txt, and CAST release binary installed into the toolchain — both required to complete terminal-grand-scheme phase 1 after this milestone. Step 7's hand-written declarative file is the interim; its shape is deliberately the flattest possible CAST target.
- Html/Css engine parses END's canon files for the first time in jam (markdown path CAST-proven; css consumer StyleManager live) — any parse gap surfaces at Step 1 validation, oracle is cast's own style.css.
- `jam_plugin_bootstrap.cpp` template instantiations (StyleKnob_V1, Model::attach<ViewManager>) reference deleted machinery — Step 2 resolves them by deletion; anything a future sprint needs returns then (YAGNI).
