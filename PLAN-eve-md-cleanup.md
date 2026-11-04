# PLAN: eve.md Cleanup

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-04
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM

## Overview

`eve.md` has 2155 lines. `## text editor` alone has 512 rows, and 480 of them are 240 forwarding pairs.
This plan removes those rows, gives each theme table its own css file, renames the 256-colour family
to `AnsiColours`, and adds one blank ANSI colour so that an application can ask for a transparent
background through the terminal contract.

## Decisions (ARCHITECT, verbatim)

1. Objective: *"this sprint i want to focus on cleaning up eve.md"*.
2. Markdown and mermaid: *"markdown/mermaid will be absorbed from whelmed as eve native markdown/mermaid renderer, so those related colour definition stays."*
3. File name: *"if we can use arbitrary name from manifest, we should just rename it to default-eve.md"*.
4. Indexes 16–255 (2026-11-03, `end/PLAN-end-eve-config.md:45`): *"that is so unnecessary, and should be internal mapping"*; *"0–15 in eve.md"*.
5. Source of the 240 colours: answer *"JAM code (Recommended)"*, then *"should not be harcoded to module. either the SSOT comes from css or table generated static by cast"*.
6. Table layout: answer *"Split all (Recommended)"*: `## colours`, `## terminal`, `## text editor`, `## markdown`, `## mermaid`, `## style`, every value kept. Grounds, `end/PLAN-end-eve-config.md:30`: *"organized like end. not just lazily write all css var with double dash into table."*
7. Columns (`end/PLAN-end-eve-config.md:37`): *"light | dark everywhere"*; an empty `dark` means the same as light.
8. Name: *"can we have most semantically accurate name other than TerminalIndexed? what actually is that? ANSIColours?"*. Reach: answer *"Whole family (Recommended)"*.
9. Blank colour, ARCHITECT's design:
   - *"eve set a colour, an ansi colour that terminal understand"*
   - *"that colour set to blank, 0 alpha, nothing to paint, transparent"*
   - *"that exact colour set as fzf and btop bg colour"*
   - *"jam::TerminalGraphics must true to its contract, no hacking no shortcut, no circus of anything to just deliver per terminal spec"*
10. Blank index: *"16"*.
11. Version (`end/PLAN-end-eve-config.md:22`): *"agents always HACK by bumping version, WHICH IS NOT ALLOWED"*. `version: 0.0.1` stays.

## Language / Framework Constraints

- LANGUAGE.md `C++ / JUCE`: one object is one header and one `.cpp` (`LANGUAGE.md:34`).
- CODING.md: templates and `constexpr` stay in the header. Every non-template body goes to the object's `.cpp`.
- CAROL.md Code Hygiene: no comment and no doxygen before the audit.
- The module header carries the includes. `user-modules-info.md` drives that include region (`jam/carol/SPRINT-LOG.md`, Sprint 95 note in END log line 163).
- `CMakeLists.txt` is generated. The glob `Source/layout/*.css` picks up new css files (`CMakeLists.txt:533`).
- CAROL.md Destructive-Edit Discipline governs every scripted edit: `carol apply --expect N`.

## Dependency & API Inventory

**How the file is made**
- `eve.md` is the default-layout text plus the css tree (`jam_PluginEditorLayout.cpp:599-604`).
- Each css file becomes one sheet, named by the file name (`jam_Css.cpp:2206`). Each sheet becomes one table (`jam_ConfigDocument.cpp:391-403`).
- A sheet named `colours` is already understood: `toDocument` reads it (`jam_ConfigDocument.cpp:394`), `getTypeName` types an alias row `colour` (`:227-232`), and `getColourValue` reads the table (`:121`).
- An alias row resolves to int64 (`jam_ConfigDocument.cpp:127-128`), and `StyleManager::setAppearance` accepts int64 (`jam_StyleManager.cpp:198-199`).
- The dark slot applies only when it exists (`generated/jam_DataStructures.h:124-130`), so an empty dark cell equals light at run time.
- An empty rule `.appearance.DARK {}` keeps a `DARK` child: `addSelectorRule` keeps it (`jam_Css.cpp:1772-1773`), the dot selector adds the child (`:673`, dispatched at `:695`), and `addRecords` appends it (`:2029-2034`). `getColumns` then writes `light | dark` (`jam_ConfigDocument.cpp:310-316`).

