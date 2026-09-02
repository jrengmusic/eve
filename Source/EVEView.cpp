#include "EVEView.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
{
    jassert (layout.isReady (model));

    initialise();

    setResizable (false, false);

    const auto [width, height] { jam::ViewManager::getUISize (layout.getMaster()) };
    setSize (width, height);
}

EVEView::~EVEView()
{
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void EVEView::initialiseTheme()
{
    styleManager.create (layout.fonts);

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    juce::LookAndFeel::setDefaultLookAndFeel (theme.get());
}

void EVEView::initialiseView()
{
    auto newView { std::make_unique<jam::ViewEditor>() };

    jam::ViewManager::buildView (layout.getMaster(), *newView);
    addAndMakeVisible (*newView);

    view = std::move (newView);
}
