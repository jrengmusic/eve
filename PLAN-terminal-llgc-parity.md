# PLAN: Terminal LLGC Full Feature Parity + One-Shot EVE Viewer

**RFC:** RFC-EVE-terminal-grand-scheme.md (§5.3.3 gap 3/4, §6 jaw order amended by ARCHITECT this session)
**Date:** 2026-09-13
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE / JAM — LANGUAGE.md C++ rules (header-only preferred ~300 LOC; split only for real dependency cost; 30/3 unchanged)

## Context

Sprint objective (ARCHITECT): implement Terminal LowLevelGraphicsContext full
feature parity for native `juce::Component`/`LookAndFeel` rendering, including
images/pixels — and prove it in a running one-shot EVE viewer with both
backends (Terminal LLGC + Vulkan LLGC) behind the same `juce::Graphics` seam.

Session rulings (all locked):
1. **Scope:** LLGC parity + one-shot EVE in one sprint. No PTY, no shell, no live lane.
2. **Text is always string bytes.** A sixth JUCE patch adds an attributed-text
   virtual to `juce::LowLevelGraphicsContext`; the default implementation falls
   through to today's glyph shaping (pixel backends unchanged); the terminal
   override emits characters. `drawGlyphs` stays asserting terminal-side
   (Amendment 16/19 upheld). Kitty protocol is dead — no negotiation.
3. **Images: both lanes.** OSC 1337 (iTerm2 — ruled most deterministic: bytes =
   base64 of the PNG, unidirectional) via a new SKiT encoder, plus half-block
   `▀` truecolor fallback. SKiT decode half already exists; this sprint builds
   the encode half.
4. **The cell law (geometry policy):** the character owns the cell; geometry
   contributes colour. Sub-cell detail (curves, corners, image edges)
   rasterizes to half-blocks; rectangular interiors are bg runs; text over
   raster inherits bg (`compositeOntoBackground` precedent,
   jam_TerminalGraphicsContext.h:394-419); clips are cell-granular — a glyph
   is atomic, in or out by coverage; transparency/masks are per-cell colour
   blends.
5. **Validation:** headless harness (byte-fixpoint + juce software-renderer
   oracle + SKiT round-trip decode + `cat` eyeball) plus the running EVE
   standalone viewer as jaw 1 of the validation ladder.

## Dependency & API Inventory (4 Pathfinders, COUNSELOR-synthesized)

**Terminal write lane (jam_terminal):**
- `GraphicsContext` (jam_TerminalGraphicsContext.h): proven — clip
  rects/exclude (:42-128), state stack (:145-167), fillRect family (:173-201),
  `drawCells` (:213-245), h/v `drawLine` box-drawing (:253-328),
  `quantizeToCells` per-edge rounding (:384-392), `compositeOntoBackground`
  (:400-420), `fillCells` (:425-447). Nine `jassertfalse` placeholders
  (:330-370): clipToPath :332, clipToImageAlpha :333, clipRegionIntersects
  :335, transparency layers :341-342, fillPath :344, drawImage :345,
  drawGlyphs :354 (stays asserting), getPreferredImageTypeForTemporaryImages
  :360, getFrameId :366.
- `GraphicsEngine` (jam_TerminalGraphicsEngine.h): `front`/`back` grids
  (:293-294), widthCols/heightRows (:295-296), `FILE* nativeHandle` (:297),
  resize (:244), beginFrame (:259), endFrame (:269-289); emission statics:
  appendDecimal :13, appendUtf8 :39, appendSgrFlags :49 (SSOT
  `Stamp::sgrFlagCodes`), appendSgrColor :80, getSgrForCell :111,
  appendHyperlink :133, getRowText :199.
- Atoms: `AttributedChar` (jam_AttributedChar.h — make :198, erase :217,
  withHyperlinkId :233, fromCodepoint :260, width :272,
  graphemeSegmentationStep/Init :311-325; widthClass narrow/wide/
  spacerTail/proportional :85-95). `Stamp` (jam_Stamp.h — Entry with role
  fields :84-157, flags :161-190, sgrFlagCodes :214-225, underlineStyleCodes
  :232-240, directColor :207).
