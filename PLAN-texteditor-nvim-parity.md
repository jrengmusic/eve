# PLAN: TextEditor nvim parity — modes, buffer, swap, StatusBar

**RFC:** `eve/RFC-EVE-terminal-grand-scheme.md` (§3.3, §3.8, §3.9, §5.3 Gap 2, §7.4, §8)
**Date:** 2026-10-23
**Status:** approved by ARCHITECT (plan mode, 2026-10-23). The last step is the log.
**BLESSED Compliance:** verified against MANIFESTO, CODING, NAMES
**Language Constraints:** C++17 / JUCE 8 / JAM. LANGUAGE.md C++ L: one object, one TU; 300/30/3 is a smell detector, not a split rule (`LANGUAGE.md:25-38`).

## Overview

`jam::TextEditor` becomes an nvim-parity editor for core editing and navigation. Its state lives on an APVTS node through `jam::Model::Component`. A new `jam::StatusBar` shows the mode. The buffer gains line insert and remove, and the ruled swap tier. EVE loads its harness as an editable file buffer. `jam::Model::Component` stops holding a model, so plugins (APVTS) and END (`jam::Model`) bind components the same way. END and KANJUT conform.

## ARCHITECT rulings (verbatim)
- R1 *"vim core features parity first then. so we have interactivity, correct design by contract of TextEditor state machine with juce::ValueTree. and the harness file just become another file to load and edit."*
- R2 *"swap, this sprint."* R3 *"this will be also used as the terminal cache"*.
- R4 *"TextEditor state machine, (juce::ValueTree) will be attached to EVEProcessor APVTS"*.
- R5 *"NORMAL, INSERT, VISUAL mode … similar to endless "modal" mode, is just entering NORMAL mode, showing status bar, and can navigate selection in history … so user can select, copy, etc … eve terminal "NORMAL" screen is actually always in TextEditor INSERT mode"*.
- R6 *"IDENTICAL UX PARITY with neovim, but not ALL THE COMPLETE FEATURES. only core editing and navigation … our Viewport is true GUI juce::Viewport with scrollbar, so the way we handle "history" to render should adhere to juce native rendering contract, with through our PIPELINE"*.
- R7 *"isnt swap, buffer and cache, are also CORE DESIGN we need to be implemented?"*
- R8 *"jreng-filter-strip can be the reference, how gui widget can attach parameter. KANJUT and JAM plugin bootstrap are identical. JFS already QA tested"*.
- R9 *"0 alloc, 0 copy, 0 mutation … 0 temp containers, 0 handroll methods, 0 manual arithmetic … jam must be prioritized over juce … 0 new struct … 0 new pattern, 0 new / foreign semantics"*.
- R10 *"TextEditor is not written in stone … advanced-width agnostic … the design should never be hardcoded to any "terminal" nor "rich text" editor"*.
- R11 *"status bar should its own component. just write jam::StatusBar? i think it could use Model::Component, so you can easily "attach" its state? NO MANUAL state machine management allowed, no shadow state, no strays "variable parameters" allowed on widget."*
- R12 *"just add an overload? ValueTree is just ValueTree no?"*; *"what need to change? jam::Model? or all its family so could also attach to APVTS? END should conform"*.
- R13 Build permission to iterate only; *"execute the plan to completion until /log without any gate"*; *"FILTER AUDIT FINDINGS"*; *"dont overthink about 300-30-3 … it's THE PRINCIPLE"*.

## Scope (artifacts)
- **jam:**
  - `jam_data_structures/model/jam_Model.h` (`Model::Component`);
  - `jam_gui/layout/jam_OwnedComponent.h`, `jam_gui/widgets/jam_MessageOverlay.h`;
  - `jam_core/document/jam_Document.h/.cpp`, `jam_DocumentIndex.cpp`;
  - `jam_terminal/graphics/jam_TerminalEncoder.h/.cpp`;
  - `jam_gui/widgets/jam_TextEditor.h/.cpp`, new `jam_gui/widgets/jam_StatusBar.h/.cpp`, `jam_gui/jam_gui.h/.cpp`;
  - `cast/gui.md`, `cast/spell.md`, `generated/`, `user-modules-info.md`, `docs/`.
