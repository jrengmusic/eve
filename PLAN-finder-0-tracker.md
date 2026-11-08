# PLAN: Finder — Tracker and Canon

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-07
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)

## Overview

This file is the index, the status tracker and the canon of the Finder work. Ten parts follow, and each part is one plan.
ARCHITECT supervises each part and locks each part alone. ARCHITECT: *"current understanding is the CANON moving forward"*.
ARCHITECT can shift a design at any part: *"plausible at execution on each plan, ARCHITECT might shift design and rulings when we found most effective and efficient pattern. but still the principle stands."*

## Status

| Part | Plan | Status | Depends on |
|---|---|---|---|
| 0 | this file | Steps 1, 2 and 3 done. | |
| 1 | `PLAN-finder-1-markdown.md` | Locked 2026-11-07. In execution. | 0 |
| 2 | `PLAN-finder-2-preview.md` | Written. Not locked. | 1 |
| 3 | `PLAN-finder-3-modes.md` | Superseded by `PLAN-text-editor-absorption.md` rulings 30 to 36 (Canon note below). File deleted 2026-11-08. | |
| 4 | `PLAN-finder-4-column-list.md` | Superseded, as Part 3. File deleted 2026-11-08. | |
| 5 | `PLAN-finder-5-list-open.md` | Superseded, as Part 3. File deleted 2026-11-08. | |
| 6 | `PLAN-finder-6-widget.md` | Written. Not locked. | 2 (Part 3 superseded) |
| 7 | `PLAN-finder-7-browse.md` | Written. Not locked. | 6 |
| 8 | `PLAN-finder-8-fuzzy.md` | Written. Not locked. | 2, 6 |
| 9 | `PLAN-finder-9-action.md` | Written. Not locked. | 8 |
| 10 | `PLAN-finder-10-protocol.md` | Written. Not locked. A pty run proves the blocking read before Step 4. | 8 |

Plans outside the numbered parts, tracked here:

| Plan | Status |
|---|---|
| `PLAN-text-editor-absorption.md` | Locked 2026-11-07. Steps 11 and 13 superseded by `PLAN-style-isomorphism.md` (`:3`). Steps 14 to 16 discarded: ARCHITECT *"discard all alternate scrollbar. remove all instrumentation."* |
| `PLAN-style-isomorphism.md` | Locked. Step 10 is open: `ENDLookAndFeel::setColours` still has its pairing table (`end/Source/lookAndFeel/ENDLookAndFeel.cpp:521-541`). Step 11's width is END's `## scrollbar` `thickness` row (`end/Source/config/theme/gfx/theme.md:108-120`). |
| `PLAN-alternate-scrollbar.md` | Discarded 2026-11-08. ARCHITECT: *"janky. remove all scrollbar for alternate."* File deleted. |
| `PLAN-Grand-Scheme.md` | Founding RFC and architecture rulings. Not a step plan. |

## The contract of every part

ARCHITECT, verbatim:

- *"no hardcoding to anything, and more importantly FULLY EVENT driven BLESSED compliant. everything is STATE model, listener react with direct lookup registered callback, always unidirectional, always SSOT. no manual state management, no manual boolean flags, no hacking with handroll anything. so designing the state management faithful synergy with Model and CommandManager is NON-NEGOTIABLE"*
- *"this codebase has building blocks, has the established pattern. no new semantics, no new pattern, must be design by contract."*
- *"DO NOT FILL IN THE GAPS WITH ASSUMPTIONS, NEVER FROM YOUR INFERENCE."*
- *"use Model as parameter, event driven registrable callback, with command manager, no handrolling anything"*

## Canon: rulings, verbatim

**Superseded 2026-11-07.** `PLAN-text-editor-absorption.md` supersedes rulings 4, 5, 6, 10 and 13 and Parts 3, 4 and 5, with its rulings 30 to 36. ARCHITECT: *"the design is wrong. we dont need LIST mode at all. it's always only OPEN mode."* In rulings 17, 19, 20, 23, 24 and 27, `MarkdownComponent` names the read-only `jam::TextEditor`. ARCHITECT: *"MarkdownComponent could be deleted"*; *"TextEditor now serves as the SSOT of Text rendering widget. setReadOnly (true), becomes preview widget."*

### The list contract