- Module manifest: jam_terminal.h include order — graphics :115-116, document
  :121-126, widgets :131, image :162-170. Submodules carry zero includes.

**SKiT / image lane (jam_terminal/image):**
- `TerminalITerm2Decoder::decode` (jam_TerminalITerm2Decoder.h:61; .cpp
  :15-125): requires `File=` prefix (:25), key=value pairs to `:` delimiter
  (:34-74), `inline=1` mandatory (:61, :79), payload =
  `juce::Base64::convertFromBase64` (:86-94), image decode native or JUCE
  ImageFileFormat fallback (:101-114). **This parse defines the exact wire
  format the encoder must produce.**
- `TerminalSkit` (jam_TerminalSkit.h:55-169): decode orchestrator; `END;` /
  `GEND;` filepath preview protocol (.cpp :59-91, :142-170, :225-252). Encode
  half does not exist (grep: no encoder in image/).
- `TerminalDecodedImage` (:29-40 — rgba HeapBlock, width, height).

**JUCE text path + patch lane:**
- Chain: every `Graphics::drawText`/`drawFittedText`/`drawSingleLineText`/
  `drawMultiLineText` overload (juce_GraphicsContext.h:139-250) builds a
  cached `GlyphArrangement` (juce_GraphicsContext.cpp:356-577) →
  `GlyphArrangement::draw` loops font batches → `ctx.setFont` +
  `ctx.drawGlyphs` (juce_GlyphArrangement.cpp:711-745) via
  `g.getInternalContext()` (juce_GraphicsContext.h:747).
- `juce::AttributedString`: Attribute{range,font,colour}
  (juce_AttributedString.h:160-185), append family (:80-92), getAttribute
  (:193), `draw (Graphics&, Rectangle<float>)` (:105).
- Patch machinery (eve CMakeLists.txt): patch list :100-106, concat+SHA256
  stamp :119-128, stamp file :117, `git apply --check` then apply :136-175.
  Template patch format: jam/patch/juce-vulkan-engine-hook.patch — unified
  diff, early-exit hook (`if (externalContextFactory != nullptr) … return;`).
- **Harness catch:** tests/ansi_fixpoint/CMakeLists.txt:16 adds the PRISTINE
  JUCE tree (`add_subdirectory(JUCE)`, no patch machinery). The sixth patch
  makes the terminal override compile-dependent on the patched virtual — the
  harness must consume the patched tree.

**Vulkan reference + glyph lane:**
- Vulkan LLGC: one concern per TU — jam_VulkanLowLevelGraphicsContext.h (state,
  rect fills, 888 lines) + Glyph/Path/Image/Transparency/Render .cpp files
  (fillPath via earcut/winding-cover, drawImage via UV quads,
  begin/endTransparencyLayer via offscreen passes). Structural reference for
  the terminal split.
- Cell lane already half-built: `jam::GlyphArrangement`
  (jam_vulkan/font/jam_GlyphArrangement.cpp) consumes `AttributedChar`
  (getCodepoint/shapeCell/findGlyph on contentTag/widthClass) and threads
  `setCellRun(...)` → `drawGlyphs` into `GlyphAtlas::Key` for cell-fit.
  `BoxDrawing` rasterizes U+2580-U+259F procedurally
  (jam_BoxDrawing.h:162-165, :624-724) — half-block cells render pixel-side
  too.
- Pixel I/O: `juce::Image::BitmapData` readOnly precedents
  (jam_VulkanLowLevelGraphicsContextGlyph.cpp:29 et al.);
  `juce::PNGImageFormat::writeImageToStream`
  (juce_ImageFileFormat.h:104) — no jam PNG-encode precedent, JUCE API is the
  framework surface.
- Oracle: `juce::LowLevelGraphicsSoftwareRenderer (const Image&)`
  (juce_LowLevelGraphicsSoftwareRenderer.h; .cpp :52).

**EVE wiring:**
- project-info.md `## user module` table :246-264 (12 jam rows; jam_terminal
  absent). CMakeLists consumes via `juce_add_module` :193-205 — regenerated by
  cast from the table.