- **END:** `Source/end/ENDView.h/.cpp`.
- **eve:**
  - `Source/EVEView.h/.cpp`, `EVEViewComponents.cpp`, `EVETerminalProcessor.h/.cpp`;
  - `Source/layout/ViewLayout.md`;
  - `SPEC.md`, `RFC-EVE-terminal-grand-scheme.md`, `CLAUDE.md`.
- **KANJUT:** `cast --sync` of the kernel changes. Build jfs and kuassa.
- **Out:**
  - the TTY processor and terminal INSERT key forwarding;
  - ex commands (`:w`, `:e`), search, registers other than the clipboard, dot repeat, text objects, mouse editing;
  - host-session persistence of the swap (RFC:787 *"Daemon persistence remains END-era"*).

## Language / Framework Constraints
- One object, one TU (`CODING.md` TU placement). `StatusBar` is a new object with its own header and `.cpp`; `TextEditor` stays one TU.
- All includes at the module header. `jam_StatusBar.h` is included from `jam_gui.h` before `jam_TextEditor.h`, under the same guard as `jam_TextEditor.h`.
- Module rows: `jam_StatusBar.h/.cpp` are `provision`, like the TextEditor rows (ruling 7 *"KANJUT does not need TextEditor"*, `PLAN-launch.md:23`).

## Dependency & API Inventory (used to the fullest)
- **jam::Model**
  - `Model::Component<Derived>` (`jam_Model.h:143-206`): the node binding.
  - `Model::Attachment` (`:213-247`).
- **jam::AudioModel**
  - `getOrCreateComponentState (componentId)` (`jam_AudioModel.cpp:510-526`).
  - `setState` replaces the state (`:177-186`). `onSetState` rebinds the view components (`jam_PluginEditor.cpp:177-183`, `jam_ViewManager.cpp:841-864`).
- **Registry and ViewManager**
  - `registerComponent<T>` → `make.get (type)` with no arguments, then `setComponentID` and `Id::parameter` (`jam_ViewManager.cpp:157-161`).
  - `applyConfig` (`:143`, `:403-415`); `addBindings` → `bindToModel (parameterID, AudioModel&)` (`:235-241`, `jam_Registry.h:362-376`).
  - The JFS shapes: per-parameter `registerConfig` (`FilterStripViewComponents.cpp:58-80`); widget `bindToModel` (`jam_FrequencyTextBox.cpp:106-116`, `jam_VariComponent.h:94-104`).
- **juce::ValueTree** — `Listener` (`juce_ValueTree.h:489-546`), `setProperty` (`:251`). Key routing: the focused component, then its key listeners, then its parents (`juce_ComponentPeer.cpp:196-221`).
- **jam::Document**
  - `addChild (parent, after, id)`, `removeChild`, file-static `addSibling` (`jam_Document.cpp:426-483`).
  - `Index` (`jam_Document.h:482-594`, `jam_DocumentIndex.cpp`).
- **jam_terminal**
  - `AnsiDocument::addLines` (`jam_AnsiDocument.cpp:5-9`), line shape (`:172-218`).
  - Encoder `append*` family (`jam_TerminalEncoder.h:28-269`); `vtCoordinateBase { 1 }` (`:152`).
- **Text and style**
  - `AttributedString::append` ×2 (`jam_AttributedString.h:62`, `:75`).
  - `AttributedChar::isWordChar` (`jam_AttributedChar.h:343`), `spacerTail` (`:82`).
  - `ReflowDocument::getSegments` (`jam_ReflowDocument.h:60`); `GlyphArrangement::getAdvance (advances, range)` (`jam_GlyphArrangement.cpp:184-187`).
  - `StyleCustom::getMonoFont`, `getOrCreateMonoMetrics` (`jam_StyleCustom.h:60`, `:68`); `StyleManager::getInstance()->getConfig (sheet)` (precedent `jam_Viewport.cpp:51`).
