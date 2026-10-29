# PLAN: EVE beats endless — no allocation on the reader, bound events, then read cadence

**RFC:** none — objective from ARCHITECT's prompt (session 2026-10-29). Companion: `scout/RFC-kernel-sampling.md` (kernel evidence, separate work)
**Date:** 2026-10-29
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 + JAM (LANGUAGE.md "C++ / JUCE": one object, one translation unit)

## Context

ARCHITECT, verbatim:
- *"continue your work with eve, what can we optimize, why writer took longer than endless, what can we improve to even beat endless"*
- Read cadence: *"Include cadence"*. The chosen option said: "Its design is still open and goes to you before that step runs."
- Event path: *"Bind once"*. The chosen option said: "registerEvents takes each Parameter once with getParameter; the handler calls store(). SPEC.md:317 says store() in place of storeValue."

Measured facts:
- **Verdict (Release, no profiler):**
  - EVE: 1:04.05, `seq` system 46.13 s, 10 waits (`seq-eve/result.txt:15-23`).
  - END: 1:07.56, `seq` system 40.54 s, 265,299 waits (`seq-end/result.txt:4-12`).
- **Reader (scout live):** 87,248,547 reads and 8,537,114 ticks per 100M run (`benchmark/live-report.md:29`, `:35`), so about 11.3 bytes per read that returns bytes.
- **EVE trace (15.8 s):**
  - leaf `read` 7,741 ms;
  - `HashMap::doFind` 1,607 ms (971 via `Model::getParameter` ← `applyEvents`);
  - allocation under `TerminalSwap::writeLine` about 1,155 ms;
  - `malloc` / `free` under `TerminalProcessor::process` (`xt-eve-aggregate.txt`, `xt-eve-callers.txt`).
- **Cause of the per-call allocation:**
  - every `juce::OutputStream` constructor builds `String newLineString` from `"\r\n"` (`juce_OutputStream.cpp:71-72`, `juce_OutputStream.h:247`);
  - `String (const char*)` calls `new char[]` (`juce_String.cpp:82`, `:90-98`, `:307-308`).
- **Allocation sites:**
  - `jam_TerminalProcessor.cpp:41`: one per read that returns bytes, on the clock;
  - `jam_TerminalSwap.cpp:80`: one per live publish, on the clock;
  - `jam_TerminalSwap.cpp:122` and `:126`: two per history row, on the history writer.

CONTRACT clauses that bind:
- MANIFESTO E, Deadline threads: "does not allocate, lock, wait, or make a blocking call … The owner sizes every buffer of that thread in `prepare`".
- MANIFESTO E, data in transit: "No allocation".
- `SPEC.md:380`: "Reader thread: zero allocations after session start".
- `SPEC.md:392`: "❌ Reader thread: … allocation".

## Language / Framework Constraints

- CODING.md CRITICAL RULES. Code Hygiene: no comments before the audit; doxygen last.
- One object, one translation unit. Every change stays in the object's existing `.h` and `.cpp`.
- The CAROL Profiling and END-safety rules from `PLAN-seq-profiling.md` hold for every run:
  - kill only `$!`;
  - check that END pid 38202 is unchanged;
  - check that the SHAs of `window.state` and `END.cache` match the backups;
  - run one measurement at a time.

## Dependency & API Inventory

**JUCE `juce::MemoryOutputStream`, external-buffer form `(void* destBuffer, size_t destBufferSize)`** (`juce_MemoryOutputStream.cpp:52-56`):
- It never grows. `prepareToWrite` returns `nullptr` past `availableSize` (`:100-106`).
- `reset()` sets `position` and `size` to 0 and does nothing else (`:80-84`, `noexcept`).
- `getPosition()` returns the write position (`juce_MemoryOutputStream.h:124`).
- The `MemoryBlock` form (`:44-50`) grows the block (`:95-96`) and trims it on destruction (`:68-72`). It is not used.