**How a colour reaches paint**
- `map::TerminalIndexed` and `map::ColourId` give the id of an index (`jam_AnsiDocument.cpp:700-706`). Ids are `0x1`–`0x100` (`generated/jam_Style.h:1390`, `:1645`).
- Those ids fall through to `laf.findColour` (`jam_Colours.cpp:153-156`). A missing id asserts and gives black (`juce_LookAndFeel.cpp:94-95`).
- A background at alpha 0 fills nothing: `Run::paint` (`jam_GlyphArrangement.cpp:589-592`), *"transparent = none"* (`jam_GlyphArrangement.h:136`).

**How a LookAndFeel takes a sheet**
- `applyAppearance` applies sheet `style`, then calls `laf.setAppearance` (`jam_ViewManager.cpp:322-328`).
- `Registry::registerStyle` makes the object and sets its sheet (`jam_Registry.h:169-176`).
- Nearest sibling for a second sheet: `StyleMarkdown::setAppearance` (`jam_StyleMarkdown.cpp:26-31`), which applies one sheet and then `StyleCustom::setAppearance`. Pattern for a named sheet: `StyleMermaid::getSheet` (`jam_StyleMermaid.cpp:26-31`).
- `jam_terminal` depends on `jam_style` (`jam_terminal.h:17`). No `jam_terminal` type derives from `StyleCustom` today.

**Cast**
- `jam::LookupTable<int, uint32_t, 256>` is the alias `@colourSpace` (`spell.md:71`). `operator[]` reads it (`jam_LookupTable.h:157`).
- Existing family for default colour values: `defaultColorRegisters`, generated by `spell.md:2265-2273`.
- An extra table column is ignored by a block that does not list it: `## PluginWrapper` `format` (`plugin_bootstrap.md:148`, `spell.md:1398-1404`).

**Name facts**
- `Ansi` spelling: `AnsiDocument`, `## AnsiTokenType` (`cast/terminal.md:67`).
- `Id::terminal` is in `cast/gui.md:71`. `jam_gui` depends on `jam_terminal` (`jam_gui.h:17`), so `jam_terminal` cannot read it there.

**No consumer breaks**
- Only EVE embeds `textEditor.css` (`CMakeLists.txt:545`); the jam copy has class `provision` (`user-modules-info.md:1357`), and a provision row is never written (`cast/Source/HELP.md:89`).
- `style.css` has 0 references to `terminalIndexed`.
- EVE builds no `StyleMarkdown` or `StyleMermaid`. The one constructor call is `cast/Source/Help.cpp:12`. So the split of markdown and mermaid rows changes nothing that EVE paints today.
- END reads no EVE sheet and no `eve.md`.
- `.appearance.DARK` of `style.css` equals `.appearance` row for row (`diff`: only `:719` differs, a font row absent from DARK). `.appearance.DARK` of `textEditor.css` equals `:264-269` (`diff`: empty).

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor
runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Rename the colour family in cast
**Scope:** `jam/cast/terminal.md`, `jam/cast/style.md`, `jam/cast/spell.md`, `kuassa/user_modules/cast/style.md`.
**Action:**
1. `cast/terminal.md:122`: the heading becomes `## AnsiColours`. Every row: `name` `indexedN` becomes `colourN`; `value` `` `terminal indexedN colour id` `` becomes `` `ansi colourN colour id` ``. The `@brief` follows.
2. `cast/style.md:577-832`: `terminalIndexedNColourId` becomes `ansiColourNColourId`, and the value becomes `` `ansi colourN colour id` ``. The keys `0x1`–`0x100` do not change.
3. `kuassa/user_modules/cast/style.md`: the same transformation, so that the kernel copy stays identical.
4. `cast/spell.md:1784-1791`: `TerminalIndexed` becomes `AnsiColours`, in the list rows, `- name:` and `- [description]:`.
**Validation:** `carol apply --expect` counts reconcile: 256 rows in each `style.md`. No `erminalIndexed`, `erminal indexed` or `indexedN` row name remains in `jam/cast/*.md` or `kuassa/user_modules/cast/style.md`. NAMES Rule 5, Rule 7, Rule 8.

### Step 2: Add the values to `## AnsiColours`
**Scope:** `jam/cast/terminal.md`, `jam/cast/spell.md`, `jam/cast/gui.md`.
**Action:**
1. Add a column `colour` to `## AnsiColours`. Each row holds its `0xffrrggbb` value, moved verbatim from `jam/resources/textEditor.css:2-257`.
2. Add a block to `spell.md`, a copy of `spell.md:2265-2273` with `@terminal:AnsiColours:key`, `@terminal:AnsiColours:colour`, `- type: @colourSpace` and `- name: defaultAnsiColours`. The `- [description]:` states that the table holds the default value of each ANSI colour.
3. Move the `@id | terminal` row from `cast/gui.md:71` to the `## identifiers` table of `cast/terminal.md`.
**Validation:** 256 `colour` cells. Values equal `textEditor.css:2-257`. The generated `defaultAnsiColours` matches the shape of `defaultColorRegisters` (`generated/jam_Terminal.h:2460`). `defaultAnsiColours` joins the `defaultColorRegisters` family (NAMES Rule 5).

