# PLAN: Swap Risks — hold ordering, images on the record ring, bounded extents, prepare thread

**RFC:** none — objective from ARCHITECT prompt: *"fix this sprint. no open risk should be left before we wrap."*
**Date:** 2026-11-19
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8 / JAM. One object, one translation unit (CODING.md).

**Plan file at lock:** the first action after the lock copies this plan, unchanged, to `D:\Poems\dev\eve\PLAN-swap-risks.md`.

## Overview

The swap-one-file sprint left three open risks. This plan closes them in the same sprint:

1. A reader can hold a line or an image that the writer is about to overwrite.
2. The image bytes do not use `TerminalRecordRing`, against PLAN-swap-one-file "Rings".
3. The file has a fixed limit of 1024 extents. `setCapacity` leaks a new index extent and a new line ring each time it runs.

It also fixes the thread of `prepare`.

## Context — facts read this session

### Threads

1. The writer `writeFromTerminalBuffer` runs in `EVETerminalProcessor::handleAsyncUpdate` (`EVETerminalProcessor.cpp:333-341`). That runs on the serial context of the processor's `jam::AsyncUpdater`: a one-thread pool on Windows, a serial queue on macOS (`jam_AsyncUpdater.cpp:49-61`, `:42-46`). `handleUpdateNow` runs it on that context and waits for it (`:90-105`).
2. The TextEditor getter calls `triggerAsyncUpdate()` from the message thread on each vblank (`EVETerminalProcessor.cpp:237-242`).
3. The clock calls `process()`, which hands the swap to `terminalProcessor.process` (`:555-568`). `prepare()` stops the device first (`:365`). `VirtualDevice::stop` cancels the pending update and waits for the callback (`jam_VirtualDevice.cpp`; `jam_AsyncUpdater.cpp:107-119`).
4. `prepare()` has three callers: `EVEProcessor::prepareToPlay` (`EVEProcessor.cpp:102`), the `winsize` property change (`:207-208`), and `clearHistory` (`EVETerminalProcessor.cpp:570-576`). `audioDeviceAboutToStart` runs the resize on the caller's thread, inside `device.start` (`:404-418`, `jam_VirtualDevice.cpp:37-46`). The resize changes the row buffer and the swap (`:517-553`).
5. The JUCE doc of `prepareToPlay` names no thread (`juce_AudioProcessor.h:121-138`). In Standalone, `AudioProcessorPlayer::audioDeviceAboutToStart` calls it (`juce_AudioProcessorPlayer.cpp:344-373`, `:180`).
6. The history job runs on the TextEditor's pool. `HistoryJob` sets `heldLine` in its constructor and resets it in its destructor, on the message thread (`jam_TextEditor.cpp:1496-1505`, `:1556-1592`, `:1524-1554`). `setImageBounds` sets `heldImage` on the message thread (`jam_TextEditor.cpp:2101-2127`). `PreviewComponent::RecordJob` reads image bytes on its pool (`jam_PreviewComponent.cpp:216-233`).

### Memory order

1. `Parameter<int64_t>::store` writes `value` relaxed, then `needsUpdate` release (`jam_ParameterBase.h:232-233`). The other three parameter types do the same (`:178-179`, `:287-288`, `:358-359`). A tree property change reaches `store` through `ParameterAdapter::setRawValue` (`jam_Model.cpp:14-21`).
2. The writer loads both holds with acquire, once per call (`EVETerminalProcessor.cpp:327-328`). Nothing pairs that acquire with a release on `value`.
3. The writer overwrites a line record in `writeLine` (`jam_TerminalSwap.cpp:113-141`). It publishes the new `firstLine` only after that, in `setLines` (`:95`, `:165-174`). A reader checks `getLines().contains (line)` before it reads (`jam_TerminalSwap.h:149-161`). Thus a reader can pass its check and then read bytes that the writer overwrites. The same order holds for images: `getOrCreateImageExtent` clears entries and gives the extent to new bytes without a published boundary (`jam_TerminalImageRecord.cpp:124-148`).

