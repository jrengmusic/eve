# PLAN: AnsiDocument — the sixth Document domain (core + fixpoint)

**RFC:** RFC-EVE-terminal-grand-scheme.md (§3.3-§3.5, §5.3, §7.1-§7.2 pulled forward by ARCHITECT this session)
**Date:** 2026-09-13
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE / JAM — LANGUAGE.md C++ rules apply

## Context

The session ruled: the terminal is bytes; `jam::Document` is the proven byte
processor (XML, HTML, CSS, Markdown, Mermaid — RFC §3.3). `AnsiDocument` becomes
the sixth domain: parser + AST + validator for ANSI-attributed text. It is the
binding seam for all terminal rendering — both LLGCs materialise from its AST
(`drawCells` on the terminal context, `setCellRun`/`drawGlyphs` on Vulkan, later
sprints). Clean-room: terminal tables conform to the Document contract, never
the reverse. Sprint 1 scope (ARCHITECT-ruled): **core + fixpoint harness only.**
Consumers (CAST `--help`, Vulkan viewer, live PTY lane) are later sprints.

## Overview

Build `AnsiDocument` + `AnsiValidator` + the `Cells` payload row in
`Document::Value`, execute the in-scope deaths, and prove the lane with a
headless byte-fixpoint harness: fixture bytes → parse → validate → derive cells
→ `GraphicsEngine` frame → re-parse → byte-identical.

## Language / Framework Constraints

- LANGUAGE.md C++/JUCE: single header with inline implementation preferred
  around ~300 LOC; split to `.cpp` only for real dependency cost. Sibling
  precedent: `XmlDocument` is header-implemented (jam_XML.h), `MarkdownDocument`
  splits (jam_MarkdownDocument.h/.cpp). AnsiDocument follows whichever the
  implementation size dictates at the 300-LOC smell line; non-template bodies
  beyond trivial go to `.cpp` per CODING.md placement table.
- Submodule include rule: zero includes in submodule headers; all includes at
  `jam_terminal.h` (CODING.md Header Discipline; jam_terminal.h:36-37).
- 30-line functions, 3-branch limit, `Function::Map` dispatch over branch
  chains, `.at()` access, alternative tokens, no bail-outs — CODING.md CRITICAL
  RULES in force for every step.

## Dependency & API Inventory (Pathfinder + COUNSELOR-verified)

**Document engine (jam_core/document/jam_Document.h):**
- Three virtual hooks a domain implements: `getToken (Cursor&, int) = 0` (:723),
  `build() = 0` (:726), `getVocabulary() const = 0` (:729); entry
  `parse (const char*, int)` (:715).
- `Vocabulary { const jam::LookupTable<int,int,256>& classes; commentOpen;
  commentClose }` (:375-388) — byte-class table is CAST-generated
  (generated/jam_LookupTables.h lane).
- `Cursor` byte walker (:396-418); `Element::add<ValueType> (key, args...)`
  (:362-368); `Owner<Element> elements` arena (:731).
- `Value` closed variant (:163-176); exactly two exhaustive visitors, both in
  `setSpanOffsets` (jam_Document.cpp:443, :485), guarded by
  `static_assert (sizeof (ValueType) == 0, …)` — the compiler enumerates every
  site the new row touches.
- `Document::Validator { Rules = Function::Map<juce::String, juce::Result>;
  getRules() = 0; isValid (const Document&) }` (:428-439).

**Domain siblings (the clone sources):**
- `XmlDocument : Document` (jam_XML.h:11): `getOrCreate` (:25-42), static
  `parse (text)` (:50-55), `getToken` segment dispatch (:217-253), `build()`
  via `static const auto treeConstruction { Function::Map keyed on
  map::XmlTokenType }` (:261-294). **This is the structural template.**
- `MarkdownDocument` (jam_MarkdownDocument.h:15): parse-with-provenance (:53),
  value-view reads `getValueView` (span, no copy).
- `XmlValidator : Document::Validator` (jam_XmlValidator.h:36-85) — the
  validator clone source.

**Atoms and registries (reused as-is, never rebuilt):**
- `AttributedChar` atom + static Unicode machinery: `fromCodepoint (cp,
  styleId, lineDrawing)` (jam_AttributedChar.h:260), `width` (:272),
  `graphemeSegmentationStep`/`Init` (:311-325), `widthClass` spacerTail
  encoding (:29-32).
- `Stamp` interning: `addIfNotAlreadyThere (Entry{fg,bg,underline,flags}) →
  styleId` (jam_Stamp.h:60-68); SGR flag vocabulary with wire numbers
  (:134-179) — **this table is the conformance oracle.**
