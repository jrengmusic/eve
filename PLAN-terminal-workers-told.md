# PLAN: Terminal Workers Told

**RFC:** none. The objective comes from ARCHITECT's prompts in this session:
- *"if Terminal<Processor> is analogous to DSP processor (check filter-strip with KANJUT), then it should not hold any atomic at all. it should not have getter. it should be TOLD by Model (parameter)"*
- *"DSP processor is `stateful` but not state machine, their state is ephemeral SYNCED unidirecionally from Model to them as dumb workers. they virtually have no getters, with the exception of filters, which Component widgets need to get SSOT directly, without copy …"*
- *"correction per @carol/MANIFESTO.md orchestrator TELL never ask, dumb workers downstream always being TOLD"*
- *"fix this sprint. no open risk should be left before we wrap."*
- Ruling on lane reads (AskUserQuestion, this session): **"Lane API: reads stay"**. Free space and ready count are the AbstractFifo API of a lane. They stay.

**Date:** 2026-11-19
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM. One object, one translation unit (CODING.md).
**Supersedes:** the approved plan "Terminal Workers Told by the Model" (M1-M6). It holds M1-M6 and adds the rest of the inventory.

## Overview

The terminal workers hold no cross-thread atomic and no decision getter. Each boundary that another thread uses to decide is a Model row. Each cross-thread lane belongs to the orchestrator, as `input`, `commands` and `gridEvents` already do. The orchestrator tells each worker; no worker asks the orchestrator, and no caller asks a worker for its state.

## Context

### Rule (MANIFESTO :329-334)
*"Orchestrator → owns Workers, listens to Model change notifications, tells workers to calculate"*. *"No Worker ever asks the Orchestrator anything."* S Stateless: *"Almost never a getter — if the orchestrator needs to ask, the design is wrong"*.

### Exemplar
`FilterStripAudioProcessor::parameterChanged` dispatches each parameter through `parameters` to a worker setter (`FilterStripAudioProcessor.cpp:155-159`, `.h:358`, `FilterStripAudioProcessorRegistration.cpp:44`, `:53`, `:89`). The one getter is an in-place read of calculation state (`getLowMagnitude`, `.h:254`).

### Inventory of violations

| # | Site | What asks or holds |
|---|---|---|
| V1 | `jam_TerminalSwap.h:286`, `:295-297` | Swap atomics `indexExtent`, `firstLine`, `endLine`, `closedLine` |
| V2 | `jam_TerminalImageRecord.h:191-194` | Record atomics `indexExtent`, `firstImage`, `endImage` |
| V3 | `jam_TerminalSwap.h:100`, `:165`, `:173`, `:176` | Getters `getNumImages`, `getLines`, `getClosedLines`, `hasLiveLines`. Readers: `jam_TextEditor.cpp:1418`, `:1499`, `:1558`, `:1567`, `:1596`; `jam_TerminalHistory.cpp:136`, `:156`, `:158`, `:187`, `:203`, `:227`, `:232`, `:255`, `:257`, `:269`; `jam_PreviewComponent.cpp:221` |
| V4 | `jam_TerminalSwap.h:292` | Cross-thread lane `liveFifo` inside the swap. The decoder writes it (`jam_TerminalProcessor.cpp:88`); the editor's LiveJob reads it (`jam_TerminalHistory.cpp:285`) |
| V5 | `jam_TerminalProcessor.h:118`, `:247` | Cross-thread lane `images` inside the decoder, handed out by `getImages()`; the orchestrator asks for it (`EVETerminalProcessor.cpp:324`) |
| V6 | `jam_TerminalProcessor.cpp:57`, `:306`, `:309`, `:315`, `:833`, `:840`, `:1052`, `:1066`, `:1073`, `:1091`, `:1246` | The decoder asks the buffer `getScreen()`. The decoder sets the screen itself through `TerminalBuffer::setScreen` (`jam_TerminalBuffer.h:313`) |
| V7 | `EVETerminalProcessor.cpp:449`, `:558` | The orchestrator asks `tty->isOpen()` |
| V8 | `EVETerminalProcessor.cpp:91-97`; `jam_TerminalTTY_windows.h:152-170`; `jam_TerminalConsole_windows.h:123-141` | The Windows TTY pulls cursor, winsize, pen, input mode and output mode from the Model through the orchestrator |
| V9 | `jam_TerminalImageRecord.cpp:59`, `:187` | F2: a hold below `firstImage` becomes `max()` |
| V10 | `jam_TerminalSwap.cpp:207` | F3: `firstLine` stored with release |
| V11 | `jam_TerminalRecordRing.h:58`; `jam_TerminalImageRecord.cpp:136` | F4: `getNumRecords` sums into `int` |
| V12 | `jam_TextEditor.cpp:2109`, `:2125`; `jam_PreviewComponent.cpp:298-301`, `:359` | F1: a box's RecordJob starts before `heldImage` is stored |
| V13 | `PLAN-swap-risks.md` Names table | F5: it says "No new name" |

