# RFC: EVE — Terminal Grand Scheme

**Project:** EVE — the terminal unit, an audio plugin hosted at END
**Location:** `~/Documents/Poems/dev/eve/` (clean slate — empty at RFC time)
**Author:** COUNSELOR, transcribing ARCHITECT's rulings — 2026-08-29
**Status:** Founding document. Every claim carries a citation; every ruling is ARCHITECT's.

> **[AMENDED 2026-10-04 — names]** ARCHITECT: *"END is host, not terminal. the
> terminal unit is EVE as plugin unit hosted at END"*; *"just like whelmed a
> markdown/mermaid renderer/editor unit hosted at END"*. At RFC time this
> project was named END and the host was named Nexus. This document now names
> the terminal unit EVE (`~/Documents/Poems/dev/eve/`) and the host END
> (`~/Documents/Poems/dev/end/`). A lowercase `nexus` path, with its line
> numbers, cites the host tree as it was at RFC time
> (`~/Documents/Poems/dev/nexus/`); that directory no longer exists. `Nexus`
> is today the host class inside END (`end/ARCHITECTURE.md:204`).

---

## 0. Mental Model — Read This First

**We are not developing existing terminal architecture.** The design in this document
is above and beyond agent training data. Every mainstream terminal emulator surveyed
below — kitty, ghostty, wezterm, alacritty, iTerm2 — fails the property EVE exists to
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

| Audio plugin | EVE |
|---|---|
| Audio thread (never blocks, never allocates) | PTY reader thread: parser + bounded grid |
| Fixed-size processing block | The active grid — O(rows × cols), bounded by max window size |
| Lock-free FIFO to editor | CellFifo — SPSC rings, seqlock, drop-oldest |
| APVTS (atomic parameter store) | TerminalModel — lock-free DEC-mode parameters |
| Editor / analyzer (FFT display: pure view of state) | The text-editor widget: renders the buffer, owns nothing |
| Processor state serialization | Session persistence via daemon (END transport) |
| Plugin instance | One terminal surface: one PTY, one buffer, one widget |
| Host (DAW) | END |

No wait, no yield, no sleep, no contention on the producer — exactly like an audio
thread. The clock is controllable via a virtual audio device.

> **[AMENDED 2026-10-20 — the isomorphism, current state]** The table above
> names parts that no longer exist. The current map, read from source:
>
> | Audio plugin (JFS) | EVE today |
> |---|---|
> | `FilterStripProcessor` (plugin processor) | `EVEProcessor` |
> | `FilterStripAudioProcessor` (the chain) | `EVETerminalProcessor` — ARCHITECT: *"EVEAudioProcessor should be renamed as EVETerminalProcessor but treated exactly like JFS AudioProcessor, IDENTICAL topology, identical ownership, treading etc"* |
> | `processBlock` (the clock) | `processBlock` — *"processBlock drains the bytes"*; *"writing bytes is processBlock"* |
> | `juce::dsp::ProcessSpec` at `prepare` | the terminal spec at `prepare`: *"exactly like process spec for init"* |
> | `setOversamplingFactor` → `setFactor` (run-time resize, `FilterStripAudioProcessor.cpp:103-128`, `jam_Oversampler.cpp:66-91`) | `setWinsize`: *"setFactor IS setWinsize, it's resizing buffer for processing"* |
> | `AudioBuffer` block | `jam::AnsiBuffer` (chain byte block) |
> | Lock-free FIFO to the editor (`SpectrumFIFO`) | `jam::TerminalFIFO` over `juce::AbstractFifo` (one reader, one writer: `juce_AbstractFifo.h:40-42`) |
> | APVTS + instance PARAM nodes | the terminal state machine (DEC modes, cursor, margins, pen): *"it belong to APVTS. the processor state. non automatable parameter. stateInfo"* |
> | `parameterChanged` → `Function::Map` setter → `calc()` (`FilterStripAudioProcessor.cpp:155-159`, `TETRIS.md:139-160`) | a VT command → `Function::Map` entry → state update (`jam_AnsiDocument.cpp:572-583`, `:620-636`): *"command is command… how the internal handles it are ISOMORPHIC"*. The entry point differs: a command arrives in the byte stream, in order, never through `parameterChanged`. |
> | Analyzer pulls through `userInterfaceGetters` on a timer (`kuassa_Analyzer.h:301-305`) | `TextEditor` pulls the history lane and the live lane through `userInterfaceGetters` on a timer |
> | `getStateInformation` / `setStateInformation` | the same — the state machine is the state blob; END is the host |
>
> `TerminalParser`, `TerminalCellFifo`, `TerminalModel` and `TerminalVideo` are
> deleted (§5.1 [EXECUTED 2026-09-20]; `jam_terminal/parser/` holds only
> `.gitkeep` and `jam_TerminalCSI.h`, neither included by `jam_terminal.h`).
> ARCHITECT: *"our terminal parser now is AnsiDocument"*.
>
> Transport law: ARCHITECT, *"there is no try-lock. ALWAYS LOCK FREE"*; *"READ
> HOW JUCE DID IT. why the fuck we need to redesign atomic transport?"*. The only
> crossing primitive is `juce::AbstractFifo`: two `juce::Atomic<int>`, one atomic
> read or store per call, no loop, no yield (`juce_AbstractFifo.h:319`,
> `juce_AbstractFifo.cpp:67-140`). Data waits only for two clocks: the
> `processBlock` period and the `TextEditor` timer.
>
> **[AMENDED 2026-10-27 — two state lanes, one tree]** The row *"APVTS +
> instance PARAM nodes | the terminal state machine"* above is replaced. The
> terminal state machine was nine APVTS parameters; REAPER listed them as host
> parameters, and the clock wrote them through `setValueNotifyingHost`, which
> takes a lock. ARCHITECT: *"using setValueNotifyingHost is smell, this mean
> we're still using automatable parameters … what i really mean with
> non-automatable parameter is PARAMETER tree still attached to APVTS but it's
> not audio parameter, still could be event driven with
> valueTreePropertyChanged. this like panel component (ui size, oversampler,
> etc) on JFS"*.
>
> | Audio plugin (JFS) | EVE today |
> |---|---|
> | APVTS, `parameters.md` (automatable host parameters) | the AudioModel lane: automatable parameters only (`zoom`, `bypass`) |
> | panel and settings state in the model tree, event driven | the Model lane: `jam::Model`, rows in `model.md`, built by `jam::ModelLayout`, its tree the `terminal` child of the APVTS state |
>
> - ARCHITECT on the lanes: *"AudioModel for true audio parameter lane, as
>   automatable parameter, while Model lane for any event driven state machine
>   that is non automatable, real time safe on its own thread, guaranteed
>   thread safe on other thread, always concurrent with atomic, think of it
>   like reverse direction of APVTS. dont treat it like written on stone"*.
> - ARCHITECT on the direction: *"IIUC EVE is the other way around,
>   everything starts at Audio thread, it's processor events that drives how
>   widget rendered."*
> - ARCHITECT on the scope: *"we only need to build jam::Model as Terminal
>   complete state machine, all terminal events including OSC 133 and future
>   shell integration is jam::Model parameter, still attached to APVTS hence
>   it's a valid stateInfo that the framework guarantee for instance
>   serialisation and state preserverance"*.
> - ARCHITECT on the tables: *"parameters.md always for automatable
>   parameter, a parameter can host can automate. this is plugin_bootstrap
>   (that synced identically with KANJUT) contract that JFS use … the most
>   elegant way is actually BOTH, [1] is the bootstrap, then [2] only call to
>   build"*.
> - Precedent: endless — `terminal::Model : jam::Model`, *"reader thread
>   writes parameters, timer flushes to ValueTree, UI reads ValueTree"*
>   (`endless/Source/terminal/Model.h:17-28`). ARCHITECT: *"you could learn how
>   endless manage its terminal state machine … factually raw bytes test beats
>   all terminal in comparison"*.
> - The producer writes with `storeValue`: an atomic store and a dirty mark,
>   no lock, no allocation, no message. EVE flushes the Model at vblank, not on
>   a timer.
> - The clock: PTY readiness and the clock run on one serial context, and each
>   tick is a zero-sample block. The transport law above holds: no lock, no
>   wait.

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