**Established member patterns (nearest sibling):**
- One stream or mapping that is re-created when its target changes: `std::unique_ptr<juce::MemoryMappedFile> index;` and `segment;` (`jam_TerminalSwap.h:176-177`), re-created through `getOrCreateMapping` (`jam_TerminalSwap.cpp:11-26`).
- A set of owned streams: `jam::Owner<juce::MemoryOutputStream> contents;` (`jam_PdfLowLevelGraphicsContext.h:435`), filled with `contents.add (std::make_unique<juce::MemoryOutputStream>())` (`.cpp:10`).
- A Model value bound once at construction: `bypass (*model.getRawParameterValue (…))` (`EVETerminalProcessor.cpp:15`, `.h:187`).

**JAM Model:**
- `jam::Model::getParameter<T> (tag, id)` (`jam_Model.h:955-967`).
- `jam::Parameter<int>::store (int)`: a relaxed store, then `needsUpdate` set with release order (`jam_ParameterBase.h:176-180`).
- `AnyMap<>` uses `juce::DummyCriticalSection`, so it takes no lock (`jam_AnyMap.h:37-41`).
- `storeValue` asserts the parameter (`jam_Model.h:983-985`).

**Where each new stream is created** (all off the clock):
- `TerminalProcessor::prepare` (`jam_TerminalProcessor.cpp:7-13`) runs from `audioDeviceAboutToStart` (`EVETerminalProcessor.cpp:186`).
- The `TerminalSwap` constructor and `prepare` (`jam_TerminalSwap.cpp:3-9`, `:43-51`) create `live`.
- `writeLine` / `setSegment` (`jam_TerminalSwap.cpp:116-119`, `:139-144`) create `index` and `segment` on the history writer. That is the creation point of each mapping today.

**New names, verbatim.** No other new name may appear.
- `reply`: the member stream in `jam::TerminalProcessor`. It takes the name of today's local.
- `liveOutputs`: owned streams, one for each live slot.
- `indexOutput`: the stream over the `index` mapping.
- `segmentOutput`: the stream over the `segment` mapping.

