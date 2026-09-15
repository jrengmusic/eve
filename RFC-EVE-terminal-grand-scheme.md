# RFC: END — Terminal Grand Scheme

**Project:** END — the terminal, standalone audio plugin
**Location:** `~/Documents/Poems/dev/end/` (clean slate — empty at RFC time)
**Author:** COUNSELOR, transcribing ARCHITECT's rulings — 2026-08-29
**Status:** Founding document. Every claim carries a citation; every ruling is ARCHITECT's.

---

## 0. Mental Model — Read This First

**We are not developing existing terminal architecture.** The design in this document
is above and beyond agent training data. Every mainstream terminal emulator surveyed
below — kitty, ghostty, wezterm, alacritty, iTerm2 — fails the property END exists to
deliver. Training priors about "how terminals work" describe the failed designs, not
this one. An agent that reaches for the familiar terminal mental model is reproducing
the documented failure.

**This is also not greenfield.** ARCHITECT has more than a decade of expertise in
exactly this field: production real-time audio architecture (where lock-free hot
paths are mandatory, not optional) and sophisticated UI/UX engineering. Every
component of this design either already exists as working code in jam/endless/nexus
or has a cited external precedent. The composition is the novel part.

**The one-sentence design:**

> The terminal is a pure producer with audio-thread discipline; the content lives in
> an append-only logical-line SSOT (a jam::Document Model); the widget is a pure view
> that derives the wrapped layout per current width at paint. Resize re-derives
> layout from logical lines — there is nothing to destroy.

**The isomorphism, stated once:**

| Audio plugin | END |
|---|---|
| Audio thread (never blocks, never allocates) | PTY reader thread: parser + bounded grid |
| Fixed-size processing block | The active grid — O(rows × cols), bounded by max window size |
| Lock-free FIFO to editor | CellFifo — SPSC rings, seqlock, drop-oldest |
| APVTS (atomic parameter store) | TerminalModel — lock-free DEC-mode parameters |
| Editor / analyzer (FFT display: pure view of state) | The text-editor widget: renders the buffer, owns nothing |
| Processor state serialization | Session persistence via daemon (Nexus transport) |
| Plugin instance | One terminal surface: one PTY, one buffer, one widget |
| Host (DAW) | Nexus |

No wait, no yield, no sleep, no contention on the producer — exactly like an audio
thread. The clock is controllable via a virtual audio device.

---

## 1. The Problem — Proven Unsolved, From Source

**Claim (ARCHITECT):** no available terminal survives window resize without
destroying content, and none decouples terminal state from rendering without locks.

**Verified from local source checkouts, file:line cited:**

| Terminal | Storage | Resize destroys? | State ↔ render coupling |
|---|---|---|---|
| **kitty** | Wrapped grid, `next_char_was_wrapped` per cell (line.h:62) | Yes — segments wider than new width skipped entirely (resize.c:224-227); widen copies `MIN(old,new)` cells, no recovery (resize.c:383) | Mutexes on write-buf + parser (screen.h:154, vt-parser.c:239); render reads cells **unsynchronized** — not SPSC, not a design |
| **ghostty** | Wrapped pages, `wrap`/`wrap_continuation` flags (page.zig:1953-1963) | Yes — discards cell managed memory on OutOfSpace (PageList.zig:1815-1828); deferred-reflow TODO admits the gap (:1273) | One mutex over the entire terminal state between IO and renderer (renderer/State.zig:13-17) |
| **wezterm** | Wrapped `VecDeque<Line>`, `wrapped()` cell attribute (screen.rs:24, line.rs:1088-1092) | Yes — alternate screen truncated/discarded outright (screen.rs:236-244); prunes lines, admits losing cursor↔line association (:149-151, :183-187) | `Mutex<Terminal>` (parking_lot) on every access (localpane.rs:126) |
| **alacritty** | Wrapped grid, `WRAPLINE` cell flag (cell.rs:28) | Yes — shrink discards truncated cells; narrow-then-widen does not restore (resize.rs:245-389, row.rs:71-87) | `Arc<FairMutex<Term>>` shared by PTY reader and renderer (sync.rs:11-49, window_context.rs:53) |
| **iTerm2** | **Scrollback = raw unwrapped logical lines**, wrap derived per width and cached (LineBuffer.h:242-250, LineBuffer.m:265-276) | Scrollback: **no**. Live + alternate grid: **cleared and reset** (VT100ScreenMutableState+Resizing.m:72-89) | Mutation thread owns state; main thread joins by **pausing the parser** (`performSynchroDanceWithBlock`, :5971-5987) — consumer blocks producer |

