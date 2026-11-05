# EVE Specification v0.0.1

**Companion:** PLAN-Grand-Scheme.md (architecture, evidence, non-negotiables — normative for all architectural questions; this SPEC does not repeat it)

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
   resize. Only an explicit `clear` (ED 2 / ED 3 semantics, below) wipes the
   *screen* — and history scrollback is still retained (tmux-like).
4. **Terminal conformance at processor level** — VT100 spec correctness as the
   foundation, plus the xterm/modern extensions jam_terminal already encodes
   (SGR truecolor, bracketed paste, progressive keyboard, mouse SGR, and the reports in "Terminal reports"),
   and the image envelopes EVE takes over: OSC 1337 (iTerm2 `File=` and EVE's own
   envelope), DCS (Sixel) and APC (Kitty graphics).
5. **Finder truth is the filesystem** — sidebars and file list read the FS
   directly, keyed off OSC 7-reported cwd. The filesystem decides what is a file.
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
      command chain, `EVEView::invokeCommand`, `EVEView.cpp:321-331`), `preview`
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
      OSC 52 write and the record of an OSC 8 link
      (`jam_TerminalProcessor.cpp:995-1012`, `:1257-1265`).
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

| Report | Table row | Handler | Result |
|---|---|---|---|
| OSC 0, OSC 2 | `terminal.md:1329-1330` | `:987-990` | The `title` row takes the text. |
| OSC 7 | `terminal.md:1328` | `:969-986` | The `cwd` row takes the decoded path. |
| OSC 8 | `terminal.md:1332` | `:995-1012` | A link record goes through the `images` lane. A click writes the `url` row and invokes the `openFile` command. |
| OSC 11 `?` | `terminal.md:1331` | `:378-412`, `:991-994` | The reply gives the `background` row as `rgb:`. The view writes that row (`EVEView.cpp:25-29`). |
| OSC 52 | `terminal.md:1097`, `:1381` | `:1100-1126`, `:1257-1265` | A write of selection `c` goes to the record, and the `clipboard` row names it. A query (`?`) is not handled. |
| OSC 133 A | `terminal.md:1350` | `:1049` | The `numPrompts` row counts up. |
| OSC 133 C | `terminal.md:1348` | `:1018-1035` | The `isRecording` row turns on. The `foregroundProcess` row takes the first word of the command line. |
| OSC 133 D | `terminal.md:1349` | `:1036-1048` | The `isRecording` row turns off. The `foregroundProcess` and `title` rows clear. |
| DEC 2026 | `terminal.md:1310` | `:867-871` | The `syncOutput` row follows the mode. A hold ends after `syncTimeoutMilliseconds` (`jam_TerminalProcessor.h:114`). |
| XTVERSION (`CSI > q`) | none | `:328-336` | The reply gives the program name. |
| DECRQM (`CSI ? Pd $ p`) | none | `:320-321`, `:368-376` | The reply covers mode 2026. Every other mode is "not recognised". |

The Model rows are in `Source/layout/parameters.md:24-68`. EVE tells END the `title`,
`foregroundProcess` and `cwd` rows through the ENAX `terminal-report` callback
(`EVEView.cpp:78-79`, `:97-102`).

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
     The getter skips the post while the `syncOutput` row is on
     (`EVETerminalProcessor.cpp:168-183`). ARCHITECT:
     *"why vblank must be branched? vblank at all build contract are exactly
     the same"*.
   - The swap keeps the last `HISTORY_CAPACITY` lines. It deletes the index
     and segment files of older lines (`jam_TerminalSwap.cpp:163-182`).
   - The TextEditor is the receiver. It builds only the blocks the view shows
     (`TerminalSwap::linesPerBlock` lines each) and keeps their line counts. A
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
- Mouse: a press sets the cursor and the anchor. A drag enters VISUAL only at 2 cells
  from the anchor. A release in another mode opens the link under the cursor. A program
  that enabled mouse reports takes the mouse first. ARCHITECT: *"dragging in range of 2
  or more cell then start VISUAL and select"*.