### Kept by ruling or by CONTRACT (no change)

| Site | Why it stays |
|---|---|
| `commands.getFreeSpace()` `:128`, `gridEvents.getFreeSpace()` `:153`, `input.getFreeSpace()` `:264`, `:278`, `images.getNumReady()` `:325`, `terminalBuffer.getNumFreeRows()` (`jam_TerminalProcessor.cpp:169`, `:1402`, `:1499`) | ARCHITECT: **"Lane API: reads stay"** |
| `TerminalBuffer` ring and atomics (`jam_TerminalBuffer.h:526-529`); the swap's row reads (`jam_TerminalSwap.cpp:89`, `:145-162`) | The buffer is the row lane. The orchestrator owns it (`EVETerminalProcessor.h:365`) and passes it to each worker. Its reads are lane API (same ruling) |
| `tty->getDefaultBufferSize()` `:376` | The device API of the TTY, the same family as `juce::AudioIODevice::getDefaultBufferSize` (`jam_VirtualDevice.cpp:18`). Same ground as the lane ruling |
| `bypass.load()` `:556`, `background` `:562` | The orchestrator reads the Model on its own thread and tells the decoder each pass (MANIFESTO :334 *"replaces output on each processing pass"*). `jam::Model::ParameterAttachment` calls back on the message thread (`jam_Model.cpp:225-232`). A tell from there to the decoder would be a cross-thread direct call (MANIFESTO B, anti-pattern "Cross-thread direct call") |
| `userInterfaceGetters` (`EVETerminalProcessor.cpp:237-254`) | The established getter channel of the exemplar (`FilterStripAudioProcessor.cpp:25`). After this plan it gives only in-place reads and lanes |
| `reply`, `buffer` FIFOs of the decoder (`jam_TerminalProcessor.h:190-191`) | Work buffers of one thread, inside `process()` (`jam_TerminalProcessor.cpp:64-93`). MANIFESTO S: *"Transient state only — calculation buffers"* |

## Design

### M1. The boundaries are Model rows
Add these rows to the `model` table of `parameters.md`. Each is `int64`, runtime 1.

| Row | Default | Description |
|---|---|---|
| `firstLine` | 0 | Number of the oldest line that a reader can read |
| `endLine` | 0 | Number after the newest stored line |
| `closedLine` | 0 | Number after the newest closed line |
| `lineIndex` | -1 | Extent of the line index of the swap; -1 before the first capacity |
| `firstImage` | 0 | Number of the oldest image that a reader can read |
| `endImage` | 0 | Number after the last ended image |
| `imageIndex` | -1 | Extent of the image index; -1 before the first image |

`cast/terminal.md` gets an `@id` row for each name it does not have. It has `firstLine` (`:53`).

### M2. Workers report, and do not publish
- The swap and the record keep each boundary as a plain writer value: calculation state of the writer.
- A changed boundary is reported through `events` with the row's id. EVE stores it with `storeValue`, in the int64 list of `registerImageEvents` (`EVETerminalProcessor.cpp:167-168`).
- Delete the atomics of V1 and V2, and the getters of V3.
- `Id::firstLine` leaves the grid lane. `gridEventTypes` keeps `gridStart` and `removedGridStart` (`EVETerminalProcessor.h:361`). `applyGridEvents` drains the lane, then drops the grid starts below the `firstLine` row of the tree, with the body of list update 2 (`EVETerminalProcessor.cpp:209-216`). `EVEView::registerTerminalEvents` adds `Id::firstLine` beside `Id::numGridEvents`, and both run `applyGridEvents` (`EVEView.cpp:147-150`). One channel carries `firstLine`: the Model row.

