# PLAN: EVE terminal protocol — one envelope, one path; measure and decode

**RFC:** none — objective from ARCHITECT's prompts (session 2026-10-29); design pass by ORACLE, verified by COUNSELOR
**Date:** 2026-10-29
**Amended:** 2026-10-30 — the Preview foundation is folded in (ARCHITECT: *"then that's THE FOUNDATION need be implemented on your PLAN in this sprint, FOLD"*)
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM (LANGUAGE.md "C++ / JUCE": one object, one translation unit)

## Context

ARCHITECT: *"next sprint is 2-5"* (`jam/carol/SPRINT-LOG.md:62-66`):
- 2. Audit #11 — *"Measure readiness ticks against reads under `cat` with the profiler build"* (`:63`).
- 3. Replay — *"run `scout benchmark/replay.md`"* (`:64`).
- 4. Decode — *"Decode is the next limit … the zero-work floor is 51.4 s"* (`:65`).
- 5. `DEBT-20261025T214955` (`eve/DEBT.md:11-15`), paid as the foundation of EVE's own terminal protocol. Scope: *"+ preview + image"*.

ARCHITECT rulings for item 5 (verbatim, closed):
- *"this is our open seam to transport, communicate, open API via shell scripting hence, any CLI … can use the exact consistent ONE path protocol"*; *"the actual "RENDERING" never ever touch the terminal itself, but an actual juce::Component widget, always GUI side."*
- *"image streamed as bytes carried from terminal to widget (message thread) is inevitable, but always unidirectional to message thread."*; *"decode only carry metadata, and materialisation always at message thread, this could be boosted with parallel job"*.
- *"change the marker "END" to "EVE""*.
- *"we also need "readback" … not for everything … still under UNIDIRECTIONAL contract. for example File Tree widget, might need to send set cwd command to terminal."* — GUI→terminal uses the existing input lane; no reply column this sprint.
- iTerm2 `File=` without a cell size: *"Read pixel size"* — header pixel size, no decode, converted with the cell size.
- Preview: *"for our own protocol the preview widget could be anywhere, but planned as the right side ephemeral dock"*; *"for third party CLI, it's layout exactly where the CLI expect terminal to render we politely hijack the bytes. terminal render nothing, GUI render the actual preview (image), on terminal cell layout"*.
- *"all state machine that drives/driven by terminal "command" should be registered as non-automatable parameter to Model"*; *"all operations/methods from/to terminal registered to CommandManager as SSOT callback"*; *"CommandManager functions could be triggered by events from Parameter, assigned keyboard shortcut, anything"*.
- *"winsize dicated by Viewport available space … REFLOW, wrapping or unwrapping are NEVER ever affected by SIGWINCH nor winsize. it's always GUI exclusive getLocalBounds() only. winsize is TERMINAL DOMAIN."*
- Docs: *"EVE now is Embeddable Viewport Endpoint"*; *"EVE is the fastest modern terminal emulator built with lock free audio architecture discipline"*; *"should not document endless at all"*; README license *"MIT"*.

ARCHITECT rulings 2026-10-30 (verbatim, closed). They supersede the Preview dock lines of 2026-10-29 that COUNSELOR wrote without an ARCHITECT quote ("the view moves to the HTML flex lane"), which are deleted:
- Goal: *"running "ls" command turn EVE into FULL FEATURE parity with Finder/Explorer"*; *"list of files could be clicked, navigate with keyboard shortcut (jump flash, this actually already implemented on endless)"*; *"an optional (user-config-driven), left side panel file tree, and right side preview column content (images, text content, etc)"*.
- Sprint scope: *"i dont think this could be implemented on single sprint. BUT since you have started the state, let's make THE FOUNDATION correct, and document the vision into grand-scheme"*.
- Preview is a component: *"Preview need component, not inlined. Because we also want gif to be animated, that's need timer."*; *"either way, the component could be SSOT, exactly ONE path, on API, only differs what dictate the layout, and what command tell them to be visible."*
- TextEditor: *"TextEditor inline image are for Rich Text rendering such as Markdown or inline mermaid. different contract, does not overlap. Or Preview as an Overlay could possibly, entirely replace TextEditor inline image. TextEditor only "reserve the space", to reflow, wrap, unwrap, etc."* The question *"which is actually better design … render back to terminal as pixel (inline on TextEditor) or dedicated Preview component? (this should answer the conflict)"* was answered with the dedicated Preview component, cited, and ARCHITECT folded it.
- Takeover: *"we capture the fzf state, listen (fully event driven) to whatever terminal command it communicate with tty, get the layout, and politely "hijack" the rendering. we render into our own "Preview" overlay component layout exactly on that box"*; *"it's layout in cell, perfectly scaled always fit into whatever fzf set the layout, maintaing its aspec ratio"*; *"if any application use Sixel, Kitty, iTerm2 image protocol EVE will take over"*; *"we dont reinvent new protocol, we only take over terminal bytes and render them on actual GUI widget, at message thread, with currently exist widely adopted terminal image protocol, our default is iTerm2."*
- Threads: *"we never choke terminal as lock free hot path. Message thread free to spawn parallel job, pipe to Vulkan immediately one shot to memoized (ephemerally, when the "widget") exists, but still respecting the encapsulation. meaning juce::Component contract still exactly native juce API calls, never poke, never acknowledge Vulkan or any rendering enginer downstream."*
- Kitty: *"restore kitty back from past commit, we still need to provide the lane to support natively third party CLI."*; *"shell integration for SkiT protocol meaning we must also ready for CLI that uses kitty image protocol"*. This supersedes *"Kitty is ruled dead"* (`PLAN-Grand-Scheme.md:911`) and the Kitty deletion of Step 5.
- EVE `image` operation: *"keep for now, but NOT for native Preview (Component)."*