- Link click: the TextEditor writes the target to the `url` row and invokes the
  `openFile` command by name. The TextEditor knows no table. ARCHITECT: *"the open file
  must be a registered command, so clicking, jump open, or pressing enter on highlighted
  at LIST mode will call the exact SSOT method of `open file`"*.

### Feature 4: Finder Layer

**cwd tracking:** shell integration emits OSC 7; EVE updates `cwd` in the Model.
No OSC 7 received → Tree/FileList show the session start directory and a status
line: `cwd tracking unavailable — install shell integration`.

**File list:** the last output block is the file list. ARCHITECT: *"when user run `ls`
on active prompt, the last block of the list will serves as the file list"*; *"when ls
"active", OSC 133 D after last called ls, mark "openable" file with underscore,
hyperlink"*.
1. After OSC 133 D, each entry of the last output block that names a real file gets a
   link and an underline. The filesystem decides, against the `cwd` row. No rule names
   a program.
2. A grid entry is one candidate. Any other line is one candidate, then each of its
   words. A range that holds an OSC 8 link stays as it is.
3. The next command removes the links. A click opens the entry through `openFile`.
4. A click on a directory takes the `directory` row of `## open with`: `cd`.

**OPEN mode:** ARCHITECT: *"user can enter OPEN (jump) mode, this is similar to vim jump
flash (?) so each file gets one character as keyboard shortcut to jump and open
immediately"*; *"this is exclusive for "terminal modal", LIST mode and OPEN mode"*.
1. `` ` `` then `o` shows one label, `a` to `z`, on each visible link.
2. A label key opens its link through `openFile`, and the mode returns to TERMINAL.
3. `space` moves to the next page of labels. A key that is not a label runs the rows of
   the mode, so `escape` returns to TERMINAL. The mode consumes every key.

**LIST mode:** ARCHITECT: *"LIST mode UI/UX is identical with using Finder/Explorer"*;
*"Both LEFT and RIGHT side panel is EVE native features, juce::Component pure gui"*;
*"although we are not touching any terminal even not TextEditor at all"*; *"only show
when user enter LIST mode, hence TextEditor content automatically reflow"*; *"EVEView
could hold Owner<Component>, 3 members. 2 SidePanel"*; *"Side panel preview immediately
dismissed when user exit modal mode"*; *"Preview is only the content"*.
1. `` ` `` then `l` shows a left `juce::SidePanel` with a `juce::FileTreeComponent`, and
   a right `juce::SidePanel` with `jam::FilePreview`.
2. The panels do not touch the terminal. EVEView holds `isResizing` on while LIST runs,
   so the TextEditor reflows and writes no winsize.
3. `j` and `k` move the highlight between links. The `url` row is the one selection:
   the tree and the preview follow it, and a click in the tree writes it.
4. `return` opens the highlighted entry through `openFile`.
5. An exit hides the panels, and the text unwraps.
6. The tree root is `cwd`, up by the `tree depth` row of `## display`.
7. A `juce::TimeSliceThread` of EVEView reads the directories, because
   `juce::DirectoryContentsList` needs one. *"EVE starts no thread"* binds the chain.
   ARCHITECT: *"Message thread free to spawn parallel job"*.

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
   command runs by a write to the terminal PTY (visible, honest — the user sees what
   ran).
4. While a program runs in the terminal, a shell command value opens the file with the
   system default application. ARCHITECT: *"Use the system default app"*.
5. A target that does not exist shows `open with: no file '<name>'` in the message
   overlay.
6. A target that is not a local file opens in the browser.

