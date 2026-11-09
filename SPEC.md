# EVE Specification v0.0.1

**Companion:** PLAN-Grand-Scheme.md (architecture, evidence, non-negotiables — normative for all architectural questions; this SPEC does not repeat it)

## Overview

**Purpose:** A terminal application whose content is never destroyed — not by
SIGWINCH, not by window resize — and whose surface composes with Finder-grade file
navigation, built as a JUCE application on the MVP audio-plugin architecture.

**Target End-User:** ARCHITECT (daily driver), then general release.

**Core Workflow:** Launch EVE → shell session in the center terminal Component →
navigate the filesystem with column lists, OPEN and the native Finder widget
(vim keys) → preview files → open with configured tools → resize/rearrange freely,
content always intact, full history always reachable.

**Mental model (binding):** EVE is a JUCE application, NOT a terminal. Every widget
is a `juce::Component`; all state belongs to `jam::Model`-family state (scalar/mode)
and the Document SSOT (content). The terminal is one Component — the center core —
among ours.

## Technology Stack

- **Language:** C++17
- **Frameworks:** JUCE + JAM (jam_terminal, jam_graphics, jam_vulkan, jam_style, jam_markdown, jam_core)
- **Renderer:** `jam::VulkanLowLevelGraphicsContext` (GPU) and `jam::CpuLowLevelGraphicsContext` (CPU lane), juce software renderer as oracle
- **Build:** self-contained single-file CMakeLists (CAST target oracle), CMake + Ninja
- **Platform:** macOS (Intel + ARM) and Windows; identical DX
- **Shape:** standalone application and audio plugin (VST2, VST3, AU). END hosts it in every pane under ENAX; the CLAP format comes later (`PLAN-Grand-Scheme.md` §4.3)
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
   resize. The `clear` command (`ctrl+l`) and ED 2 blank the live screen and keep
   history. Only ED 3, which the shell's `clear` program sends, erases history (below).
4. **Terminal conformance at processor level** — VT100 spec correctness as the
   foundation, plus the xterm/modern extensions jam_terminal already encodes
   (SGR truecolor, bracketed paste, progressive keyboard, mouse SGR, and the reports in "Terminal reports"),
   and the image envelopes EVE takes over: OSC 1337 (iTerm2 `File=` and EVE's own
   envelope), DCS (Sixel) and APC (Kitty graphics).
5. **Finder truth is the filesystem** — column lists and the Finder widget read the
   FS directly, keyed off OSC 7-reported cwd. The filesystem decides what is a file.
   Terminal output text is only the candidate for that test. No rule names a
   program. ARCHITECT: *"when ls "active", OSC 133 D after last called ls, mark
   "openable" file with underscore, hyperlink"*; *"must be SSOT, UNIVERSAL, GENERAL,
   not hardcoded to specific pattern"*; *"no hardcode about ls nor eza"*.
6. **Style roles in state; colours at materialization** (`PLAN-Grand-Scheme.md` §8).
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
   (2026-10-29, Amendment A of the terminal clock plan; the plan is deleted, END `carol/SPRINT-LOG.md:186`).
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
    `ESC ] 1337 ; EVE ; <operation> ; <key=value;…> [ : <payload> ] BEL|ST`,
    beside iTerm2's `ESC ] 1337 ; File=<args> : <base64> BEL`. ARCHITECT:
    *"change the marker "END" to "EVE""*.
    - Operations: `command` (`name=` the name of any command registered in EVE's
      command manager: EVE's, the plugin editor's or the text editor's. The
      command runs on the instance that read it, through its text editor's
      command chain, `EVEView::invokeCommand`), `preview`
      (`path=` an absolute path, with `width=` and `height=` in cells; an empty path
      shows nothing). The `eve` command emits them. Shell integration puts it on
      `PATH`, so any child process can run it.
    - State and commands. ARCHITECT: *"all state machine that drives/driven by
      terminal "command" should be registered as non-automatable parameter to
      Model"*; *"all operations/methods from/to terminal registered to
      CommandManager as SSOT callback"*; *"CommandManager functions could be
      triggered by events from Parameter, assigned keyboard shortcut,
      anything"*. A command goes through the `commands` lane and the
      `numCommands` Model row to `CommandManager::invokeDirectly` on the
      message thread. The `preview` path is a Model row.
    - The `preview` operation is the native image signal. ARCHITECT: *"this also
      open seam for our SKiT API to access via shell script. so it's not hardcoded
      to anything, but universal and generic"*; *"why not just the file as metadata?
      suffice? isnt that better and fit the contract? why would we need the
      bytes?"*; *"what actually signal a CLI send? that should be registered
      parameter. downstream we only react to layout, to paint, etc"*; *"we only want
      images for preview"*. The sender gives the file, and no image byte crosses the
      terminal. The tick stores two Model rows: `preview`, the path, and
      `previewBox`, the cursor plus `width=` and `height=` in cells. Under fzf the
      width and the height are `FZF_PREVIEW_COLUMNS` and `FZF_PREVIEW_LINES`, and the
      default is `100%`. The `images` tree is the one Model of every visible image,
      native and third party: the TextEditor writes it, because `getCellBounds` is
      the one cell map. ARCHITECT: *"Preview only. more precisely PreviewComponent"*;
      *"One component for everything ... paint must be always in a single call"*. The
      view table owns one `jam::PreviewComponent`, and the TextEditor hosts it as a
      child of its content: *"juce::Component hierarchy is decoupled from
      ownership"*. The content paints fills, then the PreviewComponent, then the text
      in `paintOverChildren`, so text shows over the image. The isomorphism with the
      TextEditor: `jam::ImageDocument` is the AST of one file, built once, as
      `AnsiDocument` is, and the decoded frames are its materialisation, as `runs`
      are. An empty path, or a file that does not decode, gives no frames. An erase sequence that touches the box, or a screen change, ends the
      box. This 2026-11-05 ruling replaces *"keep for now, but NOT for native Preview
      (Component)."*
    - SKiT is two-way. ARCHITECT: *"we have always bidirectional read/write
      decode/encode to widely adopted terminal image protocol, that's SKiT is all
      about. not reinventing new pattern but extend it, able to receive if any CLI
      asks for specific protocol such as Sizel, Kitty or iTerm2. but at the point
      where we could handle better, a more BLESSED approach, we always implement
      the best, the fastest, most efficient and most effective approach. since we
      have control end-to-end. the whole stack, encapsulated."*
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
      (format, pixel size, compression). The lane also carries the text of an
      OSC 52 write and the record of an OSC 8 link.
    - Lanes (each `jam::TerminalFIFO`, one writer, one reader): `input`
      (message thread → tick), `images` (tick → the record writer), `commands`
      (tick → message thread).
    - Third-party images: ARCHITECT *"if any application use Sixel, Kitty,
      iTerm2 image protocol EVE will take over"*; *"we dont reinvent new
      protocol, we only take over terminal bytes and render them on actual GUI
      widget, at message thread, with currently exist widely adopted terminal
      image protocol, our default is iTerm2."*
    - Widget only. ARCHITECT: *"I want to enforce the architecture fully widget wise
      on juce::Component native contract. no poking layers. TerminalGraphics lane are
      final alternatives only when it was proven to be only way"*. A terminal image
      shows on the `jam::PreviewComponent` at its cell box. `TerminalGraphics` is not this path.
