# EVE — JUCE Audio Plugin Project

**MACHINIST synthesis from: project-info.md, CMakeLists.txt, git status**

---

## Project Identity

| Field | Value |
|-------|-------|
| **Name** | EVE |
| **Version** | 0.1.0 (0x100 hex) |
| **Type** | JUCE Audio Plugin — Effect (Fx) |
| **Formats** | Standalone, VST3, AU, AAX |
| **Stack** | C++17, JUCE 8.0.14, JAM framework, Vulkan renderer |
| **Build** | CMake 4.2.0+ / Ninja, single-file manifest (CAST-generated) |
| **Platforms** | macOS 11.0+ (x86_64 + arm64), Windows (MSVC) |
| **Root** | `/Users/jreng/Documents/Poems/dev/eve` |
| **Company** | JRENG (Jubilant Research of Eclectic Novelty Generation) |
| **Bundle** | com.jreng.EVE |
| **Manufacturer** | JRNG / EVE. |

---

## Current State

**Last Work:** 2026-09-02 — EVEView lifecycle + manifest hygiene, LookAndFeel unset, EVE residue cleanup

**Branch:** main (active development)

**Outstanding:** None declared — SPEC.md present; ARCHITECTURE.md not present (EVE project artifacts remain in working tree; cleanup in progress per commit 264889a)

**Build Status:** Ready — CMakeLists.txt generated from project-info.md (CAST Projucer oracle)

---

## Layer Order & Key Directories

### 1. JUCE + Patches
- Base: `../../JUCE` (8.0.14, patched in-place to `/tmp/juce-patched` at configure time)
- **Patches applied (5):**
  - `juce-cached-image-factory-hook.patch` — External CachedComponentImage factory
  - `juce-direct2d-helpers-visibility-hook.patch` — Direct2D visibility gate
  - `juce-image-subsection-hook.patch` — Root image + subsection bounds hooks
  - `juce-vulkan-engine-hook.patch` — External graphics context factory for Vulkan
  - `juce-paint-update-rect-hook.patch` — OS dirty rectangle before native paint

### 2. JAM Framework Modules (read-only, user modules)
- **Core:** jam_core, jam_debug, jam_data_structures
- **DSP:** jam_dsp (filters, waveshaping, hysteresis, transient, oversampling, FIR, noise, spectrum)
- **Graphics:** jam_graphics (blur, shadows, colours, fonts, mesh), jam_vulkan (GPU), jam_freetype (font rasterization)
- **UI:** jam_gui (Window, Modal, Glass), jam_animation (Animator, AnimationBase, AnimationScrollingText), jam_style (LookAndFeel + ColourScheme registry)
- **Parsing:** jam_markdown (CommonMark + GFM), jam_web (HTML + CSS Level 3 subset)
- **Plugin:** jam_plugin_bootstrap (document-driven, style management, editor base, standalone shell)
- **Path:** `../jam/` (sibling to eve)

### 3. EVE Application

| File | Purpose |
|------|---------|
| `Source/EVEProcessor.h/.cpp` | AudioProcessor subclass, parameter manager, plugin state |
| `Source/EVEView.h/.cpp` | Editor component (jam_plugin_bootstrap base) |
| `Source/generated/` | CAST code-generation output (build-time) |
| `Source/layout/` | **Canonical UI definitions:** markdown tables (interface.md), CSS (style.css), XML (config) — fonts glob from `${CAST_USER_MODULE_PATH}/resources/fonts/*.ttf`, not here |

### 4. Build Manifesto

| File | Purpose |
|------|---------|
| `project-info.md` | **SSOT for all build metadata** — project name, version, company, formats, signing, compiler flags, module list, CMake defines, architecture list |
| `CMakeLists.txt` | **Generated from project-info.md** (CAST oracle, Projucer equivalent) — self-contained single file, zero external cmake modules, exact JUCE version gate |
| `cast/` | CAST identifiers table (future: generate CMakeLists.txt + metadata headers) |
| `entitlements.plist` | macOS code-signing entitlements |

### 5. Build Directories (generated)

- `Builds/Release` — CMake build output, optimized
- `Builds/Debug` — CMake build output, debug symbols
- `~/Library/Audio/Plug-Ins/VST3/`, `~/Library/Audio/Plug-Ins/Components/`, `/Library/Application Support/Avid/Audio/Plug-Ins/` — installed formats (macOS)

---

## Build Configuration

**CMake Oracle:** project-info.md (every build value traces to one table row)

### Formats & Install Paths