**Verdict:** zero of five have the full property. Four of five store the wrapped grid
as the truth — the lossy representation *is* their design. All five couple state to
rendering: four by mutex, iTerm2 by letting the renderer pause the parser. Nobody has
a processor/editor split where the widget can never block or destroy the content
model.

**The one partial insight in the field:** iTerm2's LineBuffer independently proves
the logical-line-SSOT half works at production scale — for scrollback only. Even
they stopped there: live grid destructive, threads joined.

**Each half of END's design is separately proven feasible** (iTerm2 the storage
half; endless/jam the lock-free transport half — see §2). **Nobody has composed
them.** The composition is END.

**Honest boundary:** alternate-screen content belongs to the application, which
redraws on SIGWINCH — that is the VT contract. No terminal can preserve what it does
not own. END's non-destruction guarantee covers everything END owns: primary-screen
content and the entire history.

---

## 2. Own Prior Art — Cited

### 2.1 endless (old END, OpenGL interface) — lock-free where the five others are not

Full lock audit of `~/Documents/Poems/dev/endless/Source`: exactly three locks,
**none on the state↔render path**:

1. `Processor::callbackLock` (Processor.h:327) — its own doc: *"Same implementation
   as AudioProcessor::suspendProcessing"* (Processor.cpp:359-360). The standard JUCE
   plugin lifecycle gate on entry into `process()` (reader thread at
   ProcessorEvents.cpp:422, message ingress at Session.cpp:350). Paint never takes it.
2. `Daemon::connectionsLock` (nexus/Daemon.h:413) — IPC connection registry.
3. `WindowsTTY::writeLock` (WindowsTTY.h:347) — serializes writes *to* the child PTY.

Content flows parser → render lock-free. endless proves the transport discipline —
and still produces "garbage text when resizing just like other terminals," because
its storage model is still the wrapped grid. Storage was the missing half.

### 2.2 nexus (the HOST — old END's app shell) — the thread contract already written

`~/Documents/Poems/dev/nexus/ARCHITECTURE.md:116-118` documents a three-thread
contract in ARCHITECT's own hand:

| Thread | Touches | Forbidden |
|---|---|---|
| **Reader** (TTY, high priority) | terminal::Model atomics, Buffer\<Row\> cells, CellFifo push | ValueTree, CodeModel, mutex, allocation, block |
| **Timer** (JUCE) | flush dirty atomics → ValueTree | Buffer\<Row\>, CodeModel |
| **Message** (main) | ValueTree via listener, CellFifo drain | atomics (except flush) |

Plus: "The paint path NEVER writes" (:127); ValueTree exclusive to message thread
(:133). This is the discipline END formalizes — already proven in a shipped app.

### 2.3 The ValueTree lesson

nexus/endless used `jam::Model` (ValueTree) as the content-adjacent Model. Failure
mode, ARCHITECT-diagnosed and structurally confirmed: string/`juce::var` allocation
per property write, listener machinery per node — allocation-per-write by
construction. Agents under pressure hack around it and regress into the terminal
mental model. ValueTree remains correct for *scalar/mode state* (the APVTS analog);
it is disqualified as the *content* store.

---

## 3. The Architecture

### 3.1 The bounded hot path — the load-bearing fact

VT semantics — cursor addressing, SGR, erase, scroll regions — are defined **only
within rows × cols**. The mutation surface is O(grid): fixed, small, bounded by max
window size, like an audio block. Everything that scrolls off the top is **retired:
complete, immutable, append-only**. Above the grid is storage, not processing.

Two rates, two owners:

- **Byte rate** (hot): reader thread mutates the bounded active grid. Transient
  working state, MANIFESTO **S** — lives and dies within the operation.
- **Scroll rate** (cold): retired lines cross the FIFO once, complete at creation —
  MANIFESTO **E**'s happiest case. The SSOT ingests them append-only.