13. **Reflow is the view's.** ARCHITECT: *"winsize dicated by Viewport available
    space … REFLOW, wrapping or unwrapping are NEVER ever affected by SIGWINCH nor
    winsize. it's always GUI exclusive getLocalBounds() only. winsize is TERMINAL
    DOMAIN."*

## Terminal reports

The tick decodes each report into metadata or a Model row. The dispatch tables are in
`jam/cast/terminal.md` (`## oscFunctions`, `## shellIntegrationFunctions`,
`## privateModes`, `## imageFunctions`). The handlers are in `jam_TerminalProcessor.cpp`.

| Report | Result |
|---|---|
| OSC 0, OSC 2 | The `title` row takes the text. |
| OSC 7 | The `cwd` row takes the decoded path. |
| OSC 8 | A link record goes through the `images` lane. A click writes the `url` row and invokes the `openFile` command. |
| OSC 11 `?` | The reply gives the `background` row as `rgb:`. The view writes that row (`EVEView::lookAndFeelChanged`). |
| OSC 52 | A write of selection `c` goes to the record, and the `clipboard` row names it. A query (`?`) is not handled. |
| OSC 133 A | The `numPrompts` row counts up. |
| OSC 133 C | The `isRecording` row turns on. The `foregroundProcess` row takes the first word of the command line. |
| OSC 133 D | The `isRecording` row turns off. The `foregroundProcess` and `title` rows clear. |
| DEC 2026 | The `syncOutput` row follows the mode. A hold ends after `syncTimeoutMilliseconds`. |
| XTVERSION (`CSI > q`) | The reply gives the program name. |
| DECRQM (`CSI ? Pd $ p`) | The reply covers mode 2026. Every other mode is "not recognised". |
| Progressive keyboard (`CSI = Ps ; Pm u`, `CSI > Ps u`, `CSI < Ps u`, `CSI ? u`) | Each screen keeps its own flags and stack. The `keyboardFlags` row reports the flags of the active screen, and `TextEditor` passes them to `TerminalKeyboard::map`. The flags hold the two bits that the encoder implements: disambiguate and report all keys. ARCHITECT: *"Wire the protocol"*. |

The Model rows are in `Source/layout/parameters.md`. All EVE state is a Model row, except
the `bypass` host parameter. ARCHITECT: *"there's no \"report\" it's all terminal state. a
Model parameter. its state changes, terminal command is the sole author. Listener react,
always UNIDIRECTIONAL, always SSOT, no copy, no shadow state"*.

- **EVE to the host.** A change of `title`, `foregroundProcess`, `cwd`, `zoom` or
  `shellExited` calls `updateHostDisplay` with two flags: `parameterInfoChanged` for VST2
  and AU, `nonParameterStateChanged` for VST3 (`EVEProcessor::valueTreePropertyChanged`).
  The host then reads the state.
- **The host to EVE.** A tree of type `pluginState` in `setStateInformation` sets each of
  its properties on the Model, with no clock restart (`EVETerminalProcessor::setRows`).
  Any other tree restores the state through the restart path.

## Components

| Component | Position | Job |
|---|---|---|
| **Terminal** | center core | Renders the content SSOT + active grid; input → PTY |
| **Finder** | ephemeral, at the caret | Native widget: tree, file list and preview columns; vim navigation; jump anywhere in hierarchy |
| **Preview** | a Finder column, or in the terminal cells | v1: images (SKiT decoder formats) + read-only text/code |

All Components are peers in the app layout. An ephemeral component comes and goes without
any effect on the terminal Component's content or the decoder on the clock (a message-thread
layout event only).

## Features

### Feature 1: Terminal Session

**Happy path:**
1. EVE launches → spawns `$SHELL` (fallback `/bin/zsh` macOS, PowerShell Windows) in a PTY at the terminal Component's cell size
2. Bytes flow: PTY → the clock's tick (`EVETerminalProcessor`'s `jam::VirtualDevice`) → decoder → engine rows → history and live FIFOs → TextEditor (message thread) → layout → paint
3. Keyboard/mouse → TerminalKeyboard/TerminalMouse encoders → PTY write
4. Shell exits → the `shellExited` row turns on, and `EVEView` shows `shell exited` in the message overlay; history remains browsable. `EVEView` then invokes `StandardApplicationCommandIDs::quit` through its `commandManager`. In standalone that quits the application. In a plugin no target takes the command, and END closes the pane from the row. ARCHITECT: *"`exit` should close END's pane (exit eve)"*; *"standalone eve should exit, close the app. why cant this be SSOT?"*

**Error handling:**

| Condition | User sees | System action |
|---|---|---|
| Shell spawn fails | `shell exited` in the message overlay | As on exit: `openTTY` writes the `shellExited` row |
| PTY read error | `shell exited` in the message overlay | Session closed as on exit |

### Feature 2: Non-Destruction (the headline)

1. **Resize / SIGWINCH:** processor resizes the bounded grid and signals the child
   (`TIOCSWINSZ`); the widget re-derives the wrapped layout from logical lines.
   Acceptance: fill session with known content → resize narrow → resize back →
   SSOT and re-derived layout **byte-identical** to the original.
2. **`clear` semantics:** the `clear` command (default key `ctrl+l`) takes the path of
   a resize. ARCHITECT: *"same path, same commandId, one SSOT with clear, WITHOUT
   erasing any history"*; *"clear live buffer redraw active prompt with whatever
   winsize parameter?"*. The chain restarts, ED 2 blanks the live screen, the cursor
   goes home, and `SIGWINCH` makes the shell redraw its prompt. On the alternate
   screen the command sends 0x0C to the program. ARCHITECT: *"with alternate, it
   should be just CLI responsibility"*. ED 2 from a program blanks the live screen.
   ED 3 (xterm "clear scrollback"), which the shell's `clear` program sends
   (`ESC[3J ESC[H ESC[2J`), clears the *view's* scrollback access. ARCHITECT:
   *"clear is erasing history yes"*. The SSOT retains the session record for the
   session lifetime. Alternate screen: app-owned, redrawn by the app per VT contract
   (`PLAN-Grand-Scheme.md` §1 boundary).