- **Maps and tables** — `jam::Bimap::getDefault` returns the first authored row (`jam_Bimap.h:221-225`); `Function::Map`; `Bimap<int, juce::Identifier>` (`CommandId`, `gui.md:188-204`, `jam_Gui.h:288-309`).
- **JUCE**
  - `juce::UndoManager`, `UndoableAction`, `juce::SystemClipboard`.
  - `juce::MemoryMappedFile` readOnly, `juce::TemporaryFile`, `juce::FileOutputStream::flush`, `juce::SystemStats::getMemorySizeInMegabytes`.
  - `juce::KeyPress::createFromDescription` (precedent `jam_CommandManager.cpp:15`), `juce::Viewport::setViewPosition`.
- **Colour ids** — `juce::CaretComponent::caretColourId`, `juce::TextEditor::highlightColourId`, `textColourId`, `backgroundColourId`.
- **nvim parity reference** — `normal.c:187-375`, `ops.c:94-124`, `normal_defs.h:9-17`, `memfile.c:1-36`, `memline.c:5-36`.

## Design

1. **`Model::Component` binds a node, not a model (R12).**
   - Remove the `Model& model` member (`jam_Model.h:202`) and the `Model&` parameter of both constructors (`:154`, `:187`).
   - Add the overload `Component()` for registry-built components.
   - A registry-built component adopts its node in `bindToModel` by assigning `state`. JUCE redirects the listeners of a `juce::ValueTree` object on assignment (confirm by reading `juce_ValueTree.cpp` `operator=`).
2. **The conformers (R12).**
   - `OwnedComponent` initialises the base without the model.
   - `MessageOverlay` drops `m`, and `registerParameters (jam::Model&)` takes the model.
   - `ENDView` gets the member `ENDModel& model { *ENDModel::getInstance() }`, and calls `messageOverlay.registerParameters (model)`.
   - The other layout classes are unchanged.
   - KANJUT follows through `cast --sync`.
3. **TextEditor state = its APVTS node (R4, R11).**
   - `TextEditor` derives `Model::Component<TextEditor>` and `juce::ValueTree::Listener`.
   - `bindToModel` assigns `state = model.getOrCreateComponentState (getComponentID())`.
   - Properties on `state`: `Id::mode`, `visualMode`, `cursorLine`, `cursorColumn`, `anchorLine`, `anchorColumn`, `count`, `pendingOperator`.
   - No `juce::Value` and no state member. `valueTreePropertyChanged` dispatches through a `Function::Map`.
   - An empty mode is seeded with `map::EditorMode::getDefault()` (`insert`).
4. **`jam::StatusBar` (R5, R11).**
   - A `juce::Component` + `Model::Component<StatusBar>` + `juce::ValueTree::Listener`. It is a child member of the TextEditor and shares its node.
   - `paint` draws the mode and the 1-based ruler (`rulerBase { 1 }`) with `drawText`, `StyleCustom::getMonoFont()` and LookAndFeel colour ids.
   - The TextEditor shows it when the mode is not `insert`, on the bottom row. The content height adds that row while it shows.
5. **One buffer type.** The `AnsiDocument` line shape. `TextEditor::load (const juce::File&)` maps the file readOnly and calls `addLines` into the history lane. The display is the source lines.
6. **The harness is a file buffer (R1).**
   - EVE: `registerConfig<jam::TextEditor> (Id::textEditor, Id::toTag (Id::winsize), …load (getHarness())…)`.
   - `EVETerminalProcessor::load` and its print helpers are deleted. The chain stays.