### M3. Hold order (D1) through the Model
- Writer: report the drop (seq_cst `storeValue`, `jam_ParameterBase.h:232`), then load the hold raw atomic (seq_cst).
- Reader: store the hold, then load the boundary raw atomic with seq_cst before each read.
- **F2:** pass the hold unchanged to `isReusable` and `isRangeReusable`. A conflict is `heldAfter >= 0 and heldAfter <= last` for image bytes, and `heldAfter >= 0 and heldAfter <= first` for the image index.
- **F3:** each `firstLine` report is a seq_cst `storeValue`.

### M4. Readers read the Model
- **TextEditor, message thread:** reads `firstLine`, `endLine` and `closedLine` from the `terminal` tree after the vblank flush (`EVETerminalProcessor.cpp:239`). The hold path loads the raw atomic seq_cst after the hold store.
- **Job threads** (`HistoryJob`, `LiveJob`, `RecordJob`, `TerminalHistory`): take the raw atomics of the rows they read as arguments, in the form of `writeFromTerminalBuffer (…, const std::atomic<int64_t>& heldLine, …)` (`jam_TerminalSwap.h:92`). The editor gets them with `Model::getRawParameterValue<int64_t>` (`jam_Model.h:967-980`).
- **In-place reads stay** (the `getLowMagnitude` exception). Each takes its extent number and holds no atomic: `applyToLineData (line, lineIndex, callback)`, `applyToImageData (image, imageIndex, callback)`. The reader keeps the range and index check that the swap made before (`jam_TerminalSwap.h:150-162`): the index load, the range test against the rows, then the index load again.
- **F1:** `setImageBounds` stores `heldImage` first, over the boxes and the visible images, and again at its end.

### M5. Lanes belong to the orchestrator
- **Images lane (V5):** `jam::TerminalFIFO images` moves from `TerminalProcessor` to `EVETerminalProcessor`, beside `input`, `commands` and `gridEvents`. `TerminalProcessor::process` and `TerminalProcessor::reset` take `TerminalFIFO& images`. `reset` prepares it, as it does now (`jam_TerminalProcessor.cpp:28`). Delete `getImages`.
- **Live lane (V4):** `juce::AbstractFifo liveSlots { jam::TerminalSwap::numLiveSlots }` is a member of `EVETerminalProcessor`. `numLiveSlots` (`jam_TerminalSwap.h:179`) becomes public. `TerminalSwap::setLiveLines` and `TerminalSwap::applyToLiveData` take `juce::AbstractFifo& liveSlots`. `TerminalProcessor::process` passes it on. The text editor getter fills the swap and the lane (`EVETerminalProcessor.cpp:237-252`; `jam_TextEditor.cpp:89-96`). `addJobs` reads `liveSlots.getNumReady()` (lane API, ruling). `LiveJob` holds the lane reference beside its swap reference (`jam_TextEditor.h:1558`). Delete `hasLiveLines`.
- Selection: the Model-row double buffer is deleted as an option. It writes again the hand-off that `juce::AbstractFifo` supplies (CODING "Use the framework API fully").

### M6. The decoder holds its screen (V6)
`TerminalProcessor` holds `int screen { map::Screen::normal }` as calculation state, and tells the buffer through `setScreen` (`jam_TerminalBuffer.h:313`). Each `terminalBuffer.getScreen()` site reads `screen`. Delete `TerminalBuffer::getScreen` (`:316`). The compiler lists any other caller.

### M7. The TTY exists only while its session is open (V7)
- `tty` is empty at construction (`EVETerminalProcessor.h:372`).
- `prepare()` opens the clock only for a valid winsize, a Model value (`EVETerminalProcessor.cpp:366`). For a valid winsize: `openTTY` runs when `tty == nullptr`, then `device.open (…, tty->getDefaultBufferSize())`, then `start()`. The null test reads the ownership of the orchestrator, not the state of a worker (MANIFESTO B: *"Acquisition is initialization, release is destruction"*).
- `openTTY` loses its `tty->isOpen()` branch (`:449`). The profile branch keeps its form.
- `process()` loses its `tty->isOpen()` test (`:558`). The clock runs only after a TTY exists: `VirtualDevice::clock` calls back only while playing (`jam_VirtualDevice.cpp:76-82`).
- `releaseResources()` ends the session with `tty.reset()` in place of `tty->close()`. Each TTY destructor closes its session. Engineer reads each destructor; a destructor that does not close gets the close.

