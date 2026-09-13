#include <JuceHeader.h>

#if JUCE_WINDOWS

static juce::MemoryBlock getCapturedBytes (const std::function<void (FILE*)>& writeToFile)
{
    FILE* nativeHandle { std::tmpfile() };
    jassert (nativeHandle != nullptr);

    writeToFile (nativeHandle);

    std::fseek (nativeHandle, 0, SEEK_END);
    const auto byteCount { std::ftell (nativeHandle) };
    std::rewind (nativeHandle);

    juce::MemoryBlock capturedBytes (static_cast<size_t> (byteCount));
    std::fread (capturedBytes.getData(), 1, static_cast<size_t> (byteCount), nativeHandle);
    std::fclose (nativeHandle);

    return capturedBytes;
}

#else

static juce::MemoryBlock getCapturedBytes (const std::function<void (FILE*)>& writeToFile)
{
    char* bufferPointer { nullptr };
    size_t bufferSize { 0 };
    FILE* nativeHandle { open_memstream (&bufferPointer, &bufferSize) };
    jassert (nativeHandle != nullptr);

    writeToFile (nativeHandle);

    std::fclose (nativeHandle);

    juce::MemoryBlock capturedBytes (bufferPointer, bufferSize);
    std::free (bufferPointer);

    return capturedBytes;
}

#endif

static juce::MemoryBlock getEmittedFrame (int cols, int rows, bool useInlineImages,
    const std::function<void (jam::terminal::GraphicsContext&)>& drawContent)
{
    return getCapturedBytes (
        [cols, rows, useInlineImages, &drawContent] (FILE* nativeHandle)
        {
            jam::terminal::GraphicsEngine engine (nativeHandle, useInlineImages);
            engine.resize (cols, rows);

            jam::terminal::GraphicsContext context (engine);
            drawContent (context);
        });
}

static juce::MemoryBlock getEmittedFrame (int cols, int rows,
    const std::function<void (jam::terminal::GraphicsContext&)>& drawContent)
{
    return getEmittedFrame (cols, rows, false, drawContent);
}

static juce::MemoryBlock getEmittedFrame (const jam::Document& document)
{
    int maxLineWidth { 0 };
    int lineCount { 0 };

    for (auto* line : document)
        if (line->isTag (Id::line))
        {
            maxLineWidth = juce::jmax (maxLineWidth, line->get<jam::Document::Cells> (Id::cells)->size());
            ++lineCount;
        }

    return getEmittedFrame (maxLineWidth, lineCount,
        [&document] (jam::terminal::GraphicsContext& context)
        {
            int row { 0 };

            for (auto* line : document)
                if (line->isTag (Id::line))
                {
                    context.drawCells (jam::Cell { 0 }, jam::Cell { row }, *line->get<jam::Document::Cells> (Id::cells));
                    ++row;
                }
        });
}

static int getFirstDifferingByteOffset (const juce::MemoryBlock& firstEmission, const juce::MemoryBlock& secondEmission)
{
    const auto compareSize { juce::jmin (firstEmission.getSize(), secondEmission.getSize()) };
    const auto* firstBytes { static_cast<const uint8_t*> (firstEmission.getData()) };
    const auto* secondBytes { static_cast<const uint8_t*> (secondEmission.getData()) };

    for (size_t offset { 0 }; offset < compareSize; ++offset)
        if (firstBytes[offset] != secondBytes[offset])
            return static_cast<int> (offset);

    if (firstEmission.getSize() != secondEmission.getSize())
        return static_cast<int> (compareSize);

    return -1;
}

static void dumpArtifact (const juce::String& suiteName, const juce::MemoryBlock& emission)
{
    const juce::File artifactsDirectory { juce::File (LLGC_PARITY_ARTIFACTS_DIR) };
    artifactsDirectory.createDirectory();
    artifactsDirectory.getChildFile (suiteName + ".ansi").replaceWithData (emission.getData(), emission.getSize());
}

static void appendCellCodepoint (juce::String& text, jam::AttributedChar cell)
{
    if (cell.contentTag() == jam::AttributedChar::contentCodepoint
        and cell.widthClass() != jam::AttributedChar::spacerTail
        and cell.codepoint() != 0)
        text << juce::String::charToString (static_cast<juce::juce_wchar> (cell.codepoint()));
}

static juce::String extractDocumentText (const jam::Document& document)
{
    juce::String text;

    for (auto* line : document)
        if (line->isTag (Id::line))
        {
            for (const auto& cellValue : *line->get<jam::Document::Cells> (Id::cells))
                appendCellCodepoint (text, cellValue);

            text << juce::newLine;
        }

    return text;
}