7. **Keys are config data.** `ViewLayout.md` sheets `## normal`, `## insert`, `## visual` (`action | string | chord`), read per key with `StyleManager::getConfig`. An unbound key returns false.
8. **Actions are a table.** `map::EditorAction` + file-static `Function::Map actions`. The set is the SPEC Keyboard Summary plus counts. Semantics follow nvim.
9. **Line edits are state updates.** Build a complete `AttributedString`, swap it into `Id::cells`. Tokens, advances, rows and Index counts follow.
10. **Line insert and remove.** `Document::addChild (Element&, Element*, Element&)` relinks. `Index::insertLine`, `removeLine`.
11. **Undo.** One `juce::UndoManager`. `ReplaceLine`, `InsertLine`, `RemoveLine`. One transaction per normal command or insert session.
12. **Clipboard.** `juce::SystemClipboard`. A linewise yank ends with a newline. A paste of text that ends with a newline is linewise.
13. **Cursor and selection paint.** Index row tops + summed advances; `caretColourId` (block in NORMAL/VISUAL, bar `caretWidth` in INSERT); `highlightColourId` selection; `setViewPosition` keeps the cursor visible.
14. **Swap = Index residency tier (R2, R3, R7).** The ruled Sprint 122 design.
    - **Codec:** `encode`, `decode`, `getNumBytes`, `clear`.
    - **Budget:** 1/16 RAM; watermarks high = budget, low = 7/8.
    - **Store:** ANSI wire-byte spill through `terminal::appendLine` into one RAII `juce::TemporaryFile` per Index.
    - **Evictable:** cells, decoded images, runs. **Resident:** row, tokens, advances, image sizes, counts.
    - **Rehydrate:** `addVisibleRuns` rehydrates the visible lines.
    - **Fact:** `Document::source` is outside the tier.
    - **Fact:** the buffer and the swap end with the window.
15. **Generic widget (R10).** No markdown, no TTY, no EVE id in TextEditor. Modifiability is keymap data.

## Names (ratified with this plan)
- **jam_data_structures:** `Model::Component()`.
- **jam_core:**
  - `Document::addChild (Element&, Element*, Element&)`;
  - `Index::insertLine`, `removeLine`, private `removeChild (Entry&, int)`, `getPath (int)`;
  - tier: `Codec { encode, decode, getNumBytes, clear }`, `setBudget`, `getResidentBytes`, `evict`, `rehydrate`, `setCold`, `addSpan`, `getLeftmostHotPath`, Entry `span`, `isCold`.
- **jam_terminal:** `terminal::appendLine (juce::OutputStream&, const Document::Element& line, const AnsiDocument& document)`.
- **jam_gui `StatusBar`:** class `jam::StatusBar`; constant `rulerBase`.
- **jam_gui `TextEditor`:**
  - `load`, `bindToModel`, `keyPressed`, `valueTreePropertyChanged`;
  - private `setLine`, `addLine`, `removeLine`; nested `ReplaceLine`, `InsertLine`, `RemoveLine`;
  - members `undoManager`, `statusBar`;
  - file-static `actions`, `properties`, `getText (const AttributedString&, juce::Range<int>)`;
  - constants `caretWidth`, `budgetDivisor { 16 }`, `lowWatermarkRatio`.
- **cast `gui.md`:**
  - Id rows `cursorLine`, `cursorColumn`, `anchorLine`, `anchorColumn`, `visualMode`, `count`, `pendingOperator`, `normal`, `insert`, `visual`, `statusBar` (only those no manifest has).
  - Bimaps `EditorMode` (insert, normal, visual), `VisualMode` (character, line, block), `EditorAction` (left, down, up, right, wordForward, wordBackward, wordEnd, lineStart, lineEnd, documentStart, documentEnd, halfPageUp, halfPageDown, insertBefore, insertAfter, insertLineStart, insertLineEnd, openLineBelow, openLineAbove, leaveMode, visualCharacter, visualLine, visualBlock, yank, delete, change, deleteCharacter, pasteAfter, pasteBefore, undo, redo, splitLine, deleteBackward).
- **END:** `ENDView::model`.
- **eve:** file-static `getHarness()`.

## Validation Gate
COUNSELOR validates each step before the next, by reading the changed files against MANIFESTO, NAMES, CODING and this plan. Build only to iterate. The bar is 0 errors and 0 warnings. @Auditor runs once, after the final code step. Each finding is filtered against a CONTRACT clause by a read.

