# PLAN: Swap One File — one mapped file per TerminalSwap instance, history drops oldest line first

**RFC:** none. Objective from ARCHITECT prompts and `/goplan`.
**Date:** 2026-11-19
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 / JAM. One object, one translation unit (CODING.md).

**Plan file at lock:** plan mode permits only this file. The first action after the lock copies this plan, unchanged, to `D:\Poems\dev\eve\PLAN-swap-one-file.md`.

## Overview

This is the fourth plan of the sprint of DEBT-20261117T223541 (cygwin output comes in pages). The probes found the choke: the swap writer makes a new 222,937-byte segment file every ~2,350 lines (`jam_TerminalSwap.cpp:141-147`, `:181-200`). Defender `WdFilter.sys` holds the first reopen of each new file for 4.0 ms on average (traced; 8.5 ms untraced), and up to 16 ms. While it waits, the row ring stays full, the helper's write blocks, and seq waits.

This plan puts every part of the swap into one memory-mapped file, made once per instance: the history lines, their index, the two live slots and the image record. The file grows only at its tail, in extents. The history keeps `capacity` lines and drops the oldest line first. A view that reads history publishes the first line that it reads, as a terminal Model parameter. The writer reuses no byte of that line or of a later line.

## Context

### ARCHITECT rulings (verbatim)

1. *"history cap stands, 10k, means 10k line. if incoming data still appending, drop oldest. first in first out. i dont understand why we need to create new file each time"*
2. *"let's try with only one file per instance then. @carol/MANIFESTO.md 0 rules applied, SSOT, complete AST built once cached, memoized, state update is not mutation"*
3. On reuse safety: *"can be as mode parameter no? message thread still use Value, other thread can use raw atomic. event driven always. no manual state management. it's state update"*. Then: *"Model parameter"*.
4. On the image record (AskUserQuestion, "Does 'only one file per instance' cover the image record?"): *"Images too"*. The option text that ARCHITECT selected: *"The same file also holds the image bytes and image index. Two regions grow independently (history at resize, images on demand), so the file grows by extents appended at the tail, and each ring spans its extents."*
5. Earlier, still in force: *"dont leave anything open. probe everything. find any bottleneck we CAN measure"*.

### Measurements (this session)

1. ARCHITECT's runs, Release: 4.743–5.263 s, 68–76 % CPU. *"consistently 3 phase: fast lines - choked - fast lines"*.
2. The five opens of each segment file, WdFilter mean (`xperf -a minifilterdelay`, elevated):

| Open | Source | WdFilter mean |
|---|---|---|
| 1 | `FileOutputStream` create (`jam_TerminalImageRecord.cpp:14`) | 38 µs |
| 2 | first reopen after close (the JUCE map, `:56`, without probes) | 4018 µs |
| 3 | JUCE map | 121 µs |
| 4 | reader view map (`jam_TerminalHistory.cpp:340`) | 738 µs |
| 5 | `DeleteFile` (`jam_TerminalSwap.cpp:220`) | 20 µs |

3. The swap moves about 143K rows/s, the same as the output rate. The row ring stays full (37 free rows), `numReadable` is 0, and the helper's write blocks.
4. Mac (`live-where-report.md`, 29-10-2026): the swap moves 4.57M lines/s against an output of 1.79M lines/s, so it never saturates.

### The code paths (read this session)

