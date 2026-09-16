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

static juce::MemoryBlock getEmittedFrame (int cols, int rows,
    const std::function<void (jam::terminal::GraphicsContext&)>& drawContent)
{
    return getCapturedBytes (
        [cols, rows, &drawContent] (FILE* nativeHandle)
        {
            jam::terminal::GraphicsEngine engine (nativeHandle);
            engine.resize (cols, rows);

            jam::terminal::GraphicsContext context (engine);
            drawContent (context);
        });
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

static bool isFixtureFixpointStable (const juce::String& fixtureName, const void* data, int size)
{
    auto firstDocument { jam::AnsiDocument::parse (juce::String::fromUTF8 (static_cast<const char*> (data), size)) };
    const auto firstValidation { jam::AnsiValidator{}.isValid (firstDocument) };

    if (not firstValidation.wasOk())
    {
        std::cerr << fixtureName << ": " << firstValidation.getErrorMessage() << std::endl;
        return false;
    }

    const auto firstEmission { getEmittedFrame (firstDocument) };
    auto secondDocument { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (firstEmission.getData()), static_cast<int> (firstEmission.getSize()))) };
    const auto secondValidation { jam::AnsiValidator{}.isValid (secondDocument) };

    if (not secondValidation.wasOk())
    {
        std::cerr << fixtureName << " (re-parse): " << secondValidation.getErrorMessage() << std::endl;
        return false;
    }

    const auto secondEmission { getEmittedFrame (secondDocument) };
    const auto differingOffset { getFirstDifferingByteOffset (firstEmission, secondEmission) };

    if (differingOffset >= 0)
    {
        std::cerr << fixtureName << ": byte mismatch at offset " << differingOffset << std::endl;
        return false;
    }

    return true;
}

static bool isInvalidFixtureRejected (const juce::File& fixtureFile)
{
    juce::MemoryBlock fixtureBytes;
    bool rejected { true };

    if (fixtureFile.loadFileAsData (fixtureBytes))
    {
        auto document { jam::AnsiDocument::parse (juce::String::fromUTF8 (
            static_cast<const char*> (fixtureBytes.getData()), static_cast<int> (fixtureBytes.getSize()))) };
        const auto validation { jam::AnsiValidator{}.isValid (document) };

        if (validation.wasOk())
        {
            std::cerr << fixtureFile.getFileName() << ": expected validation failure, got success" << std::endl;
            rejected = false;
        }
        else
        {
            std::cout << fixtureFile.getFileName() << ": correctly rejected -- " << validation.getErrorMessage() << std::endl;
        }
    }
    else
    {
        std::cerr << fixtureFile.getFileName() << ": failed to load fixture file" << std::endl;
        rejected = false;
    }

    return rejected;
}

static bool isStampEntryFixpointStable (const juce::String& name, const jam::Stamp::Entry& entry)
{
    const auto styleId { static_cast<uint16_t> (jam::Stamp::getInstance()->addIfNotAlreadyThere (entry)) };
    const auto cell { jam::AttributedChar::make (
        static_cast<uint32_t> ('A'), jam::AttributedChar::contentCodepoint, jam::AttributedChar::narrow, styleId) };

    const auto sourceBytes { getEmittedFrame (1, 1,
        [cell] (jam::terminal::GraphicsContext& context)
        {
            jam::Document::Cells cells;
            cells.add (cell);
            context.drawCells (jam::Cell { 0 }, jam::Cell { 0 }, cells);
        }) };

    return isFixtureFixpointStable (name, sourceBytes.getData(), static_cast<int> (sourceBytes.getSize()));
}

static bool isFlagVocabularyFixpointStable()
{
    bool allStable { true };

    for (const auto& [flagValue, wireCode] : jam::AnsiDocument::sgrFlagCodes)
    {
        jam::Stamp::Entry entry;
        entry.flags = flagValue;
        allStable = isStampEntryFixpointStable ("sgrFlag" + juce::String (wireCode), entry) and allStable;
    }

    for (const auto& [flagValue, wireValue] : jam::AnsiDocument::underlineStyleCodes)
        if (flagValue != jam::Stamp::underlineNone)
        {
            jam::Stamp::Entry entry;
            entry.flags = flagValue;
            allStable = isStampEntryFixpointStable ("underlineStyle" + juce::String (wireValue), entry) and allStable;
        }

    return allStable;
}