1. *"Left/Right Panel is NOT for ls/eza whatever CLI run as file list"*
2. *"ls can be clicked, but this should not be exclusive to list above active prompt."*
3. *"we should track as TextEditor state, whether there are "column list" exists on history"*
4. *"by default it always starts to show hyperlinks on for latest column list, but by entering let say LIST mode we could then traverse to shift the the "selected" active column list that we can operate OPEN mode, or clicking. the indicator is the hyperlinks (underline)"*
5. *"entering LIST mode, user now can press `k` up (enable previous column list) to be clicked, to enter OPEN (jump) mode, and `j` to down (enable next column list)"*
6. *"switching enable column list affects scroll, so it automatically scroll to "active" selected column list"*
7. *"left/side panel as previously planned as ls side panel idea discarded."*
8. A column list is *"Any block with real files"*.
9. *"LIST and OPEN mode must start from TERMINAL mode."*
10. On leaving LIST mode: *"Keep the selection"*. The chosen list stays active until the next new column list appears.

### TextEditor modes

11. *"its NORMAL, VISUAL (block, line, etc), TERMINAL are the canon"*
12. *"but we can register arbitrary MODE, with callback."*
13. *"for example for FINDER, we could register OPEN mode (for jump with labeled keystroke), LIST mode (to switch column list in history)"*
14. *"the usage of Status bar as modal exactly the same. but EVE as owner could wire extra MODE and register the callback"*
15. Under END: *"EVE gives rows to END"*.

### Ownership

16. *"reminder juce::Component hierarchy is not the same with ownership. completly different."*
17. *"EVEView could have pointer of FinderComponent, PreviewComponent, MarkdownComponent, (future PdfComponent) for example"*
18. *"those component always ephemeral driven by EVE's Model state. can be created anywhere, anytime"*
19. *"MarkdownComponent serves the same contract, if it's being called by open with, reset, addAndMakeVisible, toFront of TextEditor, so terminal actually still active in its backround, until user close the component with cmd+w (same shortcut with closing panel) BUT this must always WIN when there's Markdown file opened"*
20. *"cmd+w should be MarkdownComponent (full panel) `exit`."*
21. *"current markdown preview is garbage, clicking md file create component with wrong layout etc etc nothing need to be kept"*

21a. *"cmd+w should close whelmed first."*; *"EVE could have parameter if whelmed is opened, END always know its chilren state"*

### Preview

22. *"what actually current PreviewComponent is ImagePreviewComponent"*
23. *"PreviewComponent content then could be contextual: FilePreviewComponent, MarkdownComponent, ImagePreviewComponent, PdfComponent etc"*
24. *"yes. our native ImagePreview still showing metadata"*; *"probably the metadata rows could use MarkdownComponent with frontmatter? we dont need to reinvent nor handrolling anything"*
25. *"no. only metadata. no tags."*
26. *"only PreviewComponent with ImagePreviewComponent content scale to fit, otherwise scroll"*
27. *"MarkdownComponent is dumb. does not care the context and rendering context."*
28. *"a gui Style for actual Markdown preview, this is how whelmed rendered"*
29. *"a `terminal` mono style because we seamless UI to terminal cell mono, for ImagePreview metadata"*; *"but [2] is not hard constraints, because for metadata block especially we could still just use full GUI style markdown but with Display (not mono) fonts/colour scheme"*

29a. *"frontmatter should be free to add/config, what type of metadata we want to use/show, from as itself a metadata, it's all just data key/value properties, including its icon"*
29b. *"colourId better, the data is free. closed table mean the slots fixed but still configurable for any svg / codepoints just like colour and font"*

### Finder