1. **Writer.** `TerminalSwap::writeFromTerminalBuffer` (`jam_TerminalSwap.cpp:78-124`) runs in `EVETerminalProcessor::handleAsyncUpdate` (`EVETerminalProcessor.cpp:331-340`, message thread) and in `prepare` through `handleUpdateNow` (`:518`, `:520`, `:547`). `prepare` runs on the caller of `VirtualDevice::start`, synchronously (`jam_VirtualDevice.cpp:37-43`).
2. **Row read.** `TerminalBuffer::read()` reads every ready slot except the `heightRows` frame slots (`jam_TerminalBuffer.cpp:99-106`). The slots become free when the handle goes out of scope (`jam_TerminalBuffer.h:313`). No partial read exists (doxygen memberdef scan of `classjam_1_1TerminalBuffer.xml`, inventory D10).
3. **Row bound.** `TerminalBuffer::getNumBytes (winsize)` sizes a frame from the columns (`jam_TerminalBuffer.cpp:456-478`). The instance `getNumBytes()` takes the widest row (`:480-488`). `writeRow` writes the atoms, the placements, the output block, the grid start and the wrap mark (`:400-416`).
4. **History reader.** `TerminalHistory` maps each index file and segment file by name and keeps the maps (`jam_TerminalHistory.cpp:319-343`). `addSegmentLines` reads an index entry (segment, offset) and the row at that offset (`:288-317`). `removeHeadMappings` drops maps of deleted groups (`:345-362`).
5. **Jobs.** The TextEditor runs one `HistoryJob` and one `LiveJob` at a time on a pool of one thread (`jam_TextEditor.h:1634`). `addJobs` launches a history job on the message thread (`jam_TextEditor.cpp:1546-1582`). `applyJobs` removes a finished job with a wait and resets it (`:1514-1544`). `addBlocks` runs on vblank and on `handleAsyncUpdate` (`:1610-1641`, `:44-47`, `:99`).
6. **Image record.** One `.images` and one `.imageIndex` file per 256 images (`jam_TerminalImageRecord.h:13-14`). `writeImages` opens a `FileOutputStream` per chunk (`jam_TerminalImageRecord.cpp:116`). `applyToImageData` maps two files per call (`jam_TerminalImageRecord.h:95-113`). Links are image records too (`jam_TerminalSwap.cpp:140-141`; `jam_TerminalHistory.cpp:15`). `removeImageGroups` removes a group when every image is released or owned by a line before the oldest line (`jam_TerminalImageRecord.cpp:172-195`).
7. **Image readers.** `PreviewComponent::RecordJob` on a process-wide pool (`jam_PreviewComponent.h:522`; `jam_PreviewComponent.cpp:216-233`). `EVETerminalProcessor::copyToClipboard` from `EVEView.cpp:155-158`. The link lookup in `TerminalHistory::addLine` (`jam_TerminalHistory.cpp:9-22`).
8. **Model.** The TextEditor's `terminal` ValueTree is the jam::Model tree of `EVETerminalProcessor::state` (`EVETerminalProcessor.cpp:22`; `jam_TextEditor.cpp:127`). A tree property change sets the parameter (`jam_Model.cpp:14-21`). The TextEditor already writes a terminal property through a `juce::Value` (`mode.referTo`, `jam_TextEditor.cpp:137`). The `model` table row of type `int64` makes a `jam::Parameter<int64_t>` (`jam_ModelLayout.cpp:24-27`).
9. **Resize.** `prepare (winsize, historyCapacity)` sizes the row ring, calls `swap.prepare (terminalBuffer.getNumBytes())` and `swap.setCapacity (historyCapacity)` (`EVETerminalProcessor.cpp:516-552`). `setCapacity` empties the history only when the capacity changes (`jam_TerminalSwap.cpp:39-48`). History must survive SIGWINCH (`PLAN-Grand-Scheme.md:561-569`).
10. **JUCE.** `MemoryMappedFile (file, range, mode)` maps a range; the start rounds down to the allocation granularity (`juce_MemoryMappedFile.h:90-93`; `juce_Files_windows.cpp:490-496`). `FileOutputStream` opens an existing file at its end (`juce_FileOutputStream.h:56-57`). `TemporaryFile` deletes its file in its destructor (`juce_TemporaryFile.h:154-160`).

## Design

### The file