**Each half of EVE's design is separately proven feasible** (iTerm2 the storage
half; endless/jam the lock-free transport half — see §2). **Nobody has composed
them.** The composition is EVE.

**Honest boundary:** alternate-screen content belongs to the application, which
redraws on SIGWINCH — that is the VT contract. No terminal can preserve what it does
not own. EVE's non-destruction guarantee covers everything EVE owns: primary-screen
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
(:133). This is the discipline EVE formalizes — already proven in a shipped app.

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

> **[AMENDED 2026-10-20 — the parser is AnsiDocument]** ARCHITECT: *"our
> terminal parser now is AnsiDocument"*. `AnsiDocument` is the decode-in half
> (`jam_AnsiDocument.h:1-9`; `jam_terminal.h:68-69`), a jam::Document domain.
>
> - **Vocabulary today:** text, SGR, CUF (`cursorForward`), OSC 8, OSC 1337,
>   newline (`jam_AnsiDocument.cpp:189-199`, `:620-630`). The CSI vocabulary
>   already names cursor position, erase, scroll and the scroll region
>   (`generated/jam_Terminal.h:916-1002`); they are not in the dispatch table yet.
> - **Command dispatch:** each command is a `Function::Map` entry keyed by its
>   final byte or SGR code (`jam_AnsiDocument.cpp:572-583`, `:620-636`) — the
>   isomorph of the JFS `parameters` map. Commands apply in stream order
>   (`build`, `:201-202`).
> - **State and content:** the state a command leaves (modes, cursor, margins,
>   pen) is the state machine, kept as processor state through stateInfo. The
>   cells a command clears or moves are content, never ValueTree (§8).
> - **Restartable parse:** `addLines` appends raw bytes; `Document::parse()`
>   tokenizes from `tokenStart` and stops before an incomplete UTF-8 tail; an
>   unterminated escape tail is kept for the next append (`jam_AnsiDocument.cpp`,
>   `addLines`, `build`).
> - **Allocation:** the build allocates (token array, line children, cells).
>   Thus `AnsiDocument` as a whole belongs on the message side (§3.3). The PTY era
>   splits it (§3.6 amendment).

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

