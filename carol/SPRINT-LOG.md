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

## Sprint: The Theme File Is the Style Sheet in Markdown — One Mirror per CSS Block, CssWriter Feeds the CSS Parse, Fixed-Signature Function::Map ✅

**Date:** 2026-11-09 to 2026-11-10
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"theme.md user should not care about hierarchy. fonts is fonts, colours is colours, metrics is metrics."*; *"there should be only ONE metrics table"*; *"DESIGN BY CONTRACT UNIVERSAL and GENERIC. no harcoding of assumptions, that only works for this pattern. must be ABSTRACTION"*; *"what is tree with key? what the fuck is this new bullshit foreign"*; *"dont fix the symptoms. wrong name means wrong mental model ... fix the cause, fix the design"*; *"if ConfigDocument need to be rewritten from scratch as SettingsDocument to be BLESSED compliant, FIX IT NOW. no more hack and patch, no hardcoding, no workaround"*. The plan was approved through plan mode. During the run: *"if all args are always juce::ValueTree&, why not define it as jam::Function::Map<juce::Identifier, juce::ValueTree&> events; ? then add could be auto deduce?"* (option "Events: fixed-signature map" chosen); *"can we add to CODING, template with erased type such as <..., void> only allowed for different return args?"*.

### Agents Participated
- COUNSELOR — the reads of ConfigDocument, the Validator, CssDocument, StyleManager, the END and EVE theme lanes, `jam::Function` and `juce::FixedSizeFunction`; the plan copy; SPEC, ARCHITECTURE and CODING edits; the `getThemeTree` signature fix; the `user-modules-info.md` rows; the filter of 28 audit findings and the doxygen findings; this log.
- Pathfinder (haiku) — the rewrite inventory; the doxygen pass over eight headers; two `Function::Map` census runs (END with JAM, JFS with KANJUT; still running at this log).
- Engineer (sonnet) — Steps 1–4, Step 5, Step 6, the lossless-mirror batch, audit fixes A (stopped by the watchdog at 15 minutes and resumed) and B, the final two fixes, the `Function::Map` specialization and its callers, the DRY and header-body fixes.
- Auditor (opus) — one run, 28 findings, 5 blocking.

### Files Modified
- `jam/jam_markdown/document/jam_ConfigDocument.{h,cpp}` — the sheet emit and the sheet parse are deleted (`getSheetType`, `getPropertyKey`, `getComponentWords`, `addSubTable`, `###` tables, COMPONENT, lexicon guessing, `getColourValue`, the `em` UNIT child, the colour type guess). `toDocument (const CssDocument&)` writes one table per section: `colours` (palette + colour-id rows), `fonts` (face rows + font-id rows, attributes from the fonts section on the first face), `metrics`, `window`, one per class; record columns (`dark`, …) come from the section's record children. `getAliasKey` is the one lexicon rule for the writer and the Validator. `getRowKey` is public; `isRowType` is new. `toDocument (const juce::ValueTree&)` stays for `jam_Profiler.cpp:172`.
- `jam/jam_markdown/document/jam_CssWriter.{h,cpp}` (new) — `CssWriter : Document::Writer` writes the css text of a theme document. `CssDocument::parse` of that text builds the sections, so the css document is the only builder of its sections.
- `jam/jam_markdown/document/jam_ConfigValidator.{h,cpp}` — the `alias` rule (key from `getAliasKey`; each value or record cell names a colour row or a face row); `row` split into `row`, `row.key`, `row.numbers`, `row.value`, `row.src`, `row.type` (type change against the default row); palette, face and alias rows need no default row; a `colour` cell is hex in every table; named boolean terms.
- `jam/jam_markdown/jam_markdown.{h,cpp}`, `jam/user-modules-info.md` — the CssWriter submodule rows and includes.
- `jam/jam_web/css/jam_Css.{h,cpp}` — the COMPONENT section is deleted; face records are typed `Format::toType` of the family (the `addWidgets` form); `getSheetType (styleSheet)` is new.
- `jam/jam_core/function_map/jam_Function.h` — partial specialization `Map<KeyType, ReturnType (Args...)>` over `jam::HashMap<KeyType, std::function<ReturnType (Args...)>>`: `add (key, fn)` with no template argument, `get` with no `dynamic_cast`, `contains`.
- `jam/jam_plugin_bootstrap/layout/jam_PluginEditorLayout.{h,cpp}` — `getStyleTree`, `getThemeTree (defaultDocument)`, `getSettingsTree (…, getTree)`; `getThemeSettings` writes `toDocument` of the style sheet that `StyleManager::getStyleSheet()` names (`std::find_if`, no sentinel); under END only `getThemeRules()` apply; the theme default parses with origin `files::theme`.
- `end/Source/config/ConfigTheme.cpp`, `ConfigDirectory.{h,cpp}`, `ConfigModel.{h,cpp}` — `addFileTables` and `addDefaultTables` take `getDefaultDocument` and `getTree`; `getStyleTree` is `CssDocument::parse (CssWriter {}.getText (document)).getValueTree (THEMES, sheet type)`; `getDefaultText` (no memo); `getConfigTree`, `getDefaultDocument`; `saveToPath` body in the `.cpp`.
- `end/Source/config/display.md` — `## tab` and `## pane` moved here from `theme.md`. `end/Source/config/theme/gfx/theme.md` — front matter only.
- `end/Source/Main.{h,cpp}`, `lookAndFeel/ENDLookAndFeel.{h,cpp}`, `ui/ENDView.{h,cpp}`, `ui/EditorView.{h,cpp}` — events maps are `jam::Function::Map<juce::Identifier, void (juce::ValueTree&)>`; `add` has no template argument.
- `end/SPEC.md` (Theme style, Theme on disk, Theme load, Pane padding), `end/ARCHITECTURE.md` (Config Chain), `end/PLAN-style-driven-theme.md` (the locked plan with Execution facts).
- `~/.carol/CODING.md` — CRITICAL RULES and checklist: "Uniform signature in the type".
- Doxygen pass over the eight headers above.