**Preview (right panel):** `jam::FilePreview` holds a `jam::PreviewComponent` and a
`jam::MarkdownComponent`. ARCHITECT: *"we could just use MarkdownComponent with
frontmatter metadata"*.
- Images: the image, with the metadata under it.
- Text/code: the metadata and a fenced block of the first 1 MiB.
- Binary: the metadata only.
- Metadata: size, type, created and modified.

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
  anyway"* — `jam::MarkdownComponent` as a full-view sibling of the TextEditor.
  ARCHITECT: *"MarkdownComponent basically done, tested, so we only need to wire
  the event driven mechanics"*.
  1. The `.md` row of `## open with` names the `markdown` command, so `openFile`
     runs that command, also while a program runs in the terminal.
  2. The command reads the `url` row, parses the file, and writes the panel
     visibility to its component state. The view reacts to that state.
  3. The panel takes the keyboard focus. Its component state holds the `panel`
     mode, so the `dismiss` row (`escape`, `q`) matches only there.
  4. `dismiss` hides the panel, and the TextEditor takes the focus. The terminal
     path does not change.

### Feature 5: Panels

- The panels show only in LIST mode (Feature 4). A panel is a layout event only —
  the reader thread stays untouched, and the terminal reflows per Feature 2 rules
  (nothing destroyed).
- The `tree width` and `preview width` rows of `## display` give the panel widths.
- [2026-11-05] ARCHITECT: *"i actually think Preview should be the first to
  implement CORRECTLY, and wire thorugh the whole terminal protocol"*. This
  replaces the 2026-10-31 ruling *"NO PREVIEW COMPONENT"* and the 2026-10-30 Preview
  column in EVEView. One `jam::PreviewComponent` is a child of the TextEditor
  content, and text paints over it. It paints every visible image in one call. The
  right panel holds a second `jam::PreviewComponent`, through `jam::FilePreview`.

### Feature 6: Configuration