## Steps
0. Plan of record (this file).
1. `Model::Component` binds a node; OwnedComponent, MessageOverlay and ENDView conform; END builds.
2. Vocabulary in `cast/gui.md`, regenerated.
3. `Document::addChild` relink; `Index::insertLine`, `removeLine`.
4. `terminal::appendLine`.
5. Residency tier in `Document::Index`.
6. `jam::StatusBar`.
7. TextEditor state, buffer, edits, undo, tier wiring.
8. Cursor, selection, StatusBar layout.
9. Keys and actions.
10. EVE wiring and documents.
11. Audit (filtered), `cast --sync` KANJUT, jfs/kuassa builds, doxygen.
12. Log.

## Amendment A — content in the Processor (2026-10-24)

### ARCHITECT rulings (verbatim)
- A-R1 *"scrollback budget always cap from settings."*
- A-R2 *"content SSOT should not owned by TextEditor, since session preservation is part of SPEC, widget only materialise and render. the actual content should live in Processor"*
- A-R3 *"bug with no fix? what the fuck"*: the image line drift (audit 5) is fixed at its root.
- A-R4 *"what is tier? what's the most BLESSED approach? why do MANIFESTO cant answer your question?"*, *"also consider /Users/jreng/Documents/Poems/kuassa/user_modules/TETRIS.md"*, then `/go`.

### Contract basis
- MANIFESTO E rule 1: *"One responsibility"* (`MANIFESTO.md:231`). The Index finds geometry, and the swap holds residency. These are two jobs, so they are two objects.
- MANIFESTO B: *"Every resource, object, and dependency has exactly one owner"* (`:37`). The swap holds content cells, so the content owner owns it.
- Layout state: ARCHITECT ruled *"Layout state: **On the line as Model value data**"* (`jam/carol/SPRINT-LOG.md:708`). Advances, row tokens and row heights stay on the line elements as Model value data. The widget's layout writes them as a state update (MANIFESTO E: *"A state update is not mutation"*, `:256`). The lookup `Document::Index`, glyph runs and decoded images are the widget's materialisations (MANIFESTO E, Materialisation, `:279`).
- TETRIS T: *"Visualization pulls; ProcessorChain never pushes"* (`TETRIS.md:99`). TETRIS Cold vs Hot: *"Cold state lives in ProcessorChain"* (`:246`).
- MANIFESTO D: *"The same input must always yield the same output"*. A true-colour id is `terminalColourId + RGB` (`jam_TerminalTrueColours.cpp:5-18`), so every document gives the same id. Decode and edits parse in their own `AnsiDocument`, and the lane's parser pen never moves for them. This is A-R3.

### Design
1. **`Document::Swap`** (jam_core, nested Document family of `Document::Index`, NAMES Rule 5). It is extracted from `Document::Index`.
   - It owns the `Codec`, the budget, one `juce::TemporaryFile`, the span of each spilled line, the cold lines and the count of resident lines.
   - Its surface: `setBudget`, `addLine`, `removeLine`, `evict (Document&)` and `rehydrate (Element&)`.
   - Eviction walks the document chain from the first line and spills hot lines until the low watermark. Line order has one source, the chain.
2. **`Document::Index`** is geometry only: lines ↔ rows ↔ pixels. Codec, tier members and the two-argument constructor are deleted.
3. **`jam::TerminalProcessor`** (jam_terminal) owns the content. That is the history `AnsiDocument`, the live `AnsiDocument` and the `Document::Swap`.
   - `pull()` parses the FIFOs into its own documents, tells the swap about each new history line, and tells it to evict.
   - It loads a file (`load`), the Convolution model of RFC §3.8.
   - It takes content tells (add, remove and rehydrate a line) and the budget.
4. **`EVETerminalProcessor`** registers these as `userInterfaceGetters` under `Id::textEditor`, the established component-to-processor channel (`jam_ViewManager.h:42`):
   - a visitor that hands the documents to the caller;
   - the content tells.

   It sets the swap budget from the settings row `HISTORY_CAPACITY` and loads the harness.
5. **`jam::TextEditor`** owns no content. It owns its lookup `Document::Index` over the Processor's lines, glyph runs, decoded images, the `UndoManager` and the node state.
   - It reaches the content only through the visitor getter.
   - Its layout writes advances and rows onto the lines, the ruled layout state.
   - It tells edits, and rehydrates of the lines it reads, through the getters.