static jam::Array<jam::AttributedChar> buildCellGridFromDocument (const jam::Document& document, int cols, int rows)
{
    jam::Array<jam::AttributedChar> grid;
    grid.resize (cols * rows);

    int row { 0 };

    for (auto* line : document)
        if (line->isTag (Id::line) and row < rows)
        {
            const auto& lineCells { *line->get<jam::Document::Cells> (Id::cells) };

            for (int col { 0 }; col < juce::jmin (cols, lineCells.size()); ++col)
                grid.set (row * cols + col, lineCells.at (col));

            ++row;
        }

    return grid;
}

static constexpr uint32_t upperHalfBlock { 0x2580 };
static constexpr uint32_t lowerHalfBlock { 0x2584 };

/*____________________________________________________________________________*/
// ---- Suite 1 / Suite 2 shared fixture — the native component panel ----

static const juce::String componentPanelLabelText { "Component Fixpoint Label" };
static const juce::String componentPanelButtonText { "Press Me" };
static const juce::String componentPanelGroupText { "Group" };

struct ComponentPanel
{
    ComponentPanel (int cols, int rows)
    {
        parent.setSize (cols, rows);
        parent.setLookAndFeel (&lookAndFeel);

        label.setText (componentPanelLabelText, juce::dontSendNotification);
        label.setBorderSize (juce::BorderSize<int> (0));
        label.setBounds (2, 1, 40, 1);

        button.setButtonText (componentPanelButtonText);
        button.setBounds (2, 3, 20, 3);

        group.setText (componentPanelGroupText);
        group.setBounds (1, 7, 50, 10);

        parent.addAndMakeVisible (label);
        parent.addAndMakeVisible (button);
        parent.addAndMakeVisible (group);
    }

    ~ComponentPanel()
    {
        parent.setLookAndFeel (nullptr);
    }

    juce::Component parent;
    juce::Label label;
    juce::TextButton button;
    juce::GroupComponent group;
    juce::LookAndFeel_V4 lookAndFeel;
};

/*____________________________________________________________________________*/
// ---- Suite 1 — component fixpoint ----

static bool isDocumentFramePairFixpointStable (const juce::String& suiteName,
    const jam::Document& firstDocument, const juce::MemoryBlock& firstEmission)
{
    const auto secondEmission { getEmittedFrame (firstDocument) };
    auto secondDocument { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (secondEmission.getData()), static_cast<int> (secondEmission.getSize()))) };
    const auto secondValidation { jam::AnsiValidator{}.isValid (secondDocument) };

    if (not secondValidation.wasOk())
    {
        std::cerr << suiteName << " (re-parse): " << secondValidation.getErrorMessage() << std::endl;
        return false;
    }

    const auto differingOffset { getFirstDifferingByteOffset (firstEmission, secondEmission) };

    if (differingOffset >= 0)
    {
        std::cerr << suiteName << ": byte mismatch at offset " << differingOffset << std::endl;
        return false;
    }

    return true;
}

static bool runComponentFixpointSuite()
{
    static constexpr int cols { 80 };
    static constexpr int rows { 24 };

    ComponentPanel panel { cols, rows };
    const auto firstEmission { getEmittedFrame (cols, rows,
        [&panel] (jam::terminal::GraphicsContext& context)
        {
            juce::Graphics g (context);
            panel.parent.paintEntireComponent (g, true);
        }) };

    dumpArtifact ("suite1_component_fixpoint", firstEmission);

    auto firstDocument { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (firstEmission.getData()), static_cast<int> (firstEmission.getSize()))) };
    const auto firstValidation { jam::AnsiValidator{}.isValid (firstDocument) };

    if (not firstValidation.wasOk())
    {
        std::cerr << "componentFixpoint: " << firstValidation.getErrorMessage() << std::endl;
        return false;
    }

    const auto documentText { extractDocumentText (firstDocument) };

    if (not (documentText.contains (componentPanelLabelText) and documentText.contains (componentPanelButtonText)))
    {
        std::cerr << "componentFixpoint: emitted text missing label/button content" << std::endl;
        return false;
    }

    return isDocumentFramePairFixpointStable ("componentFixpoint", firstDocument, firstEmission);
}

/*____________________________________________________________________________*/
// ---- Suite 2 — software oracle ----

static constexpr int channelTolerance { 32 };