- `Grapheme` interning (jam_Grapheme.h:16-27); `jam::Hyperlink` (OSC 8,
  jam_AttributedChar.h:59-63).
- Generated protocol vocabulary: `map::SGR`, `map::CSI`, `map::ColorMode`,
  `map::OSC` (generated/jam_Generated.h:93-96), `Chars::csiIntroducer`,
  `Chars::semicolon` (generated/jam_Chars.h).
- Writer (fixpoint back end, already built): `terminal::GraphicsContext::
  drawCells` (jam_TerminalGraphicsContext.h:213), `GraphicsEngine::resize/
  beginFrame/endFrame → FILE*` (jam_TerminalGraphicsEngine.h:164-235).

**CAST lanes (established procedure):** lexicon rows in jam `cast/*.md` →
regenerate `jam/generated` → double-run fixpoint by cksum (SPRINT-LOG:85
precedent — XmlTokenType rows added exactly this way).

## Names (NAMES.md Rule -1 — ratified at plan approval)

| Name | Family precedent |
|---|---|
| `AnsiDocument`, `jam_terminal/document/jam_AnsiDocument.h` | `XmlDocument`, `MarkdownDocument`; casing per `XmlDocument` (session-ruled) |
| `AnsiValidator`, `jam_terminal/document/jam_AnsiValidator.h` | `XmlValidator` |
| `using Cells = jam::Array<AttributedChar>` | alias family `Tokens/Elements/Identifiers/Numbers` (jam_Document.h:157-160) |
| `Id::cells` (line-element property key) | lexicon lane; reuse `Id::line` if present in generated lexicon, else add |
| `map::AnsiTokenType` rows: `text`, `csi`, `osc`, `control` | `map::XmlTokenType` (cast/bimaps.md, SPRINT-LOG:85) |
| `treeConstruction` dispatch table (verbatim reuse) | XmlDocument's table, NAMES Rule 8 example |
| Harness target `ansi_fixpoint` | no test precedent in jam — new lane, flagged in Risks |

No other new names. Engineer introduces zero identifiers beyond this table.

## Validation Gate

Each step validated by COUNSELOR before the next — against MANIFESTO.md
(BLESSED), NAMES.md, CODING.md, and the locked decisions above. @Auditor runs
ONCE, after the final step. Every Engineer prompt carries: Design by Contract
per CODING.md CRITICAL RULES; the MANIFESTO **E** MVP data-flow contract
verbatim; doxygen-first on C++ reads; no comments/doxygen authorship
(Code Hygiene — dedicated pass post-audit).

## Steps

### Step 1: Delete-first — the in-scope deaths
**Scope:** jam repo — `jam_terminal/parser/jam_TerminalParser.h`,
`jam_TerminalDispatchTable.h`, `jam_TerminalTransition.h`;
`jam_markdown/widget/jam_MarkdownComponent.h`; the `getTerminalContext`
dynamic_cast seam in `jam_terminal/widgets/jam_TextEditor.h` (:31-34 region);
the corresponding include lines in `jam_terminal.h` / `jam_markdown.h`.
**Action:** Remove the files and include lines. Strip the cast member/method
from TextEditor. Do NOT touch `jam_TerminalCSI.h` (TerminalVideo parameter
type, jam_terminal.h:78-80 — TerminalVideo is out of scope). Compiler output
is the ground truth for remaining references; fix fallout only by deleting the
dead reference, never by re-adding capability.
**Validation:** jam module compiles (via eve build); zero references to the
deleted symbols repo-wide (grep); TerminalVideo/TTY/CellFifo/TerminalModel
untouched.

### Step 2: Engine — the Cells payload row
**Scope:** `jam_core/document/jam_Document.h`, `jam_core/document/jam_Document.cpp`.
**Action:** Add `using Cells = jam::Array<AttributedChar>;` to the alias family
(jam_Document.h:157-160) and `Cells` to the `Value` variant (:163-176). Build;
the two `static_assert` visitor sites (jam_Document.cpp:443, :485) fail; add
one empty `if constexpr` case each (a cell run carries no source-span offsets).
Note: `AttributedChar` lives in jam_core/text — same module, no new include
edges.
**Validation:** compiles; exactly two visitor edits; no other consumer changed
(get/get_if templates are alternative-agnostic, jam_Document.h:234-268).

### Step 3: CAST lexicon rows + regeneration
**Scope:** jam `cast/*.md` (identifiers, bimaps, lookup-tables lanes),
`jam/generated/*` (regenerated).
**Action:** Add `Id::cells` (if absent; reuse `Id::line` if present),
`map::AnsiTokenType { text, csi, osc, control }` per the XmlTokenType
precedent, and the ANSI byte-class table row for the domain's
`Vocabulary::classes` (LookupTable<int,int,256>: classify ESC 0x1b, C0
controls, printable, UTF-8 lead/continuation). Run cast; confirm double-run
fixpoint by cksum (SPRINT-LOG:85 procedure).
**Validation:** generated headers fixpoint-stable; new rows follow existing
table formats exactly; no hand-edits to generated files.