6. **Settings.** ARCHITECT: *"what scollback budget? not scrollback capacity?"*. The swap's cap is the existing `HISTORY_CAPACITY` setting (`ViewLayout.md`, `## settings`), read in `EVETerminalProcessor::prepare`.
   - Each history row from the engine is one `Id::line` (`jam_AnsiDocument.cpp:292-300`), so the swap counts resident lines. It keeps `HISTORY_CAPACITY` hot lines and evicts down to 7/8 of them.
   - The Codec has no byte measure.
   - No second budget setting exists (MANIFESTO S).

### Steps
- A1. `Document::Swap` extracted. `Document::Index` returns to geometry. Deterministic true-colour ids are done.
- A2. `jam::TerminalProcessor` owns the content, the swap, `load` and the content tells.
- A3. `EVETerminalProcessor` registers the getters, sets the swap budget from `HISTORY_CAPACITY` and loads the harness. The EVE TextEditor config is deleted, and `getHarness` moves to `EVETerminalProcessor.cpp`.
- A4. `jam::TextEditor` reads the content through the getters, owns the geometry and materialisations, and tells edits and rehydrates.
- A5. SPEC, RFC and CLAUDE.md amended. EVE, END, cast and jfs build. The Auditor runs once. `cast --sync` KANJUT. Doxygen.
- A6. Log.

## Amendment B — decouple: live in the Processor, history in the TextEditor (2026-10-24)

ARCHITECT: *"decouple. Processor is live buffer, TextEditor will hold history at least that's how the terminal designed. so SIGWINCH never ever destroy history content."*

1. **`jam::TerminalProcessor`** owns only the live document.
   - `bool pull (AnsiDocument& historyDocument)` appends the history FIFO bytes to the caller's history document, and replaces its own live document when live bytes are ready. It returns `true` when it replaced the live document.
   - `getLiveDocument()` returns the live document.
   - The history document, the swap, `load`, the capacity and the content tells leave the Processor.
2. **The EVE getter** (key `Id::toType (Id::winsize)`) takes `(AnsiDocument& historyDocument, const std::function<void (AnsiDocument&, bool)>& visitLive)`. It calls `pull (historyDocument)`, then `visitLive (getLiveDocument(), isLiveReplaced)`. The view drives every pull, so the Processor replaces the live document only inside a call that the view makes. The view's live index never outlives the live document (MANIFESTO B). The TextEditor knows no `TerminalProcessor` (R10).
3. **`jam::TextEditor`** owns the history:
   - the history `AnsiDocument`, its geometry `Document::Index` and its `Document::Swap`;
   - `load (const juce::File&)`, which parses in its own document and adopts the lines;
   - `setCapacity (int)`;
   - the edits and undo, which act on its own history document.

   SIGWINCH reflows the history and never replaces it.
4. **EVE** configures the TextEditor at build: `load (getHarness())` and `setCapacity` from `HISTORY_CAPACITY`.

### Amendment B corrections (COUNSELOR, from reads)
1. **Capacity through the pull, not the config.**
   - The config lambda that reads the model must capture the view. `SharedInstance` reads: *"First holder's args construct; later holders join and their own args are discarded"* (`jam_SharedInstance.h:1-3`). So a second EVE instance runs the first view's lambda, and that view can be destroyed (MANIFESTO B: *"No object outlives its owner"*).
   - JFS and EVE register only captureless lambdas (`FilterStripViewComponents.cpp:40-217`, `EVEViewPanelCallbacks.cpp:5-15`).
   - Thus the visitor becomes `(AnsiDocument& liveDocument, bool isLiveReplaced, int historyCapacity)`. The EVE getter reads `HISTORY_CAPACITY` from its own model at each pull, so the cap always follows the settings.
   - `TextEditor::setCapacity` loses its only caller and is deleted. The EVE config is captureless and calls only `load (getHarness())`.