static bool isColourWithinTolerance (juce::Colour expected, juce::Colour actual)
{
    return std::abs (static_cast<int> (expected.getRed())   - static_cast<int> (actual.getRed()))   <= channelTolerance
       and std::abs (static_cast<int> (expected.getGreen()) - static_cast<int> (actual.getGreen())) <= channelTolerance
       and std::abs (static_cast<int> (expected.getBlue())  - static_cast<int> (actual.getBlue()))  <= channelTolerance
       and std::abs (static_cast<int> (expected.getAlpha()) - static_cast<int> (actual.getAlpha())) <= channelTolerance;
}

static const juce::Colour boxDrawingLineColour { juce::Colours::white };

static std::pair<juce::Colour, juce::Colour> getExpectedHalfCellColours (jam::AttributedChar cell, jam::Stamp& stamp)
{
    const auto& entry { stamp.get (cell.styleId()) };

    if (cell.codepoint() == upperHalfBlock)
        return { entry.fg, entry.bg };

    if (cell.codepoint() == lowerHalfBlock)
        return { entry.bg, entry.fg };

    return { entry.bg, entry.bg };
}

// The software render anti-aliases curved edges; at half-cell (2-sample)
// resolution a boundary cell has no single truth to compare against — its
// own two half-pixels already disagree with each other more than
// quantization rounding allows. Exempt those cells from the oracle compare.
static bool isAntiAliasedBoundaryCell (juce::Colour topPixel, juce::Colour bottomPixel)
{
    // Against a transparent canvas, un-premultiplied RGB recovers the source
    // colour regardless of coverage — alpha is the only channel that carries
    // the partial-coverage signal, so it must be compared alongside RGB.
    return std::abs (static_cast<int> (topPixel.getAlpha()) - static_cast<int> (bottomPixel.getAlpha())) > channelTolerance
        or not isColourWithinTolerance (topPixel, bottomPixel);
}

static bool isHalfBlockCodepoint (jam::AttributedChar cell)
{
    return cell.codepoint() == upperHalfBlock or cell.codepoint() == lowerHalfBlock;
}

// Box-drawing glyphs (and any other rendered character) are font-shaped
// content -- the software oracle rasterizes no font, so a character cell's
// pixels carry no comparable truth. Exempt like any character.
static bool isCharacterGlyphCell (jam::AttributedChar cell)
{
    return not isHalfBlockCodepoint (cell)
        and cell.codepoint() != 0
        and cell.codepoint() != static_cast<uint32_t> (' ');
}

// The terminal's per-half coverage test is a binary threshold (halfCoverage,
// jam_TerminalGraphicsContextPath.cpp); the oracle's half-pixel is continuous
// AA. At a knife-edge cell the true coverage sits near the threshold and the
// two quantizations lawfully diverge -- that half carries no comparable
// truth. juce::Image::BitmapData::getPixelColour un-premultiplies, so alpha
// alone (not the recovered RGB) is the coverage signal.
static bool isFractionalCoverageSample (juce::Colour sample)
{
    static constexpr int solidAlpha { 255 };

    return sample.getAlpha() > channelTolerance and sample.getAlpha() < solidAlpha - channelTolerance;
}

// A solid actual half compares its full colour against the expected side; a
// knife-edge (fractional) half carries no comparable truth and is exempt; an
// empty actual half has un-premultiplied RGB recovered from whatever source
// colour last touched that pixel -- meaningless at near-zero coverage, so
// only the expected side's own emptiness is asked for, never its RGB.
static bool isHalfBlockSampleMatchingOracle (juce::Colour expected, juce::Colour actual)
{
    if (isFractionalCoverageSample (actual))
        return true;

    if (actual.getAlpha() <= channelTolerance)
        return expected.getAlpha() <= channelTolerance;

    return isColourWithinTolerance (expected, actual);
}

static bool isGeometryCellMatchingOracle (jam::AttributedChar cell, jam::Stamp& stamp,
    const juce::Image::BitmapData& bitmap, int col, int row)
{
    const auto [expectedTop, expectedBottom] { getExpectedHalfCellColours (cell, stamp) };
    const auto actualTop { bitmap.getPixelColour (col, row * 2) };
    const auto actualBottom { bitmap.getPixelColour (col, row * 2 + 1) };
    const auto colourMatch { isColourWithinTolerance (expectedTop, actualTop)
        and isColourWithinTolerance (expectedBottom, actualBottom) };

    bool matches { isHalfBlockSampleMatchingOracle (expectedTop, actualTop)
        and isHalfBlockSampleMatchingOracle (expectedBottom, actualBottom) };

    if (isCharacterGlyphCell (cell))
        matches = true;
    else if (not isHalfBlockCodepoint (cell))
        matches = isAntiAliasedBoundaryCell (actualTop, actualBottom) or colourMatch;

    if (not matches)
        std::cerr << "softwareOracle: cell (" << col << "," << row << ") expected top="
            << expectedTop.toDisplayString (false) << " bottom=" << expectedBottom.toDisplayString (false)
            << " actual top=" << actualTop.toDisplayString (false) << " bottom=" << actualBottom.toDisplayString (false) << std::endl;

    return matches;
}

