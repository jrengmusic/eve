#include <JuceHeader.h>
#include <random>

#if JUCE_MAC
#include <mach/mach.h>
#endif

#include "Anchor.h"
#include "CountedTree.h"
#include "SkipList.h"

static constexpr unsigned int randomSeed { 1337u };
static constexpr int appendWhileIndexedLineCount { 1000 };
static constexpr int viewportLineCount { 50 };
static constexpr int wrapColumnsNarrow { 80 };
static constexpr int wrapColumnsWide { 120 };
static constexpr int wrapSampleCount { 200 };

static juce::String getSyntheticLine (int lineNumber)
{
    constexpr int cellsPerLine { 80 };

    juce::String line { "line " + juce::String (lineNumber).paddedLeft ('0', 8) + " " };

    while (line.length() < cellsPerLine)
        line << static_cast<char> ('a' + (line.length() % 26));

    return line;
}

static juce::String getSyntheticText (int lineCount)
{
    constexpr size_t estimatedBytesPerLine { 96 };

    juce::MemoryOutputStream textStream { static_cast<size_t> (lineCount) * estimatedBytesPerLine };

    for (int lineNumber { 0 }; lineNumber < lineCount; ++lineNumber)
        textStream << getSyntheticLine (lineNumber) << "\n";

    return textStream.toString();
}

static double getElapsedSeconds (std::chrono::steady_clock::time_point startTime)
{
    return std::chrono::duration<double> (std::chrono::steady_clock::now() - startTime).count();
}

static jam::Document::Element* appendGeneratedLine (jam::AnsiDocument& document, int lineNumber)
{
    jam::Document::Cells cells;
    const auto syntheticLine { getSyntheticLine (lineNumber) };

    for (auto characterPointer { syntheticLine.getCharPointer() };
         not characterPointer.isEmpty(); ++characterPointer)
        cells.add (jam::AttributedChar::make (static_cast<uint32_t> (*characterPointer),
            jam::AttributedChar::contentCodepoint, jam::AttributedChar::narrow, 0));

    jam::AnsiDocument lineDocument;
    lineDocument.root->id = Id::line;
    lineDocument.root->add<jam::Document::Cells> (Id::cells, std::move (cells));

    return document.appendChild (*document.root, std::move (lineDocument));
}

static std::string getEncodedLineBytes (const jam::Document::Cells& cells)
{
    auto lineBytes { jam::terminal::getRowText (cells.data(), cells.size()) };

    lineBytes += Chars::csiIntroducer;
    jam::terminal::appendDecimal (lineBytes, static_cast<int> (map::SGR::reset));
    lineBytes += static_cast<char> (map::CSI::selectGraphicRendition);
    lineBytes += static_cast<char> (Chars::newline);

    return lineBytes;
}

static double addLines (jam::AnsiDocument& document, int lineCount)
{
    const auto startTime { std::chrono::steady_clock::now() };

    for (int lineNumber { 0 }; lineNumber < lineCount; ++lineNumber)
        appendGeneratedLine (document, lineNumber);

    return getElapsedSeconds (startTime);
}

static size_t getResidentBytes()
{
#if JUCE_MAC
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t count { MACH_TASK_BASIC_INFO_COUNT };

    if (task_info (mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t> (&info), &count) == KERN_SUCCESS)
        return static_cast<size_t> (info.resident_size);

    return 0;
#else
    return 0;
#endif
}

static int getLineCount (const jam::AnsiDocument& document)
{
    auto lineCount { 0 };

    for (auto* line : document)
        if (line->isTag (Id::line))
            ++lineCount;

    return lineCount;
}

static jam::Array<int> getColdJumpTargets (int lineCount)
{
    constexpr int randomJumpCount { 1000 };

    jam::Array<int> targets;
    targets.add (0);
    targets.add (lineCount - 1);

    std::mt19937 jumpGenerator { randomSeed };
    std::uniform_int_distribution<int> jumpPicker { 0, lineCount - 1 };

    for (int jumpIndex { 0 }; jumpIndex < randomJumpCount; ++jumpIndex)
        targets.add (jumpPicker (jumpGenerator));

    return targets;
}

static jam::Array<int> getScrollTargets (int midLineNumber, int delta, int moveCount)
{
    jam::Array<int> targets;
    auto currentLineNumber { midLineNumber };

    for (int moveIndex { 0 }; moveIndex < moveCount; ++moveIndex)
    {
        currentLineNumber += (moveIndex % 2 == 0) ? delta : -delta;
        targets.add (currentLineNumber);
    }

    return targets;
}

static jam::Array<int> getWrapSampleTargets (int upperExclusiveBound)
{
    jam::Array<int> targets;
    std::mt19937 sampleGenerator { randomSeed };
    std::uniform_int_distribution<int> samplePicker { 0, upperExclusiveBound - 1 };

    for (int sampleIndex { 0 }; sampleIndex < wrapSampleCount; ++sampleIndex)
        targets.add (samplePicker (sampleGenerator));

    return targets;
}

