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
- **Shape:** standalone application first; CLAP plugin hosted by Nexus later (RFC §4.3)
- **Config:** markdown tables + CSS (CAST conventions) — no lua

## Core Principles

1. **MVP** — Model holds state; Processor (reader thread, audio discipline) owns
   terminal emulation; View Components are dumb, paint-only, own nothing.
2. **Two dedicated threads** — reader (PTY/parser/grid, per RFC §8 prohibitions)
   and message (SSOT append, layout, paint). CellFifo between them. Nothing else.
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
terminal Component's content or the reader thread (a dock toggle is a message-thread
layout event only).

## Features

### Feature 1: Terminal Session

**Happy path:**
1. EVE launches → spawns `$SHELL` (fallback `/bin/zsh` macOS, PowerShell Windows) in a PTY at the terminal Component's cell size
2. Bytes flow: PTY → parser → grid/retirement → CellFifo → SSOT → layout → paint
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
3. **Scrollback:** unbounded in content, tiered in residency — a hot window
   (default 1/16 machine RAM, `terminal / scrollbackBudgetMb` override) lives
   in the Document; colder lines spill losslessly to disk as ANSI wire bytes
   (byte-exact round trip) and rehydrate on access (RFC §7.4 amendment,
   2026-09-15). No line is ever lost to the tier.

### Feature 3: Scrollback Navigation

- Scroll wheel / trackpad: scrolls the view through history; new output does not
  yank the view while scrolled (a `[N new lines]` pill appears bottom-right;
  clicking it or pressing `G` returns to live tail).
- Keyboard (terminal Component focused, "nav mode" entered with `Ctrl+Shift+K`,
  left with `Esc` or `i`): `k`/`j` line up/down, `Ctrl+u`/`Ctrl+d` half-page,
  `gg` top of history, `G` live tail.

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

- Markdown tables + CSS, CAST conventions (palette + appearance custom properties;
  data tables for keymaps, open-with rules, shell, fonts, scrollback budget —
  `terminal / scrollbackBudgetMb`)
- Parse error at launch: EVE starts with defaults and shows
  `config error: <file>:<line> — <message>` in the status area; never fails to launch

## State Model

Per RFC §3: content SSOT = append-only logical attributed lines (Document-domain,
message thread); scalar/mode state = lock-free TerminalModel parameters (reader) +
app state trees (message); active grid = reader-thread transient. Finder state
(cwd, selection, dock layout) = app-level jam::Model, message thread. Line
lookup and residency tiering = `Document::Index` (message thread), owned by the
terminal Component's content owner — the Document remains the sole content
truth.

## Keyboard Summary

| Key | Action | Context |
|---|---|---|
| (all keys) | encoded to PTY | terminal focused, live |
| `Ctrl+Shift+K` / `Esc` | enter/leave nav mode | terminal |
| `j k h l gg G Ctrl+u Ctrl+d /` | vim navigation | nav mode, Tree, FileList |
| `Enter` | cd / open | Tree, FileList |
| `p` | preview | FileList |
| `zh` | toggle dotfiles | Tree, FileList |
| `Ctrl+Shift+E` / `Ctrl+Shift+P` | toggle Tree / Preview | app |
| `Cmd/Ctrl +/-/0` | font size | app |

All bindings are config-table entries; the table above is the shipped default.

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
- ✅ Single-surface: one PTY, one buffer, one widget per instance; Nexus muxes

---

*Exact key strings, message strings, and default values above are shipped defaults
proposed by COUNSELOR — ARCHITECT's correction pass amends them in place.*