3. **Scrollback:** block-resident in memory, bounded on disk by
   `HISTORY_CAPACITY`. The processor owns the record, `jam::TerminalSwap`, so
   the record outlives the view. ARCHITECT: *"TextEditor INDEED hold the
   history but already materialised on demand. that's exactly the design. but
   the document SSOT outlives the widget."*
   - The record follows the JUCE disk-streaming pipeline. ARCHITECT: *"follow
     JUCE pattern, API verbatim? within the adherence into our CONTRACT?"*
     The ring of `jam::TerminalBuffer` carries the history rows. The writer
     drains them and writes each row once to the swap's index and segment
     files (`jam_TerminalSwap.cpp:78-135`, `:147-161`). The vblank getter
     posts the writer to the processor's own serial context, in every host.
     The getter skips the post while the `syncOutput` row is on. ARCHITECT:
     *"why vblank must be branched? vblank at all build contract are exactly
     the same"*.
   - The swap keeps the last `HISTORY_CAPACITY` lines. It deletes the index
     and segment files of older lines (`jam_TerminalSwap.cpp:163-182`).
   - The TextEditor is the receiver. It builds every history block of the swap
     (`TerminalSwap::linesPerBlock` lines each), from the tail block to the head
     block, and it builds a resident block again when its line count, its open end
     or its file links changed (`TextEditor::addBlocks`). A
     resize or a font change lays out the resident blocks again. ARCHITECT:
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

- Scroll wheel / trackpad: scrolls the view through history. In TERMINAL mode, new
  output moves the view to the tail only when the view already sits at the tail.
  ARCHITECT: *"user scroll up always win"*. A typed key returns the view to the tail.
  In NORMAL, `G` returns to live tail.
- Keyboard (terminal Component focused): the TextEditor starts in TERMINAL mode,
  where every key goes to the shell. `` ` `` then `]` enters NORMAL, a read-only,
  copy-only view of the history; `Esc` or `i a I A o O` return to TERMINAL. In NORMAL:
  `k`/`j` line up/down, `Ctrl+u`/`Ctrl+d` half-page, `gg` top of history, `G` live
  tail, `v`/drag select, `y` yank. TextEditor keeps the vim contract: a yank ends in its
  default mode. The `defaultMode` row of the Model tells EVE's editor TERMINAL. ARCHITECT:
  *"`default` mode is NORMAL, TERMINAL, READER. so returning to `default` with ESC from
  modal state always return to NORMAL, TERMINAL, READER"*. ARCHITECT: *"` is bind key to enter TERMINAL
  modal mode. ` + ], TERMINAL modal mode is TextEditor NORMAL"*; *"return to terminal,
  history should not be editable. read only, copy only"*; *"enter NORMAL mode, VISUAL
  mode - yank should immediately back to TERMINAL. not staying in the mode"*;
  *"TextEditor is VIM core parity, functionally. Normal, Insert, Visual must work as
  is, that's the CONTRACT. TERMINAL mode is special mode ... this state belong to its
  owner, its Orchestrator: EVEView"*.
- V-BLOCK: `ctrl+v` enters it from NORMAL, VISUAL and TERMINAL, so the shell and a
  program in the terminal do not receive `ctrl+v`. The selection is a screen rectangle
  from the anchor cell to the cursor cell. `h j k l` step one screen cell, past the end
  of a line and through wrapped and grid rows. Yank copies the cells in the rectangle.
  ARCHITECT: *"ctrl+v rect block should be FREE"*; *"i could not just move to any
  cell"*; for `ctrl+v` in TERMINAL: *"Enter V-BLOCK"*.
- Mouse: a press sets the cursor and the anchor. A drag enters VISUAL only at 2 cells
  from the anchor. A release in another mode opens the link under the cursor. A program
  that enabled mouse reports takes the mouse first. ARCHITECT: *"dragging in range of 2
  or more cell then start VISUAL and select"*.
- File drop: EVEView is the `juce::FileDragAndDropTarget`. The processor quotes each
  path for the shell and joins the paths with a space. The TextEditor returns to
  TERMINAL and pastes the text, bracketed when the terminal reports bracketed paste.
  The drop sends no Enter. EVEView refuses a drag when a path holds a control byte,
  because the quote does not escape it. ARCHITECT: *"EVEView should be
  FileDragAndDropTarget, dragging file should pasting the pathname to active prompt"*.
- Link click: the TextEditor writes the target to the `url` row and invokes the
  `openFile` command by name. The TextEditor knows no table. ARCHITECT: *"the open file
  must be a registered command, so clicking, jump open, or pressing enter on highlighted
  at LIST mode will call the exact SSOT method of `open file`"*. [AMENDED 2026-11-07]
  ARCHITECT: *"the design is wrong. we dont need LIST mode at all. it's always only OPEN
  mode."* A click and an OPEN label take this path (Feature 4). `Enter` on
  a file in the Finder widget takes the same path. ARCHITECT: *"`Enter` on a known file
  type will open the file with Open With (same contract) with `ls`"*.

### Feature 4: Finder Layer

[AMENDED 2026-11-07] The canon of this feature is `PLAN-finder-0-tracker.md`, rulings 1
to 49. ARCHITECT: *"current understanding is the CANON moving forward"*.

**cwd tracking:** shell integration emits OSC 7; EVE updates `cwd` in the Model.

**Column lists:** the TextEditor tracks the column lists of the history. ARCHITECT:
*"Left/Right Panel is NOT for ls/eza whatever CLI run as file list"*; *"ls can be
clicked, but this should not be exclusive to list above active prompt."*; *"we should
track as TextEditor state, whether there are "column list" exists on history"*; *"by
default it always starts to show hyperlinks on for latest column list, but by entering
let say LIST mode we could then traverse to shift the the "selected" active column list
that we can operate OPEN mode, or clicking. the indicator is the hyperlinks
(underline)"*.
1. A column list is *"Any block with real files"*. The filesystem decides, against the
   `cwd` row of that block. No rule names a program.
2. A grid entry is one candidate. Any other line is one candidate, then each of its
   words. A range that holds an OSC 8 link stays as it is.
3. [AMENDED 2026-11-07] Links cover the whole history. ARCHITECT: *"links always active
   for whole history content including url, i dont think current url links works"*.
   Each entry of a finished output block that names a real file gets a link and an
   underline. A word with a web scheme gets a link. The `cwd` row of each output block
   decides the file.
4. A click opens the entry through `openFile`.
5. A click on a directory takes the `directory` row of `## open with`: `cd`.