### 3.2 Terminal ≙ text editor, simplified

A terminal is TEXT with a caret, history, scrollback — a text editor whose edit
pattern is *simpler* than an editor's: mutation confined to the bounded tail,
everything above write-once. The mental-model shift, verbatim ARCHITECT: *"terminal
only produces; cache and buffer belong to a STATE."*

### 3.3 The SSOT — logical lines in a jam::Document Model

**Ruling: the content SSOT stores width-independent logical (unwrapped) attributed
lines.** The wrapped grid at any width is a *derived layout* — a materialization,
rebuilt from logical lines on resize. Wrapping is lossy; wrap flags on a wrapped
grid (the approach of kitty/ghostty/wezterm/alacritty, and of the current CellFifo
`isContinued` marker) cannot round-trip narrow→widen. Logical lines can.
Precedent at production scale: iTerm2's LineBuffer (§1).

**Why jam::Document is the Model:**

- The Document engine is the framework's generic parser: a domain injects its
  Vocabulary (character sets) and Rules (token-consumption hooks) and produces
  typed Tokens and an Element tree (jam_Document.h:20-24). Every existing domain —
  XML, HTML, CSS, Markdown, Mermaid — processes **bytes** through exactly this shape.
- It is allocation-disciplined: contiguous UTF-8 source, cursor-walk tokenization,
  `Owner<Element>` arena with **stable Element addresses across insertion** (jam_Document.h:25-28)
  — insertion post-construction is architecturally supported. This is the profile
  ValueTree lacks.
- It is fast, proven this month: CAST parses, resolves, validates and generates
  thousands of LOC subsecond, one shot. jam::Document/Markdown was hardened by the
  CAST self-hosting sprints immediately before this RFC.
- Placement resolves all prior objections: the Document lives **message-side only,
  single-threaded, append-only, at line-retirement rate** — never shared across
  threads, never touched by the hot path. Appending a retired line is *creation*
  ("complete at creation"), not mutation.

> **[AMENDED 2026-09-15]** Ruled and proven by measurement (buffer-bench sprint,
> tests/buffer_bench/artifacts/results.md): line lookup is `Document::Index` — a
> nested counted B-tree (arity 16) over the line-element chain, index-only
> (Element* leaves, zero content copies). Cold jump and scroll beat every
> chain-only strategy by 4–5 orders of magnitude at 10⁶–10⁷ lines; append tax
> nil. Measured constants: 310–850 B/line resident (varies with measurement
> point across runs — results.md:3, results-run1.md:3, results-run2.md:3),
> 89 B/line wire-encoded, ~100 µs cold-page rehydrate, spill write
> ~1.2 M lines/s.

### 3.4 TerminalParser is already a Document-domain parser

The correspondence is structural, not nominal:

| jam::Document engine | TerminalParser (jam_terminal/parser/) |
|---|---|
| Vocabulary — character sets | VT byte classes: C0, intermediates 0x20-2F, parameters 0x30-3F, finals 0x40-7E |
| Rules — consumption hooks | DispatchTable: O(1) `(state, byte) → (nextState, action)` |
| Tokens — typed spans | Print runs, CSI/OSC/DCS/APC sequences |
| Cursor walk | Byte-at-a-time DFA with UTF-8/OSC/DCS accumulators |

Both are one construct: a DFA over a character-class vocabulary with tabled
consumption rules emitting typed tokens. The VT parser is *specified* in the
literature as exactly that table.

**Ruling: TerminalParser stands as-is** — the VT domain's byte-level parser,
parallel to how CssDocument/MarkdownDocument each own their domain tokenization
(all domains parse raw bytes: e.g. `parse (binary.data, binary.size)`,
jam_Css.cpp:85). It is not rebased onto the Document engine. Its output splits into
two token species:

- **Content tokens** (print runs → text): Document material.
- **Imperative tokens** (cursor, erase, scroll): grid commands driving the bounded
  working grid — never document structure.

### 3.5 ANSI is a wire encoding — at exactly two edges

Between the edges, everything lives as clean UTF-8 + structure, the same substance
Markdown or HTML hold after their parse:

```
PTY esc bytes ──▶ TerminalParser (decode) ──▶ bounded grid ──▶ retirement
                                                                  │
                          message side                            ▼
   widget ◀── per-width layout (derived) ◀── Document SSOT ◀── CellFifo
     │
     ├──▶ pixel backend: glyphs (jam glyph pipeline → Vulkan/software LLGC)
     └──▶ terminal backend: cells → GraphicsEngine (encode) ──▶ esc bytes out
```

Decode in, encode out. `ANSI ↔ UTF-8`, materialization per backend — same law as
`"M 57 d 55 Z"` → `juce::Path`.

### 3.6 The pipeline, end to end

```
READER THREAD (audio discipline: no lock, no wait, no yield, no sleep, no alloc*, drop-oldest)
  TTY (posix/windows) ─▶ TerminalParser ─▶ imperative tokens ─▶ bounded active grid
                                        └▶ content: cells stamped (AttributedChar + Stamp styleId)
  line retirement ─▶ CellFifo push (history ring)     [*grid buffers preallocated at resize]
  live viewport   ─▶ CellFifo push (active ring)
  mode changes    ─▶ TerminalModel (lock-free Parameter writes — the APVTS analog)

MESSAGE THREAD
  CellFifo drain ─▶ append retired logical lines to Document SSOT (creation, append-only)
  widget: derives wrapped layout for current width from SSOT + active ring ─▶ paint
  resize: re-derive layout. Content untouched. Nothing destroyed.

RENDER BACKENDS (swapped at one line: juce::Graphics { context })
  vulkan::LowLevelGraphicsContext  — full juce parity, exists
  terminal::GraphicsContext/Engine — cells → esc bytes (LLGC geometry completion pending)
  juce software renderer           — behavior oracle for comparison
```

Text never flows through juce `drawGlyphs` on the terminal backend — grid placement
is UAX #29 segmentation + UAX #11 width arithmetic (advance ∈ {1,2}, wrap at column
bound, baseline += 1 row), via jam's own glyph pipeline (terminal::TextLayout,
AttributedChar). Cell units need no font metrics. Amendment 16/19 stands

> **[AMENDED 2026-09-13]** terminal::TextLayout was deleted in the AnsiDocument
> sprint; the write lane is AnsiDocument → GraphicsContext (drawCells/
> drawAttributedText).
(jam_TerminalGraphicsContext.h:351-353). The layout arithmetic is *identical* to
pixel layout — accumulate advance, break at bound, step baseline — only the unit and
metric source differ, and in cells every quantity is integral.

### 3.7 Resize — the property, proven by construction

- History: logical lines, width-independent → re-wrapped at paint for any width,
  losslessly, round-trip narrow↔widen exact.
- Live primary screen: bounded grid rewrapped from logical knowledge at retirement
  boundaries; the app-owned alternate screen redraws itself per the VT contract.
- SIGWINCH path: TTY `TIOCSWINSZ` → processor resizes bounded grid → widget
  re-derives layout. The widget never owned content, so it can destroy none.

---

## 4. Ecosystem — END and Nexus

### 4.1 Roles

- **Nexus** (`~/Documents/Poems/dev/nexus/`) — the HOST. The old END application
  role. Owns: window, tabs/panes mux, sessions composition, modals, config, fonts,
  daemon. Has its own overhaul track — jam-structure adherence, lua → markdown
  tables + CSS, CAST manifest conformance (nexus/RFC-config-migration.md exists;
  PLAN-END-plugin-host.md exists at nexus root). **Out of END's scope.**
- **END** (`~/Documents/Poems/dev/end/`, this project) — the TERMINAL, standalone
  audio plugin. Feature-modular sibling: Whelmed (markdown/mermaid) follows the same
  model, built from the exact same jam modules.

### 4.2 Mux boundary — ruled, cited

**END is a single-surface terminal plugin: one PTY, one buffer, one widget per
instance. Nexus composes instances.** Cited from nexus source: the state tree
FLEX → TAB → PANE/EDGE rows where each EDGE's head/tail names a SPACE (a PANE or
another EDGE) — the working binary-split structure (nexus/ARCHITECTURE.md:263-276);
`ID::focusedPane` focus routing (:74); per-pane APVTS bridge "one per pane, paired
under the PANE" (:85-86). One plugin instance = one voice; the host is the mixer.

