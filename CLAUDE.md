# EVE — JUCE Audio Plugin Project

---

## Project Identity

| Field | Value |
|-------|-------|
| **Name** | EVE |
| **Type** | JUCE Audio Plugin — Effect (Fx) |
| **Version** | 0.1.0 (`CMakeLists.txt:62`) |
| **Stack** | C++17, JUCE 8.0.14, JAM framework, Vulkan renderer |
| **Formats** | Standalone, VST3, AU (`CMakeLists.txt:67`) |
| **Platforms** | macOS 11.0+ (x86_64 + arm64), Windows (MSVC) |
| **Root** | `/Users/jreng/Documents/Poems/dev/eve` |

---

## Current State

**Last Sprint:** One Atom, One Style Word, Both Lanes on One SSOT — CMYK Mermaid, Display-Scale Diagrams (2026-10-17) (`carol/SPRINT-LOG.md:39`)

**Active ODE:** None found in `carol/SPRINT-LOG.md` or `DEBT.md`.

**Active Debt:** DEBT-20261016T214137 — EVE Standalone launch takes 5–7 s; jfs opens almost at once (`DEBT.md:11`).

---

## Layer Order & Key Source Dirs

1. **JUCE 8.0.14 + patches.** Eight patches, applied in order at configure time to a cached copy (`CMakeLists.txt:124-133`). Source: `../../JUCE`.
2. **JAM modules.** Eighteen modules added with `juce_add_module` (`CMakeLists.txt:229-246`). Source: `../jam/`.
3. **EVE application.**

| Path | Role |
|------|------|
| `Source/EVEProcessor.h/.cpp` | Plugin entry, state serialisation |
| `Source/EVEAudioProcessor.h` | Audio processor declaration |
| `Source/EVEView.h/.cpp` | Plugin editor: theme, component registry, panel and view construction |
| `Source/EVEViewRegistration.cpp` | Button registration and dialog config |
| `Source/EVEViewComponents.cpp` | View component configs |
| `Source/EVEViewPanelCallbacks.cpp` | Panel, About and Settings callbacks |
| `Source/generated/` | CAST output: `Identifiers.*`, `Files.*` (build-time, do not edit) |
| `Source/layout/` | Layout and style documents: `ViewLayout.md`, `component.md`, `parameters.md`, `style.css`, `PanelLayout.html`, `AboutLayout.html`, `SettingsLayout.html` |
| `Source/buttons/`, `Source/images/`, `Source/icons/` | Button SVGs, PNG images, app icon |
| `markdown/` | Markdown and mermaid harness (`markdown/markdown.md`) |

---

## Key Docs

| Document | Role | Status |
|----------|------|--------|
| `SPEC.md` | Normative specification (0.0.1) | Present |
| `RFC-EVE-terminal-grand-scheme.md` | Architecture, evidence, non-negotiables | Present |
| `ARCHITECTURE.md` | System architecture | Not present |
| `project-info.md` | SSOT for build metadata | Present |
| `HANDOFF.md` | Session handoff | Present |
| `carol/SPRINT-LOG.md` | Cross-session work log | Present |
| `DEBT.md` | Inter-sprint debt ledger | Present |

---

## Doxygen

| Library | Index | Status |
|---------|-------|--------|
| EVE (project) | `docs/xml/index.xml` | Present. Regenerate: `ninja doxygen` |
| JAM | `~/Documents/Poems/dev/jam/docs/xml/index.xml` | Present. Regenerate: `<leader>bd` in nvim |
| JUCE | `~/Documents/Poems/JUCE/docs/xml/index.xml` | Present |

KANJUT and CIUM are not used by EVE. Their indexes are not listed.

Read the index first, then the compound XML, before any grep. See the `doxygen-protocol` skill.

---

## Build

1. `project-info.md` is the SSOT. `CMakeLists.txt` is generated from it by CAST. Do not edit `CMakeLists.txt` by hand.
2. Regenerate and build: `cast cast/spell.md --debug` (`carol/SPRINT-LOG.md:92`).
3. Build directories: `Builds/Debug`, `Builds/Release`.
4. JUCE version gate: exact match to 8.0.14 (`CMakeLists.txt:215`). A mismatch stops configure.
5. Patched JUCE cache: `~/.cache/cast/juce/<hash>` on macOS, `%LOCALAPPDATA%/cast/juce/<hash>` on Windows (`CMakeLists.txt:136-152`). The hash covers the JUCE version and the patch files.
6. Compiler flags:
   - macOS Release: `-O3 -flto=thin -Wno-{shadow,unused-parameter,float-equal,sign-conversion,switch-enum,implicit-float-conversion} -dead_strip`
   - Windows Release: `/O2 /GL /permissive- /Zc:rvalueCast /W4 /FC /wd{...}`, linker `/LTCG /OPT:{REF,ICF}`
   - Debug: `-O0 -g` (macOS) or `/Od /Zi` (Windows)
7. Shaders: glslc compiles `*.vert`, `*.frag`, `*.comp` to `*.spv`, with dependency tracking (`CMakeLists.txt:443-477`).
8. Release on macOS: VST3 and AU are signed and notarised. Standalone is signed (`CMakeLists.txt:600-629`).
9. LSP errors come from the JUCE module system. Ignore them. The compiler is the truth.

---

**Rock 'n Roll!**
**JRENG!**