- Markdown tables + CSS, CAST conventions (colour + appearance custom properties;
  data tables for keymaps, open-with rules, shell, fonts; `EMBOLDEN` is on by
  default; `HISTORY_CAPACITY` is 10000 (lines of terminal history that the record keeps); EVE bundles Display Mono and the Nerd Font symbol face, so icons
  render with no system font. ARCHITECT: *"eve should bundle nerd font, just
  like endless with display mono. it should work OOTB"*)
- `ligatures` (`display` table, bool, default true) switches the ligature glyphs
  of the mono font (`Source/layout/default-eve.md:19`, `jam_StyleManager.cpp:206-212`).
- `--kerning` (custom property of the `mono` `@font-face` rule in
  `Source/layout/style.css:27`) adds the font height times the factor to the
  mono cell width (`StyleCustom::setMonoFont`, `jam_StyleCustom.cpp:35-48`).
- Both rows hot reload: the new look and feel reaches `TextEditor::lookAndFeelChanged`,
  which lays the view out again (`jam_TextEditor.cpp:1066-1070`).
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
    4. The Model `winsize` row defaults to 80 columns by 24 rows. Thus the
       winsize is valid before the first layout, and `audioDeviceAboutToStart`
       asserts it.
  - **Active prompt anchor.** ARCHITECT: *"clear: draw active prompt at 0"*.
    A prepare puts the cursor at home. The shell redraws its prompt there on
    `SIGWINCH`. For oh-my-posh, the user's zshrc repaints the prompt in
    `TRAPWINCH`.
  - **Column reflow.** A column grid in history reflows at the TextEditor width
    (the column reflow plan, done and deleted, END `carol/SPRINT-LOG.md:186`). ARCHITECT: *"no hardcode about ls nor eza"*.
- **Record:** `TerminalSwap` writes each history row once to its index and
  segment files (`jam_TerminalSwap.cpp:78-135`). It writes the live frame to a
  two-slot live file through a FIFO (`:50-71`, `:417-433`). `prepare` starts a
  new live file and drops a ready slot of the old one (`:25-37`).
- **Pull:** on each vblank of its display (`juce::VBlankAttachment`), the
  TextEditor calls the view getter. The getter flushes the terminal Model and
  gives the swap. The TextEditor builds the history blocks it shows
  (`TerminalSwap::linesPerBlock` lines each) and the live block, then sets the
  content size, then scrolls to the tail in terminal mode
  (`jam_TextEditor.cpp:55-70`, `:836-914`).
- **Layout state** (advances, rows, row heights) lives on each line as value
  data.
- **Geometry:** the TextEditor keeps one `Document::Index` for each resident
  history block and one for the live document, with their glyph runs and
  decoded images. It also keeps the line count of every history block. One
  cursor line space covers the history lines, then the live lines.
- **Scroll:** the viewport shows a whole number of rows. `resized` sets the
  viewport height to the pixel height of the cell rows that fit
  (`jam_TextEditor.cpp:1023-1039`). The scroll position is the view position of
  the viewport, in pixels. `getRowOffset` gives the distance from the top of
  the first visible row to the view top, and the content paints shifted by it
  (`:1208-1230`, `:1342-1344`). At the tail the offset is 0. In TERMINAL mode,
  new output moves the view to the tail (`:859-860`). The wheel goes to the
  mouse encoder first; otherwise the viewport scrolls it (`:1398-1407`).
- **Read only:** the history is not editable. ARCHITECT: *"return to terminal,
  history should not be editable. read only, copy only"*.

Editor state is ONE `juce::ValueTree`: the TextEditor's component node in the
plugin state, from `AudioModel::getOrCreateComponentState (componentID)`. The
node carries `mode` (a `map::EditorMode` key: 0 `normal`, 1 `visual`,
2 `terminal`, 3 `open`, 4 `list`). The key 5 `panel` belongs to the markdown panel
node. The node's `mode` is the host parameter `Mode`, attached in both
directions (`jam::Model::ValueAttachment`, `TextEditor::bindToModel`); its default
is `terminal` (`Source/layout/parameters.md`). `visualMode` (a `map::VisualMode` value:
`character`, `line`, `block`), `cursorLine`, `cursorColumn`,
`cursorWantedColumn`, `anchorLine`, `anchorColumn`, `count`, `numOperatorRepeats`,
`pendingOperator`, `layer` and `sequence`. The host serialises this node with the
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

- **Schema.** `Source/layout/parameters.md`, table `## model` (`:24-68`), columns
  `id`, `type`, `max`, `default`, `description`. `jam::ModelLayout` (jam_plugin_bootstrap) reads it,
  the sibling of `jam::ParameterLayout` for `parameters.md`.
- **Rows.** `applicationCursor`, `bracketedPaste`, `mouseTracking`,
  `mouseSgr`, `cursorVisible`, `alternateScreen`, `isRecording`, `shellExited`,
  `cursor`, `numCommands`, `numPrompts`, `outputBlock`, `url`, `preview`, `previewBox`,
  `winsize`, `cwd`, `title`, `foregroundProcess`, `syncOutput`, `clipboard` and `background`
  (`parameters.md:29-67`). `cursor` is one int: `jam::Cell::Point::pack()`, so the line and
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
- **Consumers.** Each text editor binds its own instance's `terminal` row and
  listens to it (`TextEditor::bindToModel`). The caret and the record gate react
  through the editor's property map. The key and mouse encoders read the row
  when an event arrives, and send the bytes through the `terminalInput` chain
  event (`TextEditor::bindToEvents`).
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
| Return to terminal | `escape` `i` `a` `I` `A` `o` `O` | `escape` `i` `a` `I` `A` `o` `O` | |
| Enter normal | | | `` `] `` |
| Enter visual (character) | `v` | `v` | |
| Enter visual (line) | `V` | `V` | |
| Enter visual (block) | `ctrl+v` | `ctrl+v` | |
| Yank | `y` | `y` | |
| Copy | `cmd+c` / `ctrl+shift+c` | `cmd+c` / `ctrl+shift+c` | |
| Paste | | | `cmd+v` / `ctrl+shift+v` |
| Enter OPEN | | | `` `o `` |
| Enter LIST | | | `` `l `` |

| Action | OPEN | LIST | Markdown panel |
|---|---|---|---|
| Open | label key | `return` | |
| Next page of labels | `space` | | |
| Move down | | `j` | |
| Move up | | `k` | |
| Return to terminal | `escape` | `escape` `i` `a` `I` `A` `o` `O` | |
| Close the panel | | | `escape` `q` |

Under END, END's prefix layer takes `o` for OPEN and `f` for LIST, because `l` there
moves the focus to the right pane. The hosted EVE drops its own `open` and `list` rows.

The default keys belong to the command's state owner. The text editor commands
and their keys, modes and layers are jam tables (`jam/cast/terminal.md`); zoom and
settings are the plugin editor's (`jam/cast/plugin_bootstrap.md`). The first key
of a Copy or Paste cell is macOS, the second Windows, so `ctrl+c` still sends
`\x03`. Paste sends the clipboard to the shell, wrapped in the bracketed-paste
markers while the shell sets DEC 2004 (`TextEditor` paste action). A row of
`eve.md`'s `keys` table replaces every default row of its command name
(`PluginEditor::addCommands`). A value of one or two characters is matched by text
character and is case-sensitive; a two-character cell (`gg`) is a sequence. A JUCE
key description (`ctrl+u`, `escape`) names a non-printing key. `eve.md` hot-reloads
the keys. The prefix key (`` ` ``) enters the prefix layer, and `]` there enters
NORMAL from TERMINAL; `` ` `` pressed twice types `` ` `` in the shell. Any other key
in the layer is dropped. `sequenceTimeout` (jam default 1000 ms; a `keys` row
overrides it) ends a sequence or a layer. Under END, END's own prefix layer and
`normal` command drive NORMAL through EVE's `mode` host parameter, and the hosted
EVE drops its own `prefix` and `normal` rows (`jam_ENAXClientExtensions.cpp:33-39`).
END also tells EVE its keyboard focus through the `focus` host parameter; EVE takes
keyboard focus when it turns on (`PluginEditor::addCallbacks`).