static double getSequentialWalkSeconds (const jam::AnsiDocument& document, int startLineNumber, int viewportLineCount)
{
    jassert (startLineNumber >= 0);
    jassert (viewportLineCount > 0);

    auto* element { getElementAt (document, startLineNumber) };
    jassert (element != nullptr);

    const auto startTime { std::chrono::steady_clock::now() };

    volatile size_t touchedCellCount { 0 };

    for (int visitedLineCount { 0 }; visitedLineCount < viewportLineCount; ++visitedLineCount)
    {
        touchedCellCount += element->get<jam::Document::Cells> (Id::cells)->size();

        if (visitedLineCount + 1 < viewportLineCount)
        {
            auto* sibling { element->nextSibling };

            while (not sibling->isTag (Id::line))
                sibling = sibling->nextSibling;

            element = sibling;
        }
    }

    return getElapsedSeconds (startTime);
}

template <typename ElementAtFunction>
static double getColdJumpSeconds (const jam::Array<int>& targets, ElementAtFunction elementAt)
{
    const auto startTime { std::chrono::steady_clock::now() };

    volatile size_t touchedCellCount { 0 };

    for (const auto targetLineNumber : targets)
    {
        auto* element { elementAt (targetLineNumber) };
        touchedCellCount += element->template get<jam::Document::Cells> (Id::cells)->size();
    }

    return getElapsedSeconds (startTime);
}

template <typename Candidate>
static bool isCandidateCorrect (Candidate& candidate, const jam::AnsiDocument& appendDocument, const jam::Array<int>& targets)
{
    auto correctnessMatches { true };

    for (const auto targetLineNumber : targets)
    {
        auto* candidateElement { candidate.elementAt (targetLineNumber) };
        auto* walkedElement { getElementAt (appendDocument, targetLineNumber) };

        if (candidateElement != walkedElement)
            std::cerr << "mismatch at line " << targetLineNumber << std::endl;

        correctnessMatches = (candidateElement == walkedElement) and correctnessMatches;
    }

    return correctnessMatches;
}

static void addReportRow (juce::StringArray& reportRows, const juce::String& workload, const juce::String& contender,
    int lineCount, double elapsedSeconds, int operationCount)
{
    constexpr double minimumElapsedSeconds { 1.0e-9 };

    const auto operationsPerSecond { static_cast<double> (operationCount) / juce::jmax (elapsedSeconds, minimumElapsedSeconds) };

    reportRows.add ("| " + workload + " | " + contender + " | " + juce::String (lineCount) + " | "
        + juce::String (elapsedSeconds, 6) + " | " + juce::String (operationsPerSecond, 1) + " |");
}

static void addBaselineRows (jam::AnsiDocument& appendDocument, const jam::Array<int>& coldJumpTargets,
    int midLineNumber, int lineCount, juce::StringArray& reportRows)
{
    const auto sequentialWalkSeconds { getSequentialWalkSeconds (appendDocument, midLineNumber, viewportLineCount) };
    addReportRow (reportRows, "sequentialWalk", "bareWalk", lineCount, sequentialWalkSeconds, viewportLineCount);

    auto elementAtDocument = [&appendDocument] (int line) { return getElementAt (appendDocument, line); };
    const auto coldJumpSeconds { getColdJumpSeconds (coldJumpTargets, elementAtDocument) };
    addReportRow (reportRows, "coldJump", "bareWalk", lineCount, coldJumpSeconds, coldJumpTargets.size());

    const auto appendWhileIndexedSeconds { addLines (appendDocument, appendWhileIndexedLineCount) };
    addReportRow (reportRows, "appendWhileIndexed", "document", lineCount, appendWhileIndexedSeconds, appendWhileIndexedLineCount);
}

template <typename Candidate>
static bool addCandidateRows (Candidate& candidate, const juce::String& contender,
    const jam::AnsiDocument& appendDocument, const jam::Array<int>& coldJumpTargets,
    const jam::Array<int>& scrollByOneTargets, const jam::Array<int>& scrollByHalfPageTargets,
    int lineCount, juce::StringArray& reportRows)
{
    constexpr int gateCheckCount { 100 };

    jam::Array<int> gateTargets;

    for (int checkIndex { 0 }; checkIndex < juce::jmin (gateCheckCount, coldJumpTargets.size()); ++checkIndex)
        gateTargets.add (coldJumpTargets.at (checkIndex));

    const auto gateStartTime { std::chrono::steady_clock::now() };
    const auto correctnessMatches { isCandidateCorrect (candidate, appendDocument, gateTargets) };
    const auto gateSeconds { getElapsedSeconds (gateStartTime) };

    addReportRow (reportRows, "gate", contender, lineCount, gateSeconds, gateTargets.size());

    if (correctnessMatches)
    {
        auto elementAtCandidate = [&candidate] (int line) { return candidate.elementAt (line); };

        const auto coldJumpSeconds { getColdJumpSeconds (coldJumpTargets, elementAtCandidate) };
        addReportRow (reportRows, "coldJump", contender, lineCount, coldJumpSeconds, coldJumpTargets.size());

        const auto scrollByOneSeconds { getColdJumpSeconds (scrollByOneTargets, elementAtCandidate) };
        addReportRow (reportRows, "scrollByOne", contender, lineCount, scrollByOneSeconds, scrollByOneTargets.size());

        const auto scrollByHalfPageSeconds { getColdJumpSeconds (scrollByHalfPageTargets, elementAtCandidate) };
        addReportRow (reportRows, "scrollByHalfPage", contender, lineCount, scrollByHalfPageSeconds, scrollByHalfPageTargets.size());
    }

    return correctnessMatches;
}

