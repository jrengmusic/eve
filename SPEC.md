# EVE Specification v0.0.1

**Companion:** RFC-EVE-terminal-grand-scheme.md (architecture, evidence, non-negotiables — normative for all architectural questions; this SPEC does not repeat it)

## Overview

**Purpose:** A terminal application whose content is never destroyed — not by
SIGWINCH, not by window resize — and whose surface composes with Finder-grade file
navigation, built as a JUCE application on the MVP audio-plugin architecture.

**Target End-User:** ARCHITECT (daily driver), then general release.

**Core Workflow:** Launch EVE → shell session in the center terminal Component →
navigate the filesystem via docked sidebars (vim keys) exactly like Finder/Explorer
→ preview files → open with configured tools → resize/rearrange freely, content
always intact, full history always reachable.

**Mental model (binding):** EVE is a JUCE application, NOT a terminal. Every widget
is a `juce::Component`; all state belongs to `jam::Model`-family state (scalar/mode)
and the Document SSOT (content). The terminal is one Component — the center core —
among ours.

## Technology Stack

- **Language:** C++17
- **Frameworks:** JUCE + JAM (jam_terminal, jam_graphics, jam_vulkan, jam_style, jam_markdown, jam_core)
- **Renderer:** jam::vulkan::LowLevelGraphicsContext (GPU), juce software renderer as oracle
- **Build:** self-contained single-file CMakeLists (CAST target oracle), CMake + Ninja
- **Platform:** macOS (Intel + ARM) and Windows; identical DX
- **Shape:** standalone application first; CLAP plugin hosted by END later (RFC §4.3)
- **Config:** markdown tables + CSS (CAST conventions) — no lua

## Core Principles

