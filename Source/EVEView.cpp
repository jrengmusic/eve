#include "EVEView.h"
#include "EVEProcessor.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
{
    initialise (layout.getOrCreateSettings());
    setScaling();
    juce::MessageManager::callAsync ([this] { grabKeyboardFocus(); });
}

void EVEView::focusGained (FocusChangeType)
{
    if (auto traverser { createKeyboardFocusTraverser() })
    {
        if (auto* defaultComponent { traverser->getDefaultComponent (this) })
            defaultComponent->grabKeyboardFocus();
    }
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
    auto& terminalProcessor { processor.getTerminalProcessor() };

    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, *theme, terminalProcessor.userInterfaceGetters, terminalProcessor.chainEvents, files::viewLayout);
    addAndMakeVisible (view.get());
}

void EVEView::paintOverChildren (juce::Graphics& g) { transition.paint (g, getLocalBounds()); }

void EVEView::resized()
{
    jam::PluginEditor::resized();

    const auto area { jam::ViewManager::getComponentBounds (*view).toFloat() };
    const auto metrics { theme->getOrCreateMonoMetrics() };
    const auto cells { jam::Cell::Rectangle::fromPixel (area.withZeroOrigin(), metrics.cellWidth, metrics.cellHeight) };
    const auto* display { juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds()) };

    jassert (display != nullptr);

    const auto pixels { (area * (juce::Component::getApproximateScaleFactorForComponent (view.get()) * static_cast<float> (display->scale))).getSmallestIntegerContainer() };

    model.getValue (Id::toTag (Id::winsize), Id::value).setValue (juce::var { jam::TerminalWinsize { cells.width, cells.height, pixels.getWidth(), pixels.getHeight() }.pack() });
}

void EVEView::setWinsize()
{
    auto& processor { static_cast<EVEProcessor&> (*getAudioProcessor()) };

    processor.setWinsize (jam::TerminalWinsize::unpack (static_cast<juce::int64> (model.getValue (Id::toTag (Id::winsize), Id::value).getValue())));
}