### Alignment Check
- [x] BLESSED principles followed — one Model (`CssDocument`), two serializations; the owner builds its sections; no tier is derived in ConfigDocument.
- [x] NAMES.md adhered — `CssWriter` joins `MarkdownWriter`; `getAliasKey`, `getSheetType`, `getStyleTree`, `getThemeTree`, `isRowType` join their families; the `Map` specialization adds no name.
- [x] MANIFESTO.md principles applied.
- [x] No file under `jam_vulkan`; no change to `jam_StyleManager`, KANJUT or JFS; no version bump.
- [ ] Not built. No agent ran `cast` or a build.
- [ ] Zero doxygen warnings unconfirmed; no index was regenerated.

### Problems Solved
- **The tiers were guessed back.** The emit started from the collapsed `getValueTree` tree and rebuilt the css tiers with predicates, COMPONENT and `###` tables. Now the markdown is written from the sections and read back through the css parse.
- **Two metrics tables, sheet hierarchy on disk.** `theme.md` has `## colours`, `## fonts`, one `## metrics`, `## window` and one table per class, with plain words.
- **The reader mutated a finished `CssDocument` from outside its owner** (Auditor 5). `CssWriter` + `CssDocument::parse` replaced it.
- **EVE inside END rejected END's theme** (Auditor 1): the reference rule compared END's palette with EVE's. Under END only the theme rules apply.
- **Audit, resolved:** 1–12, 15 (record columns), 16, 17, 19–23, 25–27. **Rejected with the read:** 13 (a pair of handles, MANIFESTO "A scalar, a handle ... does not copy"), 14 (`style.md:114` has no `colour id` suffix), 15 rest (YAGNI; `toType` tree identity, `jam_Css.cpp:2183`; plan excludes page/element), 18 and 28 (work buffers in creation), 24 (ConfigDocument is one object after the reader moved; CODING "Never make a small .cpp to meet 300/30/3").
- **Doxygen pass findings:** DRY `isRowType` and the header body of `saveToPath` resolved; the `atRuleTypes` instance, the `add` container parameters (NAMES Rule 1 `addToken` example), a bare line count and a two-copy list rejected.
- **Mistakes owned.** I briefed `getThemeTree()` with no parameter, so it parsed the defaults twice; I fixed the signature. I called the type-erased `Function::Map` a breach of fail-fast; ARCHITECT corrected it: the `dynamic_cast` assert is deterministic. I copied the `add<…>` family shape into my own briefs without testing it against BLESSED.