> **[AMENDED 2026-10-20 — the pipeline on the JUCE plugin contract]** The
> pipeline above is replaced. The terminal writer does not wrap: ARCHITECT,
> *"wrapping and reflow is RENDERING concern. NOT Terminal Video job… it should
> just print to materialise not the layout engine… the widget, the Viewport:
> DECOUPLED"*. The "wrap at column bound" text above described the deleted
> `terminal::TextLayout`.
>
> **The contract.** ARCHITECT: *"NO POKING LAYERS. MUST BE ENCAPSULTED"*. JUCE
> plugin contract: *"View → Model (APVTS) → parameterChanged() → Chain;
> Visualizer pulls via getters"*, *"Visualization pulls; ProcessorChain never
> pushes"* (`TETRIS.md:79-99`). The View tells and pulls. It never writes into
> the chain.
>
> **Two rendering layers.** ARCHITECT: *"our Terminal LLGC is juce contract, can
> be used to paint any component into terminal context… we can use it as SSOT
> for our terminal round trip now. because we will WIRE PTY"*.
> `terminal::GraphicsContext` materialises cells, and `GraphicsEngine` writes
> bytes. The Vulkan LLGC (with its software fallback) renders pixels on the
> message thread. *"Paint is NOT MATERIALISATION. Paint is a call for whatever
> ALREADY MATERIALISED… materialisation can be done ANYWHERE"*.
>
> ```
> MESSAGE THREAD
>   EVEView tells: harness File reference, WINSIZE (instance PARAM node)
>   EVETerminalProcessor::load (File)  — the Convolution loadImpulseResponse shape
>     MemoryMappedFile (readOnly) → MarkdownDocument parse in place (the AST, chain-owned)
>     size cold resources (suspendProcessing → prepare lane)
>     chain-owned materialiser (TextEditor + StyleMarkdown, no wrap) paints once
>       into terminal::GraphicsContext → GraphicsEngine row ring (write handle)
>   TextEditor (bound) timer pulls: history lane → AnsiDocument SSOT (append)
>                                    live lane    → live AnsiDocument (replaced whole)
>
> AUDIO THREAD (processBlock — the clock)
>   apply pending winsize (setWinsize ≙ setFactor) — resize live row count in capacity
>   GraphicsEngine::process(): rows above the newest winsize.rows → history lane bytes
>                              newest winsize.rows rows → live lane bytes (on change)
>
> IMAGELOADER POOL
>   file decodes (queued at load); completion → callAsync to the message thread
> ```
>
> - **Harness role.** ARCHITECT: **"Shell output, like cat"**. The whole harness
>   is written once; its lines pass the live rows and retire to the history.
> - **Rows.** ARCHITECT: **"Rows unbounded, rows count"**. A grid row is one
>   logical line of any length; `winsize.rows` bounds only the row count.
>   `GraphicsContext` clips vertically only.
> - **Cold and hot.** The AST, the materialiser, the rings and the grid are cold
>   resources, owned by the chain and sized at prepare; hot state holds
>   pointers (`TETRIS.md:230-285`).
> - **Style.** The materialiser uses the plugin_bootstrap Style: the
>   processor-owned `PluginEditorLayout` (`fonts`, `getOrCreateSettings`) and
>   the process-wide `StyleManager` (`jam_StyleManager.h:22`), the lane of cast
>   `printHelp` (`Help.cpp:10-35`).
> - **No lock.** Each FIFO has one writer and one reader.
>
> **The PTY era.** The TTY reader thread fires `Id::data` per chunk
> (`jam_TerminalTTY.cpp:28-63`) and is the one writer of the input
> `TerminalFIFO`; `processBlock` is its one reader. `processBlock` decodes the
> bytes in order into the live rows with the non-allocating half of
> `AnsiDocument` (tokenize and command dispatch into prepared storage); the tree
> build stays on the history lane, on the message side. The live grid then has
> one writer, the decoder, in place of the harness paint. The capacity bound
> becomes the max window size (`RFC:47`).
>
> **Superseded (PLAN-tty.md, R1 and R2).** The TTY has no thread. ARCHITECT:
> *"why cant we use AUDIO thread as the READER thread?"*. `processBlock` reads the
> PTY with a non-blocking poll, writes the queued input (`TerminalFIFO::pull
> (TerminalTTY&)`), and decodes in `jam::TerminalProcessor::process`. The
> decoder writes the engine rows through the `AttributedString` API. The View writes
> the input lane through `chainEvents[Id::terminalInput]`. Every "reader thread" and
> "CellFifo" row of this RFC describes the superseded design.

### 3.7 Resize — the property, proven by construction

- History: logical lines, width-independent → re-wrapped at paint for any width,
  losslessly, round-trip narrow↔widen exact.
- Live primary screen: bounded grid rewrapped from logical knowledge at retirement
  boundaries; the app-owned alternate screen redraws itself per the VT contract.
- SIGWINCH path: TTY `TIOCSWINSZ` → processor resizes bounded grid → widget
  re-derives layout. The widget never owned content, so it can destroy none.

