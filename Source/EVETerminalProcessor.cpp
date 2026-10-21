#include "EVETerminalProcessor.h"

EVETerminalProcessor::EVETerminalProcessor (juce::AudioProcessor& processorToUse, jam::AudioModel& modelToUse)
    : processor (processorToUse),
      model (modelToUse)
{
    registerUserInterfaceGetters();
}

void EVETerminalProcessor::registerUserInterfaceGetters()
{
    userInterfaceGetters[Id::textEditor].add<jam::AnsiDocument&, std::unique_ptr<jam::AnsiDocument>&> (
        Id::toType (Id::winsize),
        [this] (jam::AnsiDocument& historyDocument, std::unique_ptr<jam::AnsiDocument>& liveDocument)
        {
            terminalProcessor.pull (historyDocument, liveDocument);
        });
}

void EVETerminalProcessor::prepare (const juce::dsp::ProcessSpec& newSpec)
{
    spec = newSpec;
    const auto winsize { jam::TerminalWinsize::unpack (pendingWinsize.load()) };

    const auto historyRows { static_cast<int> (model.getValue (Id::toTag (Id::historyCapacity), Id::value).getValue()) };
    jassert (historyRows > 0);

    const auto liveCapacity { jam::terminal::GraphicsEngine::getNumBytes (winsize) };

    terminalProcessor.prepare (liveCapacity + engine.getNumBytes() + jam::terminal::GraphicsEngine::getNumBytes (jam::TerminalWinsize { winsize.cols, historyRows }), liveCapacity);
}

void EVETerminalProcessor::reset() noexcept
{
    terminalProcessor.reset();
}

void EVETerminalProcessor::process() noexcept
{
    const auto winsize { jam::TerminalWinsize::unpack (pendingWinsize.load()) };

    terminalProcessor.process (engine, winsize.rows);
}

void EVETerminalProcessor::setWinsize (jam::TerminalWinsize newWinsize)
{
    pendingWinsize.store (newWinsize.pack());
}

void EVETerminalProcessor::load (const juce::File& harness)
{
    const jam::GlyphArrangement glyphArrangement { jam::terminal::GraphicsEngine::getCellLookup(), jam::terminal::GraphicsEngine::getCellMeasure() };
    const juce::MemoryMappedFile mapped { harness, juce::MemoryMappedFile::readOnly };

    jassert (mapped.getData() != nullptr);

    jam::StyleMarkdown styleMarkdown { Id::toType (Id::style), model.getAppearance() };

    const auto monoMetrics { styleMarkdown.getOrCreateMonoMetrics() };

    styleMarkdown.setPixelCellSize (monoMetrics.cellWidth, monoMetrics.cellHeight);

    auto markdown { jam::MarkdownDocument::parse (std::string_view { static_cast<const char*> (mapped.getData()), mapped.getSize() }, harness.getFullPathName()) };

    markdown.addLines();

    processor.suspendProcessing (true);

    {
        static constexpr int gridColumns { 0 };

        engine.prepare (gridColumns, getNumRows (markdown, styleMarkdown));

        auto writer { engine.write() };
        int row { 0 };

        engine.reset (writer);

        for (const auto* line : *markdown.getRoot())
        {
            if (line->isTag (Id::line))
                row = addRows (writer, *line, row, styleMarkdown);
        }
    }

    prepare (spec);

    processor.suspendProcessing (false);
}

int EVETerminalProcessor::getNumRows (const jam::Document::Element& line, const jam::StyleMarkdown& style)
{
    if (jam::StyleMarkdown::getType (line) == map::BlockType::document) return jam::toInt (style.getGapHeight (line));

    const auto padding { style.getLinePadding (line) };

    return 1 + jam::toInt (padding.getTop()) + jam::toInt (padding.getBottom());
}

int EVETerminalProcessor::getNumRows (const jam::MarkdownDocument& markdown, const jam::StyleMarkdown& style)
{
    int rows { 0 };

    for (const auto* line : *markdown.getRoot())
    {
        if (line->isTag (Id::line))
            rows += getNumRows (*line, style);
    }

    return rows;
}

int EVETerminalProcessor::addRows (const juce::AbstractFifo::ScopedWrite& writer, const jam::Document::Element& line, int row, const jam::StyleMarkdown& style)
{
    const auto numRows { getNumRows (line, style) };

    if (jam::StyleMarkdown::getType (line) != map::BlockType::document)
    {
        const auto top { jam::toInt (style.getLinePadding (line).getTop()) };

        for (int index { 0 }; index < numRows; ++index)
        {
            if (index == top)
                addRow (writer, row + index, line, style);
            else
                addMarkers (writer, row + index, line, style);
        }
    }

    return row + numRows;
}

void EVETerminalProcessor::addRow (const juce::AbstractFifo::ScopedWrite& writer, int row, const jam::Document::Element& line, const jam::StyleMarkdown& style)
{
    if (line.contains (Id::key))
        addCells (writer, row, 0, *line.get<jam::AttributedString> (Id::key), jam::Document::Elements {}, style);

    int column { addCells (writer, row, jam::toInt (style.getLinePadding (line).getLeft()), *line.get<jam::AttributedString> (Id::cells), *line.get<jam::Document::Elements> (Id::images), style) };

    for (const auto* child : line)
    {
        juce::MemoryOutputStream stream;
        jam::terminal::appendColumnStart (stream, *child->get<double> (Id::left), *child->get<double> (Id::width));
        engine.addMarkerPlacement (writer, jam::Cell { row }, column, stream.getMemoryBlock());
        column = addCells (writer, row, column, *child->get<jam::AttributedString> (Id::cells), *child->get<jam::Document::Elements> (Id::images), style);
    }

    addMarkers (writer, row, line, style);
}