// Suite 2's fixture — the cell law's own promise: flat colour geometry. No
// components, no LookAndFeel, no text; every cell is a geometry cell.
static void paintGeometryOracleFixture (juce::Graphics& g)
{
    g.setColour (juce::Colours::red);
    g.fillRect (2, 2, 10, 4);

    g.setColour (juce::Colours::green);
    g.fillRect (14, 2, 10, 4);

    g.setColour (juce::Colours::blue);
    g.fillRect (26, 2, 10, 4);

    g.setColour (juce::Colours::orange);
    g.fillRoundedRectangle (2.0f, 8.0f, 15.0f, 6.0f, 3.0f);

    g.setColour (juce::Colours::cyan);
    g.fillEllipse (20.0f, 8.0f, 15.0f, 6.0f);

    g.setColour (juce::Colours::magenta);
    g.drawLine (2.0f, 16.0f, 36.0f, 16.0f, 2.0f);

    g.setColour (juce::Colours::yellow);
    g.drawLine (38.0f, 2.0f, 38.0f, 14.0f, 2.0f);

    // Line centres sit at a half-integer coordinate on the axis perpendicular
    // to travel so the unit-wide stroke's [centre-0.5, centre+0.5) footprint
    // lands exactly on one row/column of the doubled-resolution oracle
    // image -- an integer centre would straddle two, bleeding coverage into
    // the neighbour cell the per-cell compare never exempts.
    g.setColour (boxDrawingLineColour);
    g.drawLine (juce::Line<float> (2.0f, 18.5f, 20.0f, 18.5f));
    g.drawLine (juce::Line<float> (26.5f, 17.0f, 26.5f, 19.0f));
}

static bool isBoxDrawingLaneCorrect (const jam::Document& document, int cols, int rows)
{
    static constexpr uint32_t boxDrawingHorizontal { 0x2500 };
    static constexpr uint32_t boxDrawingVertical { 0x2502 };
    static constexpr int horizontalLineRow { 18 };
    static constexpr int horizontalLineStartCol { 2 };
    static constexpr int horizontalLineEndCol { 20 };
    static constexpr int verticalLineCol { 26 };
    static constexpr int verticalLineStartRow { 17 };
    static constexpr int verticalLineEndRow { 19 };

    const auto grid { buildCellGridFromDocument (document, cols, rows) };
    auto* stamp { jam::Stamp::getInstance() };
    jassert (stamp != nullptr);

    bool allMatch { true };

    for (int col { horizontalLineStartCol }; col < horizontalLineEndCol; ++col)
    {
        const auto cell { grid.at (horizontalLineRow * cols + col) };

        if (not (cell.codepoint() == boxDrawingHorizontal and stamp->get (cell.styleId()).fg == boxDrawingLineColour))
        {
            std::cerr << "softwareOracle (box-drawing horizontal): cell (" << col << "," << horizontalLineRow << ") mismatch" << std::endl;
            allMatch = false;
        }
    }

    for (int row { verticalLineStartRow }; row < verticalLineEndRow; ++row)
    {
        const auto cell { grid.at (row * cols + verticalLineCol) };

        if (not (cell.codepoint() == boxDrawingVertical and stamp->get (cell.styleId()).fg == boxDrawingLineColour))
        {
            std::cerr << "softwareOracle (box-drawing vertical): cell (" << verticalLineCol << "," << row << ") mismatch" << std::endl;
            allMatch = false;
        }
    }

    return allMatch;
}

static bool isGeometryOracleGridMatching (const jam::Document& document, int cols, int rows)
{
    static constexpr int mismatchBudget { 0 };

    const auto cells { buildCellGridFromDocument (document, cols, rows) };

    juce::Image oracleImage (juce::Image::ARGB, cols, rows * 2, true);
    {
        juce::LowLevelGraphicsSoftwareRenderer renderer (oracleImage);
        juce::Graphics g (renderer);
        g.addTransform (juce::AffineTransform::scale (1.0f, 2.0f));
        paintGeometryOracleFixture (g);
    }

    const juce::Image::BitmapData bitmap { oracleImage, juce::Image::BitmapData::readOnly };
    auto* stamp { jam::Stamp::getInstance() };
    jassert (stamp != nullptr);

    int mismatchCount { 0 };

    for (int row { 0 }; row < rows; ++row)
        for (int col { 0 }; col < cols; ++col)
            if (not isGeometryCellMatchingOracle (cells.at (row * cols + col), *stamp, bitmap, col, row))
                ++mismatchCount;

    if (mismatchCount > mismatchBudget)
        std::cerr << "softwareOracle: " << mismatchCount << " geometry-cell mismatches exceed budget " << mismatchBudget << std::endl;

    return mismatchCount <= mismatchBudget;
}

