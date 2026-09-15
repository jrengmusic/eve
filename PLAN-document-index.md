# PLAN: document-index — Document::Index Tier + Hygiene

**RFC:** RFC-EVE-terminal-grand-scheme.md (§3.3 SSOT ruling, §5.3 gap 1, §7.4 storage tier — closed by this sprint's rulings)
**Date:** 2026-09-15
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE + JAM — LANGUAGE.md C++: implementation bodies to .cpp (non-template), header carries declarations; 30-line/3-branch limits unchanged

## Context

The buffer-bench sprint proved the design by measurement: Document stays the SSOT;
a counted B-tree line index gives O(log n) lookup (4–5 orders over any chain
strategy at 10⁶–10⁷); the storage tier is wire-byte spill (89 B/line disk vs
602 B resident, byte-exact round trip, ~100 µs cold-page rehydrate, spill write
5× faster than parse). ARCHITECT rulings this session, now executed:
`Document::Index` (nested vocabulary, joins Property/Span/Token/Cells family,
jam_Document.h:43-161); **full tier inside Index** — budget default 1/16 machine
RAM, watermark eviction, wire-byte spill, rehydrate; EVE config override.
Clean-room from scratch — the sandbox (tests/buffer_bench) is measurement
evidence only, never source. Design by Contract throughout.

ARCHITECT pre-issued: execute to completion, no re-gating; audit covers all
eve+jam files touched (sandbox excluded); /log when done.

## Dependency & API Inventory (Pathfinder, file:line)

- **Document internals:** Element links parent/firstChild/lastChild/nextSibling
  (jam_Document.h:194-197); `appendChild (Element&, Document&&) -> Element*`
  (:594); sibling Iterator (:210-221, operator++ body in jam_Document.cpp —
  the nested out-of-line precedent); Document is passive — zero notification
  machinery (full-header verification): Index syncs by explicit tell.
- **Existing lookup precedent inside Document:** getLineNumber uses
  std::upper_bound over jam::Array<uint32_t> lineOffsets (jam_Document.cpp) —
  O(log n) search is already Document vocabulary.
- **Containers:** jam::Owner — `.add(unique_ptr&&)` returns reference
  (jam_Owner.h:94), flat destruction, heap-stable objects; jam::Array (doubling
  growth, .at, isEmpty); Document::Span = jam::Union<uint32_t,uint32_t>
  (jam_Document.h:51) — the file-span type, reused not reinvented.
- **Assert:** jassert (jam_Document.h:715 jassertfalse precedent).
- **RAM query:** juce::SystemStats::getMemorySizeInMegabytes (precedent:
  buffer_bench main.cpp).
- **Spill file:** juce::TemporaryFile — RAII deletion = SPEC "session lifetime"
  (SPEC Feature 2.2: SSOT retains for session lifetime).
- **Serialisation (injected, layer law):** encode lives in jam_terminal
  (jam::terminal::getRowText — the fixpoint-proven lane, ansi_fixpoint
  aca61aa); decode is jam::AnsiDocument::parse (jam_AnsiDocument.h:41). Both
  sit ABOVE jam_core — Index receives them as an injected Codec at
  construction; jam_core gains no upward include (MANIFESTO E layer topology).
- **EVE config lane:** markdown tables live (Source/layout/ViewLayout.md
  UI_size table); read via MarkdownDocument getTables/getTableRows/
  getTableValueView (jam_MarkdownDocument.h:59, :85, :155).
- **Sandbox as rig only:** buffer_bench harness (gates vs bare walk, spill
  gate) validates the jam implementation; excluded from audit per ARCHITECT.

## Names (Rule -1 ratification — approved with this plan)

- Type: `Document::Index` (ruled); nested private `Node`; nested `Codec`
  (two owned std::function capabilities: encode Element→wire bytes, decode
  wire bytes→Element cells).
- Methods: `appendLine` (joins appendChild family), `getElement (int
  lineNumber)` (joins Document get-family: getLineNumber), `setBudget (int64
  newBudgetBytes)`, `getLineCount`.
- New domain verbs, explicitly flagged for this ratification: `evict`,
  `rehydrate` (private tier operations — no fixed-set verb states these
  operations; tiering is a new domain).
- Constants: `arity { 16 }` (bench: top-2 on the scroll workloads at 10⁶/10⁷;
  coldJump ranks 1st–3rd across runs, all arities within noise),
  `defaultBudgetDenominator { 16 }`, `lowWatermarkNumerator { 7 }` /
  `lowWatermarkDenominator { 8 }`, `rehydratePageLineCount { 50 }`.
- Members: `nodes` (jam::Owner<Node> — flat ownership, Node children are
  non-owning Node*, the sandbox's proven anti-recursion shape), `codec`,
  `spillFile`, `budgetBytes`, `residentBytes`.
- EVE config: table `terminal`, row `scrollbackBudgetMb` (ARCHITECT-corrected:
  row family is `width`/`height` — camelCase data lane, not the table heading's
  shape).

## Validation Gate

COUNSELOR validates each step against MANIFESTO.md, NAMES.md, ~/.carol/CODING.md,
and this locked plan before the next step. @Auditor runs ONCE, after Step 6,
covering every eve+jam file the sprint touched — sandbox excluded per ARCHITECT.

## Steps

### Step 0: Materialise PLAN-document-index.md at eve root (COUNSELOR)

### Step 1: Hygiene — RFC/SPEC/PLAN (COUNSELOR writes; gated execution unlocked by this approval)
**Scope:** RFC-EVE-terminal-grand-scheme.md, SPEC.md, PLAN-buffer-bench.md (delete)
**Action:** RFC gains [AMENDED 2026-09-15] blocks: §3.3 — Document::Index ruling
+ measured constants (602 B/line resident, 89 B/line wire, ~100 µs/page
rehydrate, spill write ≥1.2M lines/s); §5.3 gap 1 — buffer design closed by
measurement, sandbox verdict cited; §7.4 — storage tier designed: wire-byte
spill, 1/16-RAM hot window, watermark eviction (deferral withdrawn by
ARCHITECT). SPEC.md: Feature 2.3 amended — scrollback unbounded in CONTENT,
tiered in residency (hot window default 1/16 machine RAM, cold lines lossless
on disk, byte-exact round trip); Feature 6 list gains the
`terminal / scrollbackBudgetMb` override row (documented, not shipped —
shipping a value would kill the 1/16 machine-relative default ARCHITECT ruled). Delete PLAN-buffer-bench.md (content
lives in the sprint log and results.md).
**Validation:** every amendment traces to a ruling or a measured number in
results.md; no prose contradicts RFC §8 non-negotiables.

### Step 2: jam — Index lookup core (clean-room)
**Scope:** jam/jam_core/document/jam_Document.h (Index class declarations,
nested, sibling of Element), jam/jam_core/document/jam_DocumentIndex.cpp (new —
all non-template bodies, registered for compilation exactly as jam_Document.cpp
is; Engineer verifies the module's compilation registration and mirrors it)
**Action:** counted B-tree, arity 16: `nodes` Owner<Node> flat ownership; Node
holds non-owning child Node* array + per-child line counts; leaves hold
Element*. Constructor walks a Document's Id::line chain once (bulk build);
appendLine right-edge insert; getElement prefix-sum descent. jasserts state
every precondition (DbC). No tier code this step. Written from the Tatham
algorithm description — no sandbox code copied.
**Validation:** buffer_bench gains a jam-Index lane (sandbox edit, audit-exempt):
correctness gate — getElement identical to bare-walk Element* for the fixed-seed
set at 10⁴ and 10⁶ — plus a coldJump timing row landing in the same order of
magnitude as the sandbox winner. COUNSELOR checks CONTRACT compliance on the
diff.

### Step 3: jam — the tier inside Index
**Scope:** same two jam files
**Action:** Codec injected at construction (owned std::functions — no naked
pointer members); budget: `budgetBytes` default = machineRamBytes /
defaultBudgetDenominator via SystemStats, `setBudget` override; accounting:
residentBytes = Σ hot cells.size() × sizeof(AttributedChar), updated on
appendLine/evict/rehydrate — one owner, no shadow; eviction at high watermark
(budget) down to low watermark (7/8): oldest hot lines encode via codec, append
to spillFile (TemporaryFile, RAII = session lifetime), leaf stores
Document::Span, element's Cells replaced by empty Cells — a state update
(complete replaces complete, MANIFESTO E), chain untouched, no removeChild
needed; getElement on a cold leaf rehydrates its page
(rehydratePageLineCount lines) through codec.decode before returning.
**Validation:** buffer_bench jam-Index lane extended: budget forced tiny so
eviction exercises at 10⁵; spill-gate equivalent — evict, rehydrate, byte-compare
re-encoded page vs original span bytes; resident accounting equals recomputed
sum at gate points.

### Step 4: EVE — instance + config override
**Scope:** eve/Source/EVEView.h/.cpp, eve/Source/layout/ViewLayout.md
**Action:** EVEView owns a Document::Index over its ansiDocument; codec wired
from jam_terminal encode lane + AnsiDocument::parse (EVE sits above both
layers — legal). ViewLayout.md gains the `terminal` table with
`scrollbackBudgetMb`; EVEView reads it via the bootstrap markdown lane
(getTableValueView) and calls setBudget only when the row is present —
configure only the difference (CODING.md).
**Validation:** EVE Debug all-formats configure + EVE_Standalone build green;
row absent → default path (no setBudget call); row present → override lands
(log-free verification by code read).

### Step 5: Full rig pass
**Scope:** none new — runs Steps 2–4 validations end-to-end
**Action:** buffer_bench full gates green; ansi_fixpoint and llgc_parity re-run
green (jam_core touched — the fixpoint lanes must not regress).
**Validation:** three harnesses exit 0.

### Step 6: @Auditor — once, whole sprint
**Scope:** every eve+jam file this sprint touched (RFC/SPEC included, sandbox
excluded per ARCHITECT). COUNSELOR resolves all findings in-sprint (DCF §5).

### Step 7: Doxygen pass (post-audit, pre-log — Code Hygiene order)
**Scope:** jam_Document.h Index surface, jam_DocumentIndex.cpp stays doc-free
(header-only docs), EVEView touched decls
**Action:** @Engineer authors doxygen per DOXYGEN DISCIPLINE (zero warnings,
header-only, @param match); regen via jam docs tooling (docs/ exists —
jam/docs/xml; if no regen entry point is discoverable, flag to ARCHITECT
rather than invent one).

### Step 8: /log (COUNSELOR — ARCHITECT pre-issued)
Sprint block per /log; drain any paid debts with receipts.

## BLESSED Alignment

- **B:** all Index nodes in one Owner (flat destruction — the sandbox's measured
  SIGSEGV lesson closed by ownership, not by a destructor workaround);
  spillFile RAII = session lifetime.
- **L:** bodies in .cpp; every function ≤30 lines by decomposition of
  responsibility; arity/watermarks/fractions are named constants.
- **E (Explicit):** Codec injected — all capabilities visible at construction;
  layer topology unidirectional (jam_core never includes upward); jassert
  preconditions, positive nesting.
- **S (SSOT):** residentBytes has one owner; eviction is a Cells state update —
  no second content store; Document::Span reused for file spans.
- **S (Stateless):** Index holds index state only (its one job); Document holds
  content; the widget still owns nothing.
- **D:** fixed-seed gates, byte-exact spill round trip, deterministic arity —
  same input, same output, proven by rig.

## Risks / Open Questions

- **Cold lines paint empty in today's viewer:** jam::TextEditor paints every
  line directly; an evicted line's Cells are empty until rehydrated through
  getElement. With the 1/16 default and current one-shot fixtures, eviction
  never triggers in EVE this sprint; viewport-driven paint through the Index is
  the future buffer/widget sprint. Stated, not hidden.
- **evict/rehydrate** are new verbs — ratified by this plan's approval, flagged
  per NAMES Rule -1.
- **Doxygen regen entry point** unverified (no Doxyfile found) — Step 7 flags
  rather than invents.