### Images

1. The image bytes use their own fill-from-start allocator with whole-extent reuse (`jam_TerminalImageRecord.cpp:87-159`). PLAN-swap-one-file "Rings": *"The history lines and the image bytes each use one ring."* Its BLESSED Alignment: *"the ring logic is one class used twice (history, images)."*
2. One image can arrive in two parts with another image in between. The kitty transmit action returns `true` for a chunk with `m=1`, which ends the in-transit state (`jam_TerminalProcessor.cpp:1464-1476`, `:255-263`). A later kitty chunk continues the same image number (`jam_TerminalKittyDecoder.cpp:12-34`; `jam_TerminalProcessor.cpp:1349-1360`). Thus image bytes are not in number order in the ring.

### Extents

1. `TerminalSwapFile` holds at most `maxExtents` (1024) extents and never releases one (`jam_TerminalSwapFile.h:66`, `jam_TerminalSwapFile.cpp:14`).
2. Growth per source:
   - Live: `jmax (liveBytes, 2 × current)` (`jam_TerminalSwap.cpp:14-15`).
   - Lines: `jmax (missing, numRecords) × rowBytes` (`:180-183`).
   - Image index: twice the current entries (`jam_TerminalImageRecord.cpp:75-85`).
   - Image bytes: `jmax (numBytes, total, firstExtentBytes)` (`:150-155`).
   - Swap index: a new extent at each `setCapacity`. `setCapacity` also resets the line ring, `lines = TerminalRecordRing {}` (`jam_TerminalSwap.cpp:27-40`). The history capacity callback runs `clearHistory`, which reaches `setCapacity` (`EVEView.cpp:412-419`; `EVETerminalProcessor.cpp:570-576`).
3. Every source except `setCapacity` at least doubles its bytes at each new extent. `setCapacity` adds an index extent and a full line ring at each change, so the count has no bound.

## Design

### D1. Hold ordering (closes risk 1)

The rule is MANIFESTO E, *"Published data is immutable … No thread changes the slot after the publish."* A reader may read a line or image only while the writer cannot reuse its bytes. The writer and the reader each publish first, then check the other:

- **Reader:** the view stores the hold (`heldLine`, `heldImage`). The job then loads the drop boundary (`firstLine`, `firstImage`) before each read.
- **Writer:** it stores the new drop boundary first. It then loads the hold, before it overwrites any byte that the drop frees. When the hold lies in the dropped range, the writer does not overwrite. Lines stay in the row ring, which is the back-pressure of PLAN-swap-one-file "Reuse". Images go into a new extent.

Each store and each load in this pairing is `std::memory_order_seq_cst`. With one total order, either the writer sees the hold, or the reader sees the new boundary and skips the line. The reader's store reaches the job thread through the pool hand-off (`blockJobs.addJob`).

### D2. Images on `TerminalRecordRing` (closes risk 2)

- The image bytes use a second `TerminalRecordRing`, with the record bound set to the size of the image being placed.
- A chunk extends its image in place when the image ends at the ring position. Otherwise the record places the whole image again at `setRecordStart (size + chunk)` and copies the earlier bytes. This is creation, not transit, as before (PLAN-swap-one-file "Images").
- `firstImage` becomes the one drop boundary for the bytes and for the index. Before the record overwrites bytes, it finds the highest image number `k` whose bytes start in the way (`isTaken`). When every image from `firstImage` to `k` is reusable, it publishes `firstImage = k + 1`, then loads `heldImage` (D1). When an image in that range is not reusable, or the hold is at or below `k`, it adds a ring extent of `jmax (size, ring total)` and places the image there.
- The index drop in `addEntries` uses the same boundary and the same order. When the hold conflicts, the record adds an index extent instead of overwriting the oldest entry.
- Delete `imageExtents`, `imageExtent`, `imageEnd`, `getOrCreateImageExtent` and `isExtentReusable`.

