# PLAN: Preview through the Terminal Protocol

**RFC:** none — objective from ARCHITECT prompt
**Date:** 2026-11-05
**BLESSED Compliance:** verified
**Language Constraints:** C++17 / JUCE 8.0.14 / JAM (LANGUAGE.md, C++ / JUCE)

**Contract for each step (binds this plan):** ARCHITECT builds and tests: *"you dont build. architect build and test runtime"*. *"no new semantics, no magic strings allowed anywhere, no handroll state management, no manual arithmetic, no new pattern, use framework API to its fullest extent, follow established pattern verbatim"*. *"framework API(s) are rich jam first then juce. you probably only need to use it OOTB. if there's genuienely no abstract as SSOT you can use, extend the existing."* Data in transit has *"0 allocation, 0 copy, 0 mutation, 0 temporary container, 0 new struct, and 0 shadow state"* (`MANIFESTO.md:267`). *"we only use juce::String as Input or final Output materialisation to communicate with juce's API. otherwise we always use std::string/string_view everywhere."*

## Overview

An image that any CLI sends through iTerm2, Kitty or Sixel shows on a `juce::Component` at the exact
cell box that the CLI expects. The TextEditor only reserves the cells. The tick reads metadata only.
The widget reads, decodes and materialises the image off the terminal path. The target is parity
with kitty `imgcat` and fzf with chafa.

ARCHITECT: *"we have never successfully implement the correct ISOMORPHIC ABSTRACTION to layout back
to terminal cell"*; *"TextEditor only reserve the layout"*; *"we always process/read/write/materialise/render
image on message thread, LITERALLY"*; *"I want to enforce the architecture fully widget wise on
juce::Component native contract. no poking layers. TerminalGraphics lane are final alternatives only
when it was proven to be only way"*; *"this also open seam for our SKiT API to access via shell
script. so it's not hardcoded to anything, but universal and generic"*.

## The isomorphism, stated once

A placement is a cell box on the terminal grid. The grid rows already exist.

1. **The tick** writes `{cell box, image number}` on each row of the box (`jam_TerminalBuffer.cpp:179-198`).
2. **The swap** gives each line its `Id::image` elements, on history and live lines through one
   `addLine` (`jam_TerminalSwap.cpp:199-224`, `:226-253`).
3. **The TextEditor** owns the map from cell to pixel. `getCellBounds` gives the box position.
   `Cell::Rectangle::toPixel` gives the box size, with the mono metrics (`jam_TextBlock.cpp:213`). A box
   can span the history and the live block, so its last line can be in another block. No arithmetic.
4. **The Model** holds the visible boxes as a tree. The TextEditor writes it.
5. **`jam::PreviewOverlay`** is a sibling of the TextEditor. `applyComponentBounds<MarkdownDocument>`
   gives every view child the same bounds (`jam_ViewManager.cpp:52-58`), so a box in TextEditor
   coordinates is a box in overlay coordinates, under zoom too.

The defect today: `TextBlock::setImageRows` raises the first row to the full image height
(`jam_TextBlock.cpp:204-222`), while the covered rows keep their cell height
(`jam_TerminalSwap.cpp:250-251`, `jam_TextBlock.cpp:224-228`). An N-row box takes its height plus N-1
rows, so the view leaves the cell grid. Step 1 deletes it. The rows are the reservation.

## Language / Framework Constraints

- One object, one translation unit. `jam::PreviewOverlay` is one header and one `.cpp`.
- MANIFESTO E: *"If the architecture uses listeners, use listeners"*. A manual callback slot is
  forbidden. `a11db9c15` removed `onImages`: *"manual callback slots and consumer-bound values are
  removed"*. Per-instance data arrives through the bind lanes (`fa127c3`).
- Comments and doxygen are the post-audit pass only.

## Dependency & API Inventory

**JUCE:** `juce::ValueTree` (`getChildWithName`, `appendChild`, `removeChild`, `copyPropertiesFrom`,
which sets each property, and `setProperty` sends no event for an unchanged value:
`juce_ValueTree.cpp:201-209`, `:144-145`). `juce::ValueTree::Listener` (`valueTreeChildAdded`,
`valueTreeChildRemoved`, `valueTreePropertyChanged`). `juce::ThreadPool`, `juce::ThreadPoolJob`
(default constructor `juce_ThreadPool.h:232`). `juce::AsyncUpdater`. `juce::RectanglePlacement`.
`juce::Component::getLocalArea`. `juce::Rectangle::getUnion`, `getIntersection`.