template <typename Candidate>
static bool addAppendWhileIndexedRow (Candidate& candidate, const juce::String& contender,
    jam::AnsiDocument& appendDocument, int lineCount, juce::StringArray& reportRows)
{
    const auto lineTotalBeforeAppend { getLineCount (appendDocument) };
    const auto appendStartTime { std::chrono::steady_clock::now() };

    for (int newLineIndex { 0 }; newLineIndex < appendWhileIndexedLineCount; ++newLineIndex)
    {
        auto* newElement { appendGeneratedLine (appendDocument, lineTotalBeforeAppend + newLineIndex) };
        candidate.appendLine (newElement);
    }

    const auto appendSeconds { getElapsedSeconds (appendStartTime) };
    addReportRow (reportRows, "appendWhileIndexed", contender, lineCount, appendSeconds, appendWhileIndexedLineCount);

    const auto verifyLineNumber { lineTotalBeforeAppend + appendWhileIndexedLineCount - 1 };
    const auto candidateElement { candidate.elementAt (verifyLineNumber) };
    const auto walkedElement { getElementAt (appendDocument, verifyLineNumber) };
    const auto appendVerifyMatches { candidateElement == walkedElement };

    if (not appendVerifyMatches)
        std::cerr << contender << " append-while-indexed mismatch at line " << verifyLineNumber << std::endl;

    return appendVerifyMatches;
}

template <int arity>
static bool addCountedTreeCandidateRows (jam::AnsiDocument& appendDocument, jam::Document::Element* firstLineElement,
    const jam::Array<int>& coldJumpTargets, const jam::Array<int>& scrollByOneTargets,
    const jam::Array<int>& scrollByHalfPageTargets, int lineCount, juce::StringArray& reportRows)
{
    const juce::String contender { "countedTree" + juce::String (arity) };
    CountedTree<arity> candidate { firstLineElement };

    auto passed { addCandidateRows (candidate, contender, appendDocument, coldJumpTargets,
        scrollByOneTargets, scrollByHalfPageTargets, lineCount, reportRows) };

    if (passed)
        passed = addAppendWhileIndexedRow (candidate, contender, appendDocument, lineCount, reportRows);

    return passed;
}

static bool addAllCandidateRows (jam::AnsiDocument& appendDocument, int midLineNumber, int lineCount,
    const jam::Array<int>& coldJumpTargets, const jam::Array<int>& scrollByOneTargets,
    const jam::Array<int>& scrollByHalfPageTargets, juce::StringArray& reportRows)
{
    Anchor anchor { appendDocument, midLineNumber };
    auto allPassed { addCandidateRows (anchor, "anchor", appendDocument, coldJumpTargets,
        scrollByOneTargets, scrollByHalfPageTargets, lineCount, reportRows) };

    auto* firstLineElement { getElementAt (appendDocument, 0) };

    allPassed = addCountedTreeCandidateRows<8> (appendDocument, firstLineElement, coldJumpTargets,
        scrollByOneTargets, scrollByHalfPageTargets, lineCount, reportRows) and allPassed;

    allPassed = addCountedTreeCandidateRows<16> (appendDocument, firstLineElement, coldJumpTargets,
        scrollByOneTargets, scrollByHalfPageTargets, lineCount, reportRows) and allPassed;

    allPassed = addCountedTreeCandidateRows<32> (appendDocument, firstLineElement, coldJumpTargets,
        scrollByOneTargets, scrollByHalfPageTargets, lineCount, reportRows) and allPassed;

    allPassed = addCountedTreeCandidateRows<64> (appendDocument, firstLineElement, coldJumpTargets,
        scrollByOneTargets, scrollByHalfPageTargets, lineCount, reportRows) and allPassed;

    SkipList skipList { firstLineElement, randomSeed };
    auto skipListPassed { addCandidateRows (skipList, "skipList", appendDocument, coldJumpTargets,
        scrollByOneTargets, scrollByHalfPageTargets, lineCount, reportRows) };

    if (skipListPassed)
        skipListPassed = addAppendWhileIndexedRow (skipList, "skipList", appendDocument, lineCount, reportRows);

    return allPassed and skipListPassed;
}

static bool addPopulationRows (const jam::AnsiDocument& appendDocument, double appendElapsedSeconds, int lineCount, juce::StringArray& reportRows)
{
    const auto syntheticText { getSyntheticText (lineCount) };
    const auto parseStartTime { std::chrono::steady_clock::now() };
    auto parseDocument { jam::AnsiDocument::parse (syntheticText) };
    const auto parseElapsedSeconds { getElapsedSeconds (parseStartTime) };
    const auto parseLineCount { getLineCount (parseDocument) };
    const auto parseCountMatches { parseLineCount == lineCount };

    if (not parseCountMatches)
        std::cerr << "parse lane: expected " << lineCount << " lines, got " << parseLineCount << std::endl;

    addReportRow (reportRows, "parse", "document", lineCount, parseElapsedSeconds, lineCount);

    const auto appendLineCount { getLineCount (appendDocument) };
    const auto appendCountMatches { appendLineCount == lineCount };

    if (not appendCountMatches)
        std::cerr << "append lane: expected " << lineCount << " lines, got " << appendLineCount << std::endl;

    addReportRow (reportRows, "append", "document", lineCount, appendElapsedSeconds, lineCount);

    return parseCountMatches and appendCountMatches;
}

static juce::File spillFilePath (int lineCount)
{
    return juce::File (BUFFER_BENCH_RESULTS_PATH).getSiblingFile ("spill-" + juce::String (lineCount) + ".dat");
}

