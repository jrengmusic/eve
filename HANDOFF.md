# HANDOFF — EVE

**Date:** 2026-09-13
**From sprint:** Terminal LLGC Full Feature Parity — One-Shot Viewer + llgc_parity (logged, eve + jam Sprint 119)
**Open symptom:** EVE standalone launches to a blank window (ARCHITECT-observed; not yet diagnosed at runtime).

---

## Next Sprint Goal

**First light: EVE standalone renders `ls.ansi` through the one-shot viewer.**

The write lane is proven headless (four llgc_parity suites, byte-fixpoint, pixel
oracle, decode round-trip). Nothing on screen is proven. The sprint closes when
ARCHITECT runs:

```
./Builds/Debug/EVE_artefacts/Debug/Standalone/EVE.app/Contents/MacOS/EVE tests/ansi_fixpoint/fixtures/ls.ansi
```

and sees the fixture rendered through the Vulkan backend.

---

## The Symptom

Blank window. Not diagnosed. Facts on disk:

- `EVEView::initialiseView()` (EVEView.cpp:32-52): creates the ViewEditor
  (`jam::ViewEditor::create<jam::MarkdownDocument>`, :37), then parses the first
  existing-file command-line parameter into `ansiDocument` (:40-49, cwd-relative
  safe), then `addAndMakeVisible (terminalView)`.
- `EVEView::resized()` (EVEView.cpp:59-64): `terminalView.setBounds
  (getLocalBounds())` — the viewer overlays the ViewEditor, full bounds
  (declared temporary demo placement).
- `jam::TextEditor::paint` (jam_TextEditor.h): draws line elements only. An
  empty document paints NOTHING — the component fills no background. A blank
  viewer over a blank ViewEditor is indistinguishable from a broken paint path.

## Diagnostic Leads (in order — runtime evidence first, per ODE)

1. **Was a fixture path received at all?** Launching without an argument (or
   from Finder) leaves `ansiDocument` empty by design — the viewer then paints
   nothing, correctly. Instrument with `jam::debug::Log` (EVEView already owns
   a `Log::Scope`, EVEView.h:28-29): log the parameter array, the resolved
   file, `document` line count after parse.
2. **Does paint run?** Log entry/exit of `TextEditor::paint` and the per-line
   loop count. Every EVE paint routes through
   `juce::ComponentPeer::externalContextFactory`, owned by VulkanEngine
   (jam_VulkanEngine.h:81; GPU context or CPU fallback, :656/:1058).
3. **ARCHITECT-stated fact: the ViewEditor was always blank before this
   sprint — nothing changed.** The symptom predates the viewer. Two
   independent halves: (a) the ViewEditor lane's blank is pre-existing and its
   own issue; (b) the terminal viewer's render is simply unverified. First
   light targets (b) — do not chase (a) unless ARCHITECT widens scope.
4. **Text through the patched seam on the pixel lane:** the viewer's text runs
   go `AttributedString::draw` → Vulkan default-false → stock TextLayout →
   `drawGlyphs`. Verified compiling; never verified on screen. Unstyled runs
   resolve colour via `getLookAndFeel().findColour (juce::TextEditor::
   textColourId)` (jam_TextEditor.h getRunColour) — if the active LookAndFeel
   resolves that id to the window background colour, text paints invisibly.
   Check the resolved colour value in the log.

Diagnostics are ephemeral — remove same sprint (ODE §VI).

## After First Light (same sprint or next, ARCHITECT's call)

- **RFC gap 4 — Vulkan cell-grid adapter:** viewer text currently routes
  through stock proportional shaping. The exact cell-fit lane exists half-built:
  `jam::GlyphArrangement` already consumes AttributedChar (shapeCell/findGlyph)
  and threads `setCellRun` → `drawGlyphs` into the atlas key
  (jam_vulkan/font/jam_GlyphArrangement.cpp). The walker that feeds it is the
  missing piece.
- **CAST `--help`** — next consumer on the validation ladder (RFC §6), after
  the viewer proves the pixel lane.

## State of the World (proven, this date)

- Terminal LLGC: full parity under the cell law; `drawGlyphs` asserts by design.
- Sixth JUCE patch (`juce-attributed-text-hook`) applied via the stamp lane;
  default-false — pixel backends behavior-neutral, proven by harness regression.
- SKiT: decode + encode halves; OSC 1337 round-trip pixel-exact.
- Harnesses: `tests/ansi_fixpoint`, `tests/llgc_parity` — both green from eve
  root; artifacts in `tests/llgc_parity/artifacts/*.ansi` (cat-able).
- EVE Debug: all-format configure green; EVE_Standalone links clean.

## Residuals Standing (from the sprint log, unchanged)

- eve CLAUDE.md lists five patches — now six (MACHINIST's artifact).
- eve-root `cast` exits 1 after correct generation (recursive jam toolchain
  step); generation itself is correct.
- Builds/Release partial from a killed runaway build (artifacts only).
- DEBT-20260908T000000 / DEBT-20260908T000001 — standing, EVE plugin lane.