### D3. Bounded extents (closes risk 3)

- `setCapacity` keeps the line ring. A capacity change empties the history by the published boundary only (`storedLine` and `firstLine` become `endLine`). The old records are reused under D1.
- `setCapacity` keeps the index extent when it holds `getNumStoredLines()` entries. Otherwise it adds one of `jmax (needed, 2 × current)` entries.
- After D2 and D3, every extent source at least doubles its bytes. The extent count is then bounded by the number of doublings up to the file size. `addExtent` keeps its `jassert`.

### D4. Thread of `prepare` (closes the thread risk)

Facts (Librarian, JUCE 8 source):

- Standalone: `startPlaying` → `AudioProcessorPlayer::setProcessor` → `prepareToPlay` (`juce_StandaloneFilterWindow.h:263-265`, `juce_AudioProcessorPlayer.cpp:152-192`, `:180`). It runs on the thread that builds the holder, which is `JUCEApplicationBase::initialise()` (`juce_StandaloneFilterWindow.h:637`). A device restart runs `audioDeviceAboutToStart` on the thread that calls `start()`, not on the audio thread (`juce_WASAPI_windows.cpp:1491-1515`; `juce_CoreAudio_mac.cpp:688-696`).
- VST3: `setActive (true)` → `prepareToPlay` (`juce_audio_plugin_client_VST3.cpp:2680-2712`, `:3774-3775`). The SDK states `setActive` is `[UI-thread & Setup Done]` (`ivstcomponent.h:194-196`).
- VST2: `effMainsChanged` → `resume()` → `prepareToPlay` (`juce_audio_plugin_client_VST2.cpp:885`, `:1481-1489`, `:571`). When a host never sends it, the first process callback calls `resume()` on the audio thread (`:392-398`).
- AU: `Initialize` → `prepareToPlay` (`juce_audio_plugin_client_AU_1.mm:221-237`). No thread statement was found.
- `juce::AsyncUpdater::triggerAsyncUpdate` may block when called from an audio thread (`juce_AsyncUpdater.h:72-75`). A hand-off from `prepareToPlay` is therefore not deadline-safe.

Design: `prepare()` has one thread, the message thread, the same thread as the getter trigger (Context, Threads 2). The owner states this invariant once, at its entry: `JUCE_ASSERT_MESSAGE_THREAD` (`juce_MessageManager.h:570-571`) at the start of `EVETerminalProcessor::prepare()` and `clearHistory()`. MANIFESTO D: *"establishing the invariant once, at the owner"*. MANIFESTO E: *"Prefer assert over silent fail"*. On the message thread, the getter cannot trigger the writer during `prepare`, and the stopped device cannot trigger it (Context, Threads 3). `handleUpdateNow` orders the writer's last run before the resize (`jam_AsyncUpdater.cpp:90-105`). A host that calls `prepareToPlay` on another thread (the VST2 fallback) fails the assert in a Debug build.

## Language / Framework Constraints

- No comment and no doxygen before the audit (CAROL.md Code Hygiene). The doxygen pass is a late step.
- Delete first (CAROL.md Refactor-Rewrite Discipline).
- Builds run only through cast, inside the scout run (profile skill, Builds).
- Each Engineer prompt restates: Design by Contract per CODING.md CRITICAL RULES; MANIFESTO E data-flow contract verbatim; doxygen-first; no comment or doxygen.

## Framework Inventory

### API

| Need | Module | API | Source | Step |
|---|---|---|---|---|
| Ordered store of a Model value | jam | `Parameter<…>::store` | `jam_ParameterBase.h:178`, `:232`, `:287`, `:358` | 1 |
| Raw atomic of a hold | jam | `Parameter<int64_t>::getRawValue()` | `jam_ParameterBase.h:237` | 2 |
| Partial row read | jam | `TerminalBuffer::read (int maxRows)` | `jam_TerminalBuffer.cpp:100-107` | 2 |
| Ring of records over extents | jam | `TerminalRecordRing` `addExtent`, `setRecordStart`, `isTaken`, `getNumRecords`, `getExtent`, `getPosition`, `addRecord` | `jam_TerminalRecordRing.h:20-87` | 3, 4 |
| Extent of the file | jam | `TerminalSwapFile::addExtent`, `getData`, `getNumBytes` | `jam_TerminalSwapFile.h:26-50` | 3, 4 |