> **[AMENDED 2026-10-20 — resize is setWinsize, the setFactor isomorph]**
>
> - **Init.** `prepare (spec)` sizes the cold resources once, from
>   `prepareToPlay` inside `suspendProcessing` (`FilterStripProcessor.cpp:103-116`).
> - **Run time.** ARCHITECT: *"winsize could be change RUNTIME exactly like
>   setFactor, setFactor IS setWinsize, it's resizing buffer for processing"*.
>   EVEView writes the packed winsize to the `WINSIZE` instance node; the
>   callback tells `EVEProcessor::setWinsize()`, which tells the chain a
>   pending packed atomic (`TerminalWinsize::pack`, one atomic store,
>   `jam_TerminalWinsize.h:9-11`) — the `pendingOversamplingFactor` shape. The
>   chain applies it at block start, inside the running `processBlock`, within
>   the prepared capacity.
> - **History.** Never touched, never sent again (ruling 25: *"not even SIGWINCH
>   should touch it"*). Rows that leave the live count retire to the history as
>   appends.
> - **Widget.** The `TextEditor` reflows its documents for its width through
>   `ReflowDocument::setRows` (`jam_TextEditor.cpp:60-70`, `:181-196`).
> - **Two scrolls.** ARCHITECT: *"our widget scroll is actual scroll, a Viewport
>   scroll, never conflate them"*. Terminal scroll is SU / SD inside the DECSTBM
>   region (`generated/jam_Terminal.h:975-976`, `:993`): a content operation.
>   Widget scroll is the `juce::Viewport` of `TextEditor`: rendering only.
> - **Why EVE differs.** Four of five surveyed terminals store the wrapped grid
>   as the truth, and all five couple state to rendering (§1). EVE decouples the
>   live buffer and the history from the layout entirely.

---

### 3.8 The harness is the original file [ADDED 2026-10-20]

ARCHITECT: *"the correct mental model is to think the harness is analogous to
vim swap file. it's the SSOT complete AST, on disk"*. In vim terms the harness
is the original file: an unchanged data block *"can have a negative block
number… the lines in this data block can be read from the original file"*
(neovim `memline.c:26-32`). Only what the terminal writes goes to a backing
store.

- **Read.** ARCHITECT: **"mmap readOnly"**. `juce::MemoryMappedFile` maps the
  file at its fixed size, the class contract (`juce_MemoryMappedFile.h:57-63`);
  the parse reads the mapped bytes in place. Today the harness is a disk file
  (`EVEView.cpp:65`) copied by `loadFileAsString` (`jam_TextEditor.cpp:41`).
- **Who loads.** ARCHITECT: *"the loader could be at Processor? View just tell
  the reference it's holding? isnt this just like how juce convolution loads
  IR?"* — `Convolution::loadImpulseResponse (const File&)` posts the reference
  and the engine is built off the caller (`juce_Convolution.cpp:888-898`,
  `:837-846`). Thread: **"In the tell, message thread"** (mmap, parse 4 ms,
  addLines 5 ms; no loader thread, so nothing idles —
  `BackgroundMessageQueue` sleeps 10 ms when empty, `juce_Convolution.cpp:121-122`).
- **Lock.** JUCE's Convolution takes locks around its queue (`popMutex`
  `:98`, `SpinLock` `:683`, `std::mutex` `:751`). EVE keeps the topology and
  crosses only through `AbstractFifo`.

### 3.9 Storage tier and state [ADDED 2026-10-20]

ARCHITECT: *"mmap now? swap later for session restoration? most of terminal
state machine i think can be stored exactly as stateInfo blob, just like juce
dump EVE.settings for standalone, hence as a plugin it is the transport
getState/setState info with END as host"*.

- **State machine → state blob.** `getStateInformation` writes
  `model.copyState()` (`EVEProcessor.cpp:144-157`); the Standalone saves it as
  `filterState` in `EVE.settings` when the window closes
  (`jam_AudioStandaloneApp.cpp:70-74`, `:151`); a host saves it in its session.
  Scalar and mode state only (§8).
- **mmap** fits the original file: read-only, fixed size. It cannot back a
  growing history: *"If the file is resized after being opened, the behaviour is
  undefined"* (`juce_MemoryMappedFile.h:57-58`), and it has no flush — a
  `readWrite` change reaches disk *"at the whim of the OS"* (`:51-52`,
  `:101-109`).
- **Swap (the vim model)** fits the running buffer, later, for session
  restoration: block tree over the line text, changed blocks written
  (`memline.c:5-24`, `memfile.c:1-22`). The only framework path that guarantees
  bytes on disk is `FileOutputStream::flush()` → `fsync` / `FlushFileBuffers`
  (`juce_FileOutputStream.cpp:82-86`, `juce_SharedCode_posix.h:541-545`,
  `juce_Files_windows.cpp:468-473`).
- **Today.** `Document::Index` holds the B-tree and the row geometry only
  (`jam_Document.h:482-574`); the residency tier of §3.3 is not in the code
  (no spill, rehydrate or eviction in `jam_core/document`).

