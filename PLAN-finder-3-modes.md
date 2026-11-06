# PLAN: Finder Part 3 — Canon Modes and Registered Modes

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)
**Canon:** `PLAN-finder-0-tracker.md`, rulings 11 to 15

## Overview

TextEditor keeps NORMAL, VISUAL and TERMINAL. Any other mode is a registered row with registered callbacks, and the owner registers them. OPEN, LIST and the markdown panel mode leave JAM. Under END, EVE gives its mode key rows to END.

## Language / Framework Constraints

- Generated tables come from CAST. `jam/generated` is not edited by hand.
- The mode row stays in the `terminal` tree (`jam_TextEditor.cpp:108-111`, `:653-656`).

## Dependency & API Inventory

**The name table already extends:** `map::EditorMode : jam::Bimap<int>` (`jam_Gui.h:331`). `Bimap::add (key, value)` registers one row in both directions (`jam_Bimap.h:142-149`). Every reader goes through the shared instance, so a registered row reaches all of them:
- `CommandManager::setKeyPresses` (`jam_CommandManager.cpp:33`)
- `TextEditorStatusBar::paint`, the label (`jam_TextEditorStatusBar.cpp:37`)
- `PluginEditorLayout` (`jam_PluginEditorLayout.cpp:394`)

**Registration, established:** `Registry` holds `Function::Map` rows keyed by type, and owners fill them through `registerComponent`, `registerStyle`, `registerConfig` and `Registration` (`jam_Registry.h:42-70`, `:99-179`). EVEView registers in `initialiseRegistry` (`EVEView.cpp:229-238`).

**Hardcoded slots to replace (LSP, `jam_Gui.h:331`, 24 references):**
| Slot | Site |
|---|---|
| Key input | `getKeyInputs`, rows `open`, `list` (`jam_TextEditor.cpp:582-593`) |
| Caret | `getCarets`, rows `open`, `list` (`:640-645`) |
| Entry reaction | `Id::mode` handler (`:814-830`) |
| Actions | `Id::open`, `Id::list`, `Id::listDown`, `Id::listUp` (`:440-458`) |
| Paint | `Content::paint` (`:1814-1815`), `Content::paintOverChildren` (`:1836-1837`) |
| Colour | `modeColours` (`jam_TextEditorStatusBar.cpp:19-25`), `.h:33-34`, `textEditor.css:13-14` |
| Commands | `terminal.md:1893-1896`, `2065`, `2072-2073` and the key tables, sizes `spell.md:134-136` |
| Default keys | `getDefaultKeys` reads the tables by index (`jam_TextEditor.cpp:123-142`) |
| END | `ENDView.cpp:319-323`, `end/cast/bimaps.md:96-97`, `end/Source/config/keys.md:90`, `:92`, `jam_ENAXClientExtensions.cpp:37` |
| EVE | `EVEView.cpp:12`, `:132`, `:199` (Part 1 removes `:132`, `:199`) |

**Keys and commands:** EVE adds commands through `map::CommandId` (`EVEView.cpp:447-455`) and runs them through `registry->callbacks` (`:507-516`). Key rows are data with a `mode` column (`default-eve.md:51`).

**The ENAX seam:** a hosted plugin reads host extensions by id through `getExtension` (`jam_ENAXClientExtensions.cpp:11-18`, `:67-81`). END answers from `Nexus::extensions` (`end/Source/plugins/Nexus.cpp:153-162`, `end/Source/Main.cpp:29-32`). `rules.add (Id::keys, …)` runs in the hosted case only (`jam_ENAXClientExtensions.cpp:7`, `:33-39`), and EVE calls it at `EVEView.cpp:443`. A pane sets a plugin's mode through the `mode` row of its `pluginState` (`EditorView.cpp:199`, `end/Source/plugins/PluginState.h:41`).

**New names:** `registerMode`, sibling of `registerComponent` and `registerStyle`. Slots follow `getKeyInputs`, `getCarets`. The extension id `enaxKeys`, sibling of `enaxKeyPressed` (`jam/generated/jam_Core.cpp:64`).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Registered slots in Registry
**Scope:** `jam/jam_gui/registry/jam_Registry.h`, `.cpp`
**Action:** Add `template <typename...> void registerMode (const juce::Identifier& name, int key, int colourId, KeyInput, Caret, Enter, Paint)`. It calls `map::EditorMode::getInstance()->add (key, name)`, then stores each callable in one `Function::Map<int, …>` of `Registry`, keyed by the mode key.
**Validation:** Ruling 12. `Function::Map` is the established container (`jam_Registry.h:42-53`). A struct for the row is forbidden (CODING, no fake carrier): each slot is one map, as `Registry` holds today.

Two facts bind the name row:
1. The settings validator tests each `mode` token against the table (`jam_PluginEditorLayout.cpp:385-398`), and settings parse before the registry exists (`EVEView.cpp:16`, `jam_PluginEditor.cpp:46-49`, `EVEProcessor.cpp:19`). So the name rows join `map::EditorMode` before the first `getOrCreateSettings`, in `EVEProcessor`'s construction, and `registerMode` takes only the slots.
2. `Bimap::add` asserts a new key and a new value (`jam_Bimap.h:144-145`), and a binary can hold several instances. So the rows join once per binary, on the creation of the shared instance.

