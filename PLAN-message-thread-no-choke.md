# PLAN: message-thread-no-choke

**RFC:** none. The objective comes from ARCHITECT's prompt.
**Date:** 2026-11-17
**Language Constraints:** C++17 / JUCE 8 / JAM (CODING.md, MANIFESTO.md, NAMES.md).

## Overview

ARCHITECT, verbatim:
- *"NON NEGOTIABLE message thread should never blocked while keeping the adherence to LOCK-FREE architecture especially on EXTREME THROUGHPUT like seq test"*
- *"0 rules. non-negotiable. NO COPY, NO ALLOC, NO TEMP CONTAINERS, NO SHADOW STATE, only exception copy is cross thread."*
- *"what are you waiting for? how do we fix this?"*, then `/go` on COUNSELOR's fix list.

Rulings that hold:
- **M1** *"Message thread (Recommended)"*: the message thread lays out each finished block once (`PLAN-message-thread-materialise.md:21`). Layout stays on the message thread.
- **H3** *"Batch blocks"*: one history job at a time, keyed by its first line (`PLAN-history-batch-blocks.md:21`).
- The `removeJob` of a finished job is the one accepted wait (`PLAN-message-thread-materialise.md:65`).

## Evidence

whatdbg sample, message thread, 59 samples, `seq 1 3000000` (scratchpad `seq-server-where-sample.out`). The message thread is in `TextEditor::addBlocks` in 100% of the samples:
- `setImageBounds` takes 52.5%;
- `applyToVisibleLines` takes 45.8%;
- `getRowNumberAt` takes 28.8%;
- `setBlock` takes 25.4%;
- `visibleAreaChanged` and `setBounds` take 20.3%.

Code facts:
- `history` is `jam::HashMap<juce::int64, TextBlock>` (`jam_TextEditor.h:1622`). It iterates in storage order, and erase moves the last element into the hole (`jam_HashMap.h:1416-1420`, `:2429`). Thus no walk can stop early by key.
- Each pass of `addBlocks` (`jam_TextEditor.cpp:1606-1637`) runs `setContentSize`, `setImageBounds` and `setPreviewBounds`. `setViewToEnd` then moves the viewport, so `visibleAreaChanged` runs `setImageBounds` and `setPreviewBounds` again (`:1901-1913`).
- `applyToVisibleLines` (`jam_TextEditor.h:877-922`) does these:
  1. a `min_element` over `history`;
  2. one hash lookup per block;
  3. `getRowRange` on every block, which makes two `getRowNumberAt` calls per block (`jam_DocumentIndex.cpp:320-328`);
  4. two more O(blocks) walks for the live block (`getNumHistoryPixels`, `getNumHistoryLines`).
- `setImageBounds` runs that walk whether or not any image exists (`jam_TextEditor.cpp:2082-2105`).
- `getBlockNumber` calls `getBlockStart` inside a `find_if`, so it is O(blocks²) (`:1841-1858`, `:1881-1889`).
- A pass runs on each VBlank (`:99`) and on each finished job, through `setFinished` → `triggerAsyncUpdate` (`:1470-1474`, `:44-47`).

Server write path:
- `setPendingWrite` reads the payload into `sequence` when the request arrives (`jam_TerminalConsole_windows.cpp:582-586`, `jam_TerminalConsoleOutput_windows.cpp:625-667`).
- `read` then copies `sequence` into the FIFO free space (`jam_TerminalConsoleOutput_windows.cpp:84-97`).
- Thus the payload is copied twice. The first copy is the OS boundary (MANIFESTO permitted copy 1). The second copy is forbidden: *"A second copy of the same complete data"*.
- `TerminalFIFO::writeFromTerminalTTY` gives the TTY the whole free space of the ring as the read target (`jam_TerminalFIFO.h`, `writeFromTerminalTTY`).
- `read` already completes a pending write when `render` finishes it (`jam_TerminalConsole_windows.cpp:614-615`).

## Framework Inventory

| Need | API / pattern | Cite |
|---|---|---|
| Ordered history | `std::map` (jam has no ordered map; `std::map` is already used in jam) | `jam_Listener.h:254` |
| Key test on a `std::map` | `jam::Map::contains` | `jam_Bimap.h:70-74` |
| Visible rows of a block | `Document::Index::getRowRange`, `getLineRange`, `getNumPixels`, `getNumLines` | `jam_DocumentIndex.cpp:227-339` |
| Walk lambda | `applyToBlock` | `jam_TextEditor.h:880` |
| Read target | `TerminalFIFO::writeFromTerminalTTY` gives the whole free space | `jam_TerminalFIFO.h` |
| UTF-8 writer | `juce::CharPointer_UTF8::write`, `getBytesRequiredFor` | `juce_CharPointer_UTF8.h:220`, `:261` |
| Byte bounds | `maxCharacterBytes`, `lineEndEcho`, `sanitizeCharacter` | `jam_TerminalConsoleOutput_windows.cpp:7`, `:17`, `:39-47` |
| Readiness | `TerminalConsoleOutput::getNumReady (request)`, the existing name with a new parameter | `jam_TerminalConsoleOutput_windows.cpp:109-112` |