### Step 3: Follow the rename in JAM code
**Scope:** `jam/jam_terminal/document/jam_AnsiDocument.h`, `.cpp`.
**Action:** `jam_AnsiDocument.cpp:701`: `map::TerminalIndexed` becomes `map::AnsiColours`, and the local `indexedColours` becomes `ansiColours` (`:705`, `:716`). The doc line `jam_AnsiDocument.h:483` names `map::AnsiColours`.
**Validation:** Read each changed line. `getIndexedColourId` and `getIndexedColour` keep their names: the SGR mode is `map::ColorMode::indexed256`.

### Step 4: `jam::StyleTerminal`
**Scope:** new `jam/jam_terminal/style/jam_StyleTerminal.h`, `.cpp`; `jam/jam_terminal/jam_terminal.h`, `.cpp`.
**Action:**
1. `class StyleTerminal : public StyleCustom`, in `jam_terminal`.
2. The constructor tells the LookAndFeel every default colour:
   `for` each `[index, name]` of `map::AnsiColours`, `setColour (colourId->get (name), juce::Colour { map::defaultAnsiColours[index] })`.
3. `setAppearance (juce::StringRef lightOrDark) override` applies the `terminal` sheet, then the component's own sheet:
   `StyleManager::getInstance()->setAppearance (terminalSheet, *this, lightOrDark);` then `StyleCustom::setAppearance (lightOrDark);`. `terminalSheet` is a function-local `static const juce::Identifier` of `Id::toType (Id::terminal)`, the shape of `jam_StyleTheme.cpp:5-10`.
4. [AMENDED by audit findings 9, 10: CODING.md narrowest scope, MANIFESTO S] No members. `AnsiDocument::getIndexedColourId` becomes public, and the constructor loops over `map::defaultAnsiColours.capacity`.
5. Add the two includes to `jam_terminal.h` and `jam_terminal.cpp`, in the shape of the sibling lines. [CORRECTED by read: `user-modules-info.md` holds only module-level rows for `jam_terminal` (`:96`, `:156`), so no submodule row exists to follow.]
**Validation:** One header, one `.cpp`. No guard, no flag, no state. Project code gives only the difference: the `terminal` sheet overrides the default (CODING.md Owner first). `StyleTerminal` joins the `Style*` family (NAMES Rule 5). `jam_terminal` includes nothing from `jam_gui`.

### Step 5: Clean `jam/resources/textEditor.css`
**Scope:** `jam/resources/textEditor.css`.
**Action:** Delete `--terminalIndexed0` … `--terminalIndexed255` (`:2-257`) and `--Terminal--indexed0ColourId` … `255` (`:279-534`).
**Validation:** `carol apply --expect 512`. The `--endCode*` palette and the 6 component rows of each appearance block stay.

### Step 6: Split the EVE css
**Scope:** `eve/Source/layout/style.css`, `eve/Source/textEditor.css`; new `eve/Source/layout/colours.css`, `terminal.css`, `markdown.css`, `mermaid.css`.
**Action:**
1. `colours.css`: one `:root` with the 11 names of `style.css:2-12` and the 10 names of `textEditor.css:2-10`, `:251`. Delete both source lists. The 240 `--terminalIndexedN` lines of `textEditor.css:11-250` are deleted, not moved.
2. `terminal.css`: `.appearance.DARK {}`, then `.appearance` with 17 rows: `--Ansi--colour0ColourId` … `--Ansi--colour15ColourId` with the `var()` values of `textEditor.css:270-285`, and `--Ansi--colour16ColourId: var(--blank);`.
3. `textEditor.css`: `.appearance.DARK {}`, then `.appearance` with the 6 rows of `:264-269`. The 256 `--Terminal--` rows are deleted, with the 0–15 values already in `terminal.css`.
4. `markdown.css`: `.appearance.DARK {}`, `.appearance` with the 36 `--Markdown--` rows, then the whole `.metrics` block of `style.css:805-858`.
5. `mermaid.css`: `.appearance.DARK {}`, `.appearance` with the 395 `--MermaidDiagram--` rows.
6. `style.css`: `.appearance.DARK {}`, `.appearance` with the 3 `--ResizableWindow--` and `--ScrollBar--` rows, then the `@font-face` rules of `:860-931`.
**Validation:** Row counts: 36 + 395 + 3 = 434, the count of `style.css:369-802` before the edit. Every `var()` target of a colour row is in `colours.css`. A sorted line comparison shows every kept row verbatim. The only rows without a source are `--Ansi--colour16ColourId` and the renamed keys. 350 DARK rows go, proven identical by the `diff` in the inventory. Decision 2 holds: no markdown or mermaid value changes.