void EVETerminalProcessor::setAtom (jam::AttributedString& targetRow, int targetColumn, const jam::AttributedString& cells, int index, const jam::StyleMarkdown& style)
{
    const auto& pills { jam::MarkdownGraphics::getPills() };
    auto atom { cells.at (index) };
    const auto pill { jam::MarkdownGraphics::getPill (cells, std::next (cells.begin(), index)) };
    auto colourId { cells.getColourId (static_cast<int> (atom.colourId)) };

    atom.colourId = static_cast<uint64_t> (targetRow.addColourId (engine.getOrCreateColourId (jam::colours::getColour (style, colourId, juce::Colour {}))));
    colourId = pills.contains (pill) ? std::get<0> (pills.at (pill)) : cells.getColourId (static_cast<int> (atom.bgColourId));
    atom.bgColourId = static_cast<uint64_t> (targetRow.addColourId (engine.getOrCreateColourId (jam::colours::getColour (style, colourId, juce::Colour {}))));
    atom.fontId = jam::AttributedChar {}.fontId;

    if (atom.contentTag == jam::AttributedChar::contentGrapheme)
        targetRow.set (targetColumn, atom, cells.getCluster (static_cast<int> (atom.codepoint)));
    else
        targetRow.set (targetColumn, atom);
}

int EVETerminalProcessor::addCells (const juce::AbstractFifo::ScopedWrite& writer, int row, int column, const jam::AttributedString& cells, const jam::Document::Elements& images, const jam::StyleMarkdown& style)
{
    auto image { images.begin() };
    auto& targetRow { engine.getRow (writer, jam::Cell { row }) };
    int targetColumn { column };

    targetRow.resize (column + cells.size() - getNumObjects (cells, cells.size()));

    for (int index { 0 }; index < cells.size(); ++index)
    {
        if (jam::MarkdownDocument::isObject (cells.at (index)))
        {
            addImage (writer, row, targetColumn, **image++, style);
        }
        else
        {
            setAtom (targetRow, targetColumn++, cells, index, style);
        }
    }

    for (const auto& [range, ordinal] : cells.getLinkRuns())
        targetRow.addLink ({ range.getStart() + column - getNumObjects (cells, range.getStart()), range.getEnd() + column - getNumObjects (cells, range.getEnd()) }, cells.getLinks().at (ordinal));

    return targetColumn;
}

void EVETerminalProcessor::addImage (const juce::AbstractFifo::ScopedWrite& writer, int row, int targetColumn, const jam::Document::Element& element, const jam::StyleMarkdown& style)
{
    jassert (not jam::MarkdownDocument::isVectorImage (element));

    const auto picture { element.get<int> (Id::type, map::BlockType::paragraph) == map::BlockType::mermaid ? getDiagramImage (element, style) : jam::MarkdownDocument::getImage (element) };
    const auto cellBounds { style.getCellBounds (picture.getBounds().toFloat()) };

    juce::MemoryOutputStream stream;
    jam::terminal::appendInlineImage (stream, jam::ImageLoader::getPng (picture), jam::toInt (cellBounds.getWidth()), jam::toInt (cellBounds.getHeight()));
    engine.addMarkerPlacement (writer, jam::Cell { row }, targetColumn, stream.getMemoryBlock());
}

juce::Image EVETerminalProcessor::getDiagramImage (const jam::Document::Element& element, const jam::StyleMarkdown& style)
{
    const jam::GlyphArrangement glyphArrangement { jam::GlyphArrangement::getFontLookup(), jam::GlyphArrangement::getFontMeasure() };
    const auto diagram { jam::MermaidDiagram::parse (jam::Strings::getView (*element.firstChild->get<juce::String> (Id::text)), style) };
    const jam::MermaidGraphics graphics { diagram, style };

    return graphics.getImage (style.getPixelBounds (style.getCellBounds (graphics.bounds)), style);
}

int EVETerminalProcessor::getNumObjects (const jam::AttributedString& cells, int end)
{
    return static_cast<int> (std::count_if (cells.begin(), std::next (cells.begin(), end), jam::MarkdownDocument::isObject));
}

void EVETerminalProcessor::addMarkers (const juce::AbstractFifo::ScopedWrite& writer, int row, const jam::Document::Element& line, const jam::StyleMarkdown& style)
{
    juce::MemoryOutputStream stream;

    jam::terminal::appendLineStart (stream, jam::toInt (style.getLineIndent (line)));

    if (jam::MarkdownGraphics::hasFill (line))
    {
        const auto& [colourId, heightMetric] { jam::MarkdownGraphics::getFills().at (jam::StyleMarkdown::getType (line)) };
        const auto fillColourId { engine.getOrCreateColourId (jam::colours::getColour (style, colourId, juce::Colour {})) };

        if (fillColourId != jam::AttributedChar::colourIdNone)
            jam::terminal::appendFill (stream, 0.0, 1.0, heightMetric.has_value() ? style.getMetrics<float> (*heightMetric) / style.getLineMetrics (line).cellHeight : 1.0f, engine.getColour (fillColourId));
    }

    engine.addMarkerPlacement (writer, jam::Cell { row }, 0, stream.getMemoryBlock());
}