static std::pair<uint32_t, int> getPageByteRange (const jam::Array<jam::Document::Span>& spillSpans, int targetLineNumber, int lineCount)
{
    const auto lastLineNumber { juce::jmin (targetLineNumber + viewportLineCount - 1, lineCount - 1) };
    const auto [firstOffset, firstLength] { spillSpans.at (targetLineNumber) };
    const auto [lastOffset, lastLength] { spillSpans.at (lastLineNumber) };

    return { firstOffset, static_cast<int> (lastOffset + lastLength - firstOffset) };
}

static jam::AnsiDocument getRehydratedPage (juce::FileInputStream& spillStream, const jam::Array<jam::Document::Span>& spillSpans,
    int targetLineNumber, int lineCount)
{
    const auto [pageOffset, pageByteLength] { getPageByteRange (spillSpans, targetLineNumber, lineCount) };

    juce::HeapBlock<char> pageBytes { static_cast<size_t> (pageByteLength) };
    spillStream.setPosition (static_cast<juce::int64> (pageOffset));
    spillStream.read (pageBytes.getData(), pageByteLength);

    return jam::AnsiDocument::parse (juce::String::fromUTF8 (pageBytes.getData(), pageByteLength));
}

static double getSpillWriteSeconds (const jam::AnsiDocument& appendDocument, const juce::File& spillFile,
    jam::Array<jam::Document::Span>& spillSpans)
{
    juce::FileOutputStream outputStream { spillFile };
    jassert (outputStream.openedOk());

    const auto startTime { std::chrono::steady_clock::now() };

    uint32_t currentOffset { 0 };

    for (auto* line : appendDocument)
        if (line->isTag (Id::line))
        {
            const auto lineBytes { getEncodedLineBytes (*line->get<jam::Document::Cells> (Id::cells)) };
            const auto byteLength { static_cast<uint32_t> (lineBytes.size()) };

            outputStream.write (lineBytes.data(), lineBytes.size());
            spillSpans.add (jam::Document::Span::pack (currentOffset, byteLength));
            currentOffset += byteLength;
        }

    outputStream.flush();

    return getElapsedSeconds (startTime);
}

static void addSpillSizeRow (juce::StringArray& reportRows, int lineCount, double bytesPerLine)
{
    reportRows.add ("| spillSize | document | " + juce::String (lineCount) + " | "
        + juce::String (0.0, 6) + " | " + juce::String (bytesPerLine, 1) + " |");
}

static bool isRehydratePagesCorrect (juce::FileInputStream& spillStream, const jam::Array<jam::Document::Span>& spillSpans,
    const jam::AnsiDocument& appendDocument, const jam::Array<int>& coldJumpTargets, int lineCount)
{
    auto allCorrect { true };

    for (const auto targetLineNumber : coldJumpTargets)
    {
        const auto pageDocument { getRehydratedPage (spillStream, spillSpans, targetLineNumber, lineCount) };
        auto* pageFirstLine { getElementAt (pageDocument, 0) };
        auto* liveLine { getElementAt (appendDocument, targetLineNumber) };

        const auto cellsMatch { pageFirstLine->get<jam::Document::Cells> (Id::cells)->size()
            == liveLine->get<jam::Document::Cells> (Id::cells)->size() };

        if (not cellsMatch)
            std::cerr << "rehydrate mismatch at line " << targetLineNumber << std::endl;

        allCorrect = cellsMatch and allCorrect;
    }

    return allCorrect;
}

static double getRehydrateSeconds (juce::FileInputStream& spillStream, const jam::Array<jam::Document::Span>& spillSpans,
    const jam::Array<int>& coldJumpTargets, int lineCount)
{
    const auto startTime { std::chrono::steady_clock::now() };

    for (const auto targetLineNumber : coldJumpTargets)
        getRehydratedPage (spillStream, spillSpans, targetLineNumber, lineCount);

    return getElapsedSeconds (startTime);
}

static bool isSpillGateCorrect (juce::FileInputStream& spillStream, const jam::Array<jam::Document::Span>& spillSpans,
    int targetLineNumber, int lineCount, juce::StringArray& reportRows)
{
    const auto gateStartTime { std::chrono::steady_clock::now() };

    const auto [pageOffset, pageByteLength] { getPageByteRange (spillSpans, targetLineNumber, lineCount) };

    juce::MemoryBlock originalBytes { static_cast<size_t> (pageByteLength) };
    spillStream.setPosition (static_cast<juce::int64> (pageOffset));
    spillStream.read (originalBytes.getData(), pageByteLength);

    const auto pageDocument { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (originalBytes.getData()), pageByteLength)) };

    std::string reencodedText;

    for (auto* line : pageDocument)
        if (line->isTag (Id::line))
            reencodedText += getEncodedLineBytes (*line->get<jam::Document::Cells> (Id::cells));

    const juce::MemoryBlock reencodedBytes { reencodedText.data(), reencodedText.size() };
    const auto bytesMatch { originalBytes == reencodedBytes };

    if (not bytesMatch)
        std::cerr << "spillGate mismatch at line " << targetLineNumber << std::endl;

    const auto gateSeconds { getElapsedSeconds (gateStartTime) };
    addReportRow (reportRows, "spillGate", "document", lineCount, gateSeconds, 1);

    return bytesMatch;
}