static bool isPaletteVocabularyFixpointStable()
{
    static constexpr std::array<uint8_t, 3> paletteIndices {{ 1, 9, 196 }};

    bool allStable { true };

    for (const auto paletteIndex : paletteIndices)
    {
        jam::Stamp::Entry foregroundEntry;
        foregroundEntry.fgMode = static_cast<uint8_t> (map::ColorMode::palette256);
        foregroundEntry.fgIndex = paletteIndex;
        allStable = isStampEntryFixpointStable ("paletteForeground" + juce::String (paletteIndex), foregroundEntry) and allStable;

        jam::Stamp::Entry backgroundEntry;
        backgroundEntry.bgMode = static_cast<uint8_t> (map::ColorMode::palette256);
        backgroundEntry.bgIndex = paletteIndex;
        allStable = isStampEntryFixpointStable ("paletteBackground" + juce::String (paletteIndex), backgroundEntry) and allStable;
    }

    return allStable;
}

static bool isTruecolorVocabularyFixpointStable()
{
    jam::Stamp::Entry foregroundEntry;
    foregroundEntry.fg = juce::Colour (static_cast<juce::uint8> (18), static_cast<juce::uint8> (145), static_cast<juce::uint8> (222));

    jam::Stamp::Entry backgroundEntry;
    backgroundEntry.bg = juce::Colour (static_cast<juce::uint8> (222), static_cast<juce::uint8> (18), static_cast<juce::uint8> (145));

    const auto foregroundStable { isStampEntryFixpointStable ("truecolorForeground", foregroundEntry) };
    const auto backgroundStable { isStampEntryFixpointStable ("truecolorBackground", backgroundEntry) };

    return foregroundStable and backgroundStable;
}

static bool isStampVocabularyFixpointStable()
{
    const auto flagsStable { isFlagVocabularyFixpointStable() };
    const auto paletteStable { isPaletteVocabularyFixpointStable() };
    const auto truecolorStable { isTruecolorVocabularyFixpointStable() };

    return flagsStable and paletteStable and truecolorStable;
}

static bool isFixtureSweepFixpointStable (const juce::File& fixturesDirectory)
{
    bool allStable { true };

    for (const auto& fixtureFile : fixturesDirectory.findChildFiles (juce::File::findFiles, false, "*.ansi"))
    {
        juce::MemoryBlock fixtureBytes;

        if (fixtureFile.loadFileAsData (fixtureBytes))
        {
            const auto stable { isFixtureFixpointStable (fixtureFile.getFileName(),
                fixtureBytes.getData(), static_cast<int> (fixtureBytes.getSize())) };
            allStable = stable and allStable;
        }
        else
        {
            std::cerr << fixtureFile.getFileName() << ": failed to load fixture file" << std::endl;
            allStable = false;
        }
    }

    return allStable;
}

static bool isInvalidFixtureSweepRejected (const juce::File& fixturesDirectory)
{
    bool allRejected { true };

    for (const auto& fixtureFile : fixturesDirectory.getChildFile ("invalid").findChildFiles (juce::File::findFiles, false, "*.ansi"))
    {
        const auto rejected { isInvalidFixtureRejected (fixtureFile) };
        allRejected = rejected and allRejected;
    }

    return allRejected;
}

int main()
{
    jam::Stamp stamp;
    jam::Grapheme grapheme;
    jam::Hyperlink hyperlink;

    const juce::File fixturesDirectory { juce::File (ANSI_FIXPOINT_FIXTURES_DIR) };

    const auto validSweepStable { isFixtureSweepFixpointStable (fixturesDirectory) };
    const auto invalidSweepRejected { isInvalidFixtureSweepRejected (fixturesDirectory) };
    const auto vocabularyStable { isStampVocabularyFixpointStable() };

    const auto allPassed { validSweepStable and invalidSweepRejected and vocabularyStable };
    return allPassed ? 0 : 1;
}