Facts behind the fold (read this session):
- fzf draws no image. It passes the Kitty APC `\x1b_G`, Sixel DCS `\x1bP…q` and iTerm2 `\x1b]1337;` bytes of its preview command through (fzf `src/terminal.go:87-99`, `:5132`). Its script runs `kitten icat --unicode-placeholder` (`bin/fzf-preview.sh:61-68`), `chafa` (`:70-72`) or `imgcat` (`:76-81`). It exports the box as `FZF_PREVIEW_COLUMNS`, `FZF_PREVIEW_LINES`, `FZF_PREVIEW_TOP`, `FZF_PREVIEW_LEFT` (man page `:887-894`).
- The tokenizer knows `text`, `csi`, `osc`, `control`, `unterminated` only (`jam/cast/terminal.md:63-83`): no DCS (Sixel) and no APC (Kitty) envelope.
- The Kitty decoder at jam HEAD parses `U=1`, `c=`, `r=` (`jam_TerminalKittyDecoder.cpp:336-338`, `:182-244`) and has no line for the placeholder character U+10EEEE. It decodes full RGBA and allocates on the reader thread (`.h:62-73`, `:151-155`).
- Materialisation today is serial on the message thread (`jam_TextEditor.cpp:1187-1201`, `jam_TerminalSwap.cpp:287-309`). The one parallel job is `ImageLoader::registerImages` for BinaryData (`jam_ImageLoader.cpp:755-767`). `ImageLoader` decodes one frame and memoizes by content hash (`jam_ImageLoader.h:14-20`). `loadImageSequenceNative` gives every GIF frame with its delay (`jam_TerminalImageSequence.h:65-90`).

## Design

### Envelope (one grammar)
```
ESC ] 1337 ; File= <args> : <base64> BEL|ST              iTerm2, third party
ESC ] 1337 ; EVE ; <operation> ; <args> [ : <base64> ] BEL|ST   EVE
args      key=value *( ; key=value )     TerminalITerm2Decoder::forEachArgument grammar (jam_TerminalITerm2Decoder.cpp:15-25)
command   name=<map::CommandId name>     (jam/cast/gui.md:202-238)
preview   path=<absolute path>           empty path dismisses
image     [width=…;height=…] : base64    PNG | GIF | JPEG → the dock
```

### One path on the tick (owner: `jam::TerminalProcessor`)
- OSC 1337 joins the OSC 133 sibling family (`jam_TerminalProcessor.cpp:152-158`, `:610-631`; cast `terminal.md:891-905`, `:1012-1043`, `:1189-1220`): `oscFunctions` routes `iterm2Image`; a byte table on the first byte after `1337;` routes `F` (File=) and `E` (EVE;); the EVE arm checks the `EVE;` prefix and dispatches the operation through a file-static `jam::HashMap<std::string_view, int>` (pattern `jam_AnsiDocument.cpp:393-397`).
- An OSC 1337 longer than the read buffer streams across reads: payload mode with bounded scalar state (header specs, base64 carry, header-reader state, image number). The terminator finishes it. Today such a token is dropped (`jam_TerminalProcessor.cpp:99-101`).
- File= and EVE image payloads are base64-decoded 4→3 on the tick into a lane, with no allocation. JUCE's `Base64::convertFromBase64` reads one null-terminated string and cannot resume (`juce_Base64.h:56`), so the processor owns the incremental step. Fields use `juce::ByteOrder` (`juce_ByteOrder.h:103-119`).
- Header pixel size (no decode): PNG bytes 16-23 big-endian; GIF bytes 6-9 little-endian; JPEG SOFn after APPn segments skipped by length (Librarian brief). Cell rectangle per iTerm2 (`VT100InlineImageHelper.m:332-463`, defaults `VT100Terminal.m:3952-3956`): `N` cells, `Npx` ceil(px/cell), `N%` ceil(grid×pct), `auto` ceil(image/cell), clamp ≥ 1, width > grid−cursorX → grid width with height scaled, height ≤ 255. Pixels are physical pixels: the unit of the winsize that EVE reports to the PTY (`jam_TerminalTTY.h:131-140`, `jam_TextEditor.cpp:1003-1005`).
- File= finishes with one placement `{destination, image number}` in the grid, normal or alternate screen. The cursor advances by the rows through `addLine`. When the ring lacks rows, the terminator stays pending and the tick ends drained. The writer frees rows and asks for a process (`EVETerminalProcessor.cpp:138-142`). The PTY holds the input (`SPEC.md:196`).

### Lanes (all `jam::TerminalFIFO`, sized in `audioDeviceAboutToStart`, `EVETerminalProcessor.cpp:184-191`)
| Lane | Writer | Content | Reader | Drain |
|---|---|---|---|---|
| `input` (exists, `.h:193`) | message thread | key/mouse bytes | tick | `process()` |
| `images` | tick | `{int64 number, int32 size}` + decoded bytes; size 0 = end, −1 = release | processor serial context | `handleAsyncUpdate`, before `writePendingData()` |
| `commands` | tick | `name\0` per command | message thread | `EVEView` on the `commandCount` Model change |

Image bytes cross twice: permitted copy 2 (the handoff into the lane) and permitted copy 1 (the write into the record file, the OS boundary). This is the same as the history rows (ring → segment, `jam_TerminalSwap.cpp:116-142`).