static bool runSoftwareOracleSuite()
{
    static constexpr int cols { 40 };
    static constexpr int rows { 20 };

    const auto emission { getEmittedFrame (cols, rows,
        [] (jam::terminal::GraphicsContext& context)
        {
            juce::Graphics g (context);
            paintGeometryOracleFixture (g);
        }) };

    dumpArtifact ("suite2_software_oracle", emission);

    auto document { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (emission.getData()), static_cast<int> (emission.getSize()))) };
    const auto validation { jam::AnsiValidator{}.isValid (document) };

    if (not validation.wasOk())
    {
        std::cerr << "softwareOracle: " << validation.getErrorMessage() << std::endl;
        return false;
    }

    return isGeometryOracleGridMatching (document, cols, rows)
       and isBoxDrawingLaneCorrect (document, cols, rows);
}

/*____________________________________________________________________________*/
// ---- Suite 3 — image round-trip ----

static juce::Image buildQuadrantImage()
{
    static constexpr int imageSize { 32 };
    static constexpr int halfSize { imageSize / 2 };

    juce::Image image (juce::Image::ARGB, imageSize, imageSize, true);
    juce::Graphics g (image);

    g.setColour (juce::Colours::red);
    g.fillRect (0, 0, halfSize, halfSize);
    g.setColour (juce::Colours::green);
    g.fillRect (halfSize, 0, halfSize, halfSize);
    g.setColour (juce::Colours::blue);
    g.fillRect (0, halfSize, halfSize, halfSize);
    g.setColour (juce::Colours::white);
    g.fillRect (halfSize, halfSize, halfSize, halfSize);

    return image;
}

static std::vector<uint8_t> extractIterm2Payload (const juce::MemoryBlock& emission)
{
    const auto* bytes { static_cast<const uint8_t*> (emission.getData()) };
    const auto size { emission.getSize() };
    static const std::string marker { "1337;" };

    for (size_t offset { 0 }; offset + marker.size() <= size; ++offset)
        if (std::memcmp (bytes + offset, marker.data(), marker.size()) == 0)
        {
            const auto payloadStart { offset + marker.size() };
            size_t payloadEnd { payloadStart };

            while (payloadEnd < size and bytes[payloadEnd] != static_cast<uint8_t> (Chars::bell))
                ++payloadEnd;

            return std::vector<uint8_t> (bytes + payloadStart, bytes + payloadEnd);
        }

    return {};
}

static bool isDecodedImageMatchingQuadrants (const jam::TerminalImageSequence& decoded, const juce::Image& sourceImage)
{
    static constexpr int halfSize { 16 };
    static constexpr int quarterSize { halfSize / 2 };
    static constexpr std::array<std::pair<int, int>, 4> quadrantSamplePoints
    {{
        { quarterSize, quarterSize }, { halfSize + quarterSize, quarterSize },
        { quarterSize, halfSize + quarterSize }, { halfSize + quarterSize, halfSize + quarterSize },
    }};

    if (not decoded.isValid() or decoded.width != sourceImage.getWidth() or decoded.height != sourceImage.getHeight())
    {
        std::cerr << "imageRoundTrip (iterm2): decoded dimensions mismatch" << std::endl;
        return false;
    }

    const juce::Image::BitmapData sourceBitmap { sourceImage, juce::Image::BitmapData::readOnly };
    bool allMatch { true };

    for (const auto& [sampleX, sampleY] : quadrantSamplePoints)
    {
        const auto expected { sourceBitmap.getPixelColour (sampleX, sampleY) };
        const auto* decodedPixel { decoded.pixels.get() + (static_cast<size_t> (sampleY) * static_cast<size_t> (decoded.width) + static_cast<size_t> (sampleX)) * 4 };
        const auto decodedColour { juce::Colour (decodedPixel[0], decodedPixel[1], decodedPixel[2], decodedPixel[3]) };

        if (decodedColour != expected)
        {
            std::cerr << "imageRoundTrip (iterm2): quadrant sample (" << sampleX << "," << sampleY << ") mismatch" << std::endl;
            allMatch = false;
        }
    }

    return allMatch;
}