### Step 2: TextEditor reads the slots
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:**
1. `keyPressed`: the static canon table answers a canon mode, and the registered map answers any other (`jam_TextEditor.cpp:787-790`).
2. `getCarets`, the `Id::mode` handler and the two paint sites take the same form. The registered callable receives `TextEditor&`, and the paint callable also receives `juce::Graphics&`.
3. Make public what a registered mode needs: `applyToVisibleLinks` (`jam_TextEditor.h:632`), `applyLink` (`:825`), `getCellBounds (int, int)` (already public, `:487`), `setCursor` (`:931`), `getMode`.

**Validation:** Rulings 11, 12. A lookup replaces a branch (MANIFESTO **L**). `juce::Component` API is not widened beyond these names.

### Step 3: Status bar colour by row
**Scope:** `jam/jam_gui/widgets/jam_TextEditorStatusBar.h`, `.cpp`, `eve/Source/textEditor.css`
**Action:** `modeColours` holds the canon rows `normal`, `visual`. A registered mode reads its colour id from the registered map. `openColourId` and `listColourId` leave the enum (`.h:33-34`). EVE names the two colours in its sheet.
**Validation:** Ruling 14: the bar is the same. The label already reads the table (`jam_TextEditorStatusBar.cpp:37`). The two JAM rows leave `jam/cast/style.md:254-255`. EVE adds its two rows in its own `ColourId` table, in the same form.

### Step 4: Canon tables
**Scope:** `jam/cast/gui.md:264-266`, `jam/cast/terminal.md:1893-1896`, the `textEditorKeys` tables, `textEditorModes`, `textEditorLayers`, `jam/cast/spell.md:134-136`
**Action:** `EditorMode` keeps `normal`, `visual`, `terminal`. `TextEditorCommandId` loses `open`, `list`, `listDown`, `listUp`. The three key tables lose the same rows, in order, and the sizes become 24. The `terminal` row of `textEditorModes` (`terminal.md:2065`) loses `open list`.
**Validation:** Ruling 11. The tables keep one row order (`terminal.md:1904`, `:2039`, `:2081`). `eve/Source/layout/parameters.md:73` still reads *"a key of map::EditorMode"*.

### Step 5: EVE registers
**Scope:** `eve/Source/EVEView.h`, `.cpp`, `eve/Source/layout/default-eve.md`, `eve/cast/`
**Action:**
1. `initialiseRegistry` calls `registerMode` for `open`, `list` and `panel`, with keys above the canon. The bodies move from TextEditor as they are: `applyLinkLabel`, `paintLinkLabels`, `moveListEntry`, `paintListEntry` (`jam_TextEditor.cpp:1543-1634`, `:1893-1907`). `linkLabels` moves with them (`:5`).
2. Commands `open`, `list`, `listDown`, `listUp` join `map::CommandId` in `eve/cast`. Their callbacks call `setMode` and the moved bodies.
3. Key rows join `default-eve.md`, with the keys and layers of `terminal.md` today.

**Validation:** Ruling 13, ruling 14. Behaviour does not change here. Part 5 changes LIST. `Id::page` stays in the component state (`jam_TextEditor.cpp:817`), and the entry reaction writes it.

### Step 6: EVE gives rows to END
**Scope:** `jam/jam_plugin_bootstrap/enax/jam_ENAXClientExtensions.cpp`, `end/Source/plugins/Nexus.*`, `end/Source/ui/ENDView.cpp`, `end/cast/bimaps.md`, `end/Source/config/keys.md`, `eve/Source/EVEView.cpp`
**Action:**
1. END deletes `open` and `list`: `ENDView.cpp:321-322`, `bimaps.md:96-97`, `keys.md:90-92`.
2. `Id::keys` rule (`jam_ENAXClientExtensions.cpp:33-39`): it stops naming `open` and `list`. Rows with the `prefix` layer go to the host through the extension `enaxKeys`, read by `getExtension`.
3. END's extension adds each row to its key table and registers one command per row. The command writes the row's mode to the focused pane through `EditorView::setMode` (`ENDView.cpp:325-333`, `EditorView.cpp:199`).

**Validation:** Ruling 15. END names no EVE mode. The rows cross the binary edge as bytes, the form of `PluginState::setRows` (`end/Source/plugins/PluginState.h:37-41`). `Nexus::extensions` maps an id to a callback (`end/Source/plugins/Nexus.h:150-151`), and `registerExtensions` adds each one (`Nexus.cpp:55-65`), so `enaxKeys` is one more row. `end` is a named artifact of this step, by ruling 15.

### Step 7: Hand over, audit, doxygen
**Action:** As Part 1, Step 8. ARCHITECT runs `cast` in `jam`, `eve`, `end`, then builds.

## Acceptance (ARCHITECT builds and tests)

1. Standalone: `` ` `` `o`, `` ` `` `l`, `` ` `` `]`, `v`, `escape` behave as before. The status bar shows each label and colour.
2. Under END: `` ` `` `o`, `` ` `` `f` on the focused pane behave as before.
3. A JAM consumer with no registration (`endless`, Whelmed) builds, with only the canon modes.

## BLESSED Alignment

- **Bound:** EVE owns its modes. JAM owns the canon.
- **Lean:** branches become lookups.
- **Explicit:** a mode is a row with named slots.
- **SSOT:** `map::EditorMode` is the one name table. The mode row is the one truth.
- **Stateless:** the slots are callables and hold no state.
- **Encapsulation:** *"TextEditor faithful to its contract but still scalable and extensible"*.
- **Deterministic:** the same row gives the same reaction.

## Risks / Open Questions

- The LSP references of `map::EditorMode` list EVE, END and JAM, and no `endless` file. The index may not hold `endless`. It has its own key handling (`endless/Source/terminal/action/KeyHandler.h`). ARCHITECT's build of `endless` after Step 4 is the check.