### Record (`jam::TerminalSwap`, the SSOT that outlives the view, `SPEC.md:168-172`)
- Image store beside the line store: segment files plus an image index, `{segment, position, size, ownerLine}` per image (family `.index/.segment/.live`, `jam_TerminalSwap.h:99-101`, `:167-169`).
- `writeLine` sets `ownerLine` from the placement row with `y == 0`. `removeHeadFiles` deletes an image group when each entry is finished and either released or owned by a line below `firstLine`.
- `addLine` gives the image element its number. The Preview reads the bytes through `swap.getImage (number)` on the message thread (amended 2026-10-30; until Step 12f, `TextEditor::getImage` does it). A frame that reaches the view before its bytes gets an invalid image, and the reader asks again on the next vblank.

### Grid holds layout only
`ImagePlacement { jam::Cell::Rectangle destination; juce::int64 image; }` replaces `{destination, juce::MemoryBlock png}` (`jam_TerminalBuffer.h:75-86`). The TerminalGraphicsContext inline branch (`jam_TerminalGraphicsContext.cpp:488-499`, `:523-526`) and `TerminalBuffer (bool)` go. No product uses them: EVE builds `TerminalBuffer{true}` (`EVETerminalProcessor.h:198-200`) but paints through no TerminalGraphicsContext. cast Help uses the default `false` (`cast/Source/Help.cpp:9`, `:32`). The alternate grid gains placements (today none, `jam_TerminalBuffer.cpp:128-132`).

### Commands and state (ARCHITECT's mental model)
- `command`: the tick writes the name into `commands`, then stores `++commandCount` (a `## model` int row) through `events` (`EVETerminalProcessor.cpp:90-97`). The vblank flush writes the changed count (`jam_Model.cpp:33-58`). `EVEView` (a ValueTree listener of the model, `jam_PluginEditor.h:30`) drains `commands`. For each name it calls `commandManager->invokeDirectly (map::CommandId::getInstance()->get (juce::Identifier { name }), false)`. This is the call of `CommandManager::invokeKey` (`jam_CommandManager.cpp:45`), made on the message thread as JUCE requires (`juce_ApplicationCommandManager.cpp:192-196`). A repeated command changes the count each time, so it invokes each time. This pays the DEBT in its stated shape.
- `preview` / `image`: state in `## model` rows. `preview` is a new `text` row type in ModelLayout (`jam_ModelLayout.cpp:16-18`; `Model::createAndAddParameter<ParameterText>`, `jam_Model.h:1038-1042`). `previewImage` is an int64 row holding the image number. The tick stores them through a new lock-free `ParameterText::store` (family `jam_ParameterBase.h:176-180`). `ParameterText::setValue` builds a `juce::String`, which allocates (`jam_ParameterText.h:76`).
- The dock binds `juce::Value` to those properties (pattern `EVEViewComponents.cpp:73-75`). A new `preview` CommandId with a `ctrl+shift+P` keys row (`SPEC.md:253`) toggles it through its registry callback (pattern `EVEViewPanelCallbacks.cpp:17-21`).

### Preview — one component, two layouts
- **One component, the SSOT.** `Preview` is a `juce::Component` in `jam_gui/widgets` (sibling `jam_ImageComponent.h`, `jam_TextEditor.h`). It materialises one image source on the message thread and paints it with native JUCE calls only. It never names Vulkan or any engine.
- **Sources.** A record image number (third-party takeover: `TerminalSwap::getImage` bytes, `jam_TerminalSwap.cpp:287-309`) or a file path (native: the `preview` Model row, `parameters.md:41`). The `previewImage` row does not feed the native Preview (ARCHITECT, above).
- **Materialisation.** Still images through `ImageLoader` (memoized by content hash, `jam_ImageLoader.h:14-20`). Animated GIF through `loadImageSequenceNative`, all frames and delays (`jam_TerminalImageSequence.h:65-90`), played and looped by the component's own timer. The frames live only while the component exists. The decode may run as a parallel job, and the component paints when it completes.
- **Fit.** The image always fits the component bounds and keeps its aspect ratio (`juce::RectanglePlacement`, as `jam_ImageComponent.h` uses).
- **Layout 1, overlay.** For a takeover, the Preview lies exactly on the cell box that the CLI asked for. The grid placement `{cell box, image number}` (`jam_TerminalBuffer.cpp:172-193`) is the box. The TextEditor reserves the space: it raises the rows to the box (`jam_TextEditor.cpp:1145-1173`) and keeps reflow, wrap and unwrap. The TextEditor no longer materialises or paints terminal images (`jam_TextEditor.cpp:1176-1201`, `:1544-1551` leave).
- **Layout 2, dock.** The native Preview is the right-side column. The `preview` CommandId (`jam_Gui.h:358`) with a `ctrl+shift+P` keys row (`SPEC.md:253`) toggles it.

### Third-party takeover — one lane per envelope, one path after it
- Each protocol envelope is a token type: OSC (iTerm2, exists), DCS (Sixel, new `dcs`), APC (Kitty, new `apc`) — family `csi`, `osc` (`jam/cast/terminal.md:63-83`).
- Each decoder carries metadata only on the tick: the pixel size and the cell box. The image bytes go through the one `images` lane into the record (`EVETerminalProcessor.cpp:130-146`, `jam_TerminalSwap.cpp:128-161`). The decode to pixels is the Preview's, on the message thread.
- Kitty is restored from jam HEAD (`git show HEAD:jam_terminal/image/jam_TerminalKittyDecoder.{h,cpp}`) and reshaped to this contract: no allocation and no RGBA decode on the tick. Kitty actions `a=T`, `a=t`, `a=p`, `a=d`, `a=q`, chunks `m=1`, virtual placement `U=1` with `c=`/`r=`, and the placeholder cells of `--unicode-placeholder`. A query reply goes through the existing reply stream (`jam_TerminalProcessor.cpp:46-53`).
- Sixel routes its DCS envelope to `TerminalSixelDecoder` (`jam_terminal/image/jam_TerminalSixelDecoder.h`) under the same contract.