**OPEN mode:** [AMENDED 2026-11-07] OPEN is the only link mode. ARCHITECT: *"the design
is wrong. we dont need LIST mode at all. it's always only OPEN mode."*; *"user can enter
OPEN (jump) mode, this is similar to vim jump flash (?) so each file gets one character
as keyboard shortcut to jump and open immediately"*.
1. `` ` `` then `o` enters OPEN. ARCHITECT: *"show each openable file from its filename
   unique chars a-z, A-Z, so 52 possibility `per page`, tab to cycle the page (look how
   endless did this)"*. ARCHITECT: *"`active` column list to OPEN and cycle ALWAYS 1, the
   selected only. tab cycle only on `active`"*. Each link of the active column list shows
   one label: a char of its file name that no earlier link of the page holds. The first
   char comes first. A link with no free char starts the next page.
2. A label key opens its link through `openFile`, and the mode returns to TERMINAL.
3. `tab` moves to the next page of labels of the active column list. A key that is not a
   label runs the rows of the mode, so `escape` returns to TERMINAL. The mode consumes
   every key. Links do not depend on OPEN. ARCHITECT: *"LINK(s) should be enabled always
   for the entire history"*.
4. The active column list is the first list above the active prompt. ARCHITECT: *"active
   selected column list is not what exactly above active prompt, but always first FOUND
   above active prompt in history"*; *"OPEN mode `active` block always starts from latest
   (bottom), found column list above active prompt, so marking `active` column block
   always n-1 of num column list block found in history. -1 means no column list"*;
   *"active column list, updated and back to n-1, when there's new column list being
   added from terminal. so it always from latest to oldest, bottom to top."*
5. [AMENDED] ARCHITECT: *"ctrl+shift+j/k for navigating blocks are inverted, j should be
   down, k up"*. `ctrl+shift+j` (`listDown`) moves to the newer list below, and
   `ctrl+shift+k` (`listUp`) moves to the older list above. The view scrolls to the active column list. ARCHITECT: *"switching enable
   column list affects scroll, so it automatically scroll to "active" selected column
   list"*.
6. The selection shows only in OPEN. ARCHITECT: *"active selected column only for OPEN
   mode"*.

**Finder widget:** ARCHITECT: *"a juce::Component, rendered with monospace font (the
exact same font with EVE's terminal)"*; *"yes at jam. it has its own ValueTree,
commandId, colourId, etc. isomorphic to TextEditor. style use is StyleTerminal, one
Style for Finder and all its childern"*.
1. `jam::FinderComponent` lays out in cells. *"pressing keyboard shortcut (default is
   ctrl+F) will open a widget layout starts at active prompt"*. The first row is the
   prompt row. ARCHITECT selected *"On the prompt row"*.
2. *"ctrl+f always open Finder in `file` mode at cwd. for action list it could be
   triggered by different shortcut"*.
3. *"`File` Finder consists of 3 type of column."*: *"File list column from cwd (fzf
   parity)"*; *"Preview column (PreviewComponent: ImagePreview, Markdown, FilePreview,
   etc)"*; *"Tree column, with TreeViewItem"*.
4. *"2 MODES: FUZZY and BROWSE. ALWAYS 2 COLUMN for each mode"*. FUZZY: *"LEFT column
   file list"*; *"RIGHT column preview"*; *"top is text input for filter"*. BROWSE:
   *"LEFT column tree hiearchy"*; *"RIGHT column file list (cwd)"*.
5. *"switching mode with keyboard shortcut (prefix + tab), while Finder active"*.
6. *"we can jump to browse to path in either column with `Tab` for switching column,
   `Enter` to switch cwd if folder selected, and the column updated. if we exit
   (dismiss) the Finder widget by pressing `Esc`, terminal cwd switch to latest cwd
   selected on Finder, they always synced"*. With typed text at the prompt, `Esc`
   clears the text, then runs `cd`. While a program runs, `Esc` closes the widget and
   sends nothing.
7. *"Finder is customizable, from eve table. user could set minimum row as the column
   height"*. *"column height could be set as `AUTO` as num rows. AUTO is content aware.
   if cwd have 5 items, only show 5 rows (whichever column left/right the most)."*;
   *"or user can override with number, and the row number always set as that number.
   more content than set row height, automatically create Viewport to scroll."*. In
   FUZZY with `AUTO`, the height *"Follows matches"*.
8. *"when active prompt at the bottom of the TextEditor viewport, it will create
   temporary row as user config (auto or numrow), and scroll up while the Finder widget
   opened. dismissing Finder widget will remove that emphemeral rows, then put the
   active prompt back to bottom, and scroll"*.
9. Each column is scrollable and content aware. ARCHITECT: *"only PreviewComponent with
   ImagePreviewComponent content scale to fit, otherwise scroll"*.
10. *"this also means the exact same Component could be the Finder for `Action` aka
    registered commandId."*; *"it's list and callback could be File to operate, and
    communicate with shell integration via EVE's protocol, or could list action to do
    something in app level, from/to terminal"*.
11. *"when everything could be wired, configured with md table, it could eliminate lots
    of vim deps plugin, clis, while we could have access anywhere my caret located to
    spawn Finder."* The list of a program is a markdown table. The reply carries the
    columns of the selected row as key=value.

**Open-with (configurable CLI):** the `## open with` table of the config maps an
extension to a value. A row with no extension in its key matches a file name, such as
`makefile`. The `any` row gives the value of every key that the table does not name,
and the table takes any key. ARCHITECT: *"even endless could freely add any open with
by extension to its config"*; *"Free rows: change jam"*.

| key | value |
|---|---|
| `.cpp` | `nvim` |
| `any` | empty |

1. A value that names a registered command runs that command.
2. An empty value opens the file with the system default application.
3. Any other value is a shell command with the quoted file as its last argument. The
   TextEditor returns to TERMINAL, and the command runs by a write to the terminal PTY
   (visible, honest — the user sees what ran).
4. While a program runs in the terminal, a shell command value opens the file with the
   system default application. ARCHITECT: *"Use the system default app"*. A path with a
   control byte takes the same route, because the quote does not escape it.
5. A target that does not exist shows `open with: no file '<name>'` in the message
   overlay.
6. A target that is not a local file opens in the browser.

**Preview family** [AMENDED 2026-11-07]: ARCHITECT: *"what actually current
PreviewComponent is ImagePreviewComponent"*; *"PreviewComponent content then could be
contextual: FilePreviewComponent, MarkdownComponent, ImagePreviewComponent, PdfComponent
etc"*. The name `jam::PreviewComponent` below is the image component until the rename.
`jam::TextEditor` replaces `MarkdownComponent`. ARCHITECT: *"MarkdownComponent could be
deleted"*; *"TextEditor now serves as the SSOT of Text rendering widget. setReadOnly
(true), becomes preview widget."*; *"images, including mermaid now delegated to
ImagePreviewComponent, TextEditor only holds the layout"*. Each `MarkdownComponent` below
names the read-only `TextEditor`.
- ARCHITECT: *"probably the metadata rows could use MarkdownComponent with frontmatter?
  we dont need to reinvent nor handrolling anything"*; *"no. only metadata. no tags."*