static bool isIterm2CoveredRegionBlank (const juce::MemoryBlock& emission, int cols, int rows)
{
    auto document { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (emission.getData()), static_cast<int> (emission.getSize()))) };
    const auto validation { jam::AnsiValidator{}.isValid (document) };

    if (not validation.wasOk())
    {
        std::cerr << "imageRoundTrip (iterm2): " << validation.getErrorMessage() << std::endl;
        return false;
    }

    const auto grid { buildCellGridFromDocument (document, cols, rows) };
    bool allBlank { true };

    for (int index { 0 }; index < cols * rows; ++index)
    {
        const auto cell { grid.at (index) };
        const auto isBlankGlyph { cell.codepoint() == 0 or cell.codepoint() == static_cast<uint32_t> (' ') };

        if (not isBlankGlyph or cell.styleId() != 0)
        {
            std::cerr << "imageRoundTrip (iterm2): covered cell " << index << " is not blank" << std::endl;
            allBlank = false;
        }
    }

    return allBlank;
}

static bool isIterm2LaneRoundTripExact (const juce::Image& sourceImage)
{
    static constexpr int cols { 4 };
    static constexpr int rows { 4 };

    const auto emission { getEmittedFrame (cols, rows, true,
        [&sourceImage] (jam::terminal::GraphicsContext& context)
        {
            juce::Graphics g (context);
            g.setColour (juce::Colours::yellow);
            g.fillRect (0, 0, cols, rows);
            g.drawImageAt (sourceImage, 0, 0);
        }) };

    dumpArtifact ("suite3_image_round_trip_iterm2", emission);

    const auto payload { extractIterm2Payload (emission) };

    if (payload.empty())
    {
        std::cerr << "imageRoundTrip (iterm2): OSC 1337 payload not found" << std::endl;
        return false;
    }

    jam::TerminalITerm2Decoder decoder;
    const auto decoded { decoder.decode (payload.data(), static_cast<int> (payload.size())) };

    return isDecodedImageMatchingQuadrants (decoded, sourceImage)
       and isIterm2CoveredRegionBlank (emission, cols, rows);
}

static bool isHalfBlockGridMatchingQuadrants (const jam::Document& document, const juce::Image& sourceImage, int cols, int rows)
{
    const auto grid { buildCellGridFromDocument (document, cols, rows) };
    const juce::Image::BitmapData bitmap { sourceImage, juce::Image::BitmapData::readOnly };
    auto* stamp { jam::Stamp::getInstance() };
    jassert (stamp != nullptr);

    bool allMatch { true };

    for (int row { 0 }; row < rows; ++row)
        for (int col { 0 }; col < cols; ++col)
        {
            const auto cell { grid.at (row * cols + col) };
            const auto expectedTop { bitmap.getPixelColour (col, row * 2) };
            const auto expectedBottom { bitmap.getPixelColour (col, row * 2 + 1) };
            const auto& entry { stamp->get (cell.styleId()) };

            if (not (cell.codepoint() == upperHalfBlock and entry.fg == expectedTop and entry.bg == expectedBottom))
            {
                std::cerr << "imageRoundTrip (half-block): cell (" << col << "," << row << ") mismatch" << std::endl;
                allMatch = false;
            }
        }

    return allMatch;
}

static bool isHalfBlockImageLaneStable (const juce::Image& sourceImage)
{
    static constexpr int cols { 32 };
    static constexpr int rows { 16 };

    const auto emission { getEmittedFrame (cols, rows,
        [&sourceImage] (jam::terminal::GraphicsContext& context)
        {
            juce::Graphics g (context);
            g.drawImageTransformed (sourceImage, juce::AffineTransform::scale (1.0f, 0.5f));
        }) };

    dumpArtifact ("suite3_image_round_trip_halfblock", emission);

    auto document { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (emission.getData()), static_cast<int> (emission.getSize()))) };
    const auto validation { jam::AnsiValidator{}.isValid (document) };

    if (not validation.wasOk())
    {
        std::cerr << "imageRoundTrip (half-block): " << validation.getErrorMessage() << std::endl;
        return false;
    }

    return isHalfBlockGridMatchingQuadrants (document, sourceImage, cols, rows)
       and isDocumentFramePairFixpointStable ("imageRoundTrip (half-block)", document, emission);
}

static bool runImageRoundTripSuite()
{
    const auto sourceImage { buildQuadrantImage() };
    const auto iterm2Stable { isIterm2LaneRoundTripExact (sourceImage) };
    const auto halfBlockStable { isHalfBlockImageLaneStable (sourceImage) };

    return iterm2Stable and halfBlockStable;
}

/*____________________________________________________________________________*/
// ---- Suite 4 — geometry cases ----

