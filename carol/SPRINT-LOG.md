# SPRINT-LOG.md

**Project:** EVE — Embeddable Viewport Endpoint
**Repository:** /Users/jreng/Documents/Poems/dev/eve
**Started:** 2026-08-30

**Purpose:** Long-term context memory across sessions. Tracks completed work, debts paid, and debts deferred to the inter-sprint ledger (`DEBT.md`). Written by PRIMARY agents only when ARCHITECT explicitly requests.

---

## ⚠️ CRITICAL RULES

**AGENTS BUILD CODE FOR ARCHITECT TO TEST**
- Agents build/modify code ONLY when ARCHITECT explicitly requests
- ARCHITECT tests and provides feedback
- Agents wait for ARCHITECT approval before proceeding

**AGENTS NEVER RUN GIT COMMANDS**
- Write code changes without running git commands
- Agent runs git ONLY when user explicitly requests
- **When committing:** Always stage ALL changes with `git add -A` before commit

**SPRINT-LOG WRITTEN BY PRIMARY AGENTS ONLY**
- **COUNSELOR** writes to SPRINT-LOG
- Only when user explicitly says: `"log sprint"`
- No intermediate summary files
- Latest sprint at top, keep last 5 entries

**⚠️ NEVER REMOVE THESE RULES**

---

<!-- SPRINT HISTORY STARTS BELOW -->
<!-- Latest sprint at top, oldest at bottom -->
<!-- Keep last 5 sprints, rotate older to git history -->

## SPRINT HISTORY

## Sprint: Keyboard Focus Follows Pane Focus; MatrixComponent Owns the Pane Focus Commands ✅

**Date:** 2026-11-04
**Duration:** one session, after the Shift+Enter log. The start and end times are not recorded.
**Scope (ARCHITECT):** *"keyboard focus is not following pane focus. this is contract violations. i need to click the pane although it already focused (the outline shows) after creating new split. all must be event driven, not manually manage the state maching"*; *"SSOT, navigating with prefix + hjkl, clicking, moving focus after split, etc must be exact ONE commandId, feasible?"*; *"then the design is wrong. if focus management is MatrixComponent responsibility, then it's MatrixComponent commandId, END only tell"*.

### Agents Participated
- COUNSELOR: reads of the focus path, the plugin load order, the JUCE command target chain, the cast module table; validation of each changed line; the module table row move; this log.
- Engineer (sonnet): `EditorView` change, the move of the commands to `MatrixComponent`.
- Auditor: not run. ARCHITECT: *"do not audit until i explicitly say so"*.

### Files Modified (12 total)
- `end/Source/ui/EditorView.cpp:163-183` — the plugin-loaded event writes the row's `Id::focus` into the plugin's `focus` parameter, through the one write site that `Id::focus` uses. No new state and no helper.
- `jam/cast/gui.md:55-58`, `:226` — `Id` rows `paneDown`, `paneLeft`, `paneRight`, `paneUp`; new `MatrixCommandId` table, ids from `0x4900001`.
- `jam/cast/spell.md:950-961` — `map::MatrixCommandId` in `jam_Gui.h`, a copy of the `TextEditorCommandId` row.
- `jam/user-modules-info.md:988` — `commands/jam_CommandManager.h` moves above `layout/jam_OwnedComponent.h`. This table gives the include order of the generated `jam_gui.h`.
- `jam/jam_gui/layout/jam_MatrixComponent.h:18-19`, `:100-106`, `:141`, `:220-221` — `juce::ApplicationCommandTarget` base, its four overrides, `focusPane`, `matrixCommandId`, `commandManager`.
- `jam/jam_gui/layout/jam_MatrixComponent.cpp:10`, `:16`, `:341-379` — both constructors register the commands; `perform` looks up the direction and calls `focusPane`, which ends in `setFocusedChild`.
- `end/cast/bimaps.md`, `end/cast/identifiers.md` — the four pane focus rows are deleted.
- `end/Source/ui/ENDView.h`, `end/Source/ui/ENDView.cpp:31`, `:70-73` — `registerFocusActions` is deleted; `getNextCommandTarget` returns `getActiveTabView()`; `setKeyPresses` runs after `newSession`.
- `end/Source/ui/TabView.h`, `end/Source/ui/TabView.cpp` — `focusPane` is deleted.

### Alignment Check
- [x] BLESSED principles followed: `setFocusedChild` stays the one focus write; the owner of the panes owns the commands; END tells through the target chain.
- [x] NAMES.md adhered: `MatrixCommandId` joins `TextEditorCommandId`; `focusDirections` keeps its existing name as a dispatch noun (Rule 8).
- [x] MANIFESTO.md principles applied
- [ ] Not validated by a clean compile. The last build gave one error, fixed after that build, and no build ran after the fix.
- [ ] No Auditor run, on ARCHITECT's word.

### Problems Solved
- **A new split had the outline but no keyboard focus.** `split` runs `childAdded`, then `setFocusedChild` (`jam_MatrixComponent.cpp:342`), which writes the row's `Id::focus` (`jam_OwnerComponent.cpp:111`). At that time `editor` is null, so the old write site dropped the event. The plugin loads asynchronously (`Session.cpp:91-93`), and `Id::name` then builds the editor (`Session.cpp:115`). No focus write followed, so EVE's `focus` parameter stayed 0. A second `setFocusedChild` would do nothing, because `ValueTree` sends no event for an unchanged value (`juce_ValueTree.cpp:144-145`).
- **Command ownership.** One `commandId` for every focus path is not possible: a command carries no argument (`juce_ApplicationCommandManager.h:219`, `ENDView.cpp` `perform`), and each key row maps to one command (`jam_CommandManager.cpp:28`). ARCHITECT's design holds: `MatrixComponent` owns four commands, and click and split were already inside it (`jam_OwnerComponent.cpp:94`, `jam_MatrixComponent.cpp:342`).
- **`CommandManager` unknown at `jam_MatrixComponent.h:221`.** The include order of `jam_gui.h` comes from `jam/user-modules-info.md`, so `cast` overwrote the hand edit.
- **Mistakes owned.** A code edit before ARCHITECT's *"discuss first"*. A layer argument against a JAM command id, which `TextEditorCommandId` refuted. A plan step that edited generated `jam_gui.h` when the source is `user-modules-info.md`.