- `TerminalSwapFile` owns one `juce::TemporaryFile` and its mapped extents (`jam::Owner<juce::MemoryMappedFile>`). It makes the file once, in its constructor.
- `addExtent (numBytes)` extends the file at its tail and maps the new range once (`juce::MemoryMappedFile (file, range, readWrite)`). It returns the extent number. An extent stays mapped until the swap is destroyed. No extent is mapped again and none is removed.
- `getData (extent)` returns the address of an extent. The JUCE range rule applies: the address of `range.getStart()` is `getData() + (requested start - getRange().getStart())`. `addExtent` rounds each extent start up to the allocation granularity, so the start needs no correction. Step 2 reads `SYSTEM_INFO::dwAllocationGranularity` through `juce::SystemStats` or the JUCE Windows rule cited in Context 10.
- The file never shrinks.

### Rings

`TerminalRecordRing` is one ring of variable-size records over a cycle of extents of the one file. The history lines and the image bytes each use one ring.

- A record never spans two extents. When the rest of the current extent is smaller than the record bound, the writer goes to the start of the next extent in the cycle.
- The writer drops the oldest records whose bytes it needs. It drops only records that the owner allows (see Reuse).
- `addExtent` inserts a new extent into the cycle after the extent that the writer fills. The writer goes into it next. No record moves.

### History

- **Index.** A ring of `capacity` entries, `line % capacity`. Each entry holds (extent, offset), 16 bytes, the same as `indexEntryBytes` (`jam_TerminalSwap.h:31`). `setCapacity` adds a new index extent. It empties the history, as today (`jam_TerminalSwap.cpp:46`).
- **Lines.** The history ring holds the rows that `TerminalBuffer::writeRow` writes. The record bound is `TerminalBuffer::getNumRowBytes()`. The ring keeps this invariant: the sum of (extent bytes − record bound) over its extents is at least `capacity × record bound`. Thus 10k lines always fit (ruling 1).
- **Resize.** `prepare` receives the new record bound. When the invariant fails for it, `prepare` adds one extent that restores it. The existing lines stay where they are. History survives SIGWINCH.
- **First line.** `firstLine` keeps its rule: the end line minus the capacity, and the lines that the buffer erased (`setLines`, `jam_TerminalSwap.cpp:202-211`). It also moves behind each dropped line.

### Live slots

- One live extent of `numLiveSlots × slot bytes`. `prepare` adds a new live extent only when the new slot size is larger than the current extent allows. A smaller slot uses the current extent. `prepare` drops a ready slot, as today (`jam_TerminalSwap.h:43`).

### Images

- **Bytes.** The image ring holds the image bytes. An image arrives in chunks and is read as one contiguous block (`addImageChunk`, `jam_TerminalImageRecord.cpp:134-146`; `applyToImageData`, `jam_TerminalImageRecord.h:108`). When a chunk does not fit after the earlier chunks of its image in the current extent, the writer places the image again at the start of the next extent with room, and writes its earlier chunks there first. This is creation of the stored image, not transit (MANIFESTO E, "Creation is not mutation"): the image is not readable until its end record (`jam_TerminalImageRecord.cpp:127-129`).
- **Growth.** When the image ring has no room and its oldest image is not reusable, the record adds an extent the size of the current image ring, or the size the image needs if larger.
- **Index.** A ring of image entries over extents (position, size, owner line, encoding; `imageIndexEntryBytes`, `jam_TerminalImageRecord.h:121`). When it is full and its oldest entry is not reusable, the record adds an extent of twice the current entry count, writes the live entries into it, and publishes it with an atomic store of its extent number. Readers that hold the old extent keep valid entries: the old extent stays mapped and nothing writes it again.
- **Reuse.** The rule of `removeImageGroups` stays, per image: an image is reusable when it is released, or when its owner line is before `firstLine` (`jam_TerminalImageRecord.cpp:183`).

### Reuse of history bytes (ruling 3)

