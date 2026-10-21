# PLAN: EVE launch — native TextEditor render, round trip through framework API, cast without hand-built style

**RFC:** RFC-EVE-terminal-grand-scheme.md (architecture); objective from DEBT-20261016T214137
**Date:** 2026-10-20
**BLESSED Compliance:** verified against the rulings and the reads below
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (CODING: one object, one TU)
**Approved:** ARCHITECT, 2026-10-20 (plan mode). Part B revised and approved 2026-10-20 (plan mode). The last step is the log.

## Context

DEBT-20261016T214137: *"EVE opens in subseconds, grand total, like jfs; find and remove every bottleneck on the launch path."*

Measured: the launch waits on `getTerminalFrame` (full-document paint, 46 diagram renders, PNG encodes) and on the tmpfile capture (`EVEViewComponents.cpp:3-60`). EVE hand-builds the style, cell size, rows and capture in project code. cast hand-builds a config tree (`Help.cpp:6-9`). TextEditor materialises at paint (`jam_TextEditor.cpp:141`, `:165`). The editor reads the whole settings tree from disk at creation (`EVEView.cpp:9`), with every stylesheet written to markdown and parsed again (`jam_PluginEditorLayout.cpp:510-515`).

## Rulings (ARCHITECT, this session, verbatim)

1. CONTRACT: *"0 alloc, 0 copy, 0 mutation, 0 temp containers, 0 handroll arithmetic, 0 handroll method …, 0 shadow state, 0 repetitive pattern, 0 redundancy … DESIGN BY CONTRACT"*.
2. Scope: **"jam eve"**; *"also fix cast"*. *"THERE SHOULD NOT BE ANY EXCUSES THAT MAKES RUNTIME BEHAVIOR BECOMES WORSE"*.
3. *"GPU Vulkan and its fast lane software (fallback) IS NON NEGOTIABLE"*. **"NO WAIT"**. **"Memo by content"**. **"Load queues decodes"**.
4. *"harness is only read file"*; *"load is TextEditor responsibility. Document always parse."*
5. *"TextEditor is able to write any inline image, IT SHOULD BE. mermaid is just image."*
6. *"MarkdownDocument as complete AST holds ALL METADATA AS IT NEEDS; Image is just metadata; Mermaid Graphics is just image; NO MATERIALISATION at paint(), NEVER; paint only call MATERIALISED raster, EVERYTING IN SINGLE LOOP"* (the JFS pattern).
7. jam_gui gains jam_markdown and jam_mermaid_diagram: *"PROVISION not kernel. KANJUT will be synced, KANJUT use markdown, but NOT mermaid. KANJUT does not need TextEditor"*.
8. The round trip stays (jam SPRINT-LOG:229, :232). The Component that paints the frame: **"TextEditor"**. The owner: **"EVE calls framework API"**.
9. Frame sink: *"GraphicsEngine is terminal lane ONLY? juce::OutputStream&"*.
10. cast: *"what the fuck is wrong with MarkdownComponent? why cast cant use MarkdownComponent, and eve use TextEditor?"*; *"what the fuck is wrong with having juce::Component with TerminalGraphics?"*
11. *"mono always cell, proportional always logical pixel … Style always give the complete metrics, it doesnt care what unit. LLGC always use the correct unit into its own domain. NO POKING LAYERS. the arithmetic is IDENTICAL."*
12. Chain: *"MarkdownComponent > StyleMarkdown > StyleManager > Css; TextEditor > ... same"*. **"StyleManager with overload for css"**.
13. *"the name is wrong. PluginEditorLayout::getSettingsTree"*; *"the tree that holds CSS sheet is the Settings dialog component. NOT StyleManager. it always on demand, read from disk to ValueTree"*.
14. Settings lanes: *"JFS has two lane, per instance: non automatable parameter and GLOBAL for new INSTANCE"* — `populateTree` at processor creation stays (`FilterStripProcessor.cpp:34`).
15. Earlier, still standing: **"Both"** (window first, each image encoded once per frame); **"Hold for the frame"**; *"DONT FUCKING BUMP VERSION"*; no `getDocumentConstruction`, no new lanes.

## Decisions taken from reads (approved with the plan)

- D1. cast and the EVE frame source keep the framework tell calls `StyleMarkdown::setCellSize` / `setPixelCellSize` / `setPixelScale` and `GraphicsEngine::resize (cols, rows, scale)`. The terminal context maps x one unit per column and y by the vertical scale (`jam_TerminalGraphicsContext.cpp:7-15`, `:620-642`); the stylesheet unit is the cell (cast `markdown.css:5-8`). No hand-built config tree, no capture. (Part B Step B3 renames `resize` to `prepare`.)
- D2. TextEditor reads every style value through `StyleCustom` virtuals. The `StyleMarkdown` members it needs become `StyleCustom` virtuals with the same names (`getLineFonts`, `getCellBounds`, `getPixelBounds`), the `getLineFont` / `getLineMetrics` pattern (`jam_StyleCustom.h:74-92`, `jam_StyleMarkdown.h:123-174`).
- D3. Struck. ARCHITECT: *"READING SETTINGS FROM DISK CONTRACT UNCHANGED"*. `initialise (layout.getOrCreateSettings())` and `initialiseTheme (settings)` stay.
- D4. `jam_terminal` drops its `jam_gui` dependency (`jam_terminal.h:17`, `:54`); no use found; the compiler confirms.

## Dependency & API Inventory (reads this session)

- **Image SSOT:** `ImageLoader` (`jam_ImageLoader.h`): `getFromFile`, `getFromMemory`, `registerImages (files)` (`.cpp:595-633`), content-hash memo (`:641-668`). JFS: engine queues (`jam_VulkanEngine.cpp:45`), build reads (`jam_ViewManager.cpp:304`), object stores (`jam_StyleKnob.cpp:11-14`), paint draws.
- **Markdown AST:** `MarkdownDocument::parse (text, origin)` (`.h:72`), `addLines()` (`:400`), `getImageFile` (`:436`); lines carry `Id::cells`, `Id::tokens`, `Id::images`, `Id::row` (`jam_MarkdownDocument.cpp:1009-1043`); `addObjectLine` (`:1074-1086`).
- **Wrap:** `ReflowDocument::setRows (line, width, cutWidth, measure)` (`jam_ReflowDocument.h:102`); sibling call `jam_MarkdownComponent.cpp:209-215`.
- **Glyphs:** `GlyphArrangement::getDrawRuns`, `getAdvance` (`jam_GlyphArrangement.h:282-335`); `Run::paint (g, laf, defaultColour)` (`:177`).
- **Raster:** `AttributedImage { image, bounds }` with `paint` (`jam_AttributedGraphics.h:190-203`); sibling `MarkdownGraphics::addObjectImage` (`jam_MarkdownGraphics.cpp:255-261`).
- **Mermaid:** `MermaidDiagram::parse (text, StyleMermaid&)` (`jam_MermaidDiagram.h:28`), `MermaidGraphics (diagram, style)` (`jam_MermaidGraphics.h:31`), raster sibling `getOrCreateDiagramImage` (`jam_MarkdownComponent.cpp:304-327`) with `LowLevelGraphicsGlyphRenderer<juce::LowLevelGraphicsSoftwareRenderer>` (`jam_LowLevelGraphicsGlyphRenderer.h:51-75`).
- **Terminal:** `GraphicsEngine` (`jam_TerminalGraphicsEngine.h:31-225`), `GraphicsContext` (`jam_TerminalGraphicsContext.h:84`), `AnsiDocument::parse (const char*, int)` (`jam_AnsiDocument.h:105`), `getTrueColours` (`:114`).
- **JUCE streams:** `juce::OutputStream`, `juce::MemoryOutputStream` (`juce_OutputStream.h:49`, `juce_MemoryOutputStream.h:47`).
- **Style:** `StyleManager (ResourceFetchers, juce::ValueTree)` (`jam_StyleManager.h:32`); Css loop siblings `jam_PluginEditorLayout.cpp:517-533`, `Help.cpp:6-9`.
- **Layers:** jam_gui → jam_document, jam_style, jam_graphics (`jam_gui.h:2-26`); jam_markdown and jam_mermaid_diagram do not depend on jam_gui (`jam_markdown.h:2-20`, `jam_mermaid_diagram.h:2-22`).

## Validation Gate

COUNSELOR validates each step before the next, by reading the changed files, against MANIFESTO.md, CODING.md, NAMES.md and this plan. @Auditor runs once, after the last code step.

## Steps — Part A (launch path)

### Step 0 (done, kept): settings merge
Superseded: `applyTo` deleted; `Css::getValueTree (rootType, sheetTrees)` / `(rootType, styleSheets)` (`jam_Css.cpp:2181-2217`).

### Step 1 (done, kept): ImageLoader content key, file and memory sources
`getFromFile`, `getFromMemory`, `registerImages (files)`, content-hash memo (rulings 3). Encode mirror of the decode chain; `longjmp` frames hold only trivial locals (`jam_ImageLoader.cpp:444-456`, `:475-507`, `:654-675`).

### Step 2: Revert `MarkdownDocument::load`
**Scope:** `jam_MarkdownDocument.h:74`, `.cpp:53-66`.
**Action:** delete the declaration and the body. `getImage` keeps `ImageLoader::getFromFile (getImageFile (image))` (`:911-914`).
**Validation:** no caller left (compiler); ruling 4.
**Status (read 2026-10-20):** done — `load` absent from `.h` and `.cpp`; `getImage` at `jam_MarkdownDocument.cpp:898-901`.

### Step 3 (done, kept): each distinct image encoded once per frame
`appendInlineImage (text, png, width, height)`; `ImageLoader::getPng` (rulings 15).

### Step 4 (done): GraphicsEngine writes to `juce::OutputStream&`
`explicit GraphicsEngine (juce::OutputStream& outputToUse, …)` (`jam_TerminalGraphicsEngine.h:41`); `output.write` (`.cpp:210`).