> **[AMENDED 2026-10-24 — texteditor-nvim-parity sprint]** ARCHITECT: *"swap,
> this sprint."*, *"this will be also used as the terminal cache"*,
> *"content SSOT should not owned by TextEditor … the actual content should live
> in Processor"*, *"what scollback budget? not scrollback capacity?"*.
>
> **The swap.** The swap is `Document::Swap`, its own object (MANIFESTO E
> rule 1: one responsibility). `jam::TerminalProcessor` owns it beside the
> history and live documents (MANIFESTO B; TETRIS cold state).
> - It takes an injected `Codec` (`encode`, `decode`, `clear`) and one RAII
>   `juce::TemporaryFile`.
> - It keeps `HISTORY_CAPACITY` hot lines, from the `## settings` row. Above
>   that, it spills the leading hot lines of the document chain down to 7/8.
> - A line is encoded once, and its span is reused at the next eviction.
> - `rehydrate (line)` reads the span back.
>
> **The Codec.**
> - The encoder is the LLGC writer `TerminalGraphicsEngine::appendLine`.
>   ARCHITECT: *"materialisation from id to actual bytes is LLGC
>   responsibility"*.
> - A true-colour id is `terminalColourId` plus the RGB value, so every
>   document gives the same id. The decoder therefore parses in its own
>   `AnsiDocument`, and the history document's parser state never moves.
> - Eviction replaces the cells of a line and its columns as whole objects.
>
> **The Index.** `Document::Index` is geometry only. The TextEditor owns it,
> with its glyph runs and decoded images, and pulls the content through the
> view getter. The editor state machine is the TextEditor's component node in
> the plugin state (SPEC.md, State Model).
>
> **[AMENDED 2026-10-24 — decouple]** ARCHITECT: *"decouple. Processor is live
> buffer, TextEditor will hold history at least that's how the terminal
> designed. so SIGWINCH never ever destroy history content."* This amendment
> replaces the ownership above:
> - `jam::TerminalProcessor` owns the live document only.
>   `pull (historyDocument)` appends the history bytes to the caller's document.
> - The TextEditor owns the history document, its `Document::Swap` and its
>   `Codec`. It loads files and applies edits and undo on its own document.
> - A resize replaces only the live document. The history survives SIGWINCH.
> - The getter gives the `HISTORY_CAPACITY` value to the TextEditor at each
>   pull, and the TextEditor sets the swap capacity from it.
>
> **[AMENDED 2026-10-25 — the record outlives the view]** ARCHITECT: *"GUI is
> ephemeral … all state machine, swap, history etc could always outlives GUI.
> gui only materialisation, on demand"*; *"TextEditor INDEED hold the history
> but already materialised on demand. that's exactly the design. but the
> document SSOT outlives the widget."*; *"follow JUCE pattern, API verbatim?
> within the adherence into our CONTRACT?"*. This amendment replaces the
> 2026-10-24 ownership above:
> - `jam::TerminalProcessor` owns the live document and the history record
>   `jam::SwapDocument` (`jam_terminal`). `Document::Swap` and its `Codec` are
>   deleted from `jam_core`.
> - Writer side, after JUCE `AudioFormatWriter::ThreadedWriter`: the history
>   FIFO carries the rows, `pull()` drains it on the message thread, and
>   `SwapDocument::write` appends each block to its file. The bytes are the
>   encoding, so nothing is re-encoded.
> - Reader side, after JUCE `BufferingAudioReader`: a block holds at least
>   `bytesPerBlock` bytes and ends at a line end. `getOrCreateBlock` parses one
>   block from the file. `setNextReadPosition` keeps at most `numBlocks` blocks
>   and the open block.
> - Receiver, after JUCE `AudioThumbnail`: the TextEditor lays out the blocks
>   that the view needs and keeps the line count and pixel height of every
>   block. A width change lays out every block again (SPEC Feature 2.1).
> - The history is read-only (R7). The TextEditor edit machinery and insert
>   mode are deleted. `HISTORY_CAPACITY` sizes the history FIFO.
>
> **[AMENDED 2026-10-25 — full throttle, O(visible)]** ARCHITECT: *"the events
> chain should not be manually managed, lock free, wait free, yield free,
> always full throttle"*; *"audio never drop anything, not even a single
> sample"*; *"can we make it always O(1)?"*. This amendment replaces the residency,
> pull and sizing text above:
> - Each `processBlock` reads the PTY until it is empty. The decoder never
>   waits for the view. It publishes the newest frame when the live lane is
>   empty, and the engine grid holds the live screen.
> - The clocks size the history FIFO at `prepare`: one block's output bound
>   times the blocks per vblank. `HISTORY_CAPACITY` is deleted. Input whose
>   output does not fit stays in the PTY (*"PTY holds the input"*).
> - The view pulls on `juce::VBlankAttachment`.
> - The view scrolls by lines from the tail (`scrollOffset`, `scrollRow`, with
>   the existing `jam::Scrollbar`), eased by rows. It lays out only the visible
>   lines, so a resize, a font change and the editor teardown cost the
>   visible lines, not the history.
> - Bundled fonts come before the OS in the fallback search. EVE bundles the
>   Nerd Font symbol face.
>
> **[AMENDED 2026-10-27 — the state machine is the Model tree in stateInfo]**
> The *"State machine → state blob"* item above is now exact: the terminal
> state machine is a `jam::Model` whose tree is the `terminal` child of the
> APVTS state. `getStateInformation` flushes the Model, then saves it with
> `model.copyState()`.
> - On `setStateInformation`, EVE suspends processing. The terminal Model takes
>   the restored values (`jam::Model::replaceState`), and its tree replaces the
>   restored child in the incoming state. Then the APVTS replaces its whole tree
>   (`juce_AudioProcessorValueTreeState.cpp:396-400`), so one tree remains.
>   `jam::PluginEditor` owns `onSetState`, so the restore uses no callback.
> - The record is `jam::TerminalSwap` (index and segment files, a two-slot
>   live file), not `SwapDocument`.
> - A row's atom count is its content end. ARCHITECT: *"no second copy of
>   truth allowed."*
> - Init, resize and history capacity prepare through `prepareToPlay`.
>   ARCHITECT: *"make it consistent then"*.

### 3.10 One raster code, two lanes [ADDED 2026-10-20]

ARCHITECT: *"juce rasterizer is SLOW. we have FAST LANE with Vulkan, with its
software fallback, why cant we use that as the SSOT?"*; *"Vulkan can stays with
its message thread contract, while Terminal could be used in any context"*.

- **Fast lane = raster SSOT [AMENDED 2026-10-20].** ARCHITECT: *"do not
  change Vulkan CONTRACT. it must stays as is. we only abstract its software
  rasterizer being used as SSOT on TerminalGraphics"*. The terminal lane draws
  its rasters through the software rasterizer of the fast lane,
  `LowLevelGraphicsGlyphRenderer` over the software renderer
  (`jam_VulkanEngine.h:38-42`). The Vulkan GPU path and its message-thread
  contract (`jam_VulkanImagePixelData.h:40`) stay as they are, and they serve the
  View only.
- **GlyphRasterizer.** The rasterising core leaves `GlyphAtlas`: backend
  configuration and key → coverage into a caller-owned buffer, through the
  already-shared `MonoCoverageWriter` (`jam_GlyphAtlas.h:144`,
  `jam_GlyphAtlas.cpp:617-625`, `jam_TerminalGraphicsContext.cpp:365-366`).
  Each lane keeps its own cache: the Vulkan atlas under its message-thread
  contract (its `cache` changes with no lock, `jam_GlyphAtlas.cpp:143-153`), the
  terminal engine as cold resources.