static bool addSpillRows (const jam::AnsiDocument& appendDocument, const jam::Array<int>& coldJumpTargets,
    int lineCount, juce::StringArray& reportRows)
{
    const auto spillFile { spillFilePath (lineCount) };
    spillFile.deleteFile();

    jam::Array<jam::Document::Span> spillSpans;
    const auto spillWriteSeconds { getSpillWriteSeconds (appendDocument, spillFile, spillSpans) };
    addReportRow (reportRows, "spillWrite", "document", lineCount, spillWriteSeconds, lineCount);

    const auto [lastOffset, lastLength] { spillSpans.last() };
    const auto bytesPerLine { static_cast<double> (lastOffset + lastLength) / static_cast<double> (lineCount) };
    addSpillSizeRow (reportRows, lineCount, bytesPerLine);

    juce::FileInputStream spillStream { spillFile };
    jassert (spillStream.openedOk());

    const auto rehydrateCorrect { isRehydratePagesCorrect (spillStream, spillSpans, appendDocument, coldJumpTargets, lineCount) };
    const auto rehydrateSeconds { getRehydrateSeconds (spillStream, spillSpans, coldJumpTargets, lineCount) };
    addReportRow (reportRows, "rehydrate", "document", lineCount, rehydrateSeconds, coldJumpTargets.size());

    const auto gateCorrect { isSpillGateCorrect (spillStream, spillSpans, coldJumpTargets.at (0), lineCount, reportRows) };

    spillFile.deleteFile();

    return rehydrateCorrect and gateCorrect;
}

static bool isDocumentIndexCorrect (jam::Document::Index& index, const jam::AnsiDocument& appendDocument, const jam::Array<int>& targets)
{
    auto correctnessMatches { true };

    for (const auto targetLineNumber : targets)
    {
        auto* indexedElement { index.getElement (targetLineNumber) };
        auto* walkedElement { getElementAt (appendDocument, targetLineNumber) };

        if (indexedElement != walkedElement)
            std::cerr << "documentIndex mismatch at line " << targetLineNumber << std::endl;

        correctnessMatches = (indexedElement == walkedElement) and correctnessMatches;
    }

    return correctnessMatches;
}

static bool addDocumentIndexRows (const jam::AnsiDocument& appendDocument, const jam::Array<int>& coldJumpTargets,
    int lineCount, juce::StringArray& reportRows)
{
    jam::Document::Index index { appendDocument };

    const auto correctnessMatches { isDocumentIndexCorrect (index, appendDocument, coldJumpTargets) };

    if (correctnessMatches)
    {
        auto elementAtIndex = [&index] (int line) { return index.getElement (line); };
        const auto coldJumpSeconds { getColdJumpSeconds (coldJumpTargets, elementAtIndex) };
        addReportRow (reportRows, "coldJump", "documentIndex", lineCount, coldJumpSeconds, coldJumpTargets.size());
    }

    return correctnessMatches;
}

static juce::MemoryBlock encodeLineForIndex (const jam::Document::Element& element)
{
    const auto lineBytes { getEncodedLineBytes (*element.get<jam::Document::Cells> (Id::cells)) };
    return juce::MemoryBlock (lineBytes.data(), lineBytes.size());
}

static void decodeLineForIndex (jam::Document::Element& element, const juce::MemoryBlock& wireBytes)
{
    auto pageDocument { jam::AnsiDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (wireBytes.getData()), static_cast<int> (wireBytes.getSize()))) };
    auto* decodedLine { getElementAt (pageDocument, 0) };
    auto* cells { element.get<jam::Document::Cells> (Id::cells) };

    cells->clear();

    for (const auto& cell : *decodedLine->get<jam::Document::Cells> (Id::cells))
        cells->add (cell);
}

static bool isTierCorrect (jam::Document::Index& tierIndex, const jam::AnsiDocument& tierDocument,
    const jam::Array<juce::MemoryBlock>& originalLineBytes, const jam::Array<int>& coldJumpTargets, int lineCount)
{
    static constexpr int rehydratePageLineCount { 50 };
    static constexpr int pageAnchorLineNumber { 0 };

    auto tierCorrect { true };

    for (const auto targetLineNumber : coldJumpTargets)
    {
        if (targetLineNumber >= 0 and targetLineNumber < lineCount)
        {
            auto* element { tierIndex.getElement (targetLineNumber) };
            const auto rehydratedBytes { encodeLineForIndex (*element) };
            const auto bytesMatch { rehydratedBytes == originalLineBytes.at (targetLineNumber) };

            if (not bytesMatch)
                std::cerr << "documentIndex tier rehydrate mismatch at line " << targetLineNumber << std::endl;

            tierCorrect = bytesMatch and tierCorrect;
        }
    }

    const auto pageLastLineNumber { juce::jmin (pageAnchorLineNumber + rehydratePageLineCount - 1, lineCount - 1) };

    for (int pageLineNumber { pageAnchorLineNumber }; pageLineNumber <= pageLastLineNumber; ++pageLineNumber)
    {
        auto* element { tierIndex.getElement (pageLineNumber) };
        const auto pageBytes { encodeLineForIndex (*element) };
        const auto bytesMatch { pageBytes == originalLineBytes.at (pageLineNumber) };

        if (not bytesMatch)
            std::cerr << "documentIndex tier page mismatch at line " << pageLineNumber << std::endl;

        tierCorrect = bytesMatch and tierCorrect;
    }

    juce::int64 recomputedResidentBytes { 0 };

    for (auto* line : tierDocument)
        if (line->isTag (Id::line))
            if (const auto* cells { line->get<jam::Document::Cells> (Id::cells) }; cells->size() > 0)
                recomputedResidentBytes += static_cast<juce::int64> (cells->size()) * static_cast<juce::int64> (sizeof (jam::AttributedChar));

    const auto residentBytesMatch { recomputedResidentBytes == tierIndex.getResidentBytes() };

    if (not residentBytesMatch)
        std::cerr << "documentIndex tier resident bytes mismatch: recomputed " << recomputedResidentBytes
                   << " vs reported " << tierIndex.getResidentBytes() << std::endl;

    return tierCorrect and residentBytesMatch;
}