### M8. The Windows TTY is told (V8)
- The console requests run on the context that `open()` receives (`jam_TerminalConsole_windows.h:15`). That is `device.getContext()` (`EVETerminalProcessor.cpp:467`), the context of `process()` (`jam_VirtualDevice.cpp:76-91`).
- Delete `getCursor`, `getWinsize`, `getPen`, `getInputMode` and `getOutputMode` from `TerminalWindowsTTY` and `TerminalConsole`, and their lambdas (`EVETerminalProcessor.cpp:91-94`, `:96`).
- `TerminalConsole` holds `cursor`, `winsize`, `pen`, `inputMode` and `outputMode` as calculation inputs. It sets the two modes from each `SetConsoleMode` of the shell, and reports them through `setInputMode` and `setOutputMode`, which stay (a report, like `onBytesReady`).
- `TerminalTTY` gets `virtual void setCursor (jam::Cell::Point)` and `virtual void setPen (int64_t)`, in the family of `setWinsize` (`jam_TerminalTTY.h:160`). The decoder tells them at the end of `process()`, on the device context. The Unix and Cygwin bodies are empty. `TerminalMemoryTTY` forwards.
- `open()` stores the winsize of its `dims` in the console before the sources arm.
- **Winsize on the device context (resolved during execution).** `setWinsize` ran on the message thread (`EVETerminalProcessor.cpp:550`), while the console request source runs on the device context. EVE gets a session-scoped `jam::AsyncUpdater::Work ttyResize`. Its `onWork` is `tty->setWinsize (getWinsize())`, which reads the Model row on the device context. `openTTY` opens it on `device.getContext()` after the TTY opens. `prepare (winsize, …)` calls `ttyResize.submit()` in place of `tty->setWinsize (winsize)`. `releaseResources()` closes it before `tty.reset()`. `Work::submit` allocates nothing and runs on the context's pool (`jam_AsyncUpdater.cpp:442-445`, `:482-486`). On Windows that pool has one thread (`jam_AsyncUpdater.h:16`), so the tell and the console requests are serial. The Model row is the payload, so no member crosses threads.

### M9. Ring total in 64 bits (F4)
`TerminalRecordRing::getNumRecords` returns `juce::int64` and sums without an `int` cast. `addLineExtent` casts at its own call.

## Language / Framework Constraints
- Delete first (CAROL.md Refactor-Rewrite Discipline). The compiler lists what remains.
- No comment and no doxygen before the audit.
- Builds run only through cast, inside the scout run.
- Each Engineer prompt restates: Design by Contract per CODING.md CRITICAL RULES; the MANIFESTO E data-flow contract verbatim; doxygen first; no comment or doxygen.

## Framework Inventory

### API
| Need | Module | API | Source | Step |
|---|---|---|---|---|
| A Model row from the layout | jam/CAST | `parameters.md` model table, `cast/terminal.md` `@id` | `parameters.md:16-133`; `terminal.md:53` | 1 |
| Store from any thread, seq_cst | jam | `Model::storeValue` | `jam_Model.h:1013-1014`; `jam_ParameterBase.h:230-232` | 1-4 |
| Raw atomic for a job thread | jam | `Model::getRawParameterValue<int64_t>` | `jam_Model.h:967-969` | 6 |
| Tree read on the message thread | JUCE | `ValueTree::getProperty` on `terminal` | `jam_TextEditor.cpp:127` | 6 |
| Row listener in the view | EVE | `terminalEvents.add (Id::…)` | `EVEView.cpp:147-150` | 1 |
| Lane hand-off | JUCE | `juce::AbstractFifo` | `jam_TerminalSwap.h:292` | 5 |
| Byte lane | jam | `TerminalFIFO` | `jam_TerminalFIFO.h:20-240` | 5 |
| Screen select | jam | `TerminalBuffer::setScreen` | `jam_TerminalBuffer.h:313` | 7 |
| Clock gate | jam | `VirtualDevice::clock` | `jam_VirtualDevice.cpp:76-82` | 8 |
| TTY told call | jam | `TerminalTTY::setWinsize` family | `jam_TerminalTTY.h:160` | 9 |
| Ring of records | jam | `TerminalRecordRing` | `jam_TerminalRecordRing.h:26-89` | 2 |