### Shell scripts
EVE's integration scripts gain an `eve` function: `eve preview <file>`, `eve command <name>`, `eve image <file>`. Each one emits the envelope. The precedent is END's `END preview` (`endless/Source/terminal/shell/zsh_end_integration.zsh:21-28`). The scripts are zsh (`Source/shell/zsh_integration.zsh`), bash (`bash_integration.bash`) and PowerShell (`powershell_integration.ps1`).

## New names (NAMES Rule -1 — approval of this plan approves this list)
| Name | Kind | Family |
|---|---|---|
| `map::OscFunction::iterm2Image`, `oscFunctions` row | cast rows | `terminal.md:1012-1026`, `:1189-1203` |
| `Iterm2Image` {`file`, `eve`}, `Iterm2ImageFunction`, `iterm2ImageFunctions` | cast tables | `ShellIntegration` triple, `terminal.md:891-905`, `:1028-1043`, `:1205-1220` |
| `EveOperation` {`command`, `preview`, `image`} | cast table | **new domain term** |
| `TerminalProcessor::eveMarker`, `iterm2ImageConstruction`, `eveConstruction`, `payloadState`, `imageCount` | members | `deviceAttributesReply`, `*Construction` (`jam_TerminalProcessor.h:85`, `:107-122`) |
| `Id::commandCount`, `Id::preview`, `Id::previewImage`, `Id::command` | identifiers / model rows | `eve/cast/identifiers.md`; `cursor` row `parameters.md:37` |
| `Id::text` ModelLayout row type | identifier | `boolean/choice/integer` (`jam_ModelLayout.cpp:16-18`) |
| `ParameterText::store`, `Model::storeValue (…, const char*, int)` | methods | `Parameter<T>::store` (`jam_ParameterBase.h:176`) |
| `TerminalFIFO::writePendingData (juce::OutputStream&, int maxBytes)` | overload | `jam_TerminalFIFO.h:73-75` |
| `TerminalSwap::getImage`, `imagesPerIndex`, `imageIndexEntryBytes`, `imagesExtension`, `imageIndexExtension`, `ownerLine` | members | `addLines`, `linesPerIndex`, `indexEntryBytes`, `*Extension` |
| `released` (ownerLine sentinel) | constant | **new domain term** |
| `TerminalBuffer::getWinsize`, `prepare (TerminalWinsize, int)` | members | `getBounds` (`jam_TerminalBuffer.h:174`) |
| `TerminalITerm2Decoder::getCellRectangle` | static | `isInline` (`.h:102`) |
| `preview` CommandId (`0x4600027`) + keys row | rows | `clear` (`gui.md:231`, `ViewLayout.md:52`) |
| `Preview` | component | ARCHITECT's word: *"our own "Preview" overlay component"*; sibling `jam::ImageComponent` |
| `AnsiTokenType::dcs`, `AnsiTokenType::apc` | cast rows | `csi`, `osc` (`jam/cast/terminal.md:79-80`) |
| `TerminalKittyDecoder` | class (restored) | jam HEAD |
| `eve` shell function | script | END's `END()` |
| `benchmark/cat.md` | manifest | `burn.md` |

## Validation Gate
Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

Every @Engineer brief restates: Design by Contract per CODING.md CRITICAL RULES; MANIFESTO **E** data-flow contract and deadline-thread rule verbatim; doxygen-first; no comments or doxygen before the audit; zero names outside this plan's list. END safety: kill only `$!`; END pid unchanged; `window.state` / `END.cache` SHAs match the backups.

## Steps

### Step 0: Plan to the project root
**Action:** COUNSELOR writes this plan to `eve/PLAN-eve-terminal-protocol.md`.
**Validation:** content matches.

### Phase A — measure (items 2, 3, 4)

### Step 1: Control and replay
**Scope:** `benchmark/control.md`, `benchmark/replay.md` (unchanged).
**Action:** COUNSELOR runs `scout <abs>/benchmark/control.md`, then `scout <abs>/benchmark/replay.md`.
**Validation:** both exit 0; `replay-report.md` dated today; crash-report count unchanged. Item 3 closed.

### Step 2: Audit #11 under `cat`
**Scope:** new `benchmark/cat.md` (sibling of `burn.md`, `profiler.args` = `-c "cat <abs>/scratchpad/ptyfloor/seq100M.txt"`).
**Action:** COUNSELOR writes the manifest, runs `scout`. Kernel sampling on.
**Validation:** report gives `ttyBytesReady`, `terminalRead`, `deviceClock` calls and ns. Verdict recorded with numbers: readiness ticks per read, and ticks that read 0 bytes. Item 2 closed (measure, per `SPRINT-LOG.md:63`).

### Step 3: Fresh live profile
**Scope:** `benchmark/live.md` (unchanged).
**Action:** COUNSELOR runs `scout benchmark/live.md` with kernel sampling.
**Validation:** decode time = `terminalProcess` − `terminalRead`, per epoch and per byte; `dropped` reported; top user and kernel stacks listed.