### Step 5: Module dependencies
**Scope:** `jam_gui/jam_gui.h`, `jam_terminal/jam_terminal.h`, `user-modules-info.md` rows.
**Action:** jam_gui adds `jam_markdown`, `jam_mermaid_diagram` (ruling 7). jam_terminal keeps `jam_style` (the compiler needs `map::ColourId`).
**Validation:** no cycle; build of eve, whelmed, cast.
**Status (read 2026-10-20):** done — `jam_gui.h:18-19`, `user-modules-info.md:97-98`; `jam_terminal.h:10-16` without jam_gui.

### Step 6: Style virtuals for TextEditor (D2)
**Scope:** `jam_StyleCustom.h/.cpp`, `jam_StyleMarkdown.h/.cpp`, `jam_MarkdownGraphics.h/.cpp`, `jam_TextEditor.cpp`.
**Action:** `getLineFonts`, `getCellBounds`, `getPixelBounds` as `StyleCustom` virtuals; `StyleMarkdown` overrides.
**Validation:** names are the existing family; MarkdownComponent output unchanged; one font build (S).
**Status (read 2026-10-20):** done — `jam_StyleCustom.h:94-98`; overrides `jam_StyleMarkdown.h:87`, `:109`, `:176`.

### Step 7: Mermaid block is an image line in the AST (ruling 6)
**Scope:** `jam_MarkdownDocument.cpp:1074-1086`.
**Action:** `addObjectLine` lists a mermaid block in `Id::images` as it does an image block.
**Validation:** MarkdownComponent output unchanged.
**Status (read 2026-10-20):** done — `jam_MarkdownDocument.cpp:1067-1070`.

### Step 8: `TextEditor::load` and materialisation at layout
**Scope:** `jam_TextEditor.h/.cpp`.
**Action:** `load` parses, queues decodes, `addLines`, `setDocument`. Images materialise once at layout (`addImages`, `jam_TextEditor.cpp:115-130`); paint reads `runs.at` / `images.at` in one loop.
**Validation:** ruling 6.
**Status (read 2026-10-20):** done — `jam_TextEditor.cpp:35-52`, `:103`, `:328`, `:346`.

### Step 9: StyleManager from Css; cast without config tree
**Scope:** `jam_StyleManager.h/.cpp`; cast `Help.cpp`.
**Action:** `StyleManager (ResourceFetchers, const jam::Strings& styleSheets)` via `Css::getValueTree`; cast deletes its hand-built tree.
**Validation:** ruling 12; cast `--help` bytes unchanged.
**Status (read 2026-10-20):** done — `jam_StyleManager.cpp:12-16`; cast `Help.cpp:10`.

### Step 10: Settings tree name (ruling 13)
**Scope:** `jam_PluginEditorLayout.h/.cpp`, `jam_PluginEditor.h/.cpp`, `EVEView.h/.cpp`, whelmed `WHELMEDView.*`.
**Action:** `getValueTree (rootType)` is renamed `getSettingsTree`. The editor reads the disk settings at creation, unchanged (D3 struck).
**Validation:** ruling 13.
**Status (read 2026-10-20):** done — `jam_PluginEditorLayout.h:150`, `.cpp:512`.

### Step 11: superseded by Part B (Steps B5, B6).

## Part B — terminal I/O on the JFS topology (TETRIS)

### Rulings for Part B (ARCHITECT, verbatim)

16. *"DESIGN BY CONTRACT. must be ISOMORPHIC, ANALOGOUS, IDENTICAL topology on how Audio Plugin designed. JFS is the example"*.
17. *"no tty, only the foundation. BUT adding TTY should as simple as adding new processor on chain"*.
18. *"processBlock drains the bytes."*
19. Chain processor name: **"TerminalProcessor"**. *"NOT YAGNI: EVE is actual terminal emulator. with TextEditor as the renderer. you bypass the foundation, you will writing WRONG MENTAL MODEL."*
20. *"TerminalGraphics is SENDING BYTES. it's NOT actual RENDERING ENGINE"*.
21. *"TextEditor is message thread exclusive, that's the juce native rendering CONTRACT"*; *"TextEditor is message thread exclusive, it could be fed from anywhere"*.
22. *"what's wrong with reflow at message thread while the hot path keeps its heavy lock free traffic?"* — reflow stays on the message thread.
23. Byte ring name: **"TerminalFIFO?"** (confirmed in discussion).
24. *"what's wrong with viewport winsize to render? isnt rendering is DECOUPLED already?"* — one viewport screen per send; the ring is sized by the winsize.
25. *"why reparsing? FUCK NO. it carry the whole AST. meaning, we can actually move AnsiDocument to message thread, possibly owned by ViewEditor piped into TextEditor. why? because it is THE append only scrollback history as the true SSOT "buffer", only being cleared when terminal gets "clear". not even SIGWINCH should touch it"*.
26. *"AnsiDocument is parsing, reflow, METADATA ONLY. TextEditor materialise, Processor transport the round trip."*
27. *"why cant we make TerminalGraphics thread agnostic? for TUI it's message thread, juce::Component driven, while at EVE, an actual terminal it is A PROCESSOR."*
28. *"then this is our AnsiBuffer as oppose to AudioBuffer no? resize at prepareToPlay, Processor IS ALLOCATE"*.
29. **"TETRIS processor verbs ONLY"** — no `beginFrame` / `endFrame`.
30. *"there's no fucking audit findings open."*
31. Pull owner: **"TextEditor (Recommended)"** — TextEditor::bindToProcessor + Timer, owns the AnsiDocument; jam_gui adds jam_terminal.
32. *"non automatable parameters belong to Settings. READ THOROUGHLY how JFS set this up with its Panel widget, dont conflate global settings and instance non-automatable PARAMETER"*
33. *"should not be winsize parameter authored always by EVEView?"*
34. AnsiBuffer: **"Chain byte block"**.
35. *"use ReflowDocument for reflow"*
36. B3 scratch: **"All, fixed depth"** — the engine also owns the state stack and the transparency layers (rows at grid size), sized at `prepare` to a named maximum depth and asserted; a `RectangleList` copy in `saveState` still allocates.

Rulings 31–34 supersede the earlier Part B "Resolved" items 1 (ViewEditor owner) and 3 (packed Model composite parameter), and the earlier B2 reading of AnsiBuffer as the engine cell grid.

## Part B v3 — terminal round trip on the JUCE plugin contract (approved 2026-10-20, plan mode)

Part B v2 (below, kept as a record) was rejected after the Release measurement: the writer sat in EVEView on the message thread (`EVEView.cpp:106`), the byte write was tied to the paint scope (`jam_TerminalGraphicsContext.cpp:17`), each WINSIZE change re-sent a viewport-sized screen into the history (`EVEView.cpp:86-120`), and the source widget wrapped at `winsize.cols` (`EVEView.cpp:79`, `:110`). These were COUNSELOR's mistakes, not ARCHITECT's direction. v2 steps B1 (TerminalFIFO), B2 (AnsiBuffer), B3 (engine TETRIS verbs, scratch at prepare), B4 (TerminalProcessor), B8 (AnsiDocument append path) and B9 (TextEditor pull, stored-advance reflow) stay as built and are reshaped by v3; B5, B6, B7 and B10 are replaced.

Measured (Release, warm, from exec): window +345 ms, first terminal lines +690 ms; harness load 265 ms on the message thread (46 mermaid diagrams: parse and layout 18 ms, raster 122 ms; 1415 text lines 70 ms).

### Rulings for Part B v3 (ARCHITECT, verbatim)

Rulings 1–36 stand, except where a ruling below supersedes them.

37. *"EVEAudioProcessor should be renamed as EVETerminalProcessor but treated exactly like JFS AudioProcessor, IDENTICAL topology, identical ownership, treading etc"*
38. *"it is the lane of lock free HOT PATH, and cold path for TERMINAL CONCERN"*
39. *"winsize could be change RUNTIME exactly like setFactor, setFactor IS setWinsize, it's resizing buffer for processing"*; prepare is *"exactly like process spec for init"*.
40. *"writing bytes is processBlock"*
41. *"there is no try-lock. ALWAYS LOCK FREE"*; *"READ HOW JUCE DID IT. why the fuck we need to redesign atomic transport?"* (juce::AbstractFifo).
42. Harness role: **"Shell output, like cat"**. Harness: *"analogous to vim swap file… the SSOT complete AST, on disk"* — the original file. Read: **"mmap readOnly"**.
43. Live grid: **"Yes, engine grid is live"**. Width: **"Rows unbounded, rows count"**. Extent: **"Whole harness, one paint"** (one write).
44. *"wrapping and reflow is RENDERING concern. NOT Terminal Video job… it should just print to materialise not the layout engine… the widget, the Viewport: DECOUPLED"*
45. *"Paint is NOT MATERIALISATION. Paint is a call for whatever ALREADY MATERIALISED… materialisation can be done ANYWHERE. exactly like Terminal Graphics responsibility, to materialise bytes, then piped to be rendered"*
46. *"our Terminal LLGC is juce contract, can be used to paint any component into terminal context… we can use it as SSOT for our terminal round trip now. because we will WIRE PTY"*; *"Scrollback history decoupled from terminal live buffer ENTIRELY. all layers dumb. EVEProcessor and EVEView are the orchestration layers, always TELL"*
47. *"juce rasterizer is SLOW. we have FAST LANE with Vulkan, with its software fallback, why cant we use that as the SSOT?"*; extraction **"Yes, this sprint"**; core name **"GlyphRasterizer"**; *"Vulkan can stays with its message thread contract, while Terminal could be used in any context"*.
48. Loader: *"harness, is loading file always through View… the loader could be at Processor? View just tell the reference it's holding? isnt this just like how juce convolution loads IR?"*; thread **"In the tell, message thread"**.
49. *"NO POKING LAYERS. MUST BE ENCAPSULTED. BLESSED compliant. DESIGN BY CONTRACT"*; *"how is JFS end-to-end View-Process handle this?… what is juce audio plugin contract?"*; *"plugin_bootstrap with Style"*.
50. *"isnt scroll, erase, etc is STATE MACHINE? it belong to APVTS. the processor state. non automatable parameter. stateInfo"*; *"our widget scroll is actual scroll, a Viewport scroll, never conflate them"*; *"command is command… how the internal handles it are ISOMORPHIC"*.
51. *"our terminal parser now is AnsiDocument"*.
52. Storage: *"mmap now? swap later for session restoration? most of terminal state machine i think can be stored exactly as stateInfo blob… getState/setState info with END as host"*.
53. *"CONTRACT NON-NEGOTIABLE… 0 copy 0 mutations 0 temp containers 0 shadow state 0 handroll method 0 hack… MUST USE FRAMEWORK API to its fullest extent… always UNIDIRECTIONAL"*.
54. *"you need to udpate the RFC comprehensively with current understanding"*.
55. Build: *"always build with cast"*; *"you measure and optimize"*; *"NEVER hack by bumping the version… delete on disk, let it write"*.
56. Layout width (2026-10-21): *"my ruling stands, cast shuold conform. materialiser should not poke winsize at all. winsize should stopped at TerminalGraphics, downstream everything is juce::Component::getBounds() exactly juce native rendering contract"*.
57. Encapsulation (2026-10-21): *"ENCAPSULATION. terminal concern starts and ends with Terminal context, terminal thread. wrapping, reflow, is rendering concern"*.
58. Units (2026-10-21): *"for TUI juce::Component "logical pixel" always means CELL UNIT. juce::Rectangle is in cell. this is the logic that at least cast must be using. while at GUI juce::Component always LOGICAL PIXEL. Vulkan translates to physical, Terminal always in cell"*.
59. Enforcement (2026-10-21): *"if it's not designed by contract, not aligned to my rulings, then it's violations. if you have no factual citations to prove ARCHITECT is wrong, SHUT THE FUCK UP and FIX IT IN ADHERENCE AGAINST CONTRACT"*.
60. Clean-up (2026-10-21): *"we have jam::Value::map to convert anything without manual calculation"*; *"READ THOROUGHLY what actually the framework capable of. what is ready to use, what could be extracted, what need to be added as new SSOT as extension of existing modules, that when juce doesnt provide OOTB method. jam and juce may have overlaps, but our similar methods always been tested and measured, always FASTEST against juce. synthesize the CLEAN UP against CONTRACT"*.