- `EVEView : jam::PluginEditor` (EVEView.h:5), hooks initialiseTheme/Registry/
  Panels/View/attachPanelCallbacks/Listeners (:14-19); `initialiseView()`
  creates the ViewEditor child (EVEView.cpp:32-39). Standalone shell:
  `jam::AudioStandaloneApp` (jam_AudioStandaloneApp.h:20-79), active via
  JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1 (project-info.md :295).
- Vulkan factory injection already owned by `VulkanEngine` ctor/dtor
  (jam_VulkanEngine.h:81, :108) — live for the editor's lifetime; EVE needs no
  injection code.
- Harness template: tests/ansi_fixpoint/{CMakeLists.txt,main.cpp} — console
  app, getCapturedBytes/getEmittedFrame/isFixtureFixpointStable structure
  (main.cpp:5-132), registry seeding (:293-296).
- eve SPEC.md: Processor owns terminal emulation, View Components are dumb
  paint-only (:36-37); renderer jam::vulkan LLGC + juce software renderer as
  oracle (:26).
- 256-colour materialisation: generated xterm palette table (jam generated
  lane, landed with the AnsiDocument sprint) resolves palette-indexed Stamp
  roles to RGB at materialisation.

## Names (NAMES.md Rule -1 — ratified at plan approval)

| Name | Family precedent |
|---|---|
| Patch file `jam/patch/juce-attributed-text-hook.patch` | five sibling `juce-*-hook.patch` files |
| `juce::LowLevelGraphicsContext::drawAttributedText (const AttributedString&, const Rectangle<float>&)` → `bool`, default `return false;` | early-exit hook family (vulkan-engine-hook pattern); verb `draw` per LLGC family |
| `TerminalITerm2Encoder`, `jam_terminal/image/jam_TerminalITerm2Encoder.h` | `TerminalITerm2Decoder` mirror (Rule 5 nearest sibling) |
| `jam_TerminalGraphicsContextPath.cpp`, `...Image.cpp`, `...Transparency.cpp`, `...Text.cpp` (only those the size demands) | Vulkan LLGC per-concern TU family |
| `GraphicsEngine::frameId` counter; `GraphicsEngine::imagePlacements` (row, col, escape bytes) | engine member family (front/back/widthCols) |
| Harness target `llgc_parity`, `eve/tests/llgc_parity/` | `ansi_fixpoint` sibling |
| eve project-info row `jam_terminal` | existing 12 user-module rows |

No other new names. Engineer introduces zero identifiers beyond this table plus
locals named in delegation prompts.

## Validation Gate