- New terminal Model row `heldLine`, type `int64`, default `-1`: *"Number of the first line that the history job of the view reads; -1 when no job runs"*. The default and the sentinel follow the `clipboard` row (`parameters.md:71`, *"-1 when none"*).
- `TerminalSwap::noHeldLine { -1 }` joins the family `TerminalBuffer::noOutputBlock` and `TerminalBuffer::noGridStart`.
- **Message thread, Value.** `TextEditor::HistoryJob` holds a `juce::Value heldLine` that refers to the `heldLine` property of `terminal` (pattern: `mode.referTo`, `jam_TextEditor.cpp:137`). The job constructor sets it to `jmax (lines.getStart(), swap.getLines().getStart())`. The job destructor sets it to `TerminalSwap::noHeldLine`. `addJobs` makes the job and `applyJobs` resets it, both on the message thread (`jam_TextEditor.cpp:1578`, `:1540`). Thus the hold exists exactly while the job object exists: creation and deletion are state updates (MANIFESTO E). `blockJobs` is declared after `historyJob` (`jam_TextEditor.h:1612`, `:1634`), so the pool waits for the job before the job is destroyed.
- **Any thread, raw atomic.** `EVETerminalProcessor::writePendingData` reads `state.getParameter<jam::Parameter<int64_t>> (terminalTag, Id::heldLine)->getRawValue().load (std::memory_order_acquire)` and passes it to `swap.writeFromTerminalBuffer`. This is the pattern of `pendingClipboard` (`EVETerminalProcessor.cpp:334`).
- **Writer.** With a hold `H ≥ 0`, the writer drops no line `≥ H`, and writes no index entry of a line `≥ H + capacity`. Before it reads the rows, it computes the number of rows that fit: the bytes from its position, along the cycle, to the record of line `H`, divided by the record bound, and `H + capacity − endLine`. It reads only that many rows with `TerminalBuffer::read (maxRows)`. The other rows stay in the row ring. This is back-pressure: the TTY holds the input (`PLAN-Grand-Scheme.md:605-607`). Without a hold, it reads every ready row.
- **Reader.** `TerminalSwap::applyToLineData (line, callback)` calls the callback with a `juce::MemoryInputStream` over the record of a line only when the line is in `getLines()`. The family is `applyToLiveData` and `applyToImageData`. A history read stops at the first line that it cannot read, as today (`jam_TerminalHistory.cpp:131`).

### Names (each passes NAMES.md Rule -1)

| Name | Family / source | Intent |
|---|---|---|
| `TerminalSwapFile` | `TerminalSwap`, `TerminalImageRecord`, `TerminalFIFO` | the one file of a swap |
| `TerminalRecordRing` | "record" (`TerminalImageRecord`, swap docs); "ring" (`TerminalBuffer` brief, `TerminalFIFO` brief) | a ring of records over extents |
| `addExtent` | `add` verb (Rule 1); "extent" is the storage domain term (Rule -1 exception 1) | adds an extent to the file |
| `getData` | `juce::MemoryMappedFile::getData` | address of an extent |
| `applyToLineData` | `applyToLiveData`, `applyToImageData` (`jam_TerminalSwap.h:115`, `:129`) | reads one stored line |
| `getNumRowBytes` | `getNumBytes` (`jam_TerminalBuffer.cpp:456`, `:480`) | bound of one row |
| `read (int maxRows)` | `TerminalBuffer::read` | read handle of at most `maxRows` history rows |
| `heldLine`, `Id::heldLine` | `firstLine`, `clipboard` (`cast/terminal.md:44`, `:53`) | the first line that a history job reads |
| `noHeldLine` | `noOutputBlock`, `noGridStart` | no job holds a line |

## Language / Framework Constraints

- No comment and no doxygen before the audit (CAROL.md Code Hygiene). The doxygen pass is the last code step.
- Delete first (CAROL.md Refactor-Rewrite Discipline): every per-file API goes in the step that replaces it. The compiler lists what remains.
- Builds run only through cast, inside the scout run (profile skill, Builds).
- Every Engineer prompt restates: implement with Design by Contract, per CODING.md CRITICAL RULES; MANIFESTO E data-flow contract verbatim; doxygen-first; no comment or doxygen.

## Framework Inventory