### Pattern usage
| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Orchestrator tells a worker | `FilterStripAudioProcessor::parameterChanged` | `FilterStripAudioProcessor.cpp:155-159` | all |
| Worker reports through `events`, EVE stores | int64 ids → `storeValue` | `EVETerminalProcessor.cpp:167-168` | 1-4 |
| Lane owned by the orchestrator, passed to a worker | `input.writePendingData (*tty)` | `EVETerminalProcessor.cpp:358-360`, `:560` | 5 |
| Raw atomic passed to a worker call | `writeFromTerminalBuffer (…, heldLine, heldImage)` | `EVETerminalProcessor.cpp:327` | 3, 4, 6 |
| In-place read of calculation state | `getLowMagnitude` | `FilterStripAudioProcessor.h:254` | 4, 6 |
| View reacts to a Model row | `terminalEvents.add (Id::numGridEvents, …)` | `EVEView.cpp:147-150` | 1 |
| Job result read by its owner | `historyJob->block.document` size test | `jam_TextEditor.cpp:1543` | 6 |

### Names
| Family | Members | Intent | Step |
|---|---|---|---|
| terminal Model rows (`cast/terminal.md`) | `firstLine` `:53`, `heldLine` `:57`, `heldImage` `:58` | a terminal value in the Model | 1 |
| new rows | `endLine`, `closedLine`, `lineIndex`, `firstImage`, `endImage`, `imageIndex` | the existing member names of the swap and the record, now rows | 1 |
| EVE lanes | `input`, `commands`, `gridEvents` (`EVETerminalProcessor.h:358-360`) | a noun for the content of a lane | 5 |
| new lane names | `images` (the moved member, `jam_TerminalProcessor.h:247`), `liveSlots` (swap family `numLiveSlots`, `slotExtents`, `liveOutputs`) | the content of the lane | 5 |
| TTY told setters | `setWinsize` (`jam_TerminalTTY.h:160`) | `set` replaces a value in place | 9 |
| new setters | `setCursor`, `setPen` | the console's cursor and pen | 9 |
| decoder state | `cursor`, `pen`, `winsize` (`jam_TerminalProcessor.h:192-246`) | calculation state of the decoder | 7 |
| new member | `screen` (buffer family `setScreen`) | the active screen | 7 |
| PLAN-swap-risks names (F5) | `setStoredLine`, `writeNextLine`, `addImageExtent`, `getLastTakenImage`, `isRangeReusable`, `setImageStart`, `imageData`, `setNextExtentStart`, `isAppendable` | families `set…Start`, `add…Extent`, `is…`, `get…`, `write…` | 11 |

### Synthesis
| Rule | Established form | Exemplar |
|---|---|---|
| "No non-owning naked pointers as members" | Pass the lane or the atomic at the call | `EVETerminalProcessor.cpp:327` |
| "Use the framework API fully" | `juce::AbstractFifo` for each hand-off | `jam_TerminalFIFO.h:238` |
| "Consume the event payload" | The row listener reads the row it names | `EVEView.cpp:152-155` |
| "No bail-out guards" | Positive nesting; result returns | `EVETerminalProcessor.cpp:366` |
| "Deadline threads" | No allocation on the clock: plain tells, no `Mailbox` (it allocates per write, `jam_Mailbox.h:41-44`) | M8 |

### Absence
| Need | Query | Result |
|---|---|---|
| A Model listener that tells a worker on its own thread | `jam_Model.h` read, `jam_ParameterBase.h:51-62`; `ParameterBase::Listener` derived types in doxygen | Only `Model::ParameterAttachment`, which calls back on the message thread (`jam_Model.cpp:225-232`) |
| A non-allocating cross-thread tell into a worker | `jam_core.h:80-123` concurrency list | `jam_Mailbox.h` (allocates), `jam_AsyncUpdater.h` (a call, no payload). See Risks |