### 4.3 Hosting contract (final goal, design-ahead only)

END is developed and validated **standalone first** (dodges the two known blockers:
jam clap modules half-assed; host Model complexity). Final goal: hosted into Nexus
via API — CLAP extension is the candidate mechanism — for:

- **Vulkan swapchain sharing** (host-owned `jam::VulkanEngine` resource tree,
  nexus/ARCHITECTURE.md:19),
- **state transport**: serialization, session persistence with the daemon
  (daemon lives host-side; endless/nexus Daemon is the precedent).

Any format should ultimately be supportable; CLAP is primary.

---

## 5. jam Inventory — Kill / Keep / Gap

### 5.1 Kill-list (wipe clean — clean-slate ruling)

| Remnant | Why |
|---|---|
| `jam::TextEditor` (jam_terminal/widgets/jam_TextEditor.h, 41 lines) | Base-only, no editing machinery; its contract is a `readOnly` flag + the `getTerminalContext` dynamic_cast leak (:31-34). One subclass. |
| `jam::MarkdownComponent` (43 + 279 lines) | Dual-path paint branching per block on backend; colours baked at construction from the wrong LookAndFeel (jam_MarkdownComponent.cpp:5-7 — the 601-assert bug, unfixable in this shape). |
| `jam::MarkdownProjection` | Blocks store **resolved** colours (jam_MarkdownProjection.cpp:45-185) — appearance materialised into the Model. The projection concept survives; this implementation dies. |

Confirmed absent (nothing to wipe): no CodeView/CodeEditor/Code-model classes exist
in jam; no editable text buffer exists anywhere — the buffer is pure greenfield.

### 5.2 Keep-list (complete, thread-verified building blocks)

- **Atoms:** `AttributedChar` (packed 8-byte: codepoint 21b / contentTag / widthClass
  / styleId 16b / hyperlinkId 16b), `Stamp` (style interning), `Grapheme` (cluster
  interning), `Hyperlink` (OSC 8), `AttributedString` (immutable runs, `toJuce()`).
  Shared across terminal / Vulkan / markdown.
- **Processor side (reader thread):** `TerminalParser` + DispatchTable/CSI (complete
  VT DFA), `TerminalVideo` (full VT execution — cursor/mode state only, no grid),
  `TerminalTTY` posix/windows, `TerminalSkit` + Sixel/Kitty/iTerm2 decoders + GIF.
- **Transport:** `TerminalCellFifo` — three SPSC rings (history/active/popback),
  per-entry seqlock, drop-oldest. Production-complete.
- **Mode state:** `TerminalModel` — lock-free parameter writes, DEC-mode bimap SSOT.
- **Input encoders:** `TerminalKeyboard` (VT/xterm/progressive), `TerminalMouse`
  (X11/UTF-8/SGR).