static bool addDocumentIndexTierRows (int lineCount, const jam::Array<int>& coldJumpTargets, juce::StringArray& reportRows)
{
    static constexpr juce::int64 tierBudgetBytes { 256 * 1024 };

    jam::AnsiDocument tierDocument;
    jam::Array<juce::MemoryBlock> originalLineBytes;

    const jam::Document::Index::Codec codec { encodeLineForIndex, decodeLineForIndex };
    jam::Document::Index tierIndex { tierDocument, codec };
    tierIndex.setBudget (tierBudgetBytes);

    const auto evictStartTime { std::chrono::steady_clock::now() };

    for (int lineNumber { 0 }; lineNumber < lineCount; ++lineNumber)
    {
        auto* newElement { appendGeneratedLine (tierDocument, lineNumber) };
        originalLineBytes.add (encodeLineForIndex (*newElement));
        tierIndex.appendLine (newElement);
    }

    const auto evictSeconds { getElapsedSeconds (evictStartTime) };
    addReportRow (reportRows, "evict", "documentIndex", lineCount, evictSeconds, lineCount);

    const auto rehydrateStartTime { std::chrono::steady_clock::now() };
    const auto correctnessMatches { isTierCorrect (tierIndex, tierDocument, originalLineBytes, coldJumpTargets, lineCount) };
    const auto rehydrateSeconds { getElapsedSeconds (rehydrateStartTime) };

    addReportRow (reportRows, "rehydrate", "documentIndex", lineCount, rehydrateSeconds, coldJumpTargets.size());

    return correctnessMatches;
}

static bool addDocumentIndexInterleaveRows (const jam::Array<int>& coldJumpTargets, juce::StringArray& reportRows)
{
    static constexpr juce::int64 interleaveBudgetBytes { 4 * 1024 };
    static constexpr int interleaveLineCount { 4000 };
    static constexpr int interleaveBatchLineCount { 4 };

    jam::AnsiDocument interleaveDocument;
    jam::Array<juce::MemoryBlock> originalLineBytes;

    const jam::Document::Index::Codec codec { encodeLineForIndex, decodeLineForIndex };
    jam::Document::Index interleaveIndex { interleaveDocument, codec };
    interleaveIndex.setBudget (interleaveBudgetBytes);

    const auto interleaveStartTime { std::chrono::steady_clock::now() };

    for (int lineNumber { 0 }; lineNumber < interleaveLineCount; ++lineNumber)
    {
        auto* newElement { appendGeneratedLine (interleaveDocument, lineNumber) };
        originalLineBytes.add (encodeLineForIndex (*newElement));
        interleaveIndex.appendLine (newElement);

        if ((lineNumber + 1) % interleaveBatchLineCount == 0)
            interleaveIndex.getElement (lineNumber);
    }

    const auto interleaveSeconds { getElapsedSeconds (interleaveStartTime) };
    addReportRow (reportRows, "interleave", "documentIndex", interleaveLineCount, interleaveSeconds, interleaveLineCount);

    jam::Array<int> interleaveTargets;

    for (const auto targetLineNumber : coldJumpTargets)
        if (targetLineNumber >= 0 and targetLineNumber < interleaveLineCount)
            interleaveTargets.add (targetLineNumber);

    return isTierCorrect (interleaveIndex, interleaveDocument, originalLineBytes, interleaveTargets, interleaveLineCount);
}

static int getWrapRowCount (const jam::Array<int>& cellCountOracle, int columns)
{
    auto totalRowCount { 0 };

    for (const auto cellCount : cellCountOracle)
        totalRowCount += juce::jmax (1, (cellCount + columns - 1) / columns);

    return totalRowCount;
}

static int getWrapLineNumber (const jam::Array<int>& cellCountOracle, int columns, int targetRowNumber)
{
    auto rowNumber { 0 };

    for (int lineNumber { 0 }; lineNumber < cellCountOracle.size(); ++lineNumber)
    {
        const auto lineRowCount { juce::jmax (1, (cellCountOracle.at (lineNumber) + columns - 1) / columns) };

        if (targetRowNumber < rowNumber + lineRowCount)
            return lineNumber;

        rowNumber += lineRowCount;
    }

    return cellCountOracle.size() - 1;
}

static bool isWrapCountCorrect (jam::Document::Index& wrapIndex, const jam::Array<int>& cellCountOracle, int columns)
{
    const auto expectedRowCount { getWrapRowCount (cellCountOracle, columns) };
    const auto rowCountMatches { wrapIndex.getRowCount() == expectedRowCount };

    if (not rowCountMatches)
        std::cerr << "wrap rowCount mismatch at columns " << columns << ": expected "
                   << expectedRowCount << " got " << wrapIndex.getRowCount() << std::endl;

    return rowCountMatches;
}

