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
            break;
        }
    }

    addAndMakeVisible (terminalView);
}

void EVEView::attachPanelCallbacks() {}

void EVEView::initialiseListeners() {}

void EVEView::resized()
{
    jam::PluginEditor::resized();

    terminalView.setBounds (getLocalBounds());
}