### State for Continuation
- ARCHITECT runs `cast` in jam, eve and end, builds all three, and deletes `~/.config/dev/themes/gfx/theme.md` once (same version, never overwritten).
- Check first at compile: `jam::HashMap::insert ({ key, std::function })` in the `Map` specialization (`jam_Function.h:347-373`); the `getThemeTree (defaultDocument)` declaration (`jam_PluginEditorLayout.h`).
- Runtime: a fresh `theme.md` (tables above); a palette colour, a colour id, a face height and a metric edited live; a corrupt row renders defaults with the error; hosted EVE under END with END's colours; Markdown and Mermaid colours and fonts; dark appearance.
- Run `<leader>bd` in jam and `ninja doxygen` in eve and end; confirm zero warnings.
- The `Function::Map` census (END with JAM, JFS with KANJUT) is running. Its result decides whether uniform maps store `juce::FixedSizeFunction<N, R (Args...)>` (`juce_FixedSizeFunction.h:96-101`, no allocation) and the buffer size `N`.

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261108T145906` — Ctrl+L destroys the Claude Code prompt on the normal screen (unchanged).
- `DEBT-20261105T000000` — Windows runtime test (unchanged).

---

## Sprint: END Theme on the Style Contract — style.css Is the One Source of Fonts, Colours and Metrics, the Markdown Emit Is the Inverse of the Parse, Style Reads the Disk Tree ✅

**Date:** 2026-11-09
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"previous sprint counselor tried to fix theme.md config contract because it was messy"*; *"it's either we fix this mess, or we could start from HEAD"*; the config contract: *"ConfigDocument should not doing any garbage defensive programming with bullshit jassert"*, *"error should never be failure. carried binary always provide the default fallback. validation is always Document::Validator job"*, *"binary write to virgin state to disk, disk always win."*, *"if disk was invalid, rendering never fails ... BUT Validator must show error."*; the css contract: *"css contract: ALL RELATED STYLE SSOT - fonts - colour - metrics"*, *"StyleManager build the map - StyleCustom consume downstream, style related `config` always through Style, never by poking config directly."*, *"config - file watcher listener events - style changed - component repaint"*; the format contract: *"md must be using plain words without any redudancy of `colour id`, `font id`"*, *"the contract is at CALLEE. starts from SSOT cast table manifest ... the problem is INCONSISTENCY"*; *"why both options are garbage? why there are shadow state?"*; *"there are other agents working on Vulkan at jam ... be VERY CAREFUL"*. The plan was approved through plan mode.

### Agents Participated
- COUNSELOR — the reads of StyleManager (jam and KANJUT), the theme lane, ConfigDocument and Css; the plan; Step 1 and Step 8 edits; every read-back; the filter of 49 audit findings; this log.
- Pathfinder (haiku) — tree state and diffs of eight repos, StyleManager history, plans and logs, HEAD `theme.md`, two inventories, the removed emit code, the doxygen pass.
- Engineer (sonnet) — the emit inverse and the `getOrCreate` overload; the style data, ConfigTheme and consumers; the audit fixes; the plan copy.
- Auditor (opus) — one run stopped by the watchdog at 15 minutes (one run held six traces), then two partitions, 21 and 28 findings.

### Files Modified
- `jam/jam_markdown/document/jam_ConfigDocument.{h,cpp}` — no `jassert`. The emit is the parse's inverse again: `getKeyWords` drops the component words and the id suffix, `getCellText` writes an alias in words, `addSheetTables` writes `### colours`, one table per component, `### fonts`, `### appearance` and `### metrics` under one `##` sheet; a table with child rows writes its properties as `### metrics`. `hasIdEntry` knows an id key by lexicon membership, because `text button text colour off id` (`cast/style.md:114`) does not end with `colour id`. An empty dark cell parses as the light value. `getOrCreate (name, text)` memoizes a document parsed from given text. Doxygen synced.
- `jam/jam_markdown/document/jam_ConfigValidator.cpp:231-237` — a font `src` that names no default file is a validation failure (`failUnknownReference`), so a bad disk file never reaches `StyleManager::getOrCreateTypeface`.
- `jam/jam_style/jam_StyleCustom.{h,cpp}` — `getDefaultScrollbarWidth` has no `isEND()` branch; the `scrollBarThickness` metric of the style sheet is the one path.
- `jam/jam_web/css/jam_Css.{h,cpp}` — the comma-list attribute and `getStyleAttributeType` are deleted.
- `end/Source/config/theme/gfx/style.css` (was `gfx.css`) — the three faces whose file END does not ship are deleted (`symbols`, `--notoSymbols`, `--notoEmoji`: the crash at `jam_StyleManager.cpp:105-107`); `.window` (`blur`, `mac`, `win`); `--ResizableWindow--backgroundColourId`; tab and pane metrics; `tab` and `overlay` faces. The file name makes the sheet `STYLE`, the sheet hosted EVE reads.
- `end/Source/config/theme/gfx/theme.md` — only rows that are not a font, colour or metric: `## tab` (`position`, `always visible`, `uppercase`), `## pane` (`split line`).
- `end/Source/config/ConfigTheme.{h,cpp}` — `getOrCreateDefaultText`: carried `theme.md`, a newline, then the markdown built from `style.css`, once per file name (the canon form `kuassa_PluginEditorLayout.cpp:601-603`); the virgin write uses it.
- `end/Source/lookAndFeel/ENDLookAndFeel.{h,cpp}` — StyleManager is created over the THEMES tree; the `theme` event runs `registerStyle`, `applyColours`, `applyFontStyles`, `loadGraphics`; fonts, metrics, window and colours read through StyleManager; the per-table events and the menu opacity read are deleted. Doxygen synced.
- `end/Source/Main.{h,cpp}` — `initialiseStyle` is the one init call. Doxygen synced.
- `end/cast/files.md`, `end/cast/identifiers.md` — `style.css`; eight metric ids added, seven ids with no reader deleted.
- `end/SPEC.md:164-`, `end/ARCHITECTURE.md` (engine lifecycle, thread table, config chain), `end/PLAN-style-driven-theme.md` (replaced by the locked plan).

### Alignment Check
- [x] BLESSED principles followed — one source per row, one runtime tree, one flow for init and reload.
- [x] NAMES.md adhered — `hasIdEntry` joins `isIdEntry`, `getOrCreateDefaultText` joins `getOrCreate`, the metric ids join `scrollBarThickness`.
- [x] MANIFESTO.md principles applied.
- [x] No file under `jam_vulkan` and no change to `jam_StyleManager`.
- [ ] Not built. No agent ran `cast` or a build.
- [ ] Zero doxygen warnings unconfirmed; the jam XML still names `getStyleAttributeType`.

### Problems Solved
- **Two truths for the theme.** Embolden and scrollbar thickness lived in `theme.md` and in `gfx.css`; StyleManager held a css-only tree beside the config tree. Now `style.css` holds every font, colour and metric, the disk file is built from it, and StyleManager registers the one THEMES tree on every change.
- **SCREAMING values and `colour id` keys on disk.** The css parse stores a reference as a tag (`jam_Css.cpp:411-412`) and the emit wrote it raw; the previous sprint had deleted the emit's suffix strip. The emit and the parse are inverses again, at the callee.
- **No colours a user can override.** The previous sprint's hand-written `theme.md` had no colour table. The built file has the palette and every component table.
- **Audit, resolved:** A-F1, A-F2 (lexicon membership), A-F10 (dark column), B-1 (font file validation), B-3 (empty dark cell), B-5 (default text per name), B-11 (one init flow).
- **Audit, rejected with the read:** B-6 — `Main.cpp:61-65` cascades on `Id::theme`, after the LookAndFeel listener (`juce_ListenerList.h:403-413`). B-12 — `setAppearance` changes the laf colours, not `colourScheme`, which `loadGraphics` reads (`ENDLookAndFeel.cpp:500`). A-F8 — every StyleManager lookup applies `Format::toType` (`jam_StyleManager.cpp:83`, `jam_StyleManager.h:195`), so the key case of the css tree and the disk tree meet.
- **Mistakes owned.** I first reported a StyleManager contract change, then corrected it: the KANJUT differences came from the locked isomorphism plan. I offered a HEAD-or-fix disposition and a row-source question that the contract answered. My first plan kept hand-authored style rows in `theme.md`.