Each step validated by COUNSELOR before the next — against MANIFESTO.md
(BLESSED), NAMES.md, CODING.md CRITICAL RULES, and the locked rulings above
(the cell law is a locked decision). @Auditor runs ONCE, after the final step.
Every Engineer prompt carries: Design by Contract per CODING.md CRITICAL
RULES; the MANIFESTO **E** MVP data-flow contract verbatim; doxygen-first on
C++ reads; no comments/doxygen authorship (dedicated pass post-audit);
Librarian findings prepended where external protocol facts apply (OSC 1337
findings from this session's Librarian run).

## Steps

### Step 1: The sixth patch — attributed text virtual
**Scope:** new `jam/patch/juce-attributed-text-hook.patch`; eve
`CMakeLists.txt` patch list (:100-106); `eve/tests/ansi_fixpoint/CMakeLists.txt`
JUCE consumption.
**Action:** Author the patch against pristine JUCE 8.0.14, matching the
vulkan-engine-hook diff structure:
- `juce_LowLevelGraphicsContext.h`: add
  `virtual bool drawAttributedText (const AttributedString&, const Rectangle<float>& area) { return false; }`
  (ignoring the return, pixel backends never see it).
- `juce_GraphicsContext.cpp`: each of the seven text methods
  (drawText ×3, drawFittedText ×2, drawSingleLineText, drawMultiLineText)
  gains an early-exit hunk: build a one-run `AttributedString`
  (text + `context.getFont()`, justification set from the argument),
  `if (context.drawAttributedText (attributed, area)) return;` — colourless
  run; the override reads its own fill.
- `juce_AttributedString.cpp` `draw()`: early-exit hunk
  `if (g.getInternalContext().drawAttributedText (*this, area)) return;` —
  the multi-run lane for widgets.
Register the patch: append to `CAST_JUCE_PATCHES`; the SHA256 stamp restamps
automatically. Switch the ansi_fixpoint harness (and the new Step 7 harness)
onto the patched tree: replicate the patch-apply block against the same
`/tmp` target + stamp file (idempotent — same hash, no reapply when eve
configured first).
**Validation:** `git apply --check` clean on pristine tree; eve Debug
configure applies and builds green with zero behavior change (default-false
path); harness reconfigures against patched tree and stays green.

### Step 2: Geometry parity — clips, paths, transparency, trivia
**Scope:** `jam_terminal/graphics/jam_TerminalGraphicsContext.h` (+
`jam_TerminalGraphicsContextPath.cpp` / `...Transparency.cpp` if the placement
table demands — non-template bodies beyond trivial go to .cpp), jam_terminal.h
include lines, `jam_terminal.cpp` TU includes.
**Action:** Implement per the cell law:
- `clipRegionIntersects`: intersect the argument (origin-translated,
  quantized) against clipRegions — real answer, no assert.
- `clipToPath`: cell-granular — EdgeTable-rasterize the path at cell
  resolution (juce::EdgeTable — framework API, no manual scanline
  arithmetic); cells with coverage ≥ half become the intersection region list.
- `fillPath`: rasterize via juce::EdgeTable at cols × (2·rows) — two vertical
  samples per cell; both samples covered → bg fill run (fillCells); one
  sample → `▀`/`▄` cell with fill colour through Stamp (BoxDrawing renders
  these pixel-side; any terminal renders them natively).
- `beginTransparencyLayer`/`endTransparencyLayer`: layer stack of overlay
  cell grids (Array<AttributedChar> sized to the engine); draws target the
  top layer; end composites per cell — character cells keep their codepoint
  and lerp colours by opacity; pixel cells lerp both half colours; blank
  cells lerp bg. Composite through Stamp intern (no colour math outside the
  entry build).
- `getFrameId`: return `engine.frameId` (engine gains the counter,
  incremented in endFrame).
- `getPreferredImageTypeForTemporaryImages`: return
  `std::make_unique<juce::SoftwareImageType>()`.
- `drawGlyphs` stays `jassertfalse` with its Amendment 16/19 rationale.
**Validation:** compiles via harness gate; each implementation cites its cell-
law clause; no branch chains past 3 (dispatch/loops over regions); zero new
names beyond table; CRITICAL RULES sweep.

### Step 3: Text — the terminal drawAttributedText override
**Scope:** `jam_terminal/graphics/jam_TerminalGraphicsContext.h` (+
`...Text.cpp` if size demands).
**Action:** Override `drawAttributedText`: for each attribute run —
resolve colour (run colour; transparent sentinel → own fillType colour),
resolve flags from the run Font (bold/italic → Stamp flags), intern
Stamp::Entry; walk the run text with
`AttributedChar::fromCodepoint` + `graphemeSegmentationStep` + width
arithmetic (the exact pen mechanics AnsiDocument::build uses — same atoms,
same registries); quantize the area origin to cells; write cells through the
same clip/composite path drawCells uses; return true. Justification: honor
horizontal left/centre/right by cell-count arithmetic on the measured run
width; vertical centre by row pick.
**Validation:** a `juce::Label`/`LookAndFeel_V4` panel painted via
`paintEntireComponent` emits its text as plain SGR characters (harness Step 7
proves formally); UAX #29 machinery reused — zero new segmentation code;
MANIFESTO E: cells built once at creation, written in place.

### Step 4: Images — SKiT encoder + both lanes
**Scope:** new `jam_terminal/image/jam_TerminalITerm2Encoder.h`;
`jam_TerminalGraphicsContext.h/...Image.cpp`; `jam_TerminalGraphicsEngine.h`;
jam_terminal.h include line (image section, before TerminalSkit).
**Action:**
- `TerminalITerm2Encoder`: static append — `juce::PNGImageFormat::
  writeImageToStream` into MemoryOutputStream, `juce::Base64::toBase64`,
  wrap as `ESC ] 1337 ; File = inline=1 ; size=<n> : <base64> BEL` — the
  exact form `TerminalITerm2Decoder` accepts (:25, :61, :86-94). Framework
  API only; no manual base64/PNG.
- Engine: `imagePlacements` list ({row, col, bytes}); endFrame emits each
  after row serialization with a cursor-position prefix; cleared per frame;
  frameId increment lands here too.
- `drawImage` override: quantize target bounds; lane switch on the engine's
  capability (constructor argument, default half-block):
  - **half-block lane:** `image.rescaled (cols, 2·rows)` (framework API),
    BitmapData readOnly walk; per cell top/bottom colours → `▀` cell with
    truecolor Stamp entry (bg = bottom, fg = top); cells written through the
    normal clip path — fixpoint-parseable.
  - **OSC 1337 lane:** encoder bytes into imagePlacements at the quantized
    cell origin; the covered cells receive erase cells (bg) so the grid
    stays the SSOT under the overlay.
- `clipToImageAlpha`: reduce the transformed image's alpha to per-cell
  coverage (BitmapData walk, cell-granular per the law) → intersect region
  list, same shape as clipToPath.
**Validation:** encoder output decoded by `TerminalITerm2Decoder` in-process
→ pixel-identical to source (PNG lossless); half-block cells re-parse through
AnsiDocument byte-fixpoint; no manual pixel arithmetic where a JUCE API
exists (rescaled, BitmapData, Base64, PNGImageFormat).

### Step 5: TextEditor::paint — cells to attributed runs
**Scope:** `jam_terminal/widgets/jam_TextEditor.h`.
**Action:** Give the Decision 15 scaffold its paint: walk the document root's
line elements (Id::line / Id::cells — the AnsiDocument shape), group each
line's Cells span into uniform-style runs (styleId change = run boundary,
spacerTail skipped), materialise each run: Stamp entry → colour (role palette
resolve via the generated xterm table for palette-indexed modes; direct
colour otherwise), font style from flags (bold/italic on the editor's
monospace font); build one `juce::AttributedString` per line; call
`attributed.draw (g, lineArea)` — the patched seam routes it: terminal
override → characters; Vulkan default-false → stock TextLayout/glyph
pipeline. Line geometry: rows from the component font's line metrics
(cell-height grid).
**Validation:** one paint path, zero backend knowledge in the widget, no
dynamic_cast, no getters poking the context; runs built at paint
(materialisation — creation, allowed), cells read in place from the AST
(transit untouched); read-only honored.