### API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| One file deleted with its owner | JUCE | `juce::TemporaryFile`, `getFile()` | `juce_TemporaryFile.h:92-164` | 2 |
| Extend the file at its tail | JUCE | `juce::FileOutputStream`, `setPosition`, `writeByte` | `juce_FileOutputStream.h:50-116`; precedent `jam_TerminalImageRecord.cpp:14-19` | 2 |
| Map one range of the file | JUCE | `juce::MemoryMappedFile (file, range, readWrite)`, `getData`, `getRange` | `juce_MemoryMappedFile.h:90-109`; `juce_Files_windows.cpp:486-532` | 2 |
| Owning container of maps | jam | `jam::Owner<…>` | `jam_TerminalSwap.h:244` | 2 |
| Stream over mapped bytes | JUCE | `juce::MemoryOutputStream`, `juce::MemoryInputStream` | `jam_TerminalSwap.cpp:36`; `jam_TerminalSwap.h:135` | 3, 4, 5 |
| Partial row read | JUCE | `juce::AbstractFifo::read (int)` → `ScopedRead` | `classjuce_1_1AbstractFifo.xml`; `jam_TerminalBuffer.cpp:105` | 3 |
| Row bytes | jam | `TerminalBuffer::writeRow`, `getNumBytes (winsize)` | `jam_TerminalBuffer.cpp:400-478` | 3 |
| Model row from the layout | jam | `jam::ModelLayout::addParameters`, `int64` → `jam::Parameter<int64_t>` | `jam_ModelLayout.cpp:24-27` | 6 |
| Raw atomic of a parameter | jam | `jam::Parameter<int64_t>::getRawValue()` | `jam_ParameterBase.h:237` | 6 |
| Tree → parameter | jam | `Model::ParameterAdapter::setRawValue` | `jam_Model.cpp:14-21` | 6 |
| Value over a tree property | JUCE | `juce::Value::referTo`, `ValueTree::getPropertyAsValue` | `jam_TextEditor.cpp:137` | 6 |
| Measurement | scout | manifest `## run`, `## profiler`, `## toolchain` | `benchmark/seq-1m-windows-cygwin-probe.md` | 8 |

### Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Make a sized file and map it | `TerminalImageRecord::getOrCreateMapping (file, numBytes)` (`jam_TerminalImageRecord.cpp:8-61`, without probes) | `jam_TerminalSwap.cpp:33`, `:176`, `:197` | 2 |
| Streams over slots of one map | `liveOutputs` over `live` (`jam_TerminalSwap.cpp:35-36`) | `setLiveLines` `:62-70` | 4 |
| Read callback over stored bytes | `applyToLiveData` (`jam_TerminalSwap.h:128-139`) | `jam_TerminalHistory.cpp:366` | 5 |
| Atomic first/end line publish | `firstLine`, `endLine` with release/acquire (`jam_TerminalSwap.cpp:209-210`, `:233-236`) | readers via `getLines()` | 3 |
| Model value read on the writer path | `pendingClipboard` getParameter (`EVETerminalProcessor.cpp:334`) | — | 6 |
| View writes a Model property through a Value | `mode.referTo` (`jam_TextEditor.cpp:137`) | — | 6 |
| Sentinel −1 for "none" | `clipboard` row (`parameters.md:71`) | `pendingClipboard` `:125` | 6 |

### Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "Use the framework API fully" | JUCE map and stream objects; no hand-made `CreateFileMapping` | `jam_TerminalSwap.cpp:33-36` |
| "Complete at Creation" / "state update is not mutation" | The file is made once; extents are added, never mapped again | ruling 2 |
| "Published data is immutable" | A slot or record is written completely before the release store of `endLine` | `jam_TerminalSwap.cpp:105-106`, `:209` |
| "No bail-out guards" | Positive nesting; result returns | `jam_TerminalSwap.cpp:131-148` |
| "No out-parameters" | `addExtent` returns the extent number | CODING.md |
| "Deadline threads" | The clock thread does not call the writer; the writer runs on the message thread and in `prepare` | `EVETerminalProcessor.cpp:331`, `:516` |