### State for Continuation
- ARCHITECT runs `cast` in jam, eve and end, builds all three, and deletes `~/.config/dev/themes/gfx/theme.md` once (same `version: 0.0.1`, never overwritten).
- Runtime checks: a fresh `theme.md` with `## style` and its `###` tables; a colour, metric and font height edited live; a corrupt file renders defaults with the error; hosted EVE with END's colours; Markdown and Mermaid fonts and colours; EVE standalone writes its own file in the same shape; dark appearance.
- Run `<leader>bd` in jam and `ninja doxygen` in eve and end; confirm zero warnings.
- Audit findings not resolved, each read and real, larger than this plan: A-F3, F4 (component and dark loss for font-only or palette rows), A-F5 (component tables also hold metrics, and an `### appearance` table exists, against Plan Step 2.4 wording), A-F6, F7 (font-lexicon guess outside `### fonts`), A-F9 (font numbers as strings), A-F11 (units other than `em`), A-F12 to F28 (Lean, DRY and form in `jam_ConfigDocument.cpp` 719 lines and `jam_ConfigValidator.cpp` 382 lines), B-2, B-4 (hosted EVE overlay drops nested children), B-7, B-8 (popup `SafePointer` and `callAsync`), B-9, B-10 (rasterizer assert), B-13 to B-21 (repeats, sentinel, forms, duplicate `theme`/`themes` ids).

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261108T145906` — Ctrl+L destroys the Claude Code prompt on the normal screen (unchanged).
- `DEBT-20261105T000000` — Windows runtime test (unchanged).

---

## Sprint: Claude Code Cursor — Caret Gate on the Grid, Inverse Video in the Glyph Painter, Debt Captured, Doxygen Pass ✅

**Date:** 2026-11-08
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"there something fishy with this build with claude code full screen (alternate), now there's no cursor at all with Claude code"*; *"why cursor hidden with Claude Code? although no issue with vim, because we let vim handle all cursor drawing? why eve disagree with whatever claude sent? why we hide the cursor?"*; *"caret still should be visible on normal screen too"*; *"cursor fixed. both fullscreen and normal shows caret with claude code."*; *"add to DEBT.md ctrl+l destroys CC active prompt rendering with normal screen"*; *"clean up instrumentations"*; *"update doxygen docs if necessary to methods you have just changed"*; *"/log"*.

### Agents Participated
- COUNSELOR — the caret and paint-path reads, the log-driven diagnosis, both fixes, the instrumentation and its removal, the DCF filter on the doxygen pass, this log.
- Pathfinder (haiku) — the caret-path and git-history traces, the Claude Code 2.1.319 binary analyses (three runs), the doxygen pass on `jam_GlyphArrangement.h` and `jam_TextEditor.h`.
- Engineer (sonnet) — the `carol debt add` capture.

### Files Modified (5 total)
- `jam/jam_gui/widgets/jam_TextEditor.cpp` — the terminal caret gate reads the grid: `isVisible = isCursorVisible and cursor.y < winsize.rows` (`Id::winsize` unpacked, the form of the mouse path). `getCellBounds (const TextBlock&, int, int)` extrapolates the cell for a line at or past the last line: the last row's bottom plus one cell height per row past, at the plain grid x, mono cell size.
- `jam/jam_graphics/fonts/jam_GlyphArrangement.cpp` — `addGlyph` resolves the default foreground (`juce::TextEditor::textColourId`) and default background (`juce::ResizableWindow::backgroundColourId`) and swaps them when the entry's inverse flag is set. `addBackground` adds the rectangle for an inverse entry even when its bgColourId is colourIdNone.
- `jam/jam_graphics/fonts/jam_GlyphArrangement.h`, `jam/jam_gui/widgets/jam_TextEditor.h` — doxygen synced to the two changes.
- `eve/DEBT.md` — `DEBT-20261108T145906` (Ctrl+L destroys the Claude Code prompt on the normal screen), captured with `carol debt add`.
- Ephemeral diagnostics (ODE §VI): `debug::Log` sites in `jam_TerminalProcessor.cpp` (`privateMode`, `bytes`), `jam_TextEditor.cpp` (`caretEmpty`, `caretFocus`) and a `Log::Scope` with `eveInstance` in `EVETerminalProcessor.cpp` — added, read, and removed this sprint. No trace remains (verified by search).

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered — no new name.
- [x] MANIFESTO.md principles applied
- [x] Validated by ARCHITECT's build and run: the caret shows in Claude Code on both screens.
- [ ] Zero doxygen warnings unconfirmed: regeneration is ARCHITECT's (`<leader>bd` in jam).

### Problems Solved
- **No caret with Claude Code, root cause.** The terminal caret gate compared the cursor grid row against the live document's line count (`cursor.y < live.index->getNumLines()`). Claude Code parks its prompt cursor on the empty row right after the content, where `y == lines`, so a cursor shown by DECTCEM painted nothing. Vim and the shell keep the cursor on a written row, so only Claude Code hit the boundary. The gate now reads the grid height (`Id::winsize` rows), and the cell geometry extrapolates past the last line.
- **Inverse video had no paint-path consumer.** Claude Code hides the native cursor in both screens and draws its own as an SGR 7 inverse cell (`"\x1B[7m"+char+"\x1B[27m"` in the 2.1.319 bundle). The `inverse` flag reached only the atlas key. `addGlyph` now swaps the resolved foreground and background, and `addBackground` fills the cell, per xterm semantics.
- **Evidence method.** Temporary `debug::Log` sites captured the DECTCEM traffic and the caret-gate inputs; the logs showed `visible true` with an empty caret at `y == lines`, and later that Claude Code hides the native cursor and paints its own.
- **Mistakes owned.** I second-guessed ARCHITECT's test runs twice, and misread the diagnostic log path after the config directory moved to `~/.config/dev`.

### State for Continuation
- ARCHITECT runs `<leader>bd` in jam and confirms zero warnings; the doxygen pass changed two headers and did not regenerate the index.
- Delete `~/eve-caret.log` — the diagnostic log is dead. `~/.config/end/END.ode` and `~/.config/dev/END.ode` hold only headers.
- `DEBT-20261108T145906` (Ctrl+L destroys the Claude Code prompt on the normal screen) enters the next sprint scope (JRENG law).

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261108T145906` — Ctrl+L destroys the Claude Code active prompt rendering on the normal screen (ARCHITECT, captured this sprint).
- `DEBT-20261105T000000` — Windows runtime test (unchanged).

