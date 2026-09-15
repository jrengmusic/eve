#include "EVEView.h"
#include "EVEProcessor.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout, jam::ViewManager::getUISize<jam::MarkdownDocument> (juce::Identifier { files::viewLayout }))
{
    if (layout.isReady (model))
    {
        initialise();

        setResizable (false, false);

        const auto [width, height] { view->getUISize (model) };
        setSize (width, height);
    }
}

void EVEView::initialiseTheme()
{
    styleManager.create (layout.fonts);

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    juce::LookAndFeel::setDefaultLookAndFeel (theme.get());
}

void EVEView::initialiseRegistry() {}

void EVEView::initialisePanels() {}

void EVEView::initialiseView()
{
    auto& processor { static_cast<EVEProcessor&> (*getAudioProcessor()) };
    auto& audioProcessor { processor.getAudioProcessor() };

    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, audioProcessor.userInterfaceGetters, audioProcessor.chainEvents, files::viewLayout);
    addAndMakeVisible (view.get());

    for (const auto& parameter : juce::JUCEApplicationBase::getCommandLineParameterArray())
    {
        const auto fixtureFile { juce::File::getCurrentWorkingDirectory().getChildFile (parameter) };

        if (fixtureFile.existsAsFile())
        {
            ansiDocument = jam::AnsiDocument::parse (fixtureFile.loadFileAsString());
            documentIndex = std::make_unique<jam::Document::Index> (ansiDocument, getCodec());

            const auto& viewLayoutDocument { jam::MarkdownDocument::getOrCreate (juce::Identifier { files::viewLayout }) };
            const juce::Identifier terminalTableId { "terminal" };
            const juce::Identifier scrollbackBudgetMbRowId { "scrollback_budget_mb" };
            constexpr juce::int64 bytesPerMegabyte { 1024 * 1024 };

            for (auto* table : viewLayoutDocument.getTables())
                if (table->id == terminalTableId)
                    for (auto* row : viewLayoutDocument.getTableRows (*table))
                        if (row->id == scrollbackBudgetMbRowId)
                            if (const auto scrollbackBudgetMegabytes { jam::Format::getNumber (viewLayoutDocument.getTableValueView (*row, Id::value)) };
                                scrollbackBudgetMegabytes > 0)
                                documentIndex->setBudget (scrollbackBudgetMegabytes * bytesPerMegabyte);

            break;
        }
    }

    addAndMakeVisible (terminalView);
}

void EVEView::attachPanelCallbacks() {}

void EVEView::initialiseListeners() {}

jam::Document::Index::Codec EVEView::getCodec()
{
    return { [] (const jam::Document::Element& element) -> juce::MemoryBlock
              {
                  const auto* cells { element.get<jam::Document::Cells> (Id::cells) };
                  const auto rowText { jam::terminal::getRowText (cells->data(), cells->size()) };

                  return juce::MemoryBlock (rowText.data(), rowText.size());
              },
              [] (jam::Document::Element& element, const void* wireBytes, size_t byteLength)
              {
                  const auto lineDocument { jam::AnsiDocument::parse (juce::String::fromUTF8 (
                      static_cast<const char*> (wireBytes), static_cast<int> (byteLength))) };
                  auto* decodedLine { lineDocument.root->firstChild };
                  jassert (decodedLine != nullptr);

                  auto* cells { element.get<jam::Document::Cells> (Id::cells) };
                  cells->clear();

                  for (const auto& cell : *decodedLine->get<jam::Document::Cells> (Id::cells))
                      cells->add (cell);
              } };
}

void EVEView::resized()
{
    jam::PluginEditor::resized();

    terminalView.setBounds (getLocalBounds());
}