**JAM:** `jam::SharedInstance` (`jam_SharedInstance.h:28-59`), `jam::Owner` (`add` `:94`, `remove`
`:279`), `TextEditor::getCellBounds` (`jam_TextEditor.cpp:1135-1163`), `Document::Index::getRowRange`,
`getLineRange`, `TerminalSwap::getImageData`, `getImageEncoding`, `Registry::registerComponent`
capability dispatch for `bindToProcessor` and `bindToModel` (`jam_Registry.h:151-156`,
`jam_ViewManager.cpp:240-243`), `AudioModel::getOrCreateComponentState`
(`jam_AudioModel.cpp:514-530`), `Model::storeValue`, `loadImageSequenceNative`,
`TerminalSixelDecoder::decode`.

**Patterns, verbatim:**
- Model row written on the serial context, read as a tree event: `clipboard`
  (`EVETerminalProcessor.cpp:207-216`).
- View component row: `Source/layout/component.md:8`.
- Getter by component type: `EVETerminalProcessor.cpp:166`.
- `bindToModel` binding a state tree: `jam_TextEditor.cpp:82-95`.
- The removed overlay, as a reference for the box walk only: `2b20e4f61:jam_TextEditor.cpp:1482-1523`.

**Lexicon:** `Id::image`, `Id::images`, `Id::line`, `Id::left`, `Id::width`, `Id::height`, `Id::x`,
`Id::y` exist (`jam/cast/core.md:95-162`, `graphics.md:46-72`). New rows: EVE `previewOverlay`,
jam `numImages`. New names, each in a family: `getImageBounds`, `setImageBounds` (the removed pair),
`getNumImages` (`getNumLines`), `PreviewOverlay` (`MessageOverlay`), `DecodeJob` (`LaneJob`,
`decodeJobs`), `addPreview`, `applyImage` (removed `applyImage`), `isEmpty`.

## Validation Gate

Each step is validated by COUNSELOR before proceeding to the next — against MANIFESTO.md (BLESSED),
NAMES.md, ~/.carol/CODING.md, and the locked PLAN decisions (no deviation, no scope drift). @Auditor
runs ONCE, after the final step, covering the whole sprint — never per step.

## Steps

### Step 0: Record the rulings
**Scope:** `PLAN-Grand-Scheme.md`, `SPEC.md`
**Action:** COUNSELOR writes one amendment dated 2026-11-05 in grand scheme §3.11. It holds this
session's rulings verbatim and the order of these six plans. `SPEC.md:356-360` takes the new ruling
in place of *"NO PREVIEW COMPONENT"*. SPEC principle 12 gains the widget-only rule.
**Validation:** Each decision line quotes ARCHITECT.

### Step 1: The rows are the reservation
**Scope:** `jam/jam_gui/widgets/jam_TextBlock.h`, `.cpp`
**Action:** Delete `setImageRows` (`.cpp:204-222`, `.h:115-126`), and its two calls (`.cpp:144`, `:161`).
**Validation:** The live grid and the history rows keep the cell height, so one terminal row is one
view row. A read of `addRowHeights` confirms the row height. No other caller exists
(`jam_TextBlock.cpp:144`, `:161` are the two read sites).

### Step 2: The TextEditor tells the boxes through the Model
**Scope:** `jam/jam_gui/widgets/jam_TextEditor.h`, `.cpp`
**Action:**
1. Add a private member `juce::ValueTree images { Id::images };` beside `terminal`. The tree is valid
   before the bind: `bindToProcessor` runs `addBlocks()` before `bindToModel`
   (`jam_ViewManager.cpp:240-243`).
2. In `bindToModel`, after `terminal = ...`:
   ```cpp
   images = model.getOrCreateComponentState (Format::toTag (Id::images));
   images.removeAllChildren (nullptr);
   ```
   The boxes are view state of one run, so a restored blob holds no box.