static bool isWrapLineNumberCorrect (jam::Document::Index& wrapIndex, const jam::Array<int>& cellCountOracle,
    int columns, const jam::Array<int>& rowSamples)
{
    auto lineNumberMatches { true };

    for (const auto rowNumber : rowSamples)
    {
        const auto expectedLineNumber { getWrapLineNumber (cellCountOracle, columns, rowNumber) };
        const auto actualLineNumber { wrapIndex.getLineNumber (rowNumber) };

        if (actualLineNumber != expectedLineNumber)
            std::cerr << "wrap getLineNumber mismatch at row " << rowNumber << ": expected "
                       << expectedLineNumber << " got " << actualLineNumber << std::endl;

        lineNumberMatches = (actualLineNumber == expectedLineNumber) and lineNumberMatches;
    }

    return lineNumberMatches;
}

static bool isWrapRoundTripCorrect (jam::Document::Index& wrapIndex, const jam::Array<int>& cellCountOracle,
    int columns, const jam::Array<int>& lineSamples)
{
    auto roundTripMatches { true };

    for (const auto lineNumber : lineSamples)
    {
        const auto rowNumber { wrapIndex.getRowNumber (lineNumber) };
        const auto lineRowCount { juce::jmax (1, (cellCountOracle.at (lineNumber) + columns - 1) / columns) };
        const auto tripMatches { wrapIndex.getLineNumber (rowNumber) == lineNumber
                              and wrapIndex.getLineNumber (rowNumber + lineRowCount - 1) == lineNumber };

        if (not tripMatches)
            std::cerr << "wrap round-trip mismatch at line " << lineNumber << std::endl;

        roundTripMatches = tripMatches and roundTripMatches;
    }

    return roundTripMatches;
}

static bool isWrapCorrect (jam::Document::Index& wrapIndex, const jam::Array<int>& cellCountOracle, int columns,
    const jam::Array<int>& lineSamples, const jam::Array<int>& rowSamples)
{
    const auto countCorrect { isWrapCountCorrect (wrapIndex, cellCountOracle, columns) };
    const auto lineNumberCorrect { isWrapLineNumberCorrect (wrapIndex, cellCountOracle, columns, rowSamples) };
    const auto roundTripCorrect { isWrapRoundTripCorrect (wrapIndex, cellCountOracle, columns, lineSamples) };

    return countCorrect and lineNumberCorrect and roundTripCorrect;
}

static bool isWrapTierCorrect()
{
    static constexpr juce::int64 wrapTierBudgetBytes { 4 * 1024 };
    static constexpr int wrapTierLineCount { 4000 };
    static constexpr int wrapTierBatchLineCount { 4 };

    jam::AnsiDocument wrapTierDocument;
    jam::Array<int> wrapTierCellCountOracle;

    const jam::Document::Index::Codec codec { encodeLineForIndex, decodeLineForIndex };
    jam::Document::Index wrapTierIndex { wrapTierDocument, codec };
    wrapTierIndex.setBudget (wrapTierBudgetBytes);
    wrapTierIndex.setColumns (wrapColumnsNarrow);

    for (int lineNumber { 0 }; lineNumber < wrapTierLineCount; ++lineNumber)
    {
        auto* newElement { appendGeneratedLine (wrapTierDocument, lineNumber) };
        wrapTierCellCountOracle.add (newElement->get<jam::Document::Cells> (Id::cells)->size());
        wrapTierIndex.appendLine (newElement);

        if ((lineNumber + 1) % wrapTierBatchLineCount == 0)
            wrapTierIndex.getElement (lineNumber);
    }

    const auto lineSamples { getWrapSampleTargets (wrapTierLineCount) };
    const auto rowSamples { getWrapSampleTargets (wrapTierIndex.getRowCount()) };

    return isWrapCorrect (wrapTierIndex, wrapTierCellCountOracle, wrapColumnsNarrow, lineSamples, rowSamples);
}

static bool addDocumentIndexWrapRows (int lineCount, juce::StringArray& reportRows)
{
    jam::AnsiDocument wrapDocument;
    jam::Array<int> cellCountOracle;

    for (int lineNumber { 0 }; lineNumber < lineCount; ++lineNumber)
    {
        auto* newElement { appendGeneratedLine (wrapDocument, lineNumber) };
        cellCountOracle.add (newElement->get<jam::Document::Cells> (Id::cells)->size());
    }

    jam::Document::Index wrapIndex { wrapDocument };
    const auto lineSamples { getWrapSampleTargets (lineCount) };

    const auto narrowStartTime { std::chrono::steady_clock::now() };
    wrapIndex.setColumns (wrapColumnsNarrow);
    const auto narrowSeconds { getElapsedSeconds (narrowStartTime) };
    addReportRow (reportRows, "setColumns", "documentIndexWrap" + juce::String (wrapColumnsNarrow), lineCount, narrowSeconds, lineCount);

    const auto narrowCorrect { isWrapCorrect (wrapIndex, cellCountOracle, wrapColumnsNarrow,
        lineSamples, getWrapSampleTargets (wrapIndex.getRowCount())) };

    const auto wideStartTime { std::chrono::steady_clock::now() };
    wrapIndex.setColumns (wrapColumnsWide);
    const auto wideSeconds { getElapsedSeconds (wideStartTime) };
    addReportRow (reportRows, "setColumns", "documentIndexWrap" + juce::String (wrapColumnsWide), lineCount, wideSeconds, lineCount);

    const auto wideCorrect { isWrapCorrect (wrapIndex, cellCountOracle, wrapColumnsWide,
        lineSamples, getWrapSampleTargets (wrapIndex.getRowCount())) };

    const auto tierCorrect { isWrapTierCorrect() };

    return narrowCorrect and wideCorrect and tierCorrect;
}