### Step 4: AnsiDocument — the domain
**Scope:** new `jam_terminal/document/jam_AnsiDocument.h` (+ `.cpp` if the
placement table demands), include line in `jam_terminal.h` (dependency order:
after graphics — it needs nothing from parser/video; before nothing that needs
it this sprint).
**Action:** `struct AnsiDocument : Document`, cloning the XmlDocument shape:
- `getVocabulary()` returns the generated ANSI byte-class Vocabulary (step 3).
- `getToken (Cursor&, int)`: segments the content species only — `text` print
  runs, `csi` sequences (params accumulated to final byte), `osc` sequences
  (to BEL/ST; OSC 8 payload), `control` (LF and remaining C0 singles).
  Unrecognized finals tokenize and are consumed — never an error (wire bytes
  are data).
- `build()`: `static const auto treeConstruction { Function::Map keyed on
  map::AnsiTokenType }` (XmlDocument :261-294 shape). Running pen state
  (transient creation state): `csi` SGR params → `Stamp::addIfNotAlreadyThere`
  → current styleId, per the Stamp wire vocabulary (jam_Stamp.h:134-179 —
  bold 1, dim 2, italic 3, blink 5, inverse 7, strike 9, underline 4:n,
  38;2/48;2 truecolor, 53, 73, 74); `osc` 8 → `jam::Hyperlink` intern →
  current hyperlinkId; `text` → cells via `AttributedChar::fromCodepoint` +
  `graphemeSegmentationStep` + wide/spacerTail encoding; `control` LF closes
  the current line element. Each line: `addChild (root, line id)` +
  `element->add<Cells> (Id::cells, …)` — complete at creation, appended once,
  never revisited.
- Static `parse (const juce::String&)` and `getOrCreate` per XmlDocument
  (:25-55). No SGR reverse-serialization in the domain — the writer
  (`GraphicsEngine`) already owns encode-out; decode-in lives here. Two edges,
  one owner each (RFC §3.5).
**Validation:** COUNSELOR reads the file against this step verbatim; CRITICAL
RULES sweep; every SGR row of jam_Stamp.h:134-179 has a consuming path;
no `TerminalCSI`/`TerminalVideo` dependency; zero includes in the submodule
header.

### Step 4b: Role-indexed palette colours (ARCHITECT ruling, mid-sprint)
**Ruling:** basic-16 and 256-palette colours enter the AST as **role-indexed
cells** — the palette index is the truth; RGB resolves at materialisation
("colours at materialization", RFC §8). The resolver already exists as
generated data: the 256-entry xterm palette table (jam_LookupTables.h:29).
**Scope:** `jam_core/text/jam_Stamp.h` (additive Entry extension),
`jam_terminal/document/jam_AnsiDocument.cpp/.h` (decode),
`jam_terminal/graphics/jam_TerminalGraphicsEngine.h` (palette emission),
jam cast identifiers lane (`unterminated` row).
**Action:**
- `Stamp::Entry` gains additive fields `uint8_t fgMode/bgMode/underlineMode
  { 0 }` (0 = direct colour-field semantics unchanged;
  `map::ColorMode::palette256` = index field live) and `uint8_t
  fgIndex/bgIndex/underlineIndex { 0 }`; equality and hash extended over the
  new bytes. Existing fields untouched — TerminalVideo keeps compiling.
- AnsiDocument decode: SGR 30-37/90-97 → fg palette role (index n / 8+n),
  40-47/100-107 → bg palette role, via the generated range members
  (map::SGR::foregroundFirst/Last etc., jam_Terminal.h:282-289);
  38;5;n / 48;5;n / 58;5;n → palette mode + index; extended-colour parameter
  consumption always advances the cursor correctly for both the 2- and
  5-forms (fixes the 38;5;n → blink misparse).
- `GraphicsEngine::getSgrForCell`: entries with palette mode emit
  `38;5;index` / `48;5;index` / `58;5;index`; direct mode keeps the existing
  truecolor path.
**Validation:** every palette wire form round-trips writer↔parser in the
Step 6 fixpoint; no palette RGB resolution anywhere at parse or encode.