1. **MVP** — Model holds state; Processor (the clock's tick, audio discipline) owns
   terminal emulation; View Components are dumb, paint-only, own nothing.
2. **Host-plugin threads** — the clock's tick reads and writes the PTY and
   decodes the bytes into the engine rows; the message thread lays out and
   paints. Lock-free FIFOs (`juce::AbstractFifo`) join them. EVE starts no
   thread. The clock and the history writer run on the serial event contexts
   of `jam::AsyncUpdater`: a serial GCD queue on macOS, a pool of one thread
   on Windows (`jam_AsyncUpdater.cpp:11-34`). ARCHITECT: *"why cant we use AUDIO thread as the
   READER thread?"*; *"Input: READER, Audio Thread, TTY. Output: Terminal LLGC. also
   Audio thread … Renderer is Vulkan … message thread"*.
   - **The TTY backend clocks the device in the PTY's rhythm.** Each readiness
     event is one tick; its payload is the tick's numSamples: on macOS the bytes
     the PTY holds, on Windows the completed read plus the bytes still queued in
     the pipe. After the tick, `jam::TerminalClock` waits one period on the
     clock's context; the macOS backend then re-arms readiness while the PTY is
     connected, and the Windows backend's next overlapped read is the re-arm.
     The period follows the measured traffic rate and never exceeds one vblank,
     so the reader never races the writer. ARCHITECT: *"One vblank"*;
     *"the rhythm synchronization IS THE ABSTRACTION to implement"*; *"buffer
     size should be STILL dictated by hardware == how fast native tty can
     deliver"*; *"why no TerminalClock? i just wanted to ensure nothing is
     hardcoded, nothing is manual state management, fully event driven"*.
   - **The clock is the chain's own.** `EVETerminalProcessor` owns its clock,
     `jam::VirtualDevice`, and is the clock's `juce::AudioIODeviceCallback`.
     The tick (`audioDeviceIOCallbackWithContext`) runs the chain. Above the
     seam, JUCE's contract holds: the host tells `EVEProcessor::prepareToPlay`
     and `releaseResources`, and `processBlock` carries audio only. The wiring
     is the same in every host. ARCHITECT: *"EVETerminalProcessor should own
     the VirtualDevice, hence prepareToPlay upstream still juce contract, we
     only take over the control at seam we need to drive ourselves"*; *"VirtualDevice
     exists as our clock, since the architecture must use a device"*; *"it
     SHOULD NOT MATTER. what binary we're using as host-client. VirtualDevice
     as full throttle clock is EVE SPEC contract"*.
3. **Non-destruction contract** — widget content is never destroyed by SIGWINCH or
   resize. Only an explicit `clear` (ED 2 / ED 3 semantics, below) wipes the
   *screen* — and history scrollback is still retained (tmux-like).
4. **Terminal conformance at processor level** — VT100 spec correctness as the
   foundation, plus the xterm/modern extensions jam_terminal already encodes
   (SGR truecolor, bracketed paste, progressive keyboard, mouse SGR, OSC 7/8/133),
   and the image envelopes EVE takes over: OSC 1337 (iTerm2 `File=` and EVE's own
   envelope), DCS (Sixel) and APC (Kitty graphics).
5. **Finder truth is the filesystem** — sidebars and file list read the FS
   directly, keyed off OSC 7-reported cwd. Terminal output text is never parsed.
6. **Style roles in state; colours at materialization** (RFC §8).
7. **Two state lanes, one tree.** ARCHITECT: *"AudioModel for true audio
   parameter lane, as automatable parameter, while Model lane for any event
   driven state machine that is non automatable, real time safe on its own
   thread, guaranteed thread safe on other thread, always concurrent with
   atomic, think of it like reverse direction of APVTS."*
   - The **AudioModel lane** (APVTS) holds only parameters that a host can
     automate. Their rows are in `Source/layout/parameters.md`. ARCHITECT:
     *"parameters.md always for automatable parameter, a parameter can host can
     automate. this is plugin_bootstrap (that synced identically with KANJUT)
     contract that JFS use."*
   - The **Model lane** (`jam::Model`) holds the terminal state machine: the
     DEC modes, the cursor, the OSC 133 marks and every later shell
     integration state. Its rows are the `## model` table of
     `Source/layout/parameters.md`, and
     `jam::ModelLayout` builds it. ARCHITECT: *"we only need to build
     jam::Model as Terminal complete state machine, all terminal events
     including OSC 133 and future shell integration is jam::Model parameter,
     still attached to APVTS hence it's a valid stateInfo that the framework
     guarantee for instance serialisation and state preserverance"*.
   - The Model tree is one child of the APVTS state, so the host serialises
     it with the plugin state. It is not a host parameter. ARCHITECT:
     *"what i really mean with non-automatable parameter is PARAMETER tree
     still attached to APVTS but it's not audio parameter, still could be
     event driven with valueTreePropertyChanged."*
   - Direction: the clock produces, the view follows. ARCHITECT: *"EVE is
     the other way around, everything starts at Audio thread, it's processor
     events that drives how widget rendered."*
8. **No second copy of a truth.** ARCHITECT: *"no second copy of truth
   allowed."* A row's atom count is its content end; no counter stands beside
   it. The decoder's working state is calculation state; the Model value is
   the one permitted handoff copy between threads (MANIFESTO E, permitted
   copy 2).
9. **One prepare path.** The chain prepares in one place: the clock's start
   (`audioDeviceAboutToStart`). Only the host pair starts and stops the clock:
   `prepareToPlay` starts it, `releaseResources` stops it. Resize, history
   capacity, clear and state restore restart it (stop → change → start), and
   only while it runs. ARCHITECT: *"make it consistent then"*; *"prepareToPlay
   upstream still juce contract"*; the restart rule approved with `/go`
   (2026-10-29, `PLAN-terminal-clock.md` Amendment A).
10. **One serial clock context.** PTY readiness and the clock run on one
    serial context. A readiness tick is a block of the bytes the readiness
    event reports; a requested tick is a zero-sample block. The spec has three
    values, set per OS where the OS differs: the device's buffer size is the
    PTY's capacity (macOS 1024, the kernel output queue; Windows one overlapped
    read); the processor's block size is the TTY read chunk, which also sizes
    the ring (ARCHITECT: *"spec.maximumBlockSize == tty.readChunkSize"*); the
    sample rate is the measured traffic rate, and its spec value sizes the
    ring only. ARCHITECT: *"why not using AudioProcessor spec as the SSOT? the
    number could be setup differently according to OS"*; *"spec has buffer
    size, block size, sample rate"*.