Superseded: the answer **"Handed to source widget"** (AST owner) assumed that the View paints into the chain. Ruling 49 (JUCE contract: the View tells and pulls only, `TETRIS.md:79-99`) removes that path. The chain owns the AST.

### Facts the v3 design rests on (reads 2026-10-20)

- JFS chain: owns members and cold resources; `prepare (spec)` from `prepareToPlay` inside `suspendProcessing` (`FilterStripProcessor.cpp:103-116`, `FilterStripAudioProcessor.cpp:67-100`); runtime factor: pending atomic → applied on the audio thread → `setFactor` → `calc()` resizes buffers (`FilterStripAudioProcessor.cpp:103-128`, `jam_Oversampler.cpp:66-91`); the View pulls through `userInterfaceGetters` (`FilterStripAudioProcessorRegistration.cpp:363-375`, `kuassa_Analyzer.h:301-305`).
- Cold and hot state: cold state in ProcessorChain, allocated at prepare; hot state holds `const T*` (`TETRIS.md:230-285`).
- Lock-free crossing: `juce::AbstractFifo`, one reader and one writer, two `juce::Atomic<int>` (`juce_AbstractFifo.h:40-42`, `:319`; `juce_AbstractFifo.cpp:67-140`); slot storage indexed by its handles is JUCE's `Queue` shape (`juce_Convolution.cpp:38-80`). Convolution loads by reference from the caller (`:888-898`).
- Materialiser lane (TUI): `StyleManager` + `StyleMarkdown` + a component painted into `terminal::GraphicsContext` (cast `Help.cpp:10-35`); an off-screen component paints off any peer (`juce_Component.cpp:35-38`, `:2027-2083`). The style sources are the processor-owned `PluginEditorLayout` (`EVEProcessor.h:62`, `jam_PluginEditorLayout.h:79`, `:93`); `StyleManager` is a process-wide `jam::Instance` (`jam_StyleManager.h:22`).
- Fast lane: `VulkanImageType` → GPU offscreen LLGC on the message thread (`jam_VulkanImagePixelData.h:40`, `:61-83`, `:370-399`); gate `isGpuAvailable()` (`jam_VulkanEngine.h:281-286`); software fallback = `LowLevelGraphicsGlyphRenderer` (`jam_VulkanEngine.h:38-42`); pixel readback through `BitmapData` (`jam_VulkanImagePixelData.h:140-152`, `:245-291`).
- Shared raster state: `GlyphAtlas::getOrRasterize` changes `cache` and `images` with no lock (`jam_GlyphAtlas.cpp:143-153`, `:186-239`); `MonoCoverageWriter` is already shared by the atlas and the terminal context (`jam_GlyphAtlas.h:144`, `jam_GlyphAtlas.cpp:617-625`, `jam_TerminalGraphicsContext.cpp:365-366`).
- Parser: `AnsiDocument` — text, SGR, CUF, OSC 8, OSC 1337, newline (`jam_AnsiDocument.cpp:189-199`, `:620-630`); dispatch through `Function::Map` (`:572-583`, `:620-636`), the isomorph of the JFS `parameters` map (`FilterStripAudioProcessor.cpp:155-159`).
- Harness today: disk file through `__FILE__` (`EVEView.cpp:65`), copied by `loadFileAsString` (`jam_TextEditor.cpp:41`); no BinaryData row (`cast/files.md:29`).
- State blob: `getStateInformation` and `setStateInformation` (`EVEProcessor.cpp:144-157`); the Standalone saves on close (`jam_AudioStandaloneApp.cpp:70-74`, `:151`). A disk guarantee comes only from `FileOutputStream::flush` → `fsync` / `FlushFileBuffers` (`juce_FileOutputStream.cpp:82-86`, `juce_SharedCode_posix.h:541-545`); `MemoryMappedFile` has no flush (`juce_MemoryMappedFile.h:51-52`, `:101-109`).

### Topology v3 (JFS isomorph)

| Layer | JFS | EVE |
|---|---|---|
| Plugin processor | `FilterStripProcessor` | `EVEProcessor` |
| Chain | `FilterStripAudioProcessor` | `EVETerminalProcessor` (ruling 37) |
| Spec at prepare | `juce::dsp::ProcessSpec` | `ProcessSpec` + terminal capacity (harness rows + `winsize.rows`) |
| Runtime resize | `setOversamplingFactor` → pending → `setFactor` | `setWinsize` → pending packed atomic → live rows resized in `process` (ruling 39) |
| Writer | — | `terminal::GraphicsEngine`, chain member; `process()` in processBlock (rulings 40, 43) |
| Live buffer | — | engine row ring over `juce::AbstractFifo` (ruling 43) |
| Lanes to the View | `userInterfaceGetters` | history lane → `AnsiDocument` SSOT; live lane → live rows (ruling 46) |
| State machine | APVTS + instance PARAM nodes | modes, cursor, margins → processor state, stateInfo (ruling 50) |
| View | tells parameters, pulls getters | tells the harness `File` and `WINSIZE`, pulls both lanes (rulings 48, 49) |

Threads: message (the View tells; the tell maps, parses and materialises the harness; TextEditor pulls), audio (`process()` writes bytes and applies the pending winsize), ImageLoader pool (file decodes). No lock anywhere (ruling 41).

### Validation Gate (Part B v3)

COUNSELOR validates each step against MANIFESTO.md, CODING.md, NAMES.md and this plan by reading the changed files. Engineer does not build; COUNSELOR builds with cast (ruling 55). @Auditor runs once, after the last code step.

### Step V0: Rewrite the RFC (ruling 54)
**Scope:** `eve/RFC-EVE-terminal-grand-scheme.md`.
**Action:** one amendment block per stale section, each claim cited, each ruling verbatim: §0 isomorphism table (TerminalParser → AnsiDocument; CellFifo → TerminalFIFO / AbstractFifo; TerminalModel → processor state + stateInfo; reader-thread clock → processBlock clock; command dispatch ≙ parameter dispatch, rulings 50, 51); §3.1 / §3.6 pipeline (harness lane, chain, two lanes, threads); §3.7 resize (setWinsize ≙ setFactor; Viewport scroll ≠ terminal scroll); §5 inventory (deleted parts, AnsiDocument vocabulary today); new sections: the harness as the original file + mmap; the storage tier (swap later with `FileOutputStream::flush`, mmap limits); wrap and reflow = widget; GlyphRasterizer, one code for two lanes; the fast lane as raster SSOT; the PTY era (TTY reader → input AbstractFifo → processBlock decodes in order into the live grid with the non-allocating half of AnsiDocument; the tree build stays on the history lane, message side).
**Validation:** every new claim has a citation from this session; no training-data terminal model (RFC §0).

### Step V1: Delete the rejected v2 parts (Refactor-Rewrite: delete first)
**Scope:** `EVEView.h/.cpp` (`getOrCreateHarness`, `harnessStyle`, `harnessSource`, the engine in `setWinsize`), `jam_TerminalGraphicsContext.cpp:17` (destructor `engine.process()`), `EVEAudioProcessor.*` (renamed in V2).
**Validation:** the compiler lists what remains; nothing coexists with its replacement.

### Step V2: `EVETerminalProcessor` — the JFS chain shape
**Scope:** `EVEAudioProcessor.h/.cpp` → `EVETerminalProcessor.h/.cpp`; `EVEProcessor.h/.cpp`; `EVEView.cpp`; `project-info.md` source row.
**Action:** constructor `(juce::AudioProcessor&, jam::AudioModel&)` and the registration calls of `FilterStripAudioProcessor.cpp:16-42`; members `userInterfaceGetters`, `chainEvents`; verbs `prepare (const juce::dsp::ProcessSpec&)`, `reset`, `process`; `setWinsize (jam::TerminalWinsize)` stores a pending packed `std::atomic<int64_t>` (`TerminalWinsize::pack`, `jam_TerminalWinsize.h:9-11`), the `pendingOversamplingFactor` shape; `process` applies it at block start (the `applyPendingOversamplingFactor` shape). EVEProcessor member `terminalProcessor`, accessor `getTerminalProcessor()`; `prepareToPlay`, `setStateInformation` and the WINSIZE callback follow `FilterStripProcessor` (`:103-116`, `:211-220`) and `FilterStripView::setOversamplingFactor`.
**Validation:** ruling 37 line by line against the JFS files; no View access from the chain (TETRIS T).