## Validation Gate
Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 1: Model rows and the firstLine channel
**Scope:** `eve/Source/layout/parameters.md`; `jam/cast/terminal.md`; `EVETerminalProcessor.h/.cpp` (int64 id list, `gridEventTypes`, `applyGridEvents`); `EVEView.cpp` (`registerTerminalEvents`).
**Action:** M1 rows and ids. The new ids join the int64 list. M2 firstLine channel.
**Inventory:** API rows 1, 2, 5; pattern "View reacts to a Model row"; names rows 1-2.
**Validation:** each row has its id and default; `gridEventTypes` has two entries; `applyGridEvents` trims by the row.

### Step 2: Ring total (F4)
**Scope:** `jam_TerminalRecordRing.h/.cpp`; its callers.
**Action:** M9.
**Validation:** no `int` sum remains.

### Step 3: Image record reports
**Scope:** `jam_TerminalImageRecord.h/.cpp`.
**Action:** delete the V2 atomics and `getNumImages`. Report `firstImage`, `endImage`, `imageIndex` through `events`; `writeImages` and its callees take `const Events&`. F2. `applyToImageData (file, image, imageIndex, callback)`.
**Validation:** no `std::atomic`; D1 order on `firstImage` and on its restore path.

### Step 4: Swap reports
**Scope:** `jam_TerminalSwap.h/.cpp`.
**Action:** delete the V1 atomics and the V3 getters. Plain writer values; reports in D1 order; F3. `applyToLineData (line, lineIndex, callback)`. `setCapacity` reports `firstLine`, `endLine`, `lineIndex`.
**Validation:** no `std::atomic` and no decision getter in the swap.

### Step 5: Lanes to the orchestrator
**Scope:** `jam_TerminalProcessor.h/.cpp`; `jam_TerminalSwap.h/.cpp`; `EVETerminalProcessor.h/.cpp`.
**Action:** M5 for `images` and `liveSlots`.
**Validation:** no lane member in the decoder or the swap; `getImages` and `hasLiveLines` are gone.

### Step 5b: `apply` to `set` (added mid-sprint)
**Ruling:** ARCHITECT: *"set is not only value, it's also data."* *"those `apply` should be `set`"* (on `applyToImageData`, `applyToLineData`, `applyToLiveData`). *"applyPending probably stands, since `pending` usually refers to concurrency"*. *"i saw lots of wrong usage of `apply` that should be `set`"*, then `/go` on the classification. NAMES.md Verb Contract `set`: *"Replaces an existing value or existing data in place."*
**Scope:** the declarations, definitions and every caller of the renamed functions.
**Action:**
- `applyToImageData` → `setImageData` (jam_TerminalImageRecord.h, jam_TerminalSwap.h)
- `applyToLineData` → `setLineData`; `applyToLiveData` → `setLiveData` (jam_TerminalSwap.h)
- `TerminalProcessor::applyImageStart` → `setTransitImage` (its body is one assignment to `transitImages`, jam_TerminalProcessor.cpp:197-200). `setImageStart` is already `TerminalImageRecord::setImageStart`, another operation (NAMES Rule 5).
- Unchanged, by the `apply` row ARCHITECT gave (*"apply is operational … the body of `apply` are `set` operation"*; NAMES.md: *"Runs an operation. The body of an `apply` is `set` operations. Example: `Model::applyFunctionRecursively`."*): `applyWinsize`, `applyTokens`, `applyToken`, `applyImageSequence`, `applyImageArguments`; the lane drains `applyPendingData`, `applyBytes`, `applyCommands`, `applyGridEvents`, `applyPendingInput`; the reports `applyEvents`, `applyGridStart`.
**Validation:** no old name remains at a declaration, definition or caller.

### Step 6: Readers read the Model
**Scope:** `jam_TextEditor.h/.cpp`; `jam_TerminalHistory.h/.cpp`; `jam_PreviewComponent.h/.cpp`; `EVETerminalProcessor.cpp` (`copyToClipboard`, the getters).
**Action:** M4 and the reader side of M5. F1.
**Validation:** each job loads its boundary seq_cst after its hold store. No deleted getter remains (compiler).