3. Add a private `juce::Rectangle<int> getImageBox (const TextBlock& block, float blockTop, const Document::Element& image) const;`:
   ```cpp
   const auto metrics { getStyle().getMonoMetrics() };
   const auto size { jam::Cell::Rectangle { jam::Cell { 0 }, jam::Cell { 0 }, jam::Cell { *image.get<int> (Id::width) }, jam::Cell { *image.get<int> (Id::height) } }.toPixel (metrics.cellWidth, metrics.cellHeight) };
   const auto origin { getCellBounds (block, *image.get<int> (Id::line), *image.get<int> (Id::left)).getPosition() };

   return getLocalArea (&content, size.withPosition (origin).translated (0.0f, blockTop + getRowOffset()).getSmallestIntegerContainer());
   ```
4. Add a private template `applyToVisibleImages (Callback&& callback) const`, defined in the header. It
   walks the visible lines of the live block and of each resident block, and calls
   `callback (block, blockTop, image)` for each image of those lines. The callback returns true to end
   the walk. The walk returns true when a callback ended it. The visible range is the shape of the old
   `addImageBounds`: `block.index->getRowRange (viewport.getViewArea().toFloat().getVerticalRange() - blockTop)`,
   then `getLineRange`. No container: the elements stay in their documents.
5. Add a private `void setImageBounds();`:
   ```cpp
   applyToVisibleImages ([this] (const TextBlock& block, float blockTop, const Document::Element& image)
   {
       const auto number { *image.get<juce::int64> (Id::image) };
       const auto bounds { getImageBox (block, blockTop, image) };
       auto box { images.getChildWithProperty (Id::image, number) };

       if (not box.isValid())
       {
           box = juce::ValueTree { Id::image, { { Id::image, number } } };
           images.appendChild (box, nullptr);
       }

       box.setProperty (Id::x, bounds.getX(), nullptr);
       box.setProperty (Id::y, bounds.getY(), nullptr);
       box.setProperty (Id::width, bounds.getWidth(), nullptr);
       box.setProperty (Id::height, bounds.getHeight(), nullptr);

       return false;
   });

   for (auto index { images.getNumChildren() }; --index >= 0;)
   {
       const auto number { images.getChild (index).getProperty (Id::image) };

       if (not applyToVisibleImages ([&number] (const TextBlock&, float, const Document::Element& image) { return number == juce::var { *image.get<juce::int64> (Id::image) }; }))
           images.removeChild (index, nullptr);
   }
   ```
   The key is the record number, through JUCE's `getChildWithProperty`. A line that moves from the live
   block to the history keeps its number, so its Preview stays and decodes once.
   A covered line lists its image too, so a box whose first row left the view stays. `Id::line` is a
   line of the block (`jam_TerminalSwap.cpp:246`).
6. Call `setImageBounds()` in `addBlocks()` inside `if (isUpdated or isRemoved)`, at the end of
   `visibleAreaChanged`, and at the end of `setBlocks`.
**Validation:** `getCellBounds` stays the one cell map. The TextEditor holds no image data
(`jam_TextEditor.h:94`). No temporary container (`MANIFESTO.md:263`). A `juce::var` of an int does not
allocate, and an unchanged value sends no event. A new box is a state update. No callback slot.

### Step 3: The record tells its image count
**Scope:** `jam/jam_terminal/document/jam_TerminalImageRecord.h`, `.cpp`, `jam_TerminalSwap.h`, `.cpp`,
`eve/Source/layout/parameters.md`, `eve/Source/EVETerminalProcessor.cpp`, `eve/cast/identifiers.md`
**Action:**
1. Add `juce::int64 getNumImages() const noexcept;` to the record, which returns
   `endImage.load (std::memory_order_acquire)`, and the forwarder on the swap.
2. Replace `getImageData` and `getImageEncoding` with one template on the record and its forwarder
   on the swap:
   ```cpp
   template <typename Callback>
   void applyToImageData (juce::int64 image, Callback&& callback) const;
   ```
   It maps the index and the images file as `getImageData` does (`jam_TerminalImageRecord.cpp:153-175`),
   and calls `callback (data, size, encoding)` with the pointer into the mapping. It copies nothing.
   The family: `applyPendingData`, `applyReleasedImages`, `applyToCodepoints`.
3. The two callers read in place. `TerminalSwap::addLine` passes the view to
   `AnsiDocument::getLink` (`jam_TerminalSwap.cpp:209-210`). `copyToClipboard`
   (`EVETerminalProcessor.cpp:218-224`) builds its `juce::String` with `fromUTF8` at the JUCE call.