### Step V3: `GlyphRasterizer` — one rasterising core, per-lane caches (ruling 47)
**Scope:** `jam_graphics/fonts/jam_GlyphRasterizer.h/.cpp` (new), `jam_GlyphAtlas.h/.cpp`, `jam_graphics.h`, `jam_TerminalGraphicsContext.cpp` (`rasterizePathCoverage`).
**Action:** move the backend configuration (backend, gamma and contrast coverage table, embolden, native font objects) and the rasterise functions (`rasterize`, `rasterizeEdgeTable`, `rasterizeByBackend`, `writeMonoGlyph`, `writeEdgeTableCoverage`) out of `GlyphAtlas` into `GlyphRasterizer`, which writes coverage into a caller-owned buffer through `MonoCoverageWriter`. `GlyphAtlas` keeps the cache, the packing and the frame counter, and owns one `GlyphRasterizer`. The terminal context's path coverage calls `GlyphRasterizer` in place of its own EdgeTable and writer pair. Paint can allocate (creation, MANIFESTO E); no `jam::EdgeTable`.
**Validation:** the owner API is read first (doxygen, header) before the brief; atlas output byte-identical (MarkdownComponent, cast `--help` bytes unchanged); one code for both lanes (S).

### Step V4: Fast-lane image materialisation (ruling 47)
**Scope:** `jam_gui/widgets/jam_TextEditor.cpp` (`getDiagramImage`, `getVectorImage`, `getImage`).
**Action [AMENDED 2026-10-20]:** ARCHITECT: *"do not change Vulkan CONTRACT. it must stays as is. we only abstract its software rasterizer being used as SSOT on TerminalGraphics"*. Diagram and vector rasters draw into a software `juce::Image` through `jam::LowLevelGraphicsGlyphRenderer<juce::LowLevelGraphicsSoftwareRenderer>` (`TextEditor::getFastLaneImage`), the software lane of the fast lane. No `VulkanImageType` enters the terminal lane. Layout takes the bounds from `MermaidGraphics` or the drawable. File images keep the queued decodes (`registerImages`). The PNG for the wire comes from `ImageLoader::getPng`.
**Superseded:** the first form of this step drew into `VulkanImageType` when `isGpuAvailable()`. A smoke run showed `JUCE_ASSERT_MESSAGE_THREAD` at `jam_VulkanImageRenderer.cpp:51` and `:63`, because `registerImages (juce::Array<juce::Image>)` reads the image back on a worker thread (`jam_ImageLoader.cpp:789-796`). ARCHITECT ruled the Vulkan contract out of this step.
**Validation:** no Vulkan type in `jam_TextEditor.cpp`; measure the harness load again.

### Step V5: Engine live row ring + byte write in processBlock (rulings 40, 43, 44)
**Scope:** `jam_TerminalGraphicsEngine.h/.cpp`, `jam_TerminalGraphicsContext.h/.cpp`, `jam_TerminalFIFO.*`, `jam_TerminalProcessor.*`.
**Action:** the grid becomes a row ring indexed by one `juce::AbstractFifo` (the `Queue` shape): cold resources sized at prepare (harness rows + `winsize.rows`), resized by the pending winsize in `process`. A row is one logical line of any length; `GraphicsContext` clips vertically only. Paint writes rows through the write handle and publishes them with `finishedWrite`; image placements carry their PNG, materialised at paint. `process()` (audio thread) writes the rows above the newest `winsize.rows` on the history lane (`TerminalFIFO`, `appendRowText` + newline), and the newest `winsize.rows` on the live lane (a second `TerminalFIFO`) when they change. No destructor calls `engine.process()`.
**Validation:** no allocation in `process` (TETRIS T, `TETRIS.md:72`); one writer and one reader per FIFO; no wrap in the engine (ruling 44).

### Step V6: The harness tell — Convolution shape, chain-owned materialiser (rulings 42, 46, 48, 49)
**Scope:** `EVETerminalProcessor.h/.cpp`, `EVEView.cpp`, `EVEViewPanelCallbacks.cpp`.
**Action:** EVEView holds the harness `File` and tells `getTerminalProcessor().load (harness)` on the message thread. Inside the tell: `juce::MemoryMappedFile` (`readOnly`) maps the file; `MarkdownDocument::parse` reads the mapped bytes in place; the chain sizes its cold resources (inside `suspendProcessing`, the prepare lane); the chain's materialiser (a `TextEditor` laid out without wrap, its `StyleMarkdown` from the plugin_bootstrap Style, owned by the chain as a cold resource, never a View child) paints once into `GraphicsContext` over the chain's own engine — the cast `printHelp` lane (`Help.cpp:10-35`). Images that are not ready publish when their job ends, through `juce::MessageManager::callAsync` (the `jam_Listener.cpp:65` worker shape); the row publish holds for them (ruling 15). EVEView never touches the engine.
**Validation:** ruling 49 (tell and pull only); the harness maps `readOnly` and the parse is creation (Audit 2 resolution 1); one load, no idle thread.

### Step V7: TextEditor pulls two lanes; history decoupled from the live rows (rulings 25, 44, 46)
**Scope:** `jam_TextEditor.h/.cpp`; the `EVETerminalProcessor` getter registration.
**Action:** the bound TextEditor owns the history `AnsiDocument` (append-only) and a live `AnsiDocument` replaced whole on each live frame (a state update). One getter pulls both lanes. Paint draws the history rows, then the live rows; `visibleAreaChanged` reflows both through `ReflowDocument::setRows` (ruling 35). A resize changes only the reflow (Viewport) and the chain's live row count (setWinsize); the history is never sent again.
**Validation:** no viewport coupling (ruling 44); paint reads only.

### Step V8: Build, measure, optimise (ruling 55)
COUNSELOR builds with cast (`cast cast/spell.md --debug`, `--no-sign` for Release) and measures with ephemeral `debug::Log` timing in Release (removed after). Targets: window, first history lines, harness load; resize with no stall.

### Step V9: Audit, doxygen, log
@Auditor once; resolve validated findings; doxygen zero warnings; log the sprint; `carol debt clear DEBT-20261016T214137`; commit messages in chat.

### New Names (Part B v3)
- Types: `EVETerminalProcessor` (ruling 37), `GlyphRasterizer` (ruling 47).
- Verbs: `load (const juce::File&)` (Convolution `loadImpulseResponse` / `TextEditor::load` family); `setWinsize` (the `setFactor` isomorph, `TerminalTTY::setWinsize` family); `getTerminalProcessor` (`getAudioProcessor` family).
- Members: `terminalProcessor` (EVEProcessor), `pendingWinsize` (`pendingOversamplingFactor` family).

### BLESSED Alignment (Part B v3)
- **B:** the chain owns the AST, the materialiser, the rings and the grid; the View owns only the bound TextEditor and its documents.
- **L:** existing families only; one rasteriser core.
- **E:** the View tells and pulls (TETRIS T); data moves one way; no layer reaches into another.
- **S (SSOT):** AnsiDocument is the history; the engine ring is the live buffer; GlyphRasterizer is the one raster code; the Style is the plugin_bootstrap Style.
- **S (Stateless):** paint reads materialised runs and images only; process writes bytes only.
- **D:** same harness, same bytes.

### Risks / Open Questions (Part B v3)
None.

## Part C — clean-up against CONTRACT (rulings 56–60)

Source: the arithmetic sweeps and the framework inventory of 2026-10-21. Framework first: `jam::Value::map` int→int (`jam_Value.h:141`), float→float (`:168`), float→int (`:209`); `jam::toInt` nearest (`jam_ToInt.h:35`) and ceiling (`:58`); `jam::Cell` `fromPixel` / `fromPixelCeiling` / `toPixel` (`jam_Cell.h:163`, `:173`, `:674`, `:694`, `:708`); `juce::Rectangle::operator/` (`juce_Rectangle.h:411`); `juce::RectanglePlacement`. jam before JUCE where both exist (ruling 60). X5 (a "paint into engine" lane) is withdrawn: `juce::Component` → `juce::Graphics` is the JUCE contract.

### Step C0 (done): TUI cell units
Engine `prepare (cols, rows)`; no vertical scale; no `setCellSize` (the style default is one unit = one cell); materialiser and cast bounds from the style column (`EVETerminalProcessor.cpp:71-77`, `Help.cpp:14-20`). Rulings 56, 58.

### Step C1: one span-advance sum; jam rounding; no double check
- `GlyphArrangement` gains `static float getAdvance (const Document::Numbers& advances, juce::Range<int> span)` (the `getAdvance` family, `jam_GlyphArrangement.h`); the file-static `getAdvance` of `jam_MarkdownGraphics.cpp:32` is deleted; the `std::accumulate` span sums at `jam_TextEditor.cpp:196`, `:341`, `jam_MarkdownComponent.cpp:211-213`, `jam_MarkdownGraphics.cpp` call it. Owner in jam_graphics because jam_gui does not depend on jam_markdown_graphics.
- `juce::roundToInt` → `jam::toInt` (`jam_GlyphRasterizer.cpp`, `jam_GlyphAtlas.cpp`); `static_cast<int> (std::ceil (x))` → `jam::toInt (x, true)` (`jam_GlyphRasterizer.cpp:347-348`).
- `EVEView.cpp:53-55`: the `if (display != nullptr)` after the `jassert` is deleted (MANIFESTO D).