### Step 4: Decode — remove the measured top cost
**Scope:** the files the Step 3 profile names inside `jam_terminal/processor`, `jam_terminal/document`, `jam_core/text`.
**Action:** COUNSELOR derives the change by the Lean ladder (auditor.md): removal, lookup, contract correction, then split. @Engineer applies it.
**Validation:** `live.md` re-run: decode ns per byte lower than Step 3. `seq 1 100000000` median below 53.668 s. `replay.md` placements and text identical.

### Phase B — protocol (item 5), delete first

### Step 5: Delete the dead seam
**Scope:** `jam_terminal/image/jam_TerminalSkit.h/.cpp`, `jam_TerminalKittyDecoder.h/.cpp` (Kitty ruled dead, `SPRINT-LOG.md:10467-10468`), `TerminalITerm2Decoder::decode` (`.h:104-115`, `.cpp:41-108`), the include lines (`jam_terminal.h:129-130`, `jam_terminal.cpp:43`, `:46`), cast rows `imageDecoded` and `previewFile` (`terminal.md:45`, `:52`).
**Action:** @Engineer deletes. The compiler lists what remains.
**Validation:** jam builds; no reference left (LSP + doxygen). Sixel, ImageSequence, Gif and PendingImage stay untouched (not in scope).
**Amended 2026-10-30:** the Kitty deletion is reverted by Step 12c (ARCHITECT: *"restore kitty back from past commit"*).

### Step 6: Grid holds layout only
**Scope:** `jam_TerminalBuffer.h/.cpp`, `jam_TerminalGraphicsContext.h/.cpp`, `EVETerminalProcessor.h/.cpp`.
**Action:** Delete `TerminalBuffer (bool)`, `shouldUseInlineImages`, `isUsingInlineImages`, the inline branch of `drawImage`, and `TerminalGraphicsContext::addImagePlacement`. Then make these changes:
- `ImagePlacement {destination, image}`.
- `addImagePlacement (destination, juce::int64)`.
- `writeRow` writes 4 ints and 1 int64.
- `getNumBytes` drops `pngBytes`.
- Placements on the alternate grid.
- `prepare (TerminalWinsize, transitRows)` with `getWinsize()`.
- The EVE call at `EVETerminalProcessor.cpp:230`.

**Validation:** builds; cast `Help` output byte-identical; live slot size smaller.

### Step 7: Lanes and record
**Scope:** `jam_TerminalFIFO.h/.cpp`, `jam_TerminalSwap.h/.cpp`, `jam_TextEditor.h/.cpp`.
**Action:**
- `TerminalFIFO::writePendingData (OutputStream&, int maxBytes)`.
- The image store in TerminalSwap: files, index, `getImage`, `ownerLine` in `writeLine`, group deletion in `removeHeadFiles`.
- `addLine` gives the image number.
- `TextEditor::getImage` reads through the swap.
- `addLanes` asks again for invalid images.

**Validation:** writing N images and lines, then raising `firstLine`, deletes exactly the released or old groups.

### Step 8: Cast tables
**Scope:** `jam/cast/terminal.md`, `jam/cast/gui.md`, `eve/cast/identifiers.md`; regenerated headers.
**Action:** Add the rows of the names list (`OscFunction`, `oscFunctions`, the `Iterm2Image` triple, `EveOperation`, the `preview` CommandId, the identifiers). Then run `cast`.
**Validation:** generated tables compile against the `*Construction` bounds.

### Step 9: The tick
**Scope:** `jam_TerminalProcessor.h/.cpp`, `jam_TerminalITerm2Decoder.h/.cpp`.
**Action:**
- Payload mode.
- Incremental base64.
- Header reader for PNG, GIF and JPEG.
- `getCellRectangle`.
- Placement and cursor advance.
- Writes to the `images` and `commands` lanes.
- `events.get` for `commandCount`, `preview` and `previewImage`.

`forEachArgument` becomes a template, so no `std::function` remains on the tick.
**Validation:** replay a captured `imgcat` stream through `TerminalMemoryTTY`, split at arbitrary read boundaries. Placements are identical. The instrumented tick shows zero allocations.

### Step 10: Model text and store
**Scope:** `jam_ParameterText.h`, `jam_Model.h`, `jam_ModelLayout.cpp`, `eve/Source/layout/parameters.md`.
**Action:** Add `ParameterText::store`, the text overload of `Model::storeValue`, and the ModelLayout `text` type. Add the model rows `commandCount`, `preview` and `previewImage`.
**Validation:** `store` takes no lock and does no allocation; the flush writes on the message thread.

### Step 11: EVE wiring
**Scope:** `EVETerminalProcessor.h/.cpp`, `EVEView.h/.cpp`.
**Action:**
- Size the lanes.
- `handleAsyncUpdate` drains `images` first.
- `registerEvents` additions.
- `EVEView::valueTreePropertyChanged` drains `commands` and calls `invokeDirectly`.

**Validation:** `printf '\e]1337;EVE;command;name=clear\a\e]1337;EVE;command;name=clear\a'` clears twice.

### Step 12a: Grand scheme records the vision
**Scope:** `eve/PLAN-Grand-Scheme.md`.
**Action:** New §3.11 "File browser and Preview" [ADDED 2026-10-30] with the 2026-10-30 rulings verbatim (goal, file list, layout, Preview SSOT, takeover, fit, threads, Kitty). §5.3 gap list: the Preview component, the File Tree, the `ls` link navigation (prior art `endless/Source/terminal/LinkManager.h:5-121`). §6: fzf preview of a PNG and a GIF in its box; `ls` parity with Finder. §7: *"Preview as an Overlay could possibly, entirely replace TextEditor inline image"*. Amend `:907-914` and `:911` (Kitty restored; third-party images are taken over, never rendered by the terminal).
**Validation:** each added line carries its ARCHITECT quote.