11. **One envelope, one path.** ARCHITECT: *"this is our open seam to transport,
    communicate, open API via shell scripting hence, any CLI … can use the exact
    consistent ONE path protocol"*. The envelope is
    `ESC ] 1337 ; EVE ; <operation> ; <key=value;…> [ : <base64> ] BEL|ST`,
    beside iTerm2's `ESC ] 1337 ; File=<args> : <base64> BEL`. ARCHITECT:
    *"change the marker "END" to "EVE""*.
    - Operations: `command` (`name=` a `map::CommandId` name), `preview`
      (`path=` an absolute path; an empty path hides the preview), `image`
      (base64 bytes). The shell integration's `eve` function emits them.
    - State and commands. ARCHITECT: *"all state machine that drives/driven by
      terminal "command" should be registered as non-automatable parameter to
      Model"*; *"all operations/methods from/to terminal registered to
      CommandManager as SSOT callback"*; *"CommandManager functions could be
      triggered by events from Parameter, assigned keyboard shortcut,
      anything"*. A command goes through the `commands` lane and the
      `commandCount` Model row to `CommandManager::invokeDirectly` on the
      message thread. The `preview` path is a Model row.
    - The EVE `image` operation: ARCHITECT *"keep for now, but NOT for native
      Preview (Component)."*
    - Readback. ARCHITECT: *"not for everything … still under UNIDIRECTIONAL
      contract. for example File Tree widget, might need to send set cwd command
      to terminal."* The GUI writes to the terminal through the input lane only.
12. **Rendering is GUI side.** ARCHITECT: *"the actual "RENDERING" never ever touch
    the terminal itself, but an actual juce::Component widget, always GUI
    side"*; *"image streamed as bytes carried from terminal to widget (message
    thread) is inevitable, but always unidirectional to message thread"*;
    *"decode only carry metadata, and materialisation always at message
    thread"*.
    - The tick reads only an image's metadata: its pixel size and its cell box.
      The grid holds the placement `{cell box, image number}`. The bytes go
      through the `images` lane into the record, with the image's encoding
      (format, pixel size, compression).
    - Lanes (each `jam::TerminalFIFO`, one writer, one reader): `input`
      (message thread → tick), `images` (tick → the record writer), `commands`
      (tick → message thread).
    - Third-party images: ARCHITECT *"if any application use Sixel, Kitty,
      iTerm2 image protocol EVE will take over"*; *"we dont reinvent new
      protocol, we only take over terminal bytes and render them on actual GUI
      widget, at message thread, with currently exist widely adopted terminal
      image protocol, our default is iTerm2."*
13. **Reflow is the view's.** ARCHITECT: *"winsize dicated by Viewport available
    space … REFLOW, wrapping or unwrapping are NEVER ever affected by SIGWINCH nor
    winsize. it's always GUI exclusive getLocalBounds() only. winsize is TERMINAL
    DOMAIN."*

## Components

| Component | Position | Job |
|---|---|---|
| **Terminal** | center core | Renders the content SSOT + active grid; input → PTY |
| **Tree** | left dock, show/hide | cwd-relative directory tree; vim navigation; jump anywhere in hierarchy |
| **FileList** | center-adjacent (layout-composed) | Finder/Explorer-style listing of a directory; click/keyboard open |
| **Preview** | right dock, show/hide | v1: images (SKiT decoder formats) + read-only text/code |

All Components are peers in the app layout. Docks toggle without any effect on the
terminal Component's content or the decoder on the clock (a dock toggle is a message-thread
layout event only).

## Features

### Feature 1: Terminal Session

**Happy path:**
1. EVE launches → spawns `$SHELL` (fallback `/bin/zsh` macOS, PowerShell Windows) in a PTY at the terminal Component's cell size
2. Bytes flow: PTY → the clock's tick (`EVETerminalProcessor`'s `jam::VirtualDevice`) → decoder → engine rows → history and live FIFOs → TextEditor (message thread) → layout → paint
3. Keyboard/mouse → TerminalKeyboard/TerminalMouse encoders → PTY write
4. Shell exits → terminal Component shows: `[process exited — <code>]` on the line after last output; input disabled; history remains browsable

**Error handling:**

| Condition | User sees | System action |
|---|---|---|
| Shell spawn fails | `EVE: failed to start '<shell>': <errno string>` in the terminal Component | Component stays open, history empty, retry via app menu |
| PTY read error (not EOF) | `[pty error: <errno string>]` appended | Session closed as on exit |

### Feature 2: Non-Destruction (the headline)