### State for Continuation
- Build `jam` through `cast` first. Check that `jam_gui.h` gives `commands/jam_CommandManager.h` above `layout/jam_OwnedComponent.h`, and that `map::MatrixCommandId` has its four enum names.
- ARCHITECT tests these: a new split takes keys with no click; the first pane of a new tab takes keys; prefix + hjkl moves focus, and keys follow; the same after a tab switch.
- `setKeyPresses` now runs inside the `callAsync` of the `ENDView` constructor (`ENDView.cpp:31`). A key sheet row of a pane command is skipped if it runs before a `TabView` exists (`jam_CommandManager.cpp:14`). `ENDView.cpp:445` sets it again on a config event.
- The command chain has a loop: `ENDView` goes to the active `TabView`, and `TabView` goes to its parent target, `ENDView`. `getTargetForCommand` asserts on the loop only for a command that no target owns (`juce_ApplicationCommandTarget.cpp:111`). Every registered command has an owner, so this path does not run.
- The new `MatrixComponent` declarations have no doxygen. They wait for the comment pass.
- The state of the three previous checks in the Shift+Enter log is partly known: `cast` ran, and the build reached `jam_MatrixComponent.h`. Shift+Enter and the dev config root are not reported as tested.

### Debts Paid
- None

### Debts Deferred
- None

## Sprint: Shift+Enter Is a Terminal Key Row; Debug END and EVE Use the Dev Config Root ✅

**Date:** 2026-11-04
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"why i cant shift+enter to create new line in this claude code?"*; *"we need to shift the config to ~/.config/dev/ so this running instance wont conflict with we're working on check @DEBT.md"*; *"this should not be hardcoded to TextEditor, as plain TextEditor newline should be just simply enter. shift+enter newline only for TERMINAL mode"*; *"why harcodes? isnt SSOT? fix it"*; *"dont change the comment. config/dev/ only always temporary"*; *"after plan and execution, do not audit until i explicitly say so"*.

### Agents Participated
- COUNSELOR: reads of the key encoders, the key sheet, the host key hook, the Kitty and Ghostty sources, the nvim doc, the Claude Code binary, the cast read path; validation of each changed line; this log.
- Pathfinder (haiku): plan and config root read, Claude Code binary commands.
- Engineer (sonnet): the two implementations. The first file set had eight files, the second had three.
- Auditor: not run. ARCHITECT: *"do not audit until i explicitly say so"*.

### Files Modified (11 total)
- `jam/cast/terminal.md:1851`, `:1914`, `:1970`, `:2008`, `:2046` — one `newline` row in `TextEditorCommandId`, `textEditorKeys mac`, `textEditorKeys windows`, `textEditorModes`, `textEditorLayers`: key `shift+return`, mode `terminal`. The tables are positional, so each one takes a row.
- `jam/cast/gui.md:51` — `Id::newline`.
- `jam/cast/spell.md:134-136` — the three key lookup tables grow from 23 to 24.
- `jam/jam_gui/widgets/jam_TextEditor.cpp:464-468` — `Id::newline` action beside `paste`. It writes `Chars::newline` through `terminalInput`.
- `end/project-info.md:369`, `eve/project-info.md:379` — `configDirectory` DEFINES row. `CAST_CONFIG_DIRECTORY` is `.config/dev` for Debug and `.config/end` otherwise.
- `end/Source/config/ConfigDirectory.h:75` — `configDirectoryName` reads the define.
- `eve/cast/files.md:31` — `markdown settings directory` reads the define. The cell has no format, so the macro stays raw (`cast/Source/Model.cpp:975-985`). The value column of the table is wider.
- `jam/jam_clap/format/jam_ClapPluginFormat.h:37`, `:135`, `jam/jam_clap/format/jam_ClapPluginFormat.cpp:161`, `:167`, `:257-266` — the `inHousePath` literal is gone. The host passes `hostPluginsDirectory` through the constructor.
- `end/Source/plugins/Nexus.cpp:80` — END passes `ConfigDirectory::Config::getPath (Id::plugins).getFullPathName()`.

### Alignment Check
- [x] BLESSED principles followed: one root switch in the `DEFINES` lane, each root path read from one owner, a key as a data row.
- [x] NAMES.md adhered: `hostPluginsDirectory` joins the `host*` family; `CAST_CONFIG_DIRECTORY` joins `CAST_PLUGINS_HASH`.
- [x] MANIFESTO.md principles applied
- [ ] Not validated by a compile. No cast and no build ran.
- [ ] No Auditor run, on ARCHITECT's word.

### Problems Solved
- **Shift+Enter submitted.** With no key row, terminal mode called `TerminalKeyboard::map` with `keyboardFlags` at 0 (`jam_TextEditor.cpp:511-516`), which returns `carriageReturn` for Return (`jam_TerminalKeyboard.cpp:172-183`, `:218`). Claude Code reads `0xd` as `return` and `0xa` as `enter`, a line break (binary lines 51593976-51593978, 51599945). Old endless wrote `\n` from a `newline = "shift+return"` binding (`default_keys.lua:102`, `MainComponentActions.cpp:65`). The new row has the same shape, and only terminal mode matches it (`jam_CommandManager.cpp:65-70`).
- **Convention.** No read shows a standard for this key. Kitty legacy mode sends `0xd` for Shift+Enter (`keyboard-protocol.rst:530`). Ghostty sends `\x1b[27;2;13~` (`function_keys.zig:199`). nvim negotiates `CSI > 3 u` or `CSI > 4 ; 2 m` (`tui.txt:185-195`). Claude Code's own setup binds ESC CR in VS Code and Zed (binary lines 51600164-51600174).
- **Ctrl+J gave nothing, Ctrl+Shift+J gave a new line.** The peer offers each key to END first (`jam/patch/juce-key-pressed-hook.patch:21-23`, `Nexus.cpp:7-13`, `ENDView.cpp:57-59`). `~/.config/end/keys.md:75` binds `ctrl+j` to `join down`. `ctrl+shift+j` has no row, so EVE's `mapCtrl` sends `0xa` (`jam_TerminalKeyboard.cpp:75-78`).
- **Mistakes owned.** A first Pathfinder prompt read far past the question. A claim that Ctrl+J gives a new line came from `mapCtrl` alone. A claim that Shift+Enter cannot be a key row missed `jam_PluginEditor.cpp:328`. Each one was corrected in the chat.