### Step 12b: DCS and APC envelopes
**Scope:** `jam/cast/terminal.md` (`AnsiTokenType`), `jam_AnsiDocument.h/.cpp` (`getToken`), `jam_TerminalProcessor.h/.cpp` (`screenConstruction`).
**Action:** Read `AnsiDocument::getToken` first. Add `dcs` and `apc` token types; the token runs to its string terminator, as `osc` does. `screenConstruction` gains their rows. An unterminated DCS or APC image envelope streams across reads through the one open-sequence path of Step 9 (`applyImagePayload`).
**Validation:** builds; a captured `chafa` (Sixel) and `kitten icat` (Kitty) stream splits into the expected tokens.

### Step 12c: Kitty restored, metadata only
**Scope:** `jam_terminal/image/jam_TerminalKittyDecoder.h/.cpp` (restored from jam HEAD), `jam_terminal.h`, `jam_terminal.cpp`, `jam_TerminalProcessor.h/.cpp`.
**Action:** Librarian first: the Kitty graphics protocol spec for `a=`, `m=`, `U=1`, `c=`/`r=`, `i=`, `q=` and the placeholder cells (U+10EEEE with its row and column diacritics, the image id in the foreground colour). Restore the files, then reshape: parse the keys on the tick with no allocation; payload chunks go through `applyImagePayload` into the `images` lane; `a=T` and `a=p` add a placement `{cell box, image number}`; `U=1` placements resolve the placeholder cells to the same placement; `a=q` replies through the reply stream; transmit-only ids map to image numbers in storage sized in `prepare`.
**Validation:** builds; the instrumented tick shows zero allocations; `kitten icat --unicode-placeholder` replayed through `TerminalMemoryTTY` gives the same placement as an iTerm2 `File=` of the same image.

**12c design (2026-10-30, from the Kitty spec and kitty v0.49.2 source; Librarian brief, local copies in the session scratchpad `kitty/`):**
- `kitten icat` sends raw pixels: `f=32`/`f=24` with `s`/`v`, often `o=z` (icat `transmit.go:46-72`; spec `rst:279-299`, `:371-382`). `f=100` is PNG (`rst:363-364`). So each record image carries its encoding: format, pixel width, pixel height, compression. The tick reads them from the keys; it decodes nothing (ARCHITECT: *"decode only carry metadata, and materialisation always at message thread"*). iTerm2 images carry the encoded format with the pixel size the header reader already reads.
- `Preview` materialises by the encoding: encoded bytes through `loadImageSequenceNative`; raw RGBA/RGB through the per-row conversion; `o=z` inflated first (`juce::GZIPDecompressorInputStream`, zlib format).
- Behaviour follows kitty v0.49.2 source, as the iTerm2 decoder follows iTerm2's (precedent `VT100InlineImageHelper.m:332-463`): cell box `ceil ((width + X) / cellWidth)` × `ceil ((height + Y) / cellHeight)` when `c`/`r` are absent, aspect from the one given otherwise (`graphics.c:1074-1101`); cursor `x += cols`, `y += rows − 1`, none when `C=1` or `U=1` (`:1423-1428`); a reply only when `i` or `I` is set, `q=1` drops OK, `q=2` drops every reply (`:1036-1053`); BEL ends an APC (`vt-parser.c:407-424`).
- Placeholder cells (`U=1`): the virtual placement keeps `{image id, c, r}`. A cell with U+10EEEE carries the image id in its foreground colour (low 24 bits) and the third diacritic (high byte), the row and column in the first two diacritics (`rst:753-793`; table `rowcolumn-diacritics.txt`, 297 entries, the one `kitten icat` encodes with, icat `rowcolumn_diacritics.go`). The tick turns the cell of row 0, column 0 into one placement `{cell box c × r, image number}`; placeholder cells draw nothing.
- Out of 12c: animation frames `a=f`, `a=a`, `a=c` (`rst:1056-1163`); transmission media other than `t=d`.

**12c sub-steps:**
- 12c-1: record encoding (`TerminalImageEncoding`, packed like `TerminalWinsize`; lane header, index entry, `getImageEncoding`); `Preview` materialises raw RGBA/RGB and `o=z`; iTerm2 and EVE images carry the encoded format.
- 12c-2: restore the two files from jam HEAD; APC parse on the tick, no allocation: keys, `m` chunks through `applyImagePayload`, `a=T` placement and cursor, `a=t`/`a=p` with transmit-only ids mapped to image numbers in storage sized in `prepare`, `a=d`, `a=q` and the replies.
- 12c-3: `U=1` virtual placements and the placeholder cells.

### Step 12d: Sixel route
**Scope:** `jam_TerminalSixelDecoder.h/.cpp`, `jam_TerminalProcessor.h/.cpp`.
**Action:** Read the decoder first. The DCS envelope goes through the same path: the raster size on the tick, the bytes to the `images` lane, the pixel decode on the message thread.
**Validation:** a `chafa` Sixel stream gives a placement at its cell box.

### Step 12e: The Preview component
**Scope:** new `jam_gui/widgets/jam_Preview.h/.cpp`, `jam_gui.h`.
**Action:** The component of the design section: one source (record image number or file path), still images through `ImageLoader`, GIF frames and delays through `loadImageSequenceNative` with its own timer, fit with aspect ratio, native JUCE calls only.
**Validation:** a PNG, a JPEG and an animated GIF paint and fit any bounds; the GIF loops; nothing on the tick changes.