30. *"a juce::Component, rendered with monospace font (the exact same font with EVE's terminal)"*
31. *"layout in cell, pressing keyboard shortcut (default is ctrl+F) will open a widget layout starts at active prompt"*. The first row is the prompt row: ARCHITECT selected *"On the prompt row"*.
32. *"ctrl+f always open Finder in `file` mode at cwd. for action list it could be triggered by different shortcut"*
33. *"`File` Finder consists of 3 type of column."*: *"File list column from cwd (fzf parity)"*; *"Preview column (PreviewComponent: ImagePreview, Markdown, FilePreview, etc)"*; *"Tree column, with TreeViewItem"*
34. *"2 MODES: FUZZY and BROWSE. ALWAYS 2 COLUMN for each mode"*
35. *"FUZZY: (fzf parity)"*: *"LEFT column file list"*; *"RIGHT column preview"*. *"top is text input for filter"*.
36. *"BROWSE:"*: *"LEFT column tree hiearchy"*; *"RIGHT column file list (cwd)"*
37. *"switching mode with keyboard shortcut (prefix + tab), while Finder active"*
38. *"we can jump to browse to path in either column with `Tab` for switching column, `Enter` to switch cwd if folder selected, and the column updated. if we exit (dismiss) the Finder widget by pressing `Esc`, terminal cwd switch to latest cwd selected on Finder, they always synced"*
39. *"`Enter` on a known file type will open the file with Open With (same contract) with `ls`"*
40. *"Finder is customizable, from eve table. user could set minimum row as the column height. how many shows. Both column just jam::Viewport, with scrollbar, images always fitted inside the available box"*. Amended: *"yes, each column still Viewport. scrollable, content aware"*: the tree column keeps the `juce::TreeView` viewport.
41. *"column height could be set as `AUTO` as num rows. AUTO is content aware. if cwd have 5 items, only show 5 rows (whichever column left/right the most)."*
42. *"or user can override with number, and the row number always set as that number. more content than set row height, automatically create Viewport to scroll."*
43. *"when active prompt at the bottom of the TextEditor viewport, it will create temporary row as user config (auto or numrow), and scroll up while the Finder widget opened. dismissing Finder widget will remove that emphemeral rows, then put the active prompt back to bottom, and scroll"*
44. *"with `AUTO` meaning we must track available visible empty row on TextEditor Viewport. hence if active prompt at the top most row (TextEditor blank), auto could use all the empty rows to paint its widget if the space available, otherwise capped at bottom of TextEditor viewport, and create its own viewport"*
45. In FUZZY with `AUTO`, height *"Follows matches"*.
46. Esc in BROWSE with typed text at the prompt: *"Clear, then cd"*. While a program runs: *"Close, send nothing"*.
47. *"yes at jam. it has its own ValueTree, commandId, colourId, etc. isomorphic to TextEditor. style use is StyleTerminal, one Style for Finder and all its childern"*. The markdown content keeps a `StyleMarkdown` (rulings 27 to 29).
48. *"this also means the exact same Component could be the Finder for `Action` aka registered commandId."*; *"it's list and callback could be File to operate, and communicate with shell integration via EVE's protocol, or could list action to do something in app level, from/to terminal"*
49. *"when everything could be wired, configured with md table, it could eliminate lots of vim deps plugin, clis, while we could have access anywhere my caret located to spawn Finder."*

## Questions no read answers

All are closed. Each keeps its answer here.

1. **Part 2, image content.** Closed. ARCHITECT selected *"Painter plus markdown"*: `FilePreviewComponent` keeps its image box over a `MarkdownComponent` of the metadata.
2. **Part 2, metadata header.** Closed. ARCHITECT selected *"Caller gives it"*: `addProperties (properties, header)`, with `Properties` as the default.
3. **Part 9.** Closed. ARCHITECT: *"2 column label: Keyboard shortcut | description"*; *"this is endless parity"*. One list, two labels per row.
4. **Part 10, list format.** Closed. ARCHITECT selected *"Markdown table"*: one row per item, free columns.
5. **Part 10, reply.** Closed. ARCHITECT selected *"Both"*: the reply carries the selected row's columns as key=value. nvim listens. `eve finder` waits and prints to stdout.

## Language / Framework Constraints

LANGUAGE.md, C++ / JUCE: one object, one translation unit (`LANGUAGE.md:34-35`). Each new object is one header and one `.cpp`.

## Dependency & API Inventory

Each part lists its own. Doxygen note: `jam/docs/xml` is stale for `AudioModel` (`classjam_1_1AudioModel.xml` gives lines 536 and 549, `jam_AudioModel.h` gives 540 and 553), and `eve/docs/xml` is five lines behind `EVETerminalProcessor.cpp`. COUNSELOR regenerates before it cites a location from the XML.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Delete the old plans
**Scope:** `PLAN-1-preview-protocol.md`, `PLAN-3-file-list.md`, `PLAN-4-markdown-panel.md`, `PLAN-5-open-mode.md`, `PLAN-6-list-mode.md`, `PLAN-terminal-bugs.md`, `PLAN-host-client-state.md`
**Action:** @Engineer deletes the seven files. ARCHITECT: *"delete completed/supersed plan to avoid confusion, or amend/update the content as tracker, where are we now."*
**Validation:** The sprint log closes PLAN-1, PLAN-terminal-bugs and PLAN-host-client-state (`SPRINT-LOG.md:124`, `:192`, `:271`, `:332`). The canon above replaces PLAN-3 to PLAN-6. Their open checks move to *Carried checks* below.