4. Add the `## model` row `numImages | int64 | | 0 | Number of the images that the record holds`.
5. In `handleAsyncUpdate`, after `writePendingData()`:
   `state.storeValue (jam::Format::toType (Id::terminal), Id::numImages, static_cast<int64_t> (swap.getNumImages()));`
6. Add the row `numImages` to `jam/cast/terminal.md`. `jam::PreviewOverlay` reads it, and `Id::numPrompts` is the precedent.
**Validation:** One writer: the serial context. The tick does not change. The next flush gives the
tree event. This replaces the vblank retry with an event (ARCHITECT: *"everything is EVENT DRIVEN"*).
`MANIFESTO.md:260`: *"read the data in its location"*. A mapped reader survives the deletion of its
file on the OS, as `getImageData` does today.

### Step 4: Preview decodes in parallel and materialises on the message thread
**Scope:** `jam/jam_gui/widgets/jam_Preview.h`, `.cpp`
**Action:**
1. The class bases gain `private juce::AsyncUpdater`.
2. `explicit Preview (juce::RectanglePlacement placementToUse);` stores `const juce::RectanglePlacement placement;`.
   `paint` uses it in place of `juce::RectanglePlacement::centred`.
3. Add a nested base `struct DecodeJob : public juce::ThreadPoolJob` with the members `Preview& owner`,
   `jam::Array<juce::Image> frames`, `juce::HeapBlock<int> delays`, and
   `void decode (const void* data, size_t size, TerminalImageEncoding encoding);`. That function runs
   the existing `getSequence` and `getFrames`, then `owner.triggerAsyncUpdate()`.
4. Two jobs, each with its source, read in place:
   - `RecordJob` holds `const TerminalSwap& swap` and `juce::int64 image`. `runJob` calls
     `swap.applyToImageData (image, ...)` with `decode`.
   - `FileJob` holds `juce::File file`. `runJob` builds a `juce::MemoryMappedFile` of the file and
     calls `decode` with its data and size.
5. `getFrame` and `getRawFrame` build `juce::Image { juce::Image::ARGB, width, height, false, juce::SoftwareImageType {} }`.
   A software image has no Vulkan contract, so a job can build it. The paint uploads it on the message
   thread through the LLGC, as any image.
6. `getSequence` keeps the sixel arm and the `isGif` gate. Only a GIF goes to
   `loadImageSequenceNative`. ARCHITECT: *"can you ensure we're using the fastest path, IIRC jam have its
   own image loader faster than juce"*. A PNG goes to `ImageLoader::getFromMemory`, which runs the SIMD
   `decodePng`. Any other still goes to `loadImageNative`. Both build a software image, in one buffer.
7. Public `void setImage (const TerminalSwap& swap, juce::int64 image);` and
   `void setImageFile (const juce::File& file);` remove a running job, make the new job, and call
   `decodeJobs->addJob (job.get(), false)`. Delete `setImageData`.
8. `handleAsyncUpdate` runs on the message thread. It moves the frames and the delays from the job,
   resets the job, starts the timer for more than one frame, and repaints.
9. Add `bool isEmpty() const noexcept;`, true when `frames` is empty and no job exists.
10. Members: `jam::SharedInstance<juce::ThreadPool> decodeJobs { std::in_place };`,
    `std::unique_ptr<DecodeJob> job;`.
11. `~Preview()` calls `decodeJobs->removeJob (job.get(), true, timeout)` when a job exists, with a
    named constant for the timeout. JUCE requires this before the job is deleted.
**Validation:** ARCHITECT: *"could be done in parallel"*; `9a8903ee0` removed the pool from the
TextEditor, and this pool is the widget's. No byte is copied before the decode. The decode is
creation (`MANIFESTO.md:254`). `TerminalSwap` outlives the view, because the processor owns it
(`SPEC.md:385`). `SharedInstance` makes the pool live only while a Preview exists (ARCHITECT:
*"memoized (ephemerally, when the "widget") exists"*).