### Pattern usage

| Pattern | Exemplar | Use sites | Step |
|---|---|---|---|
| Ring placement, drop, write, advance | `TerminalSwap::writeLine` | `jam_TerminalSwap.cpp:113-141` | 2, 3 |
| Geometric extent growth | `addLineExtent` | `jam_TerminalSwap.cpp:176-185` | 3, 4 |
| View writes a hold through a Value | `HistoryJob` ctor/dtor; `setImageBounds` | `jam_TextEditor.cpp:1496-1505`, `:2125` | 2, 3 |
| Writer reads a hold from the raw atomic | `writePendingData` | `EVETerminalProcessor.cpp:322-331` | 2, 3 |

### Names

The changed signatures keep their names: `writeFromTerminalBuffer`, `writeImages`, `addEntries`, `addImageChunk`, `isReusable`, `applyToLineData`, `applyToImageData`, `setCapacity`.

The plan added these names (record corrected by `PLAN-terminal-workers-told.md`, audit finding F5):

| Name | Family | Site |
|---|---|---|
| `setStoredLine` | `set…`: replaces a variable in place | `jam_TerminalSwap.h` |
| `writeNextLine` | `write…` returning bool, as `writeFromTerminalBuffer` | `jam_TerminalSwap.h` |
| `addImageExtent` | `add…Extent`, as `addLineExtent`, `addIndexExtent` | `jam_TerminalImageRecord.h` |
| `getLastTakenImage` | `get…` | `jam_TerminalImageRecord.h` |
| `isRangeReusable` | `is…`, as `isReusable` | `jam_TerminalImageRecord.h` |
| `setImageStart` | `set…Start`, as `TerminalRecordRing::setRecordStart` | `jam_TerminalImageRecord.h` |
| `imageData` | the data noun of the image bytes, as `lines` | `jam_TerminalImageRecord.h` |
| `setNextExtentStart` | `set…Start`, as `setRecordStart` | `jam_TerminalRecordRing.h` |
| `isAppendable` | `is…` (local lambda) | `jam_TerminalImageRecord.cpp` |

### Synthesis

| Rule | Established form | Exemplar |
|---|---|---|
| "No bail-out guards" | Loop conditions and positive nesting; result returns | `jam_TerminalSwap.cpp:119-120`, `:115-140` |
| "Use the framework API fully" | One ring class for every record store in the file | `jam_TerminalRecordRing.h` |
| "Published data is immutable" | Drop boundary published before a byte is reused | D1 |

### Absence

| Need | Query | Result |
|---|---|---|
| A sequentially consistent store on `jam::Parameter` | `jam_ParameterBase.h` memory-order read (`:165-372`) | None. Every `value.store` is relaxed |
| A stable-address container that grows | Pathfinder F (this session): `SegmentedVector` block table reallocates (`jam_HashMap.h:1136-1141`); `Owner`, `OwnedArray` move their pointer arrays | None. The fixed `std::array` of `TerminalSwapFile` stays |

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED), NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor runs ONCE, after the final code step, covering the whole sprint — never per step.

## Steps

### Step 1: Ordered parameter store
**Scope:** `jam/jam_data_structures/model/jam_ParameterBase.h`.
**Action:** In each `store`, write `value` with `std::memory_order_seq_cst` (`:178`, `:232`, `:287`, `:358`). The `needsUpdate` store does not change.
**Inventory:** API row 1.
**Validation:** four stores changed, no other line.