### Absence

| Need | Query | Result |
|---|---|---|
| A ring of variable-size records over a mapped file | doxygen class briefs, `MemoryMappedFile` scan over `jam/docs/xml` (inventory E11.1) | None. `TerminalFIFO` is a byte ring with no record size and no drop (`jam_TerminalFIFO.h:192-208`, `:239`) |
| A reader-position publisher | doxygen `publish` scan (inventory E11.4) | None. `TerminalSwap::firstLine` is a plain atomic (`jam_TerminalSwap.h:255`) |
| A partial read of the row ring | doxygen memberdef scan of `classjam_1_1TerminalBuffer.xml` (inventory D10) | Only `read()` |
| A `heldLine` row or id | `parameters.md` full read; `cast/terminal.md` vocabulary (inventory A1) | None |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final code step, covering the whole sprint — never per step.

## Steps

### Step 1: Remove the probes
**Scope:**
- `eve/cygwinPTY/Source/Session.cpp`, `Session::run`: the `jam::debug::Log::Scope`, `freeSpace`, `readEnd`, their captures and assignments, and the timed `Log::write ("pty", …)` loop body. The loop keeps the code of `PLAN-cygwin-bulk-read.md` Step 2.
- `jam/jam_terminal/tty/native/jam_TerminalCygwinTTY_windows.cpp:128-133`: `debug::Log::write ("tty", …)`.
- `jam/jam_terminal/processor/jam_TerminalProcessor.cpp:74`: `debug::Log::write ("process", …)`.
- `eve/Source/EVETerminalProcessor.cpp:239`, `:333`: `"ui getter"`, `"swap start"`.
- The swap and image record probes (`jam_TerminalSwap.cpp:83`, `:122`, `:186-199`; `jam_TerminalImageRecord.cpp:10-55`) go with the code that Steps 2-5 delete.
**Validation:** read each site. The code equals the pre-probe code of the plans of this sprint.

### Step 2: `TerminalSwapFile`
**Scope:** new `jam/jam_terminal/document/jam_TerminalSwapFile.h/.cpp`; `jam_terminal.h` and the module `.cpp` include them (module header pattern).
**Action:**
1. Members: `juce::TemporaryFile file`, `jam::Owner<juce::MemoryMappedFile> extents`, the file end.
2. Constructor: makes the file (one byte, the precedent `jam_TerminalImageRecord.cpp:14-19`).
3. `int addExtent (juce::int64 numBytes)`: rounds the start up to the allocation granularity, extends the file with one `FileOutputStream` (`setPosition (end − 1)`, `writeByte (0)`), maps the range, `jassert (getData() != nullptr)`, adds it, returns its number.
4. `char* getData (int extent) const`.
5. Delete `TerminalImageRecord::getOrCreateMapping (const juce::File&, juce::int64)` with its probes.
**Validation:** no extent is mapped twice. No file other than the one `TemporaryFile`.

### Step 3: `TerminalRecordRing` and the history
**Scope:** new `jam_TerminalRecordRing.h/.cpp`; `jam_TerminalSwap.h/.cpp`; `jam_TerminalBuffer.h/.cpp`.
**Action:**
1. Delete from the swap: `folder`, `index`, `indexOutput`, `segment`, `segmentOutput`, `segmentNumber`, `firstIndexNumber`, `segmentBytes`, `setIndex`, `setSegment`, `removeHeadFiles`, `getIndexFile`, `getSegmentFile`, `getLiveFile`, `liveNumber`, the extensions, `linesPerIndex`, and the probes.
2. `TerminalRecordRing`: the cycle of extents, the write position, `addExtent` insertion after the current extent, placement of a record of a bound, and the drop of the oldest records that the owner allows (a callable that answers for the oldest record).
3. Swap: one `TerminalSwapFile`, the history ring, the index extent of `capacity × indexEntryBytes`. `setCapacity` adds a new index extent and empties the history. `prepare (int newSlotBytes, int newRowBytes)` keeps the history invariant (Design, History).
4. `writeFromTerminalBuffer (terminalBuffer, images, events, heldLine)`: computes `maxRows` (Design, Reuse), takes `terminalBuffer.read (maxRows)`, writes each row and its index entry, drops lines, moves `firstLine`. The grid and closed-line logic does not change (`jam_TerminalSwap.cpp:90-116`).
5. `TerminalBuffer::read (int maxRows)` reads `jmin (maxRows, numReady − heightRows)`. `TerminalBuffer::getNumRowBytes() const`; both it and `getNumBytes()` take the widest row of `:482-485` once.
6. `applyToLineData (line, callback)` and `noHeldLine`.
**Validation:** `jam_TerminalSwap.cpp` makes no file. 10k lines fit at the widest row. Each row and its index entry are written before the release store of `endLine`.