### Step 5: The GIF frame origin
**Scope:** `jam/jam_terminal/image/native/jam_TerminalImageSequence_mac.mm`
**Action:** ARCHITECT: *"the frame position is not in one place"*. The blit uses `fi.left` and `fi.top`
(`:198`, `:205`) with the size of the `CGImage` (`:162-163`). A `CGImage` of the canvas size is
already in canvas coordinates. At the blit:
```cpp
const auto isCanvasFrame { subW == canvasW and subH == canvasH };
const auto frameLeft { isCanvasFrame ? 0 : fi.left };
const auto frameTop { isCanvasFrame ? 0 : fi.top };
```
The blit lines take `frameLeft` and `frameTop`. The Windows decoder takes sub-frames from WIC, so it
does not change.
**Validation:** Read `:196-232` after the edit. ARCHITECT's run decides: `angry-typing-kitty.gif`
stays in one place.

### Step 6: A Kitty placeholder keeps its image
**Scope:** `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`
**Action:** A placeholder redraw removes a placement of its own image, and that queues a release
(`:484`, `jam_TerminalBuffer.cpp:222`). Before line 484, add
`terminalBuffer.removeImagePlacements (stored->image);`. That overload queues no release
(`jam_TerminalBuffer.cpp:231-241`). Line 484 then only meets another image.
**Validation:** `kitten icat --unicode-placeholder` is the fzf preview path (`fzf bin/fzf-preview.sh:61-81`).

### Step 7: jam::PreviewOverlay
**Scope:** new `jam/jam_gui/widgets/jam_PreviewOverlay.h`, `.cpp`; `jam/user-modules-info.md`
**Action:**
1. Add the rows beside `jam_TextEditor`, after it, so `TerminalSwap` and `Preview` are declared
   (`jam_gui.h:136`, `:158`). This table gives the generated include order.
2. The class:
   ```cpp
   class PreviewOverlay : public juce::Component,
                          private juce::ValueTree::Listener
   {
   public:
       PreviewOverlay();
       ~PreviewOverlay() override;
       void bindToProcessor (jam::HashMap<juce::Identifier, jam::Function::Map<juce::Identifier, void>>& getters, const juce::Identifier& type, const juce::Identifier& parameterID);
   #if JUCE_MODULE_AVAILABLE_jam_plugin_bootstrap
       void bindToModel (const juce::String& parameterID, AudioModel& model);
   #endif

   private:
       void valueTreeChildAdded (juce::ValueTree& parent, juce::ValueTree& child) override;
       void valueTreeChildRemoved (juce::ValueTree& parent, juce::ValueTree& child, int index) override;
       void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;
       void addPreview (const juce::ValueTree& box);
       void applyImage (const juce::ValueTree& box, Preview& preview);

       std::function<TerminalSwap* ()> terminalSource;
       juce::ValueTree images;
       juce::ValueTree terminal;
       jam::Owner<Preview> previews;
   };
   ```
3. The constructor sets `setInterceptsMouseClicks (false, false)`. `~PreviewOverlay` removes the two
   listeners, as `TextEditor` does (`jam_TextEditor.cpp:44-47`).
4. `bindToProcessor` sets `terminalSource` with the body of `TextEditor::bindToProcessor`, without the
   vblank.
5. `bindToModel` sets `images` from `getOrCreateComponentState (Format::toTag (Id::images))`, sets
   `terminal` from the `terminal` child, and adds the listener to both.
6. `addPreview` creates `std::make_unique<Preview> (juce::RectanglePlacement { juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yTop })`,
   sets its bounds, calls `addAndMakeVisible`, and calls `applyImage`.
7. `applyImage` calls `preview.setImage (*terminalSource(), number)` when `number` is below
   `swap.getNumImages()`.
8. `valueTreeChildAdded` calls `addPreview (child)` when `parent == images`.
   `valueTreeChildRemoved` calls `previews.remove (index)` when `parent == images`.
9. `valueTreePropertyChanged`: `Id::numImages` on `terminal` runs `applyImage` for each box whose
   Preview `isEmpty()`. A property of a child of `images` sets the bounds of the Preview at
   `images.indexOf (tree)`.
**Validation:** Child order equals `previews` order: children append, and a removal gives its index.
Consume the event payload (CODING). `MessageOverlay` is the sibling family. No SafePointer: the
overlay owns each Preview, and each Preview owns its job.

### Step 8: EVE wiring
**Scope:** `eve/Source/layout/component.md`, `eve/Source/EVEView.cpp`, `eve/Source/EVETerminalProcessor.cpp`,
`eve/cast/identifiers.md`
**Action:**
1. `component.md` row 1: `| 1 | previewOverlay | images | | | | | |`.
2. `registerViewComponents`: `r.registerComponent<jam::PreviewOverlay> (Id::previewOverlay);`.
3. `registerUserInterfaceGetters`: add under `userInterfaceGetters[Id::previewOverlay]` the key
   `jam::Format::toType (Id::images)`, with a getter that sets `output = &swap;`.