1. **Resize / SIGWINCH:** processor resizes the bounded grid and signals the child
   (`TIOCSWINSZ`); the widget re-derives the wrapped layout from logical lines.
   Acceptance: fill session with known content → resize narrow → resize back →
   SSOT and re-derived layout **byte-identical** to the original.
2. **`clear` semantics:** ED 2 clears the visible screen — cleared lines retire to
   history first (tmux behavior). ED 3 (xterm "clear scrollback") clears the
   *view's* scrollback access but the SSOT retains the session record for the
   session lifetime. Alternate screen: app-owned, redrawn by the app per VT contract
   (RFC §1 boundary).
3. **Scrollback:** block-resident in memory, bounded on disk by
   `HISTORY_CAPACITY`. The processor owns the record, `jam::TerminalSwap`, so
   the record outlives the view. ARCHITECT: *"TextEditor INDEED hold the
   history but already materialised on demand. that's exactly the design. but
   the document SSOT outlives the widget."*
   - The record follows the JUCE disk-streaming pipeline. ARCHITECT: *"follow
     JUCE pattern, API verbatim? within the adherence into our CONTRACT?"*
     The ring of `jam::TerminalBuffer` carries the history rows. The writer
     drains them and writes each row once to the swap's index and segment
     files (`jam_TerminalSwap.cpp:99-158`). The vblank getter posts the
     writer to the processor's own serial context, in every host. ARCHITECT:
     *"why vblank must be branched? vblank at all build contract are exactly
     the same"*.
   - The swap keeps the last `HISTORY_CAPACITY` lines. It deletes the index
     and segment files of older lines (`jam_TerminalSwap.cpp:139-155`).
   - The TextEditor is the receiver. It builds only the lanes the view shows
     (`TerminalSwap::linesPerLane` lines each) and keeps their line counts. A
     resize or a font change lays out the resident lanes again. ARCHITECT:
     *"can we make it always O(1)?"*
   - Full throttle. ARCHITECT: *"the events chain should not be manually
     managed, lock free, wait free, yield free, always full throttle"*. Each
     clock tick reads the PTY until it is empty: a read that returns less than
     it asked for has emptied the PTY (`tty_dev.c:699-705`), so the tick ends
     there. ARCHITECT: *"why failed? what failed? why leave it failed?"*.
     Between ticks the driver waits
     one period; the wait is the driver's, never the tick's. ARCHITECT: *"the
     full throttle only when it RUNS"*. The ring holds one vblank of
     blocks in transit; `prepareToPlay` sizes it. Input whose output does not
     fit stays in the PTY. ARCHITECT: *"PTY holds the input"*.
   - History is append-only. ARCHITECT: *"history is append-only, clear with
     "clear" command."*

### Feature 3: Scrollback Navigation

- Scroll wheel / trackpad: scrolls the view through history; new output does not
  yank the view while scrolled (a `[N new lines]` pill appears bottom-right;
  clicking it or pressing `G` returns to live tail).