### Step 12f: Overlay and reserved space
**Scope:** `jam_TextEditor.h/.cpp`, `EVEView.h/.cpp` (the owner, ARCHITECT: *"Siblings via owner"*).
**Action:** The TextEditor keeps the row raise for a placement (`jam_TextEditor.cpp:1145-1173`) and drops terminal image materialisation and paint (`:1176-1201`, `:1544-1551`). One Preview per visible placement lies exactly on its reserved cell box.

**12f design (2026-10-30, from reads):**
- 12f-1, tick: a cell write (`addTextRun`, `jam_TerminalProcessor.cpp:293-323`) or a partial blank (`setBlankCells`, `:358-382`) over a placement removes that placement from every row it covers and releases its image. Today only a full-width blank clears placements (`:366-367`), so fzf's redraw of its box with text keeps the old image (and `placementsPerRow` then drops new ones, `jam_TerminalBuffer.cpp:185-191`). Family `removeCells`, `removeHeadFiles`: `TerminalBuffer::removeImagePlacements (jam::Cell row, juce::Range<int> columns)`.
- 12f-2, overlay: the TextEditor keeps, per visible image element, its box in its own coordinates (the rect it already fits, `jam_TextEditor.cpp:1155-1167`, placed as paint places it, `:1544-1551`) and tells the owner the complete set after each layout or scroll through a callback (family `onKeyPress`, `onMouse`): `onImages`. EVE binds it beside `onKeyPress` (`EVEViewComponents.cpp:38-48`). EVEView replaces its overlay set with the told set: an image already shown keeps its Preview (moved), a new one gets a Preview whose bytes the terminal processor applies (`EVETerminalProcessor::applyImage`, family `applyCommands`: `swap.getImageData` + `getImageEncoding`), an image not told any more loses its Preview, so its frames leave with it (ARCHITECT: *"memoized (ephemerally, when the "widget") exists"*).
**Validation:** fzf with `imgcat`, `kitten icat` and `chafa`: the selected PNG and GIF fill the preview box with their aspect ratio; the GIF plays and loops; moving the selection replaces the Preview; the terminal never stalls.

### Step 12g: Native dock
**Scope:** `eve/Source/layout/ViewLayout.md` (keys row), `EVEView*.cpp`, `EVEViewPanelCallbacks.cpp`. EVEView splits its bounds in `resized` (ARCHITECT: *"EVEView, like overlays"*).
**Action:** The native Preview binds the `preview` path row. The `preview` CommandId callback toggles it.
**Validation:** `eve preview <png>` and `eve preview <gif>` show in the dock; an empty path hides it; `ctrl+shift+P` toggles it; the TextEditor bounds follow, so the GUI reflows; the tick is untouched (`SPEC.md:253-255`).

### Step 13: Shell scripts
**Scope:** `Source/shell/zsh_integration.zsh`, `bash_integration.bash`, `powershell_integration.ps1`.
**Action:** Add the `eve` function: `preview`, `command` and `image`.
**Validation:** each emits the exact envelope (verified with `od -c`).

### Step 14: Third-party images end to end
**Action:** ARCHITECT runs EVE (Build: agents build, ARCHITECT tests). The senders: `imgcat` (iTerm2: cells, `px`, `auto`), `kitten icat` (Kitty, with and without `--unicode-placeholder`) and `chafa` (Sixel), with PNG, GIF and JPEG; then fzf with `bin/fzf-preview.sh`; then a full-screen CLI on the alternate screen.
**Validation:**
- Each image shows in a Preview at its cell rectangle, fitted with its aspect ratio; a GIF plays and loops.
- Text after an image lands below it.
- Scrollback shows the images after the live area has left.
- The deleted groups follow the capacity.

### Phase C — documents, audit, doxygen

### Step 15: SPEC and project documents
**Scope:**
- `eve/SPEC.md`: the protocol, the lanes, the dock and the mental model, each quoted. Line 73 gains OSC 1337.
- `eve/README.md` (new): readme-writer skill, ARCHITECT's words, MIT, no endless.
- `eve/comparison.md`: endless rows and column removed.
- `eve/carol/SPRINT-LOG.md:3`: "EVE — Embeddable Viewport Endpoint".
- `DEBT.md` stays until the log.

**Validation:** each edit carries its ARCHITECT quote; no other text changes.

### Step 16: Build, control, measure
**Action:** @Engineer builds Release and Profile, with 0 warnings. COUNSELOR runs `control.md`, `live.md` and `replay.md`.
**Validation:** control exits 0. The `seq` median is no worse than Step 4. No new crash report.

### Step 17: Audit, doxygen
**Action:** @Auditor sweeps once, and COUNSELOR filters per DCF. Then the doxygen pass, with 0 warnings, including the stale ITerm2Decoder prose (`jam_TerminalITerm2Decoder.h:23-24`, `:56-57`, `:69-76`). Log on ARCHITECT's word.

## Step 17 audit resolution (2026-10-30)

The Auditor reported 104 findings. COUNSELOR validates each against the cited code and resolves it this sprint (CAROL DCF §5).

**ARCHITECT ruling:** #89 — *"Percent-encode values"*. The shell percent-encodes `%`, `;` and control bytes in a `key=value` value, and the tick decodes it in place.