4. Add the row `previewOverlay` to `## identifiers`.
**Validation:** `addBindings` runs `bindToProcessor`, then `bindToModel`, by capability
(`jam_ViewManager.cpp:240-243`). The TextEditor row binds first, so its `removeAllChildren` runs before
the overlay listens. `applyComponentBounds` gives the overlay the TextEditor bounds.

### Step 9: The SKiT shell seam
**Scope:** `jam/jam_terminal/processor/jam_TerminalProcessor.cpp`, `eve/Source/EVETerminalProcessor.h`,
`.cpp`, `eve/Source/layout/parameters.md`, `eve/Source/shell/*`, `jam/cast/terminal.md`, `eve/README.md`,
`SPEC.md`
**Action:**
1. `imageConstruction` EVE arm (`:1239-1247`) takes the shape of the Sixel arm (`:1252-1255`): it calls
   the `file` arm, so `eve image` places at the cursor through the one path.
2. Delete the `previewImage` row, its id, and its event (`EVETerminalProcessor.cpp:122-130`).
3. ARCHITECT: *"this is wrong mental model. and you still think with the old terminal mental model. our
   gui is juce::Component, a gui widget ... what actually signal a CLI send? that should be registered
   parameter. downstream we only react to layout, to paint, etc"*; *"then it should be simply
   juce::ValueTree::Listener"*; *"why not just the file as metadata?"*; *"we only want images for
   preview"*; *"third party later. this first"*. The `EVE;image` operation is deleted. The one signal is
   `ESC ] 1337 ; EVE ; preview ; inline=1;width=C;height=L;path=<encoded path> BEL`.
   - The tick stores `preview`, the path, and `previewBox`, a `Cell::Rectangle::pack()` of the cursor
     and the size that `iterm2Decoder` reads. `applyEvents` reports the box, as it reports the other rows.
   - An erase sequence that touches the box (`setBlankCells`) or a screen change ends the box. fzf
     sends `CSI J` or `?1049l` on exit (`light.go:1360`, `:1365`). Text does not end it.
   - ARCHITECT: *"it should be SSOT. Preview only. more precisely PreviewComponent"*; *"One component for
     everything, single images multi layout, single layout, it does not matter. paint must be always in a
     single call for evertyhing. why do we even need numImages?"*. `jam::Preview` and
     `jam::PreviewOverlay` are replaced by one `jam::PreviewComponent`, one view-table row. It listens to
     the `images` tree and paints every image in one call. A `juce::VBlankAttachment` is the one clock.
   - The `images` tree is the one Model of every visible image. The native signal is one more child, with
     `Id::path` where a third-party child has its record number. `numImages` is deleted:
     `RecordJob::runJob` returns `jobNeedsRunningAgain` until the record holds the image.
   - ARCHITECT: *"we could simply pass on reference and andAndMakeVisible no? juce::Component hierarchy is
     decoupled from ownership. TextEditor already responsible for layout anyway, we only delegate the
     responsibility"*. The `primary` cell calls `bindTo`, and `TextEditor::addPreview` makes the component
     a child of `Content`. The TextEditor stores nothing.
   - The TextEditor writes the tree: `previewBox` and `preview` in its `properties` table call
     `setPreviewBounds`, and so does each layout change, after `setImageBounds()`.
   - ARCHITECT: *"do we need to write ImageDocument then? so no handroll"*. `jam::ImageDocument` is the AST
     of one image file, in the shape of `OpenTypeDocument`. It replaces `TerminalGifMetadata` and
     `TerminalGifFrameInfo`. A still is the same document with one frame.
   - ARCHITECT: *"we could still paint bg colour at paint() while all text at paintOverChildern()"*.
     `Content::paint` draws the background, the selection and the fills. The Preview paints as a child.
     `Content::paintOverChildren` draws the glyphs and the caret, so text shows over the image.
4. fzf gives the box. fzf exports `FZF_PREVIEW_COLUMNS` and `FZF_PREVIEW_LINES` (`terminal.go:1498-1499`),
   moves the cursor to the box origin (`:5128`), and passes `ESC]1337;` through (`:97`). `eve preview`
   reads the two variables as `width` and `height`. The default is `100%` (`jam_TerminalITerm2Decoder.cpp:30`).