### Step 5: AnsiValidator
**Scope:** new `jam_terminal/document/jam_AnsiValidator.h`, include line in
`jam_terminal.h`.
**Action:** `struct AnsiValidator : Document::Validator`, cloning
XmlValidator's `static const auto rules` shape (jam_XmlValidator.h:36-85).
Rules: (1) root non-null with at least the structure parse produced;
(2) every line element carries a `Cells` property of the declared type;
(3) unterminated CSI/OSC at end-of-input is reported (the parser marks it on
the element; the validator surfaces it). No registration into
`getDocumentValidators` this sprint — the harness calls
`validator.isValid (document)` directly (YAGNI; bootstrap wiring is a consumer
concern).
**Validation:** rules run on fixtures; malformed fixtures produce named
`juce::Result` failures, never asserts; parser contains zero input asserts.

### Step 6: Fixpoint harness
**Scope:** new headless console target + committed fixtures (placement:
ARCHITECT-ruled mid-sprint: `eve/tests/ansi_fixpoint/` — relocated from the
jam repo).
**Action:** One `main`: for each fixture file — (a) `AnsiDocument::parse`,
(b) `AnsiValidator::isValid` — failure prints the Result and exits non-zero,
(c) derive: for each line element, `GraphicsContext::drawCells` into a
`GraphicsEngine` sized cols×rows from the fixture (no wrapping — fixture
lines fit the grid), (d) `endFrame` to a memory `FILE*`
(`open_memstream`/`tmpfile` per platform), (e) re-parse the emitted frame,
re-derive, re-emit, (f) byte-compare emissions of pass 1 and pass 2 —
MANIFESTO **D**: bit-identical or fail with a byte-offset diff.
Fixtures: one per SGR row of the Stamp table (generated by the harness from
the table itself — the table is the SSOT, fixtures derive from it), plus
committed real captures (`ls --color=always`, git diff — captured once, with
`--color=always`/`script -q` because programs suppress SGR off-tty), plus
wide/CJK, grapheme-cluster, and OSC 8 cases.
**Validation:** all fixtures green; a deliberately corrupted fixture fails
with a named validator Result; harness runs with no window, no GPU, no PTY.

### Step 7: Sprint close
COUNSELOR re-reads every step's files against the PLAN. @Auditor sweeps once,
whole sprint. Findings resolved per DCF §5. ARCHITECT builds and runs the
harness (AGENTS BUILD FOR ARCHITECT TO TEST — the run command is one line,
reported at handoff).

## BLESSED Alignment

- **B:** AnsiDocument owns source + elements via the engine's arena; Stamp/
  Grapheme/Hyperlink identity assigned only through registry APIs (no manual
  index arithmetic — CRITICAL RULES framework-API line).
- **L:** tokenizer branch chains land in `Function::Map`/byte-class tables,
  never if/else ladders; 30-line functions; harness is one small TU.
- **E (Explicit):** wire errors are validator Results, loud and named; no
  parser asserts on input; no bail-outs.
- **S (SSOT):** Stamp table = SGR vocabulary = fixture generator — one truth,
  three uses. Cells live once in the Element; the frame is derived, never
  stored.
- **S (Stateless):** pen state is transient creation state inside build();
  dies with the parse pass.
- **E (Encapsulation):** decode-in owned by AnsiDocument, encode-out by
  GraphicsEngine — ANSI at two edges only (RFC §3.5). Data in transit
  untouched: cells are built once at creation and read in place by drawCells.
- **D:** the fixpoint IS the determinism proof — same bytes, bit-identical
  frame, automated.

## Deaths deferred (explicit, with the blocking citation)

- `TerminalLine`: carried end-to-end by `TerminalCellFifo`
  (jam_terminal.h:105-108); CellFifo is ARCHITECT-ruled untouched this sprint.
  Dies with the live-lane sprint's FIFO payload redesign.
- `terminal::TextLayout`: ARCHITECT overrode the deferral mid-sprint — dies in
  this sprint. Grep evidence: zero code callers exist (every non-comment
  TextLayout hit in jam is `juce::TextLayout`); the original deferral premise
  ("markdown draw callers") was wrong.
The TerminalLine death remains deferred; its execution sprint moves only on
ARCHITECT's word.

## Risks / Open Questions

1. **Harness placement is a new lane** — jam has zero test targets today
   (Pathfinder-confirmed). Proposed: `jam/tests/ansi_fixpoint/` console target.
   ARCHITECT amends location/name at approval.
2. **Live lane parserless in the interim:** deleting the DFA trio leaves
   TerminalVideo with no driver until the streaming AnsiDocument sprint —
   nothing in jam/eve instantiates that lane today (EVE has no PTY code).
3. **`AnsiTokenType` member set** (`text/csi/osc/control`) and `Id::cells` are
   proposed names — ratified by plan approval, amendable in place.
4. **Memory-FILE portability:** `open_memstream` is POSIX; Windows uses
   `tmpfile`. The harness isolates this in one function.
