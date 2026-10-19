#include "EVEView.h"
#include "EVEProcessor.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
{
    initialise (layout.getOrCreateSettings());
    setScaling();
}

void EVEView::initialiseTheme (const juce::ValueTree& settings)
{
    styleManager.create (layout.fonts, settings);

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

void EVEView::initialiseView()
{
    auto& processor { static_cast<EVEProcessor&> (*getAudioProcessor()) };
    auto& audioProcessor { processor.getAudioProcessor() };

    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, *theme, audioProcessor.userInterfaceGetters, audioProcessor.chainEvents, files::viewLayout);
    addAndMakeVisible (view.get());
}

void EVEView::paintOverChildren (juce::Graphics& g) { transition.paint (g, getLocalBounds()); }