| Format | Bundle Pattern | Install Directory |
|--------|---|---|
| Standalone | EVE | (none) |
| VST3 | EVE.vst3 | ~/Library/Audio/Plug-Ins/VST3 |
| AU | EVE.component | ~/Library/Audio/Plug-Ins/Components |
| AAX | EVE AAX | /Library/Application Support/Avid/Audio/Plug-Ins |

### Compiler Flags

**macOS Release:** `-O3 -flto=thin -Wno-{shadow,unused-parameter,float-equal,sign-conversion,switch-enum,implicit-float-conversion} -dead_strip`

**Windows Release:** `/O2 /GL /permissive- /Zc:rvalueCast /W4 /FC /wd{4456,4459,4100,4189,4505,4267,4065,4244,4996,4611,4324,4200,4702}` + linker `/LTCG /OPT:{REF,ICF}`

**Debug:** `-O0 -g` (macOS) or `/Od /Zi` (Windows) + same warning suppression

### Defines (CMake generator expressions)

| Define | Value | Context |
|--------|-------|---------|
| DONT_SET_USING_JUCE_NAMESPACE | 1 | All |
| JAM_USING_OVERSAMPLING | 0 | All |
| JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP | 1 | All |
| JUCE_STRICT_REFCOUNTEDPOINTER | 1 | All |
| JUCE_WEB_BROWSER, JUCE_USE_CURL | 0 | All |
| DEBUG | 1 | Debug only |
| NDEBUG | 1 | Release only |
| WIN32_LEAN_AND_MEAN, NOMINMAX, _WIN32_IE | platform-specific | Windows only |
| JUCE_GRAPHICS_INCLUDE_DIRECT2D_HELPERS | 1 | Windows only (jam patch hook) |

### Shaders & Resources

**Shader Compilation:** glslc (Vulkan SDK) compiles *.vert, *.frag, *.comp → *.spv (CONFIGURE_DEPENDS tracked)

**Binary Data:** Embedded into plugin binary:
- `BinaryData` namespace: layout (*.md, *.css, *.xml), fonts (*.ttf), shaders (*.spv)
- `jam::fonts::*` namespace: fonts for jam_graphics
- `jam::*` namespace: compiled shaders for jam_vulkan

### LTO & IPO

- **Release build:** LTO enabled (`-flto=thin` macOS, `/GL` MSVC); IPO checked and applied if supported
- **Debug build:** LTO disabled

### macOS Signing & Notarization (Release builds only)

**VST3 / AU:**
1. Remove extended attributes (`xattr -cr`)
2. Code-sign with entitlements (`codesign --force --options runtime --entitlements`)
3. Verify signature (`codesign --verify`)
4. Zip and notarize (`xcrun notarytool submit --wait`)
5. Staple ticket (`xcrun stapler staple`)
6. Archive to QA directory (`~/Documents/Poems/dev/___builds___`)

**AAX:**
1. Wrap-sign (`wraptool sign --account bayu@jrengmusic.com`)
2. Auto-notarize via PACE keychain profile

**Windows:** No signing configured

---

## Doxygen

**Status:** Not configured at project level. JAM modules include doxygen setup independently.

**EVE project:** Zero doxygen blocks (plugin-specific docs deferred; framework docs in jam modules).

**Activation:** If needed, coordinate with JAM doxygen rebuild (shared `~~~lib~~~` at framework level).

---

## Build Notes

1. **CAST Oracle:** Every value in CMakeLists.txt traces to project-info.md; no hardcodes. Edit project-info.md, run `cast`, reconfigure.

2. **JUCE Version Gate:** CMakeLists enforces exact JUCE 8.0.14; mismatch is fatal error. Patches applied and stamped on first configure; patch-set hash recomputed if any patch file changes.

3. **Paths Resolved Once:** JUCE, JAM modules, Vulkan SDK paths set at CMake configure time (generator expressions route platform/config flags downstream).

4. **LSP False Positives:** JUCE module system (intricate includes) produces LSP errors; compiler sees green. Ignore LSP errors in-editor.

5. **Module Availability:** jam_* modules included via `juce_add_module()` at configure time; if a jam module is missing or misconfigured, CMakeLists fails at add_module step.

6. **Format Targets:** Each format is a separate CMake target (e.g., `EVE_VST3`, `EVE_AU`); all inherit source list and common flags from main `EVE` target.

7. **No External Cmake Files:** CMakeLists.txt is self-contained; zero FindXXX scripts or ToolchainFiles; all paths absolute or computed from CMAKE_CURRENT_SOURCE_DIR.

---

**Rock 'n Roll!**  
**JRENG!**