### State for Continuation
- Nothing is cast or compiled. The generated files and `CMakeLists.txt` of `jam`, `end` and `eve` are stale until the next `cast`. Check these three things first:
  1. `eve/Source/generated/Files.cpp` gives `juce::String::fromUTF8 (JUCE_STRINGIFY (CAST_CONFIG_DIRECTORY))`.
  2. `CMakeLists.txt` of `end` and `eve` holds the `configDirectory` row with its `$<IF:...>` quoted.
  3. `JUCE_STRINGIFY` gives `.config/dev` in Debug and `.config/end` in Release.
- ARCHITECT tests Shift+Enter in Claude Code in hosted EVE, and in a plain TextEditor mode. The second must not change.
- A Debug END makes `~/.config/dev/`. ARCHITECT: *"config/dev/ only always temporary"*.
- Comments stay stale on ARCHITECT's word *"dont change the comment"*: `ConfigDirectory.h:46`, `:52`, `Main.cpp:252`, `Main.h:28`, `jam_ClapPluginFormat.h:108`. `jam_ClapPluginFormat.h:28-30` has no `@param` for the new parameter.
- The negotiated layer is not built. `TerminalKeyboard` encodes `CSI 13;2u` (`:172-179`), but `jam_TextEditor.cpp:516` passes no flags and `applyCsiSequence` drops `CSI >`. If it is built, the key row wins over it, because `invokeKey` runs first (`:511`). This belongs to `DEBT-20261103T233912`.
- `DEBT-20261103T235332` stays in the ledger. This sprint did not touch it. ARCHITECT: *"i have already backup old binary to endless repo"*.
- `eve/CLAUDE.md:59` lists `HANDOFF.md` as present, and `:42-44` list three `Source/EVEView*.cpp` files. None of these four files exists.

### Debts Paid
- `DEBT-20261103T235331` — a Debug END and EVE use `~/.config/dev/`, a Release build uses `~/.config/end/`, with one switch in the `DEFINES` lane of each project.

### Debts Deferred
- None

## Sprint: Closing with fzf Open No Longer Hangs; Version Reset; fzf and btop on the Transparent Contract ✅

**Date:** 2026-11-04
**Duration:** one session, after the eve.md cleanup log. The start and end times are not recorded.
**Scope (ARCHITECT):** *"closing app while fzf opened, crash"*; *"closing END while EVE terminal opened fzf still HANGS"*; *"perhaps we need better clean up, when Processor dtor"*; *"eve version should be reset to 0.0.1"*; *"also fix btop also not transparent"*; *"delete zip inside Source/plugins/"*; *"add missing handlers for claude code to eve DEBT.md"*; *"9/10 add to DEBT"*.

### Agents Participated
- COUNSELOR: reads of the close path, the triage, `project-info.md`, `CLAUDE.md`, `end/.gitignore`, `zshrc`, `btop.conf`, `DEBT.md`, the one-line race fix, this log.
- Pathfinder (haiku): `sample` of the hung END, git history reads.
- Engineer (sonnet): the `close()` reorder, the zip delete.