### Step 2: Record the canon in SPEC
**Scope:** `SPEC.md:13-16`, `:76-81`, `:355-358`, `:360-471`, `:715-728`, `PLAN-Grand-Scheme.md:793-820`, `README.md:78`
**Action:** COUNSELOR replaces the sidebars, the file list, OPEN, LIST, the panels and the markdown panel with the canon above, with ARCHITECT's words. `PLAN-Grand-Scheme.md` keeps its history and takes an *AMENDED 2026-11-07* note at `:793`, as `:8` shows.
**Validation:** No line of `SPEC.md` names `juce::SidePanel` or `juce::FileTreeComponent` as current.

### Step 3: Point the project file at the tracker
**Scope:** `CLAUDE.md`
**Action:** COUNSELOR replaces the *Plans in flight* sentence with this file's name.
**Validation:** `CLAUDE.md` names no deleted plan.

## Carried checks (ARCHITECT runs)

From `PLAN-1-preview-protocol.md`, Acceptance:

1. `imgcat a.png` at the prompt: the image sits in its cell box, and text below stays on the grid.
2. `chafa -f iterm a.png`, `chafa -f sixel a.png`, `kitten icat a.png`: the same box as kitty shows.
3. `fzf --preview 'eve preview {}'`: the image fills the box, the GIF loops in place, the terminal does not stall. A text file shows no image. `escape` removes the image.
4. Scroll, resize, zoom: the image stays on its cells. Text shows over it. An erase sequence ends it, and a new image in the same cells replaces it.
5. `eve preview a.png width=20`.
6. `kitten icat --unicode-placeholder a.png` survives a redraw.

From `PLAN-1-preview-protocol.md`, Risks: a box in a block split keeps only the part in its owner block (`jam_TerminalSwap.cpp:255-267`), and the fix waits for a report from a run. A second Kitty `a=p` placement of one id moves the box.

From `PLAN-host-client-state.md`, Verification:

1. `exit` in a focused pane, then in an unfocused pane: that pane closes. Last pane: the tab closes. Last tab: END quits.
2. `exit` in standalone EVE: the application quits with its state saved.
3. Tab name follows `title`, then `foregroundProcess`, then `cwd`. A split starts in the inherited `cwd`.
4. Zoom, mode and focus under END still work. Resize a pane: no shell restart, no lost history.
5. Load EVE as VST3, then as AU, and repeat 1 and 3.

Also from that plan: `Session.cpp:108` gates `PluginState` by company name (Step 8.5). ENAX is formalised later with CLAP (`end/SPEC.md:38`).

From `PLAN-terminal-bugs.md`, Verification:

1. Ctrl+G from a long Claude Code prompt, then quit nvim. Expect no overlaid rows. Scroll up. History is intact.
2. nvim: `i` gives a bar. Esc gives a block. Quit, and the shell has a steady block.
3. Drag one file, then several with spaces in their names, to the prompt. Expect quoted paths, joined by a space, with no Enter.
4. Open EVE under PaperWM many times after the zshrc edit. Expect no OMP error, and one prompt redraw after the tile settles.
5. Drag the window corner, and pause during the drag. Expect no winsize change until release.

MACHINIST items from that plan, for ARCHITECT to send to MACHINIST:

1. `~/.config/zsh/zshrc:181`: replace `TRAPWINCH() { omp_repaint_prompt }` with `_omp_arm_repaint() { TRAPWINCH() { omp_repaint_prompt }; add-zsh-hook -d precmd _omp_arm_repaint }` and `add-zsh-hook precmd _omp_arm_repaint`. The hook is added after `_omp_precmd`, so `_omp_status` has a value when the trap can fire.
2. `~/.config/nvim`: change the highlight or the query for `shortcut_link` (`markdown_inline/highlights.scm:81`) and `pipe_table_header` (`markdown/highlights.scm:30-31`), so a pandoc grid table does not show them.

Debt: `DEBT-20261105T000000` stays in `DEBT.md` (Windows runtime test).

## BLESSED Alignment

- **Bound:** EVEView owns each ephemeral component (rulings 16 to 19).
- **Lean:** one Finder widget serves files, programs and actions (ruling 48).
- **Explicit:** every decision line here quotes ARCHITECT.
- **SSOT:** this file is the one tracker. Each part cites it.
- **Stateless:** state is Model rows. Views react (the contract above).
- **Encapsulation:** JAM holds the generic widget. EVE wires it by registration (ruling 47).
- **Deterministic:** the same rows give the same view.

## Risks / Open Questions

None open. Each part locks alone, on ARCHITECT's approval.