2. **`load` parses into the history document.**
   - Design 5: *"calls `AnsiDocument::addLines` into the history lane"*.
   - `Document::appendChildren` does not move the true colours of an `AnsiDocument` (`jam_Document.cpp:502-526`). A file parsed in a scratch document therefore loses its colours.
   - The config runs after the LookAndFeel (`jam_ViewEditor.h:121`, `:127`), the parent (`jam_ViewManager.cpp:139`) and the bounds (`:142`). So `load` lays out at once.
3. **The history document is a member, `AnsiDocument historyDocument`.** The live lane never owned a document, and CODING Memory prefers *"1. Stack allocation"* over `std::unique_ptr`.
4. **`addLines()` is history-only.** It has only history callers. It lays out the new lines, adds them to the index and the swap, then evicts. `load` and the pull share it.
5. **The live lane after a LookAndFeel change.** The engine writes the live area only on scroll or resize (`TerminalGraphicsEngine.cpp:545`). So `terminalSource` takes `bool shouldSetLane`. `lookAndFeelChanged` pulls with `true`, and the live lane gets new advances in the same call.

### Audit B triage (COUNSELOR, each by a read)
**Accepted and fixed:**
- 1, 2: edits stop the byte parse and `appendChildren`.
  - `setSpanOffsets` shifts the cell-space `Id::tokens`/`Id::row` (`jam_Document.cpp:513`, `:599-619`; `jam_AnsiDocument.cpp:205-217`).
  - Adopted bytes stay before `tokenStart` (`jam_Document.cpp:648`).
  - The fix is Design 9: edits build cells with `AttributedString::append` (`jam_AttributedString.h:62`, `:75`) into a new line element, and `AnsiDocument::addSegments` becomes public.
  - `Element::add` appends (`jam_Document.h:453-458`), so the edit unit is a fresh element. This is why `ReplaceLine` has no use.
- 6: the same fix makes control bytes in text literal cells.
- 17: the temporary vector goes with the fix.
- 7 and 24: per-tick `AudioModel::getValue` allocates a `juce::Value` (`jam_AudioModel.cpp:79-83`; R9 "0 alloc"). The first fix, a `juce::Value` member, failed at runtime: it bound at construction to an invalid tree (`jam_ModelUtils.cpp:104-130`), and `replaceState` replaces the tree (`jam_AudioModel.cpp:181`). The fix is `getHistoryCapacity()`, which reads `jam::Model::getChildWithID (…).getProperty` at use, with no `juce::Value`.
- 8: the true-colour push runs only when lines were added, the live document was replaced, or the lane is set again.
- 13: `getLiveDocument() const`.
- 19: no sentinel accumulator.
- 32: `Swap::capacity` becomes a plain int (no cross-thread threat).
- 23: doc mismatches are fixed in the doxygen pass.

**Rejected:**
- 3: Design 6: *"The chain (`prepare`, `process`, `setWinsize`, FIFOs, engine) stays for the TTY sprint"*. `TerminalFIFO::write` asserts (`jam_TerminalFIFO.cpp:39`).
- 4: the precondition is asserted at entry (CAROL Code Contract: *"Preconditions use assert"*). `getHarness()` is a ratified name.
- 5: Design 9 makes the cells the edit unit, and the byte path wrote cells only, so there is no regression.
- 9: a covered line paints the images of earlier lines (`paintImages` finds the owner line upward). Eviction of decoded images would break the paint of a visible covered line whose owner is off screen. This corrects Design 14's evictable list.
- 10: the evict runs in the same call (`lookAndFeelChanged` → `terminalSource (true)` → `addLines()` → `swap.evict`). Before the bind the capacity is unlimited (`jam_Document.h:865`).
- 11: every delete path keeps one line (`jam_TextEditor.cpp:511`). EVE loads at config (`jam_ViewManager.cpp:143`) before the bindings (`:241`). No consumer without content exists (MANIFESTO L, YAGNI).
- 12: `component.md:8` holds one textEditor row (YAGNI).
- 14: the established getter pattern repeats the signature at registration and call (`jam_Registry.h:351-358`).
- 15: R13 *"dont overthink about 300-30-3"*; R1 and Amendment B item 3 put the state machine and the history in the TextEditor.
- 16: Design 5 (one buffer type, the ANSI line shape) and Design 14 (terminal encoder).
- 18: the caller tells and the swap decides (MANIFESTO E rule 4, *"Tell, don't ask"*).
- 20: `TerminalFIFO::pull()` returns `std::unique_ptr`.
- 21: `juce::UndoManager::perform` takes a raw owning pointer.
- 22: the clause names pointers; references bound at construction are the established pattern (`jam_Analyzer.h:59`).
- 25: no clause; the parse follows the bytes.
- 26: columns are the first children of a parsed line (`jam_AnsiDocument.cpp:188-195`).
- 27: redo relinks the same element (Design 10).
- 28: the tick refresh is the pull pacing (TETRIS T); the clause covers listeners.
- 29: `AnsiDocument::addLines` takes `int` (`jam_AnsiDocument.h:74`).
- 30: Design 14 Fact.
- 31: names changed by later rulings:
  - *"StatusBar should be renamed to TextEditorStatusBar"*;
  - *"NO FUCKING terminal namespace. DELETE. write functions at its correct domain"*;
  - *"not scrollback capacity?"*;
  - `delete` is a C++ keyword;
  - `cursorWantedColumn`, `operatorCount` and `pendingChord` join the `cursor*`, `count` and `pending*` families (NAMES Rule 5).
