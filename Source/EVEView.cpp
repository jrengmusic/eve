#include "EVEView.h"
#include "EVEProcessor.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
{
    initialise (layout.getOrCreateConfig());
    setScaling();
}

void EVEView::initialiseTheme (const juce::ValueTree& config)
{
    styleManager.create (layout.fonts, config);

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    setLookAndFeel (theme.get());
}

void EVEView::initialiseRegistry()
{
    jam::Registry::Registration registration;

    registerButtons (registration);
    registerViewComponents (registration);

    registry.create (registration);
}

void EVEView::initialisePanels()
{
    panel = jam::ViewPanel::create (model, jam::HtmlDocument::getOrCreate (juce::Identifier { files::panelLayout }));
    addAndMakeVisible (panel.get());
}

void EVEView::initialiseView()
{
    auto& processor { static_cast<EVEProcessor&> (*getAudioProcessor()) };
    auto& audioProcessor { processor.getAudioProcessor() };

    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, *theme, audioProcessor.userInterfaceGetters, audioProcessor.chainEvents, files::viewLayout);
    addAndMakeVisible (view.get());
}

void EVEView::paintOverChildren (juce::Graphics& g) { transition.paint (g, getLocalBounds()); }
