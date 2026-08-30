#include "ENDView.h"

ENDView::ENDView (jam::AudioModel& newModel, jam::PluginEditorLayout& newLayout, juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
{
    if (layout.isReady (model))
    {
        initialise();

        setResizable (false, false);

        const auto [width, height] { jam::ViewManager::getUISize (layout.getMaster()) };
        setSize (width, height);
    }
}

void ENDView::initialiseTheme()
{
    styleManager.create (layout.fonts);

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    juce::LookAndFeel::setDefaultLookAndFeel (theme.get());
}

void ENDView::initialiseView()
{
    auto newView { std::make_unique<jam::ViewEditor>() };

    jam::ViewManager::buildView (layout.getMaster(), *newView);
    addAndMakeVisible (*newView);

    view = std::move (newView);
}