### Files Modified (7 total)
- `jam/jam_terminal/tty/native/jam_TerminalTTY_posix.cpp:281-311` — `close()` kills the child, closes the read source and the master, and only then waits for the child. Nothing reads the master after `device.stop()`, so the old wait on an open master never returned.
- `jam/jam_terminal/tty/native/jam_TerminalTTY_posix.cpp:232` — the read handler also resumes a cancelled source, so a 0-byte event cannot leave the cancel wait blocked.
- `eve/project-info.md:38`, `eve/CLAUDE.md:11` — version `0.0.1`.
- `end/.gitignore:36` — the unpacked plugin folder follows the version.
- `~/.config/zsh/zshrc:98` — fzf `bg:16`, the blank index. `~/.config/btop/btop.conf:8` — `theme_background = false`.
- `eve/DEBT.md` — `DEBT-20261103T233912`, `DEBT-20261103T235331`, `DEBT-20261103T235332`.
- Deleted: `end/Source/plugins/EVE v0.0.1 macOS.zip` (END's toolchain builds it again, `end/project-info.md:106`).

### Alignment Check
- [x] BLESSED principles followed — the TTY owns its cleanup order; no Processor destructor code, per CODING.md RAII rule.
- [x] NAMES.md adhered — `isChildRunning` joins the `is` family.
- [x] MANIFESTO.md principles applied
- [ ] Not audited: this work came after the sprint's one Auditor pass.

### Problems Solved
- **Hang on close with fzf open.** `sample` of the hung END: 806 of 806 main-thread samples in `__wait4` under `close()`, through `~EVETerminalProcessor`, with the shell in state `U`. ARCHITECT named the cleanup; the fix reorders it. ARCHITECT confirmed: *"no more hangs with fzf"*.
- **Why the blank colour had no effect.** fzf sent `bg:#090D12`, an opaque true colour; `bg:16` uses index 16. btop's theme colours have no index form (`nord.theme:4`), and its own files state that a transparent background needs an empty background (`nord.theme:12`), so `btop.conf:7-8` is its route.
- **Mistakes owned.** A first race fix at `:232` from reading alone, before a stack existed; it was not the cause of the hang. A stale claim that EVE must build before END, when END's toolchain builds it (`end/project-info.md:106`). Three hand-offs of `zshrc:98` that ARCHITECT's ruling already covered.

### State for Continuation
- ARCHITECT has not yet reported fzf and btop as transparent after these edits. Test in a new shell and a restarted btop. `printf '\e[48;5;16m text \e[0m\n'` tests EVE alone.
- `btop` differs from the ruling *"that exact colour set as fzf and btop bg colour"*, for the reason above. Read `btop.conf:7-8`.
- `doxygen` crashed at 06:44:21 with a bus error (`~/Library/Logs/DiagnosticReports/doxygen-2026-11-04-064421.ips:46`). `docs/xml` is stale for `getIndexedColourId`. Check the next regeneration.
- `kuassa` C++ is regenerated and not compiled.
- `eve/CLAUDE.md:21`, `:25` name an old plan and a paid debt.
- nvim gaps are in no ledger: cursor shape `CSI Ps SP q`, the `CSI ?u` and `CSI >` key modes, `?1004` focus events (`jam_TerminalProcessor.cpp:288`, `:299`, `jam/cast/terminal.md:1286-1297`).

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261103T235332` — move old `END.app` into the endless repo before the first Release install (ARCHITECT: *"9/10 add to DEBT"*).
- `DEBT-20261103T235331` — route dev END and EVE to `~/.config/dev/` (ARCHITECT: *"9/10 add to DEBT"*).
- `DEBT-20261103T233912` — live decoder handlers for Claude Code (ARCHITECT: *"add missing handlers for claude code to eve DEBT.md"*).

---

## Sprint: eve.md Cleanup — One Table per Css File, AnsiColours in Cast, Index 16 Is Blank ✅

**Date:** 2026-11-04
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** `PLAN-eve-md-cleanup.md`. *"this sprint i want to focus on cleaning up eve.md"*; *"markdown/mermaid will be absorbed from whelmed as eve native markdown/mermaid renderer, so those related colour definition stays."*; *"we should just rename it to default-eve.md"*; *"should not be harcoded to module. either the SSOT comes from css or table generated static by cast"*; *"ANSIColours?"*; blank colour design: *"eve set a colour, an ansi colour that terminal understand"*, *"that colour set to blank, 0 alpha, nothing to paint, transparent"*, *"that exact colour set as fzf and btop bg colour"*, *"jam::TerminalGraphics must true to its contract, no hacking no shortcut, no circus of anything to just deliver per terminal spec"*; index *"16"*.

### Agents Participated
- COUNSELOR: PLAN, every validation read, the triage of the audit, `SPEC.md`, `README.md`, `CLAUDE.md`, one line in `jam_StyleTerminal.cpp`, this log.
- Pathfinder (haiku): discovery reads, cast and rename inventory, style and config read path, the doxygen pass.
- Engineer (sonnet): cast tables, `StyleTerminal`, css split, audit fixes, every cast run.
- Auditor (opus): one pass, 14 findings.

### Files Modified (29 total, generated output excluded)
**jam:**
- `cast/terminal.md:123` — `## TerminalIndexed` is `## AnsiColours`; rows `colourN`, `ansi colourN colour id`; new `colour` column with the default ARGB word of all 256 indexes; `Id::terminal` row moved in from `cast/gui.md`, because `jam_terminal` sits below `jam_gui`.
- `cast/style.md:577-832` — `ansiColourNColourId`, `ansi colourN colour id`; keys `0x1`–`0x100` unchanged.
- `cast/spell.md:1784-1789`, `:2276-2286` — `AnsiColours` bimap; new `defaultAnsiColours` lookup table.
- `jam_terminal/style/jam_StyleTerminal.h`, `.cpp` (new) — sets all 256 defaults at construction; `setAppearance` applies the `terminal` sheet, then the component sheet. Shape of `StyleMarkdown::setAppearance`.
- `jam_terminal/jam_terminal.h:39`, `:114-115`, `jam_terminal.cpp:54-55` — includes and the order prose.
- `jam_terminal/document/jam_AnsiDocument.h:221-229` — `getIndexedColourId` public; `jam_AnsiDocument.cpp:701-716` — `map::AnsiColours`.
- `jam_markdown/document/jam_ConfigDocument.cpp:203-204` — a property table with a `dark` column always holds a DARK child; `:422-431` `hasReference`; `jam_ConfigDocument.h:96`, `:215-218` doc.
- `jam_web/css/jam_Css.cpp:204`, `:1966-1967` — `compare`, not `==` or `!=` on strings.
- `resources/textEditor.css` — 512 indexed rows deleted.

**kuassa:** `user_modules/cast/style.md` — same 256 rows as jam.

**eve:**
- `Source/layout/colours.css`, `terminal.css`, `markdown.css`, `mermaid.css` (new); `Source/layout/style.css`, `Source/textEditor.css` — one css file for one table; 21 palette names; 17 `terminal` rows; 480 forwarding rows and 350 duplicate dark rows gone.
- `Source/layout/ViewLayout.md` renamed `default-eve.md`; `cast/files.md:23`.
- `Source/EVEView.cpp:171` — `registerStyle<jam::StyleTerminal>`.
- `SPEC.md` Config and ANSI colours; `README.md:73`; `CLAUDE.md:46`; `PLAN-eve-md-cleanup.md`.

### Alignment Check
- [x] BLESSED principles followed — cast is the one source of the 256 defaults; no new path in `jam::TerminalGraphics`.
- [x] NAMES.md adhered — `AnsiColours` is ARCHITECT's word; `defaultAnsiColours`, `StyleTerminal`, `hasReference` join existing families.
- [x] MANIFESTO.md principles applied
- [ ] Doxygen not regenerated; ARCHITECT regenerates and reads the warning count. `docs/xml` is stale for `getIndexedColourId`.

### Problems Solved
- **Why eve.md had 2155 lines.** `getDefaultSettings` writes the whole css tree. `textEditor.css` held 240 hex rows and 240 forwarders to them, because `addAppearance` stores only a `var()` name. All 240 are fixed values with one reader.
- **Layout.** One sheet is one css file (`jam_Css.cpp:2206`), so the split needed no translation layer. `colours.css` uses the path that `toDocument`, `getTypeName` and `getColourValue` already had.
- **Transparent background with black intact.** ARCHITECT's design: EVE sets one ANSI index to blank, and the application asks for that index. Paint already fills nothing at alpha 0 (`jam_GlyphArrangement.h:136`). No code path changed.
- **Audit.** 14 findings. Resolved: 1 (regression: a disk `dark` cell had no effect), 2, 4, 5, 6, 7, 8, 9, 10, 13, 14 `ConfigDocument`, 14 `Css` strings. Rejected: 3 (the PLAN names a set, not an order), 11 (`setMonoFont` replaces a complete state), 12 (Decision 6 and `end/PLAN-end-eve-config.md:243`), 14 `Css:1764-1767` (a copy in creation), 14 `end/CLAUDE.md:44` (no CONTRACT clause).
- **Mistakes owned.** A question that `end/PLAN-end-eve-config.md:45` had answered; `defaultColorRegisters` offered as a sibling with no read of its semantics; an `addGlyph` rule against EVE's contract, withdrawn; a claim that a glyph at index 16 goes blank, when it takes the default text colour (`jam_GlyphArrangement.cpp:598`); a stale `Id::toType` in a pasted sibling, the one build error.

### State for Continuation
- Builds: `cast cast/spell.md` in jam and kuassa/user_modules, `cast cast/spell.md --debug` in eve: exit 0, 0 errors, 0 warnings. After the build, the doxygen pass changed comment text only. END and kuassa C++ are not built; run cast there and build.
- ARCHITECT's test: delete `~/.config/end/eve.md` first (version stays `0.0.1`). Read the new tables. `printf '\e[48;5;16m text \e[0m'` shows no fill.
- App side, MACHINIST: `~/.config/zsh/zshrc:98` `bg:#090D12` becomes `bg:16`. btop: a `.theme` file with background `#000000`, and `truecolor = false` (`btop.conf:5`, `:10`). The btop source is not on disk, so the first run shows which index btop sends.
- Regenerate doxygen (`ninja doxygen`; jam `<leader>bd`).

### Debts Paid
- None

### Debts Deferred
- None

---

## Sprint: Resize Is One Final Stop, the Active Prompt Anchors at Home, Column Grids Reflow in History ✅

**Date:** 2026-10-31
**Duration:** multi-session, 2026-10-30 → 2026-10-31
**Plan:** `PLAN-column-reflow.md` (rulings 1–8, Steps 1–4). The resize and prompt work ran on rulings in chat. SPEC.md State Model → Resize now records them.

### Read First (next COUNSELOR)
1. `SPEC.md` State Model → **Resize**. It holds the final-stop coalesce, the active prompt anchor and the column reflow rulings.
2. `PLAN-column-reflow.md`. Steps 2 and 3 describe the code as built: writer-side `gridStart`, chunked tail, open lanes.
3. This entry, **State for Continuation**. It lists the open work in order.
4. The entry below this one: the handoff of the terminal protocol sprint. That sprint is committed (`326387d`) but never logged.

### Agents Participated
- COUNSELOR — design discussion, rulings, PLAN, SPEC, validation of each Engineer return, audit triage.
- Engineer (sonnet) — every code change, the audit fix batches A–D and their corrections, the doxygen pass.
- Pathfinder (haiku) — reads of the JUCE FlexBox, the terminal sources of other emulators, git status.
- Auditor (opus) — one sweep after the steps: 41 findings.

### Problems Solved
1. **Active prompt anchor on resize.**
   - ARCHITECT: *"clear: draw active prompt at 0"*; *"ONE FUNCTION, the parameter is where"*.
   - `TerminalProcessor::reset` puts the cursor at home (`setCursor ({}, terminalBuffer)`). The `pendingBytes = 0` line is gone. `prepare` asserts `pendingBytes <= maximumBlockSize`.
   - zsh redraws its prompt by relative cursor-up from its own model, and oh-my-posh bakes `--terminal-width`. The fix is in the user's zshrc: `TRAPWINCH() { omp_repaint_prompt }` (`~/.config/zsh/zshrc:181`, inside the OMP block).
2. **Resize is one final stop.**
   - ARCHITECT: *"why dont we make resize always coalesce, FINAL STOP only. - resize > pixel > cell > winsize always UNIDIRECTIONAL"*; *"what about Component::Listener? EVEView orchestrate"*.
   - `jam::AudioStandaloneApp::MainWindow::resizeStart` / `resizeEnd` forward to the editor's constrainer (`jam_AudioStandaloneApp.cpp:74-84`).
   - `EVEView` is a `juce::ComponentBoundsConstrainer`. It calls `setConstrainer (this)` before `setScaling()`. `resizeStart` / `resizeEnd` call `setResizing (bool)`, which writes `Id::isResizing` to each TextEditor's component state.
   - `TextEditor::setWinsize` computes cells from `getTerminalBounds()` (the viewport less the scroll bar). It writes only while `isResizing` is false and only a valid winsize. `bindToModel` sets `isResizing` false.
   - `TextEditor::resized` always reserves the status-bar row. ARCHITECT: *"sole author of ALL is Window size"*.
3. **Column grids reflow in history.** ARCHITECT: *"i just need ls, eza, whatever column on terminai history always layout symetrical, beautifully, deterministically, without ever losing any performance."*
   - Block mark: OSC 133 C increments `TerminalProcessor::outputBlock` and starts recording. `addTextCell` marks its row while recording. `TerminalBuffer` keeps one block per ring slot (`outputBlocks`). `writeRow` stores `outputBlock` and `gridStart` as int64 after the placements.
   - Recognition at the writer: `TerminalSwap::writeFromTerminalBuffer` computes the cell starts of each row and its `gridStart` (`getGridStart`, pure). A reader-side look-back was O(L²) per lane, so it moved.
   - Lane read: `addLines` = `addRangeLines` (crosses index files) → `removeGridHead` → `addGridTail` / `addGridLines` (one chunked read) → root `Id::size` → root `Id::end` when open → `AnsiDocument::addGrids`.
   - Open lane: a lane whose first or last line carries the swap's `previousGridStart` gets root `Id::end` = swap end. `TextEditor::addLanes` builds it again when the swap end moves.
   - Layout: one line per grid. Gutters are `AttributedChar::flexGap` atoms. The codepoint holds the entry order index (overflow asserts). `TextEditor::setGridRows` places the entries with `juce::FlexBox` (column, wrap, `withOrder`, the gutter as right margin), with the smallest row count that fits.
   - ARCHITECT on width: *"make it equal space between column? then the reminder could be added to EOL as whitespace if it not zero"*. On one rule: *"no need original form. if the layout is most deterministic with ONE SSOT layout reflow rule"*.
   - Caret, selection and hit-test read the entry positions (`getGridCellBounds`, `getGridCell`, `getCellWidth`).
4. **Crash after `ls`.** `Document::Element::add` returns a pointer into `properties`. A later `add` can reallocate it. `addGrid` adds every property first, then takes `get` references.
5. **Clock contract (audit #2).** The winsize attachment started a stopped clock and doubled the start inside `setStateInformation`'s `restart`. The attachment is `restart()` again. `prepare()` starts unconditionally (SPEC Core Principle 9). The Model `winsize` row defaults to 80 × 24 (`22518101216067584` = `TerminalWinsize { 80, 24, 0, 0 }.pack()`), so `audioDeviceAboutToStart` asserts a valid winsize.

### Audit (41 findings) — DCF triage
- **Resolved:**
  - Batch A (EVE): winsize default, `preview` / `previewImage` descriptions (`parameters.md:41-45`), dead `winsize` cell in `component.md:8`, clock contract.
  - Batch B (document): `minimumGutter` scope, gap → gutter names, `addGridRows` → `addGridLines`, overflow assert, `Id::left` / `Id::top` at creation, no default-value `get`s, single tail read, open-lane `Id::end`.
  - Batch C (gui, core, bootstrap): `applyLayout` / `getLayoutRight` (const) / `getNumRows` (by value), `setEntryPositions` set-only, `rowTolerance`, `getCellWidth`, `getGridCell` (#9), no pessimistic checks, `isResizing` reset in `bindToModel`, `applyToCodepoints` via `getCodepoint`, no `getEditor` null check, open-lane rebuild.
  - Batch D (processor, buffer): one mark site in `addTextCell` (`jam_TerminalProcessor.cpp:371-372`), `outputBlockBytes` + `gridStartBytes`.
  - COUNSELOR corrections of Engineer returns: the `setRow` lambda grew `addTextRun` to 43 lines (MANIFESTO L), so the mark went to `addTextCell` (33 lines now). `[&]` capture in `getGridCell` (MANIFESTO E). Double `find` in `addLanes`. `isGridComplete` mutated the document (Verb Contract), so `Id::end` has one site in `addLines`.
  - #5, #39 by doc updates: PLAN Steps 2–3, SPEC Resize and Feature 5.
- **Doxygen pass (post-audit).** Headers only, per the `doxygen-writer` skill:
  - eve: `EVETerminalProcessor.h`, `EVEView.h`.
  - jam: `jam_TextEditor.h`, `jam_AttributedString.h`, `jam_AudioStandaloneApp.h`, `jam_TerminalProcessor.h`, `jam_TerminalBuffer.h`, `jam_TerminalSwap.h`, `jam_AnsiDocument.h`.
  - `jam_PluginEditor.h` had no stale block.
  - Doxygen did not run, so the 0-warning check is open (State for Continuation, item 1).
- **Rejected, with the read:**
  - #3 winsize persistence: it is a Model row by design; the valid default answers the assert.
  - #6 Preview code removal: ARCHITECT *"NO PREVIEW COMPONENT. dont fucking undo the working implementation"*. Only the docs changed.
  - #7: ruling 8 was ruled with that case known.
  - #11, #12: document → buffers is the same direction as `TerminalSwap`.
  - #13b/c: function reuse; creation permits multiple passes (MANIFESTO E, Complete at Creation).
  - #14: `Id::tokens` holds layout units on every line, same role.
  - #16: the sibling `setTableRows` lives in TextEditor.
  - #19–#22: L ladder; a split only for line count is wrong decomposition. `addTextRun` stays at 33 lines: wrap, clamp and cell dispatch are one cluster write.
  - #25, #31, #37, #40: no clause broken on read.
  - #26: sibling `jam::Window` has the same shape.
  - #27: ARCHITECT ruled `registerCallbacks`.
  - #35: option (a) by ruling (*"sole author of ALL is Window size"*).
  - #38: ARCHITECT `/go`.
  - #41: construction order is required.
  - #32, part: `addRangeLines` is a real responsibility (index-file crossing).

### Files Modified
- eve: `Source/EVETerminalProcessor.h/.cpp`, `Source/EVEView.h/.cpp`, `Source/EVEViewComponents.cpp`, `Source/EVEViewPanelCallbacks.cpp`, `Source/layout/component.md`, `Source/layout/parameters.md`, `SPEC.md`, `PLAN-Grand-Scheme.md`, new `PLAN-column-reflow.md`, `carol/SPRINT-LOG.md`.
- jam: `jam_terminal/processor/jam_TerminalProcessor.h/.cpp`, `jam_terminal/buffers/jam_TerminalBuffer.h/.cpp`, `jam_terminal/document/jam_TerminalSwap.h/.cpp`, `jam_terminal/document/jam_AnsiDocument.h/.cpp`, `jam_core/text/jam_AttributedString.h/.cpp`, `jam_gui/widgets/jam_TextEditor.h/.cpp`, `jam_plugin_bootstrap/standalone/jam_AudioStandaloneApp.h/.cpp`, `jam_plugin_bootstrap/view/jam_PluginEditor.h/.cpp`, `cast/gui.md` (`isResizing`), `cast/terminal.md` (`outputBlock`, `gridStart`), `generated/jam_Gui.*`, `generated/jam_Terminal.*`.
- Outside the repos: `~/.config/zsh/zshrc:181`.
- The jam tree (`git status --short`, 2026-10-31) also holds changes that this sprint's record does not name: `jam_opentype/document/*` (eleven `.cpp` files deleted and folded into `jam_OpenTypeDocument.cpp` / `jam_OpenTypeWriter.cpp`), `jam_opentype/jam_opentype.cpp`, `jam_gui/buttons/*`, `jam_gui/menu/jam_Menu.*`, `jam_gui/registry/jam_Registry.h`, `jam_Traits.h`, `jam_gui/widgets/jam_FrequencyTextBox.*`, `jam_PresetSelector.h`, `jam_VariComponent.h`, `jam_Preview.*`, `jam_data_structures/model/jam_AudioModel.*`, `jam_data_structures/parameter/jam_ParameterManager.*`. Confirm their origin before the commit.

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered
- [x] MANIFESTO.md principles applied
- [ ] Build not run after the audit fixes and the doxygen pass. Agents never build. ARCHITECT builds.

### State for Continuation
1. **Build — ARCHITECT.** `cast cast/spell.md --debug`. Every audit fix and the doxygen pass are unbuilt. The tree already holds regenerated `generated/jam_Terminal.*` (+2 lines) and `generated/jam_Gui.*` (+1 line). Check that they hold `outputBlock`, `gridStart` and `isResizing`. Then regenerate doxygen and check 0 warnings.
2. **Runtime checks — ARCHITECT.**
   - `ls` / `eza` grid: resize narrower and wider; click and select inside the grid (new `getGridCell`).
   - A grid that crosses a 256-line lane edge while the command still prints (open-lane rebuild).
   - Window resize: one `SIGWINCH` at the final stop; the prompt stays at home.
   - First launch with no saved state: the 80 × 24 default winsize, then the first layout winsize.
3. **KANJUT** — `cast --sync` for the `jam_PluginEditor` change.
4. **Windows** — the resize coalesce is untested there.
5. **Link detection** — the PTY drops OSC 8 (`jam_Terminal.h:1970-1976`). Not started.
6. **Side panel / Preview** (grand scheme §3.11) and the GIF bug — open. No Preview component exists now (SPEC Feature 5).
7. **Previous sprint** — the terminal protocol sprint (entry below, commit `326387d`) is not logged. Its receipt and `carol debt clear DEBT-20261025T214955` wait for ARCHITECT's word.

### Debts Paid
- None

### Debts Deferred
- None

## Handoff to COUNSELOR: EVE terminal protocol — one envelope, one path; measure and decode; Preview foundation

**From:** COUNSELOR
**Date:** 2026-10-30
**Status:** In Progress. Every PLAN step through Step 17 is done except Step 14, which is ARCHITECT's runtime test. The sprint is not logged.

### Read First
1. `PLAN-eve-terminal-protocol.md`. This is the locked plan, approved with `/go`. It holds the Preview fold (Steps 12a–12g, `:226-275`) and the Step 17 audit resolution (`:309-343`).
2. `PLAN-Grand-Scheme.md` §3.11 "File browser and Preview". This is the vision recorded in Step 12a.
3. `SPEC.md`. It holds the protocol, the lanes, the dock and the mental model (Step 15).
4. `DEBT.md:11-15` — `DEBT-20261025T214955`. The sprint pays it. It stays in the ledger until the log (PLAN `:298`).

### Context
- ARCHITECT's prompt: *"next sprint is 2-5"* (`jam/carol/SPRINT-LOG.md:62-66`).
- Items 2, 3 and 4 measure and decode. Item 5 pays `DEBT-20261025T214955` as the foundation of EVE's own terminal protocol, with scope *"+ preview + image"* (PLAN `:11-15`).
- On 2026-10-30, ARCHITECT folded the Preview foundation into the sprint (PLAN `:5`, `:28-36`).
- Since then, COUNSELOR has resolved the Step 17 audit batches, run the final validation build and run the doxygen pass.

### Completed
- **Steps 0–13, 15–17.** The PLAN steps are done. The Step 4 criterion is missed (see Open Questions).
- **Step 17 audit.** The Auditor reported 104 findings. Each one is resolved or rejected per DCF. The batches A1, B2a–B9 are listed at PLAN `:315-329`. The rejected findings, each with its read, are at PLAN `:331-343`.
- **Dispositions written after PLAN `:343`:**
  - #82 is resolved by B2a. `TerminalProcessor` owns the `images` lane and sizes it in `reset` (`jam_TerminalProcessor.cpp:24`). `audioDeviceAboutToStart` reaches `reset` through `prepare` (`EVETerminalProcessor.cpp:221-229`, `:261-270`).
  - #63 is rejected. `imageBounds` is replaced whole on each vblank. MANIFESTO E: *"A complete state replaces a complete state."* The vblank tell also retries an image whose bytes are not yet in the record (PLAN `:75`).
  - #86 is rejected. The dock visibility has one truth, the `preview` component property (`EVEView.cpp:112-120`).
  - #87 is rejected. The base class also dispatches with `property ==` (`jam_PluginEditor.cpp:343`).
  - #88 is rejected. A read of a Model row asks no object for its state (the same read as #79).
  - #80, part, is rejected. `juce::Identifier { name }` is the Design's own call (PLAN `:81`).
  - #84, part, is rejected. The base class owns the view bounds (`jam_PluginEditor.cpp:43-69`). Only the hand-written inverse transform changed, to `getLocalArea` (`EVEView.cpp:102`).
- **Corrections in this session, now in the tree:**
  - `writePendingData` and `handleAsyncUpdate` are back to their original form (`EVETerminalProcessor.cpp:167-179`). `setOverlays` calls no drain (`EVEView.cpp:122-145`).
  - The raw RGB path uses the new `jam::simd::convertRgbPixel`, `convertRgb4` and `convertRgb`. They follow the RGBA sibling, with SSE2, NEON and scalar arms (`jam_SimdBlend.cpp:860-923`).
  - `Lane::images` is renamed to `Lane::imageBounds` (NAMES Rule 3 and Rule 5).
  - `setImageBounds()` runs at the end of `addLanes()` again.
- **Final validation build.** Both builds exit 0. The logs have 0 `warning:`, 0 `error:` and 0 `FAILED` lines. Both reach `[65/66]` (`<scratchpad>/build-release.log:73`, `<scratchpad>/build-profile.log:73`; scratchpad `/private/tmp/claude-501/-Users-jreng-Documents-Poems-dev-eve/750cd3a0-9445-4670-9579-0d6cb7b98a57/scratchpad`).
  - Release: `cast cast/spell.md --no-sign` (`project-info.md:138-142`).
  - Profile: `cmake -S . -B Builds/Profile -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF -DCAST_PROFILE=ON`, then `ninja -C Builds/Profile` (`benchmark/burn.md:34-36`).
- **Doxygen pass.** The pass documents all 23 changed headers.
  - jam: 0 warnings (`~/.config/nvim/doxygen/Doxyfile.lib`, run with `~/.config/nvim/scripts/build-doxygen.sh`).
  - EVE: 0 warnings (`~/.config/nvim/doxygen/Doxyfile.project`).
  - Both builds pass again after the pass.

### Remaining
1. **Step 14 — ARCHITECT.** Run EVE. Senders, with PNG, GIF and JPEG:
   - `imgcat` (cells, `px`, `auto`);
   - `kitten icat`, with and without `--unicode-placeholder`;
   - `chafa`;
   - fzf with `bin/fzf-preview.sh`;
   - a full-screen CLI on the alternate screen.

   The criteria are at PLAN `:284-288`. No run of the current build has exercised the overlays or the dock.
2. **Step 11 and Step 12g checks — ARCHITECT, in the same run:**
   - `printf '\e]1337;EVE;command;name=clear\a\e]1337;EVE;command;name=clear\a'` clears twice (PLAN `:224`).
   - `eve preview <png>` and `eve preview <gif>` show in the dock. An empty path hides it. `ctrl+shift+P` toggles it (PLAN `:275`).
3. **Log — COUNSELOR, on ARCHITECT's word only** (PLAN `:307`).
   - Write the sprint block per `/log`.
   - Then run `carol debt clear DEBT-20261025T214955`, with the receipt first.
4. **Optional — COUNSELOR.** Check the doxygen prose for ASD-STE100 sentence length. The doxygen Engineer flagged this.

### Key Decisions
- *"be careful with the design we have been meticulously optimize to get fastest performance, don't fucking break working implementation. no fucking hack, DESIGN BY CONTRACT?"*
- *"if you genuinely needed and doesn't exist, write one, follow established pattern verbatim, CROSSPLATFROM"* — about SIMD RGB.
- *"why do we need specific RGB only? why not carry the alpha? RGB can use Alpha always opaque"*
- *"that's the plan. why the fuck flip flop and fucking churn tokens? do you fucking need to ask what i have approved?"* — the overlay of PLAN `:75` and `:269` stands: the vblank tell, plus a retry when the bytes are not yet in the record.
- *"TextEditor no image"*
- *"so no images, hold no image data, and name it image? why not using RectangleList?"*
- *"NAMES.md OBEY THE FUCKING PROTOCOL DESIGN BY CONTRACT CONTRACT NON NEGOTIABLE"*
- *"final validation only. not fucking every turn"* — builds.
- *"Percent-encode values"* — audit #89 (PLAN `:313`).
- *"Siblings via owner"*; *"EVEView, like overlays"* (PLAN `:360-361`).
- *"TextEditor only "reserve the space""* (PLAN `:32`).
- *"keep for now, but NOT for native Preview (Component)."* — the EVE `image` operation (PLAN `:36`).
- *"no fucking resume"* — never resume an agent. A correction goes to a fresh Engineer.
- The earlier rulings of PLAN `:17-36` stay closed.

### Files Modified
eve (`git -C eve status --short`, 2026-10-30):
- `Source/EVETerminalProcessor.h/.cpp` — lanes `commands` and `images`, `addCommand`, `getCommands`, `getSwap`, `registerEvents` additions, `prepare` drain via `triggerAsyncUpdate` + `handleUpdateNowIfNeeded`.
- `Source/EVEView.h/.cpp` — `applyCommands`, `applyImage`, `setOverlays`, the dock (`resized`, `setPreview`, `setPreviewVisible`, `isPreviewVisible`).
- `Source/EVEViewComponents.cpp` — the `onImages` binding.
- `Source/EVEViewPanelCallbacks.cpp` — the `preview` callback.
- `Source/layout/ViewLayout.md`, `Source/layout/parameters.md`, `cast/identifiers.md`, `Source/generated/Identifiers.*` — the model rows and the keys row; `commandCount` moved to EVE's cast.
- `Source/shell/zsh_integration.zsh`, `bash_integration.bash`, `powershell_integration.ps1` — the `eve` function with percent-encoding.
- `SPEC.md`, `PLAN-Grand-Scheme.md`, `comparison.md`, `carol/SPRINT-LOG.md:3`; new `README.md`, `LICENSE`, `PLAN-eve-terminal-protocol.md`.
- `benchmark/` — the control, live and replay reports; new `cat.md`, `live-sample.md`, `live-where.md`, `replay-where.md` and their reports.

jam (`git -C jam status --short`, 2026-10-30):
- `jam_terminal/processor/jam_TerminalProcessor.h/.cpp` — payload mode, the OSC 1337, DCS and APC routes, the `images` lane, `eveOperations`, percent decoding, Kitty placements.
- `jam_terminal/processor/jam_TerminalFIFO.h/.cpp` — the `maxBytes` overload and `applyPendingData`.
- `jam_terminal/image/` — `jam_TerminalITerm2Decoder`, `jam_TerminalKittyDecoder` (restored, metadata only), `jam_TerminalSixelDecoder`, `jam_TerminalImageSequence.h`; new `jam_TerminalImageEncoding.h`. Deleted: `jam_TerminalSkit.h/.cpp`, `jam_TerminalDecodedImage.h`, `jam_TerminalPendingImage.h`.
- `jam_terminal/document/` — `jam_TerminalSwap`, `jam_AnsiDocument` (`stringTerminator`, `getIndexedColour`, `dcs`/`apc`); new `jam_TerminalImageRecord.h/.cpp`.
- `jam_terminal/buffers/jam_TerminalBuffer.h/.cpp` — `ImagePlacement {destination, image}`, placements on both grids, `removeImagePlacements`.
- `jam_terminal/graphics/jam_TerminalGraphicsContext.h/.cpp` — the inline image branch removed.
- `jam_gui/widgets/jam_TextEditor.h/.cpp` — reserved space only, `imageBounds`, `onImages`, `setImageBounds`.
- New `jam_gui/widgets/jam_Preview.h/.cpp`; `jam_gui.h/.cpp`.
- `jam_graphics/fonts/jam_SimdBlend.h/.cpp` — `convertRgbPixel`, `convertRgb4`, `convertRgb`.
- `jam_data_structures/model/jam_Model.h/.cpp`, `jam_ParameterText.h`, new `jam_ParameterText.cpp`; `jam_plugin_bootstrap/parameter/jam_ModelLayout.cpp` — the `text` row and `store`.
- `jam_core/text/jam_AttributedString.h/.cpp` — `setAtom`.
- `cast/terminal.md`, `cast/gui.md`, `cast/plugin_bootstrap.md`, `cast/spell.md`, `generated/*`, `user-modules-info.md`.

### Open Questions
- **Step 4 criterion — ARCHITECT.** Decode = `terminalProcess` − `terminalRead`. It went from 15.38 s (Step 3, this session) to 15.49 s (`benchmark/live-report.md:19`, `:27`: 16.340 s − 0.853 s). That is +0.7%. The criterion was "no worse" (PLAN `:304`).
- **Sixel caps — ARCHITECT ratifies.** 4096 × 4096 pixels, and 65535 for a parameter.
- **`ninja doxygen` — ARCHITECT.** No EVE build directory defines the target. `CLAUDE.md` lists it. The pass ran `doxygen` direct on `Doxyfile.project`.
- **scout kernel sampler — ARCHITECT.** There is a known bug at `scout/Source/Sampler.cpp:154`.

### Next Steps
- Start at Remaining item 1, when ARCHITECT reports the Step 14 run. Fix each runtime failure from a read, per CAROL.md Failure Protocol.
- Log when ARCHITECT says "log sprint".

