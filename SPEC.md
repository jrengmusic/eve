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

1. **MVP** — Model holds state; Processor (audio thread, audio discipline) owns
   terminal emulation; View Components are dumb, paint-only, own nothing.
2. **Host-plugin threads** — the audio thread reads and writes the PTY in
   `processBlock` and decodes the bytes into the engine rows; the message thread
   appends history, lays out and paints. Lock-free FIFOs (`juce::AbstractFifo`) join
   them. No thread of EVE's own. ARCHITECT: *"why cant we use AUDIO thread as the
   READER thread?"*; *"Input: READER, Audio Thread, TTY. Output: Terminal LLGC. also
   Audio thread … Renderer is Vulkan … message thread"*.
3. **Non-destruction contract** — widget content is never destroyed by SIGWINCH or
   resize. Only an explicit `clear` (ED 2 / ED 3 semantics, below) wipes the
   *screen* — and history scrollback is still retained (tmux-like).
4. **Terminal conformance at processor level** — VT100 spec correctness as the
   foundation, plus the xterm/modern extensions jam_terminal already encodes
   (SGR truecolor, bracketed paste, progressive keyboard, mouse SGR, OSC 7/8/133).
5. **Finder truth is the filesystem** — sidebars and file list read the FS
   directly, keyed off OSC 7-reported cwd. Terminal output text is never parsed.
6. **Style roles in state; colours at materialization** (RFC §8).

## Components

| Component | Position | Job |
|---|---|---|
| **Terminal** | center core | Renders the content SSOT + active grid; input → PTY |
| **Tree** | left dock, show/hide | cwd-relative directory tree; vim navigation; jump anywhere in hierarchy |
| **FileList** | center-adjacent (layout-composed) | Finder/Explorer-style listing of a directory; click/keyboard open |
| **Preview** | right dock, show/hide | v1: images (SKiT decoder formats) + read-only text/code |

All Components are peers in the app layout. Docks toggle without any effect on the
terminal Component's content or the audio-thread decoder (a dock toggle is a message-thread
layout event only).

## Features

### Feature 1: Terminal Session

**Happy path:**
1. EVE launches → spawns `$SHELL` (fallback `/bin/zsh` macOS, PowerShell Windows) in a PTY at the terminal Component's cell size
2. Bytes flow: PTY → `processBlock` (audio thread) → decoder → engine rows → history and live FIFOs → TextEditor (message thread) → layout → paint
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
3. **Scrollback:** unbounded in content, block-resident in memory. The processor
   owns the history record, `jam::SwapDocument`, so the record outlives the
   view. ARCHITECT: *"TextEditor INDEED hold the history but already
   materialised on demand. that's exactly the design. but the document SSOT
   outlives the widget."*
   - The record follows the JUCE disk-streaming pipeline. ARCHITECT: *"follow
     JUCE pattern, API verbatim? within the adherence into our CONTRACT?"*
     The history FIFO carries the rows. The message-thread pull drains them.
     The record appends each drained block to its file one time.
   - A block holds at least `SwapDocument::bytesPerBlock` bytes and ends at a
     line end. Memory holds the open block and the blocks of the visible lines.
   - The TextEditor is the receiver. It lays out only the lines that the view
     shows, and it keeps the line count of every block. A resize or a font
     change lays out the visible lines again. ARCHITECT: *"can we make it
     always O(1)?"*
   - Full throttle. ARCHITECT: *"the events chain should not be manually
     managed, lock free, wait free, yield free, always full throttle"*. Each
     `processBlock` reads the PTY until it is empty. The history FIFO holds
     one vblank of blocks; the clocks size it at `prepare`. The view drains it
     on each vblank. Input whose output does not fit stays in the PTY.
     ARCHITECT: *"PTY holds the input"*.
   - No line is ever lost. The file keeps every byte for the session.
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

### Feature 5: Docking

- `Ctrl+Shift+E` toggle Tree, `Ctrl+Shift+P` toggle Preview
- Toggling is a layout event only — reader thread untouched; terminal reflows per
  Feature 2 rules (nothing destroyed)
- Dock visibility + widths persist in config state

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
- **Ownership:** `jam::TerminalProcessor` owns the live document and the
  history record `SwapDocument`. A resize replaces only the live document, so
  SIGWINCH never destroys history content. ARCHITECT: *"GUI is ephemeral …
  gui only materialisation, on demand"*.
- **Record:** `SwapDocument` appends each drained block to its file and keeps
  the byte range of every block. It parses a block into an `AnsiDocument` when
  the view asks for it. It keeps the open block and the blocks that the view
  holds in memory.
- **Pull:** on each vblank of its display (`juce::VBlankAttachment`), the
  TextEditor calls the view getter. The processor drains its FIFOs. It then gives the history record, the live document and whether the
  live document changed to the TextEditor's visitor for the duration of the
  call. The TextEditor reads the record only inside that call.
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

- ❌ Reader thread: ValueTree, Document, mutex, allocation, block, yield, sleep
- ❌ Widget-owned content; paint-path writes
- ❌ Wrapped grid as truth; baked colours in state; ANSI beyond the two edges
- ❌ Terminal mental model from training data — RFC §0/§1 govern
- ✅ Single-surface: one PTY, one buffer, one widget per instance; END muxes

---

*Exact key strings, message strings, and default values above are shipped defaults
proposed by COUNSELOR — ARCHITECT's correction pass amends them in place.*
