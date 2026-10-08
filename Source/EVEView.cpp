#include "EVEView.h"
#include "EVEProcessor.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
{
    if (layout.isReady (model))
    {
        initialise();

        setResizable (false, false);

        const auto [width, height] { view->getUISize (model) };
        setSize (width, height);
    }
}

void EVEView::fileChanged (const juce::File& file, jam::File::Watcher::Event event)
{
    auto* param { jam::ParameterManager::getInstance() };

    if (file == param->getUserSettings()
        and (event == jam::File::Watcher::fileUpdated or event == jam::File::Watcher::fileRenamedNewName))
    {
        resized();
    }
}

void EVEView::initialiseTheme()
{
    const auto file { jam::ParameterManager::getInstance()->getUserSettings() };
    const auto document { jam::ConfigDocument::parse (file.loadFileAsString(), file.getFullPathName()) };

    styleManager.create (layout.fonts,
                         document.getValueTree (Id::toType (Id::config)),
                         document.getValueTree (Id::toType (Id::config), Id::dark));

    styleManager->registerStyle (files::markdownStyleSheet);
    styleManager->registerStyle (files::mermaidStyleSheet);

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    juce::LookAndFeel::setDefaultLookAndFeel (theme.get());
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

    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, audioProcessor.userInterfaceGetters, audioProcessor.chainEvents, files::viewLayout);
    addAndMakeVisible (view.get());
}

void EVEView::paintOverChildren (juce::Graphics& g) { transition.paint (g, getLocalBounds()); }