### Step C2: one pixel↔cell conversion (X1)
`jam::Cell::Point` / `jam::Cell::Rectangle` conversions become the only pixel↔cell conversion, bodies through `jam::Value::map`, asserts in place of the silent `> 0 ? … : 0` guards (`jam_Cell.h:165-166`, `:677`). Every hand-written conversion (`jam_StyleMarkdown.cpp:59-102`, `EVEView.cpp:50`, `:57`, `jam_TextEditor.cpp:61`, `:242-243`, `:273`, `:291`, `jam_MarkdownComponent.cpp:313`) calls it with its own rounding kept.

### V5–V7 implementation (execution under the lock, reads 2026-10-21)
- Ring: `GraphicsEngine` rows are slots of one `juce::AbstractFifo` (the `juce::dsp::Queue` shape, `juce_Convolution.cpp:38-80`); one reserved slot (`TerminalFIFO::reservedSlot`, `jam_TerminalFIFO.h:24`).
- Publish: `GraphicsContext` holds the `juce::AbstractFifo::ScopedWrite` of its frame; its destruction calls `finishedWrite` (`juce_AbstractFifo.h:243-247`). No byte write in any destructor.
- Outputs at the call site: `process (history, live, liveRows)`; the stored `juce::OutputStream& output` is deleted (CODING: no stored non-owning reference).
- Image placements live per slot and carry their PNG, materialised at paint (`ImageLoader::getPng`); `process()` takes no lock (ruling 41, Auditor F15).
- Audio thread: rows above the newest `liveRows` are read (consumed) to the history lane; the remaining ready rows are peeked (`prepareToRead`, `juce_AbstractFifo.h:199`) to the live lane after ED 2 (`appendEraseScreen`, `jam_TerminalEncoder.cpp:236-240`), when rows moved to history or `liveRows` changed.
- Live frame boundary: AnsiDocument handles `map::CSI::eraseInDisplay` (`generated/jam_Terminal.h:970`) as a state update that clears the document (ruling 50).
- V6 image unit: the materialiser tells `setPixelCellSize` from its own Style's `getOrCreateMonoMetrics()` — the metrics EVEView turns into WINSIZE (`EVEView.cpp:49-50`); no winsize read in `load` (rulings 11, 56).
- Removed: the `input` → `block` hop and `AnsiBuffer` (Auditor F05).

### Step C3 (amended 2026-10-21): engine slot SSOT; TextEditor terminal-only
- X4 dropped: in the cell domain `getCellMeasure` defines the cell width — `StyleMarkdown::getLineMetrics` measures `cellWidth` as the advance of a space through the told arrangement (`jam_StyleMarkdown.cpp:270-272`); `span × cellWidth` would feed back into itself.
- X3 / R4 resolved by deletion: `TextEditor` is terminal-only (no `setDocument` caller; AnsiDocument creates only `Id::image` payload children, `jam_AnsiDocument.cpp:245`), so its markdown branches (`getFileImage`, `getRasterImage`, `getVectorImage`, `getDiagramImage`, the `MarkdownDocument::isObject` branches) are unreachable and go; the diagram raster stays only in `MarkdownComponent`.
- One frame-row → slot mapping in `GraphicsEngine` (the two copies at `jam_TerminalGraphicsEngine.cpp:150-151`, `:163-164`).

### Audit 2 (2026-10-21) — items for ARCHITECT (stop set: new names, PLAN vs code)
1. A04 — V6 "no copy of the file (mmap, read in place)" vs `Document::parse` copying the mapped bytes three times: `juce::String::createStringFromData`, `preprocess`, `source.append` (`jam_Document.cpp:621-624`, `:267`). In-place parse needs `Document` to hold a view it does not own.
2. A07 — V3 end state (rasterizer writes coverage into the atlas region) needs a new signature: the pack request from rasterizer to atlas; whether the EdgeTable mono lane keeps `juce::Graphics::fillPath` (byte-identical) or moves to `EdgeTable` + `MonoCoverageWriter` (byte risk); how the atlas learns a setting changed (`jam_GlyphAtlas.cpp:69`, `:84`).
3. A08 / A09 — V5 "GraphicsContext clips vertically only", ring sized "harness rows + winsize.rows"; V6 "materialiser … owned by the chain as a cold resource", images "publish when their job ends, through callAsync". Code: full-grid clip (`jam_TerminalGraphicsContext.cpp:13`), ring = harness rows, materialiser and style are `load` locals, decode synchronous.
4. A05 / A13 / A18 / A22 / A34 / A35 / A19 — new names: exact byte count for FIFO sizing; engine verbs to make `stateStack` … `mergedRuns` private; the column-width owner (`EVETerminalProcessor.cpp:61-63`, `Help.cpp:14-16`); one per-cell advance function (3 loops); AnsiDocument line-build vs colour-index split; one ReflowDocument cut/boundary function; a jam float→int floor SSOT.
5. A15 — `EVETerminalProcessor::load` calls `processor.suspendProcessing` (chain calls up into its owner).
6. A17 — `EVETerminalProcessor::spec { 44100.0, 1024, 2 }` duplicates `EVEProcessor::spec`.
7. A23 — `jam_SimdBlend.h` non-template `static inline` bodies in a header.
8. A24 — jam_gui depends on jam_markdown and jam_mermaid_diagram (ruling 7) but no jam_gui source uses them since C3.
9. A31 — every true colour set on the shared LookAndFeel per update (PLAN B9 shape).
10. A38 — `Cell::Rectangle` and `TerminalWinsize`: same 4-int layout and pack code.
11. A41 — unnamed-use `jam::GlyphArrangement` locals as the active font source (jam::Instance pattern).
12. A44 / A45 — `load` runs in the editor constructor; settings CSS → markdown → parse twice at launch (`EVEProcessor.cpp:16`, `EVEView.cpp:9`).

### Audit 2 — resolution (2026-10-21, re-read against CONTRACT; CONTRACT outranks PLAN, CAROL Instruction Hierarchy)
1. A04 — resolved, no code change. `Document::parse` is creation: `preprocess` rewrites CR and FF to LF (`jam_Document.cpp:289-297`), so the source cannot be the mapped bytes. MANIFESTO E: *"Creation can be complex. Creation can use multiple passes, look-ahead, reverse steps, work buffers, and memory allocation."* Ruling 42 holds: the harness maps `readOnly` (`EVETerminalProcessor.cpp:51`). V6 validation reads: the harness maps `readOnly`; the parse is creation.
2. A07 — open: V3 action (mono lane into the atlas region through `EdgeTable` + `MonoCoverageWriter`) against V3 validation (atlas output byte-identical; the mono lane fills with `juce::Graphics::fillPath`, `jam_GlyphRasterizer.cpp:369-380`). ARCHITECT.
3. A08 — resolved, no code change: ruling 56 (*"downstream everything is juce::Component::getBounds() exactly juce native rendering contract"*); the grid clip is the component bounds at the style column. Ring sized to the harness rows: a slot nothing writes is YAGNI (MANIFESTO L). A09 — resolved, no code change: a materialiser kept after `load` holds state with no use (MANIFESTO S, YAGNI); decode is synchronous, so every image is ready and the V6 condition *"Images that are not ready"* never applies.
4. A13 — done: the per-paint scratch is private to `GraphicsContext` (`jam_TerminalGraphicsContext.h:794-799`), the JUCE shape (`juce_RenderingHelpers.h:2703`, the state stack is a member of `StackBasedLowLevelGraphicsContext`); layers sized when opened (`GraphicsEngine::resize`, public with a caller). A18 — done: `StyleMarkdown::getColumnWidth()` (owner of `maxLineWidth` and `margin`; name from the local `columnWidth` at both sites). A19 — done where a framework API exists: `Rectangle::operator*` (`EVEView.cpp:54`, `jam_MarkdownComponent.cpp:319`), `jam::toInt (x, true)` (`setContentSize` ×2, `jam_SimdBlend.cpp:667`, `getColumnWidth`); open: a jam floor-to-int SSOT (`jam_Cell.cpp:60-63`, `:80-81`; `jam_TerminalGraphicsContext.cpp:25`). C1 — done: `GlyphArrangement::getAdvance (juce::Span<const double>, juce::Range<int>)`, callers pass the container (`juce_Span.h:121-123`), `getStringBounds` uses it. A22 per-cell loop — rejected: two identical loops (TextEditor, `getStringBounds`); MarkdownGraphics has the object branch (`jam_MarkdownGraphics.cpp:319-326`); under MANIFESTO S *"more than twice"*. A34 — done: indexed-colour functions in `jam_TerminalIndexedColours.h/.cpp` (sibling `jam_TerminalTrueColours`); `addLine` split into `addImages` and `addCoveredImages`. A35 — rejected: the two `getCutSegments` work in different units (source bytes with `getRows` by columns, `jam_ReflowDocument.cpp:712-730`; cell positions one unit per step, `:666-690`); `getSegments` and `build` are two sites. A05 — open (see Audit 2 questions).
5. A15 — rejected: the JFS chain calls `processor.suspendProcessing` itself (`FilterStripAudioProcessor.cpp:176`, `:205`, `:214`, `:232`); ruling 37 *"IDENTICAL topology"*.
6. A17 — rejected: the JFS chain holds `spec { 44100.0, 1024, 2 }` (`FilterStripAudioProcessor.h:350`) beside the plugin's `spec` (`FilterStripProcessor.h:189`).
7. A23 — done: `jam_SimdBlend.cpp` holds the 23 bodies; the header declares them.
8. A24 — done: jam_gui no longer depends on jam_markdown or jam_mermaid_diagram (ruling 7, *"PROVISION not kernel"*; CODING *"Compile time SHOULD NEVER slowed down by redundant headers inclusion"*).
9. A31 — rejected: PLAN B9 shape.
10. A38 — rejected: `TerminalWinsize` mirrors POSIX `struct winsize` (`jam_TerminalWinsize.h:5-7`), fields cols, rows, pixelWidth, pixelHeight; two pack bodies are under *"more than twice"*.
11. A41 — rejected: the `jam::Instance` pattern (`jam_Instance.h`); the locals set the active instance.
12. A44 / A45 — rejected: ruling 48 (*"In the tell, message thread"*); the JFS shape (commit 02278e4). Measured instead (V8).
13. V8 measurement (Release, warm, from exec): processor +69 ms, editor +257 ms, `load` +308 → +646 ms, first window paint +706 ms, history pulled +1059 ms before / +893 ms after. The gain: PNG write with `png_set_compression_level (Z_BEST_SPEED)` and `png_set_filter (PNG_FILTER_NONE)` (`jam_ImageLoader.cpp:629-630`): encode 220.2 → 62.2 ms, decode 113.3 → 88.6 ms for 51 images. Resize reflow: `TextEditor::resized()` (JUCE `TextEditor::resized` lays out, `juce_TextEditor.cpp:1992-1998`; `updateVisibleArea` calls `visibleAreaChanged` only on a changed visible area, `juce_Viewport.cpp:454-461`).