- **Allocation.** The paint is creation, so it can allocate (MANIFESTO E);
  `juce::EdgeTable` allocates per construction (`juce_EdgeTable.cpp:301-303`)
  and runs only in the paint. The audio thread rasterises nothing.
- **Raster configuration [AMENDED 2026-10-21].** ARCHITECT: *"rasterizer/embolden
  should be at LLGC only"*; *"the sole author should be Style since the SSOT is
  css-settings. so css carried by binary as default, write to disk as settings
  (exactly like other settings contract), settings always win, when a project did
  not "open" the parameter as settings, Style always return the default carried
  css"*; *"LookAndFeel communicate to GlyphAtlas. Window is responsible for
  rendering engine, PluginEditor should not ever know about Vulkan"*.
  1. `GlyphRasterizer` holds the backend, gamma, contrast, embolden and the
     coverage table. `GlyphAtlas::setRasterization` / `setEmbolden` tell it and
     flush the cache (`jam_GlyphAtlas.cpp:65-75`).
  2. `StyleManager::getFontsStyleValue` returns the settings sheet value
     (`Id::toTag` key) when the project opens it, else the CSS `@font-face`
     attribute (`jam_StyleManager.cpp:118-129`). JFS carries the CSS default only
     (`jreng-filter-strip/Source/layout/style.css:216-224`); EVE opens
     `RASTERIZER`, `GAMMA`, `CONTRAST`, `EMBOLDEN` in `ViewLayout.md`
     `## settings`, seeded to `~/.config/end/eve.md`.
  3. `StyleTheme` (the LookAndFeel) tells the atlas
     (`jam_StyleTheme.cpp:25-45`). `StyleManager` registers embedded faces in
     `jam::Typeface` and holds no atlas call (`jam_StyleManager.cpp:99-101`).
  4. The `VulkanEngine` constructor default stays (*"do not change Vulkan
     CONTRACT"*).
- **History capacity [ADDED 2026-10-21].** ARCHITECT: *"history capacity same
  Settings contract: ViewLayout.md to eve.md, 10k row"*. `HISTORY_CAPACITY` (int,
  10000 rows) sizes the history lane; the live lane is sized by the winsize alone
  (`EVETerminalProcessor.cpp:20-31`, `jam_TerminalProcessor.cpp:5-9`).

## 4. Ecosystem — EVE and END

### 4.1 Roles

- **END** (`~/Documents/Poems/dev/end/`) — the HOST. Named Nexus
  (`~/Documents/Poems/dev/nexus/`) at RFC time: the old END application
  role. Owns: window, tabs/panes mux, sessions composition, modals, config, fonts,
  daemon. Has its own overhaul track — jam-structure adherence, lua → markdown
  tables + CSS, CAST manifest conformance (nexus/RFC-config-migration.md exists;
  PLAN-END-plugin-host.md exists at nexus root). **Out of EVE's scope.**
- **EVE** (`~/Documents/Poems/dev/eve/`, this project) — the TERMINAL unit, an
  audio plugin hosted at END. Feature-modular sibling: Whelmed (markdown/mermaid) follows the same
  model, built from the exact same jam modules.

### 4.2 Mux boundary — ruled, cited

**EVE is a single-surface terminal plugin: one PTY, one buffer, one widget per
instance. END composes instances.** Cited from nexus source: the state tree
FLEX → TAB → PANE/EDGE rows where each EDGE's head/tail names a SPACE (a PANE or
another EDGE) — the working binary-split structure (nexus/ARCHITECTURE.md:263-276);
`ID::focusedPane` focus routing (:74); per-pane APVTS bridge "one per pane, paired
under the PANE" (:85-86). One plugin instance = one voice; the host is the mixer.

### 4.3 Hosting contract (final goal, design-ahead only)

EVE is developed and validated **standalone first** (dodges the two known blockers:
jam clap modules half-assed; host Model complexity). Final goal: hosted into END
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

> **[EXECUTED 2026-09-20]** The video lane (`jam_terminal/video/**`,
> `transport/jam_TerminalCellFifo.h`, `jam_graphics/text/jam_Row.h`,
> `jam_TerminalLine.h`) and `jam_terminal/model/jam_TerminalModel.*` are
> deleted. The terminal seam is rebuilt later, against Document and
> TextEditor.
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
terminal**. Subsystem completeness ≠ architecture proven. The assembly is EVE.