## Validation Gate
Each step is validated by COUNSELOR before proceeding to the next — against
MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions
(no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering
the whole sprint — never per step.

## Steps

### Step 0: Write the plan to the project root
**Scope:** `eve/PLAN-eve-beat-endless.md`.
**Action:** COUNSELOR writes this plan to the EVE root.
**Validation:** the file matches this plan.

### Step 1: The reply stream is a member, created in `prepare`
**Scope:** `jam/jam_terminal/processor/jam_TerminalProcessor.h`, `.cpp`.
**Action:** @Engineer:
1. Add the member `std::unique_ptr<juce::MemoryOutputStream> reply;` after `replyBytes` (`.h:87`).
2. In `prepare`, after `replyBytes.setSize (…)` (`.cpp:12`), create `reply = std::make_unique<juce::MemoryOutputStream> (replyBytes.getData(), replyBytes.getSize());`.
3. In `process`, delete the local at `.cpp:41`. Call `reply->reset();`.
4. Pass `*reply` to `applyTokens`. Use `reply->getDataSize()` for the TTY write (`.cpp:44-45`).
5. The table signatures (`.h:106`, `:110`, `:146`, `:176`) stay `juce::MemoryOutputStream&`.

**Validation:** COUNSELOR reads the lines.
- No `MemoryOutputStream` is constructed in `process`.
- The only creation is in `prepare`.
- `reset` runs before each `applyTokens`.
- CODING: brace initialisation, `not`/`and`/`or`, no bail-out guard.

### Step 2: The live slot streams are members, created with `live`
**Scope:** `jam/jam_terminal/document/jam_TerminalSwap.h`, `.cpp`.
**Action:** @Engineer:
1. Add the member `jam::Owner<juce::MemoryOutputStream> liveOutputs;` after `live` (`.h:171`).
2. Each time `live` is created (constructor `.cpp:8`, `prepare` `.cpp:50`), refill `liveOutputs` with `numLiveSlots` streams. Stream `slot` uses `static_cast<char*> (live->getData()) + slot * segmentBytes` and `segmentBytes`.
3. In `setLiveLines`, delete the local at `.cpp:80`. Call `reset()` on the stream of slot `start1`, then pass it to `writeToStream`.

**Validation:** COUNSELOR reads the lines.
- `setLiveLines` constructs nothing.
- Every creation of `live` refills `liveOutputs`, so no stream outlives its mapping.
- Read the `jam::Owner` API before the delegation, and paste the accessor it uses into the prompt.

### Step 3: The history writer streams are members; the stream position replaces `segmentPosition`
**Scope:** `jam/jam_terminal/document/jam_TerminalSwap.h`, `.cpp`.
**Action:** @Engineer:
1. Add the members `std::unique_ptr<juce::MemoryOutputStream> indexOutput;` and `segmentOutput;` beside `index` and `segment` (`.h:176-177`).
2. Delete the member `segmentPosition` (`.h:179`). It would be a second copy of `segmentOutput->getPosition()` (MANIFESTO S; `SPEC.md:399`).
3. In `writeLine`, at the start of a group (`.cpp:116-120`), create `indexOutput` over the whole `index` mapping.
4. In `setSegment` (`.cpp:139-144`), create `segmentOutput` over the whole `segment` mapping.
5. Rewrite the body of `writeLine` as follows:
   - Take `segmentOutput->getPosition()` before `writeRow`.
   - On success, write `segmentNumber` and that position to `indexOutput`.
   - On failure, keep `setSegment` plus the recursive call (`.cpp:132-136`).
   - Delete the locals at `.cpp:122` and `:126`.
6. The index entries stay at their offsets, because lines arrive in order from the start of each group (`writeFromTerminalBuffer`, `.cpp:96-104`).

**Validation:** COUNSELOR reads the lines.
- `writeLine` constructs a stream only where it creates a mapping.
- `segmentPosition` is gone, with all of its uses.
- `addLines` / `addLine` read the same layout as before (`.cpp:219-251`).

### Step 4: Bind each Model parameter once in `registerEvents`
**Scope:** `eve/Source/EVETerminalProcessor.cpp`, `eve/SPEC.md`.
**Action:**
1. @Engineer: in `registerEvents` (`.cpp:89-96`), for each id:
   - take `state.getParameter<jam::Parameter<int>> (Id::toType (Id::terminal), id)` once;
   - `jassert` it is not `nullptr`, as `storeValue` does (`jam_Model.h:984`);
   - register a handler that captures the parameter by reference and calls `store`. The `bool` handlers pass `static_cast<int> (value)`.
2. COUNSELOR: change `SPEC.md:317-318` to "write the Model with `store` on the `jam::Parameter` that the owner binds once at registration: an atomic store and a dirty mark", quoting *"Bind once"*.

**Validation:**
- No handler calls `storeValue`.
- Each capture is explicit (MANIFESTO E).
- `state` is declared before `events` (`EVETerminalProcessor.h:188-189`), so the bound parameter outlives its handler (MANIFESTO B).

### Step 5: Build and control run
**Scope:** `eve/Builds/Profile`.
**Action:** @Engineer:
1. Run `cmake -S . -B Builds/Profile -G Ninja -DCMAKE_BUILD_TYPE=Release -DCAST_SIGN=OFF -DCAST_PROFILE=OFF`, then `ninja -C Builds/Profile`.
2. COUNSELOR runs `scout benchmark/control.md`.

**Validation:**
- ninja exits 0 with 0 warnings.
- `JAM_USING_PROFILER` is absent from `compile_commands.json`.
- `control` exits 0.
- The count of crash reports is unchanged.

### Step 6: Measure the effect of Steps 1-4
**Scope:** scratchpad scripts only. No source.
**Action:** COUNSELOR:
1. Run `run-seq.sh` 5 times on EVE and 2 times on END (drift control), with the Verdict lane as before.
2. Rebuild `Builds/Profile` without the profiler if a manifest changed it.
3. Run `xctrace-seq.sh` on EVE once, then `xt-aggregate.py` and `xt-callers.py`, for the comparison with `xt-eve-*.txt`.
4. Run `scout benchmark/live.md` for zone timings.
5. Apply END safety around every END run.

**Validation:**
- A change counts only when the EVE median is outside its earlier spread (1:03.84-1:04.42) and END stays inside its own.
- The trace shows no `malloc`, `free` or `operator new` leaf under `TerminalProcessor::process`.
- The `doFind` share under `applyEvents` falls.

### Step 7: Read-cadence research, then the design goes to ARCHITECT
**Scope:** read-only.
**Action:** @Pathfinder and @Librarian read:
- XNU `xnu-12377.121.6`:
  - the PTY buffer capacity (`bsd/kern/tty.c`, `bsd/kern/tty_pty.c`, `bsd/sys/tty.h`: `TTYHOG`, clist sizes);
  - whether a tty read filter honours a low-water mark (`filt_ttyread`, `NOTE_LOWAT`);
  - when the writer of a PTY blocks.
- The GCD `DISPATCH_SOURCE_TYPE_READ` low-water support.
- END's reader loop and per-chunk work (`endless/Source/terminal/tty/TTY.cpp:53-88`, `ProcessorEvents.cpp:416-426`).

COUNSELOR then presents the cadence design options to ARCHITECT, each with its citations, as ARCHITECT ruled: *"Include cadence"*, with the design going to ARCHITECT before the step runs.
**Validation:** every option cites the read. No option waits, sleeps or blocks on the clock (`SPEC.md:392`). No option adds a second thread context between PTY readiness and the clock (`SPEC.md:403`).

### Step 8: Implement the cadence design that ARCHITECT chose, and measure it
**Scope:** the files that the chosen design names.
**Action:** @Engineer implements the chosen design. COUNSELOR repeats the Step 5 build and the Step 6 measurements.
**Validation:** the same rules as Step 6. The `seq` waits and the bytes per read are reported against END.

### Step 9: Audit, doxygen, endpoint
**Scope:** the files changed in Steps 1-4 and 8.
**Action:** @Auditor sweeps once. COUNSELOR filters the findings (Design by Contract Filter). Then a doxygen pass covers the changed headers, including the `registerEvents` doc at `EVETerminalProcessor.h:144`. The result is 0 warnings under the real Doxyfile settings. The sprint log is written only on ARCHITECT's word.
**Validation:** every accepted finding is resolved. The doxygen run gives 0 warnings.

## BLESSED Alignment
- **B:** each stream is owned beside the mapping or buffer it views, and is re-created with it. The bound parameter is owned by `state`, which outlives `events`.
- **L:** no new object and no new file. Each change stays in its object's existing translation unit.
- **E (Explicit):** explicit captures. Asserts at the binding point.
- **S (SSOT):** `segmentPosition` is deleted. The stream position is the one truth.
- **S (Stateless):** the streams hold transient write positions only. `reply` and `liveOutputs` are reset before each write. `indexOutput` and `segmentOutput` carry the write position of the history writer, which `segmentPosition` held before. That is not new state. (Audit F2, corrected 2026-10-29.)
- **E (Encapsulation), Deadline threads:** no allocation on the clock after `prepare`. No allocation in transit on the history writer, except where a mapping is created.
- **D:** each change counts only outside the measured spread (Step 6).

## Risks / Open Questions
- Closed by ARCHITECT's ruling: the cadence design is presented at Step 7 before Step 8 runs (*"Include cadence"*). No other question is open.