### Rulings 61–64 (ARCHITECT, 2026-10-21, verbatim) — supersede Audit 2 resolution items 2 (A07), 4 (A05, A19 floor)
61. Raster configuration: *"StyleTheme::setFontRasterization and setEmbolden << this is the smell. why lookAndFeel call rasterizer and embolden? rasterizer/embolden should be at LLGC only"*; *"the sole author should be Style since the SSOT is css-settings. so css carried by binary as default, write to disk as settings (exactly like other settings contract), settings always win, when a project did not "open" the parameter as settings, Style always return the default carried css. eve should have settings for rasterisation, embolden, etc it's the contract carried from old end (endless)"*.
62. Settings file: *"EVE using its own eve.md"*.
63. A05: *"history capacity same Settings contract: ViewLayout.md to eve.md, 10k row"*.
64. A19: jam floor-to-int, *"add it"*; *"what case do we need toInt floor? if it changes toInt signature just create overload. toInt without flag always nearest"*. Done: `toInt (value, std::float_round_style)` and `toInt (value, bool shouldRoundUp)` delegating to it (`jam_ToInt.h:57-72`); floor at `jam_Cell.cpp:60-63`, `:80-81`, `jam_TerminalGraphicsContext.cpp:27`.
65. Kernel (2026-10-21): *"both StyleTheme are identical. it's kernel module. both framework should be synced with cast --sync. be careful with the design. don't break JFS. IIUC JFS is the example of rasterisation/embolden default value carried by css, but never write to Settings. the contract Holds"*. 66. Layers (2026-10-21): *"that's the smell. GlyphAtlas IS owned by VulkanEngine. but StyleManager should be GlyphAtlas agnostic whenever Vulkan not included as deps"* — the smell: `StyleManager::getOrCreateTypeface` calls `GlyphAtlas::getInstance()->registerTypeface` (`jam_StyleManager.cpp:99-101`).
67. Layers (2026-10-21): *"LookAndFeel communicate to GlyphAtlas. Window is responsible for rendering engine, PluginEditor should not ever know about Vulkan"*; *"why this is widen?"*.

