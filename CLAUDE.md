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

**Last Sprint:** JAM is the primary record of EVE sprints (`~/Documents/Poems/dev/jam/carol/SPRINT-LOG.md`, Sprint 176, 2026-10-27). Plan in flight: `~/Documents/Poems/dev/jam/PLAN-terminal-model-lane.md`.

**Active ODE:** None.

**Active Debt:** DEBT-20261025T214955 — a custom OSC code carries a CommandId name (`DEBT.md:11-15`).

**Architecture rulings:** `SPEC.md` (Core Principles 7–10, State Model, Architecture Constraints) and `RFC-EVE-terminal-grand-scheme.md` (amendments 2026-10-27 in §0, §3.9, §8). Read them before any change to state, parameters, threads or the clock.

---

## Layer Order & Key Source Dirs

1. **JUCE 8.0.14 + patches.** Eight patches, applied in order at configure time to a cached copy (`CMakeLists.txt:124-133`). Source: `../../JUCE`.
2. **JAM modules.** Nineteen modules added with `juce_add_module` (`CMakeLists.txt`, MODULES section): core, audio_devices, debug, data_structures, dsp, gui, graphics, animation, freetype, vulkan, style, markdown, document, web, plugin_bootstrap, pdf, mermaid_diagram, markdown_graphics, terminal. Source: `../jam/`.
3. **EVE application.**

| Path | Role |
|------|------|
| `Source/EVEProcessor.h/.cpp` | Plugin entry, state serialisation (one APVTS tree with the terminal Model child), MASTER BYPASS parameter, the virtual device (standalone clock), `prepare()` → `prepareToPlay` |
| `Source/EVETerminalProcessor.h/.cpp` | Terminal chain on the clock: TTY (`$SHELL`), input lane, decoder (`jam::TerminalProcessor`), row ring (`jam::TerminalBuffer`), record (`jam::TerminalSwap`), and the terminal state machine (`jam::Model`, the Model lane) |
| `Source/EVEView.h/.cpp` | Plugin editor: theme, component registry and view construction |
| `Source/EVEViewRegistration.cpp` | Button registration and dialog config |
| `Source/EVEViewComponents.cpp` | View component configs; the TextEditor event binding: key and mouse encoding to the input lane, live caret values, TERMINAL mode at start |
| `Source/EVEViewPanelCallbacks.cpp` | UI scale and winsize callbacks |
| `Source/generated/` | CAST output: `Identifiers.*`, `Files.*` (build-time, do not edit) |
| `Source/layout/` | Layout and style documents: `ViewLayout.md`, `component.md`, `parameters.md` (automatable host parameters only), `model.md` (the terminal state machine, the Model lane), `style.css`, `SettingsLayout.html` |
| `Source/buttons/`, `Source/icons/` | Button SVGs, app icon |

---

## Key Docs

| Document | Role | Status |
|----------|------|--------|
| `SPEC.md` | Normative specification (0.0.1) | Present |
| `RFC-EVE-terminal-grand-scheme.md` | Architecture, evidence, non-negotiables | Present |
| `ARCHITECTURE.md` | System architecture | Not present |
| `project-info.md` | SSOT for build metadata | Present |
| `HANDOFF.md` | Session handoff: current state and the plan in flight | Present |
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
2. Regenerate and build: `cast cast/spell.md --debug` (`carol/SPRINT-LOG.md:60`).
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