- Metadata is free data. ARCHITECT: *"frontmatter should be free to add/config, what
  type of metadata we want to use/show, from as itself a metadata, it's all just data
  key/value properties, including its icon"*; *"colourId better, the data is free.
  closed table mean the slots fixed but still configurable for any svg / codepoints just
  like colour and font"*.
- Images: the image, with the metadata under it.
- Text/code: the metadata and a fenced block of the first 1 MiB.
- Binary: the metadata only.

**Preview is one component** [2026-10-30]. ARCHITECT: *"Preview need component,
not inlined. Because we also want gif to be animated, that's need timer."*;
*"the component could be SSOT, exactly ONE path, on API, only differs what
dictate the layout, and what command tell them to be visible."*
- `jam::PreviewComponent` materialises every image source: a file path (the
  native signal) or a record image (a takeover). One vertical-blank clock plays and
  loops every GIF. Each image fits its box with the aspect ratio kept. It calls
  native JUCE API only.
- Takeover overlay: ARCHITECT *"Siblings via owner"*. The TextEditor reserves
  the cells (*"TextEditor only "reserve the space", to reflow, wrap, unwrap,
  etc."*) and tells EVEView the visible image boxes; EVEView lays one Preview on
  each box. A cell write over an image removes it.
- Markdown: ARCHITECT *"opening md from ls is telling View to load md and render
  Markdown like current whelmed render the harness, it's always read-only
  anyway"* — a read-only `jam::TextEditor` (registered as `Id::markdown`,
  `EVEView::registerViewComponents`) as a full-view sibling of the terminal TextEditor.
  `EVEView::addMarkdown` loads the document with `setDocument`.
  ARCHITECT: *"MarkdownComponent basically done, tested, so we only need to wire
  the event driven mechanics"*.
  [AMENDED 2026-11-07] ARCHITECT: *"MarkdownComponent serves the same contract, if
  it's being called by open with, reset, addAndMakeVisible, toFront of TextEditor, so
  terminal actually still active in its backround, until user close the component with
  cmd+w (same shortcut with closing panel) BUT this must always WIN when there's
  Markdown file opened"*; *"cmd+w should be MarkdownComponent (full panel) `exit`."*;
  *"a gui Style for actual Markdown preview, this is how whelmed rendered"*.
  1. The `.md` row of `## open with` names the `markdown` command, so `openFile`
     runs that command, also while a program runs in the terminal.
  2. The command writes the path to the `markdownFile` row of the terminal tree. An
     empty row means closed. The view reacts to that row.
  3. The view creates the component on demand, in front of the TextEditor. The
     component takes the keyboard focus. The component state holds the `panel` mode
     only while the component exists, so the `dismiss` row (`cmd+w`) matches only then.
  4. `dismiss` writes the empty row. The view destroys the component, and the
     TextEditor takes the focus. The terminal path does not change.
  5. Under END, END takes `cmd+w` first and reads the row from `pluginState`. ARCHITECT:
     *"cmd+w should close whelmed first."*; *"EVE could have parameter if whelmed is
     opened, END always know its chilren state"*. A non-empty row: END writes the empty
     row. An empty row: END closes the pane. A shell exit closes the pane directly.

### Feature 5: Ephemeral components

[AMENDED 2026-11-07] The side panels are gone. ARCHITECT: *"left/side panel as previously
planned as ls side panel idea discarded."* The read-only `TextEditor` takes the place of
`MarkdownComponent`. ARCHITECT: *"all project using MarkdownComponent migrated. runtime
behaviour must be identical. we only swap the Component"*.

- ARCHITECT: *"reminder juce::Component hierarchy is not the same with ownership.
  completly different."*; *"EVEView could have pointer of FinderComponent,
  PreviewComponent, MarkdownComponent, (future PdfComponent) for example"*; *"those
  component always ephemeral driven by EVE's Model state. can be created anywhere,
  anytime"*.
- An ephemeral component is a layout event only — the reader thread stays untouched,
  and the terminal keeps its winsize (nothing destroyed).
- [2026-11-05] ARCHITECT: *"i actually think Preview should be the first to
  implement CORRECTLY, and wire thorugh the whole terminal protocol"*. This
  replaces the 2026-10-31 ruling *"NO PREVIEW COMPONENT"* and the 2026-10-30 Preview
  column in EVEView. One `jam::PreviewComponent` is a child of the TextEditor
  content, and text paints over it. It paints every visible image in one call.

### Feature 6: Configuration

- Markdown tables + CSS, CAST conventions (colour + appearance custom properties;
  data tables for keymaps, open-with rules, shell, fonts; `EMBOLDEN` is on by
  default; `HISTORY_CAPACITY` is 10000 (lines of terminal history that the record keeps); EVE bundles Display Mono, the Nerd Font symbol face and the two Noto
  faces, so icons and symbols render with no system font. ARCHITECT: *"eve should
  bundle nerd font, just like endless with display mono. it should work OOTB"*;
  *"we only need to include noto as binary, exactly like how other fonts carry"*;
  *"emoji/icons etc are EVE contract, not Display Mono contract"*. The emoji face
  answers only an emoji-presentation codepoint. ARCHITECT: *"definitely wrong
  logic. a wrong SSOT abstraction"*. A fallback glyph takes the em of the run face,
  and a Noto text symbol fits its cell through the `GlyphConstraint` table. ARCHITECT:
  *"icons/mono emoji scaling is off"*; *"why not just fit and obey its codepoints
  width"*. A cluster with U+FE0F takes two cells, a cluster with U+FE0E takes one, and
  a mark draws on its base. ARCHITECT: *"emoji should be wide, not narrow cell"*. On
  macOS a system face takes its glyph outline from CoreText, on the screen and in a
  PDF. ARCHITECT: *"we delegate what native OS supplies"*)
- `ligatures` (`display` table, bool, default true) switches the ligature glyphs
  of the mono font (`Source/layout/default-eve.md`, `jam::StyleManager`).
- `--kerning` (custom property of the `mono` `@font-face` rule in
  `Source/layout/style.css`) adds the font height times the factor to the
  mono cell width (`StyleCustom::setMonoFont`).
- Both rows hot reload: the new look and feel reaches `TextEditor::lookAndFeelChanged`,
  which lays the view out again.