- Keyboard (terminal Component focused): the TextEditor starts in TERMINAL mode,
  where every key goes to the shell. `` ` `` then `]` enters NORMAL, a read-only,
  copy-only view of the history; `Esc` or `i a I A o O` return to TERMINAL. In NORMAL:
  `k`/`j` line up/down, `Ctrl+u`/`Ctrl+d` half-page, `gg` top of history, `G` live
  tail, `v`/drag select, `y` yank. ARCHITECT: *"` is bind key to enter TERMINAL modal
  mode. ` + ], TERMINAL modal mode is TextEditor NORMAL"*; *"return to terminal,
  history should not be editable. read only, copy only"*.

### Feature 4: Finder Layer

**cwd tracking:** shell integration emits OSC 7; EVE updates `cwd` in the Model.
No OSC 7 received → Tree/FileList show the session start directory and a status
line: `cwd tracking unavailable — install shell integration`.

**Tree (left dock):**
1. Shows directory tree rooted at filesystem root, expanded and centered on cwd
2. Vim navigation: `j`/`k` move, `h` collapse/parent, `l` expand/enter, `gg`/`G`
   top/bottom, `/` filter-as-you-type within visible tree, `Enter` = cd the
   terminal (`writes `cd '<path>'\n` to PTY`), `o` = reveal in FileList
3. Live: FS watcher refreshes the visible nodes on change

**FileList:**
1. Finder-style listing of the selected directory: name, size, modified (columns
   sortable by clicking header; default name-ascending, dirs first)
2. `Enter`/double-click on dir → descend; on file → open (below)
3. `p` or single-click → show in Preview
4. Dotfiles hidden by default; `zh` toggles (vim-style)

**Open-with (configurable CLI):** config table maps glob → command:

| pattern | command |
|---|---|
| `*.md` | `nvim "$FILE"` |
| `*` | system default (`open` / `start`) |

Command runs by writing it to the terminal PTY (visible, honest — the user sees
what ran). Missing/invalid config row → `open with: no rule matched '<name>'` in
the status area.

**Preview (right dock), v1:**
- Images: formats SKiT decoders support (PNG, JPEG, GIF incl. animation)
- Text/code: read-only buffer widget, first 1 MiB; beyond that:
  `preview truncated at 1 MiB — open to view all`
- Binary/undecodable: `no preview — <size>, <kind>`
- Markdown: rendered as plain text in v1 (Whelmed later)

**Preview is one component** [2026-10-30]. ARCHITECT: *"Preview need component,
not inlined. Because we also want gif to be animated, that's need timer."*;
*"the component could be SSOT, exactly ONE path, on API, only differs what
dictate the layout, and what command tell them to be visible."*
- `jam::Preview` materialises one image source on the message thread: a file
  path (the native column) or a record image (a takeover). It plays and loops a
  GIF with its own timer, fits its bounds with the aspect ratio kept, and calls
  native JUCE API only.
- Takeover overlay: ARCHITECT *"Siblings via owner"*. The TextEditor reserves
  the cells (*"TextEditor only "reserve the space", to reflow, wrap, unwrap,
  etc."*) and tells EVEView the visible image boxes; EVEView lays one Preview on
  each box. A cell write over an image removes it.
- Markdown: ARCHITECT *"opening md from ls is telling View to load md and render
  Markdown like current whelmed render the harness, it's always read-only
  anyway"* — `jam::MarkdownComponent` as a full-view sibling of the TextEditor
  (grand scheme §3.11; not built yet).

### Feature 5: Docking

- `Ctrl+Shift+E` toggle Tree, `Ctrl+Shift+P` toggle Preview
- Toggling is a layout event only — reader thread untouched; terminal reflows per
  Feature 2 rules (nothing destroyed)
- Dock visibility + widths persist in config state
- [2026-10-30] ARCHITECT *"EVEView, like overlays"*: EVEView owns the Preview
  column and splits its bounds in `resized`. The `preview` keys row toggles the
  persisted component state `PREVIEW`; its width is the display row
  `previewWidth`, a fraction of the window width.

### Feature 6: Configuration

- Markdown tables + CSS, CAST conventions (colour + appearance custom properties;
  data tables for keymaps, open-with rules, shell, fonts; `EMBOLDEN` is on by
  default; `HISTORY_CAPACITY` is 10000 (lines of terminal history that the record keeps); EVE bundles Display Mono and the Nerd Font symbol face, so icons
  render with no system font. ARCHITECT: *"eve should bundle nerd font, just
  like endless with display mono. it should work OOTB"*)
- Parse error at launch: EVE starts with defaults and shows
  `config error: <file>:<line> — <message>` in the status area; never fails to launch

## State Model

The content SSOT is the terminal wire bytes. `AnsiDocument` parses them into a
`jam::Document` of `Id::line` elements, each carrying `Id::cells`.
- **Ownership:** `EVETerminalProcessor` owns the decoder
  (`jam::TerminalProcessor`), the row ring (`jam::TerminalBuffer`) and the
  record (`jam::TerminalSwap`). ARCHITECT: *"GUI is ephemeral … gui only
  materialisation, on demand"*.
- **Rows:** a row's atom count is its content end. A scrolled line enters the
  ring empty; nothing clears a full row width. A prepare with an unchanged grid
  keeps the screen rows; a grid change starts an empty screen, and the shell
  redraws it on `SIGWINCH`.
- **Record:** `TerminalSwap` writes each history row once to its index and
  segment files (`jam_TerminalSwap.cpp:99-158`). It writes the live frame to a
  two-slot live file through a FIFO (`:64-87`, `:248-277`). `prepare` starts a
  new live file and drops a ready slot of the old one.
- **Pull:** on each vblank of its display (`juce::VBlankAttachment`), the
  TextEditor calls the view getter. The getter flushes the terminal Model and
  gives the swap. The TextEditor builds the history lanes it shows
  (`TerminalSwap::linesPerLane` lines each) and the live lane, then sets the
  content size, then scrolls to the tail in terminal mode
  (`jam_TextEditor.cpp:803-884`).
- **Layout state** (advances, rows, row heights) lives on each line as value
  data.
- **Geometry:** the TextEditor keeps one `Document::Index` for each resident
  history block and one for the live document, with their glyph runs and
  decoded images. It also keeps the line count of every history block. One
  cursor line space covers the history lines, then the live lines.
- **Scroll:** the node holds `scrollOffset` (lines from the tail) and
  `scrollRow` (rows of the bottom line below the view). The view paints up
  from the bottom line. At offset 0 it follows the tail; at any other offset
  new output does not move it. The wheel scrolls eased, by rows.
- **Read only:** the history is not editable. ARCHITECT: *"return to terminal,
  history should not be editable. read only, copy only"*.

Editor state is ONE `juce::ValueTree`: the TextEditor's component node in the
plugin state, from `AudioModel::getOrCreateComponentState (componentID)`. The
node carries `mode` (a `map::EditorMode` value: `normal`, `visual`,
`terminal`; the default is `normal`), `visualMode` (a `map::VisualMode` value:
`character`, `line`, `block`), `cursorLine`, `cursorColumn`,
`cursorWantedColumn`, `anchorLine`, `anchorColumn`, `count`, `operatorCount`,
`pendingOperator` and `pendingChord`. The host serialises this node with the
rest of the plugin state.

`TextEditor` is a `juce::ValueTree::Listener`. Its `valueTreePropertyChanged`
dispatches the changed property through a `Function::Map`.
`TextEditorStatusBar` binds the same node. It shows the mode and the
`line:column` ruler in `normal` and `visual` mode. Its colours are its own
colour ids, `--TextEditorStatusBar--backgroundColourId` and
`--TextEditorStatusBar--textColourId`.

The unnamed register is `juce::SystemClipboard`.

`TerminalModel` no longer exists — the class is deleted. The video lane
(`TerminalVideo`, `CellFifo`, `Row`) is deleted with it.

### Terminal state machine (the Model lane)

The terminal state machine is a `jam::Model`. `EVETerminalProcessor` owns it,
in the shape of endless (`terminal::Model : jam::Model`, `endless/Source/
terminal/Model.h:17-28`). ARCHITECT: *"you could learn how endless manage its
terminal state machine … factually raw bytes test beats all terminal in
comparison"*.

- **Schema.** `Source/layout/model.md`, table `## model`, columns `id`, `type`,
  `default`, `description`. `jam::ModelLayout` (jam_plugin_bootstrap) reads it,
  the sibling of `jam::ParameterLayout` for `parameters.md`.