---

## Sprint: END Crashes at Init and after a Pane Split — MessageOverlay Null Read, One Split Path, Grid Events Lane off the Record Writer, Audit, Doxygen Pass ✅

**Date:** 2026-11-08
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"latest end's built, always crash related to config. deleted config, crash. adding files, crash. always crash at init"*; *"there are multiple instances of END crashing, after splitting pane. both case either with click drag from corner or with keyboard shortcut"*; *"i believe there are also race when setting winsize"*; *"fix it"*; *"1-2 supposed to call the exact SSOT commandId, same flow, same path, ONE path. only differs in interaction source"*; *"there are always only 2 split, either horizontal/vertical. keyboard shortcut pass them explicitly, while drag from corner decide when mouse up should split either vertical/horizontal"*; the concurrency contract: *"Processor (audio thread) ALWAYS LOCK FREE; the exception is only at chain where we have no control, such as OS tty; message thread always use juce::Value accessor; other thread other than message thread, may access through atomic accessor"*; *"dont create new pattern, no new semantics. what pattern already exists? no manual handroll anything new. just follow verbatim, nothing to reinvent. we only fix the contract adherence. DESIGN BY CONTRACT"*; the chosen pattern *"Lane + counter"*; *"doxygen until /log"*.

### Agents Participated
- COUNSELOR — crash-report triage, the reads of JAM, END, EVE, JUCE and SPEC behind each claim, every delegation, the read-back of every edit, the DCF filter on 26 audit findings and 9 doxygen-pass findings, this log.
- Pathfinder — the faulting-thread stacks of 21 `END-*.ips` reports; all-thread stacks of the two split crashes; git history of the grid handlers in EVE and JAM; the doxygen pass in two runs.
- Engineer — the MessageOverlay fix; the drag split on the command path; the grid events lane; the audit fixes; the scope and separator fixes.
- Auditor — one sweep over the three sprint changes, 26 findings.

### Files Modified (10 total)
- `jam/jam_gui/widgets/jam_MessageOverlay.h:34-65` — the `axisLineStyle` default is `{}`. The old default `map::OverlayAxisLine::getInstance()->getDefault()` ran at the call site before any `SharedInstance` holder existed, and `getInstance()` returned null in Release (`jam_SharedInstance.h:69-75`). New `regionSeparator` beside `noSplitLine`. Doxygen synced.
- `jam/jam_gui/widgets/jam_MessageOverlay.cpp:11-111` — the style lookup runs only inside the split branch, through the function-static holder. One `axisLines.get` call. Each constant lives in the one function that uses it; `textPadding` stays at file scope (two users). The split token is `regionSeparator`.
- `end/Source/ui/TabView.cpp:19-28` — `addPane` has no `proportion`; it calls `split (edge)`.
- `end/Source/ui/TabView.cpp:137-159` — an inward drag invokes `map::CommandId::splitVertical` or `splitHorizontal` through `commandManager`, the commands of the keyboard shortcut (`ENDView.cpp:333-348`).
- `end/Source/ui/TabView.cpp:226-228` — the split preview joins its sizes with `jam::regionSeparator`.
- `end/Source/ui/TabView.h` — `addPane` declaration; `addPane` and `mouseUp` doxygen synced.
- `eve/Source/EVETerminalProcessor.cpp` — `nullTerminatorBytes`; file-static `applyNames` (the in-place parse of zero-terminated names, shared by `applyCommands` and `applyGridEvents`); `registerGridEvents` writes `<event>\0<line>\0` to the `gridEvents` lane and raises `numGridEvents` with `storeValue`; `applyGridEvents` applies the three `gridStarts` row bodies (moved unchanged) on the message thread through `listUpdates`; both lane writers assert when full; `getWinsize` reads the atomic `Parameter<int64_t>`; `prepare` runs the record writer again after the swap prepare, so `firstLine` comes from the writer's queue (`jam_TerminalSwap.cpp:145`).
- `eve/Source/EVETerminalProcessor.h` — `applyGridEvents`, `gridEvents`; doxygen synced.
- `eve/Source/EVEView.cpp:137-140` and `EVEView.h` — the `numGridEvents` handler runs `applyGridEvents()`; doxygen synced.
- `eve/cast/identifiers.md:26` — `numGridEvents`.
- `eve/Source/layout/parameters.md:43` — `numGridEvents`, `int64`.

### Alignment Check
- [x] BLESSED principles followed
- [x] NAMES.md adhered
- [x] MANIFESTO.md principles applied
- [ ] Not validated by a compile. No agent ran `cast` or a build.
- [ ] Zero doxygen warnings not confirmed. Regeneration is ARCHITECT's.

