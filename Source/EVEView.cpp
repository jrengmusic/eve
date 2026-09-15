#include "EVEView.h"
#include "EVEProcessor.h"

static const juce::Identifier terminalTableId { "terminal" };
static const juce::Identifier scrollbackBudgetMbRowId { "scrollbackBudgetMb" };

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
            constexpr juce::int64 bytesPerMegabyte { 1024 * 1024 };

            for (auto* table : viewLayoutDocument.getTables (terminalTableId))
                if (auto* row { viewLayoutDocument.getTableRow (*table, scrollbackBudgetMbRowId) })
                {
                    const auto scrollbackBudgetMegabytes { jam::Format::getNumber (viewLayoutDocument.getTableValueView (*row, Id::value)) };
                    jassert (scrollbackBudgetMegabytes > 0);

                    if (scrollbackBudgetMegabytes > 0)
                        documentIndex->setBudget (scrollbackBudgetMegabytes * bytesPerMegabyte);
                }

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
              [] (jam::Document::Element& element, const juce::MemoryBlock& wireBytes)
              {
                  const auto lineDocument { jam::AnsiDocument::parse (juce::String::fromUTF8 (
                      static_cast<const char*> (wireBytes.getData()), static_cast<int> (wireBytes.getSize()))) };
                  auto* decodedLine { lineDocument.root->firstChild };
                  auto* cells { element.get<jam::Document::Cells> (Id::cells) };

                  if (decodedLine != nullptr)
                      *cells = std::move (*decodedLine->get<jam::Document::Cells> (Id::cells));
                  else
                      *cells = jam::Document::Cells {};
              } };
}

void EVEView::resized()
{
    jam::PluginEditor::resized();

    terminalView.setBounds (getLocalBounds());
}