- **Rows.** `applicationCursor`, `bracketedPaste`, `mouseSgr`,
  `mouseTracking`, `cursorVisible`, `alternateScreen`, `isRecording` and
  `cursor`. `cursor` is one int: `jam::Cell::Point::pack()`, so the line and
  the column change together.
- **Producer.** The decoder (`jam::TerminalProcessor`) keeps its working state
  and fires one event per value at the end of each drain. The owner's handlers
  write the Model with `storeValue`: an atomic store and a dirty mark. The
  write takes no lock, allocates nothing and posts no message. A one-time
  binding of each parameter measured 3.3 s slower on the 100M run
  (2026-10-29: 1:07.39 against 1:04.18), so the handlers keep `storeValue`.
  ARCHITECT: *"regression is FACTS"*.
- **Flush.** The Model runs no timer. The vblank pull flushes it to its tree on
  the message thread, on the same tick that reads the live frame.
  `getStateInformation` also flushes it, under the message-manager lock, before
  it copies the state.
- **Consumers.** The view binds `juce::Value`s to the properties of the Model
  tree (`getPropertyAsValue`, `referTo`). The caret and the record gate listen
  to their Values. The key and mouse encoders read their Values when an event
  arrives (`EVEViewComponents.cpp`).
- **Tree.** The Model tree is the `terminal` child of the APVTS state.
  `getStateInformation` saves it. On `setStateInformation`, EVE suspends
  processing. The Model takes the restored values, and its tree replaces the
  restored child in the incoming state. Then the APVTS replaces its whole tree,
  so one tree remains. No callback takes part: `jam::PluginEditor` owns
  `onSetState`.