**Config.** `eve.md` follows END's contract. The binary carries the defaults
(`default-eve.md` plus the binary css, written by `PluginEditorLayout::getDefaultSettings`);
a valid disk copy overlays them row by row (`ConfigDocument::addTables`,
`Model::setValuesFrom`). The front-matter version gates a rewrite of an older file
(`ConfigDocument::getOrCreateDefaultFile`); agents never change a version. Every key
is lower-case words; the reader formats it when it creates the row. One css file gives
one table, named by the file (`Css::getValueTree`), and its `@font-face` rules give
`## fonts`. `## colours` holds the named palette. The other theme tables use
`light | dark` columns over that palette; an empty
`dark` cell means the same as `light`. A validation or column error goes to
`EVEView::messageOverlay`, the default stays, and nothing asserts on the disk file.

**ANSI colours.** The cast table `AnsiColours` (`jam/cast/terminal.md`) gives the default
of all 256 indexes (`map::defaultAnsiColours`). `jam::StyleTerminal` tells these defaults
to the LookAndFeel, then applies `## terminal`, so a row of that table overrides a
default. `## terminal` holds indexes 0–16; a row for any other index is an unknown key.
Index 16 is blank: alpha 0, so a cell with that background fills nothing, and a glyph
with that foreground takes the default text colour. An application asks for a
transparent background with index 16. Index 0 stays `BUNKER`, so only index 16 is blank.

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

## Architecture Constraints (binding, from `PLAN-Grand-Scheme.md` §8)

- ❌ The tick: ValueTree, Document, mutex, allocation, block, yield, sleep. The
  wait between ticks belongs to the driver. ARCHITECT: *"audio thread STILL
  LOCK FREE, processBlock never blocked as PROCESSOR"*
- ❌ Widget-owned content; paint-path writes
- ❌ Wrapped grid as truth; baked colours in state; ANSI beyond the two edges
- ❌ Terminal mental model from training data — `PLAN-Grand-Scheme.md` §0/§1 govern
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