### Step 7: Wire EVE, rename the default file
**Scope:** `eve/Source/EVEView.cpp`, `eve/Source/layout/ViewLayout.md`, `eve/cast/files.md`.
**Action:**
1. `EVEView.cpp:171`: `r.registerStyle<jam::StyleTerminal> (Id::textEditor);`.
2. Rename `Source/layout/ViewLayout.md` to `Source/layout/default-eve.md`.
3. `cast/files.md:23`: the value becomes `default-eve.md`. The constant `files::viewLayout` keeps its name.
4. `version: 0.0.1` does not change.
**Validation:** `cast/files.md` is the only hand-edited holder of the name. `Source/generated/Files.cpp` changes through cast only.

### Step 8: Documents
**Scope:** `eve/SPEC.md`, `eve/README.md`, `eve/CLAUDE.md`.
**Action:**
1. `SPEC.md:543-551`: the default is `default-eve.md` plus the binary css. One css file gives one table. `## colours` holds the palette. `## terminal` holds ANSI 0–15 and the blank colour 16. Indexes 16–255 default from the cast table `AnsiColours`, and a `## terminal` row overrides a default.
2. `README.md:73`: follow the same text.
3. `CLAUDE.md:46`: name the new file set.
**Validation:** Every sentence cites what the code does. Each behaviour read against the file:line in the inventory.

### Step 9: Generate and build
**Scope:** jam, kuassa/user_modules, eve.
**Action:** Run, in order: `cast cast/spell.md` in `jam`; `cast cast/spell.md` in `kuassa/user_modules`; `cast cast/spell.md --debug` in `eve`.
**Validation:** `EXIT=0`, 0 errors, 0 warnings. Read `generated/jam_Terminal.h`: `struct AnsiColours`, `defaultAnsiColours`. Read `generated/jam_Style.h`: `ansiColour0ColourId = 0x1`. A compile error on an old name goes back to the owning step.

### Step 10: Audit, doxygen, log
**Action:** @Auditor sweeps once. COUNSELOR resolves every validated finding. @Pathfinder writes the doxygen pass over each changed header, with the exact targets from COUNSELOR. COUNSELOR logs the sprint per `/log` and writes the commit messages in chat.
**Validation:** CAROL.md DCF §5. Every finding is resolved or rejected with its read.

## ARCHITECT's test after the sprint

1. Delete `~/.config/end/eve.md`. The version stays `0.0.1`, so the version gate does not rewrite the old file (`jam_ConfigDocument.cpp:405-417`). The old file holds tables that the new default does not have, and those show as errors in the overlay.
2. EVE writes the new `eve.md` with `## colours`, `## terminal`, `## text editor`, `## markdown`, `## mermaid`, `## style`, `## fonts`.
3. `## terminal` shows 17 rows. `printf '\e[48;5;16m text \e[0m'` shows no fill. `\e[30m` text stays `BUNKER`.

**The app side is outside this repository, and outside COUNSELOR's role (MACHINIST owns `~/.config`).**
- `~/.config/zsh/zshrc:98`: `bg:#090D12` becomes `bg:16` (`fzf.1:316`, `:333`).
- btop: `color_theme` names a `.theme` file whose background is `#000000`, and `truecolor = false` (`btop.conf:5`, `:10`). The btop source is not on disk, so the first run shows which index btop sends.

## BLESSED Alignment

- **B:** the LookAndFeel owns its colours. `StyleTerminal` is its owner's creation (`ViewManager.cpp:167-170`).
- **L:** 480 rows go. YAGNI: no setting exists for a colour nobody changes.
- **E:** a colour rule stays one rule: id, `findColour`, paint. No new path in `jam::TerminalGraphics`, no guard.
- **S (SSOT):** cast owns the 256 default values. Each palette name is in one file. The forwarders were a second copy of each value.
- **S (Stateless):** `StyleTerminal` holds no state beyond the LookAndFeel colours that it sets at creation.
- **E (Encapsulation):** `jam_terminal` owns its colours. The layer order does not change.
- **D:** the same index gives the same colour, at creation and after each appearance change.
- LANGUAGE.md override: one object, one translation unit, in Step 4.

## Risks / Open Questions

None. Every decision above is ARCHITECT's, quoted.