- Parse error at launch: EVE starts with defaults and shows the validation message in
  `EVEView::messageOverlay`; never fails to launch

## State Model

The content SSOT is the terminal wire bytes. `AnsiDocument` parses them into a
`jam::Document` of `Id::line` elements, each carrying `Id::cells`.
- **Ownership:** `EVETerminalProcessor` owns the decoder
  (`jam::TerminalProcessor`), the row ring (`jam::TerminalBuffer`) and the
  record (`jam::TerminalSwap`). ARCHITECT: *"GUI is ephemeral … gui only
  materialisation, on demand"*.
- **Rows:** a row's atom count is its content end. A scrolled line enters the
  ring empty; nothing clears a full row width.
- **History and live, split at OSC 133 D.** ARCHITECT: *"history is everything
  after OSC 133 D"*. Every row up to the last OSC 133 D is history: the prompt,
  the command line and its output. The active prompt after it is the live buffer.
  ARCHITECT, on the example: *"--- history ends here --- OSC 133 D here"*;
  *"--- ACTIVE PROMPT below always live buffer"*.
  - The OSC 133 D mark is a terminal command with a callback in the decoder's
    shell integration table. ARCHITECT: *"must be event driven, just like the
    foundation we have built in previous sprints, this is just another terminal
    command we need to register, and assign the callback"*.
  - On the normal screen, the callback scrolls the screen up by the cursor row,
    so each row above the cursor becomes a history row, and the cursor row
    becomes the top row. The alternate screen keeps its rows.
  - The marks come from EVE's own shell integration, from the TTY bytes. No
    rule names a prompt framework. ARCHITECT: *"CLI is CLI, OSC 133 is
    deterministic"*.
- **Saved cursor.** The decoder keeps one saved cursor and one saved pen for each
  screen (`savedCursors`, `savedPens`). A nested `?1049h` saves into the
  alternate slot, so the normal slot keeps the position that the first `?1049h`
  saved. Each `?1049l` restores the normal slot. A CLI that opens an editor on the
  alternate screen thus returns to its own cursor. ARCHITECT: *"edit in nvim with
  ctrl+g, going back to claude code the history is broken"*.
- **Resize.** ARCHITECT: *"dont conflate about 2 size. Window size (component
  size) and winsize (tty side)"*; *"sole author of ALL is Window size"*.
  1. The GUI side, on the message thread: the window resize reaches the
     TextEditor, which reflows the history at its width.
  2. The terminal side: the TextEditor writes the packed `jam::TerminalWinsize`
     to the `winsize` row of the Model lane. The processor's attachment on that
     row restarts the chain, which prepares the grid and sends `TIOCSWINSZ` and
     `SIGWINCH`.
  - ARCHITECT: *"resizing can call SIGWINCH, clear live buffer, realloc, then
    redraw active prompt, whatever it needs to be done as tty, history should be
    UNAFFECTED"*. A prepare with an unchanged grid keeps the screen rows. A grid
    change starts an empty live screen, and the shell redraws its active prompt
    on `SIGWINCH`.
  - ARCHITECT: *"history only clear, when user explicitly call command
    `clear`"*.
  - **Final stop only.** ARCHITECT: *"why dont we make resize always coalesce,
    FINAL STOP only. - resize > pixel > cell > winsize always UNIDIRECTIONAL"*;
    *"what about Component::Listener? EVEView orchestrate"*.
    1. The standalone window forwards `resizeStart` and `resizeEnd` to the
       constrainer of the editor. EVEView is that constrainer.
    2. EVEView writes `Id::isResizing` to the component state of each
       TextEditor. The TextEditor sets it false when it binds.
    3. The TextEditor computes the winsize from the cells of its terminal area
       (the viewport less the scroll bar). It writes the winsize only while
       `isResizing` is false, and only when the winsize is valid.
    4. The Model `winsize` row has no default. `audioDeviceAboutToStart` prepares
       the grid and opens the TTY only with a valid winsize (`parameters.md:59`,
       `EVETerminalProcessor.cpp:300-301`). ARCHITECT: *"Amend SPEC (Recommended)"*.
       A fresh start has no valid winsize until a layout writes one. The view
       builds the TextEditor with its gate open (`jam_ViewEditor.h:127-130`), and
       that first write forks the shell. A restored state forks at the restored
       size. Each later size goes through the final stop.
    5. A frame change from outside the window, such as a window manager, runs no
       `resizeStart`. EVEView coalesces it: when its timer runs or the gate is
       open, `EVEView::resized` closes the gate and starts the timer again, before
       the layout. The timer runs `resizeEnd`, which is the final stop. A live
       resize holds the gate with no timer, so `resized` adds nothing. ARCHITECT:
       *"Both (Recommended)"*.
    6. Under END, the pane is the sole author of the `width` and `height` rows.
       END writes them before it creates the editor (`EditorView.cpp:240`), so EVE
       is built at the pane size. `EVEView::resizeEnd` does not write them under
       END. ARCHITECT: *"you fix the cause, the design, the race, you'll fix both.
       DESIGN BY CONTRACT."*
  - **Active prompt anchor.** ARCHITECT: *"clear: draw active prompt at 0"*.
    A prepare puts the cursor at home. The shell redraws its prompt there on
    `SIGWINCH`. For oh-my-posh, the user's zshrc repaints the prompt in
    `TRAPWINCH`. The zshrc arms that trap at the first prompt, because
    `_omp_status` has no value before the first `_omp_precmd`. ARCHITECT:
    *"Both (Recommended)"*.
  - **Column reflow.** A column grid in history reflows at the TextEditor width
    (the column reflow plan, done and deleted, END `carol/SPRINT-LOG.md:186`). ARCHITECT: *"no hardcode about ls nor eza"*.
- **Record:** `TerminalSwap` writes each history row once to its index and
  segment files (`jam_TerminalSwap.cpp:78-135`). It writes the live frame to a
  two-slot live file through a FIFO (`:50-71`, `:417-433`). `prepare` starts a
  new live file and drops a ready slot of the old one (`:25-37`).
- **Pull:** on each vblank of its display (`juce::VBlankAttachment`), the
  TextEditor calls the view getter. The getter flushes the terminal Model and
  gives the swap. The TextEditor builds every history block of the swap
  (`TerminalSwap::linesPerBlock` lines each) and the live block, then sets the
  content size, then scrolls to the tail in terminal mode when the view sat at the
  tail before the update (`TextEditor::addBlocks`).
- **Layout state** (advances, rows, row heights) lives on each line as value
  data.
- **Geometry:** the TextEditor keeps one `Document::Index` for each resident
  history block and one for the live document, with their glyph runs and
  decoded images. It also keeps the line count of every history block. One
  cursor line space covers the history lines, then the live lines.