static bool isRoundedRectangleCornerCell (jam::Cell::Rectangle fillArea, int cornerRadius, int col, int row)
{
    const bool nearLeft { col < fillArea.x + cornerRadius };
    const bool nearRight { col >= fillArea.getRight().value - cornerRadius };
    const bool nearTop { row < fillArea.y + cornerRadius };
    const bool nearBottom { row >= fillArea.getBottom().value - cornerRadius };

    return (nearLeft or nearRight) and (nearTop or nearBottom);
}

static bool isForeignRoundedRectangleContent (jam::AttributedChar cell)
{
    return not (cell.codepoint() == 0 or cell.codepoint() == static_cast<uint32_t> (' ')
        or cell.codepoint() == upperHalfBlock or cell.codepoint() == lowerHalfBlock);
}

static bool isRoundedRectangleFillConsistent (const jam::Document& document, int cols, int rows, juce::Colour fillColour)
{
    const jam::Cell::Rectangle fillArea { jam::Cell { 2 }, jam::Cell { 2 }, jam::Cell { 30 }, jam::Cell { 8 } };
    static constexpr int cornerRadius { 3 };

    const auto grid { buildCellGridFromDocument (document, cols, rows) };
    auto* stamp { jam::Stamp::getInstance() };
    jassert (stamp != nullptr);

    bool allConsistent { true };

    for (int row { fillArea.y }; row < fillArea.getBottom().value; ++row)
        for (int col { fillArea.x }; col < fillArea.getRight().value; ++col)
        {
            const auto cell { grid.at (row * cols + col) };

            if (isRoundedRectangleCornerCell (fillArea, cornerRadius, col, row))
            {
                if (isForeignRoundedRectangleContent (cell))
                    allConsistent = false;
            }
            else if (stamp->get (cell.styleId()).bg != fillColour)
            {
                allConsistent = false;
            }
        }

    return allConsistent;
}

static bool isRoundedRectangleGeometryCorrect()
{
    static constexpr int cols { 40 };
    static constexpr int rows { 12 };
    static const juce::Colour fillColour { juce::Colours::orange };
    static const juce::String labelText { "Rounded" };

    const auto emission { getEmittedFrame (cols, rows,
        [] (jam::terminal::GraphicsContext& context)
        {
            juce::Graphics g (context);
            g.setColour (fillColour);
            g.fillRoundedRectangle (2.0f, 2.0f, 30.0f, 8.0f, 3.0f);

            juce::Component labelParent;
            labelParent.setSize (cols, rows);
            juce::Label label;
            label.setText (labelText, juce::dontSendNotification);
            label.setBorderSize (juce::BorderSize<int> (0));
            label.setBounds (4, 5, 20, 1);
            labelParent.addAndMakeVisible (label);
            labelParent.paintEntireComponent (g, true);
        }) };

    dumpArtifact ("suite4_geometry_cases_rounded_rect", emission);

    auto document { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (emission.getData()), static_cast<int> (emission.getSize()))) };
    const auto validation { jam::AnsiValidator{}.isValid (document) };

    if (not validation.wasOk())
    {
        std::cerr << "geometryCases (rounded-rect): " << validation.getErrorMessage() << std::endl;
        return false;
    }

    return isRoundedRectangleFillConsistent (document, cols, rows, fillColour);
}

static bool isCurvedClipGridConsistent (const jam::Document& document, int cols, int rows, juce::Colour fillColour)
{
    static constexpr float centreX { 10.0f };
    static constexpr float centreY { 5.0f };
    static constexpr float radiusX { 8.0f };
    static constexpr float radiusY { 4.0f };
    static constexpr float insideThreshold { 0.8f };
    static constexpr float outsideThreshold { 1.3f };

    const auto grid { buildCellGridFromDocument (document, cols, rows) };
    auto* stamp { jam::Stamp::getInstance() };
    jassert (stamp != nullptr);

    bool allConsistent { true };

    for (int row { 0 }; row < rows; ++row)
        for (int col { 0 }; col < cols; ++col)
        {
            const auto normalisedRadius { std::pow ((static_cast<float> (col) + 0.5f - centreX) / radiusX, 2.0f)
                + std::pow ((static_cast<float> (row) + 0.5f - centreY) / radiusY, 2.0f) };
            const auto painted { stamp->get (grid.at (row * cols + col).styleId()).bg == fillColour };

            if (normalisedRadius <= insideThreshold and not painted)
                allConsistent = false;

            if (normalisedRadius >= outsideThreshold and painted)
                allConsistent = false;
        }

    return allConsistent;
}