### Problems Solved
- **Crash at init and on each config load.** `ConfigModel::loadFromPath()` always ends with `appModel.setMessage` (`ConfigModel.cpp:87-90`), runs in the constructor (`:20`) and on each `.md`/`.svg` change (`:110-113`). The message repaints `MessageOverlay`, and its paint read the null `OverlayAxisLine` instance (`.ips` frame `jam::MessageOverlay::paint + 84`, x1 `Bimap::getDefault()::none`).
- **Crash after a pane split.** Both split crashes (`END-2026-11-08-172712.ips`, `-172814.ips`) caught the message thread in `jam::TextEditor::setWinsize()` → `ValueTree::setProperty` and a `libdispatch` thread in `EVETerminalProcessor::registerGridEvents()::$_2` → `ValueTree::setProperty` at the same instant; one then jumped to address 0 from a display link callback, the other hit *"memory corruption of free block"*. The grid handlers came in with `fe0465a` and wrote the tree from `jam::AsyncUpdater`'s serial GCD queue (`jam_AsyncUpdater.h:18-20`), against `SPEC.md:771-779` and `jam_Model.h:14-15`. The record writer fires one event per row (`jam_TerminalSwap.cpp:133-136`, `:185-195`), so a scalar Model row would drop events; the `commands`/`numCommands` lane + counter carries them.
- **One split path.** The drag called `addPane` directly; it now runs the same command as the shortcut.
- **Audit (26), resolved:** 1, 12 in-place drain shared with `applyCommands`; 2 `int64` counter; 3 one writer context for the lane; 5 asserts; 6 named terminator; 7, 8, 10 renames to the `events` family (`lists` kept, ARCHITECT's name from `fe0465a`); 17 `jam::Model::getOrCreateChildWithName`; 18 `getWinsize` only; 19; 22.
- **Audit, rejected with the read:** 4 the flush runs under the message-manager lock (`SPEC.md:780-781`); 9 `applyPendingInput()` takes no target in the same class; 11 two uses, MANIFESTO S abstracts *"more than twice"*; 15 the counter is the Model change signal, as `numCommands` (`SPEC.md:151-153`); 16 the tree is the message-thread accessor (`jam_Model.h:15`, `jam_TextEditor.cpp:1861`); 20 the default serves the two callers that draw no split line and is never looked up there; 21 `getInstance` asserts a live holder (`jam_SharedInstance.h:67-73`), the call site was the defect; 23 different mappings (command → edge at `ENDView.cpp:335-338`, axis → command at `TabView.cpp:151`); 25 the invalid identifier is the documented add contract (`TabView.h:55`); 26 a registration table is the lookup form. 24 noted, the preview is ruled out of discussion.
- **Doxygen-pass findings:** C6 (narrow scope) and C7 (`" | "` twice) resolved. C1, C2 rejected (MANIFESTO E's CORRECT example: `jassert` then the same positive check). C3 rejected (the table is the lookup). C8 rejected (`theme.md:216-218` allows only `solid, dash, bracket`, the keys at `jam_Gui.h:314-318`). C9 rejected (`ValueTree` is not a container).
- **Mistakes owned.** I offered the MessageOverlay fix as an A/B disposition when CONTRACT answered it. I asked a preview question that was ruled out. My `/ask` answer said no open questions before the race evidence came in.

### State for Continuation
- Run `cast cast/spell.md --debug` in `eve` so `Source/generated/` gets `Id::numGridEvents` (`cast/identifiers.md:26`). Then build `jam`, `eve`, `end`. No compiler has seen this sprint.
- JAM already holds `c5587c6a7` *"Stop the message overlay crash on its first paint"*; the later JAM edits (one axis call, constant scope, `regionSeparator`, doxygen) are uncommitted.
- Run `<leader>bd` (JAM) and `ninja doxygen` (EVE, END), and confirm zero warnings. LSP on JAM files returned wrong or empty targets this session.
- Open, presented to ARCHITECT, no ruling yet: (a) audit 18 rest — `getHistoryCapacity` and the `cwd` read in `openTTY` read the tree on the `start()` caller's thread; `ParameterText::getValue` is consumer-thread only (`jam_ParameterText.h:24-26`) and `historyCapacity` has no atomic accessor. (b) audit 13, C4, C5 — `EVETerminalProcessor.cpp` is 510 lines with four jobs; `openTTY` is 35 lines. (c) audit 14 — `SPEC.md:198-200` lists the lanes without `gridEvents`, and `:755-764` lacks `numGridEvents`; writing SPEC is gated.
- Fact noted, ruled out of discussion: the split preview paints at the cursor (`TabView.cpp:193-232`); the command splits at `Id::right` or `Id::bottom` with the default proportion.

### Debts Paid
- None

### Debts Deferred
- None

---

## Sprint: Scrollbar Width from END's Theme, Drag and Autoscroll Fixed, Alternate-Screen Bar Discarded, Plan Hygiene, Wholesale Audit of the Uncommitted Tree, Doxygen Pass ✅

**Date:** 2026-11-08
**Duration:** one session. The start and end times are not recorded.
**Scope (ARCHITECT):** *"i want my scrollbar thickness at theme.md NOW"*; *"NO FUCKING BUMPING VERSION"*; *"let's fix why scrollbar now cant be dragged"*; *"always STAY at wherever the position of the scrollbar ... when at bottom, this automatically scroll down to bottom always ... when not at bottom ... DONT SCROLL AT ALL, stay"*; *"adding block and set view should always be DECOUPLED"*; *"i still wanna try alternate screen with scrollbar"*; then *"janky. remove all scrollbar for alternate."*; *"wholesale /audit all files touched from last sprints against contract"*; *"comprehensive doxygen pass"*; *"/log"*; *"NO GATE"*; *"check all plan, we want hygiene. what actually completed, supersed, can be deleted. the canon is numbered multi part PLAN. PLAN 0 is the tracker"*; *"no audit. audit later wholesale for the whole sprint (pre-handoff)"*.

### Agents Participated
- COUNSELOR — orchestration, every read-back of subagent edits, the DCF filter on every audit finding, two one-line fixes (`ENDView.cpp:259`/`ENDView.h:165` rename, `jam_MessageOverlay.h` dead member), PLAN writing.
- Pathfinder — inventories (plans, `git status` of eight repos, the alternate-screen design reads) and the doxygen pass (four partitions, several rounds).
- Librarian — alternate-screen research in the local neovim, ghostty, wezterm, iTerm2, kitty and alacritty trees (first run stopped by the watchdog for scope drift into a binary; second run narrowed).
- Engineer — every code change: scrollbar theme row, drag and autoscroll fixes, the alternate-screen bar and its removal, the plan deletions, and twelve audit-fix batches.
- Auditor — the wholesale audit in ten partitions (A1a, A1b, A2, A3, A4, A5a, A5b, A6; A1 and A5 stopped by the watchdog and re-run narrower).

### Files Modified (key changes; the full set is the uncommitted tree of jam, eve, whelmed, stamp, cast, scout, ggwp, end)
- **Scrollbar width (Task 1).** `end/Source/config/theme/gfx/theme.md:108-120` — `## scrollbar` gains `thickness | int | 8`, version unchanged. `jam/cast/style.md` — `@id thickness`. `jam/jam_style/jam_StyleCustom.cpp:256-262` — `getDefaultScrollbarWidth` reads END's `scrollbar/thickness` row under `isEND()`, else the STYLE `scrollBarThickness` metric. `jam/jam_plugin_bootstrap/layout/jam_PluginEditorLayout.cpp:697-702` — `addThemeTables` after `setValuesFrom`.
- **Drag regression (Task 2).** Root cause: bar visibility tied to keyboard focus; focus moves during the press, so the bar hid mid-drag. `jam_StyleCustom.cpp:269-302` — focus gates only the paint. `jam_TextEditor.cpp:58-65` — `applyScrollBars` sets `setScrollBarsShown (not isAlternateScreen, false)`.
- **Autoscroll (Task 3).** `jam_TextEditor.cpp` — `addBlock` and `removeHeadBlocks` no longer move the view; `addBlocks` follows the tail only when `isViewAtEnd()` was true before `setContentSize` (audit T4 replaced the bar-range read).
- **Alternate-screen bar (built, then discarded).** `PLAN-alternate-scrollbar.md` Steps 1-6 (Model rows `programTop`/`programLines`, buffer counting, `applyScrollBars` range, event-driven drag) were executed, tested on vim, and removed entirely on ARCHITECT's ruling. No trace remains in `jam/jam_terminal`, `jam/jam_gui/widgets`, `eve/Source`, the cast tables or `SPEC.md`.
- **Plan hygiene.** Deleted: `PLAN-alternate-scrollbar.md`, `PLAN-finder-3-modes.md`, `PLAN-finder-4-column-list.md`, `PLAN-finder-5-list-open.md`. `PLAN-finder-0-tracker.md` Status table marks Parts 3-5 superseded and deleted, and a second table tracks `PLAN-text-editor-absorption.md`, `PLAN-style-isomorphism.md`, `PLAN-alternate-scrollbar.md` (discarded) and `PLAN-Grand-Scheme.md`. `PLAN-text-editor-absorption.md` Steps 11 and 13 marked superseded, Steps 14-16 discarded.
- **Audit fixes, END.** `Source/config/` — `ConfigShader.{h,cpp}` and `ConfigTheme.{h,cpp}` split out of `ConfigModel` (one object, one TU); `addFileFlex` extracted; `addFileTables`/`addDefaultTables` public statics of `ConfigDirectory`; `findChildFiles` parenthesised init (Windows clang-cl); guards without a threat removed; `getRowValue` reads the row once. `lookAndFeel/ENDLookAndFeel` — `applyBarRotation` extracted, `applyColours` (was `initialiseColours`), `getCommonFont` inlined, dead `tab` event and redundant include removed, `newGraphics` built whole. `ui/TabView` — `getJoinCommands` (was `getJoinDirections`), `class Preview`. `plugins/Nexus` — `class`, const getters. `ui/EditorView` — direct pane parent. `ui/ENDView` — direct `setMouseConfig`. `cast/identifiers.md` — `visible` deleted. `SPEC.md`, `README.md` — synced to the code.
- **Audit fixes, jam.** `jam_gui` — `history.addOrReplace`, `copyPropertiesFrom`, one ±1 pair, typed `AnsiDocument` locals, destructor `= default`, `getLinkLabel` fills 52 labels per page, MessageOverlay bodies to the .cpp, `getSibling`/`isRotatable` const, dead `overlayAxisLine` member. `jam_markdown`/`jam_plugin_bootstrap`/`jam_style`/`jam_web`/`jam_graphics` — empty cells fail validation instead of asserting, result returns, `Format::appendWithSpace`, structured bindings, `jam::PluginHostType`, flex `fileCreated` reload, StyleManager and AttributedGraphics bodies to the .cpp, dead `colours` member, Typeface RAII for HarfBuzz and FreeType handles, stale font rows. `jam_markdown_graphics`/`jam_mermaid_diagram` — `Strings::getView`/`fromView`, `indexOfIgnoreCase`, `getMetrics<double>`, `addImage` deleted, dead `numEdges` and `setTreeRanks`, null-drawable bug, arrow-point and requirement-index bugs, namespace closers. `jam_terminal` — single assert per invariant, `placementsPerRow` sizing, `Id::end` in lines, in-place byte read, `stringTerminatorLength` from `.size()`; keyboard result returns, `Chars::` constants, constexpr tables; `cast/core.md` `fromUTF8Prefix`/`fromUTF8Suffix`. `jam_vulkan` — single `find`, positive-nesting failure blocks.
- **Audit fixes, EVE.** `EVETerminalProcessor` — `terminalTag` member everywhere, exact refresh period, braces. `EVEView` — `getOrCreateMarkdownState` (was `getMarkdownState`). `CLAUDE.md`, `SPEC.md`, `README.md` — synced (one `style.css`, every history block built, `reader` mode, Model row list, ANSI palette source, OPEN mode keys).
- **Audit fixes, small apps.** `stamp` — framework `Css::getValueTree`, result returns, one `getBoxProperty`, unused font removed. `whelmed` — unused fonts and palette entries removed. `cast`/`scout`/`ggwp` — style.css headers corrected, dead MermaidDiagram font ids removed.
- **Doxygen pass.** Every touched header in END, EVE, cast/scout/ggwp `Help.h`, jam_gui, jam_terminal, jam_markdown, jam_plugin_bootstrap, jam_style, jam_web, jam_graphics, jam_markdown_graphics, jam_mermaid_diagram, jam_core text and jam_vulkan, each block read against its .cpp; unverifiable claims removed.

### Alignment Check
- [x] BLESSED principles followed — every accepted audit fix cites its clause; every rejection cites the read that refutes it.
- [x] NAMES.md adhered — renames join their family (`applyColours`, `getJoinCommands`, `getSibling`, `getOrCreateMarkdownState`, `fromUTF8*`); no new domain term.
- [x] MANIFESTO.md principles applied
- [ ] Not every audit finding is resolved. The held set below needs ARCHITECT rulings (new object names under NAMES Rule -1, design choices, format changes).

### Problems Solved
- Drag broke because focus gated visibility; focus now gates only the paint.
- Autoscroll fought the user because block insertion and head removal moved the view; they no longer do.
- The alternate-screen bar flooded vim: JUCE's own range reset reached `scrollBarMoved` during the press, and one wheel report per row had a gain of `'mousescroll'` 3 (`neovim/runtime/doc/options.txt:4707`). Discarded by ARCHITECT.
- COUNSELOR errors corrected in-sprint: Auditor A2's G1 deleted the Step 28 column-list keys (ruling 32, `PLAN-text-editor-absorption.md:66`, `:325-328`) — restored byte-for-byte; A3 item 35 changed layout behaviour — reverted.

### State for Continuation
- Nothing is built. ARCHITECT runs cast for jam (`cast/terminal.md`, `cast/style.md`, `cast/core.md`, `cast/gui.md`), EVE (`parameters.md`, `component.md`), Whelmed (`component.md`), stamp (font row removed) and END (`cast/identifiers.md`), then a CMake re-configure for END (new `ConfigShader.cpp`, `ConfigTheme.cpp`), then builds every product. `jam/generated/jam_Core.{h,cpp}` were hand-edited for the `fromUTF8*` rename; cast regenerates them.
- Add `| thickness | int | 8 | ... |` by hand to `~/.config/end/themes/gfx/theme.md` `## scrollbar`.
- Doxygen XML is not regenerated: `<leader>bd` (jam), `ninja doxygen` (EVE, END); confirm zero warnings.
- Runtime checks: drag and autoscroll on the normal screen; OPEN mode `ctrl+shift+j`/`k`; whole-history reflow; a restored `flex/scrollbar.svg` reloads.
- Held for ARCHITECT (each with its read):
  - New object names (NAMES Rule -1): TextEditor split (A2 T1), MermaidDiagram/Layout/Graphics splits and one-object-per-TU (A4 1-3), ConfigDocument/Css/ViewManager/PluginEditorLayout splits (A3), ENDLookAndFeel typeface registry split (A6 21), Nexus jobs (A6 37), TerminalBuffer VT writer and TerminalSwap grid split (A1a 5, 16), Win32 key encoder (A1b 8), shared Help function (A5b 11).
  - Design: END theme in the Step 9 shape and `ENDLookAndFeel::setColours` without its pairing table (A6 1-6, 66-69; `SPRINT-LOG` open question); commands owned by TabView/EditorView (A6 41); the popup menu through `addCommandItem` needs human labels and enable tests in `getCommandInfo` (`ENDView.cpp:78-81`); unzip failure path (A6 34); TextEditor block index instead of history scans (A2 T6, ruling 41); one truth for read-only (A2 T2); gridStarts as Model rows (A5a 3); TerminalSwap wrapped-row join and possible allocation on the clock thread (A1a 10, 11); links sharing the image record lane (A1a 25); Css parse keeping the tier (A3 7); AttributedString stream format (A1b 19); Cmd in the keyboard modifier code (A1b 4); MessageOverlay message storage (A2 M3).
  - Facts: Windows `style` choices acrylic10/acrylic11/mica are not keys of `map::WindowFX` (`jam_Gui.h:233-234`); `EVEView.cpp:411` passes an empty callback to `set (Id::keys, ...)`; Whelmed harness path from `__FILE__` (A5b 1); `Session.h:137-158` doc claims (outside this scope).
  - The remaining clause-backed items that need a new helper name are HELD-NAME in the fix reports (for example A6 35, 44, 45, 52; A1a 8, 15, 17, 23, 24, 26, 30; A2 T13; A4 13-21).

### Debts Paid
- None

### Debts Deferred
- `DEBT-20261105T000000` — Windows runtime test (ARCHITECT, unchanged).

---