static bool addWorkloadRows (jam::AnsiDocument& appendDocument, double appendElapsedSeconds, int lineCount, juce::StringArray& reportRows)
{
    const auto populationPassed { addPopulationRows (appendDocument, appendElapsedSeconds, lineCount, reportRows) };

    const auto midLineNumber { lineCount / 2 };
    constexpr int halfPageLineCount { 25 };
    constexpr int scrollMoveCount { 1000 };

    const auto coldJumpTargets { getColdJumpTargets (lineCount) };
    const auto scrollByOneTargets { getScrollTargets (midLineNumber, 1, scrollMoveCount) };
    const auto scrollByHalfPageTargets { getScrollTargets (midLineNumber, halfPageLineCount, scrollMoveCount) };

    addBaselineRows (appendDocument, coldJumpTargets, midLineNumber, lineCount, reportRows);

    const auto candidatesPassed { addAllCandidateRows (appendDocument, midLineNumber, lineCount,
        coldJumpTargets, scrollByOneTargets, scrollByHalfPageTargets, reportRows) };

    const auto documentIndexPassed { addDocumentIndexRows (appendDocument, coldJumpTargets, lineCount, reportRows) };

    const auto wrapPassed { addDocumentIndexWrapRows (lineCount, reportRows) };

    const auto spillPassed { addSpillRows (appendDocument, coldJumpTargets, lineCount, reportRows) };

    static constexpr int tierValidationLineCount { 100000 };
    auto tierPassed { true };

    if (lineCount == tierValidationLineCount)
    {
        tierPassed = addDocumentIndexTierRows (lineCount, coldJumpTargets, reportRows);
        tierPassed = addDocumentIndexInterleaveRows (coldJumpTargets, reportRows) and tierPassed;
    }

    return populationPassed and candidatesPassed and documentIndexPassed and wrapPassed and spillPassed and tierPassed;
}

static juce::String getReport (const juce::StringArray& reportRows, int largestLineCount, size_t residentAtLargest)
{
    const juce::String residentText { residentAtLargest == 0
        ? juce::String ("unavailable")
        : juce::String (static_cast<juce::int64> (residentAtLargest)) + " bytes" };

    juce::StringArray tableLines;
    tableLines.add ("machine: " + juce::SystemStats::getCpuModel() + ", "
        + juce::String (juce::SystemStats::getMemorySizeInMegabytes()) + " MB RAM");
    tableLines.add ("build: " + juce::String (BUFFER_BENCH_COMPILER_FLAGS));
    tableLines.add ("resident at largest completed N (" + juce::String (largestLineCount) + " lines): " + residentText);
    tableLines.add ("");
    tableLines.add ("| workload | contender | lines | elapsedSeconds | operationsPerSecond |");
    tableLines.add ("|---|---|---|---|---|");
    tableLines.addArray (reportRows);

    return tableLines.joinIntoString ("\n") + "\n";
}

static bool writeResults (const juce::String& resultsPath, const juce::String& report)
{
    const juce::File resultsFile { resultsPath };
    const auto directoryCreated { resultsFile.getParentDirectory().createDirectory().wasOk() };
    const auto fileWritten { resultsFile.replaceWithText (report) };
    const auto artifactWritten { directoryCreated and fileWritten };

    if (not artifactWritten)
        std::cerr << "failed to write results artifact at " << resultsPath << std::endl;

    return artifactWritten;
}

int main (int argumentCount, char* argumentValues[])
{
    jam::Stamp stamp;
    jam::Grapheme grapheme;
    jam::Hyperlink hyperlink;

    constexpr size_t memoryCeilingBytes { static_cast<size_t> (8) * 1024 * 1024 * 1024 };
    constexpr std::array<int, 4> lineCountSteps {{ 10000, 100000, 1000000, 10000000 }};

    juce::StringArray reportRows;
    auto allPassed { true };
    auto largestLineCount { 0 };
    size_t residentAtLargest { 0 };

    for (const auto lineCount : lineCountSteps)
    {
        jam::AnsiDocument appendDocument;
        const auto appendElapsedSeconds { addLines (appendDocument, lineCount) };
        const auto residentBytes { getResidentBytes() };

        if (residentBytes >= memoryCeilingBytes)
        {
            std::cout << "stopping growth at " << lineCount << " lines -- resident "
                       << residentBytes << " bytes approaches ceiling "
                       << memoryCeilingBytes << " bytes" << std::endl;
            break;
        }

        allPassed = addWorkloadRows (appendDocument, appendElapsedSeconds, lineCount, reportRows) and allPassed;
        largestLineCount = lineCount;
        residentAtLargest = residentBytes;
    }

    const auto report { getReport (reportRows, largestLineCount, residentAtLargest) };
    std::cout << report;

    const juce::String resultsPath { argumentCount > 1
        ? juce::String (argumentValues[1])
        : juce::String (BUFFER_BENCH_RESULTS_PATH) };

    const auto artifactWritten { writeResults (resultsPath, report) };

    return (allPassed and artifactWritten) ? 0 : 1;
}
