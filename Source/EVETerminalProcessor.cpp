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

    terminalProcessor.prepare (liveCapacity + jam::terminal::GraphicsEngine::getNumBytes (jam::TerminalWinsize { engine.getBounds().width, historyRows }), liveCapacity);
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

    const auto columnWidth { styleMarkdown.getColumnWidth() };

    jam::MarkdownComponent component;
    component.setLookAndFeel (&styleMarkdown);
    component.setBounds (0, 0, columnWidth, 0);
    component.setDocument (jam::MarkdownDocument::parse (std::string_view { static_cast<const char*> (mapped.getData()), mapped.getSize() }, harness.getFullPathName()));

    const auto rows { component.getViewedComponent()->getHeight() };

    processor.suspendProcessing (true);

    {
        engine.prepare (columnWidth, rows);

        jam::terminal::GraphicsContext context { engine };
        juce::Graphics g { context };
        component.getViewedComponent()->paintEntireComponent (g, false);
    }

    prepare (spec);

    processor.suspendProcessing (false);
}
