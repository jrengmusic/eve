# EVE — JUCE Audio Plugin Project

**MACHINIST synthesis: 2026-09-15 — project-info.md, CMakeLists.txt, SPEC.md, SPRINT-LOG.md, DEBT.md**

---

## Project Identity

| Field | Value |
|-------|-------|
| **Name** | EVE |
| **Version** | 0.1.0 (0x100 hex) |
| **Type** | JUCE Audio Plugin — Effect (Fx) |
| **Formats** | Standalone, VST3, AU, AAX |
| **Stack** | C++17, JUCE 8.0.14, JAM framework, Vulkan renderer |
| **Build** | CMake 4.2.0+ / Ninja, CAST-generated single-file manifest (project-info.md SSOT) |
| **Platforms** | macOS 11.0+ (x86_64 + arm64), Windows (MSVC) |
| **Root** | `/Users/jreng/Documents/Poems/dev/eve` |
| **Company** | JRENG (Jubilant Research of Eclectic Novelty Generation) |
| **Bundle** | com.jreng.EVE |
| **Manufacturer** | JRNG / EVE. |

---

## Current State

**Last Sprint:** TextEditor Foundation — renderer core, MODE-node state, vim basic (2026-09-20)
- Agents: COUNSELOR (fable-5), Engineer ×8, Pathfinder ×1

**Active ODE:** None declared

**Active Debt:**
- DEBT-20260908T000000 — superseded (NO SVG ruling; editor built in code) — disposition at log time
- DEBT-20260908T000001 — satisfied (EVEAudioProcessor.h uses jam Getters/Events types)

**Branch:** main (active development)

**Build Status:** Ready — CMakeLists.txt generated from project-info.md; all targets link clean (Debug)

**Documentation:**
- SPEC.md (0.0.1, normative) ✓
- RFC-EVE-terminal-grand-scheme.md (architecture, evidence, non-negotiables) ✓
- PLAN-terminal-llgc-parity.md (locked) ✓
- PLAN-ansidocument.md (locked) ✓
- ARCHITECTURE.md (not present — not yet written)

---

## Layer Order & Key Directories

### 1. JUCE Base + Patches (8.0.14, in-place patched to `/tmp/juce-patched`)

**Six patches applied:**
1. `juce-cached-image-factory-hook.patch` — External CachedComponentImage factory
2. `juce-direct2d-helpers-visibility-hook.patch` — Direct2D visibility gate
3. `juce-image-subsection-hook.patch` — Root image + subsection bounds hooks
4. `juce-vulkan-engine-hook.patch` — External graphics context factory for Vulkan
5. `juce-paint-update-rect-hook.patch` — OS dirty rectangle before native paint
6. `juce-attributed-text-hook.patch` — Attributed text integration

**Location:** `../../JUCE` (sibling to eve directory)

### 2. JAM Framework Modules (read-only, user modules)

**Core:** jam_core, jam_debug, jam_data_structures

**DSP:** jam_dsp (filters, waveshaping, hysteresis, transient, oversampling, FIR, noise, spectrum)

**Graphics:** jam_graphics (blur, shadows, colours, fonts, mesh), jam_vulkan (GPU), jam_freetype (font rasterization)

**UI:** jam_gui (Window, Modal, Glass, TextEditor), jam_animation (Animator, AnimationBase, AnimationScrollingText), jam_style (LookAndFeel + ColourScheme registry)

**Parsing:** jam_markdown (CommonMark + GFM), jam_web (HTML + CSS Level 3 subset), **jam_terminal (AnsiDocument, GraphicsEngine/Context, SKiT, Keyboard/TTY/Parser — first consumer via eve harness)**

**Plugin:** jam_plugin_bootstrap (document-driven, style management, editor base, standalone shell)

**Path:** `../jam/` (sibling to eve)

### 3. EVE Application

| File | Purpose |
|------|---------|
| `Source/EVEAudioProcessor.h` | AudioProcessor subclass, parameter manager, plugin state |
| `Source/EVEProcessor.h/.cpp` | Processor implementation |
| `Source/EVEView.h/.cpp` | Editor component (jam_plugin_bootstrap base); owns the AnsiDocument and the jam::TextEditor main view; status bar ViewPanel; eve.md hot reload (style + keys) |
| `Source/generated/` | CAST code-generation output (build-time) |
| `Source/layout/eve.md` | EVE config+style document (settings, UI_size, colours, window, style, fonts); seeded to ~/.config/end/eve.md, watched at runtime |
| `Source/layout/ViewLayout.md` | Title-only view layout document (PluginEditorLayout requires one; the editor is built in code) |
| `Source/layout/interface.md` | (if present) interface definition |
| `Source/layout/parameters.md` | Host parameter table: `mode` choice (normal, insert, visual) |
| `Source/layout/PanelLayout.html` | Status bar layout (bottom ViewPanel row): mode, line, column labels bound to the MODE node |

### 4. Test Harnesses (formal proofs)