### Step 2: Line hold ordering
**Scope:** `jam_TerminalSwap.h/.cpp`; `EVETerminalProcessor.cpp` (`writePendingData`).
**Action:**
1. `writeFromTerminalBuffer` takes `const std::atomic<int64_t>& heldLine` and `const std::atomic<int64_t>& heldImage`. `writePendingData` passes `getRawValue()` of each parameter.
2. Delete `getNumFreeLines`. The writer takes one row at a time with `terminalBuffer.read (1)` while a row is ready and the next drop is allowed.
3. For each row: compute the lines that the new record drops (`storedLine` advance, as `writeLine` does now); store `firstLine` with seq_cst; load `heldLine` with seq_cst. When the hold is at or above the new `storedLine`, take the row and write it. Otherwise end the loop with the row still in the ring.
4. `applyToLineData` loads `firstLine` with seq_cst for its check.
**Inventory:** API rows 2-3; pattern rows 1, 3, 4.
**Validation:** no byte of a line is written before `firstLine` passes it. No hold load comes before that store.

### Step 3: Images on the record ring
**Scope:** `jam_TerminalImageRecord.h/.cpp`; `jam_TerminalSwap.cpp` (the call of `writeImages`).
**Action:** D2, with D1 order on `firstImage` and `heldImage`. Delete first: `imageExtents`, `imageExtent`, `imageEnd`, `getOrCreateImageExtent`, `isExtentReusable`. `applyToImageData` loads `firstImage` with seq_cst.
**Inventory:** API rows 4-5; pattern rows 1-2.
**Validation:** no image byte or entry is overwritten before `firstImage` passes it and `heldImage` is loaded. No second allocator remains.

### Step 4: Bounded extents
**Scope:** `jam_TerminalSwap.cpp` (`setCapacity`).
**Action:** D3.
**Inventory:** API row 5; pattern row 2.
**Validation:** each `addExtent` call site grows by at least 2×. A repeated `setCapacity` with the same or a smaller entry count adds no extent.

### Step 5: Thread of `prepare`
**Scope:** `eve/Source/EVETerminalProcessor.cpp` (`prepare()`, `clearHistory()`).
**Action:** `JUCE_ASSERT_MESSAGE_THREAD` as the first statement of each (D4).
**Inventory:** JUCE `JUCE_ASSERT_MESSAGE_THREAD` (`juce_MessageManager.h:570-571`); exemplar `assertHostMessageThread` (`juce_audio_plugin_client_VST3.cpp:332-339`).
**Validation:** two lines added, nothing else.

### Step 6: SPEC amendment
**Scope:** `eve/SPEC.md` [AMENDED 2026-11-19] block.
**Action:** add D1-D4, with ARCHITECT's words: *"fix this sprint. no open risk should be left before we wrap."*

### Step 7: Measure
**Action:** scout `benchmark/seq-1m-windows-cygwin-one-file.md` again (its `## toolchain` rows build jam, the helper and EVE). Watchdog 15 min. Compare epoch and swap history lines with the last report.
**Validation:** exit 0, 0 dropped, epoch within the last spread.

### Step 8: Auditor
Every file of Steps 1-5. COUNSELOR validates each finding (Design by Contract Filter).

### Step 9: Doxygen pass
@Pathfinder updates the blocks of every changed symbol. COUNSELOR reads each block against the code.

### Step 10: Report
COUNSELOR reports Step 7 values. ARCHITECT runs the interactive tests.

## BLESSED Alignment

- **B:** each byte has one owner at a time: the writer until the publish, the readers after it.
- **S (SSOT):** one drop boundary per store (`firstLine`, `firstImage`). One ring class.
- **E:** the holds reach the writer as explicit parameters.
- **D:** the seq_cst pairing removes the read-after-reuse race.

## Risks / Open Questions

None. The VST2 fallback (`prepareToPlay` on the audio thread) is a host fault. Before this plan it already ran the whole `prepare`, with the TTY open and allocations, on that thread. D4 makes the fault fail loudly. It does not hide it.