- **Scroll:** the viewport shows a whole number of rows. `resized` sets the
  viewport height to the pixel height of the cell rows that fit. The scroll position
  is the view position of the viewport, in pixels. `getRowOffset` gives the distance
  from the top of the first visible row to the view top, and the content paints
  shifted by it. At the tail the offset is 0. In TERMINAL mode, new output keeps a view
  at the tail there, and it does not move a view that the user scrolled up. The wheel goes to the mouse encoder first; otherwise the viewport
  scrolls it.
- **Read only:** the history is not editable. ARCHITECT: *"return to terminal,
  history should not be editable. read only, copy only"*.

The editor mode, `mode` (a `map::EditorMode` key: 0 `normal`, 1 `visual`, 2 `terminal`,
3 `open`, 4 `reader`), exists only as the `mode` Model row, with the default `terminal`
(`Source/layout/parameters.md`). `layer` and `sequence` are Model rows on the same tree.
ARCHITECT: *"why copy? no copy allowed"*. The key 5 `panel` belongs to the markdown panel
node.

[AMENDED 2026-11-07] ARCHITECT: *"its NORMAL, VISUAL (block, line, etc), TERMINAL are the
canon"*; *"but we can register arbitrary MODE, with callback."*; *"for example for
FINDER, we could register OPEN mode (for jump with labeled keystroke), LIST mode (to
switch column list in history)"*; *"the usage of Status bar as modal exactly the same.
but EVE as owner could wire extra MODE and register the callback"*. Under END: *"EVE
gives rows to END"*. [AMENDED 2026-11-07] ARCHITECT: *"the design is wrong. we dont need
LIST mode at all. it's always only OPEN mode."*

The rest of the editor state is the TextEditor's component node in the
plugin state, from `AudioModel::getOrCreateComponentState (componentID)`: `visualMode`
(a `map::VisualMode` value: `character`, `line`, `block`), `cursorLine`, `cursorColumn`,
`cursorWantedColumn`, `anchorLine`, `anchorColumn`, `count`, `numOperatorRepeats` and
`pendingOperator`. The host serialises this node with the rest of the plugin state.

`TextEditor` is a `juce::ValueTree::Listener` of both trees. Its `valueTreePropertyChanged`
dispatches the changed property through a `Function::Map`.
`TextEditorStatusBar` binds the same two trees. It shows the mode and the
`line:column` ruler in `normal` and `visual` mode. The left segment holds the mode in
bold, and the right segment holds the ruler. A visual label is `VISUAL`, `V-LINE` or
`V-BLOCK`. Its colours are EVE's, in `Source/layout/style.css`, block `.appearance` (`:118-124`): one colour id for each
shown mode, the label text, the ruler segment, the ruler text, and the bar. ARCHITECT:
*"NORMAL, VISUAL, INSERT, etc could have its own unique colour, while the text colour
remains contrast"*; *"visual line should be V-LINE not VISUAL LINE, as V-BLOCK not
VISUAL BLOCK"*; *"remove the unused END's colours. add to EVE"*.

The unnamed register is `juce::SystemClipboard`.

`TerminalModel` no longer exists — the class is deleted. The video lane
(`TerminalVideo`, `CellFifo`, `Row`) is deleted with it.

### Terminal state machine (the Model lane)

The terminal state machine is a `jam::Model`. `EVETerminalProcessor` owns it,
in the shape of endless (`terminal::Model : jam::Model`, `endless/Source/
terminal/Model.h:17-28`). ARCHITECT: *"you could learn how endless manage its
terminal state machine … factually raw bytes test beats all terminal in
comparison"*.

- **Schema.** `Source/layout/parameters.md`, table `## model` (`:14-120`), columns
  `id`, `type`, `max`, `default`, `description`. `jam::ModelLayout` (jam_plugin_bootstrap) reads it,
  the sibling of `jam::ParameterLayout` for `parameters.md`.
- **Rows.** `applicationCursor`, `bracketedPaste`, `mouseTracking`, `cursorShape`,
  `keyboardFlags`, `mouseSgr`, `cursorVisible`, `alternateScreen`, `isRecording`,
  `shellExited`, `cursor`, `numCommands`, `numPrompts`, `outputBlock`, `preview`,
  `previewBox`, `winsize`, `cwd`, `title`, `foregroundProcess`, `url`, `markdownFile`,
  `syncOutput`, `clipboard`, `background`, `zoom`, `width`, `height`, `mode`, `defaultMode`,
  `focus`, `autoWrap`, `originMode`, `insertMode`, `isWrapPending`, `activeLink`,
  `scrollingRegion`, `savedCursorNormal`, `savedCursorAlternate`, `pen`, `savedPenNormal`,
  `savedPenAlternate`, `keyboardFlagsInactive`, `keyboardModesNormal`,
  `keyboardModesAlternate`, `numKeyboardModesNormal`, `numKeyboardModesAlternate`,
  `syncStart`, `layer`, `sequence` and `pendingClipboard` (`parameters.md`, `## model`).
  ARCHITECT: *"everything should
  be Parameter, no strays ana manual state management allowed"*. `cursor` is one int: `jam::Cell::Point::pack()`, so the line and
  the column change together. `cursorShape` is a key of `map::CursorShape`, from
  `CSI Ps SP q`. In TERMINAL, the caret paints a block, an underline, or a bar from this
  row. A blink key paints as its steady key. ARCHITECT: *"block char only at normal mode,
  entering insert mode it changed to | pipe cursor by default following nvim"*.
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
- **Consumers.** Each text editor binds its own instance's `terminal` row and
  listens to it (`TextEditor::bindToModel`). The caret and the record gate react
  through the editor's property map. The key and mouse encoders read the row
  when an event arrives, and send the bytes through the `terminalInput` chain
  event (`TextEditor::bindToEvents`).
- **Tree.** The Model tree is the `terminal` child of the APVTS state.
  `getStateInformation` saves it as a `pluginState` child, the state instance that a host holds.
  ARCHITECT: *"the correct design is not `terminal`, but the state instance of `EVE`. `EVE` could
  be any plugin"*. On `setStateInformation`, EVE suspends
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
| Return to terminal | `escape` `i` `a` `I` `A` `o` `O` | `escape` `i` `a` `I` `A` `o` `O` | |
| Enter normal | | | `` `] `` |
| Enter visual (character) | `v` | `v` | |
| Enter visual (line) | `V` | `V` | |
| Enter visual (block) | `ctrl+v` | `ctrl+v` | `ctrl+v` |
| Yank | `y` | `y` | |
| Copy | `cmd+c` / `ctrl+shift+c` | `cmd+c` / `ctrl+shift+c` | |
| Paste | | | `cmd+v` / `ctrl+shift+v` |
| Enter OPEN | | | `` `o `` |