### Step 6: EVE wiring — module row + one-shot viewer
**Scope:** eve `project-info.md` (:246-264 table), regenerated
`CMakeLists.txt` (cast), `Source/EVEView.h/.cpp`.
**Action:** Add the `jam_terminal` user-module row; run cast; verify
regenerated CMakeLists gains the juce_add_module line. EVEView
`initialiseView()`: alongside the existing ViewEditor, own an
`jam::AnsiDocument` + `jam::TextEditor` (readOnly) child; fixture source: the
standalone command line (`JUCEApplication::getCommandLineParameters()` file
path); absent → the viewer stays empty. Layout: TextEditor fills the editor
bounds (temporary demo placement — ARCHITECT amends visually).
**Validation:** eve Debug builds all formats; standalone launches; `EVE
tests/ansi_fixpoint/fixtures/ls.ansi` renders the fixture through the Vulkan
backend (ARCHITECT runs and eyeballs — AGENTS BUILD FOR ARCHITECT TO TEST);
cast double-run fixpoint on regenerated files.

### Step 7: llgc_parity harness — the formal proof
**Scope:** new `eve/tests/llgc_parity/` (CMakeLists.txt, main.cpp,
fixtures/), mirroring ansi_fixpoint structure; ansi_fixpoint re-run.
**Action:** Console app on the patched JUCE tree + jam modules. Suites:
1. **Component fixpoint:** a panel of native components (juce::Label,
   juce::TextButton, juce::GroupComponent under juce::LookAndFeel_V4) built
   headless (ScopedJuceInitialiser_GUI), `paintEntireComponent` through
   `Graphics { terminal GraphicsContext }` → frame bytes → AnsiDocument
   parse + AnsiValidator → re-derive → re-emit → byte-identical
   (MANIFESTO D).