> **[AMENDED 2026-10-20 — inventory, current state]**
>
> - **Gone:** `TerminalParser`, `TerminalVideo`, `TerminalCellFifo`,
>   `TerminalModel`, `terminal::TextLayout` (§5.1 executed; `jam_terminal.h`
>   includes none of them).
> - **Parser:** `AnsiDocument` (§3.4 amendment).
> - **Chain parts (jam_terminal/processor):** `TerminalFIFO` (a
>   `juce::OutputStream` over `juce::AbstractFifo`), `AnsiBuffer` (the chain
>   byte block, sized at prepare), `TerminalProcessor` (the chain member that
>   owns an output `TerminalFIFO`).
> - **Writer:** `terminal::GraphicsEngine` on the TETRIS verbs `prepare` /
>   `process` / `reset`, with its draw scratch sized at `prepare`;
>   `terminal::GraphicsContext` is the juce LLGC that materialises cells.
> - **Project chain:** `EVETerminalProcessor` (ruling: renamed from
>   `EVEAudioProcessor`, the JFS chain isomorph).
> - **Shared raster core:** `GlyphRasterizer` (§3.10).
> - **Widget:** `jam::TextEditor` (`juce::Viewport`) pulls through
>   `bindToProcessor` and a timer; it reflows over stored advances with
>   `ReflowDocument::setRows`.
> - **TTY:** `TerminalTTY` posix and windows — the PTY-era producer of the
>   input lane.

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

   > **[AMENDED 2026-09-15 — texteditor-viewport/config sprint]** Gap 2 is executed.
   > `jam::TextEditor` is a `juce::Viewport` subclass. It owns its `AnsiDocument`,
   > its `Document::Index` (wire codec included), and a nested `Content` painter.
   > It is event-driven: `visibleAreaChanged` windows the index
   > (`Index::setViewport (firstRow, lastRow)`) and sizes the content. The owner
   > only sets child bounds. Font and colours resolve through the LookAndFeel
   > chain (`StyleTheme::getMonoFont`, style-table colour ids).
   >
   > EVE rides the stock plugin_bootstrap chain with md overloads — no new
   > patterns. `eve.md` is EVE's sole config+style document: carried as binary,
   > seeded to `~/.config/end/eve.md` when missing, validated by
   > `ConfigValidator`, hot-reloaded by the chain's `File::Watcher`, with
   > binary-defaults fallback plus a `jam::MessageOverlay` validator-error
   > surface when the disk copy is invalid. Its tables: `settings`
   > (UI_SCALE, APPEARANCE → persisted-parameter subtree via the md
   > `populateTree`), `UI_size` (SSOT — ViewLayout.md's table deleted),
   > `colours` palette, `window` chrome, `style` (light/dark columns), `fonts`
   > (mono record). `StyleManager` gained the `juce::ValueTree` lane with the
   > `registerStyle (config, darkConfig)` reload API;
   > `ConfigDocument::getValueTree` gained column selection (the `dark` tree).
   > Sprint closed: 47 audit findings resolved or dispositioned, doxygen pass
   > complete, superseded PLANs deleted.

   > **[NEXT SPRINT — ruled 2026-09-15]** Build `jam::TextEditor` as a
   > full-feature, vim-based text editor FIRST. The terminal wires into it
   > afterwards, the way nvim hosts a terminal inside itself (reference:
   > `~/Documents/Poems/dev/neovim/`). This inverts endless
   > (`~/Documents/Poems/dev/endless/` SPEC.md:44-45, :98-152 — its modal
   > visual/visual-line/visual-block selection and prefix-modal action registry
   > are the feature SPEC lineage): not pseudo-vim bolted onto a terminal — a
   > vim-based TextEditor, an actual FAST text-rendering `juce::Component`
   > widget with an actual Viewport, scrollbars, and vim basic capability:
   >
   > - text editing
   > - modal insert/normal/visual
   > - configurable cursor shape — any codepoint is a valid cursor, emoji
   >   included
   > - terminal usage defaults: always insert mode, status bar hidden; Visual
   >   mode enterable as a modal to copy any text in the buffer
   >
   > The ARCHITECTURE is designed correctly this time: a vim-based TextEditor;
   > the terminal is its client.

   > **[AMENDED 2026-09-20 — texteditor-foundation sprint]** Gap 2's mandate
   > is executed as the foundation. `jam::TextEditor` moved to `jam_gui`; the
   > Document and the codec are injected, and EVE owns the `AnsiDocument`.
   > The widget paints through `GlyphArrangement` (`clear`/`shape`/`arrange`/
   > `draw`), with cell metrics read from the LookAndFeel
   > (`StyleCustom::getMonoMetrics`). `Document::Index` is the sole wrap
   > owner.
   >
   > The editor is the plugin's main view — a sibling of `ViewEditor`. The
   > status bar is the bottom `ViewPanel` row, wired through the Registry.
   > State lives on the `MODE` parameter node (SPEC.md, State Model).
   >
   > The vim foundation covers: modes; counts; motions (`h j k l w b e 0 $
   > gg G ctrl+u ctrl+d`); operators `d c y x p P`; `dd`, `cc`, `yy`; `o O i
   > a I A`; visual character, line, and block selection with `d c y`; undo
   > and redo as line-granular `juce::UndoableAction` transactions. Keys
   > come from `eve.md` `## keys`.
   >
   > Open items, not yet ruled: cursor shape as an arbitrary codepoint, and
   > the terminal-usage defaults (always insert mode, hidden status bar).

   > **[AMENDED 2026-10-24 — texteditor-nvim-parity sprint]** ARCHITECT:
   > *"IDENTICAL UX PARITY with neovim, but not ALL THE COMPLETE FEATURES. only
   > core editing and navigation"*. The vim core is in `jam::TextEditor`.
   >
   > **State.** It lives on the TextEditor's component node, not on a `MODE`
   > parameter (SPEC.md, State Model).
   >
   > **Status bar.** `jam::TextEditorStatusBar` is a child of the editor. It
   > binds the same node, has its own colour ids, and shows outside insert mode.
   >
   > **Keys.** The bindings come from the `eve.md` sheets `## normal`,
   > `## insert` and `## visual`. Dispatch is through the tables `getMotions()`
   > and `getActions()`.
   >
   > **Edits.**
   > - An edit is a creation: the new line is parsed from LLGC wire bytes into
   >   the Document (*"Document always parse"*).
   > - The undo actions `InsertLine` and `RemoveLine` link and unlink line
   >   elements, and a replace is one transaction of both.
   > - Every property of the editor state machine is on the node. The editor
   >   holds no member copy of mode, cursor, anchor, count or pending operator.
   >
   > **Harness.** The processor loads the harness as an editable file buffer,
   > with `jam::TerminalProcessor::load` (the Convolution model of §3.8). The
   > old `EVETerminalProcessor::load` and its print helpers are deleted. The
   > TextEditor owns no content: it pulls the processor's documents through the
   > view getter, and tells edits and rehydrates the same way.
   >
   > **Namespace.** The `jam::terminal` namespace is deleted. ARCHITECT: *"NO
   > FUCKING terminal namespace. DELETE. write functions at its correct
   > domain"*.
   > - The classes are `TerminalGraphicsContext`, `TerminalGraphicsEngine` and
   >   `TerminalTrueColours`.
   > - The byte writers are statics of the engine, and `TerminalIndexedColours`
   >   holds the indexed-colour statics.
   > - `jam::Format` holds `appendDecimal` and `appendUtf8`.
3. **Terminal LLGC completion** — geometry parity only (text goes through jam's
   pipeline, §3.6). Six stubs at jam_TerminalGraphicsContext.h: `fillPath`/
   `clipToPath` (:344, :332), `drawImage`/`clipToImageAlpha` (:345, :333),
   transparency layers (:341-342), plus trivial `getPreferredImageTypeForTemporaryImages`/
   `getFrameId` (:360, :366). Reference: the Vulkan twin's structure; oracle: juce
   software renderer. Image degradation ladder: Kitty/`END;` protocol where the
   receiver supports it (EVE itself always does — both ends of the wire are ours),
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
2. **EVE standalone** — the terminal itself. Validates: full processor side against
   endless's known-good behavior, the buffer under real PTY load, **the resize
   property** (the headline test: narrow→widen round-trip, byte-exact history),
   SKiT, Vulkan cell adapter.
3. **Whelmed standalone** — same buffer/widget on the pixel backend with editing and
   markdown/CSS config; proves renderer-agnosticism (one component, two backends)
   and pilots the lua→markdown config pattern for END.
4. **CLAP / END hosting** — last, after the foundation is proven; unblocks on the
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
   storage tier (memory cap, disk spill, daemon persistence) is an END-transport
   era decision.

   > **[AMENDED 2026-09-15]** Deferral withdrawn by ARCHITECT. Tier designed and
   > measured: cold lines spill as ANSI wire bytes (byte-fixpoint proven,
   > 89 B/line); hot-window budget defaults to 1/16 machine RAM
   > (config-overridable via `terminal / scrollbackBudgetMb`); eviction by
   > watermark (high = budget, low = 7/8 budget); rehydrate ~100 µs per 50-line
   > page. Daemon persistence remains END-era.

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

> **[AMENDED 2026-10-20 — added non-negotiables, ARCHITECT verbatim]**
>
> - *"ALWAYS LOCK FREE"* — no lock, no try-lock; every crossing is a
>   `juce::AbstractFifo` with one writer and one reader.
> - *"writing bytes is processBlock"* — the byte write is a chain verb on the
>   audio thread, never in a paint scope or a destructor.
> - *"NO POKING LAYERS. MUST BE ENCAPSULTED"* — the View tells and pulls only;
>   the chain owns its engine, rings and grid.
> - *"wrapping and reflow is RENDERING concern"* — no wrap in the writer; the
>   widget reflows.
> - *"Paint is NOT MATERIALISATION"* — paint calls only what is already
>   materialised.
> - *"CONTRACT NON-NEGOTIABLE… 0 copy 0 mutations 0 temp containers 0 shadow
>   state 0 handroll method 0 hack… MUST USE FRAMEWORK API to its fullest
>   extent… always UNIDIRECTIONAL"*.
> - The reader-thread rule above now binds `processBlock`, the clock that
>   drains the bytes.

> **[AMENDED 2026-10-27 — non-negotiables, ARCHITECT verbatim]**
>
> - *"no second copy of truth allowed."*
> - *"parameters.md always for automatable parameter, a parameter can host can
>   automate."* Terminal state is never a host parameter.
> - *"using setValueNotifyingHost is smell"* — the clock writes the Model lane
>   with lock-free stores only.
> - *"jam::Model … Model lane for any event driven state machine that is non
>   automatable, real time safe on its own thread, guaranteed thread safe on
>   other thread, always concurrent with atomic, think of it like reverse
>   direction of APVTS."* Each parameter has one writer thread.
> - *"all terminal events including OSC 133 and future shell integration is
>   jam::Model parameter, still attached to APVTS"* — a new terminal state is a
>   new `model.md` row, never a new channel.
> - *"make it consistent then"* — one prepare path, `prepareToPlay`.
> - One serial context for PTY readiness and the clock; zero-sample ticks;
>   `spec.maximumBlockSize == tty.readChunkSize`.
> - *"my rulings must be documented comprehensively, so no more violation how
>   to design, extend, and scale the ARCHITECTURE and topology"*.

> **[AMENDED 2026-10-29 — the clock is the chain's own, ARCHITECT verbatim]**
>
> - *"EVETerminalProcessor should own the VirtualDevice, hence prepareToPlay
>   upstream still juce contract, we only take over the control at seam we need
>   to drive ourselves"*. `EVETerminalProcessor` owns `jam::VirtualDevice` and is
>   its `juce::AudioIODeviceCallback`; the host's `processBlock` carries audio
>   only.
> - *"dont conflate VirtualDevice == actual analog drivers. it's not.
>   VirtualDevice exists as our clock, since the architecture must use a
>   device."*
> - *"it SHOULD NOT MATTER. what binary we're using as host-client. VirtualDevice
>   as full throttle clock is EVE SPEC contract"*. One wiring in every host.
> - *"writing bytes is processBlock"* and *"the clock that drains the bytes"*
>   above now name the clock's tick, `audioDeviceIOCallbackWithContext` of
>   `EVETerminalProcessor`.
> - *"why vblank must be branched? vblank at all build contract are exactly the
>   same"* — the vblank getter posts the history writer to the processor's
>   serial context in every host.

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