### Step 7: Decoder screen
**Scope:** `jam_TerminalProcessor.h/.cpp`; `jam_TerminalBuffer.h/.cpp`.
**Action:** M6.
**Validation:** `getScreen` is gone; one screen value, set only by the decoder's tell.

### Step 8: TTY session lifecycle
**Scope:** `EVETerminalProcessor.h/.cpp`; the TTY destructors (`jam_TerminalTTY_posix`, `jam_TerminalTTY_windows`, `jam_TerminalCygwinTTY_windows`, `TerminalMemoryTTY`).
**Action:** M7.
**Validation:** no `isOpen()` call in EVE; the clock starts only with a TTY.

### Step 9: Windows TTY told
**Scope:** `jam_TerminalTTY.h`; the four TTY implementations; `jam_TerminalConsole_windows.h/.cpp`; `jam_TerminalProcessor.cpp`; `EVETerminalProcessor.cpp`.
**Action:** M8, with the winsize item as ARCHITECT rules it (Risks).
**Validation:** no `get…` callback in the Windows TTY or the console; each told value is written on the device context.

### Step 9b: Report cost (added mid-sprint)
**Ruling:** ARCHITECT: *"can we eliminate the cost"*, then `/go` on the three cuts. Measured: `swap history lines` 3.66e8 → 6.80e8 ns (`benchmark/seq-1m-windows-cygwin-one-file-report.md:73`).
**Scope:** `jam_TerminalSwap.cpp`; `EVETerminalProcessor.cpp` (`registerImageEvents`).
**Action:**
1. `endLine` and `closedLine` are reported once per `writeFromTerminalBuffer` call, after the final `setLines`. The per-line reports go.
2. `firstLine` is reported only when its value changes. D1 holds: a moved boundary is still reported before the hold load.
3. EVE resolves each int64 row parameter once in `registerImageEvents`; its handler calls `store` on it (`jam_ParameterBase.h:230-233`), with no tag and id lookup.
**Validation:** D1 order on every `firstLine` change; one `endLine` and one `closedLine` report per write call; no `storeValue` lookup in the int64 handler.

### Step 10: SPEC amendment
**Scope:** `eve/SPEC.md`, the swap-risks amendment (`~:465`).
**Action:** M1-M9 with ARCHITECT's words (header).

### Step 11: Names record (F5)
**Scope:** `PLAN-swap-risks.md` Names table.

### Step 12: Measure
**Action:** scout `benchmark/seq-1m-windows-cygwin-one-file.md`. Watchdog 15 min.
**Validation:** exit 0, 0 dropped, epoch within the last spread (4.97–6.69 s).

### Step 13: Auditor
Every file of Steps 1-9. COUNSELOR validates each finding.

### Step 14: Doxygen pass
@Pathfinder updates every changed block, and the stale blocks of the PLAN-swap-risks audit. COUNSELOR reads each block against the code.

### Step 15: Report
COUNSELOR reports the Step 12 values. ARCHITECT runs the interactive tests.

## BLESSED Alignment
- **S (Stateless):** workers hold calculation state only. The Model holds every boundary that another thread uses to decide.
- **S (SSOT):** one copy of each boundary (the row); one channel for `firstLine`; one screen value.
- **E (Encapsulation):** no decision getter; in-place reads take their extent; lanes belong to the orchestrator.
- **B:** the TTY session lives as long as its owner holds it.
- **D:** D1 order through seq_cst `storeValue` and raw-atomic loads.

## Risks / Open Questions
- **Windows console winsize (M8).** The console needs the winsize on the device context. `setWinsize` runs on the message thread (`EVETerminalProcessor.cpp:550`, inside `audioDeviceAboutToStart`), while the console request source runs on the device context (`jam_TerminalConsole_windows.h:15`). No compliant hand-off was found:
  - an atomic in the console breaks *"should not hold any atomic at all"*;
  - a reference member to the Model atomic breaks CODING "No non-owning naked pointers as members";
  - `jam::Mailbox` allocates on each write (`jam_Mailbox.h:41-44`).
  Cursor and pen have no such conflict: the decoder tells them on the device context.
  **Closed during execution:** M8 "Winsize on the device context" (`ttyResize`, an `AsyncUpdater::Work` on `device.getContext()` that reads the `winsize` row).