| Path | Purpose |
|------|---------|
| `tests/ansi_fixpoint/` | AnsiDocument fixpoint loop (parse → validate → derive → emit → re-parse → byte-compare); fixtures: `ls.ansi`, `wide.ansi`, `grapheme.ansi`, `hyperlink.ansi`, `colon.ansi`, `invalid/unterminated.ansi` |
| `tests/llgc_parity/` | Four-suite LLGC proof: (1) native-component byte-fixpoint + text oracle, (2) software-renderer pixel oracle (AA-boundary exemptions), (3) image round-trip (OSC 1337), (4) rounded-rect inheritance + transparency. Artifacts dump to `artifacts/*.ansi`. |

### 5. Build Manifesto

| File | Purpose |
|------|---------|
| `project-info.md` | **SSOT for all build metadata** — project name, version, company, formats, signing, compiler flags, module list, CMake defines, architecture list, patch table |
| `CMakeLists.txt` | **Generated from project-info.md** (CAST oracle) — self-contained single file, zero external cmake modules, exact JUCE version gate, format targets |
| `cast/cmake.cast` | CAST identifiers table (generates CMakeLists.txt + metadata headers) |
| `entitlements.plist` | macOS code-signing entitlements (Release builds only) |

### 6. Generated Build Directories

- `Builds/Release` — CMake build output, optimized, LTO enabled
- `Builds/Debug` — CMake build output, debug symbols, LTO disabled
- `~/Library/Audio/Plug-Ins/VST3/EVE.vst3` — VST3 format (macOS)
- `~/Library/Audio/Plug-Ins/Components/EVE.component` — AU format (macOS)
- `/Library/Application Support/Avid/Audio/Plug-Ins/EVE AAX` — AAX format (macOS)

---

## Key Documentation Table

| Document | Role | Status |
|----------|------|--------|
| SPEC.md | Normative specification (0.0.1) | Written ✓ |
| RFC-EVE-terminal-grand-scheme.md | Architecture, evidence, non-negotiables | Referenced from SPEC ✓ |
| PLAN-terminal-llgc-parity.md | Implementation plan (locked) | Executed ✓ |
| PLAN-ansidocument.md | Implementation plan (locked) | Executed ✓ |
| ARCHITECTURE.md | System architecture (persistent) | **Not yet written** |
| SPRINT-LOG.md | Cross-session work log | Active (project root, carol/) |
| DEBT.md | Inter-sprint ledger | Active (project root, two entries deferred) |

---

## Compiler Flags & Configuration

**macOS Release:** `-O3 -flto=thin -Wno-{shadow,unused-parameter,float-equal,sign-conversion,switch-enum,implicit-float-conversion} -dead_strip`

**Windows Release:** `/O2 /GL /permissive- /Zc:rvalueCast /W4 /FC /wd{...}` + linker `/LTCG /OPT:{REF,ICF}`

**Debug:** `-O0 -g` (macOS) or `/Od /Zi` (Windows) + same warning suppression

---

## Doxygen

**EVE Project:** Zero doxygen blocks. Plugin-specific docs deferred; framework docs owned by JAM modules.

**JAM Framework:** Doxygen setup independent at framework level. Last full pass completed in sprint 116 (ansidocument sprint, 2026-09-13) — zero warnings (doxygen 1.18.0).

**Activation:** If EVE doxygen needed, coordinate with JAM doxygen rebuild (shared `~~~lib~~~` at framework level). No EVE-specific doxygen XML generated at this time.

---

## Build Notes

1. **CAST Oracle:** Every value in CMakeLists.txt traces to project-info.md; no hardcodes. Edit project-info.md, run `cast`, reconfigure.

2. **JUCE Version Gate:** CMakeLists enforces exact JUCE 8.0.14; mismatch is fatal error. Patches applied and stamped on first configure; patch-set hash recomputed if any patch file changes.

3. **Paths Resolved Once:** JUCE, JAM modules, Vulkan SDK paths set at CMake configure time (generator expressions route platform/config flags downstream).

4. **LSP False Positives:** JUCE module system produces LSP errors; compiler sees green. Ignore LSP errors in-editor.

5. **Module Availability:** jam_* modules included via `juce_add_module()` at configure time; if a jam module is missing or misconfigured, CMakeLists fails at add_module step.

6. **Format Targets:** Each format is a separate CMake target (e.g., `EVE_VST3`, `EVE_AU`); all inherit source list and common flags from main `EVE` target.

7. **No External Cmake Files:** CMakeLists.txt is self-contained; zero FindXXX scripts or ToolchainFiles; all paths absolute or computed from CMAKE_CURRENT_SOURCE_DIR.

8. **Compiler Flags:**
   - **macOS Release:** `-O3 -flto=thin -Wno-{shadow,unused-parameter,float-equal,sign-conversion,switch-enum,implicit-float-conversion} -dead_strip`
   - **Windows Release:** `/O2 /GL /permissive- /Zc:rvalueCast /W4 /FC /wd{...}` + linker `/LTCG /OPT:{REF,ICF}`
   - **Debug:** `-O0 -g` (macOS) or `/Od /Zi` (Windows) + same warning suppression

9. **Shader Compilation:** glslc (Vulkan SDK) compiles *.vert, *.frag, *.comp → *.spv; CONFIGURE_DEPENDS tracked.

10. **Code-Signing (Release macOS only):** VST3/AU signed + notarized; AAX wrapped via wraptool with PACE keychain profile.

---

**Rock 'n Roll!**  
**JRENG!**