- 32 (silent read): the jassert+if form was ruled in audit A.

### Runtime corrections (COUNSELOR, from reads and runtime evidence)
1. **Focus.**
   - JUCE `TopLevelWindow` wants focus itself (`juce_TopLevelWindow.cpp:51`). A traced launch showed the window holding focus, so keys never reached the TextEditor.
   - `EVEView` grabs focus in `callAsync` after construction (END pattern, `ENDView.cpp:28-42`).
   - `EVEView::focusGained` forwards it to `createKeyboardFocusTraverser()->getDefaultComponent (this)` (endless pattern, `Display.cpp:212`).
   - `debug::Log` confirmed that every key reaches `TextEditor::keyPressed`.
2. **Status row (corrects Design 4).** ARCHITECT: *"status bar collide with the text."*
   - Neovim: `w_height` is the *"number of rows in window, excluding status/command line(s)"*, and `w_status_height` is the separate row (`neovim/src/nvim/buffer_defs.h:1200-1204`).
   - JUCE: a `Viewport`'s visible area is its bounds minus the scroll bars (`juce_Viewport.cpp:374`, `:394-410`), and it has no inset API. R6 requires the JUCE-native Viewport contract.
   - So the TextEditor becomes a `juce::Component` that holds a `jam::Viewport viewport` and the `TextEditorStatusBar` as siblings. The status row is cut from the bottom of the bounds outside insert mode, and the viewport takes the rest.
   - The viewport does not want keyboard focus, because JUCE's default is `true` (`juce_Viewport.cpp:165`). Thus the TextEditor keeps the keys.
   - The viewed `Content` reports scrolls through `Component::moved()`.
3. **Edit line shape.** `TextEditor::addLine` gives the new line an empty `Id::images` list (the parser's shape, `jam_AnsiDocument.cpp:252`). `setImageRows` reads it (`jam_TextEditor.cpp:1279`), and `Element::get` throws on an absent key (`jam_Document.h:309`).
4. **Bimap holders.** The TextEditor and its status bar own `SharedInstance` holders of the editor bimaps (`jam_FrequencyTextBox.h:168` pattern).
5. **Capacity read.** `EVETerminalProcessor::getHistoryCapacity()` reads the current state tree at use.

## Execution State
- Step 0: done.
- Steps 1–11: done; audit resolved except audit 2 (rejected, MANIFESTO E creation), 5 (A-R3), 28 (A-R1) and 29 (A-R2).
- Amendment A: done, then replaced in ownership by Amendment B.
- Amendment B: done with its corrections. Audit B is triaged and fixed. EVE, END and jfs build 0/0. The KANJUT sync is at fixpoint, and doxygen gives 0 warnings for jam and KANJUT.
- Step 12: logged (jam Sprint 171, eve, END Sprint 91).