5. `Source/shell/eve` is a bash script on `PATH`, so a child process reaches SKiT. It holds the bash
   `_eve_encode` and `eve` once. zsh keeps its own 1-based `_eve_encode` for its hooks. `bash_integration.bash` sources it. `zsh_integration.zsh` puts its directory on `path`.
   PowerShell keeps its function. `skit_signal` in `fzf-end.sh` calls `eve preview` for all three branches.
   The empty signal goes to `/dev/tty`, because fzf treats any `ESC]1337;` as a full-window image and stops
   rendering after it (`terminal.go:5090-5097`, `:5135-5138`).
6. README Shell API and SPEC principle 11 state the operation.
7. Third-party `File=`, Kitty and Sixel keep the placement lane, with the native end rule. A text write
   no longer ends a placement, an erase sequence does, and a new image replaces what it covers.
   `PreviewComponent` is a child of `Content` through the `primary` hook, so text paints over those images.
8. ARCHITECT: *"why cant we decode in parallel while the first frame already rendered? then immediately
   loop?"*. `DecodeJob::decodeDocument` reads the `ImageDocument`, sizes the slots once, and fans out one frame task per
   frame on `frameJobs`, each with its own source over the same mapped bytes. It composes in frame order
   and publishes each frame by the release of `numPublished`. Preview paints when slot 0 is published, and
   loops over the frames published so far. A new file signals `shouldExit`, so nothing blocks the message
   thread. `frameJobs` is a separate pool from `decodeJobs`, so a waiting job never holds the threads that
   its frames need.
**Validation:** no image byte crosses the terminal for a native signal. Limit: in fzf with a third-party
image, moving to a text file sends no erase, because fzf clears with spaces (`light.go:1643`). The old
image stays under the text until the next image or fzf's exit.

### Step 10: Hand over
**Action:** COUNSELOR reports that Steps 0 to 9 are done, and stops. ARCHITECT runs cast, which
regenerates `Source/generated/*` and `jam/generated/*`, builds, and runs the Acceptance list. A reported
failure goes through the Validation Gate to @Engineer.

### Step 11: Audit
**Action:** After ARCHITECT's report, @Auditor sweeps every file once. COUNSELOR filters each finding
by a read. @Engineer resolves each validated finding.

### Step 12: Doxygen pass
**Action:** @Pathfinder writes the blocks on exact targets. COUNSELOR reads each block against the code.

## Acceptance (ARCHITECT builds and tests)

1. `imgcat a.png` at the prompt: the image sits in its cell box, and text below stays on the grid.
2. `chafa -f iterm a.png`, `chafa -f sixel a.png`, `kitten icat a.png`: the same box as kitty shows.
3. `fzf --preview 'eve preview {}'`: the image fills the box, the GIF loops in place, the terminal does
   not stall. A text file shows no image. `escape` removes the image.
4. Scroll, resize, zoom: the image stays on its cells. Text shows over it. An erase sequence ends it, and
   a new image in the same cells replaces it.
5. `eve preview a.png width=20`.
6. `kitten icat --unicode-placeholder a.png` survives a redraw.

## BLESSED Alignment

- **Bound:** the overlay owns each Preview, and each Preview owns its job.
- **Lean:** one deletion in the layout, one view component.
- **Explicit:** named constants, no bail-out.
- **SSOT:** `getCellBounds` is the cell map. The rows are the reservation.
- **Stateless:** the boxes are Model tree state. The widgets hold no position field.
- **Encapsulation:** the TextEditor hosts a `juce::Component` and knows no Preview. The overlay knows no cell. `TerminalGraphics`
  stays as it is and does not serve this path.
- **Deterministic:** the same bytes and the same grid give the same box.

## Risks / Open Questions

- Closed: child of the view by ruling (*"Siblings via owner"*). The owner is the view table row.
- A box in a block split keeps only the part in its owner block (`jam_TerminalSwap.cpp:255-267`).
  A block holds `TerminalSwap::linesPerBlock` lines, and the fix waits for a report from a run.
- One Preview per record image. A second Kitty `a=p` placement of one id moves the box. `imgcat`,
  chafa and `kitten icat` send a new image each time.