## Steps

### Step 1: Ordered history, one walk per question
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.{h,cpp}` only.

1. `history` becomes `std::map<juce::int64, TextBlock>`. It is the same key, ordered by key. jam has no ordered map (Pathfinder found none), and `std::map` is already used at `jam_Listener.h:254`. The calls become:
   - `addOrReplace` → `insert_or_assign`;
   - `contains (k)` → `history.find (k) != history.end()` (the language is C++17);
   - `at`, `erase (iterator)` and `begin`/`end` stay.
2. In `applyToVisibleLines`:
   1. Delete the `min_element` and the `history.contains` / `at` chain.
   2. Walk `history` in order, and add up `blockTop` and `firstLine`.
   3. Call `applyToBlock` only when `juce::Range<float>::withStartAndLength (blockTop, block.index->getNumPixels())` intersects `visibleRange`.
   4. The live block gets the totals of that walk. Do not call `getNumHistoryPixels()` and `getNumHistoryLines()` there.
3. `addBlock` (`:1382-1403`): `isTail` is `history.empty()`, or `blockNumber` is greater than the key of the last entry (`*history.rbegin()`, structured binding).
4. `getHistoryEnd` (`:1584-1592`): use the last entry's key plus its `Id::size`, with `swap.getClosedLines().getStart()` as the floor. Blocks are contiguous and ordered.
5. `removeHeadBlocks` (`:1405-1435`): erase from `begin()` while the head entry meets the condition. Stop at the first entry that stays.
6. `getBlockNumber`, `getBlockNumberAt`, `getBlockStart` and `getBlockTop` (`:1841-1899`): each does one ordered walk, adds up lines or pixels, and returns when it finds the answer (result return).
7. The doxygen at `jam_TextEditor.h:1622` is not edited (Code Hygiene).

**Validation:**
- No walk calls `getRowRange` on a block outside the view.
- `getBlockNumber` is one walk.
- Each `history` call compiles under C++17.

### Step 2: The payload goes straight into the FIFO
**Scope:** `jam/jam_terminal/tty/native/jam_TerminalConsole_windows.cpp`, `jam_TerminalConsoleOutput_windows.{h,cpp}` only.

1. `TerminalConsole::setPendingWrite` (`:582-586`) calls `output.setPendingWrite (getRequest())`, then returns `output.render (…) ? setWritten() : nullptr`. `render` renders only output rows. For a payload write, it reads no payload and returns `writeOffset == writeSize`. Thus a zero-length write completes at once, and any other write stays pending.
2. `reader.onEvent` (`:259-263`) counts the pending payload bytes as ready: `output.getNumReady()` plus the pending payload of a write request. Thus the reader ticks in the same pass, and the tick calls `read`.
3. `TerminalConsoleOutput::read (char* destination, int maxBytes)`:
   1. It first copies the generated VT bytes of `sequence`, as it does now. These come from the screen-API rows. They are not a client payload.
   2. Then, when a payload write is pending and `sequence` is empty, it reads the payload with `readInputPayload` straight into `destination + numRead`.
   3. `renderBytes` and `renderUnicode` take `char* destination` and `int maxBytes`, and return the bytes written.
      - Bytes: read at most half the room when `isAutoReturn` is set, then add the CRs in place in `destination`. `addAutoReturns` takes the destination and the count, and returns the new count.
      - Unicode: read the UTF-16 units into `units` (the OS boundary). Write the UTF-8 into `destination` with `juce::CharPointer_UTF8::write`, after the room check `juce::CharPointer_UTF8::getBytesRequiredFor`.
4. `TerminalConsole::read` (`:610-619`) completes the write when `output` reports the payload done, as `:614-615` does now.
5. Delete the request-time render of payload writes. The screen-API rows (`isOutputRow`) keep `render` into `sequence`.

**Validation:**
- Each payload byte is copied once: the client's buffer → the FIFO ring, through `READ_INPUT`.
- No wait is added.
- The request completes in the same reader pass.

### Step 3: Build
Pathfinder builds through cast: `cast cast/spell.md --debug` in `D:\Poems\dev\eve`.

### Step 4: Audit
@Auditor, once, across every changed file.

### Step 5: Doxygen pass (Pathfinder, after the audit)
It covers:
- the declarations changed in Steps 1-2;
- `jam_TextEditor.h:1622`, `:866-876`;
- `jam_TerminalConsoleOutput_windows.h` for `read`, `renderBytes`, `renderUnicode` and `addAutoReturns`.

### Step 6: ARCHITECT tests
On Windows, ARCHITECT runs `time seq 1 1000000`. He checks that the message thread does not freeze, and that the prompt does not come before the history ends.

## Risks / Open Questions
- The generated VT bytes of the screen-API rows still go `sequence` → FIFO (Step 2.3.1). They are bytes that the server creates, not client data in transit. `seq` does not make them.