### Step 4: Live slots in the one file
**Scope:** `jam_TerminalSwap.h/.cpp`.
**Action:** `prepare` places the two live slots in a live extent. It adds a live extent only when the new slot size does not fit the current one. `liveOutputs`, `liveFifo`, `setLiveLines`, `applyToLiveData` keep their code over the new address.
**Validation:** no file per `prepare`. A ready slot is dropped, as today.

### Step 5: Image record in the one file; history reader
**Scope:** `jam_TerminalImageRecord.h/.cpp`; `jam_TerminalHistory.h/.cpp`.
**Action:**
1. Delete from the record: `directory`, `firstImageIndexNumber`, the extensions, `imagesPerIndex`, `getImageIndexFile`, `getImagesFile`, `getOrCreateMapping (image)`, `removeImageGroups`.
2. The record receives the `TerminalSwapFile` at each call that adds an extent (no stored pointer, CODING.md OWNERSHIP). It keeps an image ring (`TerminalRecordRing`) and the image index ring (Design, Images). `writeImages`, `addImageChunk`, `setImageOwnerLine`, `setImageEncoding`, `applyToImageData` work on the entries in the file. Reuse follows Design, Images.
3. `TerminalHistory`: delete `indexMappings`, `segmentMappings`, `getOrCreateIndexMapping`, `getOrCreateSegmentMapping`, `removeHeadMappings`, `addSegmentLines`. `addRangeLines` calls `swap.applyToLineData` for each line and stops at the first line that it cannot read.
**Validation:** no `FileOutputStream` per chunk; no map per read. The link lookup (`jam_TerminalHistory.cpp:9-22`) reads through `applyToImageData`.

### Step 6: `heldLine` Model parameter
**Scope:** `jam/cast/terminal.md` (vocabulary row); `eve/Source/layout/parameters.md` (model row); `jam_TextEditor.h/.cpp` (`HistoryJob`); `eve/Source/EVETerminalProcessor.cpp` (`writePendingData`, `prepare`).
**Action:**
1. `| @id | heldLine | `heldLine` |` after `gridStart` in `cast/terminal.md`.
2. `| heldLine | int64 | | -1 | 1 | Number of the first line that the history job of the view reads; -1 when no job runs |` in the `model` table.
3. `HistoryJob`: member `juce::Value heldLine`; the constructor refers it to `owner.terminal` `Id::heldLine` and sets `jmax (lines.getStart(), swap.getLines().getStart())`; the destructor sets `TerminalSwap::noHeldLine`.
4. `writePendingData`: reads the raw atomic with acquire and passes it.
5. `prepare`: `swap.prepare (terminalBuffer.getNumBytes(), terminalBuffer.getNumRowBytes())`.
**Validation:** the hold changes only with the job object. The writer never reads a ValueTree.

### Step 7: SPEC amendment
**Scope:** `eve/SPEC.md:377-417` and `:797-802`.
**Action:** COUNSELOR adds a `[AMENDED 2026-11-19]` block at each range. It supersedes *"deletes the index and segment files of older lines"* (`SPEC.md:400-401`) and the per-file record. Text: one file per instance, extents at the tail, the history invariant, drop oldest first, the live extent, the image ring, `heldLine`. Rulings 1-4 beside it, verbatim, and `/go` on this plan.
**Validation:** each line carries a citation or a verbatim quote.