static bool isCurvedClipConsistent()
{
    static constexpr int cols { 20 };
    static constexpr int rows { 10 };
    static const juce::Colour fillColour { juce::Colours::cyan };

    juce::Path ellipsePath;
    ellipsePath.addEllipse (2.0f, 1.0f, 16.0f, 8.0f);

    const auto emission { getEmittedFrame (cols, rows,
        [&ellipsePath] (jam::terminal::GraphicsContext& context)
        {
            juce::Graphics g (context);
            g.saveState();
            g.reduceClipRegion (ellipsePath, juce::AffineTransform());
            g.setColour (fillColour);
            g.fillRect (0, 0, cols, rows);
            g.restoreState();
        }) };

    dumpArtifact ("suite4_geometry_cases_curved_clip", emission);

    auto document { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (emission.getData()), static_cast<int> (emission.getSize()))) };
    const auto validation { jam::AnsiValidator{}.isValid (document) };

    if (not validation.wasOk())
    {
        std::cerr << "geometryCases (curved-clip): " << validation.getErrorMessage() << std::endl;
        return false;
    }

    return isCurvedClipGridConsistent (document, cols, rows, fillColour);
}

static bool isTransparencyLayerGridConsistent (const jam::Document& document, int cols, int rows,
    juce::Colour redColour, juce::Colour blueColour, const juce::String& labelText)
{
    const auto grid { buildCellGridFromDocument (document, cols, rows) };
    auto* stamp { jam::Stamp::getInstance() };
    jassert (stamp != nullptr);

    const auto expectedBlend { blueColour.interpolatedWith (redColour, 0.5f) };
    bool allConsistent { true };

    for (int row { 3 }; row < rows; ++row)
        for (int col { 0 }; col < cols; ++col)
            if (not isColourWithinTolerance (expectedBlend, stamp->get (grid.at (row * cols + col).styleId()).bg))
                allConsistent = false;

    juce::String labelRowText;

    for (int col { 0 }; col < cols; ++col)
        appendCellCodepoint (labelRowText, grid.at (col));

    if (not labelRowText.contains (labelText))
        allConsistent = false;

    return allConsistent;
}

static bool isTransparencyLayerConsistent()
{
    static constexpr int cols { 20 };
    static constexpr int rows { 10 };
    static const juce::Colour redColour { juce::Colours::red };
    static const juce::Colour blueColour { juce::Colours::blue };
    static const juce::String labelText { "Layered" };

    const auto emission { getEmittedFrame (cols, rows,
        [] (jam::terminal::GraphicsContext& context)
        {
            juce::Graphics g (context);
            g.setColour (redColour);
            g.fillRect (0, 0, cols, rows);

            juce::Component labelParent;
            labelParent.setSize (cols, rows);
            juce::Label label;
            label.setText (labelText, juce::dontSendNotification);
            label.setBorderSize (juce::BorderSize<int> (0));
            label.setBounds (1, 0, 10, 1);
            labelParent.addAndMakeVisible (label);
            labelParent.paintEntireComponent (g, true);

            g.beginTransparencyLayer (0.5f);
            g.setColour (blueColour);
            g.fillRect (0, 3, cols, rows - 3);
            g.endTransparencyLayer();
        }) };

    dumpArtifact ("suite4_geometry_cases_transparency", emission);

    auto document { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (emission.getData()), static_cast<int> (emission.getSize()))) };
    const auto validation { jam::AnsiValidator{}.isValid (document) };

    if (not validation.wasOk())
    {
        std::cerr << "geometryCases (transparency): " << validation.getErrorMessage() << std::endl;
        return false;
    }

    return isTransparencyLayerGridConsistent (document, cols, rows, redColour, blueColour, labelText);
}

static bool runGeometryCasesSuite()
{
    const auto roundedRectangleCorrect { isRoundedRectangleGeometryCorrect() };
    const auto curvedClipConsistent { isCurvedClipConsistent() };
    const auto transparencyConsistent { isTransparencyLayerConsistent() };

    return roundedRectangleCorrect and curvedClipConsistent and transparencyConsistent;
}

/*____________________________________________________________________________*/

int main()
{
    jam::Stamp stamp;
    jam::Grapheme grapheme;
    jam::Hyperlink hyperlink;

    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const auto componentFixpointStable { runComponentFixpointSuite() };
    const auto softwareOracleStable { runSoftwareOracleSuite() };
    const auto imageRoundTripStable { runImageRoundTripSuite() };
    const auto geometryCasesStable { runGeometryCasesSuite() };

    const auto allPassed { componentFixpointStable and softwareOracleStable and imageRoundTripStable and geometryCasesStable };
    return allPassed ? 0 : 1;
}