| Action | OPEN | Markdown panel |
|---|---|---|
| Open | label key | |
| Next page of labels | `tab` | |
| Next column list | `ctrl+shift+k` | |
| Previous column list | `ctrl+shift+j` | |
| Return to terminal | `escape` | |
| Close the panel | | `cmd+w` |

Under END, END's prefix layer takes `o` for OPEN. The hosted EVE drops its own `open` row.

The default keys belong to the command's state owner. The text editor commands
and their keys, modes and layers are jam tables (`jam/cast/terminal.md`); zoom and
settings are the plugin editor's (`jam/cast/plugin_bootstrap.md`). The first key
of a Copy or Paste cell is macOS, the second Windows, so `ctrl+c` still sends
`\x03`. Paste sends the clipboard to the shell, wrapped in the bracketed-paste
markers while the shell sets DEC 2004 (`TextEditor::applyPaste`). A row of
`eve.md`'s `keys` table replaces every default row of its command name
(`PluginEditor::addCommands`). A value of one or two characters is matched by text
character and is case-sensitive; a two-character cell (`gg`) is a sequence. A JUCE
key description (`ctrl+u`, `escape`) names a non-printing key. `eve.md` hot-reloads
the keys. The prefix key (`` ` ``) enters the prefix layer, and `]` there enters
NORMAL from TERMINAL; `` ` `` pressed twice types `` ` `` in the shell. Any other key
in the layer is dropped. `sequenceTimeout` (jam default 1000 ms; a `keys` row
overrides it) ends a sequence or a layer. Under END, END's own prefix layer and
`normal` command drive NORMAL through EVE's `mode` Model row, and the hosted
EVE drops its own `prefix`, `normal` and `open` rows (`jam_ENAXClientExtensions.cpp`).
END also tells EVE its keyboard focus through the `focus` Model row; EVE takes
keyboard focus when it turns on and gives it up when it turns off
(`EVEView::registerTerminalEvents`). `commandManager` is EVE's, not `jam::PluginEditor`'s, and
zoom and size are EVE's contract: `zoom`, `width` and `height` are Model rows, `resized` writes no
row, and `resizeEnd` writes `width` and `height` at the final stop. A host sets `width` and `height`,
and EVE sets the size of that axis from the row (`EVEView::registerTerminalEvents`). ARCHITECT: *"it's
still the same tree but adhere to how audio plugin host-client contract"*; *"CommandManager is
EVE's, NOT PLUGIN EDITOR"*; *"no fucking applyGesture at resized(). never call atomic on message
thread. why branching on message thread. this is EVE's CONTRACT. not PluginEditor."*. The TextEditor paints its caret only while it holds the
keyboard focus, and `focusOfChildComponentChanged` repaints it on each change. A pane
paints its focused and unfocused colours from END's theme. ARCHITECT: *"TextEditor
cursor should only visible when it was in focus ... no poking layers, END already
responsible for tracking and forwarding focus to its PANE. full event driven, no manual
state machine management"*; *"backgroundColourId config driven state like focus
outline"*.

**Config.** `eve.md` follows END's contract. The binary carries the defaults.
`default-eve.md` gives `display.md` (`PluginEditorLayout::getDefaultSettings`). The one
`Source/layout/style.css` gives `theme.md`: `PluginEditorLayout::getThemeSettings` writes
the tree that `Css::getValueTree` builds from the css document of the binary. A valid
disk copy overlays the defaults row by row (`PluginEditorLayout::getSettingsTree`,
`Model::setValuesFrom`). The front-matter version gates a rewrite of an older file
(`ConfigDocument::getOrCreateDefaultFile`); agents never change a version. Every key
is lower-case words; the reader formats it when it creates the row. Each product owns
one `style.css`. In EVE it has these blocks: `:root` holds the named palette,
`.appearance` holds the colour ids and font ids, `.metrics` holds the metrics,
`@font-face` rules give `## fonts`, and `.markdown` holds the overrides of the
markdown TextEditor. A `.DARK` block (`.appearance.DARK`, `.markdown.DARK`) gives the
dark values of its base block. A validation or column error goes to
`EVEView::messageOverlay`, the default stays, and nothing asserts on the disk file.

**ANSI colours.** The cast table `AnsiColours` (`jam/cast/terminal.md`) gives the default
of all 256 indexes (`map::defaultAnsiColours`). `jam::StyleTerminal` tells these defaults
to the LookAndFeel as indexed colour ids. The `.appearance` block of `Source/layout/style.css`
(`:126-142`) then overrides indexes 0–16 with the keys `--Ansi--colour0ColourId` to
`--Ansi--colour16ColourId`. The other indexes keep the defaults.
Index 16 is blank: alpha 0, so a cell with that background fills nothing, and a glyph
with that foreground takes the default text colour. An application asks for a
transparent background with index 16. Index 0 stays `BUNKER`, so only index 16 is blank.

## Success Criteria

The system:
- [ ] Byte-exact resize round trip (Feature 2.1 acceptance) — automated test
- [ ] Passes vttest sections 1, 2, 3, 8, 11 (cursor, screen, characters, modes, xterm) with zero regressions vs endless
- [ ] Zero locks on the state↔render path (audit: the only permitted lock is the suspendProcessing lifecycle gate)
- [ ] Reader thread: zero allocations after session start (instrumented assert)
- [ ] The `clear` command (`ctrl+l`) retains history and redraws the prompt; the shell's `clear` program erases history
- [ ] Ephemeral components and resizes never alter SSOT content (hash check)

An end-user can:
- [ ] Run vim, fzf, lazygit, tmux inside EVE without visual defects
- [ ] Navigate any directory tree entirely by keyboard and open a file with a configured tool
- [ ] Preview an image and a source file without leaving the keyboard
- [ ] Resize the window during `yes`-level output flood without losing a line the SSOT retired

## Architecture Constraints (binding, from `PLAN-Grand-Scheme.md` §8)

- ❌ The tick: ValueTree, Document, mutex, allocation, block, yield, sleep. The
  wait between ticks belongs to the driver. ARCHITECT: *"audio thread STILL
  LOCK FREE, processBlock never blocked as PROCESSOR"*
- ❌ Widget-owned content; paint-path writes
- ❌ Wrapped grid as truth; baked colours in state; ANSI beyond the two edges
- ❌ Terminal mental model from training data — `PLAN-Grand-Scheme.md` §0/§1 govern
- ❌ State as a host parameter: `bypass` is the one host parameter. ARCHITECT: *"we dont need to expose any `parameter` that should be just internal host-client contract"*
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