### Step 8: Measure
**Scope:** new `eve/benchmark/seq-1m-windows-cygwin-one-file.md` (COUNSELOR writes it).
**Action:**
1. Copy `benchmark/seq-1m-windows-cygwin-probe.md`: `samples` = `seq-1m-windows-cygwin-one-file-samples.md`, `output` = `seq-1m-windows-cygwin-one-file-report.md`, `baseline` = `seq-1m-windows-cygwin-bulk-report.md`. Epochs 3. The `## toolchain` rows stay: jam, `../cygwinPTY/cast/spell.md`, EVE `--standalone-profile-symbols`.
2. @Pathfinder runs `scout D:/Poems/dev/eve/benchmark/seq-1m-windows-cygwin-one-file.md` with a 15-minute watchdog.
3. COUNSELOR reads `epoch`, `swap history lines`, `tty bytes ready` and `terminal read` against the baseline.
4. Elevated: @Pathfinder runs `wpr -start Minifilter`, one seq epoch, `wpr -stop`, and `xperf -a minifilterdelay`. COUNSELOR counts the swap file opens per epoch (target: none after the first prepare).
**Validation:** scout exit 0. The epoch shows no choked phase: the swap rate exceeds the output rate.

### Step 9: Auditor
**Scope:** every file of Steps 1-6.
**Action:** @Auditor validates against MANIFESTO.md, NAMES.md, CODING.md and this plan. COUNSELOR reads each cited file:line, names the clause, and resolves each confirmed finding in this sprint. Step 8 runs again when a fix changes code.

### Step 10: Doxygen pass
**Scope:** `TerminalSwapFile`, `TerminalRecordRing`, `TerminalSwap`, `TerminalImageRecord`, `TerminalHistory`, `TerminalBuffer::read`/`getNumRowBytes`, `TextEditor::HistoryJob`, the `Session::run` doc where Step 1 changed code.
**Action:** @Pathfinder writes the pass on COUNSELOR's exact targets (doxygen-writer skill). COUNSELOR reads each block against the code.

### Step 11: Report
**Action:** COUNSELOR reports the Step 8 values against the baseline in chat. ARCHITECT runs the interactive `seq` test.

## BLESSED Alignment

- **B:** one owner of the file (`TerminalSwapFile`); extents live until the swap ends; the hold lives exactly as long as the job object.
- **L:** the ring logic is one class used twice (history, images). Each class has one job.
- **E:** the writer receives the hold as a parameter. The reuse rules are named data, not flags.
- **S (SSOT):** the hold is one Model row. `firstLine`/`endLine` stay the one published range.
- **S (Stateless):** the writer's working state stays calculation state; the hold is Model state.
- **E (Encapsulation):** the view and the swap talk through the Model and the swap API. The swap does not know the view.
- **D:** a record is complete before the release of `endLine`. The writer reuses no byte that a running job can read.
- **0 rules:** a line is written once into the file and read in place. The one move is an image placed again during its creation.

## Risks / Open Questions

1. **The writer in `prepare`.** `prepare` runs the writer on the caller of `VirtualDevice::start` (`jam_VirtualDevice.cpp:37-43`), through `handleUpdateNow`. The TextEditor sets the hold on the message thread through the ValueTree; the parameter stores it with a relaxed store (`jam_ParameterBase.h:230-234`). When that caller is not the message thread, the acquire load does not synchronise with that store. This is a fact for ARCHITECT's ruling at approval. The plan as written uses the acquire load.
2. **Extent growth cost.** Each `addExtent` closes and reopens the file (open 2 of the table, 4.0 ms mean). It runs only at `prepare`, at `setCapacity` and at image growth, never per seq run. Step 8 action 4 counts it.