- **Glyph pipeline:** `terminal::TextLayout` (UAX #29 + width arithmetic, Amendment 17).

  > **[AMENDED 2026-09-13]** terminal::TextLayout was deleted in the AnsiDocument
  > sprint; the write lane is AnsiDocument → GraphicsContext (drawCells/
  > drawAttributedText).
- **Pixel backend:** `jam::vulkan::LowLevelGraphicsContext` — full juce LLGC parity
  (dedicated Glyph/Path/Image/Transparency TUs).
- **Document engine:** `jam::Document` + domain parsers, CAST-hardened.

Caveat, ARCHITECT-ruled: these parts were **never assembled into a fully working
terminal**. Subsystem completeness ≠ architecture proven. The assembly is END.

### 5.3 Gap-list (to build — the actual new work)

1. **THE BUFFER** — the Document-domain content SSOT (§3.3): append-only logical
   attributed lines, message-side, populated from CellFifo drains, serving per-width
   layout derivation. The centerpiece.

   > **[AMENDED 2026-09-15]** Design closed by measurement: Document stays the
   > SSOT (no rewrite, ARCHITECT-ruled); `Document::Index` supplies lookup and
   > owns the residency tier (1/16-RAM hot window, watermark eviction,
   > wire-byte spill, rehydrate). Implementation: PLAN-document-index.md sprint.
2. **The widget** — plain `juce::Component` editor as pure view over the buffer;
   read-only static mode; zero backend knowledge. Replaces the killed TextEditor/
   MarkdownComponent.
3. **Terminal LLGC completion** — geometry parity only (text goes through jam's
   pipeline, §3.6). Six stubs at jam_TerminalGraphicsContext.h: `fillPath`/
   `clipToPath` (:344, :332), `drawImage`/`clipToImageAlpha` (:345, :333),
   transparency layers (:341-342), plus trivial `getPreferredImageTypeForTemporaryImages`/
   `getFrameId` (:360, :366). Reference: the Vulkan twin's structure; oracle: juce
   software renderer. Image degradation ladder: Kitty/`END;` protocol where the
   receiver supports it (END itself always does — both ends of the wire are ours),
   half-block cells (▀, 2px/cell) on dumb terminals.

   > **[AMENDED 2026-09-13]** Kitty is ruled dead — stateful, bidirectional,
   > renderer-aware, not a writer's protocol. The shipped ladder is iTerm2
   > OSC 1337 (TerminalITerm2Encoder, decoder round-trip proven) + half-block
   > fallback.

   > **[AMENDED 2026-09-13]** Gap 3 is executed this sprint — all placeholders
   > resolved under the cell law, proven by the llgc_parity harness. Gap 4
   > (Vulkan cell-grid adapter) remains open — viewer text currently routes
   > through stock TextLayout shaping.
4. **Vulkan cell-grid adapter** — AttributedChar grid → glyph atlas rendering path
   in jam_vulkan (does not exist; endless renders in-app today).
5. **Markdown as projection into the buffer** — Whelmed's producer: Document parse →
   attributed logical lines with style *roles* (never resolved colours), same buffer,
   same widget.

Related defect, already root-caused this session (fix rides the rewrite): cast
`--help` breaks because MarkdownComponent bakes colours pre-`setLookAndFeel`
(601× juce_LookAndFeel.cpp:94) and style.css carries one stale id
(`--Markdown--syntaxTypeColourId`, removed from ColourIdMap — enum gap at 0x4400012)
and lacks `--Markdown--tableHeaderBackgroundColourId`.

---

## 6. Validation Strategy

Two jaws of the vise; both converge on the backend-swap proof:

1. **CAST --help** — the smallest live consumer: one-shot, headless, no host, no
   CLAP, no Vulkan. Validates: terminal LLGC output side + buffer + read-only widget
   + markdown projection. Currently broken; the oracle is free (HELP.md rendered
   right, style.css colours landing).
2. **END standalone** — the terminal itself. Validates: full processor side against
   endless's known-good behavior, the buffer under real PTY load, **the resize
   property** (the headline test: narrow→widen round-trip, byte-exact history),
   SKiT, Vulkan cell adapter.
3. **Whelmed standalone** — same buffer/widget on the pixel backend with editing and
   markdown/CSS config; proves renderer-agnosticism (one component, two backends)
   and pilots the lua→markdown config pattern for Nexus.
4. **CLAP / Nexus hosting** — last, after the foundation is proven; unblocks on the
   jam clap hardening track.

Acceptance test for the core property, stated now: fill a session with known
content; resize narrow, then back; the SSOT and the re-derived layout are
byte-identical to the original. No surveyed terminal passes this today (§1).

---

## 7. Open Design Surfaces (deliberate, not blockers)

1. **Attribute representation in the Document SSOT** — retired lines arrive as
   AttributedChar runs (text + styleId spans); the Element-level representation of
   style spans is a design decision to make deliberately at SPEC/PLAN time, not
   hack. Style *roles*, never resolved colours (the MarkdownComponent lesson).
2. **Streaming Document unification** — teaching the Document engine resumable
   parse so TerminalParser becomes a literal Document domain. Real but separable
   engine work; the retirement-boundary wiring (§3.4) needs neither seam crossed.
   Deferred unless ARCHITECT pulls it forward.
3. **Path/image degradation policy on the ANSI backend** — per-stub policy table
   (§5.3.3) to be fixed in SPEC.
4. **Scrollback bound / spill-to-storage** — "above the grid is storage"; the
   storage tier (memory cap, disk spill, daemon persistence) is a Nexus-transport
   era decision.

   > **[AMENDED 2026-09-15]** Deferral withdrawn by ARCHITECT. Tier designed and
   > measured: cold lines spill as ANSI wire bytes (byte-fixpoint proven,
   > 89 B/line); hot-window budget defaults to 1/16 machine RAM
   > (config-overridable via `terminal / scrollbackBudgetMb`); eviction by
   > watermark (high = budget, low = 7/8 budget); rehydrate ~100 µs per 50-line
   > page. Daemon persistence remains Nexus-era.

---

## 8. Non-Negotiables (agent contract for every future session)

- Reader thread: no ValueTree, no Document, no mutex, no allocation, no block, no
  yield, no sleep. (nexus/ARCHITECTURE.md:116 — already law once; now again.)
- The widget owns no content. The paint path never writes.
- The SSOT stores logical lines, width-independent. A wrapped grid is never the
  truth.
- ValueTree/jam::Model is for scalar/mode state only — never content.
- ANSI appears at two edges only. Between them, UTF-8 + structure.
- Style roles in state; colour resolution at materialization. Never bake appearance
  into the Model.
- No terminal mental model from training data. When in doubt, re-read §0 and §1.
- BLESSED governs; CAROL contract applies; scope is ARCHITECT's alone.

---

## 9. Source Citations Index

- kitty: `~/Documents/Poems/dev/kitty/` — line.h:62; resize.c:83, 224-227, 383;
  screen.h:154; vt-parser.c:239, 1568-1638; screen.c:4067, 4093
- ghostty: `~/Documents/Poems/dev/ghostty/` — page.zig:1953-1963;
  PageList.zig:1273, 1638-1836, 1815-1828; renderer/State.zig:13-105
- wezterm: `~/Documents/Poems/dev/wezterm/` — screen.rs:24, 100-244;
  line.rs:1088-1092; localpane.rs:126; mux/lib.rs:104
- alacritty: `~/Documents/Poems/dev/alacritty/` — cell.rs:28; resize.rs:101-389;
  row.rs:71-87; sync.rs:11-49; event_loop.rs:51, 117-268; window_context.rs:53
- iTerm2: `~/Documents/Poems/dev/iterm2/` — LineBuffer.h:242-250, 325-326;
  LineBuffer.m:265-276; LineBlock.h:22-23, 105-131;
  VT100ScreenMutableState+Resizing.m:37-89, 141-165, 515-580;
  VT100ScreenMutableState.m:5943-6051
- endless: `~/Documents/Poems/dev/endless/Source/` — Processor.h:327,
  Processor.cpp:355-368; ProcessorEvents.cpp:415-426; Session.cpp:344-357;
  nexus/Daemon.h:413; tty/WindowsTTY.h:347
- nexus: `~/Documents/Poems/dev/nexus/` — ARCHITECTURE.md:19, 74, 85-86, 116-147,
  235-276; PLAN-END-plugin-host.md; RFC-config-migration.md
- jam: `~/Documents/Poems/dev/jam/` — jam_Document.h:20-34, 679-694;
  jam_TerminalGraphicsContext.h:332-370 (stubs), :351-353 (Amendment 16/19);
  jam_TerminalGraphicsEngine.h:164-172; jam_TerminalCellFifo.h:105+;
  jam_TerminalParser.h:112-287; jam_TerminalModel.h:129-242; jam_TextEditor.h:31-34;
  jam_MarkdownComponent.cpp:5-7; jam_MarkdownProjection.cpp:45-185;
  jam_StyleManager.cpp:8-14, 153-199; jam_Css.cpp:73-90;
  jam_AttributedChar.h; jam_Stamp.h:68-132; jam_TerminalSkit.h:95+
- JUCE: `~/Documents/Poems/JUCE/modules/juce_graphics/contexts/juce_LowLevelGraphicsContext.h:64-193`;
  `juce_gui_basics/lookandfeel/juce_LookAndFeel.cpp:86-96`

---

**ARCHITECT is supreme on decisions and judgment. Facts, cited, are the only
override.**

**JRENG!**