**Batches:**
- A1 (done): #34, #35, #36 (placements), #29, #28, #18, #17, #48 (reset release).
- B2a: the `images` lane moves into `TerminalBuffer`, the owner of the tick's transit stores beside the row ring. This gives:
  - back-pressure as for rows (#77);
  - one lane layout (#47);
  - releases written direct, with no staging (#37, #36 releases);
  - `prepare (TerminalWinsize, …)` and `getWinsize` per Step 6 (#10).
- B2b: the record: entry writes by field after publish (#49), `setImageEntry` split (#43), `writeImages` (#44), the entry layout once (#46), late releases (#51), the long functions (#45, #38).
- B3: `ParameterText` single-writer handoff (#67), and #68-#70, #72.
- B4: the image takeover leaves `TerminalProcessor` into its own object (#1), with #2, #4-#9, #11-#14, #16, #19 and #104. The EVE operations dispatch by `HashMap<std::string_view, int>` per the Design (#6).
- B5: the decoders: #20-#27, #30-#33.
- B6: the view: Preview path source and `ImageLoader` stills per the Design (#56-#59, #61, #62), #63-#66, commands and images applied by EVEView (#76, #80), #82, #84-#88, `commandCount` to EVE's cast (#75, part).
- B7: the shell: percent-encoding (#89), and #90.
- B8: Sixel, pre-existing and now in use: #91-#99.
- B9: #53-#55, #73, #100, #103.

**Rejected, with the read that refutes each:**
- #3: `addTextRun`'s cached row and placements pointers live within one call. MANIFESTO S: *"Transient state only … anything that lives and dies within a single operation"*. It has 3 if-decisions.
- #15: the plan line concerns `forEachArgument`. `events` (`Function::Map`) is the existing tick event mechanism, for example `Id::cursor` (`jam_TerminalProcessor.cpp` `applyEvents`).
- #50: the mapped file into a `MemoryBlock` is the OS-boundary read, MANIFESTO E permitted copy 1.
- #60: playback state is the materialisation's own. MANIFESTO E: *"A stateful runtime object is itself the materialisation"*.
- #71, #74, #101, #102: family names (NAMES Rule 5). `Id::string` and `Id::int64` follow the `@id` name-equals-value rows (`jam/cast/core.md:103-143`).
- #75 (part): `Id::command`, `Id::preview` and `Id::previewImage` are used by jam's tick (`jam_TerminalProcessor.cpp` `eveConstruction`). Only `commandCount` is EVE-only.
- #79: the Model is the SSOT, so a read of a Model row asks no object for its state.
- #81: the getter signature is the framework's `ViewManager::Getters` pattern (`jam_TextEditor.cpp:64-71`).
- #10: the processor's `winsize` is a calculation input, synced from the Model by `reset`. MANIFESTO S: *"DSP processor parameters look like state but are not machinery state — they are calculation inputs … Processor values are a synced working copy"*. `TerminalBuffer::prepare` keeps `(cols, rows, transitRows)`, because cast and scout call it.
- #37: `releasedImages` lives within one tick: filled by `clear` and `removeImagePlacements`, written out at the end of `applyTokens`. MANIFESTO S: *"Transient state only … anything that lives and dies within a single operation"*.
- B2a adjusted: the `images` lane moves to `TerminalProcessor`, its one writer and the owner of the tick's buffers, not to `TerminalBuffer`.
- #83: `commandCount` counts commands sent and only grows. The lane's ready count drops to 0 at each drain, so no truth is copied (`SPEC.md:496-497`).

## BLESSED Alignment
- **B:** each lane has one writer and one reader. The record owns the image bytes, and the grid owns the placements.
- **L:** every operation is a table row, and no branch chain exceeds 3. TerminalSkit's mixed responsibilities are deleted.
- **E:** one envelope and one path. Every state is a Model row. Every operation is a CommandManager command.
- **S (SSOT):**
  - The operation names are `map::CommandId` and the `EveOperation` table.
  - The wire vocabulary is `TerminalITerm2Decoder`.
  - The image bytes live only in the record.
  - The pixel unit is the winsize.
- **S (Stateless):** the payload state is decoder working state, the same as `pendingBytes`. Everything else lives in the Model.
- **E (Encapsulation):** data flows one way: PTY → tick → lane → consumer → Component. No reply this sprint.
- **D:** replay split at arbitrary reads gives identical placements. Steps 1-4 and 16 measure.

## Risks / Open Questions
Two points no read and no ARCHITECT quote decide yet. Step 12f and Step 12g wait on them:
1. **Overlay owner (Step 12f) — closed 2026-10-30.** ARCHITECT: *"Siblings via owner"*. The Previews are siblings above the TextEditor, owned by EVEView (or the view), laid out by the owner from the placement boxes; the TextEditor exposes no child.
2. **Dock layout (Step 12g) — closed 2026-10-30.** ARCHITECT: *"EVEView, like overlays"*. EVEView owns the dock Preview as a sibling of the view and splits its bounds in `resized`: the view takes the left part, the dock the right column. jam's ViewEditor stays unchanged.

No open question remains.

## Verification (end to end)
1. `scout benchmark/control.md`, `replay.md`, `cat.md`, `live.md`: all exit 0, and the reports are dated.
2. In EVE: run `eve command clear` twice, then `eve preview <png>`, then `eve preview <gif>` (the dock; the GIF loops).
3. Run `imgcat`, `kitten icat` and `chafa` (PNG, GIF, JPEG), fzf with `bin/fzf-preview.sh`, and a full-screen CLI on the alternate screen. Each image shows in a Preview on its cell box.
4. Scroll back past the images, then run `clear`. The scrollback images show, and the image files follow the capacity.
5. Run `seq 1 100000000`. The median is below 53.668 s.