### Step R — raster configuration, Style sole author (rulings 61–67)
1. `StyleManager::getOrCreateTypeface` registers embedded faces in `jam::Typeface::getInstance()->registerTypeface` — the call `GlyphAtlas::registerTypeface` forwards to (`jam_GlyphAtlas.cpp:356-359`); `StyleManager` holds no atlas call (ruling 66).
2. Withdrawn: `jam_VulkanEngine.cpp:18` and `jam_VulkanEngine.h:731-737` stay unchanged — standing constraint *"do not change Vulkan CONTRACT"*; ARCHITECT: *"why the fuck do you think you need to change all Vulkan contract? why bring it up?"*.
3. `StyleTheme::setFontRasterization` / `setEmbolden` (the LookAndFeel tells the atlas, ruling 67): each key from the settings sheet (`Id::toTag`: `RASTERIZER`, `GAMMA`, `CONTRAST`, `EMBOLDEN`) when present — settings win; else the CSS `@font-face` attribute (JFS, ruling 65). Hard defaults `defaultFontGamma` / `defaultFontContrast` and the fallback branches deleted (`jam_StyleTheme.h:227-231`, `.cpp:25-71`).
4. EVE opens the four keys as settings: `Source/layout/ViewLayout.md` `## settings` rows `RASTERIZER freetype`, `GAMMA 2.2`, `CONTRAST 0.0`, `EMBOLDEN false` (EVE's runtime values today: `jam_VulkanEngine.cpp:18`, `jam_StyleTheme.cpp:53`, `:67-68`).
5. A07: backend, gamma, contrast, embolden, coverage LUT move into `GlyphRasterizer`; `GlyphAtlas::setRasterization` / `setEmbolden` tell the rasterizer and flush.
6. `cast --sync` jam → kuassa (ruling 65); JFS builds and renders unchanged.
7. A05 (ruling 63): `HISTORY_CAPACITY | int | 10000` in `ViewLayout.md` `## settings`; `Id::historyCapacity` (`cast/identifiers.md`); `TerminalProcessor::prepare (historyCapacity, liveCapacity)`; the live lane is sized by the winsize, the history lane by the winsize plus `HISTORY_CAPACITY` rows (`EVETerminalProcessor.cpp:20-31`).
8. KANJUT kernel boundary (ruling 7, *"KANJUT does not need TextEditor"*): `jam_gui.h` includes `jam_terminal` under `#if JUCE_MODULE_AVAILABLE_jam_terminal` (the `jam_core.h:73-75` shape); the TextEditor submodule rows are `provision`; KANJUT drops `kuassa_TextEditor.h/.cpp` and their rows; KANJUT manifest mirrors the jam graphics rows in jam order (`GlyphRasterizer` before `GlyphAtlas`).

#### Step R — state (2026-10-21)
- Done: 1, 3, 4, 5, 7, 8; 2 withdrawn; 6 at fixpoint (second `cast --sync` writes 0).
- Builds through cast, 0 errors, 0 warnings: EVE `--debug`, `--no-sign`; jfs `--debug`, `--no-sign`; kuassa `cast cast/spell.md`.
- Visual (Debug, fresh `eve.md` and `EVE.settings`, deleted on disk per ruling 55): f0 text crisp, layout correct; f03 lists, rules, links, logos; f45 logo image sequence; f6 mermaid radar, venn, quadrant, timeline, journey; f99 CAST text and code. The seeded `eve.md` carries `RASTERIZER`, `GAMMA`, `CONTRAST`, `EMBOLDEN`, `HISTORY_CAPACITY` (`eve.md:63-71`).
    Read (ruling 65): JFS first `@font-face` carries `--rasterizer: native; --gamma: 2.2; --contrast: 0.0; --embolden: false;` (`jreng-filter-strip/Source/layout/style.css:216-224`).

### Step C3 (original text, superseded above): shared layout statics (X3) and identical measure (X4)
Diagram raster and pixel bounds (three copies: `jam_TextEditor.cpp:273`, `:291`, `jam_MarkdownComponent.cpp:313`) to one owner; `getCellMeasure` returns `span × cellWidth`, the `getFontMeasure` arithmetic (`jam_GlyphArrangement.cpp:195`), after the read of the `cellWidth` that reaches it.

## Part B v2 — record (superseded by Part B v3)

### Dependency & API Inventory for Part B (reads 2026-10-20)

- **JUCE:** `juce::AbstractFifo` (`juce_AbstractFifo.h:92`, `:99-112`, `:154-204`, `read`/`write` scoped `:292`, `:314`); `juce::OutputStream` pure virtuals `flush`, `setPosition`, `getPosition`, `write` (`juce_OutputStream.h:66`, `:75`, `:81`, `:94-95`); non-seekable sibling `GZIPCompressorOutputStream::setPosition` → `jassertfalse; return false;` (`juce_GZIPCompressorOutputStream.cpp:163-167`); `juce::Base64::convertToBase64 (OutputStream&, const void*, size_t)` (`juce_Base64.h:50`).
- **JFS chain (sibling, `kuassa/jreng-filter-strip`):** `prepareToPlay` stores `spec`, `suspendProcessing (true)`, `audioProcessor.prepare (spec)`, `suspendProcessing (false)` (`FilterStripProcessor.cpp:103-116`); `process` clears outputs, then calls the chain (`:154-158`); a non-automatable instance value reaches the chain at creation (`:45-46`), at host restore (`:216-218`) and from the View (`FilterStripView.cpp:142-144` → `FilterStripProcessor.cpp:237-239`); getters `userInterfaceGetters[Id::analyzer].add<float*&, int&> (Id::lowPassFrequency, …)` (`FilterStripAudioProcessorRegistration.cpp:347-375`).
- **jam ring and processor siblings:** `SpectrumFIFO` (`jam_dsp/analyzer/jam_SpectrumFIFO.h:38-64` prepare, `:106-115` reset, `:151-173` `pull*`, `:177-213` push/pull); `SpectrumProcessor` verbs `prepare`/`process`/`reset`, getter `getSmoothedBins (destination, numBins)` (`jam_SpectrumProcessor.h:53-114`, `:157`).
- **View pull:** Registry calls `bindToProcessor` on the component itself (`jam_Registry.h:346-360`), from a non-empty `component.md` parameter cell (`jam_ViewManager.cpp:238-239`); the cell is `Format::toScreamingSnakeCase` (`jam_ViewManager.cpp:7`) = `Id::toTag` (`jam_Id.h:16-19`). Analyzer shape: `bindToProcessor`, `startTimerHz (jam::Component::refreshRateHz)`, `timerCallback` (`jam_Analyzer.h:55-73`, `:301-319`).
- **Instance non-automatable parameter (JFS Panel lane):** settings `settings` properties become instance PARAM nodes at processor creation (`jam_PluginEditorLayout.cpp:50-55`, `:558-565`); EVE's settings table is `ViewLayout.md:51-62`; the editor writes an instance node (`jam_PluginEditor.cpp:336`); a `value` change of a node with an `id` queues its id and runs the registry callback of that key on the message thread (`jam_PluginEditor.cpp:316-324`, `:347-356`); EVE registers callbacks in `attachPanelCallbacks` (`EVEViewPanelCallbacks.cpp:3-10`); `AudioModel::getValue (parameterId, propertyId)` (`jam_AudioModel.cpp:79-83`). Settings value types: `int`, `float`, `em`, `bool`, `string`, `colour`, `numbers` (`jam_ConfigDocument.cpp:12-55`).
- **Winsize:** `TerminalWinsize { cols, rows, pixelWidth, pixelHeight }`, `pack`/`unpack` int64; *"Display is the sole author"* (`jam_TerminalWinsize.h:9`, `:34-57`).
- **Terminal writer:** `GraphicsEngine` (`jam_TerminalGraphicsEngine.h/.cpp`): `resize` (`.cpp:57-75`), `beginFrame` (`:142-146`), `appendFrameRow` into a `std::string` (`:158-187`), `endFrame` builds one `std::string frame` and writes it (`:189-205`); encoder free functions append to `std::string&` (`jam_TerminalEncoder.h:31-197`). `GraphicsContext` ctor/dtor call `beginFrame`/`endFrame` (`jam_TerminalGraphicsContext.cpp:7-17`); per-call scratch: `stateStack` (`:73`), path coverage (`:352-353`), runs (`:366`), cell coverage (`:430`), image-alpha coverage (`:462`), transparency layers (`:525-529`).
- **AnsiDocument:** `parse (const char*, int)` builds a fresh document (`jam_AnsiDocument.cpp:10-14`); `Pen` is local to `build()` (`:188`); `addLine` sets one segment over all cells (`:216`); `addCoveredImages` walks every line (`:234-250`).
- **Reflow (ruling 35):** `ReflowDocument::getSegments (const AttributedString&)` (`jam_ReflowDocument.h:60`), `ReflowDocument::setRows (line, width, cutWidth, measure)` (`:102`). Sibling lane: advances and heights stored once per line (`MarkdownGraphics::setAdvances`, `jam_MarkdownGraphics.cpp:308-338`; properties added at line creation, `jam_MarkdownDocument.cpp:1022-1023`); rows from stored advances (`jam_MarkdownComponent.cpp:191-217`), row heights (`:243-267`).
- **TextEditor today:** owns `std::unique_ptr<Document>` (`jam_TextEditor.h:115`); `visibleAreaChanged` only sets the content size (`jam_TextEditor.cpp:59-63`); `getAdvances` builds a local array per layout (`:220-243`).
- **Layers:** jam_terminal depends on core, data_structures, gui_basics, jam_core, jam_graphics, jam_data_structures, jam_style (`jam_terminal.h:10-16`) — no jam_gui, so jam_gui → jam_terminal has no cycle.
- **Module TUs:** jam lists no `.cpp` rows in markdown; `jam_terminal.cpp` includes each sub-TU.

### Topology (JFS mirror)

| Layer | JFS | EVE |
|---|---|---|
| Plugin processor | `FilterStripProcessor` (`:103-116`, `:154-158`) | `EVEProcessor` |
| ProcessorChain | `FilterStripAudioProcessor` | `EVEAudioProcessor` |
| Chain member | `SpectrumProcessor` | `TerminalProcessor` |
| Cross-thread ring | `SpectrumFIFO` | `TerminalFIFO` |
| Block | `juce::AudioBuffer` | `AnsiBuffer` (ruling 34) |
| View pull | `Analyzer::bindToProcessor` + timer | `TextEditor::bindToProcessor` + timer (ruling 31) |
| Non-automatable instance value | `OVERSAMPLING` node, panel widget, View → processor | `WINSIZE` node, authored by EVEView (rulings 32, 33) |
| Writer | — | `terminal::GraphicsEngine`, thread agnostic (ruling 27) |

Threads (no TTY this sprint):
1. Message thread — the source TextEditor paints one viewport screen into `GraphicsContext`; `GraphicsEngine::process` writes the rows to the chain's input `TerminalFIFO`.
2. Audio thread — `processBlock` is the clock (ruling 18). The chain pulls the input `TerminalFIFO` into the `AnsiBuffer`; `TerminalProcessor::process` writes the block to its output `TerminalFIFO`.
3. Message thread — TextEditor's timer pulls the output `TerminalFIFO` into its AnsiDocument (the SSOT, ruling 25), then reflows and materialises the new lines (ruling 26).

A TTY joins later as a processor in `EVEAudioProcessor::process` in place of the input `TerminalFIFO` drain; TerminalGraphics joins the chain when a VT source writes the bytes (ruling 27).

### Validation Gate (Part B)

COUNSELOR validates each step before the next, by reading the changed files, against MANIFESTO.md, CODING.md, NAMES.md and this plan. Engineer does not build. @Auditor runs once, after the last code step.

### Step B1: `TerminalFIFO` (jam_terminal)
**Scope:** new `jam_terminal/processor/jam_TerminalFIFO.h/.cpp`; `jam_terminal.h` include; `jam_terminal.cpp` TU include.
**Action:**
- `class TerminalFIFO : public juce::OutputStream`, one type for both rings (CODING: no near-duplicate structs). Members: `juce::AbstractFifo` and a `juce::HeapBlock<char>` byte store (the `SpectrumFIFO` shape).
- `prepare (int capacity)`: allocates the store and sets the fifo size (`setTotalSize`, `juce_AbstractFifo.h:112`; message thread, inside `suspendProcessing`).
- `write (const void*, size_t)`: one `write` scoped handle, copy, commit (`:314`); returns whether every byte fit.
- `flush()`: no buffer, empty body. `setPosition`: the GZIP sibling (`jassertfalse; return false;`). `getPosition`: no seek position exists; `jassertfalse`, result `0`.
- `reset()`: `fifo.reset()`.
- `pull (AnsiBuffer& destination)` and `pull (AnsiDocument& destination)` — the `SpectrumFIFO::pull*` family (`jam_SpectrumFIFO.h:151-173`). The first copies the ready bytes into the block. The second calls `destination.addLines` with each of the two ready ranges in place (no staging buffer).
**Validation:** framework API only; one copy per thread hop; MANIFESTO E (no temporary container); one object one TU; no allocation outside `prepare`.

### Step B2: `AnsiBuffer` (jam_terminal)
**Scope:** new `jam_terminal/processor/jam_AnsiBuffer.h/.cpp`; module includes.
**Action:** the chain byte block, the `juce::AudioBuffer` analog (ruling 34). `setSize (int capacity)` allocates once, at `prepare` (ruling 28). `getWritePointer()`, `getReadPointer()`, `getNumBytes()` (the `AudioBuffer` reader names), and `setNumBytes (int)` sets the bytes that the current block holds, never above the capacity (assert).
**Validation:** allocation only in `setSize`; the `AudioBuffer` names; no shadow state (one count, one store).

### Step B3: GraphicsEngine on TETRIS verbs (jam_terminal)
**Scope:** `jam_TerminalGraphicsEngine.h/.cpp`, `jam_TerminalGraphicsContext.h/.cpp`; callers `cast/Source/Help.cpp:31`, `eve/Source/EVEViewComponents.cpp:61`.
**Action:**
- `prepare (int cols, int rows, float scale)` replaces `resize (cols, rows, scale)`. It sizes the cell grid `back` and allocates the draw scratch at the grid size: path coverage, merged runs, cell coverage, image-alpha coverage, transparency layers, state stack, one row of wire bytes. The scratch moves from `GraphicsContext` to the engine; the context writes into it only.
- `process()` writes each row to `output` as it encodes it, through the one row scratch (`appendFrameRow` without `std::string frame`). An inline image goes to `output` through `juce::Base64::convertToBase64 (output, …)` (`juce_Base64.h:50`), not through the row string.
- `reset()` clears the grid (today `beginFrame`). `beginFrame` / `endFrame` are deleted (ruling 29). The `GraphicsContext` ctor calls `reset`, the dtor calls `process`.
- `getNumBytes (jam::TerminalWinsize)` (the `getNumBytes` family, `jam_AnsiDocument.h`): the bytes of one screen. Text: `rows × (cols × utf8MaxBytes + sgrReserveBytes)` (today's `endFrame` reserve, `.cpp:192-194`, moved here as the SSOT). Images: the Base64 size of the PNG worst case of the pixel area (4 bytes per pixel, one filter byte per row, the zlib stored-block overhead), each as a named constant.
**Validation:** ruling 29; TETRIS.md:72, :246 (allocation only in `prepare`); thread agnostic (no `MessageManager`, `Desktop`, `LookAndFeel`); cast `--help` bytes unchanged.

### Step B4: `TerminalProcessor` (jam_terminal)
**Scope:** new `jam_terminal/processor/jam_TerminalProcessor.h/.cpp`; module includes after `TerminalFIFO`.
**Action:** the chain member (ruling 19), `SpectrumProcessor` verbs:
- `prepare (int capacity)`: prepares its output `TerminalFIFO fifo`.
- `process (const AnsiBuffer& block) noexcept`: writes the block bytes into `fifo`. Audio thread.
- `reset()`: `fifo.reset()`.
- `pull (AnsiDocument& destination)`: the View getter, `fifo.pull (destination)` (the `getSmoothedBins (destination, …)` shape). Message thread.
**Validation:** TETRIS T (no View, no allocation in `process`); `SpectrumProcessor` family names.

### Step B5: `EVEAudioProcessor` is the chain
**Scope:** `EVEAudioProcessor.h`, new `EVEAudioProcessor.cpp`; `project-info.md` source row if the manifest lists sources.
**Action:** the `FilterStripAudioProcessor` shape:
- Members: `jam::TerminalFIFO input` (public, as `userInterfaceGetters`, `EVEAudioProcessor.h:16`; the sender's `juce::OutputStream&`), `jam::AnsiBuffer block`, `jam::TerminalProcessor terminalProcessor`, `jam::TerminalWinsize winsize` (calculation input synced from the Model, MANIFESTO S).
- Constructor calls `registerUserInterfaceGetters()`: `userInterfaceGetters[Id::textEditor].add<jam::AnsiDocument&> (Id::toTag (Id::winsize), [this] (jam::AnsiDocument& destination) { terminalProcessor.pull (destination); })`.
- `prepare (const juce::dsp::ProcessSpec&)`: `capacity = jam::terminal::GraphicsEngine::getNumBytes (winsize)`; `input.prepare`, `block.setSize`, `terminalProcessor.prepare`.
- `setWinsize (jam::TerminalWinsize)`: stores it and prepares the rings and the block again (the `setOversamplingFactor` analog; message thread, inside the caller's `suspendProcessing`).
- `reset()`; `process()`: `input.pull (block)`, `terminalProcessor.process (block)` (TETRIS R: block handling only here).
**Validation:** JFS shape; TETRIS T; no allocation in `process`.

### Step B6: `EVEProcessor` drives the chain
**Scope:** `EVEProcessor.h/.cpp`.
**Action:** `juce::dsp::ProcessSpec spec` member. `prepareToPlay` stores `spec` and calls `audioProcessor.prepare (spec)` inside `suspendProcessing (true/false)` (`FilterStripProcessor.cpp:103-116`). `process` calls `audioProcessor.process()` after the output clear (`:154-158`). `setWinsize (jam::TerminalWinsize)` calls `audioProcessor.setWinsize` inside `suspendProcessing` (`:237-239`). The constructor (after `populateTree`) and `setStateInformation` (after `model.setState`) tell the chain the model's winsize (`:45-46`, `:216-218`): `jam::TerminalWinsize::unpack (model.getValue (Id::toTag (Id::winsize), Id::value) …)`.
**Validation:** JFS shape; one source for the winsize (the instance node).

### Step B7: `WINSIZE` instance parameter, authored by EVEView (rulings 32, 33)
**Scope:** `eve/cast/identifiers.md`; `eve/Source/layout/ViewLayout.md` `## settings`; `EVEView.h/.cpp`; `EVEViewPanelCallbacks.cpp`.
**Action:**
- `identifiers.md` row `winsize` (`@id`, value `winsize`).
- `ViewLayout.md` `## settings` row `WINSIZE | int | 0 | Terminal size of the instance, packed jam::TerminalWinsize; EVEView writes it.` — populated into the instance node at processor creation (`jam_PluginEditorLayout.cpp:50-55`). Version unchanged (ruling 15).
- `EVEView::resized() override`: `PluginEditor::resized()`, then writes the instance node: `model.getValue (Id::toTag (Id::winsize), Id::value).setValue (winsize.pack())`. `cols`, `rows` from the TextEditor area `jam::ViewManager::getComponentBounds (*view)` (`jam_ViewManager.cpp:23-26`; `component.md:3`) and the mono metrics of the terminal style (`StyleCustom::getOrCreateMonoMetrics`, `jam_StyleCustom.h:68`); `pixelWidth`, `pixelHeight` from that area at the display scale. EVEView never writes the global settings file.
- `attachPanelCallbacks` adds `Id::toType (Id::winsize)` → `EVEView::setWinsize()` → `static_cast<EVEProcessor&> (*getAudioProcessor()).setWinsize (…)` (`FilterStripView.cpp:142-144` shape).
**Validation:** ruling 32 (instance node, not global settings); ruling 33 (EVEView the sole author, `jam_TerminalWinsize.h:9`); the Panel-lane pattern; no new channel.

### Step B8: AnsiDocument append path (jam_terminal)
**Scope:** `jam_AnsiDocument.h/.cpp`.
**Action:**
- `addLines (const char* data, int size)` (the `MarkdownDocument::addLines` family, NAMES verb `add`): appends the bytes to the document source, tokenizes only the new bytes and builds lines onto the existing tree. `Pen` becomes a member that carries across appends, so a sequence split over two pulls parses as one. `parse (data, size)` is an empty document plus `addLines` (*"Document always parse"*, ruling 4).
- Each new line gets `Id::tokens` from `ReflowDocument::getSegments (cells)` (ruling 35; today one segment, `:216`) and empty `Id::advances`, `Id::heights` (the `jam_MarkdownDocument.cpp:1022-1023` shape).
- `addCoveredImages` walks only the new lines.
- Lines are never removed except by `clear` (ED 2 / ED 3, SPEC.md:39-41, :85-86); resize never touches the tree (ruling 25). No removal path is added this sprint.
- The owner API read precedes the brief: `jam::Document` parse, source and token APIs (doxygen first).
**Validation:** rulings 25, 26, 35; no re-parse; the Model grows by state update only.

### Step B9: TextEditor pull and reflow over stored advances (jam_gui)
**Scope:** `jam_TextEditor.h/.cpp`; `jam_gui.h` dependency `jam_terminal`; `user-modules-info.md` jam_gui row.
**Action:**
- `TextEditor` gains `private juce::Timer`. `bindToProcessor (getters, type, parameterID)` (Analyzer shape, `jam_Analyzer.h:55-73`): the document becomes an `AnsiDocument`; the source callable calls `getters.at (type).get (parameterID, document)`; `startTimerHz (jam::Component::refreshRateHz)`.
- `timerCallback`: pulls; when lines arrived, sets the document's true colours on the LookAndFeel (`getTrueColours`, the current `EVEViewComponents.cpp:74-75` shape), lays out the new lines only, updates the index, sets the content size and repaints.
- Reflow lane (ruling 35, the `MarkdownComponent` lane): per line, advances and heights are stored once in `Id::advances`, `Id::heights` (`MarkdownGraphics::setAdvances` shape). A width change in `visibleAreaChanged` reflows every line with `ReflowDocument::setRows (line, width, width, measure)` over the stored advances, then row heights and runs. `getAdvances` and its local array are deleted.
- `load` and `setDocument` stay (the source TextEditor and cast-style use).
**Validation:** pull only (TETRIS.md:99); message thread (ruling 21); no re-measure on resize; one reflow lane with MarkdownComponent (S); paint reads only (ruling 6).

### Step B10: EVE sender and component row
**Scope:** `EVEViewComponents.cpp:25-80`; `eve/Source/layout/component.md:8`.
**Action:**
- `component.md` textEditor row parameter cell `winsize` (keys the getter, `jam_ViewManager.cpp:7`, `:238-239`).
- The TextEditor config keeps the source TextEditor, the `StyleMarkdown` tells and the `GraphicsContext` paint. It paints one viewport screen (the model's winsize rows, ruling 24) into an engine whose output is the chain's `input` (`EVEProcessor::getAudioProcessor()`, `EVEProcessor.h:49`), prepared with `GraphicsEngine::prepare`. `AnsiDocument::parse`, the colour loop and `setDocument` leave the config.
**Validation:** ruling 24; the destination is fed only through the chain.

### Step B11: Remove the ephemeral `launch` log lines
**Scope:** `jam_AudioStandaloneApp.cpp:140`; `EVEViewComponents.cpp:30, 58, 67, 69, 72, 78`; `jam_TextEditor.cpp:15, 27, 30, 38, 48, 50`; `jam_TerminalGraphicsEngine.cpp:201, 214` (lines that later steps did not already remove).
**Validation:** `grep "\"launch\""` over jam and eve gives nothing.

### Step 12: Build and measure (ARCHITECT)
ARCHITECT builds (`cast cast/spell.md --debug`, and cast for jam). Fact for the run: the disk `~/.config/end/eve.md` (version 0.0.2) has no `WINSIZE` row; the disk file wins when its version is not older (`jam_PluginEditorLayout.cpp:27-44`), and the version stays (ruling 15). `trace.lua` / `sample.lua` specs on EVE Release.
**Validation:** window before the chain; launch in subseconds; resize reflows without stall.

### Step 13: Audit, doxygen, log
@Auditor once over the sprint's work (ruling 30); resolve findings; doxygen pass (zero warnings; the stale blocks above `createPngReadState` and `readPngRowsToImage` in `jam_ImageLoader.cpp`); log the sprint; `carol debt clear DEBT-20261016T214137`.

## BLESSED Alignment
- **B:** the chain owns its rings and block; TextEditor owns its AnsiDocument; EVEView owns the winsize write; ImageLoader owns decodes.
- **L:** existing families only: `SpectrumFIFO`/`SpectrumProcessor` → `TerminalFIFO`/`TerminalProcessor`; `AudioBuffer` → `AnsiBuffer`; TETRIS verbs; `pull`, `addLines`, `getNumBytes`, `setWinsize`.
- **E:** layers point down (jam_gui → jam_terminal, no cycle); the writer writes to a told stream; the View pulls; the Model → View → processor path for the winsize is the JFS Panel lane.
- **S (SSOT):** AnsiDocument is the one scrollback; one ring type; one reflow lane (ReflowDocument); one winsize truth (the instance node); one screen-size formula (`getNumBytes`).
- **S (Stateless):** paint reads only; the hot path writes into buffers allocated in `prepare`; the chain's winsize is a calculation input.
- **D:** same harness, same bytes per launch.

## New Names
- Part A: `TextEditor::load`; `PluginEditorLayout::getSettingsTree`; `StyleCustom::getLineFonts`, `getCellBounds`, `getPixelBounds`; `GraphicsEngine` member `output`, parameter `outputToUse`; `StyleManager` parameter `styleSheets`.
- Part B types: `TerminalFIFO` (ruling 23), `TerminalProcessor` (ruling 19), `AnsiBuffer` (rulings 28, 34).
- Part B verbs: `prepare` / `process` / `reset` (ruling 29); `pull` (SpectrumFIFO family); `addLines` (MarkdownDocument family); `getNumBytes` (AnsiDocument family); `setSize`, `getWritePointer`, `getReadPointer`, `getNumBytes`, `setNumBytes` (AudioBuffer family); `setWinsize` (TerminalTTY::setWinsize, `jam_TerminalWinsize.h:14`).
- Part B members: `input`, `block`, `terminalProcessor`, `winsize` (EVEAudioProcessor); `fifo` (TerminalProcessor, SpectrumProcessor family); `spec` (EVEProcessor, JFS family).
- Part B identifier: `winsize` / `WINSIZE` (TerminalWinsize family).

## Risks / Open Questions
None.