2. **Software oracle:** same panel → `LowLevelGraphicsSoftwareRenderer` onto
   an Image at cols × 2·rows; per half-cell colour compare against the
   terminal grid for geometry cells (non-character cells) within a stated
   per-channel tolerance constant; character cells verified by codepoint
   equality against the drawn strings instead (text is bytes, not pixels —
   the cell law).
3. **Image round-trip:** drawImage OSC 1337 lane → decode via
   TerminalITerm2Decoder → pixel-exact vs source; half-block lane → expected
   downsample colours; both lanes fixpoint-stable where cells carry them.
4. **Transparency/path/clip cases:** rounded-rect fill (corner half-blocks +
   interior bg + label text riding it via compositeOntoBackground), curved
   clip over text (cell-granular pass/drop), transparency layer over mixed
   content (colour lerp, characters survive).
Artifacts dump to `.ansi` files for `cat` eyeball. COUNSELOR builds and
iterates the harness to green (standing permission from the AnsiDocument
sprint: "you run the test, iterate until all tests conformance green");
ARCHITECT runs EVE.
**Validation:** all suites green; a deliberately corrupted expectation fails
loudly; no window/GPU/PTY in the harness process.

### Step 8: Sprint close
COUNSELOR re-reads every file against this plan. @Auditor sweeps once, whole
sprint. Findings resolved per DCF §5. Then ARCHITECT's checklist commands
(doxygen pass, logs) on his word.

## BLESSED Alignment

- **B:** layers/overlays owned by the context's stack (RAII, dies with the
  frame); imagePlacements owned by the engine, cleared per frame; Stamp/
  Grapheme/Hyperlink identity only through registry APIs.
- **L:** per-concern TUs mirror the Vulkan family; EdgeTable replaces manual
  scanline math; dispatch/loops over branch chains; 30-line functions.
- **E (Explicit):** default-false hook is loud and visible in the patch; no
  bail-outs — positive nesting per existing GraphicsContext shape; the lane
  switch is a named constructor argument, not hidden state.
- **S (SSOT):** the decoder's parse defines the encoder's format — one wire
  truth, proven by round-trip; Stamp tables remain the SGR vocabulary;
  palette resolve through the one generated xterm table; cell law stated once,
  cited per implementation.
- **S (Stateless):** context state is per-frame transient (already the
  design); overlay grids are creation buffers, die at endLayer.
- **E (Encapsulation):** widgets see `juce::Graphics` only — the patch is the
  seam; decode-in stays SKiT's, encode-out enters SKiT's family; ANSI at two
  edges only.
- **D:** fixpoint + oracle + round-trip are the determinism proofs — same
  input, bit-identical bytes.

## Risks / Open Questions

1. **Grid-exact glyph placement on the Vulkan lane:** Step 5 routes Vulkan
   text through stock TextLayout shaping — proportional-correct, monospace-
   approximate. The exact cell-fit lane (`jam::GlyphArrangement::setCellRun`,
   already AttributedChar-aware) is wired in a follow-up when the live grid
   demands it. Flagged, not blocking the one-shot.
2. **Default-path cost of the patch:** the seven Graphics hunks construct a
   one-run AttributedString before the (default-false) vcall. Bounded — JUCE
   already heap-builds a GlyphArrangement per call — but it taxes pixel
   backends slightly. Alternative (a gating `is*` virtual) adds patch surface;
   ARCHITECT may amend.
3. **OSC 1337 overlay vs fixpoint:** placements are emitted bytes, not cells;
   AnsiDocument consumes generic OSC without image semantics, so fixpoint
   covers the grid and the round-trip proof covers the image bytes. Stated,
   not hidden.
4. **RFC residuals:** :347 (Kitty/`END;` ladder) superseded by this sprint's
   rulings — ARCHITECT's amendment pass owns the RFC edit.