### Cell-bearing element

A Document element that carries `Id::cells` plus layout properties
(`position` — flow or absolute; `left`/`top`/`width`/`height` — in cells or
pixels; `widthClass` lane — mono or proportional) is the unit every producer
emits. ANSI, Markdown, and HTML each emit this unit; the widget renders it.
A non-text element is a Document element too — it is hosted as a child
component through `ViewManager::buildContent`. This contract is written now;
later sprints implement it.

## Keyboard Summary

| Action | Normal | Visual | Terminal |
|---|---|---|---|
| Move left | `h` | `h` | |
| Move down | `j` | `j` | |
| Move up | `k` | `k` | |
| Move right | `l` | `l` | |
| Word forward | `w` | `w` | |
| Word backward | `b` | `b` | |
| Word end | `e` | `e` | |
| Line start | `0` | `0` | |
| Line end | `$` | `$` | |
| Document start | `gg` | `gg` | |
| Document end | `G` | `G` | |
| Half page up | `ctrl+u` | `ctrl+u` | |
| Half page down | `ctrl+d` | `ctrl+d` | |
| Return to terminal | `escape` `i` `a` `I` `A` `o` `O` | | |
| Leave mode | | `escape` | `` `] `` |
| Enter visual (character) | `v` | `v` | |
| Enter visual (line) | `V` | `V` | |
| Enter visual (block) | `ctrl+v` | `ctrl+v` | |
| Yank | `y` | `y` | |

All bindings are config-table entries; the table above is the shipped
default. A chord is matched by text character and is case-sensitive.
A two-character cell (`gg`) is a prefix sequence. A JUCE key description
(`ctrl+u`, `escape`) names a non-printing key. `eve.md` hot-reloads this
table.

## Success Criteria

The system:
- [ ] Byte-exact resize round trip (Feature 2.1 acceptance) — automated test
- [ ] Passes vttest sections 1, 2, 3, 8, 11 (cursor, screen, characters, modes, xterm) with zero regressions vs endless
- [ ] Zero locks on the state↔render path (audit: the only permitted lock is the suspendProcessing lifecycle gate)
- [ ] Reader thread: zero allocations after session start (instrumented assert)
- [ ] `clear` retains history; ED 2 lines reachable by scrollback
- [ ] Dock toggles and resizes never alter SSOT content (hash check)

An end-user can:
- [ ] Run vim, fzf, lazygit, tmux inside EVE without visual defects
- [ ] Navigate any directory tree entirely by keyboard and open a file with a configured tool
- [ ] Preview an image and a source file without leaving the keyboard
- [ ] Resize the window during `yes`-level output flood without losing a line the SSOT retired

## Architecture Constraints (binding, from RFC §8)

- ❌ The tick: ValueTree, Document, mutex, allocation, block, yield, sleep. The
  wait between ticks belongs to the driver. ARCHITECT: *"audio thread STILL
  LOCK FREE, processBlock never blocked as PROCESSOR"*
- ❌ Widget-owned content; paint-path writes
- ❌ Wrapped grid as truth; baked colours in state; ANSI beyond the two edges
- ❌ Terminal mental model from training data — RFC §0/§1 govern
- ❌ Terminal state as a host parameter: `parameters.md` rows are automatable only
- ❌ `setValueNotifyingHost` or any lock on the clock. ARCHITECT: *"using
  setValueNotifyingHost is smell"*
- ❌ A second copy of a truth: a counter beside a row's atom count, a member
  that mirrors a Model value, a second dirty flag
- ❌ A second prepare path beside the clock's start; a clock start outside the
  host's `prepareToPlay` / `releaseResources` pair
- ❌ A second thread context between PTY readiness and the clock
- ✅ Single-surface: one PTY, one buffer, one widget per instance; END muxes
- ✅ Two lanes, one tree: AudioModel for automatable parameters, jam::Model for
  the terminal state machine, its tree inside the APVTS state
- ✅ `EVETerminalProcessor` owns its clock in every host; above the seam,
  JUCE's contract. ARCHITECT: *"we only take over the control at seam we need
  to drive ourselves"*

---

*Exact key strings, message strings, and default values above are shipped defaults
proposed by COUNSELOR — ARCHITECT's correction pass amends them in place.*
